function FA = famFrequencyVariation(coeff, famRow, Fam1MHz, frequency)
%FAMFREQUENCYVARIATION Scale atmospheric noise from 1 MHz to another frequency.
%   FA = p372.famFrequencyVariation(coeff, famRow, Fam1MHz, frequency)
%
%   Implements the frequency dependence of Fam from NBS Technical Note 318
%   (page 5): two polynomials in u = (8*2^log10(f) - 11)/4, evaluated once
%   at u = -0.75 (the 1 MHz anchor) to fix the constant cz, then at the
%   requested frequency. This is the polynomial fragment of
%   GetFamParameters() that ITURNoise.c also inlines to produce the b)
%   figures (Fam versus frequency for Fam(1 MHz) = 10..100 dB).
%
%   Inputs:
%     coeff     - struct from p372.readFamDud (only coeff.fam is used).
%     famRow    - 1-based row(s) of coeff.fam: timeBlock + 1, plus 6 for
%                 the southern hemisphere. Scalar or column vector.
%     Fam1MHz   - atmospheric noise at 1 MHz, dB above kT0b. Scalar or
%                 same size as famRow.
%     frequency - scalar, MHz.
%   Outputs:
%     FA - Fam at the requested frequency, dB above kT0b, column vector.
%
%   Example (one point of the P.372 b) curves):
%     coeff = p372.readFamDud([], 1);
%     FA = p372.famFrequencyVariation(coeff, 1, 50, 10)   % 50 dB at 1 MHz -> 10 MHz
%
%   See also p372.getFamParameters, p372.runAtmosNoiseMonths.
u = [-0.75, (8.0 * 2.0 ^ log10(frequency) - 11.0) / 4.0];
F = coeff.fam(famRow, :);
cz = [];
for k = 1:2
    pz = u(k) * F(:, 1) + F(:, 2);
    px = u(k) * F(:, 8) + F(:, 9);
    for j = 3:7
        pz = u(k) * pz + F(:, j);
        px = u(k) * px + F(:, j + 7);
    end
    if k == 1
        cz = Fam1MHz(:) .* (2.0 - pz) - px;
    end
end
FA = cz .* pz + px;
end
