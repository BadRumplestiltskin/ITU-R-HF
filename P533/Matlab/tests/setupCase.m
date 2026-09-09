function path = setupCase(cases, i, path)
%SETUPCASE Path structure for row i of cases.csv (reusing loaded data in path).
[~, ~, antDir] = testPaths(); %#ok<ASGLU>
if nargin < 3 || isempty(path), path = p533.newPath(); end
D2R = pi / 180;
path.year = cases.year(i); path.month = cases.month(i); path.hour = cases.hour(i); path.SSN = cases.ssn(i);
path.frequency = cases.freq(i);
path.L_tx = struct('lat', cases.txlat(i) * D2R, 'lng', cases.txlng(i) * D2R);
path.L_rx = struct('lat', cases.rxlat(i) * D2R, 'lng', cases.rxlng(i) * D2R);
path.manMadeNoise = cases.mm(i); path.Modulation = cases.mod(i); path.txpower = cases.txpower(i);
path.BW = cases.bw(i); path.SNRr = cases.snrr(i); path.SNRXXp = cases.snrxxp(i); path.SIRr = cases.sirr(i);
path.A = cases.A(i); path.TW = cases.TW(i); path.FW = cases.FW(i); path.T0 = cases.T0(i); path.F0 = cases.F0(i);
path.SorL = cases.sorl(i);
long = path.SorL == 1;
if strcmp(cases.txant{i}, 'ISOTROPIC')
    path.A_tx = p533.isotropicPattern(cases.txgos(i));
else
    path.A_tx = p533.readAntenna(cases.txant{i}, p533.bearing(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng, long));
end
if strcmp(cases.rxant{i}, 'ISOTROPIC')
    path.A_rx = p533.isotropicPattern(cases.rxgos(i));
else
    path.A_rx = p533.readAntenna(cases.rxant{i}, p533.bearing(path.L_rx.lat, path.L_rx.lng, path.L_tx.lat, path.L_tx.lng, long));
end
path.sigmaRule = 'reference'; path.liRule = 'reference';
path = p533.loadData(path);
end
