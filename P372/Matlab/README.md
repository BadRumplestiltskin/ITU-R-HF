# P372 for MATLAB

A MATLAB port of the ITU-R Study Group 3 reference implementation of
Recommendation ITU-R P.372-14 (radio noise), engine version 14.3. The
original C sources live in the `ITU-R-HF` repository
(<https://github.com/ITU-R-Study-Group-3/ITU-R-HF>), folder `P372`.

Everything is plain MATLAB (R2019b or newer for the GUI, base toolboxes
only). The engine, data generation and tests also run in GNU Octave.

## Layout

| Path | Contents |
|---|---|
| `+p372/` | Noise engine and coefficient readers (see below) |
| `+p372/+plots/` | Figure generation replacing `MakeP372figs.py` |
| `apps/P372NoiseApp.m` | Interactive calculator (uifigure) |
| `../Data/` | ITU coefficient files `COEFF01W.txt` .. `COEFF12W.txt`, `V_d.txt`, `sigma_V_d.txt` (the repository's `P372/Data`; a `Data/` folder next to `+p372` takes precedence if present) |
| `tests/` | Test suite plus golden reference data generated from the C code |
| `tools/` | C reference drivers and `build_reference.sh` |
| `docs/` | `ALGORITHM.md`, `DATA_FORMATS.md`, `API.md`, `VALIDATION.md` |

Type `help p372` for the function index and `help p372.<name>` for any
function. Requirements: MATLAB R2019b or newer for the app; the engine,
tests and figure generation also run under GNU Octave 11.

## Quick start

```matlab
addpath('/path/to/ITU-R-HF/P372/Matlab')

% January, 14 UTC (hour index 13), 40 N 165 E, 1 MHz, City man-made noise
[n, out] = p372.makeNoise(1, 13, 40, 165, 1.0, 0, [], 1);
n.FamT                       % 76.987 dB above kT0b

% command-line style, arguments as for ITURNoise.exe (hour 1..24)
p372.iturNoise(1, 14, 1.0, 40.0, 165.0, 0, p372.defaultDataDir(), 3);

% Recommendation figures: data files, then plots
p372.runAtmosNoiseMonths();          % writes ./P372_figures/{a,b,c}/csv
p372.plots.makeP372Figs();           % writes svg/png/pdf next to them

% GUI
P372NoiseApp
```

## Function map

| C (ITU) | MATLAB | Notes |
|---|---|---|
| `ReadFamDud()` | `p372.readFamDud(dataDir, month)` | month is 1..12 (C used 0..11). Arrays keep C index order, 1-based |
| `GetFamParameters()` | `p372.getFamParameters(coeff, tmblk, lng, lat, f)` | vectorised over lat/lng arrays |
| `AtmosphericNoise()` | `p372.atmosphericNoise(coeff, hourUTC, lng, lat, f)` | hour 0..23, radians |
| `AtmosphericNoise_LT()` | `p372.atmosphericNoiseLT(coeff, lrxmt, lng, lat, f)` | full statistics, vectorised |
| `GalacticNoise()`, `ManMadeNoise()` | `p372.galacticNoise`, `p372.manMadeNoise` | |
| `Noise()` | `p372.noise(coeff, mmnoise, hourUTC, lng, lat, f)` | returns struct of the 12 outputs |
| `MakeNoise()` | `p372.makeNoise(month, hourUTC, latDeg, lngDeg, f, mmnoise, dataDir, pntflag)` | degrees in, struct and 12-vector out |
| `PrintFam()` | `p372.formatReport(...)` | identical text layout |
| `ITURNoise.exe` mode 1 | `p372.iturNoise(...)` | hour 1..24 as on the command line |
| `ITURNoise.exe` mode 2 | `p372.runAtmosNoiseMonths(dataDir, outRoot)` | identical CSV names, headers and formats |
| `FindV_d()` | `p372.findVd`, `p372.readVdCoeffs` | |
| `MakeP372figs.py` | `p372.plots.makeP372Figs(root)` | a) contour map, b) and c) log-frequency plots |
| `ReadCoeff.c` | `p372.readCoeff(dataDir, month [, names])` | all 28 arrays, Fortran order |
| `AllocateNoiseMemory`, `FreeNoiseMemory`, `InitializeNoise` | not needed | |

Angles passed to the engine functions are radians, converted with the
same truncated constants as the C code (`p372.D2R`, `p372.R2D`).

## Fidelity

The port reproduces the C implementation to round-off. `tests/reference/`
holds outputs of the original C code built by `tools/build_reference.sh`:
24 500 noise calculations over months, hours, positions, frequencies and
man-made categories agree to better than 1e-9 dB, and the a/b/c figure
CSV files are byte-identical.

Man-made and galactic constants, decile tables and the combination
formulas were checked against Recommendation ITU-R P.372-17 (08/2024)
and agree (docs/ALGORITHM.md sections 3-5). One reading difference is
documented there: the C code applies equation (25) unconditionally when
a decile exceeds 12 dB, whereas the Recommendation describes it as an
upper limit on sigma_T.

Behaviours of the C code that were kept deliberately:

- `D2R = 0.0174532925` and `R2D = 57.2957795` (truncated), and local
  time from `hour + int(lng/15)` with truncation toward zero.
- For a user-supplied man-made noise value (anything other than the
  categories 0..5), the C code sets `DlM = 11.0` and `DuM = 6.7`, the
  reverse of every category branch. This looks like an upstream bug; it
  is reproduced and documented in `p372.manMadeNoise`.
- The report labels the atmospheric lower decile line "Upper Decile",
  exactly as `PrintFam()` does.
- `Data/sigma_V_d.txt` contains a few tokens with a letter `l` in place
  of the digit `1` (for example `2.45l13428E+00`). C `atof()` reads the
  numeric prefix only; `p372.readVdCoeffs` does the same so the c) figure
  data matches. Fixing the data file would change those curves slightly.

## Tests

```matlab
cd tests; run_all_tests
```

or from a shell with Octave: `octave --no-gui --eval "cd tests; run_all_tests"`.
The suite needs about 90 s in Octave, mostly for the 24 500 reference points.

To regenerate the reference data (needs clang and the ITU C sources, path
in `P372_C_SRC` or the default inside the script):

```bash
tools/build_reference.sh
```

## Documentation

- [docs/ALGORITHM.md](docs/ALGORITHM.md): the model, equations and how the code maps to P.372-14.
- [docs/DATA_FORMATS.md](docs/DATA_FORMATS.md): coefficient and output file layouts.
- [docs/API.md](docs/API.md): signatures and conventions at a glance.
- [docs/VALIDATION.md](docs/VALIDATION.md): how the golden data was made and the test results.
- [CHANGELOG.md](CHANGELOG.md), [LICENSE.md](LICENSE.md).

## Licence

The ITU software is provided by ITU-R Study Group 3 "free from any
copyright assertions" and "as is" without warranty; see the notice in
the original sources. This port carries the same terms.
