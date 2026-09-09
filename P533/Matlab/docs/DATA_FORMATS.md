# Data files

All files are read at run time from the ITU `P533/Data` folder.

## ionosNN.bin (NN = 01..12)

Grid-point tables of monthly median foF2 and M(3000)F2 produced by the ITU
`iongrid` program from the P.1239 numerical coefficients (P.1239 section
9). Each file is 11 197 844 bytes:

| offset | bytes | content |
|---|---|---|
| 0 | 5 | record header (0xFF, int32 little-endian byte count) |
| 5 | 5 598 912 | 1 399 728 x float32 foF2 (MHz) |
| 5 598 917 | 10 | record trailer + header |
| 5 598 927 | 5 598 912 | 1 399 728 x float32 M(3000)F2 |
| 11 197 839 | 5 | trailer |

Values are little-endian, stored with the UTC hour (24) varying fastest,
then latitude (121, -90..90 step 1.5 deg), longitude (241, -180..180 step
1.5 deg), and the two solar levels R12 = 0 and 100. `p533.readIonParameters`
returns `(24, 241, 121, 2)` single arrays indexed (hour+1, lng, lat, ssn).

## P1239-3 Decile Factors.txt

P.1239 Tables 2 (lower decile) and 3 (upper decile) of foF2 within-month
variability: 18 blocks (decile / season / R12 band) of a caption, a
"Lat." line, an hour header and 19 latitude lines (90..0 deg step 5)
with 24 local-time values. The file mixes line endings and contains a
byte-order mark and Latin-1 degree signs; `p533.readP1239` splits on any
line ending, drops blanks, replaces non-ASCII bytes and locates the blocks
by their captions. Result `(3 season, 24 hour, 19 lat, 3 ssn, 2 decile)`.

## COEFFmmW.txt

Read by `p372.readFamDud` for the noise coefficients (see the P372
package). The other 24 arrays in these files (foF2, M3000, foE, foEs,
h'F coefficients) are readable with `p372.readCoeff` but are not needed by
this implementation, which uses the grid tables above.

## Antenna patterns (VOACAP Type 11, 13, 14)

`p533.readAntenna` accepts the three text formats: a name line, a
parameter count, `MaxGain`, `Antenna Type` (11, 13 or 14), efficiency,
frequency, then gains in dBi in blocks of 10 per line. Type 11 gives one
elevation cut (0..90 deg) replicated to all azimuths with MaxGain added;
Type 13 gives 360 azimuth blocks of 91 elevations (dBi as stored, rotated
by the whole-degree bearing); Type 14 gives 30 cuts for 1..30 MHz with
MaxGain added. Patterns are `(nf, 360, 91)` arrays.

## ITURHFProp input files (.in)

`Keyword value` or `Keyword "text"` lines, `//` comments; see
`help p533.readInputConfiguration` for the keyword list. Months and hours
may be comma-separated lists; hours are 1..24 in the file.

## Reference data (tests/reference)

`cases.csv` and `ref_p533.csv`: 1 405 cases and the outputs of the ITU C
code (`tools/ref_p533.c`); `dump/`: reader and control-point dumps;
`d1_cases.csv`, `d1_measured.csv`, `d1_ref_p533.csv`, `d1_matlab.csv`: the
CCIR D1 databank comparison (`tools/make_d1_cases.py`, `tests/d1_compare.m`).
