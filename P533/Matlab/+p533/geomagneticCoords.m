function [glat, glng] = geomagneticCoords(lat, lng, pole)
%GEOMAGNETICCOORDS Geomagnetic latitude and longitude for a dipole pole.
%   [glat, glng] = p533.geomagneticCoords(lat, lng)        pole 78.5 N 68.2 W
%                                                          (P.533 section 5.2, Lh)
%   [glat, glng] = p533.geomagneticCoords(lat, lng, pole)  pole = [lat lng] rad,
%                  e.g. p533.constants().POLE_1239 for P.842 Table 2 note (1)
%   Inputs radians, arrays allowed. glat = asin(sin p sin lat + cos p cos lat cos(lng - plng)).
c = p533.constants();
if nargin < 3, pole = c.POLE_LH; end
plat = pole(1); plng = pole(2);
glat = asin(sin(plat) .* sin(lat) + cos(plat) .* cos(lat) .* cos(lng - plng));
glng = atan2(cos(lat) .* sin(lng - plng), sin(plat) .* cos(lat) .* cos(lng - plng) - cos(plat) .* sin(lat));
end
