function C = columns(path)
%COLUMNS Report column definitions in the order and format of ITURHFProp's Report.c.
%   C = p533.report.columns(path) returns a struct array with fields
%   flag (RPT_* name), name (RFC 4180 header), desc (Data Format text),
%   fmt (C printf format for the text report), fmtcsv (for CSV) and
%   value (function of path). RPT_ALL selects every flag.
c = p533.constants();
d2 = '% 9.2f'; d3 = '% 9.3f'; d4 = '% 9.4f';
n = 0; C = struct('flag', {}, 'name', {}, 'desc', {}, 'fmt', {}, 'fmtcsv', {}, 'value', {});
    function add(flag, name, desc, fmt, fmtcsv, fn)
        n = n + 1; C(n) = struct('flag', flag, 'name', name, 'desc', desc, 'fmt', fmt, 'fmtcsv', fmtcsv, 'value', fn);
    end
R2D = 180 / pi;
add('RPT_RXLOCATION', 'rxlat', 'Receiver latitude (deg)', d4, '%.4f', @(p) p.L_rx.lat * R2D);
add('RPT_RXLOCATION', 'rxlng', 'Receiver longitude (deg)', d4, '%.4f', @(p) p.L_rx.lng * R2D);
add('RPT_D', 'distance', 'D - Path distance (km)', d2, '%.2f', @(p) p.distance);
add('RPT_DMAX', 'dmax', 'dmax - Path maximum hop distance (km)', d2, '%.2f', @(p) p.dmax);
add('RPT_DMAX', 'ptick', 'ptick - Slant Path distance (km)', d2, '%.2f', @(p) slant(p));
add('RPT_ELE', 'ele', 'ele - Path minimum Rx elevation angle (deg)', d2, '%.2f', @(p) p.ele * R2D);
add('RPT_BMUF', 'BMUF', 'BMUF - Path basic MUF (MHz)', d2, '%.2f', @(p) p.BMUF);
add('RPT_BMUFD', 'MUF50', 'MUF50 - 50% Path basic MUF (MHz)', d2, '%.2f', @(p) p.MUF50);
add('RPT_BMUFD', 'MUF90', 'MUF90 - 90% Path basic MUF (MHz)', d2, '%.2f', @(p) p.MUF90);
add('RPT_BMUFD', 'MUF10', 'MUF10 - 10% Path basic MUF (MHz)', d2, '%.2f', @(p) p.MUF10);
add('RPT_OPMUF', 'OPMUF', 'OPMUF - Operation MUF (MHz)', d2, '%.2f', @(p) p.OPMUF);
add('RPT_OPMUFD', 'OPMUF90', 'OPMUF90 - 90% Operation MUF (MHz)', d2, '%.2f', @(p) p.OPMUF90);
add('RPT_OPMUFD', 'OPMUF10', 'OPMUF10 - 10% Operation MUF (MHz)', d2, '%.2f', @(p) p.OPMUF10);
add('RPT_N0_F2', 'n0_F2', 'Lowest order mode for the F2 layer', '%9s', '%s', @(p) modeName(p.n0_F2, 'F2'));
add('RPT_N0_E', 'n0_E', 'Lowest order mode for the E layer', '%9s', '%s', @(p) modeName(p.n0_E, 'E'));
add('RPT_E', 'E', 'E - Path Field Strength (dB(1uV/m))', d2, '%.2f', @(p) p.Ep);
add('RPT_PR', 'Pr', 'Pr - Median receiver power (dB)', d2, '%.2f', @(p) p.Pr);
add('RPT_GRW', 'Grw', 'Grw - Receive Antenna Gain (dbi)', d2, '%.2f', @(p) p.Grw);
add('RPT_NOISESOURCES', 'FaA', 'FaA - Atmospheric noise (dB)', d2, '%.2f', @(p) p.noise.FaA);
add('RPT_NOISESOURCES', 'FaM', 'FaM - Man-made noise (dB)', d2, '%.2f', @(p) p.noise.FaM);
add('RPT_NOISESOURCES', 'FaG', 'FaG - Galactic noise (dB)', d2, '%.2f', @(p) p.noise.FaG);
add('RPT_NOISESOURCESD', 'DuA', 'DuA - Upper decile deviation of atmospheric noise (dB)', d2, '%.2f', @(p) p.noise.DuA);
add('RPT_NOISESOURCESD', 'DlA', 'DlA - Lower decile deviation of atmospheric noise (dB)', d2, '%.2f', @(p) p.noise.DlA);
add('RPT_NOISESOURCESD', 'DuM', 'DuM - Upper decile deviation of man-made noise (dB)', d2, '%.2f', @(p) p.noise.DuM);
add('RPT_NOISESOURCESD', 'DlM', 'DlM - Lower decile deviation of man-made noise (dB)', d2, '%.2f', @(p) p.noise.DlM);
add('RPT_NOISESOURCESD', 'DuG', 'DuG - Upper decile deviation of galactic noise (dB)', d2, '%.2f', @(p) p.noise.DuG);
add('RPT_NOISESOURCESD', 'DlG', 'DlG - Lower decile deviation of galactic noise (dB)', d2, '%.2f', @(p) p.noise.DlG);
add('RPT_NOISETOTALD', 'DuT', 'DuT - Upper decile deviation of total noise (dB)', d2, '%.2f', @(p) p.noise.DuT);
add('RPT_NOISETOTALD', 'DlT', 'DlT - Lower decile deviation of total noise (dB)', d2, '%.2f', @(p) p.noise.DlT);
add('RPT_NOISETOTAL', 'FamT', 'FamT - Total noise (dB)', d2, '%.2f', @(p) p.noise.FamT);
add('RPT_SNR', 'SNR', 'SNR - Median signal-to-noise ratio (dB)', d2, '%.2f', @(p) p.SNR);
add('RPT_SNRD', 'DuSN', 'DuSN - Upper decile deviation of signal-to-noise ratio (dB)', d2, '%.2f', @(p) p.DuSN);
add('RPT_SNRD', 'DlSN', 'DlSN - Lower decile deviation of signal-to-noise ratio (dB)', d2, '%.2f', @(p) p.DlSN);
add('RPT_SNRXX', 'SNRXX', sprintf('SNRXXp - Signal-to-noise ratio at %d%% of month', path.SNRXXp), d2, '%.2f', @(p) p.SNRXX);
add('RPT_SIR', 'SIR', 'SIR - Signal-to-interference ratio (dB)', d2, '%.2f', @(p) p.SIR);
add('RPT_SIRD', 'DuSI', 'DuSI - Upper decile deviation of signal-to-interference ratio (dB)', d2, '%.2f', @(p) p.DuSI);
add('RPT_SIRD', 'DlSI', 'DlSI - Lower decile deviation of signal-to-interference ratio (dB)', d2, '%.2f', @(p) p.DlSI);
add('RPT_BCR', 'BCR', 'BCR - Basic circuit reliability (%)', d2, '%.2f', @(p) p.BCR);
add('RPT_OCR', 'OCR', 'OCR - Overall circuit reliability not considering scattering (%)', d2, '%.2f', @(p) p.OCR);
add('RPT_OCRS', 'OCRs', 'OCRs - Overall circuit reliability considering scattering (%)', d2, '%.2f', @(p) p.OCRs);
add('RPT_OCRS', 'probocc', 'Probocc - Probability of scattering (%)', d2, '%.2f', @(p) 100 * p.probocc);
add('RPT_MIR', 'MIR', 'MIR - Multimode Interference (%)', d2, '%.2f', @(p) p.MIR);
add('RPT_DOMMODE', 'dommode', 'Dominant mode', '%9s', '%s', @(p) domMode(p, c));
add('RPT_ESL', 'Es', 'Short Path (<=7000 km) Field Strength (dB(1uV/m))', d2, '%.2f', @(p) p.Es);
add('RPT_ESL', 'El', 'Long Path (>9000km) Field Strength (dB(1uV/m))', d2, '%.2f', @(p) p.El);
add('RPT_LONG', 'Gap', 'Gap - Long path focusing gain (dB)', d2, '%.2f', @(p) p.Gap);
add('RPT_LONG', 'E0', 'E0 - Long path free-space field strength (dB(1uV/m))', d2, '%.2f', @(p) p.E0);
add('RPT_LONG', 'fM', 'fM - Long path upper reference frequency (MHz)', d2, '%.2f', @(p) p.fM);
add('RPT_LONG', 'fL', 'fL - Long path lower reference frequency (MHz)', d2, '%.2f', @(p) p.fL);
add('RPT_LONG', 'Ly', 'Ly - Long path excess loss (dB)', d2, '%.2f', @(p) p.Ly);
end

function s = slant(p)
if p.distance > 7000 && ~isnan(p.ptickLong), s = p.ptickLong; else, s = p.ptick; end
end
function s = modeName(n0, layer)
if n0 == 99, s = 'NONE'; else, s = sprintf('%d%s', n0, layer); end
end
function s = domMode(p, c)
if isnan(p.DMidx), s = 'NONE';
elseif p.DMidx <= c.MAXEMDS, s = sprintf('%dE', p.Md_E(p.DMidx).hops);
else, s = sprintf('%dF2', p.Md_F2(p.DMidx - c.MAXEMDS).hops); end
end
