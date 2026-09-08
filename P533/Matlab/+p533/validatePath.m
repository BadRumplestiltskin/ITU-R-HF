function validatePath(path)
%VALIDATEPATH Check the inputs of a path structure (ranges of ValidatePath.c).
%   p533.validatePath(path) errors with identifier p533:validate:<field>.
chk = @(ok, f, msg) assert(ok, 'p533:validate:%s', f, msg);
if ~(path.year >= 1900 && path.year <= 2100), error('p533:validate:year', 'year %d out of 1900..2100', path.year); end
if ~(path.month >= 1 && path.month <= 12 && path.month == fix(path.month)), error('p533:validate:month', 'month must be 1..12'); end
if ~(path.hour >= 0 && path.hour <= 23 && path.hour == fix(path.hour)), error('p533:validate:hour', 'hour must be 0..23 UTC'); end
if ~(path.SSN >= 0 && path.SSN <= 311), error('p533:validate:SSN', 'SSN must be 0..311'); end
if ~(path.frequency >= 1 && path.frequency <= 30), error('p533:validate:frequency', 'frequency must be 1..30 MHz'); end
if ~(path.BW >= 0.005 && path.BW <= 3e6), error('p533:validate:BW', 'bandwidth must be 0.005..3e6 Hz'); end
if ~(path.txpower >= -30 && path.txpower <= 60), error('p533:validate:txpower', 'txpower must be -30..60 dB(1 kW)'); end
if ~(path.SNRXXp >= 1 && path.SNRXXp <= 99), error('p533:validate:SNRXXp', 'SNRXXp must be 1..99'); end
if ~(path.SNRr >= -30 && path.SNRr <= 200), error('p533:validate:SNRr', 'SNRr out of range'); end
if ~(path.SIRr >= -30 && path.SIRr <= 200), error('p533:validate:SIRr', 'SIRr out of range'); end
if ~(path.F0 >= 0 && path.F0 <= 1000), error('p533:validate:F0', 'F0 must be 0..1000 Hz'); end
if ~(path.T0 >= 0 && path.T0 <= 1000), error('p533:validate:T0', 'T0 must be 0..1000 ms'); end
if ~(path.A >= 0 && path.A <= 1000), error('p533:validate:A', 'A must be 0..1000 dB'); end
if ~(path.TW >= 0 && path.TW <= 50), error('p533:validate:TW', 'TW must be 0..50 ms'); end
if ~(path.FW >= 0 && path.FW <= 1000), error('p533:validate:FW', 'FW must be 0..1000 Hz'); end
if ~all(ismember(path.Modulation, [0 1])), error('p533:validate:Modulation', 'Modulation must be 0 or 1'); end
if ~all(ismember(path.SorL, [0 1])), error('p533:validate:SorL', 'SorL must be 0 or 1'); end
for e = {'L_tx', 'L_rx'}
    L = path.(e{1});
    if ~(abs(L.lat) <= pi / 2 && abs(L.lng) <= pi), error('p533:validate:%s', e{1}, '%s out of range (radians)', e{1}); end
end
if isempty(path.maps) || isempty(path.foF2var) || isempty(path.coeff372)
    error('p533:validate:data', 'maps, foF2var and coeff372 must be loaded (p533.loadData)');
end
if ~(path.maps.month == path.month), error('p533:validate:maps', 'maps are for month %d, path.month is %d', path.maps.month, path.month); end
for e = {'A_tx', 'A_rx'}
    if ~isequal(size(path.(e{1}).pattern, 2), 360) || ~isequal(size(path.(e{1}).pattern, 3), 91)
        error('p533:validate:%s', e{1}, '%s pattern must be (nf,360,91)', e{1});
    end
end
end
