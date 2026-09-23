# Plan: MATLAB port of ITU-R P.533 (P533 engine 14.2)

Status: implemented 2026-09-08 (steps 0-9). Code in `P533/`, validation in `P533/docs/VALIDATION.md`, deviations in `P533_DEVIATIONS.md` (32 items). Companion to the completed P372 port in
`P372/` (see `P372/README.md`). Written after reading all 20 C files in
`ITU-R-HF/P533/Src/P533` (8 232 lines), the ITURHFProp driver, the data
files, and Recommendation ITU-R P.533-14 (08/2019).

## 1. What P533 is

`P533()` predicts, for one circuit at one month, hour, SSN and frequency:

| Part (P.533-14 Annex 1) | Outputs | C files |
|---|---|---|
| 1 Frequency availability (sections 2-4) | control points; basic MUF per E and F2 mode, path MUF, MUF deciles, operational MUF, mode probability; E-layer screening frequency | Geometry.c, CalculateCPParameters.c, Magfit.c, MUFBasic.c, MUFVariability.c, MUFOperational.c, ELayerScreeningFrequency.c |
| 2 Median sky-wave field strength (sections 5-6) | per-mode field strength (paths to 9 000 km), long-path field strength (from 7 000 km), interpolation 7 000-9 000 km, median available receiver power, dominant mode, receive gain | MedianSkywaveFieldStrengthShort.c, MedianSkywaveFieldStrengthLong.c, Between7000kmand9000km.c, MedianAvailableReceiverPower.c |
| 3 System performance (sections 7-10) | SNR and deciles, SNR at required reliability, SIR, digital spread parameters, BCR, OCR, OCR with equatorial scattering, multimode interference | CircuitReliability.c |
| Infrastructure | PathData struct, validation, initialisation, memory, data readers, antenna readers | P533.h, P533.c, ValidatePath.c, InitializePath.c, PathMemory.c, InputDump.c, ReadIonParameters.c, ReadP1239.c, ReadType13.c |

Noise comes from P372, called once at the receiver (`Noise(hour, lng_rx,
lat_rx, f)`), which the finished `p372` package already provides.

Sizes: 384-value Lh table, 2x84 absorption polynomials, 9x29 ATNO table,
50-value inverse-normal table, decile tables, magnetic field Gauss
coefficients (7x7 G and H), and about 60 numeric constants scattered
through the physics files. All are function-local in C; none are read
from files.

## 2. Inputs the port must read

| Data | File | Format (established from the readers) | MATLAB reader |
|---|---|---|---|
| foF2 and M(3000)F2 maps | `Data/ionosNN.bin`, 12 x 11.2 MB | Fortran unformatted stream: 5-byte header (0xFF + int32 LE length), 1 399 728 little-endian float32 foF2, 10 bytes of markers, 1 399 728 float32 M3kF2, 5-byte trailer. File order `[ssn 2][lng 241][lat 121][hour 24]`, hour fastest; 1.5 deg grid from 90S/180W; SSN 0 and 100 | `p533.readIonParameters(dataDir, month)` via `fread(fid, n, 'float32', 0, 'ieee-le')` + `reshape`/`permute` |
| foF2 within-month deciles (P.1239-3 Tables 2/3) | `Data/P1239-3 Decile Factors.txt` | UTF-8 BOM, 2 title lines, 18 blocks of 4 header + 19 latitude lines x 24 hours; block order decile / season / SSN band; latitude 90..0 step 5 | `p533.readP1239(dataDir)` -> `foF2var(3 season, 24 hour, 19 lat, 3 ssn, 2 decile)` |
| Antenna patterns | VOACAP Type 11, 13, 14 text; or `ISOTROPIC` | 1 x 360 x 91 or 30 x 360 x 91 gains dBi; Type 13 rotated by integer bearing; MaxGain added for 11/14 only | `p533.readAntenna(file, bearing)` and `p533.isotropicPattern(G)` |
| Noise coefficients | `COEFFmmW.txt` | already done | `p372.readFamDud` |
| Circuit definition | ITURHFProp `.in` keyword file | `Keyword value` / `Keyword "string"`, `//` comments | `p533.readInputConfiguration(file)` (app layer) |

`COEFF*.BIN`, `FOF2*.DAW`, `FOF2DALW.BIN` are inputs to the external
`iongrid` program and are not read by P533; they are out of scope.

## 3. Design

### 3.1 Package layout (`P533/Matlab`)

