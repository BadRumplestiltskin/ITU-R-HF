#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
#include "ITURHFProp.h"
// End local includes

// Local prototypes
void substrbtwnchar(char instr[256], char searchchar, char * outstr);
static int ReadList(char *line, double *vals, int max, int integer);
static int ReadScalar(const char *line, double *v);
static double ScalarOrNaN(const char *line);
static int ReadWhole(const char *line, int *out);
static int ReadReportFormat(char *instr, unsigned long *format);
unsigned long OutputOption(const char *optstr);
void InitializeInput(struct ITURHFProp *ITURHFP, struct PathData *path);
// End local prototypes

int ReadInputConfiguration(char InFilePath[256], struct ITURHFProp *ITURHFP, struct PathData *path) {

	// This program reads data from a file and calls ITURHFProp().
	// All inputs are in degrees.

	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

	int i, n;
	double vals[NMBOFFREQS];	// the longest of the three lists

	char line[256];
	char instr[256];

	FILE *fp;

	// Open the file that is specified on the command line.
	fp = fopen(InFilePath, "r");
	if(fp == NULL) {
		printf("ReadingInputConfiguration: ERROR Can't find input file - %s\n", InFilePath);
		return RTN_ERRNOINPUTFILE;
	}

    // Initailize the input values in the structure PathData path and
	// structure ITURHFProp ITURHFP to default values.
	InitializeInput(ITURHFP, path);

	// fgets() in the loop condition: the old read-then-test-feof() loop dropped a
	// last line with no newline, because that read already sets end-of-file.
	while (fgets(line, sizeof(line), fp) != NULL) {

		// Check if the line is a comment or blank.
		if (((line[0] == '/') && (line[1] == '/'))
			||
			((line[0] == ' ') && (line[1] == ' '))) {
			// Ignore line
		}
		else { // Valid line
			// Check for the reserved words
			if (strncmp("PathName", line, 8) == 0) { // char 8 is a space
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', path->name);
			}
            if (strncmp("TXGOS", line, 5) == 0) {
				ITURHFP->TXGOS = ScalarOrNaN(line);
			}
            if (strncmp("RXGOS", line, 5) == 0) {
				ITURHFP->RXGOS = ScalarOrNaN(line);
			}
            if (strncmp("AntennaOrientation", line, 18) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', instr);
				if (strcmp(instr, "TX2RX") == 0) {
					ITURHFP->AntennaOrientation = TX2RX;
				}
				else if ((strcmp(instr, "ARBITRARY") == 0) || (strcmp(instr, "MANUAL") == 0)) {
					ITURHFP->AntennaOrientation = MANUAL;
				}
				else {
					printf("ReadInputConfiguration: ERROR AntennaOrientation \"%s\" is not TX2RX, ARBITRARY or MANUAL\n", instr);
					fclose(fp);
					return RTN_ERRANTENNAORN;
				}
            }
            if (strncmp("TXBearing", line, 5) == 0) {
				ITURHFP->TXBearing = ScalarOrNaN(line);
				ITURHFP->TXBearing = ITURHFP->TXBearing*D2R;
			}
            if (strncmp("RXBearing", line, 5) == 0) {
				ITURHFP->RXBearing = ScalarOrNaN(line);
				ITURHFP->RXBearing = ITURHFP->RXBearing*D2R;
			}
            if (strncmp("PathTXName", line, 10) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', path->txname);
			}
            if (strncmp("Path.L_tx.lat", line, 13) == 0) {
				path->L_tx.lat = ScalarOrNaN(line);
				path->L_tx.lat = path->L_tx.lat*D2R;
			}
            if (strncmp("Path.L_tx.lng", line, 13) == 0) {
				path->L_tx.lng = ScalarOrNaN(line);
				path->L_tx.lng = path->L_tx.lng*D2R;
			}
            if (strncmp("TXAntFilePath", line, 13) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', ITURHFP->TXAntFilePath);
			}
            if (strncmp("PathRXName", line, 10) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', path->rxname);
			}
            if (strncmp("Path.L_rx.lat", line, 13) == 0) {
				path->L_rx.lat = ScalarOrNaN(line);
				path->L_rx.lat = path->L_rx.lat*D2R;
			}
            if (strncmp("Path.L_rx.lng", line, 13) == 0) {
				path->L_rx.lng = ScalarOrNaN(line);
				path->L_rx.lng = path->L_rx.lng*D2R;
			}
            if (strncmp("RXAntFilePath", line, 13) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', ITURHFP->RXAntFilePath);
			}
            if (strncmp("Path.year", line, 9) == 0) {
				if (ReadWhole(line, &path->year) != 0) { fclose(fp); return RTN_ERRYEAR; }
			}
            if (strncmp("Path.month", line, 10) == 0) {
				n = ReadList(line, vals, NMBOFMONTHS, TRUE);
				if (n < 0) { fclose(fp); return RTN_ERRMONTH; }
				for (i = 0; i < n; i++) ITURHFP->months[i] = (int)vals[i];
				ITURHFP->imnthend = n;
            }
            if (strncmp("Path.hour", line, 9) == 0) {
				n = ReadList(line, vals, NMBOFHOURS, TRUE);
				if (n < 0) { fclose(fp); return RTN_ERRHOUR; }
				for (i = 0; i < n; i++) ITURHFP->hrs[i] = (int)vals[i];
				ITURHFP->ihrend = n;
            }
            if (strncmp("Path.SSN", line, 8) == 0) {
				if (ReadWhole(line, &path->SSN) != 0) { fclose(fp); return RTN_ERRSSNVALUE; }
			}
            if (strncmp("Path.frequency", line, 14) == 0) {
				n = ReadList(line, vals, NMBOFFREQS, FALSE);
				if (n < 0) { fclose(fp); return RTN_ERRFREQUENCY; }
				for (i = 0; i < n; i++) ITURHFP->frqs[i] = vals[i];
				ITURHFP->ifrqend = n;
            }
            if (strncmp("Path.txpower", line, 12) == 0) {
				path->txpower = ScalarOrNaN(line);
			}
            if (strncmp("Path.BW", line, 7) == 0) {
				path->BW = ScalarOrNaN(line);
			}
            if (strncmp("Path.SNRr", line, 9) == 0) {
				path->SNRr = ScalarOrNaN(line);
			}
            if (strncmp("Path.SNRXXp", line, 9) == 0) {
				if (ReadWhole(line, &path->SNRXXp) != 0) { fclose(fp); return RTN_ERRSNRXXP; }
			}
            if (strncmp("Path.ManMadeNoise", line, 17) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', instr);
				if (strcmp(instr, "RESIDENTIAL") == 0) {
					path->noiseP.ManMadeNoise = RESIDENTIAL;
				}
				else if (strcmp(instr, "CITY") == 0) {
					path->noiseP.ManMadeNoise = CITY;
				}
				else if (strcmp(instr, "RURAL") == 0) {
					path->noiseP.ManMadeNoise = RURAL;
				}
				else if (strcmp(instr, "QUIETRURAL") == 0) {
					path->noiseP.ManMadeNoise = QUIETRURAL;
				}
				else if (strcmp(instr, "NOISY") == 0) {
					path->noiseP.ManMadeNoise = NOISY;
				}
				else if (strcmp(instr, "QUIET") == 0) {
					path->noiseP.ManMadeNoise = QUIET;
				}
				else { // Not a category: a figure in dB. NaN if it is not a number,
					// which ValidatePath() rejects with RTN_ERRMANMADENOISE.
					path->noiseP.ManMadeNoise = ScalarOrNaN(line);
				}
            }
            if (strncmp("Path.Modulation", line, 15) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', instr);
				if (strcmp(instr, "DIGITAL") == 0) {
					path->Modulation = DIGITAL;
				}
				else if (strcmp(instr, "ANALOG") == 0) {
					path->Modulation = ANALOG;
				}
				else {
					printf("ReadInputConfiguration: ERROR Path.Modulation \"%s\" is not ANALOG or DIGITAL\n", instr);
					fclose(fp);
					return RTN_ERRMODULATION;
				}
            }
            if (strncmp("Path.SIRr", line, 9) == 0) {
				path->SIRr = ScalarOrNaN(line);
			}
            if (strncmp("Path.A", line, 6) == 0) {
				path->A = ScalarOrNaN(line);
			}
            if (strncmp("Path.TW", line, 7) == 0) {
				path->TW = ScalarOrNaN(line);
			}
            if (strncmp("Path.FW", line, 7) == 0) {
				path->FW = ScalarOrNaN(line);
			}
            if (strncmp("Path.T0", line, 7) == 0) {
				path->T0 = ScalarOrNaN(line);
			}
            if (strncmp("Path.F0", line, 7) == 0) {
				path->F0 = ScalarOrNaN(line);
			}
            if (strncmp("Path.SorL", line, 9) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', instr);
				if (strcmp(instr, "SHORTPATH") == 0) {
					path->SorL = SHORTPATH;
				}
				else if (strcmp(instr, "LONGPATH") == 0) {
					path->SorL = LONGPATH;
				}
				else {
					printf("ReadInputConfiguration: ERROR Path.SorL \"%s\" is not SHORTPATH or LONGPATH\n", instr);
					fclose(fp);
					return RTN_ERRSORL;
				}
            }
            if (strncmp("RptFilePath", line, 11) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', ITURHFP->RptFilePath);
			}
            if (strncmp("RptFileFormat", line, 13) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', instr);

				if (ReadReportFormat(instr, &ITURHFP->RptFileFormat) != 0) {
					fclose(fp);
					return RTN_ERRRPTFILEFORMAT;
				}
            }
            if (strncmp("LL.lat", line, 6) == 0) {
				ITURHFP->L_LL.lat = ScalarOrNaN(line);
				ITURHFP->L_LL.lat = ITURHFP->L_LL.lat*D2R;
			}
            if (strncmp("LL.lng", line, 6) == 0) {
				ITURHFP->L_LL.lng = ScalarOrNaN(line);
				ITURHFP->L_LL.lng = ITURHFP->L_LL.lng*D2R;
			}
            if (strncmp("LR.lat", line, 6) == 0) {
				ITURHFP->L_LR.lat = ScalarOrNaN(line);
				ITURHFP->L_LR.lat = ITURHFP->L_LR.lat*D2R;
			}
            if (strncmp("LR.lng", line, 6) == 0) {
				ITURHFP->L_LR.lng = ScalarOrNaN(line);
				ITURHFP->L_LR.lng = ITURHFP->L_LR.lng*D2R;
			}
            if (strncmp("UL.lat", line, 6) == 0) {
				ITURHFP->L_UL.lat = ScalarOrNaN(line);
				ITURHFP->L_UL.lat = ITURHFP->L_UL.lat*D2R;
			}
            if (strncmp("UL.lng", line, 6) == 0) {
				ITURHFP->L_UL.lng = ScalarOrNaN(line);
				ITURHFP->L_UL.lng = ITURHFP->L_UL.lng*D2R;
			}
            if (strncmp("UR.lat", line, 6) == 0) {
				ITURHFP->L_UR.lat = ScalarOrNaN(line);
				ITURHFP->L_UR.lat = ITURHFP->L_UR.lat*D2R;
			}
            if (strncmp("UR.lng", line, 6) == 0) {
				ITURHFP->L_UR.lng = ScalarOrNaN(line);
				ITURHFP->L_UR.lng = ITURHFP->L_UR.lng*D2R;
			}
            // An alternative way to input data is by two points of the analysis rectangle
			// which is more efficient since 4 corner input is redundant
			// The south east corner of the analysis rectangle
			if (strncmp("SE.lat", line, 6) == 0) {
				ITURHFP->L_LR.lat = ScalarOrNaN(line);
				ITURHFP->L_LR.lat = ITURHFP->L_LR.lat*D2R;
			}
            if (strncmp("SE.lng", line, 6) == 0) {
				ITURHFP->L_LR.lng = ScalarOrNaN(line);
				ITURHFP->L_LR.lng = ITURHFP->L_LR.lng*D2R;
			}
            // The north west corner of the analysis rectangle
			if (strncmp("NW.lat", line, 6) == 0) {
				ITURHFP->L_UL.lat = ScalarOrNaN(line);
				ITURHFP->L_UL.lat = ITURHFP->L_UL.lat*D2R;
			}
            if (strncmp("NW.lng", line, 6) == 0) {
				ITURHFP->L_UL.lng = ScalarOrNaN(line);
				ITURHFP->L_UL.lng = ITURHFP->L_UL.lng*D2R;
			}
            // Analysis window increments
			if (strncmp("latinc", line, 6) == 0) {
				ITURHFP->latinc = ScalarOrNaN(line);
				ITURHFP->latinc = ITURHFP->latinc*D2R;
			}
            if (strncmp("lnginc", line, 6) == 0) {
				ITURHFP->lnginc = ScalarOrNaN(line);
				ITURHFP->lnginc = ITURHFP->lnginc*D2R;
			}
            //
			if (strncmp("DataFilePath", line, 12) == 0) {
				// The name is between two quotes-find them.
				substrbtwnchar(line, '\"', ITURHFP->DataFilePath);
			}
        }
	}

    // There are optional ways to enter the analysis rectangle.
	// Determine if the user choose to inpout the analysis rectangle
	// defined by the north west and south east corner
	if ((ITURHFP->L_LR.lat != TOOBIG) && (ITURHFP->L_LR.lng != TOOBIG) &&
		(ITURHFP->L_LL.lat == TOOBIG) && (ITURHFP->L_LL.lng == TOOBIG) &&
		(ITURHFP->L_UR.lat == TOOBIG) && (ITURHFP->L_UR.lng == TOOBIG) &&
		(ITURHFP->L_UL.lat != TOOBIG) && (ITURHFP->L_UL.lng != TOOBIG)) {
		// Option 1: Is used so set the remaining corners
		ITURHFP->L_LL.lat = ITURHFP->L_LR.lat; // South
		ITURHFP->L_LL.lng = ITURHFP->L_UL.lng; // West
		ITURHFP->L_UR.lat = ITURHFP->L_UL.lat; // North
		ITURHFP->L_UR.lng = ITURHFP->L_LR.lng; // East
	}

    fclose(fp);

	return RTN_RICOK;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}


