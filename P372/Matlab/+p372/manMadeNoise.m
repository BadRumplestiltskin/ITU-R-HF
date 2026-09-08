function [FaM, DuM, DlM] = manMadeNoise(manMadeNoise, frequency)
%MANMADENOISE Man-made radio noise (P.372 section 5, Table 2 / Figure 2).
%   [FaM, DuM, DlM] = p372.manMadeNoise(category, frequency)
%
%   Inputs:
%     category  - environmental category code or override:
%                   0  City            FaM = 76.8 - 27.7 log10(f)  Du 11.0 Dl 6.7
%                   1  Residential     FaM = 72.5 - 27.7 log10(f)  Du 10.6 Dl 5.3
%                   2  Rural           FaM = 67.2 - 27.7 log10(f)  Du  9.2 Dl 4.6
%                   3  Quiet rural     FaM = 53.6 - 28.6 log10(f)  Du  9.2 Dl 4.6
%                   4  Noisy (*)       FaM = 83.2 - 37.5 log10(f)  Du 11.0 Dl 6.7
%                   5  Quiet (*)       FaM = 65.2 - 29.1 log10(f)  Du  9.2 Dl 4.6
%                   other value V     FaM = 204 - V (frequency independent)
%                 (*) categories 4 and 5 are not in P.372; they are kept
%                 from the ITU software for compatibility with ITURHFProp.
%                 The comparison is exact (==), as in the C code.
%     frequency - MHz, scalar.
%   Outputs:
%     FaM - median man-made noise figure, dB above kT0b.
%     DuM - upper decile deviation, dB.
%     DlM - lower decile deviation, dB.
%
%   Known upstream quirk (reproduced deliberately): for the "other value"
%   branch the C code assigns DlM = 11.0 and DuM = 6.7, the reverse of the
%   City category it claims to use. It is reproduced so that outputs match
%   the reference implementation; see README.md "Fidelity".
%
%   Note that p372.noise treats a NEGATIVE category as a request to bypass
%   the whole calculation, so the "other value" branch is reached only
%   for positive values outside 0..5.
%
%   Reference: Noise.c ManMadeNoise().
%
%   See also p372.galacticNoise, p372.noise.
switch manMadeNoise
    case 0.0    % CITY
        c = 76.8; d = 27.7; DuM = 11.0; DlM = 6.7;
    case 1.0    % RESIDENTIAL
        c = 72.5; d = 27.7; DuM = 10.6; DlM = 5.3;
    case 2.0    % RURAL
        c = 67.2; d = 27.7; DuM = 9.2;  DlM = 4.6;
    case 3.0    % QUIETRURAL
        c = 53.6; d = 28.6; DuM = 9.2;  DlM = 4.6;
    case 5.0    % QUIET (not in P.372)
        c = 65.2; d = 29.1; DuM = 9.2;  DlM = 4.6;
    case 4.0    % NOISY (not in P.372)
        c = 83.2; d = 37.5; DuM = 11.0; DlM = 6.7;
    otherwise   % user input value
        c = -manMadeNoise + 204.0; d = 0.0;
        DlM = 11.0; DuM = 6.7;     % sic, see NOTE
end
FaM = c - d * log10(frequency);
end
