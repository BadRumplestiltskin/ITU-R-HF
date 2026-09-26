#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

static int FreeAntenna(struct Antenna *ant, int retval);

/*
 * Allocates the Antenna structure (Part of the PathData struct).  This 
 * function is called when the antenna types have been defined which in
 * turn define the dimensions of the required data structure.
 * Only 360 azimuths by 91 elevations is accepted. Any existing pattern is
 * freed first, so the antenna must not hold uninitialised pointers.
 */
P533_API int AllocateAntennaMemory(struct Antenna *ant, int freqn, int azin, int elen) {
	int m, n;

	// The engine indexes every pattern as [freq][0..359][0..90] and
	// FreeAntenna() releases 360 azimuths, so no other shape is usable.
	// Rejected before anything is touched, so an existing pattern survives.
	if ((freqn < 1) || (azin != 360) || (elen != 91)) return RTN_ERRALLOCATEANT;

	// Release any pattern already here; re-allocating used to leak it. The
	// antenna must be empty or allocated: AllocatePathMemory() sets it empty.
	FreeAntenna(ant, 0);

	// Built on the antenna as it goes, so on failure FreeAntenna() can release
	// the partial structure and leave the antenna empty rather than half-set.
	ant->freqn = freqn;
	ant->freqs = (double *) malloc(freqn * sizeof(double));
	ant->pattern = (double ***) calloc(freqn, sizeof(double **));
	if ((ant->freqs == NULL) || (ant->pattern == NULL)) return FreeAntenna(ant, RTN_ERRALLOCATEANT);
	for (m=0; m < freqn; m++) {
		ant->pattern[m] = (double **) calloc(azin, sizeof(double *));
		if (ant->pattern[m] == NULL) return FreeAntenna(ant, RTN_ERRALLOCATEANT);
		for (n=0; n<azin; n++) {
			ant->pattern[m][n] = (double*) malloc(elen * sizeof(double));
			if (ant->pattern[m][n] == NULL) return FreeAntenna(ant, RTN_ERRALLOCATEANT);
		}
	}

	return RTN_ALLOCATEP533OK;

}



/*
	FreeAntenna() - Releases an antenna pattern and returns retval.

		NULL-tolerant at every level, and leaves the antenna empty (NULL
		pointers, freqn 0), so it is safe on an antenna that was never
		allocated, was partly allocated, or has already been freed.

		INPUT
			struct Antenna *ant, int retval

		OUTPUT
			returns retval

*/
static int FreeAntenna(struct Antenna *ant, int retval) {

	int m, n;

	if (ant->pattern != NULL) {
		for (m=0; m < ant->freqn; m++) {
			if (ant->pattern[m] == NULL) continue;
			for (n=0; n<360; n++) free(ant->pattern[m][n]);		// 360 azimuths
			free(ant->pattern[m]);
		}
		free(ant->pattern);
		ant->pattern = NULL;
	}
	free(ant->freqs);
	ant->freqs = NULL;
	ant->freqn = 0;

	return retval;

}



/*
	FreefoF2var() - Releases the foF2 variability array, NULL-tolerant at every
		level, and leaves path->foF2var NULL so a second call is harmless.

*/
static void FreefoF2var(struct PathData *path) {

	int i, j, k, m;

	if (path->foF2var != NULL) {
		for (i=0; i<3; i++) {					// season
			if (path->foF2var[i] == NULL) continue;
			for (j=0; j<24; j++) {				// hours
				if (path->foF2var[i][j] == NULL) continue;
				for (k=0; k<19; k++) {			// latitude
					if (path->foF2var[i][j][k] == NULL) continue;
					for (m=0; m<3; m++) free(path->foF2var[i][j][k][m]);
					free(path->foF2var[i][j][k]);
				}
				free(path->foF2var[i][j]);
			}
			free(path->foF2var[i]);
		}
		free(path->foF2var);
		path->foF2var = NULL;
	}

}



/*
	AllocFailed() - Releases a partially built path and returns the error.

		AllocatePathMemory() used to return on a failed allocation without
		freeing what it had already built, and left the caller no way to know
		whether FreePathMemory() was safe to call. The arrays are now attached
		to the path as each is completed, so this releases whatever exists and
		leaves the path safe either to discard or to free again.

		INPUT
			struct PathData *path, int retval

		OUTPUT
			returns retval

		SUBROUTINES
			FreeIonMaps()
*/
static int AllocFailed(struct PathData *path, int retval) {

	FreeIonMaps(path);		// foF2 and M3kF2, NULL-tolerant
	FreefoF2var(path);

	return retval;

}

