#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#endif
#ifdef __linux__
#include <sys/prctl.h>
#endif

// Local includes
#include "Common.h"
#include "P533.h"
#include "ITURHFProp.h"
#include "LoadLib.h"
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
			-F <spec>	also scan each circuit over a list of frequencies
			-S <file>	with -F, csv of every scanned frequency's results
			-j <n>		run in n worker processes, 0 for one per processor

		OUTPUT
			A csv with the input columns echoed, followed by the calculated
			columns. See PrintHeader() for the column list and the units.

		SUBROUTINES
			LoadP533(), LoadAntennas(), ReadCircuit(), RunCircuit(), WriteRow()
*/

// Library loading goes through the shared shim in LoadLib.h.
static HFLIBHANDLE P533Lib = NULL;
static HFLIBHANDLE P372Lib = NULL;

static int    (*csvP533)(struct PathData *);
static int    (*csvAllocatePathMemory)(struct PathData *);
static int    (*csvFreePathMemory)(struct PathData *);
static double (*csvBearing)(struct Location, struct Location, int);
static int    (*csvReadIonParametersBin)(int, float ****, float ****, char *, int);
static int    (*csvReadP1239)(struct PathData *, const char *);
static int    (*csvReadType13)(struct Antenna *, FILE *, double, int);
static void   (*csvIsotropicPattern)(struct Antenna *, double, int);
static int    (*csvReadType11)(struct Antenna *, FILE *, int);
static int    (*csvReadType14)(struct Antenna *, FILE *, int);
static double (*csvElevationAngle)(double, double);
static int    (*csvReadFamDud)(struct NoiseParams *, const char *, int);
static int    (*csvIonMapGet)(int, char *, int, float *****, float *****);
static void   (*csvIonMapFree)(void);
static void   (*csvFreeIonMaps)(struct PathData *);
static int    (*csvValidatePath)(struct PathData *);
static void   (*csvInitializePath)(struct PathData *);
static void   (*csvMUFBasic)(struct PathData *);
static void   (*csvMUFVariability)(struct PathData *);
static void   (*csvMUFOperational)(struct PathData *);

// The three characteristic frequencies each circuit is evaluated at.
#define FRQBUF	0	// basic MUF of the dominant mode
#define FRQMUF	1	// path MUF exceeded 50% of days
#define FRQOWF	2	// path MUF exceeded 90% of days, the FOT
#define NFRQ	3

// Azimuth resolution of a Type 13 pattern, per ReadType13().
#define ANTAZIMUTHS	360

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
	int    snok[NFRQ];		// FALSE when that frequency is outside P.533's 1-30 MHz
	double prob;			// basic circuit reliability at the BUF (%)
	double toa;				// take-off angle of the dominant mode (degrees)
	double loss;			// basic transmission loss of the dominant mode (dB)
	double delay;			// group delay of the dominant mode (s)
	double grange;			// group range of the dominant mode (km)
	double noiseRx;			// total noise at the receiver (dB above kT0B)
	double fM, fL;			// long-model upper/lower reference frequencies (MHz)
	double snfM, snfL;		// dB at those, when they are inside 1-30 MHz
	int    fMok, fLok;		// FALSE when not computed
	int    islong;			// TRUE when the long model was used
	int    scanned;			// TRUE when the -F scan ran on this circuit
	double luf;				// lowest scanned MHz with SNR >= reqSN, 0 if none
	double bestF;			// scanned MHz with the highest SNR, 0 if none ran
	double snBest, duBest, dlBest, bcrBest;	// SNR, its deciles and BCR there
	double bw10;			// 10 log10(bandW), to refer SN_Best to 1 Hz
	int    valid;
	const char *status;	// why a row has no results, or "OK"
};

static void PrintUsage(void);

// Antenna orientation; defined below, used by RunCircuit().
struct AntennaMaster {
	double ***rows;		// [freq][azimuth] as loaded, unrotated
	int      freqn;
	int      valid;
};
static struct AntennaMaster TxMaster, RxMaster;
static int  SaveAntennaMaster(struct AntennaMaster *m, struct Antenna *ant);
static void OrientAntenna(struct Antenna *ant, struct AntennaMaster *m, double bearing);

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

	// One row per entry point; the shim binds them and names the first missing.
	static const struct hfSymbol p533syms[] = {
		{ "P533",                 (void **)&csvP533 },
		{ "AllocatePathMemory",   (void **)&csvAllocatePathMemory },
		{ "FreePathMemory",       (void **)&csvFreePathMemory },
		{ "Bearing",              (void **)&csvBearing },
		{ "ReadIonParametersBin", (void **)&csvReadIonParametersBin },
		{ "ReadP1239",            (void **)&csvReadP1239 },
		{ "ReadType13",           (void **)&csvReadType13 },
		{ "ReadType11",           (void **)&csvReadType11 },
		{ "ReadType14",           (void **)&csvReadType14 },
		{ "ElevationAngle",       (void **)&csvElevationAngle },
		{ "IsotropicPattern",     (void **)&csvIsotropicPattern },
		{ "IonMapGet",            (void **)&csvIonMapGet },
		{ "IonMapFree",           (void **)&csvIonMapFree },
		{ "FreeIonMaps",          (void **)&csvFreeIonMaps },
		{ "ValidatePath",         (void **)&csvValidatePath },
		{ "InitializePath",       (void **)&csvInitializePath },
		{ "MUFBasic",             (void **)&csvMUFBasic },
		{ "MUFVariability",       (void **)&csvMUFVariability },
		{ "MUFOperational",       (void **)&csvMUFOperational },
	};
	static const struct hfSymbol p372syms[] = {
		{ "ReadFamDud",           (void **)&csvReadFamDud },
	};

	P533Lib = hfLibOpen(HFLIB_P533);
	if (P533Lib == NULL) {
		printf("CircuitCSV: Error %d Can't load %s (%s)\n",
			RTN_ERRCSVP533LIB, HFLIB_P533, hfLibError());
		return RTN_ERRCSVP533LIB;
	}

	P372Lib = hfLibOpen(HFLIB_P372);
	if (P372Lib == NULL) {
		printf("CircuitCSV: Error %d Can't load %s (%s)\n",
			RTN_ERRCSVP372LIB, HFLIB_P372, hfLibError());
		return RTN_ERRCSVP372LIB;
	}

	if (hfLibBind(P533Lib, p533syms, (int)(sizeof(p533syms)/sizeof(p533syms[0])), "CircuitCSV") == 0 ||
		hfLibBind(P372Lib, p372syms, (int)(sizeof(p372syms)/sizeof(p372syms[0])), "CircuitCSV") == 0) {
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
	size_t len;
	size_t i, j;

	// Leading and trailing white space first.
	while (*s == ' ' || *s == '\t') s++;
	end = s + strlen(s);
	while (end > s) {
		char c = *(end-1);
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') end--;
		else break;
	}
	*end = '\0';

	/*
		Then unquote, per RFC 4180: a field is quoted only when it both starts
		and ends with a quote, and an embedded quote is written doubled. This
		used to strip quote characters from either end greedily, which turned
		a name like  say "hi"  into  say ""hi  by removing the wrong ones and
		leaving the doubling in place.
	*/
	len = strlen(s);
	if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
		s[len-1] = '\0';
		s++;
		for (i = 0, j = 0; s[i] != '\0'; i++, j++) {
			if (s[i] == '"' && s[i+1] == '"') i++;	// collapse a doubled quote
			s[j] = s[i];
		}
		s[j] = '\0';
	}

	return s;

}

/*
	CsvStep() - Advances the csv quote state by one character.

		The one quoting rule shared by SplitCSV(), ReadRecord() and
		CopyRecord(), per RFC 4180: a quote opens a quoted field only when it
		is the first character of the field other than white space; inside
		one, a doubled quote is a literal quote and a single one closes it. A
		quote anywhere else, as in  Perth 12" dish , is an ordinary character.
		Every quote used to toggle the state, so that one such name ran on to
		the next quote in the file, however many rows later, and swallowed
		every row in between. A line break or a comma separates only outside
		an open quoted field (CSV_QUOTED); the caller tests that.

		INPUT
			int st, the state before ch; int ch

		OUTPUT
			returns the state after ch

		SUBROUTINES
			None
*/
enum { CSV_START, CSV_PLAIN, CSV_QUOTED, CSV_CLOSED, CSV_AFTER };

static int CsvStep(int st, int ch) {

	switch (st) {
		case CSV_START:
			if (ch == ' ' || ch == '\t') return CSV_START;
			if (ch == '"') return CSV_QUOTED;
			return (ch == ',') ? CSV_START : CSV_PLAIN;
		case CSV_QUOTED:
			return (ch == '"') ? CSV_CLOSED : CSV_QUOTED;
		case CSV_CLOSED:	// a quote inside a quoted field: doubled, or the close
			if (ch == '"') return CSV_QUOTED;
			if (ch == ' ' || ch == '\t' || ch == '\r') return CSV_CLOSED;
			return (ch == ',') ? CSV_START : CSV_AFTER;
		default:			// CSV_PLAIN, or text after a closing quote
			return (ch == ',') ? CSV_START : st;
	}

}

