function Li = absorptionLoss(path, hops, hr, f, fLref)
%ABSORPTIONLOSS Ionospheric absorption loss Li of an n-hop mode (eq. 20).
%   Li = p533.absorptionLoss(path, hops, hr, f)
%   Li = p533.absorptionLoss(path, hops, hr, f, fLref)   fLref: mean fL over the
%        Table 1d) control points, used only by the 'reference' rule
%   Penetration points: two per hop at 90 km with a 300 km reflection height
%   (section 5.2, Li). At each point j: ATjnoon (Figure 1), F(chi_j) /
%   F(chi_jnoon) with F = max(cos^p(0.881 chi), 0.02) and chi limited to
%   102 deg (eq. 21), p from Figure 3, fL = |fH sin I| at 100 km (eq. 23),
%   phi_n(fv/foE_j) (Figure 2) with fv = f cos i (eq. 22), i the angle of
%   incidence at 110 km for the mode's actual reflection height hr.
%   Li = (1 + 0.0067 R12) sec i sum_j ATjnoon/(f + fLj)^2 F(chi_j)/F(chi_jnoon) phi_n.
%   path.liRule selects the reading of the sum (docs/P533_DEVIATIONS.md D30):
%     'hopmean'   (default) n hops times the mean of the term over the 2n
%                 points, each with its own fL: the reading validated against
%                 the CCIR D1 databank (docs/VALIDATION.md), equivalent to
%                 the P.533-12 control-point form;
%     'sum'       the sum over all m = 2n penetration points as printed in
%                 P.533-14 (twice the value; biased by -5 to -10 dB on D1);
%     'reference' as 'hopmean' but with the ITU C code conventions: fL
%                 averaged over the Table 1d) control points and the
%                 penetration points located from the mode's own reflection
%                 height instead of 300 km (D31).
c = p533.constants();
D = path.distance; dh = D / hops;
R12 = min(path.SSN, c.MAXSSN);
delta = p533.elevationAngle(dh, hr);
i110 = p533.incidenceAngle(delta, c.HR_E);
fv = f * cos(i110);
% ground distance from a terminal to the 90 km penetration point of a hop with 300 km reflection
R = p533.rules(path); ref = R.ref;
if ref, hgeo = hr; else, hgeo = c.HREF_LONG; end      % D31: C uses the mode's hr
d300 = p533.elevationAngle(dh, hgeo);
psi90 = asin(c.R0 * cos(d300) / (c.R0 + c.HPEN)) ;              % incidence at 90 km
theta = pi / 2 - d300 - psi90;                                  % Earth-centre angle to penetration
dpen = c.R0 * theta;
dist = zeros(1, 2 * hops);
for n = 1:hops
    dist(2 * n - 1) = (n - 1) * dh + dpen;
    dist(2 * n) = n * dh - dpen;
end
[lat, lng] = p533.pathPoint(path, dist);
P = p533.pointParameters(path.maps, path.month, path.hour, path.SSN, lat, lng, ref);
AT = p533.absorptionFactor(lat, path.month);
p = p533.diurnalAbsorptionExponent(P.dip100, lat, path.month);
chi = min(P.sza, 102 * pi / 180);
chin = zeros(size(lat));
for k = 1:numel(lat)
    Sk = p533.solarParameters(lat(k), lng(k), path.month, P.lsn(k));
    chin(k) = Sk.sza;
end
Fchi = max(cos(0.881 * chi) .^ p, 0.02);
Fnoon = max(cos(0.881 * chin) .^ p, 0.02);
fL = abs(P.fH100 .* sin(P.dip100));
phi = p533.penetrationFactor(fv ./ P.foE);
terms = AT .* Fchi ./ Fnoon .* phi;
if ref
    if nargin < 5 || isempty(fLref), fLref = mean(fL); end
    Li = (1 + 0.0067 * R12) / cos(i110) * hops * mean(terms) / (f + fLref) ^ 2;
elseif R.liSum
    Li = (1 + 0.0067 * R12) / cos(i110) * sum(terms ./ (f + fL) .^ 2);
else
    Li = (1 + 0.0067 * R12) / cos(i110) * hops * mean(terms ./ (f + fL) .^ 2);
end
end
