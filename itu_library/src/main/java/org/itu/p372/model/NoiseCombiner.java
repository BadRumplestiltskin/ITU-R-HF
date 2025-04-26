// Directory: src/main/java/org/itu/p372/model

package org.itu.p372.model;

import java.util.List;

/**
 * Utility for combining multiple noise levels (in dB) into a single overall noise level.
 * Based on ITU-R P.372-17 Part 7: total noise = 10·log10(∑10^(Nᵢ/10)).
 */
public class NoiseCombiner {

    /**
     * Combines a list of noise levels in dB to one overall noise level in dB.
     *
     * @param noiseLevelsDb List of noise levels (dB) to combine
     * @return Combined noise level in dB
     */
    public double combine(List<Double> noiseLevelsDb) {
        double linearSum = 0.0;
        for (double ndB : noiseLevelsDb) {
            linearSum += Math.pow(10.0, ndB / 10.0);
        }
        return 10.0 * Math.log10(linearSum);
    }
}
