#!/usr/bin/env python3
"""Build the CCIR D1 databank comparison cases.
Reads ITURHFProp/D1/D1_Table1.csv (circuits) and D1_Table2.csv (measured
median field strength per UTC hour, 99 = no data) and writes
  tests/reference/d1_cases.csv     ref_p533 input (one row per circuit-month-hour)
  tests/reference/d1_measured.csv  id,row,hour,E_measured
Hours are 1..24 in the table (= UTC hour 0..23); every second hour is used
to halve the run time. Field strengths are normalised to 1 kW e.i.r.p., so
txpower = 0 dB(1 kW) with isotropic antennas (P.533 section 1)."""
import csv, math, sys, os
# Default to the checkout this script lives in: tools/ is P533/Matlab/tools.
_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
D1 = os.environ.get("ITU_R_HF", _ROOT) + "/ITURHFProp/D1"
out = sys.argv[1]
def dm(s):
    s = s.strip(); sign = -1.0 if s[-1] in "SW" else 1.0; s = s.rstrip("NSEW")
    d, m = (s.split(".") + ["0"])[:2]
    return sign * (abs(int(d)) + int(m) / 60.0)
t1 = [r for r in csv.reader(open(os.path.join(D1, "D1_Table1.csv"))) if r and r[0].strip().isdigit()]
t2 = [r for r in csv.reader(open(os.path.join(D1, "D1_Table2.csv"))) if r and r[0].strip().isdigit()]
assert len(t1) == len(t2), (len(t1), len(t2))
cases = []; meas = []; cid = 0
for k, (a, b) in enumerate(zip(t1, t2)):
    if a[0].strip() != b[0].strip(): raise SystemExit("row mismatch at %d" % k)
    freq = float(a[3]); txlat, txlng, rxlat, rxlng = dm(a[4]), dm(a[5]), dm(a[6]), dm(a[7])
    ssn = int(a[9]); yy = int(a[10]); year = 1900 + yy if yy > 30 else 2000 + yy; month = int(a[11])
    for h in range(0, 24, 2):
        E = float(b[3 + h])
        if E == 99: continue
        cid += 1
        cases.append([cid, year, month, h, ssn, freq, txlat, txlng, rxlat, rxlng, 2.0, 0, 0.0, 6000.0, 0.0, 90, 0.0, 0, 0, 0, 0, 0, 0, "ISOTROPIC", 0.0, "ISOTROPIC", 0.0])
        meas.append([cid, a[0].strip(), k + 1, h, E, freq, month, ssn])
cases.sort(key=lambda r: r[2])
order = {r[0]: i + 1 for i, r in enumerate(cases)}
with open(os.path.join(out, "d1_cases.csv"), "w", newline="") as fp:
    w = csv.writer(fp)
    w.writerow("id,year,month,hour,ssn,freq,txlat,txlng,rxlat,rxlng,mm,mod,txpower,bw,snrr,snrxxp,sirr,A,TW,FW,T0,F0,sorl,txant,txgos,rxant,rxgos".split(","))
    for r in cases: w.writerow(r)
with open(os.path.join(out, "d1_measured.csv"), "w", newline="") as fp:
    w = csv.writer(fp); w.writerow(["id", "circuit", "tablerow", "hour", "Emeas", "freq", "month", "ssn"])
    for r in meas: w.writerow(r)
print(len(t1), "circuit-months,", len(cases), "cases")
