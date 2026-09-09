function R = rules(path)
%RULES Resolve the reading options of a path structure.
%   R = p533.rules(path) returns
%     R.liSum    true: eq. (20) evaluated as the printed sum over the 2n
%                penetration points; false: n hops times the mean over the
%                points (default), the reading validated by the D1 databank
%     R.ref      true: reproduce the ITU C code conventions for comparison
%                tests (penetration geometry from the mode's hr, fL and Lh
%                averaged over the Table 1d) points, C night foE, C solar
%                geometry and hour indexing of the long-path model)
%   from path.liRule: 'hopmean' (default) | 'sum' | 'p533-14' (= sum) |
%   'reference' (= hopmean with R.ref true).
lr = 'hopmean';
if isfield(path, 'liRule') && ~isempty(path.liRule), lr = lower(path.liRule); end
switch lr
    case {'hopmean'},          R.liSum = false; R.ref = false;
    case {'sum', 'p533-14'},   R.liSum = true;  R.ref = false;
    case {'reference'},        R.liSum = false; R.ref = true;
    otherwise, error('p533:rules', 'Unknown liRule ''%s'' (hopmean | sum | reference).', lr);
end
end
