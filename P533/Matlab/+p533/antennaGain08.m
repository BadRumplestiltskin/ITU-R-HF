function [G, ele] = antennaGain08(ant, freq, azimuth)
%ANTENNAGAIN08 Largest gain over elevations 0..8 deg at an azimuth (sections 5.3.3, 6).
%   [G, ele] = p533.antennaGain08(ant, freq, azimuth)  ele radians of the maximum
els = (0:8) * pi / 180;
g = arrayfun(@(e) p533.antennaGain(ant, freq, azimuth, e), els);
[G, i] = max(g);
ele = els(i);
end
