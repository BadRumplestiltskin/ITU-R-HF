#ifndef P533_H
#define P533_H

// Operating system preprocessor directives *********************************************************
#ifdef _WIN32
	#define DLLEXPORT __declspec(dllexport)
#endif
#ifdef __linux__
	#define DLLEXPORT
#endif
#ifdef __APPLE__
	#define DLLEXPORT
#endif

// External Preprocessors Dependancies
// The path structure constains the P372 noise structure
#include "Noise.h"
// End External Preprocessor Dependancies

// P533 *******************************************************************************************

// Version number
#define P533VER		"14.2"

// Have the preprocessor time stamp the compile time
#define P533CT		__TIMESTAMP__

/*
 * Conventions
 *	Latitude and Longitude used by P533 are to be in radians under the following convention:
 *			latitude (-90.0 - +90.0)
 *				        S=-     N=+
 *			longitude (-180.0 - +180.0)
 *				         W=-      E=+
 */

// Indicies for magfit() The gyrofrequency and magnetic dip are calculated at a height
//		of either 100 or 300 km. For absorption calculation 100 km is typically used
//		while 300 km is used for other calculations.
#define HR100km		0		// height = 100 (km)
#define HR300km		1		// height = 300 (km)

// Returns ************************************************************************

// Note: All error numbers in the calculation engine P533() are < 1000

// Return ERROR >= 100 and < 150

// Returns from ValidData()
#define RTN_ERRYEAR						100	// ERROR: Invalid Input Year
#define RTN_ERRMONTH					101 // ERROR: Invalid Input Month
#define RTN_ERRHOUR						102 // ERROR: Invalid Input Hour
#define RTN_ERRMANMADENOISE				103 // ERROR: Invalid Input Man-Made Noise
#define	RTN_ERRNOFOF2DATA				104 // ERROR: Invalid Input Missing foF2 array data
#define RTN_ERRNOM3KF2DATA				105 // ERROR: Invalid Input Missing M(3000)F2 array data
#define RTN_ERRNODUDDATA				106 // ERROR: Invalid Input Missing DuD array data
#define	RTN_ERRNOFAMDATA				107 // ERROR: Invalid Input Missing Fam array data
#define RTN_ERRNOFOF2VARDATA			108 // ERROR: Invalid Input Missing foF2 Variability array data
#define RTN_ERRSSN						109 // ERROR: Invalid Input Sun Spot Number
#define RTN_ERRMODULATION				110 // ERROR: Invalid Input Modulation
#define RTN_ERRFREQUENCY				111 // ERROR: Invalid Input Frequency
#define RTN_ERRBW						112 // ERROR: Invalid Input Bandwidth
#define RTN_ERRTXPOWER					113 // ERROR: Invalid Input Transmit Power
#define RTN_ERRSNRR						114 // ERROR: Invalid Input Required Signal-to-Noise ratio
#define RTN_ERRSIRR						115 // ERROR: Invalid Input Required Signal-to-Interference ratio
#define RTN_ERRF0						116 // ERROR: Invalid Input F0
#define RTN_ERRT0						117 // ERROR: Invalid Input T0
#define RTN_ERRA						118 // ERROR: Invalid Input Digital Modulation Amplitude ratio
#define RTN_ERRTW						119 // ERROR: Invalid Input Digital Modulation Time Window
#define RTN_ERRFW						120 // ERROR: Invalid Input Digital Modulation Frequency Window
#define RTN_ERRLTX						121 // ERROR: Invalid Input Transmit Location
#define RTN_ERRLRX						122 // ERROR: Invalid Input Receive Location
#define RTN_ERRRXANTENNAPATTERN			123 // ERROR: Invalid Input Receive Antenna Pattern
#define RTN_ERRTXANTENNAPATTERN			124 // ERROR: Invalid Input Transmit Antenna Pattern
#define	RTN_ERRSNRXXP					125 // ERROR: Invalid Input SNRXX Percentage
// END returns from ValidData()

