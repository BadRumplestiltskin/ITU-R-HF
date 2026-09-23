#ifndef CIRCUITCSV_H
#define CIRCUITCSV_H

// Maximum number of input columns and the maximum length of one CSV record.
#define CSVMAXFIELDS	64
#define CSVMAXLINE		4096
#define CSVMAXNAME		256

// Returns. These are kept above the ITURHFProp range (< 100) and the P533
// engine range (< 1000) so that a return value identifies its origin.
#define RTN_CSVOK					0
#define RTN_ERRCSVARGS				1001	// ERROR: Invalid command line
#define RTN_ERRCSVOPENIN			1002	// ERROR: Can not open the input csv
#define RTN_ERRCSVOPENOUT			1003	// ERROR: Can not open the output csv
#define RTN_ERRCSVHEADER			1004	// ERROR: Missing or unrecognised header
#define RTN_ERRCSVFIELD				1005	// ERROR: Malformed record
#define RTN_ERRCSVP533LIB			1006	// ERROR: Can not load the P533 library
#define RTN_ERRCSVP372LIB			1007	// ERROR: Can not load the P372 library
#define RTN_ERRCSVANTENNA			1008	// ERROR: Can not read an antenna pattern

/*
	One row of the input file. The names match the csv header exactly.
	t_Index is the 12-month smoothed sunspot number, R12, which P.533 calls SSN.
*/
struct Circuit {
	int    row;				// 1-based data row number in the input file
	int    parsed;			// TRUE when every input column was present
	char   txSite[CSVMAXNAME];
	double txLat;			// degrees, N positive
	double txLon;			// degrees, E positive
	char   rxSite[CSVMAXNAME];
	double rxLat;
	double rxLon;
	int    year;
	int    month;			// 1 - 12 in the file, 0 - 11 in the engine
	int    day;				// read and echoed; P.533 works on monthly medians
	int    hour;			// UTC, 0 - 23
	int    t_Index;			// SSN (R12)
	double minTOA;			// minimum take-off angle (degrees)
	double txPow;			// transmitter power (W)
	double reqSN;			// required signal-to-noise ratio (dB)
	double rxNoise;			// receiver man-made noise power (dBW)
	double bandW;			// bandwidth (Hz)
	double percDays;		// required reliability (% of days)
};

#endif
