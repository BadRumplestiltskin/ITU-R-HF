function test_readIonParameters()
%TEST_READIONPARAMETERS ionosNN.bin reader equals the C arrays (subset and sums).
[dataDir, refDir] = testPaths();
for m = [1 7]
    maps = p533.readIonParameters(dataDir, m);
    assert(isequal(size(maps.foF2), [24 241 121 2]));
    fid = fopen(fullfile(refDir, 'dump', sprintf('ionos_%02d_subset.csv', m))); fgetl(fid);
    C = textscan(fid, '%s%s%s%s%f%f', 'Delimiter', ','); fclose(fid);
    n = numel(C{1}) - 1;
    for i = 1:n
        h = str2double(C{1}{i}) + 1; j = str2double(C{2}{i}) + 1; k = str2double(C{3}{i}) + 1; s = str2double(C{4}{i}) + 1;
        assert(double(maps.foF2(h, j, k, s)) == C{5}(i) || abs(double(maps.foF2(h, j, k, s)) - C{5}(i)) < 1e-6, 'foF2 mismatch at %d,%d,%d,%d', h, j, k, s);
        assert(abs(double(maps.M3kF2(h, j, k, s)) - C{6}(i)) < 1e-6, 'M3kF2 mismatch');
    end
    assert(abs(sum(double(maps.foF2(:))) - C{5}(n + 1)) < 1e-3 * abs(C{5}(n + 1)) * 1e-6 + 0.05, 'foF2 sum differs');
    assert(abs(sum(double(maps.M3kF2(:))) - C{6}(n + 1)) < 0.05, 'M3kF2 sum differs');
end
end
