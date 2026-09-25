# Defect ledger

One list of every known defect, from the 2026-09-25 coverage work that took
tests/cases from 2 to 491 cases. Each defect has a case under tests/pending/
that asserts the correct behaviour and fails today; fixing it means moving that
case to tests/cases/ in the same commit. Items marked *verify* have no case yet
and must be confirmed before they are fixed.

The gate for every fix: regression, smoke and cases pass on the normal build
and the ASan/UBSan build, and CI is green on all four jobs, before the next
fix starts.

Status: open / fixed (commit) / not a defect (reason).

Progress at 2026-09-25 end of session: U1 fixed (5f3ad5d). Also done outside
the units: all text data and source files converted to UTF-8 (813a116,
69073e6), guarded by cases data-text-is-utf8 and source-text-is-utf8.
Next: U3 (U2 fixed, see below). Local gate before each push: normal + ASan/UBSan builds, all three
suites; then CI green on all four jobs.

## U1 NaN antenna bearing writes outside the pattern - high - FIXED
- read-pending-nan-tx-bearing-rejected, read-pending-nan-rx-bearing-rejected:
  ValidateITURHFP.c range checks pass NaN; ReadType13.c:217 (int)NaN, :272
  negative index write. Should exit 55 / 56. Also every other double that
  ValidateITURHFP checks.

## U2 ITURHFProp .in values silently misread - medium - FIXED
- read-unknown-manmade-noise-rejected (103), read-unknown-modulation-rejected,
  prop-in-modulation-unknown-value (110): unknown word left the field unset.
