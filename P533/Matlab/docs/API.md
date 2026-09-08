# API summary

Angles: radians in `+p533` functions and in the path structure; degrees
in `iturhfprop` input files and in the app. Month 1..12, hour 0..23 UTC,
frequencies MHz, distances km, field strengths dB(1 uV/m), powers dBW,
noise dB above kT0b.

## Path structure (`p533.newPath`)

Inputs: `name txname rxname year month hour SSN frequency BW txpower
Modulation SorL SNRr SNRXXp SIRr A TW FW T0 F0 L_tx L_rx A_tx A_rx
manMadeNoise sigmaRule liRule maps foF2var coeff372`.

Outputs: `season distance txBearing rxBearing ptick ptickLong dmax B ele
BMUF MUF50 MUF90 MUF10 OPMUF OPMUF90 OPMUF10 n0_F2 n0_E Es El Ei Ep Pr Lz
E0 Gap Ly fM fL F fH Gtl K SNR DuSN DlSN SNRXX SIR DuSI DlSI BCR OCR OCRs
MIR probocc Grw EIRP DMidx noise CP(1..5) Md_E(1..3) Md_F2(1..6)`.
`n0_*` are hop counts (99 = no mode); `DMidx` indexes `[Md_E Md_F2]`;
control points `CP` are T1k, Td02, MP, Rd02, R1k. Mode fields: `hops BMUF
MUF90 MUF50 MUF10 OPMUF OPMUF10 OPMUF90 Fprob deltal deltau hr fs Lb Ew
ele Prw Grw tau MC ptick`. `RSN RT RF` of older editions are not provided.

## Functions

| Function | Signature | Reference |
|---|---|---|
| `p533.run` | `path = run(path [, stopAfter])` | P533() sequence |
| `p533.loadData` | `path = loadData(path [, dataDir])` | |
| `p533.validatePath` | `validatePath(path)` | |
| `p533.initializePath` | `path = initializePath(path)` | section 2 |
| `p533.mufBasic` | `path = mufBasic(path)` | 3.1-3.5 |
| `p533.mufVariability` | `path = mufVariability(path)` | 3.6 |
| `p533.mufOperational` | `path = mufOperational(path)` | 3.7, P.1240 Table 1 |
| `p533.eLayerScreeningFrequency` | `path = eLayerScreeningFrequency(path)` | 4 |
| `p533.mirrorReflectionHeight` | `hr = mirrorReflectionHeight(cp, d, f, R12)` | 5.1 |
| `p533.medianSkywaveFieldStrengthShort` | `path = ...(path)` | 5.2 |
| `p533.absorptionLoss` | `Li = absorptionLoss(path, hops, hr, f [, fLref])` | eq. (20) |
| `p533.medianSkywaveFieldStrengthLong` | `path = ...(path)` | 5.3 |
| `p533.between7000kmand9000km` | `path = ...(path)` | 5.4 |
| `p533.medianAvailableReceiverPower` | `path = ...(path)` | 6 |
| `p533.circuitReliability` | `path = circuitReliability(path)` | 7-10, P.842, Attachment 1 |
| `p533.pointParameters` | `P = pointParameters(maps, month, hourUTC, R12, lat, lng)` | vectorised |
| `p533.calculateCPParameters` | `cp = calculateCPParameters(path, cp)` | |
| `p533.ionosphericParameters` | `[foF2, M3kF2] = ...(maps, hourUTC, R12, lat, lng)` | 3.4, P.1144 |
| `p533.findfoE` | `foE = findfoE(lat, S, month, hourUTC, R12)` | P.1239 4 |
| `p533.solarParameters` | `S = solarParameters(lat, lng, month, hourUTC)` | |
| `p533.magfit` | `[dip, fH] = magfit(lat, lng, height)` | P.1239 2 |
| `p533.findfoF2var` | `[dl, du] = findfoF2var(foF2var, season, ltime, lat, R12)` | P.1239 Tables 2-3 |
| `p533.findLh` | `Lh = findLh(glat, ltime, hopRange, season)` | Table 2 |
| `p533.absorptionFactor`, `penetrationFactor`, `diurnalAbsorptionExponent` | Figures 1-3 | |
| `p533.winterAnomaly` | `Aw = winterAnomaly(lat, month)` | Table 5 |
| `p533.antennaGain`, `antennaGain08` | gain at (az, el); best gain 0-8 deg | |
| `p533.greatCircleDistance`, `greatCirclePoint`, `bearing`, `geomagneticCoords`, `pathPoint` | geometry | |
| `p533.readIonParameters`, `readP1239`, `readAntenna`, `isotropicPattern`, `readInputConfiguration` | data | |
| `p533.report.write` | `write(fid, path, cfg, what [, when])` | Report.c layout |
| `iturhfprop` | `outFile = iturhfprop(inFile [, outFile, 'csv', tf, 'silent', tf, 'liRule', r])` | ITURHFProp.exe |
| `HFPropApp` | `app = HFPropApp([dataDir])` | MATLAB app |

Error identifiers: `p533:validate:<field>`, `p533:readIonParameters`,
`p533:readP1239`, `p533:readAntenna`, `p533:defaultDataDir`,
`p533:getFamParameters`, `iturhfprop:open`.
