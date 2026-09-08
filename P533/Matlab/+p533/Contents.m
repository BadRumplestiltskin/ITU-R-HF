% P533 - MATLAB implementation of Recommendation ITU-R P.533-14
% Version 1.0.0  8-Sep-2026
%
% Written from the Recommendation text (docs/P533_EQUATIONS.md); the ITU
% C software is a tolerance reference only (docs/P533_DEVIATIONS.md).
% Angles are radians inside the package; iturhfprop and the app take degrees.
%
% Running a prediction
%   newPath                 - Default path structure (inputs, outputs, data slots)
%   loadData                - Load ionospheric maps, P.1239 deciles and P.372 coefficients
%   run                     - The P533 sequence: validate, Parts 1-3
%   validatePath            - Input range checks
%   initializePath          - Distance, bearings, first control points, season
%
% Part 1: frequency availability (sections 2-4)
%   mufBasic                - Basic MUFs of the E and F2 modes and of the path
%   calcB, calcCd, calcdmax, calcF2DMUF - eqs (3)-(6)
%   mufVariability          - MUF deciles and probability of support, eqs (9)-(10)
%   findfoF2var             - P.1239 Tables 2 and 3 lookup
%   mufOperational          - Operational MUF, P.1240 Table 1
%   eLayerScreeningFrequency - fs of the F2 modes, eqs (11)-(12)
%   mirrorReflectionHeight  - F2 mirror height, eqs (14)-(16)
%   elevationAngle, incidenceAngle - eqs (13), (12)
%
% Part 2: field strength and receiver power (sections 5-6)
%   medianSkywaveFieldStrengthShort - modes up to 9000 km, eqs (17)-(28)
%   absorptionLoss          - Li at the penetration points, eqs (20)-(23)
%   absorptionFactor, penetrationFactor, diurnalAbsorptionExponent - Figures 1-3
%   findLh                  - Table 2 auroral losses
%   medianSkywaveFieldStrengthLong - paths beyond 7000 km, eqs (29)-(41)
%   winterAnomaly           - Table 5
%   between7000kmand9000km  - eq. (42)
%   medianAvailableReceiverPower - eqs (43)-(44)
%   antennaGain, antennaGain08 - pattern interpolation, best gain 0-8 deg
%
% Part 3: system performance (sections 7-10, P.842, Attachment 1)
%   circuitReliability      - SNR, deciles, SNRXX, BCR, multimode interference, scattering
%
% Ionosphere, Sun and Earth
%   pointParameters         - Vectorised foF2, M(3000)F2, foE, dip, fH, solar geometry
%   calculateCPParameters   - Same for one control point
%   ionosphericParameters   - Map interpolation (P.1144)
%   findfoE                 - P.1239 section 4
%   solarParameters         - Declination, hour angle, zenith, sunrise/sunset
%   magfit                  - P.1239 section 2 field model
%   greatCircleDistance, greatCirclePoint, bearing, geomagneticCoords, pathPoint
%   seasonOf                - Season conventions of P.533 and P.1239
%
% Data
%   readIonParameters       - ionosNN.bin maps
%   readP1239               - P1239-3 Decile Factors.txt
%   readAntenna, isotropicPattern - VOACAP Type 11/13/14 patterns
%   readInputConfiguration  - ITURHFProp .in files
%   defaultDataDir, constants, tables, version
%
% Reports
%   report.columns, report.write - ITURHFProp-compatible reports and CSV
%
% See also iturhfprop (driver), HFPropApp (apps folder), tests/run_all_tests.
