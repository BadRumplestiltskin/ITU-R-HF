classdef HFPropApp < handle
%HFPROPAPP Interactive HF circuit prediction per Recommendation ITU-R P.533-14.
%   app = HFPropApp            opens the app (data from p533.defaultDataDir)
%   app = HFPropApp(dataDir)   with another data folder
%
%   Requirements: MATLAB R2019b or newer (uifigure); the P533 and P372
%   folders on the path. Not runnable in GNU Octave.
%
%   Circuit tab: transmitter and receiver (name, latitude, longitude in
%   degrees), antennas (ISOTROPIC with gain, or a VOACAP Type 11/13/14
%   file), year, month, hour UTC, sunspot number, frequency, power,
%   bandwidth, required S/N and reliability, man-made noise category,
%   modulation and the digital window parameters, short or long path.
%   "Run" executes p533.run and fills the results table (every output of
%   the path structure) and the mode table (E and F2 modes: hops, MUF,
%   elevation, losses, field strength, power, delay).
%   Sweeps tab: MUF, OPMUF, FOT/HPF, field strength, receiver power, SNR
%   and BCR versus UTC hour (0-23) and versus frequency (2-30 MHz).
%   Area tab: map of Ep, Pr, SNR or BCR over a receiver area (lat/lng
%   range and step), computed with the area loop of iturhfprop.
%
%   Options: absorption reading (p533.rules: 'hopmean' default, 'sum' as
%   printed, 'reference' C conventions; docs/P533_DEVIATIONS.md D30) and
%   the P.372 sigma rule.
%   The app calls only the p533 package; results equal iturhfprop's.

    properties
        Fig; DataDir
        In       % struct of input controls
        ResTable; ModeTable; StatusLbl
        AxHour; AxFreq; AxMap
        Path     % last path structure
    end

    methods
        function app = HFPropApp(dataDir)
            if nargin < 1 || isempty(dataDir), dataDir = p533.defaultDataDir(); end
            app.DataDir = dataDir;
            app.build();
        end

        function build(app)
            %BUILD Create the window, input panel and result tabs.
            app.Fig = uifigure('Name', sprintf('ITU-R P.533-14 HF prediction  (%s)', p533.version()), 'Position', [80 80 1200 720]);
            g = uigridlayout(app.Fig, [1 2]); g.ColumnWidth = {330, '1x'};
            pnl = uipanel(g, 'Title', 'Circuit'); pg = uigridlayout(pnl, [26 2]); pg.ColumnWidth = {130, '1x'};
            pg.RowHeight = repmat({24}, 1, 26);
            f = @(lbl, ctrl) addRow(app, pg, lbl, ctrl);
            I = struct();
            I.txname = f('Tx name', @() uieditfield(pg, 'text', 'Value', 'TX'));
            I.txlat = f('Tx latitude (deg)', @() uieditfield(pg, 'numeric', 'Limits', [-90 90], 'Value', 49.6667));
            I.txlng = f('Tx longitude (deg)', @() uieditfield(pg, 'numeric', 'Limits', [-180 180], 'Value', 6.3167));
            I.txant = f('Tx antenna', @() uieditfield(pg, 'text', 'Value', 'ISOTROPIC'));
            I.txgos = f('Tx gain / offset (dBi)', @() uieditfield(pg, 'numeric', 'Value', 0));
            I.rxname = f('Rx name', @() uieditfield(pg, 'text', 'Value', 'RX'));
            I.rxlat = f('Rx latitude (deg)', @() uieditfield(pg, 'numeric', 'Limits', [-90 90], 'Value', 51.1167));
            I.rxlng = f('Rx longitude (deg)', @() uieditfield(pg, 'numeric', 'Limits', [-180 180], 'Value', 7.2667));
            I.rxant = f('Rx antenna', @() uieditfield(pg, 'text', 'Value', 'ISOTROPIC'));
            I.rxgos = f('Rx gain / offset (dBi)', @() uieditfield(pg, 'numeric', 'Value', 0));
            I.year = f('Year', @() uieditfield(pg, 'numeric', 'Limits', [1900 2100], 'Value', 2024, 'RoundFractionalValues', 'on'));
            I.month = f('Month', @() uidropdown(pg, 'Items', {'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'}, 'ItemsData', 1:12, 'Value', 8));
            I.hour = f('Hour (UTC 0-23)', @() uispinner(pg, 'Limits', [0 23], 'Value', 12));
            I.ssn = f('Sunspot number R12', @() uieditfield(pg, 'numeric', 'Limits', [0 311], 'Value', 50));
            I.freq = f('Frequency (MHz)', @() uieditfield(pg, 'numeric', 'Limits', [1 30], 'Value', 7));
            I.power = f('Tx power dB(1 kW)', @() uieditfield(pg, 'numeric', 'Limits', [-30 60], 'Value', 0));
            I.bw = f('Bandwidth (Hz)', @() uieditfield(pg, 'numeric', 'Limits', [0.005 3e6], 'Value', 3000));
            I.snrr = f('Required S/N (dB)', @() uieditfield(pg, 'numeric', 'Value', 10));
            I.snrxxp = f('Required reliability (%)', @() uispinner(pg, 'Limits', [1 99], 'Value', 90));
            I.mm = f('Man-made noise', @() uidropdown(pg, 'Items', {'City','Residential','Rural','Quiet rural','Noisy','Quiet'}, 'ItemsData', 0:5, 'Value', 2));
            I.mod = f('Modulation', @() uidropdown(pg, 'Items', {'Analog','Digital'}, 'ItemsData', [0 1], 'Value', 0));
            I.digital = f('A / TW / FW / T0 / F0', @() uieditfield(pg, 'text', 'Value', '3 1 10 0.5 1'));
            I.sorl = f('Path', @() uidropdown(pg, 'Items', {'Short','Long'}, 'ItemsData', [0 1], 'Value', 0));
            I.lirule = f('Absorption reading', @() uidropdown(pg, 'Items', {'n x mean (D1 validated)', 'eq. (20) sum as printed', 'ITU C code conventions'}, 'ItemsData', {'hopmean', 'sum', 'reference'}, 'Value', 'hopmean'));
            I.sigma = f('P.372 sigma rule', @() uidropdown(pg, 'Items', {'P.372-17 text', 'ITU C code'}, 'ItemsData', {'p372-17', 'reference'}, 'Value', 'p372-17'));
            b = uibutton(pg, 'Text', 'Run', 'ButtonPushedFcn', @(~, ~) app.runOnce()); b.Layout.Column = [1 2];
            app.In = I;
            tg = uitabgroup(g);
            t1 = uitab(tg, 'Title', 'Results'); tg1 = uigridlayout(t1, [3 1]); tg1.RowHeight = {'2x', '1x', 22};
            app.ResTable = uitable(tg1, 'ColumnName', {'Output', 'Value', 'Unit'});
            app.ModeTable = uitable(tg1, 'ColumnName', {'Mode', 'Hops', 'BMUF', 'OPMUF', 'Fprob', 'fs', 'hr', 'Elev deg', 'Lb', 'Ew', 'Prw', 'tau ms'});
            app.StatusLbl = uilabel(tg1, 'Text', 'Ready', 'FontColor', [0.3 0.3 0.3]);
            t2 = uitab(tg, 'Title', 'Sweeps'); tg2 = uigridlayout(t2, [2 1]);
            app.AxHour = uiaxes(tg2); app.AxFreq = uiaxes(tg2);
            t3 = uitab(tg, 'Title', 'Area'); tg3 = uigridlayout(t3, [2 1]); tg3.RowHeight = {30, '1x'};
            bar = uigridlayout(tg3, [1 6]);
            uilabel(bar, 'Text', 'Quantity'); app.In.areaq = uidropdown(bar, 'Items', {'Ep', 'Pr', 'SNR', 'BCR', 'BMUF'});
            uilabel(bar, 'Text', 'Half-size (deg)'); app.In.areah = uieditfield(bar, 'numeric', 'Value', 20);
            uilabel(bar, 'Text', 'Step (deg)'); app.In.areas = uieditfield(bar, 'numeric', 'Value', 2);
            uibutton(bar, 'Text', 'Map', 'ButtonPushedFcn', @(~, ~) app.runArea());
            app.AxMap = uiaxes(tg3);
        end

        function h = addRow(~, pg, label, maker)
            uilabel(pg, 'Text', label); h = maker();
        end

        function path = pathFromInputs(app)
            %PATHFROMINPUTS Build a path structure from the controls.
            I = app.In; c = p533.constants(); D2R = pi / 180;
            path = p533.newPath();
            path.txname = I.txname.Value; path.rxname = I.rxname.Value; path.name = sprintf('%s to %s', path.txname, path.rxname);
            path.L_tx = struct('lat', I.txlat.Value * D2R, 'lng', I.txlng.Value * D2R);
            path.L_rx = struct('lat', I.rxlat.Value * D2R, 'lng', I.rxlng.Value * D2R);
            path.year = I.year.Value; path.month = I.month.Value; path.hour = I.hour.Value; path.SSN = I.ssn.Value;
            path.frequency = I.freq.Value; path.txpower = I.power.Value; path.BW = I.bw.Value;
            path.SNRr = I.snrr.Value; path.SNRXXp = I.snrxxp.Value; path.manMadeNoise = I.mm.Value;
            path.Modulation = I.mod.Value; path.SorL = I.sorl.Value;
            d = sscanf(I.digital.Value, '%f'); d(end + 1:5) = 0;
            if path.Modulation == c.MOD.DIGITAL, path.A = d(1); path.TW = d(2); path.FW = d(3); path.T0 = d(4); path.F0 = d(5); end
            path.liRule = I.lirule.Value; path.sigmaRule = I.sigma.Value;
            long = path.SorL == c.SORL.LONG;
            txb = p533.bearing(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng, long);
            rxb = p533.bearing(path.L_rx.lat, path.L_rx.lng, path.L_tx.lat, path.L_tx.lng, long);
            if strcmpi(I.txant.Value, 'ISOTROPIC'), path.A_tx = p533.isotropicPattern(I.txgos.Value); else, path.A_tx = p533.readAntenna(I.txant.Value, txb); end
            if strcmpi(I.rxant.Value, 'ISOTROPIC'), path.A_rx = p533.isotropicPattern(I.rxgos.Value); else, path.A_rx = p533.readAntenna(I.rxant.Value, rxb); end
            path = p533.loadData(path, app.DataDir);
        end

        function runOnce(app)
            %RUNONCE Run the prediction for the current inputs and refresh all tabs.
            try
                app.StatusLbl.Text = 'Running...'; drawnow;
                path = app.pathFromInputs();
                path = p533.run(path);
                app.Path = path;
                app.fillTables(path);
                app.sweeps(path);
                app.StatusLbl.Text = sprintf('Done: D = %.0f km, BMUF %.1f MHz, Ep %.1f dB(uV/m), SNR %.1f dB, BCR %.0f %%', ...
                    path.distance, path.BMUF, path.Ep, path.SNR, path.BCR);
            catch err
                app.StatusLbl.Text = ['Error: ' err.message];
            end
        end

        function fillTables(app, p)
            rows = {'Distance', p.distance, 'km'; 'dmax', p.dmax, 'km'; 'Season', p.season, '1 W 2 E 3 S';
                    'Basic MUF', p.BMUF, 'MHz'; 'MUF90 (FOT)', p.MUF90, 'MHz'; 'MUF10 (HPF)', p.MUF10, 'MHz';
                    'Operational MUF', p.OPMUF, 'MHz'; 'OPMUF90', p.OPMUF90, 'MHz'; 'OPMUF10', p.OPMUF10, 'MHz';
                    'Lowest F2 mode hops', p.n0_F2, ''; 'Lowest E mode hops', p.n0_E, '';
                    'Field strength Ep', p.Ep, 'dB(1uV/m)'; 'Es (<= 9000 km)', p.Es, 'dB(1uV/m)'; 'El (>= 7000 km)', p.El, 'dB(1uV/m)';
                    'Receiver power Pr', p.Pr, 'dBW'; 'Rx gain Grw', p.Grw, 'dBi'; 'Elevation', p.ele * 180 / pi, 'deg';
                    'Noise FamT', p.noise.FamT, 'dB(kT0b)'; 'FaA / FaM / FaG', sprintf('%.1f / %.1f / %.1f', p.noise.FaA, p.noise.FaM, p.noise.FaG), 'dB';
                    'SNR', p.SNR, 'dB'; 'DuSN / DlSN', sprintf('%.1f / %.1f', p.DuSN, p.DlSN), 'dB'; sprintf('SNR at %d %%', p.SNRXXp), p.SNRXX, 'dB';
                    'BCR', p.BCR, '%'; 'MIR', p.MIR, '%'; 'OCR', p.OCR, '%'; 'OCRs', p.OCRs, '%'; 'probocc', p.probocc, '';
                    'fM / fL (long)', sprintf('%.1f / %.1f', p.fM, p.fL), 'MHz'; 'Gap / Gtl', sprintf('%.1f / %.1f', p.Gap, p.Gtl), 'dB'};
            app.ResTable.Data = rows;
            modes = [p.Md_E, p.Md_F2]; names = [arrayfun(@(k) sprintf('E'), 1:3, 'UniformOutput', false), arrayfun(@(k) 'F2', 1:6, 'UniformOutput', false)];
            M = {};
            for k = 1:numel(modes)
                m = modes(k);
                if m.BMUF <= 0, continue; end
                M(end + 1, :) = {names{k}, m.hops, m.BMUF, m.OPMUF, m.Fprob, m.fs, m.hr, m.ele * 180 / pi, m.Lb, m.Ew, m.Prw, m.tau}; %#ok<AGROW>
            end
            app.ModeTable.Data = M;
        end

        function sweeps(app, base)
            %SWEEPS Hour and frequency sweeps for the current circuit.
            hrs = 0:23; f = base.frequency;
            [muf, opmuf, ep, snr, bcr] = deal(nan(size(hrs)));
            for k = 1:numel(hrs)
                p = base; p.hour = hrs(k); p = p533.run(p);
                muf(k) = p.BMUF; opmuf(k) = p.OPMUF; ep(k) = p.Ep; snr(k) = p.SNR; bcr(k) = p.BCR;
            end
            ax = app.AxHour; cla(ax); yyaxis(ax, 'left');
            plot(ax, hrs, muf, '-o', hrs, opmuf, '-s', hrs, snr, '-^', 'MarkerSize', 4); ylabel(ax, 'MHz / dB');
            yyaxis(ax, 'right'); plot(ax, hrs, bcr, '--'); ylabel(ax, 'BCR %'); ylim(ax, [0 100]);
            xlabel(ax, 'Hour (UTC)'); grid(ax, 'on'); legend(ax, {'Basic MUF', 'OPMUF', 'SNR', 'BCR'}, 'Location', 'best');
            title(ax, sprintf('%g MHz, month %d', f, base.month));
            fr = 2:1:30; [ep2, snr2, bcr2] = deal(nan(size(fr)));
            for k = 1:numel(fr)
                p = base; p.frequency = fr(k); p = p533.run(p);
                ep2(k) = p.Ep; snr2(k) = p.SNR; bcr2(k) = p.BCR;
            end
            ax = app.AxFreq; cla(ax); yyaxis(ax, 'left');
            plot(ax, fr, ep2, '-o', fr, snr2, '-^', 'MarkerSize', 4); ylabel(ax, 'dB');
            yyaxis(ax, 'right'); plot(ax, fr, bcr2, '--'); ylabel(ax, 'BCR %'); ylim(ax, [0 100]);
            xlabel(ax, 'Frequency (MHz)'); grid(ax, 'on'); legend(ax, {'Ep', 'SNR', 'BCR'}, 'Location', 'best');
            title(ax, sprintf('%02d UTC, basic MUF %.1f MHz', base.hour, base.BMUF));
        end

        function runArea(app)
            %RUNAREA Map a quantity over a receiver area around the current receiver.
            try
                base = app.pathFromInputs(); D2R = pi / 180;
                h = app.In.areah.Value; s = app.In.areas.Value; q = app.In.areaq.Value;
                lats = (app.In.rxlat.Value - h):s:(app.In.rxlat.Value + h);
                lngs = (app.In.rxlng.Value - h):s:(app.In.rxlng.Value + h);
                Z = nan(numel(lats), numel(lngs));
                for i = 1:numel(lats)
                    for j = 1:numel(lngs)
                        p = base; p.L_rx = struct('lat', max(min(lats(i), 90), -90) * D2R, 'lng', mod(lngs(j) + 180, 360) * D2R - pi);
                        p.txBearing = p533.bearing(p.L_tx.lat, p.L_tx.lng, p.L_rx.lat, p.L_rx.lng, p.SorL == 1);
                        p = p533.run(p);
                        Z(i, j) = p.(q);
                    end
                end
                ax = app.AxMap; cla(ax);
                imagesc(ax, lngs, lats, Z); set(ax, 'YDir', 'normal'); colorbar(ax);
                hold(ax, 'on'); plot(ax, app.In.txlng.Value, app.In.txlat.Value, 'kp', 'MarkerFaceColor', 'w', 'MarkerSize', 12); hold(ax, 'off');
                xlabel(ax, 'Longitude (deg)'); ylabel(ax, 'Latitude (deg)'); title(ax, sprintf('%s, %g MHz, %02d UTC', q, base.frequency, base.hour));
                app.StatusLbl.Text = sprintf('Area map of %s done (%d points)', q, numel(Z));
            catch err
                app.StatusLbl.Text = ['Error: ' err.message];
            end
        end
    end
end
