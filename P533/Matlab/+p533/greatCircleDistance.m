function d = greatCircleDistance(lat1, lng1, lat2, lng2)
%GREATCIRCLEDISTANCE Great-circle distance in km (haversine, R0 = 6371 km).
%   d = p533.greatCircleDistance(lat1, lng1, lat2, lng2), radians, arrays ok.
c = p533.constants();
a = sin((lat2 - lat1) / 2) .^ 2 + cos(lat1) .* cos(lat2) .* sin((lng2 - lng1) / 2) .^ 2;
d = 2 * c.R0 * atan2(sqrt(a), sqrt(1 - a));
end
