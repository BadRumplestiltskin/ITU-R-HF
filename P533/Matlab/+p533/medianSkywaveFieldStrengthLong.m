function path = medianSkywaveFieldStrengthLong(path)
%MEDIANSKYWAVEFIELDSTRENGTHLONG Field strength of paths longer than 7000 km (section 5.3).
%   path = p533.medianSkywaveFieldStrengthLong(path)
%   5.3.1 fM: nM equal hops <= 4000 km with elevation >= 3 deg at 300 km,
%   control points at dM/2 from each end; fBM = fz + (f4 - fz) fD (29),
%   fD polynomial (30), K factor (32) with Table 3 azimuth weights and the
%   24-hour fBM history; fM = min over the two points of K fBM (31).
%   5.3.2 fL: nL equal hops <= 3000 km, 2 nL penetration points at 90 km;
%   eq. (33) with the Table 4 declination and eq. (35) hour angle, Aw from
%   Table 5, night LUF (36), and the sunset decay (37)-(38) over 24 hours.
%   5.3.3 Etl from (39)-(41) with E0 = 139.6 - 20 log p', Gtl the largest
%   transmit gain 0..8 deg, Gap limited to 15 dB, Ly = -0.14 dB.
%   With path.liRule = 'reference' (C comparison mode) the zenith angles of eq. (33) come from
%   the full solar geometry instead of eqs (34)-(35) (deviation D20).
%   For D > 9000 km the path MUF values are taken from this model (fM as
%   OPMUF, fBM as BMUF, deciles from P.1239 at the mid-path).
c = p533.constants();
D = path.distance; f = path.frequency;
if D < 7000, return; end
Tt = p533.tables();
% ---- 5.3.1 hops for fM
nM = ceil(D / 4000);
while p533.elevationAngle(D / nM, c.HREF_LONG) < c.MINELE, nM = nM + 1; end
dM = D / nM;
[latM, lngM] = p533.pathPoint(path, [dM / 2, D - dM / 2]);
% 24-hour fBM at both control points
fBM = zeros(2, 24);
for h = 0:23
    [foF2, M3k] = p533.ionosphericParameters(path.maps, h, path.SSN, latM, lngM);
    [~, fH] = p533.magfit(latM, lngM, 300);
    f4 = 1.1 * foF2 .* M3k;
    fz = foF2 + fH / 2;
    fBM(:, h + 1) = fz + (f4 - fz) * fD(dM);
end
% local noon UTC hour at each control point (Table 4 / mid-month geometry)
hourNow = path.hour;
S = p533.solarParameters(latM, lngM, path.month, hourNow);
R = p533.rules(path); refRule = R.ref;
if refRule
    noonIdx = mod(fix(12 - lngM * 180 / pi / 15) - 1, 24) + 1;   % C: int(12 - lng/15) - 1
else
    noonIdx = mod(round(S.lsn), 24) + 1;                           % UTC hour of local solar noon
end
% Table 3 weights interpolated on the path azimuth at the path centre
[latC, lngC] = p533.pathPoint(path, D / 2);
[latC2, lngC2] = p533.pathPoint(path, D / 2 + 1);
az = mod(p533.bearing(latC, lngC, latC2, lngC2), pi);
if az > pi / 2, az = pi - az; end                 % fold to 0 (N-S) .. pi/2 (E-W)
w = az / (pi / 2);                                % 1 = E-W
W = 0.2 + w * (0.1 - 0.2); X = 0.2 + w * (1.2 - 0.2); Y = 0.4 + w * (0.6 - 0.4);
fMc = zeros(1, 2); fBMc = zeros(1, 2); Kc = zeros(1, 2);
for j = 1:2
    fnoon = fBM(j, noonIdx(j)); fmin = min(fBM(j, :)); fnow = fBM(j, hourNow + 1);
    K = 1.2 + W * fnow / fnoon + X * ((fnoon / fnow) ^ (1 / 3) - 1) + Y * (fmin / fnoon) ^ 2;
    Kc(j) = K; fBMc(j) = fnow; fMc(j) = K * fnow;
