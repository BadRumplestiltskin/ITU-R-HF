#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

/*
 * These routines use the ionospheric data from the monthly median parameter maps that are generated internally to the REC533() 
 * program. It is hoped that the routines in this program which use ionospheric maps can then take advantage of more contemporary 
 * data sometime in the future. Be aware that the ionospheric maps used here are 1.5-degree resolution maps and it may take some
 * effort to make these routines take advantage of arbitrary resolution maps if and when they become available.
 */

int ReadIonParametersTxt(struct PathData *path, char DataFilePath[256], int silent) {
	/*
	 * ReadIonParametersTxt() is a routine to read ionospheric parameters from a file into arrays necessary for the ITU-R P.533 
	 *		calculation engine. All of the input data here that is "hard coded" will be passed presumably to the final version
	 *		of the P.533 engine.
	 *
	 *	This routine reads Peter Suessman's ionospheric parameters from the program iongrid.
	 *	The file that is read is a text file. Once the file is read, the result is stored in two arrays. foF2 and M3kF2
	 *	that will be passed to the ITU HFProp engine to the propagation prediction
	 *	The arrays are of the format
	 *
	 *			foF2[hour][longitude][latitude][SSN] & M3kF2[hour][longitude][latitude][SSN]
	 *
	 *				where
	 *
	 *					hour =		0 to 23
	 *					longitude = 0 to 240 in 1.5-degree increments from 180 degrees West
	 *					latitude =	0 to 120 in 1.5-degree increments from 90 degrees South
	 *					SSN =		0 to 1 - 0 and 100 12-month smoothed sun spot number
	 *
	 *	The eccentricities of how the input file was created are based on P.1239 and Suessman's code, which is based
	 *	on work of several administrations. These ionospheric parameter files are based on CCIR spherical harmonic coefficients from 
	 *	the 1958 Geophysical year. Please refer to P.1239 for details on how to convert between the coefficients and foF2 and M(3000)F2
	 *
	 *	These are the monthly median foF2 and M(3000)F2 for R12 = 0 and 100 of P.533-14 section 3.4
	 *	(numerical maps of Recommendation ITU-R P.1239), held on a 1.5-degree grid; linear
	 *	interpolation in R12 and bilinear interpolation in position are done elsewhere
	 *	(CalculateCPParameters()).
	 *
	 *	File format: "ionosMM.txt", MM = month + 1 (01 - 12), in DataFilePath. The foF2 map and then
	 *	the M(3000)F2 map, each ordered R12 (0, 100), then longitude (241, from 180 W), then latitude
	 *	(121, from 90 S), each grid point being its 24 hourly values on six lines of 5, 3, 5, 3, 5 and
	 *	3 numbers. The hour index h is the one the engine uses with path->hour (h:00 UTC).
	 *
	 *		INPUT
	 *			struct PathData *path - path->month (0-based) selects the file; path->foF2 and
	 *				path->M3kF2 must already be allocated (AllocatePathMemory())
	 *			char DataFilePath[256] - directory holding the file (a separator is added if needed)
	 *			int silent - TRUE suppresses the progress messages
	 *	
	 *		OUTPUT
	 *			Data is read into the arrays path->foF2 (MHz) and path->M3kF2
	 *			returns RTN_READIONPARAOK (14), or RTN_ERRREADIONPARAMETERS (141) if the path is too
	 *			long, the file cannot be opened, or it is truncated or malformed (the maps are then
	 *			partly filled)
	 *
	 */

	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

	int		j, k, m;
	int		p, r, h, i, n;		// Map, line, hour, value and count indices
	float	v[5];				// Values on one line
	int		/*hrs,*/ lng, lat, ssn; // Temp gridmap maxima

	int linelen = 300;
	char line[300];

	char InFilePath[256];
	char MapFile[32];
	
	FILE *fp;
	
	// At present the path structure is not used but is passed in so that it can select the correct map file based on month

	// The dimensions of the array are fixed by Suessman's file generating program "iongrid"
	// Eventually it would be nice if these were not fixed values so that other resolutions could be used. 
	//hrs = 24;	// 24 hours
	lng = 241;	// 241 longitudes at 1.5-degree increments
	lat = 121;	// 121 latitudes at 1.5-degree increments
	ssn = 2;	// 2 SSN (12-month smoothed sun spot numbers) high and low
	
	// This may require error handling at some point.
	// Eventually you want the file that is indicated by GUIConfig to be opened.
	sprintf(MapFile, "ionos%02d.txt", path->month+1);
	if (BuildDataPath(InFilePath, sizeof(InFilePath), DataFilePath, MapFile) != TRUE) {
		printf("ReadIonParameters: ERROR Data file path too long\n");
		return RTN_ERRREADIONPARAMETERS;
	}
	fp = fopen(InFilePath, "r"); 
	//fp = fopen("..\\..\\ionmap\\ionos04.txt", "r"); 
	if(fp == NULL) {
		printf("ReadIonParameters: ERROR Can't find input file %s\n", InFilePath);
		return RTN_ERRREADIONPARAMETERS;
	}

    if(silent != TRUE) {
		printf("ReadIonParameters: Reading file ionos%02d.txt for ionospheric parameters\n", path->month+1);
		printf("ReadIonParameters: Reading foF2 into array\n");
	}

    // Read in foF2, then M3kF2. Each grid point is 24 hours of data from the
	// Dambolt/Seussman ionospheric atlas file, on six lines of 5, 3, 5, 3, 5
	// and 3 values. Every line is checked: a truncated or corrupt file used to
	// leave the rest of the maps unset and still return OK.
	for (p = 0; p < 2; p++) {
		float ****map = (p == 0) ? path->foF2 : path->M3kF2;

		if ((p == 1) && (silent != TRUE)) {
			printf("ReadIonParameters: Reading M3kF2 into array\n");
		}

		for(m = 0; m < ssn; m++) { // SSN
			for(j = 0; j < lng; j++) { // Longitude
				for(k = 0; k < lat; k++) { // Latitude
					for (h = 0, r = 0; r < 6; r++) {
						n = (r % 2 == 0) ? 5 : 3;
						if ((fgets(line, linelen, fp) == NULL) ||
							(sscanf(line, "%f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4]) < n)) {
							fclose(fp);
							printf("ReadIonParameters: ERROR Truncated or malformed file %s\n", InFilePath);
							return RTN_ERRREADIONPARAMETERS;
						}
						for (i = 0; i < n; i++, h++) map[h][j][k][m] = v[i];
					}
				}
			}
		}
	}

    // Close the file and return.
	fclose(fp);

	return RTN_READIONPARAOK;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}


/*
	Ionospheric map cache.

	Each month's maps are 24 x 241 x 121 x 2 floats for foF2 and the same again
	for M(3000)F2 -- 10.7 MB a month -- and reading one costs about 6 ms. Once
	parsed they are read-only: P533() only ever reads path->foF2 and
	path->M3kF2. So one copy per month can be shared by every circuit needing it.

	Holding all twelve months costs 128 MB, and that is the point: it is a fixed
	ceiling, unlike buffering the input, which grows with the number of circuits.
	A batch can therefore stream its input in file order and still read each
	month at most once. Months load lazily, so a run touching three holds three.

	Not thread safe, in keeping with the rest of the engine.
*/
static struct {
	float ****foF2;
	float ****M3kF2;
	char  path[512];	// DataFilePath the month was read from
	int   loaded;
} IonMapCache[12];

static float ****AllocIonMap(void) {

	/*
		AllocIonMap() - Allocates one zero-filled [IONMAPHRS][IONMAPLNG][IONMAPLAT][IONMAPSSN]
		float map for the cache and returns it, or NULL if any allocation fails (nothing is
		then held).

		One contiguous block for the 698,544 floats, plus three small blocks of
		pointers indexing into it, rather than 700,000 separate allocations of
		two floats each. The jagged form costs more in allocator headers than in
		data: measured, it took 402 MB to cache twelve months where the data is
		only 134 MB.

		The [i][j][k][m] indexing the engine uses is unchanged.
	*/

	int i, j;
	float ****m;
	float *data;
	float **lvl3;
	float ***lvl2;

	m    = (float ****)calloc(IONMAPHRS, sizeof(float ***));
	lvl2 = (float ***) calloc((size_t)IONMAPHRS*IONMAPLNG, sizeof(float **));
	lvl3 = (float **)  calloc((size_t)IONMAPHRS*IONMAPLNG*IONMAPLAT, sizeof(float *));
	data = (float *)   calloc((size_t)IONMAPHRS*IONMAPLNG*IONMAPLAT*IONMAPSSN, sizeof(float));

	if (m == NULL || lvl2 == NULL || lvl3 == NULL || data == NULL) {
		free(m); free(lvl2); free(lvl3); free(data);
		return NULL;
	}

	// Wire the pointer levels to their slices of the block.
	for (i = 0; i < IONMAPHRS; i++) {
		m[i] = lvl2 + (size_t)i*IONMAPLNG;
		for (j = 0; j < IONMAPLNG; j++) {
			m[i][j] = lvl3 + ((size_t)i*IONMAPLNG + j)*IONMAPLAT;
		}
	}
	for (i = 0; i < IONMAPHRS; i++) {
		for (j = 0; j < IONMAPLNG; j++) {
			int k;
			for (k = 0; k < IONMAPLAT; k++) {
				m[i][j][k] = data + (((size_t)i*IONMAPLNG + j)*IONMAPLAT + k)*IONMAPSSN;
			}
		}
	}

	return m;

}

static void FreeIonMap(float ****m) {

	/*
		FreeIonMap() - Releases a map made by AllocIonMap(); NULL is accepted. Must not be
		given a map from AllocatePathMemory(), whose layout is different (FreeIonMaps()).

		Mirrors AllocIonMap(). Four blocks were allocated and each is reachable
		from the first entry of the level above, so they are released innermost
		first: the float data, then the level-3, level-2 and level-1 pointers.
	*/

	if (m == NULL) return;

	if (m[0] != NULL) {
		if (m[0][0] != NULL) {
			free(m[0][0][0]);	// the contiguous float data
			free(m[0][0]);		// the level-3 pointer block
		}
		free(m[0]);				// the level-2 pointer block
	}
	free(m);					// the level-1 pointer block

}

/*
	IonMapGet() - Returns a month's maps, reading them only on first request.

		The arrays belong to the cache. A caller may point path->foF2 and
		path->M3kF2 at them but must not free them; call IonMapFree() once at
		the end of the run instead.

		The cache is keyed on month and DataFilePath. Asking for a month from a
		different directory re-reads it and frees the other directory's copy,
		which invalidates pointers earlier returned for that month.

		INPUT
			int month (0 - 11), char *DataFilePath, int silent

		OUTPUT
			*foF2 and *M3kF2 point at the cached maps
			returns RTN_READIONPARAOK (14), or RTN_ERRREADIONPARAMETERS (141) for a month outside
			0 - 11, a NULL argument, a DataFilePath of 512 characters or more, an allocation
			failure or a failed read (ReadIonParametersBin()); on error *foF2 and *M3kF2 are not
			written and any cached month is kept

		SUBROUTINES
			AllocIonMap(), FreeIonMap(), ReadIonParametersBin()
*/
DLLEXPORT int IonMapGet(int month, char *DataFilePath, int silent,
                        float *****foF2, float *****M3kF2) {

	int retval;
	float ****newfoF2;
	float ****newM3kF2;

	if (month < 0 || month > 11 || foF2 == NULL || M3kF2 == NULL || DataFilePath == NULL) {
		return RTN_ERRREADIONPARAMETERS;
	}
	// The path is the cache key, so it has to fit the key field.
	if (strlen(DataFilePath) >= sizeof(IonMapCache[month].path)) {
		printf("IonMapGet: ERROR Data file path too long\n");
		return RTN_ERRREADIONPARAMETERS;
	}

	// A hit needs the same month from the same directory. Keying on the month
	// alone returned the first directory's maps to a caller naming another.
	if (IonMapCache[month].loaded != TRUE || strcmp(IonMapCache[month].path, DataFilePath) != 0) {

		// Read into fresh maps, so a failed read leaves any cached month intact.
		newfoF2  = AllocIonMap();
		newM3kF2 = AllocIonMap();
		if (newfoF2 == NULL || newM3kF2 == NULL) {
			FreeIonMap(newfoF2);
			FreeIonMap(newM3kF2);
			printf("IonMapGet: ERROR Out of memory caching month %d\n", month+1);
			return RTN_ERRREADIONPARAMETERS;
		}

		retval = ReadIonParametersBin(month, newfoF2, newM3kF2, DataFilePath, silent);
		if (retval != RTN_READIONPARAOK) {
			FreeIonMap(newfoF2);
			FreeIonMap(newM3kF2);
			return retval;
		}

		// Replacing another directory's month frees it: pointers a caller got
		// for this month from that directory are no longer valid.
		FreeIonMap(IonMapCache[month].foF2);
		FreeIonMap(IonMapCache[month].M3kF2);
		IonMapCache[month].foF2  = newfoF2;
		IonMapCache[month].M3kF2 = newM3kF2;
		strcpy(IonMapCache[month].path, DataFilePath);	// length checked above
		IonMapCache[month].loaded = TRUE;
	}

	*foF2  = IonMapCache[month].foF2;
	*M3kF2 = IonMapCache[month].M3kF2;

	return RTN_READIONPARAOK;

}

/*
	IonMapFree() - Releases every cached month.

		INPUT
			None

		OUTPUT
			None

		SUBROUTINES
			FreeIonMap()
*/
DLLEXPORT void IonMapFree(void) {

	int m;

	for (m = 0; m < 12; m++) {
		FreeIonMap(IonMapCache[m].foF2);
		FreeIonMap(IonMapCache[m].M3kF2);
		IonMapCache[m].foF2 = NULL;
		IonMapCache[m].M3kF2 = NULL;
		IonMapCache[m].loaded = FALSE;
		IonMapCache[m].path[0] = '\0';
	}

}

/*
	IonBinFailed() - Releases what ReadIonParametersBin() holds when a read
		fails and returns the error. free(NULL) is harmless.

		INPUT
			FILE *fp - the open map file (closed here)
			float *readBuffer - the read buffer or NULL (freed here)
			char *InFilePath - the file's path, for the message

		OUTPUT
			returns RTN_ERRREADIONPARAMETERS (141)
*/
static int IonBinFailed(FILE *fp, float *readBuffer, char *InFilePath) {
	printf("ReadIonParameters: ERROR %s is truncated or unreadable\n", InFilePath);
	free(readBuffer);
	fclose(fp);
	return RTN_ERRREADIONPARAMETERS;
}

int ReadIonParametersBin(int month, float ****foF2, float ****M3kF2, char DataFilePath[256], int silent) {
	/*
	 * ReadIonParametersBin() is a routine to read ionospheric parameters from a file into arrays necessary for the ITU-R P.533 
	 *		calculation engine. All of the input data here that is "hard coded" will be passed presumably to the final version
	 *		of the P.533 engine.
	 *
	 *	This routine reads Peter Suessman's ionospheric parameters from the program iongrid.
	 *	The file that is read is a binary file. Once the file is read, the result is stored in two arrays, foF2 and M3kF2,  
	 *	that will be passed to the ITU HFProp engine to the propagation prediction
	 *	The arrays are of the format
	 *
	 *			foF2[hour][longitude][latitude][SSN] & M3kF2[hour][longitude][latitude][SSN]
	 *
	 *				where
	 *
	 *					hour =		0 to 23
	 *					longitude = 0 to 240 in 1.5-degree increments from 180 degrees West
	 *					latitude =	0 to 120 in 1.5-degree increments from 90 degrees South
	 *					SSN =		0 to 1 - 0 and 100 12-month smoothed sun spot number
	 *
	 *	The eccentricities of how the input file was created are based on P.1239 and Suessman's code, which is based
	 *	on work of several administrations. These ionospheric parameter files are based on CCIR spherical harmonic coefficients from 
	 *	the 1958 Geophysical year. Please refer to P.1239 for details on how to convert between the coefficients and foF2 and M(3000)F2
	 *
	 *	These are the monthly median foF2 and M(3000)F2 for R12 = 0 and 100 of P.533-14 section 3.4
	 *	(numerical maps of Recommendation ITU-R P.1239) on a 1.5-degree grid.
	 *
	 *	File format: "ionosMM.bin", MM = month + 1 (01 - 12), in DataFilePath, written by Fortran
	 *	as unformatted records: 5 bytes of record overhead, 24 x 241 x 121 x 2 foF2 values as
	 *	4-byte floats in the reading machine's byte order, 10 bytes (the end of the foF2 record and
	 *	the start of the M(3000)F2 record), then the same number of M(3000)F2 floats. Within each
	 *	block the order is R12 (0, 100), longitude (from 180 W), latitude (from 90 S), hour (fastest).
	 *	The hour index h is the one the engine uses with path->hour (h:00 UTC).
	 *
	 *		INPUT
	 *			int month - 0-based month index (selects the file)
	 *			float ****foF2, float ****M3kF2 - allocated [24][241][121][2] maps to fill
	 *			char DataFilePath[256] - directory holding the file (a separator is added if needed)
	 *			int silent - TRUE suppresses the progress messages
	 *	
	 *		OUTPUT
	 *			data is read into the arrays foF2 (MHz) and M3kF2
	 *			returns RTN_READIONPARAOK (14), or RTN_ERRREADIONPARAMETERS (141) if the path is too
	 *			long, the file cannot be opened, memory runs out, or the file is short (foF2 may then
	 *			be filled while M3kF2 is not)
	 *
	 */
	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

	char buffer[256];

	int	i, j, k, m;
	int	hrs, lng, lat, ssn; // Temp gridmap maxima
	int numfoF2;

	//warning C4189: 'linelen': local variable is initialized but not referenced
	//int linelen = 300;

	float * readBuffer;

	char InFilePath[256];
	char MapFile[32];
	
	FILE *fp;
	
	// At present the path structure is not used but is passed in so that it can select the correct map file based on month.

	// The dimensions of the array are fixed by Suessman's file generating program "iongrid".
	// Eventually it would be nice if these were not fixed values so that other resolutions could be used. 
	hrs = 24;	// 24 hours
	lng = 241;	// 241 longitudes at 1.5-degree increments
	lat = 121;	// 121 latitudes at 1.5-degree increments
	ssn = 2;	// 2 SSN (12-month smoothed sun spot numbers) high and low
	numfoF2 = hrs * lng * lat * ssn;
	// This may require error handling at some point.
	// Eventually you want the file that is indicated by GUIConfig to be opened.
	sprintf(MapFile, "ionos%02d.bin", month+1);
	if (BuildDataPath(InFilePath, sizeof(InFilePath), DataFilePath, MapFile) != TRUE) {
		printf("ReadIonParameters: ERROR Data file path too long\n");
		return RTN_ERRREADIONPARAMETERS;
	}
	fp = fopen(InFilePath, "rb"); 
	if(fp == NULL) {
		printf("ReadIonParameters: ERROR Can't find input file %s\n", InFilePath);
		return RTN_ERRREADIONPARAMETERS;
	}

    if(silent != TRUE) {
		printf("ReadIonParameters: Reading file ionos%02d.txt for ionospheric parameters\n", month+1);
	}

    //The first 5 bytes of the file are overhead that FORTRAN puts in 
	// Every read is checked: a short file would otherwise leave the maps holding
	// uninitialised heap memory, which the caller caches for the whole run.
	readBuffer = (float *) malloc(sizeof(float) * numfoF2);
	if ((readBuffer == NULL) || (fread(&buffer, sizeof(char), 5, fp) != 5) ||
		(fread(readBuffer, sizeof(float), numfoF2, fp) != (size_t)numfoF2)) {
		return IonBinFailed(fp, readBuffer, InFilePath);
	}

	if(silent != TRUE) {
		printf("ReadIonParameters: Reading foF2 (binary) into array\n");
	}

    // Read in foF2
	for(m = 0; m < ssn; m++) { // SSN
		for(j = 0; j < lng; j++) { // Longitude
			for(k = 0; k < lat; k++) { // Latitude
				// Read 24 hours of data from the Dambolt/Seussman ionospheric atlas file
				for(i = 0; i < hrs; i++) { // Latitude
					foF2[i][j][k][m] = readBuffer[ (m * (lng * lat * hrs)) + 
													(j * (lat * hrs)) +
													(k * (hrs)) +
													 i];
					// fread(&foF2[i][j][k][m], sizeof(float), 1, fp);
				}
            }
        }
    }

    free(readBuffer);

	// The next 5 bytes are the tail of the foF2 record followed by 5 bytes of header for the M(3000)F2 record.
	if (fread(&buffer, sizeof(char), 10, fp) != 10) return IonBinFailed(fp, NULL, InFilePath);
	
	if(silent != TRUE) {
		printf("ReadIonParameters: Reading M3kF2 (binary) into array\n");
	}

    readBuffer = (float *) malloc(sizeof(float) * numfoF2);
	if ((readBuffer == NULL) || (fread(readBuffer, sizeof(float), numfoF2, fp) != (size_t)numfoF2)) {
		return IonBinFailed(fp, readBuffer, InFilePath);
	}

	// Read in M3kF2
	for(m = 0; m < ssn; m++) { // SSN
		for(j = 0; j < lng; j++) { // Longitude
			for(k = 0; k < lat; k++) { // Latitude
				// Read 24 hours of data from the Dambolt/Seussman ionospheric atlas file.
				for(i = 0; i < hrs; i++) { // Latitude
					M3kF2[i][j][k][m] = readBuffer[ (m * (lng * lat * hrs)) + 
													(j * (lat * hrs)) +
													(k * (hrs)) +
													 i];
					// fread(&M3kF2[i][j][k][m], sizeof(float), 1, fp);
				}
            }
        }
    }

    free(readBuffer);

	// Close the file and return.
	fclose(fp);

	return RTN_READIONPARAOK;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}
