
// File: LocationRecord.java
package org.itu.p372.api;

/**
 * Geographical coordinates.
 *
 * @param latitudeDeg  Latitude in decimal degrees
 * @param longitudeDeg Longitude in decimal degrees
 */
public record LocationRecord(
    double latitudeDeg,
    double longitudeDeg
) {}

