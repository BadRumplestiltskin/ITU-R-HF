#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

// Local includes
#include "Common.h"
#include "P533.h"
#include "ITURHFProp.h"
#include "CircuitCSV.h"
// End local includes

/*
	CircuitCSV() - Batch front end to the ITU-R P.533-14 engine.

		Reads a csv of circuit definitions, runs P533() for each one and writes a
		csv of the calculated circuit parameters. It exists so that a list of
		circuits can be processed in one pass, which the .in file interface of
		ITURHFProp() cannot do: that interface describes a single circuit swept
		over a grid of hours, frequencies and receiver locations.

		INPUT
			-i <file>	csv of circuit definitions
			-o <file>	csv to write
			-d <path>	directory holding the coefficient and ionospheric data
			-t <file>	transmit antenna, a Type 13 file, or ISOTROPIC
			-r <file>	receive antenna, a Type 13 file, or ISOTROPIC
			-g <dBi>	gain to use for an isotropic pattern (default 0.0)

		OUTPUT
			A csv with the input columns echoed, followed by the calculated
			columns. See PrintHeader() for the column list and the units.

		SUBROUTINES
			LoadP533(), LoadAntennas(), ReadCircuit(), RunCircuit(), WriteRow()
*/

// The P533 library, loaded the same way ITURHFProp() loads it.
#ifdef _WIN32
	#include <windows.h>
	#define CSVLIBCLOSE(h)	FreeLibrary((HMODULE)(h))
	#define CSVP533LIB		"P533.dll"
	#define CSVP372LIB		"P372.dll"
#else
	#include <dlfcn.h>
	#define CSVLIBCLOSE(h)	dlclose(h)
	#define CSVP533LIB		"libp533.so"
	#define CSVP372LIB		"libp372.so"
#endif

static void  *P533Lib = NULL;
static void  *P372Lib = NULL;

static int    (*csvP533)(struct PathData *);
static int    (*csvAllocatePathMemory)(struct PathData *);
static int    (*csvFreePathMemory)(struct PathData *);
static double (*csvBearing)(struct Location, struct Location, int);
static int    (*csvReadIonParametersBin)(int, float ****, float ****, char *, int);
static int    (*csvReadP1239)(struct PathData *, const char *);
static int    (*csvReadType13)(struct Antenna *, FILE *, double, int);
static void   (*csvIsotropicPattern)(struct Antenna *, double, int);
static int    (*csvReadFamDud)(struct NoiseParams *, const char *, int);

// The three characteristic frequencies each circuit is evaluated at.
#define FRQBUF	0	// basic MUF of the dominant mode
#define FRQMUF	1	// path MUF exceeded 50% of days
#define FRQOWF	2	// path MUF exceeded 90% of days, the FOT
#define NFRQ	3

/*
	Results of one circuit. Every value is in the engine's own units, with no
	scaling applied, so that whatever reads this file can round as it likes.
*/
struct Result {
	double dist;			// km
	double txBearing;		// degrees from true north
	double rxBearing;		// degrees
	int    hops;			// number of hops of the dominant mode
	char   layer;			// 'E' or 'F' for the dominant mode
	double f[NFRQ];			// MHz: BUF, MUF, OWF
	double sn[NFRQ];		// dB at each of the above
	double prob;			// basic circuit reliability at the BUF (%)
	double toa;				// take-off angle of the dominant mode (degrees)
	double loss;			// basic transmission loss of the dominant mode (dB)
	double delay;			// group delay of the dominant mode (s)
	double grange;			// group range of the dominant mode (km)
	double noiseRx;			// total noise at the receiver (dB above kT0B)
	int    valid;
};

static void PrintUsage(void);

