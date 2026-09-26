#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Local includes
#include "Common.h"
#include "Noise.h"

/*
	Definitions of the P372 handle and entry points that Noise.h declares extern.
	This is the one translation unit in this artifact that defines them; every
	other includer of Noise.h now merely declares them. Before this, each
	includer defined its own copy and the link depended on -z muldefs.
*/
#ifdef _WIN32
	HINSTANCE hLib;
	cP372Info dllP372Version;
	cP372Info dllP372CompileTime;
	iNoise dllNoise;
	iNoiseMemory dllAllocateNoiseMemory;
	iNoiseMemory dllFreeNoiseMemory;
	iReadFamDud dllReadFamDud;
	vInitializeNoise dllInitializeNoise;
	vAtmosphericNoise dllAtmosphericNoise;
	vAtmosphericNoise_LT dllAtmosphericNoise_LT;
	iMakeNoise dllMakeNoise;
	dFamFreqVariation dllFamFreqVariation;
#elif defined(__linux__) || defined(__APPLE__)
	void *hLib;
	char *(*dllP372Version)();
	char *(*dllP372CompileTime)();
	int (*dllNoise)(struct NoiseParams *, int, double, double, double);
	int (*dllAllocateNoiseMemory)(struct NoiseParams *);
	int (*dllFreeNoiseMemory)(struct NoiseParams *);
	int (*dllReadFamDud)(struct NoiseParams *, const char *, int);
	void (*dllInitializeNoise)(struct NoiseParams *);
	void (*dllAtmosphericNoise)(struct NoiseParams *, int, double, double, double);
	void (*dllAtmosphericNoise_LT)(struct NoiseParams *, struct FamStats *, int, double, double, double);
	int (*dllMakeNoise)(int, int, double, double, double, double, char *, double *, int);
	double (*dllFamFreqVariation)(struct NoiseParams *, int, double, double);
#endif


// Local prototypes
void GalacticNoise(
    struct NoiseParams *noiseP,
    double frequency
);
void ManMadeNoise(
    struct NoiseParams* noiseP,
    double frequency
);
void GetFamParameters(
    struct NoiseParams *noiseP,
    struct FamStats *FS,
    double lng,
    double lat,
    double frequency
);
// End Local prototypes

