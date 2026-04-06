package org.itu.p372.model;

import org.itu.p372.api.GalacticNoiseInput;
import org.itu.p372.api.GalacticNoiseResult;

/**
 * Calculator for galactic (extra-terrestrial) noise according to ITU-R P.372-17 Part 4.
 * <p>
 * Computes noise level as a simple power-law function of frequency,
 * with fixed decile deviations.
 * </p>
 */
public class CosmicNoiseCalculator {

    /**
     * Computes galactic noise level.
     *
     * @param input Input parameters: frequency (MHz), location, UTC time
     * @return GalacticNoiseResult containing noise level (median = fixed), and deciles
     */
    public GalacticNoiseResult calculate(GalacticNoiseInput input) {
        // P.372-17 Table: FaG = 52 - 23 * log10(freq)
        double noiseDb = 52.0 - 23.0 * Math.log10(input.frequencyMHz());
        return new GalacticNoiseResult(noiseDb);
    }
}
