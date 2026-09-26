#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"

// Reference radius of the magnetic field model, P.1239-4 equation (8) (km)
#define RMAG 6371.2

void magfit(struct ControlPt *here, double height) {

	/*
	 	magfit() calculates the magnetic dip and the gyrofrequency
			This calculation is described in Section 2 of Rec P.1239 (1997) equations (5) thru (11).
			(Verified against P.1239-4 Annex 1 section 2: the sixth-order spherical-harmonic
			field model, epoch 1960, equations (5a)-(5c) for Fx, Fy, Fz, (6a)-(6c), (7) the
			associated Legendre functions, (8) R = 6371.2/(6371.2 + hr), (9) F, (10) the dip
			I = arctan(Fz/sqrt(Fx^2 + Fy^2)) and (11) fH = 2.8 F.)
			This subroutine is patterned after the Fortran subroutine by the same name in the ITS
			propagation package, 2006.
			The input lat and long are in radians.
			P.533-14 uses the results at 300 km for fH in equation (3) (section 3.5.1.1) and at
			100 km for fL = |fH sin(I)|, equation (23), and the dip of the absorption and
			scattering calculations.

	 		Initialized the arrays
	 		P = The Associated Legendre function
	 		DP = The derivative of P
	 		CT = Appears to be the Associated Legendre function coefficients as a function of m and n
	 		G & H = Numerical coefficients for the field model (gauss)
			(The G and H values are not printed in P.1239-4, which only states that the
			epoch 1960 model is used; they are taken from the ITS/REC533 code and are not
			verified against the Recommendation here.)

	 		INPUT
	 			struct ControlPt *here - Control point of interest; here->L.lat and
					here->L.lng (radians, N and E positive) are read
	 			double height - Height at which the calculation is made (km). Must be exactly
					100.0 or 300.0: the result is stored only in the HR100km or HR300km slot.
					Any other height is computed but written to the HR100km slot (index 0).

 	 		OUTPUT
	 			here->dip[hr] - Magnetic dip I (radians, positive when the field points
					downward, i.e. in the northern magnetic hemisphere)
	 			here->fH[hr] - Gyrofrequency (MHz), 2.8 x total field in gauss
				where hr = HR100km (height 100 km) or HR300km (height 300 km)

			NOTES
				The scaling radius is RMAG = 6371.2 km, P.1239-4 equation (8), not the path
				geometry's R0 = 6371 km (P.533-14 section 4).

	 		SUBROUTINES
				None
	 
	 	For the following input parameters produce the following outputs for the Fh, gyrofrequency, and I, magnetic dip angle.
	 	
	 	lat_d =   0.732665 radians or (41 + 58/60 + 43/3600) degrees 
	 	long_d =  -1.534227 radians or -(87 + 54/60 + 17/3600) degrees
	 	height = 1800;
 	 
	 	Fh = 0.733686 
	 	I = 1.241669
	 
	 	This routine is based on MAGFIT.FOR in REC533.
	 
	 */

    double P[7][7] = {{1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
    };

    double DP[7][7] = { {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
    };

    double G[7][7] = { {0.000000, 0.304112, 0.024035,-0.031518,-0.041794, 0.016256,-0.019523},
                       {0.000000, 0.021474,-0.051253, 0.062130,-0.045298,-0.034407,-0.004853},
                       {0.000000, 0.000000,-0.013381,-0.024898,-0.021795,-0.019447, 0.003212},
                       {0.000000, 0.000000, 0.000000,-.0064960, 0.007008,-0.000608, 0.021413},
                       {0.000000, 0.000000, 0.000000, 0.000000,-0.002044, 0.002775, 0.001051},
                       {0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000697, 0.000227},
                       {0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.001115}
    };

    double H[7][7] = { {0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000},
                       {0.000000,-0.057989, 0.033124, 0.014870,-0.011825,-0.000796,-0.005758},
                       {0.000000, 0.000000,-0.001579,-0.004075, 0.010006,-0.002000,-0.008735},
                       {0.000000, 0.000000, 0.000000, 0.000210, 0.000430, 0.004597,-0.003406},
                       {0.000000, 0.000000, 0.000000, 0.000000, 0.001385, 0.002421,-0.000118},
                       {0.000000, 0.000000, 0.000000, 0.000000, 0.000000,-0.001218,-0.001116},
                       {0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,-0.000325}
    };

    double CT[7][7] = { {0.0000000,0.0000000,0.33333333,0.266666666,0.25714286,0.25396825,0.25252525},
                        {0.0000000,0.0000000,0.00000000,0.200000000,0.22857142,0.23809523,0.24242424},
                        {0.0000000,0.0000000,0.00000000,0.000000000,0.14285714,0.19047619,0.21212121},
                        {0.0000000,0.0000000,0.00000000,0.000000000,0.00000000,0.11111111,0.16161616},
                        {0.0000000,0.0000000,0.00000000,0.000000000,0.00000000,0.00000000,0.09090909},
                        {0.0000000,0.0000000,0.00000000,0.000000000,0.00000000,0.00000000,0.00000000},
                        {0.0000000,0.0000000,0.00000000,0.000000000,0.00000000,0.00000000,0.00000000}
    };


    // Temps
	double Fx, Fy, Fz, AR;
	double SUMZ, SUMX, SUMY;
    int N, M;

	//Initialise to prevent - potentially uninitialized local variable 'hr' used
	int hr = 0; // Height index

    
/*******************************************************************************************************/
	// P.1239-4 equation (8): R = 6371.2/(6371.2 + hr). The field model's own
	// radius; it used the path geometry's R0.
	AR = RMAG/(RMAG+height);	
	
	Fz=0.0;
	Fx=0.0;
	Fy=0.0;

	for (N = 1; N <= 6; N++) {
	SUMZ=0.0;
	SUMX=0.0;
	SUMY=0.0;

	for (M = 0; M <= N; M++) {
		if(N == M) { 
     		P[M][N]= cos(here->L.lat)*P[M-1][N-1];
      		DP[M][N]=cos(here->L.lat)*DP[M-1][N-1]+sin(here->L.lat)*P[M-1][N-1];
		}
		else if (N != 1) { 
      		P[M][N]= sin(here->L.lat)*P[M][N-1] - CT[M][N]*P[M][N-2];
			DP[M][N]=sin(here->L.lat)*DP[M][N-1]- cos(here->L.lat)*P[M][N-1] - CT[M][N]*DP[M][N-2];
		}
		else {
      		P[M][N]= sin(here->L.lat)*P[M][N-1];
      		DP[M][N]=sin(here->L.lat)*DP[M][N-1]-cos(here->L.lat)*P[M][N-1];
		} 
                
	SUMZ=SUMZ+P[M][N]      *( G[M][N]*cos(M*here->L.lng)+H[M][N]*sin(M*here->L.lng));
	SUMX=SUMX+DP[M][N]     *( G[M][N]*cos(M*here->L.lng)+H[M][N]*sin(M*here->L.lng));
	SUMY=SUMY+M*P[M][N]	   *( G[M][N]*sin(M*here->L.lng)-H[M][N]*cos(M*here->L.lng));
	}

	Fz=Fz+pow(AR,(N+2))*(N+1)*SUMZ;
	Fx=Fx-pow(AR,(N+2))*SUMX;
	Fy=Fy+pow(AR,(N+2))*SUMY;
	}

	// dip and fH can only be calculated for 2 heights in this project
	if(height == 100) hr = HR100km;
	else if(height == 300) hr = HR300km;

	here->dip[hr]=atan(Fz/sqrt(pow(Fx,2) + pow((Fy/cos(here->L.lat)),2)));
	here->fH[hr]=2.8*sqrt(pow(Fx,2)+pow((Fy/cos(here->L.lat)),2)+pow(Fz,2));
  
}
