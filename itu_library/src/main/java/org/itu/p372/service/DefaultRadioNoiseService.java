// Directory: src/main/java/org/itu/p372/service

package org.itu.p372.service;

import org.itu.p372.api.*;
import org.itu.p372.model.*;
import java.util.List;

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

    /**
     * Constructs the service with required calculators and combiner.
     * @param lightningCalculator
     * @param manMadeCalculator
     * @param cosmicCalculator
     * @param noiseCombiner
     */
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
        // 1. Calculate individual components
        AtmosphericNoiseResult atm = lightningCalculator.calculate(params);
        ManMadeNoiseResult man = manMadeCalculator.calculate(
            new ManMadeNoiseInput(params.frequencyMHz(), params.location(), params.dateTimeUtc(), /* default env factor */ 0)
        );
        GalacticNoiseResult gal = cosmicCalculator.calculate(
            new GalacticNoiseInput(params.frequencyMHz(), params.location(), params.dateTimeUtc())
        );

        // 2. Combine the components
        List<Double> deciles = List.of(atm.medianDb(), man.medianDb(), gal.noiseDb());
        double totalDb = noiseCombiner.combine(deciles);

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
