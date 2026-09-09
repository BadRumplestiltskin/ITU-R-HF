# Differences between the ITU P533 C code (14.2) and P.533-14, and what the port does

Each item was checked against the Recommendation text (P.533-14 and the
cited P.1239-4, P.1240-2, P.842-5). "Port" states the behaviour of the
MATLAB implementation, which follows the text. The C behaviour is
recorded so that tolerance failures in the C comparison can be tagged.
Items marked (upstream) are worth reporting to ITU-R Study Group 3.

| Id | Section | C code (file:line) | Recommendation | Port | Numerical effect |
|---|---|---|---|---|---|
| D01 | 5.2 Lm, eq. (25) | E modes: min(46 sqrt(f/fb - 1) + 5, 58) (MedianSkywaveFieldStrengthShort.c:212) | min(130 (f/fb - 1)^2, 81) | text | large for f > fb: at f/fb = 1.2, C 25.6 dB, text 5.2 dB (upstream) |
| D02 | 5.2 Lm, eq. (26) | F2: min(36 sqrt(f/fb - 1) + 5, 60) for d <= 3000, min(70 (f/fb - 1) + 8, 80) beyond (…Short.c:381-384) | min(36 sqrt(f/fb - 1), 62), no distance dependence | text | +5 dB offset and different form for long hops (upstream) |
| D03 | 5.2 Lz | 9.14 dB (…Short.c:19) | 8.72 dB | text | 0.42 dB in every short-path field strength (upstream: value changed in -14) |
| D04 | 5.3.3 Ly | -0.17 dB (…Long.c:17) | -0.14 dB | text | 0.03 dB on long paths |
| D05 | 5.1 eq. (14) G | -2.102 xr^4 + 19.50 xr^3 - 63.15 xr^2 - 44.73 (ELayerScreeningFrequency.c:143) | ... - 63.15 xr^2 + 90.47 xr - 44.73 | text | skip distance ds wrong by (H + 43) 90.47 xr km in C; changes hr for x > 3.33, xr >= 1 (upstream) |
| D06 | 5.3.2 Table 5 | Aw September northern = 0.00 (…Long.c Aw table) | 0.01 | text | fL in September at high northern latitudes |
| D07 | 3.7, P.1240 Table 1 | EIRP row index always 0; the > 30 dBW row is also mis-ordered by season (MUFOperational.c:38, 63-68) | two rows by EIRP <= 30 / > 30 dBW | text, EIRP = Pt + Gt at the path elevation | OPMUF 5 % higher for EIRP > 30 dBW (upstream) |
| D08 | P.1239 eq. (17d,e) polar night | southern test lat < +72.56 deg selects (17e) for almost all latitudes (CalculateCPParameters.c:493-495) | (17e) when the Sun does not rise | text: (17e) when sunrise does not exist for the day, either hemisphere; otherwise greater of (17d) and (17e) | foE at night |
| D09 | Attachment 1 F_R | 0.1 + 0.008 max(R12, 160) (CircuitReliability.c:814) | min(0.1 + 0.008 R12, 1) | text | probocc always >= 1.38 in C, i.e. OCRs wrong for every case (upstream) |
| D10 | Attachment 1 F_lambda | dip in radians compared with 25, 10, 5 in the 15..25 deg band (CircuitReliability.c:948) | degrees | text | scattering probability in the transition band (upstream) |
| D11 | Attachment 1 indexing | PTspread indexed by mode in one place, by control point in another; F2 mode index not offset by 3 (CircuitReliability.c:753, 762, 781, 832-891) | one value per F mode, probability per control point | clean data model: per-mode spread, per-control-point probability, maximum taken | undefined in C |
| D12 | 10.2.2 eq. (47) | tau in ms but TW/1000 added; slant range with delta - psi (CircuitReliability.c:592, 605) | tau = p'/c 1e3 with p' from (19), i.e. delta + psi; Tw in ms | text | window test wrong in C |
| D13 | 10.2.3 step 4 | signal sum squares the mode powers then takes the square root (CircuitReliability.c:653, 667) | power summation of in-window modes | plain power sum | dB level of S |
| D14 | 3.5.2.2 eq. (7), (8) | eq. (3) evaluated with the unclamped d/dmax beyond dmax (MUFBasic.c:387-431) | Mn/Mn0 ratio from eq. (3) with dmax recomputed at the control point | text | higher-order MUFs beyond dmax |
| D15 | 5.2.1 hr choice | "SmallestCPfoF2" selection loop is not a sort and cannot return index 0 (…Short.c:1087) | control point with the lower foF2 | min() | hr for dmax < D <= 9000 |
| D16 | 10.2.3 mode ordering | ModeSort double loops are not valid sorts (CircuitReliability.c:428) | modes by strength / arrival | sort() | dominant and first-arriving mode identification |
| D17 | 8, P.1057 | SNRXX from a 50-entry table, out of range for SNRXXp outside 1..99, not computed when no interferer (CircuitReliability.c:116, 370, 414) | log-normal distribution | erfinv-based quantile, always computed | |
| D18 | 3.6, P.1239 Tables 2/3 | latitude index rolls over across the pole (MUFVariability.c:187-207) | tables for 0..90 deg | abs(lat) clamped to 0..90 | high-latitude deciles |
| D19 | 7, eq. (45) | P533 accepts only RTN_NOISEOK, rejecting the P372 user-override return (P533.c:264) | Fa from P.372 | `p372.noise` override supported | |
| D20 | 5.3.2 eq. (34), (35) | chi from full solar geometry with equation of time for all hours (…Long.c:594) | Table 4 declination and eta = (UTC/12 - 1) pi + longitude | text for fL; full geometry retained for section 5.2 where the text asks for the equation of time | small fL differences |
| D21 | 5.2 Li absorption points | penetration points (PEN = TRUE) | penetration points, two per hop, 90 km / 300 km | same; the dead control-point branch is not ported | none |
| D22 | 5.2 Lh pole | 78.5 N, 68.2 W in code, comment says 69.2 W (Geometry.c:96-99) | 78.5 N, 68.2 W | 78.5 N, 68.2 W | none (comment only) |
| D23 | P.842 Table 2 note (1) | geomagnetic latitude with the Lh pole, tested only at M, T+1000, R-1000 (CircuitReliability.c:176-187) | any point between T+1000 and R-1000 reaching 60 deg, P.1239 Fig. 2 (pole 78.3 N, 69.0 W) | maximum geomagnetic latitude along the great circle between T+1000 and R-1000, P.1239 pole | day-to-day deciles near 60 deg |
| D24 | 3.3 E MUF | E modes considered when hop <= 2000 for n0 only (MUFBasic.c:191-239) | E modes for D <= 4000; foE from Table 1a) | text | none expected |
| D25 | 5.4 endpoints | interpolation only for 7000 < D < 9000 strictly | 7000..9000 both methods used | D = 7000 gives Es, D = 9000 gives El via eq. (42) weights 0 and 1 | continuity at the endpoints |
| D27 | 3.4, P.1144 bilinear interpolation | in the southern and western hemispheres the fractional row/column weight is measured from the truncated (nearer-to-zero) grid line but applied to the far line, i.e. the point is mirrored within the cell (CalculateCPParameters.c:147-335) | bilinear interpolation between the enclosing grid points | correct bilinear weights (p533.ionosphericParameters) | foF2 and M(3000)F2 errors up to the change across one 1.5 deg cell, S and W hemispheres only (upstream) |
| D28 | P.1239 eq. (17d) | hours after sunset counted from UTC hour + 1 (CalculateCPParameters.c:472); control point "local time" set to the UTC hour (CalculateCPParameters.c:651) | h = hours after sunset; local time at the control point | h from the UTC hour and the sunset time; ltime = UTC + longitude/15 | night-time foE, and every table indexed by local time (D29) |
| D29 | 3.6, 3.7, 10.3 local time | P.1239 decile tables, the Rop day/night test and Attachment 1 F_T are indexed with CP.ltime, which the C code sets to the UTC hour (see D28) | local time at the control point | local mean time | MUF deciles and OPMUF for longitudes away from Greenwich (upstream) |
| D30 | 5.2 eq. (20) | Li = n (1 + 0.0067 R12) sec i AT_mean / (f + fL_mean)^2 with AT_mean the mean of the absorption term over the 2n penetration points and fL the mean over the Table 1d) points (MedianSkywaveFieldStrengthShort.c:205, 373, 1262-1340) | Li = (1 + 0.0067 R12) sec i sum over the m = 2n penetration points of ATjnoon/(f + fLj)^2 F(chi_j)/F(chi_jnoon) phi_n | **hop-mean reading by default** (`path.liRule = 'hopmean'`: n times the mean over the points, each point with its own fL); `'sum'` gives the printed form | the printed sum is twice the hop-mean value. Against the CCIR D1 databank (8 084 measured hourly medians, docs/VALIDATION.md) the printed sum is biased by -5.5 dB overall and -10 dB below 5 MHz (rms 16.6 dB), the hop-mean reading is unbiased (rms 12.9 dB) and equals the ITU code's accuracy (rms 12.7 dB). The -14 text should say the summed terms are per hop, or divide by two (upstream) |
| D31 | 5.2 Li penetration points | located from the elevation of the mode's own reflection height (110 km for E modes, hr for F2), MedianSkywaveFieldStrengthShort.c:1274-1278 | "assuming a fixed reflection height of 300 km and a penetration height of 90 km" | 300 km (`liRule = 'reference'` reproduces the C geometry) | E-mode absorption: points move by several hundred km, a few dB at low frequencies |
| D32 | 7 eq. (45), 10.2.3 | the wanted signal S entering the S/N is taken from the digital mode-window logic for every modulation: with A = 0 and Tw = 0 (analog defaults) only the dominant mode remains, so S < Pr whenever several modes propagate (CircuitReliability.c:611-667) | S/N = Pr - Fa - 10 log b + 204 with Pr the power sum of all modes (sections 6 and 7); the window logic applies to digital systems (10.2) | Pr for analog; in-window power sum for digital | analog SNR up to a few dB lower in C (upstream) |
| D26 | 7 noise combination | Fa = power sum of FaA, FaM, FaG (P.842 Table 1 step 3) | same | `p372.noise` FamT (P.372 section 8 log-normal combination) is used as Fa and the P.842 step 3 sum is used for the decile terms; documented | up to a few tenths of a dB |

Items confirmed as not deviations: R0 = 6371 km; penetration-point
absorption; K-factor and fD polynomial; e^-0.23 decay; E0 = 139.6; the
136.6, 32.45 and 107.2 constants; P.1240 Table 1 values for <= 30 dBW.

Sources of numerical tables not given as formulas in the text (taken from
the C code, which carries the REC533 fits): Fig. 1 ATnoon (9 x 29 table),
Fig. 2 phi_n (piecewise polynomials), Fig. 3 p (two 6 x 2 x 7 coefficient
sets), and the 1960-epoch Gauss coefficients of P.1239 section 2.
