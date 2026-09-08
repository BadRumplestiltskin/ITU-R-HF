function lines = readLines(fname)
%READLINES Read a text file into a cell array of lines.
%   lines = p372.readLines(fname)
%
%   Inputs:
%     fname - char, path of the text file.
%   Outputs:
%     lines - 1xN cell array of char, one element per line, without line
%             terminators. Both LF and CRLF files are accepted.
%   Errors:
%     p372:readLines  if the file does not exist.
%
%   Internal helper used by the coefficient readers; it is a public package
%   function only because Octave does not resolve package-private folders.
%
%   See also p372.readNamedBlock, p372.readFamDud, p372.readCoeff.
if ~exist(fname, 'file')
    error('p372:readLines', 'Can''t find input file - %s', fname);
end
txt = fileread(fname);
txt = strrep(txt, char(13), '');        % tolerate CRLF
lines = strsplit(txt, char(10));
end