/*
	SplitCSV() - Splits one record into fields on commas.

		A comma inside a quoted field (see CsvStep()) does not split.

		INPUT
			char *line		the record, modified in place
			char **field	receives a pointer to each field
			int maxfields

		OUTPUT
			returns the number of fields found

		SUBROUTINES
			CsvStep(), Trim()
*/
static int SplitCSV(char *line, char **field, int maxfields) {

	int n = 0;
	int st = CSV_START;
	char *p = line;

	field[n++] = line;
	for (; *p != '\0'; p++) {
		if (*p == ',' && st != CSV_QUOTED) {
			*p = '\0';
			st = CSV_START;
			// Stop collecting, but still fall through to the trim below: an
			// early return here left every field untrimmed, quotes and all.
			if (n >= maxfields) break;
			field[n++] = p + 1;
		}
		else st = CsvStep(st, *p);
	}

	for (int i = 0; i < n; i++) field[i] = Trim(field[i]);

	return n;

}

/*
	ReadRecord() - Reads one logical csv record, of any length.

		fgets() into a fixed buffer split a record longer than the buffer into
		two rows, and a quoted field holding a newline into two as well; either
		broke the one-row-in, one-row-out numbering. This reads up to a newline
		that is outside a quoted field (see CsvStep()), growing the buffer as
		needed. The newline is kept, as fgets() kept it.

		A quoted field that runs over a line break is taken as malformed, so
		that one bad row can never swallow the rows after it, when it is still
		open at end of file, when it spans more than CSVMAXQLINES line breaks,
		or when its closing quote is followed by anything but white space, a
		comma or the end of the line (as when a stray opening quote pairs with
		the first quote of a later row). The record is then only its first
		line, returned as CSVBADQUOTE for the caller to report as BAD_RECORD,
		and reading resumes at the next line. A quote left open at the end of
		the file's last line is CSVBADQUOTE too.

		INPUT
			FILE *fp, char **buf, size_t *cap	(*buf may start NULL)

		OUTPUT
			returns TRUE with the record in *buf, CSVBADQUOTE with a malformed
			record's first line in *buf, FALSE at end of file, or -1 when memory
			runs out or the file cannot be repositioned

		SUBROUTINES
			CsvStep()
*/
#define CSVBADQUOTE		2
#define CSVMAXQLINES	64

static int ReadRecord(FILE *fp, char **buf, size_t *cap) {

	size_t n = 0, first = 0;
	long resume = -1;
	int ch, st = CSV_START, lines = 0, bad = FALSE;

	ch = getc(fp);
	if (ch == EOF) return FALSE;
	for (; ch != EOF; ch = getc(fp)) {
		if (n + 2 > *cap) {
			size_t nc = (*cap < CSVMAXLINE) ? CSVMAXLINE : *cap * 2;
			char *nb = (char *)realloc(*buf, nc);
			if (nb == NULL) return -1;
			*buf = nb;
			*cap = nc;
		}
		(*buf)[n++] = (char)ch;
		if (ch == '\n') {
			if (st != CSV_QUOTED) break;
			if (lines++ == 0) {
				first = n;
				resume = ftell(fp);
			}
			if (lines > CSVMAXQLINES) { bad = TRUE; break; }
		}
		else {
			st = CsvStep(st, ch);
			if (st == CSV_AFTER && lines > 0) { bad = TRUE; break; }
		}
	}
	if (ch == EOF && st == CSV_QUOTED) bad = TRUE;
	if (bad == TRUE && lines > 0) {
		if (resume < 0 || fseek(fp, resume, SEEK_SET) != 0) return -1;
		n = first;
	}
	(*buf)[n] = '\0';

	return (bad == TRUE) ? CSVBADQUOTE : TRUE;

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
	"year", "month", "day", "hour", "SSN", "minTOA",
	"txPow", "reqSN", "rxNoise", "bandW", "percDays"
};
#define NINPUTCOLUMNS ((int)(sizeof(InputColumns)/sizeof(InputColumns[0])))

// The solar index column, InputColumns[COLINDEX], may be given either as "SSN" --
// the 12-month smoothed sunspot number R12 on the SIDC version 2 scale, used as
// it is -- or as "t_Index", the IPS ionospheric T index, which is converted to
// SSN with the SIDC version 2 relation T = 4.90 + 0.670 SSN. If both are present
// SSN is used. P.533 takes SSN; IndexName records which column the file had, so
// that the output echoes it under its own name.
#define COLINDEX 10
static int IndexIsT = FALSE;
static const char *IndexName = "SSN";

/*
	SSNFromIndex() - The SSN P.533 is given for one row's solar index.

		SSN, the SIDC version 2 R12, is rounded to the integer the engine holds.
		A T index becomes SSN = (T - 4.90)/0.670, rounded. T falls below 4.90 at
		deep solar minimum, which would make SSN negative; P.533 section 3.4 gives
		the ionospheric maps for R12 from 0 upward and nothing below, so a negative
		result is taken as 0.

		INPUT
			double index, int isT

		OUTPUT
			returns SSN

		SUBROUTINES
			None
*/
static int SSNFromIndex(double index, int isT) {

	double ssn = isT ? (index - 4.90)/0.670 : index;

	if (ssn < 0.0) ssn = 0.0;

	return (int)floor(ssn + 0.5);

}

/*
	FieldNum(), FieldInt() - Parse one numeric field strictly.

		atof()/atoi() read an empty or non-numeric field as 0, so such a row
		ran with zeros and reported OK. These accept only a finite number with
		nothing but white space around it; FieldInt() also needs a whole
		number in int range ("4" or "4.0", not "4.5"). On failure *ok is
		cleared and the value is still the best-effort atof() one, for the echo.

		INPUT
			const char *s, int *ok

		OUTPUT
			returns the value

		SUBROUTINES
			None
*/
static double FieldNum(const char *s, int *ok) {

	char *end;
	double v;

	errno = 0;
	v = strtod(s, &end);
	if (end != s) while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') end++;
	if (end == s || *end != '\0' || errno == ERANGE || !isfinite(v)) {
		*ok = FALSE;
		return atof(s);
	}

	return v;

}

static int FieldInt(const char *s, int *ok) {

	int good = TRUE;
	double v = FieldNum(s, &good);

	if (good == FALSE || v != floor(v) || v < INT_MIN || v > INT_MAX) {
		*ok = FALSE;
		return atoi(s);
	}

	return (int)v;

}

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

	// Parse on a best-effort basis: a short record still yields a Circuit, with
	// the missing fields left empty, so that it can be echoed back and matched
	// to its input row. c->parsed says whether anything was missing, empty or
	// not a number.
	int complete = TRUE;
	for (int i = 0; i < NINPUTCOLUMNS; i++) {
		if (col[i] >= nf) complete = FALSE;
	}

	#define CSVFLD(i) ((col[i] < nf) ? f[col[i]] : "")

	// A site name that does not fit would be echoed cut short and no longer
	// match its input, so such a row is BAD_RECORD rather than silently altered.
	if ((size_t)snprintf(c->txSite, sizeof(c->txSite), "%s", CSVFLD(0)) >= sizeof(c->txSite)) complete = FALSE;
	c->txLat    = FieldNum(CSVFLD(1), &complete);
	c->txLon    = FieldNum(CSVFLD(2), &complete);
	if ((size_t)snprintf(c->rxSite, sizeof(c->rxSite), "%s", CSVFLD(3)) >= sizeof(c->rxSite)) complete = FALSE;
	c->rxLat    = FieldNum(CSVFLD(4), &complete);
	c->rxLon    = FieldNum(CSVFLD(5), &complete);
	c->year     = FieldInt(CSVFLD(6), &complete);
	c->month    = FieldInt(CSVFLD(7), &complete);
	c->day      = FieldInt(CSVFLD(8), &complete);
	c->hour     = FieldInt(CSVFLD(9), &complete);
	c->index    = FieldNum(CSVFLD(COLINDEX), &complete);
	c->ssn      = SSNFromIndex(c->index, IndexIsT);
	c->minTOA   = FieldNum(CSVFLD(11), &complete);
	c->txPow    = FieldNum(CSVFLD(12), &complete);
	c->reqSN    = FieldNum(CSVFLD(13), &complete);
	c->rxNoise  = FieldNum(CSVFLD(14), &complete);
	c->bandW    = FieldNum(CSVFLD(15), &complete);
	c->percDays = FieldNum(CSVFLD(16), &complete);

	#undef CSVFLD

	c->parsed = complete;

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
	path->SSN   = c->ssn;			// from SSN, or converted from t_Index

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
	Frequency scan (-F).

	The characteristic-frequency columns describe a circuit at three points.
	Coverage work needs the signal-to-noise ratio across the band: P.533 gives,
	for each frequency, the monthly median S/N and its upper and lower decile
	deviations (P.842 Table 1 Steps 3, 6 and 9), and none of the three depends
	on the required S/N or, beyond the 10 log10(b) term in Step 3, on the
	bandwidth. So one scan answers every service and threshold afterwards.

	-F takes either a range, start:stop:step in MHz, or a comma separated list,
	e.g. assigned frequencies. -S names a second csv that receives one row per
	circuit and frequency, keyed by Circuit#; the main output keeps its one row
	per input row and gains summary columns.
*/
#define SCANMAX 10000

