#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local includes
#include "Common.h"
#include "Noise.h"
// End Local includes 

void InitializeNoise(
    struct NoiseParams* noiseP
) {
    /*
    Set every output member of the noise structure to the sentinel TINYDB
    (Common.h: DBL_MIN_10_EXP, i.e. about -307 dB), marking the results as
    "not yet computed". Call after AllocateNoiseMemory() and before Noise().

        INPUT
            struct NoiseParams *noiseP - structure to initialise

        OUTPUT
            noiseP->FaA, FaG, FaM, FamT      (noise figures)      = TINYDB
            noiseP->DlA, DlG, DlM, DlT       (lower deciles)      = TINYDB
            noiseP->DuA, DuG, DuM, DuT       (upper deciles)      = TINYDB
            Not touched: ManMadeNoise and the coefficient arrays fakp,
            fakabp, fam, dud.

        SUBROUTINES
            None
    */
    noiseP->FaA = TINYDB;
    noiseP->FaG = TINYDB;
    noiseP->FaM = TINYDB;
    noiseP->FamT = TINYDB;

    noiseP->DlA = TINYDB;
    noiseP->DlG = TINYDB;
    noiseP->DlM = TINYDB;
    noiseP->DuG = TINYDB;
    noiseP->DuM = TINYDB;
    noiseP->DlT = TINYDB;
    noiseP->DuA = TINYDB;
    noiseP->DuT = TINYDB;
}
