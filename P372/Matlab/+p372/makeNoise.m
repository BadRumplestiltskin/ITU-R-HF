function [n, out] = makeNoise(month, hour, latDeg, lngDeg, freq, mmnoise, dataDir, pntflag)
%MAKENOISE Stand-alone single-point P.372-14 noise calculation.
%   [n, out] = p372.makeNoise(month, hour, latDeg, lngDeg, freq, mmnoise)
%   [n, out] = p372.makeNoise(month, hour, latDeg, lngDeg, freq, mmnoise, dataDir, pntflag)
%
%   Convenience wrapper that reads the coefficients, converts degrees to
%   radians, runs p372.noise and optionally prints the report. Equivalent
%   of MakeNoise() in MakeNoise.c except that month is 1-based.
%
%   Inputs:
%     month   - 1..12.
%     hour    - UTC hour 0..23.
%     latDeg  - latitude, degrees, -90..90 (north positive).
%     lngDeg  - longitude, degrees, -180..180 (east positive).
%     freq    - MHz, 0.01..30.
%     mmnoise - man-made category 0..5 or negative override (see p372.noise).
%     dataDir - optional folder with COEFFmmW.txt; default p372.defaultDataDir().
%     pntflag - optional: 0 silent (default), 1 print the report to the
%               command window, 2 write it to MakeNoiseOut.txt in the
%               current folder.
%   Outputs:
%     n   - result struct (see p372.noise).
%     out - 1x12 double [FaA DuA DlA FaM DuM DlM FaG DuG DlG FamT DuT DlT],
%           the order of the C output array.
%   Errors:
%     p372:makeNoise  if the report file cannot be opened; plus the
%     errors of p372.readFamDud.
%
%   Example:
%     [n, out] = p372.makeNoise(1, 13, 40, 165, 1.0, 0, [], 1);
%
%   See also p372.noise, p372.iturNoise, p372.formatReport.
if nargin < 8 || isempty(pntflag)
    pntflag = 0;
end
if nargin < 7 || isempty(dataDir)
    dataDir = p372.defaultDataDir();
end
coeff = p372.readFamDud(dataDir, month);
rlat = latDeg * p372.D2R();
rlng = lngDeg * p372.D2R();
n = p372.noise(coeff, mmnoise, hour, rlng, rlat, freq);
out = [n.FaA n.DuA n.DlA n.FaM n.DuM n.DlM n.FaG n.DuG n.DlG n.FamT n.DuT n.DlT];

if pntflag == 1
    fprintf('%s', p372.formatReport(n, month, hour, rlng * p372.R2D(), rlat * p372.R2D(), freq));
elseif pntflag == 2
    fname = 'MakeNoiseOut.txt';
    fid = fopen(fname, 'w');
    if fid < 0
        error('p372:makeNoise', 'Can''t open output file %s', fname);
    end
    fprintf('MakeNoise: Writing output file %s\n', fname);
    fprintf(fid, '%s', p372.formatReport(n, month, hour, rlng * p372.R2D(), rlat * p372.R2D(), freq));
    fclose(fid);
end
end
