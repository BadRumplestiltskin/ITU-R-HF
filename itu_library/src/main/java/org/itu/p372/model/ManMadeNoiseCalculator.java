package org.itu.p372.model;

import org.itu.p372.api.ManMadeNoiseInput;
import org.itu.p372.api.ManMadeNoiseResult;

/**
 * Calculator for man-made noise according to ITU-R P.372-17 Part 6.
 * <p>
 * Applies environment category and frequency-dependent model
 * to compute median and decile noise levels.
 * </p>
 */
public class ManMadeNoiseCalculator {

    /**
     * Computes man-made noise levels.
     *
     * @param input Input parameters: frequency (MHz), location, UTC time, environment factor or category
     * @return ManMadeNoiseResult containing median, lower, and upper decile noise in dB
     */
    public ManMadeNoiseResult calculate(ManMadeNoiseInput input) {
        double freq = input.frequencyMHz();
        int envCat = (int) input.environmentFactor();

        double c;
        double d;
        double du;
        double dl;

        switch (envCat) {
            case 0 -> {
                // CITY
                c = 76.8; d = 27.7; du = 11.0; dl = 6.7;
            }
            case 1 -> {
                // RESIDENTIAL
                c = 72.5; d = 27.7; du = 10.6; dl = 5.3;
            }
            case 2 -> {
                // RURAL
                c = 67.2; d = 27.7; du = 9.2;  dl = 4.6;
            }
            case 3 -> {
                // QUIETRURAL
                c = 53.6; d = 28.6; du = 9.2;  dl = 4.6;
            }
            case 4 -> {
                // NOISY
                c = 83.2; d = 37.5; du = 11.0; dl = 6.7;
            }
            case 5 -> {
                // QUIET
                c = 65.2; d = 29.1; du = 9.2;  dl = 4.6;
            }
            default -> {
                // Numeric override of noise level
                c = -input.environmentFactor() + 204.0;
                d = 0.0;
                du = 11.0; dl = 6.7;
            }
        }

        double medianDb = c - d * Math.log10(freq);
        return new ManMadeNoiseResult(medianDb, dl, du);
    }
}
