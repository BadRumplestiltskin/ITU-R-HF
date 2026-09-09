function cp = newControlPoint(lat, lng, distance)
%NEWCONTROLPOINT Empty control point struct (P.533 Table 1).
%   cp = p533.newControlPoint(lat, lng, distance)  radians, km from the transmitter
%   Fields: lat, lng, distance, foE, foF2, M3kF2, dip100, dip300, fH100, fH300,
%   ltime, hr, x, and the solar fields (decl, eot, ha, sza, sha, lsr, lsn, lss,
%   polarNight, polarDay). Unset numeric fields are NaN.
if nargin < 3, distance = NaN; end
if nargin < 1, lat = NaN; lng = NaN; end
f = {'foE','foF2','M3kF2','dip100','dip300','fH100','fH300','ltime','hr','x', ...
     'decl','eot','ha','sza','sha','lsr','lsn','lss'};
cp = struct('lat', lat, 'lng', lng, 'distance', distance);
for k = 1:numel(f), cp.(f{k}) = NaN; end
cp.polarNight = false; cp.polarDay = false;
end
