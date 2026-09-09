function path = loadData(path, dataDir)
%LOADDATA Load the monthly data a path needs (ionospheric maps, deciles, noise).
%   path = p533.loadData(path)            uses p533.defaultDataDir()
%   path = p533.loadData(path, dataDir)
%   Fills path.maps (p533.readIonParameters for path.month), path.foF2var
%   (p533.readP1239, once) and path.coeff372 (p372.readFamDud for
%   path.month). Maps are reloaded only when the month changes.
if nargin < 2 || isempty(dataDir), dataDir = p533.defaultDataDir(); end
if isempty(path.foF2var), path.foF2var = p533.readP1239(dataDir); end
if isempty(path.maps) || path.maps.month ~= path.month
    path.maps = p533.readIonParameters(dataDir, path.month);
end
if isempty(path.coeff372) || path.coeff372.month ~= path.month
    path.coeff372 = p372.readFamDud(dataDir, path.month);
end
end
