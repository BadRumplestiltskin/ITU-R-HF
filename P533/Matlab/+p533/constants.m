function c = constants()
%CONSTANTS Physical constants, limits and index conventions of the P.533 port.
%   c = p533.constants() returns a struct. Angles are radians throughout the
%   package; these are the exact values, not the truncated ones of the ITU
%   C code (see docs/P533_DEVIATIONS.md).
%     R0        Earth radius 6371 km (P.533-14 section 4)
%     RMAG      6371.2 km, magnetic field scale height base (P.1239 eq. 8)
%     C_KM_S    speed of light km/s (eq. 47)
%     HR_E      E-layer mirror height 110 km; HPEN 90 km; HREF_LONG 300 km
%     MINELE    minimum elevation 3 deg (radians)
%     MAXSSN    160 (sections 3.4, 5.1, 5.2)
%     MAXF2MDS 6, MAXEMDS 3
%     cp.T1k .. cp.R1k  control point indices 1..5 into path.CP
%     LZ 8.72 dB, LY -0.14 dB (sections 5.2, 5.3.3)
%     POLE_LH   [78.5 -68.2] deg geomagnetic pole for Table 2 (section 5.2)
%     POLE_1239 [78.3 -69.0] deg pole of P.1239 section 5 (P.842 Table 2 note)
%     TINYDB    -307, sentinel for "no value" in dB fields (as in the C code)
c.R0 = 6371.0;
c.RMAG = 6371.2;
c.C_KM_S = 299792.458;
c.HR_E = 110.0;
c.HPEN = 90.0;
c.HREF_LONG = 300.0;
c.MINELE = 3.0 * pi / 180;
c.MAXSSN = 160;
c.MAXF2MDS = 6;
c.MAXEMDS = 3;
c.cp = struct('T1k', 1, 'Td02', 2, 'MP', 3, 'Rd02', 4, 'R1k', 5);
c.LZ = 8.72;
c.LY = -0.14;
c.POLE_LH = [78.5, -68.2] * pi / 180;
c.POLE_1239 = [78.3, -69.0] * pi / 180;
c.TINYDB = -307;
c.NOLOWESTMODE = 99;
c.SEASON = struct('WINTER', 1, 'EQUINOX', 2, 'SUMMER', 3);
c.MM = struct('CITY', 0, 'RESIDENTIAL', 1, 'RURAL', 2, 'QUIETRURAL', 3, 'NOISY', 4, 'QUIET', 5);
c.MOD = struct('ANALOG', 0, 'DIGITAL', 1);
c.SORL = struct('SHORT', 0, 'LONG', 1);
end
