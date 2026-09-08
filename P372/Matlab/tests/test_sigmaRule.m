function test_sigmaRule()
%TEST_SIGMARULE The 'p372-17' sigma rule caps sigma_T by equation (25).
%   For every reference point, recomputes the Part 7 combination from the
%   reference component values with sigma_T = min(eq.19, eq.25) when a
%   decile exceeds 12 dB, and checks p372.noise(..., 'p372-17') against it.
%   Also checks that both rules agree when no decile exceeds 12 dB, that
%   'p372-17' is the default, and that an unknown rule errors.
[dataDir, refDir] = testPaths();
M = csvread(fullfile(refDir, 'ref_points.csv'), 1, 0);
M = M(M(:, 6) >= 0, :);                    % skip the override rows
M = M(1:7:end, :);                         % every 7th row keeps runtime low
D2R = p372.D2R(); c = 10 / log(10);
coeff = []; cur = -1; nCapped = 0;
for r = 1:size(M, 1)
    if M(r, 1) ~= cur, coeff = p372.readFamDud(dataDir, M(r, 1)); cur = M(r, 1); end
    args = {coeff, M(r, 6), M(r, 2), M(r, 4) * D2R, M(r, 3) * D2R, M(r, 5)};
    nRef = p372.noise(args{:}, 'reference');
    nCap = p372.noise(args{:});
    nDef = p372.noise(args{:}, '');
    assert(isequal(nCap, nDef) && isequal(nCap, p372.noise(args{:}, 'p372-17')), ...
           'default must be the p372-17 rule');
    % expected values from the reference components
    [FamTu, DuT, cu] = expected(nRef.FaA, nRef.DuA, nRef.FaG, nRef.DuG, nRef.FaM, nRef.DuM, c);
    [FamTl, DlT, cl] = expected(nRef.FaA, nRef.DlA, nRef.FaG, nRef.DlG, nRef.FaM, nRef.DlM, c);
    assert(abs(nCap.DuT - DuT) < 1e-9 && abs(nCap.DlT - DlT) < 1e-9 && ...
           abs(nCap.FamT - min(FamTu, FamTl)) < 1e-9, 'row %d: capped result differs', r);
    assert(nCap.DuT <= nRef.DuT + 1e-12 && nCap.DlT <= nRef.DlT + 1e-12, 'cap must not raise sigma');
    if ~cu && ~cl
        assert(isequal(nCap, nRef), 'row %d: rules must agree below 12 dB', r);
    end
    nCapped = nCapped + (cu || cl);
end
assert(nCapped > 0, 'test data never triggered the cap');
try
    p372.noise(coeff, 0, 0, 0, 0, 1, 'bogus');
    error('expected an error for an unknown rule');
catch err
    assert(strcmp(err.identifier, 'p372:noise'));
end
fprintf('(%d rows, %d with a >12 dB decile) ', size(M, 1), nCapped);
end

function [FamT, DT, triggered] = expected(FaA, DA, FaG, DG, FaM, DM, c)
sA = DA / 1.282; sG = 1.56; sM = DM / 1.282;
eA = exp(FaA / c + sA^2 / (2*c^2)); eG = exp(FaG / c + sG^2 / (2*c^2)); eM = exp(FaM / c + sM^2 / (2*c^2));
al = eA + eG + eM;
be = eA^2 * (exp((sA/c)^2) - 1) + eG^2 * (exp((sG/c)^2) - 1) + eM^2 * (exp((sM/c)^2) - 1);
ga = exp(FaA / c) + exp(FaG / c) + exp(FaM / c);
s19 = c * sqrt(log(1 + be / al^2));
triggered = DA > 12 || DG > 12 || DM > 12;
if triggered
    sT = min(s19, c * sqrt(2 * log(al / ga)));
else
    sT = s19;
end
FamT = c * (log(al) - sT^2 / (2*c^2));
DT = 1.282 * sT;
end
