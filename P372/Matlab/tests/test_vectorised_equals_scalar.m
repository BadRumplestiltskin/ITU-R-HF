function test_vectorised_equals_scalar()
%TEST_VECTORISED_EQUALS_SCALAR Array evaluation equals point-by-point evaluation.
%   p372.getFamParameters over a lat/lng grid must give bit-identical
%   results to scalar calls, for several time blocks and frequencies.
dataDir = testPaths();
coeff = p372.readFamDud(dataDir, 7);
D2R = p372.D2R();
[LNG, LAT] = meshgrid((-180:37:180) * D2R, (-90:23:90) * D2R);
for tb = [0 3 5]
    for f = [0.02 1 12 30]
        V = p372.getFamParameters(coeff, tb, LNG, LAT, f);
        assert(isequal(size(V.FA), size(LNG)));
        for k = 1:numel(LNG)
            S = p372.getFamParameters(coeff, tb, LNG(k), LAT(k), f);
            assert(S.FA == V.FA(k) && S.Du == V.Du(k) && S.Dl == V.Dl(k) && ...
                   S.SigmaDu == V.SigmaDu(k) && S.SigmaDl == V.SigmaDl(k) && S.SigmaFam == V.SigmaFam(k));
        end
    end
end
end
