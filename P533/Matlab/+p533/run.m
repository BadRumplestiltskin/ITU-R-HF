function path = run(path, stopAfter)
%RUN Execute the P.533-14 prediction for one path (the P533() sequence).
%   path = p533.run(path)
%   path = p533.run(path, stopAfter)   stop after 'part1', 'part2' or 'noise'
%   Requires path.maps, path.foF2var, path.coeff372 (see p533.loadData).
%   Sequence: validate, initialise (section 2), Part 1 (sections 3-4),
%   Part 2 (sections 5-6), noise at the receiver (P.372 via p372.noise),
%   Part 3 (sections 7-10).
if nargin < 2, stopAfter = ''; end
p533.validatePath(path);
path = p533.initializePath(path);
path = p533.mufBasic(path);
path = p533.mufVariability(path);
path = p533.mufOperational(path);
path = p533.eLayerScreeningFrequency(path);
if strcmpi(stopAfter, 'part1'), return; end
path = p533.medianSkywaveFieldStrengthShort(path);
path = p533.medianSkywaveFieldStrengthLong(path);
path = p533.between7000kmand9000km(path);
path = p533.medianAvailableReceiverPower(path);
if strcmpi(stopAfter, 'part2'), return; end
path.noise = p372.noise(path.coeff372, path.manMadeNoise, path.hour, path.L_rx.lng, path.L_rx.lat, path.frequency, path.sigmaRule);
if strcmpi(stopAfter, 'noise'), return; end
path = p533.circuitReliability(path);
end
