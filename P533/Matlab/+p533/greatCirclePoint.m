function [lat, lng] = greatCirclePoint(lat1, lng1, lat2, lng2, fraction)
%GREATCIRCLEPOINT Point at a fraction of the great circle from 1 to 2.
%   [lat, lng] = p533.greatCirclePoint(lat1, lng1, lat2, lng2, fraction)
%   radians; fraction may be an array (0 = point 1, 1 = point 2, values
%   outside 0..1 continue along the same great circle, which is how long
%   path control points are found).
d = 2 * asin(sqrt(sin((lat2 - lat1) / 2) .^ 2 + cos(lat1) .* cos(lat2) .* sin((lng2 - lng1) / 2) .^ 2));
if d == 0
    lat = lat1 * ones(size(fraction)); lng = lng1 * ones(size(fraction)); return;
end
A = sin((1 - fraction) * d) / sin(d);
B = sin(fraction * d) / sin(d);
x = A * cos(lat1) * cos(lng1) + B * cos(lat2) * cos(lng2);
y = A * cos(lat1) * sin(lng1) + B * cos(lat2) * sin(lng2);
z = A * sin(lat1) + B * sin(lat2);
lat = atan2(z, sqrt(x .^ 2 + y .^ 2));
lng = atan2(y, x);
end
