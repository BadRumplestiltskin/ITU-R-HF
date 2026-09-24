#!/usr/bin/env python3
"""Break down the short model's D1 bias near and above the basic MUF.

d1_absolute.py reports that P.533-14's above-the-MUF loss leaves the short
model (d <= 7000 km) biased high when f > basic MUF. This script asks where
that bias actually lives and whether changing Lm removes it.

It builds a private, instrumented copy of libp533 in a temporary directory --
the tree's sources are copied and patched there, never edited -- whose
MedianSkywaveFieldStrengthShort() takes two knobs from the environment:

    LMVAR  0  P.533-14 equations (24)-(26), as the tree has it
           1  the code before commit 6cce8d5: E 46x^0.5+5 cap 58; F2 36x^0.5+5
              cap 60 up to 3000 km, 70x+8 cap 80 beyond; Lz 9.14
           2  P.533-14's F2 expression plus 5 dB, cap 62
    FBK    factor applied to each mode's basic MUF inside the Lm expressions
           only (x = f/(FBK*fb) - 1); 1.0 leaves the model unchanged

and, when MODEDUMP names a file, appends every mode summed into Es to it.
Each variant is run over all of D1 and reported as bias/RMS (predicted minus
measured, dB) by population.

The instrumented library with LMVAR=0, FBK=1 must reproduce the tree's
library exactly; the script checks this and stops if it does not.

Usage: d1_bias.py [path_to_ITURHFProp] [path_to_libp372_dir]
Run `make -C Linux all` first. Takes about two minutes on 10 cores.
"""
import collections, csv, math, os, platform, shutil, statistics, subprocess, sys, tempfile
from multiprocessing import Pool

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import d1_absolute as d1

ROOT = d1.ROOT
EXE  = sys.argv[1] if len(sys.argv) > 1 else d1.EXE
P372 = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "P372/Linux")
SRC  = os.path.join(ROOT, "P533/Src/P533")

VARIANTS = [                          # (label, LMVAR, FBK)
    ("P.533-14",       "0", "1.0"),
    ("pre-6cce8d5",    "1", "1.0"),
    ("F2 Lm +5 dB",    "2", "1.0"),
    ("fb x 0.95",      "0", "0.95"),
    ("fb x 0.90",      "0", "0.90"),
    ("fb x 0.85",      "0", "0.85"),
]

def sub(s, old, new, count=1):
    """Replace exactly `count` occurrences, or stop: the patch must not drift
    silently if MedianSkywaveFieldStrengthShort.c changes."""
    if s.count(old) != count:
        sys.exit(f"d1_bias: patch anchor found {s.count(old)} times, expected {count}:\n  {old}")
    return s.replace(old, new)

def instrument(s):
    s = ('#include <stdio.h>\n#include <stdlib.h>\n'
         'static int lmvar(void) { const char *e = getenv("LMVAR"); return e ? atoi(e) : 0; }\n'
         'static double fbk(void) { const char *e = getenv("FBK"); return e ? atof(e) : 1.0; }\n'
         'static double LmE[16], LmF[16];\n') + s

    # Scale fb inside the two Lm blocks only.
    for eq, md in (("(24) and (25)", "Md_E"), ("(24) and (26)", "Md_F2")):
        a = s.index(f'"Above-the-MUF" loss, P.533-14 equations {eq}')
        b = s.index('// Ground reflection loss', a)
        blk = s[a:b].replace(f'path->{md}[n].BMUF', f'(fbk()*path->{md}[n].BMUF)')
        s = s[:a] + blk + s[b:]

    e14 = "Lm = MIN(130.0*pow(((path->frequency/(fbk()*path->Md_E[n].BMUF)) - 1.0), 2.0), 81.0);"
    s = sub(s, e14, "if(lmvar() == 1) Lm = MIN(46.0*pow(((path->frequency/(fbk()*path->Md_E[n].BMUF)) - 1.0), 0.5) + 5.0, 58.0);\n"
                    "\t\t\t\t\telse " + e14)
    f14 = "Lm = MIN(36.0*pow(((path->frequency/(fbk()*path->Md_F2[n].BMUF)) - 1.0), 0.5), 62.0);"
    s = sub(s, f14, "if(lmvar() == 1) {\n"
                    "\t\t\t\t\t\tif(path->distance <= 3000) Lm = MIN(36.0*pow(((path->frequency/(fbk()*path->Md_F2[n].BMUF)) - 1.0), 0.5) + 5.0, 60.0);\n"
                    "\t\t\t\t\t\telse Lm = MIN(70.0*(path->frequency/(fbk()*path->Md_F2[n].BMUF) - 1.0) + 8.0, 80.0);\n"
                    "\t\t\t\t\t}\n"
                    "\t\t\t\t\telse if(lmvar() == 2) Lm = MIN(36.0*pow(((path->frequency/(fbk()*path->Md_F2[n].BMUF)) - 1.0), 0.5) + 5.0, 62.0);\n"
                    "\t\t\t\t\telse " + f14)
    s = sub(s, "path->Lz = NOIL;", "path->Lz = (lmvar() == 1) ? 9.14 : NOIL;", 2)

    s = sub(s, "path->Md_E[n].Lb = 32.45", "LmE[n] = Lm; path->Md_E[n].Lb = 32.45")
    s = sub(s, "path->Md_F2[n].Lb = 32.45", "LmF[n] = Lm; path->Md_F2[n].Lb = 32.45")
    for lay, md, lm, fs in (("E", "Md_E", "LmE", "0.0"), ("F", "Md_F2", "LmF", "path->Md_F2[n].fs")):
        acc = f"Etw += pow(10.0, (path->{md}[n].Ew/10.0));"
        s = sub(s, acc, acc +
            ' { const char *d = getenv("MODEDUMP"); if(d) { FILE *f = fopen(d, "a"); if(f) {'
            f' fprintf(f, "%d,{lay},%d,%.3f,%.3f,%.2f,%.2f\\n", path->hour, n, path->{md}[n].BMUF, {fs}, {lm}[n], path->{md}[n].Ew);'
            ' fclose(f); } } }')
    return s