P533_API int AllocatePathMemory(struct PathData *path) {
	
	/*

	  AllocatePathMemory() - Allocates the memory necessary for the path structure. The data must be read into these structures elsewhere.
	 
	 		INPUT
	 			struct PathData *path	
	 		
	 		OUTPUT
	 			path->foF2
	 			path->M3kF2
	 			path->foF2var
	 			path->dud
	 			path->fam 
	 
	 		SUBROUTINES
	 			None
	 
	 */

	float ****foF2;			// foF2 ionospheric map
	float ****M3kF2;		// M(3000)F2 ionospheric map
	double *****foF2var;	// foF2 statistics

	int retval;
	int hrs, lng, lat, ssn;
	int i, j, k, m;
	int season;
	int decile;

	/*
	 * Allocate the ionospheric parameter arrays that will be used by the P533 engine.
	 */
	hrs = 24;	// 24 hours
	lng = 241;	// 241 longitudes at 1.5 degree increments
	lat = 121;	// 121 latitudes at 1.5 degree increments
	ssn = 2;	// 2 SSN (12-month smoothed sun spot numbers) high and low

	/* 
	 * Create the foF2 array so you can pass it into the core P.533 process.
	 */
	// calloc() so a partly built array is all-NULL below the point of failure,
	// and every result is checked before it is indexed. The checks further down
	// that test foF2/M3kF2/foF2var against NULL could never fire: the arrays
	// were already dereferenced here, several levels deep, before reaching them.
	foF2 = (float****) calloc(hrs, sizeof(float***));
	if (foF2 == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2);
	for (i=0; i<hrs; i++) {
		foF2[i] = (float***) calloc(lng, sizeof(float**));
		if (foF2[i] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2);
		for (j=0; j<lng; j++) {
			foF2[i][j] = (float**) calloc(lat, sizeof(float*));
			if (foF2[i][j] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2);
			for (k=0; k<lat; k++) {
				foF2[i][j][k] = (float*) calloc(ssn, sizeof(float));
				if (foF2[i][j][k] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2);
			}
        }
    }

	path->foF2 = foF2;

    /* 
     * Create the M(3000)F2 array so you can pass it into the core P.533 process.
     */
	M3kF2 = (float****) calloc(hrs, sizeof(float***));
	if (M3kF2 == NULL) return AllocFailed(path, RTN_ERRALLOCATEM3KF2);
	for (i=0; i<hrs; i++) {
		M3kF2[i] = (float***) calloc(lng, sizeof(float**));
		if (M3kF2[i] == NULL) return AllocFailed(path, RTN_ERRALLOCATEM3KF2);
		for (j=0; j<lng; j++) {
			M3kF2[i][j] = (float**) calloc(lat, sizeof(float*));
			if (M3kF2[i][j] == NULL) return AllocFailed(path, RTN_ERRALLOCATEM3KF2);
			for (k=0; k<lat; k++) {
				M3kF2[i][j][k] = (float*) calloc(ssn, sizeof(float));
				if (M3kF2[i][j][k] == NULL) return AllocFailed(path, RTN_ERRALLOCATEM3KF2);
			}
        }
    }

	path->M3kF2 = M3kF2;

    /*
     * Allocate the foF2 variablity arrays that will be used by the P533 engine.
     */
	season = 3;	// 3 seasons
				//		1) WINTER 2) EQUINOX 3) SUMMER
	hrs = 24;	// 24 hours  
	lat = 19;	// 19 latitude by 5
				//      0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90
	ssn = 3;	// 3 SSN ranges
				//		1) R12 < 50 2) 50 <= R12 <= 100 3) R12 > 100
	decile = 2;	// 2 deciles 
				//	1) lower 2) upper

	/* 
	 * Create the foF2 array so you can pass it into the core P.533 process.
	 */
	foF2var = (double*****) calloc(season, sizeof(double****));
	if (foF2var == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2VAR);
	for (i=0; i<season; i++) {
		foF2var[i] = (double****) calloc(hrs, sizeof(double***));
		if (foF2var[i] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2VAR);
		for (j=0; j<hrs; j++) {
			foF2var[i][j] = (double***) calloc(lat, sizeof(double**));
			if (foF2var[i][j] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2VAR);
			for (k=0; k<lat; k++) {
				foF2var[i][j][k] = (double**) calloc(ssn, sizeof(double*));
				if (foF2var[i][j][k] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2VAR);
				for (m=0; m<ssn; m++) {
					foF2var[i][j][k][m] = (double*) calloc(decile, sizeof(double));
					if (foF2var[i][j][k][m] == NULL) return AllocFailed(path, RTN_ERRALLOCATEFOF2VAR);
				}
			}
		}
	}

	path->foF2var = foF2var;

	/*
	 * The TX and RX antenna arrays are allocated when parsing the
	 * input files (e.g. ReadType13) as the array size varies with the antenna
	 * type and the number of frequencies for which pattern data is available.
	 *
	 * The arrays are free'd in FreePathMemory.
	 */
	path->A_tx.pattern = NULL;
	path->A_tx.freqs = NULL;
	path->A_tx.freqn = 0;
	path->A_rx.pattern = NULL;
	path->A_rx.freqs = NULL;
	path->A_rx.freqn = 0;

	// The arrays are attached to the path as they are completed, above, so that
	// AllocFailed() can release a partially built structure.

	// Allocate the memory in the noise structure (libp372)
	retval = AllocateNoiseMemory(&path->noiseP);
	if (retval != RTN_ALLOCATEP372OK) {
		return RTN_ERRALLOCATENOISE;
	}

	return RTN_ALLOCATEP533OK;

}



