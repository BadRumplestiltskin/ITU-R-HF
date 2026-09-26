# Deviations, rulings and open questions

Where this implementation departs from, or has to interpret, the text of
ITU-R P.533-14 and the Recommendations it calls on (P.1239-4, P.842-5,
P.372, P.1240). The owner's instruction is that the code follows the
Recommendations exactly as stated, and that ambiguities go to the owner
for resolution. This file records both.

The MATLAB port keeps a longer, numbered list against the original ITU
14.2 C code (`p533-matlab/P533/docs/P533_DEVIATIONS.md`, entries D01 to
D40); where an item below has a number there, it is given.

## 1. Resolved by the owner

| Topic | Text | Ruling | Commit |
|---|---|---|---|
| Lh averaging (P.533-14 5.2.2, D39) | "Each value is evaluated in terms of the geomagnetic latitude Gn ... and local time t ...: mean values for the control points of Table 1d) are taken." | Lh is looked up at each Table 1d) control point, with the season of that point's own hemisphere, and the values are averaged. | unchanged (the code already did this) |
| Lh sub-table (P.533-14 Table 2, D39) | "a) Transmission ranges less than or equal to 2 500 km" | "Range" is the path length D, as elsewhere in the text; hop length is D/n. | 513091d |
| P.842-5 Table 2 note (1) on short paths (D23) | "... between control points located 1 000 km from each end of the path, reaches a geomagnetic latitude of 60 deg or more ..." | A path of 2000 km or less has no such segment and takes the < 60 deg values. | unchanged (the code already did this) |
| Long-path zenith angle, eq. (33)-(35) (D20) | delta "can be approximated" by Table 4; eta "can be approximated" by eq. (35) | Full solar geometry. | unchanged |
| Long-path local noon, eq. (32) | "value of fBM for a time corresponding to local noon" | The whole UTC hour nearest 12 - lng/15, where eq. (35) puts the hour angle at zero. | 86c022c |
| Long-path current-hour fL | "the current hour fL value is selected" | fL[path->hour], the hour the rest of the engine computes for (was hour + 1). | 86c022c |

## 2. Corrected to the text, no ruling needed

| Topic | Text | Was | Commit |
|---|---|---|---|
| Earth radius (P.533-14 section 4) | "R0: radius of the Earth, 6 371 km" | 6371.009 km | 6819fdd |
| Magnetic field radius (P.1239-4 eq. (8)) | R = 6371.2/(6371.2 + hr) | the path R0 | 6819fdd |
| Degree and radian constants | - | truncated to 9 and 7 significant figures | 6819fdd |
| fLj in eq. (20) | "fLj: ... determined at the j-th penetration point" | fL averaged over the Table 1d) control points, outside the sum | c95e053 |
| Eq. (7) higher-order MUFs beyond dmax (D40) | "The lower of the values calculated at the two control points of Table 1a) is selected." | the lower F2(dmax)MUF times the lower Mn/Mn0, possibly from different points | 94612d6 |

Earlier alignments with the text (D01 to D38 in the MATLAB list) are
recorded in the fork's history and in `p533-matlab/P533/docs/P533_DEVIATIONS.md`,
section "Status in the ITU-R-HF fork".

## 3. Open: the text is ambiguous or silent (awaiting a ruling)

Found while documenting the code (September 2026). The code is unchanged
for all of these.

