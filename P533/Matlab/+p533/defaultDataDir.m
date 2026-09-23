function d = defaultDataDir()
%DEFAULTDATADIR Folder with ionosNN.bin, P1239-3 Decile Factors.txt and COEFFmmW.txt.
%   Looks for <root>/Data, then <root>/../Data (repository layout P533/Matlab
%   next to P533/Data). Both are relative to this file, so the function works
%   wherever the repository is checked out.
root = fileparts(fileparts(mfilename('fullpath')));
cands = {fullfile(root, 'Data'), fullfile(fileparts(root), 'Data')};
for k = 1:numel(cands)
    if exist(fullfile(cands{k}, 'ionos01.bin'), 'file')
        d = cands{k}; return;
    end
end
error('p533:defaultDataDir', 'No data folder with ionos01.bin found; pass dataDir explicitly.');
end
