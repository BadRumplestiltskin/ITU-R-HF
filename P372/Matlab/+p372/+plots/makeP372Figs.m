function makeP372Figs(root, formats)
%MAKEP372FIGS Render P.372-14 Figures 13-36 a), b), c) from the CSV data.
%   p372.plots.makeP372Figs()
%   p372.plots.makeP372Figs(root)
%   p372.plots.makeP372Figs(root, formats)
%
%   Replacement for MakeP372figs.py. Reads root/{a,b,c}/csv/*.csv, as
%   written by p372.runAtmosNoiseMonths, and writes
%   root/{a,b,c}/<format>/Fig_<x>_<season>_<time>.<format>.
%
%   Inputs:
%     root    - default fullfile(pwd, 'P372_figures').
%     formats - cell array of print formats, default {'svg','png','pdf'};
%               png and pdf are rendered at 300 dpi.
%   Errors:
%     p372:makeP372Figs if an input csv folder is missing.
%
%   Existing output folders are deleted and recreated, as in the Python
%   script. A full run produces 72 figures per format.
%
%   See also p372.runAtmosNoiseMonths, p372.plots.makeAFigure,
%   p372.plots.makeBFigure, p372.plots.makeCFigure.
if nargin < 1 || isempty(root), root = fullfile(pwd, 'P372_figures'); end
if nargin < 2 || isempty(formats), formats = {'svg', 'png', 'pdf'}; end
kinds = 'abc';
makers = {@p372.plots.makeAFigure, @p372.plots.makeBFigure, @p372.plots.makeCFigure};
for k = 1:3
    csvDir = fullfile(root, kinds(k), 'csv');
    if ~exist(csvDir, 'dir')
        error('p372:makeP372Figs', 'Expected input directory does not exist: %s\nRun p372.runAtmosNoiseMonths first.', csvDir);
    end
end
for k = 1:3
    for f = 1:numel(formats)
        d = fullfile(root, kinds(k), formats{f});
        if exist(d, 'dir'), rmdir(d, 's'); end
        mkdir(d);
    end
    files = dir(fullfile(root, kinds(k), 'csv', '*.csv'));
    for n = 1:numel(files)
        csvFile = fullfile(files(n).folder, files(n).name);
        fprintf('\nCreating %s figure for %s\n', kinds(k), csvFile);
        [season, time_lt] = p372.plots.parseFilename(csvFile);
        fig = makers{k}(csvFile);
        base = sprintf('Fig_%s_%s_%s', kinds(k), season, time_lt);
        for f = 1:numel(formats)
            out = fullfile(root, kinds(k), formats{f}, [base '.' formats{f}]);
            fprintf('    Saving Figure %s\n', out);
            switch formats{f}
                case 'svg', print(fig, out, '-dsvg');
                case 'png', print(fig, out, '-dpng', '-r300');
                case 'pdf', print(fig, out, '-dpdf', '-r300');
                otherwise, print(fig, out, ['-d' formats{f}]);
            end
        end
        close(fig);
    end
end
end
