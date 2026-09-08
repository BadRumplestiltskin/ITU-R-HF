function txt = formatReport(n, month, hour, lngDeg, latDeg, freq, timestr)
%FORMATREPORT Detailed text report of a noise calculation.
%   txt = p372.formatReport(n, month, hour, lngDeg, latDeg, freq)
%   txt = p372.formatReport(n, month, hour, lngDeg, latDeg, freq, timestr)
%
%   Produces the same block of text as PrintFam() in MakeNoise.c and
%   ITURNoise.exe print mode 1, so reports can be compared line by line.
%
%   Inputs:
%     n       - result struct from p372.noise or p372.makeNoise.
%     month   - 1..12.
%     hour    - UTC hour 0..23; printed as 1..24 like the C program.
%     lngDeg, latDeg - degrees.
%     freq    - MHz.
%     timestr - optional analysis time stamp; default is now in the C
%               format d/m/yy - HH:MM:SS.
%   Outputs:
%     txt - char, multi-line, terminated by a newline.
%
%   The label of the atmospheric lower decile reads "Upper Decile", as in
%   the original; it is kept so that diffs against C output are clean.
%
%   See also p372.makeNoise, p372.iturNoise.
months = {'JANUARY ', 'FEBRUARY', 'MARCH', 'APRIL', 'MAY', 'JUNE', 'JULY', ...
          'AUGUST', 'SEPTEMBER', 'OCTOBER', 'NOVEMBER', 'DECEMBER'};
if nargin < 7 || isempty(timestr)
    c = clock();
    timestr = sprintf('%d/%d/%d - %02d:%02d:%02d', c(3), c(2), mod(c(1), 100), c(4), c(5), floor(c(6)));
end
nl = sprintf('\n');
stars = repmat('*', 1, 58);
L = {};
L{end+1} = stars;
L{end+1} = sprintf('\tITU-R Study Group 3: Radiowave Propagation');
L{end+1} = stars;
L{end+1} = sprintf('\tAnalysis: %s', timestr);
L{end+1} = sprintf('\tP372 Version:      %s', p372.version());
L{end+1} = sprintf('\tP372 Compile Time: %s', 'MATLAB port');
L{end+1} = stars;
L{end+1} = '';
L{end+1} = sprintf('\t%s : %d (UTC) (1 to 24)', months{month}, hour + 1);
L{end+1} = sprintf('\t%5.4f (deg lat) %5.4f (deg long)', latDeg, lngDeg);
L{end+1} = sprintf('\t%5.3f (MHz)', freq);
L{end+1} = '';
L{end+1} = sprintf('\t[FaA]  Noise Component (Atmospheric): %5.3f', n.FaA);
L{end+1} = sprintf('\t[DuA]  Upper Decile    (Atmospheric): %5.3f', n.DuA);
L{end+1} = sprintf('\t[DlA]  Upper Decile    (Atmospheric): %5.3f', n.DlA);   % sic (C label)
L{end+1} = sprintf('\t[FaM]  Noise Component    (Man-Made): %5.3f', n.FaM);
L{end+1} = sprintf('\t[DuM]  Upper Decile       (Man-Made): %5.3f', n.DuM);
L{end+1} = sprintf('\t[DlM]  Lower Decile       (Man-Made): %5.3f', n.DlM);
L{end+1} = sprintf('\t[FaG]  Noise Component    (Galactic): %5.3f', n.FaG);
L{end+1} = sprintf('\t[DuG]  Upper Decile       (Galactic): %5.3f', n.DuG);
L{end+1} = sprintf('\t[DlG]  Lower Decile       (Galactic): %5.3f', n.DlG);
L{end+1} = sprintf('\t[FamT] Noise                 (Total): %5.3f', n.FamT);
L{end+1} = sprintf('\t[DuT]  Upper Decile          (Total): %5.3f', n.DuT);
L{end+1} = sprintf('\t[DlT]  Lower Decile          (Total): %5.3f', n.DlT);
L{end+1} = '';
L{end+1} = stars;
txt = [strjoin(L, nl) nl];
end
