function FamS = atmosphericNoiseLT(coeff, lrxmt, lng, lat, frequency)
%ATMOSPHERICNOISELT Full atmospheric noise statistics at a local mean time.
%   FamS = p372.atmosphericNoiseLT(coeff, lrxmt, lng, lat, frequency)
%
%   Same time-block interpolation as p372.atmosphericNoise, but the input
%   is the receiver local mean time rather than UTC, and all six
%   statistics are returned. This is the routine used to generate the
%   Recommendation's Figures 13-36 (see p372.runAtmosNoiseMonths).
%
%   Inputs:
%     coeff     - struct from p372.readFamDud.
%     lrxmt     - local mean time, integer hour 0..23 (scalar). Values
%                 -24..-1 and 24..47 are rolled into range once.
%     lng, lat  - radians; arrays of identical size are evaluated together
%                 (used for the 181x361 world grid of the a) figures).
%     frequency - MHz (scalar).
%   Outputs:
%     FamS - struct with fields, each the size of lng:
%       FA, Du, Dl, SigmaFam, SigmaDu, SigmaDl   (dB)
%       tmblk = 99 (marker copied from the C code; not meaningful)
%
%   Example (Fam at 1 MHz over the globe at 12 h local time in July):
%     coeff = p372.readFamDud([], 7);
%     [LNG, LAT] = meshgrid((-180:180) * p372.D2R(), (-90:90) * p372.D2R());
%     FamS = p372.atmosphericNoiseLT(coeff, 12, LNG, LAT, 1.0);
%     contour(-180:180, -90:90, FamS.FA, 5:5:115)
%
%   Reference: Noise.c AtmosphericNoise_LT().
%
%   See also p372.atmosphericNoise, p372.getFamParameters, p372.runAtmosNoiseMonths.
if lrxmt < 0
    lrxmt = lrxmt + 24;
elseif lrxmt > 23
    lrxmt = lrxmt - 24;
end
tbNow = mod(fix(lrxmt / 4), 6);
tbAdj = mod(tbNow + 1, 6);
FSn = p372.getFamParameters(coeff, tbNow, lng, lat, frequency);
FSa = p372.getFamParameters(coeff, tbAdj, lng, lat, frequency);
slp = mod(lrxmt, 4) / 4;
FamS.tmblk = 99;
FamS.FA = p372.interpdB(FSn.FA, FSa.FA, slp);
FamS.Du = p372.interpdB(FSn.Du, FSa.Du, slp);
FamS.Dl = p372.interpdB(FSn.Dl, FSa.Dl, slp);
FamS.SigmaDl = p372.interpdB(FSn.SigmaDl, FSa.SigmaDl, slp);
FamS.SigmaDu = p372.interpdB(FSn.SigmaDu, FSa.SigmaDu, slp);
FamS.SigmaFam = p372.interpdB(FSn.SigmaFam, FSa.SigmaFam, slp);
end