// -j: worker processes, 1 for a serial run. Workers claim the rows in blocks of
// CSVBLOCK from a counter in shared memory, so a fast core takes more blocks than
// a slow one; each records the blocks it took, in order, in BlockIdx.
#define CSVMAXWORKERS 256
#define CSVBLOCK      32
static int Workers = 1;
static int *NextBlock = NULL;
static FILE *BlockIdx = NULL;
#ifndef _WIN32
// The -j parent's pid, as each worker saw it at fork: a worker whose parent
// has changed was orphaned and stops at its next block. The workers started
// so far, for the parent's SIGINT/SIGTERM handler to stop.
static pid_t ParentPid = 0;
static pid_t WorkerPid[CSVMAXWORKERS];
static volatile sig_atomic_t NStarted = 0;
static volatile sig_atomic_t StopSignal = 0;
#endif

struct Counts {
	int number;			// circuits that ran
	int failed;			// rows that never reached the engine
	int monthsused;		// month switches
};
static double *ScanF = NULL;
static int     NScan = 0;
static FILE   *ScanOut = NULL;

static int CompareDouble(const void *a, const void *b) {

	double x = *(const double *)a, y = *(const double *)b;

	return (x > y) - (x < y);

}

/*
	ParseScan() - Reads the -F specification into ScanF.

		"start:stop:step" gives start, start+step, ... up to stop inclusive; each
		value is rounded to 1 kHz so that 0.1 MHz steps do not accumulate binary
		fractions, and a value the rounding repeats is dropped. Anything else is read as a comma separated list, sorted
		ascending with duplicates removed. Every frequency must lie in P.533's
		1-30 MHz, which ValidatePath() enforces. NaN and infinity are refused
		outright: a NaN compares false with everything, so it slipped past the
		1-30 MHz test on the sorted ends.

		INPUT
			const char *spec

		OUTPUT
			returns RTN_CSVOK, or RTN_ERRCSVARGS

		SUBROUTINES
			CompareDouble()
*/
static int ParseScan(const char *spec) {

	double a, b, c;
	char tail;
	int n = 0;

	ScanF = (double *)calloc(SCANMAX, sizeof(double));
	if (ScanF == NULL) return RTN_ERRCSVARGS;

	if (strchr(spec, ':') != NULL) {
		if (sscanf(spec, "%lf:%lf:%lf%c", &a, &b, &c, &tail) != 3 ||
			!isfinite(a) || !isfinite(b) || !isfinite(c) || c <= 0.0 || b < a) {
			printf("CircuitCSV: Error %d -F range must be start:stop:step, finite numbers with step > 0 and stop >= start\n", RTN_ERRCSVARGS);
			return RTN_ERRCSVARGS;
		}
		for (int k = 0; ; k++) {
			double f = floor((a + k*c)*1000.0 + 0.5)/1000.0;
			if (f > b + 1e-9) break;
			if (n >= SCANMAX) {
				printf("CircuitCSV: Error %d -F gives more than %d frequencies\n", RTN_ERRCSVARGS, SCANMAX);
				return RTN_ERRCSVARGS;
			}
			// A step under 1 kHz rounds some values onto the one before; the
			// values ascend, so a repeat can only be the previous one.
			if (n > 0 && f == ScanF[n-1]) continue;
			ScanF[n++] = f;
		}
	}
	else {
		char buf[CSVMAXLINE], *tok, *end;
		snprintf(buf, sizeof(buf), "%s", spec);
		for (tok = strtok(buf, ","); tok != NULL; tok = strtok(NULL, ",")) {
			double f = strtod(tok, &end);
			while (*end == ' ') end++;
			if (end == tok || *end != '\0' || !isfinite(f)) {
				printf("CircuitCSV: Error %d -F list entry '%s' is not a finite number\n", RTN_ERRCSVARGS, tok);
				return RTN_ERRCSVARGS;
			}
			if (n >= SCANMAX) {
				printf("CircuitCSV: Error %d -F gives more than %d frequencies\n", RTN_ERRCSVARGS, SCANMAX);
				return RTN_ERRCSVARGS;
			}
			ScanF[n++] = f;
		}
		qsort(ScanF, n, sizeof(double), CompareDouble);
		{
			int m = 0;
			for (int k = 0; k < n; k++) if (m == 0 || ScanF[k] != ScanF[m-1]) ScanF[m++] = ScanF[k];
			n = m;
		}
	}

	if (n == 0 || ScanF[0] < 1.0 || ScanF[n-1] > 30.0) {
		printf("CircuitCSV: Error %d -F frequencies must lie within 1-30 MHz\n", RTN_ERRCSVARGS);
		return RTN_ERRCSVARGS;
	}

	NScan = n;

	return RTN_CSVOK;

}

