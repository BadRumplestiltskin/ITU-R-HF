/* ref_driver.c - golden reference generator for the MATLAB P372 tests.
   See ../docs/VALIDATION.md. Compiled by build_reference.sh against the
   unmodified ITU C sources. */
/* ref_driver.c - generates golden reference data from the original ITU P372 C code.
   Built by build_reference.sh. Outputs:
     tests/reference/ref_points.csv   : Noise() + AtmosphericNoise_LT() over a fixed grid
     tests/reference/coeff_dump_01.txt: fakp/fakabp/dud/fam for month 1 (C index order) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Common.h"
#include "Noise.h"

int main(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "usage: ref_driver <datadir/> <outdir/>\n"); return 1; }
    const char *datadir = argv[1];
    const char *outdir = argv[2];
    char fn[512];
    struct NoiseParams np;
    struct FamStats fs;
    int months[] = {1, 4, 7, 10};
    int hours[] = {0, 5, 11, 17, 23};
    double lats[] = {-60, -30, 0, 30, 60};
    double lngs[] = {-150, -105, 0, 45, 165};
    double freqs[] = {0.01, 0.5, 1.0, 3.0, 10.0, 20.0, 30.0};
    double mms[] = {0, 1, 2, 3, 4, 5, -50};

    AllocateNoiseMemory(&np);
    InitializeNoise(&np);

    sprintf(fn, "%sref_points.csv", outdir);
    FILE *fp = fopen(fn, "w");
    if (!fp) { perror(fn); return 1; }
    fprintf(fp, "month,hour,lat,lng,freq,mmnoise,FaA,DuA,DlA,FaM,DuM,DlM,FaG,DuG,DlG,FamT,DuT,DlT,"
                "LT_FA,LT_Du,LT_Dl,LT_SigmaFam,LT_SigmaDu,LT_SigmaDl\n");
    for (int im = 0; im < 4; im++) {
        int month = months[im];
        if (ReadFamDud(&np, datadir, month - 1) != RTN_READFAMDUDOK) { fprintf(stderr, "ReadFamDud failed\n"); return 1; }
        for (int ih = 0; ih < 5; ih++)
        for (int ia = 0; ia < 5; ia++)
        for (int io = 0; io < 5; io++)
        for (int ifq = 0; ifq < 7; ifq++)
        for (int imm = 0; imm < 7; imm++) {
            double rlat = lats[ia] * D2R, rlng = lngs[io] * D2R;
            InitializeNoise(&np);
            np.ManMadeNoise = mms[imm];
            Noise(&np, hours[ih], rlng, rlat, freqs[ifq]);
            /* LT stats use the same hour value interpreted as local time */
            AtmosphericNoise_LT(&np, &fs, hours[ih], rlng, rlat, freqs[ifq]);
            fprintf(fp, "%d,%d,%.10g,%.10g,%.10g,%.10g,"
                        "%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,"
                        "%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                    month, hours[ih], lats[ia], lngs[io], freqs[ifq], mms[imm],
                    np.FaA, np.DuA, np.DlA, np.FaM, np.DuM, np.DlM, np.FaG, np.DuG, np.DlG,
                    np.FamT, np.DuT, np.DlT,
                    fs.FA, fs.Du, fs.Dl, fs.SigmaFam, fs.SigmaDu, fs.SigmaDl);
        }
    }
    fclose(fp);

    /* Coefficient dump for month 1 */
    ReadFamDud(&np, datadir, 0);
    sprintf(fn, "%scoeff_dump_01.txt", outdir);
    fp = fopen(fn, "w");
    fprintf(fp, "fakp 6 16 29\n");
    for (int i = 0; i < 6; i++) for (int j = 0; j < 16; j++) for (int k = 0; k < 29; k++)
        fprintf(fp, "%.17g\n", np.fakp[i][j][k]);
    fprintf(fp, "fakabp 6 2\n");
    for (int j = 0; j < 6; j++) for (int k = 0; k < 2; k++) fprintf(fp, "%.17g\n", np.fakabp[j][k]);
    fprintf(fp, "dud 5 12 5\n");
    for (int i = 0; i < 5; i++) for (int j = 0; j < 12; j++) for (int k = 0; k < 5; k++)
        fprintf(fp, "%.17g\n", np.dud[i][j][k]);
    fprintf(fp, "fam 12 14\n");
    for (int j = 0; j < 12; j++) for (int k = 0; k < 14; k++) fprintf(fp, "%.17g\n", np.fam[j][k]);
    fclose(fp);

    /* README example: Jan, hour index 13 (14 UTC), 40N 165E, 1 MHz, City, printed report */
    double out[12];
    sprintf(fn, "%sreadme_example.txt", outdir);
    FILE *save = stdout;
    (void)save;
    MakeNoise(0, 13, 40.0, 165.0, 1.0, 0.0, (char *)datadir, out, MNNOPRINT);
    fp = fopen(fn, "w");
    for (int i = 0; i < 12; i++) fprintf(fp, "%.12g\n", out[i]);
    fclose(fp);

    FreeNoiseMemory(&np);
    return 0;
}
