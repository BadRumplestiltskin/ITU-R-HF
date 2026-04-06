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
        CoefficientProvider coeffProvider = new CoefficientProviderImpl();
        LightningNoiseCalculator lightningCalc = new LightningNoiseCalculator(coeffProvider);
        ManMadeNoiseCalculator manMadeCalc = new ManMadeNoiseCalculator();
        CosmicNoiseCalculator cosmicCalc = new CosmicNoiseCalculator();
        NoiseCombiner combiner = new NoiseCombiner();
        return new DefaultRadioNoiseService(lightningCalc, manMadeCalc, cosmicCalc, combiner);
    }
}
