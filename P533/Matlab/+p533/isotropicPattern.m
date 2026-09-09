function ant = isotropicPattern(G)
%ISOTROPICPATTERN Antenna structure with constant gain G (dBi) in all directions.
%   ant = p533.isotropicPattern(G)  (G default 0)
%   ant.name, ant.freqs (0 = all frequencies), ant.pattern (1, 360, 91) dBi
%   indexed (frequency, azimuth 0..359 deg, elevation 0..90 deg).
if nargin < 1, G = 0; end
ant.name = sprintf('ISOTROPIC %g dBi', G);
ant.freqs = 0;
ant.pattern = G * ones(1, 360, 91);
end
