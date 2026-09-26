#ifndef NOISE_H
#define NOISE_H

/*
	P372_API marks the functions libp372 exports.

	Programs link against the library when they are built, so the compiler
	checks every call against the prototypes below. On Windows a function is
	visible outside its DLL only when it is exported: the library is built with
	P372_BUILD defined, so the declarations here export, and every other
	includer sees them as imports. Elsewhere the library is built with
	-fvisibility=hidden, so these are the only symbols it exports.
*/
#if defined(_WIN32)
	#ifdef P372_BUILD
		#define P372_API __declspec(dllexport)
	#else
		#define P372_API __declspec(dllimport)
	#endif
#elif defined(__GNUC__)
	#define P372_API __attribute__((visibility("default")))
#else
	#define P372_API
#endif

/* Defines */
// Version number.
#define P372VER "14.3"

// Have the preprocessor time stamp the compile time.
#define P372CT __TIMESTAMP__

// Noise calculation (See ITU-R P.372).
// Man-made noise category codes for NoiseParams.ManMadeNoise. They are
// compared exactly (==) in ManMadeNoise() (Noise.c). Any other value >= 0 is
// a user value (FaM = 204 - value); a value < 0 makes Noise() skip the
// calculation and set FamT = -value (dB above kT0b).
#define CITY 0.0
#define RESIDENTIAL 1.0
#define RURAL 2.0
#define QUIETRURAL 3.0
#define NOISY 4.0
#define QUIET 5.0

// MakeNoise().
#define MNNOPRINT 0
#define MNPRINTTOSTDOUT 1
#define MNPRINTTOFILE 2

// Return ERROR >= 200 and < 220.
// Return ERROR from ReadFamDud().
// ERROR: Can Not Open Coefficient File.
#define RTN_ERROPENCOEFFFILE 201

// Return ERROR from AllocatePathMemory(), FreePathMemory() and InputDump().
// ERROR: Allocating Memory for DuD.
#define RTN_ERRALLOCATEDUD 202
// ERROR: Allocating Memory for Fam.
#define RTN_ERRALLOCATEFAM 203
// ERROR: Allocating Memory for FakP.
#define RTN_ERRALLOCATEFAKP 204
// ERROR: Allocating Memory for FakABP// Return ERROR from P533().
#define RTN_ERRALLOCATEFAKABP 205
// 206 was RTN_ERRP372DLL (P372 library not found or incomplete). It is no longer
// returned: programs and libp533 are linked against libp372, so the system
// loader refuses to start without it. The number is kept out of use.
// ERROR: Allocating Memory for Noise Structure.
#define RTN_ERRALLOCATENOISE 207
// ERROR: Can't open output file in MakeNoise().
#define RTN_ERRMNCANTOPENFILE 208
// ERROR: Coefficient File Truncated or Malformed in ReadFamDud().
#define RTN_ERRREADCOEFFFILE 209

// Return OKAY > 20 and <= 30.
// AllocatePathMemory().
#define RTN_ALLOCATEP372OK 21
// ReadFamDud(),
#define RTN_READFAMDUDOK 22
// NoiseMemory.c FreeNoiseMemory().
#define RTN_NOISEFREED 23
// Noise().
#define RTN_NOISEOK 24
// Noise() Man-made noise override.
#define RTN_NOISEMANMADEOK 25
// MakeNoise() Stand alone P372 caller.
#define RTN_MAKENOISEOK 26
// 27 was RTN_P372LOADOK, returned by P533.c LoadP372(), which is gone.
/* End Defines */

/* Struct Definitions */
// Atmospheric noise statistics for one 4-hour time block (GetFamParameters())
// or interpolated to a local hour (AtmosphericNoise_LT()). All values in dB.
struct FamStats {
  int tmblk;       // Timeblock: 0-5 = local time 00-04 ... 20-24 h;
                   // 99 on return from AtmosphericNoise_LT()
  double FA;       // Atmospheric noise in dB above kT0b at 1 MHz
  double SigmaFam; // Standard deviation of values, Fam
  double Du;       // Ratio of upper decile to median value, Fam
  double SigmaDu;  // Standard deviations of values of Du
  double Dl;       // Ratio of median value, Fam, to lower decile
  double SigmaDl;  // Standard deviation of values of Dl
};

// Inputs, coefficients and results of the P372 noise calculation.
// Life cycle: AllocateNoiseMemory() -> InitializeNoise() -> ReadFamDud()
// -> set ManMadeNoise -> Noise() -> ... -> FreeNoiseMemory().
// Noise figures (Fa*) are in dB above kT0b at the operating frequency;
// deciles (Du*, Dl*) are deviations from the median in dB.
// InitializeNoise() sets all twelve outputs to TINYDB.
struct NoiseParams {
  // Output Parameters (set by Noise())
  double FaA;  // Atmospheric noise
  double DuA;  // Atmospheric noise upper decile
  double DlA;  // Atmospheric noise lower decile
  double FaM;  // Man-made noise (holds ManMadeNoise itself on override)
  double DuM;  // Man-made noise upper decile
  double DlM;  // Man-made noise lower decile
  double FaG;  // Galactic noise
  double DuG;  // Galactic noise upper decile
  double DlG;  // Galactic noise lower decile
  double DuT;  // Total noise upper decile
  double DlT;  // Total noise lower decile
  double FamT; // Total noise

