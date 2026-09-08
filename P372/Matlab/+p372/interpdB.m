function y = interpdB(a, b, slp)
%INTERPDB Interpolate between two dB values linearly in power.
%   y = p372.interpdB(a, b, slp) returns 10*log10(Pa + (Pb - Pa)*slp) where
%   Pa = 10^(a/10) and Pb = 10^(b/10). a and b may be arrays of the same
%   size; slp is a scalar in 0..1.
%
%   Used to blend the statistics of adjacent 4-hour time blocks in
%   p372.atmosphericNoise and p372.atmosphericNoiseLT, exactly as
%   Noise.c does.
%
%   See also p372.atmosphericNoise, p372.atmosphericNoiseLT.
fa = 10 .^ (a / 10) + (10 .^ (b / 10) - 10 .^ (a / 10)) * slp;
y = 10 * log10(fa);
end
