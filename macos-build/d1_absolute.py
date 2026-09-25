#!/usr/bin/env python3
"""Absolute D1 validation: predicted vs MEASURED median sky-wave field strength.

Unlike d1_validate.py -- which despite its docstring only compares the C engine
against a Java service and never reads a measured value -- this joins the CCIR
D1 measured field strengths in D1_Table2.csv to the circuit geometry in
D1_Table1.csv and reports the error of the C engine against them.

Measurements are dB(1 uV/m) normalised for 1 kW EIRP, so the circuit is run at
0 dB(1kW) into isotropic antennas. 99 marks a missing hour.

Usage: d1_absolute.py [n_cases] [path_to_ITURHFProp] [path_to_libdir]
"""
import csv, math, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
D1   = os.path.join(ROOT, "ITURHFProp/D1")
DATA = os.path.join(ROOT, "ITURHFProp/Data/")
EXE   = os.path.join(ROOT, "ITURHFProp/Linux/ITURHFProp")
LIBS  = os.path.join(ROOT, "P533/Linux") + ":" + os.path.join(ROOT, "P372/Linux")

def dm(s):
    s = s.strip(); sign = -1.0 if s[-1] in "SW" else 1.0; s = s.rstrip("NSEW")
    if "." in s:
        d, f = s.split("."); return sign * (abs(int(d)) + int(f) / 60.0)
    return sign * float(s)

def sorl(c):
    """D1 tabulates some circuits the long way round (their TX name ends "LP").
    Pick the sense whose great-circle distance matches the tabulated one."""
    import math
    a, b, x, y = map(math.radians, (c['txlat'], c['txlng'], c['rxlat'], c['rxlng']))
    cosd = math.sin(a)*math.sin(x) + math.cos(a)*math.cos(x)*math.cos(y-b)
    short = 6371.009 * math.acos(max(-1.0, min(1.0, cosd)))
    long_ = 2.0*math.pi*6371.009 - short
    return "LONGPATH" if abs(c['dist']-long_) < abs(c['dist']-short) else "SHORTPATH"

def geometry():
    g = {}
    with open(os.path.join(D1, "D1_Table1.csv")) as f:
        r = csv.reader(f); next(r)
        for row in r:
            if len(row) < 12 or not row[0].strip().isdigit(): continue
            g[(int(row[0]), int(row[10]), int(row[11]))] = dict(
                id=int(row[0]), freq=float(row[3]),
                txlat=dm(row[4]), txlng=dm(row[5]), rxlat=dm(row[6]), rxlng=dm(row[7]),
                dist=float(row[8]) if row[8].strip().replace('.','').isdigit() else 0.0,
                ssn=int(row[9]))
    return g

def measured():
    out = []
    with open(os.path.join(D1, "D1_Table2.csv")) as f:
        r = csv.reader(f); next(r)
        for row in r:
            if len(row) < 27 or not row[0].strip().isdigit(): continue
            vals = [float(x) for x in row[3:27]]
            out.append((int(row[0]), int(row[1]), int(row[2]), vals))
    return out

