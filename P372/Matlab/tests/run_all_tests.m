function ok = run_all_tests()
%RUN_ALL_TESTS Run the P372 test suite (MATLAB and GNU Octave).
%   run_all_tests            prints one line per test and errors on failure
%   ok = run_all_tests       returns true when every test passed
%
%   Every test_*.m file in this folder is a function that takes no
%   arguments and throws on failure (plain assert, so no toolbox or
%   matlab.unittest dependency). The parent folder is added to the path so
%   that the +p372 package resolves.
%
%   Reference data in ./reference is produced from the original ITU C code
%   by ../tools/build_reference.sh; see ../docs/VALIDATION.md.
%   Typical run time: about 90 s in Octave, dominated by
%   test_noise_vs_reference (24 500 points).
here = fileparts(mfilename('fullpath'));
addpath(fileparts(here));                    % make +p372 visible
files = dir(fullfile(here, 'test_*.m'));
names = sort({files.name});
nfail = 0;
t0 = tic;
for k = 1:numel(names)
    [~, fn] = fileparts(names{k});
    fprintf('%-32s ', fn);
    try
        feval(fn);
        fprintf('PASS\n');
    catch err
        nfail = nfail + 1;
        fprintf('FAIL\n    %s\n', strrep(err.message, sprintf('\n'), sprintf('\n    ')));
    end
end
fprintf('%d of %d tests passed (%.1f s)\n', numel(names) - nfail, numel(names), toc(t0));
ok = nfail == 0;
if nargout == 0 && ~ok
    error('run_all_tests:failed', '%d test(s) failed.', nfail);
end
end
