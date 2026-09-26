# Traceability: Recommendation text to code

Where each equation and table of the Recommendations is implemented.
Every P.533-14, P.1239-4 and P.842-5 entry was checked against the text
of those Recommendations. P.372, P.1240, P.1144 and P.371 are cited by the
code but their texts were not available when this table was made; those
entries are marked *unverified*.

Line numbers are those of the commit that added this file; the function
names are the stable reference. Where the code departs from the text, or
the text is ambiguous, see `DEVIATIONS.md`.

Files: `P533/Src/P533/` unless stated. MSFSS = `MedianSkywaveFieldStrengthShort.c`,
MSFSL = `MedianSkywaveFieldStrengthLong.c`, MARP = `MedianAvailableReceiverPower.c`.

## P.533-14

| Section | Equation / table | File | Function | Line |
|---|---|---|---|---|
| 2, 4 | Table 1 control points; R0 = 6371 km | Geometry.c | GreatCircleDistance, GreatCirclePoint | 19, 53 |
| 2 | Table 1a) M, T+1000, R-1000 | InitializePath.c | InitializeCPs | 237, 250 |
| 3.1 | path basic MUF = higher of E and F2 | MUFBasic.c | MUFBasic | 293 |
| 3.3 | (1) E-layer basic MUF | MUFBasic.c | MUFBasic | 268, 271 |
| 3.4 | foF2, M(3000)F2 from the maps, R12 limited to 160 | CalculateCPParameters.c | IonosphericParameters | 236-237 |
| 3.5.1.1 | (2) mirror reflection height | MUFBasic.c | MUFBasic | 100 |
| 3.5.1.1 | (3) F2(D)MUF, dmax <= 4000 km | MUFBasic.c | MUFBasic, CalcF2DMUF | 146, 156, 487 |
| 3.5.1.1 | (4) Cd | MUFBasic.c | CalcCd | 432 |
| 3.5.1.1 | (5) dmax | MUFBasic.c | Calcdmax | 363 |
| 3.5.1.1 | (6) B | MUFBasic.c | CalcB | 400 |
| 3.5.1.2 | F2(dmax)MUF, lower of the two Table 1a) points | MUFBasic.c | MUFBasic | 170, 186 |
| 3.5.2.1 | (3) at mid-path, d = D/n | MUFBasic.c | MUFBasic | 200 |
| 3.5.2.2 | (7), (8) higher-order modes beyond dmax | MUFBasic.c | MUFBasic | 213, 224 |
| 3.6 | (9), (10) probability of ionospheric support; E factors 0.95, 1.05 | MUFVariability.c | MUFVariability | 101, 105, 121-135 |
| 3.7 | operational MUF (Rop from P.1240 Table 1, *unverified*) | MUFOperational.c | MUFOperational | 61, 143-193 |
| 4 | (11) E-layer screening frequency; Table 1b) foE | ELayerScreeningFrequency.c | ELayerScreeningFrequency | 71, 104 |
| 4 | (12) angle of incidence | ELayerScreeningFrequency.c | IncidenceAngle | 291 |
| 5.1 | (13) elevation angle | ELayerScreeningFrequency.c | ElevationAngle | 260 |
| 5.1 | delta M, H, (14), (15), (16); Table 1c) mean | ELayerScreeningFrequency.c | MirrorReflectionHeight, ELayerScreeningFrequency | 165-228, 81, 88 |
| 5.2.1 | modes considered (n0 + 2 E with E <= 4000 km, n0 + 5 F2) | MUFBasic.c | MUFBasic | 198, 248 |
| 5.2.1 | mode selection (E hop <= 2000 km; F2 hop <= dmax, fs < f) | MSFSS | MedianSkywaveFieldStrengthShort | 180, 328 |
| 5.2.1 | (2) hr at the Table 1c) point with the lowest foF2 | MSFSS | MedianSkywaveFieldStrengthShort, SmallestCPfoF2 | 157, 1336 |
| 5.1 | (13) elevation for the field strength | MSFSS | MedianSkywaveFieldStrengthShort | 186, 335 |
| 5.2.2 | (17) Ew | MSFSS | MedianSkywaveFieldStrengthShort | 285, 504 |
| 5.2.2 | (18) Lb | MSFSS | MedianSkywaveFieldStrengthShort | 277, 497 |
| 5.2.2 | (19) p' | MSFSS | MedianSkywaveFieldStrengthShort | 203, 353 |
| 5.2.2 | (20) Li | MSFSS | MedianSkywaveFieldStrengthShort | 251, 430 |
| 5.2.2 | (20) penetration points (300 km, 90 km), per-point (f + fLj)^2 | MSFSS | PenetrationPoints | 1589, 1625, 1636, 1642 |
| 5.2.2 | (21) F(chi), chi <= 102 deg | MSFSS | AbsorptionTerm | 672, 674 |
| 5.2.2 | (22) fv = f cos i | MSFSS | MedianSkywaveFieldStrengthShort | 195, 344 |
| 5.2.2 | (23) fL = abs(fH sin I) | MSFSS | LongitudinalGyrofrequency | 1659 |
| 5.2.2 | Fig. 1 ATnoon | MSFSS | AbsorptionFactor | 898, 987 |
| 5.2.2 | Fig. 2 phi_n | MSFSS | AbsorptionLayerPenetrationFactor | 992-1049 |
| 5.2.2 | Fig. 3 p | MSFSS | DiurnalAbsorptionExponent | 749, 816 |
| 5.2.2 | (24), (25) Lm, E modes | MSFSS | MedianSkywaveFieldStrengthShort | 267 |
| 5.2.2 | (24), (26) Lm, F2 modes | MSFSS | MedianSkywaveFieldStrengthShort | 487 |
| 5.2.2 | (27) Lg | MSFSS | MedianSkywaveFieldStrengthShort | 271, 491 |
| 5.2.2 | Table 2 Lh; Table 1d) mean; seasons | MSFSS | FindLh, WhatSeasonforLh, MedianSkywaveFieldStrengthShort | 1108, 1279, 1283, 221, 371 |
| 5.2.2 | Table 2 Lh geomagnetic dipole 78.5 N, 68.2 W | Geometry.c | GeomagneticCoords | 166 |
| 5.2.2 | Lz = 8.72 dB | MSFSS | MedianSkywaveFieldStrengthShort | 22, 274, 494 |
| 5.2.2 | (28) Es | MSFSS | MedianSkywaveFieldStrengthShort | 549, 564, 594 |
| 5.2.2 | solar zenith angle with the equation of time, mid-month | CalculateCPParameters.c | SolarParameters | 527, 562 |
| 5.3.1 | hops <= 4000 km, (13) with hr = 300 km, >= 3 deg | MSFSL | MedianSkywaveFieldStrengthLong | 206, 214 |
| 5.3.1 | Table 1a) points T+dM/2, R-dM/2 | MSFSL | MedianSkywaveFieldStrengthLong | 281-289 |
| 5.3.1 | (29) fBM, f4, fz | MSFSL | FindMUFsandfM | 528 |
| 5.3.1 | (30) fD | MSFSL | FindMUFsandfM | 496 |
| 5.3.1 | (31) fM | MSFSL | FindMUFsandfM | 597 |
| 5.3.1 | (32) K; Table 3 | MSFSL | FindMUFsandfM | 480, 557, 564 |
| 5.3.2 | hops <= 3000 km, 90 km penetration points | MSFSL | MedianSkywaveFieldStrengthLong | 176, 188 |
| 5.3.2 | (33), (34) | MSFSL | FindfL | 712, 727 |
| 5.3.2 | Table 5 Aw | MSFSL | WinterAnomaly | 839, 866, 869 |
| 5.3.2 | (36) fLN | MSFSL | FindfL | 720 |
| 5.3.2 | (37) | MSFSL | FindfL | 743, 750 |
| 5.3.2 | (38) | MSFSL | FindfL | 764 |
| 5.3.3 | (19) p', (40) E0 | MSFSL | MedianSkywaveFieldStrengthLong | 311, 314 |
| 5.3.3 | (41) Gap <= 15 dB | MSFSL | MedianSkywaveFieldStrengthLong | 321 |
| 5.3.3 | Gtl, 0 to 8 deg | MSFSL | AntennaGain08 | 914 |
| 5.3.3 | (39) Etl, Ly = -0.14 dB | MSFSL | MedianSkywaveFieldStrengthLong | 19, 340-346 |
| 3.6, 3.7 | deciles beyond 9000 km (P.1239-4 Tables 2, 3) | MSFSL | FindMUFsandfM | 619-633 |
| 5.4 | (42) Ei | Between7000kmand9000km.c | Between7000kmand9000km | 68 |
| 5.4, 3.5.1.2 | basic MUF, lower of (3) at the Table 1a) points | Between7000kmand9000km.c | Between7000kmand9000km | 76-82 |
| 6 | (43) Prw | MARP | SumModePowers | 231, 261 |
| 6 | (44) Pr | MARP | SumModePowers, MedianAvailableReceiverPower | 247, 277, 86 |
| 6 | 7000 to 9000 km through (42) | MARP | MedianAvailableReceiverPower | 111-116 |
| 6 | beyond 9000 km, (43) with Grw 0 to 8 deg | MARP | MedianAvailableReceiverPower | 130-133 |
| 7 | (45) S/N | CircuitReliability.c | CircuitReliability | 214 |
| 10.2.2 | (47) mode delay, with (19) and (13) | CircuitReliability.c | DigitalModulationSignalandInterferers | 740-754 |
| 10.2.3 | steps 1 to 4; 5.4 (42) | CircuitReliability.c | DigitalModulationSignalandInterferers | 792, 822, 855 |
| 10.2.3 | step 5 (P.842-5 Table 3 steps 4 to 14) | CircuitReliability.c | CircuitReliability | 409-487 |
| 10.2.3 | beyond 9000 km, 3 ms and 5 ms delay spread | CircuitReliability.c | CircuitReliability | 502-503 |
| 10.3, Att. 1 sections 1 to 3 | steps 7 to 10, (48), pTspread, pFspread, FR, FS | CircuitReliability.c | EquatorialScattering | 981-1034 |
| Att. 1 section 3 | F(lambda_d) | CircuitReliability.c | FindFlambdad | 1067-1072 |
| Att. 1 section 3 | F(Tl), Tl local mean time | CircuitReliability.c, Geometry.c | FindFTl, LocalMeanTime | 1110-1123, 231 |

