package org.itu.p372.api;

import java.time.ZonedDateTime;

/**
 * Parameters for combined noise calculation.
 *
 * @param frequencyMHz Frequency in MHz
 * @param location     Geographical location (lat/lon)
 * @param dateTimeUtc  UTC date-time for retrieval of noise parameters
 */
public record NoiseInputParameters(
    double frequencyMHz,
    LocationRecord location,
    ZonedDateTime dateTimeUtc
) {}

