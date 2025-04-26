// Directory: src/main/java/org/itu/p372/model

package org.itu.p372.model;

import org.itu.p372.api.AtmosphericNoiseInput;
import org.itu.p372.api.AtmosphericNoiseResult;
import org.itu.p372.data.CoefficientProvider;
import org.itu.p372.data.FamParameters;
import java.time.ZonedDateTime;
import org.itu.p372.api.NoiseInputParameters;

/**
 * Calculator for lightning-induced (atmospheric) noise according to ITU-R P.372-17 Part 5.
 * <p>
 * Uses harmonized coefficients to compute median and decile noise
 * for a given UTC time, frequency, and location.
 * </p>
 */
public class LightningNoiseCalculator {

    private final CoefficientProvider coeffProvider;

    /**
     * @param coeffProvider Provider for Fam/Du/Dl coefficient tables
     */
    public LightningNoiseCalculator(CoefficientProvider coeffProvider) {
        this.coeffProvider = coeffProvider;
    }

    /**
     * Computes atmospheric noise levels (median, lower decile, upper decile) in dB.
     *
     * @param input Input parameters: frequency (MHz), location (lat/lon), UTC date-time
     * @return AtmosphericNoiseResult containing median, lower, and upper decile noise in dB
     */
    public AtmosphericNoiseResult calculate(AtmosphericNoiseInput input) {
        double freq = input.frequencyMHz();
        double lat = input.location().latitudeDeg();
        double lon = input.location().longitudeDeg();
        ZonedDateTime utcTime = input.dateTimeUtc();

        int utcHour = utcTime.getHour();
        int monthIndex = utcTime.getMonthValue() - 1;

        // 1. Compute Local Mean Time (LMT)
        double lmt = utcHour + lon / 15.0;
        lmt = (lmt % 24 + 24) % 24;

        // 2. Identify current and adjacent 4-hour blocks
        int blockNow = (int) (lmt / 4) % 6;
        int blockAdj = (blockNow + 1) % 6;
        double slp = (lmt % 4) / 4.0;

        // 3. Lookup Fam stats for both blocks
        FamParameters statsNow = coeffProvider.getFamParameters(
            monthIndex, blockNow, lat, lon, freq);
        FamParameters statsAdj = coeffProvider.getFamParameters(
            monthIndex, blockAdj, lat, lon, freq);

        // 4. Interpolate median noise (Fa)
        double faLin = Math.pow(10.0, statsNow.fa() / 10.0)
            + (Math.pow(10.0, statsAdj.fa() / 10.0)
            - Math.pow(10.0, statsNow.fa() / 10.0)) * slp;
        double medianDb = 10.0 * Math.log10(faLin);

        // 5. Interpolate upper-decile noise (Du)
        double duLin = Math.pow(10.0, statsNow.du() / 10.0)
            + (Math.pow(10.0, statsAdj.du() / 10.0)
            - Math.pow(10.0, statsNow.du() / 10.0)) * slp;
        double upperDb = 10.0 * Math.log10(duLin);

        // 6. Interpolate lower-decile noise (Dl)
        double dlLin = Math.pow(10.0, statsNow.dl() / 10.0)
            + (Math.pow(10.0, statsAdj.dl() / 10.0)
            - Math.pow(10.0, statsNow.dl() / 10.0)) * slp;
        double lowerDb = 10.0 * Math.log10(dlLin);

        // 7. Return results
        return new AtmosphericNoiseResult(medianDb, lowerDb, upperDb);
    }
}
