function muf = calcF2DMUF(cp, d, dmax)
%CALCF2DMUF F2-layer basic MUF for hop length d at a control point (eq. 3).
%   muf = p533.calcF2DMUF(cp, d, dmax)
%   cp from p533.calculateCPParameters (foF2, foE, M3kF2, fH300), d km,
%   dmax km. muf = [1 + (Cd/C3000)(B - 1)] foF2 + fH/2 (1 - d/dmax).
B = p533.calcB(cp.M3kF2, cp.foF2, cp.foE);
muf = (1 + (p533.calcCd(d, dmax) / p533.calcCd(3000, dmax)) * (B - 1)) * cp.foF2 + cp.fH300 / 2 * (1 - d / dmax);
end