def predict(c, year, month, tmp, exe=None, libs=None, env=None):
    """Run one circuit-month; returns {hour: (basic MUF, field strength)}.
    `env` adds variables to the engine's environment (d1_bias.py uses it)."""
    inp, outp = os.path.join(tmp, "c.in"), os.path.join(tmp, "c.out")
    open(inp, "w").write(f'''PathName "D1 {c['id']}"
PathTXName "TX"
Path.L_tx.lat {c['txlat']}
Path.L_tx.lng {c['txlng']}
TXAntFilePath "ISOTROPIC"
TXGOS 0.0
TXBearing 0.0
PathRXName "RX"
Path.L_rx.lat {c['rxlat']}
Path.L_rx.lng {c['rxlng']}
RXAntFilePath "ISOTROPIC"
RXGOS 0.0
RXBearing 0.0
AntennaOrientation "TX2RX"
Path.year {year}
Path.month {month}
Path.hour 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24
Path.SSN {c['ssn']}
Path.frequency {c['freq']}
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
Path.SorL "{sorl(c)}"
RptFilePath "{tmp}/"
RptFileFormat "RPT_D | RPT_BMUF | RPT_E"
LL.lat {c['rxlat']}
LL.lng {c['rxlng']}
LR.lat {c['rxlat']}
LR.lng {c['rxlng']}
UL.lat {c['rxlat']}
UL.lng {c['rxlng']}
UR.lat {c['rxlat']}
UR.lng {c['rxlng']}
latinc 1.0
lnginc 1.0
DataFilePath "{DATA}"
''')
    # The report path is shared by every call: remove the last one so a failed
    # run cannot be read as this circuit's prediction.
    if os.path.exists(outp): os.remove(outp)
    rc = subprocess.run([exe or EXE, "-s", inp, outp],
                        env={**os.environ, **(env or {}), "DYLD_LIBRARY_PATH": libs or LIBS},
                        capture_output=True).returncode
    res = {}
    if rc != 0:
        print(f"circuit {c['id']} {year}/{month}: engine exit {rc}; skipped", file=sys.stderr)
        return res
    try:
        for line in open(outp, errors="surrogateescape"):
            p = [x.strip() for x in line.split(",")]
            if len(p) >= 6 and p[0].isdigit() and p[1].isdigit():
                res[int(p[1])] = (float(p[4]), float(p[5]))   # basic MUF, E
    except FileNotFoundError:
        pass
    return res

def report(rs, label):
    if not rs:
        print(f"  {label:26s} n=     0")
        return
    d = [r[0] for r in rs]
    n = len(d)
    print(f"  {label:26s} n={n:6d}  bias {sum(d)/n:+7.3f}  "
          f"RMS {math.sqrt(sum(x*x for x in d)/n):7.3f}  "
          f"max {max(abs(x) for x in d):8.2f}")

def main():
    ncase = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    exe   = sys.argv[2] if len(sys.argv) > 2 else EXE
    libs  = sys.argv[3] if len(sys.argv) > 3 else LIBS
    geo, meas, rec, used = geometry(), measured(), [], 0
    with tempfile.TemporaryDirectory() as tmp:
        for cid, yy, mm, vals in meas:
            if used >= ncase: break
            year = 2000 + yy if yy < 50 else 1900 + yy
            c = geo.get((cid, yy, mm))
            if c is None: continue
            pred = predict(c, year, mm, tmp, exe, libs)
            if not pred: continue
            got = False
            for h in range(1, 25):
                m = vals[h-1]
                if m >= 99 or h not in pred: continue
                bmuf, e = pred[h]
                rec.append((e - m, c['dist'], c['freq'] > bmuf)); got = True
            if got: used += 1

    if rec:
        print(f"  cases {used}, hourly comparisons {len(rec)}  (dB, predicted - measured)")
        report(rec, "all")
        # By distance: which of the three P.533 models produced the value.
        report([r for r in rec if r[1] <= 7000],            "d <= 7000 km")
        report([r for r in rec if 7000 < r[1] <= 9000],     "7000 < d <= 9000 km")
        report([r for r in rec if r[1] > 9000],             "d > 9000 km")
        # By operating frequency relative to the basic MUF. The above-the-MUF loss
        # Lm, P.533-14 eq (24)-(26), acts only on the second of these. The short
        # model's bias shows there, but d1_bias.py finds it starts just below the
        # MUF -- see the KNOWN BIAS note in
        # P533/Src/P533/MedianSkywaveFieldStrengthShort.c.
        report([r for r in rec if r[1] <= 7000 and not r[2]], "d<=7000, f <= basic MUF")
        report([r for r in rec if r[1] <= 7000 and r[2]],     "d<=7000, f  > basic MUF")
    else:
        print("  no comparisons made")

if __name__ == "__main__":
    main()