// Return ERROR from AllocatePathMemory(), FreePathMemory() and InputDump()
#define RTN_ERRALLOCATEFOF2				131 // ERROR: Allocating Memory for foF2
#define RTN_ERRALLOCATEM3KF2			132 // ERROR: Allocating Memory for M(3000)F2
#define RTN_ERRALLOCATEFOF2VAR			133 // ERROR: Allocating Memory for foF2 Variability
#define RTN_ERRALLOCATETX				134 // ERROR: Allocating Memory for Tx Antenna Pattern
#define RTN_ERRALLOCATERX				135 // ERROR: Allocating Memory for Rx Antenna Pattern
#define RTN_ERRALLOCATEANT			    137 // ERROR: Allocating Memory for Antenna Pattern

// Return ERROR from ReadAntennaPatterns() ReadType13()
#define	RTN_ERRCANTOPENANTFILE	        138 // ERROR: Can Not Open Recieve Antenna File
#define	RTN_ERRREADANTFILE		        142 // ERROR: Antenna File Truncated or Malformed

// Return ERROR from ReadP1239()
#define RTN_ERRCANTOPENP1239FILE		139 // ERROR: Can Not Open foF2 Variability file "P1239-2 Decile Factors.txt"
#define RTN_ERRNOTP12393				140 // ERROR: Invalid P.1239-3 File

// Return ERROR from ReadIonParametersTxt()
#define RTN_ERRREADIONPARAMETERS		141 // ERROR: Can Not Open Ionospheric Parameters File


// Return OKAY > 10 and <= 20
#define RTN_ALLOCATEP533OK				11 // AllocatePathMemory()
#define RTN_PATHFREED					12 // PathMemory.c FreePathMemory(()
#define RTN_INPUTDUMPOK					13 // InputDump()
#define RTN_READIONPARAOK			    14 // ReadIonParameters()
#define RTN_READP1239OK					15 // ReadP1239()
#define RTN_READANTENNAPATTERNSOK		16 // ReadAntennaPatterns()
#define	RTN_VALIDDATAOK					17 // ValidPath()

#define	RTN_P533OK						10 // P533()

// End Returns ********************************************************************
// End Returns ********************************************************************

// Control point index names for readability
// These are defined from the sense of the short model
// Please note these change meaning when the long model is exclusively used
// i.e when the path->distance is > 9000 km. This is done for diagnostic purposes.
#define T1k		0	// T + 1000 (km)
// Note: Alternative use in long model penetration point closest to the trasmitter at the current hour
#define Td02	1	// T + d0/2 (km)
// Note: Alternative use in long model as T + dM/2
#define MP		2	// path mid-path (km)
#define	Rd02	3	// R - d0/2 (km)
// Note: Alternative use in lone model as R - dM/2
#define	R1k		4	// R - 1000 (km)
// Note: Alternative use in long model at last penetration point, 2*nL, at current hour

// foF2 variability index names for readability
#define WINTER	0
#define EQUINOX 1
#define SUMMER  2

// Decile flags
#define DL		0 // Lower decile
#define DU		1 // Upper decile

#define DAY		0 // DAY index for the rop array in MUFOperational()
#define NIGHT	1 // NIGHT index for the rop array in MUFOperational()

#define JAN		0
#define FEB		1
#define MAR		2
#define APR		3
#define MAY		4
#define JUN		5
#define JUL		6
#define AUG		7
#define SEP		8
#define OCT		9
#define NOV		10
#define DEC		11

// For the determination of the lowest order E and F2 mode
#define NOLOWESTMODE	99

// For the determination of the dominant mode
#define NODOMINANTMODE	99

// Noise calculation (See ITU-R P.372)
#define CITY		0.0
#define RESIDENTIAL 1.0
#define RURAL		2.0
#define QUIETRURAL	3.0
#define NOISY		4.0
#define	QUIET		5.0

// Modulation flags
#define ANALOG		0
#define DIGITAL		1

// Short or long path flags
#define SHORTPATH	0
#define LONGPATH	1