/*
	LoadP533() - Resolves the entry points of the P533 and P372 libraries.

		Done once, before any circuit is read.

		INPUT
			None

		OUTPUT
			returns RTN_CSVOK, or an error

		SUBROUTINES
			None
*/
static int LoadP533(void) {

#ifdef _WIN32
	P533Lib = (void *)LoadLibrary(CSVP533LIB);
	P372Lib = (void *)LoadLibrary(CSVP372LIB);
	if (P533Lib == NULL || P372Lib == NULL) {
		printf("CircuitCSV: Error %d Can't load %s / %s\n", RTN_ERRCSVP533LIB, CSVP533LIB, CSVP372LIB);
		return RTN_ERRCSVP533LIB;
	}
	csvP533                 = (void *)GetProcAddress((HMODULE)P533Lib, "P533");
	csvAllocatePathMemory   = (void *)GetProcAddress((HMODULE)P533Lib, "AllocatePathMemory");
	csvFreePathMemory       = (void *)GetProcAddress((HMODULE)P533Lib, "FreePathMemory");
	csvBearing              = (void *)GetProcAddress((HMODULE)P533Lib, "Bearing");
	csvReadIonParametersBin = (void *)GetProcAddress((HMODULE)P533Lib, "ReadIonParametersBin");
	csvReadP1239            = (void *)GetProcAddress((HMODULE)P533Lib, "ReadP1239");
	csvReadType13           = (void *)GetProcAddress((HMODULE)P533Lib, "ReadType13");
	csvIsotropicPattern     = (void *)GetProcAddress((HMODULE)P533Lib, "IsotropicPattern");
	csvReadFamDud           = (void *)GetProcAddress((HMODULE)P372Lib, "ReadFamDud");
#else
	P533Lib = dlopen(CSVP533LIB, RTLD_NOW);
	if (P533Lib == NULL) {
		printf("CircuitCSV: Error %d Can't load %s (%s)\n", RTN_ERRCSVP533LIB, CSVP533LIB, dlerror());
		return RTN_ERRCSVP533LIB;
	}
	P372Lib = dlopen(CSVP372LIB, RTLD_NOW);
	if (P372Lib == NULL) {
		printf("CircuitCSV: Error %d Can't load %s (%s)\n", RTN_ERRCSVP372LIB, CSVP372LIB, dlerror());
		return RTN_ERRCSVP372LIB;
	}
	csvP533                 = dlsym(P533Lib, "P533");
	csvAllocatePathMemory   = dlsym(P533Lib, "AllocatePathMemory");
	csvFreePathMemory       = dlsym(P533Lib, "FreePathMemory");
	csvBearing              = dlsym(P533Lib, "Bearing");
	csvReadIonParametersBin = dlsym(P533Lib, "ReadIonParametersBin");
	csvReadP1239            = dlsym(P533Lib, "ReadP1239");
	csvReadType13           = dlsym(P533Lib, "ReadType13");
	csvIsotropicPattern     = dlsym(P533Lib, "IsotropicPattern");
	csvReadFamDud           = dlsym(P372Lib, "ReadFamDud");
#endif

	// A missing symbol would otherwise surface as a call through a NULL pointer.
	if (csvP533 == NULL || csvAllocatePathMemory == NULL || csvFreePathMemory == NULL ||
		csvBearing == NULL || csvReadIonParametersBin == NULL || csvReadP1239 == NULL ||
		csvReadType13 == NULL || csvIsotropicPattern == NULL || csvReadFamDud == NULL) {
		printf("CircuitCSV: Error %d P533/P372 entry point not found\n", RTN_ERRCSVP533LIB);
		return RTN_ERRCSVP533LIB;
	}

	return RTN_CSVOK;

}

/*
	Trim() - Removes leading and trailing white space and any quotes in place.

		INPUT
			char *s

		OUTPUT
			returns s, trimmed

		SUBROUTINES
			None
*/
static char *Trim(char *s) {

	char *end;

	while (*s == ' ' || *s == '\t' || *s == '"') s++;
	end = s + strlen(s);
	while (end > s) {
		char c = *(end-1);
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '"') end--;
		else break;
	}
	*end = '\0';

	return s;

}

