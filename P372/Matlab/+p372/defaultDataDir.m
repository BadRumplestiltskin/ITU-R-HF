function d = defaultDataDir()
%DEFAULTDATADIR Folder containing the ITU coefficient files.
%   d = p372.defaultDataDir() returns the first existing folder of
%     <root>/Data      (stand-alone distribution of this package)
%     <root>/../Data   (package installed as P372/Matlab inside the
%                       ITU-R-HF repository, next to P372/Data)
%   where <root> is the parent of the +p372 package directory. The folder
%   holds COEFF01W.txt .. COEFF12W.txt, V_d.txt and sigma_V_d.txt.
%
%   Functions that take a dataDir argument use this folder when the
%   argument is omitted or empty.
%
%   See also p372.readFamDud, p372.readVdCoeffs, p372.makeNoise.
root = fileparts(fileparts(mfilename('fullpath')));
d = fullfile(root, 'Data');
if ~exist(d, 'dir')
    alt = fullfile(fileparts(root), 'Data');
    if exist(alt, 'dir')
        d = alt;
    end
end
end
