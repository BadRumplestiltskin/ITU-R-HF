function foF2var = readP1239(dataDir)
%READP1239 Read the foF2 within-month decile factors (P.1239 Tables 2 and 3).
%   foF2var = p533.readP1239(dataDir) reads "P1239-3 Decile Factors.txt" and
%   returns a (3 season, 24 hour, 19 lat, 3 ssn, 2 decile) double array:
%     season 1 winter, 2 equinox, 3 summer (P.1239 section 3.2 definitions)
%     hour   local time 0..23
%     lat    0, 5, ..., 90 deg (index = lat/5 + 1)
%     ssn    1: R12 < 50, 2: 50 <= R12 <= 100, 3: R12 > 100
%     decile 1 lower (Table 2, MUF90/MUF50), 2 upper (Table 3, MUF10/MUF50)
%   File: two title lines, then 18 blocks (decile, season, ssn order) of a
%   blank line, a caption, a "Lat." header, an hour header and 19 latitude
%   lines 90..0 deg with 24 values.
fname = fullfile(dataDir, 'P1239-3 Decile Factors.txt');
txt = fileread(fname);
txt(double(txt) > 127) = ' ';                  % BOM, degree signs (not valid UTF-8)
lines = regexp(txt, '[\r\n]+', 'split');
lines = lines(~cellfun(@isempty, strtrim(lines)));
foF2var = zeros(3, 24, 19, 3, 2);
% blocks are located by their caption "x) foF2 variability: ..." in file
% order: decile (lower, upper) / season (winter, equinox, summer) /
% R12 band (< 50, 50..100, > 100); each caption is followed by a "Lat."
% line, an hour header and 19 latitude lines from 90 down to 0 deg.
cap = find(~cellfun(@isempty, regexp(lines, '^[a-z]\) foF2 variability', 'once')));
if numel(cap) ~= 18
    error('p533:readP1239', 'Expected 18 table blocks in %s, found %d', fname, numel(cap));
end
n = 0;
for d = 1:2
    for s = 1:3
        for r = 1:3
            n = n + 1; p = cap(n) + 3;
            for k = 19:-1:1
                v = sscanf(lines{p}, '%f');       % latitude then 24 values
                if numel(v) < 24
                    error('p533:readP1239', 'Bad line after block %d in %s', n, fname);
                end
                foF2var(s, :, k, r, d) = v(end-23:end);
                p = p + 1;
            end
        end
    end
end
end
