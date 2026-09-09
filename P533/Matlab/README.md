# P533 for MATLAB

A MATLAB implementation of Recommendation ITU-R P.533-14, *Method for the
prediction of the performance of HF circuits*, written from the
Recommendation text. It predicts, for a circuit at a given month, hour,
sunspot number and frequency: basic and operational MUFs, mode field
strengths, median available receiver power, signal-to-noise ratio and
circuit reliability, using the ITU ionospheric map data and the P.372
noise model of the companion `p372` package.

The ITU-R Study Group 3 reference software (ITU-R-HF, P533 14.2) was used
as a tolerance reference, not as the specification: where its behaviour
differs from the text, the text is followed and the difference is
recorded in `docs/P533_DEVIATIONS.md`.

Plain MATLAB, no toolboxes; the engine and tests also run in GNU Octave.
The application needs MATLAB R2019b or newer.

## Layout

| Path | Contents |
|---|---|
| `+p533/` | Engine, data readers, geometry (`help p533` for the index) |
| `+p533/+report/` | ITURHFProp-style report and CSV output |
| `iturhfprop.m` | Driver equivalent to ITURHFProp.exe (input file to report) |
| `apps/HFPropApp.m` | Interactive prediction app (circuit, sweeps, area map) |
| `tests/` | Test suite, C reference data, D1 databank comparison |
| `tools/` | Reference builders (`build_reference.sh`, `ref_p533.c`, `make_d1_cases.py`) |
| `docs/` | `ALGORITHM.md`, `DATA_FORMATS.md`, `API.md`, `VALIDATION.md` |
| `../docs/` | `P533_EQUATIONS.md` (specification), `P533_DEVIATIONS.md`, `P533_PORT_PLAN.md` |

Data: the ITU `P533/Data` folder (`ionosNN.bin`, `P1239-3 Decile Factors.txt`,
`COEFFmmW.txt`) is found automatically next to the package, one level up,
or in the ITU-R-HF checkout (`p533.defaultDataDir`).

## Quick start

```matlab
addpath('/path/to/matlab_p372_p533/P533', '/path/to/matlab_p372_p533/P372')

% a single circuit
path = p533.newPath();
path.L_tx = struct('lat', 49.67 * pi/180, 'lng', 6.32 * pi/180);
path.L_rx = struct('lat', 51.12 * pi/180, 'lng', 7.27 * pi/180);
path.month = 8; path.hour = 12; path.SSN = 37; path.frequency = 6.1;
path = p533.loadData(path);
path = p533.run(path);
[path.BMUF, path.Ep, path.Pr, path.SNR, path.BCR]

% an ITURHFProp input file
iturhfprop('circuit.in', 'report.txt');
iturhfprop('circuit.in', 'report.csv', 'csv', true);

% the app (MATLAB only)
HFPropApp
```

## Options

- `path.liRule`: `'hopmean'` (default) evaluates the absorption loss of
  equation (20) as n hops times the mean of the term over the 2n
  penetration points, the reading that the CCIR D1 databank validates;
  `'sum'` evaluates the printed sum (twice the absorption, biased by
  -5 to -10 dB against D1); `'reference'` adds the ITU C code conventions
  for comparison tests. See `docs/VALIDATION.md` and deviation D30.
- `path.sigmaRule`: passed to `p372.noise` (`'p372-17'` default).

## Tests

```matlab
cd tests; run_all_tests
```

About two minutes in Octave. The C comparison tests (`test_part1..3_vs_reference`)
use `liRule = 'reference'` and `sigmaRule = 'reference'` so that only real
differences remain; tolerances and tags are explained in each test's help
and in `docs/VALIDATION.md`. `tools/build_reference.sh` regenerates the
reference data from the ITU C sources with clang.

## Licence

The ITU software and data are provided by ITU-R Study Group 3 free from
copyright assertions and without warranty; this implementation follows the
same terms (see `../P372/LICENSE.md`). The Recommendation text is
copyright ITU.
