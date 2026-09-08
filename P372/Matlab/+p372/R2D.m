function v = R2D()
%R2D Radians-to-degrees factor used by the ITU P372 C reference code.
%   v = p372.R2D() returns 57.2957795.
%
%   Deliberately the truncated constant from Common.h, not 180/pi, so that
%   printed coordinates match the reference output to every decimal.
%
%   See also p372.D2R, p372.formatReport.
v = 57.2957795;
end
