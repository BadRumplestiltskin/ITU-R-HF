# Changelog

## 1.0.0 - 2026-09-08

- Initial MATLAB implementation of Recommendation ITU-R P.533-14 Annex 1
  written from the text (`../docs/P533_EQUATIONS.md`): Parts 1-3,
  Attachment 1, P.1239 foE and magnetic field, P.1240 operational MUF,
  P.842 reliability tables, P.372 noise through the `p372` package.
- Data readers for the ITU grid maps, P.1239 decile tables, VOACAP antenna
  patterns and ITURHFProp input files; `iturhfprop` driver with
  ITURHFProp-compatible text and CSV reports; `HFPropApp` application.
- Validation against the ITU C code (1 405 cases, 32 documented
  deviations) and the CCIR D1 databank (8 084 measurements).
- Absorption loss reading `liRule`: `'hopmean'` (default, D1 validated),
  `'sum'` (equation 20 as printed), `'reference'` (C conventions).
