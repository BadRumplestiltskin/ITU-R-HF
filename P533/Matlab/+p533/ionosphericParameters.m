function [foF2, M3kF2] = ionosphericParameters(maps, hourUTC, R12, lat, lng)
%IONOSPHERICPARAMETERS foF2 and M(3000)F2 at points by bilinear interpolation.
%   [foF2, M3kF2] = p533.ionosphericParameters(maps, hourUTC, R12, lat, lng)
%   maps from p533.readIonParameters; hourUTC integer 0..23; R12 sunspot
%   number; lat, lng radians (arrays of equal size).
%   Bilinear interpolation on the 1.5 deg grid (P.1144 Annex 1) with
%   longitude wrap-around; linear interpolation/extrapolation in R12
%   between the R12 = 0 and 100 maps, with R12 limited to 160 (P.533
%   section 3.4, P.1239 section 3.1).
c = p533.constants();
sz = size(lat);
inc = 1.5 * pi / 180;
u = (lat(:) + pi / 2) / inc;                 % 0..120
v = mod(lng(:) + pi, 2 * pi) / inc;          % 0..240
k0 = min(floor(u), 119); r = u - k0;         % rows k0, k0+1 (1-based +1)
j0 = floor(v); cidx = v - j0; j1 = mod(j0 + 1, 240);
h = mod(round(hourUTC), 24) + 1;
w = [(1 - r) .* (1 - cidx), (1 - r) .* cidx, r .* (1 - cidx), r .* cidx];
out = cell(1, 2); src = {maps.foF2, maps.M3kF2};
for q = 1:2
    M = src{q}; val = zeros(numel(u), 2);
    for s = 1:2
        idx = @(j, k) sub2ind(size(M), h * ones(size(j)), j + 1, k + 1, s * ones(size(j)));
        val(:, s) = w(:, 1) .* double(M(idx(j0, k0))) + w(:, 2) .* double(M(idx(j1, k0))) + ...
                    w(:, 3) .* double(M(idx(j0, k0 + 1))) + w(:, 4) .* double(M(idx(j1, k0 + 1)));
    end
    ssn = min(R12, c.MAXSSN);
    out{q} = reshape((val(:, 2) * ssn + val(:, 1) * (100 - ssn)) / 100, sz);
end
foF2 = out{1}; M3kF2 = out{2};
end
