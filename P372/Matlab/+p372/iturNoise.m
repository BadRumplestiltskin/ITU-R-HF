function [retval, out] = iturNoise(varargin)
%ITURNOISE Command-line style front end equivalent to ITURNoise.exe.
%   p372.iturNoise(dataDir)
%       Mode 2: generate the P.372-14 figure data files in ./P372_figures
%       (see p372.runAtmosNoiseMonths).
%   [retval, out] = p372.iturNoise(month, hour, freq, lat, lng, mmnoise, dataDir)
%   [retval, out] = p372.iturNoise(month, hour, freq, lat, lng, mmnoise, dataDir, pntflag)
%       Mode 1: single point calculation with the argument conventions of
%       the ITU command-line program.
%
%   Inputs (numbers or numeric strings, as on a command line):
%     month   - 1..12
%     hour    - 1..24 UTC (NOTE: 1-based like the C program; hour h maps to
%               engine hour h-1)
%     freq    - 0.01..30 MHz
%     lat     - -90..90 degrees
%     lng     - -180..180 degrees
%     mmnoise - 0..5 category or negative dB override
%     dataDir - char, folder with COEFFmmW.txt (must exist)
%     pntflag - 0 no output; 1 detailed report to stdout (default);
%               2 report to MakeNoiseOut.txt; 3 CSV line with a column
%               description header; 4 CSV line only. The CSV columns are
%               month, hour-1, freq, lat, lng, then the 12 results.
%   Outputs:
%     retval - 0 on success (errors are raised as MATLAB errors rather than
%              returned as codes).
%     out    - 1x12 result vector (see p372.makeNoise).
%   Errors:
%     p372:iturNoise for out-of-range arguments or a missing data folder.
%
%   Example:
%     p372.iturNoise(1, 14, 1.0, 40.0, 165.0, 0, p372.defaultDataDir(), 3)
%
%   See also p372.makeNoise, p372.runAtmosNoiseMonths.
retval = 0; out = [];
args = varargin;
for k = 1:numel(args)
    if ischar(args{k}) && k ~= 7 && ~(numel(args) == 1)
        args{k} = str2double(args{k});
    end
end
if numel(args) == 1
    p372.runAtmosNoiseMonths(args{1});
    return;
end
if numel(args) < 7
    printUsage();
    error('p372:iturNoise', 'Insufficient number (%d) of arguments, 7 required.', numel(args));
end
[month, hour, freq, lat, lng, mmnoise, dataDir] = args{1:7};
if month < 1 || month > 12, error('p372:iturNoise', 'Month (%d) Out of Range (1 to 12)', month); end
if hour < 1 || hour > 24, error('p372:iturNoise', 'Hour (%d (UTC)) Out of Range (1 to 24 UTC)', hour); end
if freq < 0.01 || freq > 30, error('p372:iturNoise', 'Frequency (%5.4f (MHz)) Out of Range (0.01 to 30 MHz)', freq); end
if lat < -90 || lat > 90, error('p372:iturNoise', 'Latitude (%5.4f (degrees)) Out of Range (-90 to 90 degrees)', lat); end
if lng < -180 || lng > 180, error('p372:iturNoise', 'Longitude (%5.4f (degrees)) Out of Range (-180 to 180 degrees)', lng); end
if ~exist(dataDir, 'dir'), error('p372:iturNoise', 'Data file path %s does not exist', dataDir); end
pntflag = 1;
if numel(args) >= 8, pntflag = args{8}; end
mnpntflag = 1;
switch pntflag
    case 0, mnpntflag = 0;
    case 1, mnpntflag = 1;
    case 2, mnpntflag = 2;
    case {3, 4}, mnpntflag = 0;
end
[~, out] = p372.makeNoise(month, hour - 1, lat, lng, freq, mmnoise, dataDir, mnpntflag);
if pntflag == 3
    printCSVHeader();
end
if pntflag == 3 || pntflag == 4
    fprintf(['%d, %d, %5.4f, %5.4f, %5.4f' repmat(', %5.4f', 1, 12) '\n'], month, hour - 1, freq, lat, lng, out);
end
end

function printUsage()
fprintf('USAGE: p372.iturNoise(month, hour, frequency, latitude, longitude, manMadeNoise, dataDir [, printFlag])\n');
fprintf('\tmonth 1 to 12; hour 1 to 24 (UTC); frequency 0.01 to 30 MHz\n');
fprintf('\tman-made noise: 0 City, 1 Residential, 2 Rural, 3 Quiet Rural, 4 Noisy, 5 Quiet, or -dB value\n');
fprintf('\tprintFlag: 0 none, 1 report, 2 report to file, 3 CSV with header, 4 CSV\n');
end

function printCSVHeader()
stars = repmat('*', 1, 78);
fprintf('%s\n\t\tITU-R Study Group 3: Radiowave Propagation\n%s\n', stars, stars);
fprintf('\t\tAnalysis: %s\n\t\tP372 Version:      %s\n%s\n', datestr(now), p372.version(), stars);
names = {'Month', 'Hour (UTC)', 'Frequency (MHz)', 'Latitude (deg)', 'Longitude (deg)', ...
    '[FaA]  Noise Component (Atmospheric)', '[DuA]  Upper Decile    (Atmospheric)', ...
    '[DlA]  Lower Decile    (Atmospheric)', '[FaM]  Noise Component    (Man-Made)', ...
    '[DuM]  Upper Decile       (Man-Made)', '[DlM]  Lower Decile       (Man-Made)', ...
    '[FaG]  Noise Component    (Galactic)', '[DuG]  Upper Decile       (Galactic)', ...
    '[DlG]  Lower Decile       (Galactic)', '[FamT] Noise                 (Total)', ...
    '[DuT]  Upper Decile          (Total)', '[DlT]  Lower Decile          (Total)'};
for k = 1:numel(names)
    fprintf('Column %-2d: %s\n', k, names{k});
end
fprintf('%s\n', stars);
end
