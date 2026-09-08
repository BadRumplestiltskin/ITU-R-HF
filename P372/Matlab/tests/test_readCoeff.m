function test_readCoeff()
%TEST_READCOEFF Generic coefficient reader: dimensions, consistency, selection.
%   Checks array sizes, that the four noise arrays agree with
%   p372.readFamDud after reordering, and that the names filter works.
dataDir = testPaths();
C = p372.readCoeff(dataDir, 1);
assert(isequal(C.if2, [11 35 53 63 67 71 75 77 79 80]) || numel(C.if2) == 10);
assert(isequal(size(C.xf2), [13 76 2]));
assert(isequal(size(C.fakp), [29 16 6]) && isequal(size(C.ccr), [8 7 6]));
% consistency with readFamDud (C index order vs Fortran order)
F = p372.readFamDud(dataDir, 1);
assert(isequal(permute(C.fakp, [3 2 1]), F.fakp));
assert(isequal(C.fakabp.', F.fakabp));
assert(isequal(permute(C.dud, [3 2 1]), F.dud));
assert(isequal(C.fam.', F.fam));
% selective read
S = p372.readCoeff(dataDir, 6, {'xe', 'sys2'});
assert(isfield(S, 'xe') && isfield(S, 'sys2') && ~isfield(S, 'xf2'));
assert(isequal(size(S.xe), [9 22 2]));
end
