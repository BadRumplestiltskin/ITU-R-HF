function [season, time_lt, m, h] = parseFilename(file)
%PARSEFILENAME Season and local-time labels from a figure data file name.
%   [season, time_lt, m, h] = p372.plots.parseFilename(file)
%
%   Inputs:
%     file - path of an x_<m>m<h>h.csv file from p372.runAtmosNoiseMonths.
%   Outputs:
%     season  - 'DEC-JAN-FEB', 'MAR-APR-MAY', 'JUN-JUL-AUG' or 'SEP-OCT-NOV'
%     time_lt - '0000-0400_LT' .. '2000-2400_LT'
%     m, h    - the month and local hour parsed from the name.
%
%   See also p372.plots.makeP372Figs.
[~, name] = fileparts(file);
nums = str2double(regexp(name, '\d+', 'match'));
m = nums(1); h = nums(2);
seasons = {'DEC-JAN-FEB', 'MAR-APR-MAY', 'JUN-JUL-AUG', 'SEP-OCT-NOV'};
season = seasons{floor(mod(m, 12) / 3) + 1};
blocks = {'0000-0400_LT', '0400-0800_LT', '0800-1200_LT', '1200-1600_LT', '1600-2000_LT', '2000-2400_LT'};
time_lt = blocks{floor(h / 4) + 1};
end
