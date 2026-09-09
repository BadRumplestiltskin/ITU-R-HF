function delta = elevationAngle(d, hr)
%ELEVATIONANGLE Ray elevation for a hop of length d and mirror height hr (eq. 13).
%   delta = p533.elevationAngle(d, hr)  d km, hr km, result radians; arrays ok.
c = p533.constants();
x = d / (2 * c.R0);
delta = atan(cot(x) - (c.R0 ./ (c.R0 + hr)) ./ sin(x));
end
