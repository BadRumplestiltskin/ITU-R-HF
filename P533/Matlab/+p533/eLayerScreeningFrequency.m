function path = eLayerScreeningFrequency(path)
%ELAYERSCREENINGFREQUENCY E-layer maximum screening frequency of F2 modes (section 4).
%   path = p533.eLayerScreeningFrequency(path)   (D <= 4000 km only)
%   For each F2 mode: hr from section 5.1 at MP (D <= dmax) or the mean
%   over T+d0/2, MP, R-d0/2 (Table 1c), elevation from eq. (13), incidence
%   at 110 km from eq. (12), fs = 1.05 foE sec i (eq. 11) with foE at MP
%   (D <= 2000) or the higher of T+1000 and R-1000. Stores Md_F2(k).hr and .fs.
c = p533.constants();
D = path.distance;
if D > 4000 || path.n0_F2 == c.NOLOWESTMODE, return; end
if D <= 2000
    foE = path.CP(c.cp.MP).foE;
else
    foE = max(path.CP(c.cp.T1k).foE, path.CP(c.cp.R1k).foE);
end
for k = 1:c.MAXF2MDS
    if path.Md_F2(k).BMUF <= 0, continue; end
    dh = D / path.Md_F2(k).hops;
    if D <= path.dmax
        hr = p533.mirrorReflectionHeight(path.CP(c.cp.MP), dh, path.frequency, path.SSN);
    else
        hr = mean([p533.mirrorReflectionHeight(path.CP(c.cp.Td02), dh, path.frequency, path.SSN), ...
                   p533.mirrorReflectionHeight(path.CP(c.cp.MP), dh, path.frequency, path.SSN), ...
                   p533.mirrorReflectionHeight(path.CP(c.cp.Rd02), dh, path.frequency, path.SSN)]);
    end
    path.Md_F2(k).hr = hr;
    i = p533.incidenceAngle(p533.elevationAngle(dh, hr), c.HR_E);
    path.Md_F2(k).fs = 1.05 * foE / cos(i);
end
end
