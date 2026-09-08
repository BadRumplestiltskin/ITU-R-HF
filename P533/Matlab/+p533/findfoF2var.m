function [dl, du] = findfoF2var(foF2var, season, ltime, lat, R12)
%FINDFOF2VAR foF2 decile factors from P.1239 Tables 2 and 3.
%   [dl, du] = p533.findfoF2var(foF2var, season, ltime, lat, R12)
%   foF2var from p533.readP1239; season 1..3 in the P.1239 sense
%   (p533.seasonOf(lat, month, 'p1239')); ltime local time in hours
%   (0..24); lat radians (absolute value used, 0..90 deg); R12 selects the
%   band < 50, 50..100, > 100. Bilinear interpolation in local time
%   (cyclic over 24 h) and latitude (5 deg steps). dl = MUF90/MUF50,
%   du = MUF10/MUF50.
if R12 < 50, r = 1; elseif R12 <= 100, r = 2; else, r = 3; end
la = min(abs(lat) * 180 / pi, 90) / 5;
k0 = min(floor(la), 17); fk = la - k0;
t = mod(ltime, 24);
h0 = floor(t); fh = t - h0; h1 = mod(h0 + 1, 24);
out = zeros(1, 2);
for d = 1:2
    v00 = foF2var(season, h0 + 1, k0 + 1, r, d); v01 = foF2var(season, h1 + 1, k0 + 1, r, d);
    v10 = foF2var(season, h0 + 1, k0 + 2, r, d); v11 = foF2var(season, h1 + 1, k0 + 2, r, d);
    out(d) = (1 - fk) * ((1 - fh) * v00 + fh * v01) + fk * ((1 - fh) * v10 + fh * v11);
end
dl = out(1); du = out(2);
end
