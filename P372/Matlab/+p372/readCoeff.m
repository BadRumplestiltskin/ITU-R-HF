function C = readCoeff(dataDir, month, names)
%READCOEFF Read any of the arrays in an ITU COEFFmmW.txt coefficient file.
%   C = p372.readCoeff(dataDir, month)
%   C = p372.readCoeff(dataDir, month, names)
%
%   General-purpose reader for the monthly ionospheric/noise coefficient
%   files used by the ITU-R HF prediction methods (P.533, P.372, P.1239).
%   Every section of the file whose header has the form name(d1,d2,...) is
%   read. This is the counterpart of ReadCoeff.c and is intended as
%   groundwork for a P.533 port; the noise engine uses p372.readFamDud.
%
%   Inputs:
%     dataDir - char, folder containing the COEFFmmW.txt files.
%     month   - integer 1..12.
%     names   - optional cell array of char; read only these arrays
%               (e.g. {'xf2','xfm3'}). Default: all sections.
%   Outputs:
%     C - struct with one field per array, plus name (file title line),
%         month and file. Each array has the dimensions stated in its
%         header, in Fortran column-major order, 1-based, so C.xf2(i,j,k)
%         equals the Fortran element XF2(I,J,K). One-dimensional integer
%         headers such as if2(10) are returned as 1xN double row vectors.
%         Sections present in the files (Fortran dims):
%           if2(10) xf2(13,76,2)     foF2
%           ifm3(10) xfm3(9,49,2)    M(3000)F2
%           ie(10) xe(9,22,2)        foE
%           iesu(10) xesu(5,55,2)    foEs upper decile
%           ies(10) xes(7,61,2)      foEs median
%           iesl(10) xesl(5,55,2)    foEs lower decile
%           ihpo1(10) xhpo1(13,29,2) h'F,F2 (ITS-78)
%           ihpo2(10) xhpo2(9,55,2)  h'F,F2 (P.1239)
%           ihp(10) xhp(9,37,2)      h'F
%           fakp(29,16,6) fakabp(2,6) dud(5,12,5) fam(14,12)  atmospheric noise
%           sys1(9,16,6) sys2(9,16,6) perr(9,4,6)             system/error terms
%           f2d(16,6,6) pko(8,7,6) slp(8,7,6) ccr(8,7,6)      MUF statistics
%   Errors:
%     p372:readLines, p372:readNamedBlock
%
%   Example:
%     C = p372.readCoeff(p372.defaultDataDir(), 1, {'fakp'});
%     isequal(permute(C.fakp, [3 2 1]), p372.readFamDud(p372.defaultDataDir(), 1).fakp)
%
%   See also p372.readFamDud, p372.readNamedBlock.
fname = fullfile(dataDir, sprintf('COEFF%02dW.txt', month));
lines = p372.readLines(fname);
C.name = strtrim(lines{1});
C.month = month;
C.file = fname;

% Find every header line of the form name(d1,d2,...)
hdr = regexp(strtrim(lines), '^([a-z0-9]+)\(([0-9,]+)\)$', 'tokens', 'once');
isHdr = ~cellfun(@isempty, hdr);
hidx = find(isHdr);
for n = 1:numel(hidx)
    tok = hdr{hidx(n)};
    nm = tok{1};
    dims = sscanf(tok{2}, '%d,').';
    if nargin >= 3 && ~any(strcmp(nm, names))
        continue;
    end
    A = p372.readNamedBlock(lines, lines{hidx(n)}, prod(dims));
    if numel(dims) == 1
        C.(nm) = A.';
    else
        C.(nm) = reshape(A, dims);
    end
end
end
