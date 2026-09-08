function A = readNamedBlock(lines, header, n)
%READNAMEDBLOCK Read the numbers that follow a named section of a coefficient file.
%   A = p372.readNamedBlock(lines, header, n)
%
%   The ITU coefficient files (COEFFmmW.txt) consist of sections that start
%   with a header line naming a Fortran array and its dimensions, for
%   example 'fakp(29,16,6)', followed by the array values, five per line,
%   in Fortran column-major order (first index varies fastest).
%
%   Inputs:
%     lines  - cell array of char from p372.readLines.
%     header - char, exact header text (compared after trimming blanks).
%     n      - number of values to read (product of the dimensions).
%   Outputs:
%     A      - nx1 double, the values in file order.
%   Errors:
%     p372:readNamedBlock  if the header is missing or fewer than n values
%                          follow it.
%
%   Internal helper for p372.readFamDud and p372.readCoeff.
%
%   See also p372.readLines, p372.readFamDud, p372.readCoeff.
idx = find(strcmp(strtrim(lines), header), 1);
if isempty(idx)
    error('p372:readNamedBlock', 'Section "%s" not found in coefficient file.', header);
end
nl = ceil(n / 5);                       % 5 values per line
block = strjoin(lines(idx + 1 : idx + nl), ' ');
A = sscanf(block, '%f');
if numel(A) < n
    error('p372:readNamedBlock', 'Section "%s": expected %d values, read %d.', header, n, numel(A));
end
A = A(1:n);
end
