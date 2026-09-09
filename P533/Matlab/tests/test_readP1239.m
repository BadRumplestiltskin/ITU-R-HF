function test_readP1239()
%TEST_READP1239 Decile factor reader equals the C foF2var array exactly.
[dataDir, refDir] = testPaths();
V = p533.readP1239(dataDir);
assert(isequal(size(V), [3 24 19 3 2]));
M = csvread(fullfile(refDir, 'dump', 'foF2var.csv'), 1, 0);
for i = 1:size(M, 1)
    v = V(M(i,1)+1, M(i,2)+1, M(i,3)+1, M(i,4)+1, M(i,5)+1);
    assert(abs(v - M(i,6)) < 1e-9, 'foF2var mismatch at row %d', i);
end
end
