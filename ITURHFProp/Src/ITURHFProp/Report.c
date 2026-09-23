#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Local includes
#include "Common.h"
#include "P533.h"
#include "ITURHFProp.h"
// End local includes

// Local #defines
#define DBLFIELD		"% 7.2lf"
#define DBLFIELD1		"% 9.4lf"
#define DBLFIELD2		"% 9.2lf"
#define DBLFIELD3		"% 9.3lf"
#define STRFIELD		"%5s"

#define RFC4180_DBLFIELD		"%.2lf"
#define RFC4180_DBLFIELD1		"%.4lf"
#define RFC4180_DBLFIELD2		"%.2lf"
#define RFC4180_STRFIELD		"%s"

#define PRINT_RFC4180_HEADER 3
#define PRINT_RFC4180_DATA 2
#define PRINT_HEADER	1
#define PRINT_DATA		0
// End local #define

// Local prototypes
void PrintHeader(struct PathData path, struct ITURHFProp ITURHFP);
void PrintRecord(struct PathData path, struct ITURHFProp ITURHFP, int printhr);
void PrintLastRecord(struct PathData path, struct ITURHFProp ITURHFP);
char EW(double lng);
char NS(double lat);
void function_RPT_D(   struct PathData path, int option, int *col);
void function_RPT_ELE( struct PathData path, int option, int *col);
void function_RPT_N0_F2(struct PathData path, int option, int *col);
void function_RPT_N0_E(struct PathData path, int option, int *col);
void function_RPT_SNRXX(struct PathData path, int option, int *col);
void function_RPT_OCRS(struct PathData path, int option, int *col);
void function_RPT_ANTENNA(struct PathData path, int option, int *col);
void function_RPT_DOMMODE(struct PathData path, int option, int *col);
void function_RPT_RXLOCATION(struct PathData path, int option, int *col);
void function_RPT_LONG(struct PathData path, int option, int *col);
// End local prototypes

// Local globals
char *months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
char outstr[256] = "";
FILE *fp; // Temp file pointer for readability
static int Header = TRUE; // The first time you enter this routine the head will need to be printed.
// End local globals

static double rget_distance(const struct PathData *p) { return p->distance; }
static double rget_dmax(const struct PathData *p) { return p->dmax; }
static double rget_ptick(const struct PathData *p) { return p->ptick; }
static double rget_ele_deg(const struct PathData *p) { return p->ele*R2D; }
static double rget_BMUF(const struct PathData *p) { return p->BMUF; }
static double rget_MUF50(const struct PathData *p) { return p->MUF50; }
static double rget_MUF90(const struct PathData *p) { return p->MUF90; }
static double rget_MUF10(const struct PathData *p) { return p->MUF10; }
static double rget_OPMUF(const struct PathData *p) { return p->OPMUF; }
static double rget_OPMUF90(const struct PathData *p) { return p->OPMUF90; }
static double rget_OPMUF10(const struct PathData *p) { return p->OPMUF10; }
static double rget_Ep(const struct PathData *p) { return p->Ep; }
static double rget_Pr(const struct PathData *p) { return p->Pr; }
static double rget_Grw(const struct PathData *p) { return p->Grw; }
static double rget_FaA(const struct PathData *p) { return p->noiseP.FaA; }
static double rget_FaM(const struct PathData *p) { return p->noiseP.FaM; }
static double rget_FaG(const struct PathData *p) { return p->noiseP.FaG; }
static double rget_DuA(const struct PathData *p) { return p->noiseP.DuA; }
static double rget_DlA(const struct PathData *p) { return p->noiseP.DlA; }
static double rget_DuM(const struct PathData *p) { return p->noiseP.DuM; }
static double rget_DlM(const struct PathData *p) { return p->noiseP.DlM; }
static double rget_DuG(const struct PathData *p) { return p->noiseP.DuG; }
static double rget_DlG(const struct PathData *p) { return p->noiseP.DlG; }
static double rget_DuT(const struct PathData *p) { return p->noiseP.DuT; }
static double rget_DlT(const struct PathData *p) { return p->noiseP.DlT; }
static double rget_FamT(const struct PathData *p) { return p->noiseP.FamT; }
static double rget_SNR(const struct PathData *p) { return p->SNR; }
static double rget_DuSN(const struct PathData *p) { return p->DuSN; }
static double rget_DlSN(const struct PathData *p) { return p->DlSN; }
static double rget_SIR(const struct PathData *p) { return p->SIR; }
static double rget_DuSI(const struct PathData *p) { return p->DuSI; }
static double rget_DlSI(const struct PathData *p) { return p->DlSI; }
static double rget_RSN(const struct PathData *p) { return p->RSN; }
static double rget_RT(const struct PathData *p) { return p->RT; }
static double rget_RF(const struct PathData *p) { return p->RF; }
static double rget_BCR(const struct PathData *p) { return p->BCR; }
static double rget_OCR(const struct PathData *p) { return p->OCR; }
static double rget_MIR(const struct PathData *p) { return p->MIR; }
static double rget_Es(const struct PathData *p) { return p->Es; }
static double rget_El(const struct PathData *p) { return p->El; }

