function d = defaultDataDir()
%DEFAULTDATADIR Folder with ionosNN.bin, P1239-3 Decile Factors.txt and COEFFmmW.txt.
%   Looks for <root>/Data, then <root>/../Data (repository layout P533/Matlab
%   next to P533/Data), then the ITU-R-HF checkout used during development.
root = fileparts(fileparts(mfilename('fullpath')));
cands = {fullfile(root, 'Data'), fullfile(fileparts(root), 'Data'), ...
         '/Users/warrensly/NetBeansProjects/ITU-R-HF/P533/Data'};
for k = 1:numel(cands)
    if exist(fullfile(cands{k}, 'ionos01.bin'), 'file')
        d = cands{k}; return;
    end
end
error('p533:defaultDataDir', 'No data folder with ionos01.bin found; pass dataDir explicitly.');
end
