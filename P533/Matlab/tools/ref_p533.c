/* ref_p533.c - golden reference generator for the MATLAB P533 port.
   Links the unmodified ITU P533 C sources; loads libp372.so from the
   current directory (as P533 itself does) for ReadFamDud.
   usage: ref_p533 <datadir/> <cases.csv> <out.csv>
          ref_p533 --dump <datadir/> <antfile> <dumpdir/>
   See ../docs/VALIDATION.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include "Common.h"
#include "P533.h"

static int (*fReadFamDud)(struct NoiseParams *, const char *, int);

static void loadP372(void) {
    void *h = dlopen("libp372.so", RTLD_NOW);
    if (!h) { fprintf(stderr, "cannot dlopen libp372.so: %s\n", dlerror()); exit(2); }
    fReadFamDud = dlsym(h, "ReadFamDud");
}

static void writeCP(FILE *fp, struct ControlPt *c) {
    fprintf(fp, ",%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g",
        c->L.lat, c->L.lng, c->distance, c->foE, c->foF2, c->M3kF2, c->dip[0], c->dip[1], c->fH[0], c->fH[1],
        c->ltime, c->hr, c->x, c->Sun.ha, c->Sun.sha, c->Sun.sza, c->Sun.decl, c->Sun.eot, c->Sun.lsr, c->Sun.lsn, c->Sun.lss);
}
static void writeMode(FILE *fp, struct Mode *m) {
    fprintf(fp, ",%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%d",
        m->BMUF, m->MUF90, m->MUF50, m->MUF10, m->OPMUF, m->OPMUF10, m->OPMUF90, m->Fprob, m->deltal, m->deltau,
        m->hr, m->fs, m->Lb, m->Ew, m->ele, m->Prw, m->Grw, m->tau, m->MC);
}
static void header(FILE *fp) {
    const char *cpf[] = {"lat","lng","distance","foE","foF2","M3kF2","dip100","dip300","fH100","fH300","ltime","hr","x","ha","sha","sza","decl","eot","lsr","lsn","lss"};
    const char *cpn[] = {"T1k","Td02","MP","Rd02","R1k"};
    const char *mdf[] = {"BMUF","MUF90","MUF50","MUF10","OPMUF","OPMUF10","OPMUF90","Fprob","deltal","deltau","hr","fs","Lb","Ew","ele","Prw","Grw","tau","MC"};
    fprintf(fp, "id,retval,season,distance,ptick,dmax,B,ele,BMUF,MUF50,MUF90,MUF10,OPMUF,OPMUF90,OPMUF10,n0_F2,n0_E,Es,El,Ei,Ep,Pr,Lz,E0,Gap,Ly,fM,fL,F,fH,Gtl,K0,K1,SNR,DuSN,DlSN,SNRXX,SIR,DuSI,DlSI,RSN,RT,RF,BCR,OCR,OCRs,MIR,probocc,Grw,EIRP,DMidx,FaA,DuA,DlA,FaM,DuM,DlM,FaG,DuG,DlG,FamT,DuT,DlT");
    for (int c = 0; c < 5; c++) for (int f = 0; f < 21; f++) fprintf(fp, ",CP_%s_%s", cpn[c], cpf[f]);
    for (int m = 0; m < 3; m++) for (int f = 0; f < 19; f++) fprintf(fp, ",E%d_%s", m + 1, mdf[f]);
    for (int m = 0; m < 6; m++) for (int f = 0; f < 19; f++) fprintf(fp, ",F2%d_%s", m + 1, mdf[f]);
    fprintf(fp, "\n");
}

static int dumpReaders(const char *datadir, const char *antfile, const char *dumpdir) {
    char fn[512];
    struct PathData path;
    memset(&path, 0, sizeof(path));
    if (AllocatePathMemory(&path) != RTN_ALLOCATEP533OK) return 3;
    /* ionos maps, month 1 and 7, strided subset + full sums */
    int months[2] = {1, 7};
    for (int im = 0; im < 2; im++) {
        if (ReadIonParametersBin(months[im] - 1, path.foF2, path.M3kF2, (char *)datadir, TRUE) != RTN_READIONPARAOK) return 4;
        sprintf(fn, "%sionos_%02d_subset.csv", dumpdir, months[im]);
        FILE *fp = fopen(fn, "w");
        fprintf(fp, "hour,ilng,ilat,issn,foF2,M3kF2\n");
        double s1 = 0, s2 = 0;
        for (int h = 0; h < 24; h++) for (int j = 0; j < 241; j++) for (int k = 0; k < 121; k++) for (int s = 0; s < 2; s++) {
            s1 += path.foF2[h][j][k][s]; s2 += path.M3kF2[h][j][k][s];
            if (h % 5 == 0 && j % 24 == 0 && k % 12 == 0)
                fprintf(fp, "%d,%d,%d,%d,%.9g,%.9g\n", h, j, k, s, path.foF2[h][j][k][s], path.M3kF2[h][j][k][s]);
        }
        fprintf(fp, "sum,,,,%.15g,%.15g\n", s1, s2);
        fclose(fp);
    }
    /* P1239 deciles, full */
    if (ReadP1239(&path, datadir) != RTN_READP1239OK) return 5;
    sprintf(fn, "%sfoF2var.csv", dumpdir);
    FILE *fp = fopen(fn, "w");
    fprintf(fp, "season,hour,ilat,issn,decile,value\n");
    for (int s = 0; s < 3; s++) for (int h = 0; h < 24; h++) for (int l = 0; l < 19; l++) for (int r = 0; r < 3; r++) for (int d = 0; d < 2; d++)
        fprintf(fp, "%d,%d,%d,%d,%d,%.9g\n", s, h, l, r, d, path.foF2var[s][h][l][r][d]);
    fclose(fp);
    /* antenna T13 at bearing 0 and 123.4 deg */
    double bearings[2] = {0.0, 123.4 * D2R};
    for (int b = 0; b < 2; b++) {
        FILE *fa = fopen(antfile, "r");
        if (!fa) return 6;
        struct Antenna ant; memset(&ant, 0, sizeof(ant));
        if (ReadType13(&ant, fa, bearings[b], TRUE) != RTN_READANTENNAPATTERNSOK) return 7;
        fclose(fa);
        sprintf(fn, "%santenna_t13_b%d.csv", dumpdir, b);
        fp = fopen(fn, "w");
        fprintf(fp, "az,el,gain\n");
        for (int a = 0; a < 360; a++) for (int e = 0; e < 91; e++) fprintf(fp, "%d,%d,%.9g\n", a, e, ant.pattern[0][a][e]);
        fclose(fp);
    }
    /* geometry and control point parameters at fixed points */
    sprintf(fn, "%sgeometry.csv", dumpdir);
    fp = fopen(fn, "w");
    fprintf(fp, "lat1,lng1,lat2,lng2,dist,bearingS,bearingL,midlat,midlng,q3lat,q3lng,gmlat,gmlng\n");
    double pts[6][2] = {{49.6667, 6.3167}, {40.0157, -105.2793}, {-33.87, 151.21}, {-1.29, 36.82}, {69.65, 18.96}, {-34.6, -58.4}};
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) {
        if (i == j) continue;
        struct Location a = {pts[i][0] * D2R, pts[i][1] * D2R}, bq = {pts[j][0] * D2R, pts[j][1] * D2R}, gm;
        struct ControlPt mid, q3;
        double d = GreatCircleDistance(a, bq);
        GreatCirclePoint(a, bq, &mid, d, 0.5);
        GreatCirclePoint(a, bq, &q3, d, 0.75);
        GeomagneticCoords(a, &gm);
        fprintf(fp, "%.9g,%.9g,%.9g,%.9g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n", pts[i][0], pts[i][1], pts[j][0], pts[j][1], d,
            Bearing(a, bq, SHORTPATH), Bearing(a, bq, LONGPATH), mid.L.lat, mid.L.lng, q3.L.lat, q3.L.lng, gm.lat, gm.lng);
    }
    fclose(fp);
    /* control point parameters: months 1,4,7,10 x hours 0,6,12,18 x ssn 10,70,150 at the 6 points */
    sprintf(fn, "%scontrolpoints.csv", dumpdir);
    fp = fopen(fn, "w");
    fprintf(fp, "month,hour,ssn,lat,lng,foE,foF2,M3kF2,dip100,dip300,fH100,fH300,ltime,ha,sha,sza,decl,eot,lsr,lsn,lss\n");
    int mm[4] = {1, 4, 7, 10}, hh[4] = {0, 6, 12, 18}, ss[3] = {10, 70, 150};
    for (int im = 0; im < 4; im++) {
        if (ReadIonParametersBin(mm[im] - 1, path.foF2, path.M3kF2, (char *)datadir, TRUE) != RTN_READIONPARAOK) return 4;
        path.month = mm[im] - 1;
        for (int ih = 0; ih < 4; ih++) for (int is = 0; is < 3; is++) for (int i = 0; i < 6; i++) {
            path.hour = hh[ih]; path.SSN = ss[is];
            struct ControlPt cp; memset(&cp, 0, sizeof(cp));
            cp.L.lat = pts[i][0] * D2R; cp.L.lng = pts[i][1] * D2R;
            CalculateCPParameters(&path, &cp);
            fprintf(fp, "%d,%d,%d,%.9g,%.9g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                mm[im], hh[ih], ss[is], pts[i][0], pts[i][1], cp.foE, cp.foF2, cp.M3kF2, cp.dip[0], cp.dip[1], cp.fH[0], cp.fH[1],
                cp.ltime, cp.Sun.ha, cp.Sun.sha, cp.Sun.sza, cp.Sun.decl, cp.Sun.eot, cp.Sun.lsr, cp.Sun.lsn, cp.Sun.lss);
        }
    }
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc >= 5 && strcmp(argv[1], "--dump") == 0) return dumpReaders(argv[2], argv[3], argv[4]);
    if (argc < 4) { fprintf(stderr, "usage: ref_p533 <datadir/> <cases.csv> <out.csv>\n"); return 1; }
    const char *datadir = argv[1];
    loadP372();
    struct PathData path;
    memset(&path, 0, sizeof(path));
    if (AllocatePathMemory(&path) != RTN_ALLOCATEP533OK) { fprintf(stderr, "alloc failed\n"); return 3; }
    if (ReadP1239(&path, datadir) != RTN_READP1239OK) { fprintf(stderr, "P1239 failed\n"); return 5; }
    IsotropicPattern(&path.A_tx, 0.0, TRUE);
    IsotropicPattern(&path.A_rx, 0.0, TRUE);
    struct Antenna isoTx = path.A_tx, isoRx = path.A_rx;   /* keep the isotropic patterns */

    FILE *fin = fopen(argv[2], "r"), *fout = fopen(argv[3], "w");
    if (!fin || !fout) { fprintf(stderr, "cannot open files\n"); return 1; }
    header(fout);
    char line[2048];
    fgets(line, sizeof line, fin);                 /* header */
    int curMonth = -1, n = 0;
    while (fgets(line, sizeof line, fin)) {
        int id, year, month, hour, ssn, mod, snrxxp, sorl;
        double freq, txlat, txlng, rxlat, rxlng, mm, txpower, bw, snrr, sirr, A, TW, FW, T0, F0, txgos, rxgos;
        char txant[256], rxant[256];
        if (sscanf(line, "%d,%d,%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%d,%lf,%lf,%lf,%d,%lf,%lf,%lf,%lf,%lf,%lf,%d,%255[^,],%lf,%255[^,],%lf",
                &id, &year, &month, &hour, &ssn, &freq, &txlat, &txlng, &rxlat, &rxlng, &mm, &mod, &txpower, &bw, &snrr, &snrxxp,
                &sirr, &A, &TW, &FW, &T0, &F0, &sorl, txant, &txgos, rxant, &rxgos) != 27) {
            fprintf(stderr, "bad case line: %s", line); continue;
        }
        if (month != curMonth) {
            if (ReadIonParametersBin(month - 1, path.foF2, path.M3kF2, (char *)datadir, TRUE) != RTN_READIONPARAOK) { fprintf(stderr, "ionos failed\n"); return 4; }
            if (fReadFamDud(&path.noiseP, datadir, month - 1) != RTN_READFAMDUDOK) { fprintf(stderr, "FamDud failed\n"); return 4; }
            curMonth = month;
        }
        strcpy(path.name, "ref"); strcpy(path.txname, "TX"); strcpy(path.rxname, "RX");
        path.year = year; path.month = month - 1; path.hour = hour; path.SSN = ssn;
        path.Modulation = mod; path.SorL = sorl; path.frequency = freq; path.BW = bw; path.txpower = txpower;
        path.SNRXXp = snrxxp; path.SNRr = snrr; path.SIRr = sirr; path.F0 = F0; path.T0 = T0; path.A = A; path.TW = TW; path.FW = FW;
        path.L_tx.lat = txlat * D2R; path.L_tx.lng = txlng * D2R; path.L_rx.lat = rxlat * D2R; path.L_rx.lng = rxlng * D2R;
        path.noiseP.ManMadeNoise = mm;
        /* antennas: ISOTROPIC or a Type 13 file; bearing points at the other end as ITURHFProp does */
        if (strcmp(txant, "ISOTROPIC") == 0) { path.A_tx = isoTx; IsotropicPattern(&path.A_tx, txgos, TRUE); }
        else { FILE *fa = fopen(txant, "r"); if (!fa) { fprintf(stderr, "no antenna %s\n", txant); return 6; }
               ReadType13(&path.A_tx, fa, Bearing(path.L_tx, path.L_rx, sorl), TRUE); fclose(fa); }
        if (strcmp(rxant, "ISOTROPIC") == 0) { path.A_rx = isoRx; IsotropicPattern(&path.A_rx, rxgos, TRUE); }
        else { FILE *fa = fopen(rxant, "r"); if (!fa) { fprintf(stderr, "no antenna %s\n", rxant); return 6; }
               ReadType13(&path.A_rx, fa, Bearing(path.L_rx, path.L_tx, sorl), TRUE); fclose(fa); }

        int rv = P533(&path);
        fprintf(fout, "%d,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g",
            id, rv, path.season, path.distance, path.ptick, path.dmax, path.B, path.ele, path.BMUF, path.MUF50, path.MUF90, path.MUF10,
            path.OPMUF, path.OPMUF90, path.OPMUF10, path.n0_F2, path.n0_E, path.Es, path.El, path.Ei, path.Ep, path.Pr, path.Lz,
            path.E0, path.Gap, path.Ly, path.fM, path.fL, path.F, path.fH, path.Gtl, path.K[0], path.K[1],
            path.SNR, path.DuSN, path.DlSN, path.SNRXX, path.SIR, path.DuSI, path.DlSI, path.RSN, path.RT, path.RF,
            path.BCR, path.OCR, path.OCRs, path.MIR, path.probocc, path.Grw, path.EIRP, path.DMidx,
            path.noiseP.FaA, path.noiseP.DuA, path.noiseP.DlA, path.noiseP.FaM, path.noiseP.DuM, path.noiseP.DlM,
            path.noiseP.FaG, path.noiseP.DuG, path.noiseP.DlG, path.noiseP.FamT, path.noiseP.DuT, path.noiseP.DlT);
        for (int c = 0; c < 5; c++) writeCP(fout, &path.CP[c]);
        for (int m = 0; m < 3; m++) writeMode(fout, &path.Md_E[m]);
        for (int m = 0; m < 6; m++) writeMode(fout, &path.Md_F2[m]);
        fprintf(fout, "\n");
        n++;
    }
    fclose(fin); fclose(fout);
    fprintf(stderr, "%d cases written\n", n);
    return 0;
}
