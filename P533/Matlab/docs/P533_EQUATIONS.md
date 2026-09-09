# P.533-14 equations and data, as implemented by the MATLAB port

Source: Recommendation ITU-R P.533-14 (08/2019), Annex 1, with the
sections it invokes: P.1239-4 (08/2023), P.1240-2 (07/2015), P.842-5
(09/2013), P.1057 (log-normal statistics), P.372-17 (via `p372`).
Equation numbers are P.533-14 numbers unless prefixed. This is the
specification the `+p533` code is written from; every function's help
cites the rows it implements.

Symbols: D path length (km); d hop length D/n; n hops; R0 = 6371 km
(P.533 section 4 states 6 371; P.1239 eq. 8 uses 6371.2 for the magnetic
field scale only); f frequency (MHz); R12 sunspot number; all angles
radians in code, degrees in this document where the text uses degrees.

## 0. Conventions decided for the port

| Item | Decision | Source |
|---|---|---|
| R12 clamp | foF2 and M(3000)F2: linear interpolation/extrapolation of the R12 = 0 and 100 maps for 0..160, value at 160 above that. fL (eq. 33) uses the unclamped R12. All other uses of R12 (eq. 20, section 5.1 deltaM, Attachment 1 FR) use min(R12, 160) unless the text says otherwise. | sections 3.4, 5.3.2; P.1239 3.1 |
| Month and hour | month 1..12, hour 0..23 UTC at the API; the 15th of the month for solar geometry (P.533 Table 4 gives mid-month subsolar latitudes) | section 5.3.2 |
| Local time | UTC + longitude/15 h (continuous, not truncated) unless a table is indexed by whole hours | sections 5.2, 10.3 |
| Seasons for Lh | N: winter Dec-Feb, equinox Mar-May and Sep-Nov, summer Jun-Aug; S: winter and summer interchanged | section 5.2 Lh |
| Seasons for P.1239 Tables 2/3 | N: winter Nov-Feb, equinox Mar, Apr, Sep, Oct, summer May-Aug; S interchanged | P.1239 3.2 |
| Geomagnetic pole for Lh | 78.5 N, 68.2 W (Earth-centred dipole) | section 5.2 Lh |
| Geomagnetic latitude for P.842 Table 2 (>= 60 deg test) | P.1239 Fig. 2 / eq. in P.1239 section 5: pole 78.3 N, 69.0 W | P.842 Table 2 note (1) |
| Magnetic field model | P.1239 eqs (5)-(11), epoch 1960 sixth-order Gauss coefficients (taken from Magfit.c, the only machine-readable source), evaluated at 100 km (absorption, dip for section 10.3) and 300 km (gyrofrequency for MUF and long paths) | P.1239 section 2; P.533 3.4, 5.2, 5.3 |
| Modified dip | X = atan(I / sqrt(cos lambda)) at 100 km | P.1239 eq. (4); P.533 5.2 p |

## 1. Control points (section 2, Table 1)

Great-circle geometry: distance by the haversine formula, intermediate
points by spherical interpolation, bearings by atan2. Long path:
D_long = 2 pi R0 - D_short, bearings reversed.

| Purpose | 0 < D <= 2000 | 2000 < D <= 4000 (E) / <= dmax (F2) | D > dmax | dmax < D < 9000 |
|---|---|---|---|---|
| a) basic MUF, fH | M | E: T+1000, R-1000; F2: M | F2: T+d0/2, R-d0/2 | |
| b) E-layer screening (F2 modes, D < 9000) | M | T+1000, R-1000 (2000 < D < 9000) | | |
| c) mirror reflection height (F2) | M (D <= dmax) | | | T+d0/2, M, R-d0/2 |
| d) absorption, fH at 100 km | M | E: T+1000, M, R-1000; F2 (<= dmax): T+1000, M, R-1000 | | F2: T+1000, T+d0/2, M, R-d0/2, R-1000 |

d0 = hop length of the lowest-order mode; dmax computed at M (eq. 5).
Section 5.2 Li replaces the Table 1d) absorption points by penetration
points (two per hop, 90 km penetration, 300 km reflection); Table 1d)
remains the source for the gyrofrequency and for Lh "mean values for the
control points".

## 2. Ionospheric characteristics at a point

