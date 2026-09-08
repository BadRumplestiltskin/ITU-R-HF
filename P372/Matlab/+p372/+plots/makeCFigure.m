function fig = makeCFigure(csvFile)
%MAKECFIGURE Data on noise variability and character (P.372-14 Figures 13c-36c).
%   fig = p372.plots.makeCFigure(csvFile)
%
%   Inputs:
%     csvFile - a c_<m>m<h>h.csv file written by p372.runAtmosNoiseMonths.
%   Outputs:
%     fig - handle of an invisible figure.
%
%   Plots Du, Dl, sigma_Fam, sigma_Du and sigma_Dl versus frequency
%   (0.01..20 MHz). sigma_Fam is drawn only up to 10 MHz, the limit of
%   the source curves. V_d and sigma_V_d are present in the data file but
%   not plotted, matching the current published c) figures.
%
%   See also p372.plots.makeP372Figs.
[season, time_lt] = p372.plots.parseFilename(csvFile);
M = csvread(csvFile, 1, 0);
x = M(:, 3);
cols = [7 8 9 10 11];
labels = {'D_u', 'D_l', '\sigma_{F_{am}}', '\sigma_{D_u}', '\sigma_{D_l}'};
fig = figure('Color', 'w', 'Visible', 'off', 'Position', [100 100 700 550]);
ax = axes('Parent', fig);
hold(ax, 'on');
for k = 1:5
    keep = true(size(x));
    if k == 3
        keep = x <= 10;                    % sigma_Fam curves stop at 10 MHz
    end
    plot(ax, x(keep), M(keep, cols(k)), 'LineWidth', 0.8);
end
hold(ax, 'off');
p372.plots.bcAxes(ax, 20);
cmax = ceil(max(M(:, 7))); cmax = cmax - mod(cmax, 2) + 2;
set(ax, 'YLim', [0 cmax], 'YTick', 0:2:cmax, 'YMinorGrid', 'on');
ylabel(ax, '(dB)', 'FontSize', 8);
legend(ax, labels, 'Location', 'northeast', 'FontSize', 6);
title(ax, sprintf('Data on noise variability and character  %s  %s', season, strrep(time_lt, '_', ' ')), 'FontSize', 9);
end