int Noise(
    struct NoiseParams *noiseP,
    int hour,
    double rlng,
    double rlat,
    double frequency
) {
    /*
        Determines atmospheric, man-made and galactic noise and the associated
        decile values.
        The routine AtmosphericNoise() is the most involved calculation.
        The other two routines (GalacticNoise() and ManMadeNoise()) were
        written for consistancy.
        The Noise() routine also determines the combined noise.

        All noise figures are in dB above kT0b; deciles are in dB.

        INPUT
            struct NoiseParams *noiseP  Allocated structure whose fakp,
                    fakabp, fam and dud arrays hold the month's coefficients
                    (ReadFamDud()), and whose ManMadeNoise member selects the
                    man-made noise:
                      0..5 (CITY, RESIDENTIAL, RURAL, QUIETRURAL, NOISY,
                      QUIET; Noise.h)  category codes, see ManMadeNoise()
                      other value >= 0 user value, see ManMadeNoise()
                      value < 0        override, see below
                    (There is no separate manMadeNoise argument; it is the
                    structure member.)
            int hour          UTC hour; AtmosphericNoise() converts it to
                              receiver local mean time
            double rlng (rad) Receiver longitude, east positive
            double rlat (rad) Receiver latitude, north positive
            double frequency  (MHz)

        OUTPUT
            noiseP->FamT - Total noise
            noiseP->DuT - Upper decile deviation of total noise
            noiseP->DlT - Lower decile deviation of total noise

            via AtmosphericNoise()
            noiseP->FaA - Atmospheric noise
            noiseP->DuA - Upper decile deviation of atmospheric noise
            noiseP->DlA - Lower decile deviation of atmospheric noise

            via GalacticNoise()
            noiseP->FaG - Galactic noise
            noiseP->DuG - Upper decile deviation of galactic noise
            noiseP->DlG - Lower decile deviation of galactic noise

            via ManMadeNoise()
            noiseP->FaM - Man-made noise
            noiseP->DuM - Upper decile deviation of man-made noise
            noiseP->DlM - Lower decile deviation of man-made noise

            Returns RTN_NOISEOK in every case (including the override); no
            input is validated.

        OVERRIDE (noiseP->ManMadeNoise < 0.0)
            No noise calculation is done. FaA, FaG, all component deciles and
            DuT, DlT are set to 0.0; FaM is set to ManMadeNoise itself (the
            negative number); FamT is set to -ManMadeNoise, i.e. a value of
            -X requests a total noise of X dB above kT0b.

        COMBINATION (the code cites ITU-R P.372-10 Section 8)
            The three components are combined as log-normal variables, once
            with the upper deciles and once with the lower deciles.
            With c = 10/ln(10) and, for each component i,
                sigma_i = D_i / 1.282 (atmospheric and man-made),
                sigma_G = 1.56 (galactic, fixed),
                alpha_T = sum exp(Fa_i/c + sigma_i^2/(2 c^2))
                beta_T  = sum exp(Fa_i/c + sigma_i^2/(2 c^2))^2
                              * (exp((sigma_i/c)^2) - 1)
                gamma_T = sum exp(Fa_i/c)
            the default is
                sigma_T = c * sqrt(ln(1 + beta_T / alpha_T^2)).
            Decile rule: if ANY component decile of the side being computed
            (DuA, DuG, DuM for the upper; DlA, DlG, DlM for the lower)
            exceeds 12 dB, the code instead uses
                sigma_T = c * sqrt(2 ln(alpha_T / gamma_T))
            as a replacement for the expression above. Another
            implementation treats this expression as a maximum (upper bound)
            on sigma_T rather than a replacement. Which reading is correct
            awaits a check against the P.372-17 text; the code is left
            as it is.
            Then Fam_T = c * (ln(alpha_T) - sigma_T^2/(2 c^2)) and
            D_T = 1.282 * sigma_T (DuT from the upper pass, DlT from the
            lower pass). FamT is the smaller of the two Fam_T values
            (commented in the code as "Worst-case noise").

        SUBROUTINES
            AtmosphericNoise()
            GalacticNoise()
            ManMadeNoise()

            ******************************************************************
            These software methods for the prediction of the performance of HF
            circuits based on Recommendations ITU-R P.533-14 and P.372-13.
            The ITURHFProp, P533 and P372 software has been developed
            collaboratively by participants in ITU-R Study Group 3.
            It may be used by implementers in their implementation of the
            Recommendation as well as in revisions of the specific original
            Recommendation and in other ITU Recommendations, free from any
            copyright assertions.

            This software is provided "as is" WITH NO WARRANTIES,
            EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE WARRANTIES OF
            MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
            NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS.

            The ITU shall not be held liable in any event for any damages
            whatsoever (including, without limitation, damages for loss of
            profits, business interruption, loss of information, or any other
            pecuniary loss) arising out of or related to use of the software.
            *******************************************************************
     */

    double sigmaA; // Standard deviation of the atmospheric noise
    double sigmaG; // Standard deviation of the galactic noise
    double sigmaM; // Standard deviation of the man-made noise
    double c;      // Constant for the calculation of the combined noise
    double alphaT;
    double betaT;
    double gammaT;
    double sigmaT;
    double FamTu, FamTl;

    // ******************************************************************* //
    // **************** Noise Calulation Override ************************ //
    // ******************************************************************* //
    if (noiseP->ManMadeNoise < 0.0) {
        /*
        If the manMadeNoise is negative, the user wants to override the
        noise calculation. There are no decile values or noise components.
        Although they should already be initialized, set all of the statistical
        noise components to 0.0 to make it clear that you are canceling the 
        noise calculation.
        Then, set the total noise to the desired values and return.
       */

        noiseP->DuA = 0.0;
        noiseP->DuG = 0.0;
        noiseP->DuM = 0.0;

        noiseP->DlA = 0.0;
        noiseP->DlG = 0.0;
        noiseP->DlM = 0.0;

        noiseP->FaA = 0.0;
        noiseP->FaG = 0.0;
        noiseP->FaM = noiseP->ManMadeNoise;

        noiseP->DuT = 0.0;
        noiseP->DlT = 0.0;

        noiseP->FamT = -noiseP->ManMadeNoise;

        return RTN_NOISEOK;
    }
    // ********************************************************************* //
    // **************** End Noise Calulation Override ********************** //
    // ********************************************************************* //

    AtmosphericNoise(
        noiseP,
        hour,
        rlng,
        rlat,
        frequency
    );

    GalacticNoise(
        noiseP,
        frequency
    );

    ManMadeNoise(
        noiseP,
        frequency
    );

    /*
    Determine the combined noise according to ITU-R P.372-10 Section 8
    "The Combination of Noises from Several Sources".
    Find the upper decile sigmaT.
    */
    sigmaA = noiseP->DuA / 1.282;
    sigmaG = 1.56;
    sigmaM = noiseP->DuM / 1.282;

    c = 10.0 / log(10.0);

    alphaT = exp((noiseP->FaA / c) + (pow(sigmaA, 2) / (2.0 * pow(c, 2)))) +
        exp((noiseP->FaG / c) + (pow(sigmaG, 2) / (2.0 * pow(c, 2)))) +
        exp((noiseP->FaM / c) + (pow(sigmaM, 2) / (2.0 * pow(c, 2))));

    betaT = pow(exp((noiseP->FaA / c) + (pow(sigmaA, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaA / c, 2)) - 1.0) +
        pow(exp((noiseP->FaG / c) + (pow(sigmaG, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaG / c, 2)) - 1.0) +
        pow(exp((noiseP->FaM / c) + (pow(sigmaM, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaM / c, 2)) - 1.0);

    gammaT = exp(noiseP->FaA / c) + exp(noiseP->FaG / c) + exp(noiseP->FaM / c);

    if ((noiseP->DuA > 12.0)
        || (noiseP->DuG > 12.0)
        || (noiseP->DuM > 12.0))
    {
        sigmaT = c * sqrt(2.0 * log(alphaT / gammaT));
    } else {
        sigmaT = c * sqrt(log(1.0 + (betaT / pow(alphaT, 2))));
    }

    FamTu = c * (log(alphaT) - (pow(sigmaT, 2) / (2.0 * pow(c, 2))));

    noiseP->DuT = 1.282 * sigmaT;

    // Find the lower decile sigmaT.
    sigmaA = noiseP->DlA / 1.282;
    sigmaG = 1.56;
    sigmaM = noiseP->DlM / 1.282;

    c = 10.0 / log(10.0);

    alphaT = exp((noiseP->FaA / c) + (pow(sigmaA, 2) / (2.0 * pow(c, 2)))) +
        exp((noiseP->FaG / c) + (pow(sigmaG, 2) / (2.0 * pow(c, 2)))) +
        exp((noiseP->FaM / c) + (pow(sigmaM, 2) / (2.0 * pow(c, 2))));

    betaT = pow(exp((noiseP->FaA / c) + (pow(sigmaA, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaA / c, 2)) - 1.0) +
        pow(exp((noiseP->FaG / c) + (pow(sigmaG, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaG / c, 2)) - 1.0) +
        pow(exp((noiseP->FaM / c) + (pow(sigmaM, 2) / (2.0 * pow(c, 2)))), 2) * (exp(pow(sigmaM / c, 2)) - 1.0);

    gammaT = exp(noiseP->FaA / c) + exp(noiseP->FaG / c) + exp(noiseP->FaM / c);

    if ((noiseP->DlA > 12.0)
        || (noiseP->DlG > 12.0)
        || (noiseP->DlM > 12.0))
    {
        sigmaT = c * sqrt(2.0 * log(alphaT / gammaT));
    } else {
        sigmaT = c * sqrt(log(1.0 + (betaT / pow(alphaT, 2))));
    }

    FamTl = c * (log(alphaT) - (pow(sigmaT, 2) / (2.0 * pow(c, 2))));

    noiseP->DlT = 1.282 * sigmaT;

    noiseP->FamT = min(FamTu, FamTl); // Worst-case noise.

    return RTN_NOISEOK;
}

/*
	FamFreqVariation() - Frequency variation of atmospheric noise.

		The polynomial from NBS Tech Note 318 (Lucas and Harper), "A Numerical
		Representation of CCIR Report 322 High Frequency (3-30 Mc/s) Atmospheric
		Radio Noise Data", page 5, which turns the 1 MHz noise figure into the
		figure at the frequency of interest.

		AtmosphericNoise() and the figure generator in ITURNoise.c both need it.
		ITURNoise.c carried a verbatim copy, which would have drifted from this
		one the moment either was corrected; the figure generator is what
		implementers validate against, so the two must not diverge.

		The polynomial is evaluated twice by Horner's rule: first at
		u = -0.75 (giving the constant cz from Fam1MHz), then at
		u = (8 * 2^log10(frequency) - 11) / 4; the result is cz * pz + px.
		Coefficients fam[tmblk][0..6] feed pz and fam[tmblk][7..13] feed px.

		INPUT
			struct NoiseParams *noiseP	supplies noiseP->fam (from ReadFamDud())
			int tmblk		the time-block index, already adjusted for hemisphere
							(0-5 northern, 6-11 southern: row of fam[12][14])
			double Fam1MHz	the noise figure at 1 MHz (dB above kT0b)
			double frequency (MHz)

		OUTPUT
			returns the noise figure at frequency (dB above kT0B)

		SUBROUTINES
			None
*/
DLLEXPORT double FamFreqVariation(struct NoiseParams *noiseP, int tmblk,
                                  double Fam1MHz, double frequency) {

    double u[2];
    double pz = 0.0, px = 0.0, cz = 0.0;
    int j, k;

    u[0] = -0.75;
    // U = (8. * 2.**X - 11.)/4. where X = ALOG10(FREQ)
    u[1] = (8.0 * pow(2.0, log10(frequency)) - 11.0) / 4.0;

    for (k = 0; k < 2; k++) {
        // PZ = U1*FAM(1,TIMEBLOCKINDX) + FAM(2,TIMEBLOCKINDX)
        pz = u[k] * noiseP->fam[tmblk][0] + noiseP->fam[tmblk][1];
        // PX = U1*FAM(8,TIMEBLOCKINDX) + FAM(9,TIMEBLOCKINDX)
        px = u[k] * noiseP->fam[tmblk][7] + noiseP->fam[tmblk][8];

        for (j = 2; j < 7; j++) {
            // PZ = U1*PZ + FAM(I,TIMEBLOCKINDX)
            pz = u[k] * pz + noiseP->fam[tmblk][j];
            // PX = U1*PX + FAM(I+7,TIMEBLOCKINDX)
            px = u[k] * px + noiseP->fam[tmblk][j + 7];
        } // j=2,6

        if (k == 0) {
            cz = Fam1MHz * (2.0 - pz) - px;
            // U1 = U
        }
    } // k=0,1

    return cz * pz + px;

}


void AtmosphericNoise(
    struct NoiseParams *noiseP,
    int hour, 
    double rlng,
    double rlat, 
    double frequency
) {
    /*
     This method is based on REC533() code and not an ITU Recommendation. 
     Although it does provide the numbers that are required.
     Specifically, the statistics of Fam described in ITU-R P.372-10
     Section 4 Figs. 15c to 38c.

     The calculation of the atmospheric noise is based on using one or two four
     hour time blocks to determine the noise. The following code sets the two 
     counters FS_now.tmblk and FS_adj.tmblk to the correct 4 hour time block,
     See P.372-9 for the six 4hr time blocks graph sets.
     The time blocks are determined by first choosing the timeblock that 
     contains LMT of the reciever, lrxmt.
     The time block, FS_now.tmblk, is set to the index of the timeblock that 
     contains lrxmt. The "adjacent" time block, FS_adj.tmblk is then determined
     in the following way.
     If lrsmt occurs in the first 2 hours of the FS_now.tmblk'th time block,
     then FS_adj.tmblk is set to the timeblock previous to FS_now.tmblk. 
     If lrxmt occurs in the 3rd hour of the FS_now.tmblk'th time block, then 
     FS_adj.tmblk is set to FS_now.tmblk, i.e., FS_now.tmblk = FS_adj.tmblk.
     If lrxmt occurs in the 4th hour of the time block, then FS_adj.tmblk is 
     set to the next timeblock.
         tmblk = time block of interest
         tmblk     Reciever LMT hours
           0           0000-0400
           1           0400-0800
           2           0800-1200
           3           1200-1600
           4           1600-2000
           5           2000-2400

    This routine is based on portions of the REC533() routines:
    GENFAM(), GENOIS1(), ANOIS1() and NOISY().

     NOTE ON THE TEXT ABOVE: the code below does not select the previous
     or the same block as described. It always takes FS_adj.tmblk as the
     NEXT block, (FS_now.tmblk + 1) % 6, and interpolates linearly in power
     with slp = (lrxmt mod 4) / 4, so slp is 0, 0.25, 0.5 or 0.75.

     What the code does:
         1) lrxmt = hour + (int)(rlng / 15 degrees), truncated toward zero,
            wrapped once into 0..23.
         2) FS_now.tmblk = lrxmt / 4; FS_adj.tmblk = the following block.
         3) GetFamParameters() for both blocks.
         4) FaA, DuA and DlA are each interpolated between the two blocks
            as powers (10^(x/10)) and converted back to dB.

     INPUT
         struct NoiseParams *noiseP  Coefficient arrays from ReadFamDud()
         int hour           UTC hour (0-23 expected)
         double lng (rad)   East positive
         double lat (rad)   North positive
         double frequency   (MHz)

     OUTPUT
         noiseP->FaA - Atmospheric noise (dB above kT0b)
         noiseP->DuA - Upper decile deviation of atmospheric noise (dB)
         noiseP->DlA - Lower decile deviation of atmospheric noise (dB)
         Nothing is returned.

     SUBROUTINES
         GetFamParameters()
     */

    int lrxmt; // Local reciever mean time
    double slp; // Interpolation factor
    double fa; // Linear noise power above kTB in 1 MHz

    struct FamStats FS_now;
    struct FamStats FS_adj;

    // Find the local time (0-23) from the clock UTC, hour, and the longitude.
    lrxmt = hour + (int)(rlng / (15.0 * D2R));

    // Roll over the local time if necessary.
    if (lrxmt < 0) {
        lrxmt += 24;
    } else if (lrxmt > 23) {
        lrxmt -= 24;
    }

    /*
    The atmospheric noise is determined by:
        i) finding the atmospheric noise at the current time block at the 
           reciever local mean time.
        ii) finding the noise for the "adjacent" time block and
        iii) iterating between the two noise values.
    There are 6 time blocks so the modulo 6 keeps the indexes inbounds.
    */
    FS_now.tmblk = (lrxmt / 4) % 6;
    FS_adj.tmblk = (FS_now.tmblk + 1) % 6;

    GetFamParameters(
        noiseP,
        &FS_now,
        rlng,
        rlat,
        frequency
    );
    GetFamParameters(
        noiseP,
        &FS_adj,
        rlng,
        rlat,
        frequency
    );

    // Interpolate is based on the local reciever mean time, lrxmt,
    // and the 4 hour timeblock.
    slp = fmod(lrxmt, 4.0) / 4.0;

    fa = pow(10.0, (FS_now.FA / 10.0))
       + (pow(10.0, (FS_adj.FA / 10.0)) - pow(10.0, (FS_now.FA / 10.0))
         ) * slp;
    noiseP->FaA = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.Du / 10.0))
       + (pow(10.0, (FS_adj.Du / 10.0)) - pow(10.0, (FS_now.Du / 10.0))
         ) * slp;
    noiseP->DuA = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.Dl / 10.0))
       + (pow(10.0, (FS_adj.Dl / 10.0)) - pow(10.0, (FS_now.Dl / 10.0))
         ) * slp;
    noiseP->DlA = 10.0 * log10(fa);

    return;
}

void GetFamParameters(
    struct NoiseParams *noiseP,
    struct FamStats* FS,
    double lng,
    double lat,
    double frequency
) {
    /*
     Find the atmospheric noise parameters given in Figs. 15c to 38c in
     P.372-10.
     This routine is based on portions of the REC533() routines:
     GENFAM(), GENOIS1(), ANOIS1() and NOISY().

     Steps:
         1) Fam at 1 MHz (dB above kT0b) from a double Fourier (sine)
            series: for each of 29 latitude terms, a 15-term sine series in
            q = (east longitude, 0..2 pi)/2 using fakp[tmblk][0..14][j] plus
            the constant fakp[tmblk][15][j]; then a 29-term sine series in
            q = lat + pi/2, plus the linear term
            fakabp[tmblk][0] + fakabp[tmblk][1] * q.
         2) The time-block index is shifted by 6 for a southern-hemisphere
            receiver (lat < 0) to address the fam and dud rows.
         3) FA at the operating frequency from FamFreqVariation().
         4) Du, Dl, SigmaDu, SigmaDl and SigmaFam from 4th-order
            polynomials in x = log10(frequency) with coefficients
            dud[0..4][row][0..4]; x is capped at log10(20) (the code notes
            the curves stop at 20 MHz), and for SigmaFam x is set to 1.0
            (10 MHz) above 10 MHz.

         INPUT
             struct NoiseParams *noiseP  fakp, fakabp, fam, dud arrays
             struct FamStats *FS  FS->tmblk (0-5, receiver local time
                                  block) must be set by the caller
             double lng   (rad) east positive; negative values are mapped
                          to 0..2 pi
             double lat   (rad) north positive
             double frequency (MHz)

        OUTPUT
            struct FamStats *FS - structure containing the noise parameters
                FS->FA        Fam at frequency (dB above kT0b)
                FS->Du, FS->Dl                     deciles (dB)
                FS->SigmaDu, FS->SigmaDl, FS->SigmaFam  standard
                                                   deviations (dB)
                FS->tmblk is not changed.

        SUBROUTINES
            None
     */

    double v[5];
    double x;
    double y;
    double Fam1MHz; // Atmospheric noise Fam (dB above kT0b at 1 MHz)
    double R;
    double ZZ[30]; // This assumes lm = 29
    double q; // Temp Latitude or Longitude

    int i, j, k;
    int lm, ln;

    // First find the atmospheric noise Fam (dB above kT0b at 1 MHz).
    // Set the limits of the Fourier series
    lm = 29;
    ln = 15;
    /*
    The longitude used here is the geographic east longitude
    (0 to 2*PI radians).
    Initialize the temp, q, as half the geographic east longitude.
    */
    if (lng < 0.0) {
        q = (lng + 2.0 * PI) / 2.0;
    } else {
        q = lng / 2.0;
    }

    // Calculate the longitude series
    for (j = 0; j < lm; j++) {
        ZZ[j] = 0.0; // Initialize ZZ[j]
        R = 0.0;
        for (k = 0; k < ln; k++) {
            R = R + sin((k + 1.0) * q) * noiseP->fakp[FS->tmblk][k][j];
        }
        ZZ[j] = R + noiseP->fakp[FS->tmblk][15][j];
    }

    // Calculate the latitude series
    // Reuse the temp, q, as the latitude plus 90 degrees
    q = (lat + PI / 2.0);

    R = 0.0;
    for (j = 0; j < lm; j++) {
        R = R + sin((j + 1.0) * q) * ZZ[j];
    }
    // Final Fourier series calculation.
    // (Note the linear nomalization using fakabp values)
    Fam1MHz = R + noiseP->fakabp[FS->tmblk][0] + noiseP->fakabp[FS->tmblk][1] * q;

    // Determine if the reciever latitude is positive or negative
    if (lat < 0) {
        i = FS->tmblk + 6; // TIMEBLOCKINDX=TIMEBLOCKINDX+6
    } else {
        i = FS->tmblk; // TIMEBLOCKINDX=TIMEBLOCKINDX
    }

    // for K = 0 then U1 = -0.75
    // for K = 1 then U1 = U
    // Frequency variation of atmospheric noise. Shared with ITURNoise.c, which
    // used to hold a verbatim copy of this polynomial.
    FS->FA = FamFreqVariation(noiseP, i, Fam1MHz, frequency);

    // Limit frequency to 20 MHz for Du, Dl, SigmaDu, SigmaDl
    // because curves in ITU-R P.372 only go to 20 MHz.
    x = log10(frequency);
    if (frequency > 20.0) {
        x = log10(20.0);
    }

    for (j = 0; j < 5; j++) { // DO I=1,5
        // Limit frequency to 10 MHz for SigmaFam
        // because curves in ITU-R P.372 only go to 10 MHz.
        // IF((I .EQ. 5) .AND. (FREQ .GT. 10.0)) THEN
        if ((j == 4) && (frequency > 10.0)) {
            x = 1.0;
        }
        // Y = DUD(1,TIMEBLOCKINDX,I)
        y = noiseP->dud[j][i][0];

        for (k = 1; k < 5; k++) {
           // Y = Y*X + DUD(J,TIMEBLOCKINDX,I)
            y = y * x + noiseP->dud[j][i][k];
        } // k=1,4

        v[j] = y; // V(I) = Y
    } // j=0,4

    // Store the return values
    FS->Du = v[0];      // Du = V(1)
    FS->Dl = v[1];      // Dl = V(2)
    FS->SigmaDu = v[2]; // Sigma_Du = V(3)
    FS->SigmaDl = v[3]; // Sigma_Dl = V(4)
    FS->SigmaFam = v[4];// Sigma_Fam = V(5)

    return;
}

void ManMadeNoise(
    struct NoiseParams *noiseP,
    double frequency
) {
    /*
    Determine the man-made noise in accordance with Section 5 
    "Man-made noise" P.372-10.

    FaM = c - d * log10(frequency), with c, d and the deciles chosen by the
    value of noiseP->ManMadeNoise (compared exactly with ==):

        code  name        c      d     DuM   DlM   (deciles taken from)
        0.0   CITY        76.8   27.7  11.0  6.7   CITY
        1.0   RESIDENTIAL 72.5   27.7  10.6  5.3   RESIDENTIAL
        2.0   RURAL       67.2   27.7   9.2  4.6   RURAL
        3.0   QUIETRURAL  53.6   28.6   9.2  4.6   RURAL
        5.0   QUIET       65.2   29.1   9.2  4.6   RURAL
        4.0   NOISY       83.2   37.5  11.0  6.7   CITY
        other (user value)  c = 204 - ManMadeNoise, d = 0, so FaM is
              204 - ManMadeNoise independent of frequency; deciles CITY.

    The code states that QUIET and NOISY are not categories of P.372-10.
    Negative values never reach this routine (Noise() handles them as an
    override). The units intended for a user value are not stated in the
    code; the arithmetic only shows FaM = 204 - value.

        INPUT
            struct NoiseParams *noiseP  noiseP->ManMadeNoise is the selector
                    (there is no separate manMadeNoise argument)
            double frequency (MHz)

        OUTPUT
            noiseP->FaM - Man-made noise (dB above kT0b)
            noiseP->DuM - Upper decile deviation of man-made noise (dB)
            noiseP->DlM - Lower decile deviation of man-made noise (dB)

        SUBROUTINES
            None
     */

    double c, d; // Intermediate values for the calculation of FaM

    if (noiseP->ManMadeNoise == CITY) {
        c = 76.8;
        d = 27.7;
        // Use the CITY category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 11.0;
        noiseP->DlM = 6.7;
    }
    else if (noiseP->ManMadeNoise == RESIDENTIAL) {
        c = 72.5;
        d = 27.7;
        // Use the RESIDENTIAL category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 10.6;
        noiseP->DlM = 5.3;
    }
    else if (noiseP->ManMadeNoise == RURAL) {
        c = 67.2;
        d = 27.7;
        // Use the RURAL category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 9.2;
        noiseP->DlM = 4.6;
    }
    else if (noiseP->ManMadeNoise == QUIETRURAL) {
        c = 53.6;
        d = 28.6;
        // Use the RURAL category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 9.2;
        noiseP->DlM = 4.6;
    }
    // The noise categories QUIET and NOISY are not in ITU-R P.372-10
    else if (noiseP->ManMadeNoise == QUIET) {
        c = 65.2;
        d = 29.1;
        // Use the RURAL category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 9.2;
        noiseP->DlM = 4.6;
    }
    else if (noiseP->ManMadeNoise == NOISY) {
        c = 83.2;
        d = 37.5;
        // Use the CITY category in Table 2 P.372-10 for the deciles
        noiseP->DuM = 11.0;
        noiseP->DlM = 6.7;
    }
    else { // If manMadeNoise is any other number then it must be user input
        c = -noiseP->ManMadeNoise + 204.0;
        d = 0.0;
        // Use the CITY category in Table 2 P.372-10 for the deciles
        // (upper 11.0 dB, lower 6.7 dB; these were assigned the other way round)
        noiseP->DuM = 11.0;
        noiseP->DlM = 6.7;
    }

    // Calculate the man made noise, FaM
    noiseP->FaM = c - d * log10(frequency);

}

void GalacticNoise(
    struct NoiseParams *noiseP,
    double frequency
) {
    /*
    Determine the galactic noise in accordance with Section 5 "Man-made noise" 
    Table 1 P.372-10.
    FaG = 52 - 23 * log10(frequency); both deciles are fixed at 2 dB.
    (Noise() uses sigma_G = 1.56 dB, i.e. 2/1.282, for this component.)

        INPUT
            struct NoiseParams *noiseP  structure to fill
            double frequency (MHz)

        OUTPUT
            noiseP->FaG - Galactic noise (dB above kT0b)
            noiseP->DuG - Upper decile deviation of galactic noise (2 dB)
            noiseP->DlG - Lower decile deviation of galactic noise (2 dB)

        SUBROUTINES
            None
     */

    double c, d; // Intermediate values for the calculation of FaG

    // Calculate the galactic noise, FaG
    c = 52.0;
    d = 23.0;
    noiseP->FaG = c - d * log10(frequency);

    // Determine the decile values are set to 2 dB (3/1.282)
    noiseP->DuG = 2.0;
    noiseP->DlG = 2.0;
}


/*
	Coefficient cache for ReadFamDud().

	ReadFamDud() parses a 235 KB fixed-format COEFF text file to fill four
	arrays totalling about 25 KB. A batch whose circuits are not grouped by
	month calls it on every month change, so a 100,000-circuit file with mixed
	months re-parsed the same twelve files tens of thousands of times.

	The parsed arrays are small, so all twelve months cost 306 KB. They are
	cached on first use and copied out thereafter, which keeps ReadFamDud()'s
	contract of filling the caller's NoiseParams unchanged. Each entry records
	the file it came from, so a call naming a different DataFilePath re-reads
	rather than returning another directory's coefficients.

	Not thread safe, in keeping with the rest of the engine.
*/
static struct {
	double fakp[6][16][29];
	double fakabp[6][2];
	double dud[5][12][5];
	double fam[12][14];
	char   path[270];	// File the month was parsed from (InFilePath)
	int    loaded;
} FamDudCache[12];

/*
	FamDudCopyOut() - Fills a NoiseParams from a cached month.

		INPUT
			struct NoiseParams *noiseP	allocated destination
			int month	0-based month (0-11); the caller has checked the
						range and that FamDudCache[month] is loaded

		OUTPUT
			noiseP->fakp, fakabp, dud and fam

		SUBROUTINES
			None
*/
static void FamDudCopyOut(struct NoiseParams *noiseP, int month) {

    int i, j, k;

    for (i = 0; i < 6; i++)
        for (j = 0; j < 16; j++)
            for (k = 0; k < 29; k++) noiseP->fakp[i][j][k] = FamDudCache[month].fakp[i][j][k];

    for (j = 0; j < 6; j++)
        for (k = 0; k < 2; k++) noiseP->fakabp[j][k] = FamDudCache[month].fakabp[j][k];

    for (i = 0; i < 5; i++)
        for (j = 0; j < 12; j++)
            for (k = 0; k < 5; k++) noiseP->dud[i][j][k] = FamDudCache[month].dud[i][j][k];

    for (j = 0; j < 12; j++)
        for (k = 0; k < 14; k++) noiseP->fam[j][k] = FamDudCache[month].fam[j][k];

}

/*
	FamDudCopyIn() - Saves a freshly parsed month into the cache.

		INPUT
			struct NoiseParams *noiseP	freshly filled source arrays
			int month	0-based month (0-11), range checked by the caller
			const char *path	file the month was parsed from (<= 269 chars)

		OUTPUT
			FamDudCache[month], tagged with the file path, loaded = TRUE

		SUBROUTINES
			None
*/
static void FamDudCopyIn(struct NoiseParams *noiseP, int month, const char *path) {

    int i, j, k;

    for (i = 0; i < 6; i++)
        for (j = 0; j < 16; j++)
            for (k = 0; k < 29; k++) FamDudCache[month].fakp[i][j][k] = noiseP->fakp[i][j][k];

    for (j = 0; j < 6; j++)
        for (k = 0; k < 2; k++) FamDudCache[month].fakabp[j][k] = noiseP->fakabp[j][k];

    for (i = 0; i < 5; i++)
        for (j = 0; j < 12; j++)
            for (k = 0; k < 5; k++) FamDudCache[month].dud[i][j][k] = noiseP->dud[i][j][k];

    for (j = 0; j < 12; j++)
        for (k = 0; k < 14; k++) FamDudCache[month].fam[j][k] = noiseP->fam[j][k];

    // InFilePath and the cache field are the same size, and the path was
    // length-checked when it was built.
    strcpy(FamDudCache[month].path, path);
    FamDudCache[month].loaded = TRUE;

}

int ReadFamDud(
    struct NoiseParams *noiseP,
    const char* DataFilePath,
    int month
) {
    /*
    Read the harmonized coefficient files from Peter Suessman.
    The files have been renamed from Suessman's implementation to COEFFXX.TXT 
    from itucofXX.txt, where XX is a two-digit number for the month.
    The names were changed so the binary names and the text file names 
    correspond.
    For this implementation of ITU-R P.533-12, the only values that are used
    from these coefficient files are the arrays fakp[][], fakabp[][], dud[][][]
    and fam[][] for the calculation of the atmospheric noise.

    The file actually opened is <DataFilePath>[/]COEFF<mm>W.txt, where mm
    is month + 1 with two digits; a '/' is inserted only if DataFilePath
    does not already end in '/' or '\\'.

    File format (fixed layout, as read by the code): one header line; then
    1563 lines of other coefficient sets that are skipped (if2/xf2,
    ifm3/xfm3, ie/xe, iesu/xesu, ies/xes, iels/xels, ihpo1/xhpo1,
    ihpo2/xhpo2, ihp/xhp); then four blocks, each introduced by one label
    line and holding whitespace separated numbers, five per line, with a
    shorter last line:
        fakp(29,16,6)  2784 values (556 lines of 5 + 1 of 4)
                       -> fakp[6][16][29]
        fakabp(2,6)    12 values (2 lines of 5 + 1 of 2) -> fakabp[6][2]
        dud(5,12,5)    300 values (60 lines of 5)        -> dud[5][12][5]
        fam(14,12)     168 values (33 lines of 5 + 1 of 3) -> fam[12][14]
    The values are stored in file order with the last C index varying
    fastest (Fortran-style dimensions in the labels are reversed).

    Parsed months are cached per month and per file path (see
    FamDudCache above); a repeat call copies from the cache without opening
    the file.

        INPUT
            struct NoiseParams *noiseP  arrays allocated by
                                        AllocateNoiseMemory()
            const char *DataFilePath    directory holding the COEFF files
            int month                   0-based month (0 = January). A value
                                        outside 0-11 is not rejected: the
                                        file name is still built from it and
                                        the result is not cached.

        OUTPUT
            noiseP->fam
            noiseP->dud
            noiseP->fakp
            noiseP->fakabp

            Returns
                RTN_READFAMDUDOK      success (from file or cache)
                RTN_ERROPENCOEFFFILE  path too long or file not found
                RTN_ERRREADCOEFFFILE  file truncated or a line did not parse
                RTN_ERRALLOCATEFAKP, RTN_ERRALLOCATEFAKABP,
                RTN_ERRALLOCATEDUD, RTN_ERRALLOCATEFAM
                                      temporary buffer allocation failed
            On failure the arrays may hold a partial month and are not
            cached. Error messages are printed to stdout.

        SUBROUTINES
            FamDudCopyOut(), FamDudCopyIn()
     */

	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

    int i, j, k;
    int n;

    /*
    A is the array that is read into from the file and aids in reshaping 
    the target arrays in the Coeff structure.
    */
    double* A = NULL;

    // Returned through the fail label: a read or parse failure, unless an
    // allocation failure sets its own code first.
    int retval = RTN_ERRREADCOEFFFILE;

    char line[256];

    char CoeffFile[22];
    char InFilePath[270];

    FILE *fp;

    // Bounded join. P372 is the lower layer and cannot call P533's
    // BuildDataPath(), so the same rule is applied inline here: insert the
    // separator only when the caller's directory does not already end in one.
    {
        size_t dirlen = strlen(DataFilePath);
        const char *sep = (dirlen == 0 || DataFilePath[dirlen-1] == '/'
                           || DataFilePath[dirlen-1] == '\\') ? "" : "/";
        sprintf(CoeffFile, "COEFF%02dW.txt", month + 1);
        if ((size_t)snprintf(InFilePath, sizeof(InFilePath), "%s%s%s",
                             DataFilePath, sep, CoeffFile) >= sizeof(InFilePath)) {
            printf("ReadFamDud: ERROR Data file path too long\n");
            return RTN_ERROPENCOEFFFILE;
        }
    }

    // Already parsed this month from this file: copy it out and skip the file.
    if (month >= 0 && month < 12 && FamDudCache[month].loaded == TRUE
        && strcmp(FamDudCache[month].path, InFilePath) == 0) {
        FamDudCopyOut(noiseP, month);
        return RTN_READFAMDUDOK;
    }

    fp = fopen(InFilePath, "r");
    if (fp == NULL) {
        printf("ReadFamDud: ERROR Can't find input file - %s\n", InFilePath);

        return RTN_ERROPENCOEFFFILE;
    }

    // Read the first header line.
    if (fgets(line, 256, fp) == NULL) goto fail;

    //*************************************************************************
    // Skip if2(10) & xf2(13,76,2)

    for (n = 0; n < 400; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip ifm3(10) & xfm3(9,49,2)

    for (n = 0; n < 181; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip ie(10) & xe(9,22,2)
    for (n = 0; n < 84; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip iesu(10) & xesu(5,55,2)

    for (n = 0; n < 114; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip ies(10) & xes(7,61,2)

    for (n = 0; n < 175; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip iels(10) & xels(5,55,2)

    for (n = 0; n < 114; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip ihpo1(10) & xhpo1(13,29,2)

    for (n = 0; n < 155; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // Skip ihpo2(10) & xhpo2(9,55,2)

    for (n = 0; n < 202; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // ihp(10) & xhp(9,37,2)

    for (n = 0; n < 138; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
    }

    //*************************************************************************
    // fakp(29,16,6)

    // Allocate the array A that will allow for reshaping
    A = (double*)malloc(29 * 16 * 6 * sizeof(double));
    if (A == NULL) {
        retval = RTN_ERRALLOCATEFAKP;
        goto fail;
    }

    // Read the line "fakp(29,16,6)"
    if (fgets(line, 256, fp) == NULL) goto fail;

    // Read 556 lines into the array A
    for (n = 0; n < 556; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
        if (sscanf(
            line,
            " %lf %lf %lf %lf %lf\n",
            A + 5 * n,
            A + 5 * n + 1,
            A + 5 * n + 2,
            A + 5 * n + 3,
            A + 5 * n + 4
        ) != 5) goto fail;
    }
    // Read the last partial line
    if (fgets(line, 256, fp) == NULL) goto fail;
    if (sscanf(
        line,
        " %lf %lf %lf %lf\n",
        A + 5 * n,
        A + 5 * n + 1,
        A + 5 * n + 2,
        A + 5 * n + 3
    ) != 4) goto fail;

    // Reshape A into the Coeff structure
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 16; j++) {
            for (k = 0; k < 29; k++) {
                noiseP->fakp[i][j][k] = *(A + 16 * 29 * i + 29 * j + k);
            }
        }
    }

    // Free A
    free(A);
    A = NULL;
    //*************************************************************************
    // fakabp(2,6)

    // Allocate the array A that will allow for reshaping
    A = (double*)malloc(2 * 6 * sizeof(double));
    if (A == NULL) {
        retval = RTN_ERRALLOCATEFAKABP;
        goto fail;
    }

    // Read the line "fakabp(2,6)"
    if (fgets(line, 256, fp) == NULL) goto fail;

    // Read 2 lines into the array A
    for (n = 0; n < 2; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
        if (sscanf(
            line,
            " %lf %lf %lf %lf %lf\n",
            A + 5 * n,
            A + 5 * n + 1,
            A + 5 * n + 2,
            A + 5 * n + 3,
            A + 5 * n + 4
        ) != 5) goto fail;
    }
    // Read the last partial line
    if (fgets(line, 256, fp) == NULL) goto fail;
    if (sscanf(
        line,
        " %lf %lf\n",
        A + 5 * n,
        A + 5 * n + 1
    ) != 2) goto fail;

    // Reshape A into the Coeff structure
    for (j = 0; j < 6; j++) {
        for (k = 0; k < 2; k++) {
            noiseP->fakabp[j][k] = *(A + 2 * j + k);
        }
    }

    // Free A
    free(A);
    A = NULL;
    //*************************************************************************
    // dud(5,12,5)

    // Allocate the array A that will allow for reshaping.
    A = (double*)malloc(5 * 12 * 5 * sizeof(double));
    if (A == NULL) {
        retval = RTN_ERRALLOCATEDUD;
        goto fail;
    }

    // Read the line "dud(5,12,5)".
    if (fgets(line, 256, fp) == NULL) goto fail;

    // Read 60 lines into the array A.
    for (n = 0; n < 60; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
        if (sscanf(
            line,
            " %lf %lf %lf %lf %lf\n",
            A + 5 * n,
            A + 5 * n + 1,
            A + 5 * n + 2,
            A + 5 * n + 3,
            A + 5 * n + 4
        ) != 5) goto fail;
    }

    // Reshape A into the Coeff structure.
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 12; j++) {
            for (k = 0; k < 5; k++) {
                noiseP->dud[i][j][k] = *(A + 5 * 12 * i + 5 * j + k);
            }
        }
    }

    // Free A
    free(A);
    A = NULL;

    //*************************************************************************
    // fam(14,12)

    // Allocate the array A that will allow for reshaping.
    A = (double*)malloc(12 * 14 * sizeof(double));
    if (A == NULL) {
        retval = RTN_ERRALLOCATEFAM;
        goto fail;
    }

    // Read the line "fam(14,12)".
    if (fgets(line, 256, fp) == NULL) goto fail;

    // Read 33 lines into the array A.
    for (n = 0; n < 33; n++) {
        if (fgets(line, 256, fp) == NULL) goto fail;
        if (sscanf(
            line,
            " %lf %lf %lf %lf %lf\n",
            A + 5 * n,
            A + 5 * n + 1,
            A + 5 * n + 2,
            A + 5 * n + 3,
            A + 5 * n + 4
        ) != 5) goto fail;
    }
    // Read the last partial line.
    if (fgets(line, 256, fp) == NULL) goto fail;
    if (sscanf(
        line,
        " %lf %lf %lf\n",
        A + 5 * n,
        A + 5 * n + 1,
        A + 5 * n + 2
    ) != 3) goto fail;

    // Reshape A into the Coeff structure.
    for (j = 0; j < 12; j++) {
        for (k = 0; k < 14; k++) {
            noiseP->fam[j][k] = *(A + 14 * j + k);
        }
    }

    // Free A
    free(A);
    A = NULL;

    // Clean up;
    fclose(fp);

    // Seed the cache from the freshly parsed month. This call used to sit on
    // the hit path above, where it could never run, so the cache never loaded.
    if (month >= 0 && month < 12) FamDudCopyIn(noiseP, month, InFilePath);

    return RTN_READFAMDUDOK;

    // A short or malformed file, or a failed allocation. noiseP may hold a
    // partial month, so it is reported and deliberately not cached.
fail:
    free(A);
    fclose(fp);
    if (retval == RTN_ERRREADCOEFFFILE) {
        printf("ReadFamDud: ERROR Truncated or malformed file - %s\n", InFilePath);
    }
    return retval;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}

char const* P372Version(void) {
    /*
    Return the version of the P533 DLL.
    (The string returned is P372VER from Noise.h, i.e. the P372 library
    version.)

        INPUT
           None

        OUTPUT
           Returns a pointer to the version character string.

        SUBROUTINES
            None
    */

    return P372VER;
}

char const* P372CompileTime(void) {
    /*
    Return the compile time of the P533 DLL.
    (The string returned is P372CT, the __TIMESTAMP__ of this file's
    compilation, i.e. of the P372 library.)

        INPUT
            None

        OUTPUT
            Returns a pointer to the P372 Compile Time string.

        SUBROUTINES
            None
    */

    return P372CT;
}

void AtmosphericNoise_LT(
    struct NoiseParams *noiseP,
    struct FamStats *FamS,
    int lrxmt,
    double rlng,
    double rlat,
    double frequency
) {
    /*
    This is a utility subroutine used to generate the atmospheric noise figures
    in Rec. P.372-14.
    This is a modification of AtmosphericNoise() above that uses the local time
    as the input and outputs the full statistics of atmospheric noise.

        INPUT
            struct NoiseParams *noiseP This is used solely to pass in the arrays for the atmospheric noise
            struct FamStats *FamS  receives the results
            int lrxmt     Local time (0-23); wrapped once if in -24..47
            double rlng   (rad)
            double rlat   (rad)
            double frequency (MHz)

        Unlike AtmosphericNoise() there is no UTC-to-local conversion. The
        same next-block linear power interpolation is used, applied to all
        six statistics (the standard deviations are also interpolated as
        if they were powers in dB).

        OUTPUT
            FamS->FA  Atmospheric noise (dB above kT0b)
            FamS->Du  Upper decile deviation of atmospheric noise
            FamS->Dl  Lower decile deviation of atmospheric noise
            FamS->SigmaFam  Standard deviation of values, Fam
            FamS->SigmaDu   Standard deviations of values of Du
            FamS->SigmaDl   Standard deviations of values of Dl
            FamS->tmblk     set to 99 (not meaningful on return)
            noiseP is only read.

        SUBROUTINES
            GetFamParameters()

    This routine is based on portions of the REC533() routines: 
    GENFAM(), GENOIS1(), ANOIS1() and NOISY().
     */

    double slp; // Interpolation factor
    double fa; // Linear noise power above kTB in 1 MHz

    struct FamStats FS_now; //
    struct FamStats FS_adj; //

    // Roll over the local time, lrxmt, if necessary
    if (lrxmt < 0) {
        lrxmt += 24;
    }
    else if (lrxmt > 23) {
        lrxmt -= 24;
    }
    /*
    The atmospheric noise is determined by
        i) finding the atmospheric noise at the current time block at the 
           reciever local mean time,
        ii) finding the noise for the "adjacent" time block and
        iii) iterating between the two noise values.
    There are 6 time blocks so the modulo 6 keeps the indexes inbounds.
    */
    FS_now.tmblk = (lrxmt / 4) % 6;
    FS_adj.tmblk = (FS_now.tmblk + 1) % 6;

    GetFamParameters(noiseP, &FS_now, rlng, rlat, frequency);
    GetFamParameters(noiseP, &FS_adj, rlng, rlat, frequency);

    // Interpolate is based on the local reciever mean time, lrxmt,
    // and the 4 hour timeblock.
    slp = fmod(lrxmt, 4.0) / 4.0;

    // Load the  return structure 
    fa = pow(10.0, (FS_now.FA / 10.0))
       + (pow(10.0, (FS_adj.FA / 10.0)) - pow(10.0, (FS_now.FA / 10.0))
         ) * slp;
    FamS->FA = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.Du / 10.0))
       + (pow(10.0, (FS_adj.Du / 10.0)) - pow(10.0, (FS_now.Du / 10.0))
         ) * slp;
    FamS->Du = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.Dl / 10.0))
       + (pow(10.0, (FS_adj.Dl / 10.0)) - pow(10.0, (FS_now.Dl / 10.0))
         ) * slp;
    FamS->Dl = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.SigmaDl / 10.0))
       + (pow(10.0, (FS_adj.SigmaDl / 10.0)) 
          - pow(10.0, (FS_now.SigmaDl / 10.0))
         ) * slp;
    FamS->SigmaDl = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.SigmaDu / 10.0))
       + (pow(10.0, (FS_adj.SigmaDu / 10.0))
          - pow(10.0, (FS_now.SigmaDu / 10.0))
         ) * slp;
    FamS->SigmaDu = 10.0 * log10(fa);

    fa = pow(10.0, (FS_now.SigmaFam / 10.0))
       + (pow(10.0, (FS_adj.SigmaFam / 10.0))
          - pow(10.0, (FS_now.SigmaFam / 10.0))
         ) * slp;
    FamS->SigmaFam = 10.0 * log10(fa);

    // The time block for in the FamS structure is irrelevant to return
    // so set it to 99 as an indicator.
    FamS->tmblk = 99;

    return;
}

