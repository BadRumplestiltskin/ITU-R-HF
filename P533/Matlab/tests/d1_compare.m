function d1_compare(outFile, rules)
%D1_COMPARE Run the CCIR D1 databank cases through the MATLAB engine.
%   d1_compare(outFile)  writes id,rule,Ep,Es,El,BMUF per case for the
%   absorption readings 'p533-14' and 'reference' (docs/VALIDATION.md).
%   Inputs: tests/reference/d1_cases.csv (from tools/make_d1_cases.py).
if nargin < 2, rules = {'p533-14', 'reference'}; end
here = fileparts(mfilename('fullpath'));
addpath(fileparts(here));
for cand = {fullfile(fileparts(fileparts(here)), 'P372'), fullfile(fileparts(fileparts(fileparts(here))), 'P372', 'Matlab')}
    if exist(fullfile(cand{1}, '+p372'), 'dir'), addpath(cand{1}); end
end
fid = fopen(fullfile(here, 'reference', 'd1_cases.csv')); hdr = strsplit(strtrim(fgetl(fid)), ','); fclose(fid);
cases = refCasesFile(fullfile(here, 'reference', 'd1_cases.csv'));
fo = fopen(outFile, 'w'); fprintf(fo, 'id,rule,Ep,Es,El,BMUF,distance\n');
t0 = tic;
for r = 1:numel(rules)
    path = [];
    for i = 1:numel(cases.id)
        path = setupCase(cases, i, path);
        path.liRule = rules{r}; path.sigmaRule = 'reference';
        try
            path = p533.run(path, 'part2');
            fprintf(fo, '%d,%s,%.4f,%.4f,%.4f,%.4f,%.1f\n', cases.id(i), rules{r}, path.Ep, path.Es, path.El, path.BMUF, path.distance);
        catch err
            fprintf(fo, '%d,%s,NaN,NaN,NaN,NaN,NaN\n', cases.id(i), rules{r});
            fprintf('case %d (%s): %s\n', cases.id(i), rules{r}, err.message);
        end
        if mod(i, 500) == 0, fprintf('%s: %d of %d (%.0f s)\n', rules{r}, i, numel(cases.id), toc(t0)); end
    end
end
fclose(fo);
fprintf('done in %.0f s -> %s\n', toc(t0), outFile);
end

function T = refCasesFile(f)
fid = fopen(f); hdr = strsplit(strtrim(fgetl(fid)), ','); fclose(fid);
txt = fileread(f); L = strsplit(strtrim(txt), char(10)); L = L(2:end);
n = numel(L); raw = cell(n, numel(hdr));
for i = 1:n, c = strsplit(L{i}, ','); raw(i, 1:numel(c)) = c; end
T = struct();
for k = 1:numel(hdr)
    col = raw(:, k); v = str2double(col);
    if all(~isnan(v)), T.(hdr{k}) = v; else, T.(hdr{k}) = col; end
end
end