// Minimum Elevation Angle (degrees) for the Short model
#define MINELEANGLES 3.0
// Minimum Elevation Angle (degree) for the Long model
#define MINELEANGLEL 3.0

// Maximum Sun Spot Number
#define MAXSSN		160

// Mode slots, indexed by hop count - 1. P.533-14 section 5.2.1 considers "up to
// three E modes (for paths up to 4 000 km) and up to six F2 modes": the lowest-order
// mode and the next two (E) or five (F2) higher-order modes. The lowest-order E mode is
// 1E or 2E, so E needs four slots; the lowest-order F2 mode is searched up to 6F2, so
// F2 needs eleven. MUFBasic() leaves the slots beyond those counts without a basic MUF.
// These were 6 and 3, which dropped the top modes whenever the lowest order was not 1.

// Number of F2 mode slots
#define MAXF2MDS	11
// Highest lowest-order F2 mode searched (index), as before the slots were widened
#define MAXN0F2		5
// Number of higher-order modes considered above the lowest-order one
#define NHIGHERF2	5
#define NHIGHERE	2

// Number of E mode slots
#define MAXEMDS		4

// Maximum number of modes
#define MAXMDS	(MAXEMDS+MAXF2MDS)

// Direction of the AntennaGain()
#define TXTORX	1
#define RXTOTX  2

// End #define ************************************************************************************


// Structures *************************************************************************************

/*
 *	The naming convention of structures is that the first letter is capitalized and instances of that structure in other
 *	structures use all-cap abbreviations. Thus, a ControlPt structure in a mode structure is a CP_ and a location structure
 *	is a L or L_. This note may help as one delves into the code.
 *
 *  The CP[MP] structure in the path is a special midpoint control point.
 *
 *  The lowest index of the Md_F2 and Md_E structure arrays are to be the lowest order mode where the lowest order mode is
 *  the lowest index + 1
 *
 */

// A geographic position, in radians (see "Conventions" above).
struct Location {
	double lat, lng;	// latitude (north +), longitude (east +), radians
};

// Solar geometry at a point for the middle of the month and a UTC hour, set by SolarParameters().
// The long model uses sza for equation (34) of P.533-14 and the short model for chi_j of equation (20).
struct SolarParameters {
	double ha;		// hour angle (radians), from the true solar time
	double sha;		// Sunrise/sunset hour angle (radians); PI in polar day, 0 in polar night
	double sza;		// Solar zenith angle (radians)
	double decl;	// Solar declination (radians)
	double eot;		// Equation of time (minutes)
	double lsr;		// local sunrise (UTC hours, 0 - 24)
	double lsn;		// local solar noon (UTC hours, 0 - 24)
	double lss;		// local sunset (UTC hours, 0 - 24)
};

// A control point (P.533-14 Table 1) or, in the long model and PenetrationPoints(), a 90 km
// penetration point. The ionospheric and solar values are for the hour last evaluated.
struct ControlPt {
	struct Location L;	// Position (radians)
	double distance;// This is the distance (km) from the transmitter to the CP and not the hop range
	double foE;		// E layer critical frequency (MHz)
	double foF2;	// F2 layer critical frequency (MHz)
	double M3kF2;	// M(3000)F2, the F2-layer propagation factor for 3000 km (dimensionless)
	double dip[2];	// Magnetic dip (radians), [HR100km] and [HR300km]
	double fH[2];	// Gyrofrequency (MHz), [HR100km] and [HR300km]
	double ltime;	// Despite the name, the UTC hour the point was evaluated for (hours);
					// LocalMeanTime() gives the local mean time
	double hr;		// Mirror reflection height (km); 90 for penetration points
	double x;		// foF2/foE, or 2 whichever is larger: x of P.533-14 equations (5) and (6)
					// (set by CalcB() in MUFBasic.c; the section 5.1 x = foF2/foE is computed locally)
	// Solar parameters
	struct SolarParameters Sun;
};

