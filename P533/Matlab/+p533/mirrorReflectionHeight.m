function hr = mirrorReflectionHeight(cp, d, f, R12)
%MIRRORREFLECTIONHEIGHT F2-layer mirror reflection height (section 5.1, eqs 14-16).
%   hr = p533.mirrorReflectionHeight(cp, d, f, R12)
%   cp control point (foF2, foE, M3kF2), d hop length km, f MHz, R12
%   (limited to 160). Returns hr in km, at most 800 km.
R12 = min(R12, 160);
x = cp.foF2 / cp.foE; y = max(x, 1.8);
dM = 0.18 / (y - 1.4) + 0.096 * (R12 - 25) / 150;
H = 1490 / (cp.M3kF2 + dM) - 316;
xr = f / cp.foF2;
if x > 3.33 && xr >= 1                                     % eq. (14)
    E1 = -0.09707 * xr ^ 3 + 0.6870 * xr ^ 2 - 0.7506 * xr + 0.6;
    if xr <= 1.71, F1 = -1.862 * xr ^ 4 + 12.95 * xr ^ 3 - 32.03 * xr ^ 2 + 33.50 * xr - 10.91;
    else, F1 = 1.21 + 0.2 * xr; end
    if xr <= 3.7, G = -2.102 * xr ^ 4 + 19.50 * xr ^ 3 - 63.15 * xr ^ 2 + 90.47 * xr - 44.73;
    else, G = 19.25; end
    A1 = 140 + (H - 47) * E1;
    B1 = 150 + (H - 17) * F1 - A1;
    ds = 160 + (H + 43) * G;
    a = (d - ds) / (H + 140);
    if B1 >= 0 && a >= 0, h = A1 + B1 * 2.4 ^ (-a); else, h = A1 + B1; end
elseif x > 3.33                                            % eq. (15), xr < 1
    Z = max(xr, 0.1);
    E2 = 0.1906 * Z ^ 2 + 0.00583 * Z + 0.1936;
    F2 = 0.645 * Z ^ 2 + 0.883 * Z + 0.162;
    A2 = 151 + (H - 47) * E2;
    B2 = 141 + (H - 24) * F2 - A2;
    df = min(0.115 * d / (Z * (H + 140)), 0.65);
    b = -7.535 * df ^ 4 + 15.75 * df ^ 3 - 8.834 * df ^ 2 - 0.378 * df + 1;
    if B2 >= 0, h = A2 + B2 * b; else, h = A2 + B2; end
else                                                       % eq. (16)
    J = -0.7126 * y ^ 3 + 5.863 * y ^ 2 - 16.13 * y + 16.07;
    U = 8e-5 * (H - 80) * (1 + 11 * y ^ (-2.2)) + 1.2e-3 * H * y ^ (-3.6);
    h = 115 + H * J + U * d;
end
hr = min(h, 800);
end
