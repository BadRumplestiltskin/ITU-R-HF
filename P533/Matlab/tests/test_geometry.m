function test_geometry()
%TEST_GEOMETRY Distances, bearings, intermediate points and geomagnetic coordinates.
[~, refDir] = testPaths();
M = csvread(fullfile(refDir, 'dump', 'geometry.csv'), 1, 0);
D2R = pi / 180;
for i = 1:size(M, 1)
    la1 = M(i,1)*D2R; lo1 = M(i,2)*D2R; la2 = M(i,3)*D2R; lo2 = M(i,4)*D2R;
    d = p533.greatCircleDistance(la1, lo1, la2, lo2);
    assert(abs(d - M(i,5)) < 0.05, 'distance row %d: %g vs %g', i, d, M(i,5));  % R0 6371 vs 6371.009
    assert(abs(p533.bearing(la1, lo1, la2, lo2) - M(i,6)) < 1e-7 && abs(p533.bearing(la1, lo1, la2, lo2, true) - M(i,7)) < 1e-7, 'bearing row %d', i);
    [mlat, mlng] = p533.greatCirclePoint(la1, lo1, la2, lo2, 0.5);
    [qlat, qlng] = p533.greatCirclePoint(la1, lo1, la2, lo2, 0.75);
    assert(abs(mlat - M(i,8)) < 1e-7 && abs(angdiff(mlng, M(i,9))) < 1e-7, 'midpoint row %d', i);
    assert(abs(qlat - M(i,10)) < 1e-7 && abs(angdiff(qlng, M(i,11))) < 1e-7, '3/4 point row %d', i);
    [glat, glng] = p533.geomagneticCoords(la1, lo1);
    assert(abs(glat - M(i,12)) < 1e-7, 'geomagnetic lat row %d', i);
end
end
function a = angdiff(x, y)
a = mod(x - y + pi, 2 * pi) - pi;
end
