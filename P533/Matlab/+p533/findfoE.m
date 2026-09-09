function foE = findfoE(lat, S, month, hourUTC, R12, cMode)
%FINDFOE Monthly median E-layer critical frequency (P.1239-4 section 4).
%   foE = p533.findfoE(lat, S, month, hourUTC, R12)
%   lat radians (array), S from p533.solarParameters for the same points,
%   month 1..12, hourUTC, R12 sunspot number (clamped to 160).
%   Implements eqs (12)-(18): (foE)^4 = A B C D with
%     A = 1 + 0.0094 (Phi - 66), Phi = 63.7 + 0.728 R12 + 0.00089 R12^2 (P.371)
%     B = cos^m N, N = min(|lat - decl|, 80 deg), m by |lat| < 32 deg
%     C = X + Y cos lat, (23, 116) or (92, 35)
%     D = cos^p chi (chi <= 73), cos^p(chi - dchi) (73..90),
%         night: greater of 0.072^p exp(-1.4 h) and 0.072^p exp(25.2 - 0.28 chi);
%         polar night (Sun does not rise): 0.072^p exp(25.2 - 0.28 chi)
%     p = 1.31 (|lat| <= 12 deg) else 1.20; floor foE^4 >= 0.004 (1 + 0.021 Phi)^2
%   h = hours after sunset (from the UTC hour and the sunset time S.lss).
%   cMode (optional, false): reproduce the ITU C night branch for reference
%   comparisons: h counted from the UTC hour + 1 and the polar-night
%   selection of CalculateCPParameters.c (deviations D08, D28).
cst = p533.constants();
D2R = pi / 180;
R12 = min(R12, cst.MAXSSN);
phi = 63.7 + 0.728 * R12 + 0.00089 * R12 ^ 2;
A = 1 + 0.0094 * (phi - 66);
low = abs(lat) < 32 * D2R;
m = (-1.93 + 1.92 * cos(lat)) .* low + (0.11 - 0.49 * cos(lat)) .* ~low;
N = min(abs(lat - S.decl), 80 * D2R);
B = cos(N) .^ m;
C = (23 + 116 * cos(lat)) .* low + (92 + 35 * cos(lat)) .* ~low;
p = 1.31 * (abs(lat) <= 12 * D2R) + 1.20 * (abs(lat) > 12 * D2R);
chi = S.sza; chid = chi / D2R;
D = zeros(size(lat));
i1 = chid <= 73;
D(i1) = cos(chi(i1)) .^ p(i1);
i2 = chid > 73 & chid < 90;
dchi = 6.27e-13 * (chid(i2) - 50) .^ 8 * D2R;
D(i2) = cos(chi(i2) - dchi) .^ p(i2);
i3 = chid >= 90;
if nargin < 6, cMode = false; end
% hours after sunset: sunset UTC S.lss, current UTC hourUTC (both mod 24)
if cMode
    hh = mod(hourUTC + 1, 24);
    h = zeros(size(S.lss));
    a = S.lss >= S.lsr & hh >= S.lss & hh >= S.lsr; h(a) = hh - S.lss(a);
    b = S.lss < S.lsr & hh >= S.lss & hh < S.lsr; h(b) = hh - S.lss(b);
    d = S.lss >= S.lsr & hh < S.lss & hh < S.lsr; h(d) = 24 - S.lss(d) + hh;
    h(isnan(S.lss)) = 0;
else
    h = mod(hourUTC - S.lss, 24);
end
d17d = 0.072 .^ p .* exp(-1.4 * h);
d17e = 0.072 .^ p .* exp(25.2 - 0.28 * chid);
Dn = max(d17d, d17e);
if cMode
    polar = (lat > 72.5622 * D2R & any(month == [11 12 1])) | (lat < 72.5622 * D2R & any(month == [5 6 7]));
    Dn(polar) = d17e(polar);
else
    Dn(S.polarNight) = d17e(S.polarNight);
end
D(i3) = Dn(i3);
foE = max((A * B .* C .* D) .^ 0.25, (0.004 * (1 + 0.021 * phi) ^ 2) ^ 0.25);
end
