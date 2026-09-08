classdef P372NoiseApp < handle
%P372NOISEAPP Interactive calculator for Recommendation ITU-R P.372-14 radio noise.
%   app = P372NoiseApp            opens the app with the packaged data folder
%   app = P372NoiseApp(dataDir)   uses another folder of COEFFmmW.txt files
%
%   Requirements: MATLAB R2019b or newer (uifigure, uigridlayout, uitable).
%   The P372 folder (parent of +p372) must be on the MATLAB path. Not
%   runnable in GNU Octave.
%
%   Inputs panel:
%     Month, Hour (UTC 0-23), Latitude and Longitude (degrees, north and
%     east positive), Frequency (0.01-30 MHz), Man-made noise category
%     (City, Residential, Rural, Quiet rural, Noisy, Quiet) or an override
%     value in dB that bypasses the model, the sigma_T rule for deciles
%     above 12 dB (ITU C code behaviour or P.372-17 wording, see
%     p372.noise), and the data folder.
%   Results tab:
%     Table of the twelve outputs of p372.noise (medians and decile
%     deviations of the atmospheric, man-made, galactic and total noise,
%     dB) and the text report of p372.formatReport, identical to the
%     ITURNoise.exe output.
%   Plots tab:
%     Noise components versus frequency at the chosen point and hour, and
%     atmospheric / total noise versus UTC hour at the chosen frequency.
%
%   The app contains no physics; every value comes from the p372 package,
%   so results are identical to p372.makeNoise. Errors from the engine
%   (for example a missing data file) are shown in the status line.
%
%   To convert to an App Designer .mlapp file, open this file in MATLAB
%   and use "Open as App"; the class is structured with a build() method
%   and callbacks in the same way as App Designer generated code.
%
%   Example:
%     addpath('/path/to/ITU-R-HF/P372/Matlab');
%     app = P372NoiseApp;
%     app.LatEF.Value = 51.5; app.LngEF.Value = -0.1; app.calculate();
%
%   See also p372.makeNoise, p372.noise, p372.formatReport.

    properties
        Fig          % uifigure handle
        DataDir      % initial coefficient folder
        % Input controls
        MonthDD      % uidropdown, month 1..12
        HourSp       % uispinner, UTC hour 0..23
        LatEF        % numeric edit field, latitude deg
        LngEF        % numeric edit field, longitude deg
        FreqEF       % numeric edit field, MHz
        MMDD         % uidropdown, man-made category or override (-1)
        MMValEF      % numeric edit field, override value dB
        RuleDD       % uidropdown, sigma_T rule ('reference' / 'p372-17')
        DataEF       % text edit field, data folder
        % Output controls
        ResultTable  % uitable of the 12 results
        ReportTA     % uitextarea with the formatted report
        StatusLbl    % error / status line
        AxFreq       % uiaxes: noise versus frequency
        AxHour       % uiaxes: noise versus UTC hour
    end

    methods
        function app = P372NoiseApp(dataDir)
            if nargin < 1 || isempty(dataDir)
                dataDir = p372.defaultDataDir();
            end
            app.DataDir = dataDir;
            app.build();
            app.calculate();
        end

        function build(app)
            %BUILD Create the figure, input panel and output tabs.
            app.Fig = uifigure('Name', sprintf('ITU-R P.372 Noise  (P372 %s)', p372.version()), ...
                               'Position', [100 100 1000 640]);
            g = uigridlayout(app.Fig, [1 2]);
            g.ColumnWidth = {300, '1x'};

            % ---------- input panel ----------
            pnl = uipanel(g, 'Title', 'Inputs');
            pg = uigridlayout(pnl, [11 2]);
            pg.RowHeight = repmat({28}, 1, 11);
            pg.ColumnWidth = {120, '1x'};

            uilabel(pg, 'Text', 'Month');
            app.MonthDD = uidropdown(pg, 'Items', {'January','February','March','April','May','June', ...
                'July','August','September','October','November','December'}, 'ItemsData', 1:12, 'Value', 1);
            uilabel(pg, 'Text', 'Hour (UTC, 0-23)');
            app.HourSp = uispinner(pg, 'Limits', [0 23], 'Value', 13, 'Step', 1);
            uilabel(pg, 'Text', 'Latitude (deg)');
            app.LatEF = uieditfield(pg, 'numeric', 'Limits', [-90 90], 'Value', 40);
            uilabel(pg, 'Text', 'Longitude (deg)');
            app.LngEF = uieditfield(pg, 'numeric', 'Limits', [-180 180], 'Value', 165);
            uilabel(pg, 'Text', 'Frequency (MHz)');
            app.FreqEF = uieditfield(pg, 'numeric', 'Limits', [0.01 30], 'Value', 1.0);
            uilabel(pg, 'Text', 'Man-made noise');
            app.MMDD = uidropdown(pg, 'Items', {'City (0)', 'Residential (1)', 'Rural (2)', ...
                'Quiet Rural (3)', 'Noisy (4)', 'Quiet (5)', 'Override (dB value)'}, ...
                'ItemsData', [0 1 2 3 4 5 -1], 'Value', 0, ...
                'ValueChangedFcn', @(~, ~) app.onMMChanged());
            uilabel(pg, 'Text', 'Override Fa (dB)');
            app.MMValEF = uieditfield(pg, 'numeric', 'Value', 50, 'Enable', 'off');
            uilabel(pg, 'Text', 'sigma_T rule (>12 dB)');
            app.RuleDD = uidropdown(pg, 'Items', {'P.372-17 text (min of eq. 19, 25)', 'ITU C code (eq. 25 replaces 19)'}, ...
                'ItemsData', {'p372-17', 'reference'}, 'Value', 'p372-17');
            uilabel(pg, 'Text', 'Data folder');
            app.DataEF = uieditfield(pg, 'text', 'Value', app.DataDir);
            b1 = uibutton(pg, 'Text', 'Browse...', 'ButtonPushedFcn', @(~, ~) app.browse());
            b1.Layout.Column = 1;
            b2 = uibutton(pg, 'Text', 'Calculate', 'ButtonPushedFcn', @(~, ~) app.calculate());
            b2.Layout.Column = 2;
            app.StatusLbl = uilabel(pg, 'Text', '', 'FontColor', [0.6 0 0]);
            app.StatusLbl.Layout.Column = [1 2];

            % ---------- output tabs ----------
            tg = uitabgroup(g);
            t1 = uitab(tg, 'Title', 'Results');
            tgd = uigridlayout(t1, [2 1]);
            tgd.RowHeight = {'1x', '1x'};
            app.ResultTable = uitable(tgd, 'ColumnName', {'Symbol', 'Quantity', 'Value (dB)'}, ...
                'ColumnWidth', {60, 260, 100});
            app.ReportTA = uitextarea(tgd, 'Editable', 'off', 'FontName', 'Courier New');

            t2 = uitab(tg, 'Title', 'Plots');
            pgd = uigridlayout(t2, [2 1]);
            app.AxFreq = uiaxes(pgd);
            app.AxHour = uiaxes(pgd);
        end

        function onMMChanged(app)
            %ONMMCHANGED Enable the override field only for the override choice.
            if app.MMDD.Value < 0
                app.MMValEF.Enable = 'on';
            else
                app.MMValEF.Enable = 'off';
            end
        end

        function browse(app)
            %BROWSE Folder picker for the coefficient data.
            d = uigetdir(app.DataEF.Value, 'Select folder with COEFFxxW.txt');
            if ischar(d)
                app.DataEF.Value = d;
            end
        end

        function mm = manMadeValue(app)
            %MANMADEVALUE Category code, or negative override value for p372.noise.
            mm = app.MMDD.Value;
            if mm < 0
                mm = -abs(app.MMValEF.Value);
            end
        end

        function calculate(app)
            %CALCULATE Run the engine for the current inputs and refresh all outputs.
            app.StatusLbl.Text = '';
            try
                month = app.MonthDD.Value; hour = app.HourSp.Value;
                lat = app.LatEF.Value; lng = app.LngEF.Value; freq = app.FreqEF.Value;
                mm = app.manMadeValue();
                dataDir = app.DataEF.Value;
                coeff = p372.readFamDud(dataDir, month);
                D2R = p372.D2R();
                rule = app.RuleDD.Value;
                n = p372.noise(coeff, mm, hour, lng * D2R, lat * D2R, freq, rule);
                names = {'FaA', 'Noise component (atmospheric)'; 'DuA', 'Upper decile (atmospheric)'; ...
                         'DlA', 'Lower decile (atmospheric)'; 'FaM', 'Noise component (man-made)'; ...
                         'DuM', 'Upper decile (man-made)'; 'DlM', 'Lower decile (man-made)'; ...
                         'FaG', 'Noise component (galactic)'; 'DuG', 'Upper decile (galactic)'; ...
                         'DlG', 'Lower decile (galactic)'; 'FamT', 'Noise (total)'; ...
                         'DuT', 'Upper decile (total)'; 'DlT', 'Lower decile (total)'};
                vals = cellfun(@(f) n.(f), names(:, 1));
                app.ResultTable.Data = [names, num2cell(round(vals * 1000) / 1000)];
                app.ReportTA.Value = strsplit(p372.formatReport(n, month, hour, lng, lat, freq), newline);
                app.updatePlots(coeff, mm, hour, lat, lng, freq, rule);
            catch err
                app.StatusLbl.Text = err.message;
            end
        end

        function updatePlots(app, coeff, mm, hour, lat, lng, freq, rule)
            %UPDATEPLOTS Redraw the frequency and hour sweeps for the current point.
            D2R = p372.D2R();
            % Fam vs frequency at this point and UTC hour
            f = logspace(-2, log10(30), 80);
            FaA = zeros(size(f)); FaM = FaA; FaG = FaA; FamT = FaA;
            for k = 1:numel(f)
                n = p372.noise(coeff, mm, hour, lng * D2R, lat * D2R, f(k), rule);
                FaA(k) = n.FaA; FaM(k) = n.FaM; FaG(k) = n.FaG; FamT(k) = n.FamT;
            end
            ax = app.AxFreq;
            semilogx(ax, f, FaA, f, FaM, f, FaG, f, FamT, 'LineWidth', 1.2);
            grid(ax, 'on'); xlim(ax, [0.01 30]);
            xlabel(ax, 'Frequency (MHz)'); ylabel(ax, 'F_a (dB above kT_0b)');
            legend(ax, {'Atmospheric', 'Man-made', 'Galactic', 'Total'}, 'Location', 'northeast');
            title(ax, sprintf('Noise vs frequency, %d UTC, %.2f\\circ, %.2f\\circ', hour, lat, lng));
            % Noise vs UTC hour at this frequency
            h = 0:23; A = zeros(size(h)); T = A;
            for k = 1:numel(h)
                n = p372.noise(coeff, mm, h(k), lng * D2R, lat * D2R, freq, rule);
                A(k) = n.FaA; T(k) = n.FamT;
            end
            ax = app.AxHour;
            plot(ax, h, A, '-o', h, T, '-s', 'LineWidth', 1.2, 'MarkerSize', 4);
            grid(ax, 'on'); xlim(ax, [0 23]);
            xlabel(ax, 'Hour (UTC)'); ylabel(ax, 'F_a (dB above kT_0b)');
            legend(ax, {'Atmospheric', 'Total'}, 'Location', 'best');
            title(ax, sprintf('Noise vs hour at %.3g MHz', freq));
        end
    end
end
