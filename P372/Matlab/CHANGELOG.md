# Changelog

## 1.1.0 - 2026-09-08

- Man-made, galactic and combination checked against P.372-17 (08/2024).
- `p372.noise` now applies equation (25) as a cap on sigma_T
  (`min(eq.19, eq.25)`) when a decile exceeds 12 dB, as P.372-17 words
  it. The ITU C behaviour (equation 25 replaces 19) is available with the
  new optional `sigmaRule = 'reference'` argument on `p372.noise` and
  `p372.makeNoise`, and via a drop-down in the app; the reference tests
  use it. Results differ only when a decile exceeds 12 dB.

## 1.0.0 - 2026-09-08

- Initial MATLAB port of ITU-R-HF/P372 engine version 14.3.
- Engine, coefficient readers, ITURNoise Mode 1 and Mode 2 equivalents,
  figure rendering, generic coefficient reader, uifigure application.
- Test suite with golden data from the C reference build; validated in
  GNU Octave 11.3 (see docs/VALIDATION.md).
