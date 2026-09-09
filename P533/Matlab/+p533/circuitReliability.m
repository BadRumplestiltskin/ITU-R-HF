function path = circuitReliability(path)
%CIRCUITRELIABILITY Signal-to-noise, reliabilities and digital performance (Part 3).
%   path = p533.circuitReliability(path)   requires path.noise (p372.noise)
%   Section 7:  SNR = Pr - Fa - 10 log b + 204 (45) with Fa = P.372 total
%               noise FamT (see docs/P533_DEVIATIONS.md D26).
%   Section 8 / P.842 Table 1: DuSN, DlSN from the within-hour signal
%               deciles (5, 8 dB), the day-to-day deciles of P.842 Table 2
%               (f/BMUF, geomagnetic latitude >= 60 deg anywhere between
%               T+1000 and R-1000 with the P.1239 pole) and the noise
%               deciles; SNRXX for the required reliability from the
%               log-normal distribution (P.1057); BCR (Table 1 step 11).
%   Section 10.2 (digital): mode delays (47), dominant and in-window
%               modes (steps 1-4), multimode interference via P.842 Table 3
%               with protection ratio A (step 5) giving SIR, DuSI, DlSI,
%               MIR = ICR; OCR = BCR MIR/100 (DCR of the text); beyond
%               9000 km the composite spread test. Section 10.3 /
%               Attachment 1: probocc and OCRs = OCR (1 - probocc).
%   RSN, RT, RF of older editions are not part of P.533-14 and stay NaN.
c = p533.constants(); T = p533.tables();
N = path.noise; f = path.frequency; D = path.distance;
Fa = N.FamT;
% ---- geomagnetic latitude between T+1000 and R-1000 (P.842 Table 2 note 1)
gt60 = false;
if D > 2000
    dd = linspace(1000, D - 1000, max(2, ceil((D - 2000) / 100) + 1));
    [gla, glo] = p533.pathPoint(path, dd);
    g = p533.geomagneticCoords(gla, glo, c.POLE_1239);
    gt60 = any(abs(g) >= 60 * pi / 180);
else
    g = p533.geomagneticCoords(path.CP(c.cp.MP).lat, path.CP(c.cp.MP).lng, c.POLE_1239);
    gt60 = abs(g) >= 60 * pi / 180;
end
row = 1 + gt60;
if path.BMUF > 0
    ratio = f / path.BMUF;
    col = find(T.P842ratio <= ratio + 1e-9, 1, 'last'); if isempty(col), col = 1; end
else
    col = 1;
end
DlSd = T.P842LD(row, col); DuSd = T.P842UD(row, col);
DuSh = 5; DlSh = 8;
% ---- noise decile terms (P.842 Table 1 steps 6 and 9)
Fsum = 10 ^ (N.FaA / 10) + 10 ^ (N.FaM / 10) + 10 ^ (N.FaG / 10);
Nl = 10 * log10(Fsum / (10 ^ ((N.FaA - N.DlA) / 10) + 10 ^ ((N.FaM - N.DlM) / 10) + 10 ^ ((N.FaG - N.DlG) / 10)));
Nu = 10 * log10((10 ^ ((N.FaA + N.DuA) / 10) + 10 ^ ((N.FaM + N.DuM) / 10) + 10 ^ ((N.FaG + N.DuG) / 10)) / Fsum);
path.DuSN = sqrt(DuSd ^ 2 + DuSh ^ 2 + Nl ^ 2);
path.DlSN = sqrt(DlSd ^ 2 + DlSh ^ 2 + Nu ^ 2);
% ---- wanted signal power S: all modes (analog) or in-window modes (digital)
S = path.Pr;
modes = [path.Md_E, path.Md_F2];
considered = find([modes.MC]);
for k = considered
    modes(k).tau = modes(k).ptick / c.C_KM_S * 1e3;               % eq. (47), ms
end
interf = [];                                                      % out-of-window mode powers (dBW)
if path.Modulation == c.MOD.DIGITAL && D <= 9000 && ~isempty(considered)
    Prw = [modes(considered).Prw];
    [pdom, idom] = max(Prw);
    active = considered(Prw >= pdom - path.A);                    % steps 1-2
    tau = [modes(active).tau];
    tfirst = min(tau);
    inwin = active(tau <= tfirst + path.TW);                      % step 3
    S = 10 * log10(sum(10 .^ ([modes(inwin).Prw] / 10)));         % step 4
    out = setdiff(active, inwin);
    interf = [modes(out).Prw];                                    % step 5
    path.DMidx = considered(idom);