void substrbtwnchar(char instr[256], char searchchar, char * outstr) {

	char * frstchar;
	char * scndchar;

	int n;

	frstchar = strchr(instr, searchchar);
	scndchar = strrchr(instr, searchchar);
	// Needs two distinct delimiters: with one, strchr() and strrchr() agree and n
	// would be -1; with none, both are NULL.
	if((frstchar != NULL) && (scndchar != frstchar)) {
		n =  (int)(&scndchar[0] - &frstchar[0] - sizeof(char));
		strncpy(outstr, &frstchar[0]+sizeof(char), n);
		outstr[n] = '\0';
	}
	else {
		outstr[0] = '\0';
	}

    return;
}

unsigned long OutputOption(const char *optstr) {

	/*
	  OutputOption() - Returns the report bits for one RptFileFormat option.

		The name must match an option exactly. The old test compared
		prefixes, so a misspelling such as "RPT_PRX" or "RPT_ALLMODES" was
		taken silently as RPT_PR or RPT_ALL.

		INPUTS
			const char *optstr - one option name, e.g. "RPT_PR", as defined
				in ITURHFProp.h

		OUTPUT
			returns unsigned long where each bit field represents a
				particular variable or set of variables as defined in
				ITURHFProp.h, or 0 when the name is not an option
	*/

	static const struct {
		const char *name;
		unsigned long bits;
	} options[] = {
		{"RPT_D",				RPT_D},
		{"RPT_DMAX",			RPT_DMAX},
		{"RPT_ELE",				RPT_ELE},
		{"RPT_BMUF",			RPT_BMUF},
		{"RPT_BMUFD",			RPT_BMUFD},
		{"RPT_OPMUF",			RPT_OPMUF},
		{"RPT_OPMUFD",			RPT_OPMUFD},
		{"RPT_N0_F2",			RPT_N0_F2},
		{"RPT_N0_E",			RPT_N0_E},
		{"RPT_E",				RPT_E},
		{"RPT_PR",				RPT_PR},
		{"RPT_NOISESOURCES",	RPT_NOISESOURCES},
		{"RPT_NOISESOURCESD",	RPT_NOISESOURCESD},
		{"RPT_NOISETOTALD",		RPT_NOISETOTALD},
		{"RPT_NOISETOTAL",		RPT_NOISETOTAL},
		{"RPT_SNR",				RPT_SNR},
		{"RPT_SNRD",			RPT_SNRD},
		{"RPT_SNRXX",			RPT_SNRXX},
		{"RPT_SIR",				RPT_SIR},
		{"RPT_SIRD",			RPT_SIRD},
		{"RPT_RSN",				RPT_RSN},
		{"RPT_BCR",				RPT_BCR},
		{"RPT_OCR",				RPT_OCR},
		{"RPT_OCRS",			RPT_OCRS},
		{"RPT_MIR",				RPT_MIR},
		{"RPT_RXLOCATION",		RPT_RXLOCATION},
		{"RPT_DOMMODE",			RPT_DOMMODE},
		{"RPT_GRW",				RPT_GRW},
		{"RPT_ESL",				RPT_ESL},
		{"RPT_LONG",			RPT_LONG},
		{"RPT_ALL",				RPT_ALL},
		{"RPT_DUMPPATH",		RPT_DUMPPATH},
	};
	size_t i;

	for (i = 0; i < sizeof(options)/sizeof(options[0]); i++) {
		if (strcmp(options[i].name, optstr) == 0) {
			return options[i].bits;
		}
	}

	return 0;
}