| Quantity | Method | Source |
|---|---|---|
| foF2, M(3000)F2 | grid tables ionosNN.bin, 1.5 deg, 24 h, R12 = 0 and 100; bilinear interpolation (P.1144 Annex 1) in lat/lng at the UTC hour; linear in R12 (see section 0) | P.533 3.4; P.1239 3.1 |
| foE | P.1239 eqs (12)-(18): (foE)^4 = A B C D; A = 1 + 0.0094(Phi - 66) with Phi from R12 (P.371: Phi = 63.7 + 0.728 R12 + 0.00089 R12^2); B = cos^m N, N = min(abs(lambda - delta), 80 deg), m = -1.93 + 1.92 cos lambda (abs lambda < 32) or 0.11 - 0.49 cos lambda; C = X + Y cos lambda with (23, 116) or (92, 35); D: chi <= 73: cos^p chi; 73 < chi < 90: cos^p(chi - dchi), dchi = 6.27e-13 (chi - 50)^8 deg; chi >= 90: max of (0.072)^p exp(-1.4 h) and (0.072)^p exp(25.2 - 0.28 chi), h = hours after sunset; polar winter (sun does not rise): (17e). p = 1.31 (abs lambda <= 12) else 1.20. foE^4 >= 0.004 (1 + 0.021 Phi)^2 | P.1239 section 4 |
| Solar geometry | declination, equation of time, hour angle, zenith angle chi, sunrise/sunset, local noon for the 15th of the month; used for foE, eq. (20) chi and chi_noon, Attachment 1 local time. For eq. (33)-(35) the text's simplified form is used: delta = Table 4 subsolar latitude, eta = (UTC/12 - 1) pi + longitude | P.533 5.2, 5.3.2 |
| Magnetic dip I, gyrofrequency fH | P.1239 eqs (5)-(11) at 100 and 300 km | P.1239 section 2 |
| foF2 deciles delta_l, delta_u | P.1239 Tables 2 and 3 by local time (h), latitude (abs, 0..90 step 5), season, R12 band (< 50, 50..100, > 100); bilinear interpolation in time and latitude | P.1239 3.2 |

## 3. Part 1: frequency availability

| Eq. | Formula | Notes |
|---|---|---|
| (1) | nE(D)MUF = foE sec i110, i110 at 110 km for hop d = D/n | foE at M (D <= 2000) or the lower of T+1000, R-1000 (2000 < D <= 4000); E modes only for D <= 4000; path E MUF = lowest-order mode n0 |
| (2) | hr = min(1490 / M(3000)F2 - 176, 500) km at M | for n0 of F2 (geometry with 3 deg minimum elevation) |
| (3) | nF2(D)MUF = [1 + (Cd/C3000)(B - 1)] foF2 + (fH/2)(1 - d/dmax) | d = D/n <= dmax; fH at 300 km at the Table 1a) point |
| (4) | Cd = 0.74 - 0.591 Z - 0.424 Z^2 - 0.090 Z^3 + 0.088 Z^4 + 0.181 Z^5 + 0.096 Z^6, Z = 1 - 2d/dmax | C3000 = Cd at d = 3000 |
| (5) | dmax = 4780 + (12610 + 2140/x^2 - 49720/x^4 + 688900/x^6)(1/B - 0.303) | x = max(foF2/foE, 2); for the basic MUF dmax <= 4000; for Mn/Mn0 (eq. 8) dmax is recomputed at the control point and may exceed 4000 |
| (6) | B = M(3000)F2 - 0.124 + [M(3000)F2^2 - 4][0.0215 + 0.005 sin(7.854/x - 1.9635)] | |
| (7),(8) | D > dmax: nF2(D)MUF = n0F2(dmax)MUF Mn/Mn0, Mn/Mn0 = nF2(D)MUF / n0F2(D)MUF from eq. (3) | evaluated at T+d0/2 and R-d0/2, lower value taken |
| (9) | f < MUF(50): Fprob = min(1.3 - 0.8 / (1 + (1 - f/MUF50)/(1 - delta_l)), 1) | E modes: delta_l = 0.95 |
| (10) | f > MUF(50): Fprob = max(0.8 / (1 + (f/MUF50 - 1)/(delta_u - 1)) - 0.3, 0) | E modes: delta_u = 1.05 |
| 3.7 | OPMUF(F2) = BMUF Rop, Rop from P.1240 Table 1 by season, day/night, EIRP <= 30 or > 30 dBW; OPMUF(E) = BMUF; OPMUF10/90 = OPMUF delta_u / delta_l (E: 1.05, 0.95) | P.1240 Table 1: Summer N 1.20 D 1.10, Equinox N 1.25 D 1.15, Winter N 1.30 D 1.20 for <= 30 dBW; 1.25/1.15, 1.30/1.20, 1.35/1.25 for > 30 dBW |
| (11),(12) | fs = 1.05 foE sec i, i = asin(R0 cos DeltaF / (R0 + 110)) | foE at M (D <= 2000) or the higher of T+1000, R-1000; F2 modes with fs >= f are screened; D <= 4000 only |
| Path BMUF | higher of the lowest-order E and F2 MUFs | 3.1 |
| MUF deciles | MUF50 = BMUF; MUF90 = MUF50 delta_l; MUF10 = MUF50 delta_u | 3.6 |

