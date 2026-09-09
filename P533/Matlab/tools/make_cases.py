#!/usr/bin/env python3
"""Generate tests/reference/cases.csv for ref_p533. Deterministic."""
import math, random, csv, sys, os
random.seed(533)
R0 = 6371.009
TX = [("Luxembourg", 49.6667, 6.3167), ("Boulder", 40.0157, -105.2793), ("Sydney", -33.87, 151.21),
      ("Nairobi", -1.29, 36.82), ("Tromso", 69.65, 18.96), ("BuenosAires", -34.6, -58.4)]
DIST = [100, 500, 1500, 2500, 3500, 5000, 6999, 7000, 8000, 9000, 9001, 12000, 18000]
BEAR = [0, 90, 225]
MONTHS = [1, 4, 7, 10]; HOURS = [0, 6, 12, 18]; SSN = [10, 70, 150]; FREQ = [2, 5, 10, 20, 30]
antdir = sys.argv[1] if len(sys.argv) > 1 else ""
T13 = os.path.join(antdir, "141-10_0.t13") if antdir else "ISOTROPIC"

def dest(lat, lng, brg, d):
    la, lo, b = map(math.radians, (lat, lng, brg)); dr = d / R0
    la2 = math.asin(math.sin(la) * math.cos(dr) + math.cos(la) * math.sin(dr) * math.cos(b))
    lo2 = lo + math.atan2(math.sin(b) * math.sin(dr) * math.cos(la), math.cos(dr) - math.sin(la) * math.sin(la2))
    lo2 = (lo2 + math.pi) % (2 * math.pi) - math.pi
    return round(math.degrees(la2), 6), round(math.degrees(lo2), 6)

rows = []; cid = 0
# README-style validation circuit first
rows.append([1984, 8, 1, 37, 6.1, 49.6666666667, 6.31666666667, 51.1166666667, 7.26666666667, 2.0, 0, 0.0, 1000.0, 10.0, 90, 23.76, 0, 0, 0, 0, 0, 0, "ISOTROPIC", 0.0, "ISOTROPIC", 0.0])
for name, tlat, tlng in TX:
    for brg in BEAR:
        for d in DIST:
            rlat, rlng = dest(tlat, tlng, brg, d)
            combos = set()
            while len(combos) < 6:
                combos.add((random.choice(MONTHS), random.choice(HOURS), random.choice(SSN), random.choice(FREQ), random.choice([0, 0, 1])))
            for (m, h, s, f, mod) in sorted(combos):
                mm = random.choice([0.0, 1.0, 2.0, 3.0])
                sorl = 1 if (d in (12000, 18000) and random.random() < 0.3) else 0
                ant = T13 if (random.random() < 0.2) else "ISOTROPIC"
                pw = random.choice([0.0, 10.0, 20.0])
                if mod == 1:
                    rows.append([2020, m, h, s, f, tlat, tlng, rlat, rlng, mm, 1, pw, 3000.0, 15.0, 90, 20.0, 3.0, 1.0, 10.0, 0.5, 1.0, sorl, ant, 0.0, ant, 0.0])
                else:
                    rows.append([2020, m, h, s, f, tlat, tlng, rlat, rlng, mm, 0, pw, 3000.0, 10.0, random.choice([50, 90, 99]), 20.0, 0.0, 0.0, 0.0, 0.0, 0.0, sorl, ant, 0.0, ant, 0.0])
rows.sort(key=lambda r: r[1])   # by month so the driver reloads maps 4 times
with open(sys.argv[2] if len(sys.argv) > 2 else "cases.csv", "w", newline="") as fp:
    w = csv.writer(fp)
    w.writerow("id,year,month,hour,ssn,freq,txlat,txlng,rxlat,rxlng,mm,mod,txpower,bw,snrr,snrxxp,sirr,A,TW,FW,T0,F0,sorl,txant,txgos,rxant,rxgos".split(","))
    for i, r in enumerate(rows, 1):
        w.writerow([i] + r)
print(len(rows), "cases")