/*
	SplitCSV() - Splits one record into fields on commas.

		Quoted fields are handled so that a site name may contain a comma.

		INPUT
			char *line		the record, modified in place
			char **field	receives a pointer to each field
			int maxfields

		OUTPUT
			returns the number of fields found

		SUBROUTINES
			Trim()
*/
static int SplitCSV(char *line, char **field, int maxfields) {

	int n = 0;
	int inquote = 0;
	char *p = line;

	field[n++] = line;
	for (; *p != '\0'; p++) {
		if (*p == '"') inquote = !inquote;
		else if (*p == ',' && inquote == 0) {
			*p = '\0';
			if (n >= maxfields) return n;
			field[n++] = p + 1;
		}
	}

	for (int i = 0; i < n; i++) field[i] = Trim(field[i]);

	return n;

}

/*
	FindColumn() - Locates a named column in the header, ignoring case.

		INPUT
			char **hdr, int nhdr, const char *name

		OUTPUT
			returns the column index, or -1 when the name is absent

		SUBROUTINES
			None
*/
static int FindColumn(char **hdr, int nhdr, const char *name) {

	for (int i = 0; i < nhdr; i++) {
#ifdef _WIN32
		if (_stricmp(hdr[i], name) == 0) return i;
#else
		if (strcasecmp(hdr[i], name) == 0) return i;
#endif
	}

	return -1;

}

// The input columns, in the order the header must provide them. They are looked
// up by name rather than position, so extra columns are ignored and the order in
// the file does not matter.
static const char *InputColumns[] = {
	"txSite", "txLat", "txLon", "rxSite", "rxLat", "rxLon",
	"year", "month", "day", "hour", "t_Index", "minTOA",
	"txPow", "reqSN", "rxNoise", "bandW", "percDays"
};
#define NINPUTCOLUMNS ((int)(sizeof(InputColumns)/sizeof(InputColumns[0])))

/*
	ReadCircuit() - Fills a Circuit from one split record.

		INPUT
			struct Circuit *c, char **f, int nf, int *col

		OUTPUT
			returns RTN_CSVOK, or RTN_ERRCSVFIELD when a field is missing

		SUBROUTINES
			None
*/
static int ReadCircuit(struct Circuit *c, char **f, int nf, int *col) {

	// col[] was validated when the header was read, so every index is present.
	for (int i = 0; i < NINPUTCOLUMNS; i++) {
		if (col[i] >= nf) return RTN_ERRCSVFIELD;
	}

	snprintf(c->txSite, sizeof(c->txSite), "%s", f[col[0]]);
	c->txLat    = atof(f[col[1]]);
	c->txLon    = atof(f[col[2]]);
	snprintf(c->rxSite, sizeof(c->rxSite), "%s", f[col[3]]);
	c->rxLat    = atof(f[col[4]]);
	c->rxLon    = atof(f[col[5]]);
	c->year     = atoi(f[col[6]]);
	c->month    = atoi(f[col[7]]);
	c->day      = atoi(f[col[8]]);
	c->hour     = atoi(f[col[9]]);
	c->t_Index  = atoi(f[col[10]]);
	c->minTOA   = atof(f[col[11]]);
	c->txPow    = atof(f[col[12]]);
	c->reqSN    = atof(f[col[13]]);
	c->rxNoise  = atof(f[col[14]]);
	c->bandW    = atof(f[col[15]]);
	c->percDays = atof(f[col[16]]);

	return RTN_CSVOK;

}

