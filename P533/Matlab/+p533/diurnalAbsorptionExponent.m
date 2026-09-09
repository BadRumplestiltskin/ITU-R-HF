function p = diurnalAbsorptionExponent(dip100, lat, month)
%DIURNALABSORPTIONEXPONENT Diurnal absorption exponent p (Figure 3).
%   p = p533.diurnalAbsorptionExponent(dip100, lat, month)
%   dip100 magnetic dip at 100 km (rad), lat radians (arrays), month 1..12.
%   Modified dip X = atan(I / sqrt(cos lat)) (P.1239 eq. 4), limited to
%   70 deg; southern hemisphere uses the month + 6. REC533 CONTP
%   polynomial fit in the dip mapped to -1..1 on either side of a monthly
%   breakpoint.
T = p533.tables();
sz = size(dip100); dip100 = dip100(:); lat = lat(:);
X = min(abs(atan2(dip100, sqrt(cos(lat)))), 70 * pi / 180);
p = zeros(size(X));
for i = 1:numel(X)
    m = month;
    if lat(i) < 0, m = mod(m + 5, 12) + 1; end
    PP = T.ppt(m) * pi / 180;
    if X(i) > PP, hi = 2; u = -1 + 2 * (X(i) - PP) / (70 * pi / 180 - PP);
    else, hi = 1; u = -1 + 2 * X(i) / PP; end
    if m <= 6, A = squeeze(T.pval1(m, hi, :)); else, A = squeeze(T.pval2(m - 6, hi, :)); end
    p(i) = sum(A(:) .* u .^ (0:6)');
end
p = reshape(p, sz);
end
