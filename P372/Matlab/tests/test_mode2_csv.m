function test_mode2_csv()
%TEST_MODE2_CSV Figure data files are byte-identical to the C program output.
%   Regenerates a_1m0h.csv, b_1m0h.csv and c_1m0h.csv in a temporary folder
%   with p372.runAtmosNoiseMonths and compares the text with
%   reference/*.csv from tools/ref_mode2.c.
[dataDir, refDir] = testPaths();
outRoot = fullfile(tempdir(), sprintf('p372_test_%d', round(rand * 1e9)));
evalc('p372.runAtmosNoiseMonths(dataDir, outRoot, 1, 0);');
files = {fullfile('a', 'csv', 'a_1m0h.csv'), fullfile('b', 'csv', 'b_1m0h.csv'), fullfile('c', 'csv', 'c_1m0h.csv')};
refs = {'a_1m0h.csv', 'b_1m0h.csv', 'c_1m0h.csv'};
for k = 1:3
    got = fileread(fullfile(outRoot, files{k}));
    ref = fileread(fullfile(refDir, refs{k}));
    ref = strrep(ref, char(13), '');
    if ~strcmp(got, ref)
        G = csvread(fullfile(outRoot, files{k}), 1, 0);
        R = csvread(fullfile(refDir, refs{k}), 1, 0);
        assert(isequal(size(G), size(R)), '%s: size differs', refs{k});
        d = max(abs(G(:) - R(:)));
        error('%s: text differs, max numeric diff %g (rounding at 4 decimals)', refs{k}, d);
    end
end
rmdir(outRoot, 's');
end