## P.1239-4

| Section | Equation / table | File | Function | Line |
|---|---|---|---|---|
| 2 | (4) modified dip | MSFSS | DiurnalAbsorptionExponent | 816 |
| 2 | (5) to (11) magnetic field; (8) with 6371.2 km | Magfit.c | magfit | 128, 158, 167-168 |
| 3.1 | foF2 and M(3000)F2 maps, R12 limited to 160, linear in R12 | CalculateCPParameters.c | IonosphericParameters | 236-237 |
| 3.1 | map files | ReadIonParameters.c | ReadIonParametersTxt, ReadIonParametersBin | 18, 374 |
| 3.2 | Tables 2 and 3 decile factors; seasons | MUFVariability.c, InitializePath.c | FindfoF2var, WhatSeason | 258-263, 316 |
| 3.2 | Tables 2 and 3 file | ReadP1239.c | ReadP1239 | 27 |
| 4 | (12) to (18) foE | CalculateCPParameters.c | FindfoE | 325-416 |

## P.842-5

| Section | Equation / table | File | Function | Line |
|---|---|---|---|---|
| 3 | Table 1 steps 4, 6, 7, 9, 11 | CircuitReliability.c | CircuitReliability | 287-312 |
| 3 | Table 2 and note (1) | CircuitReliability.c | CircuitReliability | 173, 239, 287 |
| 6 | Table 3 steps 4 to 14 | CircuitReliability.c | CircuitReliability | 409-487 |

## Cited but not verified (texts not available)

| Recommendation | What | File | Function |
|---|---|---|---|
| P.1240 | Table 1 Rop; day/night, season and e.i.r.p. selection | MUFOperational.c | MUFOperational |
| P.1144 | bilinear interpolation of the maps | CalculateCPParameters.c | BilinearInterpolation |
| P.371 | Phi12 from R12 in the foE model | CalculateCPParameters.c | FindfoE |
| P.842-4 section 9 | simplified digital BCR (RSN, RT, RF) | CircuitReliability.c | CircuitReliability |
| CCIR Report 322 | NORM scaling for SNRXX | CircuitReliability.c | CircuitReliability |
| P.372 (editions -9 to -17 as cited in the code) | atmospheric, man-made and galactic noise; combination of the noise sources; decile rule | `P372/Src/P372/Noise.c` | Noise, AtmosphericNoise, GetFamParameters, ManMadeNoise, GalacticNoise, AtmosphericNoise_LT |
| P.372-14 | noise figures for a point | `P372/Src/P372/MakeNoise.c` | MakeNoise |
