function [B, x] = calcB(M3kF2, foF2, foE)
%CALCB Factor B of eq. (6) and the ratio x = max(foF2/foE, 2).
%   [B, x] = p533.calcB(M3kF2, foF2, foE)
if foE > 0, x = max(foF2 / foE, 2); else, x = 2; end
B = M3kF2 - 0.124 + (M3kF2 ^ 2 - 4) * (0.0215 + 0.005 * sin(7.854 / x - 1.9635));
end
