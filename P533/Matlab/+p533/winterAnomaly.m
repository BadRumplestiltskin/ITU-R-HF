function Aw = winterAnomaly(lat, month)
%WINTERANOMALY Winter-anomaly factor Aw for eq. (33) (Table 5).
%   Aw = p533.winterAnomaly(lat, month)  lat radians. Zero for |lat| <= 30
%   deg and at 90 deg, the Table 5 value at 60 deg, linear in between.
T = p533.tables();
peak = T.Aw(month, 1 + (lat < 0));
a = abs(lat) * 180 / pi;
if a <= 30 || a >= 90, Aw = 0;
elseif a <= 60, Aw = peak * (a - 30) / 30;
else, Aw = peak * (90 - a) / 30; end
end