/*
	SetPath() - Transfers a Circuit into the PathData structure P533() reads.

		The unit conversions are all here, so there is one place to look:
			txPow    W        -> path->txpower, dB relative to 1 kW
			rxNoise  dBW      -> path->noiseP.ManMadeNoise
			month    1-12     -> path->month, 0-11
			lat/lng  degrees  -> radians, which every P533 routine expects

		P372's ManMadeNoise takes either one of the six category constants or a
		user value, and for a user value it forms the noise figure as
		(204 - ManMadeNoise). A receiver noise power of -137 dBW is therefore
		passed as +137, which is why the sign is inverted below.

		INPUT
			struct PathData *path, struct Circuit *c, double frequency

		OUTPUT
			path, ready for P533()

		SUBROUTINES
			None
*/
static void SetPath(struct PathData *path, struct Circuit *c, double frequency) {

	snprintf(path->name,   sizeof(path->name),   "%s to %s", c->txSite, c->rxSite);
	snprintf(path->txname, sizeof(path->txname), "%s", c->txSite);
	snprintf(path->rxname, sizeof(path->rxname), "%s", c->rxSite);

	path->L_tx.lat = c->txLat * D2R;
	path->L_tx.lng = c->txLon * D2R;
	path->L_rx.lat = c->rxLat * D2R;
	path->L_rx.lng = c->rxLon * D2R;

	path->year  = c->year;
	path->month = c->month - 1;		// the file is 1-12, the engine is 0-11
	path->hour  = c->hour;
	path->SSN   = c->t_Index;		// t_Index is the smoothed sunspot number

	path->frequency = frequency;
	path->BW        = c->bandW;

	// txpower is dB relative to 1 kW, so watts convert as 10*log10(W/1000).
	path->txpower = (c->txPow > 0.0) ? 10.0*log10(c->txPow/1000.0) : 0.0;

	path->SNRr    = c->reqSN;
	path->SNRXXp  = (int)(c->percDays + 0.5);
	path->SIRr    = 0.0;
	path->Modulation = ANALOG;
	path->SorL       = SHORTPATH;

	// Digital modulation parameters are unused for ANALOG but must be valid.
	path->F0 = 0.0;
	path->T0 = 0.0;
	path->A  = 0.0;
	path->TW = 0.0;
	path->FW = 0.0;

	path->noiseP.ManMadeNoise = -c->rxNoise;

}

/*
	DominantMode() - Identifies the dominant mode that P533() selected.

		P533() leaves path->DMptr pointing into either the F2 or the E mode
		array. Comparing the pointer against each array gives both the layer and
		the mode order, which is the hop count.

		INPUT
			struct PathData *path, int *hops, char *layer

		OUTPUT
			returns TRUE when a dominant mode exists

		SUBROUTINES
			None
*/
static int DominantMode(struct PathData *path, int *hops, char *layer) {

	*hops  = 0;
	*layer = '?';

	if (path->DMptr == NULL) return FALSE;

	for (int n = 0; n < MAXF2MDS; n++) {
		if (path->DMptr == &path->Md_F2[n]) {
			*hops = n + 1;
			*layer = 'F';
			return TRUE;
		}
	}
	for (int n = 0; n < MAXEMDS; n++) {
		if (path->DMptr == &path->Md_E[n]) {
			*hops = n + 1;
			*layer = 'E';
			return TRUE;
		}
	}

	return FALSE;

}

/*
	RunCircuit() - Calculates one circuit.

		P.533's three characteristic frequencies do not depend on the frequency
		of interest, but the signal-to-noise ratio does. So the engine runs once
		on a seed frequency to obtain the MUFs, then once at each of them:

			BUF	the basic MUF of the dominant mode
			MUF	the path MUF exceeded 50% of days
			OWF	the path MUF exceeded 90% of days, the FOT

		The geometry, mode, take-off angle, loss, delay and noise are taken from
		the run at the BUF, which is what the _BUF suffixed columns mean.

		INPUT
			struct PathData *path, struct Circuit *c, struct Result *r

		OUTPUT
			returns RTN_CSVOK, or the engine's error

		SUBROUTINES
			SetPath(), DominantMode(), csvP533()
*/
static double FreqMargin = 1.0;

