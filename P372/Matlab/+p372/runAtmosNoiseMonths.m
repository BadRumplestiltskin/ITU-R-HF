function outRoot = runAtmosNoiseMonths(dataDir, outRoot, months, hours)
%RUNATMOSNOISEMONTHS Generate the data files behind P.372-14 Figures 13-36.
%   outRoot = p372.runAtmosNoiseMonths()
%   outRoot = p372.runAtmosNoiseMonths(dataDir, outRoot)
%   outRoot = p372.runAtmosNoiseMonths(dataDir, outRoot, months, hours)
%
%   Port of RunAtmosNoiseMonths() in ITURNoise.c (ITURNoise.exe "Mode 2").
%   For each selected month and local hour it writes three CSV files:
%     a/csv/a_<m>m<h>h.csv  Fam at 1 MHz on a 1-degree world grid
%                           (columns month,hour,freq,latitude,longitude,FaA;
%                           latitude outer loop -90..90, longitude inner
%                           -180..180, 65341 rows)
%     b/csv/b_<m>m<h>h.csv  Fam versus frequency (41 frequencies 0.01..30
%                           MHz) for Fam(1 MHz) = 5, 10, 20, ..., 100 dB
%     c/csv/c_<m>m<h>h.csv  FaA, DuA, DlA, sigmaFaA, sigmaDuA, sigmaDlA,
%                           V_d, sigma_V_d versus frequency
%   b) and c) are evaluated at Boulder, Colorado (40.015744 N, 105.27932 W)
%   as in the reference program. Numbers are written with %5.4f so the
%   files are byte-identical to the C output (verified by the tests).
%
%   Inputs:
%     dataDir - coefficient folder; default p372.defaultDataDir().
%     outRoot - output root; default fullfile(pwd, 'P372_figures').
%     months  - vector, default [1 4 7 10] (central month of each season).
%     hours   - vector of local hours, default 0:4:20.
%   Outputs:
%     outRoot - the output root actually used.
%
%   Full default run: 72 files, about 40 s in Octave, less in MATLAB.
%
%   Example:
%     root = p372.runAtmosNoiseMonths();
%     p372.plots.makeP372Figs(root);
%
%   See also p372.plots.makeP372Figs, p372.atmosphericNoiseLT, p372.findVd.
if nargin < 1 || isempty(dataDir), dataDir = p372.defaultDataDir(); end
if nargin < 2 || isempty(outRoot), outRoot = fullfile(pwd, 'P372_figures'); end
if nargin < 3 || isempty(months), months = [1 4 7 10]; end
if nargin < 4 || isempty(hours), hours = 0:4:20; end

D2R = p372.D2R(); R2D = p372.R2D();
adir = fullfile(outRoot, 'a', 'csv');
bdir = fullfile(outRoot, 'b', 'csv');
cdir = fullfile(outRoot, 'c', 'csv');
cellfun(@(d) mkdirIfNeeded(d), {adir, bdir, cdir});

[cV, dV] = p372.readVdCoeffs(dataDir);

f_log = [0.01, 0.015, 0.02, 0.025, 0.03, 0.035, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, ...
         0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, ...
         1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, ...
         10.0, 15.0, 20.0, 25.0, 30.0];

fprintf('************************************************************\n');
fprintf('                    ITU-R Study Group 3\n');
fprintf('************************************************************\n');
fprintf('         p372: P372 Figure Data Generator\n');
fprintf('         Creation: %s\n', datestr(now));
fprintf('************************************************************\n');