struct Mode {
	// Define the myriad of MUFs
	double BMUF;	// Basic MUF (MHz). Typically there is no difference between the basic and the 50% MUF
					// The BMUF is checked to see if it is != 0.0 to determine if the mode exists
	double MUF90;	// MUF exceeded for 90% of the days of the month (MHz)
	double MUF50;	// MUF exceeded for 50% of the days of the month(MHz)
	double MUF10;	// MUF exceeded for 10% of the days of the month(MHz)
	double OPMUF;	// Operation MUF(MHz)
	double OPMUF10; // Operation MUF exceeded 10% of the days of the month(MHz)
	double OPMUF90; // Operation MUF exceeded 90% of the days of the month(MHz)
	double Fprob;	// Probability that the mode is supported at the frequency of interest
	double deltal;	// Lower decile for the MUF calculations
	double deltau;	// Upper decile for the MUF calculations
	// Other parameters associated with the mode
	double hr;		// Mirror reflection height for the mode (km)
	double fs;		// E-Layer screening frequency for F2 modes only (MHz), P.533-14 equation (11)
	double Lb;		// <= 9000 km ray path basic transmission loss (dB), equation (18)
	double Ew;		// <= 9000 km field strength (dB(1 uV/m)), equation (17)
	double ele;		// Elevation angle (radians), equation (13)
	double Prw;		// Available receiver power (dBW), equation (43)
	double Grw;		// Receive antenna gain (dBi) at ele
	double tau;		// Time delay (ms), equation (47)
	int MC;			// TRUE if the mode is included in Es (equation (28)), set by
					// MedianSkywaveFieldStrengthShort()
};

// An antenna beam direction and gain. Not used anywhere else in this repository.
struct Beam {
	double azm;		// Azimuth
	double ele;		// Elevation angle
	double G;		// Gain for the azimuth and elevation
};

struct Antenna {
	char Name[256];

	/*
	 * Int used to track the number of frequencies for which we have pattern data
	 * (e.g. the size of the freqs array).
	 */
	int freqn;

	/*
	 * An array to store the frequencies we have pattern data for.
	 */
	double *freqs;

	// 3D double pointer to the antenna pattern data
	// [freq_index][azimuth][elevation]
	// The following is assumed about the antenna pattern when the program is run:
	//		i) The orientation is correct. The antenna pattern is in the orientation as it would be on the Earth.
	//		ii) The data is valid. It is the responsibility of the calling program to ensure this.
	double ***pattern;
};

// Any "adjustment" to the contents of the structure PathData to make indices out of some of the variables, such as month and hour
// are done in InitializePath()

struct PathData {

	// User-provided Input ************************************************************************

	char name[256];		// The path name
	char txname[256];	// The transmitter name
	char rxname[256];	// The receiver name

	int year;
	int month;			// Note: This is 0 - 11
	int hour;			// Note: This is an hour index 0 - 23: path->hour = h means h:00 UTC.
						//       The ionospheric maps and every 24-hour array in the engine
						//       (fBM[][t], fL[t], CP[][t]) use the same convention.
	int SSN;			// 12-month smoothed sun spot number a.k.a. R12 (>= 0, no upper limit;
						// foF2 and M(3000)F2 limit it to MAXSSN themselves)

	int Modulation;		// Modulation flag: ANALOG or DIGITAL

	int SorL;			//  Short or long path switch: SHORTPATH or LONGPATH great circle

	double frequency;	// Frequency (MHz)
	double BW;			// Bandwidth (Hz)

	double txpower;		// Transmitter power (dB(1 kW))

	int SNRXXp;			// Required signal-to-noise ration (%) of the time (1 to 99)
	double SNRr;		// Required signal-to-noise ratio (dB)
	double SIRr;		// Required signal-to-interference ratio (dB)

	// Parameters for approximate basic circuit reliability for digital modulation
	// (used by CircuitReliability() only when both are non-zero)
	double F0;			// Frequency dispersion at a level -10 dB relative to the peak signal amplitude
	double T0;			// Time spread at a level -10 dB relative to the peak signal amplitude

	// Parameters for digitial modulation performance
	double A;			// Required A ratio (dB)
	double TW;			// Time window (msec)
	double FW;			// Frequency window (Hz)

