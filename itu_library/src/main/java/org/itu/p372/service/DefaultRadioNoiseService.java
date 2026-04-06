package org.itu.p372.service;

import java.util.List;
import org.itu.p372.api.AtmosphericNoiseInput;
import org.itu.p372.api.AtmosphericNoiseResult;
import org.itu.p372.api.GalacticNoiseInput;
import org.itu.p372.api.GalacticNoiseResult;
import org.itu.p372.api.ManMadeNoiseInput;
import org.itu.p372.api.ManMadeNoiseResult;
import org.itu.p372.api.NoiseInputParameters;
import org.itu.p372.api.NoiseResult;
import org.itu.p372.api.RadioNoiseService;
import org.itu.p372.model.CosmicNoiseCalculator;
import org.itu.p372.model.LightningNoiseCalculator;
import org.itu.p372.model.ManMadeNoiseCalculator;
import org.itu.p372.model.NoiseCombiner;

/**
 * Default implementation of RadioNoiseService.
 * Coordinates calculation of atmospheric, man-made, and galactic noise,
 * then combines them into a total noise level.
 */
public class DefaultRadioNoiseService implements RadioNoiseService {

    private final LightningNoiseCalculator lightningCalculator;
    private final ManMadeNoiseCalculator manMadeCalculator;
    private final CosmicNoiseCalculator cosmicCalculator;
    private final NoiseCombiner noiseCombiner;

    public DefaultRadioNoiseService(
            LightningNoiseCalculator lightningCalculator,
            ManMadeNoiseCalculator manMadeCalculator,
            CosmicNoiseCalculator cosmicCalculator,
            NoiseCombiner noiseCombiner) {
        this.lightningCalculator = lightningCalculator;
        this.manMadeCalculator = manMadeCalculator;
        this.cosmicCalculator = cosmicCalculator;
        this.noiseCombiner = noiseCombiner;
    }

    @Override
    public NoiseResult calculateCombinedNoise(NoiseInputParameters params) {
        AtmosphericNoiseResult atm = lightningCalculator.calculate(
            new AtmosphericNoiseInput(params.frequencyMHz(), params.location(), params.dateTimeUtc(), 0.0)
        );
        ManMadeNoiseResult man = manMadeCalculator.calculate(
            new ManMadeNoiseInput(params.frequencyMHz(), params.location(), params.dateTimeUtc(), 0)
        );
        GalacticNoiseResult gal = cosmicCalculator.calculate(
            new GalacticNoiseInput(params.frequencyMHz(), params.location(), params.dateTimeUtc())
        );

        double totalDb = noiseCombiner.combine(List.of(atm.medianDb(), man.medianDb(), gal.noiseDb()));

        return new NoiseResult(
            atm.medianDb(),
            man.medianDb(),
            gal.noiseDb(),
            totalDb
        );
    }

    @Override
    public AtmosphericNoiseResult calculateAtmosphericNoise(AtmosphericNoiseInput input) {
        return lightningCalculator.calculate(input);
    }

    @Override
    public ManMadeNoiseResult calculateManMadeNoise(ManMadeNoiseInput input) {
        return manMadeCalculator.calculate(input);
    }

    @Override
    public GalacticNoiseResult calculateGalacticNoise(GalacticNoiseInput input) {
        return cosmicCalculator.calculate(input);
    }
}
