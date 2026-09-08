function test_readFamDud()
%TEST_READFAMDUD Coefficient parser equals the C ReadFamDud() arrays exactly.
%   Compares fakp, fakabp, dud and fam for January with reference/coeff_dump_01.txt
%   (written with %.17g by tools/ref_driver.c) and checks that all twelve
%   monthly files parse to finite values.
[dataDir, refDir] = testPaths();
coeff = p372.readFamDud(dataDir, 1);
txt = fileread(fullfile(refDir, 'coeff_dump_01.txt'));
lines = strsplit(strtrim(txt), sprintf('\n'));
vals = str2double(lines);              % header lines become NaN
vals = vals(~isnan(vals));
n1 = 6*16*29; n2 = 6*2; n3 = 5*12*5; n4 = 12*14;
assert(numel(vals) == n1 + n2 + n3 + n4, 'dump size mismatch');
% dump is written in C loop order i,j,k (last index fastest)
fakp = permute(reshape(vals(1:n1), [29 16 6]), [3 2 1]);
fakabp = reshape(vals(n1+1:n1+n2), [2 6]).';
dud = permute(reshape(vals(n1+n2+1:n1+n2+n3), [5 12 5]), [3 2 1]);
fam = reshape(vals(n1+n2+n3+1:end), [14 12]).';
assert(isequal(size(coeff.fakp), [6 16 29]));
assert(max(abs(coeff.fakp(:) - fakp(:))) == 0, 'fakp differs');
assert(max(abs(coeff.fakabp(:) - fakabp(:))) == 0, 'fakabp differs');
assert(max(abs(coeff.dud(:) - dud(:))) == 0, 'dud differs');
assert(max(abs(coeff.fam(:) - fam(:))) == 0, 'fam differs');
% every month must parse
for m = 2:12
    c = p372.readFamDud(dataDir, m);
    assert(all(isfinite(c.fakp(:))) && all(isfinite(c.fam(:))));
end
end
