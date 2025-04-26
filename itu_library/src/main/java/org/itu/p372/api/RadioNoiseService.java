// Directory: src/main/java/org/itu/p372/api

// File: RadioNoiseService.java
package org.itu.p372.api;

import java.time.ZonedDateTime;

/**
 * Service interface for calculating radio noise according to ITU-R P.372-17.
 * <p>
 * External applications should obtain an implementation via a factory or
 * dependency injection.
 * </p>
 * @see <a href="https://www.itu.int/rec/R-REC-P.372-17">ITU-R P.372-17</a>
 */
public interface RadioNoiseService {

    /**
     * Calculates combined noise components and total noise level.
     *
     * @param params Input parameters including frequency, location, and time.
     * @return NoiseResult containing per-component and total noise levels in dB.
     */
    NoiseResult calculateCombinedNoise(NoiseInputParameters params);

    /**
     * Calculates atmospheric (lightning) noise alone.
     *
     * @param input Input parameters specific to atmospheric noise.
     * @return AtmosphericNoiseResult with atmospheric noise components in dB.
     */
    AtmosphericNoiseResult calculateAtmosphericNoise(AtmosphericNoiseInput input);

    /**
     * Calculates man-made noise.
     *
     * @param input Input parameters specific to man-made noise.
     * @return ManMadeNoiseResult with man-made noise components in dB.
     */
    ManMadeNoiseResult calculateManMadeNoise(ManMadeNoiseInput input);

    /**
     * Calculates galactic noise.
     *
     * @param input Input parameters specific to galactic noise.
     * @return GalacticNoiseResult with galactic noise components in dB.
     */
    GalacticNoiseResult calculateGalacticNoise(GalacticNoiseInput input);
}
