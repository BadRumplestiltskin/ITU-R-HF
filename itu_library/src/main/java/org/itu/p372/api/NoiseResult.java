package org.itu.p372.api;

/**
 * Combined noise calculation output.
 *
 * @param atmosphericDb Atmospheric noise (dB)
 * @param manMadeDb     Man-made noise (dB)
 * @param galacticDb    Galactic noise (dB)
 * @param totalDb       Total combined noise (dB)
 */
public record NoiseResult(
    double atmosphericDb,
    double manMadeDb,
    double galacticDb,
    double totalDb
) {}

