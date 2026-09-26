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
struct FamStats {
  int tmblk;       // Timeblock
  double FA;       // Atmospheric noise in dB above kT0b at 1 MHz
  double SigmaFam; // Standard deviation of values, Fam
  double Du;       // Ratio of upper decile to median value, Fam
  double SigmaDu;  // Standard deviations of values of Du
  double Dl;       // Ratio of median value, Fam, to lower decile
  double SigmaDl;  // Standard deviation of values of Dl
};

struct NoiseParams {
  // Output Parameters
  double FaA;  // Atmospheric noise
  double DuA;  // Atmospheric noise upper decile
  double DlA;  // Atmospheric noise lower decile
  double FaM;  // Man-made noise
  double DuM;  // Man-made noise upper decile
  double DlM;  // Man-made noise lower decile
  double FaG;  // Galactic noise
  double DuG;  // Galactic noise upper decile
  double DlG;  // Galactic noise lower decile
  double DuT;  // Total noise upper decile
  double DlT;  // Total noise lower decile
  double FamT; // Total noise

  // Non-Output Parameters
  double ManMadeNoise;
  double ***fakp;
  double **fakabp;
  double **fam;
  double ***dud;
};
/* End Struct Definitions */

/* Prototypes */
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