static int RunCircuit(struct PathData *path, struct Circuit *c, struct Result *r) {

	int retval;
	int hops;
	char layer;

	memset(r, 0, sizeof(*r));

	// Seed run. Any frequency yields the MUFs; 10 MHz sits mid-band.
	SetPath(path, c, 10.0);
	retval = csvP533(path);
	if (retval != RTN_P533OK) return retval;

	r->dist      = path->distance;
	r->txBearing = csvBearing(path->L_tx, path->L_rx, path->SorL) * R2D;
	r->rxBearing = csvBearing(path->L_rx, path->L_tx, path->SorL) * R2D;

	// A path with no supported mode has a zero basic MUF. Report it as blank
	// rather than running the engine again on a meaningless frequency.
	if (DominantMode(path, &hops, &layer) == FALSE || path->DMptr->BMUF <= 0.0) {
		r->valid = FALSE;
		return RTN_CSVOK;
	}

	// BUF is the basic MUF of the dominant mode. MUF is the operational MUF,
	// which is the basic MUF scaled by the P.1240 operational factor, and OWF is
	// the operational MUF exceeded for 90% of days, the FOT.
	r->f[FRQBUF] = path->DMptr->BMUF;
	r->f[FRQMUF] = (path->OPMUF   > 0.0) ? path->OPMUF   : path->MUF50;
	r->f[FRQOWF] = (path->OPMUF90 > 0.0) ? path->OPMUF90 : path->MUF90;

	for (int n = 0; n < NFRQ; n++) {

		if (r->f[n] <= 0.0) continue;

		// The engine's loss has a step at the MUF: a mode that is supported just
		// below it is screened just above, which is worth about 8 dB. Evaluating
		// exactly at the MUF therefore sits on that step, so -m allows a small
		// margin below it for numbers that do not depend on rounding.
		SetPath(path, c, r->f[n] * FreqMargin);
		retval = csvP533(path);
		if (retval != RTN_P533OK) return retval;

		r->sn[n] = path->SNR;

		if (n == FRQBUF) {
			// Everything that describes the mode is reported at the BUF.
			if (DominantMode(path, &hops, &layer) == TRUE) {
				r->hops  = hops;
				r->layer = layer;
				// P.533 carries the elevation angle in radians and the group
				// delay in seconds.
				r->toa  = path->DMptr->ele * R2D;
				r->loss = path->DMptr->Lb;

				// P.533 fills Mode.tau only on the digital-modulation branch of
				// CircuitReliability(), so for an analogue circuit it is still
				// zero here. Reproduce the engine's own calculation instead: the
				// slant range of one hop from its elevation angle, times the
				// number of hops.
				{
					double dh    = path->distance / hops;		// hop distance (km)
					double psi   = dh / (2.0*R0);
					double ptick = 2.0*R0*(sin(psi)/cos(path->DMptr->ele - psi));
					r->grange = hops * ptick;					// group range (km)
					r->delay  = r->grange * 1000.0 / VofL;		// group delay (s)
				}
			}
			r->prob    = path->BCR;
			r->noiseRx = path->noiseP.FamT;
		}
	}

	r->valid = TRUE;

	return RTN_CSVOK;

}

/*
	PrintHeader() - Writes the output header, documenting every unit.

		INPUT
			FILE *fp

		OUTPUT
			The header record

		SUBROUTINES
			None
*/
static void PrintHeader(FILE *fp) {

	fprintf(fp,
		"txSite,txLat,txLon,rxSite,rxLat,rxLon,year,month,day,hour,t_Index,"
		"minTOA,txPow,reqSN,rxNoise,bandW,percDays,"
		"Circuit#,Dist,Tx-Bearing,Rx-Bearing,Mode,BUF,Prob,TOA,Losses,"
		"SN_BUF,Delay_BUF,Grange_BUF,Noise Rx,Noise Tx,MUF,SN_MUF,OWF,SN_OWF\n");

}

