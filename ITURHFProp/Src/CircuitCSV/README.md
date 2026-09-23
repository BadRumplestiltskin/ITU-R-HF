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
      -s          silent

Antenna patterns are read once at startup and reused for every circuit.

## Input columns

Looked up by name, so column order does not matter and extra columns are
ignored. All are required.

| Column | Meaning |
|---|---|
| `txSite`, `txLat`, `txLon` | transmitter name and position, degrees, N/E positive |
| `rxSite`, `rxLat`, `rxLon` | receiver name and position |
| `year`, `month`, `day`, `hour` | month is **1-12**, hour is **0-23 UTC** |
| `t_Index` | the 12-month smoothed sunspot number R12, which P.533 calls SSN |
| `minTOA` | minimum take-off angle, degrees |
| `txPow` | transmitter power, **watts** |
| `reqSN` | required signal-to-noise ratio, dB |
| `rxNoise` | receiver man-made noise power, **dBW** |
| `bandW` | bandwidth, Hz |
| `percDays` | required reliability, % of days |

`day` is echoed to the output but unused: P.533 predicts monthly medians.

## Row matching

**Every input row produces exactly one output row, in input order.** The input
columns are echoed ahead of the results, so the output file is the input
concatenated with the calculated fields.

Nothing is ever dropped. A row that cannot be calculated still appears, with
empty result fields and a `Status` saying why:

| `Status` | Meaning |
|---|---|
| `OK` | calculated normally |
| `NO_MODE` | ran, but no propagation mode is supported; geometry columns are still filled |
| `BAD_RECORD` | the record was short, so some input columns were missing |
| `BAD_MONTH` | `month` was outside 1-12, so the circuit was not run |
| `P533_ERROR` | the engine rejected the circuit, e.g. an out-of-range latitude |

`Circuit#` is the input file's **data row number** (1 for the first row after
the header), so it is a direct index back into the input regardless of how many
rows failed. Blank lines are skipped and do not consume a number.

## Output columns

Written in the engine's own units with `%.6g`, no fixed-point scaling.

| Column | Unit |
|---|---|
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
| `Status` | see Row matching above |

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

**Three runs per circuit.** The characteristic frequencies do not depend on the
frequency of interest but the signal-to-noise ratio does, so the engine runs on
a seed frequency to obtain the MUFs and then once at each of BUF, MUF and OWF.
The mode, take-off angle, loss, delay and noise are reported from the BUF run.

**The step at the MUF.** A mode supported just below a MUF is screened just
above it, and the engine's loss jumps by about 8 dB across that boundary. For
one test circuit, 15.525567 MHz gives an SNR of 20.12 dB and 15.5256 MHz gives
12.18 dB. Evaluating exactly at a MUF therefore sits on the step and the result
depends on rounding. Use `-m 0.99` to sit clear of it.

**Group delay.** P.533 fills `Mode.tau` only on the digital-modulation branch of
`CircuitReliability()`, so for an analogue circuit it is zero. This tool
reproduces the engine's own calculation from the mode's elevation angle instead.

**Noise.** P.533 calculates noise at the receiver only; there is no
transmitter-end noise figure in the Recommendation, so `Noise Tx` repeats
`Noise Rx`. `rxNoise` is passed to P.372 as a user-supplied man-made noise
value, which forms the noise figure as `204 - value`; because P.372 expects the
magnitude, the sign of `rxNoise` is inverted on the way in.

**minTOA** is read and echoed, but P.533 applies its own minimum elevation angle
(`MINELEANGLES`, 3 degrees) which is set at compile time.

## Verification

Circuit 1 of the sample input was checked against `ITURHFProp` driven by an
equivalent `.in` file at the same frequency and hour. Distance, take-off angle,
basic MUF, operational MUF, total noise, dominant mode, loss and SNR all agree:

    ITURHFProp  dist 6815.24  ele 2.79  BMUF 15.53  OPMUF 18.63  noise 67.00  SNR 20.12  2F2  Lb 156.29
    CircuitCSV  dist 6815.24  ele 2.79  BMUF 15.53  OPMUF 18.63  noise 67.00  SNR 20.12  2F   Lb 156.29

Note that `.in` files number months and hours from 1 and the parser subtracts
one; this tool takes the hour as 0-23 directly.