def build(dst):
    for f in os.listdir(SRC):
        if f.endswith((".c", ".h")):
            shutil.copy(os.path.join(SRC, f), dst)
    p = os.path.join(dst, "MedianSkywaveFieldStrengthShort.c")
    s = open(p, encoding="latin-1").read()     # several P533 sources are Latin-1
    open(p, "w", encoding="latin-1").write(instrument(s))
    cc = "clang" if platform.system() == "Darwin" else "gcc"
    srcs = sorted(os.path.join(dst, f) for f in os.listdir(dst) if f.endswith(".c"))
    subprocess.run([cc, "-std=c99", "-fPIC", "-O2", "-w", "-I" + dst, "-I" + os.path.join(ROOT, "Include"),
                    "-shared", "-o", os.path.join(dst, "libp533.so"), *srcs, "-lm", "-ldl"], check=True)

GEO = None
def work(job):
    """One circuit-month under one variant: a row per measured hour."""
    (cid, yy, mm, vals), libs, env = job
    c = GEO.get((cid, yy, mm))
    if c is None: return []
    year = 2000 + yy if yy < 50 else 1900 + yy
    with tempfile.TemporaryDirectory() as tmp:
        md = os.path.join(tmp, "modes.txt")
        pred = d1.predict(c, year, mm, tmp, EXE, libs, {**env, "MODEDUMP": md})
        modes = collections.defaultdict(list)
        if os.path.exists(md):
            for line in open(md):
                h, lay, n, b, fs, lm, ew = line.split(",")
                modes[int(h)].append((f"{int(n) + 1}{'F2' if lay == 'F' else 'E'}", float(lm), float(ew)))
    out = []
    for h in range(1, 25):
        m = vals[h-1]
        if m >= 99 or h not in pred: continue
        bmuf, e = pred[h]
        ms = modes.get(h - 1, [])               # path->hour counts from 0
        dom = max(ms, key=lambda x: x[2]) if ms else ("-", 0.0, 0.0)
        out.append(dict(id=cid, yy=yy, mm=mm, h=h, dist=c['dist'], x=c['freq']/bmuf,
                        pred=e, meas=m, dom=dom[0], domlm=dom[1],
                        modesum=10*math.log10(sum(10**(x[2]/10) for x in ms)) if ms else None))
    return out

def init(geo):
    global GEO
    GEO = geo

def run(pool, meas, libs, env):
    return [r for rs in pool.map(work, [(m, libs, env) for m in meas], chunksize=4) for r in rs]

def stat(rs):
    d = [r['pred'] - r['meas'] for r in rs]
    if len(d) < 20: return f"({len(d)})"
    return f"{statistics.mean(d):+6.2f}/{math.sqrt(sum(v*v for v in d)/len(d)):5.2f}"

