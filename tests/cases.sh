#!/bin/sh
# Behaviour cases: every run of the programs that is not a normal prediction -
# bad input, malformed files, edge geometry, command-line errors - with the
# exit status and message it must produce.
#
# Each case is a directory tests/cases/<name>/ holding:
#   cmd       one shell command line, run with sh from a scratch directory
#             ($WORK, which starts as a copy of the case directory). These
#             variables are set for it:
#               $ITURHFPROP $CIRCUITCSV $ITURNOISE  the programs
#               $P533DATA   P533/Data (ionospheric maps, P.1239, COEFF files)
#               $P372DATA   P372/Data
#               $ANT        "ITURHFProp/Data/Antenna/T13 Files" (Type 13; quote it: it has a space)
#               $CASE       the case directory, read-only
#   exit      the expected exit status: a number, or "nonzero" for a failure
#             whose status is set by the platform rather than by the program
#             (the dynamic loader refusing to start it: 127 on Linux, an
#             abort on macOS)
#   expect    optional: lines that must each appear in stdout+stderr
#             (fixed strings, one per line)
#   reject    optional: lines that must NOT appear in stdout+stderr
#   out.expected  optional: must equal $WORK/out, once the lines matching
#             'ITURHFProp *Ver|Analysis Prepared' are removed from both
#
# Usage: tests/cases.sh [name-pattern]
# Exits 0 when every case passes, 1 otherwise. Library paths are injected
# with env(1) for the reason given in tests/regression.sh. Sanitizer builds
# work unchanged: any sanitizer report makes the exit status differ.

root=$(cd "$(dirname "$0")/.." && pwd)
# The C locale for every case: text tools behave the same on every machine,
# and sed/grep accept any byte a case writes into a deliberately corrupt file.
LC_ALL=C
export LC_ALL
libdir=$root/P533/Linux:$root/P372/Linux
pattern=${1:-*}

ITURHFPROP="env DYLD_LIBRARY_PATH=$libdir LD_LIBRARY_PATH=$libdir $root/ITURHFProp/Linux/ITURHFProp"
CIRCUITCSV="env DYLD_LIBRARY_PATH=$libdir LD_LIBRARY_PATH=$libdir $root/ITURHFProp/Src/CircuitCSV/CircuitCSV"
ITURNOISE="env DYLD_LIBRARY_PATH=$libdir LD_LIBRARY_PATH=$libdir $root/P372/Src/ITURNoise/ITURNoise"
P533DATA=$root/P533/Data
P372DATA=$root/P372/Data
ANT="$root/ITURHFProp/Data/Antenna/T13 Files"
export ITURHFPROP CIRCUITCSV ITURNOISE P533DATA P372DATA ANT

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

volatile='ITURHFProp *Ver|Analysis Prepared'
pass=0
fail=0

for case_dir in "$root"/tests/cases/$pattern/; do
	[ -f "$case_dir/cmd" ] || continue
	name=$(basename "$case_dir")
	WORK=$tmp/$name
	CASE=${case_dir%/}
	export WORK CASE
	mkdir -p "$WORK"
	cp -R "$case_dir". "$WORK"/

	status=0
	(cd "$WORK" && sh "$CASE/cmd") >"$tmp/$name.log" 2>&1 || status=$?

	why=
	want=$(cat "$case_dir/exit")
	if [ "$want" = nonzero ]; then
		[ "$status" -ne 0 ] || why="exit 0, expected nonzero"
	else
		[ "$status" -eq "$want" ] || why="exit $status, expected $want"
	fi
	if [ -z "$why" ] && [ -f "$case_dir/expect" ]; then
		while IFS= read -r line; do
			[ -n "$line" ] || continue
			grep -qF -- "$line" "$tmp/$name.log" || { why="missing: $line"; break; }
		done <"$case_dir/expect"
	fi
	if [ -z "$why" ] && [ -f "$case_dir/reject" ]; then
		while IFS= read -r line; do
			[ -n "$line" ] || continue
			! grep -qF -- "$line" "$tmp/$name.log" || { why="unexpected: $line"; break; }
		done <"$case_dir/reject"
	fi
	if [ -z "$why" ] && [ -f "$case_dir/out.expected" ]; then
		if [ ! -f "$WORK/out" ]; then
			why="no output file"
		else
			grep -Ev "$volatile" "$case_dir/out.expected" >"$tmp/$name.exp" || true
			grep -Ev "$volatile" "$WORK/out" >"$tmp/$name.got" || true
			diff -u "$tmp/$name.exp" "$tmp/$name.got" >"$tmp/$name.diff" || why="output differs"
		fi
	fi

	if [ -z "$why" ]; then
		echo "PASS $name"
		pass=$((pass + 1))
	else
		echo "FAIL $name ($why)"
		sed 's/^/      /' "$tmp/$name.log" | head -20
		[ -f "$tmp/$name.diff" ] && head -20 "$tmp/$name.diff" | sed 's/^/      /'
		fail=$((fail + 1))
	fi
done

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