## 4. Part 2: median sky-wave field strength

### 4.1 Elevation and reflection height (section 5.1)

| Eq. | Formula |
|---|---|
| (13) | Delta = atan(cot(d/2R0) - (R0/(R0 + hr)) cosec(d/2R0)) |
| hr E | 110 km |
| hr F2 | x = foF2/foE, y = max(x, 1.8), dM = 0.18/(y - 1.4) + 0.096 (R12 - 25)/150, H = 1490/(M(3000)F2 + dM) - 316 |
| (14) x > 3.33, xr = f/foF2 >= 1 | hr = min(h, 800); h = A1 + B1 2.4^(-a) (B1, a >= 0) else A1 + B1; A1 = 140 + (H - 47) E1; B1 = 150 + (H - 17) F1 - A1; E1 = -0.09707 xr^3 + 0.6870 xr^2 - 0.7506 xr + 0.6; F1 = -1.862 xr^4 + 12.95 xr^3 - 32.03 xr^2 + 33.50 xr - 10.91 (xr <= 1.71) else 1.21 + 0.2 xr; a = (d - ds)/(H + 140); ds = 160 + (H + 43) G; G = -2.102 xr^4 + 19.50 xr^3 - 63.15 xr^2 + 90.47 xr - 44.73 (xr <= 3.7) else 19.25 |
| (15) x > 3.33, xr < 1 | hr = min(h, 800); h = A2 + B2 b (B2 >= 0) else A2 + B2; A2 = 151 + (H - 47) E2; B2 = 141 + (H - 24) F2 - A2; E2 = 0.1906 Z^2 + 0.00583 Z + 0.1936; F2 = 0.645 Z^2 + 0.883 Z + 0.162; Z = max(xr, 0.1); b = -7.535 df^4 + 15.75 df^3 - 8.834 df^2 - 0.378 df + 1; df = min(0.115 d / (Z (H + 140)), 0.65) |
| (16) x <= 3.33 | hr = min(115 + H J + U d, 800); J = -0.7126 y^3 + 5.863 y^2 - 16.13 y + 16.07; U = 8e-5 (H - 80)(1 + 11 y^-2.2) + 1.2e-3 H y^-3.6 |
| location | D <= dmax: at M; longer: mean over Table 1c) points |

### 4.2 Paths up to 9000 km (section 5.2)

Modes: up to 3 E (D <= 4000; lowest order with hop <= 2000 and next two)
and up to 6 F2 (lowest order with hop <= dmax and next five) whose fs < f.
hr for F2 modes from eq. (2) at M (D <= dmax) or at the Table 1c) point
with the lower foF2 (dmax < D <= 9000).

| Eq. | Formula |
|---|---|
| (17) | Ew = 136.6 + Pt + Gt + 20 log f - Lb, Pt dB(1 kW), Gt dBi at bearing and Delta |
| (18) | Lb = 32.45 + 20 log f + 20 log p' + Li + Lm + Lg + Lh + Lz |
| (19) | p' = 2 R0 sum_{1..n} sin(d/2R0) / cos(Delta + d/2R0) |
| (20) | Li = (1 + 0.0067 R12) sec i sum_{j=1..m} ATjnoon / (f + fLj)^2 * F(chi_j)/F(chi_jnoon) * phi_n(fv/foE_j); m = 2n penetration points at 90 km with 300 km reflection; i at 110 km |
| (21) | F(chi) = max(cos^p(0.881 chi), 0.02), chi <= 102 deg |
| (22) | fv = f cos i |
| (23) | fL = abs(fH sin I) at 100 km |
| ATnoon | Fig. 1 (geographic latitude, month) at R12 = 0: REC533 table ATNO[9][29] from the C code |
| phi_n | Fig. 2: REC533 polynomial fit from the C code |
| p | Fig. 3 (modified dip at 100 km, month): REC533 polynomial fit from the C code |
| (24)-(26) | Lm = 0 (f <= fb); E: min(130 [(f/fb) - 1]^2, 81); F2: min(36 [(f/fb) - 1]^0.5, 62) |
| (27) | Lg = 2(n - 1) |
| Lh | Table 2 (typed from the PDF, two ranges <= 2500 / > 2500 km, 3 seasons, 8 Gn bands, 8 local-time bins); 0 for Gn < 42.5; Gn and t as mean values over the Table 1d) points |
| Lz | 8.72 dB |
| (28) | Es = 10 log sum 10^(Ew/10) over unscreened modes |