def main():
    geo, meas = d1.geometry(), d1.measured()
    with tempfile.TemporaryDirectory() as lib, Pool(os.cpu_count(), init, (geo,)) as pool:
        build(lib)
        tree = run(pool, meas, os.path.join(ROOT, "P533/Linux") + ":" + P372, {})
        res = {v[0]: run(pool, meas, lib + ":" + P372, {"LMVAR": v[1], "FBK": v[2]}) for v in VARIANTS}

    base = res["P.533-14"]
    key = lambda rs: [(r['id'], r['yy'], r['mm'], r['h'], r['pred']) for r in rs]
    if key(base) != key(tree):
        sys.exit("d1_bias: instrumented library does not reproduce the tree's -- results not valid")
    summed = [r for r in base if r['dist'] <= 7000 and r['modesum'] is not None]
    worst = max(abs(r['pred'] - r['modesum']) for r in summed)
    print(f"Instrumented library reproduces the tree's on all {len(base)} hourly comparisons; "
          f"dumped modes power-sum to Es within {worst:.2f} dB.\n")

    short = lambda r: r['dist'] <= 7000
    print("Bias/RMS (dB, predicted - measured) by f/fb and distance, P.533-14, d <= 7000 km.")
    print("fb is the path basic MUF, the larger of the lowest-order E and F2 mode MUFs.")
    xs = [0, 0.7, 0.85, 0.95, 1.0, 1.05, 1.1, 1.2, 1.3, 1.5, 2, 3, 99]
    ds = [(0, 1000), (1000, 2000), (2000, 3000), (3000, 4000), (4000, 7000)]
    print(f"{'f/fb':12s}" + "".join(f"{f'{a}-{b} km':>15s}" for a, b in ds))
    for a, b in zip(xs, xs[1:]):
        print(f"{a:4.2f}-{b:<6.2f} " + "".join(
            f"{stat([r for r in base if lo < r['dist'] <= hi and a < r['x'] <= b]):>15s}" for lo, hi in ds))

    pops = [
        ("d <= 7000",           lambda r: short(r)),
        ("f/fb <= 0.9",         lambda r: short(r) and r['x'] <= 0.9),
        ("0.9 < f/fb <= 1",     lambda r: short(r) and 0.9 < r['x'] <= 1),
        ("1 < f/fb <= 1.2",     lambda r: short(r) and 1 < r['x'] <= 1.2),
        ("1.2 < f/fb <= 2",     lambda r: short(r) and 1.2 < r['x'] <= 2),
        ("f/fb > 2",            lambda r: short(r) and r['x'] > 2),
        ("f>fb, d <= 1000",     lambda r: r['dist'] <= 1000 and r['x'] > 1),
        ("f>fb, 1000-3000",     lambda r: 1000 < r['dist'] <= 3000 and r['x'] > 1),
        ("f>fb, 3000-4000",     lambda r: 3000 < r['dist'] <= 4000 and r['x'] > 1),
        ("f>fb, 4000-7000",     lambda r: 4000 < r['dist'] <= 7000 and r['x'] > 1),
    ]
    print("\nBias/RMS by Lm variant (rows are fixed by the P.533-14 run's f/fb).")
    print(f"{'population':18s}{'n':>6s}" + "".join(f"{v[0]:>14s}" for v in VARIANTS))
    for name, fn in pops:
        idx = [i for i, r in enumerate(base) if fn(r)]
        print(f"{name:18s}{len(idx):6d}" + "".join(
            f"{stat([res[v[0]][i] for i in idx]):>14s}" for v in VARIANTS))

    above = [r for r in base if short(r) and r['x'] > 1]
    print(f"\nDominant mode when f > fb, d <= 7000 (n={len(above)}): " + ", ".join(
        f"{k} {v}" for k, v in collections.Counter(r['dom'] for r in above).most_common(6)))

    # Near-MUF hours cluster at dawn and dusk, so check the hour convention:
    # RMS should be least with no shift.
    P = {(r['id'], r['yy'], r['mm'], r['h']): r for r in base}
    print("\nHour alignment, all comparisons (prediction shifted against measurement):")
    for s in (-1, 0, 1):
        d = []
        for r in base:
            q = P.get((r['id'], r['yy'], r['mm'], (r['h'] - 1 + s) % 24 + 1))
            if q is not None: d.append(q['pred'] - r['meas'])
        print(f"  {s:+d} h  RMS {math.sqrt(sum(v*v for v in d)/len(d)):6.3f}  (n={len(d)})")

if __name__ == "__main__":
    main()
