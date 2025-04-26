// Directory: src/test/java/org/itu/p372/service

package org.itu.p372.service;

import org.itu.p372.api.LocationRecord;
import org.itu.p372.api.NoiseInputParameters;
import org.itu.p372.api.NoiseResult;
import org.junit.jupiter.api.Test;

import java.time.ZoneOffset;
import java.time.ZonedDateTime;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the RadioNoiseService implementation.
 */
public class RadioNoiseServiceTest {

    @Test
    public void testComputeSampleNoise() {
        // Arrange
        DefaultRadioNoiseService service = RadioNoiseServiceFactory.createDefaultService();
        NoiseInputParameters params = new NoiseInputParameters(
            10.0,  // frequency MHz
            new LocationRecord(40.0, 10.0),  // lat, lon deg
            ZonedDateTime.of(2021, 2, 1, 7, 0, 0, 0, ZoneOffset.UTC)  // UTC date/time
        );

        // Act
        NoiseResult result = service.calculateCombinedNoise(params);

        // Assert: ensure service returns sensible values
        assertNotNull(result, "Result should not be null");
        assertTrue(result.atmosphericDb() > 0.0, "Atmospheric noise should be positive");
        assertEquals(44.8, result.manMadeDb(), 1e-6, "Man-made noise should match expected for RESIDENTIAL");
        assertEquals(29.0, result.galacticDb(), 1e-6, "Galactic noise should match fixed model");
        assertTrue(result.totalDb() > result.manMadeDb(), "Total noise should exceed the largest component");
    }
}
