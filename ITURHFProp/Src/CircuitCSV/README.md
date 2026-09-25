# CircuitCSV — batch circuit processing for ITU-R P.533

Reads a csv of circuit definitions, runs the P.533-14 engine on each one and
writes a csv of results. It complements `ITURHFProp`, whose `.in` format
describes a *single* circuit swept over a grid; this tool processes a *list* of
different circuits in one pass.

## Usage

    CircuitCSV -i circuits.csv -o results.csv -d /path/to/Data [options]

      -i <file>   input csv
      -o <file>   output csv
      -d <path>   directory holding the CCIR coefficients and ionospheric maps
      -t <file>   transmit antenna: a Type 13 file, or ISOTROPIC (default)
      -r <file>   receive antenna:  a Type 13 file, or ISOTROPIC (default)
      -g <dBi>    gain for an isotropic pattern (default 0.0)
      -m <factor> evaluate at factor x each MUF (default 1.0)
      -F <spec>   also scan every circuit over a set of frequencies (see below)
      -S <file>   with -F, write every scanned frequency's results to this csv
      -j <n>      run in n worker processes, 0 for one per processor (not on Windows)
      -s          silent

Antenna patterns are read once at startup and re-pointed along each circuit's
own great circle before its gains are evaluated, which is what ITURHFProp's
`AntennaOrientation "TX2RX"` does. Type 11, 13 and 14 VOACAP files are all
accepted; the type is read from the file rather than assumed.

## Input columns

Looked up by name, so column order does not matter and extra columns are
ignored. All are required.

| Column | Meaning |
|---|---|
| `txSite`, `txLat`, `txLon` | transmitter name and position, degrees, N/E positive |
| `rxSite`, `rxLat`, `rxLon` | receiver name and position |
| `year`, `month`, `day`, `hour` | month is **1-12**, hour is **0-23 UTC** |
| `SSN` or `t_Index` | the solar index, see below |
| `minTOA` | minimum take-off angle, degrees |
| `txPow` | transmitter power, **watts** |
| `reqSN` | required signal-to-noise ratio, dB |
| `rxNoise` | receiver man-made noise power, **dBW** |
| `bandW` | bandwidth, Hz |
| `percDays` | required reliability, % of days |

`day` is echoed to the output but unused: P.533 predicts monthly medians.

**Solar index.** P.533 is driven by the 12-month smoothed sunspot number R12,
which it calls SSN. The input gives it in one of two columns:

- `SSN`: taken as the SIDC version 2 sunspot number and used as it is (rounded
  to an integer).
- `t_Index`: the IPS ionospheric T index, converted with the SIDC version 2
  relation T = 4.90 + 0.670 SSN, i.e. SSN = (T - 4.90)/0.670, rounded. T falls
  below 4.90 at deep solar minimum; P.533 gives its maps for R12 from 0 upward,
  so a negative result is taken as 0.

If both columns are present `SSN` is used. The index is echoed under the name the
file gave it, and the `SSN_used` output column shows the value P.533 received:
a T index of 10 runs as SSN 8, and 120 as SSN 172.

## Row matching

**Every input row produces exactly one output row, in input order.** The input
columns are echoed ahead of the results, so the output file is the input
concatenated with the calculated fields.

A row is one csv record, not one line: there is no length limit, and a quoted
field (a site name) may contain commas, doubled quotes or line breaks. Blank
lines are skipped and do not count as rows.

Quoting follows RFC 4180: a quote opens a quoted field only as the first
character of the field (after any spaces), so a quote inside a name, as in
`Perth 12" dish`, is just a character. A quoted field that crosses a line break
is taken as malformed if it is still open at the end of the file, spans more
than 64 line breaks, or has anything but spaces after its closing quote before
the next comma or line end. That row is then only its first line, reported
`BAD_RECORD`, and the following lines are read as rows of their own, so one
stray quote never swallows the rows after it.

Nothing is ever dropped. A row that cannot be calculated still appears, with
empty result fields and a `Status` saying why:

| `Status` | Meaning |
|---|---|
| `OK` | calculated normally |
| `NO_MODE` | ran, but no propagation mode is supported; geometry columns are still filled |
| `BAD_RECORD` | an input column was missing, empty or not a number (`year`, `month`, `day` and `hour` must be whole numbers), a site name was over 255 characters, or a quoted field was left open (see above) |
| `BAD_MONTH` | `month` was outside 1-12, so the circuit was not run |
| `P533_ERROR` | the engine rejected the circuit, e.g. an out-of-range latitude |
| `FREQ_RANGE` | every characteristic frequency fell outside P.533's 1-30 MHz, so nothing was evaluated |
| `LONG_PATH` | over 9000 km: the long model applies, so `fM`/`fL` are filled instead of `BUF`/`MUF`/`OWF` |

`Circuit#` is the input file's **data row number** (1 for the first row after
the header), so it is a direct index back into the input regardless of how many
rows failed. Blank lines are skipped and do not consume a number.

## Output columns

Written in the engine's own units with `%.6g`, no fixed-point scaling.

