function path = mufVariability(path)
%MUFVARIABILITY MUF deciles and probability of ionospheric support (section 3.6).
%   path = p533.mufVariability(path)
%   For every existing mode: MUF50 = BMUF, MUF90 = MUF50 delta_l, MUF10 =
%   MUF50 delta_u, Fprob from eqs (9)-(10). F2 deciles from P.1239
%   Tables 2/3 at the mid-path local time and latitude; E modes 0.95 and
%   1.05. Path MUF90/MUF10 follow the layer that gives the path BMUF.
c = p533.constants();
if path.distance > 9000, return; end
MP = path.CP(c.cp.MP);
[dl, du] = p533.findfoF2var(path.foF2var, p533.seasonOf(MP.lat, path.month, 'p1239'), MP.ltime, MP.lat, path.SSN);
f = path.frequency;
for k = 1:c.MAXF2MDS
    if path.Md_F2(k).BMUF > 0
        path.Md_F2(k) = deciles(path.Md_F2(k), dl, du, f);
    end
end
for k = 1:c.MAXEMDS
    if path.Md_E(k).BMUF > 0
        path.Md_E(k) = deciles(path.Md_E(k), 0.95, 1.05, f);
    end
end
if path.n0_F2 ~= c.NOLOWESTMODE && path.Md_F2(1).BMUF >= path.BMUF
    m = path.Md_F2(1);
elseif path.n0_E ~= c.NOLOWESTMODE
    m = path.Md_E(1);
else
    return;
end
path.MUF50 = m.MUF50; path.MUF90 = m.MUF90; path.MUF10 = m.MUF10;
end

function m = deciles(m, dl, du, f)
m.deltal = dl; m.deltau = du;
m.MUF50 = m.BMUF; m.MUF90 = m.BMUF * dl; m.MUF10 = m.BMUF * du;
if f < m.MUF50
    m.Fprob = min(1.3 - 0.8 / (1 + (1 - f / m.MUF50) / (1 - dl)), 1);
elseif f > m.MUF50
    m.Fprob = max(0.8 / (1 + (f / m.MUF50 - 1) / (du - 1)) - 0.3, 0);
else
    m.Fprob = 0.5;
end
end
