function S = solarParameters(lat, lng, month, hourUTC)
%SOLARPARAMETERS Solar geometry for the 15th of a month at a UTC hour.
%   S = p533.solarParameters(lat, lng, month, hourUTC)
%   lat, lng radians (arrays of equal size); month 1..12; hourUTC decimal.
%   Returns arrays (size of lat):
%     S.decl  solar declination (rad)     S.eot  equation of time (minutes)
%     S.ha    hour angle (rad)            S.sza  solar zenith angle (rad, 0..pi)
%     S.sha   sunrise/sunset hour angle (rad); NaN when the Sun does not
%             rise (polar night) or does not set (polar day, see S.polarDay)
%     S.lsr, S.lsn, S.lss  UTC hours of local sunrise, solar noon, sunset
%     S.polarNight, S.polarDay logical
%     S.ltime local mean time = hourUTC + lng/15 deg, hours 0..24
%   The declination and equation of time use the same mid-month
%   approximations as the ITU software (day 15, mean anomaly rate
%   0.98565 deg/day); P.533 asks for the equation of time to be included
%   for the zenith angles of eq. (20). Sunrise/sunset use a zenith of 90.833 deg.
D2R = pi / 180;
sz = size(lat); lat = lat(:); lng = lng(:);
A = 0.98565327; B = 3.98891967; V = 78.746118 * D2R; tilt = 23.45 * D2R;
doty = [0 31 59 90 120 152 181 212 243 273 304 334];
D = doty(month) + 15 + hourUTC / 24;
lambda = A * D2R * (D - 2);
nu = lambda + 1.915169 * D2R * sin(lambda);
eps = A * D2R * (D - 80);
if eps >= 270 * D2R, eps = eps - 2 * pi; elseif eps >= 90 * D2R, eps = eps - pi; end
beta = atan(cos(tilt) * tan(eps));
eot = B * ((eps - beta) + (lambda - nu)) / D2R;             % minutes, scalar
decl = asin(sin(tilt) * sin((sin(A * (D - 2) * D2R) * 0.016713 + A * (D - 2) * D2R) - V));
tst = hourUTC * 60 + lng / (15 * D2R) * 60 + eot;           % true solar time, minutes
ha = ((tst / 4) - 180) * D2R;
cosphi = sin(lat) * sin(decl) + cos(lat) * cos(decl) .* cos(ha);
cosphi = max(min(cosphi, 1), -1);
arg = cos(90.833 * D2R) ./ (cos(lat) * cos(decl)) - tan(lat) * tan(decl);
sha = acos(max(min(arg, 1), -1));
S.decl = decl * ones(sz); S.eot = eot * ones(sz);
S.ha = reshape(ha, sz); S.sza = reshape(acos(cosphi), sz);
S.polarNight = reshape(arg > 1, sz); S.polarDay = reshape(arg < -1, sz);
sha(arg > 1 | arg < -1) = NaN;
S.sha = reshape(sha, sz);
S.lsr = reshape(mod((720 + (-lng - sha) / D2R * 4 - eot) / 60, 24), sz);
S.lss = reshape(mod((720 + (-lng + sha) / D2R * 4 - eot) / 60, 24), sz);
S.lsn = reshape(mod((720 + (-lng) / D2R * 4 - eot) / 60, 24), sz);
S.ltime = reshape(mod(hourUTC + lng / (15 * D2R), 24), sz);
end
