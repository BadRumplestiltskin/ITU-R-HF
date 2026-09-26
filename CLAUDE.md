# Working on ITU-R-HF

Guidance for Claude Code sessions in this repository. The current state of
the work, and what to do next, is in `Docs/HANDOFF.md`; read it first.

## The owner's standing instructions

- Nothing is ever pushed to the upstream (source) repository on GitHub.
  Push only to the owner's forks: `BadRumplestiltskin/ITU-R-HF` and
  `BadRumplestiltskin/p533-matlab` (the MATLAB/Octave port). In a fresh
  clone, install the pre-push guard first (see `Docs/HANDOFF.md`).
- Implement ITU-R P.533 and P.372, and the Recommendations they call on,
  exactly as stated. Where the text is ambiguous or silent, do not choose:
  put the question to the owner, with the text, what the code does and the
  numerical effect, and record the ruling in `Docs/DEVIATIONS.md` (and the
  MATLAB `P533/docs/P533_DEVIATIONS.md`).
- Never make anything up and never insert stubs or placeholders. If
  something is not known definitively, stop and ask.
- For each issue: investigate, confirm it is real, plan, fix, and prove the
  fix is complete with tests before committing.
- Document code to production quality; keep cognitive complexity low;
  match the surrounding style.
- Merges: the owner says "merge and commit #N" or "merge #N when CI is
  green". Merge only after every CI job on the head commit has passed
  (this repository has CI; p533-matlab has none). Open PRs as drafts.
- Exit codes: new error codes were chosen as unique values (79, 80, 81 in
  ReadInputConfiguration/ValidateITURHFP); keep any new ones unique.

## Build and test

```sh
make -C Linux all                      # libraries and programs
sh tests/regression.sh                 # 29 committed reports (ITURHFProp/Bin, P533/Bin)
sh tests/smoke.sh                      # CircuitCSV and ITURNoise
sh tests/cases.sh                      # behaviour cases in tests/cases (518 at 2026-09-26)
```

Two cases fail only when run as root or with mawk (`csv-long-line`,
`noise-flag-2-unwritable`); they pass for a normal user with gawk.
Sanitizer build, as CI runs it:

```sh
make -C Linux clean
make -C Linux all OPTIMIZE=-O1 CC="gcc -g -fno-omit-frame-pointer -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=undefined,float-cast-overflow"
ASAN_OPTIONS=detect_leaks=0 sh tests/regression.sh     # and the other suites
make -C Linux clean && make -C Linux all              # back to a normal build
```

`ITURHFProp/Bin` is in `.gitignore`, but its `.in`/`.out` pairs are
tracked: stage changed reports with `git add -u`. When a ruling changes
results on purpose, regenerate the affected `.out` files by running
`../Linux/ITURHFProp -s <case>.in <case>.out` in `ITURHFProp/Bin` (with
`LD_LIBRARY_PATH` set to `P533/Linux:P372/Linux`), and
`tests/circuitcsv/expected.csv` with CircuitCSV, after checking that the
differences are the ones the change should cause.

## Documents

- `Docs/DEVELOPER_GUIDE.md` - layout and flow of the engine.
- `Docs/TRACEABILITY.md` - equation and table to function.
- `Docs/DEVIATIONS.md` - rulings, open questions, probable defects.
- `tests/LEDGER.md` - the test-case ledger (units U1-U8).
