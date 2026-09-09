function phi = penetrationFactor(T)
%PENETRATIONFACTOR Absorption layer penetration factor phi_n(fv/foE) (Figure 2).
%   phi = p533.penetrationFactor(T), T = fv/foE (array). REC533 PHIFUN fit:
%   polynomials below and above T = 1 (both limited to 0.53 before scaling),
%   linear 2.2..10, constant beyond; scaled so that phi(10+) = 1.
phi = zeros(size(T));
a = T >= 0 & T <= 1;
X = (T(a) - 0.475) / 0.475;
phi(a) = min((((((-0.093 * X + 0.04) .* X + 0.127) .* X - 0.027) .* X + 0.044) .* X + 0.159) .* X + 0.225, 0.53);
b = T > 1 & T <= 2.2;
X = (T(b) - 1.65) / 0.55;
phi(b) = min((((((0.043 * X - 0.07) .* X - 0.027) .* X + 0.034) .* X + 0.054) .* X - 0.049) .* X + 0.375, 0.53);
c = T > 2.2 & T <= 10;
phi(c) = 0.34 + ((10 - T(c)) * 0.02) / 7.8;
phi(T > 10) = 0.34;
phi = phi / 0.34;
end
