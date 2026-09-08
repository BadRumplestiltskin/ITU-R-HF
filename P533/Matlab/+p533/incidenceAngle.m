function i = incidenceAngle(delta, h)
%INCIDENCEANGLE Angle of incidence at height h for elevation delta (eq. 12).
%   i = p533.incidenceAngle(delta, h)  radians, h km; arrays ok.
c = p533.constants();
i = asin(c.R0 * cos(delta) ./ (c.R0 + h));
end