/*
	The report columns as data.

	Each row is one output column: the flag that selects it, the text for the
	header listing, the RFC4180 column name, the two field formats and a getter.
	A row with a fn instead is dispatched to a hand-written function, for the
	six columns that are not a plain value -- a receiver location, the two
	lowest-order mode names, SNRXX, the dominant mode and the long-model block.

	This replaced 23 near-identical functions, each a four-case switch varying
	only in those five slots, plus a dispatch branch apiece. Adding a column is
	now one row here rather than a prototype, a dispatch branch and a 20-line
	body that can fall out of step between its four cases.

	The order of this table is the column order of the report.
*/
typedef double (*RptGet)(const struct PathData *);

struct RptCol {
	unsigned long flag;			// which RptFileFormat bit selects it
	const char   *desc;			// header text, NULL when fn is used instead
	const char   *csvname;		// RFC4180 column name
	const char   *fmt;			// field format for the human-readable report
	const char   *fmt4180;		// field format for the RFC4180 report
	RptGet        get;			// value
	void        (*fn)(struct PathData, int, int *);	// irregular columns
};

static const struct RptCol RptCols[] = {
	{ RPT_RXLOCATION,      NULL, NULL, NULL, NULL, NULL, function_RPT_RXLOCATION },
	{ RPT_D,               "D - Path distance (km)", "distance", DBLFIELD2, RFC4180_DBLFIELD2, rget_distance, NULL },
	{ RPT_DMAX,            "dmax - Path maximum hop distance (km)", "dmax", DBLFIELD, RFC4180_DBLFIELD, rget_dmax, NULL },
	{ RPT_DMAX,            "ptick - Slant Path distance (km)", "ptick", DBLFIELD2, RFC4180_DBLFIELD2, rget_ptick, NULL },
	{ RPT_ELE,             "ele - Path minimum Rx elevation angle (deg)", "ele", DBLFIELD, RFC4180_DBLFIELD, rget_ele_deg, NULL },
	{ RPT_BMUF,            "BMUF - Path basic MUF (MHz)", "BMUF", DBLFIELD, RFC4180_DBLFIELD, rget_BMUF, NULL },
	{ RPT_BMUFD,           "MUF50 - 50% Path basic MUF (MHz)", "MUF50", DBLFIELD, RFC4180_DBLFIELD, rget_MUF50, NULL },
	{ RPT_BMUFD,           "MUF90 - 90% Path basic MUF (MHz)", "MUF90", DBLFIELD, RFC4180_DBLFIELD, rget_MUF90, NULL },
	{ RPT_BMUFD,           "MUF10 - 10% Path basic MUF (MHz)", "MUF10", DBLFIELD, RFC4180_DBLFIELD, rget_MUF10, NULL },
	{ RPT_OPMUF,           "OPMUF - Operation MUF (MHz)", "OPMUF", DBLFIELD, RFC4180_DBLFIELD, rget_OPMUF, NULL },
	{ RPT_OPMUFD,          "OPMUF90 - 90% Operation MUF (MHz)", "OPMUF90", DBLFIELD, RFC4180_DBLFIELD, rget_OPMUF90, NULL },
	{ RPT_OPMUFD,          "OPMUF10 - 10% Operation MUF (MHz)", "OPMUF10", DBLFIELD, RFC4180_DBLFIELD, rget_OPMUF10, NULL },
	{ RPT_N0_F2,           NULL, NULL, NULL, NULL, NULL, function_RPT_N0_F2 },
	{ RPT_N0_E,            NULL, NULL, NULL, NULL, NULL, function_RPT_N0_E },
	{ RPT_E,               "E - Path Field Strength (dB(1uV/m))", "Ep", DBLFIELD, RFC4180_DBLFIELD, rget_Ep, NULL },
	{ RPT_PR,              "Pr - Median receiver power (dB)", "PR", DBLFIELD, RFC4180_DBLFIELD, rget_Pr, NULL },
	{ RPT_GRW,             "Grw - Receive Antenna Gain (dbi)", "Grw", DBLFIELD, RFC4180_DBLFIELD, rget_Grw, NULL },
	{ RPT_NOISESOURCES,    "FaA - Atmospheric noise (dB)", "FaA", DBLFIELD, RFC4180_DBLFIELD, rget_FaA, NULL },
	{ RPT_NOISESOURCES,    "FaM - Man-made noise (dB)", "FaM", DBLFIELD, RFC4180_DBLFIELD, rget_FaM, NULL },
	{ RPT_NOISESOURCES,    "FaG - Galactic noise (dB)", "FaG", DBLFIELD, RFC4180_DBLFIELD, rget_FaG, NULL },
	{ RPT_NOISESOURCESD,   "DuA - Upper decile deviation of atmospheric noise (dB)", "DuA", DBLFIELD, RFC4180_DBLFIELD, rget_DuA, NULL },
	{ RPT_NOISESOURCESD,   "DlA - Lower decile deviation of atmospheric noise (dB)", "DlA", DBLFIELD, RFC4180_DBLFIELD, rget_DlA, NULL },
	{ RPT_NOISESOURCESD,   "DuM - Upper decile deviation of man-made noise (dB)", "DuM", DBLFIELD, RFC4180_DBLFIELD, rget_DuM, NULL },
	{ RPT_NOISESOURCESD,   "DlM - Lower decile deviation of man-made noise (dB)", "DlM", DBLFIELD, RFC4180_DBLFIELD, rget_DlM, NULL },
	{ RPT_NOISESOURCESD,   "DuG - Upper decile deviation of atmospheric noise (dB)", "DuG", DBLFIELD, RFC4180_DBLFIELD, rget_DuG, NULL },
	{ RPT_NOISESOURCESD,   "DlG - Lower decile deviation of atmospheric noise (dB)", "DlG", DBLFIELD, RFC4180_DBLFIELD, rget_DlG, NULL },
	{ RPT_NOISETOTALD,     "DuT - Upper decile deviation of total noise (dB)", "DuT", DBLFIELD, RFC4180_DBLFIELD, rget_DuT, NULL },
	{ RPT_NOISETOTALD,     "DlT - Lower decile deviation of total noise (dB)", "DlT", DBLFIELD, RFC4180_DBLFIELD, rget_DlT, NULL },
	{ RPT_NOISETOTAL,      "FamT - Total noise (dB)", "FamT", DBLFIELD, RFC4180_DBLFIELD, rget_FamT, NULL },
	{ RPT_SNR,             "SNR - Median signal-to-noise ratio (dB)", "SNR", DBLFIELD, RFC4180_DBLFIELD, rget_SNR, NULL },
	{ RPT_SNRD,            "DuSN - Upper decile deviation of signal-to-noise ratio (dB)", "DuSN", DBLFIELD, RFC4180_DBLFIELD, rget_DuSN, NULL },
	{ RPT_SNRD,            "DlSN - Lower decile deviation of signal-to-noise ratio (dB)", "DlSN", DBLFIELD, RFC4180_DBLFIELD, rget_DlSN, NULL },
	{ RPT_SNRXX,           NULL, NULL, NULL, NULL, NULL, function_RPT_SNRXX },
	{ RPT_SIR,             "SIR - Signal-to-interference ratio (dB)", "SIR", DBLFIELD, RFC4180_DBLFIELD, rget_SIR, NULL },
	{ RPT_SIRD,            "DuSI - Upper decile deviation of signal-to-interference ratio (dB)", "DuSI", DBLFIELD, RFC4180_DBLFIELD, rget_DuSI, NULL },
	{ RPT_SIRD,            "DlSI - Lower decile deviation of signal-to-interference ratio (dB)", "DlSI", DBLFIELD, RFC4180_DBLFIELD, rget_DlSI, NULL },
	{ RPT_RSN,             "RSN - Probability that the required SNR is achieved (%)", "RSN", DBLFIELD, RFC4180_DBLFIELD, rget_RSN, NULL },
	{ RPT_RSN,             "RT - Probability that the required time spread T0 is not exceeded (%)", "RT", DBLFIELD, RFC4180_DBLFIELD, rget_RT, NULL },
	{ RPT_RSN,             "RF - Probability that the required frequency spread f0 is not exceeded (%)", "RF", DBLFIELD, RFC4180_DBLFIELD, rget_RF, NULL },
	{ RPT_BCR,             "BCR - Basic circuit reliability (%)", "BCR", DBLFIELD, RFC4180_DBLFIELD, rget_BCR, NULL },
	{ RPT_OCR,             "OCR - Overall circuit reliability not considering scattering (%)", "OCR", DBLFIELD, RFC4180_DBLFIELD, rget_OCR, NULL },
	{ RPT_OCRS,            NULL, NULL, NULL, NULL, NULL, function_RPT_OCRS },
	{ RPT_MIR,             "MIR - Multimode Interference (%)", "MIR", DBLFIELD, RFC4180_DBLFIELD, rget_MIR, NULL },
	{ RPT_DOMMODE,         NULL, NULL, NULL, NULL, NULL, function_RPT_DOMMODE },
	{ RPT_ESL,             "Short Path (<=7000 km) Field Strength (dB(1uV/m))", "Es", DBLFIELD1, RFC4180_DBLFIELD1, rget_Es, NULL },
	{ RPT_ESL,             "Long Path (>9000km) Field Strength (dB(1uV/m))", "El", DBLFIELD1, RFC4180_DBLFIELD1, rget_El, NULL },
	{ RPT_LONG,            NULL, NULL, NULL, NULL, NULL, function_RPT_LONG },
};
#define NRPTCOLS ((int)(sizeof(RptCols)/sizeof(RptCols[0])))

