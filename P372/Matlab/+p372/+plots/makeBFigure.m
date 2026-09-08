function fig = makeBFigure(csvFile)
%MAKEBFIGURE Variation of radio noise with frequency (P.372-14 Figures 13b-36b).
%   fig = p372.plots.makeBFigure(csvFile)
%
%   Inputs:
%     csvFile - a b_<m>m<h>h.csv file written by p372.runAtmosNoiseMonths.
%   Outputs:
%     fig - handle of an invisible figure.
%
%   Plots Fam versus frequency (0.01..30 MHz, log axis) for Fam(1 MHz) of
%   10, 20, ..., 100 dB; the 5 dB curve in the data file is omitted as in
%   MakeP372figs.py. y axis -20..180 dB.
%
%   See also p372.plots.makeP372Figs.
[season, time_lt] = p372.plots.parseFilename(csvFile);
M = csvread(csvFile, 1, 0);
x = M(:, 3);
fig = figure('Color', 'w', 'Visible', 'off', 'Position', [100 100 700 550]);
ax = axes('Parent', fig);
hold(ax, 'on');
labels = cell(1, 10);
for k = 1:10                               % skip Fam5 as the Python does
    plot(ax, x, M(:, 6 + k), 'LineWidth', 0.8);
    labels{k} = sprintf('%2d', k * 10);
end
hold(ax, 'off');
p372.plots.bcAxes(ax, 30);
set(ax, 'YLim', [-20 180], 'YTick', -20:20:180, 'YMinorGrid', 'on');
ylabel(ax, 'F_{am} (dB above kT_0B)', 'FontSize', 8);
lg = legend(ax, labels, 'Location', 'northeast', 'FontSize', 6);
try, title(lg, 'F_{am} 1 MHz (dB)'); catch, end
title(ax, sprintf('Variation of radio noise with frequency  %s  %s', season, strrep(time_lt, '_', ' ')), 'FontSize', 9);
end
