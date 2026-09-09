function path = medianSkywaveFieldStrengthShort(path)
%MEDIANSKYWAVEFIELDSTRENGTHSHORT Field strength of paths up to 9000 km (section 5.2).
%   path = p533.medianSkywaveFieldStrengthShort(path)
%   Modes (5.2.1): up to three E modes (D <= 4000, lowest order with hop
%   <= 2000 and next two) and up to six F2 modes (lowest order with hop
%   <= dmax and next five) whose screening frequency fs < f. For each:
%     Ew = 136.6 + Pt + Gt + 20 log f - Lb                       (17)
%     Lb = 32.45 + 20 log f + 20 log p' + Li + Lm + Lg + Lh + Lz  (18)
%   with p' from (19), Li from p533.absorptionLoss (20)-(23), Lm from
%   (24)-(26), Lg = 2(n - 1) (27), Lh from Table 2 at the Table 1d)
%   control points (mean of Gn and local time), Lz = 8.72 dB.
%   Es = 10 log sum 10^(Ew/10) over the modes (28). F2 mirror heights hr:
%   eq. (2) at MP for D <= dmax, otherwise at the Table 1c) point with the
%   lower foF2. Sets Md_*.ele, Lb, Ew, MC and path.Es, path.ptick.
c = p533.constants();
D = path.distance; f = path.frequency;
if D > 9000, return; end
Pt = path.txpower;
% F2 reflection height for the geometry (5.2.1)
if D <= path.dmax
    hrF2 = path.CP(c.cp.MP).hr;
else
    cps = [path.CP(c.cp.Td02), path.CP(c.cp.MP), path.CP(c.cp.Rd02)];
    [~, im] = min([cps.foF2]);
    hrF2 = min(1490 / cps(im).M3kF2 - 176, 500);
end
% Table 1d) points for Lh
if D <= 2000, lhIdx = c.cp.MP;
elseif D <= 4000 || D <= path.dmax, lhIdx = [c.cp.T1k c.cp.MP c.cp.R1k];
else, lhIdx = [c.cp.T1k c.cp.Td02 c.cp.MP c.cp.Rd02 c.cp.R1k]; end
lhIdx = lhIdx(arrayfun(@(k) ~isnan(path.CP(k).lat), lhIdx));
gls = arrayfun(@(k) abs(p533.geomagneticCoords(path.CP(k).lat, path.CP(k).lng)), lhIdx);
lt = path.CP(c.cp.MP).ltime;
season = path.season;
fLref = mean(arrayfun(@(k) abs(path.CP(k).fH100 * sin(path.CP(k).dip100)), lhIdx));
R = p533.rules(path);
if R.ref
    LhOf = @(dh) mean(arrayfun(@(g) p533.findLh(g, lt, dh, season), gls));   % C: mean of the table values
else
    LhOf = @(dh) p533.findLh(mean(gls), lt, dh, season);                      % text: mean Gn over the points
end
Ews = [];
% E modes
for k = 1:c.MAXEMDS
    m = path.Md_E(k);
    if m.BMUF <= 0 || D > 4000, continue; end
    n = m.hops; dh = D / n;
    if k == 1 && dh > 2000, break; end
    m.ele = p533.elevationAngle(dh, c.HR_E);
    ptick = 2 * c.R0 * n * sin(dh / (2 * c.R0)) / cos(m.ele + dh / (2 * c.R0));
    Li = p533.absorptionLoss(path, n, c.HR_E, f, fLref);
    if f <= m.BMUF, Lm = 0; else, Lm = min(130 * (f / m.BMUF - 1) ^ 2, 81); end
    Lg = 2 * (n - 1);
    Lh = LhOf(dh);
    m.Lb = 32.45 + 20 * log10(f) + 20 * log10(ptick) + Li + Lm + Lg + Lh + c.LZ;
    m.Ew = 136.6 + Pt + p533.antennaGain(path.A_tx, f, path.txBearing, m.ele) + 20 * log10(f) - m.Lb;
    m.MC = true; m.ptick = ptick;
    path.Md_E(k) = m; Ews(end + 1) = m.Ew; %#ok<AGROW>
end
% F2 modes
for k = 1:c.MAXF2MDS
    m = path.Md_F2(k);
    if m.BMUF <= 0, continue; end
    n = m.hops; dh = D / n;
    if k == 1 && dh > path.dmax, continue; end
    if D <= 4000 && ~isnan(m.fs) && m.fs >= f, continue; end     % screened by the E layer
    hr = hrF2;
    m.ele = p533.elevationAngle(dh, hr);
    if m.ele < 0, continue; end
    ptick = 2 * c.R0 * n * sin(dh / (2 * c.R0)) / cos(m.ele + dh / (2 * c.R0));
    Li = p533.absorptionLoss(path, n, hr, f, fLref);
    if f <= m.BMUF, Lm = 0; else, Lm = min(36 * sqrt(f / m.BMUF - 1), 62); end
    Lg = 2 * (n - 1);
    Lh = LhOf(dh);
    m.Lb = 32.45 + 20 * log10(f) + 20 * log10(ptick) + Li + Lm + Lg + Lh + c.LZ;
    m.Ew = 136.6 + Pt + p533.antennaGain(path.A_tx, f, path.txBearing, m.ele) + 20 * log10(f) - m.Lb;
    m.MC = true; m.ptick = ptick;
    path.Md_F2(k) = m; Ews(end + 1) = m.Ew; %#ok<AGROW>
end
path.Lz = c.LZ;
if isempty(Ews)
    path.Es = c.TINYDB;
else
    path.Es = 10 * log10(sum(10 .^ (Ews / 10)));
end
end
