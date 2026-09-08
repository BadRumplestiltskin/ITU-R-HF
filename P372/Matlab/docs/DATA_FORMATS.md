# Data file formats

All files live in `Data/` and are read at run time; nothing is
pre-converted.

## COEFFmmW.txt (mm = 01..12)

Monthly ionospheric and noise coefficients harmonised by P. Suessmann
for the ITU-R HF prediction software (renamed from `itucofmm.txt`).
Plain text, about 2940 lines, structure:

    month =  1 ITU Ionospheric coefficients      <- title line
    if2(10)                                      <- section header
        11    35    53 ...                       <- 5 values per line
    xf2(13,76,2)
      0.12345678E+01  ...
    ...

Each section header is a Fortran array name with its dimensions. The
values that follow are in Fortran column-major order (first index
fastest), five per line, the last line possibly shorter. The 28
sections and their dimensions are listed in `help p372.readCoeff`.

The noise engine needs only four sections (`p372.readFamDud`):

| Section | Fortran dims | Returned as | Meaning |
|---|---|---|---|
| `fakp` | (29,16,6) | `fakp(6,16,29)` | Fourier coefficients of the 1 MHz map per time block |
| `fakabp` | (2,6) | `fakabp(6,2)` | linear term of the latitude series |
| `dud` | (5,12,5) | `dud(5,12,5)` | decile / sigma polynomials (stat, block[+6 south], power) |
| `fam` | (14,12) | `fam(12,14)` | frequency-variation polynomials (block[+6 south], coeff) |

`p372.readFamDud` returns the arrays in the index order of the C code
(time block first); `p372.readCoeff` returns them in Fortran order.
The relation is `readFamDud.fakp = permute(readCoeff.fakp, [3 2 1])`.

The C reader skips a fixed number of lines per section; the MATLAB
reader locates the section headers by name, which is robust to blank
lines and gives identical arrays (verified by `test_readFamDud`).

`COEFFmmW.BIN` are binary copies of the same data used by other ITU
programs; they are shipped but not read.

## V_d.txt and sigma_V_d.txt

24 lines each: `season block a4 a3 a2 a1 a0`, seasons 1..4 (Dec-Feb,
Mar-May, Jun-Aug, Sep-Nov), blocks 1..6, and the five coefficients of a
quartic in `log10(f MHz)`, highest power first. Extracted from NTIA
Report 85-173. `p372.readVdCoeffs` reverses the coefficients so that
index 1 is the constant term.

**Data quality note.** `sigma_V_d.txt` contains four tokens where the
digit `1` was replaced by the letter `l` (lines 2-4, 3-4, 3-6, 2-2 in
season-block terms, e.g. `2.45l13428E+00`, `l.65289800E-01`). The ITU C
program reads these with `atof()`, which stops at the first invalid
character, so it uses 2.45 and 0. `p372.readVdCoeffs` reproduces this
so that the c) figure data matches. Correcting the file would change
`sigma_V_d` for those season/time blocks.

## Other files in Data/

`FOF2CCIR.DAW`, `FOF2URSI.DAW`, `FOF2DALW.BIN`, `P1239-3 Decile
Factors.txt`: ionospheric data for P.533 / P.1239, not used by P372.

## Output CSV files (`p372.runAtmosNoiseMonths`)

Written to `<root>/{a,b,c}/csv/<x>_<m>m<h>h.csv` with a one-line header
and comma-space separated `%5.4f` numbers, matching ITURNoise.exe:

- `a`: `month,hour,freq,latitude,longitude,FaA`; 65 341 rows, latitude
  -90..90 outer loop, longitude -180..180 inner loop.
- `b`: `month,hour,freq,latitude,longitude,Fam5,Fam10,...,Fam100`; 41 rows.
- `c`: `month,hour,freq,latitude,longitude,FaA,DuA,DlA,sigmaFaA,sigmaDuA,sigmaDlA,V_d,sigma_V_d`; 41 rows.
