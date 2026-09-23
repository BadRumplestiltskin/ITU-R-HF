# CCIR Data Bank D1

Measured median sky-wave field strengths, dB(1 µV/m) normalised for 1 kW EIRP,
compiled January 1989. `dbank_d1.txt` is the original document; its header notes
that D1 supersedes data banks A, B, C and D. 181 circuits, 1613 circuit-month
cases, 24 hourly values each, with 99 marking a missing hour.

## What is here

| File | Status |
|---|---|
| `dbank_d1.txt` | the databank document, authoritative |
| `D1_Table1.csv` | circuit geometry: id, terminals, frequency, distance, SSN, year, month |
| `D1_Table2.csv` | the measurements, one row per circuit-year-month, 24 hourly values |
| `D1_Table3.csv` | monthly smoothed sunspot number by year, 1964 onward |
| `Catagories.csv` | the stratification the P.1148 report groups by |
| `D1_1148.py` | the comparison itself, per Recommendation ITU-R P.1148. **Works, runs under python3 unmodified** |
| `D1Comp.csv` | predictions to compare. Tracked, but produced by an engine that is not identified anywhere |
| `D1_GenComp.py`, `D1_GenList.py`, `D1_InputFiles.py`, `D1_InputFiles_REC.py` | earlier or partial stages, see below |

## Running the comparison

`D1_1148.py` reads `D1Comp.csv`, `D1_Table1.csv` and `D1_Table2.csv` **in
lockstep**, one line at a time, so `D1Comp.csv` must hold exactly one row per
`D1_Table1.csv` row, in the same order:

    ID,YY,MM,<24 predicted field strengths>

separated by CR, not LF. Then:

    python3 macos-build/d1_makecomp.py <ITURHFProp> <libdir> ITURHFProp/D1/D1Comp.csv
    cd ITURHFProp/D1 && python3 D1_1148.py <a name for the engine>

which writes `D1 to <name> <timestamp>.txt`: mean difference and standard
deviation by frequency group, distance band, geomagnetic latitude, sunspot
number, season, midpoint local time and contributing organisation, then a
per-circuit table.

`D1Comp.csv` is tracked, so regenerating it in place replaces the shipped
predictions; pass a different output path, or restore it with `git checkout`.
Reports for this engine and for the shipped file are kept in `macos-build/`.

## What is missing

The script that turned ITURHFProp output into `D1Comp.csv` — referred to in
`D1_1148.py`'s own comments as `ITUHFProp_parse.py` and `REC_parse.py` — is not
in the repository. `macos-build/d1_makecomp.py` replaces it by driving the
engine directly rather than parsing saved reports.

`D1_InputFiles.py` reads a `D1.csv` that is also absent; `D1_Table1.csv` holds
the same fields. It and `D1_InputFiles_REC.py` write `D1.bat` and `D1_REC.bat`,
which drive ITURHFProp over `ITU_in\` and `ITU_out\`. Those two runners were
committed holding absolute paths from a contributor's machine; since they are
generated rather than written, they have been deleted and are now ignored. An
earlier variant of the comparison, `D1_1148_00.py`, has been deleted as well.
Both are recoverable from history if the generators are ever revived.

## Long-path circuits

Ten circuits are tabulated the long way round — their transmitter name ends
`LP`, and the distance column is the great-circle complement. Predicting them
over the short path produces field strengths near −400 dB(1 µV/m). Both
`d1_makecomp.py` and `d1_absolute.py` choose the sense whose great-circle
distance matches the tabulated one.
