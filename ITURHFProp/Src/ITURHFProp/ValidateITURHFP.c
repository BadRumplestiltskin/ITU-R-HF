#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

// Local includes
#include "Common.h"
#include "P533.h"
#include "ITURHFProp.h"
// End local includes


int ValidateITURHFP(struct ITURHFProp ITURHFP) {

	int i;
	
	// Written as !(lo <= x && x <= hi) so that NaN, which fails every
	// comparison, is rejected: as (x < lo) || (x > hi) it passed, and a NaN
	// bearing reached ReadType13() as (int)NaN, an index outside the pattern.
	#define OUTSIDE(x, lo, hi) (!(((lo) <= (x)) && ((x) <= (hi))))
	// A bearing is checked only when the user gave it. With TX2RX main()
	// derives both from the path ends, which P533's ValidatePath() checks;
	// a bad end (NaN latitude, say) must be reported as that, not as the
	// bearing it produced.
	if(ITURHFP.AntennaOrientation != TX2RX) {
		if(OUTSIDE(ITURHFP.TXBearing, 0.0, 2.0*PI))	return RTN_ERRTXBEARING;
		if(OUTSIDE(ITURHFP.RXBearing, 0.0, 2.0*PI))	return RTN_ERRRXBEARING;
	}
	if(OUTSIDE(ITURHFP.TXGOS, TINYDB, 60.0))		return RTN_ERRTXGOS;
	if(OUTSIDE(ITURHFP.RXGOS, TINYDB, 60.0))		return RTN_ERRRXGOS;

	if(OUTSIDE(ITURHFP.L_LL.lat, -PI/2.0, PI/2.0))	return RTN_ERRLLLAT;
	if(OUTSIDE(ITURHFP.L_LR.lat, -PI/2.0, PI/2.0))	return RTN_ERRLRLAT;
	if(OUTSIDE(ITURHFP.L_UL.lat, -PI/2.0, PI/2.0))	return RTN_ERRULLAT;
	if(OUTSIDE(ITURHFP.L_UR.lat, -PI/2.0, PI/2.0))	return RTN_ERRURLAT;

	if(OUTSIDE(ITURHFP.L_LL.lng, -PI, PI))			return RTN_ERRLLLNG;
	if(OUTSIDE(ITURHFP.L_LR.lng, -PI, PI))			return RTN_ERRLRLNG;
	if(OUTSIDE(ITURHFP.L_UL.lng, -PI, PI))			return RTN_ERRULLNG;
	if(OUTSIDE(ITURHFP.L_UR.lng, -PI, PI))			return RTN_ERRURLNG;
	#undef OUTSIDE

	// Lower/upper checks
	if((ITURHFP.L_LL.lat > ITURHFP.L_UL.lat) || (ITURHFP.L_LL.lat > ITURHFP.L_UR.lat))			return RTN_ERRLL;
	if((ITURHFP.L_LR.lat > ITURHFP.L_UL.lat) || (ITURHFP.L_LR.lat > ITURHFP.L_UR.lat))			return RTN_ERRLR;
	if((ITURHFP.L_UL.lat < ITURHFP.L_LL.lat) || (ITURHFP.L_UL.lat < ITURHFP.L_LR.lat))			return RTN_ERRUL;
	if((ITURHFP.L_UR.lat < ITURHFP.L_LL.lat) || (ITURHFP.L_UR.lat < ITURHFP.L_LR.lat))			return RTN_ERRUR;

	// Left/right checks
	if((ITURHFP.L_LL.lng > ITURHFP.L_LR.lng) || (ITURHFP.L_LL.lng > ITURHFP.L_UR.lng))			return RTN_ERRLL;
	if((ITURHFP.L_LR.lng < ITURHFP.L_UL.lng) || (ITURHFP.L_LR.lng < ITURHFP.L_LL.lng))			return RTN_ERRLR;
	if((ITURHFP.L_UL.lng > ITURHFP.L_LR.lng) || (ITURHFP.L_UL.lng > ITURHFP.L_UR.lng))			return RTN_ERRUL;
	if((ITURHFP.L_UR.lng < ITURHFP.L_UL.lng) || (ITURHFP.L_UR.lng < ITURHFP.L_LL.lng))			return RTN_ERRUR;

	// Make sure the user is asking for a box.
	if(ITURHFP.L_LL.lat != ITURHFP.L_LR.lat)													return RTN_ERRLLAT;
	if(ITURHFP.L_UL.lat != ITURHFP.L_UR.lat)													return RTN_ERRULAT;
	if(ITURHFP.L_LL.lng != ITURHFP.L_UL.lng)													return RTN_ERRLLNG;
	if(ITURHFP.L_LR.lng != ITURHFP.L_UR.lng)													return RTN_ERRRLNG;

	// Area increments. ITURHFProp() divides the span by each increment and
	// casts the point count to int, so it must be finite and positive, and
	// the count must fit in an int. A single-point run divides a zero span,
	// so zero is rejected there too (0/0 is NaN).
	if(!(isfinite(ITURHFP.latinc) && (ITURHFP.latinc > 0.0)) ||
	   !(fabs(ITURHFP.L_UL.lat - ITURHFP.L_LR.lat)/ITURHFP.latinc < (double)INT_MAX - 1.0)) {
		printf("ValidateITURHFP: Error latinc is %g (deg); it must be greater than 0 and give fewer than %d latitude points\n", ITURHFP.latinc*R2D, INT_MAX);
		return RTN_ERRLATINC;
	}
	if(!(isfinite(ITURHFP.lnginc) && (ITURHFP.lnginc > 0.0)) ||
	   !(fabs(ITURHFP.L_LR.lng - ITURHFP.L_LL.lng)/ITURHFP.lnginc < (double)INT_MAX - 1.0)) {
		printf("ValidateITURHFP: Error lnginc is %g (deg); it must be greater than 0 and give fewer than %d longitude points\n", ITURHFP.lnginc*R2D, INT_MAX);
		return RTN_ERRLNGINC;
	}

	if ((ITURHFP.AntennaOrientation != MANUAL) && (ITURHFP.AntennaOrientation != TX2RX))		return RTN_ERRANTENNAORN;

	// Hour, frequency and month lists. ReadInputConfiguration() sets each
	// list's length; a list the input file never gave is empty, and a run
	// without it would do nothing and still report success. Every entry must
	// be in range: ITURHFProp() runs them all. Hours and months are held as
	// typed, 1-based.
	if(ITURHFP.ihrend == 0) {
		printf("ValidateITURHFP: Error Path.hour is missing\n");
		return RTN_ERRHOUR;
	}
	for(i=0; i<ITURHFP.ihrend; i++) {
		if((ITURHFP.hrs[i] < 1) || (ITURHFP.hrs[i] > 24)) {
			printf("ValidateITURHFP: Error Path.hour entry %d is %d; hours must be 1 to 24\n", i+1, ITURHFP.hrs[i]);
			return RTN_ERRHOUR;
		}
	}
	if(ITURHFP.ifrqend == 0) {
		printf("ValidateITURHFP: Error Path.frequency is missing\n");
		return RTN_ERRFREQUENCY;
	}
	for(i=0; i<ITURHFP.ifrqend; i++) {
		if(!((ITURHFP.frqs[i] >= 1.0) && (ITURHFP.frqs[i] <= 30.0))) {
			printf("ValidateITURHFP: Error Path.frequency entry %d is %g; frequencies must be 1 to 30 MHz\n", i+1, ITURHFP.frqs[i]);
			return RTN_ERRFREQUENCY;
		}
	}
	if(ITURHFP.imnthend == 0) {
		printf("ValidateITURHFP: Error Path.month is missing\n");
		return RTN_ERRMONTH;
	}
	for(i=0; i<ITURHFP.imnthend; i++) {
		if((ITURHFP.months[i] < 1) || (ITURHFP.months[i] > NMBOFMONTHS)) {
			printf("ValidateITURHFP: Error Path.month entry %d is %d; months must be 1 to 12\n", i+1, ITURHFP.months[i]);
			return RTN_ERRMONTH;
		}
	}

	return RTN_VALIDATEITURHFPOK;
}