/*
	ScanCircuit() - Runs one circuit at every -F frequency.

		Called after RunCircuit(), which has validated the path and pointed the
		antennas along it. Each frequency is a full P533() call, since the
		engine evaluates one frequency per call.

		The summary kept in the Result is P.533 section 9's LUF on the scan grid
		-- the lowest scanned frequency whose median S/N reaches reqSN -- and the
		frequency with the highest median S/N, with that S/N, its decile
		deviations and the BCR for reqSN there. With -S every frequency is also
		written out, so that nothing is lost to the summary.

		INPUT
			struct PathData *path, struct Circuit *c, struct Result *r, int number

		OUTPUT
			r->scanned and the summary fields; one -S row per frequency

		SUBROUTINES
			SetPath(), csvP533(), DominantMode()
*/
static void ScanCircuit(struct PathData *path, struct Circuit *c, struct Result *r, int number) {

	int hops;
	char layer;

	r->scanned = TRUE;
	r->luf = r->bestF = 0.0;
	r->bw10 = 10.0*log10(c->bandW);

	for (int k = 0; k < NScan; k++) {

		double f = ScanF[k];
		int ok;

		SetPath(path, c, f);
		ok = (csvP533(path) == RTN_P533OK);

		if (ok) {
			if (r->luf == 0.0 && path->SNR >= path->SNRr) r->luf = f;
			if (r->bestF == 0.0 || path->SNR > r->snBest) {
				r->bestF   = f;
				r->snBest  = path->SNR;
				r->duBest  = path->DuSN;
				r->dlBest  = path->DlSN;
				r->bcrBest = path->BCR;
			}
		}

		if (ScanOut == NULL) continue;

		if (!ok) {
			fprintf(ScanOut, "%d,%.6g,,,,,,,,,,P533_ERROR\n", number, f);
			continue;
		}

		{
			char mode[16] = "";
			if (DominantMode(path, &hops, &layer) == TRUE) snprintf(mode, sizeof(mode), "%d%c", hops, layer);
			fprintf(ScanOut, "%d,%.6g,%s,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,OK\n",
				number, f, mode, path->Pr, path->noiseP.FamT,
				path->SNR, path->SNR + r->bw10, path->DuSN, path->DlSN, path->BCR, path->SNRXX);
		}
	}

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

	/*
		Step 1: the characteristic frequencies, without a propagation run.

		MUFBasic(), MUFVariability() and MUFOperational() do not depend on the
		frequency of interest -- MUFVariability() reads path->frequency only to
		form each mode's Fprob, never to form a MUF -- so running the MUF chain
		alone yields BUF, MUF and OWF. This replaces what used to be a full
		P533() call on a seed frequency purely to discover them.
	*/
	SetPath(path, c, 10.0);

	retval = csvValidatePath(path);
	if (retval != RTN_VALIDDATAOK) return retval;

	csvInitializePath(path);
	csvMUFBasic(path);
	csvMUFVariability(path);
	csvMUFOperational(path);

	r->dist      = path->distance;
	r->txBearing = csvBearing(path->L_tx, path->L_rx, path->SorL) * R2D;
	r->rxBearing = csvBearing(path->L_rx, path->L_tx, path->SorL) * R2D;

	// Point each antenna along this circuit's great circle, as ITURHFProp's
	// AntennaOrientation TX2RX does, before any gain is evaluated.
	OrientAntenna(&path->A_tx, &TxMaster, r->txBearing * D2R);
	OrientAntenna(&path->A_rx, &RxMaster, r->rxBearing * D2R);

	/*
		The path-level MUFs, not the dominant mode's. P.533 sets a dominant mode
		only for paths it treats with the short model; between 7000 and 9000 km
		it interpolates, and beyond 9000 km it uses the long model, and in both
		cases path->DMptr stays NULL while the MUFs and the signal-to-noise
		ratio remain perfectly valid. Keying the whole circuit off the dominant
		mode discarded every result for any path over about 7000 km.
	*/
	r->f[FRQBUF] = path->BMUF;
	r->f[FRQMUF] = (path->OPMUF   > 0.0) ? path->OPMUF   : path->MUF50;
	r->f[FRQOWF] = (path->OPMUF90 > 0.0) ? path->OPMUF90 : path->MUF90;

	/*
		InitializePath() seeds the MUFs with 99.9 meaning "not calculated", and
		the basic-MUF chain only applies out to 9000 km. Beyond that P.533 uses
		the long model, which characterises the circuit by the upper and lower
		reference frequencies fM and fL instead, and leaves these at the
		sentinel. Report that rather than letting 99.9 reach the file as though
		it were a 99.9 MHz MUF.
	*/
	/*
		MUFBasic() signals "no mode supported" with path->BMUF = TOOBIG, which is
		DBL_MAX, so it must be tested BEFORE the long-path check -- DBL_MAX also
		satisfies >= 99.0 and short no-mode circuits were being labelled
		LONG_PATH, which left the NO_MODE branch unreachable.
	*/
	if (r->f[FRQBUF] >= TOOBIG) {
		r->f[FRQBUF] = r->f[FRQMUF] = r->f[FRQOWF] = 0.0;
		r->valid = FALSE;
		return RTN_CSVOK;			// caller labels this NO_MODE
	}

	if (r->f[FRQBUF] >= 99.0 || path->distance > 9000.0) {

		/*
			Beyond 9000 km P.533 uses the long model, which characterises the
			circuit by the upper and lower reference frequencies fM and fL
			rather than by a basic or operational MUF. Those come only from a
			propagation run -- MedianSkywaveFieldStrengthLong() sets them -- but
			they do not depend on the frequency of interest, so one run finds
			them and one run at each evaluates the circuit there.
		*/
		r->f[FRQBUF] = r->f[FRQMUF] = r->f[FRQOWF] = 0.0;
		r->islong = TRUE;
		r->status = "LONG_PATH";

		SetPath(path, c, 10.0);
		retval = csvP533(path);
		if (retval != RTN_P533OK) return retval;

		/*
			The long model sets the MUFs too (MedianSkywaveFieldStrengthLong.c
			lines 520-546), so read them back rather than leaving the zeros
			written above: they were being blanked one line before the run that
			computes them.
		*/
		if (path->BMUF > 0.0 && path->BMUF < 99.0) r->f[FRQBUF] = path->BMUF;
		if (path->OPMUF   > 0.0 && path->OPMUF   < 99.0) r->f[FRQMUF] = path->OPMUF;
		else if (path->MUF50 > 0.0 && path->MUF50 < 99.0) r->f[FRQMUF] = path->MUF50;
		if (path->OPMUF90 > 0.0 && path->OPMUF90 < 99.0) r->f[FRQOWF] = path->OPMUF90;
		else if (path->MUF90 > 0.0 && path->MUF90 < 99.0) r->f[FRQOWF] = path->MUF90;

		r->fM      = path->fM;
		r->fL      = path->fL;
		r->noiseRx = path->noiseP.FamT;
		r->prob    = path->BCR;

		if (r->fM >= 1.0 && r->fM <= 30.0) {
			SetPath(path, c, r->fM * FreqMargin);
			if (csvP533(path) == RTN_P533OK) { r->snfM = path->SNR; r->fMok = TRUE; }
		}
		if (r->fL >= 1.0 && r->fL <= 30.0) {
			SetPath(path, c, r->fL * FreqMargin);
			if (csvP533(path) == RTN_P533OK) { r->snfL = path->SNR; r->fLok = TRUE; }
		}

		r->valid = TRUE;
		return RTN_CSVOK;
	}

	if (r->f[FRQBUF] <= 0.0) {
		// No mode is supported at all: nothing propagates on this circuit.
		r->valid = FALSE;
		return RTN_CSVOK;
	}

	/*
		Step 2: one propagation run per characteristic frequency. Three runs are
		irreducible -- the signal-to-noise ratio depends on frequency through the
		field strength, the absorption and the noise, and P533() evaluates one
		frequency per call.
	*/
	for (int n = 0; n < NFRQ; n++) {

		double f = r->f[n] * FreqMargin;

		/*
			P.533 is defined from 1 to 30 MHz and ValidatePath() enforces it. A
			characteristic frequency can legitimately fall outside that -- an
			operational MUF above 30 MHz is common at low latitudes near solar
			maximum. The frequency is still a valid result, so it is reported
			and only its signal-to-noise ratio is left empty. Returning the
			engine's error here would have discarded the whole circuit.
		*/
		if (r->f[n] <= 0.0 || f < 1.0 || f > 30.0) continue;

		// -m evaluates a fraction below each characteristic frequency. It was
		// added to dodge an 8 dB step at the MUF that came from an incorrect
		// above-the-MUF loss; with Lm corrected to P.533-14 equations (24)-(26)
		// that step is gone, so the default of 1.0 is now the right choice and
		// the option remains only for probing below a MUF deliberately.
		SetPath(path, c, f);
		retval = csvP533(path);
		if (retval != RTN_P533OK) return retval;

		r->sn[n]   = path->SNR;
		r->snok[n] = TRUE;

		if (n == FRQBUF) {

			/*
				For 7000 < d < 9000 km, Between7000kmand9000km() overwrites
				path->BMUF with P.533 section 5.4's interpolation. That runs only
				inside P533(), so the MUF chain above could not see it and the
				BUF column carried the un-interpolated short-model value.
			*/
			if (path->distance > 7000.0 && path->distance < 9000.0 &&
				path->BMUF > 0.0 && path->BMUF < 99.0) {
				r->f[FRQBUF] = path->BMUF;
			}

			r->prob    = path->BCR;
			r->noiseRx = path->noiseP.FamT;

			// Mode, take-off angle, loss, delay and group range describe the
			// dominant mode, so they are reported only when there is one.
			if (DominantMode(path, &hops, &layer) == TRUE) {
				r->hops  = hops;
				r->layer = layer;
				r->toa   = path->DMptr->ele * R2D;
				r->loss  = path->DMptr->Lb;
				{
					/*
						Reproduce CircuitReliability()'s own calculation, which
						re-derives the elevation deliberately: the elevation
						stored on the mode was computed under the E-layer
						screening condition, which does not apply here. Using the
						stored one made Grange_BUF short by 210-416 km.
					*/
					double dh    = path->distance / hops;
					double hr    = (layer == 'E') ? 110.0 : path->DMptr->hr;
					double delta = csvElevationAngle(dh, hr);
					double psi   = dh / (2.0*R0);
					double ptick = 2.0*R0*(sin(psi)/cos(delta - psi));
					r->grange = hops * ptick;
					r->delay  = r->grange * 1000.0 / VofL;
				}
			}
		}
	}

	/*
		Only claim a result if at least one frequency was actually evaluated. All
		three can be skipped when they fall outside P.533's 1-30 MHz, and valid
		was set unconditionally, so such a row was written as OK with Prob and
		Noise reported as a computed 0.
	*/
	r->valid = (r->snok[FRQBUF] == TRUE || r->snok[FRQMUF] == TRUE || r->snok[FRQOWF] == TRUE)
	           ? TRUE : FALSE;
	if (r->valid != TRUE) r->status = "FREQ_RANGE";

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
		"txSite,txLat,txLon,rxSite,rxLat,rxLon,year,month,day,hour,%s,"
		"minTOA,txPow,reqSN,rxNoise,bandW,percDays,"
		"Circuit#,SSN_used,Dist,Tx-Bearing,Rx-Bearing,Mode,BUF,Prob,TOA,Losses,"
		"SN_BUF,Delay_BUF,Grange_BUF,Noise Rx,Noise Tx,MUF,SN_MUF,OWF,SN_OWF,"
		"fM,SN_fM,fL,SN_fL,%sStatus\n", IndexName,
		(NScan > 0) ? "LUF,BestF,SN_Best,SN0_Best,DuSN_Best,DlSN_Best,BCR_Best," : "");

}


