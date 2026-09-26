#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

int ValidatePath(struct PathData *path) {

	/*

		ValidatePath() - Checks the user-supplied inputs in the path structure before P533()
			calculates anything. It implements no equation of the Recommendation; the limits
			are those of this implementation (for example 1 - 30 MHz, the HF band the
			ionospheric maps and the method are used for).

		INPUT
		struct PathData *path
			year (1900 - 2100), month (0-based index 0 - 11), hour (0 - 23; path->hour = h
			means h:00 UTC), noiseP.ManMadeNoise (one of the CITY ... QUIET category flags,
			a figure in the open interval (100, 200), or any negative value to skip the
			man-made noise), SSN (R12 >= 0, no upper limit), Modulation (ANALOG or DIGITAL),
			frequency (1 - 30 MHz), BW (0.005 - 3e6 Hz), txpower (-30 - 60 dB(1 kW)),
			SNRr and SIRr (-30 - 200 dB), F0 and T0 (0 - 1000; frequency and time spread at -10 dB),
			A (0 - 1000 dB),
			TW (0 - 50 ms), FW (0 - 1000 Hz), L_tx and L_rx (|lat| <= PI/2, |lng| <= PI
			radians), SNRXXp (1 - 99 %), and the array pointers foF2, M3kF2, foF2var,
			noiseP.dud, noiseP.fam, A_tx.pattern and A_rx.pattern (must be non-NULL).

		OUTPUT
		returns RTN_VALIDDATAOK (17) when every check passes, otherwise the code of the
		first failing check, in the order tested: RTN_ERRYEAR (100), RTN_ERRMONTH (101),
		RTN_ERRHOUR (102), RTN_ERRMANMADENOISE (103), RTN_ERRNOFOF2DATA (104),
		RTN_ERRNOM3KF2DATA (105), RTN_ERRNODUDDATA (106), RTN_ERRNOFAMDATA (107),
		RTN_ERRNOFOF2VARDATA (108), RTN_ERRSSN (109), RTN_ERRMODULATION (110),
		RTN_ERRFREQUENCY (111), RTN_ERRBW (112), RTN_ERRTXPOWER (113), RTN_ERRSNRR (114),
		RTN_ERRSIRR (115), RTN_ERRF0 (116), RTN_ERRT0 (117), RTN_ERRA (118), RTN_ERRTW (119),
		RTN_ERRFW (120), RTN_ERRLTX (121), RTN_ERRLRX (122), RTN_ERRRXANTENNAPATTERN (123),
		RTN_ERRTXANTENNAPATTERN (124), RTN_ERRSNRXXP (125).
		The path structure is not modified. NaN in any range-checked double is rejected.

		SUBROUTINES
		None

		*/

	if ((path->year < 1900) || (path->year > 2100))						return RTN_ERRYEAR;
	if ((path->month < 0) || (path->month > 11))						return RTN_ERRMONTH;
	if ((path->hour < 0) || (path->hour > 23))							return RTN_ERRHOUR;

	// There are two conditions on the path->ManMadeNoise
	//	1) if path->ManMadeNoise is positive 
	//			Then the value is either a flag or a single value > 100 and < 200
	//  2) if path->ManMadeNoise is negative 
	//			Then the user desires to cancel the noise calculation and and value is valid
	// A NaN or infinite figure would satisfy neither branch and be accepted.
	if (!isfinite(path->noiseP.ManMadeNoise))							return RTN_ERRMANMADENOISE;
	if (path->noiseP.ManMadeNoise > 0.0) {
		// Note: the three clauses below were once ANDed together, which made the
		// test unsatisfiable - no value is both under 100 and over 200 - so every
		// out-of-range figure was accepted silently. Reject anything that is
		// neither one of the named categories nor a figure in the open interval
		// (100, 200).
		if (((path->noiseP.ManMadeNoise != CITY) && (path->noiseP.ManMadeNoise != RESIDENTIAL) && (path->noiseP.ManMadeNoise != RURAL)
			&& (path->noiseP.ManMadeNoise != QUIETRURAL) && (path->noiseP.ManMadeNoise != QUIET) && (path->noiseP.ManMadeNoise != NOISY))
			&& ((path->noiseP.ManMadeNoise <= 100.0) || (path->noiseP.ManMadeNoise >= 200.0)))
																			return RTN_ERRMANMADENOISE;
	}

	if (path->foF2 == NULL)												return RTN_ERRNOFOF2DATA;
	if (path->M3kF2 == NULL)											return RTN_ERRNOM3KF2DATA;
	if (path->noiseP.dud == NULL)										return RTN_ERRNODUDDATA;
	if (path->noiseP.fam == NULL)										return RTN_ERRNOFAMDATA;
	if (path->foF2var == NULL)											return RTN_ERRNOFOF2VARDATA;
	// The upper bound of 311 was removed upstream (no_limits, ae38e22): it is
	// not a limit of the method. foF2 and M(3000)F2 clamp R12 to MAXSSN themselves
	// in IonosphericParameters() (P.533-14 section 3.4 and P.1239-4 section 3.1);
	// nothing else in P.533 limits it.
	// R12 = 0 is valid: P.533-14 section 3.4 gives the maps for R12 from 0 up.
	// The floating-point range checks below are written as !(lo <= x && x <= hi)
	// rather than (lo > x) || (x > hi): every comparison with NaN is false, so
	// the second form accepted NaN, which later reached (int) conversions (UB).
	if (0 > path->SSN)												return RTN_ERRSSN;
	if ((path->Modulation != DIGITAL) && (path->Modulation != ANALOG))	return RTN_ERRMODULATION;
	if (!((1.0 <= path->frequency) && (path->frequency <= 30.0)))				return RTN_ERRFREQUENCY;
	if (!((0.005 <= path->BW) && (path->BW <= 3e6)))							return RTN_ERRBW;
	if (!((-30.0 <= path->txpower) && (path->txpower <= 60)))				return RTN_ERRTXPOWER;
	if (!((-30.0 <= path->SNRr) && (path->SNRr <= 200)))						return RTN_ERRSNRR;
	if (!((-30.0 <= path->SIRr) && (path->SIRr <= 200)))						return RTN_ERRSIRR;
	if (!((0.0 <= path->F0) && (path->F0 <= 1000)))							return RTN_ERRF0;
	if (!((0.0 <= path->T0) && (path->T0 <= 1000)))							return RTN_ERRT0;
	if (!((0.0 <= path->A) && (path->A <= 1000)))							return RTN_ERRA;
	if (!((0.0 <= path->TW) && (path->TW <= 50.0)))							return RTN_ERRTW;
	if (!((0.0 <= path->FW) && (path->FW <= 1000)))							return RTN_ERRFW;
	if (!((fabs(path->L_tx.lat) <= PI / 2.0) && (fabs(path->L_tx.lng) <= PI)))	return RTN_ERRLTX;
	if (!((fabs(path->L_rx.lat) <= PI / 2.0) && (fabs(path->L_rx.lng) <= PI)))	return RTN_ERRLRX;
	if (path->A_rx.pattern == NULL)										return RTN_ERRRXANTENNAPATTERN;
	if (path->A_tx.pattern == NULL)										return RTN_ERRTXANTENNAPATTERN;
	if (!((1 <= path->SNRXXp) && (path->SNRXXp <= 99)))					 	return RTN_ERRSNRXXP;

	// path data valid
	return RTN_VALIDDATAOK;

}
