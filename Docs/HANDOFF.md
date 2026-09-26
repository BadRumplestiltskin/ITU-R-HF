# Handoff: state of the work (2026-09-26)

Where the ITU-R-HF fork and the MATLAB port stand, and what comes next, so
that a new session can continue without the previous conversation. The
owner's standing instructions are in `CLAUDE.md` at the repository root.

## Repositories

| Repository | Default branch | Role |
|---|---|---|
| `BadRumplestiltskin/ITU-R-HF` | `master` | the owner's fork of the ITU C code (P533, P372, ITURHFProp, CircuitCSV, ITURNoise) |
| `BadRumplestiltskin/p533-matlab` | `main` | the MATLAB/GNU Octave port (`P533/+p533`, `P372/+p372`) |

Both were worked on the branch `claude/blissful-bardeen-mp8gl3`. The
MATLAB port reads the fork's data and reports through the environment
variable `ITU_R_HF` (a checkout of the fork).

In a fresh clone of each repository, install a pre-push guard so that
nothing can go to any other remote (the owner's instruction):

```sh
cat > .git/hooks/pre-push <<'HOOK'
#!/bin/sh
# Refuse any push whose URL is not one of the owner's forks.
case "$2" in
  *github.com/BadRumplestiltskin/ITU-R-HF|*github.com/BadRumplestiltskin/ITU-R-HF.git) exit 0 ;;
  *github.com/BadRumplestiltskin/p533-matlab|*github.com/BadRumplestiltskin/p533-matlab.git) exit 0 ;;
  *) echo "pre-push: refusing push to $2 (only the BadRumplestiltskin forks)" >&2; exit 1 ;;
esac
HOOK
chmod +x .git/hooks/pre-push
```

## Done

- Test ledger (`tests/LEDGER.md`) units worked through, including U2
  (exit codes 79, 80, 81) and U7 (MakeNoise output path).
- C and MATLAB compared over the 1 405 reference cases
  (`p533-matlab/P533/tests/reference/cases.csv`); every unexplained
  difference was traced to a defect or an ambiguity and resolved.
- Engine documented in both repositories (function headers,
  `Docs/DEVELOPER_GUIDE.md`, `Docs/TRACEABILITY.md`, `Docs/DEVIATIONS.md`;
  the MATLAB equivalents under `P533/docs` and `P372/docs`).
- Probable defects of `Docs/DEVIATIONS.md` section 4: fixed or ruled
  (NoiseDriver.c deleted, MakeNoise path, Magfit heights, eq. (3) hop
  limited to dmax, `struct Beam` and `PathData.B` removed; ReadType13 not
  adding Max Gain confirmed correct). Items 3 and 4 wait on P.372.
- 16 of the 18 ambiguities in `Docs/DEVIATIONS.md` section 3 ruled by the
  owner on 2026-09-26 (listed in section 1; MATLAB entries D42-D45 and the
  "Ruled" marks in its open-question list). After them the two codebases
  still agree on all 1 405 cases: path BMUF, MUF90 and MUF10 identical,
  Pr within 0.05 dB, S/N within 0.37 dB (the remaining S/N difference is
  the P.372 decile rule, below).
- Man-made noise for a user-supplied value: the fork took the City decile
  deviations the right way round in ea4ff1f (DuM 11.0, DlM 6.7), but the
  MATLAB port still reproduced the old swap; it was ported on 2026-09-26.
  The 1 405-case comparison does not exercise this input; the CircuitCSV
  comparison (`test_circuitcsv_vs_fork`, rxNoise in dBW) does, and now
  agrees exactly.

## Open, in order

1. **P.372-17 text.** The owner said it was attached on 2026-09-26, but no
   P.372 file reached the session; ask for it again. With it, settle:
   - the decile rule: Noise.c replaces the log-normal sigma by
     sigma_T = c sqrt(2 ln(alpha_T/gamma_T)) when a component decile exceeds
     12 dB; the MATLAB port (`sigmaRule 'p372-17'`, its default) treats it
     as a maximum. This is the S/N difference left in the comparison;
   - `Docs/DEVIATIONS.md` section 3 item 1: Fa in eq. (45) (P.372 total
     FamT, or the P.842-5 Table 1 step 3 power sum; D26);
   - section 4 items 3 and 4: the adjacent 4-hour block in AtmosphericNoise
     and FamT = min(...) in Noise();
   - the P.372 references listed in `p533-matlab/P372/docs/TRACEABILITY.md`
     (all marked unverified) and its five open questions.
2. **Section 3 item 2** (not yet put to the owner): where exactly 7000 and
   9000 km fall in the section 6 distance bands.
3. **MATLAB open questions** not covered by the 2026-09-26 rulings: items
   1, 6, 12, 13, 14, 15, 17, 18, 19 of the list at the end of
   `p533-matlab/P533/docs/P533_DEVIATIONS.md`.
4. **P.1240** (not in hand): the Rop table, its season and day/night
   selection. The two codebases select the Rop season differently (C: the
   P.1239-4 seasons at mid-path; MATLAB: the P.533-14 section 5.2 seasons),
   noted in `Docs/DEVIATIONS.md` section 5. P.1144 and P.371 are also not
   in hand.

## Recommendation texts

The owner supplied P.533-14, P.842-5 and P.1239-4 as PDFs in the session
that did this work. Uploads do not survive the session: ask the owner to
attach them again when a question needs the text. `pdftotext`
(poppler-utils) extracts them; render a page with `pdftoppm -f N -l N -png`
to read an equation whose layout the text extraction loses.

## Comparing C and MATLAB

```sh
p533-matlab/P533/tools/compare/run_compare.sh <ITU-R-HF checkout> <output dir>
```

builds the fork, runs the 1 405 cases through both implementations and
prints the comparison (see the script's header). Run it after any change
to either engine; the expected result is no case outside tolerance and
identical path MUFs. The MATLAB test suites:

```sh
cd p533-matlab/P533/tests && ITU_R_HF=<ITU-R-HF checkout> octave --no-gui --quiet --eval "addpath('..'); addpath('../../P372'); addpath('.'); run_all_tests"
cd p533-matlab/P372/tests && ITU_R_HF=<ITU-R-HF checkout> octave --no-gui --quiet --eval "addpath('..'); addpath('.'); run_all_tests"
```

(12 and 9 tests, about 10 and 5 minutes in GNU Octave 8.4; install with
`apt-get update && apt-get install -y --no-install-recommends octave`.)
The CCIR D1 databank check is described in
`p533-matlab/P533/docs/VALIDATION.md` section 3.
