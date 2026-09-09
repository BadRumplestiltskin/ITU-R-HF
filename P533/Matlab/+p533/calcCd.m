function Cd = calcCd(d, dmax)
%CALCCD Distance factor Cd of eq. (4).
%   Cd = p533.calcCd(d, dmax), Z = 1 - 2 d/dmax.
Z = 1 - 2 * d / dmax;
Cd = 0.74 - 0.591 * Z - 0.424 * Z ^ 2 - 0.090 * Z ^ 3 + 0.088 * Z ^ 4 + 0.181 * Z ^ 5 + 0.096 * Z ^ 6;
end