| # | Where | Text | What the code does |
|---|---|---|---|
| 1 | eq. (20) Li, MSFSS PenetrationPoints (D30) | Li = (1 + 0.0067 R12) sec i sum over j = 1..m | n hops times the mean over the m = 2n points, half the printed sum. The MATLAB port's D1 databank validation supports the hop-mean reading. |
| 2 | F2 reflection height for the field-strength geometry, MSFSS | 5.1: eq. (13) for F2 uses hr from (14)-(16); 5.2.1: F2 heights "from equation (2)" | eq. (2) hr for elevation, p' and Li; the (14)-(16) height for the mode delays of eq. (47) |
| 3 | eq. (33) p', MSFSL | "p': slant path length" | p' of the fM hops (dM) |
| 4 | eq. (33) fH, MSFSL | not defined in 5.3.2 | the mean fH at 300 km of the two fM control points (the 5.3.3 definition) |
| 5 | Aw, WinterAnomaly | "unity for geographic latitudes 0 deg to 30 deg and at 90 deg" | Aw = 0 there, used as (Aw + 1) |
| 6 | P.842-5 Table 2 rows between tabulated f/BMUF ratios (D38) | no rule given | the row at or above f/fb, no interpolation |
| 7 | 10.3 step 9, EquatorialScattering | "exceeds (Ew - A)" (field strength) | compares available powers in dBW |
| 8 | 10.3 step 8, EquatorialScattering | "symmetrically at the edges of the frequency window, Fw" | edges at +/- FW from the carrier (could be +/- FW/2) |
| 9 | Attachment 1, FindFlambdad | "lambda_d is the magnetic dip" | the dip at 100 km |
| 10 | 3.5.1.1 n0, MUFBasic | "determined by geometrical considerations" | also requires a 3 deg minimum elevation, which the text gives only for 5.3.1 |
| 11 | P.1239-4 3.2 decile location, MUFVariability | "for the local time and geographic latitude at the control point" | the mid-path point for every path length |
| 12 | path MUF90 and MUF10, MUFVariability | 3.6 defines no path value | the highest over the modes, each taken separately (OPMUF was aligned to the setting mode, D37) |
| 13 | 4, E-layer screening limit | "paths up to 4 000 km (see Table 1b)", but Table 1b) lists 2000 < D < 9000 | screening up to 4000 km |
| 14 | Lh local time, MSFSS | "local time t ...: mean values for the control points" against the Table 2 heading "Mid-path local time, t" | mid-path time at every point |
| 15 | fL night rule numbering, FindfL | "the larger of the values calculated from equations (32) and (35)" | equations (33) and (36), the only reading that makes sense |
| 16 | P.1239-4 eq. (18), FindfoE | the minimum is stated "At night" | max((12), (18)) at every hour |
| 17 | 7 eq. (45) Fa (D26) | Fa from P.372; P.842-5 Table 1 step 3 uses the power sum of FaA, FaM, FaG | P.372 total FamT for the median S/N, the power sum for the decile steps |
| 18 | 6 distance bands, MARP | where exactly 7000 and 9000 km fall is not stated | <= 7000 short, (7000, 9000) interpolated, >= 9000 long |

## 4. Open: probable defects not tied to the text

| # | Where | What |
|---|---|---|
| 1 | P372/Src/P372/NoiseDriver.c main | `noiseP.ManMadeNoise` is never set and `InitializeNoise` is not called, so `Noise()` reads an uninitialised value; return codes ignored; memory not freed. |
| 2 | P372/Src/P372/MakeNoise.c | output file `".\\MakeNoiseOut.txt"` is a Windows path; on Linux it creates a file with a backslash in its name. Fixed: `MakeNoiseOut.txt` in the working directory. |
| 3 | P372 Noise.c AtmosphericNoise, AtmosphericNoise_LT | the adjacent 4-hour block is always the next one, where the code's own description says previous, same or next depending on the hour. |
| 4 | P372 Noise.c Noise | FamT = min(upper-decile median, lower-decile median), commented "worst case"; override path sets FaM = input, FamT = -input. |
| 5 | MUFBasic.c CalcF2DMUF | Cd is clamped at dmax, but the fH/2 (1 - d/dmax) term goes negative when a hop exceeds the recalculated dmax at a control point. |
| 6 | Magfit.c | any height other than 100 or 300 km is written to the 100 km slot. Fixed: such a height now leaves the control point unchanged (the only callers pass 100 and 300). |
| 7 | ReadType13.c | reads the maximum gain but does not add it (types 11 and 14 do). |
| 8 | Include/P533.h | `path->B` is never stored; `struct Beam` is unused. |

## 5. Waiting on texts that are not in hand

P.372-17: the decile rule (Noise.c uses sigma_T = c sqrt(2 ln(alpha_T/gamma_T))
in place of the log-normal sigma when a component decile exceeds 12 dB;
the MATLAB port treats it as a maximum), and every P.372 equation and
table reference in `P372/Src/P372`.
P.1240: the Rop table and its season and day/night selection.
P.1144: the bilinear interpolation of the maps.
P.371: the Phi12 relation used for foE.
