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

ROOT = "/Users/warrensly/NetBeansProjects/ITU-R-HF"
D1   = os.path.join(ROOT, "ITURHFProp/D1")
DATA = os.path.join(ROOT, "ITURHFProp/Data/")
NCASE = int(sys.argv[1]) if len(sys.argv) > 1 else 40
EXE   = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "ITURHFProp/Linux/ITURHFProp")
LIBS  = sys.argv[3] if len(sys.argv) > 3 else os.path.join(ROOT, "P533/Linux") + ":" + os.path.join(ROOT, "P372/Linux")

def dm(s):
    s = s.strip(); sign = -1.0 if s[-1] in "SW" else 1.0; s = s.rstrip("NSEW")
    if "." in s:
        d, f = s.split("."); return sign * (abs(int(d)) + int(f) / 60.0)
    return sign * float(s)

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

def predict(c, year, month, tmp):
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
Path.SSN {min(c['ssn'],160)}
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
Path.SorL "SHORTPATH"
RptFilePath "{tmp}/"
RptFileFormat "RPT_D | RPT_E"
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
    subprocess.run([EXE, "-s", inp, outp],
                   env={**os.environ, "DYLD_LIBRARY_PATH": LIBS}, capture_output=True)
    res = {}
    try:
        for line in open(outp, errors="surrogateescape"):
            p = [x.strip() for x in line.split(",")]
            if len(p) >= 5 and p[0].isdigit() and p[1].isdigit():
                res[int(p[1])] = float(p[4])
    except FileNotFoundError:
        pass
    return res

geo, meas, diffs, used = geometry(), measured(), [], 0
with tempfile.TemporaryDirectory() as tmp:
    for cid, yy, mm, vals in meas:
        if used >= NCASE: break
        year = 2000 + yy if yy < 50 else 1900 + yy
        c = geo.get((cid, yy, mm))
        if c is None: continue
        pred = predict(c, year, mm, tmp)
        if not pred: continue
        got = False
        for h in range(1, 25):
            m = vals[h-1]
            if m >= 99 or h not in pred: continue
            diffs.append(pred[h] - m); got = True
        if got: used += 1

if diffs:
    n = len(diffs)
    bias = sum(diffs)/n
    rms  = math.sqrt(sum(d*d for d in diffs)/n)
    print(f"  cases {used}, hourly comparisons {n}")
    print(f"  bias (predicted - measured) = {bias:+.3f} dB")
    print(f"  RMS error                   = {rms:.3f} dB")
    print(f"  max |error|                 = {max(abs(d) for d in diffs):.2f} dB")
else:
    print("  no comparisons made")
