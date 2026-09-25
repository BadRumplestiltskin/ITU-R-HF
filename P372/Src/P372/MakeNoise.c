#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// Local includes
#include "Common.h"
#include "Noise.h"

// Local Prototypes
void PrintFam(
    FILE *fp,
    struct NoiseParams *noiseP,
    int month,
    int hour,
    double lng,
    double lat,
    double freq,
    char *ntimestr,
    const char *P372ver,
    const char *P372compt
);
// End Local Prototypes

int MakeNoise(
    int month,
    int hour,
    double lat,
    double lng,
    double freq,
    double mmnoise,
    char *datafilepath,
    double *out,
    int pntflag
) {

    /*
    Stand alone subroutine to use the P.372-14 method.
    Performs a complete noise calculation for one location, month, hour and
    frequency: allocates and initialises a private NoiseParams, reads the
    month's coefficient file with ReadFamDud(), sets the man-made noise
    selector, calls Noise(), copies the twelve results into out[] and
    optionally prints them. The structure's memory is always freed before
    return once allocation succeeded.

        INPUT
            int month         0-based month (0 = January ... 11 = December);
                              selects COEFF<month+1>W.txt (see ReadFamDud())
            int hour          UTC hour index passed unchanged to Noise(); the
                              printout labels it as hour + 1 "(UTC) (1 to 24)"
            double lat        (degrees, decimal; converted to radians here)
            double lng        (degrees, decimal, east positive; converted to
                              radians here)
            double freq       (MHz)
            double mmnoise    Man-made noise selector copied into
                              noiseP.ManMadeNoise: one of the category codes
                              CITY 0, RESIDENTIAL 1, RURAL 2, QUIETRURAL 3,
                              NOISY 4, QUIET 5 (Noise.h); any other
                              non-negative value is used as a user value (see
                              ManMadeNoise() in Noise.c); a negative value
                              overrides the whole calculation (see Noise()).
            char* datafilepath  Directory holding the COEFFxxW.txt files
            int pntflag       MNNOPRINT 0 - no output
                              MNPRINTTOSTDOUT 1 - print report to stdout
                              MNPRINTTOFILE 2 - write report to
                                  ".\\MakeNoiseOut.txt" (name hard coded)
                              Any other value is treated as MNNOPRINT.

        OUTPUT
            double* out
                Pointer to an array of 12 doubles supplied by the caller,
                all in dB (figures are dB above kT0b):
                  out[0]  FaA   out[1]  DuA   out[2]  DlA   (atmospheric)
                  out[3]  FaM   out[4]  DuM   out[5]  DlM   (man-made)
                  out[6]  FaG   out[7]  DuG   out[8]  DlG   (galactic)
                  out[9]  FamT  out[10] DuT   out[11] DlT   (total)
                out[] is written only if ReadFamDud() and Noise() succeed.

            Returns
                RTN_MAKENOISEOK        success
                RTN_ERRALLOCATENOISE   AllocateNoiseMemory() failed
                RTN_ERROPENCOEFFFILE, RTN_ERRREADCOEFFFILE or an allocation
                                       code passed through from ReadFamDud()
                RTN_ERRMNCANTOPENFILE  report file could not be opened

        SUBROUTINES
            AllocateNoiseMemory(), InitializeNoise(), P372Version(),
            P372CompileTime(), ReadFamDud(), Noise(), PrintFam(),
            FreeNoiseMemory()
    */

    FILE *fp;

    int retval;

    double rlat, rlng;

    const char *P372ver;
    const char *P372compt;
    char outputfile[256];

    struct NoiseParams noiseP;

    struct tm* ntime;
    time_t tm;

    char ntimestr[64];

    rlat = lat * D2R;
    rlng = lng * D2R;

    // Allocate the memory in the noise structure
    retval = AllocateNoiseMemory(&noiseP);
    if (retval != RTN_ALLOCATEP372OK) {
        return RTN_ERRALLOCATENOISE;
    }

    // Initialize Noise from the P372.dll
    InitializeNoise(&noiseP);
    // End Initialize Noise

    // Load the version and compile time of the P372.DLL
    P372ver = P372Version();
    P372compt = P372CompileTime();

    // Get the time to time stamp the output files.
    tm = time(NULL);
    ntime = localtime(&tm);
    sprintf(
        ntimestr,
        "%d/%d/%d - %02d:%02d:%02d",
        ntime->tm_mday,
        ntime->tm_mon + 1,
        ntime->tm_year - 100,
        ntime->tm_hour,
        ntime->tm_min,
        ntime->tm_sec
    );

    // Read in the atmospheric coefficients for the particular month.
    // The subroutine dllReadFamDud() is from P372.dll
    retval = ReadFamDud(
        &noiseP,
        datafilepath,
        month
    );
    if (retval != RTN_READFAMDUDOK) goto done;

    noiseP.ManMadeNoise = mmnoise;

    // Call noise from the P372.dll
    retval = Noise(
        &noiseP,
        hour,
        rlng,
        rlat,
        freq
    );
    // check that the input parameters are correct
    if (retval != RTN_NOISEOK) goto done;

    *out = noiseP.FaA;
    *(out + 1) = noiseP.DuA;
    *(out + 2) = noiseP.DlA;
    *(out + 3) = noiseP.FaM;
    *(out + 4) = noiseP.DuM;
    *(out + 5) = noiseP.DlM;
    *(out + 6) = noiseP.FaG;
    *(out + 7) = noiseP.DuG;
    *(out + 8) = noiseP.DlG;
    *(out + 9) = noiseP.FamT;
    *(out + 10) = noiseP.DuT;
    *(out + 11) = noiseP.DlT;

    // Check to see if there is a pntflag
    if ((pntflag != MNNOPRINT)
        && (pntflag != MNPRINTTOFILE)
        && (pntflag != MNPRINTTOSTDOUT)) {
        // The caller wants MakeNoise() to be silent and will presumably
        // use the output parameters elsewhere.
        pntflag = MNNOPRINT;
    }

    // Does the caller desirer output?
    if (pntflag == MNPRINTTOSTDOUT) {
        PrintFam(
            stdout,
            &noiseP,
            month,
            hour,
            rlng,
            rlat,
            freq,
            ntimestr,
            P372ver,
            P372compt
        );
    }
    else if (pntflag == MNPRINTTOFILE) {
        sprintf(
            outputfile,
            ".\\MakeNoiseOut.txt"
        );
        fp = fopen(outputfile, "w");
        if (fp == NULL) {
            printf(
                "ITURNoise: Error: Can't open output file %s (%s)\n",
                outputfile,
                strerror(errno)
            );
            retval = RTN_ERRMNCANTOPENFILE;
            goto done;
        }
        printf(
            "MakeNoise: Writing output file %s\n",
            outputfile
        );
        PrintFam(
            fp,
            &noiseP,
            month,
            hour,
            rlng,
            rlat,
            freq,
            ntimestr,
            P372ver,
            P372compt
        );
        fclose(fp);
    }

    retval = RTN_MAKENOISEOK;

    // Every path after a successful allocation leaves through here, so the
    // noise arrays are released on success and on error alike.
done:
    FreeNoiseMemory(&noiseP);
    return retval;
}