	struct Location L_tx, L_rx;	// Transmitter and receiver locations (radians)
	struct Antenna A_tx, A_rx;	// Transmit and receive antenna patterns (dBi)

	// End User Provided Input *********************************************************************

	// Array pointers ******************************************************************************
	// The advantage of having these pointers in the PathData structure is that p533() can be
	// re-entered with the data allocations intact since they are determined and loaded externally
	// to p533(). This is done to make area coverage calculations, multiple hours and/or
	// any calculations that require the path be examined for another location or time within the
	// current month. If the month changes foF2 and M3kF2 will have to be reloaded, while the pointer
	// foF2var does not since it is for the entire year
	// Pointers to array extracted from the coefficients in ~/IonMap directory
	// [hour 0-23 (h:00 UTC)][longitude 0-240, 1.5 deg from 180 W][latitude 0-120, 1.5 deg from 90 S][R12 = 0, 100]
	float ****foF2;			// foF2 (MHz)
	float ****M3kF2;		// M(3000)F2
	// Pointer to array extracted from the file "P1239-3 Decile Factors.txt"
	// [season WINTER/EQUINOX/SUMMER][local hour 0-23][latitude 0-90 by 5 deg][R12 < 50, 50-100, > 100][DL, DU]
	double *****foF2var;	// foF2 Variablity from ITU-R P.1239 TABLE 2 and TABLE 3

 	// End Array Pointers *************************************************************************

	// Calculated Parameters **********************************************************************
	int season;			// P.1239 season index (WINTER, EQUINOX, SUMMER) at the mid-path, for the
						// foF2 decile factors and P.1240 Rop in the MUF calculations
	double distance;	// This is the great circle distance (km) between the rx and tx
	double ptick;		// Virtual slant range p' (km), equation (19): of the last short-path mode
						// calculated, replaced for D >= 7000 km by the long-model (fM hop) value
	double dmax;		// d sub max (km), equation (5) at the midpoint of the path, limited to 4000 km
	double B;			// Intermediate value when calculating dmax, equation (6). Initialised to 99.9
						// by InitializePath(); no routine currently stores a value here
	double ele;			// Elevation angle (radians): of the dominant mode for D <= 7000 km, otherwise
						// that of the largest 0 - 8 degree receive gain; for D > 9000 km the long
						// model first stores the fM hop elevation, which the receive gain replaces

	// MUFs
	double BMUF;	// Basic MUF (MHz)
	double MUF50;	// MUF exceeded for 50% of the days of the month (MHz)
	double MUF90;	// MUF exceeded for 90% of the days of the month (MHz)
	double MUF10;	// MUF exceeded for 10% of the days of the month (MHz)
	double OPMUF;	// Operation MUF (MHz)
	double OPMUF90; // OPMUF exceeded for 90% of the days of the month (MHz)
	double OPMUF10; // OPMUF exceeded for 10% of the days of the month (MHz)
	// Highest probable frequency, HPF, is 10% MUF (MHz)
	// Optimum working frequency, FOT, is 90% MUF (MHz)

	int n0_F2;		// Index (hops - 1) of the lowest-order F2 mode (0 to MAXN0F2), or NOLOWESTMODE
	int n0_E;		// Index (hops - 1) of the lowest-order E mode, or NOLOWESTMODE

	// Signal powers
	// All field strengths in dB(1 uV/m); TINYDB (-307) means no mode.
	double Es;	// The overall resultant equivalent median sky-wave field strength for path->distance <= 9000 km, equation (28)
	double El;	// The overall resultant median field strength for paths->distance >= 7000 km, equation (39)
	double Ei;	// For paths->distance between 7000 and 9000 km the interpolated resultant median field strength, equation (42)
	double Ep;	// The Path Field Strength (dBu) Depending on the path distance this is either Es, El or Ei.
	double Pr;	// Median available receiver power (dBW), section 6

	// Short path (< 7000 km) parameters
	double Lz;		// 	"Not otherwise included" loss (dB), 8.72