/*
	EmitCol() - Writes one table-driven column in the requested form.

		INPUT
			const struct RptCol *c, struct PathData path, int option, int *col

		OUTPUT
			One column of the header listing, the csv header or a data record

		SUBROUTINES
			None
*/
static void EmitCol(const struct RptCol *c, struct PathData path, int option, int *col) {

	switch (option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: %s\n", ++*col, c->desc);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",%s", c->csvname);
			++*col;
			break;
		case PRINT_DATA:
			fprintf(fp, ",");
			fprintf(fp, c->fmt, c->get(&path));
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp, ",");
			fprintf(fp, c->fmt4180, c->get(&path));
			break;
	}

	return;

}

void Report(struct PathData path, struct ITURHFProp ITURHFP) {

	/*
	 * Report() Prints a report of the format specified by the variable ITURHFP.RptFileFormat
	 *		The report that is generated is a comma separated value (CSV) file. Setting the
	 *		value of ITURHFP.RptFileFormat before this routine is called will allow for the custom
	 *		output of the CSV file. The options to set the variable ITURHFP.RptFileFormat are in
	 *		ITURHFProp.h under the heading "Report output options".
	 *
	 *			INPUT
	 *				struct PathData path
	 *				struct ITURHFProp ITURHFP
	 *
	 *			OUTPUT
	 *	 			Printed data to the file named
	 *				RPTddmmyy-hhnnss.txt - where the report file is time stamped as follows:
	 *					dd = day
	 *					mm = month
	 *					yy = year
	 *					hh = hour
	 *					nn = minute
	 *					ss = seconds
	 *
	 */

	// For readability set the local global file pointer.
	fp = ITURHFP.rptfp;

	// Determine if the the user wants the header printed
	if (ITURHFP.header == TRUE) {
		if (Header == TRUE) {

			// Only print out the header one time.
			Header = FALSE;

			// First output
			// Write the header to the report file.
			PrintHeader(path, ITURHFP);
			PrintRecord(path, ITURHFP, PRINT_HEADER);
			// If there is only one line to be printed in the output than the first record is the last record
			if ((ITURHFP.ihr == ITURHFP.ihrend - 1) &&
				(ITURHFP.ifrq == ITURHFP.ifrqend - 1) &&
				(ITURHFP.ilng == ITURHFP.ilngend - 1) &&
				(ITURHFP.imnth == ITURHFP.imnthend - 1) &&
				(ITURHFP.ilat == ITURHFP.ilatend - 1)) {
				// Last record of the report
				PrintLastRecord(path, ITURHFP);
			}
			else {
				PrintRecord(path, ITURHFP, PRINT_DATA);
			}
        }
		else if ((ITURHFP.ihr == ITURHFP.ihrend - 1) &&
			(ITURHFP.ifrq == ITURHFP.ifrqend - 1) &&
			(ITURHFP.ilng == ITURHFP.ilngend - 1) &&
			(ITURHFP.imnth == ITURHFP.imnthend - 1) &&
			(ITURHFP.ilat == ITURHFP.ilatend - 1)) {
			// Last record of the report
			PrintLastRecord(path, ITURHFP);
		}
		else { // Middle of the report
			PrintRecord(path, ITURHFP, PRINT_DATA);
		}
    }
	else if (ITURHFP.header == FALSE) {
		if (ITURHFP.csvRFC4180 == TRUE) {
			if (Header == TRUE) {
				PrintRecord(path, ITURHFP, PRINT_RFC4180_HEADER);
				Header = FALSE;
			}
			PrintRecord(path, ITURHFP, PRINT_RFC4180_DATA);
		} else {
			PrintRecord(path, ITURHFP, PRINT_DATA);
		}
	}

    return;

}