### 4.3 Paths longer than 7000 km (section 5.3)

| Eq. | Formula |
|---|---|
| 5.3.1 | nM = minimum equal hops with dM <= 4000; Delta from (13) with hr = 300; while Delta < 3 deg add a hop; control points at dM/2 from each end |
| (29) | fBM = fz + (f4 - fz) fD; f4 = 1.1 foF2 M(3000)F2; fz = foF2 + fH/2 |
| (30) | fD = ((((((C6 dM + C5) dM + C4) dM + C3) dM + C2) dM + C1) dM + C0) dM, C6..C0 = -2.40074637494790e-24, 25.8520201885984e-21, -92.4986988833091e-18, 102.342990689362e-15, 22.0776941764705e-12, 87.4376851991085e-9, 29.1996868566837e-6 |
| (31) | fM = K fBM, per control point, lower value taken |
| (32) | K = 1.2 + W fBM/fBMnoon + X [ (fBMnoon/fBM)^(1/3) - 1 ] + Y (fBMmin/fBMnoon)^2; W, X, Y from Table 3 (E-W 0.1, 1.2, 0.6; N-S 0.2, 0.2, 0.4) linearly interpolated on the path azimuth at the path centre |
| 5.3.2 | nL equal hops with dL <= 3000; penetration points at 90 km, reflection 300 km; m = 2 nL |
| (33) | fL = (5.3 [ (1 + 0.009 R12) sum cos^0.5 chi / (cos i90 ln(9.5e6/p')) ]^0.5 - fH)(Aw + 1); cos^0.5 chi = 0 for chi > 90; R12 unclamped |
| (34),(35) | cos chi = sin phi_m sin delta + cos phi_m cos delta cos eta; delta from Table 4 (J -21.2, F -12.7, M -2.2, A 9.7, M 18.8, J 23.3, J 21.6, A 14.1, S 3.1, O -8.4, N -18.4, D -23.3 deg); eta = (UTC/12 - 1) pi + y_m |
| Aw | Table 5 at 60 deg: N: .30 .15 .03 .00 .00 .00 .00 .00 .01 .03 .15 .30; S: .00 .00 .00 .03 .15 .30 .30 .15 .03 .00 .00 .00; 0 for abs lat <= 30 and at 90, linear to the peak at 60 |
| (36) | fLN = sqrt(D/3000); fL(hour) = max(eq. 33, fLN) for all 24 hours |
| (37),(38) | tr = first hour with fL(tr) < 2 fLN and fL(tr - 1) > 2 fLN; fL(tr) = e^-0.23 fL(tr - 1)(dt (1 - e^-0.23) + e^-0.23), dt = (2 fLN - fL(tr)) / (fL(tr - 1) - fL(tr)); fL(tr + n) = fL(tr + n - 1) e^-0.23, n = 1..3; new values replace old only if larger; fL for the current hour is then selected |
| (39) | Etl = E0 [1 - (fM + fH)^2 / ((fM + fH)^2 + (fL + fH)^2) [ (fL + fH)^2 / (f + fH)^2 + (f + fH)^2 / (fM + fH)^2 ] ] - 30 + Pt + Gtl + Gap - Ly |
| (40) | E0 = 139.6 - 20 log p', p' from (19) and (13) with hr = 300 |
| (41) | Gap = min(10 log (D / (R0 abs sin(D/R0))), 15) |
| Ly | -0.14 dB |
| Gtl | largest transmit gain at the bearing over elevations 0..8 deg |
| fH | mean of the gyrofrequencies at the two control points |

### 4.4 7000 to 9000 km (section 5.4) and receiver power (section 6)

| Eq. | Formula |
|---|---|
| (42) | Ei = 100 log Xi, Xi = Xs + (D - 7000)/2000 (Xl - Xs), Xs = 10^(0.01 Es), Xl = 10^(0.01 El); path BMUF = lower of eq. (3) at the two Table 1a) points |
| (43) | Prw = Ew + Grw - 20 log f - 107.2 dBW, Grw at the mode's elevation |
| (44) | Pr = 10 log sum 10^(Prw/10) (D <= 7000) |
| > 9000 | Pr from (43) with El and Grw = largest receive gain over 0..8 deg |
| 7000..9000 | Pr from (42) applied to the powers corresponding to Es and El |

## 5. Part 3: system performance

| Eq. | Formula |
|---|---|
| (45) | S/N = Pr - Fa - 10 log b + 204; P.842 Table 1 step 3 gives Fa as the power sum of FaA, FaM, FaG (the port uses the P.372 combined FamT from `p372.noise`, which is the same quantity with the P.372 section 8 combination; the difference is documented) |
| section 8 | within-hour signal deciles: upper 5 dB, lower 8 dB; day-to-day signal deciles from P.842 Table 2 by f/BMUF and geomagnetic latitude (< 60 / >= 60 deg, any point between T+1000 and R-1000); noise deciles from P.372; galactic +-2 dB |
| P.842 Table 1 step 6 | DuSN = rss(DuSd, DuSh, 10 log [ (10^(FaA/10) + 10^(FaM/10) + 10^(FaG/10)) / (10^((FaA - DlA)/10) + 10^((FaM - DlM)/10) + 10^((FaG - DlG)/10)) ]) |
| step 9 | DlSN = rss(DlSd, DlSh, 10 log [ (10^((FaA + DuA)/10) + ...) / (10^(FaA/10) + ...) ]) |
| (46) | S/N90 = S/N50 - DlSN; other percentages from P.1057 log-normal: S/Nxx = S/N50 - z(xx) DlSN/1.282 (xx > 50) or + z(1 - xx) DuSN/1.282 (xx < 50), z the standard normal quantile |
| section 9 | LUF: lowest f (0.1 MHz steps) with S/N50 >= S/Nr (reported for information) |
| P.842 Table 1 step 11 | BCR = min(130 - 80/(1 + (S/N - S/Nr)/DlSN), 100) for S/N >= S/Nr; max(80/(1 + (S/Nr - S/N)/DuSN) - 30, 0) otherwise |
| P.842 Table 2 | LD/UD (< 60): 8/6 (<= 0.8), 12/8 (1.0), 13/12 (1.2), 10/13 (1.4), 8/12 (1.6), 8/9 (1.8), 8/9 (2.0), 7/8 (3.0), 6/7 (4.0), 5/7 (>= 5.0); (>= 60): 11/9, 16/11, 17/12, 13/13, 11/12, 11/9, 11/9, 9/8, 8/7, 7/7; ratio bands as tabulated (nearest tabulated value below; the port interpolates none, matching the step table) |
| (47) | tau = (p'/c) 1e3 ms, p' from (13) and (19) with hr from 5.1 |
| 10.2.3 steps 1-5 (D <= 9000) | dominant mode Ew; active modes within A dB; first arriving mode and modes within Tw of it; power sum of in-window modes -> S; BCR via Table 1 with S/Nr; modes beyond Tw treated as interferers via P.842 Table 3 with protection ratio A and day-to-day deciles 0: S/I = S - 10 log sum 10^((Ii + A)/10); DuSI, DlSI from within-hour deciles only (5/8 and 8/5); ICR as step 12; MIR = ICR; DCR = BCR MIR / 100 |
| > 9000 | composite signal; time spread 3 ms at 7000 km rising linearly to 5 ms at 20000 km; system fails if Tw < spread |
| (48) | DCR = BCR MIR (1 - probocc)/100 (probocc a probability) |
| Attachment 1 | pTspread = 0.056 pm exp(-(tau - tau_m)^2 / (2 Tspread^2)), Tspread = 1 ms; pFspread = 0.056 pm exp(-(f - fm)^2 / (2 Fspread^2)), Fspread = 3 Hz; scattering counted when pTspread or pFspread at the window edges exceeds Ew - A; probocc = F_lambda F_T F_R F_S at the F-mode control points (largest taken) |
| F_lambda | 1 (abs dip < 15 deg); ((25 - abs dip)/10)^2 ((abs dip - 10)/5) (15..25); 0 (> 25); dip in degrees at 100 km |
| F_T | 1 (00..03); ((7 - T)/4)^2 ((T - 1)/2) (03..07); 0 (07..19); (T - 19)^2 (41 - 2T) (19..20); 1 (20..24); T local time (h) |
| F_R | min(0.1 + 0.008 R12, 1) |
| F_S | 0.55 + 0.45 sin(60 deg (m - 1.5)), m = month number 1..12 |

## 6. Inputs read from files

See `P533_PORT_PLAN.md` section 2 and, once written, `P533/docs/DATA_FORMATS.md`.