/*
	FreeIonMaps() - Releases just the path's own ionospheric maps.

		AllocatePathMemory() gives each path its own 10.7 MB pair of maps. A
		caller that instead shares the library's month cache (IonMapGet()) calls
		this first to release its private copy, then points path->foF2 and
		path->M3kF2 at the cached arrays. FreePathMemory() skips NULL maps, so
		the cached arrays are never freed through the path.

		INPUT
			struct PathData *path

		OUTPUT
			path->foF2 and path->M3kF2 are freed and set to NULL

		SUBROUTINES
			None
*/
P533_API void FreeIonMaps(struct PathData *path) {

	int i, j, k;

	if (path->foF2 != NULL) {
		for (i=0; i<IONMAPHRS; i++) {
			for (j=0; j<IONMAPLNG; j++) {
				for (k=0; k<IONMAPLAT; k++) free(path->foF2[i][j][k]);
				free(path->foF2[i][j]);
			}
			free(path->foF2[i]);
		}
		free(path->foF2);
		path->foF2 = NULL;
	}

	if (path->M3kF2 != NULL) {
		for (i=0; i<IONMAPHRS; i++) {
			for (j=0; j<IONMAPLNG; j++) {
				for (k=0; k<IONMAPLAT; k++) free(path->M3kF2[i][j][k]);
				free(path->M3kF2[i][j]);
			}
			free(path->M3kF2[i]);
		}
		free(path->M3kF2);
		path->M3kF2 = NULL;
	}

}

P533_API int FreePathMemory(struct PathData *path) {	
	/*

	 	FreePath() - Frees the memory that was dynamically (m) allocated for the structure PathData path
	 
	 		INPUT
	 			struct PathData *path
	 
	 		OUTPUT
	 			void
	 
	 		SUBROUTINES
	 			None
	 
	 */

	int retval;
	int hrs, lng, lat;
	int i, j, k;
	
	/*
	 * Free the ionospheric parameter arrays.
	 */
	hrs = 24;	// 24 hours
	lng = 241;	// 241 longitudes at 1.5 degree increments
	lat = 121;	// 121 latitudes at 1.5 degree increments

	// NULL when the caller released its own maps with FreeIonMaps() and pointed
	// the path at the shared month cache instead; those belong to IonMapFree().
	if (path->foF2 != NULL) {
		for (i=0; i<hrs; i++) {
			for (j=0; j<lng; j++) {
				for (k=0; k<lat; k++) {
					free(path->foF2[i][j][k]);
					}
				free(path->foF2[i][j]);
				}
			free(path->foF2[i]);
		}
		free(path->foF2);
		path->foF2 = NULL;
	}

	if (path->M3kF2 != NULL) {
		for (i=0; i<hrs; i++) {
			for (j=0; j<lng; j++) {
				for (k=0; k<lat; k++) {
					free(path->M3kF2[i][j][k]);
				}
				free(path->M3kF2[i][j]);
			}
			free(path->M3kF2[i]);
		}
		free(path->M3kF2);
		path->M3kF2 = NULL;
	}

	FreefoF2var(path);

	FreeAntenna(&path->A_tx, 0);
	FreeAntenna(&path->A_rx, 0);

	// Free the noise memory
	retval = FreeNoiseMemory(&path->noiseP);
	if (retval != RTN_NOISEFREED) return retval; // check that the input parameters are correct
	
	return RTN_PATHFREED;

}