void PrintFam(
    FILE* fp,
    struct NoiseParams* noiseP,
    int month,
    int hour,
    double rlng,
    double rlat,
    double freq,
    char* ntimestr,
    const char* P372ver,
    const char* P372compt
) {
    /*
    Write a formatted report of one MakeNoise() result: a banner with the
    analysis time stamp and the P372 version/compile time, the month, hour,
    location and frequency, then the twelve noise figures and deciles.

        INPUT
            FILE *fp               Open output stream (stdout or a file)
            struct NoiseParams *noiseP  Structure already filled by Noise()
            int month              0-based month (index into month names)
            int hour               Hour index; printed as hour + 1 (1 to 24)
            double rlng            Longitude (rad); printed in degrees
            double rlat            Latitude (rad); printed in degrees
            double freq            Frequency (MHz)
            char *ntimestr         Analysis time stamp text
            const char *P372ver    Version string
            const char *P372compt  Compile time string

        OUTPUT
            Text written to fp. Nothing is returned.

        SUBROUTINES
            None
    */

    const char* monthnames[] = {
        "JANUARY ",
        "FEBRUARY",
        "MARCH",
        "APRIL",
        "MAY",
        "JUNE",
        "JULY",
        "AUGUST",
        "SEPTEMBER",
        "OCTOBER",
        "NOVEMBER",
        "DECEMBER"
    };

    fprintf(fp, "**********************************************************\n");
    fprintf(fp, "\tITU-R Study Group 3: Radiowave Propagation\n");
    fprintf(fp, "**********************************************************\n");
    fprintf(fp, "\tAnalysis: %s\n", ntimestr);
    fprintf(fp, "\tP372 Version:      %s\n", P372ver);
    fprintf(fp, "\tP372 Compile Time: %s\n", P372compt);
    fprintf(fp, "**********************************************************\n");
    fprintf(fp, "\n");
    fprintf(fp, "\t%s : %d (UTC) (1 to 24)\n", monthnames[month], hour + 1);
    fprintf(fp, "\t%5.4f (deg lat) %5.4f (deg long)\n", rlat * R2D, rlng * R2D);
    fprintf(fp, "\t%5.3f (MHz)\n", freq);
    fprintf(fp, "\n");
    fprintf(fp, "\t[FaA]  Noise Component (Atmospheric): %5.3f\n", noiseP->FaA);
    fprintf(fp, "\t[DuA]  Upper Decile    (Atmospheric): %5.3f\n", noiseP->DuA);
    fprintf(fp, "\t[DlA]  Lower Decile    (Atmospheric): %5.3f\n", noiseP->DlA);
    fprintf(fp, "\t[FaM]  Noise Component    (Man-Made): %5.3f\n", noiseP->FaM);
    fprintf(fp, "\t[DuM]  Upper Decile       (Man-Made): %5.3f\n", noiseP->DuM);
    fprintf(fp, "\t[DlM]  Lower Decile       (Man-Made): %5.3f\n", noiseP->DlM);
    fprintf(fp, "\t[FaG]  Noise Component    (Galactic): %5.3f\n", noiseP->FaG);
    fprintf(fp, "\t[DuG]  Upper Decile       (Galactic): %5.3f\n", noiseP->DuG);
    fprintf(fp, "\t[DlG]  Lower Decile       (Galactic): %5.3f\n", noiseP->DlG);
    fprintf(fp, "\t[FamT] Noise                 (Total): %5.3f\n", noiseP->FamT);
    fprintf(fp, "\t[DuT]  Upper Decile          (Total): %5.3f\n", noiseP->DuT);
    fprintf(fp, "\t[DlT]  Lower Decile          (Total): %5.3f\n", noiseP->DlT);
    fprintf(fp, "\n");
    fprintf(fp, "**********************************************************\n");

    return;
}
