#!/usr/bin/env python3
"""Generate ITURHFProp/D1/D1Comp.csv, the prediction file D1_1148.py consumes.

D1_1148.py walks D1Comp.csv, D1_Table1.csv and D1_Table2.csv in lockstep, so
this writes exactly one row per D1_Table1 row, in that order:

    ID,YY,MM,<24 predicted median field strengths, dB(1 uV/m)>

separated by CR, matching the file that shipped with the repository. Hours the
engine does not return are written as -307.0 (TINYDB), the sentinel D1_1148.py
was originally written to skip.

Measurements are normalised for 1 kW EIRP, so circuits run at 0 dB(1kW) into
isotropic antennas. Ten circuits are tabulated the long way round (TX name ends
"LP"); their path sense is taken from the tabulated distance.

Usage: d1_makecomp.py [exe] [libdir] [outfile]

D1Comp.csv is a tracked file holding some other engine's predictions, so
writing there replaces it; pass an explicit outfile to leave it alone, or
restore it afterwards with git checkout.
"""
import csv, math, os, subprocess, sys, tempfile

ROOT = "/Users/warrensly/NetBeansProjects/ITU-R-HF"
D1   = os.path.join(ROOT, "ITURHFProp/D1")
DATA = os.path.join(ROOT, "ITURHFProp/Data/")
EXE  = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "ITURHFProp/Linux/ITURHFProp")
LIBS = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "P533/Linux") + ":" + os.path.join(ROOT, "P372/Linux")
OUT  = sys.argv[3] if len(sys.argv) > 3 else os.path.join(D1, "D1Comp.csv")

def dm(s):
    s = s.strip(); sign = -1.0 if s[-1] in "SW" else 1.0; s = s.rstrip("NSEW")
    if "." in s:
        d, f = s.split("."); return sign * (abs(int(d)) + int(f) / 60.0)
    return sign * float(s)

def sorl(txlat, txlng, rxlat, rxlng, dist):
    a, b, x, y = map(math.radians, (txlat, txlng, rxlat, rxlng))
    cosd = math.sin(a)*math.sin(x) + math.cos(a)*math.cos(x)*math.cos(y-b)
    short = 6371.009 * math.acos(max(-1.0, min(1.0, cosd)))
    return "LONGPATH" if abs(dist - (2.0*math.pi*6371.009 - short)) < abs(dist - short) else "SHORTPATH"

TEMPLATE = '''PathName "D1 {id}"
PathTXName "TX"
Path.L_tx.lat {txlat}
Path.L_tx.lng {txlng}
TXAntFilePath "ISOTROPIC"
TXGOS 0.0
TXBearing 0.0
PathRXName "RX"
Path.L_rx.lat {rxlat}
Path.L_rx.lng {rxlng}
RXAntFilePath "ISOTROPIC"
RXGOS 0.0
RXBearing 0.0
AntennaOrientation "TX2RX"
Path.year {year}
Path.month {month}
Path.hour 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24
Path.SSN {ssn}
Path.frequency {freq}
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
Path.SorL "{sorl}"
RptFilePath "{tmp}/"
RptFileFormat "RPT_D | RPT_E"
LL.lat {rxlat}
LL.lng {rxlng}
LR.lat {rxlat}
LR.lng {rxlng}
UL.lat {rxlat}
UL.lng {rxlng}
UR.lat {rxlat}
UR.lng {rxlng}
latinc 1.0
lnginc 1.0
DataFilePath "{DATA}"
'''

rows, missing = [], 0
with tempfile.TemporaryDirectory() as tmp:
    inp, outp = os.path.join(tmp, "c.in"), os.path.join(tmp, "c.out")
    with open(os.path.join(D1, "D1_Table1.csv")) as f:
        r = csv.reader(f); next(r)
        for row in r:
            if len(row) < 12 or not row[0].strip().isdigit(): continue
            cid, yy, mm = int(row[0]), int(row[10]), int(row[11])
            txlat, txlng = dm(row[4]), dm(row[5])
            rxlat, rxlng = dm(row[6]), dm(row[7])
            dist = float(row[8]) if row[8].strip().replace('.', '').isdigit() else 0.0
            with open(inp, "w") as g:
                g.write(TEMPLATE.format(id=cid, txlat=txlat, txlng=txlng, rxlat=rxlat,
                                        rxlng=rxlng, year=1900+yy, month=mm, ssn=int(row[9]),
                                        freq=float(row[3]),
                                        sorl=sorl(txlat, txlng, rxlat, rxlng, dist),
                                        tmp=tmp, DATA=DATA))
            subprocess.run([EXE, "-s", inp, outp],
                           env={**os.environ, "DYLD_LIBRARY_PATH": LIBS}, capture_output=True)
            pred = {}
            try:
                for line in open(outp, errors="surrogateescape"):
                    p = [x.strip() for x in line.split(",")]
                    if len(p) >= 5 and p[0].isdigit() and p[1].isdigit():
                        pred[int(p[1])] = float(p[4])
            except FileNotFoundError:
                pass
            vals = []
            for h in range(1, 25):
                if h in pred: vals.append(f"{pred[h]:.0f}")
                else: vals.append("-307.0"); missing += 1
            rows.append(f"{cid:3d},{yy:2d},{mm:02d}," + ",".join(vals))

with open(OUT, "w", newline="") as f:
    f.write("\r".join(rows) + "\r")
print(f"wrote {len(rows)} rows to {OUT} ({missing} hours had no prediction)")