| Column | Unit |
|---|---|
| `SSN_used` | the sunspot number P.533 was run with (see Solar index) |
| `Dist` | km |
| `Tx-Bearing`, `Rx-Bearing` | degrees from true north |
| `Mode` | dominant mode, e.g. `2F` = two-hop F2 |
| `BUF` | basic MUF of the dominant mode, MHz |
| `MUF` | operational MUF, MHz |
| `OWF` | operational MUF exceeded 90% of days (the FOT), MHz |
| `SN_BUF`, `SN_MUF`, `SN_OWF` | signal-to-noise ratio at each, dB |
| `Prob` | basic circuit reliability at the BUF, % |
| `TOA` | take-off angle of the dominant mode, degrees |
| `Losses` | basic transmission loss of the dominant mode, dB |
| `Delay_BUF` | group delay, seconds |
| `Grange_BUF` | group range, km |
| `Noise Rx`, `Noise Tx` | total noise, dB above kT0B |
| `fM`, `fL` | long-model upper and lower reference frequencies, MHz (blank on short paths) |
| `SN_fM`, `SN_fL` | signal-to-noise ratio at each, dB |
| `Status` | see Row matching above |

With `-F`, seven columns are inserted before `Status`; see Frequency scan.

## Frequency scan (`-F`, `-S`)

    CircuitCSV -i circuits.csv -o results.csv -d Data -F 2:30:0.5 -S scan.csv

`-F` evaluates every circuit at a set of frequencies, given either as a range
`start:stop:step` in MHz (values rounded to 1 kHz, `stop` included) or as a
comma separated list such as assigned frequencies, `4.5,7.1,11.2` (sorted,
duplicates dropped). All must be finite and lie within P.533's 1-30 MHz.

Why: for each frequency P.533 gives the monthly median S/N and its upper and
lower decile deviations (P.842 Table 1 Steps 3, 6 and 9). None of the three
depends on `reqSN`, and bandwidth enters only as the `10 log10(bandW)` term of
Step 3. So one scan serves every service and threshold afterwards: refer the S/N
to 1 Hz (`SN0`, dB-Hz) and compare with `reqSN + 10 log10(bandwidth)` for any
service.

The main output keeps one row per input row and gains, before `Status`:

| Column | Meaning |
|---|---|
| `LUF` | lowest scanned frequency whose median S/N reaches `reqSN`, MHz (P.533 section 9 on the scan grid); blank if none does |
| `BestF` | scanned frequency with the highest median S/N, MHz (the lowest, on a tie) |
| `SN_Best` | median S/N there, dB in `bandW` |
| `SN0_Best` | the same referred to 1 Hz, `SN_Best + 10 log10(bandW)`, dB-Hz |
| `DuSN_Best`, `DlSN_Best` | upper and lower decile deviations of the S/N there, dB |
| `BCR_Best` | basic circuit reliability there for `reqSN`, % of days |

The seven are blank on rows that never reached the engine (`BAD_RECORD`,
`BAD_MONTH`, `P533_ERROR`). `NO_MODE` and `LONG_PATH` circuits are scanned like
any other.

`LUF` follows P.533's definition, the *lowest* passing frequency, so it can be
an isolated pass window below the main usable band; the `-S` file shows the
whole curve.

`-S` writes a second csv with one row per circuit and frequency, joined to the
main output by `Circuit#`:

| Column | Unit |
|---|---|
| `Circuit#` | input data row number |
| `Freq` | MHz |
| `Mode` | dominant mode, blank beyond about 7000 km |
| `Pr` | median available receiver power, dBW |
| `Noise` | total noise, dB above kT0B |
| `SNR` | median S/N in `bandW`, dB |
| `SN0` | median S/N referred to 1 Hz, dB-Hz |
| `DuSN`, `DlSN` | upper and lower decile deviations of the S/N, dB |
| `BCR` | basic circuit reliability for `reqSN`, % of days |
| `SNRXX` | S/N exceeded on `percDays` % of days, dB |
| `Status` | `OK`, or `P533_ERROR` when the engine rejected that frequency |

Cost: each frequency is one full `P533()` call, about 0.1 ms. 2,000 circuits on
the 57-frequency grid `2:30:0.5` take 11 s (0.55 s without `-F`), so 100,000
circuits take about 9 minutes, or 45 at 0.1 MHz steps. The `-S` file is about
4.4 KB per circuit at 57 frequencies. Without `-F` the output is unchanged.

## Parallel runs (`-j`)

    CircuitCSV -i circuits.csv -o results.csv -d Data -F 2:30:0.5 -S scan.csv -j 0

`-j n` runs the rows in `n` worker processes; `-j 0` uses one per online
processor. The output, `-S` file included, is byte-identical to a serial run:
same rows, same order, same `Circuit#`.

Workers take the rows in blocks of 32 from a shared counter, so a fast core
simply takes more blocks than a slow one (this matters on machines that mix
performance and efficiency cores). Each worker writes `<out>.part<k>` (and
`<scan>.part<k>`) beside the output; the parent merges them back into input
order when all have finished and deletes them, so allow about twice the output's
size on disk during a run. Each worker loads the months it meets, so memory is
up to about 300 MB per worker. A failing worker fails the whole run with error
1009.

