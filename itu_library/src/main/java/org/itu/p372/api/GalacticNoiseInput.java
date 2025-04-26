// File: GalacticNoiseInput.java
package org.itu.p372.api;

import java.time.ZonedDateTime;

/**
 * Input parameters specific to galactic noise calculations.
 *
 * @param frequencyMHz Frequency in MHz
 * @param location     Geographical location
 * @param dateTimeUtc  UTC date-time for galactic noise model
 */
public record GalacticNoiseInput(
    double frequencyMHz,
    LocationRecord location,
    ZonedDateTime dateTimeUtc
) {}