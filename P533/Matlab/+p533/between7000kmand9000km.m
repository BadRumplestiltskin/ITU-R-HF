function path = between7000kmand9000km(path)
%BETWEEN7000KMAND9000KM Interpolated field strength for 7000..9000 km (section 5.4).
%   path = p533.between7000kmand9000km(path)
%   Ei = 100 log Xi with Xi = Xs + (D - 7000)/2000 (Xl - Xs), Xs = 10^(0.01 Es),
%   Xl = 10^(0.01 El) (eq. 42). The path BMUF is the lower of eq. (3) at
%   the two Table 1a) control points, already set by p533.mufBasic.
D = path.distance;
if D < 7000 || D > 9000, return; end
Xs = 10 ^ (0.01 * path.Es); Xl = 10 ^ (0.01 * path.El);
path.Ei = 100 * log10(Xs + (D - 7000) / 2000 * (Xl - Xs));
end
