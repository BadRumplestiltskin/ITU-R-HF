// Directory: src/main/java/org/itu/p372/model

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
        double freq = input.frequencyMHz();

        // 1. Constants from P.372-17 Table for galactic noise
        double c = 52.0;
        double d = 23.0;

        // 2. Compute median noise: FaG = c - d * log10(freq)
        double noiseDb = c - d * Math.log10(freq);

        // 3. Standard decile deviation is fixed at 2 dB
        double decile = 2.0;

        return new GalacticNoiseResult(noiseDb);
    }
}