	// Long path (> 9000 km) parameters
	double E0;		// The free-space field strength for 3 MW EIRP (dB(1 uV/m)), equation (40)
	double Gap;		// Focusing on long distance gain (dB), equation (41), at most 15 dB
	double Ly;		// "Not otherwise included" loss (dB), -0.14
	double fM;		// Upper reference frequency, operational MUF (MHz), equation (31)
	double fL;		// Lower reference frequency, LUF (MHz), equations (33) - (38), current hour
	double F;		// The factor 1 - [...] multiplying E0 in equation (39) P.533-14
					// (a function of f, fH, fL and fM; eqn 28 in P.533-12)
	double fH;		// Mean gyrofrequency (MHz) at the two Table 1a) control points, 300 km
	double Gtl;		// Largest tx antenna gain in the range 0 to 8 degrees (dBi)
	double K[2];	// Correction factor K, equation (32), at T + dM/2 [0] and R - dM/2 [1]

	// Signal-to-noise ratio
	double SNR;	 // Median resultant signal-to-noise ratio (dB) for bandwidth b (Hz)
	double DuSN; // Upper decile deviation of the signal-to-noise ratio (dB)
	double DlSN; // Lower decile deviation of the signal-to-noise ratio (dB)

	// Signal-to-noise at the required reliability
	double SNRXX; //

	// Digitially modulated system stats
	double SIR;  // Signal-to-interference ratio (db)
	double DuSI; // Upper decile deviation of the signal-to-interference ratio (db)
	double DlSI; // Lower decile deviation of the signal-to-interference ratio (db)
	double RSN;  // Probability that the required SNR is achieved
	double RT;	 // Probability that the required time spread T0 is not exceeded
	double RF;	 // Probability that the required frequency spread f0 is not exceeded

	// Reliability
	double BCR;		// Basic circuit reliability
	double OCR;		// Overall circuit reliability without scattering
	double OCRs;	// Overall circuit reliability with scattering
	double MIR;		// Multimode interference
	double probocc; // Probability of scattering occuring (%)

	// Antenna related parameters

	// Grw
	//	path->distance <= 7000 km
	//		Grw is the "lossless receiving antenna of gain Grw
	//		(dB relative to an isotropic radiator) in the direction of signal incidence"
	//		Grw will be the dominant mode gain
	//	path->distance >= 9000 km
	//		Grw is the "largest value of receiving antenna gain at the required azimuth in the
	//		elevation range 0 to 8 degrees."
	double Grw;

	// Transmitter EIRP
	double EIRP;

	// There are a maximum of 5 CP from P.533-14 Table 1d)
	// See #define above for "Control point index names for readability"
	struct ControlPt CP[5];

	// ITU-R P.533-14 5.2.1 modes considered "Up to three E modes (for paths up to 4 000 km) and
	// up to six F2 modes are selected"
	// Slot i holds the (i+1)-hop mode; see MAXEMDS and MAXF2MDS above.
	// In part three of P.533-14 it would have been easier to make all nine modes in one array for digitally
	// modulated systems. To increase the readability and because the method often treats layers differently
	// the modes are separated by layer.
	struct Mode Md_F2[MAXF2MDS];
	struct Mode Md_E[MAXEMDS];

	// The following are conveniences for examining data
	// The variables *DMptr and DMidx are set in MedianAvailableReceiverPower()
	struct Mode *DMptr; // Pointer to the dominant mode (largest Prw), D <= 7000 km only
	int DMidx;			// Index to the dominant mode: E slot (0 to MAXEMDS-1) or MAXEMDS + F2 slot;
						// NODOMINANTMODE (99) if none

	// Noise Structure
	struct NoiseParams noiseP;

	// P372.DLL Information
	char const *P372ver;		// P372() Version number
	char const *P372compt;		// P372() Compile time

	// End Calculated Parameters *****************************************************************************
};

// End Structures *********************************************************************************

// Prototypes *************************************************************************************