void PrintRecord(struct PathData path, struct ITURHFProp ITURHFP, int option) {

	int col;

	col = 3;

	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column 01: Month\n");
			fprintf(fp, "Column 02: Hour\n");
			fprintf(fp, "Column 03: Frequency (MHz)\n");
			break;
		case PRINT_RFC4180_HEADER:
		  fprintf(fp, "month,hour,frequency");
			break;
		case PRINT_DATA:
			// Each record will require the month, hour, and frequency
			// Month
			fprintf(fp, "%02d", path.month+1);
			fprintf(fp,",");
			// Hour
			fprintf(fp, " %02d", path.hour+1);
			fprintf(fp,",");
			// Frequency
			fprintf(fp, DBLFIELD3, path.frequency);
			break;
		case PRINT_RFC4180_DATA:
			// Each record will require the month, hour, and frequency
			// Month
			fprintf(fp, "%d,%d,", path.month+1, path.hour+1);
			fprintf(fp, RFC4180_DBLFIELD, path.frequency);
			break;
	}

    // Walk the column table in order; a row with a fn is one of the six
    // columns that needs its own code.
    for (int n = 0; n < NRPTCOLS; n++) {
        if ((ITURHFP.RptFileFormat & RptCols[n].flag) != RptCols[n].flag) continue;
        if (RptCols[n].fn != NULL) RptCols[n].fn(path, option, &col);
        else                       EmitCol(&RptCols[n], path, option, &col);
    }

    // If the data format header is being printed, put the tail on.
	if(option == PRINT_HEADER) {
		fprintf(fp, "\n");
	    fprintf(fp, "************************** End Data Format ********************************\n");
		fprintf(fp, "\n");
		fprintf(fp, "************************ Calculated Parameters ****************************\n");
		fprintf(fp, "\n");
	}
	else {
		// End of output record line
		fprintf(fp, "\n");
	}

    return;
}