Measured on an Apple M4 (4 performance + 6 efficiency cores), 10,000 circuits on
the 57-frequency grid `2:30:0.5` with `-S`:

| `-j` | time | speed-up |
|---|---|---|
| 1 | 58 s | 1.0x |
| 4 | 16 s | 3.6x |
| 10 (`-j 0`) | 11 s | 5.3x |

So 1,000,000 such circuits take about 18 minutes with `-j 0`, against about 1.6
hours serially.

Processes rather than threads, because the engine keeps its month caches in
library globals. Windows has no `fork()`, so there `-j` is ignored with a
message and the run is serial.

## Notes on the calculation

**Large batches.** The input is streamed: each row is read, calculated and
written in turn, so memory does not grow with the number of circuits. Measured
peak RSS is 299 MB for 1,000 rows and the same 299 MB for 100,000 rows, and
100,000 circuits take about 46 seconds.

That ceiling is the ionospheric map cache in the P533 library. Each month's maps
are 10.7 MB and cost about 6 ms to read, so they are parsed once and shared by
every circuit that needs them. A month is therefore read at most once per run no
matter how the input is ordered -- a 100,000-row file with months shuffled across
all twelve reads twelve map files. Months load lazily, so a run touching three
months holds three. The P.372 noise coefficients are cached the same way, at
306 KB for all twelve months.

You do not need to sort the input by month.

**Three runs per circuit, not four.** BUF, MUF and OWF come from the MUF chain
alone -- `MUFBasic`, `MUFVariability` and `MUFOperational` do not depend on the
frequency of interest, since `MUFVariability` reads it only to form each mode's
`Fprob` -- so no propagation run is needed to discover them. The engine then runs
once at each of the three frequencies, which is irreducible: the signal-to-noise
ratio depends on frequency through the field strength, the absorption and the
noise, and `P533()` evaluates one frequency per call.

**Paths without a dominant mode.** P.533 sets a dominant mode only under the
short model. Between 7000 and 9000 km it interpolates and beyond 9000 km it uses
the long model, leaving `path->DMptr` NULL while the MUFs and the SNR stay
valid. So `Mode`, `TOA`, `Losses`, `Delay_BUF` and `Grange_BUF` are blank on
those circuits, and the rest of the row is filled in as usual.

**Frequencies outside 1-30 MHz.** P.533 is defined over 1 to 30 MHz. An
operational MUF above 30 MHz is common at low latitudes near solar maximum; the
frequency is still reported and only its SNR column is left empty.

**Over 9000 km.** The long model characterises a circuit by the upper and lower
reference frequencies fM and fL, and also sets the MUFs, so those rows carry
`Status` `LONG_PATH` and report `BUF`/`MUF`/`OWF` *and* `fM`/`fL` with the
signal-to-noise ratio at each. fM and fL come only from a propagation run, but they do not depend on the
frequency of interest, so a long circuit costs one run to find them plus one at
each -- the same three as a short one.

**What `-m` changes.** The frequency columns always report the true
characteristic frequency; `-m` moves only the frequency the circuit is
*evaluated* at, so the `SN_` columns are the ratio at that fraction of the
frequency printed beside them. It applies to BUF, MUF, OWF, fM and fL alike.
The default of 1.0 is correct: evaluating exactly at a MUF is well defined now
that the above-the-MUF loss follows P.533-14 equations (24)-(26).

**Group delay.** P.533 fills `Mode.tau` only on the digital-modulation branch of
`CircuitReliability()`, so for an analogue circuit it is zero. This tool
reproduces the engine's own calculation from the mode's elevation angle instead.

**Noise.** P.533 calculates noise at the receiver only; there is no
transmitter-end noise figure in the Recommendation, so `Noise Tx` repeats
`Noise Rx`. `rxNoise` is passed to P.372 as a user-supplied man-made noise
value, which forms the noise figure as `204 - value`; because P.372 expects the
magnitude, the sign of `rxNoise` is inverted on the way in.

**minTOA** is read and echoed but not applied. `MINELEANGLES` in P533.h looks
like the place it would be enforced, but nothing in the engine references it --
only `MINELEANGLEL`, in the long model, is used. So the short model applies no
minimum take-off angle at all, and a per-circuit minTOA cannot be honoured
without changing the engine.

## Verification

Circuit 1 of the sample input was checked against `ITURHFProp` driven by an
equivalent `.in` file at the same frequency and hour. Distance, take-off angle,
basic MUF, operational MUF, total noise, dominant mode, loss and SNR all agree:

    ITURHFProp  dist 6815.24  ele 2.79  BMUF 15.53  OPMUF 18.63  noise 67.00  SNR 20.12  2F2  Lb 156.29
    CircuitCSV  dist 6815.24  ele 2.79  BMUF 15.53  OPMUF 18.63  noise 67.00  SNR 20.12  2F   Lb 156.29

Note that `.in` files number months and hours from 1 and the parser subtracts
one; this tool takes the hour as 0-23 directly.
