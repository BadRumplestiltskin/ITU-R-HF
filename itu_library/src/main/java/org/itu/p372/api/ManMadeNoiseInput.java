package org.itu.p372.api;

import java.time.ZonedDateTime;

/**
 * Input parameters specific to man-made noise calculations.
 *
 * @param frequencyMHz Frequency in MHz
 * @param location     Geographical location
 * @param dateTimeUtc  UTC date-time for man-made noise model
 * @param environmentFactor  Optional environment factor to account for local interference
 */
public record ManMadeNoiseInput(
    double frequencyMHz,
    LocationRecord location,
    ZonedDateTime dateTimeUtc,
    double environmentFactor
) {}