  // Non-Output Parameters
  double ManMadeNoise; // Input selector: category code CITY..QUIET (0-5),
                       // other value >= 0 = user value, < 0 = override
                       // (see the defines above and Noise() in Noise.c)
  // Atmospheric noise coefficients for one month, filled by ReadFamDud()
  // from COEFFmmW.txt, allocated by AllocateNoiseMemory():
  double ***fakp;   // [6][16][29] Fourier coefficients of Fam at 1 MHz:
                    // [time block][longitude term 0-14, 15 = constant]
                    // [latitude term 0-28]
  double **fakabp;  // [6][2] linear latitude term per time block
  double **fam;     // [12][14] frequency-variation polynomial coefficients
                    // per time block (0-5 north, 6-11 south)
  double ***dud;    // [5][12][5] polynomial coefficients in log10(f) for
                    // [Du, Dl, SigmaDu, SigmaDl, SigmaFam][time block
                    // 0-5 north, 6-11 south][coefficient]
};
/* End Struct Definitions */

/* Prototypes */
// _cdecl exports for all environments __linux__ && __APPLE__ && _WIN32.
// Full descriptions are in the function headers in the .c files:
//   AllocateNoiseMemory()  NoiseMemory.c  allocate fakp, fakabp, fam, dud;
//                          RTN_ALLOCATEP372OK or RTN_ERRALLOCATE*
//   FreeNoiseMemory()      NoiseMemory.c  free them; RTN_NOISEFREED
//   Noise()                Noise.c  all components and the total for UTC
//                          hour, rlng/rlat (rad), frequency (MHz);
//                          RTN_NOISEOK
//   ReadFamDud()           Noise.c  read COEFF<month+1>W.txt (0-based
//                          month); RTN_READFAMDUDOK or an error code
//   InitializeNoise()      InitializeNoise.c  outputs = TINYDB
//   P372CompileTime(), P372Version()  Noise.c  build strings
//   AtmosphericNoise()     Noise.c  FaA, DuA, DlA for UTC hour iutc
//   AtmosphericNoise_LT()  Noise.c  full FamStats for local hour lrxmt
//   FamFreqVariation()     Noise.c  Fam at frequency (MHz) from Fam at
//                          1 MHz for a hemisphere-adjusted time block
//   MakeNoise()            MakeNoise.c  stand alone wrapper, lat/lng in
//                          degrees, 12 results in out[]; RTN_MAKENOISEOK
P372_API int AllocateNoiseMemory(
    struct NoiseParams *noiseP
);
P372_API int FreeNoiseMemory(
    struct NoiseParams *noiseP
);
P372_API int Noise(
    struct NoiseParams *noiseP,
    int hour,
    double rlng,
    double rlat,
    double frequency
);
P372_API int ReadFamDud(
    struct NoiseParams *noiseP,
    const char *DataFilePath,
    int month
);
P372_API void InitializeNoise(
    struct NoiseParams *noiseP
);
P372_API char const *P372CompileTime(void);
P372_API char const *P372Version(void);
P372_API void AtmosphericNoise(
    struct NoiseParams *noiseP,
    int iutc,
    double rlng,
    double rlat,
    double frequency
);
P372_API void AtmosphericNoise_LT(
    struct NoiseParams *noiseP,
    struct FamStats *FamS,
    int lrxmt,
    double rlng,
    double rlat,
    double frequency
);
// Note: MakeNoise() requires decimal degrees lat and lng.
P372_API double FamFreqVariation(struct NoiseParams *noiseP, int tmblk, double Fam1MHz, double frequency);
P372_API int MakeNoise(
    int month,
    int hour,
    double lat,
    double lng,
    double freq,
    double mmnoise,
    char *datafilepath,
    double *out,
    int pntflag
);

#ifdef _WIN32
    // _stdcall exports dummies used to provide entry points in the DLL for 
    // MS Excel.
    // Each passes its arguments to the _cdecl routine of the same name
    // without the leading underscore (see Noise.c).
    P372_API int __stdcall _AllocateNoiseMemory(
        struct NoiseParams *noiseP
    );
    P372_API int __stdcall _FreeNoiseMemory(
        struct NoiseParams *noiseP
    );
    P372_API int __stdcall _Noise(
        struct NoiseParams *noiseP,
        int hour,
        double rlng,
        double rlat,
        double frequency
    );
    P372_API int __stdcall _ReadFamDud(
        struct NoiseParams *noiseP,
        const char *DataFilePath,
        int month
    );
    P372_API void __stdcall _InitializeNoise(
        struct NoiseParams *noiseP
    );
    P372_API char const *__stdcall _P372CompileTime(void);
    P372_API char const *__stdcall _P372Version(void);
    P372_API void __stdcall _AtmosphericNoise(
        struct NoiseParams *noiseP,
        int iutc,
        double rlng,
        double rlat,
        double frequency
    );
    P372_API void __stdcall _AtmosphericNoise_LT(
        struct NoiseParams *noiseP,
        struct FamStats *FamS,
        int lrxmt,
        double rlng,
        double rlat,
        double frequency
    );
    // Note: MakeNoise() requires decimal degrees lat and lng as input.
    P372_API int __stdcall _MakeNoise(
        int month,
        int hour,
        double lat,
        double lng,
        double freq,
        double mmnoise,
        char *datafilepath,
        double *out,
        int pntflag
    );
#endif
/* End Prototypes */

#endif // NOISE_H
