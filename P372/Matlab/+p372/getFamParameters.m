function FS = getFamParameters(coeff, tmblk, lng, lat, frequency)
%GETFAMPARAMETERS Atmospheric noise statistics for one 4-hour time block.
%   FS = p372.getFamParameters(coeff, tmblk, lng, lat, frequency)
%
%   Evaluates the numerical representation of the CCIR Report 322 /
%   Recommendation ITU-R P.372 atmospheric noise maps (Figures 13-36 in
%   P.372-14) for a single seasonal time block, without the interpolation
%   between adjacent blocks that p372.atmosphericNoise adds. The method
%   is from NBS Technical Note 318 (Lucas and Harper, "A numerical
%   representation of CCIR Report 322 high frequency (3-30 Mc/s)
%   atmospheric radio noise data") as implemented in REC533 and Noise.c.
%
%   Algorithm:
%     1. Fam at 1 MHz: a 15-term Fourier sine series in half the east
%        longitude (0..2*pi) gives 29 latitude coefficients ZZ(j); a
%        29-term sine series in (latitude + pi/2) plus a linear term
%        (fakabp) gives Fam(1 MHz).
%     2. Frequency scaling with the fam polynomials (p372.famFrequencyVariation).
%     3. Deciles and standard deviations: quartic polynomials in
%        log10(frequency) from dud, with frequency clamped at 20 MHz
%        (10 MHz for sigma_Fam) because the source curves stop there.
%   Loop and summation order follow the C code exactly so results agree
%   to round-off (see docs/VALIDATION.md).
%
%   Inputs:
%     coeff     - struct from p372.readFamDud.
%     tmblk     - integer 0..5, local mean time block: 0 = 0000-0400,
%                 1 = 0400-0800, ..., 5 = 2000-2400.
%     lng, lat  - receiver longitude (east positive) and latitude in
%                 RADIANS. Arrays of identical size are evaluated
%                 together (vectorised over points).
%     frequency - scalar, MHz. Valid range 0.01..30.
%   Outputs:
%     FS - struct of arrays the size of lng:
%       FA        Fam, median noise figure, dB above kT0b
%       Du, Dl    ratio of upper decile to median / median to lower
%                 decile, dB
%       SigmaDu, SigmaDl, SigmaFam  standard deviations, dB
%   Errors:
%     p372:getFamParameters  if lng and lat differ in size.
%
%   Reference: P.372-14 section 4; Noise.c GetFamParameters().
%
%   See also p372.atmosphericNoise, p372.atmosphericNoiseLT,
%   p372.famFrequencyVariation, p372.readFamDud.
sz = size(lng);
lng = lng(:); lat = lat(:);
if numel(lat) ~= numel(lng)
    error('p372:getFamParameters', 'lng and lat must have the same size.');
end
tb = tmblk + 1;                          % 1-based time block index
lm = 29; ln = 15;

% Longitude series: q is half the geographic east longitude (0..2*pi)
q = lng / 2;
neg = lng < 0;
q(neg) = (lng(neg) + 2 * pi) / 2;
ZZ = zeros(numel(q), lm);
for j = 1:lm
    R = zeros(size(q));
    for k = 1:ln
        R = R + sin(k * q) * coeff.fakp(tb, k, j);
    end
    ZZ(:, j) = R + coeff.fakp(tb, 16, j);
end

% Latitude series: q is latitude + 90 degrees
q = lat + pi / 2;
R = zeros(size(q));
for j = 1:lm
    R = R + sin(j * q) .* ZZ(:, j);
end
Fam1MHz = R + coeff.fakabp(tb, 1) + coeff.fakabp(tb, 2) * q;

% Southern hemisphere uses rows 7..12 of fam / dud
i = tb + 6 * (lat < 0);

% Frequency variation (NBS TN 318 p.5)
FA = p372.famFrequencyVariation(coeff, i, Fam1MHz, frequency);

% Deciles and sigmas. Curves in P.372 only go to 20 MHz (10 MHz for
% sigma_Fam), so the frequency is clamped.
x = log10(frequency);
if frequency > 20
    x = log10(20);
end
v = zeros(numel(q), 5);
for j = 1:5
    if j == 5 && frequency > 10
        x = 1.0;
    end
    D = reshape(coeff.dud(j, :, :), [12, 5]);
    y = D(i, 1);
    for k = 2:5
        y = y * x + D(i, k);
    end
    v(:, j) = y;
end

FS.FA = reshape(FA, sz);
FS.Du = reshape(v(:, 1), sz);
FS.Dl = reshape(v(:, 2), sz);
FS.SigmaDu = reshape(v(:, 3), sz);
FS.SigmaDl = reshape(v(:, 4), sz);
FS.SigmaFam = reshape(v(:, 5), sz);
end
