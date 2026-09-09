function path = mufOperational(path)
%MUFOPERATIONAL Operational MUFs (section 3.7, P.1240-2 Table 1).
%   path = p533.mufOperational(path)
%   F2 modes: OPMUF = BMUF Rop with Rop by season (P.533 section 5.2
%   definition at the mid-point), day/night at the mid-point (local time
%   between sunrise and sunset) and e.i.r.p. <= 30 or > 30 dBW, where
%   e.i.r.p. = txpower + 30 + transmit gain at the path bearing and the
%   lowest-order F2 mode elevation. E modes: OPMUF = BMUF. OPMUF10 and
%   OPMUF90 = OPMUF times the P.1239 deciles (E: 1.05, 0.95). Path values
%   are the greater of the E and F2 lowest-order modes.
c = p533.constants();
if path.distance > 9000, return; end
MP = path.CP(c.cp.MP);
Rop = [1.30 1.20; 1.25 1.15; 1.20 1.10;    % <= 30 dBW: winter, equinox, summer x night, day
       1.35 1.25; 1.30 1.20; 1.25 1.15];   %  > 30 dBW
day = isDay(MP);
if path.n0_F2 ~= c.NOLOWESTMODE
    ele = p533.elevationAngle(path.distance / path.Md_F2(1).hops, path.Md_F2(1).hr);
    path.EIRP = path.txpower + 30 + p533.antennaGain(path.A_tx, path.frequency, path.txBearing, ele);
    row = path.season + 3 * (path.EIRP > 30);
    r = Rop(row, 1 + day);
    for k = 1:c.MAXF2MDS
        if path.Md_F2(k).BMUF > 0
            path.Md_F2(k).OPMUF = path.Md_F2(k).BMUF * r;
            path.Md_F2(k).OPMUF10 = path.Md_F2(k).OPMUF * path.Md_F2(k).deltau;
            path.Md_F2(k).OPMUF90 = path.Md_F2(k).OPMUF * path.Md_F2(k).deltal;
        end
    end
end
for k = 1:c.MAXEMDS
    if path.Md_E(k).BMUF > 0
        path.Md_E(k).OPMUF = path.Md_E(k).BMUF;
        path.Md_E(k).OPMUF10 = path.Md_E(k).OPMUF * 1.05;
        path.Md_E(k).OPMUF90 = path.Md_E(k).OPMUF * 0.95;
    end
end
cands = [];
if path.n0_F2 ~= c.NOLOWESTMODE, cands = [cands, path.Md_F2(1)]; end
if path.n0_E ~= c.NOLOWESTMODE, cands = [cands, path.Md_E(1)]; end
if isempty(cands), return; end
[~, i] = max([cands.OPMUF]);
path.OPMUF = cands(i).OPMUF; path.OPMUF10 = cands(i).OPMUF10; path.OPMUF90 = cands(i).OPMUF90;
end

function d = isDay(cp)
if cp.polarDay, d = true; return; end
if cp.polarNight, d = false; return; end
t = cp.ltime; lsr = mod(cp.lsr + cp.lng * 180 / pi / 15, 24); lss = mod(cp.lss + cp.lng * 180 / pi / 15, 24);
if lsr <= lss, d = t >= lsr && t <= lss; else, d = t >= lsr || t <= lss; end
end