void InitializeInput(struct ITURHFProp *ITURHFP, struct PathData *path) {

	/*
		InitializeInput = Initializes all of the input parameters that are read by
			ReadInputConfiguration()

			INPUT
				struct ITURHFProp *ITURHFP
				struct PathData *path

			OUTPUT
				none
	*/

	int i;

	sprintf(path->name, "Path Data");
	sprintf(path->txname, "Transmitter");
	path->L_tx.lat = 0.0;
	path->L_tx.lng = 0.0;
	sprintf(path->rxname, "Receiver");
	path->L_rx.lat = 0.0;
	path->L_rx.lng = 0.0;
	path->year = 2022;
	path->SSN = 99;
	path->txpower = 0.0;
	path->BW = 0.0;
	path->SNRr = 0.0;
	path->SNRXXp = 99;
	path->noiseP.ManMadeNoise = NOISY;
	path->Modulation = ANALOG;
	path->SIRr = 0.0;
	path->A = 0.0;
	path->TW = 0.0;
	path->FW = 0.0;
	path->T0 = 0.0;
	path->F0 = 0.0;
	path->SorL= SHORTPATH;

	ITURHFP->TXGOS = 0.0;
	ITURHFP->RXGOS = 0.0;
	ITURHFP->AntennaOrientation = TX2RX;
	ITURHFP->TXBearing = 0.0;
	ITURHFP->RXBearing = 0.0;
	sprintf(ITURHFP->TXAntFilePath, ".");
	sprintf(ITURHFP->RXAntFilePath, ".");
	// The lists start empty; ValidateITURHFP() rejects a run without them.
	for(i=0; i<NMBOFFREQS; i++) ITURHFP->frqs[i] = 0.0;
	for(i=0; i<NMBOFHOURS; i++) ITURHFP->hrs[i] = 0;
	for(i=0; i<NMBOFMONTHS; i++) ITURHFP->months[i] = 0;
	ITURHFP->ifrqend = 0;
	ITURHFP->ihrend = 0;
	ITURHFP->imnthend = 0;
	// Defaulted on every platform. This used to be guarded by #ifdef _WIN32, so
	// on Linux and macOS an input file that omitted RptFilePath left the member
	// holding whatever was on the stack, which was then used to build the output
	// path. ITURHFP is not zeroed by its caller.
	sprintf(ITURHFP->RptFilePath, ".");
	ITURHFP->RptFileFormat = RPT_ALL;
	ITURHFP->L_LL.lat = TOOBIG;
	ITURHFP->L_LL.lng = TOOBIG;
	ITURHFP->L_LR.lat = TOOBIG;
	ITURHFP->L_LR.lng = TOOBIG;
	ITURHFP->L_UL.lat = TOOBIG;
	ITURHFP->L_UL.lng = TOOBIG;
	ITURHFP->L_UR.lat = TOOBIG;
	ITURHFP->L_UR.lng = TOOBIG;
	ITURHFP->latinc = 1.0*D2R;	// 1 degree; a value read from the file is also converted to radians
	ITURHFP->lnginc = 1.0*D2R;
	sprintf(ITURHFP->DataFilePath, ".");

	return;

}


