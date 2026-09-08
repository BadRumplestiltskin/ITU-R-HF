function path = mufBasic(path)
%MUFBASIC Basic MUFs of the E and F2 modes and of the path (sections 3.1-3.5).
%   path = p533.mufBasic(path)   (paths up to 9000 km; longer paths get
%   their MUF from p533.medianSkywaveFieldStrengthLong)
%   Sets path.CP(MP).hr (eq. 2), path.dmax (eq. 5, capped 4000 km for the
%   basic MUF), path.n0_F2, path.n0_E, the control points Td02/Rd02 for
%   D > dmax, path.Md_F2(k).BMUF/.hops/.hr and path.Md_E(k).BMUF/.hops/.hr,
%   and path.BMUF = higher of the lowest-order E and F2 MUFs.
%   F2 modes: lowest order n0 by geometry with hr of eq. (2) and the 3 deg
%   minimum elevation; for D <= dmax eq. (3) at MP (n0 and the next five);
%   for D > dmax the lower of eq. (3) at T+d0/2 and R-d0/2 (3.5.1.2; with
%   d = min(d0, dmax) so that d0 > dmax gives F2(dmax)MUF, P.1240 3.2) and
%   eqs (7)-(8) for higher orders with dmax recomputed at each control point. E modes (D <= 4000): eq. (1) with hr 110 km; foE at
%   MP or the lower of T+1000 and R-1000 for D > 2000 km.
c = p533.constants();
D = path.distance;
if D > 9000, return; end
MP = path.CP(c.cp.MP);
MP.hr = min(1490 / MP.M3kF2 - 176, 500);
[B, x] = p533.calcB(MP.M3kF2, MP.foF2, MP.foE);
MP.x = x;
path.B = B;
dmaxMP = min(p533.calcdmax(B, x), 4000);
path.dmax = dmaxMP;
path.CP(c.cp.MP) = MP;
% lowest-order F2 mode: smallest n with hop <= dmax (<= 4000 km) and elevation >= 3 deg (3.5.1.1, 5.2.1)
n0 = NaN;
for n = 1:c.MAXF2MDS
    if D / n <= dmaxMP && p533.elevationAngle(D / n, MP.hr) >= c.MINELE, n0 = n; break; end
end
if isnan(n0)
    path.n0_F2 = c.NOLOWESTMODE;
else
    path.n0_F2 = n0;
    d0 = D / n0;
    if D <= dmaxMP                       % Table 1a): control points depend on the path length
        for k = 1:c.MAXF2MDS
            n = n0 + k - 1;
            path.Md_F2(k).hops = n;
            path.Md_F2(k).hr = MP.hr;
            path.Md_F2(k).BMUF = p533.calcF2DMUF(MP, D / n, dmaxMP);
        end
    else
        % control points at d0/2 from each end (Table 1a), dmax recomputed there
        [la, lo] = p533.pathPoint(path, d0 / 2);
        path.CP(c.cp.Td02) = p533.calculateCPParameters(path, p533.newControlPoint(la, lo, d0 / 2));
        [la, lo] = p533.pathPoint(path, D - d0 / 2);
        path.CP(c.cp.Rd02) = p533.calculateCPParameters(path, p533.newControlPoint(la, lo, D - d0 / 2));
        mufs = zeros(2, c.MAXF2MDS);
        for j = 1:2
            cp = path.CP(c.cp.Td02 * (j == 1) + c.cp.Rd02 * (j == 2));
            [Bj, xj] = p533.calcB(cp.M3kF2, cp.foF2, cp.foE);
            cp.x = xj;
            dmaxj = p533.calcdmax(Bj, xj);             % may exceed 4000 km (3.5.2.2)
            f0 = p533.calcF2DMUF(cp, d0, dmaxj);                 % eq. (3) at d0 (may exceed dmax: eq. 8)
            dmaxb = min(dmaxj, 4000);                              % basic MUF: dmax limited to 4000 km (3.5.1.1)
            flow = p533.calcF2DMUF(cp, min(d0, dmaxb), dmaxb);   % lowest order: F2(d0)MUF, or F2(dmax)MUF when d0 > dmax (P.1240 3.2)
            for k = 1:c.MAXF2MDS
                n = n0 + k - 1;
                if k == 1
                    mufs(j, k) = flow;
                else
                    mufs(j, k) = flow * p533.calcF2DMUF(cp, D / n, dmaxj) / f0;   % eqs (7), (8)
                end
            end
            path.CP(c.cp.Td02 * (j == 1) + c.cp.Rd02 * (j == 2)) = cp;
        end
        for k = 1:c.MAXF2MDS
            path.Md_F2(k).hops = n0 + k - 1;
            path.Md_F2(k).hr = MP.hr;
            path.Md_F2(k).BMUF = min(mufs(:, k));
        end
    end
end
% E modes, D <= 4000 km
if D <= 4000
    if D <= 2000
        foE = MP.foE;
    else
        foE = min(path.CP(c.cp.T1k).foE, path.CP(c.cp.R1k).foE);
    end
    n0E = NaN;
    for n = 1:c.MAXEMDS
        if D / n <= 2000 && p533.elevationAngle(D / n, c.HR_E) >= c.MINELE, n0E = n; break; end
    end
    if ~isnan(n0E)
        path.n0_E = n0E;
        for k = 1:c.MAXEMDS
            n = n0E + k - 1;
            dh = D / n;
            i110 = p533.incidenceAngle(p533.elevationAngle(dh, c.HR_E), c.HR_E);
            path.Md_E(k).hops = n;
            path.Md_E(k).hr = c.HR_E;
            path.Md_E(k).BMUF = foE / cos(i110);
        end
    end
end
bE = 0; bF = 0;
if path.n0_E ~= c.NOLOWESTMODE, bE = path.Md_E(1).BMUF; end
if path.n0_F2 ~= c.NOLOWESTMODE, bF = path.Md_F2(1).BMUF; end
path.BMUF = max(bE, bF);
path.MUF50 = path.BMUF;
end
