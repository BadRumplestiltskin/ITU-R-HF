function cfg = readInputConfiguration(fname)
%READINPUTCONFIGURATION Read an ITURHFProp input (.in) file.
%   cfg = p533.readInputConfiguration(fname)
%   Parses the keyword/value lines of the ITURHFProp configuration format
%   (see ReadInputConfiguration.c): "Keyword value" or "Keyword "text"",
%   comments starting with // or \\. Returns a struct with
%     cfg.path      p533.newPath() with the single-value inputs applied
%                   (year, SSN, txpower, BW, SNRr, SNRXXp, SIRr, A, TW, FW,
%                   T0, F0, Modulation, SorL, manMadeNoise, L_tx, L_rx, names)
%     cfg.months    1..12 (file values, one or a comma-separated list)
%     cfg.hours     0..23 UTC (file gives 1..24)
%     cfg.freqs     MHz
%     cfg.txAnt, cfg.rxAnt   'ISOTROPIC' or an antenna file path
%     cfg.txGOS, cfg.rxGOS   gain offsets dBi (isotropic gain)
%     cfg.txBearing, cfg.rxBearing   radians (MANUAL orientation)
%     cfg.orientation   'TX2RX' (default, antennas point at each other) or 'MANUAL'
%     cfg.area      struct LL, LR, UL, UR (radians), latinc, lnginc (radians)
%     cfg.rptFormat cell array of RPT_* names ({'RPT_ALL'} default)
%     cfg.rptPath   report folder; cfg.dataDir data folder
%   Latitudes and longitudes in the file are decimal degrees.
c = p533.constants();
path = p533.newPath();
cfg = struct('months', 1, 'hours', 0, 'freqs', 10, 'txAnt', 'ISOTROPIC', 'rxAnt', 'ISOTROPIC', ...
             'txGOS', 0, 'rxGOS', 0, 'txBearing', NaN, 'rxBearing', NaN, 'orientation', 'TX2RX', ...
             'rptFormat', {{'RPT_ALL'}}, 'rptPath', '', 'dataDir', '');
area = struct('LL', [NaN NaN], 'LR', [NaN NaN], 'UL', [NaN NaN], 'UR', [NaN NaN], 'latinc', 1, 'lnginc', 1);
D2R = pi / 180;
txt = fileread(fname); txt = strrep(txt, char(13), '');
L = strsplit(txt, char(10));
for k = 1:numel(L)
    line = strtrim(L{k});
    if isempty(line) || strncmp(line, '//', 2) || strncmp(line, '\\', 2), continue; end
    [key, rest] = strtok(line);
    rest = strtrim(regexprep(rest, '//.*$', ''));
    q = regexp(rest, '^"([^"]*)"', 'tokens', 'once');
    if ~isempty(q), str = q{1}; else, str = rest; end
    num = str2double(strsplit(strrep(str, ' ', ''), ','));
    switch key
        case 'PathName', path.name = str;
        case 'PathTXName', path.txname = str;
        case 'PathRXName', path.rxname = str;
        case 'Path.L_tx.lat', path.L_tx.lat = num(1) * D2R;
        case 'Path.L_tx.lng', path.L_tx.lng = num(1) * D2R;
        case 'Path.L_rx.lat', path.L_rx.lat = num(1) * D2R;
        case 'Path.L_rx.lng', path.L_rx.lng = num(1) * D2R;
        case 'TXAntFilePath', cfg.txAnt = str;
        case 'RXAntFilePath', cfg.rxAnt = str;
        case 'TXGOS', cfg.txGOS = num(1);
        case 'RXGOS', cfg.rxGOS = num(1);
        case 'TXBearing', cfg.txBearing = num(1) * D2R;
        case 'RXBearing', cfg.rxBearing = num(1) * D2R;
        case 'AntennaOrientation'
            if strcmpi(str, 'TX2RX'), cfg.orientation = 'TX2RX'; else, cfg.orientation = 'MANUAL'; end
        case 'Path.year', path.year = num(1);
        case 'Path.month', cfg.months = num;
        case 'Path.hour', cfg.hours = num - 1;
        case 'Path.SSN', path.SSN = num(1);
        case 'Path.frequency', cfg.freqs = num;
        case 'Path.txpower', path.txpower = num(1);
        case 'Path.BW', path.BW = num(1);
        case 'Path.SNRr', path.SNRr = num(1);
        case 'Path.SNRXXp', path.SNRXXp = num(1);
        case 'Path.SIRr', path.SIRr = num(1);
        case 'Path.ManMadeNoise'
            names = {'CITY', 'RESIDENTIAL', 'RURAL', 'QUIETRURAL', 'NOISY', 'QUIET'};
            idx = find(strcmpi(str, names), 1);
            if ~isempty(idx), path.manMadeNoise = idx - 1; else, path.manMadeNoise = num(1); end
        case 'Path.Modulation'
            if strcmpi(str, 'DIGITAL'), path.Modulation = c.MOD.DIGITAL; else, path.Modulation = c.MOD.ANALOG; end
        case 'Path.SorL'
            if strcmpi(str, 'LONGPATH'), path.SorL = c.SORL.LONG; else, path.SorL = c.SORL.SHORT; end
        case 'Path.A', path.A = num(1);
        case 'Path.TW', path.TW = num(1);
        case 'Path.FW', path.FW = num(1);
        case 'Path.T0', path.T0 = num(1);
        case 'Path.F0', path.F0 = num(1);
        case 'RptFilePath', cfg.rptPath = str;
        case 'RptFileFormat', cfg.rptFormat = strtrim(strsplit(str, '|'));
        case 'DataFilePath', cfg.dataDir = str;
        case {'LL.lat', 'LR.lat', 'UL.lat', 'UR.lat'}, area.(key(1:2))(1) = num(1) * D2R;
        case {'LL.lng', 'LR.lng', 'UL.lng', 'UR.lng'}, area.(key(1:2))(2) = num(1) * D2R;
        case 'latinc', area.latinc = num(1) * D2R;
        case 'lnginc', area.lnginc = num(1) * D2R;
        otherwise
            % unknown keywords are ignored, as in the C program
    end
end
if any(isnan(area.LL)), area.LL = [path.L_rx.lat, path.L_rx.lng]; end
if any(isnan(area.LR)), area.LR = area.LL; end
if any(isnan(area.UL)), area.UL = area.LL; end
if any(isnan(area.UR)), area.UR = area.LL; end
cfg.path = path;
cfg.area = area;
end
