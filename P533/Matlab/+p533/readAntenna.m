function ant = readAntenna(fname, bearing)
%READANTENNA Read a VOACAP antenna pattern file (Type 11, 13 or 14).
%   ant = p533.readAntenna(fname)            pattern as stored
%   ant = p533.readAntenna(fname, bearing)   Type 13 rotated so that file
%                                            azimuth 0 points at bearing (rad)
%   ant = p533.readAntenna('ISOTROPIC', G)   isotropic with gain G dBi
%   Returns ant.name, ant.freqs (MHz; 0 for a frequency-independent
%   pattern) and ant.pattern (nf, 360, 91) dBi at 1 deg azimuth (0..359,
%   clockwise from north) and elevation (0..90).
%
%   Type 11: one elevation cut, replicated to all azimuths, MaxGain added.
%   Type 13: 360 azimuth blocks of 91 gains for one frequency; the file's
%            MaxGain is a header value and is not added (values are dBi).
%            Rotation is by the nearest whole degree, as in ITURHFProp.
%   Type 14: 30 elevation cuts for 1..30 MHz, replicated to all azimuths,
%            MaxGain added; efficiency ignored.
%   The type is taken from the "Antenna Type" header parameter.
if nargin < 2, bearing = 0; end
if strcmpi(fname, 'ISOTROPIC')
    ant = p533.isotropicPattern(bearing); return;
end
txt = fileread(fname); txt = strrep(txt, char(13), '');
L = strsplit(txt, char(10));
ant.name = strtrim(L{1});
maxG = sscanf(L{3}, '%f', 1);
atype = sscanf(L{4}, '%d', 1);
switch atype
    case 11
        ant.freqs = 0;
        v = sscanf(strjoin(L(6:15), ' '), '%f');
        cut = v(1:91) + maxG;
        ant.pattern = repmat(reshape(cut, [1 1 91]), [1 360 1]);
    case 13
        ant.freqs = sscanf(L{6}, '%f', 1);
        ant.pattern = zeros(1, 360, 91);
        shift = fix(bearing * 180 / pi);
        p = 7;
        for a = 0:359
            v = sscanf(strjoin(L(p:p+9), ' '), '%f');   % az, 90 gains, then 1
            ia = mod(shift + a, 360) + 1;
            ant.pattern(1, ia, :) = v(2:92);
            p = p + 10;
        end
    case 14
        ant.freqs = zeros(1, 30); ant.pattern = zeros(30, 360, 91);
        p = 6;
        for i = 1:30
            v = sscanf(strjoin(L(p:p+9), ' '), '%f');   % f, eff, 91 gains
            ant.freqs(i) = v(1);
            ant.pattern(i, :, :) = repmat(reshape(v(3:93) + maxG, [1 1 91]), [1 360 1]);
            p = p + 10;
        end
    otherwise
        error('p533:readAntenna', 'Unsupported antenna type %d in %s', atype, fname);
end
end