/* BEGIN Windows __stdcall Interface routines to the Noise.c routines. */
#ifdef _WIN32
    /* 
    All these silly functions do is allow __stdcall to access __cdel functions
    so that the P372.dll can interface to Windows programs like Excel.
    Each _Name() below passes its arguments unchanged to Name() and
    returns its result; parameters, units and return codes are those of
    the wrapped routine. Exceptions: _P372CompileTime() and _P372Version()
    discard the wrapped call's result and return P372CT / P372VER directly.
    Compiled only on _WIN32.
    */
    int __stdcall _AllocateNoiseMemory(
        struct NoiseParams* noiseP
    ) {
        int retval = AllocateNoiseMemory(
            noiseP
        );
        return retval;
    }

int __stdcall _FreeNoiseMemory(
        struct NoiseParams *noiseP
    ) {
        int retval = FreeNoiseMemory(
            noiseP
        );
        return retval;
    }

int __stdcall _Noise(
        struct NoiseParams *noiseP,
        int hour,
        double lng,
        double lat,
        double frequency
    ) {
        int retval = Noise(
            noiseP,
            hour,
            lng,
            lat,
            frequency
        );
        return retval;
    }

int __stdcall _ReadFamDud(
        struct NoiseParams *noiseP,
        const char *DataFilePath,
        int month
    ) {
        int retval = ReadFamDud(
            noiseP,
            DataFilePath,
            month
        );
        return retval;
    }