/*
	CsvQuote() - Renders a field, quoting it when the csv grammar requires.

		A site name may contain a comma -- SplitCSV() reads one correctly from a
		quoted input field -- but writing it back raw split the row into more
		fields than the header has, which silently broke the one-row-in,
		one-row-out column alignment. A field containing a comma, a quote, a
		newline or leading or trailing space is wrapped in quotes with any
		embedded quote doubled, per RFC 4180.

		INPUT
			char *out, size_t n, const char *in

		OUTPUT
			returns out

		SUBROUTINES
			None
*/
static const char *CsvQuote(char *out, size_t n, const char *in) {

	size_t i, j = 0;
	int need = 0;

	if (in == NULL) { out[0] = '\0'; return out; }

	for (i = 0; in[i] != '\0'; i++) {
		if (in[i] == ',' || in[i] == '"' || in[i] == '\n' || in[i] == '\r') need = 1;
	}
	if (i > 0 && (in[0] == ' ' || in[i-1] == ' ')) need = 1;

	if (need == 0) {
		snprintf(out, n, "%s", in);
		return out;
	}

	if (n < 3) { out[0] = '\0'; return out; }
	out[j++] = '"';
	for (i = 0; in[i] != '\0' && j + 2 < n; i++) {
		if (in[i] == '"' && j + 3 < n) out[j++] = '"';	// double an embedded quote
		out[j++] = in[i];
	}
	out[j++] = '"';
	out[j] = '\0';

	return out;

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

		Every input row produces exactly one output row, in input order, with the
		input columns echoed ahead of the results. Circuit# is the input file's
		data row number and Status says why a row has no results, so an input
		row and its output row can always be matched one to one.

		INPUT
			FILE *fp, struct Circuit *c, struct Result *r, int number

		OUTPUT
			One csv record

		SUBROUTINES
			None
*/
/*
	WriteTail() - Ends a row: the -F summary columns, when scanning, and Status.

		The summary is blank when the scan did not run on the row, LUF is blank
		when no scanned frequency reached reqSN, and the columns at the best
		frequency are blank when no frequency ran. SN0_Best is SN_Best referred
		to 1 Hz, SN_Best + 10 log10(bandW), in dB-Hz.

		INPUT
			FILE *fp, struct Result *r, const char *dflt

		OUTPUT
			The rest of the record, newline included

		SUBROUTINES
			None
*/
static void WriteTail(FILE *fp, struct Result *r, const char *dflt) {

	if (NScan > 0) {
		if (r->scanned != TRUE) fprintf(fp, ",,,,,,,");
		else {
			if (r->luf > 0.0) fprintf(fp, "%.6g,", r->luf);
			else              fprintf(fp, ",");
			if (r->bestF > 0.0) {
				fprintf(fp, "%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,", r->bestF, r->snBest,
					r->snBest + r->bw10, r->duBest, r->dlBest, r->bcrBest);
			}
			else fprintf(fp, ",,,,,,");
		}
	}

	fprintf(fp, "%s\n", (r->status != NULL) ? r->status : dflt);

}

static void WriteRow(FILE *fp, struct Circuit *c, struct Result *r, int number) {

	// Site names are the only free-text input columns, so they are the only
	// ones that can need quoting on the way out.
	char txq[CSVMAXNAME*2+3], rxq[CSVMAXNAME*2+3];

	fprintf(fp, "%s,%.6g,%.6g,%s,%.6g,%.6g,%d,%d,%d,%d,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,%.6g,",
		CsvQuote(txq, sizeof(txq), c->txSite), c->txLat, c->txLon,
		CsvQuote(rxq, sizeof(rxq), c->rxSite), c->rxLat, c->rxLon,
		c->year, c->month, c->day, c->hour, c->index,
		c->minTOA, c->txPow, c->reqSN, c->rxNoise, c->bandW, c->percDays);

	if (r->valid == FALSE) {
		// No results. Geometry is still meaningful when the circuit ran but no
		// mode was supported; it is zero when the circuit never ran at all.
		fprintf(fp, "%d,%d,%.6g,%.6g,%.6g,NONE,,,,,,,,,,,,,,,,,,",
			number, c->ssn, r->dist, r->txBearing, r->rxBearing);
		WriteTail(fp, r, "NO_MODE");
		return;
	}

	// A path longer than about 7000 km has no dominant mode, so the columns that
	// describe one are left empty while the frequencies, SNRs and noise are not.
	// SNR columns are blank where the frequency fell outside P.533's 1-30 MHz.
	char snbuf[3][32];
	for (int n = 0; n < NFRQ; n++) {
		if (r->snok[n] == TRUE) snprintf(snbuf[n], sizeof(snbuf[n]), "%.6g", r->sn[n]);
		else                    snbuf[n][0] = '\0';
	}

	// BUF, MUF and OWF do not exist on the long model, so they print blank
	// rather than as a zero.
	char fbuf[3][32];
	for (int n = 0; n < NFRQ; n++) {
		if (r->f[n] > 0.0) snprintf(fbuf[n], sizeof(fbuf[n]), "%.6g", r->f[n]);
		else               fbuf[n][0] = '\0';
	}

	// fM and fL exist only on the long model, so they are blank on short paths.
	char fmbuf[32] = "", fmsn[32] = "", flbuf[32] = "", flsn[32] = "";
	if (r->islong == TRUE) {
		snprintf(fmbuf, sizeof(fmbuf), "%.6g", r->fM);
		snprintf(flbuf, sizeof(flbuf), "%.6g", r->fL);
		if (r->fMok == TRUE) snprintf(fmsn, sizeof(fmsn), "%.6g", r->snfM);
		if (r->fLok == TRUE) snprintf(flsn, sizeof(flsn), "%.6g", r->snfL);
	}

	// A path longer than about 7000 km has no dominant mode, so the columns that
	// describe one are left empty while the frequencies, SNRs and noise are not.
	if (r->hops > 0) {
		fprintf(fp, "%d,%d,%.6g,%.6g,%.6g,%d%c,%s,%.6g,%.6g,%.6g,%s,%.6g,%.6g,%.6g,%.6g,%s,%s,%s,%s,%s,%s,%s,%s,",
			number, c->ssn, r->dist, r->txBearing, r->rxBearing,
			r->hops, r->layer,
			fbuf[FRQBUF], r->prob, r->toa, r->loss, snbuf[FRQBUF],
			r->delay, r->grange,
			r->noiseRx, r->noiseRx,
			fbuf[FRQMUF], snbuf[FRQMUF],
			fbuf[FRQOWF], snbuf[FRQOWF],
			fmbuf, fmsn, flbuf, flsn);
	}
	else {
		fprintf(fp, "%d,%d,%.6g,%.6g,%.6g,,%s,%.6g,,,%s,,,%.6g,%.6g,%s,%s,%s,%s,%s,%s,%s,%s,",
			number, c->ssn, r->dist, r->txBearing, r->rxBearing,
			fbuf[FRQBUF], r->prob, snbuf[FRQBUF],
			r->noiseRx, r->noiseRx,
			fbuf[FRQMUF], snbuf[FRQMUF],
			fbuf[FRQOWF], snbuf[FRQOWF],
			fmbuf, fmsn, flbuf, flsn);
	}
	WriteTail(fp, r, "OK");

}


/*
	Antenna orientation.

	ReadType13() stores a pattern indexed by ABSOLUTE azimuth: it applies an
	integer offset iMBOS = (int)(bearing*R2D) so that pattern[az] holds the
	file's gain for (az - bearing). AntennaGain() then looks the pattern up at
	the path's own bearing, so a pattern loaded at one bearing is only correct
	for circuits on that bearing.

	Loading the file once at bearing 0.0 and never re-orienting left every
	circuit in a batch using a pattern pointed at true north. Re-reading the
	file per circuit would be 100,000 file reads, but the rotation is only a
	cyclic shift of the azimuth axis, so keeping the unrotated rows and
	permuting the 360 row pointers per circuit costs 360 pointer writes.
*/
/*
	SaveAntennaMaster() - Keeps the unrotated azimuth rows of a loaded pattern.

		INPUT
			struct AntennaMaster *m, struct Antenna *ant

		OUTPUT
			returns TRUE on success

		SUBROUTINES
			None
*/
static int SaveAntennaMaster(struct AntennaMaster *m, struct Antenna *ant) {

	int f, a;

	m->valid = FALSE;
	if (ant->pattern == NULL || ant->freqn <= 0) return FALSE;

	m->rows = (double ***)calloc(ant->freqn, sizeof(double **));
	if (m->rows == NULL) return FALSE;
	for (f = 0; f < ant->freqn; f++) {
		m->rows[f] = (double **)calloc(ANTAZIMUTHS, sizeof(double *));
		if (m->rows[f] == NULL) return FALSE;
		for (a = 0; a < ANTAZIMUTHS; a++) m->rows[f][a] = ant->pattern[f][a];
	}
	m->freqn = ant->freqn;
	m->valid = TRUE;

	return TRUE;

}

/*
	OrientAntenna() - Points a loaded pattern along the given bearing.

		Equivalent to having called ReadType13() with this bearing, but without
		re-reading the file: the azimuth axis is a cyclic shift, so only the row
		pointers move.

		INPUT
			struct Antenna *ant, struct AntennaMaster *m, double bearing (radians)

		OUTPUT
			ant->pattern re-indexed for that bearing

		SUBROUTINES
			None
*/
static void OrientAntenna(struct Antenna *ant, struct AntennaMaster *m, double bearing) {

	int f, a, iMBOS;

	if (m->valid != TRUE) return;		// ISOTROPIC, or nothing loaded

	iMBOS = (int)(bearing*R2D);
	iMBOS = ((iMBOS % ANTAZIMUTHS) + ANTAZIMUTHS) % ANTAZIMUTHS;

	for (f = 0; f < m->freqn; f++) {
		for (a = 0; a < ANTAZIMUTHS; a++) {
			ant->pattern[f][(iMBOS + a) % ANTAZIMUTHS] = m->rows[f][a];
		}
	}

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
	int retval;

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

	/*
		Identify the VOACAP antenna type the way ReadAntennaPatterns() does. The
		type marker is on the fourth line; ReadType13() reads it but never checks
		it, so handing every file to ReadType13() parsed a Type 11 or Type 14
		pattern into garbage gains with no error.
	*/
	{
		char line[256];
		int antType = 0, i;

		for (i = 0; i < 4; i++) {
			if (fgets(line, sizeof(line), fp) == NULL) {
				printf("CircuitCSV: Error %d Antenna file %s is too short\n", RTN_ERRCSVANTENNA, spec);
				fclose(fp);
				return RTN_ERRCSVANTENNA;
			}
		}
		sscanf(line, " %d", &antType);
		rewind(fp);

		// A bearing of 0.0 loads the pattern unrotated; OrientAntenna() points
		// it along each circuit's own great circle.
		if (antType == 11)      retval = csvReadType11(ant, fp, silent);
		else if (antType == 13) retval = csvReadType13(ant, fp, 0.0, silent);
		else if (antType == 14) retval = csvReadType14(ant, fp, silent);
		else {
			printf("CircuitCSV: Error %d Unsupported antenna type %d in %s\n",
				RTN_ERRCSVANTENNA, antType, spec);
			fclose(fp);
			return RTN_ERRCSVANTENNA;
		}

		if (retval != RTN_READANTENNAPATTERNSOK) {
			printf("CircuitCSV: Error %d Can't read antenna file %s\n", RTN_ERRCSVANTENNA, spec);
			fclose(fp);
			return RTN_ERRCSVANTENNA;
		}
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
	printf("  -m <factor> Evaluate at factor x each characteristic frequency\n");
	printf("              (default 1.0). The frequency columns still report the\n");
	printf("              true frequency; only the SN_ columns move with it.\n");
	printf("  -F <spec>   Also scan every circuit over a set of frequencies, MHz:\n");
	printf("              start:stop:step (e.g. 2:30:0.5) or a list (e.g. 4.5,7.1,11.2),\n");
	printf("              all within 1-30. Adds LUF, BestF, SN_Best, SN0_Best, DuSN_Best,\n");
	printf("              DlSN_Best and BCR_Best before Status.\n");
	printf("  -S <file>   With -F, also write every scanned frequency's results to this\n");
	printf("              csv, one row per circuit and frequency, keyed by Circuit#\n");
	printf("  -j <n>      Run in n worker processes (0: one per processor). Output\n");
	printf("              is identical to a serial run. Not on Windows.\n");
	printf("  -s          Silent: suppress progress output\n");
	printf("  -h          This help\n\n");
	printf("Input columns (looked up by name, order and extra columns do not matter):\n");
	printf("  txSite txLat txLon rxSite rxLat rxLon year month day hour\n");
	printf("  SSN|t_Index minTOA txPow reqSN rxNoise bandW percDays\n\n");
	printf("  SSN is the 12-month smoothed sunspot number R12, SIDC version 2, used as\n");
	printf("  given. t_Index, the IPS T index, is accepted instead and converted as\n");
	printf("  SSN = (T - 4.90)/0.670 (negative results taken as 0). If both columns are\n");
	printf("  present SSN is used. The SSN_used output column shows the value applied.\n");
	printf("  txPow is watts, rxNoise is dBW, bandW is Hz, percDays is %% of days.\n");
	printf("  day is echoed but unused: P.533 predicts monthly medians.\n");
	printf("  Site names over %d characters make the row BAD_RECORD.\n\n", CSVMAXNAME - 1);
	printf("Every non-blank input row gives exactly one output row, in input order;\n");
	printf("Circuit# is its data row number. Blank lines are skipped and not numbered.\n\n");

}

/*
	PrintScanHeader() - Writes the header of the -S file.

		SN0 is the S/N referred to 1 Hz, SNR + 10 log10(bandW), in dB-Hz; DuSN
		and DlSN are its upper and lower decile deviations; BCR is for reqSN and
		SNRXX the S/N exceeded on percDays of the days.
*/
static void PrintScanHeader(FILE *fp) {

	fprintf(fp, "Circuit#,Freq,Mode,Pr,Noise,SNR,SN0,DuSN,DlSN,BCR,SNRXX,Status\n");

}

/*
	ProcessRows() - Reads, calculates and writes the data rows.

		Streams the input: one row read, calculated and written at a time. The
		ionospheric maps are held by the library's month cache, so a month is
		still read at most once however the file is ordered, while memory stays
		flat in the number of circuits. This is what lets the tool take batches
		of 100,000+ rows.

		Every non-blank data row is numbered. A serial run (nworkers 1)
		calculates them all. A worker process claims blocks of CSVBLOCK rows from
		the shared NextBlock counter and calculates only those, skipping the
		rows between, and writes each block number it takes to BlockIdx so that
		the parent can put the blocks back in order.

		INPUT
			FILE *fin, positioned after the header; FILE *fout
			struct PathData *path, int *col, const char *dpath, int silent
			int worker, int nworkers

		OUTPUT
			returns RTN_CSVOK, or the error that stopped the run
			cnt, the circuits processed and skipped and the month changes

		SUBROUTINES
			SplitCSV(), ReadCircuit(), RunCircuit(), ScanCircuit(), WriteRow()
*/
static int ProcessRows(FILE *fin, FILE *fout, struct PathData *path, int *col, const char *dpath,
					   int silent, int worker, int nworkers, struct Counts *cnt) {

	char *line = NULL;
	size_t cap = 0;
	char *field[CSVMAXFIELDS];
	struct Circuit c;
	struct Result  r;
	int nrow = 0, loadedmonth = -1, nf, retval, rc;
	int mine = -1;			// the block this worker holds
	char dp[256];

	(void)worker;

	snprintf(dp, sizeof(dp), "%s", dpath);

	while ((rc = ReadRecord(fin, &line, &cap)) > 0) {

		// Test for a blank record without touching it: Trim() unquotes a FIELD
		// in place, so running it on the whole record stripped the closing quote
		// of the last field before SplitCSV() ever saw it.
		{
			const char *scan = line;
			while (*scan == ' ' || *scan == '\t' || *scan == '\r' || *scan == '\n') scan++;
			if (*scan == '\0') continue;	// skip blank records
		}

		nrow++;
		if (nworkers > 1) {
			int blk = (nrow - 1) / CSVBLOCK;
			// Claims only increase, so a claim behind the reader is simply
			// taken again; a claim ahead is skipped to.
#ifndef _WIN32
			// Where there is no PR_SET_PDEATHSIG (macOS), this is how a worker
			// learns that the parent died: it is re-parented.
			if (blk > mine && getppid() != ParentPid) {
				free(line);
				return RTN_ERRCSVWORKER;
			}
#endif
			while (blk > mine) mine = __atomic_fetch_add(NextBlock, 1, __ATOMIC_SEQ_CST);
			if (blk < mine) continue;
			if ((nrow - 1) % CSVBLOCK == 0) fprintf(BlockIdx, "%d\n", blk);
		}

		nf = SplitCSV(line, field, CSVMAXFIELDS);
		ReadCircuit(&c, field, nf, col);
		c.row = nrow;
		if (rc == CSVBADQUOTE) c.parsed = FALSE;	// a quote left open

		memset(&r, 0, sizeof(r));

		// A row that is short, or carries a month outside 1-12, still appears in
		// the output; it just never reaches the engine.
		if (c.parsed != TRUE) {
			r.status = "BAD_RECORD";
			cnt->failed++;
		}
		else if (c.month < 1 || c.month > 12) {
			r.status = "BAD_MONTH";
			cnt->failed++;
		}
		else {
			if (c.month - 1 != loadedmonth) {

				float ****foF2, ****M3kF2;

				retval = csvIonMapGet(c.month - 1, dp, silent, &foF2, &M3kF2);
				if (retval != RTN_READIONPARAOK) {
					printf("CircuitCSV: Error %d from IonMapGet for month %d\n", retval, c.month);
					free(line);
					return retval;
				}
				// Point at the cache rather than copying 10.7 MB per switch.
				path->foF2  = foF2;
				path->M3kF2 = M3kF2;

				retval = csvReadFamDud(&path->noiseP, dp, c.month - 1);
				if (retval != RTN_READFAMDUDOK) {
					printf("CircuitCSV: Error %d from ReadFamDud for month %d\n", retval, c.month);
					free(line);
					return retval;
				}
				loadedmonth = c.month - 1;
				cnt->monthsused++;
			}

			retval = RunCircuit(path, &c, &r);
			if (retval != RTN_CSVOK) {
				printf("CircuitCSV: Warning: input row %d returned error %d from P533\n", c.row, retval);
				r.valid = FALSE;
				r.status = "P533_ERROR";
				cnt->failed++;
			}
			else {
				// RunCircuit() may already have said why there are no results.
				if (r.status == NULL) r.status = (r.valid == TRUE) ? "OK" : "NO_MODE";
				cnt->number++;
				// The path validated, so every circuit that reaches here is
				// scanned, NO_MODE and LONG_PATH ones included: whether any
				// scanned frequency propagates is what the scan is for.
				if (NScan > 0) ScanCircuit(path, &c, &r, c.row);
			}
		}

		// One output row per input row, written as it is produced.
		WriteRow(fout, &c, &r, c.row);

		if (silent == FALSE && nworkers == 1 && (nrow % 1000) == 0) printf("\rCircuit %d", nrow);
	}

	free(line);
	if (rc < 0 || ferror(fin)) {
		printf("CircuitCSV: Error %d reading the input after row %d\n", RTN_ERRCSVFIELD, nrow);
		return RTN_ERRCSVFIELD;
	}

	return RTN_CSVOK;

}

#ifndef _WIN32
/*
	CopyRecord() - Copies one record from in to out, of any length.

		A quoted site name may hold a newline, so a record ends only at a
		newline outside a quoted field, by the rule ReadRecord() reads with.

		INPUT
			FILE *in, FILE *out

		OUTPUT
			returns the line's last field (Status) in status, truncated to n,
			and FALSE at end of file
*/
static int CopyRecord(FILE *in, FILE *out, char *status, size_t n) {

	int ch, st = CSV_START;
	size_t j = 0;

	ch = getc(in);
	if (ch == EOF) return FALSE;
	for (; ch != EOF && (ch != '\n' || st == CSV_QUOTED); ch = getc(in)) {
		putc(ch, out);
		if (ch == ',' && st != CSV_QUOTED) st = CSV_START;
		else st = CsvStep(st, ch);
		if (ch == ',' && st == CSV_START) j = 0;
		else if (j + 1 < n) status[j++] = (char)ch;
	}
	putc('\n', out);
	status[j] = '\0';

	return TRUE;

}

/*
	StopWorkers() - The -j parent's SIGINT/SIGTERM handler.

		Records the signal and passes SIGTERM on to every worker started, so
		that the parent's waitpid() returns promptly; RunParallel() then
		removes the part files and re-raises the signal. Only kill() is
		called, which is async-signal-safe.

		INPUT
			int sig

		OUTPUT
			StopSignal

		SUBROUTINES
			None
*/
static void StopWorkers(int sig) {

	StopSignal = sig;
	for (int k = 0; k < NStarted; k++) kill(WorkerPid[k], SIGTERM);

}

/*
	RunParallel() - Runs the rows in Workers processes and merges their output.

		The parent has read the header and loaded everything that is read once;
		each forked child inherits that, reopens the input for its own file
		position, and runs ProcessRows() on its share of the rows into
		<out>.part<k> (and <scan>.part<k>). Once all have finished, the parent
		puts the blocks back in order from the workers' block indexes -- every
		data row gives exactly one output row, so a block is CSVBLOCK lines of
		its worker's part -- and -S rows by their Circuit#, since a row gives
		none or one per frequency.
		The parts are then removed. Any worker failing fails the run.

		SIGINT or SIGTERM to the parent stops the workers, removes the parts
		and ends the process by that signal. If fork() fails part-way, the
		workers already started are stopped rather than left to run the job.
		A worker dies with its parent: through PR_SET_PDEATHSIG on Linux, and
		everywhere by checking getppid() before each block it takes.

		INPUT
			FILE *fin, const char *infile, *outfile, *scanfile
			struct PathData *path, int *col, const char *dpath, int silent

		OUTPUT
			returns RTN_CSVOK, or an error

		SUBROUTINES
			ProcessRows(), CopyRecord(), PrintHeader(), PrintScanHeader()
*/
static int RunParallel(FILE *fin, const char *infile, const char *outfile, const char *scanfile,
					   struct PathData *path, int *col, const char *dpath, int silent) {

	// The part names were CSVMAXNAME+16 bytes on the stack, and snprintf()
	// silently cut a long output path's ".part<k>" suffix off, so workers
	// shared one file. They are sized for any path now, on the heap, and a
	// name that still does not fit stops the run.
	#define CSVPARTNAME (PATH_MAX + 32)
	char (*outpart)[CSVPARTNAME] = malloc(sizeof(*outpart) * CSVMAXWORKERS);
	char (*scanpart)[CSVPARTNAME] = malloc(sizeof(*scanpart) * CSVMAXWORKERS);
	char (*idxpart)[CSVPARTNAME] = malloc(sizeof(*idxpart) * CSVMAXWORKERS);
	int nextidx[CSVMAXWORKERS];
	struct sigaction sa, oldint, oldterm;
	sigset_t block, oldmask;
	pid_t parent = getpid();
	int stopped = 0;			// the workers were sent SIGTERM
	int k, bad = 0, number = 0, failed = 0, nrow = 0;
	FILE *fout, *fsout = NULL, *pin[CSVMAXWORKERS], *sin[CSVMAXWORKERS], *xin[CSVMAXWORKERS];
	char pend[CSVMAXWORKERS][512];
	int havepend[CSVMAXWORKERS];
	time_t t0 = time(NULL);

	if (outpart == NULL || scanpart == NULL || idxpart == NULL) {
		printf("CircuitCSV: Error %d Out of memory\n", RTN_ERRCSVWORKER);
		free(outpart); free(scanpart); free(idxpart);
		return RTN_ERRCSVWORKER;
	}
	for (k = 0; k < Workers; k++) {
		if ((size_t)snprintf(outpart[k], CSVPARTNAME, "%s.part%d", outfile, k) >= CSVPARTNAME ||
			(size_t)snprintf(idxpart[k], CSVPARTNAME, "%s.part%d.idx", outfile, k) >= CSVPARTNAME ||
			(scanfile != NULL &&
			 (size_t)snprintf(scanpart[k], CSVPARTNAME, "%s.part%d", scanfile, k) >= CSVPARTNAME)) {
			printf("CircuitCSV: Error %d Output path too long for -j\n", RTN_ERRCSVARGS);
			free(outpart); free(scanpart); free(idxpart);
			return RTN_ERRCSVARGS;
		}
	}

	NextBlock = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	if (NextBlock == MAP_FAILED) {
		printf("CircuitCSV: Error %d Can't share memory with workers (%s)\n", RTN_ERRCSVWORKER, strerror(errno));
		free(outpart); free(scanpart); free(idxpart);
		return RTN_ERRCSVWORKER;
	}
	*NextBlock = 0;

	if (silent == FALSE) printf("CircuitCSV: %d workers\n", Workers);
	fflush(stdout);

	// The handler goes in before the first fork, with the two signals held
	// until every worker is started and recorded in WorkerPid.
	StopSignal = 0;
	NStarted = 0;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = StopWorkers;
	sigemptyset(&sa.sa_mask);
	// A signal ignored on entry (nohup, a background job) stays ignored.
	sigaction(SIGINT, NULL, &oldint);
	sigaction(SIGTERM, NULL, &oldterm);
	if (oldint.sa_handler != SIG_IGN) sigaction(SIGINT, &sa, NULL);
	if (oldterm.sa_handler != SIG_IGN) sigaction(SIGTERM, &sa, NULL);
	sigemptyset(&block);
	sigaddset(&block, SIGINT);
	sigaddset(&block, SIGTERM);
	sigprocmask(SIG_BLOCK, &block, &oldmask);

	for (k = 0; k < Workers; k++) {
		pid_t p = fork();
		if (p < 0) {
			printf("CircuitCSV: Error %d Can't start worker %d (%s)\n", RTN_ERRCSVWORKER, k, strerror(errno));
			Workers = k;			// wait only for those started
			for (int i = 0; i < k; i++) kill(WorkerPid[i], SIGTERM);
			stopped = 1;
			bad = 1;
			break;
		}
		if (p == 0) {
			// The worker. It must not share the parent's FILE position, so it
			// opens the input afresh and skips the header.
			char *hdr = NULL;
			size_t hcap = 0;
			struct Counts cnt = { 0, 0, 0 };
			FILE *in = fopen(infile, "r"), *out = fopen(outpart[k], "w");
			int rv;

			sigaction(SIGINT, &oldint, NULL);
			sigaction(SIGTERM, &oldterm, NULL);
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			ParentPid = parent;
#ifdef __linux__
			prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif
			if (getppid() != parent) _exit(2);	// the parent died already

			BlockIdx = fopen(idxpart[k], "w");
			if (in == NULL || out == NULL || BlockIdx == NULL || ReadRecord(in, &hdr, &hcap) != TRUE) _exit(2);
			free(hdr);
			if (scanfile != NULL) {
				ScanOut = fopen(scanpart[k], "w");
				if (ScanOut == NULL) _exit(2);
			}
			rv = ProcessRows(in, out, path, col, dpath, TRUE, k, Workers, &cnt);
			if (getppid() != parent) {
				// Orphaned: nobody will merge or remove the parts.
				remove(outpart[k]);
				remove(idxpart[k]);
				if (scanfile != NULL) remove(scanpart[k]);
				_exit(1);
			}
			fclose(in);
			if (fclose(out) != 0) rv = RTN_ERRCSVOPENOUT;
			if (ScanOut != NULL && fclose(ScanOut) != 0) rv = RTN_ERRCSVOPENOUT;
			if (fclose(BlockIdx) != 0) rv = RTN_ERRCSVOPENOUT;
			fflush(stdout);
			_exit(rv == RTN_CSVOK ? 0 : 1);
		}
		WorkerPid[k] = p;
		NStarted = k + 1;
	}
	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	for (k = 0; k < Workers; k++) {
		int st;
		pid_t w;
		while ((w = waitpid(WorkerPid[k], &st, 0)) < 0 && errno == EINTR) ;
		if (StopSignal != 0 || stopped) bad = 1;
		else if (w < 0 || !WIFEXITED(st) || WEXITSTATUS(st) != 0) {
			printf("CircuitCSV: Error %d worker %d failed\n", RTN_ERRCSVWORKER, k);
			bad = 1;
		}
	}
	// All reaped: a signal from here on must not reach a pid since reused.
	NStarted = 0;
	(void)fin;

	if (bad == 0) {
		fout = fopen(outfile, "w");
		if (fout == NULL) {
			printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENOUT, outfile, strerror(errno));
			bad = 1;
		}
		else {
			PrintHeader(fout);
			if (scanfile != NULL) {
				fsout = fopen(scanfile, "w");
				if (fsout == NULL) {
					printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENOUT, scanfile, strerror(errno));
					bad = 1;
				}
				else PrintScanHeader(fsout);
			}
			for (k = 0; k < Workers; k++) {
				pin[k] = fopen(outpart[k], "r");
				sin[k] = (fsout != NULL) ? fopen(scanpart[k], "r") : NULL;
				havepend[k] = (sin[k] != NULL && fgets(pend[k], sizeof(pend[k]), sin[k]) != NULL);
				xin[k] = fopen(idxpart[k], "r");
				if (xin[k] == NULL || fscanf(xin[k], "%d", &nextidx[k]) != 1) nextidx[k] = -1;
				if (pin[k] == NULL || (fsout != NULL && sin[k] == NULL)) bad = 1;
			}

			// Block b is held by the worker whose index lists it next. Its rows
			// are the next CSVBLOCK lines of that worker's part (fewer for the
			// last block), and each row's -S lines are the lines of the same
			// worker's -S part that carry its Circuit#.
			for (int b = 0; bad == 0 && StopSignal == 0; b++) {
				for (k = 0; k < Workers && nextidx[k] != b; k++) ;
				if (k == Workers) break;			// no such block: the end
				for (int j = 0; j < CSVBLOCK; j++) {
					char status[32];
					if (CopyRecord(pin[k], fout, status, sizeof(status)) == FALSE) break;
					nrow++;
					if (strcmp(status, "BAD_RECORD") == 0 || strcmp(status, "BAD_MONTH") == 0 ||
						strcmp(status, "P533_ERROR") == 0) failed++;
					else number++;
					while (fsout != NULL && havepend[k] && atoi(pend[k]) == nrow) {
						fputs(pend[k], fsout);
						havepend[k] = (fgets(pend[k], sizeof(pend[k]), sin[k]) != NULL);
					}
				}
				if (fscanf(xin[k], "%d", &nextidx[k]) != 1) nextidx[k] = -1;
			}

			for (k = 0; k < Workers; k++) {
				if (pin[k] != NULL) fclose(pin[k]);
				if (sin[k] != NULL) fclose(sin[k]);
				if (xin[k] != NULL) fclose(xin[k]);
			}
			if (fsout != NULL && fclose(fsout) != 0) bad = 1;
			if (fclose(fout) != 0) bad = 1;
		}
	}

	for (k = 0; k < Workers; k++) {
		remove(outpart[k]);
		remove(idxpart[k]);
		if (scanfile != NULL) remove(scanpart[k]);
	}

	munmap(NextBlock, sizeof(int));
	NextBlock = NULL;
	free(outpart); free(scanpart); free(idxpart);
	#undef CSVPARTNAME

	sigaction(SIGINT, &oldint, NULL);
	sigaction(SIGTERM, &oldterm, NULL);
	if (StopSignal != 0) {
		// The parts are gone; end as the signal would have ended the run.
		printf("CircuitCSV: Interrupted, workers stopped\n");
		fflush(stdout);
		signal(StopSignal, SIG_DFL);
		raise(StopSignal);
		return RTN_ERRCSVWORKER;
	}

	if (bad != 0) return RTN_ERRCSVWORKER;

	if (silent == FALSE) {
		printf("Processed %d circuit(s), %d skipped, %d worker(s), %ld s\n",
			number, failed, Workers, (long)(time(NULL) - t0));
	}

	return RTN_CSVOK;

}
#else
static int RunParallel(FILE *fin, const char *infile, const char *outfile, const char *scanfile,
					   struct PathData *path, int *col, const char *dpath, int silent) {
	(void)fin; (void)infile; (void)outfile; (void)scanfile; (void)path; (void)col; (void)dpath; (void)silent;
	return RTN_ERRCSVWORKER;	// unreachable: main() forces Workers = 1 on Windows
}
#endif

