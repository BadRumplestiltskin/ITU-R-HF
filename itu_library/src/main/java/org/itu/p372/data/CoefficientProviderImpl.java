// Directory: src/main/java/org/itu/p372/data

package org.itu.p372.data;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

/**
 * Implementation of CoefficientProvider that reads ITU-R P.372 coefficient files
 * from classpath resources and interpolates parameters.
 */
public class CoefficientProviderImpl implements CoefficientProvider {

    private static final int HEADER_LINES1 = 400;
    private static final int HEADER_LINES2 = 181;
    private static final int HEADER_LINES3 = 84;
    private static final int HEADER_LINES4 = 114;
    private static final int HEADER_LINES5 = 175;
    private static final int HEADER_LINES6 = 114;
    private static final int HEADER_LINES7 = 155;
    private static final int HEADER_LINES8 = 202;
    private static final int HEADER_LINES9 = 138;

    private final double[][][][] fakp = new double[12][6][16][29];
    private final double[][][] fakabp = new double[12][6][2];
    private final double[][][][] dud = new double[12][5][12][5];
    private final double[][] famCoeff = new double[12][14];

    public CoefficientProviderImpl() {
        for (int month = 0; month < 12; month++) {
            String resource = String.format(Locale.ROOT, "/coeff/COEFF%02dW.txt", month + 1);
            try (InputStream is = getClass().getResourceAsStream(resource)) {
                if (is == null) {
                    throw new RuntimeException("Coefficient resource not found: " + resource);
                }
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {
                    skipHeader(reader);
                    parseFakp(reader, month);
                    parseFakabp(reader, month);
                    parseDud(reader, month);
                    parseFam(reader, month);
                }
            } catch (IOException e) {
                throw new RuntimeException("Failed to load coefficients for month " + (month + 1), e);
            }
        }
    }

    private void skipHeader(BufferedReader reader) throws IOException {
        reader.readLine();
        skipLines(reader, HEADER_LINES1);
        skipLines(reader, HEADER_LINES2);
        skipLines(reader, HEADER_LINES3);
        skipLines(reader, HEADER_LINES4);
        skipLines(reader, HEADER_LINES5);
        skipLines(reader, HEADER_LINES6);
        skipLines(reader, HEADER_LINES7);
        skipLines(reader, HEADER_LINES8);
        skipLines(reader, HEADER_LINES9);
    }

    private void skipLines(BufferedReader reader, int count) throws IOException {
        for (int i = 0; i < count; i++) reader.readLine();
    }

    private void parseFakp(BufferedReader reader, int month) throws IOException {
        reader.readLine(); // fakp header
        double[] temp = new double[29 * 16 * 6];
        int idx = 0;
        for (int i = 0; i < 556; i++) {
            idx = readValues(reader.readLine(), temp, idx);
        }
        idx = readValues(reader.readLine(), temp, idx);
        idx = 0;
        for (int tb = 0; tb < 6; tb++) {
            for (int hi = 0; hi < 16; hi++) {
                for (int ti = 0; ti < 29; ti++) {
                    fakp[month][tb][hi][ti] = temp[idx++];
                }
            }
        }
    }

    private void parseFakabp(BufferedReader reader, int month) throws IOException {
        reader.readLine();
        double[] temp = new double[6 * 2];
        int idx = 0;
        for (int i = 0; i < 2; i++) {
            idx = readValues(reader.readLine(), temp, idx);
        }
        idx = readValues(reader.readLine(), temp, idx);
        idx = 0;
        for (int j = 0; j < 6; j++) {
            for (int k = 0; k < 2; k++) {
                fakabp[month][j][k] = temp[idx++];
            }
        }
    }

    private void parseDud(BufferedReader reader, int month) throws IOException {
        reader.readLine();
        double[] temp = new double[5 * 12 * 5];
        int idx = 0;
        for (int i = 0; i < 60; i++) {
            idx = readValues(reader.readLine(), temp, idx);
        }
        idx = 0;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 12; j++) {
                for (int k = 0; k < 5; k++) {
                    dud[month][i][j][k] = temp[idx++];
                }
            }
        }
    }

    private void parseFam(BufferedReader reader, int month) throws IOException {
        reader.readLine();
        double[] temp = new double[12 * 14];
        int idx = 0;
        for (int i = 0; i < 33; i++) {
            idx = readValues(reader.readLine(), temp, idx);
        }
        idx = readValues(reader.readLine(), temp, idx);
        idx = 0;
        for (int j = 0; j < 12; j++) {
            for (int k = 0; k < 14; k++) {
                famCoeff[j][k] = temp[idx++];
            }
        }
    }

    private int readValues(String line, double[] array, int offset) {
        String[] parts = line.trim().split("\\s+");
        for (String p : parts) {
            array[offset++] = Double.parseDouble(p);
        }
        return offset;
    }

    @Override
    public FamParameters getFamParameters(int month, int blockIndex,
                                          double latDeg, double lonDeg, double freqMHz) {
        // existing implementation remains unchanged
        double rlng = Math.toRadians(lonDeg);
        double rlat = Math.toRadians(latDeg);
        double q = (rlng < 0 ? (rlng + 2*Math.PI)/2 : rlng/2);
        double[] ZZ = new double[29];
        for (int j = 0; j < 29; j++) {
            double sum = 0;
            for (int k = 0; k < 15; k++) {
                sum += Math.sin((k+1)*q)*fakp[month][blockIndex][k][j];
            }
            ZZ[j] = sum + fakp[month][blockIndex][15][j];
        }
        q = rlat + Math.PI/2;
        double R = 0;
        for (int j = 0; j < 29; j++) {
            R += Math.sin((j+1)*q)*ZZ[j];
        }
        double Fam1 = R + famCoeff[month][0] + famCoeff[month][1]*q;
        double X = Math.log10(Math.min(freqMHz, 20.0));
        double u1 = -0.75;
        double u  = (8*Math.pow(2,X)-11)/4;
        int i = blockIndex + (rlat<0?6:0);
        double pz = u1*famCoeff[month][0] + famCoeff[month][1];
        double px = u1*famCoeff[month][7] + famCoeff[month][8];
        for (int j = 2; j < 7; j++) {
            pz = u1*pz + famCoeff[month][j];
            px = u1*px + famCoeff[month][j+7];
        }
        double cz = Fam1*(2-pz)-px;
        pz = u*famCoeff[month][0] + famCoeff[month][1];
        px = u*famCoeff[month][7] + famCoeff[month][8];
        for (int j = 2; j < 7; j++) {
            pz = u*pz + famCoeff[month][j];
            px = u*px + famCoeff[month][j+7];
        }
        double fa = cz*pz + px;
        double duPoly = dud[month][0][i][0];
        double dlPoly = dud[month][1][i][0];
        for (int k = 1; k < 5; k++) {
            duPoly = duPoly*X + dud[month][0][i][k];
            dlPoly = dlPoly*X + dud[month][1][i][k];
        }
        return new FamParameters(fa, duPoly, dlPoly);
    }
}
