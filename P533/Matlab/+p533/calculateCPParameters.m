function cp = calculateCPParameters(path, cp)
%CALCULATECPPARAMETERS Fill a control point with ionospheric and solar data.
%   cp = p533.calculateCPParameters(path, cp) evaluates foF2, M(3000)F2
%   (section 3.4), foE (P.1239 section 4), dip and gyrofrequency at 100
%   and 300 km (P.1239 section 2) and the solar geometry at cp.lat, cp.lng
%   for path.maps, path.month, path.hour and path.SSN. Vectorised work is
%   done by p533.pointParameters.
R = p533.rules(path); cMode = R.ref;
P = p533.pointParameters(path.maps, path.month, path.hour, path.SSN, cp.lat, cp.lng, cMode);
fn = fieldnames(P);
for k = 1:numel(fn), cp.(fn{k}) = P.(fn{k}); end
end
