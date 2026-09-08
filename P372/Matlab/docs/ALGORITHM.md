# Algorithm description

This document explains what the `p372` package computes and how the
implementation maps onto Recommendation ITU-R P.372-14 and the ITU-R
Study Group 3 reference C code. Symbols follow the Recommendation.

## 1. Quantities

All noise levels are external noise figures `Fa` in dB above `k T0 b`
(`k` Boltzmann's constant, `T0` = 290 K, `b` bandwidth in Hz). For each
source the model gives the median `Fam` and two decile deviations:

| Symbol | Meaning |
|---|---|
| `Du` | ratio of the upper decile to the median, dB |
| `Dl` | ratio of the median to the lower decile, dB |
| `sigma_Fam`, `sigma_Du`, `sigma_Dl` | standard deviations of the above across the data set, dB (atmospheric only) |
| `V_d`, `sigma_V_d` | voltage deviation (r.m.s. / average envelope), dB, and its standard deviation (atmospheric only, figure c) |

## 2. Atmospheric noise (`p372.atmosphericNoise`, `p372.getFamParameters`)

The Recommendation presents atmospheric noise as maps of `Fam` at 1 MHz
for four seasons and six 4-hour local-time blocks (figures a), curves of
frequency dependence (figures b) and variability (figures c). The ITU
software uses the numerical representation of those maps from NBS
Technical Note 318 (Lucas and Harper, 1965), carried over from the
REC533 Fortran program.

Per time block `tb` (0..5) and month the coefficient file provides
`fakp(tb, 1:16, 1:29)`, `fakabp(tb, 1:2)`, `fam(row, 1:14)` and
`dud(1:5, row, 1:5)`, where `row = tb + 1` in the northern hemisphere and
`tb + 7` in the southern.

**Step 1: `Fam` at 1 MHz.** With `q = lambda_E / 2` (half the east
longitude in radians, 0..2 pi):

    ZZ(j) = sum_{k=1..15} sin(k q) fakp(tb,k,j) + fakp(tb,16,j),  j = 1..29
    q'    = phi + pi/2                                         (phi = latitude)
    Fam1  = sum_{j=1..29} sin(j q') ZZ(j) + fakabp(tb,1) + fakabp(tb,2) q'

**Step 2: frequency scaling** (`p372.famFrequencyVariation`). With
`u = (8 * 2^log10(f) - 11) / 4` two Horner polynomials `pz(u)` and
`px(u)` are built from `fam(row, 1:7)` and `fam(row, 8:14)`. Evaluating
first at `u = -0.75` (1 MHz) fixes `cz = Fam1 (2 - pz) - px`; then
`Fam(f) = cz pz(u_f) + px(u_f)`.

**Step 3: deciles and sigmas.** Each of the five statistics is a quartic
polynomial in `x = log10(f)` with coefficients `dud(stat, row, 1:5)`.
`f` is clamped at 20 MHz for all statistics and at 10 MHz for
`sigma_Fam`, because the Recommendation's curves end there.

**Step 4: time interpolation.** The receiver local mean time is
`LMT = hour_UTC + int(lambda / 15 deg)` (truncated toward zero). The
statistics are evaluated for the block containing LMT and the following
block, converted to linear power, blended with weight
`mod(LMT, 4) / 4`, and converted back to dB.

`p372.atmosphericNoiseLT` is the same computation with LMT given
directly and all six statistics returned; it is what the figures use.

## 3. Man-made noise (`p372.manMadeNoise`)

P.372-17 Part 6, section 6.1.1 (outdoor, additive white Gaussian noise),
equation (17) `Fam = c - d log10(f)` with the constants of Table 1:

| Category | c | d | Du | Dl | Source of deciles |
|---|---|---|---|---|---|
| City (curve A) | 76.8 | 27.7 | 11.0 | 6.7 | Table 2 |
| Residential (curve B) | 72.5 | 27.7 | 10.6 | 5.3 | Table 2 |
| Rural (curve C) | 67.2 | 27.7 | 9.2 | 4.6 | Table 2 |
| Quiet rural (curve D) | 53.6 | 28.6 | 9.2 | 4.6 | not in Table 2; Rural values used, as in the C code |
| Noisy (ITU software only) | 83.2 | 37.5 | 11.0 | 6.7 | City values |
| Quiet (ITU software only) | 65.2 | 29.1 | 9.2 | 4.6 | Rural values |

All values checked against P.372-17 (08/2024); they are unchanged from
the edition the C code was written against. Equation (17) is stated
valid for 0.3 to 250 MHz; the engine extrapolates below 0.3 MHz as the C
code does. Table 2's "variation with location" deviations are not used.
Any other positive value `V` is treated as a noise figure specified
directly, `Fam = 204 - V`.

## 4. Galactic noise (`p372.galacticNoise`)

P.372-17 Table 1 curve E, `Fam = 52 - 23 log10(f)`, deciles 2 dB
(sigma 1.56 dB as specified in Part 7).

## 5. Combination (`p372.noise`)

P.372-17 Part 7, equations (18) to (26). Each component is a log-normal variable with median
`Fam_i` and standard deviation `sigma_i = D_i / 1.282` (galactic fixed
at 1.56 dB). With `c = 10 / ln 10`:

    alpha = sum_i exp(Fam_i / c + sigma_i^2 / (2 c^2))
    beta  = sum_i exp(Fam_i / c + sigma_i^2 / (2 c^2))^2 (exp(sigma_i^2 / c^2) - 1)
    gamma = sum_i exp(Fam_i / c)

    sigma_T = c sqrt(2 ln(alpha / gamma))         if any D_i > 12 dB
            = c sqrt(ln(1 + beta / alpha^2))      otherwise
    Fam_T   = c (ln alpha - sigma_T^2 / (2 c^2))
    D_T     = 1.282 sigma_T

This is evaluated once with the upper deciles (giving `FamT_u`, `DuT`)
and once with the lower deciles (`FamT_l`, `DlT`); the reported total
median is `min(FamT_u, FamT_l)`.

**Divergence from the Recommendation text.** P.372-17 says that when a
decile exceeds 12 dB the sigma_T from equation (19) "should be restricted
to a maximum value of" equation (25), i.e. `sigma_T = min(eq.19, eq.25)`.
The C code uses equation (25) unconditionally in that case, and the port
reproduces that. Over the 24 500 reference points the rule triggers in
5 640 cases; in 1 285 of them equation (25) exceeds equation (19), by up
to 0.64 dB. The `min` reading would also lower sigma_T in 9 191 cases
where no decile exceeds 12 dB if applied unconditionally; the port only
applies it in the >12 dB case that the text describes.

The port follows the Recommendation: by default `p372.noise` uses
`sigma_T = min(eq.19, eq.25)` when a decile exceeds 12 dB (`sigmaRule =
'p372-17'`). Passing `sigmaRule = 'reference'` to `p372.noise` or
`p372.makeNoise` reproduces the C code instead; the test suite uses it to
compare against the golden data, and the app offers it in a drop-down.

## 6. Bypass

A negative man-made value `-X` skips everything and returns
`FamT = X`, `FaM = -X`, all other outputs 0. ITURHFProp uses this to
run link analyses with a user-specified noise level.

## 7. Figure data (`p372.runAtmosNoiseMonths`)

Reproduces ITURNoise.exe Mode 2 for months 1, 4, 7, 10 and local hours
0, 4, ..., 20: a) `Fam` at 1 MHz on a 1 degree grid, b) `Fam(f)` for
`Fam1 = 5, 10, ..., 100 dB` at Boulder, c) all statistics plus `V_d`
(NTIA Report 85-173 equations 30 and 31) at Boulder.

## References

- Recommendation ITU-R P.372-17 (08/2024), *Radio noise*. Parts 5-7
  (atmospheric, man-made, combination) were checked against this
  edition; the numerical content used by the engine is unchanged since
  P.372-14.
- ITU-R Study Group 3, *ITU-R-HF* software, folder P372 (Noise.c,
  MakeNoise.c, ITURNoise.c), engine version 14.3.
- Lucas, D. L. and Harper, J. D., *A numerical representation of CCIR
  Report 322 high frequency (3-30 Mc/s) atmospheric radio noise data*,
  NBS Technical Note 318, 1965.
- Spaulding, A. D. and Washburn, J. S., *Atmospheric radio noise:
  worldwide levels and other characteristics*, NTIA Report 85-173, 1985.
