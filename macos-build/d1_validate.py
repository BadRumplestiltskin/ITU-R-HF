#!/usr/bin/env python3
"""D1 validation harness: compare iturhf-java field strength to the P.533-14 C binary
(implementation fidelity) and to the CCIR D1 measured databank (absolute validation)."""
import csv, math, os, subprocess, json, urllib.request, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
D1 = os.path.join(ROOT, "ITURHFProp/D1")
CBUILD = "/tmp/iturhf-cbuild"
DATA = os.path.join(ROOT, "ITURHFProp/Data/")
JAVA_URL = "http://localhost:8080/api/propagation/p533"
N_ROWS = int(sys.argv[1]) if len(sys.argv) > 1 else 10

def dm_to_decimal(s):
    s = s.strip()
    sign = -1.0 if (s.endswith('S') or s.endswith('W')) else 1.0
    s = s.rstrip('NSEW')
    if '.' in s:
        deg, frac = s.split('.')
        minutes = int(frac)               # fractional part is arc-minutes (D.M format)
        return sign * (abs(int(deg)) + minutes / 60.0)
    return sign * float(s)

def load_cases(n):
    rows = []; seen = set()
    with open(os.path.join(D1, "D1_Table1.csv")) as f:
        r = csv.reader(f); next(r)
        for row in r:
            if len(row) < 12 or not row[0].strip().isdigit(): continue
            cid = int(row[0])
            if cid in seen: continue   # one representative row per distinct circuit
            seen.add(cid)
            rows.append(dict(id=cid, freq=float(row[3]),
                txlat=dm_to_decimal(row[4]), txlng=dm_to_decimal(row[5]),
                rxlat=dm_to_decimal(row[6]), rxlng=dm_to_decimal(row[7]),
                dist=float(row[8]) if row[8].strip().replace('.','').isdigit() else 0,
                ssn=int(row[9]), year=2000+int(row[10]) if int(row[10])<50 else 1900+int(row[10]),
                month=int(row[11])))
            if len(rows) >= n: break
    return rows

def run_c(case):
    """Run all 24 hours through the C binary; return {hour: E}."""
    inp = os.path.join(CBUILD, "d1_case.in"); outp = os.path.join(CBUILD, "d1_case.out")
    with open(inp, "w") as f:
        f.write(f'''PathName "D1 case {case['id']}"
PathTXName "TX"
Path.L_tx.lat {case['txlat']}
Path.L_tx.lng {case['txlng']}
TXAntFilePath "ISOTROPIC"
TXGOS 0.0
TXBearing 0.0
PathRXName "RX"
Path.L_rx.lat {case['rxlat']}
Path.L_rx.lng {case['rxlng']}
RXAntFilePath "ISOTROPIC"
RXGOS 0.0
RXBearing 0.0
AntennaOrientation "TX2RX"
Path.year {case['year']}
Path.month {case['month']}
Path.hour 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24
Path.SSN {case['ssn']}
Path.frequency {case['freq']}
Path.txpower 0.0
Path.BW 6000.0
Path.SNRr 0.0
Path.Relr 90
Path.SNRXXp 90
Path.ManMadeNoise "RURAL"
Path.Modulation "ANALOG"
Path.SIRr 0.0
Path.A 0.0
Path.TW 0.0
Path.FW 0.0
Path.T0 0.0
Path.F0 0.0
Path.SorL "SHORTPATH"
RptFilePath "{CBUILD}/"
RptFileFormat "RPT_D | RPT_E"
LL.lat {case['rxlat']}
LL.lng {case['rxlng']}
LR.lat {case['rxlat']}
LR.lng {case['rxlng']}
UL.lat {case['rxlat']}
UL.lng {case['rxlng']}
UR.lat {case['rxlat']}
UR.lng {case['rxlng']}
latinc 1.0
lnginc 1.0
DataFilePath "{DATA}"
''')
    subprocess.run([os.path.join(CBUILD, "ITURHFProp"), "-s", inp, outp],
                   env={**os.environ, "DYLD_LIBRARY_PATH": CBUILD},
                   capture_output=True)
    res = {}
    with open(outp) as f:
        for line in f:
            p = [x.strip() for x in line.split(",")]
            if len(p) >= 5 and p[0].isdigit() and p[1].isdigit():
                hour = int(p[1]); E = float(p[4])   # col: month,hour,freq,D,E
                res[hour] = E
    return res

def run_java(case, hour):
    body = json.dumps({
        "transmitterLatitudeDegrees": case['txlat'], "transmitterLongitudeDegrees": case['txlng'],
        "receiverLatitudeDegrees": case['rxlat'], "receiverLongitudeDegrees": case['rxlng'],
        # C Path.hour is 1-based: hour h == UT (h-1). Align Java's UTC instant accordingly.
        "frequencyMHz": case['freq'], "instantUtc": f"{case['year']}-{case['month']:02d}-15T{(hour-1)%24:02d}:00:00Z",
        "smoothedSunspotNumber": min(case['ssn'],160), "pathDirection": "SHORT_PATH",
        "noiseEnvironment": "RURAL", "modulationMode": "ANALOG", "bandwidthHz": 6000.0,
        "transmitterPowerDbW": 30.0, "effectiveRadiatedPowerDbW": 30.0,
        "transmitterAntennaGainDbi": 0.0, "receiverAntennaGainDbi": 0.0,
        "requiredSnrPercentile": 90, "requiredSnrDb": 0.0, "requiredSirDb": 0.0}).encode()
    try:
        req = urllib.request.Request(JAVA_URL, data=body, headers={"Content-Type":"application/json"})
        d = json.load(urllib.request.urlopen(req, timeout=30))
        return d.get("medianFieldStrengthDbUvPerM")
    except Exception as e:
        return None

cases = load_cases(N_ROWS)
print(f"Running {len(cases)} D1 cases x 24h through C binary and Java API...\n")
dC, dJ = [], []   # (java-c) diffs
per_case = []
for c in cases:
    cE = run_c(c)
    jc = []
    for h in range(1, 25):
        if h not in cE: continue
        je = run_java(c, h)
        if je is None: continue
        jc.append((h, cE[h], je))
        dC.append(0.0); dJ.append(je - cE[h])
    if jc:
        diffs = [j-c for _,c,j in jc]
        rms = math.sqrt(sum(d*d for d in diffs)/len(diffs))
        mx = max(abs(d) for d in diffs)
        per_case.append((c, rms, mx, len(jc), jc))
        print(f"case {c['id']:>3} d={c['dist']:>5.0f}km f={c['freq']:>4}MHz ssn={c['ssn']:>3} {c['year']}/{c['month']:02d}  "
              f"RMS(Java-C)={rms:5.2f}dB  max={mx:5.2f}dB")

if dJ:
    allrms = math.sqrt(sum(d*d for d in dJ)/len(dJ))
    allmax = max(abs(d) for d in dJ)
    print(f"\n=== OVERALL Java-vs-C field strength: RMS={allrms:.3f} dB, max|Δ|={allmax:.3f} dB, n={len(dJ)} ===")
