package org.itu.p372.data;

/**
 * Provider for ITU-R P.372-17 harmonized atmospheric noise coefficients.
 * <p>
 * Implementations should load raw coefficient tables and perform
 * geographic and frequency interpolation to supply {@link FamParameters}.
 * </p>
 */
public interface CoefficientProvider {

    /**
     * Retrieves interpolated atmospheric noise parameters for a given month,
     * 4-hour time block, location, and frequency.
     *
     * @param month         Month index (0–11)
     * @param blockIndex    Index of the 4-hour time block (0–5)
     * @param latitudeDeg   Latitude in decimal degrees
     * @param longitudeDeg  Longitude in decimal degrees
     * @param frequencyMHz  Frequency in MHz
     * @return FamParameters containing F<sub>A</sub>, D<sub>u</sub>, and D<sub>l</sub> in dB
     */
    FamParameters getFamParameters(int month, int blockIndex, double latitudeDeg, double longitudeDeg, double frequencyMHz);
}