/*
	AntennaFileType() - Reads the antenna type from the fourth line of an open
		VOACAP-style antenna file, e.g.
			  14    [ 2] Antenna Type..: 30 x (efficiency + 91 gain values) follow
		and rewinds the file for the type reader.

		Returns the type, or -1 (after printing why) when the file has fewer
		than four lines or the fourth does not start with an integer. Every
		read was unchecked before, so a short or garbled file left the type
		as whatever was on the stack.
*/
static int AntennaFileType(FILE *fp, const char *name) {

	char line[256];
	int lineCtr;
	int antType;

	for (lineCtr = 0; lineCtr < 4; ++lineCtr) {
		if (fgets(line, sizeof(line), fp) == NULL) {
			printf("ReadAntennaPatterns: Error antenna file %.200s has only %d lines; the antenna type is on line 4\n", name, lineCtr);
			return -1;
		}
	}
	if (sscanf(line, " %d", &antType) != 1) {
		printf("ReadAntennaPatterns: Error antenna file %.200s line 4 does not start with an antenna type\n", name);
		return -1;
	}
	rewind(fp);

	return antType;

}

int ReadAntennaPatterns(struct PathData *path, struct ITURHFProp ITURHFP) {

	#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#endif

	int retval;
    int antType = -1;

    FILE *fp;

	// User feedback
	if(ITURHFP.silent != TRUE) {
		printf("ReadAntennaPatterns: Reading transmit and receive antenna\n");
	}

    // At this point there is only one other antenna pattern file type that can be read.
	// The antenna pattern must be rotated to the correct azimuth before P533() is run.

	// Determine the type of receiver antenna file.
	if(strcmp(ITURHFP.RXAntFilePath, "ISOTROPIC") == 0) { // Isotropic Antenna
		ITURHFP.rxantfp = NULL;
		IsotropicPattern(&path->A_rx, ITURHFP.RXGOS, ITURHFP.silent);
		// Store the name of the antenna to the path structure.
		strcpy(path->A_rx.Name, "ISOTROPIC");
	} else {
	    //If it's not an ISOTROPIC, open the file and take a look...
	    /* VOACAP type antennas identify the antenna type on the forth
	     * line; e.g;
         *   14    [ 2] Antenna Type..: 30 x (efficiency + 91 gain values) follow
         */
	    fp = fopen(ITURHFP.RXAntFilePath, "r");

	    if (fp == NULL) {
            if(ITURHFP.silent != TRUE) {
				printf("Error opening Rx. antenna file %.65s\n", ITURHFP.RXAntFilePath);
			}
            return RTN_ERRCANTOPENRXANTFILE;
	    }

	    antType = AntennaFileType(fp, ITURHFP.RXAntFilePath);
	    if (antType < 0) {
            fclose(fp);
            return RTN_ERRCANTOPENRXANTFILE;
	    }

	    if(antType == 11) {
		    retval = ReadType11(&path->A_rx, fp, ITURHFP.silent);
            fclose(fp);
		    if(retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
            }
		} else if(antType == 13) {
		    retval = ReadType13(&path->A_rx, fp, ITURHFP.RXBearing, ITURHFP.silent);
            fclose(fp);
		    if (retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
		    }
        } else if (antType == 14) {
		    retval = ReadType14(&path->A_rx, fp, ITURHFP.silent);
            fclose(fp);
		    if (retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
		    }
        } else {
            printf("ReadAntennaPatterns: Error unsupported antenna type %d in %.200s\n", antType, ITURHFP.RXAntFilePath);
            fclose(fp);
            return RTN_ERRCANTOPENRXANTFILE;
        }
    } // end of the rx antenna type

	// Determine the type of transmitter antenna file.
	if(strcmp(ITURHFP.TXAntFilePath, "ISOTROPIC") == 0) { // Isotropic Antenna
		ITURHFP.txantfp = NULL;
		IsotropicPattern(&path->A_tx, ITURHFP.TXGOS, ITURHFP.silent);
		// Store the name of the antenna to the path structure.
		strcpy(path->A_tx.Name, "ISOTROPIC");
	} else {
        fp = fopen(ITURHFP.TXAntFilePath, "r");

	    if (fp == NULL) {
			if(ITURHFP.silent != TRUE) {
				printf("Error opening Tx. antenna file %.65s\n", ITURHFP.TXAntFilePath);
			}
            return RTN_ERRCANTOPENTXANTFILE;
	    }
	    antType = AntennaFileType(fp, ITURHFP.TXAntFilePath);
	    if (antType < 0) {
            fclose(fp);
            return RTN_ERRCANTOPENTXANTFILE;
	    }

	    if(antType == 11) {
		    retval = ReadType11(&path->A_tx, fp, ITURHFP.silent);
            fclose(fp);
		    if(retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
            }
		} else if(antType == 13) {
		    retval = ReadType13(&path->A_tx, fp, ITURHFP.TXBearing, ITURHFP.silent);
            fclose(fp);
		    if(retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
            }
		} else if (antType == 14) {
		    retval = ReadType14(&path->A_tx, fp, ITURHFP.silent);
            fclose(fp);
		    if (retval != RTN_READANTENNAPATTERNSOK) {
				    return retval;
		    }
        } else {
			printf("ReadAntennaPatterns: Error unsupported antenna type %d in %.200s\n", antType, ITURHFP.TXAntFilePath);
            fclose(fp);
            return RTN_ERRCANTOPENTXANTFILE;
	    }
    }
	return RTN_READANTENNAPATTERNSOK;

	#ifdef __GNUC__
	#pragma GCC diagnostic pop
	#endif
}