// Note: The arguments passed are by reference (pointer) if the subroutine changes the argument within it.
//       Otherwise arguments are passed by value. There are a few cases that the only reason that the arguments
//		 are passed by reference was because the next level program required it.
// Any subroutines prototyped here are used external to the file that contains them. There may be local subroutines in
// each program file, consult them for more details. These subroutines were developed as the code was being written
// in the order necessary. If the order is maintained then the correspondence will be be preserved between the code and
// the recommendation ITU-R P.533-14. In that regard the order of execution of the subroutines is important since
// calculations in P.533-14 build on one another.

// CalculateCPParameters.c Prototype
void CalculateCPParameters(struct PathData *path, struct ControlPt *here);
void SolarParameters(struct ControlPt *here, int month, double hour);
double BilinearInterpolation(double LL, double LR, double UL, double UR, double r, double c);
void IonosphericParameters(struct ControlPt *here, float ****foF2, float ****M3kF2, int hour, int SSN);
void FindfoE(struct ControlPt *here, int month, int hour, int SSN);

// Initialize.c Prototypes
//	Only three of the five control points are determined in InitializePath() T + 1000, M and R - 1000.
//	The control points T + d0/2 and R - d0/2  are determined in MUFBasic()
// Exported: CircuitCSV binds these to run the MUF chain without a full P533().
// Without DLLEXPORT they are absent from P533.dll and the bind fails on Windows.
DLLEXPORT void InitializePath(struct PathData *path);

// P533.c Prototype for the P533 propagation model engine
//
// THREAD SAFETY: the library is not thread safe. It holds three process-wide
// caches with no locking - the P372 entry points (LoadP372(), reached from
// AllocatePathMemory()), the ionospheric maps (IonMapGet()/IonMapFree()) and
// libp372's Fam/Dud coefficients (ReadFamDud()). Callers must serialise every
// call into the library: AllocatePathMemory(), IonMapGet(), ReadP1239(),
// P533() and IonMapFree() must not run concurrently with one another. Note
// that concurrent P533() calls on separate PathData structs are not safe
// either, since each reads the shared caches that another may be refilling.
DLLEXPORT int P533(struct PathData *path);
// Resolves the P372 entry points once per process; see P533.c.
int LoadP372(void);
// Joins a data directory and a file name into a bounded buffer; see P533.c.
DLLEXPORT int BuildDataPath(char *out, size_t n, const char *dir, const char *file);
// Returns the version string P533VER.
DLLEXPORT char const * P533Version(void);

// Geometry.c Prototypes
DLLEXPORT void GreatCirclePoint(struct Location here, struct Location there, struct ControlPt *midpnt, double distance, double fraction);
double LocalMeanTime(struct ControlPt CP);
DLLEXPORT double GreatCircleDistance(struct Location here, struct Location there);
DLLEXPORT void GeomagneticCoords(struct Location here, struct Location *there);
DLLEXPORT double Bearing(struct Location here, struct Location there, int direction);

// ValidataPath.c Prototypes
DLLEXPORT int ValidatePath(struct PathData *path);

// magfit.c Prototype
void magfit(struct ControlPt *here, double height);

// MUFBasic Prototype
//	Note MUFBasic() determines the control points T + d0/2 and R - d0/2
DLLEXPORT void MUFBasic(struct PathData *path);
double CalcCd(double d, double dmax);
double CalcF2DMUF(struct ControlPt *CP, double distance, double dmax, double B);
double Calcdmax(struct ControlPt *CP);
double CalcB(struct ControlPt *CP);

// MUFVariability.c Prototype
DLLEXPORT void MUFVariability(struct PathData *path);
double FindfoF2var(struct PathData path, double hour, double lat, int decile);

// MUFOperational.c Prototype
DLLEXPORT void MUFOperational(struct PathData *path);

// ELayerScreeningFrequency.c Prototype
void ELayerScreeningFrequency(struct PathData *path);
double ElevationAngle(double dh, double hr);
double IncidenceAngle(double deltaf, double hr);

