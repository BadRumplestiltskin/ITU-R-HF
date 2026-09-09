# Algorithm overview

The implementation follows Recommendation ITU-R P.533-14 Annex 1. The
complete equation list with the port's conventions is in
`../../docs/P533_EQUATIONS.md`; this page describes the flow and where
each step lives.

## Sequence (`p533.run`)

1. `validatePath`: input ranges.
2. `initializePath`: great-circle distance (long path = circumference
   minus the short distance), bearings, control points M and, for
   D >= 2000 km, T+1000 and R-1000 (Table 1), season at the mid-point.
   Each control point gets foF2 and M(3000)F2 (grid interpolation, linear
   in R12 up to 160), foE (P.1239 section 4), magnetic dip and
   gyrofrequency at 100 and 300 km (P.1239 section 2) and the solar
   geometry for the 15th of the month (`pointParameters`).
3. Part 1: `mufBasic` (E-mode MUF eq. 1, F2 MUF eqs 2-8 with the d0/2
   control points beyond dmax), `mufVariability` (P.1239 deciles, eqs
   9-10), `mufOperational` (P.1240 Table 1 with e.i.r.p. and day/night at
   the mid-point), `eLayerScreeningFrequency` (eqs 11-16).
4. Part 2: `medianSkywaveFieldStrengthShort` for D <= 9000 km (mode
   selection of 5.2.1; eqs 17-28 with `absorptionLoss`, `findLh`, Lz =
   8.72 dB), `medianSkywaveFieldStrengthLong` for D >= 7000 km (fM from
   eqs 29-32 with 24-hour histories at the two control points, fL from
   eqs 33-38 at the 90 km penetration points, Etl from eqs 39-41),
   `between7000kmand9000km` (eq. 42), `medianAvailableReceiverPower`
   (eqs 43-44, dominant mode, receive gain).
5. Noise at the receiver from `p372.noise` (P.372-17).
6. Part 3: `circuitReliability`: S/N (eq. 45), deciles (section 8 and
   P.842 Table 1 with Table 2 day-to-day values by f/BMUF and geomagnetic
   latitude), S/N at the required percentage (P.1057 log-normal), BCR
   (P.842 Table 1 step 11), digital mode window logic (10.2.3 steps 1-5)
   with multimode interference via P.842 Table 3, equatorial scattering
   (10.3, Attachment 1), OCR = BCR MIR/100 and OCRs = OCR (1 - probocc).

## Absorption loss and the `liRule` option

Equation (20) of P.533-14 sums the absorption term over the m = 2n
penetration points. Read literally this is twice the absorption of the
P.533-12 control-point formula (n hops times a mean), and the CCIR D1
databank shows the literal sum to be biased by -5 to -10 dB while the
hop-mean reading is unbiased (`VALIDATION.md`). The port therefore uses
`path.liRule = 'hopmean'` by default; `'sum'` gives the printed form and
`'reference'` adds the ITU C conventions (`p533.rules`).

## Numerical tables

Figures 1, 2 and 3 (absorption factor, penetration factor, diurnal
exponent) are given only graphically in the Recommendation. Their
numerical forms are the REC533 fits carried by the ITU software (`tables`,
`absorptionFactor`, `penetrationFactor`, `diurnalAbsorptionExponent`).
Table 2 (Lh), Table 5 (Aw), Table 4 (subsolar latitude), P.1240 Table 1
and P.842 Table 2 are typed from the Recommendations. The 1960 Gauss
coefficients of the magnetic field model come from the ITU software
(`magfit`), which is what P.1239 section 2 requires.

## Conventions worth knowing

- Local time is UTC + longitude/15 h (continuous). The C code uses the
  UTC hour for the "local time" of control points (deviations D28, D29).
- Seasons: P.533 section 5.2 (Lh, Rop) and P.1239 section 3.2 (decile
  tables) define seasons differently; `seasonOf` implements both.
- Geomagnetic pole 78.5 N 68.2 W for Lh; 78.3 N 69.0 W (P.1239) for the
  P.842 Table 2 latitude test.
- Modes are stored by hop count (`Md_F2(k).hops`), the lowest order first.
