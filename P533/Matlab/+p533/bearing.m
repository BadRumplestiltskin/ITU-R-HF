function b = bearing(lat1, lng1, lat2, lng2, longPath)
%BEARING Initial great-circle bearing from point 1 to point 2, radians 0..2pi.
%   b = p533.bearing(lat1, lng1, lat2, lng2)          short path
%   b = p533.bearing(lat1, lng1, lat2, lng2, true)    long path (reverse)
if nargin < 5, longPath = false; end
y = sin(lng2 - lng1) .* cos(lat2);
x = cos(lat1) .* sin(lat2) - sin(lat1) .* cos(lat2) .* cos(lng2 - lng1);
b = mod(atan2(y, x), 2 * pi);
if longPath, b = mod(b + pi, 2 * pi); end
end
