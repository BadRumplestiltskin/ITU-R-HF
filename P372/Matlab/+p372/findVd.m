function [V_d, sigma_V_d] = findVd(freq, c, d)
%FINDVD Voltage deviation V_d and its standard deviation versus frequency.
%   [V_d, sigma_V_d] = p372.findVd(freq, c, d)
%
%   Evaluates equations (30) and (31) of NTIA Report 85-173 (Spaulding and
%   Washburn, "Atmospheric radio noise: worldwide levels and other
%   characteristics"): quartic polynomials in log10(freq). V_d is the
%   ratio of the r.m.s. to the average noise envelope voltage, in dB, and
%   characterises the impulsiveness of the noise.
%
%   Inputs:
%     freq - MHz, scalar or array.
%     c, d - 5-element coefficient vectors (constant term first) for one
%            season and time block, from p372.readVdCoeffs.
%   Outputs:
%     V_d, sigma_V_d - dB, same size as freq.
%
%   Reference: ITURNoise.c FindV_d().
%
%   See also p372.readVdCoeffs, p372.runAtmosNoiseMonths.
x = log10(freq);
x2 = x .* x; x3 = x2 .* x; x4 = x3 .* x;
V_d       = c(1) + c(2) * x + c(3) * x2 + c(4) * x3 + c(5) * x4;
sigma_V_d = d(1) + d(2) * x + d(3) * x2 + d(4) * x3 + d(5) * x4;
end
