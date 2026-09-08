function [c, d] = readVdCoeffs(dataDir)
%READVDCOEFFS Read the V_d and sigma_V_d polynomial coefficient files.
%   [c, d] = p372.readVdCoeffs(dataDir)
%
%   Reads V_d.txt and sigma_V_d.txt (extracted from NTIA Report 85-173;
%   these values are not in the CCIR coefficient files).
%
%   Inputs:
%     dataDir - folder containing the two files; [] for the default.
%   Outputs:
%     c, d - 4x6x5 double indexed (season, timeBlock, coefficient), with
%            c(:,:,1) the constant term. Seasons 1..4 correspond to the
%            figure months 1, 4, 7, 10 (Dec-Feb, Mar-May, Jun-Aug,
%            Sep-Nov); time blocks 1..6 are the 4-hour LMT blocks.
%
%   File format: 24 lines of "season block a4 a3 a2 a1 a0"; the five
%   coefficients are stored highest power first and are reversed on
%   reading, as ITURNoise.c does. Tokens are converted like C atof()
%   (longest valid numeric prefix, else 0) because the shipped
%   sigma_V_d.txt contains a few "l"-for-"1" typos such as 2.45l13428E+00;
%   this keeps the c) figure data identical to the reference program.
%
%   See also p372.findVd, p372.runAtmosNoiseMonths.
c = readOne(fullfile(dataDir, 'V_d.txt'));
d = readOne(fullfile(dataDir, 'sigma_V_d.txt'));
end

function A = readOne(fname)
lines = p372.readLines(fname);
lines = lines(~cellfun(@isempty, strtrim(lines)));
A = zeros(4, 6, 5);
s = 1; tb = 1;
for n = 1:numel(lines)
    tok = strsplit(strtrim(lines{n}));
    if numel(tok) < 7
        continue;
    end
    v = cellfun(@atof, tok(3:7));
    A(s, tb, :) = v(5:-1:1);
    tb = tb + 1;
    if tb == 7
        tb = 1; s = s + 1;
    end
end
end

function v = atof(tok)
% Mimic C atof(): convert the longest valid numeric prefix, else 0.
% Needed because sigma_V_d.txt contains typos such as "2.45l13428E+00"
% (letter l for digit 1), which the C code silently reads as 2.45.
m = regexp(tok, '^[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?', 'match', 'once');
if isempty(m)
    v = 0;
else
    v = str2double(m);
end
end
