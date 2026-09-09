function s = seasonOf(lat, month, scheme)
%SEASONOF Season index for a latitude and month.
%   s = p533.seasonOf(lat, month)           P.533 section 5.2 (Lh, Rop):
%       N: winter Dec-Feb, equinox Mar-May and Sep-Nov, summer Jun-Aug; S interchanged
%   s = p533.seasonOf(lat, month, 'p1239')  P.1239 section 3.2 (decile tables):
%       N: winter Nov-Feb, equinox Mar, Apr, Sep, Oct, summer May-Aug; S interchanged
%   Returns 1 winter, 2 equinox, 3 summer (p533.constants().SEASON).
if nargin < 3, scheme = 'p533'; end
switch lower(scheme)
    case 'p533',  tab = [1 1 2 2 2 3 3 3 2 2 2 1];
    case 'p1239', tab = [1 1 2 2 3 3 3 3 2 2 1 1];
    otherwise, error('p533:seasonOf', 'unknown scheme %s', scheme);
end
s = tab(month);
if lat < 0
    s = 4 - s;                         % winter <-> summer, equinox unchanged
end
end