/*
	ReadList() - Parses the comma-separated list after a keyword, e.g.
		"Path.hour 1, 3, 5 // comment", into vals.

		Values may be separated by commas or spaces and the list ends at the
		line end or a "//" comment. Values past max are ignored with a warning.
		Anything that is not a number (or, when integer is TRUE, not a whole
		number in int range) is an error naming the token: the old sscanf loop stopped at it
		and silently dropped it and everything after it.

		INPUT
			char *line - the whole input line, keyword first
			double *vals - receives up to max values
			int max
			int integer - TRUE for months and hours

		OUTPUT
			returns the number of values (at least 1), or -1 on an error

*/
static int ReadList(char *line, double *vals, int max, int integer) {

	char key[64];
	char *p, *end;
	double v;
	int n = 0;

	sscanf(line, "%63s", key);
	p = line + strcspn(line, " \t");	// skip the keyword

	for (;;) {
		p += strspn(p, " \t,");
		if ((*p == '\0') || (*p == '\n') || (*p == '\r') || (*p == '/')) break;
		v = strtod(p, &end);
		if ((end == p) || !isfinite(v) || (integer && ((v != floor(v)) || (v < INT_MIN) || (v > INT_MAX))) ||
			((*end != '\0') && (strchr(" \t,\r\n/", *end) == NULL))) {
			printf("ReadInputConfiguration: ERROR %s has an invalid value \"%.*s\"\n",
				key, (int)strcspn(p, " \t,\r\n"), p);
			return -1;
		}
		if (n < max) {
			vals[n] = v;
		}
		else if (n == max) {
			fprintf(stderr, "ReadInputConfiguration: %s lists more than %d values; the rest are ignored\n", key, max);
		}
		n++;
		p = end;
	}

	if (n == 0) {
		printf("ReadInputConfiguration: ERROR %s has no values\n", key);
		return -1;
	}

	return (n < max) ? n : max;

}


