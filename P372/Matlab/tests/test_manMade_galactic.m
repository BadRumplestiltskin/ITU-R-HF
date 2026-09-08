function test_manMade_galactic()
%TEST_MANMADE_GALACTIC Table values of the man-made and galactic models.
%   Checks every category constant, the user-value branch (including the
%   swapped deciles of the C code) and the negative-value bypass of p372.noise.
f = 1.0;
exp_c  = [76.8 72.5 67.2 53.6 83.2 65.2];
exp_Du = [11.0 10.6 9.2 9.2 11.0 9.2];
exp_Dl = [6.7 5.3 4.6 4.6 6.7 4.6];
for cat = 0:5
    [FaM, DuM, DlM] = p372.manMadeNoise(cat, f);
    assert(abs(FaM - exp_c(cat+1)) < 1e-12 && DuM == exp_Du(cat+1) && DlM == exp_Dl(cat+1), ...
        'category %d wrong', cat);
end
[FaM, DuM, DlM] = p372.manMadeNoise(2, 10);
assert(abs(FaM - (67.2 - 27.7)) < 1e-12);
% user value branch (with the swapped deciles of the C code)
[FaM, DuM, DlM] = p372.manMadeNoise(150, 10);
assert(FaM == 54 && DlM == 11.0 && DuM == 6.7);
[FaG, DuG, DlG] = p372.galacticNoise(10);
assert(abs(FaG - 29) < 1e-12 && DuG == 2 && DlG == 2);
% override path of noise()
n = p372.noise([], -50, 0, 0, 0, 1);
assert(n.FamT == 50 && n.FaM == -50 && n.FaA == 0 && n.DuT == 0);
end