/*
	WriteRow() - Writes one input row and its results.

		Values are written in the engine's own units with %.6g, which keeps the
		significant figures P.533 carries without imposing a fixed scaling:

			Dist        km            BUF/MUF/OWF   MHz
			Bearings    degrees       SN_*          dB
			TOA         degrees       Losses        dB
			Delay_BUF   seconds       Grange_BUF    km
			Prob        %             Noise Rx/Tx   dB above kT0B

		Noise Tx repeats Noise Rx: P.533 calculates the noise at the receiver
		only, and there is no transmitter-end noise figure in the Recommendation.

		INPUT
			FILE *fp, struct Circuit *c, struct Result *r, int number

		OUTPUT
			One csv record

		SUBROUTINES
			None
*/
static void WriteRow(FILE *fp, struct Circuit *c, struct Result *r, int number) {

	fprintf(fp, "%s,%.6g,%.6g,%s,%.6g,%.6g,%d,%d,%d,%d,%d,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,",
		c->txSite, c->txLat, c->txLon, c->rxSite, c->rxLat, c->rxLon,
		c->year, c->month, c->day, c->hour, c->t_Index,
		c->minTOA, c->txPow, c->reqSN, c->rxNoise, c->bandW, c->percDays);

	if (r->valid == FALSE) {
		// Geometry is still meaningful when no mode is supported.
		fprintf(fp, "%d,%.6g,%.6g,%.6g,NONE,,,,,,,,,,,,,\n",
			number, r->dist, r->txBearing, r->rxBearing);
		return;
	}

	fprintf(fp, "%d,%.6g,%.6g,%.6g,%d%c,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g\n",
		number, r->dist, r->txBearing, r->rxBearing,
		r->hops, r->layer,
		r->f[FRQBUF], r->prob, r->toa, r->loss, r->sn[FRQBUF],
		r->delay, r->grange,
		r->noiseRx, r->noiseRx,
		r->f[FRQMUF], r->sn[FRQMUF],
		r->f[FRQOWF], r->sn[FRQOWF]);

}

/*
	LoadAntenna() - Loads one antenna pattern, once, for the whole batch.

		The pattern is read at startup and reused by every circuit, so a Type 13
		file is opened only once no matter how many rows the input holds. The
		bearing is not baked in here: each circuit points its antennas along its
		own great circle, which the caller applies per row.

		INPUT
			struct Antenna *ant, const char *spec, double gos, int silent

		OUTPUT
			returns RTN_CSVOK, or RTN_ERRCSVANTENNA

		SUBROUTINES
			csvIsotropicPattern(), csvReadType13()
*/
static int LoadAntenna(struct Antenna *ant, const char *spec, double gos, int silent) {

	FILE *fp;

	if (spec == NULL || strcmp(spec, "ISOTROPIC") == 0) {
		csvIsotropicPattern(ant, gos, silent);
		return RTN_CSVOK;
	}

	fp = fopen(spec, "r");
	if (fp == NULL) {
		printf("CircuitCSV: Error %d Can't open antenna file %s (%s)\n",
			RTN_ERRCSVANTENNA, spec, strerror(errno));
		return RTN_ERRCSVANTENNA;
	}

	// A bearing of 0.0 keeps the pattern in the file's own orientation.
	if (csvReadType13(ant, fp, 0.0, silent) != RTN_READANTENNAPATTERNSOK) {
		printf("CircuitCSV: Error %d Can't read antenna file %s\n", RTN_ERRCSVANTENNA, spec);
		fclose(fp);
		return RTN_ERRCSVANTENNA;
	}
	fclose(fp);

	return RTN_CSVOK;

}