/*
	ReadScalar() - Parses the single number after a keyword, e.g.
		"latinc 1.5 // comment", into *v.

		The number may be followed only by white space or a "//" comment.
		Anything else - no number, a word, trailing text or a second number -
		is an error naming the text. "inf" and "nan" are numbers to strtod()
		and are passed on: the range checks after reading reject them and
		name the value. The old
		unchecked sscanf("%*s %lf") left the default in place on such input,
		and for the degree keywords then multiplied that radian default by
		D2R a second time.

		INPUT
			const char *line - the whole input line, keyword first
			double *v - receives the value

		OUTPUT
			returns 0, or -1 (after printing why) on an error

*/
static int ReadScalar(const char *line, double *v) {

	char key[64];
	const char *p;
	char *end;
	size_t rest;

	sscanf(line, "%63s", key);
	p = line + strcspn(line, " \t");	// skip the keyword
	p += strspn(p, " \t");

	*v = strtod(p, &end);
	rest = strspn(end, " \t\r\n");
	if ((end == p) ||
		((end[rest] != '\0') && (strncmp(&end[rest], "//", 2) != 0))) {
		printf("ReadInputConfiguration: ERROR %s has an invalid value \"%.*s\"\n",
			key, (int)strcspn(p, "\r\n"), p);
		return -1;
	}

	return 0;

}