```
P533/
  +p533/
    Contents.m
    % infrastructure
    newPath.m               % path = newPath()  default PathData struct with C sentinels
    validatePath.m          % errors with p533:validate:<field> identifiers, same ranges as ValidatePath.c
    initializePath.m        % distance, long-path, CP[MP], CP[T1k]/CP[R1k] (>=2000 km), season
    run.m                   % path = run(path, coeff372)   = P533(): the ordered call sequence
    % data
    readIonParameters.m  readP1239.m  readAntenna.m  isotropicPattern.m  readInputConfiguration.m
    % geometry and control points
    greatCircleDistance.m  greatCirclePoint.m  bearing.m  geomagneticCoords.m  magfit.m
    calculateCPParameters.m  ionosphericParameters.m  solarParameters.m  findfoE.m  bilinearInterpolation.m
    % part 1
    mufBasic.m  calcB.m  calcCd.m  calcdmax.m  calcF2DMUF.m
    mufVariability.m  findfoF2var.m  mufOperational.m
    eLayerScreeningFrequency.m  mirrorReflectionHeight.m  elevationAngle.m  incidenceAngle.m
    % part 2
    medianSkywaveFieldStrengthShort.m  absorptionTerm.m  diurnalAbsorptionExponent.m
    absorptionFactor.m  absorptionLayerPenetrationFactor.m  findLh.m  penetrationPoints.m
    antennaGain.m  antennaGain08.m
    medianSkywaveFieldStrengthLong.m  findMUFsandfM.m  findfL.m  winterAnomaly.m
    between7000kmand9000km.m  medianAvailableReceiverPower.m
    % part 3
    circuitReliability.m  digitalModulationSignalandInterferers.m  equatorialScattering.m
    findFlambdad.m  findFTl.m  modeSort.m
    % constants and tables
    constants.m             % R0, D2R, R2D, VofL, TINYDB, limits, CP and mode indices
    tables.m                % Lh, pval1/pval2, ATNO, NORM, Table2LD/UD, Rop, Aw, Gauss G/H/CT
  +p533/+report/            % ITURHFProp Report.c equivalent (RPT_* column selection, header)
  apps/HFPropApp.m          % uifigure: circuit inputs, run, results table, MUF/field/SNR vs hour and frequency plots
  iturhfprop.m              % ITURHFProp.exe equivalent: .in file -> report file, area/frequency/hour/month loops
  tests/  tools/  docs/
```

Data stays in the ITU repository (`P533/Data`, `ITURHFProp/Data/Antenna`);
`p533.defaultDataDir` uses the same fallback logic as `p372`.

### 3.2 Data model
One MATLAB struct `path` mirroring `struct PathData` field for field
(same names, so the C documentation and the report generator carry over),
with these conversions:
- `month` 1..12 and `hour` 0..23 at the API (C uses 0..11 internally;
  the port converts once in `readInputConfiguration`/`run`).
- `CP` is a 1x5 struct array (indices T1k=1, Td02=2, MP=3, Rd02=4, R1k=5
  via `p533.constants`); `Md_E` 1x3 and `Md_F2` 1x6 struct arrays with
  the Mode fields; `DMidx` replaces the C pointer.
- Maps are numeric arrays: `foF2(24,241,121,2)` single, `M3kF2` same,
  `foF2var(3,24,19,3,2)` double, `A_tx.pattern(nf,360,91)`.
- Angles radians internally; degrees only at the input file, the report
  and the app, exactly as in C.

### 3.3 Fidelity rules (decided 2026-09-08: implement the Recommendation directly)

The port implements the text of Recommendation ITU-R P.533-14 Annex 1 and
the Recommendations it invokes (P.1239-4 for foE and foF2 variability,
P.1240-2 for the operational MUF ratios, P.842-5 for reliability tables,
P.372-17 via the `p372` package, P.1057 for the normal distribution). The
ITU C code (P533 14.2) is a loose reference: it is used to understand
intent, to supply the numerical tables that the Recommendation only
publishes as figures (Fig. 1 absorption factor, Fig. 2 penetration
factor, Fig. 3 diurnal exponent, Table 2 auroral loss, the REC533
polynomial fits), and to give approximate expected values. It is not the
specification.

