#!/bin/sh
# Shared by the read-* cases (this directory has no cmd, so cases.sh skips it).
#
# mkant.sh TYPE [MAXG] - writes a well-formed VOACAP antenna file of TYPE 11,
# 13 or 14 to stdout, laid out as ReadType11/13/14() in
# P533/Src/P533/ReadType13.c read it. Gains are chosen so the gain a run
# reports shows which part of the file was used:
#   11  every elevation 3.0 dBi, plus MAXG (the header's max gain)
#   13  azimuth blocks 0 and 1 (the main beam once rotated) 10.0 dBi, every
#       other azimuth -20.0 dBi; MAXG is written but a Type 13 ignores it
#   14  the block for f MHz (f = 1 .. 30) is f dBi at every elevation, plus MAXG
type=$1
maxg=${2:-0.0}
awk -v type="$type" -v maxg="$maxg" '
function row(first, n, g,   s, k) {
	s = first
	for (k = 0; k < n; k++) s = s sprintf(" %7.3f", g)
	print s
}
function block(first, g,   j) {
	row(first, 10, g)
	for (j = 10; j < 90; j += 10) row("       ", 10, g)
	row("       ", 1, g)
}
BEGIN {
	if (type == 11) {
		print "Synthetic whip :type 11 gain table versus elevation angle"
		print "  3     3 parameters"
		printf " %6.2f  [ 1] Max Gain dBi..:\n", maxg
		print "   11    [ 2] Antenna Type..: 91 values gain in elevation angle follows"
		print "  -4.8   [ 3] Efficiency (for IONCAP)"
		block("", 3.0)
	} else if (type == 13) {
		print "Synthetic beam :type 13 360-degree gain table"
		print " 4     4 parameters"
		printf " %6.3f  [ 1] Max Gain dBi..:\n", maxg
		print "   13    [ 2] Antenna Type..: 360 x 91 gain values follow"
		print "   0.0   [ 3] Efficiency (for IONCAP)"
		print " 6.100  [ 4] Frequency"
		for (a = 0; a < 360; a++) block(sprintf("%3d", a), (a < 2) ? 10.0 : -20.0)
	} else if (type == 14) {
		print "Synthetic yagi :type 14"
		print "  3     3 parameters"
		printf " %6.2f  [ 1] Max Gain dBi..:\n", maxg
		print "  14    [ 2] Antenna Type..: 30 x (efficiency + 91 gain values) follow"
		print "  14.0  [ 3] Frequency"
		for (f = 1; f <= 30; f++) block(sprintf("%3d -0.57", f), f)
	}
}'
