package org.itu.p372.api;

/**
 * Output for atmospheric noise calculations.
 *
 * @param medianDb  Median atmospheric noise level (dB)
 * @param lowerDb   Lower decile (e.g., 10th percentile) noise level (dB)
 * @param upperDb   Upper decile (e.g., 90th percentile) noise level (dB)
 */
public record AtmosphericNoiseResult(
    double medianDb,
    double lowerDb,
    double upperDb
) {}

