function test_readVdCoeffs()
%TEST_READVDCOEFFS V_d coefficient reader equals the C parse (typos included).
%   Compares with reference/vd_coeffs.txt written by tools/ref_mode2.c.
[dataDir, refDir] = testPaths();
[c, d] = p372.readVdCoeffs(dataDir);
v = load(fullfile(refDir, 'vd_coeffs.txt'));
% C dump order: s, tb, i with i fastest
cref = permute(reshape(v(1:120), [5 6 4]), [3 2 1]);
dref = permute(reshape(v(121:240), [5 6 4]), [3 2 1]);
assert(max(abs(c(:) - cref(:))) == 0, 'V_d coefficients differ');
assert(max(abs(d(:) - dref(:))) == 0, 'sigma_V_d coefficients differ');
[V, S] = p372.findVd(1.0, squeeze(c(1,1,:)), squeeze(d(1,1,:)));
assert(V == c(1,1,1) && S == d(1,1,1));
end
