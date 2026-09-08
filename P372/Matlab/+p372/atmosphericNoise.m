function [FaA, DuA, DlA] = atmosphericNoise(coeff, hour, rlng, rlat, frequency)
%ATMOSPHERICNOISE Atmospheric radio noise at a UTC hour (P.372 section 4).
%   [FaA, DuA, DlA] = p372.atmosphericNoise(coeff, hour, rlng, rlat, frequency)
%
%   Converts the UTC hour to receiver local mean time, evaluates the noise
%   maps for the 4-hour time block containing that time and for the next
%   block, and interpolates between them linearly in power according to
%   the position of the local hour within the block. This follows the
%   REC533 routines GENFAM/GENOIS1/ANOIS1/NOISY as ported in Noise.c;
%   it is not itself specified in the Recommendation but produces the
%   values the Recommendation's figures represent.
%
%   Inputs:
%     coeff     - struct from p372.readFamDud for the month of interest.
%     hour      - integer UTC hour 0..23.
%     rlng      - receiver longitude, radians, east positive (scalar).
%     rlat      - receiver latitude, radians, north positive (scalar).
%     frequency - MHz, 0.01..30 (scalar).
%   Outputs:
%     FaA - median atmospheric noise figure, dB above kT0b.
%     DuA - upper decile deviation, dB.
%     DlA - lower decile deviation, dB.
%
%   Implementation notes (kept for equivalence with the C code):
%     * local time = hour + int(rlng / (15 deg)), truncated toward zero
%       with the truncated constant p372.D2R, then rolled once by 24 h;
%     * the "adjacent" block is always the following one, and the blend
%       factor is mod(localHour, 4) / 4.
%
%   Example:
%     coeff = p372.readFamDud([], 1);
%     [FaA, DuA, DlA] = p372.atmosphericNoise(coeff, 13, 165*p372.D2R(), 40*p372.D2R(), 1)
%     % FaA = 60.733, DuA = 10.601, DlA = 8.278
%
%   Reference: P.372-14 section 4; Noise.c AtmosphericNoise().
%
%   See also p372.atmosphericNoiseLT, p372.getFamParameters, p372.noise.
lrxmt = hour + fix(rlng / (15.0 * p372.D2R()));
if lrxmt < 0
    lrxmt = lrxmt + 24;
elseif lrxmt > 23
    lrxmt = lrxmt - 24;
end
tbNow = mod(fix(lrxmt / 4), 6);
tbAdj = mod(tbNow + 1, 6);
FSn = p372.getFamParameters(coeff, tbNow, rlng, rlat, frequency);
FSa = p372.getFamParameters(coeff, tbAdj, rlng, rlat, frequency);
slp = mod(lrxmt, 4) / 4;
FaA = p372.interpdB(FSn.FA, FSa.FA, slp);
DuA = p372.interpdB(FSn.Du, FSa.Du, slp);
DlA = p372.interpdB(FSn.Dl, FSa.Dl, slp);
end
