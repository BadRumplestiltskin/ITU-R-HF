// Directory: src/main/java/org/itu/p372/service

package org.itu.p372.service;

import org.itu.p372.data.CoefficientProvider;
import org.itu.p372.data.CoefficientProviderImpl;
import org.itu.p372.model.CosmicNoiseCalculator;
import org.itu.p372.model.LightningNoiseCalculator;
import org.itu.p372.model.ManMadeNoiseCalculator;
import org.itu.p372.model.NoiseCombiner;

/**
 * Factory to create pre-configured RadioNoiseService instances.
 */
public final class RadioNoiseServiceFactory {

    private RadioNoiseServiceFactory() {
        // Prevent instantiation
    }

    /**
     * Constructs a default RadioNoiseService with all required components.
     *
     * @return a fully initialized DefaultRadioNoiseService
     */
    public static DefaultRadioNoiseService createDefaultService() {
        // 1. Provider for atmospheric coefficients
        CoefficientProvider coeffProvider = new CoefficientProviderImpl();

        // 2. Calculators
        LightningNoiseCalculator lightningCalc = new LightningNoiseCalculator(coeffProvider);
        ManMadeNoiseCalculator manMadeCalc = new ManMadeNoiseCalculator();
        CosmicNoiseCalculator cosmicCalc = new CosmicNoiseCalculator();

        // 3. Noise combiner
        NoiseCombiner combiner = new NoiseCombiner();

        // 4. Service wiring
        return new DefaultRadioNoiseService(lightningCalc, manMadeCalc, cosmicCalc, combiner);
    }
}