end
for k = considered
    if k <= c.MAXEMDS, path.Md_E(k) = modes(k); else, path.Md_F2(k - c.MAXEMDS) = modes(k); end
end
% ---- SNR, SNRXX, BCR (section 7, 8, P.842 Table 1)
path.SNR = S - Fa - 10 * log10(path.BW) + 204;
pxx = path.SNRXXp / 100;
if pxx >= 0.5
    z = sqrt(2) * erfinv(2 * pxx - 1);  path.SNRXX = path.SNR - z * path.DlSN / 1.282;
else
    z = sqrt(2) * erfinv(1 - 2 * pxx);  path.SNRXX = path.SNR + z * path.DuSN / 1.282;
end
path.BCR = bcr(path.SNR, path.SNRr, path.DlSN, path.DuSN);
% ---- multimode interference (P.842 Table 3 with R = A, day-to-day deciles 0)
if ~isempty(interf)
    IR = interf + path.A;
    Isum = sum(10 .^ (IR / 10));
    path.SIR = S - 10 * log10(Isum);
    DlIh = 8; DuIh = 5;
    path.DuSI = sqrt(DuSh ^ 2 + (10 * log10(Isum / sum(10 .^ ((IR - DlIh) / 10)))) ^ 2);
    path.DlSI = sqrt(DlSh ^ 2 + (10 * log10(sum(10 .^ ((IR + DuIh) / 10)) / Isum)) ^ 2);
    path.MIR = bcr(path.SIR, path.SIRr, path.DlSI, path.DuSI);   % ICR of Table 3 step 12
else
    path.SIR = c.TINYDB; path.DuSI = NaN; path.DlSI = NaN; path.MIR = 100;
end
if path.Modulation == c.MOD.DIGITAL && D > 9000 && path.TW > 0
    spread = 3 + 2 * min(max((D - 7000) / 13000, 0), 1);          % 3 ms at 7000 km to 5 ms at 20000 km
    if path.TW < spread, path.MIR = 0; end
end
path.OCR = path.BCR * path.MIR / 100;
% ---- equatorial scattering (section 10.3, Attachment 1)
path.probocc = 0;
if path.Modulation == c.MOD.DIGITAL && D <= 9000 && ~isempty(considered)
    Fmodes = considered(considered > c.MAXEMDS);
    if ~isempty(Fmodes) && (path.TW > 0 || path.FW > 0)
        Prw = [modes(considered).Prw]; pdom = max(Prw);
        scatter = false;
        for k = Fmodes
            pm = 10 ^ (modes(k).Prw / 10);
            pT = 0.056 * pm * exp(-path.TW ^ 2 / (2 * 1 ^ 2));
            pF = 0.056 * pm * exp(-path.FW ^ 2 / (2 * 3 ^ 2));
            if 10 * log10(max(pT, pF)) > pdom - path.A, scatter = true; end
        end
        if scatter
            cps = path.CP(arrayfun(@(k) ~isnan(path.CP(k).lat), 1:5));
            po = 0;
            for q = 1:numel(cps)
                po = max(po, probScatter(cps(q), path.SSN, path.month));
            end
            path.probocc = po;
        end
    end
end
path.OCRs = path.OCR * (1 - path.probocc);
end

function r = bcr(sn, snr, dl, du)
if sn >= snr, r = min(130 - 80 / (1 + (sn - snr) / dl), 100);
else, r = max(80 / (1 + (snr - sn) / du) - 30, 0); end
end

function p = probScatter(cp, R12, month)
ld = abs(cp.dip100) * 180 / pi;
if ld < 15, Fl = 1; elseif ld < 25, Fl = ((25 - ld) / 10) ^ 2 * ((ld - 10) / 5); else, Fl = 0; end
Tl = mod(cp.ltime, 24);
if Tl < 3, Ft = 1; elseif Tl < 7, Ft = ((7 - Tl) / 4) ^ 2 * ((Tl - 1) / 2); elseif Tl < 19, Ft = 0;
elseif Tl < 20, Ft = (Tl - 19) ^ 2 * (41 - 2 * Tl); else, Ft = 1; end
Fr = min(0.1 + 0.008 * R12, 1);
Fs = 0.55 + 0.45 * sin(60 * pi / 180 * (month - 1.5));
p = Fl * Ft * Fr * Fs;
end
