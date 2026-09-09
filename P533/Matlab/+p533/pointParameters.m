function P = pointParameters(maps, month, hourUTC, R12, lat, lng, cMode)
%POINTPARAMETERS Ionospheric, solar and magnetic parameters at points.
%   P = p533.pointParameters(maps, month, hourUTC, R12, lat, lng)
%   P = p533.pointParameters(..., cMode)   cMode true: C night foE (see p533.findfoE)
%   Vectorised over lat, lng (radians, equal size). Returns a struct of
%   arrays: foF2, M3kF2 (P.533 3.4), foE (P.1239 section 4), dip100, fH100,
%   dip300, fH300 (P.1239 section 2), and the solar fields of
%   p533.solarParameters (decl, eot, ha, sza, sha, lsr, lsn, lss, ltime,
%   polarNight, polarDay). This is the vectorised core of
%   p533.calculateCPParameters and of the area-coverage mode of the app.
[P.foF2, P.M3kF2] = p533.ionosphericParameters(maps, hourUTC, R12, lat, lng);
S = p533.solarParameters(lat, lng, month, hourUTC);
fn = fieldnames(S);
for k = 1:numel(fn), P.(fn{k}) = S.(fn{k}); end
if nargin < 7, cMode = false; end
P.foE = p533.findfoE(lat, S, month, hourUTC, R12, cMode);
[P.dip100, P.fH100] = p533.magfit(lat, lng, 100);
[P.dip300, P.fH300] = p533.magfit(lat, lng, 300);
end
