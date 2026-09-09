function maps = readIonParameters(dataDir, month)
%READIONPARAMETERS Read the monthly foF2 and M(3000)F2 grid maps (ionosNN.bin).
%   maps = p533.readIonParameters(dataDir, month)   month 1..12
%   Returns
%     maps.foF2  (24 hour, 241 lng, 121 lat, 2 ssn) single, MHz
%     maps.M3kF2 same dims, dimensionless
%     maps.month, maps.file
%   Grid: longitude -180..180 deg step 1.5 (241), latitude -90..90 step 1.5
%   (121), hour 0..23 UTC, ssn levels R12 = 0 and 100 (P.1239 section 3.1;
%   grid-point tables produced by the ITU iongrid program).
%
%   File layout (Fortran unformatted stream, little-endian): 5-byte record
%   header, 1 399 728 float32 foF2, 10 bytes of record markers, 1 399 728
%   float32 M(3000)F2, 5-byte trailer. Values are stored with hour varying
%   fastest, then latitude, longitude, ssn.
if month < 1 || month > 12, error('p533:readIonParameters', 'month out of range'); end
fname = fullfile(dataDir, sprintf('ionos%02d.bin', month));
fid = fopen(fname, 'r', 'ieee-le');
if fid < 0, error('p533:readIonParameters', 'Cannot open %s', fname); end
n = 24 * 121 * 241 * 2;
fseek(fid, 5, 'bof');
a = fread(fid, n, 'float32=>single');
fseek(fid, 10, 'cof');
b = fread(fid, n, 'float32=>single');
fclose(fid);
if numel(a) ~= n || numel(b) ~= n, error('p533:readIonParameters', '%s is truncated', fname); end
% file order [ssn][lng][lat][hour] with hour fastest -> reshape (hour,lat,lng,ssn)
maps.foF2 = permute(reshape(a, [24, 121, 241, 2]), [1 3 2 4]);
maps.M3kF2 = permute(reshape(b, [24, 121, 241, 2]), [1 3 2 4]);
maps.month = month;
maps.file = fname;
end