- prop-orient-unknown-value (54): unknown AntennaOrientation stayed TX2RX.
- prop-area-latinc-not-a-number (77): unchecked sscanf for every scalar keyword.
  Every floating-point keyword now reads as NaN when it is not exactly one
  number (the range checks reject NaN with that keyword's code); year, SSN and
  SNRXXp must be whole numbers. Cases prop-in-txpower-not-a-number (113),
  prop-in-bw-two-values (112), prop-in-year-fractional (100),
  prop-in-scalar-trailing-comment (0).
- prop-list-hour-collides-with-unset (102), prop-list-frequency-collides-with-unset
  (111): the -9999 unused-slot marker is gone; the reader sets each list length.
- prop-list-frequency-missing (111), prop-list-hour-missing (102),
  prop-list-month-missing (101): a run without the list ran nothing, exit 0.
- prop-list-month-int-min (101): hours and months are now held 1-based as
  typed, so nothing subtracts from INT_MIN.
- prop-rpt-combo-without-spaces: options split on '|' with or without spaces.
- prop-rpt-unknown-option, prop-rpt-empty: new exit 79 (was: no result columns).
- prop-rpt-misspelled-option (79): OutputOption() compared prefixes, so RPT_PRX
  ran as RPT_PR; now exact. tests/cases/eng-lib/path.tmpl asked for the
  non-existent RPT_ALLMODES (read as RPT_ALL); it now says RPT_ALL, same bits.
- prop-in-sorl-unknown-value: new exit 80 (was: silently short path).
- prop-in-ssn-beyond-int, prop-in-ssn-not-a-number: new exit 81 (1e12 was read as 1).
- README: ITURHFProp return-code table listed 1000-1201, which the program
  never returns; replaced with 0, 32-34, 50-81 from ITURHFProp.h. Path.SSN
  row said 1 to 311; 0 is valid and there is no upper limit (ValidatePath.c).

## U3 ITURHFProp command line and messages - medium/low
- prop-cli-invalid-option: prints help, exits 0. Should be 75.
- prop-cli-extra-argument: ignored. Should be 75.
- prop-cli-output-name-starting-with-0: EMPTY is '0', not '\0' (ITURHFProp.h:224).
- prop-ant-missing-rx-silent, read-pending-missing-antenna-named-when-silent:
  -s suppresses the only message for 52/53.
- read-pending-antenna-error-names-file: error 142 does not name the file.
- read-pending-no-uninitialised-path-hour: " path hour -460002776" (ITURHFProp.c:375).

## U4 Ionospheric map file accepted when it is not one - medium
- read-pending-ionos-zero-filled-rejected: the FORTRAN record markers
  (ReadIonParameters.c:422, 450) are read but not checked. Should be 141.
- read-pending-ionos-message-names-bin-file: progress message says .txt.

## U5 Zero-length path loss below free space - medium
- eng-csv-zero-length-loss, eng-zero-length-loss-below-free-space: distance
  DBL_EPSILON (InitializePath.c:116) makes the eq (19) slant range ~0
  (MedianSkywaveFieldStrengthShort.c:165, 309); group delay ~0
  (CircuitReliability.c:669, 681). Should tend to the vertical-hop value.

## U6 CircuitCSV input and output - medium/low
- csv-pending-txpow-not-positive, read-pending-csv-txpow-not-positive: txPow <= 0
  runs at 1 kW, OK (:626).
- csv-pending-negative-ssn, read-pending-csv-ssn-negative: negative SSN clamped
  to 0 (:474), README says used as given.
- eng-csv-ssn-beyond-int, read-pending-csv-huge-ssn, read-pending-csv-ssn-not-finite,
  read-pending-csv-huge-percdays: float-to-int UB (:476, :629). BAD_RECORD / P533_ERROR.
- read-pending-csv-rxnoise-category-collision: rxNoise 0..-5 dBW taken as noise
  category codes (:641).
- csv-pending-g-*, csv-pending-m-*, csv-pending-j-not-a-number: unchecked
  atof/atoi options (:2043-2047). Should be 1001.
- csv-pending-F-list-over-4095-chars: -F list silently cut (:788).
- csv-pending-duplicate-header-column: first match silently used. Should be 1004.
- csv-pending-header-column-beyond-64: required column after field 64 "missing".
- csv-pending-j-output-unwritable: -j gives "worker 0 failed" not the serial message.
- csv-pending-freq-range-keeps-mufs: FREQ_RANGE rows blank computed MUFs (:1277).
- *verify*: exit codes 1001-1009 wrap modulo 256 (233-241) on POSIX.
- *verify*: P533_ERROR rows print distance/bearings 0 and mode NONE, README says blank.
- *verify*: LONG_PATH status when BMUF >= 99 (:984) at extreme SSN.
- *verify*: group range uses cos(delta - psi) (~:1103); the engine was corrected
  to cos(delta + psi).
- *verify*: -S scan shows -307 / -205 sentinels when there is no dominant mode.
- *verify*: coordinates echoed with 6 significant digits (180.0001 -> 180).

## U7 ITURNoise - medium/low
- noise-freq-nan, noise-lat-nan, noise-lng-nan: NaN accepted; (int)NaN UB at
  Noise.c:391. Should be 103 / 110 / 111.
- noise-lat-non-numeric: atof("north") = 0. Should be 110 (and lng 111).
- noise-csv-hour-column-1-to-24: CSV prints the 0-based hour (ITURNoise.c:1012).
- noise-flag-2-writes-MakeNoiseOut: file literally named ".\MakeNoiseOut.txt".
- noise-flag-101-not-csv: undocumented flags 101/102 select CSV.
- noise-mm-fractional-category: 2.5 taken as dB (FaM 201.5); README range 100-200.
- noise-readme-arguments (upstream 29): README Mode 1 list omitted frequency
  (argv[3]), so arguments 3-7 were misnumbered. Fixed (6d6fd5c).

## U8 Documentation and report text - low
- *verify*: README return-code table disagrees with P533.h (131-142 vs 130-135/140/141/160/161/170).
- *verify*: README says CircuitCSV -t/-r take Type 13 only; they take 11, 13, 14.
- *verify*: RPT_ALL CSV header has two Grw columns.
- *verify*: "path minimum Rx elevation" shows 360 deg at SSN >= 8000 (InitializePath.c:92 seeds 2*PI).

## U9 Hygiene - low
- sprintf in ReadIonParameters.c, MakeNoise.c, Noise.c, DumpPathData.c,
  ReadInputConfiguration.c, Report.c: deprecated on macOS; move to snprintf.

## Upstream issues
Open issues on ITU-R-Study-Group-3/ITU-R-HF, checked against this fork on
2026-09-25. Recorded here only; nothing is posted upstream.
- 38 P.1240 Table 1 Rop seasons reversed in the > 30 dBW row: fixed (cdf6b0b).
- 31 MUFOperational EIRP index always 0: fixed (cdf6b0b).
- 25 CircuitReliability ptick uses cos(delta - psi): fixed (a04ebe8).
- 18 RPT_N0_E / RPT_N0_F2 end the report-option scan: fixed (6b40e40).
- 29 README ITURNoise arguments omit frequency: fixed (6d6fd5c).
- 24 area prediction SNR contours not smooth: open, *verify*. Likely the
  7000/9000 km method bands and mode changes, which step by design.
- 16 embed Docs/Repo Arch.pptx diagram in README: open (documentation request).
