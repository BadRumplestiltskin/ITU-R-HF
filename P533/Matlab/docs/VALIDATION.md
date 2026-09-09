# Validation

Three layers, as planned in `P533_PORT_PLAN.md` section 4.

## 1. Against the ITU C code (loose reference)

`tools/build_reference.sh` compiles the unmodified ITU P533 14.2 sources
with clang, links them into `tools/ref_p533.c` and produces
`tests/reference/`: 1 405 cases (`cases.csv`, `ref_p533.csv`: 6 transmit
sites x 3 bearings x 13 distances from 100 to 18 000 km, months 1/4/7/10,
hours 0/6/12/18, SSN 10/70/150, 2-30 MHz, analog and digital, isotropic
and Type 13 antennas) and reader dumps (`dump/`).

The comparison tests run the engine with `liRule = 'reference'` and
`sigmaRule = 'reference'` so that only genuine differences remain, and
tag the cases that hit a documented deviation (`P533_DEVIATIONS.md`):

| Test | Scope | Result |
|---|---|---|
| test_readIonParameters | ionos maps, 2 months, subset and sums | exact |
| test_readP1239 | 8 208 decile factors | exact |
| test_readAntenna | Type 13 pattern, two bearings | exact |
| test_geometry | distances, bearings, intermediate and geomagnetic points | 1e-7 rad (C truncates pi/180) |
| test_pointParameters | 288 points: dip, fH 1e-4; solar 1e-7; foF2 exact in the NE quadrant (D27 elsewhere); foE 1e-8 by day (D08/D28 at night) | pass |
| test_part1_vs_reference | 1 033 cases to 9 000 km: path and per-mode MUFs | untagged worst 0.000 MHz; D14/D27/D28 tagged within 5 % |
| test_part2_vs_reference | Es (after the 0.42 dB Lz and the D31 geometry are accounted for) 0.48 dB worst on clean cases, 2.6 dB with night or S/W hemisphere; El 0.40 dB; above-MUF cases skipped (D01/D02 differ by formula) | pass |
| test_part3_vs_reference | noise exact; SNR identity; deciles within 5.3 dB (D23 geomagnetic test); BCR consistent | pass |

Run time about 220 s in GNU Octave 11.3.

## 2. Against the Recommendation

Every formula is coded from `P533_EQUATIONS.md` with the equation number
in the source; tables are typed from the PDFs. The comparison tests above
double as equation tests wherever the C code agrees with the text.

## 3. Against measurements: the CCIR D1 databank

`tools/make_d1_cases.py` builds 8 094 cases from `ITURHFProp/D1`
(1 613 circuit-months, every second hour with a measured median);
`tests/d1_compare.m` runs them. Predicted minus measured median sky-wave
field strength, dB, 1 kW e.i.r.p., isotropic antennas:

| predictor | n | mean | rms | <5 MHz mean/rms | 5-10 | 10-15 | >=15 |
|---|---|---|---|---|---|---|---|
| ITU C code (P533 14.2) | 8084 | -1.42 | 12.71 | 0.5 / 8.3 | -0.6 / 12.1 | -3.0 / 16.1 | -1.9 / 11.5 |
| MATLAB `liRule = 'hopmean'` (default) | 8082 | -0.21 | 13.42 | 1.2 / 8.5 | 0.6 / 12.7 | -2.1 / 17.0 | 0.0 / 12.4 |
| MATLAB `liRule = 'reference'` (C conventions) | 8084 | 0.25 | 12.87 | 1.2 / 8.5 | 0.9 / 12.1 | -1.1 / 16.1 | 0.4 / 12.1 |
| MATLAB `liRule = 'sum'` (eq. 20 as printed) | 8082 | -5.54 | 16.57 | -10.1 / 18.9 | -5.9 / 15.6 | -4.1 / 17.7 | -3.9 / 14.8 |

Conclusions:
- Equation (20) evaluated as printed (a sum over all 2n penetration
  points) doubles the absorption and is biased by -5.5 dB overall and
  -10 dB below 5 MHz. The hop-mean reading is unbiased. It is the default
  (deviation D30, to be raised with ITU-R Study Group 3).
- With the text's other choices (Lz 8.72 dB, equations 25-26 for the
  above-MUF loss, 300 km penetration geometry, local time at the control
  points) the implementation is as accurate as the ITU code on D1
  (rms 13.4 vs 12.7 dB, mean -0.2 vs -1.4 dB).
- Regenerate with `python3 tools/make_d1_cases.py tests/reference`,
  `tools/build/ref_p533 <Data/> tests/reference/d1_cases.csv ...`, and
  `d1_compare('reference/d1_matlab_hopmean.csv', {'hopmean'})`; summarise
  with `python3 tests/d1_stats.py`.

## Not executed here

`apps/HFPropApp.m` requires MATLAB (uifigure) and was not run in this
environment; it contains no physics of its own.
