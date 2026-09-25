BEGIN { FS = ","; n = split(show, S, ",") }
NR == 1 { for (i = 1; i <= NF; i++) col[$i] = i; first = col["Circuit#"]; next }
{
	for (i = first; i <= NF; i++) {
		v = tolower($i)
		if (v ~ /nan|inf/) print "BAD row " NR-1 " " $i
		else if ($i ~ /^-?[0-9.e+-]+$/ && ($i + 0 == 99.9 || $i + 0 > 1e6 || $i + 0 < -1e6)) print "BAD row " NR-1 " value " $i
	}
	st = $col["Status"]
	if (st !~ /^(OK|NO_MODE|BAD_RECORD|BAD_MONTH|P533_ERROR|FREQ_RANGE|LONG_PATH)$/) print "BAD row " NR-1 " status " st
	if (mono != "") {
		if (NR > 2 && $col[mono] + 0 < last) print "BAD row " NR-1 " " mono " decreased"
		last = $col[mono] + 0
	}
	line = ""
	for (k = 1; k <= n; k++) {
		split(S[k], p, ":")
		v = $col[p[1]]
		if (p[2] != "" && v != "") v = sprintf("%." p[2] "f", v)
		line = line (k > 1 ? " " : "") v
	}
	print line
}