int main(int argc, char *argv[]) {

	struct PathData path;

	char *infile = NULL, *outfile = NULL, *datapath = NULL;
	char *txant = NULL, *rxant = NULL;
	char *scanspec = NULL, *scanfile = NULL;
	double gos = 0.0;
	int silent = FALSE;

	char *line = NULL;
	size_t linecap = 0;
	char *field[CSVMAXFIELDS];
	int col[NINPUTCOLUMNS];
	int nhdr, retval;
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
		else if (strcmp(argv[i], "-F") == 0 && i+1 < argc) scanspec = argv[++i];
		else if (strcmp(argv[i], "-S") == 0 && i+1 < argc) scanfile = argv[++i];
		else if (strcmp(argv[i], "-j") == 0 && i+1 < argc) Workers  = atoi(argv[++i]);
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

	if (scanfile != NULL && scanspec == NULL) {
		printf("CircuitCSV: Error %d -S needs -F\n", RTN_ERRCSVARGS);
		return RTN_ERRCSVARGS;
	}
	if (scanspec != NULL && ParseScan(scanspec) != RTN_CSVOK) return RTN_ERRCSVARGS;

	// -j 0 means one worker per online processor.
	if (Workers < 0) {
		printf("CircuitCSV: Error %d -j must be 0 (all processors) or a positive count\n", RTN_ERRCSVARGS);
		return RTN_ERRCSVARGS;
	}
#ifdef _WIN32
	if (Workers != 1) {
		printf("CircuitCSV: -j is not supported on Windows; running one process\n");
		Workers = 1;
	}
#else
	if (Workers == 0) {
		long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
		Workers = (ncpu > 0) ? (int)ncpu : 1;
	}
	if (Workers > CSVMAXWORKERS) Workers = CSVMAXWORKERS;
#endif

	// The P533/P372 readers insert the separator themselves now, so this only
	// has to bound the copy.
	if ((size_t)snprintf(dpath, sizeof(dpath), "%s", datapath) >= sizeof(dpath)) {
		printf("CircuitCSV: Error %d Data file path too long\n", RTN_ERRCSVARGS);
		return RTN_ERRCSVARGS;
	}

	retval = LoadP533();
	if (retval != RTN_CSVOK) return retval;

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

	// Keep the unrotated rows so each circuit can point the pattern along its
	// own great circle. An isotropic pattern needs no orientation.
	if (txant != NULL && strcmp(txant, "ISOTROPIC") != 0) SaveAntennaMaster(&TxMaster, &path.A_tx);
	if (rxant != NULL && strcmp(rxant, "ISOTROPIC") != 0) SaveAntennaMaster(&RxMaster, &path.A_rx);

	// The path's own 10.7 MB maps are released immediately: from here on it
	// borrows the library's month cache instead, so nothing is duplicated.
	csvFreeIonMaps(&path);

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

	retval = ReadRecord(fin, &line, &linecap);
	if (retval != TRUE) {
		if (retval == CSVBADQUOTE)
			printf("CircuitCSV: Error %d %s header has an unclosed quote\n", RTN_ERRCSVHEADER, infile);
		else printf("CircuitCSV: Error %d %s is empty\n", RTN_ERRCSVHEADER, infile);
		free(line);
		fclose(fin);
		return RTN_ERRCSVHEADER;
	}

	nhdr = SplitCSV(line, field, CSVMAXFIELDS);
	for (int i = 0; i < NINPUTCOLUMNS; i++) {
		col[i] = FindColumn(field, nhdr, InputColumns[i]);
		if ((i == COLINDEX) && (col[i] < 0)) {
			// No SSN column: accept the IPS T index instead and convert it.
			col[i] = FindColumn(field, nhdr, "t_Index");
			if (col[i] >= 0) {
				IndexIsT = TRUE;
				IndexName = "t_Index";
			}
			else {
				printf("CircuitCSV: Error %d Header needs an 'SSN' or a 't_Index' column\n", RTN_ERRCSVHEADER);
				free(line);
				fclose(fin);
				return RTN_ERRCSVHEADER;
			}
		}
		if (col[i] < 0) {
			printf("CircuitCSV: Error %d Header is missing the column '%s'\n",
				RTN_ERRCSVHEADER, InputColumns[i]);
			free(line);
			fclose(fin);
			return RTN_ERRCSVHEADER;
		}
	}
	free(line);			// the header fields are not needed past here
	line = NULL;

	/*
		With -j the rows go to worker processes in blocks of CSVBLOCK, each
		worker taking the next block when it finishes one, so faster cores do
		more. Each writes its own part files and the parent puts the blocks back
		in input order, so the output is the same as a serial run's, row for
		row. Processes rather than threads, because
		the engine keeps its month caches in library globals; each worker loads
		the months it meets, as a serial run does.
	*/
	if (Workers > 1) {
		retval = RunParallel(fin, infile, outfile, scanfile, &path, col, dpath, silent);
		fclose(fin);
	}
	else {
		struct Counts cnt = { 0, 0, 0 };

		fout = fopen(outfile, "w");
		if (fout == NULL) {
			printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENOUT, outfile, strerror(errno));
			fclose(fin);
			return RTN_ERRCSVOPENOUT;
		}
		PrintHeader(fout);

		if (scanfile != NULL) {
			ScanOut = fopen(scanfile, "w");
			if (ScanOut == NULL) {
				printf("CircuitCSV: Error %d Can't open %s (%s)\n", RTN_ERRCSVOPENOUT, scanfile, strerror(errno));
				fclose(fin); fclose(fout);
				return RTN_ERRCSVOPENOUT;
			}
			PrintScanHeader(ScanOut);
		}

		retval = ProcessRows(fin, fout, &path, col, dpath, silent, 0, 1, &cnt);

		if (retval == RTN_CSVOK && silent == FALSE) {
			printf("\rProcessed %d circuit(s), %d skipped, %d month change(s)\n",
				cnt.number, cnt.failed, cnt.monthsused);
		}

		fclose(fin);
		// A full disk shows up here, not at fprintf() time: the -j path
		// already failed the run on it, and so must a serial one.
		if ((ferror(fout) | fclose(fout)) != 0) {
			printf("CircuitCSV: Error %d writing %s\n", RTN_ERRCSVOPENOUT, outfile);
			if (retval == RTN_CSVOK) retval = RTN_ERRCSVOPENOUT;
		}
		if (ScanOut != NULL && (ferror(ScanOut) | fclose(ScanOut)) != 0) {
			printf("CircuitCSV: Error %d writing %s\n", RTN_ERRCSVOPENOUT, scanfile);
			if (retval == RTN_CSVOK) retval = RTN_ERRCSVOPENOUT;
		}
	}

	free(ScanF);
	// The maps the path points at belong to the cache, so FreePathMemory() must
	// not see them; it skips NULL.
	path.foF2  = NULL;
	path.M3kF2 = NULL;
	csvFreePathMemory(&path);
	csvIonMapFree();
	if (P533Lib != NULL) hfLibClose(P533Lib);
	if (P372Lib != NULL) hfLibClose(P372Lib);

	return retval;

}
