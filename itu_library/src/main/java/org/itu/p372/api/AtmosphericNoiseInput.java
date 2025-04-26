
// File: AtmosphericNoiseInput.java
package org.itu.p372.api;

import java.time.ZonedDateTime;

/**
 * Input parameters specific to atmospheric noise calculations.
 *
 * @param frequencyMHz Frequency in MHz
 * @param location     Geographical location
 * @param dateTimeUtc  UTC date-time for atmospheric noise model
 * @param stormRate    Estimated storm rate (storms per hour)
 */
public record AtmosphericNoiseInput(
    double frequencyMHz,
    LocationRecord location,
    ZonedDateTime dateTimeUtc,
    double stormRate
) {}

