function v = D2R()
%D2R Degrees-to-radians factor used by the ITU P372 C reference code.
%   v = p372.D2R() returns 0.0174532925.
%
%   This is deliberately the truncated constant from the C header Common.h,
%   not pi/180 (which differs by about 7.9e-11). Using the identical value
%   keeps results bit-compatible with the reference implementation, in
%   particular the local-time truncation in p372.atmosphericNoise.
%
%   Example:
%     rlat = 40 * p372.D2R();
%
%   See also p372.R2D, p372.atmosphericNoise, p372.makeNoise.
v = 0.0174532925;
end