Concretely:
- Each module is written from the Recommendation section, with the
  equation numbers in the help text and in comments next to each formula.
  Where the text is ambiguous the C code decides; where the C code and
  the text disagree the text wins and the case is recorded in
  `docs/P533_DEVIATIONS.md` (section, C behaviour, port behaviour,
  numerical effect). Twelve suspects are already known from the code
  reading; each is confirmed against the PDF before the module is
  written:
  1. `MUFOperational`: P.1240 Table 1 has an EIRP dimension the C code
     never indexes (power index always 0).
  2. `FindfoE`: polar-night test for the southern hemisphere compares
     against +72.56 deg.
  3. `EquatorialScattering` (Attachment 1): `FR = 0.1 + 0.008*max(SSN,160)`
     (text: SSN limited to 160, i.e. `min`); spread array indexed by mode
     in one place and by control point in another; F2 mode index not
     offset.
  4. `FindFlambdad`: dip in radians compared against degree constants
     in the 15-25 deg transition.
  5. `DigitalModulationSignalandInterferers` (section 10.2): `tau` in ms
     but `TW/1000` added; slant range uses `delta - psi` while section 5.2
     uses `delta + psi`; signal sum squares and square-roots.
  6. `CalcF2DMUF` (eq. 3): final term uses the unclamped `d/dmax`.
  7. `SmallestCPfoF2`, `ModeSort`: invalid sort loops.
  8. `MirrorReflectionHeight` (section 5.1): quartic `G` term set.
  9. `FindfoF2var`: latitude roll-over across the pole.
  10. `CircuitReliability`: `SNRXX` unset when there is no interferer;
      `SNRXXp` outside 1..99 indexes off the normal table (use P.1057 or
      `erfinv` instead of the 50-entry table).
  11. `P533()` rejects the P372 man-made override return code.
  12. `GeomagneticCoords`: pole longitude 68.2 W in code, 69.2 W in comment
      (P.533 section 5.2 states the pole to use).
- No "reproduce the C code" compatibility mode is built. Where a C
  behaviour is a plain bug there is nothing to preserve; where it is a
  defensible reading of ambiguous text, the port follows the text and the
  deviations file explains the difference.
- Numerical conventions are free to be exact: `pi/180`, `deg2rad`, real
  sorts, `erfinv`, vectorised evaluation. Results are compared with the C
  output by tolerance, not bit for bit.
- Idioms carried over from C only because they are the Recommendation:
  SSN limited to 160 (section 1), reflection height rules of section
  5.1, 3 deg minimum elevation, the 2 000 / 4 000 / 7 000 / 9 000 km
  regime boundaries, control-point definitions of section 2, and the
  penetration-point absorption at 90 km if and only if the -14 text
  specifies it (the C `PEN` switch; to be checked against section 5.2).

### 3.4 Application layer
- `iturhfprop.m`: reads a `.in` file (all keywords of
  `ReadInputConfiguration.c`), loads maps once per month, loops over the
  LL/LR/UL/UR area grid, frequencies, hours and months exactly like
  `ITURHFProp.c`, and writes the report with the same header, "Data
  Format" column list and `%` formats as `Report.c`, so reports diff
  against the C program. Supports `RPT_ALL` and the 30 `RPT_*` flags.
- `apps/HFPropApp.m`: circuit form (Tx/Rx, antenna files or isotropic,
  bearings, date/hour/SSN, frequency, power, bandwidth, SNR and
  reliability requirements, man-made category, modulation and spread
  parameters), a results tab (all PathData outputs as a table, plus the
  report text), and plots: MUF/OPMUF/FOT/HPF, field strength, Pr, SNR
  and BCR versus hour (0-23) and versus frequency (2-30 MHz), and a
  world map of field strength or SNR for the area mode. Built on the
  engine only; MATLAB R2019b+.

## 4. Validation strategy

The C output is a loose reference, so validation has three layers.

1. **Recommendation-derived unit tests (primary).** For every equation
   with closed form, a test evaluates it at hand-checked points taken from
   the text: eq. (3)-(6) F2 MUF at 3 000 km and at `dmax`, the E-layer
   MUF and screening (sections 3.3, 4), elevation angle (13), slant range
   (19), the loss terms (21)-(25) at values where Figures 1-3 can be read
   off, the long-path terms of section 5.3, eq. (42) interpolation, the
   receiver power conversion (section 6), SNR and deciles (sections 7-8),
   BCR/OCR sigmoids (section 10, P.842 Table 1), and the scattering
   probability functions of Attachment 1. Tables typed in from the
   Recommendation (P.1240 Table 1, P.842 Tables 2-3, P.533 Table 2, Table
   5) are checked against the PDF, not the C source.
2. **C comparison with tolerance (secondary).** `tools/ref_p533.c`
   dumps all PathData outputs for about 5 000 cases spanning every
   distance regime, both hemispheres, months 1/4/7/10, hours 0/6/12/18,
   SSN 10/70/150, 2-30 MHz, analog and digital, short and long path,
   isotropic and Type 13 antennas. Test expectations:
   - readers, geometry, solar and magnetic parameters, ionospheric map
     interpolation: agree to 1e-9 (no deviation expected);
   - MUFs, field strengths, Pr, SNR: agree to 0.05 dB / 0.05 MHz except in
     cases that hit a listed deviation, which are tagged in the case file
     with the deviation id and checked against the Recommendation-derived
     value instead;
   - any untagged case outside tolerance is a test failure to be
     investigated, never silently widened.