static void PrintUsage(void) {

	printf("\n");
	printf("USAGE: CircuitCSV -i <in.csv> -o <out.csv> -d <datapath> [options]\n\n");
	printf("  -i <file>   Input csv of circuit definitions\n");
	printf("  -o <file>   Output csv to write\n");
	printf("  -d <path>   Directory holding the CCIR coefficients and ionospheric maps\n");
	printf("  -t <file>   Transmit antenna: a Type 13 file, or ISOTROPIC (default)\n");
	printf("  -r <file>   Receive antenna:  a Type 13 file, or ISOTROPIC (default)\n");
	printf("  -g <dBi>    Gain of an isotropic pattern (default 0.0)\n");
	printf("  -m <factor> Evaluate at factor x each MUF (default 1.0). The loss has an\n");
	printf("              ~8 dB step at the MUF, so 0.99 sits clear of it.\n");
	printf("  -s          Silent: suppress progress output\n");
	printf("  -h          This help\n\n");
	printf("Input columns (looked up by name, order and extra columns do not matter):\n");
	printf("  txSite txLat txLon rxSite rxLat rxLon year month day hour\n");
	printf("  t_Index minTOA txPow reqSN rxNoise bandW percDays\n\n");
	printf("  t_Index is the 12-month smoothed sunspot number, R12.\n");
	printf("  txPow is watts, rxNoise is dBW, bandW is Hz, percDays is %% of days.\n");
	printf("  day is echoed but unused: P.533 predicts monthly medians.\n\n");

}

