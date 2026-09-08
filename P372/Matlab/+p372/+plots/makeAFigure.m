function fig = makeAFigure(csvFile)
%MAKEAFIGURE World contour map of Fam at 1 MHz (P.372-14 Figures 13a-36a).
%   fig = p372.plots.makeAFigure(csvFile)
%
%   Inputs:
%     csvFile - an a_<m>m<h>h.csv file written by p372.runAtmosNoiseMonths.
%   Outputs:
%     fig - handle of an invisible figure (Visible 'off'); the caller
%           prints or shows it and closes it.
%
%   Contours are drawn at 5 dB steps from 5 to 115 dB on a plate carree
%   grid with 15/10 degree ticks, as in the published figures (which span
%   the full longitude range here rather than -60..60). Coastlines are
%   added from the base-MATLAB 'coastlines' data set when available; in
%   Octave, where that data set does not exist, they are omitted silently.
%
%   See also p372.plots.makeP372Figs, p372.runAtmosNoiseMonths.
[season, time_lt] = p372.plots.parseFilename(csvFile);
M = csvread(csvFile, 1, 0);
lat = M(:, 4); lng = M(:, 5); FaA = M(:, 6);
nlat = 181; nlng = 361;
% file order: latitude outer loop, longitude inner
Z = reshape(FaA, [nlng, nlat]).';          % rows = latitude
X = reshape(lng, [nlng, nlat]).';
Y = reshape(lat, [nlng, nlat]).';
fig = figure('Color', 'w', 'Visible', 'off', 'Position', [100 100 1100 600]);
ax = axes('Parent', fig);
hold(ax, 'on');
[C, hc] = contour(ax, X, Y, Z, 5:5:115, 'LineWidth', 0.75);
clabel(C, hc, 'FontSize', 6, 'LabelSpacing', 400);
try
    S = load('coastlines');                % base MATLAB data set
    plot(ax, S.coastlon, S.coastlat, 'k', 'LineWidth', 0.3);
catch
    % no coastline data (e.g. Octave): skip silently
end
set(ax, 'XLim', [-180 180], 'YLim', [-90 90], 'XTick', -180:15:180, 'YTick', -90:10:90, ...
        'FontSize', 6, 'DataAspectRatio', [1 1 1], 'Layer', 'top');
grid(ax, 'on'); box(ax, 'on');
xlabel(ax, 'Longitude (deg)', 'FontSize', 8); ylabel(ax, 'Latitude (deg)', 'FontSize', 8);
title(ax, sprintf('Expected values of atmospheric noise, F_{am} (dB above kT_0b at 1 MHz)  %s  %s', ...
      season, strrep(time_lt, '_', ' ')), 'FontSize', 9);
colormap(ax, jet(23));
hold(ax, 'off');
end
