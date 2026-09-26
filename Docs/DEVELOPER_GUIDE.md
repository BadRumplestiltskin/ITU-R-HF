# Developer guide: the P533 and P372 engines

This guide is for people changing the prediction engines. The input
file format, the command line and the return codes of the ITURHFProp,
CircuitCSV and ITURNoise programs are in the top-level `README.md`.

Companion documents in this folder:

- `TRACEABILITY.md`: which function and line implements each equation
  and table of the Recommendations.
- `DEVIATIONS.md`: where the code departs from, or interprets, the
  Recommendation text, with the owner's rulings and the open questions.

## Layout

| Path | What it is |
|---|---|
| `P533/Src/P533` | The P.533-14 engine, built as `libp533` (`P533.dll` on Windows) |
| `P372/Src/P372` | The P.372 radio-noise engine, built as `libp372`; P533 loads it at run time |
| `Include` | Shared headers: `Common.h` (constants, `R0`, `D2R`), `P533.h` (the `PathData` structure and the P533 API), `Noise.h` (the `NoiseParams` structure and the P372 API) |
| `ITURHFProp/Src/ITURHFProp` | Driver: reads an `.in` file, runs P533 over months, hours, frequencies and an area, writes a report |
| `ITURHFProp/Src/CircuitCSV` | Driver: runs P533 for each row of a CSV file of circuits |
| `P372/Src/ITURNoise` | Driver: runs P372 alone |
| `P533/Data`, `P372/Data` | Ionospheric maps (`ionosNN.bin`), P.1239 decile factors, P.372 coefficient files (`COEFFmmW.txt`) |
| `ITURHFProp/Data/Antenna` | Antenna pattern files (VOACAP types 11, 13 and 14) |
| `Linux/Makefile` | Builds both libraries and the three drivers |
| `tests` | Regression, smoke and case suites, and `LEDGER.md` of known defects |

The engines are the libraries. The drivers only set up a `PathData`
structure, call `P533()` and report the results.

## What one prediction does

`P533()` (`P533/Src/P533/P533.c`) runs one prediction for one path, month,
hour and frequency. The caller has already allocated the path
(`AllocatePathMemory`), read the ionospheric maps and P.1239 tables
(`ReadIonParametersBin`, `ReadP1239`) and the P.372 coefficients
(`ReadFamDud`, through the P372 library), and set the antenna patterns
(`ReadType13` and friends, or `IsotropicPattern`). It then calls, in order:

| Step | Function (file) | P.533-14 section |
|---|---|---|
| Check the inputs | `ValidatePath` (`ValidatePath.c`) | – (range checks, not a section of the text) |
| Distance, bearings, control points M, T+1000, R-1000; their foF2, M(3000)F2, foE, fH, solar geometry | `InitializePath` (`InitializePath.c`), `CalculateCPParameters` (`CalculateCPParameters.c`), `Geometry.c`, `Magfit.c` | 2, 3.4 |
| Basic MUFs of the E and F2 modes, dmax, the T+d0/2 and R-d0/2 points | `MUFBasic` (`MUFBasic.c`) | 3.1, 3.3, 3.5 |
| MUF deciles | `MUFVariability` (`MUFVariability.c`) | 3.6 |
| Operational MUF | `MUFOperational` (`MUFOperational.c`) | 3.7 |
| E-layer screening frequency and F2 reflection heights | `ELayerScreeningFrequency` (`ELayerScreeningFrequency.c`) | 4, 5.1 |
| Field strength, paths up to 9000 km | `MedianSkywaveFieldStrengthShort` (`MedianSkywaveFieldStrengthShort.c`) | 5.2 |
| Field strength, paths from 7000 km | `MedianSkywaveFieldStrengthLong` (`MedianSkywaveFieldStrengthLong.c`) | 5.3 |
| Interpolation between the two, 7000 to 9000 km | `Between7000kmand9000km` (`Between7000kmand9000km.c`) | 5.4 |
| Received power | `MedianAvailableReceiverPower` (`MedianAvailableReceiverPower.c`) | 6 |
| Noise at the receiver | `Noise` in the P372 library | 7 (P.372) |
| S/N, reliability, digital performance, equatorial scattering | `CircuitReliability` (`CircuitReliability.c`) | 7 to 10, Attachment 1 |

Each field-strength routine returns straight away when the path length is
outside its range, so all three are always called. Everything is written
into the `PathData` structure (`Include/P533.h`); its members are
documented there.

## Conventions

- **Angles** are radians inside the engine. The drivers convert from the
  degrees in their input files.
- **Earth radius** `R0` is 6371 km (P.533-14 section 4). The magnetic field
  model uses its own radius, 6371.2 km (P.1239-4 equation (8)), as `RMAG`
  in `Magfit.c`.
- **Hours:** `path->hour = h` means h:00 UTC, and the ionospheric maps, the
  solar geometry and the 24-hour tables of the long-path model all use
  that convention. The ITURHFProp input file takes hours 1 to 24 and
  subtracts 1.
- **Months** are 0 to 11 inside the engine.
- **Sentinels:** a quantity that was not computed holds `TINYDB` (about
  -307) or `TOOBIG`; the report prints them as they are.
- **Modes:** `path->Md_E[]` holds up to `MAXEMDS` E modes and
  `path->Md_F2[]` up to `MAXF2MDS` F2 modes, indexed by hops minus one.

## Building and testing

```sh
make -C Linux all          # libraries and the three drivers
sh tests/regression.sh     # every ITURHFProp/Bin and P533/Bin case against its committed .out
sh tests/smoke.sh          # CircuitCSV and ITURNoise against tests/*/expected.csv
sh tests/cases.sh          # behaviour cases: bad input, edge geometry, exit codes
```

CI (`.github/workflows`) builds on Ubuntu, macOS and Windows and runs the
three suites under AddressSanitizer and UndefinedBehaviorSanitizer on
Ubuntu. To run that build locally:

```sh
make -C Linux clean
make -C Linux all OPTIMIZE=-O1 CC="gcc -g -fno-omit-frame-pointer -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=undefined,float-cast-overflow"
ASAN_OPTIONS=detect_leaks=0 sh tests/cases.sh
```

A change to the engine that changes results has to regenerate the
committed outputs it moves: run the driver on the case's `.in` from its
`Bin` folder, or regenerate a case's `out.expected` by running its `cmd`.
Look at how far each value moved before committing it; the commit
messages in the history show the size of change each fix produced.

## Comparing with the MATLAB port

The MATLAB port (`p533-matlab`) implements the same Recommendations from
the text. Its test `test_digital_vs_fork` runs this repository's
`ITURHFProp/Bin/digital_*.in` cases and requires S/N, S/I, the
reliabilities and MIR to agree with the committed `.out` to 0.02, so a
change here that moves those cases shows up there too. Over the port's
1 405 reference cases the two agree at every distance: received power
within 0.52 dB, S/N within 0.51 dB, BCR within 0.67 points, MUFs and S/N
deciles equal.
