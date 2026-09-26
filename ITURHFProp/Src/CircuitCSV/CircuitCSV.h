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
// 1006 and 1007 were RTN_ERRCSVP533LIB and RTN_ERRCSVP372LIB (a library could
// not be loaded). They are no longer returned: the program is linked against
// both, so the system loader refuses to start it without them.
#define RTN_ERRCSVANTENNA			1008	// ERROR: Can not read an antenna pattern
#define RTN_ERRCSVWORKER			1009	// ERROR: A -j worker process failed

/*
	One row of the input file. The names match the csv header exactly.
	The solar index column is either SSN (R12, SIDC version 2) or t_Index (the IPS
	T index), which is converted to SSN; see SSNFromIndex() in CircuitCSV.c.
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
	double index;			// the solar index as given: SSN, or t_Index
	int    ssn;				// SSN applied: index, or converted from the T index
	double minTOA;			// minimum take-off angle (degrees)
	double txPow;			// transmitter power (W)
	double reqSN;			// required signal-to-noise ratio (dB)
	double rxNoise;			// receiver man-made noise power (dBW)
	double bandW;			// bandwidth (Hz)
	double percDays;		// required reliability (% of days)
};

#endif