void __stdcall _InitializeNoise(
        struct NoiseParams *noiseP
    ) {
        InitializeNoise(noiseP);
    }

char const *__stdcall _P372CompileTime(void) {
        P372CompileTime();
        return P372CT;
    }

char  const *__stdcall _P372Version(void) {
        P372Version();
        return P372VER;
    }

void __stdcall _AtmosphericNoise(
        struct NoiseParams *noiseP,
        int iutc,
        double lng,
        double lat,
        double frequency
    ) {
        AtmosphericNoise(
            noiseP,
            iutc,
            lng,
            lat,
            frequency
        );
    }

void __stdcall _AtmosphericNoise_LT(
        struct NoiseParams *noiseP,
        struct FamStats* FamS,
        int lrxmt,
        double lng,
        double lat,
        double frequency
    ) {
        AtmosphericNoise_LT(
            noiseP,
            FamS,
            lrxmt,
            lng,
            lat,
            frequency
        );
    }

int __stdcall _MakeNoise(
        int month,
        int hour,
        double lat,
        double lng,
        double freq,
        double mmnoise,
        char *datafilepath,
        double *out,
        int pntflag
    ) {
        int retval = MakeNoise(
            month,
            hour,
            lat,
            lng,
            freq,
            mmnoise,
            datafilepath,
            out,
            pntflag
        );
        return retval;
    }
#endif
/* END Windows __stdcall Interface routines to the Noise.c routines. */
