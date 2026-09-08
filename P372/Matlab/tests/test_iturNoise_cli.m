function test_iturNoise_cli()
%TEST_ITURNOISE_CLI Command-line front end: string arguments, CSV output, validation.
dataDir = testPaths();
rv = evalc('[~, out] = p372.iturNoise(''1'', ''14'', ''1.0'', ''40.0'', ''165.0'', ''0'', dataDir, ''4'');');
assert(abs(out(1) - 60.7326498519) < 1e-6 && abs(out(10) - 76.9865260841) < 1e-6);
assert(~isempty(strfind(rv, '1, 13, 1.0000, 40.0000, 165.0000, 60.7326')));
try
    evalc('p372.iturNoise(13, 1, 1, 0, 0, 0, dataDir, 0);');
    error('expected an error for month 13');
catch err
    assert(~isempty(strfind(err.message, 'Month')));
end
end
