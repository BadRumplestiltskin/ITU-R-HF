function test_readAntenna()
%TEST_READANTENNA Type 13 reader (with bearing rotation) equals the C pattern.
[~, refDir, antDir] = testPaths();
f = fullfile(antDir, '141-10_0.t13');
b = [0, 123.4 * pi / 180];
for k = 1:2
    ant = p533.readAntenna(f, b(k));
    M = csvread(fullfile(refDir, 'dump', sprintf('antenna_t13_b%d.csv', k - 1)), 1, 0);
    P = squeeze(ant.pattern(1, :, :));
    got = P(sub2ind(size(P), M(:,1) + 1, M(:,2) + 1));
    assert(max(abs(got - M(:,3))) < 1e-9, 'antenna pattern differs (bearing %d)', k);
end
iso = p533.readAntenna('ISOTROPIC', 2.5);
assert(all(iso.pattern(:) == 2.5) && isequal(size(iso.pattern), [1 360 91]));
end
