package org.itu.p372.data;

/**
 * Holds interpolated parameters for atmospheric noise (Fam) at a given frequency.
 * <p>
 * These values are obtained per 4-hour time block and represent:
 * <ul>
 *   <li><b>fa</b>: Atmospheric noise level (F<sub>A</sub>) in dB above kT<sub>0</sub>b at the specified frequency.</li>
 *   <li><b>du</b>: Upper decile deviation (D<sub>u</sub>) in dB above the median.</li>
 *   <li><b>dl</b>: Lower decile deviation (D<sub>l</sub>) in dB below the median.</li>
 * </ul>
 * </p>
 *
 * @param fa  Noise level (dB)
 * @param du  Upper decile deviation (dB)
 * @param dl  Lower decile deviation (dB)
 */
public record FamParameters(
    double fa,
    double du,
    double dl
) {}
