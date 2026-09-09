function test_pointParameters()
%TEST_POINTPARAMETERS Control point parameters against the C dump.
%   Magnetic field to 1e-4 (C scales with 6371.009, P.1239 with 6371.2),
%   solar geometry to 1e-9, foF2/M3kF2 exact in the NE quadrant and within
%   the neighbouring-cell spread elsewhere (C deviation D27), foE within
%   tolerance except at night in the southern hemisphere (D08).
[dataDir, refDir] = testPaths();
M = csvread(fullfile(refDir, 'dump', 'controlpoints.csv'), 1, 0);
D2R = pi / 180; V = p533.readP1239(dataDir); %#ok<NASGU>
maps = []; curM = -1; nNE = 0; worstFoE = 0; nfoeTag = 0;
for i = 1:size(M, 1)
    m = M(i,1); h = M(i,2); s = M(i,3); lat = M(i,4)*D2R; lng = M(i,5)*D2R;
    if m ~= curM, maps = p533.readIonParameters(dataDir, m); curM = m; end
    P = p533.pointParameters(maps, m, h, s, lat, lng);
    assert(abs(P.dip100 - M(i,9)) < 1e-4 && abs(P.dip300 - M(i,10)) < 1e-4, 'dip row %d', i);
    assert(abs(P.fH100 - M(i,11)) < 1e-4 && abs(P.fH300 - M(i,12)) < 1e-4, 'fH row %d', i);
    assert(abs(P.ha - M(i,14)) < 1e-7 && abs(P.sza - M(i,16)) < 1e-7 && abs(P.decl - M(i,17)) < 1e-8 && abs(P.eot - M(i,18)) < 1e-6, 'solar row %d', i);
    if ~isnan(P.sha), assert(abs(P.sha - M(i,15)) < 1e-7 && abs(P.lsr - M(i,19)) < 1e-6 && abs(P.lsn - M(i,20)) < 1e-6 && abs(P.lss - M(i,21)) < 1e-6, 'sunrise row %d', i); end
    if lat >= 0 && lng >= 0
        assert(abs(P.foF2 - M(i,7)) < 1e-5 && abs(P.M3kF2 - M(i,8)) < 1e-5, 'foF2 NE row %d: %g vs %g', i, P.foF2, M(i,7));
        nNE = nNE + 1;
    else
        assert(abs(P.foF2 - M(i,7)) < 1.5 && abs(P.M3kF2 - M(i,8)) < 0.3, 'foF2 SW row %d: %g vs %g', i, P.foF2, M(i,7));
    end
    dfoe = abs(P.foE - M(i,6));
    if P.sza >= pi/2
        nfoeTag = nfoeTag + 1;          % D08 / D28: C night branch differs (hour+1, hemisphere test)
        if ~P.polarNight    % polar night: text uses (17e) only, C falls back to h = 0
            assert(P.foE > 0.3 * M(i,6) && P.foE < 3 * M(i,6), 'foE night row %d grossly off: %g vs %g', i, P.foE, M(i,6));
        end
    else
        assert(dfoe < 2e-3, 'foE row %d: %g vs %g (sza %g)', i, P.foE, M(i,6), P.sza*180/pi);
        worstFoE = max(worstFoE, dfoe);
    end
end
fprintf('(%d rows, %d NE exact, worst foE %.1e, %d night rows tagged D08/D28) ', size(M,1), nNE, worstFoE, nfoeTag);
end
