function path = newPath()
%NEWPATH Default path structure for the P.533 prediction.
%   path = p533.newPath() returns a struct with every input at a documented
%   default and every output at its sentinel, mirroring the ITU PathData
%   structure. Inputs (units):
%     name, txname, rxname   char
%     year                   1900..2100 (informational)
%     month 1..12            hour 0..23 UTC          SSN R12 (0..311)
%     frequency MHz (1..30)  BW Hz                   txpower dB(1 kW)
%     Modulation 0 analog / 1 digital                SorL 0 short / 1 long
%     SNRr required S/N dB   SNRXXp required reliability %  SIRr required S/I dB
%     A amplitude ratio dB   TW time window ms       FW frequency window Hz
%     T0 time spread ms      F0 frequency spread Hz  (digital, section 10.2.1)
%     L_tx, L_rx             struct lat, lng radians
%     A_tx, A_rx             antenna structs (p533.readAntenna / isotropicPattern)
%     manMadeNoise           P.372 category 0..5 or negative override
%     sigmaRule              passed to p372.noise ('p372-17' default)
%     liRule                 'hopmean' (default) | 'sum' | 'reference', see p533.rules
%     maps, foF2var, coeff372   data loaded by p533.loadData
%   Outputs are documented in docs/API.md; all dB outputs start at TINYDB.
c = p533.constants();
path.name = ''; path.txname = ''; path.rxname = '';
path.year = 2020; path.month = 1; path.hour = 0; path.SSN = 50;
path.frequency = 10; path.BW = 3000; path.txpower = 0;
path.Modulation = c.MOD.ANALOG; path.SorL = c.SORL.SHORT;
path.SNRr = 10; path.SNRXXp = 90; path.SIRr = 20;
path.A = 0; path.TW = 0; path.FW = 0; path.T0 = 0; path.F0 = 0;
path.L_tx = struct('lat', 0, 'lng', 0); path.L_rx = struct('lat', 0, 'lng', 0);
path.A_tx = p533.isotropicPattern(0); path.A_rx = p533.isotropicPattern(0);
path.manMadeNoise = c.MM.RURAL; path.sigmaRule = 'p372-17'; path.liRule = 'hopmean';
path.maps = []; path.foF2var = []; path.coeff372 = [];
% outputs
T = c.TINYDB;
path.season = NaN; path.distance = NaN; path.ptick = NaN; path.dmax = NaN; path.B = NaN;
path.ele = NaN; path.BMUF = NaN; path.MUF50 = NaN; path.MUF90 = NaN; path.MUF10 = NaN;
path.OPMUF = NaN; path.OPMUF90 = NaN; path.OPMUF10 = NaN;
path.n0_F2 = c.NOLOWESTMODE; path.n0_E = c.NOLOWESTMODE;
path.Es = T; path.El = T; path.Ei = T; path.Ep = T; path.Pr = T; path.Lz = c.LZ;
path.E0 = T; path.Gap = NaN; path.Ly = c.LY; path.fM = NaN; path.fL = NaN; path.fH = NaN;
path.Gtl = T; path.K = [NaN NaN];
path.SNR = T; path.DuSN = NaN; path.DlSN = NaN; path.SNRXX = T;
path.SIR = T; path.DuSI = NaN; path.DlSI = NaN; path.RSN = NaN; path.RT = NaN; path.RF = NaN;
path.BCR = NaN; path.OCR = NaN; path.OCRs = NaN; path.MIR = NaN; path.probocc = NaN;
path.Grw = T; path.EIRP = T; path.DMidx = NaN; path.LUF = NaN;
path.txBearing = NaN; path.rxBearing = NaN; path.ptickLong = NaN;
path.noise = [];
path.CP = repmat(p533.newControlPoint(), 1, 5);
path.Md_E = repmat(p533.newMode(), 1, c.MAXEMDS);
path.Md_F2 = repmat(p533.newMode(), 1, c.MAXF2MDS);
end
