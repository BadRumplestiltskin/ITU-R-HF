// File: ManMadeNoiseResult.java
package org.itu.p372.api;

/**
 * Output for man-made noise calculations.
 *
 * @param medianDb  Median man-made noise level (dB)
 * @param lowerDb   Lower decile (e.g., 10th percentile) noise level (dB)
 * @param upperDb   Upper decile (e.g., 90th percentile) noise level (dB)
 */
public record ManMadeNoiseResult(
    double medianDb,
    double lowerDb,
    double upperDb
) {}