3. **Behavioural checks.** (a) `iturhfprop` reports on the repository's
   `Bin/*.in` and `macos-build/*_test.in` inputs compared column by column
   with the C reports within the same tolerances; (b) the CCIR D1
   databank (`ITURHFProp/D1/D1_Table1.csv`, 1 613 rows): the residual
   statistics of predicted minus measured field strength must be at least
   as good as the C program's, which guards against a deviation fix that
   worsens agreement with measurements.

All tests are plain-`assert` scripts under `run_all_tests.m`, run in
Octave 11.3 locally; the app is left for MATLAB.

## 5. Work breakdown and order

| Step | Content | Depends on | Effort |
|---|---|---|---|
| 0 | Read P.533-14 Annex 1 in full plus the cited sections of P.1239-4, P.1240-2, P.842-5, P.1057; write `docs/P533_EQUATIONS.md` (every equation, symbol, unit, section) and confirm or dismiss the 12 suspects into `docs/P533_DEVIATIONS.md` | - | 1.5 days |
| 1 | C reference build and `ref_p533.c` driver; golden data and reader dumps | 0 | 1 day |
| 2 | `constants`, `newPath`, `validatePath`, readers (ionos, P1239, antennas, isotropic) + exact-agreement tests | 1 | 1 day |
| 3 | Geometry, magnetic field (P.1239 section 2, Gauss coefficients), solar geometry, foE (P.1239 section 3), map interpolation (P.1144 bilinear), control points (section 2) | 2 | 1 day |
| 4 | Part 1: sections 3.1-3.7 and 4 + equation tests and C comparison | 3 | 1 day |
| 5 | Part 2 to 9 000 km: section 5.1-5.2 incl. Figures 1-3 fits, Table 2, antenna gain; section 6 | 4 | 1.5 days |
| 6 | Part 2 long path: section 5.3-5.4 | 5 | 1.5 days |
| 7 | Part 3: sections 7-10 and Attachment 1; `run` sequencing; full C comparison with tagged deviations | 6 | 1.5 days |
| 8 | `readInputConfiguration`, `+report`, `iturhfprop`; report comparisons; D1 databank run | 7 | 1 day |
| 9 | `HFPropApp`; documentation to the P372 standard (`ALGORITHM`, `DATA_FORMATS`, `API`, `VALIDATION`, `DEVIATIONS`, `EQUATIONS`), CHANGELOG, PR to ITU-R-HF as `P533/Matlab` | 8 | 1.5 days |

About 12 working days of agent time. Step 0 is new relative to the
C-faithful plan and is what makes "implement the Recommendation" real:
each formula is traced to the text before it is coded.

## 6. Risks and open decisions
- **Figures without formulas.** Sections 5.2 uses Figures 1-3 (absorption
  factor, penetration factor, diurnal exponent) and section 5.3 uses
  Figure 4 style data that the Recommendation gives only graphically.
  The port must take the REC533 numerical fits from the C code for these
  (they are the only machine-readable source) and state so in
  `P533_DEVIATIONS.md`; digitising the figures independently is out of
  scope.
- **Divergence from ITURHFProp users' expectations.** Because deviations
  are fixed rather than reproduced, some results will differ from the
  ITU program. The deviations file and the D1 comparison are the
  evidence base for the PR discussion with ITU-R Study Group 3; each
  item should be reported upstream as an issue as well.
- **Version.** The C engine is 14.2 against P.533-14; the ITU has since
  published P.533-15 (check the itu-r folder; only -14 is present). If a
  -15 text exists the man-made noise and equatorial scattering parts may
  have changed and the deviations list should be checked against it.
- **Performance in Octave.** A single long path evaluates 672 control
  points; a 360x181 area map at 24 hours is millions of control points.
  Vectorising `ionosphericParameters`/`solarParameters`/`magfit` over
  point arrays is required for the app's map mode.
- **Memory.** Maps are 1.4 M single per array per month; loading all 12
  months is 135 MB. Load per month on demand, cache the last two.
- **Data path conventions.** The C readers require a trailing separator
  and 256-byte buffers; the port uses `fullfile` and has no limit, which
  is a deliberate, documented difference.
- **Antenna files.** Only the `Bin/*.in` samples reference `.T13`
  files under Windows paths; `ITURHFProp/Data/Antenna` holds NEC and T13
  folders. Golden cases will use those local files.

## 7. Immediate next steps
1. Step 0: read P.533-14 Annex 1 and the cited Recommendations, produce
   `P533_EQUATIONS.md` and the confirmed `P533_DEVIATIONS.md`.
2. Steps 1-3: C reference build, readers, geometry and control points,
   reusing the P372 test runner and documentation templates.
3. Report back with the equations document and the first control-point
   comparison before starting the physics modules.
