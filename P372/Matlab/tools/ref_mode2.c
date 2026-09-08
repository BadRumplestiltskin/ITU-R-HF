/* ref_mode2.c - replicates RunAtmosNoiseMonths() from ITURNoise.c for month 1 (Jan)
   and local hour 0 only, writing a_1m0h.csv, b_1m0h.csv, c_1m0h.csv with the exact
   C formatting so the MATLAB output can be diffed. Also dumps V_d coefficient arrays. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Common.h"
#include "Noise.h"

static void FindV_d(double freq, double c[5], double d[5], double *V_d, double *sigma_V_d) {
    double x = log10(freq), x2 = x * x, x3 = x2 * x, x4 = x3 * x;
    *V_d = c[0] + c[1] * x + c[2] * x2 + c[3] * x3 + c[4] * x4;
    *sigma_V_d = d[0] + d[1] * x + d[2] * x2 + d[3] * x3 + d[4] * x4;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "usage: ref_mode2 <datadir/> <outdir/>\n"); return 1; }
    const char *datadir = argv[1], *outdir = argv[2];
    char fn[512], line[256], strl[5][256];
    int dummy, i, s, tb, retval;
    double c[4][6][5], d[4][6][5];
    double f_log[41] = { 0.01, 0.015, 0.02, 0.025, 0.03, 0.035, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09,
        0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
        1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 15.0, 20.0, 25.0, 30.0};
    struct NoiseParams noiseP; struct FamStats FamS;
    double rlat, rlng, freq, V_d, sigma_V_d, pz, px, cz, u[2], Fam[11], Fam1MHz;
    FILE *fp;

    sprintf(fn, "%sV_d.txt", datadir); FILE *fv = fopen(fn, "r");
    sprintf(fn, "%ssigma_V_d.txt", datadir); FILE *fs = fopen(fn, "r");
    if (!fv || !fs) { fprintf(stderr, "cannot open V_d files\n"); return 1; }
    tb = 0; s = 0;
    while (fscanf(fv, "%[^\n] ", line) != EOF) {
        sscanf(line, "%d %d %s %s %s %s %s", &dummy, &dummy, strl[4], strl[3], strl[2], strl[1], strl[0]);
        for (i = 0; i < 5; i++) c[s][tb][i] = atof(strl[i]);
        if (++tb == 6) { tb = 0; s++; }
    }
    tb = 0; s = 0;
    while (fscanf(fs, "%[^\n] ", line) != EOF) {
        sscanf(line, "%d %d %s %s %s %s %s", &dummy, &dummy, strl[4], strl[3], strl[2], strl[1], strl[0]);
        for (i = 0; i < 5; i++) d[s][tb][i] = atof(strl[i]);
        if (++tb == 6) { tb = 0; s++; }
    }
    fclose(fv); fclose(fs);

    sprintf(fn, "%svd_coeffs.txt", outdir); fp = fopen(fn, "w");
    for (s = 0; s < 4; s++) for (tb = 0; tb < 6; tb++) for (i = 0; i < 5; i++) fprintf(fp, "%.17g\n", c[s][tb][i]);
    for (s = 0; s < 4; s++) for (tb = 0; tb < 6; tb++) for (i = 0; i < 5; i++) fprintf(fp, "%.17g\n", d[s][tb][i]);
    fclose(fp);

    AllocateNoiseMemory(&noiseP); InitializeNoise(&noiseP);
    noiseP.ManMadeNoise = RURAL;
    int m = 0, h = 0;
    retval = ReadFamDud(&noiseP, datadir, m);
    if (retval != RTN_READFAMDUDOK) return retval;

    /* a) */
    freq = 1.0;
    sprintf(fn, "%sa_%dm%dh.csv", outdir, m + 1, h); fp = fopen(fn, "w");
    fprintf(fp, "month,hour,freq,latitude,longitude,FaA\n");
    for (int ilat = -90; ilat <= 90; ilat++) {
        rlat = ilat * D2R;
        for (int ilng = -180; ilng <= 180; ilng++) {
            rlng = ilng * D2R;
            AtmosphericNoise_LT(&noiseP, &FamS, h, rlng, rlat, freq);
            fprintf(fp, "%d, %d, %5.4f, %5.4f, %5.4f, %5.4f\n", m + 1, h, freq, rlat * R2D, rlng * R2D, FamS.FA);
        }
    }
    fclose(fp);

    /* b) */
    rlat = 40.015744 * D2R; rlng = -105.27932 * D2R;
    sprintf(fn, "%sb_%dm%dh.csv", outdir, m + 1, h); fp = fopen(fn, "w");
    fprintf(fp, "month,hour,freq,latitude,longitude,Fam5,Fam10,Fam20,Fam30,Fam40,Fam50,Fam60,Fam70,Fam80,Fam90,Fam100\n");
    for (int f = 0; f < 41; f++) {
        for (int F1 = 0; F1 <= 10; F1++) {
            Fam1MHz = (F1 == 0) ? 5.0 : F1 * 10.0;
            FamS.tmblk = (int)(h / 4.0);
            i = (rlat < 0) ? FamS.tmblk + 6 : FamS.tmblk;
            u[0] = -0.75;
            u[1] = (8.0 * pow(2.0, log10(f_log[f])) - 11.0) / 4.0;
            for (int k = 0; k < 2; k++) {
                pz = u[k] * noiseP.fam[i][0] + noiseP.fam[i][1];
                px = u[k] * noiseP.fam[i][7] + noiseP.fam[i][8];
                for (int j = 2; j < 7; j++) { pz = u[k] * pz + noiseP.fam[i][j]; px = u[k] * px + noiseP.fam[i][j + 7]; }
                if (k == 0) cz = Fam1MHz * (2.0 - pz) - px;
            }
            Fam[F1] = cz * pz + px;
        }
        fprintf(fp, "%d, %d, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f\n",
            m + 1, h, f_log[f], rlat * R2D, rlng * R2D, Fam[0], Fam[1], Fam[2], Fam[3], Fam[4], Fam[5], Fam[6], Fam[7], Fam[8], Fam[9], Fam[10]);
    }
    fclose(fp);

    /* c) */
    s = m / 3; tb = h / 4;
    sprintf(fn, "%sc_%dm%dh.csv", outdir, m + 1, h); fp = fopen(fn, "w");
    fprintf(fp, "month,hour,freq,latitude,longitude,FaA,DuA,DlA,sigmaFaA,sigmaDuA,sigmaDlA,V_d,sigma_V_d\n");
    for (int f = 0; f < 41; f++) {
        AtmosphericNoise_LT(&noiseP, &FamS, h, rlng, rlat, f_log[f]);
        FindV_d(f_log[f], c[s][tb], d[s][tb], &V_d, &sigma_V_d);
        fprintf(fp, "%d, %d, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f, %5.4f\n",
            m + 1, h, f_log[f], rlat * R2D, rlng * R2D, FamS.FA, FamS.Du, FamS.Dl, FamS.SigmaFam, FamS.SigmaDu, FamS.SigmaDl, V_d, sigma_V_d);
    }
    fclose(fp);
    FreeNoiseMemory(&noiseP);
    return 0;
}
