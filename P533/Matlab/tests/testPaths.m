function [dataDir, refDir, antDir] = testPaths()
%TESTPATHS Data, reference and antenna folders used by the tests.
dataDir = p533.defaultDataDir();
refDir = fullfile(fileparts(mfilename('fullpath')), 'reference');
here = fileparts(mfilename('fullpath'));
cands = {fullfile(here, '..', '..', '..', 'ITURHFProp', 'Data', 'Antenna', 'T13 Files'), ...   % repository: P533/Matlab/tests
         fullfile(here, '..', '..', 'ITURHFProp', 'Data', 'Antenna', 'T13 Files')};
antDir = cands{1};
for k = 1:numel(cands), if exist(cands{k}, 'dir'), antDir = cands{k}; break; end, end
end
