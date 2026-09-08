% P372 - MATLAB implementation of Recommendation ITU-R P.372-14 (radio noise)
% Version 1.1.0 (engine 14.3)  8-Sep-2026
%
% Port of the ITU-R Study Group 3 reference C code (ITU-R-HF/P372).
% Angles passed to engine functions are radians; use p372.D2R to convert.
%
% Single point calculation
%   makeNoise             - Noise at one place, time and frequency (degrees in)
%   noise                 - Engine entry point: components and P.372 sec. 8 combination
%   iturNoise             - Command-line style front end (ITURNoise.exe arguments)
%   formatReport          - Text report identical to the C program output
%
% Noise components
%   atmosphericNoise      - Atmospheric noise at a UTC hour (median, deciles)
%   atmosphericNoiseLT    - Atmospheric noise at a local hour, full statistics
%   getFamParameters      - Statistics for one 4-hour time block (vectorised)
%   famFrequencyVariation - Scale Fam from 1 MHz to another frequency
%   galacticNoise         - Galactic noise (Table 1)
%   manMadeNoise          - Man-made noise by environmental category (Table 2)
%   interpdB              - Power-domain interpolation helper
%
% Coefficient data
%   readFamDud            - Read the four atmospheric-noise arrays for a month
%   readCoeff             - Read any array of a COEFFmmW.txt file (P.533 groundwork)
%   readVdCoeffs          - Read V_d / sigma_V_d polynomial coefficients
%   findVd                - Voltage deviation V_d versus frequency
%   readLines             - Text file to cell array of lines
%   readNamedBlock        - Numeric block following a named section header
%   defaultDataDir        - Folder of the coefficient files shipped with the package
%
% Recommendation figures
%   runAtmosNoiseMonths   - Write the CSV data behind Figures 13-36 (Mode 2)
%   plots.makeP372Figs    - Render all figures from the CSV data
%   plots.makeAFigure     - World contour map of Fam at 1 MHz
%   plots.makeBFigure     - Fam versus frequency curves
%   plots.makeCFigure     - Deciles and standard deviations versus frequency
%   plots.parseFilename   - Season / time labels from a data file name
%   plots.bcAxes          - Shared log-frequency axis style
%
% Constants
%   D2R, R2D              - Truncated conversion factors of the C code
%   version               - Engine version string
%
% See also P372NoiseApp (apps folder), tests/run_all_tests.
