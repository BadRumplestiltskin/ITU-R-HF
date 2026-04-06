package org.itu.p372.api;

/**
 * Output for galactic noise calculations.
 *
 * @param noiseDb Galactic noise level (dB)
 */
public record GalacticNoiseResult(
    double noiseDb
) {}
