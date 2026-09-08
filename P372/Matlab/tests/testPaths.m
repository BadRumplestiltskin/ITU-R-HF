function [dataDir, refDir] = testPaths()
%TESTPATHS Locations of the data and reference folders used by the tests.
%   [dataDir, refDir] = testPaths()
dataDir = p372.defaultDataDir();
refDir = fullfile(fileparts(mfilename('fullpath')), 'reference');
end