void PrintHeader(struct PathData path, struct ITURHFProp ITURHFP) {

	PrintITUHeader(ITURHFP.rptfp, asctime(ITURHFP.time), ITURHFP.P533ver, ITURHFP.P533compt, path.P372ver, path.P372compt);

	fprintf(fp, "***************************** P533 Input Parameters ****************************\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t%s\n", path.name);
	fprintf(fp, "\tYear          : %d\n", path.year);
	fprintf(fp, "\tMonth         : %s\n", months[path.month]);
	fprintf(fp, "\tHour          : %d (hour UTC)\n", path.hour + 1);
	fprintf(fp, "\tSSN (R12)     : %d\n", path.SSN);
	fprintf(fp, "\tDistance      : %lf (km)\n", path.distance);
	fprintf(fp, "\tdmax          : %lf (km)\n", path.dmax);
	fprintf(fp, "\tTx power      : %lf\n", path.txpower);
	fprintf(fp, "\tTx Location     %s\n", path.txname);
	fprintf(fp, "\tTx latitude   : %10.6lf %c\n", fabs(path.L_tx.lat*R2D), NS(path.L_tx.lat));
	fprintf(fp, "\tTx longitude  : %10.6lf %c\n", fabs(path.L_tx.lng*R2D), EW(path.L_tx.lng));
	fprintf(fp, "\tRx Location     %s\n", path.rxname);
	fprintf(fp, "\tRx latitude   : %10.6lf %c\n", fabs(path.L_rx.lat*R2D), NS(path.L_rx.lat));
	fprintf(fp, "\tRx longitude  : %10.6lf %c\n", fabs(path.L_rx.lng*R2D), EW(path.L_rx.lng));
	fprintf(fp, "\tlocal time Rx : %d (hour UTC)\n", (int)fmod((path.hour + 1 + (int)(path.L_rx.lng/(15.0*D2R)))+24,24.0));
	fprintf(fp, "\tlocal time Tx : %d (hour UTC)\n", (int)fmod((path.hour + 1 + (int)(path.L_tx.lng/(15.0*D2R)))+24,24.0));
	fprintf(fp, "\tFrequency     : %lf\n", path.frequency);
	fprintf(fp, "\tBandwidth     : %lf\n", path.BW);

	if(path.Modulation == ANALOG) {
		strcpy(outstr, "ANALOG");
		fprintf(fp, "\tModulation : %s\n", outstr);
	}
	else {
		strcpy(outstr, "DIGITAL");
		fprintf(fp, "\tModulation : %s\n", outstr);
	}

    fprintf(fp, "\tRequired signal-to-noise ratio : %lf\n", path.SNRr);
	fprintf(fp, "\tRequired %% of month signal-to-noise ratio : % d\n", path.SNRXXp);
	fprintf(fp, "\tRequired signal-to-interference ratio : %lf\n", path.SIRr);

	if(path.noiseP.ManMadeNoise == CITY) {
		strcpy(outstr, "CITY");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else if(path.noiseP.ManMadeNoise == RESIDENTIAL) {
		strcpy(outstr, "RESIDENTIAL");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else if(path.noiseP.ManMadeNoise == RURAL) {
		strcpy(outstr, "RURAL");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else if(path.noiseP.ManMadeNoise == QUIETRURAL) {
		strcpy(outstr, "QUIETRURAL");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else if(path.noiseP.ManMadeNoise == NOISY) {
		strcpy(outstr, "NOISY");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else if(path.noiseP.ManMadeNoise == QUIET) {
		strcpy(outstr, "QUIET");
		fprintf(fp, "\tMan-made noise : %s\n", outstr);
	}
	else {
		fprintf(fp, "\tMan-made noise : %lf (dB)\n", path.noiseP.ManMadeNoise);
	}

	if(path.Modulation == DIGITAL) {
		fprintf(fp, "\tFrequency dispersion for simple BCR (F0) : %lf\n", path.F0);			// Frequency dispersion at a level -10 dB relative to the peak signal amplitude
		fprintf(fp, "\tTime spread for simple BCR (T0) : %lf\n", path.T0);
		fprintf(fp, "\tRequired Amplitude ratio (A) : %lf\n", path.A);
		fprintf(fp, "\tTime window (usec) : %lf\n", path.TW);
		fprintf(fp, "\tFrequency window (Hz) : %lf\n", path.FW);
	}

    if(ITURHFP.AntennaOrientation == TX2RX) {
		fprintf(fp, "\tAntenna configuration : Transmitter main beam to receiver main beam\n");
	}
	else if(ITURHFP.AntennaOrientation == MANUAL) {
		fprintf(fp, "\tAntenna configuration : User determined\n");
	}
	else {
		fprintf(fp, "\tAntenna configuration : UNKNOWN\n");
	}

    if (path.SorL == SHORTPATH) {
		strcpy(outstr, "SHORTPATH");
	}
	else if (path.SorL == LONGPATH) {
		strcpy(outstr, "LONGPATH");
	}
	else {
		strcpy(outstr, "ERROR");
	}
    fprintf(fp, "\tPath Direction : %s\n", outstr);

	fprintf(fp, "\tTransmit antenna               %.40s\n", path.A_tx.Name);
	fprintf(fp, "\tTransmit antenna bearing     : %lf\n", ITURHFP.TXBearing*R2D);
	fprintf(fp, "\tTransmit antenna gain offset : %lf\n", ITURHFP.TXGOS);
	fprintf(fp, "\tReceive antenna                %.40s\n", path.A_rx.Name);
	fprintf(fp, "\tReceive antenna bearing      : %lf\n", ITURHFP.RXBearing*R2D);
	fprintf(fp, "\tReceive antenna gain offset  : %lf\n", ITURHFP.RXGOS);

	fprintf(fp, "\n");
	fprintf(fp, "************************ End P533 Input Parameters *****************************\n");
	fprintf(fp, "\n");
	fprintf(fp, "************************** ITURHFP Input Parameters *****************************\n");
	fprintf(fp, "\n");
	fprintf(fp, "\tUpper left (North West) latitude   : %10.6lf %c\n", fabs(ITURHFP.L_UL.lat*R2D), NS(ITURHFP.L_UL.lat*R2D));
	fprintf(fp, "\tUpper left (North West) longitude  : %10.6lf %c\n", fabs(ITURHFP.L_UL.lng*R2D), EW(ITURHFP.L_UL.lng*R2D));
	fprintf(fp, "\tLower right (South East) latitude  : %10.6lf %c\n", fabs(ITURHFP.L_LR.lat*R2D), NS(ITURHFP.L_LR.lat*R2D));
	fprintf(fp, "\tLower right (South East) longitude : %10.6lf %c\n", fabs(ITURHFP.L_LR.lng*R2D), EW(ITURHFP.L_LR.lng*R2D));
	fprintf(fp, "\tNumber of frequencies : %d\n", ITURHFP.ifrqend);
	fprintf(fp, "\tNumber of hours       : %d\n", ITURHFP.ihrend);
	fprintf(fp, "\tNumber of months      : %d\n", ITURHFP.imnthend);
	fprintf(fp, "\tLatitude increment    : %lf (deg)\n", ITURHFP.latinc*R2D);
	fprintf(fp, "\tLongitude increment   : %lf (deg)\n", ITURHFP.lnginc*R2D);
	fprintf(fp, "\n");
	fprintf(fp, "************************** ITURHFP Input Parameters *****************************\n");
	fprintf(fp, "\n");
	fprintf(fp, "******************************** Data Format ***********************************\n");
	fprintf(fp, "\n");

	return;

}

void PrintLastRecord(struct PathData path, struct ITURHFProp ITURHFP) {

	PrintRecord(path, ITURHFP, PRINT_DATA);

	fprintf(fp, "\n");
	fprintf(fp, "**************************End Calculated Parameters ***********************\n\n");

	return;

}








void function_RPT_N0_F2(struct PathData path, int option, int *col) {
	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: Lowest order mode for the F2 layer\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",n0_F2");
			++*col;
			break;
		case PRINT_DATA:
			fprintf(fp,",");
			if(path.n0_F2 != NOLOWESTMODE) {
				sprintf(outstr, "  %1dF2 ", path.n0_F2+1);
			}
			else {
				sprintf(outstr, " NONE ");
			}
            fprintf(fp, STRFIELD, outstr);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			if(path.n0_F2 != NOLOWESTMODE) {
				sprintf(outstr, "%dF2", path.n0_F2+1);
			}
			else {
				sprintf(outstr, "NONE");
			}
            fprintf(fp, RFC4180_STRFIELD, outstr);
			break;
	}
    return;
}

void function_RPT_N0_E(struct PathData path, int option, int *col) {
	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: Lowest order mode for the E layer\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",n0_E");
			++*col;
			break;
		case PRINT_DATA:
			fprintf(fp,",");
			if(path.n0_E != NOLOWESTMODE) {
				sprintf(outstr, "   %1dE ", path.n0_E+1);
			}
			else {
				sprintf(outstr, " NONE ");
			}
            fprintf(fp, STRFIELD, outstr);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			if(path.n0_E != NOLOWESTMODE) {
				sprintf(outstr, "%dE", path.n0_E+1);
			}
			else {
				sprintf(outstr, "NONE");
			}
            fprintf(fp, RFC4180_STRFIELD, outstr);
			break;
	}
    return;
}










void function_RPT_SNRXX(struct PathData path, int option, int *col) {
	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: SNRXXp - Signal-to-noise ratio at %0d%% of month\n", ++*col, path.SNRXXp);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",SNRXXp");
			++*col;
			break;
		case PRINT_DATA:
			fprintf(fp,",");
			fprintf(fp, DBLFIELD, path.SNRXX);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD, path.SNRXX);
			break;
	}
    return;
}