/*
	ScalarOrNaN() - The number after a keyword, or NaN when ReadScalar()
		rejects it.

		Every floating-point input is range checked afterwards, by
		ValidateITURHFP() or by P533's ValidatePath(), and both reject NaN, so
		an unreadable value is reported with that keyword's own return code.

		INPUT
			const char *line - the whole input line, keyword first

		OUTPUT
			returns the value, or NaN

*/
static double ScalarOrNaN(const char *line) {

	double v;

	return (ReadScalar(line, &v) == 0) ? v : NAN;

}


/*
	ReadWhole() - Parses the whole number after a keyword into *out.

		As ReadScalar(), and the value must also be a whole number in int
		range. The old sscanf("%*s %d") stopped at the first character that
		could not be part of an int, so "Path.SSN 1e12" was read as 1 and
		"Path.year 1985.7" as 1985.

		INPUT
			const char *line - the whole input line, keyword first
			int *out - receives the value; unchanged on an error

		OUTPUT
			returns 0, or -1 (after printing why) on an error

*/
static int ReadWhole(const char *line, int *out) {

	char key[64];
	double v;

	if (ReadScalar(line, &v) != 0) {
		return -1;
	}
	if ((v != floor(v)) || (v < INT_MIN) || (v > INT_MAX)) {
		sscanf(line, "%63s", key);
		printf("ReadInputConfiguration: ERROR %s is %g; it must be a whole number from %d to %d\n",
			key, v, INT_MIN, INT_MAX);
		return -1;
	}
	*out = (int)v;

	return 0;

}


/*
	ReadReportFormat() - Combines the RptFileFormat options, e.g.
		"RPT_D | RPT_PR", into *format.

		Options are separated by '|', spaces or tabs in any mix, so "RPT_D|RPT_PR"
		reads the same as "RPT_D | RPT_PR"; the old scan needed spaces round
		each '|' and silently dropped every option after one without them. An
		option OutputOption() does not know, or an empty list, is an error:
		before, either left the report with no result columns.

		INPUT
			char *instr - the text between the quotes; it is modified
			unsigned long *format - receives the combined option bits

		OUTPUT
			returns 0, or -1 (after printing why) on an error

*/
static int ReadReportFormat(char *instr, unsigned long *format) {

	const char *seps = "| \t";
	char *opt;
	unsigned long bits;

	*format = 0;
	for (opt = strtok(instr, seps); opt != NULL; opt = strtok(NULL, seps)) {
		bits = OutputOption(opt);
		if (bits == 0) {
			printf("ReadInputConfiguration: ERROR RptFileFormat option \"%s\" is unknown\n", opt);
			return -1;
		}
		*format |= bits;
	}
	if (*format == 0) {
		printf("ReadInputConfiguration: ERROR RptFileFormat names no options\n");
		return -1;
	}

	return 0;

}
