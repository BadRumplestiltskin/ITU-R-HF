# Validation

## Reference implementation

The golden data in `tests/reference/` is produced by the original ITU C
sources (`Noise.c`, `MakeNoise.c`, `NoiseMemory.c`, `InitializeNoise.c`
from ITU-R-HF/P372/Src/P372, engine 14.3) compiled with clang on macOS
by `tools/build_reference.sh`:

- `tools/ref_driver.c` links the engine and evaluates `Noise()` and
  `AtmosphericNoise_LT()` on a fixed grid, writing `ref_points.csv`
  (24 500 rows, 12 + 6 outputs each, `%.12g`), dumps the parsed
  coefficient arrays for January (`coeff_dump_01.txt`, `%.17g`) and the
  README example (`readme_example.txt`).
- `tools/ref_mode2.c` replicates `RunAtmosNoiseMonths()` from
  `ITURNoise.c` for January, 0 h local time and writes `a_1m0h.csv`,
  `b_1m0h.csv`, `c_1m0h.csv` and the parsed `V_d` coefficients
  (`vd_coeffs.txt`).

The ITU sources are compiled unmodified; two missing `#include`s in the
upstream files are supplied with `-include stdlib.h -include errno.h`,
and `-fcommon` accommodates the tentative global definitions in
`Noise.h`.

## Results

| Test | Scope | Criterion | Observed |
|---|---|---|---|
| test_readFamDud | 4 arrays x 12 months | exact equality | 0 |
| test_readVdCoeffs | 240 coefficients | exact equality | 0 |
| test_readCoeff | 28 arrays, dims and consistency | exact | pass |
| test_noise_vs_reference | 24 500 points, 12 outputs, `sigmaRule='reference'` | < 1e-6 dB | 5.0e-10 dB |
| test_sigmaRule | 3 000 points, default rule vs independent eq. 18-26 | < 1e-9 dB | pass |
| test_noise_vs_reference | 3 500 points, 6 LT statistics | < 1e-6 dB | 5.0e-10 dB |
| test_vectorised_equals_scalar | grid vs point evaluation | bit-identical | pass |
| test_mode2_csv | a, b, c data files | byte-identical text | pass |
| test_manMade_galactic | table constants, bypass | exact | pass |
| test_iturNoise_cli | string args, CSV, validation | exact | pass |

The residual 5e-10 dB comes from libm `exp`/`log`/`pow` differences
between clang's C library and the MATLAB/Octave runtime; summation
order in the Fourier series is kept identical to the C loops so the
coefficient arithmetic itself is bit-exact.

Validated with GNU Octave 11.3.0 (aarch64, macOS 26). The GUI
(`apps/P372NoiseApp.m`) requires MATLAB and has not been executed in
this environment; it contains no numerical code of its own.

## Reproducing

    tools/build_reference.sh                 # rebuild golden data (needs clang)
    octave --no-gui --eval "cd tests; run_all_tests"

or in MATLAB: `cd tests; run_all_tests`.
