function path = medianAvailableReceiverPower(path)
%MEDIANAVAILABLERECEIVERPOWER Median available receiver power Pr (section 6).
%   path = p533.medianAvailableReceiverPower(path)
%   D <= 7000: Prw = Ew + Grw - 20 log f - 107.2 per mode with Grw at the
%   mode elevation (43), Pr = 10 log sum 10^(Prw/10) (44); Ep = Es. The
%   dominant mode (largest Prw) gives path.Grw, path.ele, path.DMidx
%   (1..3 E, 4..9 F2). D > 9000: Pr from El with Grw the largest receive
%   gain over 0..8 deg; Ep = El. 7000..9000: eq. (42) applied to the
%   powers of Es and El; Ep = Ei.
c = p533.constants();
D = path.distance; f = path.frequency;
if D <= 7000
    best = -Inf; Psum = 0; modes = [path.Md_E, path.Md_F2];
    for k = 1:numel(modes)
        m = modes(k);
        if ~m.MC, continue; end
        m.Grw = p533.antennaGain(path.A_rx, f, path.rxBearing, m.ele);
        m.Prw = m.Ew + m.Grw - 20 * log10(f) - 107.2;
        Psum = Psum + 10 ^ (m.Prw / 10);
        if m.Prw > best, best = m.Prw; path.DMidx = k; path.Grw = m.Grw; path.ele = m.ele; end
        if k <= c.MAXEMDS, path.Md_E(k) = m; else, path.Md_F2(k - c.MAXEMDS) = m; end
    end
    if Psum > 0, path.Pr = 10 * log10(Psum); else, path.Pr = c.TINYDB; end
    path.Ep = path.Es;
else
    [Grw, ele] = p533.antennaGain08(path.A_rx, f, path.rxBearing);
    path.Grw = Grw; path.ele = ele;
    Pl = path.El + Grw - 20 * log10(f) - 107.2;
    if D >= 9000
        path.Pr = Pl; path.Ep = path.El;
    else
        Ps = path.Es + Grw - 20 * log10(f) - 107.2;      % powers corresponding to Es and El
        Xs = 10 ^ (0.01 * Ps); Xl = 10 ^ (0.01 * Pl);
        path.Pr = 100 * log10(Xs + (D - 7000) / 2000 * (Xl - Xs));
        path.Ep = path.Ei;
    end
end
end
