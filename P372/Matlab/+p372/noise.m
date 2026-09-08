function n = noise(coeff, manMadeNoise, hour, rlng, rlat, frequency)
%NOISE Atmospheric, man-made, galactic and combined radio noise (P.372-14).
%   n = p372.noise(coeff, manMadeNoise, hour, rlng, rlat, frequency)
%
%   Main entry point of the engine. Computes the three noise components
%   and combines them according to P.372 section 8 ("The combination of
%   noises from several sources") assuming log-normal distributions.
%   The combination is evaluated twice, once with the upper and once with
%   the lower decile deviations; the total median is the lower of the
%   two (worst-case noise), as in the C code.
%
%   Inputs:
%     coeff        - struct from p372.readFamDud for the month of interest.
%                    Ignored (may be []) when manMadeNoise is negative.
%     manMadeNoise - man-made noise category 0..5 (see p372.manMadeNoise),
%                    or a NEGATIVE value -X to bypass the model entirely
%                    and return FamT = X with all other outputs zero
%                    (ITURHFProp "user defined noise" feature).
%     hour         - UTC hour 0..23.
%     rlng, rlat   - receiver longitude/latitude in radians.
%     frequency    - MHz, 0.01..30.
%   Outputs:
%     n - struct with, in this order (matching the MakeNoise output vector):
%       FaA  DuA  DlA   atmospheric median and decile deviations (dB)
%       FaM  DuM  DlM   man-made
%       FaG  DuG  DlG   galactic
%       FamT DuT  DlT   total (combined) median and decile deviations
%       Noise figures are dB above kT0b (k = Boltzmann's constant,
%       T0 = 290 K, b = bandwidth).
%
%   Example (README example of the ITU software):
%     coeff = p372.readFamDud([], 1);
%     n = p372.noise(coeff, 0, 13, 165 * p372.D2R(), 40 * p372.D2R(), 1.0);
%     % n.FamT = 76.987, n.DuT = 10.940, n.DlT = 6.574
%
%   Reference: P.372-14 sections 4-8; Noise.c Noise().
%
%   See also p372.makeNoise, p372.atmosphericNoise, p372.manMadeNoise,
%   p372.galacticNoise, p372.readFamDud.
if manMadeNoise < 0
    n = struct('FaA', 0, 'DuA', 0, 'DlA', 0, ...
               'FaM', manMadeNoise, 'DuM', 0, 'DlM', 0, ...
               'FaG', 0, 'DuG', 0, 'DlG', 0, ...
               'FamT', -manMadeNoise, 'DuT', 0, 'DlT', 0);
    return;
end

[n.FaA, n.DuA, n.DlA] = p372.atmosphericNoise(coeff, hour, rlng, rlat, frequency);
[n.FaM, n.DuM, n.DlM] = p372.manMadeNoise(manMadeNoise, frequency);
[n.FaG, n.DuG, n.DlG] = p372.galacticNoise(frequency);

% Combination of noises from several sources, P.372 section 8.
[FamTu, n.DuT] = combine(n.FaA, n.DuA, n.FaG, n.DuG, n.FaM, n.DuM);
[FamTl, n.DlT] = combine(n.FaA, n.DlA, n.FaG, n.DlG, n.FaM, n.DlM);
n.FamT = min(FamTu, FamTl);            % worst-case noise

% Field order as in the C struct / MakeNoise output vector
n = orderfields(n, {'FaA', 'DuA', 'DlA', 'FaM', 'DuM', 'DlM', ...
                    'FaG', 'DuG', 'DlG', 'FamT', 'DuT', 'DlT'});
end

function [FamT, DT] = combine(FaA, DA, FaG, DG, FaM, DM)
sigmaA = DA / 1.282;
sigmaG = 1.56;
sigmaM = DM / 1.282;
c = 10.0 / log(10.0);
eA = exp(FaA / c + sigmaA ^ 2 / (2.0 * c ^ 2));
eG = exp(FaG / c + sigmaG ^ 2 / (2.0 * c ^ 2));
eM = exp(FaM / c + sigmaM ^ 2 / (2.0 * c ^ 2));
alphaT = eA + eG + eM;
betaT = eA ^ 2 * (exp((sigmaA / c) ^ 2) - 1.0) + ...
        eG ^ 2 * (exp((sigmaG / c) ^ 2) - 1.0) + ...
        eM ^ 2 * (exp((sigmaM / c) ^ 2) - 1.0);
gammaT = exp(FaA / c) + exp(FaG / c) + exp(FaM / c);
if DA > 12.0 || DG > 12.0 || DM > 12.0
    sigmaT = c * sqrt(2.0 * log(alphaT / gammaT));
else
    sigmaT = c * sqrt(log(1.0 + betaT / alphaT ^ 2));
end
FamT = c * (log(alphaT) - sigmaT ^ 2 / (2.0 * c ^ 2));
DT = 1.282 * sigmaT;
end
