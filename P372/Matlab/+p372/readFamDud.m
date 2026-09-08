function coeff = readFamDud(dataDir, month)
%READFAMDUD Read the atmospheric-noise coefficients for one month.
%   coeff = p372.readFamDud(dataDir, month)
%
%   Reads the four arrays that the P.372 atmospheric noise model needs from
%   the file COEFFmmW.txt (mm = month, two digits) in dataDir. These are
%   the CCIR Report 322 / NBS Technical Note 318 numerical representations
%   of the atmospheric noise maps, as harmonised by P. Suessmann for the
%   ITU-R HF prediction software.
%
%   Inputs:
%     dataDir - char, folder containing COEFF01W.txt .. COEFF12W.txt. Pass
%               [] to use p372.defaultDataDir().
%     month   - integer 1..12 (January = 1). Note the C function
%               ReadFamDud() takes 0..11.
%   Outputs:
%     coeff   - struct with fields
%       fakp   (6,16,29) double  Fourier coefficients of the 1 MHz noise
%                                map, indexed (timeBlock, k, j)
%       fakabp (6,2)     double  linear normalisation (offset, slope)
%                                of the latitude series per time block
%       dud    (5,12,5)  double  polynomial coefficients for Du, Dl,
%                                sigma_Du, sigma_Dl, sigma_Fam indexed
%                                (statistic, timeBlock[+6 south], power)
%       fam    (12,14)   double  frequency-variation polynomials indexed
%                                (timeBlock[+6 south], coefficient)
%       month, file              provenance
%     Array index order is that of the C code (first index = time block),
%     made 1-based. For the same arrays in Fortran order use p372.readCoeff.
%   Errors:
%     p372:readFamDud       month out of range
%     p372:readLines        file not found
%     p372:readNamedBlock   malformed file
%
%   Example:
%     coeff = p372.readFamDud(p372.defaultDataDir(), 7);   % July
%     n = p372.noise(coeff, 2, 12, -105 * p372.D2R(), 40 * p372.D2R(), 5);
%
%   Reference: Noise.c ReadFamDud(); docs/DATA_FORMATS.md.
%
%   See also p372.readCoeff, p372.noise, p372.getFamParameters.
if month < 1 || month > 12
    error('p372:readFamDud', 'Month (%d) out of range (1 to 12).', month);
end
fname = fullfile(dataDir, sprintf('COEFF%02dW.txt', month));
lines = p372.readLines(fname);

% fakp(29,16,6): C reshapes A[16*29*i + 29*j + k] -> fakp[i][j][k]
A = p372.readNamedBlock(lines, 'fakp(29,16,6)', 29 * 16 * 6);
coeff.fakp = permute(reshape(A, [29, 16, 6]), [3 2 1]);

% fakabp(2,6): A[2*j + k] -> fakabp[j][k]
A = p372.readNamedBlock(lines, 'fakabp(2,6)', 2 * 6);
coeff.fakabp = reshape(A, [2, 6]).';

% dud(5,12,5): A[5*12*i + 5*j + k] -> dud[i][j][k]
A = p372.readNamedBlock(lines, 'dud(5,12,5)', 5 * 12 * 5);
coeff.dud = permute(reshape(A, [5, 12, 5]), [3 2 1]);

% fam(14,12): A[14*j + k] -> fam[j][k]
A = p372.readNamedBlock(lines, 'fam(14,12)', 14 * 12);
coeff.fam = reshape(A, [14, 12]).';

coeff.month = month;
coeff.file = fname;
end
