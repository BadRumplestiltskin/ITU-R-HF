#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

/*
	P1239Fail() - Closes the decile file, reports it and returns RTN_ERRNOTP12393.

		INPUT
			FILE *fp - the open decile file
			const char *InFilePath - its path, for the message

		OUTPUT
			returns RTN_ERRNOTP12393 (140)
*/
static int P1239Fail(FILE *fp, const char *InFilePath) {
	fclose(fp);
	printf("ReadP1239: ERROR Truncated or malformed decile file\n");
	printf("\t\t<%s>\n", InFilePath);
	return RTN_ERRNOTP12393;
}

int ReadP1239(struct PathData *path, const char * DataFilePath) {

	/*
	 * ReadP1239() - Read the file "P1239-3 Decile Factors.txt", which is Tables 2 and 3 of ITU-R P.1239.
	 *		The data in this file are the decile factors for within-the-month variations of foF2
	 *		(P.1239-4 section 3.2, Table 2 lower decile, Table 3 upper decile), given for the local
	 *		time and geographic latitude at the control point, three ranges of R12 and three
	 *		seasons. They are used for MUF(90)/MUF(50) and MUF(10)/MUF(50) (P.533-14 sections 3.6
	 *		and 3.7).
	 *
	 *		File format (text): two header lines, then for each decile (lower, upper), each season
	 *		(winter, equinox, summer) and each R12 range (R12 < 50, 50 <= R12 <= 100, R12 > 100):
	 *		four title lines followed by 19 lines for latitudes 90, 85, ..., 0 degrees, each a
	 *		latitude label and 24 factors for local times 00 - 23 h.
	 *
	 *			INPUT
	 *				struct PathData *path - path->foF2var must already be allocated (AllocatePathMemory())
	 *				const char *DataFilePath - directory holding the file (a separator is added if needed)
	 *
	 *			OUTPUT
	 *				path->foF2var[season][hour][latitude index 0 - 18 = 0 - 90 degrees][R12 range][decile]
	 *				returns RTN_READP1239OK (15); RTN_ERRCANTOPENP1239FILE (139) if the path is too long
	 *				or the file cannot be opened; RTN_ERRNOTP12393 (140) if it is truncated or a
	 *				latitude line does not hold a label and 24 numbers (foF2var is then partly filled)
	 *
	 */
	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

	int i, k, m, n;
	int /*hrs,*/ season, lat, ssn, decile;
	char line[256]; // There are 80 characters in the line.
	char substr[45]; 
	char InFilePath[256];

	FILE *fp;
	
	if (BuildDataPath(InFilePath, sizeof(InFilePath), DataFilePath, "P1239-3 Decile Factors.txt") != TRUE) {
		printf("ReadP1239: ERROR Data file path too long\n");
		return RTN_ERRCANTOPENP1239FILE;
	}

	fp = fopen (InFilePath, "r");  // Open the file. Home
	if(fp == NULL) {
		printf("ReadP1239: ERROR Can't find input file 'P1239-3 Decile Factors.txt'\n");
		printf("\t\t<%s>\n", InFilePath);
		return RTN_ERRCANTOPENP1239FILE;
	}
	
	season = 3;	// 3 seasons
				//		1) WINTER 2) EQUINOX 3) SUMMER
	//hrs = 24;	// 24 hours  
	lat = 19;	// 19 latitude by 5
				//      0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90
	ssn = 3;	// 3 SSN ranges
				//		1) R12 < 50 2) 50 <= R12 <= 100 3) R12 > 100
	decile = 2;	// 2 deciles 
				//	1) lower 2) upper

	// Every line is checked: a truncated or corrupt file used to leave the
	// remaining factors uninitialised (or repeat the last line) and still
	// return OK.
	for (k = 0; k < 2; k++) {
		if (fgets(line, 256, fp) == NULL) return P1239Fail(fp, InFilePath);
	}

	// Now read numbers for the lower decile.
	for(n=0;n<decile;n++) { // 2 deciles lower and upper
		for(i=0;i<season;i++) { // Three seasons
			for(m=0; m<ssn; m++) { // Three sunspot ranges
				// Read the next four lines of text.
				for (k = 0; k < 4; k++) {
					if (fgets(line, 256, fp) == NULL) return P1239Fail(fp, InFilePath);
				}
				for(k=lat-1;k>=0;k--) {  // 19 latitudes counting backward to make the indices correspond to increasing latitude
					// Read the next latitude line of text
					if (fgets(line, 256, fp) == NULL) return P1239Fail(fp, InFilePath);
					// Scan 1 string latitude and 24 numbers corresponding to hours
					// %44s, not %s: line is 256 bytes and substr is 45, so an
					// over-long first token -- a corrupt or hand-edited decile
					// file -- overran the stack. The latitude labels are a few
					// characters, so the width costs nothing.
					if (sscanf(line, "%44s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf/n", substr,
									&path->foF2var[i][0][k][m][n],  &path->foF2var[i][1][k][m][n],  &path->foF2var[i][2][k][m][n],  &path->foF2var[i][3][k][m][n], 
									&path->foF2var[i][4][k][m][n],  &path->foF2var[i][5][k][m][n],  &path->foF2var[i][6][k][m][n],  &path->foF2var[i][7][k][m][n],
									&path->foF2var[i][8][k][m][n],  &path->foF2var[i][9][k][m][n],  &path->foF2var[i][10][k][m][n], &path->foF2var[i][11][k][m][n], 
									&path->foF2var[i][12][k][m][n], &path->foF2var[i][13][k][m][n], &path->foF2var[i][14][k][m][n], &path->foF2var[i][15][k][m][n], 
									&path->foF2var[i][16][k][m][n], &path->foF2var[i][17][k][m][n], &path->foF2var[i][18][k][m][n], &path->foF2var[i][19][k][m][n], 
									&path->foF2var[i][20][k][m][n], &path->foF2var[i][21][k][m][n], &path->foF2var[i][22][k][m][n], &path->foF2var[i][23][k][m][n]) != 25) {
						return P1239Fail(fp, InFilePath);
					}
				}
			}
		}
	}

	
	fclose(fp);  // Close the file.

	return RTN_READP1239OK;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}

