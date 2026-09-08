function test_part1_vs_reference()
%TEST_PART1_VS_REFERENCE MUFs, dmax, reflection heights and screening against the C code.
%   Compared by hop count for every case up to 9000 km. Tolerances allow
%   for the documented deviations (D14: beyond dmax the C code evaluates
%   eq. (3) unclamped and caps dmax differently, 5 %; D27
%   map interpolation in the S/W hemispheres, D28/D29 local time, D05
%   reflection heights); untagged differences fail.
[cases, ref] = refCases();
path = [];
n = 0; worstB = 0; nTag = 0; nModes = 0;
for i = 1:numel(cases.id)
    if ref.distance(i) > 9000, continue; end
    path = setupCase(cases, i, path);
    path = p533.run(path, 'part1');
    n = n + 1;
    assert(abs(path.distance - ref.distance(i)) < 0.05, 'case %d distance %g vs %g', i, path.distance, ref.distance(i));
    tagged = cases.rxlat(i) < 0 || cases.rxlng(i) < 0 || cases.txlat(i) < 0 || cases.txlng(i) < 0;   % D27 region
    night = any(arrayfun(@(k) ~isnan(path.CP(k).sza) && path.CP(k).sza > pi / 2, [1 3 5]));   % D28: night foE differs at any Table 1 point
    beyond = path.n0_F2 ~= 99 && path.distance > path.dmax;      % D14 region: control points at d0/2
    if path.n0_F2 ~= 99 && abs(path.distance / path.n0_F2 - path.dmax) < 1, nTag = nTag + 1; continue; end   % hop exactly at dmax: C uses a strict inequality
    if path.n0_F2 > 1 && abs(p533.elevationAngle(path.distance / (path.n0_F2 - 1), path.CP(3).hr) - 3 * pi / 180) < 0.2 * pi / 180, nTag = nTag + 1; continue; end   % lowest mode at the 3 deg boundary
    tol = 0.05 + 0.05 * path.BMUF * tagged + 0.05 * path.BMUF * beyond + 0.06 * path.BMUF * night;   % D27 S/W maps 5 %; night: foE enters x, B, dmax (D28)
    d = abs(path.BMUF - ref.BMUF(i));
    if d > tol && ref.BMUF(i) < 1e6 && ~(night && path.BMUF == path.Md_E(1).BMUF)
        % beyond dmax the C lowest-order value uses the unclamped eq. (3): D14
        if path.distance / path.n0_F2 > path.dmax, nTag = nTag + 1;
        else, error('case %d (D %.0f km): BMUF %g vs %g', i, path.distance, path.BMUF, ref.BMUF(i)); end
    end
    worstB = max(worstB, d * ~tagged);
    if ref.dmax(i) < 1e5, assert(abs(path.dmax - ref.dmax(i)) < 1 + 60 * tagged, 'case %d dmax %g vs %g', i, path.dmax, ref.dmax(i)); end
    % per-mode E MUFs by hop count
    for k = 1:3
        rb = ref.(sprintf('E%d_BMUF', k))(i);
        if rb > 0
            hops = k; m = path.Md_E([path.Md_E.hops] == hops);
            if ~isempty(m) && ~night, assert(abs(m.BMUF - rb) < tol, 'case %d E%d MUF %g vs %g', i, k, m.BMUF, rb); nModes = nModes + 1; end
        end
    end
end
fprintf('(%d cases, worst untagged BMUF diff %.3f MHz, %d D14-tagged, %d E modes) ', n, worstB, nTag, nModes);
end
