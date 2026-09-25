BEGIN { n = split(show, S, ",") }
END { print "ROWS " rows + 0 }
/Calculated Parameters/ { on = !on; next }
!on || NF == 0 { next }
{ rows++ }
{
	gsub(/ /, "")
	m = split($0, f, ",")
	for (i = 1; i <= m; i++) {
		v = tolower(f[i])
		if (v ~ /nan|inf/) print "BAD " $0
		else if (f[i] + 0 == 99.9 && f[i] ~ /^-?[0-9.]+$/) print "BAD 99.9 in column " i ": " $0
	}
	# The dominant mode's basic loss can never be below the free-space loss over
	# a vertical hop, 2 hr: the slant range p' of eq. (19) is at least 2 hr.
	if (f[49] != "NONE" && f[49] != "" && f[54] + 0 > 0 && f[52] + 0 < 32.45 + 20*log(f[3])/log(10) + 20*log(2*f[54])/log(10) - 0.01)
		print "BAD loss " f[52] " below free space for 2 hr = " 2*f[54] " km at " f[3] " MHz, hour " f[2]
	line = ""
	for (k = 1; k <= n; k++) {
		split(S[k], p, ":")
		v = f[p[1]]
		if (p[2] != "") v = sprintf("%." p[2] "f", v)
		line = line (k > 1 ? " " : "") v
	}
	print line
}
