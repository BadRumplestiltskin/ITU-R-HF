#!/usr/bin/env python3
"""Summarise the CCIR D1 databank comparison: predicted minus measured field
strength for the ITU C code and for the MATLAB engine (both absorption readings)."""
import csv, math, sys, os, collections
ref = os.path.join(os.path.dirname(__file__), 'reference')
meas = {int(r['id']): r for r in csv.DictReader(open(os.path.join(ref, 'd1_measured.csv')))}
cpred = {int(r['id']): float(r['Ep']) for r in csv.DictReader(open(os.path.join(ref, 'd1_ref_p533.csv')))}
mpred = collections.defaultdict(dict)
for r in csv.DictReader(open(os.path.join(ref, 'd1_matlab.csv'))):
    mpred[r['rule']][int(r['id'])] = float(r['Ep'])
def stats(pairs):
    d = [p - m for p, m in pairs if p > -200 and not math.isnan(p)]
    n = len(d)
    if n == 0: return (0, float('nan'), float('nan'), float('nan'))
    mean = sum(d) / n; rms = math.sqrt(sum(x * x for x in d) / n); sd = math.sqrt(max(rms * rms - mean * mean, 0))
    return n, mean, sd, rms
def band(x, edges):
    for i, e in enumerate(edges):
        if x < e: return i
    return len(edges)
rows = [('ITU C code (P533 14.2)', cpred)] + [('MATLAB liRule=%s' % k, v) for k, v in mpred.items()]
print('%-32s %6s %8s %8s %8s' % ('predictor', 'n', 'mean', 'sd', 'rms'))
for name, pred in rows:
    n, mean, sd, rms = stats([(pred[i], float(meas[i]['Emeas'])) for i in meas if i in pred])
    print('%-32s %6d %8.2f %8.2f %8.2f' % (name, n, mean, sd, rms))
print('\nby frequency band (MHz): <5, 5-10, 10-15, >=15')
for name, pred in rows:
    out = []
    for b in range(4):
        pr = [(pred[i], float(meas[i]['Emeas'])) for i in meas if i in pred and band(float(meas[i]['freq']), [5, 10, 15]) == b]
        n, mean, sd, rms = stats(pr); out.append('%5d %6.2f %5.2f' % (n, mean, rms))
    print('%-32s | %s' % (name, ' | '.join(out)))
print('\nby measured hour (UTC): 0-5, 6-11, 12-17, 18-23')
for name, pred in rows:
    out = []
    for b in range(4):
        pr = [(pred[i], float(meas[i]['Emeas'])) for i in meas if i in pred and int(meas[i]['hour']) // 6 == b]
        n, mean, sd, rms = stats(pr); out.append('%5d %6.2f %5.2f' % (n, mean, rms))
    print('%-32s | %s' % (name, ' | '.join(out)))