% ---------------- a) Figures: Fam (dB above kT0B at 1 MHz) --------------
fprintf('\nBegin Data Generation for a) Figures\n');
freq = 1.0;
[LNG, LAT] = meshgrid(-180:180, -90:90);      % rows = latitude, cols = longitude
rlng = LNG * D2R; rlat = LAT * D2R;
for m = months
    coeff = p372.readFamDud(dataDir, m);
    for h = hours
        fname = fullfile(adir, sprintf('a_%dm%dh.csv', m, h));
        fprintf('Writing file %s\n', fname);
        FamS = p372.atmosphericNoiseLT(coeff, h, rlng, rlat, freq);
        % C order: latitude outer loop, longitude inner -> row-major of the grid
        rows = [repmat(m, numel(LAT), 1), repmat(h, numel(LAT), 1), ...
                repmat(freq, numel(LAT), 1), ...
                reshape((rlat * R2D).', [], 1), reshape((rlng * R2D).', [], 1), ...
                reshape(FamS.FA.', [], 1)];
        fid = fopen(fname, 'w');
        fprintf(fid, 'month,hour,freq,latitude,longitude,FaA\n');
        fprintf(fid, '%d, %d, %5.4f, %5.4f, %5.4f, %5.4f\n', rows.');
        fclose(fid);
    end
end
fprintf('p372: Data for a) Figures Complete\n');

% Location for the b) and c) figures: Boulder, Colorado
rlat = 40.015744 * D2R;
rlng = -105.27932 * D2R;

% ---------------- b) Figures: variation of noise with frequency ----------
fprintf('\nBegin Data Generation for b) Figures\n');
Fam1MHz = [5, 10:10:100];
for m = months
    coeff = p372.readFamDud(dataDir, m);
    for h = hours
        fname = fullfile(bdir, sprintf('b_%dm%dh.csv', m, h));
        fprintf('Writing file %s\n', fname);
        tb = fix(h / 4);
        famRow = tb + 1 + 6 * (rlat < 0);
        fid = fopen(fname, 'w');
        fprintf(fid, 'month,hour,freq,latitude,longitude,Fam5,Fam10,Fam20,Fam30,Fam40,Fam50,Fam60,Fam70,Fam80,Fam90,Fam100\n');
        for f = f_log
            Fam = zeros(1, 11);
            for k = 1:11
                Fam(k) = p372.famFrequencyVariation(coeff, famRow, Fam1MHz(k), f);
            end
            fprintf(fid, ['%d, %d, %5.4f, %5.4f, %5.4f' repmat(', %5.4f', 1, 11) '\n'], ...
                    m, h, f, rlat * R2D, rlng * R2D, Fam);
        end
        fclose(fid);
    end
end
fprintf('p372: Data for b) Figures Complete\n');

% ---------------- c) Figures: noise variability and character ------------
fprintf('\nBegin Data Generation for c) Figures\n');
for m = months
    s = fix((m - 1) / 3) + 1;                  % season index
    coeff = p372.readFamDud(dataDir, m);
    for h = hours
        tb = fix(h / 4) + 1;
        fname = fullfile(cdir, sprintf('c_%dm%dh.csv', m, h));
        fprintf('Writing file %s\n', fname);
        fid = fopen(fname, 'w');
        fprintf(fid, 'month,hour,freq,latitude,longitude,FaA,DuA,DlA,sigmaFaA,sigmaDuA,sigmaDlA,V_d,sigma_V_d\n');
        for f = f_log
            FamS = p372.atmosphericNoiseLT(coeff, h, rlng, rlat, f);
            [V_d, sigma_V_d] = p372.findVd(f, squeeze(cV(s, tb, :)), squeeze(dV(s, tb, :)));
            fprintf(fid, '%d, %d, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f\n', ...
                    m, h, f, rlat * R2D, rlng * R2D, FamS.FA, FamS.Du, FamS.Dl, ...
                    FamS.SigmaFam, FamS.SigmaDu, FamS.SigmaDl, V_d, sigma_V_d);
        end
        fclose(fid);
    end
end
fprintf('p372: Data for c) Figures Complete\n');
fprintf('\n*** End p372 Data Generation ***\n');
end

function mkdirIfNeeded(d)
if ~exist(d, 'dir')
    mkdir(d);
end
end