end
[path.fM, jm] = min(fMc);
path.K = Kc;
[~, fH2] = p533.magfit(latM, lngM, 300);
path.fH = mean(fH2);
% ---- 5.3.2 fL
nL = ceil(D / 3000); dL = D / nL;
d300 = p533.elevationAngle(dL, c.HREF_LONG);
i90 = asin(c.R0 * cos(d300) / (c.R0 + c.HPEN));
dpen = c.R0 * (pi / 2 - d300 - i90);
dist = zeros(1, 2 * nL);
for n = 1:nL, dist(2 * n - 1) = (n - 1) * dL + dpen; dist(2 * n) = n * dL - dpen; end
[latP, lngP] = p533.pathPoint(path, dist);
delta = Tt.subsolar(path.month) * pi / 180;
ptick = 2 * c.R0 * nL * sin(dL / (2 * c.R0)) / cos(d300 + dL / (2 * c.R0));
Aw = p533.winterAnomaly(latC, path.month);
fLN = sqrt(D / 3000);
fL24 = zeros(1, 24);
for h = 0:23
    if refRule                                   % D20: C uses the full solar geometry
        Sh = p533.solarParameters(latP, lngP, path.month, h);
        coschi = cos(Sh.sza);
    else                                         % eqs (34), (35) with Table 4
        eta = (h / 12 - 1) * pi + lngP;
        coschi = sin(latP) * sin(delta) + cos(latP) * cos(delta) .* cos(eta);
    end
    s = sum(sqrt(max(coschi, 0)));
    v = 5.3 * sqrt((1 + 0.009 * path.SSN) * s / (cos(i90) * log(9.5e6 / ptick))) - path.fH;
    fL24(h + 1) = max(v * (Aw + 1), fLN);
end
% day-to-night decay (37), (38)
tr = [];
for h = 1:24
    hp = mod(h - 2, 24) + 1;
    if fL24(h) < 2 * fLN && fL24(hp) > 2 * fLN, tr = h; break; end
end
if ~isempty(tr)
    hp = mod(tr - 2, 24) + 1;
    e = exp(-0.23);
    dt = (2 * fLN - fL24(tr)) / (fL24(hp) - fL24(tr));
    new = e * fL24(hp) * (dt * (1 - e) + e);
    fL24(tr) = max(fL24(tr), new);
    for n = 1:3
        idx = mod(tr - 1 + n, 24) + 1; prev = mod(tr - 2 + n, 24) + 1;
        fL24(idx) = max(fL24(idx), fL24(prev) * e);
    end
end
if refRule, path.fL = fL24(mod(hourNow + 1, 24) + 1); else, path.fL = fL24(hourNow + 1); end   % C picks hour + 1
% ---- 5.3.3 field strength
[Gtl, eleG] = p533.antennaGain08(path.A_tx, f, path.txBearing);
path.Gtl = Gtl;
path.E0 = 139.6 - 20 * log10(ptick);
path.Gap = min(10 * log10(D / (c.R0 * abs(sin(D / c.R0)))), 15);
path.Ly = c.LY;
fM = path.fM; fL = path.fL; fH = path.fH;
path.F = 1 - (fM + fH) ^ 2 / ((fM + fH) ^ 2 + (fL + fH) ^ 2) * ((fL + fH) ^ 2 / (f + fH) ^ 2 + (f + fH) ^ 2 / (fM + fH) ^ 2);
path.El = path.E0 * path.F - 30 + path.txpower + Gtl + path.Gap - path.Ly;
path.ptickLong = ptick;
if D > 9000
    path.BMUF = fBMc(jm); path.MUF50 = path.BMUF; path.OPMUF = path.fM;
    [dl, du] = p533.findfoF2var(path.foF2var, p533.seasonOf(latC, path.month, 'p1239'), S.ltime(1), latC, path.SSN);
    path.MUF90 = path.BMUF * dl; path.MUF10 = path.BMUF * du;
    path.OPMUF90 = path.OPMUF * dl; path.OPMUF10 = path.OPMUF * du;
    path.ele = eleG;
end
end

function v = fD(dM)
C = [-2.40074637494790e-24 25.8520201885984e-21 -92.4986988833091e-18 102.342990689362e-15 22.0776941764705e-12 87.4376851991085e-9 29.1996868566837e-6];
v = ((((((C(1) * dM + C(2)) * dM + C(3)) * dM + C(4)) * dM + C(5)) * dM + C(6)) * dM + C(7)) * dM;
end
