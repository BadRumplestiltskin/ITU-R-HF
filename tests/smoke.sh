#!/bin/sh
# Smoke test of the two companion programs built by 'make -C Linux all':
#   CircuitCSV on tests/circuitcsv/sample.csv, compared with expected.csv
#     (four circuits: short, long-path and polar);
#   ITURNoise for one point, compared with tests/iturnoise/expected.csv.
# ITURNoise exits 0 on success.
#
# Usage: tests/smoke.sh
# Exits 0 when both pass, 1 otherwise. The library path is injected with
# env(1) rather than exported, for the reason given in tests/regression.sh.

root=$(cd "$(dirname "$0")/.." && pwd)
libdir=$root/P533/Linux:$root/P372/Linux
run="env DYLD_LIBRARY_PATH=$libdir LD_LIBRARY_PATH=$libdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
csv=$root/ITURHFProp/Src/CircuitCSV/CircuitCSV
noise=$root/P372/Src/ITURNoise/ITURNoise
fail=0

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

for exe in "$csv" "$noise"; do
	[ -x "$exe" ] || { echo "smoke: no executable at $exe - run 'make -C Linux all' first" >&2; exit 2; }
done

# CircuitCSV
status=0
$run "$csv" -s -i "$root/tests/circuitcsv/sample.csv" -o "$work/circuitcsv.csv" \
	-d "$root/P533/Data" >"$work/circuitcsv.log" 2>&1 || status=$?
in_rows=$(grep -c . "$root/tests/circuitcsv/sample.csv")
out_rows=$(grep -c . "$work/circuitcsv.csv" 2>/dev/null || true)
if [ "$status" -ne 0 ]; then
	echo "FAIL CircuitCSV (exit $status)"; sed 's/^/      /' "$work/circuitcsv.log"; fail=1
elif [ "$out_rows" != "$in_rows" ]; then
	echo "FAIL CircuitCSV ($out_rows output lines for $in_rows input lines)"; fail=1
elif ! diff -u "$root/tests/circuitcsv/expected.csv" "$work/circuitcsv.csv" >"$work/circuitcsv.diff"; then
	echo "FAIL CircuitCSV (output differs from expected.csv)"; head -40 "$work/circuitcsv.diff" | sed 's/^/      /'; fail=1
else
	echo "PASS CircuitCSV"
fi

# ITURNoise, one point, CSV output (print flag 3); the last line is the result
status=0
$run "$noise" 1 14 1.0 40 165 0 "$root/P372/Data" 3 >"$work/iturnoise.log" 2>&1 || status=$?
if [ "$status" -ne 0 ]; then
	echo "FAIL ITURNoise (exit $status, expected 0)"; sed 's/^/      /' "$work/iturnoise.log"; fail=1
elif ! tail -n 1 "$work/iturnoise.log" | diff -u "$root/tests/iturnoise/expected.csv" - >"$work/iturnoise.diff"; then
	echo "FAIL ITURNoise (result differs from expected.csv)"; sed 's/^/      /' "$work/iturnoise.diff"; fail=1
else
	echo "PASS ITURNoise"
fi

exit $fail
