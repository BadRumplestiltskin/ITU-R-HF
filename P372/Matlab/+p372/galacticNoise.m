function [FaG, DuG, DlG] = galacticNoise(frequency)
%GALACTICNOISE Galactic radio noise (P.372 section 6, Table 1 / Figure 10).
%   [FaG, DuG, DlG] = p372.galacticNoise(frequency)
%
%   Inputs:
%     frequency - MHz, scalar or array.
%   Outputs:
%     FaG - median galactic noise figure, 52 - 23*log10(f), dB above kT0b.
%     DuG - upper decile deviation, 2 dB (constant).
%     DlG - lower decile deviation, 2 dB (constant).
%
%   The 2 dB decile corresponds to the 1.56 dB standard deviation used for
%   the galactic term in the combination formula of p372.noise.
%
%   Reference: Noise.c GalacticNoise().
%
%   See also p372.manMadeNoise, p372.noise.
c = 52.0; d = 23.0;
FaG = c - d * log10(frequency);
DuG = 2.0;
DlG = 2.0;
end
