function [dip, fH] = magfit(lat, lng, height)
%MAGFIT Magnetic dip and gyrofrequency from the P.1239 epoch-1960 field model.
%   [dip, fH] = p533.magfit(lat, lng, height)
%   lat, lng radians (arrays of equal size), height km (100 or 300 typically).
%   dip in radians (positive downward), fH in MHz (P.1239 eqs 5-11:
%   fH = 2.8 F, F in gauss). Sixth-order spherical harmonic model with the
%   Gauss coefficients g, h of the ITU software (MAGFIT.FOR of REC533),
%   which are the 1960-epoch values P.1239 section 2 requires. Field scale
%   R = 6371.2 / (6371.2 + height) (P.1239 eq. 8).
c = p533.constants();
G = [0 0.304112 0.024035 -0.031518 -0.041794 0.016256 -0.019523
     0 0.021474 -0.051253 0.062130 -0.045298 -0.034407 -0.004853
     0 0 -0.013381 -0.024898 -0.021795 -0.019447 0.003212
     0 0 0 -0.0064960 0.007008 -0.000608 0.021413
     0 0 0 0 -0.002044 0.002775 0.001051
     0 0 0 0 0 0.000697 0.000227
     0 0 0 0 0 0 0.001115];
H = [0 0 0 0 0 0 0
     0 -0.057989 0.033124 0.014870 -0.011825 -0.000796 -0.005758
     0 0 -0.001579 -0.004075 0.010006 -0.002000 -0.008735
     0 0 0 0.000210 0.000430 0.004597 -0.003406
     0 0 0 0 0.001385 0.002421 -0.000118
     0 0 0 0 0 -0.001218 -0.001116
     0 0 0 0 0 0 -0.000325];
CT = [0 0 0.33333333 0.266666666 0.25714286 0.25396825 0.25252525
      0 0 0 0.200000000 0.22857142 0.23809523 0.24242424
      0 0 0 0 0.14285714 0.19047619 0.21212121
      0 0 0 0 0 0.11111111 0.16161616
      0 0 0 0 0 0 0.09090909
      0 0 0 0 0 0 0
      0 0 0 0 0 0 0];
sz = size(lat); lat = lat(:); lng = lng(:); np = numel(lat);
AR = c.RMAG / (c.RMAG + height);
sl = sin(lat); cl = cos(lat);
P = zeros(np, 7, 7); DP = zeros(np, 7, 7);   % (point, M+1, N+1)
P(:, 1, 1) = 1;
Fx = zeros(np, 1); Fy = Fx; Fz = Fx;
for N = 1:6
    SZ = zeros(np, 1); SX = SZ; SY = SZ;
    for M = 0:N
        m = M + 1; n = N + 1;
        if N == M
            P(:, m, n) = cl .* P(:, m - 1, n - 1);
            DP(:, m, n) = cl .* DP(:, m - 1, n - 1) + sl .* P(:, m - 1, n - 1);
        elseif N ~= 1
            P(:, m, n) = sl .* P(:, m, n - 1) - CT(m, n) * P(:, m, n - 2);
            DP(:, m, n) = sl .* DP(:, m, n - 1) - cl .* P(:, m, n - 1) - CT(m, n) * DP(:, m, n - 2);
        else
            P(:, m, n) = sl .* P(:, m, n - 1);
            DP(:, m, n) = sl .* DP(:, m, n - 1) - cl .* P(:, m, n - 1);
        end
        cm = cos(M * lng); sm = sin(M * lng);
        SZ = SZ + P(:, m, n) .* (G(m, n) * cm + H(m, n) * sm);
        SX = SX + DP(:, m, n) .* (G(m, n) * cm + H(m, n) * sm);
        SY = SY + M * P(:, m, n) .* (G(m, n) * sm - H(m, n) * cm);
    end
    Fz = Fz + AR ^ (N + 2) * (N + 1) * SZ;
    Fx = Fx - AR ^ (N + 2) * SX;
    Fy = Fy + AR ^ (N + 2) * SY;
end
Fy = Fy ./ cl;
dip = reshape(atan(Fz ./ sqrt(Fx .^ 2 + Fy .^ 2)), sz);
fH = reshape(2.8 * sqrt(Fx .^ 2 + Fy .^ 2 + Fz .^ 2), sz);
end