// MedianSkywaveFieldStrengthShort.c Prototype
void MedianSkywaveFieldStrengthShort(struct PathData *path);
double AntennaGain(struct PathData path, struct Antenna Ant, double delta, int direction);
void ZeroCP(struct ControlPt *CP);

// MedianSkywaveFieldStrengthLong.c Prototype
void MedianSkywaveFieldStrengthLong(struct PathData *path);
double AntennaGain08(struct PathData path, struct Antenna Ant, int direction, double * elevation);

// Between7000kmand9000km.c Prototypes
void Between7000kmand9000km(struct PathData *path);

// MedianAvailableReceiverPower.c Prototypes
void MedianAvailableReceiverPower(struct PathData *path);

// CircuitReliability.c Prototype
void CircuitReliability(struct PathData *path);

// PathMemory.c prototype
// Allocates the ionospheric maps, foF2 variability array and noise arrays of a path
// (RTN_ALLOCATEP533OK on success); FreePathMemory() releases them (RTN_PATHFREED).
DLLEXPORT int AllocatePathMemory(struct PathData *path);
DLLEXPORT int FreePathMemory(struct PathData *path);
// Allocates an antenna pattern of freqn x 360 x 91 (RTN_ALLOCATEP533OK or RTN_ERRALLOCATEANT).
DLLEXPORT int AllocateAntennaMemory(struct Antenna *ant, int freqn, int azin, int elen);

// InputDump. c Prototype
DLLEXPORT int InputDump(struct PathData *path);

//Antenna file AND COEFFICIENT routines
// ReadType11/13/14 read VOACAP antenna files from an open FILE (RTN_READANTENNAPATTERNSOK on
// success); bearing is in radians. IsotropicPattern() sets a constant gain G (dBi).
// ReadIonParametersBin()/ReadIonParametersTxt() read the monthly ionos MM .bin/.txt maps
// (RTN_READIONPARAOK); month is the 0-based index. See ReadType13.c and ReadIonParameters.c.
DLLEXPORT int ReadType11(struct Antenna *Ant, FILE *fp, int silent);
DLLEXPORT int ReadType13(struct Antenna *Ant, FILE *fp, double bearing, int silent);
DLLEXPORT int ReadType14(struct Antenna *Ant, FILE *fp, int silent);
DLLEXPORT void IsotropicPattern(struct Antenna *Ant, double G, int silent);
DLLEXPORT int ReadIonParametersBin(int month, float ****foF2, float ****M3kF2, char DataFilePath[256], int silent);
// Ionospheric map cache: one parsed copy per month, shared by every circuit.
// The maps belong to the cache; release them with IonMapFree(), not free().
// Not thread safe: see THREAD SAFETY above P533().
#define IONMAPHRS	24		// hours
#define IONMAPLNG	241		// longitudes at 1.5-degree increments
#define IONMAPLAT	121		// latitudes at 1.5-degree increments
#define IONMAPSSN	2		// sunspot numbers, high and low
DLLEXPORT int IonMapGet(int month, char *DataFilePath, int silent, float *****foF2, float *****M3kF2);
DLLEXPORT void IonMapFree(void);
// Releases only path->foF2/M3kF2, so the path can instead point at the cache.
DLLEXPORT void FreeIonMaps(struct PathData *path);
DLLEXPORT int ReadIonParametersTxt(struct PathData *path, char DataFilePath[256], int silent) ;
// Reads the P.1239 foF2 decile factors into path->foF2var (RTN_READP1239OK).
DLLEXPORT int ReadP1239(struct PathData *path, const char * DataFilePath);
// Sets one gain (dBi) at a whole-degree azimuth and elevation of the tx (TXorRX == 0) or rx pattern.
DLLEXPORT void SetAntennaPatternVal(struct PathData * path, int TXorRX, int azimuth, int elevation, double value);

//Testing Routines
// Returns sizeof(struct PathData), for checking a foreign-language copy of the layout.
DLLEXPORT int sizeofPathDataStruct(void);


// End Prototypes *********************************************************************************

// End P533 ***************************************************************************************
#endif // P533_H