void function_RPT_OCRS(struct PathData path, int option, int *col) {

	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: OCRs - Overall circuit reliability considering scattering (%%)\n", ++*col);
			fprintf(fp, "Column %02d: Probocc - Probability of scattering (%%)\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",OCRs,probocc");
			*col = *col + 2;
			break;
		case PRINT_DATA:
			fprintf(fp,",");
			fprintf(fp, DBLFIELD, path.OCRs);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD, path.probocc);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD, path.OCRs);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD, path.probocc);
			break;
	}
    return;
}


void function_RPT_RXLOCATION(struct PathData path, int option, int *col) {
	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: Receiver latitude (deg)\n", ++*col);
			fprintf(fp, "Column %02d: Receiver longitude (deg)\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",rxlat,rxlng");
			*col = *col+2;
		  break;
		case PRINT_DATA:
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.L_rx.lat*R2D);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.L_rx.lng*R2D);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.L_rx.lat*R2D);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.L_rx.lng*R2D);
			break;
	}
    return;
}


void function_RPT_LONG(struct PathData path, int option, int *col) {
	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: Free-space Field Strength 3 MW e.i.r.p. (dB(1uV/m)\n", ++*col);
			fprintf(fp, "Column %02d: Increased Long Distance Field Strength due to Focusing (dB)\n", ++*col);
			fprintf(fp, "Column %02d: \"Not otherwise included loss\" (dB)\n", ++*col);
			fprintf(fp, "Column %02d: Upper Reference Frequency (MHz)\n", ++*col);
			fprintf(fp, "Column %02d: Lower Reference Frequency (MHz)\n", ++*col);
			fprintf(fp, "Column %02d: Correction Factor at T + d0/2\n", ++*col);
			fprintf(fp, "Column %02d: Correction Factor at R - d0/2\n", ++*col);
			fprintf(fp, "Column %02d: Max Antenna Gain G_tl (0 - 8 deg)\n", ++*col);
			fprintf(fp, "Column %02d: Max Antenna Gain G_rw (0 - 8 deg)\n", ++*col);
			fprintf(fp, "Column %02d: Mean gyrofrequency\n", ++*col);
			fprintf(fp, "Column %02d: Scale factor f(f ,fL, fM, fH)\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",E0,Gap,Ly,fM,fL,K0,K1,Gtl,Grw,fH,Fscale");
			*col = *col + 11;
			break;
		case PRINT_DATA:
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.E0);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.Gap);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.Ly);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.fM);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.fL);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.K[0]);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.K[1]);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.Gtl);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.Grw);
			fprintf(fp, ",");
			fprintf(fp, DBLFIELD1, path.fH);
			fprintf(fp,",");
			fprintf(fp, DBLFIELD1, path.F);
			break;
		case PRINT_RFC4180_DATA:
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.E0);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.Gap);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.Ly);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.fM);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.fL);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.K[0]);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.K[1]);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.Gtl);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.Grw);
			fprintf(fp, ",");
			fprintf(fp, RFC4180_DBLFIELD1, path.fH);
			fprintf(fp,",");
			fprintf(fp, RFC4180_DBLFIELD1, path.F);
			break;
	}
    return;
}

