function G = antennaGain(ant, freq, azimuth, elevation)
%ANTENNAGAIN Gain (dBi) of an antenna pattern at an azimuth and elevation.
%   G = p533.antennaGain(ant, freq, azimuth, elevation)  radians
%   Uses the pattern frequency nearest to freq (ant.freqs = 0 means
%   frequency independent) and bilinear interpolation on the 1 deg grid;
%   elevation is limited to 0..90 deg, azimuth wraps.
if isscalar(ant.freqs)
    fi = 1;
else
    [~, fi] = min(abs(ant.freqs - freq));
end
P = reshape(ant.pattern(fi, :, :), 360, 91);
az = mod(azimuth * 180 / pi, 360); el = min(max(elevation * 180 / pi, 0), 90);
a0 = floor(az); fa = az - a0; a1 = mod(a0 + 1, 360);
e0 = min(floor(el), 89); fe = el - e0;
G = (1 - fa) * ((1 - fe) * P(a0 + 1, e0 + 1) + fe * P(a0 + 1, e0 + 2)) + ...
    fa * ((1 - fe) * P(a1 + 1, e0 + 1) + fe * P(a1 + 1, e0 + 2));
end