int main(int argc, char *argv[]) {

	struct PathData path;
	struct Circuit c;
	struct Result r;

	char *infile = NULL, *outfile = NULL, *datapath = NULL;
	char *txant = NULL, *rxant = NULL;
	double gos = 0.0;
	int silent = FALSE;

	char line[CSVMAXLINE];
	char *field[CSVMAXFIELDS];
	int col[NINPUTCOLUMNS];
	int nf, nhdr, retval, number = 0, failed = 0;
	int loadedmonth = -1;
	FILE *fin, *fout;
	char dpath[256];

	for (int i = 1; i < argc; i++) {
		if      (strcmp(argv[i], "-i") == 0 && i+1 < argc) infile   = argv[++i];
		else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) outfile  = argv[++i];
		else if (strcmp(argv[i], "-d") == 0 && i+1 < argc) datapath = argv[++i];
		else if (strcmp(argv[i], "-t") == 0 && i+1 < argc) txant    = argv[++i];
		else if (strcmp(argv[i], "-r") == 0 && i+1 < argc) rxant    = argv[++i];
		else if (strcmp(argv[i], "-g") == 0 && i+1 < argc) gos      = atof(argv[++i]);
		else if (strcmp(argv[i], "-m") == 0 && i+1 < argc) FreqMargin = atof(argv[++i]);
		else if (strcmp(argv[i], "-s") == 0) silent = TRUE;
		else if (strcmp(argv[i], "-h") == 0) { PrintUsage(); return RTN_CSVOK; }
		else {
			printf("CircuitCSV: Error %d Unrecognised argument %s\n", RTN_ERRCSVARGS, argv[i]);
			PrintUsage();
			return RTN_ERRCSVARGS;
		}
	}

	if (infile == NULL || outfile == NULL || datapath == NULL) {
		printf("CircuitCSV: Error %d -i, -o and -d are required\n", RTN_ERRCSVARGS);
		PrintUsage();
		return RTN_ERRCSVARGS;
	}

	// ReadIonParametersBin() and ReadFamDud() expect a path with a separator.
	if ((size_t)snprintf(dpath, sizeof(dpath), "%s%s", datapath,
			(datapath[strlen(datapath)-1] == '/' || datapath[strlen(datapath)-1] == '\\') ? "" : "/")
		>= sizeof(dpath)) {
		printf("CircuitCSV: Error %d Data file path too long\n", RTN_ERRCSVARGS);
		return RTN_ERRCSVARGS;
	}

	retval = LoadP533();
	if (retval != RTN_CSVOK) return retval;

	// PathData is large and P533() reads fields this program does not set.
	// Clear it before AllocatePathMemory() fills in the pointers, so that no
	// result depends on whatever was on the stack.
	memset(&path, 0, sizeof(path));

	// PathData is large and P533() reads fields this program does not set.
	// Clear it before AllocatePathMemory() fills in the pointers, so that no
	// result depends on whatever was on the stack.
	memset(&path, 0, sizeof(path));

	retval = csvAllocatePathMemory(&path);
	if (retval != RTN_ALLOCATEP533OK) {
		printf("CircuitCSV: Error %d from AllocatePathMemory\n", retval);
		return retval;
	}

	// The antenna patterns and the P.1239 decile factors are read once and
	// reused by every circuit in the batch.
	if (LoadAntenna(&path.A_tx, txant, gos, silent) != RTN_CSVOK) return RTN_ERRCSVANTENNA;
	if (LoadAntenna(&path.A_rx, rxant, gos, silent) != RTN_CSVOK) return RTN_ERRCSVANTENNA;

	retval = csvReadP1239(&path, dpath);
	if (retval != RTN_READP1239OK) {
		printf("CircuitCSV: Error %d from ReadP1239\n", retval);
		return retval;
	}

	fin = fopen(infile, "r");
	if (fin == NULL) {
		printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENIN, infile, strerror(errno));
		return RTN_ERRCSVOPENIN;
	}

	if (fgets(line, sizeof(line), fin) == NULL) {
		printf("CircuitCSV: Error %d %s is empty\n", RTN_ERRCSVHEADER, infile);
		fclose(fin);
		return RTN_ERRCSVHEADER;
	}

	nhdr = SplitCSV(line, field, CSVMAXFIELDS);
	for (int i = 0; i < NINPUTCOLUMNS; i++) {
		col[i] = FindColumn(field, nhdr, InputColumns[i]);
		if (col[i] < 0) {
			printf("CircuitCSV: Error %d Header is missing the column '%s'\n",
				RTN_ERRCSVHEADER, InputColumns[i]);
			fclose(fin);
			return RTN_ERRCSVHEADER;
		}
	}

	fout = fopen(outfile, "w");
	if (fout == NULL) {
		printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENOUT, outfile, strerror(errno));
		fclose(fin);
		return RTN_ERRCSVOPENOUT;
	}
	PrintHeader(fout);

	while (fgets(line, sizeof(line), fin) != NULL) {

		if (Trim(line)[0] == '\0') continue;	// skip blank records

		nf = SplitCSV(line, field, CSVMAXFIELDS);
		if (ReadCircuit(&c, field, nf, col) != RTN_CSVOK) {
			printf("CircuitCSV: Warning: skipping short record %d\n", number+1);
			failed++;
			continue;
		}

		number++;

		if (c.month < 1 || c.month > 12) {
			printf("CircuitCSV: Warning: circuit %d has month %d, skipping\n", number, c.month);
			failed++;
			continue;
		}

		// The coefficients and maps are per month, so they are re-read only
		// when the month changes. A file sorted by month reads them once.
		if (c.month - 1 != loadedmonth) {
			retval = csvReadIonParametersBin(c.month - 1, path.foF2, path.M3kF2, dpath, silent);
			if (retval != RTN_READIONPARAOK) {
				printf("CircuitCSV: Error %d from ReadIonParametersBin for month %d\n", retval, c.month);
				fclose(fin); fclose(fout);
				return retval;
			}
			retval = csvReadFamDud(&path.noiseP, dpath, c.month - 1);
			if (retval != RTN_READFAMDUDOK) {
				printf("CircuitCSV: Error %d from ReadFamDud for month %d\n", retval, c.month);
				fclose(fin); fclose(fout);
				return retval;
			}
			loadedmonth = c.month - 1;
		}

		retval = RunCircuit(&path, &c, &r);
		if (retval != RTN_CSVOK) {
			printf("CircuitCSV: Warning: circuit %d returned %d from P533, skipping\n", number, retval);
			failed++;
			continue;
		}

		WriteRow(fout, &c, &r, number);

		if (silent == FALSE) printf("\rCircuit %d", number);
	}

	if (silent == FALSE) printf("\rProcessed %d circuit(s), %d skipped\n", number, failed);

	fclose(fin);
	fclose(fout);
	csvFreePathMemory(&path);
	if (P533Lib != NULL) CSVLIBCLOSE(P533Lib);
	if (P372Lib != NULL) CSVLIBCLOSE(P372Lib);

	return RTN_CSVOK;

}
