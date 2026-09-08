# API summary

Conventions: angles in radians for engine functions (`p372.D2R` converts),
degrees for `makeNoise`, `iturNoise` and the app; frequencies in MHz;
month 1..12; UTC hour 0..23 except `iturNoise`, which keeps the 1..24
convention of the ITU command line. Full details: `help p372.<name>`.

## Single point

| Function | Signature |
|---|---|
| `p372.makeNoise` | `[n, out] = makeNoise(month, hourUTC, latDeg, lngDeg, freq, mmnoise [, dataDir, pntflag])` |
| `p372.noise` | `n = noise(coeff, mmnoise, hourUTC, rlng, rlat, freq)` |
| `p372.iturNoise` | `[retval, out] = iturNoise(month, hour1to24, freq, lat, lng, mmnoise, dataDir [, pntflag])` or `iturNoise(dataDir)` |
| `p372.formatReport` | `txt = formatReport(n, month, hourUTC, lngDeg, latDeg, freq [, timestr])` |

Result struct `n` fields, in order: `FaA DuA DlA FaM DuM DlM FaG DuG DlG FamT DuT DlT`.

## Components

| Function | Signature |
|---|---|
| `p372.atmosphericNoise` | `[FaA, DuA, DlA] = atmosphericNoise(coeff, hourUTC, rlng, rlat, freq)` |
| `p372.atmosphericNoiseLT` | `FamS = atmosphericNoiseLT(coeff, localHour, lng, lat, freq)` (arrays of lng/lat allowed) |
| `p372.getFamParameters` | `FS = getFamParameters(coeff, tmblk0to5, lng, lat, freq)` (arrays allowed) |
| `p372.famFrequencyVariation` | `FA = famFrequencyVariation(coeff, famRow, Fam1MHz, freq)` |
| `p372.manMadeNoise` | `[FaM, DuM, DlM] = manMadeNoise(category, freq)` |
| `p372.galacticNoise` | `[FaG, DuG, DlG] = galacticNoise(freq)` |

## Data

| Function | Signature |
|---|---|
| `p372.readFamDud` | `coeff = readFamDud(dataDir, month)` |
| `p372.readCoeff` | `C = readCoeff(dataDir, month [, names])` |
| `p372.readVdCoeffs` | `[c, d] = readVdCoeffs(dataDir)` |
| `p372.findVd` | `[V_d, sigma_V_d] = findVd(freq, c5, d5)` |
| `p372.defaultDataDir` | `dir = defaultDataDir()` |

## Figures

| Function | Signature |
|---|---|
| `p372.runAtmosNoiseMonths` | `root = runAtmosNoiseMonths([dataDir, outRoot, months, hours])` |
| `p372.plots.makeP372Figs` | `makeP372Figs([root, formats])` |
| `p372.plots.makeAFigure` / `makeBFigure` / `makeCFigure` | `fig = makeXFigure(csvFile)` |

## Application

`P372NoiseApp` (apps folder): `app = P372NoiseApp([dataDir])`, MATLAB R2019b+.

## Error identifiers

`p372:readLines`, `p372:readNamedBlock`, `p372:readFamDud`,
`p372:getFamParameters`, `p372:makeNoise`, `p372:iturNoise`,
`p372:makeP372Figs`.
