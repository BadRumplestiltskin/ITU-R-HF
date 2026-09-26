#!/bin/sh
# Run every case in ITURHFProp/Bin and P533/Bin and compare against its
# committed .out file.
#
# Two lines of each report are volatile and are filtered from both sides: the
# "ITURHFProp Ver" line carries the compiler's __DATE__, and "Analysis Prepared"
# carries the run time. Everything else - the echoed input parameters, the
# column descriptions and every computed value - must match exactly. The column
# descriptions matter: a report column was once silently dropped by a refactor
# and only the descriptions would have caught it.
#
# Usage: tests/regression.sh [bindir] [libdir]
#   bindir  directory holding the ITURHFProp executable (default ITURHFProp/Linux)
#   libdir  directory holding libp533.so and libp372.so (default P533/Linux:P372/Linux)

set -e

root=$(cd "$(dirname "$0")/.." && pwd)
bindir=${1:-$root/ITURHFProp/Linux}
libdir=${2:-$root/P533/Linux:$root/P372/Linux}
exe=$bindir/ITURHFProp

[ -x "$exe" ] || { echo "regression: no executable at $exe - run 'make -C Linux all' first" >&2; exit 2; }

# The programs are linked against libp533.so and libp372.so, and their rpath
# names the install layout, not the build tree, so the loader search path has
# to name the build directories. DYLD_LIBRARY_PATH cannot be
# exported here: macOS System Integrity Protection strips DYLD_* when it starts
# a protected binary such as /bin/sh, so the variable would never reach the
# executable. Injecting it with env(1) at exec time does reach it, because the
# engine itself is not protected. Getting this wrong makes the whole suite pass
# against stale libraries.
LD_LIBRARY_PATH=$libdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export LD_LIBRARY_PATH
run="env DYLD_LIBRARY_PATH=$libdir"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

volatile='ITURHFProp *Ver|Analysis Prepared'
pass=0
fail=0

# Each .in names its data directory relative to its own Bin directory, so every
# case has to run with that directory as the working directory.
for bin_dir in ITURHFProp/Bin P533/Bin; do
cd "$root/$bin_dir"
for in_file in *.in; do
	case_name=$bin_dir/${in_file%.in}
	ref=${in_file%.in}.out
	if [ ! -f "$ref" ]; then
		echo "SKIP $case_name (no reference output)"
		continue
	fi
	mkdir -p "$work/$bin_dir"
	# Capture the status directly: after "if !" $? is the negated status, 0.
	status=0
	$run "$exe" -s "$in_file" "$work/$case_name.out" >"$work/$case_name.log" 2>&1 || status=$?
	if [ "$status" -ne 0 ]; then
		echo "FAIL $case_name (exit $status)"
		sed 's/^/      /' "$work/$case_name.log"
		fail=$((fail + 1))
		continue
	fi
	if [ ! -s "$work/$case_name.out" ]; then
		echo "FAIL $case_name (exit 0 but the report is missing or empty)"
		fail=$((fail + 1))
		continue
	fi
	# grep exits 1 when it selects nothing, which set -e would turn into
	# an abort of the whole suite; the diff below judges the content.
	grep -Ev "$volatile" "$ref"                 >"$work/$case_name.ref.f" || true
	grep -Ev "$volatile" "$work/$case_name.out" >"$work/$case_name.new.f" || true
	if diff -u "$work/$case_name.ref.f" "$work/$case_name.new.f" >"$work/$case_name.diff"; then
		echo "PASS $case_name"
		pass=$((pass + 1))
	else
		echo "FAIL $case_name"
		head -40 "$work/$case_name.diff" | sed 's/^/      /'
		fail=$((fail + 1))
	fi
done
done

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
