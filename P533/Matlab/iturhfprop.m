function outFile = iturhfprop(inFile, outFile, varargin)
%ITURHFPROP Run the P.533-14 prediction from an ITURHFProp input file.
%   outFile = iturhfprop(inFile)            report to <RptFilePath>/RPTddmmyy-hhnnss.txt
%   outFile = iturhfprop(inFile, outFile)   report to the given file
%   iturhfprop(inFile, outFile, 'csv', true)     RFC 4180 CSV instead of the text report
%   iturhfprop(inFile, outFile, 'silent', true)  no progress output
%   iturhfprop(inFile, outFile, 'liRule', 'sum')  eq. (20) as printed (see p533.rules)
%
%   MATLAB counterpart of ITURHFProp.exe: reads the configuration
%   (p533.readInputConfiguration), loads the data for each month, points
%   the antennas at each other (TX2RX) or uses the given bearings, then
%   loops over months, hours, frequencies and the receiver area grid
%   (LL..UR, latinc, lnginc) calling p533.run for each point and writing
%   one record per run through p533.report.write.
%   Requires the P533 and P372 folders on the path.
opt = struct('csv', false, 'silent', false, 'liRule', 'hopmean', 'sigmaRule', 'p372-17');
for k = 1:2:numel(varargin), opt.(varargin{k}) = varargin{k + 1}; end
cfg = p533.readInputConfiguration(inFile);
path = cfg.path;
path.liRule = opt.liRule; path.sigmaRule = opt.sigmaRule;
dataDir = cfg.dataDir; if isempty(dataDir), dataDir = p533.defaultDataDir(); end
if nargin < 2 || isempty(outFile)
    rp = cfg.rptPath; if isempty(rp), rp = pwd; end
    outFile = fullfile(rp, ['RPT' datestr(now, 'ddmmyy-HHMMSS') '.txt']);
end
fid = fopen(outFile, 'w');
if fid < 0, error('iturhfprop:open', 'Cannot open %s for writing', outFile); end
cleanup = onCleanup(@() fclose(fid));
a = cfg.area;
nlat = abs(round((a.UL(1) - a.LR(1)) / a.latinc)) + 1;
nlng = abs(round((a.LR(2) - a.LL(2)) / a.lnginc)) + 1;
first = true; count = 0;
for m = cfg.months(:)'
    path.month = m;
    path = p533.loadData(path, dataDir);
    for h = cfg.hours(:)'
        path.hour = h;
        for f = cfg.freqs(:)'
            path.frequency = f;
            for ila = 0:nlat - 1
                for iln = 0:nlng - 1
                    path.L_rx.lat = a.LL(1) + ila * a.latinc;
                    path.L_rx.lng = a.LL(2) + iln * a.lnginc;
                    long = path.SorL == 1;
                    if strcmp(cfg.orientation, 'TX2RX') || isnan(cfg.txBearing)
                        txb = p533.bearing(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng, long);
                        rxb = p533.bearing(path.L_rx.lat, path.L_rx.lng, path.L_tx.lat, path.L_tx.lng, long);
                    else
                        txb = cfg.txBearing; rxb = cfg.rxBearing;
                    end
                    path.A_tx = antenna(cfg.txAnt, cfg.txGOS, txb);
                    path.A_rx = antenna(cfg.rxAnt, cfg.rxGOS, rxb);
                    path = p533.run(path);
                    if strcmp(cfg.orientation, 'MANUAL') && ~isnan(cfg.txBearing)
                        path.txBearing = cfg.txBearing; path.rxBearing = cfg.rxBearing;
                    end
                    if first
                        if opt.csv, p533.report.write(fid, path, cfg, 'csvheader');
                        else, p533.report.write(fid, path, cfg, 'header'); end
                        first = false;
                    end
                    if opt.csv, p533.report.write(fid, path, cfg, 'csvrecord');
                    else, p533.report.write(fid, path, cfg, 'record'); end
                    count = count + 1;
                    if ~opt.silent, fprintf('\r%d', count); end
                end
            end
        end
    end
end
if ~opt.csv, p533.report.write(fid, path, cfg, 'end'); end
if ~opt.silent, fprintf('\n%d runs written to %s\n', count, outFile); end
end

function ant = antenna(spec, gos, bearingRad)
if strcmpi(spec, 'ISOTROPIC'), ant = p533.isotropicPattern(gos);
else, ant = p533.readAntenna(spec, bearingRad); end
end
