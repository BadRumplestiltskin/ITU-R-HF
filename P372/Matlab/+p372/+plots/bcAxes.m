function bcAxes(ax, xmax)
%BCAXES Apply the common log-frequency axis style of the b) and c) figures.
%   p372.plots.bcAxes(ax, xmax) sets a logarithmic x axis from 0.01 MHz to
%   xmax (30 for b, 20 for c) with the tick positions and labels used in
%   the Recommendation, grid lines, box and axis label.
%
%   See also p372.plots.makeBFigure, p372.plots.makeCFigure.
xt = [0.01 0.02 0.03 0.05 0.07 0.1 0.2 0.3 0.5 0.7 1 2 3 5 7 10 20 30];
xl = {'0.01','0.02','0.03','0.05','0.07','0.1','0.2','0.3','0.5','0.7','1','2','3','5','7','10','20','30'};
keep = xt <= xmax + eps;
set(ax, 'XScale', 'log', 'XTick', xt(keep), 'XTickLabel', xl(keep), 'XLim', [0.01 xmax], ...
        'FontSize', 7, 'XMinorGrid', 'on', 'XMinorTick', 'on');
grid(ax, 'on');
box(ax, 'on');
xlabel(ax, 'Frequency (MHz)', 'FontSize', 8);
end
