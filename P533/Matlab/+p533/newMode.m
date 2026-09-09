function m = newMode()
%NEWMODE Empty propagation mode structure (E or F2 mode of P.533 section 5.2.1).
%   Fields: hops (number of hops n), BMUF (0 = mode does not exist), MUF90, MUF50, MUF10, OPMUF,
%   OPMUF10, OPMUF90 (MHz); Fprob; deltal, deltau; hr (km); fs (MHz, F2);
%   Lb, Ew (dB); ele (rad); Prw (dBW); Grw (dBi); tau (ms); MC (considered).
cst = p533.constants();
T = cst.TINYDB;
m = struct('hops', NaN, 'BMUF', 0, 'MUF90', 0, 'MUF50', 0, 'MUF10', 0, 'OPMUF', 0, 'OPMUF10', 0, 'OPMUF90', 0, ...
           'Fprob', 0, 'deltal', NaN, 'deltau', NaN, 'hr', NaN, 'fs', NaN, 'Lb', -T, 'Ew', T, ...
           'ele', NaN, 'Prw', T, 'Grw', T, 'tau', NaN, 'MC', false, 'ptick', NaN);
end
