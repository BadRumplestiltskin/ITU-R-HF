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
Next: U2. Local gate before each push: normal + ASan/UBSan builds, all three
suites; then CI green on all four jobs.

## U1 NaN antenna bearing writes outside the pattern - high - FIXED
- read-pending-nan-tx-bearing-rejected, read-pending-nan-rx-bearing-rejected:
  ValidateITURHFP.c range checks pass NaN; ReadType13.c:217 (int)NaN, :272
  negative index write. Should exit 55 / 56. Also every other double that
  ValidateITURHFP checks.

## U2 ITURHFProp .in values silently misread - medium
- read-pending-unknown-manmade-noise-rejected, read-pending-unknown-modulation-rejected,
  prop-in-modulation-unknown-value: unknown word leaves the field uninitialised
  (ReadInputConfiguration.c:177-190). Should be 103 / 110.
- prop-orient-unknown-value: unknown AntennaOrientation stays TX2RX; 54 unreachable.
- prop-area-latinc-not-a-number: unchecked sscanf multiplies the radian default
  by D2R again; same unchecked sscanf for every scalar keyword. Should be 77.
- prop-list-hour-collides-with-unset, prop-list-frequency-collides-with-unset:
  -9998 / -9999 equal LISTUNSET. Should be 102 / 111.
- prop-list-frequency-missing: no Path.frequency (or hour, month) runs nothing,
  exits 0. Should be 111 (102, 101).
- prop-list-month-int-min: (int)v - 1 overflows. Should be 101 without UB.
- prop-rpt-combo-without-spaces: "RPT_D|RPT_PR" drops RPT_PR.
- *verify*: unknown RptFileFormat option (RPT_FOO) gives an empty report, exit 0.
- *verify*: unknown Path.SorL ignored.
- *verify*: Path.SSN 1e12 read as 1.

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

## U8 Documentation and report text - low
- *verify*: README return-code table disagrees with P533.h (131-142 vs 130-135/140/141/160/161/170).
- *verify*: README says CircuitCSV -t/-r take Type 13 only; they take 11, 13, 14.
- *verify*: RPT_ALL CSV header has two Grw columns.
- *verify*: "path minimum Rx elevation" shows 360 deg at SSN >= 8000 (InitializePath.c:92 seeds 2*PI).

## U9 Hygiene - low
- sprintf in ReadIonParameters.c, MakeNoise.c, Noise.c, DumpPathData.c,
  ReadInputConfiguration.c, Report.c: deprecated on macOS; move to snprintf.
