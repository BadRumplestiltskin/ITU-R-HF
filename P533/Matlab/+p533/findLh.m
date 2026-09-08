function Lh = findLh(glat, ltime, hopRange, season)
%FINDLH Auroral and other signal losses Lh from Table 2.
%   Lh = p533.findLh(glat, ltime, hopRange, season)
%   glat geomagnetic latitude (rad, pole 78.5 N 68.2 W), ltime mid-path
%   local time (h), hopRange km (<= 2500 or greater), season 1..3
%   (P.533 section 5.2 definition). 0 dB for |Gn| < 42.5 deg.
T = p533.tables();
g = abs(glat) * 180 / pi;
if g < 42.5, Lh = 0; return; end
if g >= 77.5, band = 1; else, band = min(floor((77.5 - g) / 5) + 2, 8); end   % 77.5+ -> 1 ... 42.5-47.5 -> 8
t = mod(ltime, 24);
bin = floor(mod(t - 1, 24) / 3) + 1;            % 01-04 -> 1 ... 22-01 -> 8
rng = 1 + (hopRange > 2500);
Lh = T.Lh(rng, season, band, bin);
end
