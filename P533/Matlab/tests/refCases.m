function [cases, ref, hdr] = refCases()
%REFCASES Load cases.csv and ref_p533.csv as structs of column vectors.
[~, refDir] = testPaths();
[cases, ~] = readTable(fullfile(refDir, 'cases.csv'));
[ref, hdr] = readTable(fullfile(refDir, 'ref_p533.csv'));
end
function [T, hdr] = readTable(f)
fid = fopen(f); hdr = strsplit(strtrim(fgetl(fid)), ','); fclose(fid);
txt = fileread(f); L = strsplit(strtrim(txt), char(10)); L = L(2:end);
n = numel(L); T = struct();
raw = cell(n, numel(hdr));
for i = 1:n, c = strsplit(L{i}, ','); raw(i, 1:numel(c)) = c; end
for k = 1:numel(hdr)
    col = raw(:, k); v = str2double(col);
    if all(~isnan(v)), T.(matlab_name(hdr{k})) = v; else, T.(matlab_name(hdr{k})) = col; end
end
end
function n = matlab_name(s)
n = regexprep(s, '[^A-Za-z0-9_]', '_');
end