void function_RPT_DOMMODE(struct PathData path, int option, int *col) {

	switch(option) {
		case PRINT_HEADER:
			fprintf(fp, "Column %02d: Dominant mode\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Elevation angle (deg)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Delay (mS)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Loss (dB)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Propagation probability (%%)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Virtual height (km)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Median received power (dB)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Receiver Antenna Gain (dBi)\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Field Strength (dB(1uV/m))\n", ++*col);
			fprintf(fp, "Column %02d: Dominant mode - Basic MUF (MHz)\n", ++*col);
			break;
		case PRINT_RFC4180_HEADER:
			fprintf(fp, ",DMidx,DMele,DMtau,DMLb,DMFprob,DMhr,DMPrw,DMGrw,DMEw,DMBMUF");
			*col = *col + 10;
			break;
		case PRINT_DATA:
			if(path.DMidx < MAXEMDS) { // E mode dominant
				sprintf(outstr,  "   %1dE ", path.DMidx+1);
			}
			else if((path.DMidx >= MAXEMDS) && (path.DMidx <= MAXMDS)) { // F2 mode dominant
				sprintf(outstr, "  %1dF2 ", path.DMidx-2);
			}
			else { // There is no dominant mode
				sprintf(outstr, " NONE ");
			}

            if(path.DMidx <= MAXMDS) {
				fprintf(fp,",");
				fprintf(fp, STRFIELD, outstr);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->ele*R2D);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->tau);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->Lb);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->Fprob);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->hr);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->Prw);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->Grw);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->Ew);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, path.DMptr->BMUF);
			}
			else {
				fprintf(fp,",");
				fprintf(fp, STRFIELD, outstr);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, DBLFIELD, 0.0);
			}
            break;
		case PRINT_RFC4180_DATA:
			if(path.DMidx < MAXEMDS) { // E mode dominant
				sprintf(outstr,  "%dE", path.DMidx+1);
			}
			else if((path.DMidx >= MAXEMDS) && (path.DMidx <= MAXMDS)) { // F2 mode dominant
				sprintf(outstr, "%dF2", path.DMidx-2);
			}
			else { // There is no dominant mode
				sprintf(outstr, "NONE");
			}

            if(path.DMidx <= MAXMDS) {
				fprintf(fp,",");
				fprintf(fp, RFC4180_STRFIELD, outstr);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->ele*R2D);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->tau);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->Lb);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->Fprob);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->hr);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->Prw);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->Grw);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->Ew);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, path.DMptr->BMUF);
			}
			else {
				fprintf(fp,",");
				fprintf(fp, RFC4180_STRFIELD, outstr);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
				fprintf(fp,",");
				fprintf(fp, RFC4180_DBLFIELD, 0.0);
			}
            break;
	}

    return;
}

char EW(double lng) {

	if(lng < 0.0) return 'W';
	else if(lng > 0.0) return 'E';
	else return '=';

}

char NS(double lat) {

	if(lat < 0.0) return 'S';
	else if(lat > 0.0) return 'N';
	else return '=';

}

