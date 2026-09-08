function AT = absorptionFactor(lat, month)
%ABSORPTIONFACTOR Absorption factor ATnoon at local noon for R12 = 0 (Figure 1).
%   AT = p533.absorptionFactor(lat, month)  lat radians (array), month 1..12.
%   Linear interpolation in |latitude| on the 2.5 deg grid of the REC533
%   table; latitudes of 70 deg and above use the 70 deg value (the figure
%   is flat over 70-90 deg).
T = p533.tables();
X = min(abs(lat) * 180 / pi, 69.999) / 2.5;
j = floor(X); f = X - j;
row = T.ATNO(T.ATNOrow(month), :);
AT = row(j + 2) .* f + row(j + 1) .* (1 - f);
end
