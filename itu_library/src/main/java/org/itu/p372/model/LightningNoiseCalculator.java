package org.itu.p372.model;

import java.time.ZonedDateTime;
import org.itu.p372.api.AtmosphericNoiseInput;
import org.itu.p372.api.AtmosphericNoiseResult;
import org.itu.p372.data.CoefficientProvider;
import org.itu.p372.data.FamParameters;

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

        int monthIndex = utcTime.getMonthValue() - 1;

        // Compute Local Mean Time and identify current/adjacent 4-hour blocks
        double lmt = utcTime.getHour() + lon / 15.0;
        lmt = (lmt % 24 + 24) % 24;
        int blockNow = (int) (lmt / 4) % 6;
        int blockAdj = (blockNow + 1) % 6;
        double slp = (lmt % 4) / 4.0;

        FamParameters statsNow = coeffProvider.getFamParameters(monthIndex, blockNow, lat, lon, freq);
        FamParameters statsAdj = coeffProvider.getFamParameters(monthIndex, blockAdj, lat, lon, freq);

        double medianDb = interpolateDb(statsNow.fa(), statsAdj.fa(), slp);
        double upperDb  = interpolateDb(statsNow.du(), statsAdj.du(), slp);
        double lowerDb  = interpolateDb(statsNow.dl(), statsAdj.dl(), slp);

        return new AtmosphericNoiseResult(medianDb, lowerDb, upperDb);
    }

    private static double interpolateDb(double nowDb, double adjDb, double slp) {
        double linNow = Math.pow(10.0, nowDb / 10.0);
        double linAdj = Math.pow(10.0, adjDb / 10.0);
        return 10.0 * Math.log10(linNow + (linAdj - linNow) * slp);
    }
}
