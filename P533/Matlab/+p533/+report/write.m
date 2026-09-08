function write(fid, path, cfg, what, when)
%WRITE Write the ITURHFProp-style report (header, data format, records).
%   p533.report.write(fid, path, cfg, 'header', timestr)  ITU header, P533
%       and ITURHFP input parameter blocks and the Data Format column list
%   p533.report.write(fid, path, cfg, 'record')  one calculated record line
%   p533.report.write(fid, path, cfg, 'end')     closing lines
%   p533.report.write(fid, path, cfg, 'csvheader') / 'csvrecord'   RFC 4180 CSV
%   cfg is the struct of p533.readInputConfiguration (rptFormat selects the
%   RPT_* columns; 'RPT_ALL' selects everything). Layout follows Report.c
%   so that reports can be compared column by column with the C program;
%   the ITU header carries the port version.
C = p533.report.columns(path);
sel = ismember({C.flag}, cfg.rptFormat) | any(strcmp(cfg.rptFormat, 'RPT_ALL'));
C = C(sel);
R2D = 180 / pi;
switch lower(what)
    case 'header'
        if nargin < 5, when = datestr(now); end
        fprintf(fid, '---------------------------------------------------------------------------\n');
        fprintf(fid, ' International Telecommunications Union - Radiocommunication Sector (ITU-R)\n');
        fprintf(fid, '     ITURHFProp         Ver %s (MATLAB port)\n', p533.version());
        fprintf(fid, '     HF Model (P533)    Ver %s\n', p533.version());
        fprintf(fid, '     Noise Model (P372) Ver %s\n', p372.version());
        fprintf(fid, '     Analysis Prepared  %s\n', when);
        fprintf(fid, '---------------------------------------------------------------------------\n\n');
        fprintf(fid, '***************************** P533 Input Parameters ****************************\n\n');
        months = {'January','February','March','April','May','June','July','August','September','October','November','December'};
        fprintf(fid, '\t%s\n', path.name);
        fprintf(fid, '\tYear          : %d\n', path.year);
        fprintf(fid, '\tMonth         : %s\n', months{path.month});
        fprintf(fid, '\tHour          : %d (hour UTC)\n', path.hour + 1);
        fprintf(fid, '\tSSN (R12)     : %d\n', path.SSN);
        fprintf(fid, '\tDistance      : %f (km)\n', path.distance);
        fprintf(fid, '\tdmax          : %f (km)\n', path.dmax);
        fprintf(fid, '\tTx power      : %f\n', path.txpower);
        fprintf(fid, '\tTx Location     %s\n', path.txname);
        fprintf(fid, '\tTx latitude   : %10.6f %s\n', abs(path.L_tx.lat * R2D), ns(path.L_tx.lat));
        fprintf(fid, '\tTx longitude  : %10.6f %s\n', abs(path.L_tx.lng * R2D), ew(path.L_tx.lng));
        fprintf(fid, '\tRx Location     %s\n', path.rxname);
        fprintf(fid, '\tRx latitude   : %10.6f %s\n', abs(path.L_rx.lat * R2D), ns(path.L_rx.lat));
        fprintf(fid, '\tRx longitude  : %10.6f %s\n', abs(path.L_rx.lng * R2D), ew(path.L_rx.lng));
        fprintf(fid, '\tFrequency     : %f\n', path.frequency);
        fprintf(fid, '\tBandwidth     : %f\n', path.BW);
        if path.Modulation == 1, mods = 'DIGITAL'; else, mods = 'ANALOG'; end
        fprintf(fid, '\tModulation : %s\n', mods);
        fprintf(fid, '\tRequired signal-to-noise ratio : %f\n', path.SNRr);
        fprintf(fid, '\tRequired %% of month signal-to-noise ratio : % d\n', path.SNRXXp);
        fprintf(fid, '\tRequired signal-to-interference ratio : %f\n', path.SIRr);
        names = {'CITY', 'RESIDENTIAL', 'RURAL', 'QUIETRURAL', 'NOISY', 'QUIET'};
        if path.manMadeNoise >= 0 && path.manMadeNoise <= 5 && path.manMadeNoise == fix(path.manMadeNoise)
            fprintf(fid, '\tMan-made noise : %s\n', names{path.manMadeNoise + 1});
        else
            fprintf(fid, '\tMan-made noise : %f (dB)\n', path.manMadeNoise);
        end
        if path.Modulation == 1
            fprintf(fid, '\tFrequency dispersion for simple BCR (F0) : %f\n', path.F0);
            fprintf(fid, '\tTime spread for simple BCR (T0) : %f\n', path.T0);
            fprintf(fid, '\tRequired Amplitude ratio (A) : %f\n', path.A);
            fprintf(fid, '\tTime window (usec) : %f\n', path.TW);
            fprintf(fid, '\tFrequency window (Hz) : %f\n', path.FW);
        end
        if strcmp(cfg.orientation, 'TX2RX'), o = 'Transmitter main beam to receiver main beam'; else, o = 'User determined'; end
        fprintf(fid, '\tAntenna configuration : %s\n', o);
        if path.SorL == 1, sl = 'LONGPATH'; else, sl = 'SHORTPATH'; end
        fprintf(fid, '\tPath Direction : %s\n', sl);
        fprintf(fid, '\tTransmit antenna               %.40s\n', path.A_tx.name);
        fprintf(fid, '\tTransmit antenna bearing     : %f\n', path.txBearing * R2D);
        fprintf(fid, '\tTransmit antenna gain offset : %f\n', cfg.txGOS);
        fprintf(fid, '\tReceive antenna                %.40s\n', path.A_rx.name);
        fprintf(fid, '\tReceive antenna bearing      : %f\n', path.rxBearing * R2D);
        fprintf(fid, '\tReceive antenna gain offset  : %f\n\n', cfg.rxGOS);
        fprintf(fid, '************************ End P533 Input Parameters *****************************\n\n');
        fprintf(fid, '************************** ITURHFP Input Parameters *****************************\n\n');
        a = cfg.area;
        fprintf(fid, '\tUpper left (North West) latitude   : %10.6f %s\n', abs(a.UL(1) * R2D), ns(a.UL(1)));
        fprintf(fid, '\tUpper left (North West) longitude  : %10.6f %s\n', abs(a.UL(2) * R2D), ew(a.UL(2)));
        fprintf(fid, '\tLower right (South East) latitude  : %10.6f %s\n', abs(a.LR(1) * R2D), ns(a.LR(1)));
        fprintf(fid, '\tLower right (South East) longitude : %10.6f %s\n', abs(a.LR(2) * R2D), ew(a.LR(2)));
        fprintf(fid, '\tNumber of frequencies : %d\n', numel(cfg.freqs));
        fprintf(fid, '\tNumber of hours       : %d\n', numel(cfg.hours));
        fprintf(fid, '\tNumber of months      : %d\n', numel(cfg.months));
        fprintf(fid, '\tLatitude increment    : %f (deg)\n', a.latinc * R2D);
        fprintf(fid, '\tLongitude increment   : %f (deg)\n\n', a.lnginc * R2D);
        fprintf(fid, '************************** ITURHFP Input Parameters *****************************\n\n');
        fprintf(fid, '******************************** Data Format ***********************************\n\n');
        fprintf(fid, 'Column 01: Month\nColumn 02: Hour\nColumn 03: Frequency (MHz)\n');
        for k = 1:numel(C), fprintf(fid, 'Column %02d: %s\n', k + 3, C(k).desc); end
        fprintf(fid, '\n************************** End Data Format ********************************\n\n');
        fprintf(fid, '************************ Calculated Parameters ****************************\n\n');
    case 'record'
        fprintf(fid, '%02d, %02d,% 9.3f', path.month, path.hour + 1, path.frequency);
        for k = 1:numel(C)
            v = C(k).value(path);
            if ischar(v), fprintf(fid, [',' C(k).fmt], v); else, fprintf(fid, [',' C(k).fmt], v); end
        end
        fprintf(fid, '\n');
    case 'end'
        fprintf(fid, '\n**************************End Calculated Parameters ***********************\n');
    case 'csvheader'
        fprintf(fid, 'month,hour,frequency');
        for k = 1:numel(C), fprintf(fid, ',%s', C(k).name); end
        fprintf(fid, '\n');
    case 'csvrecord'
        fprintf(fid, '%d,%d,%.2f', path.month, path.hour + 1, path.frequency);
        for k = 1:numel(C)
            v = C(k).value(path);
            fprintf(fid, [',' C(k).fmtcsv], v);
        end
        fprintf(fid, '\n');
end
end
function s = ns(x), if x < 0, s = 'S'; else, s = 'N'; end, end
function s = ew(x), if x < 0, s = 'W'; else, s = 'E'; end, end
