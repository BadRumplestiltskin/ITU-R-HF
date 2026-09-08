function path = initializePath(path)
%INITIALIZEPATH Path length, bearings, first control points, season, noise.
%   path = p533.initializePath(path)
%   Sets path.distance (km, long path = 2 pi R0 - short distance), the
%   control points of Table 1 that do not depend on d0: MP at D/2 and,
%   for D >= 2000 km, T1k and R1k at 1000 km from each end (Td02 and Rd02
%   are set by p533.mufBasic once the lowest-order hop is known), the
%   P.533 section 5.2 season at the mid-point, the bearings, and resets
%   every output to its sentinel value.
c = p533.constants();
out = p533.newPath();
in = {'name','txname','rxname','year','month','hour','SSN','frequency','BW','txpower', ...
      'Modulation','SorL','SNRr','SNRXXp','SIRr','A','TW','FW','T0','F0','L_tx','L_rx', ...
      'A_tx','A_rx','manMadeNoise','sigmaRule','liRule','maps','foF2var','coeff372'};
for k = 1:numel(in), out.(in{k}) = path.(in{k}); end
path = out;
ds = p533.greatCircleDistance(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng);
if path.SorL == c.SORL.LONG
    path.distance = 2 * pi * c.R0 - ds;
else
    path.distance = ds;
end
path.txBearing = p533.bearing(path.L_tx.lat, path.L_tx.lng, path.L_rx.lat, path.L_rx.lng, path.SorL == c.SORL.LONG);
path.rxBearing = p533.bearing(path.L_rx.lat, path.L_rx.lng, path.L_tx.lat, path.L_tx.lng, path.SorL == c.SORL.LONG);
[lat, lng] = p533.pathPoint(path, path.distance / 2);
path.CP(c.cp.MP) = p533.calculateCPParameters(path, p533.newControlPoint(lat, lng, path.distance / 2));
if path.distance >= 2000
    [lat, lng] = p533.pathPoint(path, 1000);
    path.CP(c.cp.T1k) = p533.calculateCPParameters(path, p533.newControlPoint(lat, lng, 1000));
    [lat, lng] = p533.pathPoint(path, path.distance - 1000);
    path.CP(c.cp.R1k) = p533.calculateCPParameters(path, p533.newControlPoint(lat, lng, path.distance - 1000));
end
path.season = p533.seasonOf(path.CP(c.cp.MP).lat, path.month);
end
