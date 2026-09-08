function [lat, lng] = pathPoint(path, distFromTx)
%PATHPOINT Location at a given distance from the transmitter along the path.
%   [lat, lng] = p533.pathPoint(path, distFromTx)  distFromTx km (array ok)
%   Follows the short great-circle arc, or the long way round when
%   path.SorL is 1 (section 2; the long path continues the same great
%   circle away from the receiver).
c = p533.constants();
ds = p533.greatCircleDistance(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng);
if path.SorL == c.SORL.LONG
    frac = -distFromTx / ds;
else
    frac = distFromTx / ds;
end
[lat, lng] = p533.greatCirclePoint(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng, frac);
end
