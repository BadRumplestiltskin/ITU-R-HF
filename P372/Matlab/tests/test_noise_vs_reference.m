function test_noise_vs_reference()
%TEST_NOISE_VS_REFERENCE Engine reproduces 24 500 C reference calculations.
%   reference/ref_points.csv covers months {1,4,7,10}, UTC hours
%   {0,5,11,17,23}, latitudes {-60..60}, longitudes {-150..165},
%   frequencies {0.01..30} MHz and man-made settings {0..5, -50}. All 12
%   outputs of p372.noise and the 6 statistics of p372.atmosphericNoiseLT
%   must agree within 1e-6 dB (observed: 5e-10).
[dataDir, refDir] = testPaths();
M = csvread(fullfile(refDir, 'ref_points.csv'), 1, 0);
D2R = p372.D2R();
tol = 1e-6;
worst = 0; worstLT = 0;
coeff = []; curMonth = -1;
for r = 1:size(M, 1)
    month = M(r, 1); hour = M(r, 2); lat = M(r, 3); lng = M(r, 4);
    freq = M(r, 5); mm = M(r, 6);
    if month ~= curMonth
        coeff = p372.readFamDud(dataDir, month);
        curMonth = month;
    end
    n = p372.noise(coeff, mm, hour, lng * D2R, lat * D2R, freq);
    got = [n.FaA n.DuA n.DlA n.FaM n.DuM n.DlM n.FaG n.DuG n.DlG n.FamT n.DuT n.DlT];
    d = max(abs(got - M(r, 7:18)));
    if d > tol
        error('row %d (m%d h%d lat%g lng%g f%g mm%g): max diff %g\n got %s\n ref %s', ...
            r, month, hour, lat, lng, freq, mm, d, mat2str(got, 10), mat2str(M(r, 7:18), 10));
    end
    worst = max(worst, d);
    if mm == 0     % LT stats do not depend on mm; check once per point
        fs = p372.atmosphericNoiseLT(coeff, hour, lng * D2R, lat * D2R, freq);
        gotLT = [fs.FA fs.Du fs.Dl fs.SigmaFam fs.SigmaDu fs.SigmaDl];
        dLT = max(abs(gotLT - M(r, 19:24)));
        if dLT > tol
            error('row %d LT stats: max diff %g', r, dLT);
        end
        worstLT = max(worstLT, dLT);
    end
end
fprintf('(%d points, worst diff %.2e, LT %.2e) ', size(M, 1), worst, worstLT);
end
