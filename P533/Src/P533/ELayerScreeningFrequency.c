#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

// Local prototypes
double MirrorReflectionHeight(struct PathData path, struct ControlPt CP, double dh);
// End local prototypes

void ELayerScreeningFrequency(struct PathData *path) {

	/*

	 	ELayerScreeningFrequency() Calculates the E-Layer screening frequency for the F2 modes
	 		from Section 4 E-layer maximum screening frequency (fs) ITU-R P533-11.
			Verified against P.533-14 section 4, equations (11) fs = 1.05 foE sec i and (12),
			with the foE of Table 1b): the mid-path foE for D <= 2000 km, otherwise the higher
			of the foE at T + 1000 km and R - 1000 km. It also sets the F2 mirror-reflection
			height hr of every F2 mode (P.533-14 section 5.1, equations (14)-(16), at the
			control points of Table 1c): mid-path for D <= dmax, otherwise the mean over
			T + d0/2, M and R - d0/2).

	 		INPUT
	 			struct PathData *path - reads path->distance (km), path->dmax (km),
					path->n0_F2, path->CP[MP, T1k, R1k, Td02, Rd02] and, through
					MirrorReflectionHeight(), path->frequency and path->SSN

	 		OUTPUT
	 			path->Md_F2[k].hr - Reflection height (km), for k = n0_F2 .. MAXF2MDS-1, D <= 9000 km
	 			path->Md_F2[k].fs - E layer screening frequency (MHz), only for D <= 4000 km;
					left at its initial 0.0 beyond 4000 km

			NOTES
				- hr is set for every index from n0_F2 up, including modes that have no basic
				  MUF; it is needed up to 9000 km for the mode delays of equation (47).
				- For D <= dmax the hr of each mode uses its own hop d = D/n, as equation (16)
				  depends on d.
				- Must run after MUFBasic() (n0_F2, dmax and the Td02/Rd02 control points).
				  If n0_F2 is NOLOWESTMODE (99) the loop does nothing.

			SUBROUTINES
				MirrorReflectionHeight()
				ElevationAngle()
				IncidenceAngle()
	 
	 */

	double dh;		// Hop distance
	double deltaf;	// Elevation angle calculated for F2 layer
	double foE;		// Critical frequency E layer
	double i;		// Angle of incidence

	int k;			// Temp
	
	// E layer screening is restricted to paths no longer than 4000 km, but the
	// mirror-reflection heights of section 5.1 are needed for every F2 mode up to
	// 9000 km: equation (47) takes the mode delay from p' "and the reflection
	// height, hr, determined as in section 5.1". This returned before setting hr
	// beyond 4000 km, so every F2 mode there had hr = 0 and its delay was the
	// ground distance over c -- the same for all modes (14.67 ms at 4399 km).

	// Determine the foE for this calculation.
	if(path->distance <= 2000.0) {
		foE = path->CP[MP].foE;
	}
	else {
		foE = max(path->CP[T1k].foE, path->CP[R1k].foE);
	}

    // Now find the E-Layer screening for the F2 modes that exist.
	// The hop of the F2 mode is related to the index i. Since i is an C index use the hop number n = k + 1; 
	for(k=path->n0_F2; k<MAXF2MDS; k++) {
		// Determine the hop distance
		dh = path->distance/(k+1);

		if(path->distance <= path->dmax) {
			path->Md_F2[k].hr = MirrorReflectionHeight(*path, path->CP[MP], dh);

		}
		else if(path->distance > path->dmax){
			// In this case you have to find the mirror reflection height at all the control points and take the mean.
			// (P.533-14 section 5.1 last paragraph and Table 1c).) The hop distance used is the
			// mode's own dh = D/n, not dmax.
			path->Md_F2[k].hr = (MirrorReflectionHeight(*path, path->CP[Td02], dh) +
					            MirrorReflectionHeight(*path, path->CP[MP], dh) +
					            MirrorReflectionHeight(*path, path->CP[Rd02], dh))/3.0;
		}

        if(path->distance > 4000) continue; // no E-layer screening beyond 4000 km

		// Find the elevation angle from equation 13 Section 5.1 Elevation angle.
		// ITU-R P.533-12 (same equation (13) in P.533-14 section 5.1)
		deltaf = ElevationAngle(dh, path->Md_F2[k].hr);

		// angle of incidence at height hr = 110 km
		i = IncidenceAngle(deltaf, 110.0);
		
		// Now calculate the E layer screening frequency.
		if(path->distance <= 2000) {
			path->Md_F2[k].fs = (1.05 * foE)/cos(i);
		}
		else { // (path->distance > 2000)
			// Use the larger of the foE at the control points 1000 km from either end.
			path->Md_F2[k].fs = (1.05 * foE)/cos(i);
		}
    } // End for(k=path->n0_F2; k<MAXF2MODES; k++)

	return;

}

double MirrorReflectionHeight(struct PathData path, struct ControlPt CP, double d) {

	/*

	 	MirrorReflectionHeight() - Calculates the mirror reflection height by the method
	 		in ITU-R P.533-12 Section 5.1 "Elevation angle".
			Verified against P.533-14 section 5.1: x = foF2/foE, y = max(x, 1.8),
			deltaM = 0.18/(y - 1.4) + 0.096 (R12 - 25)/150, H = 1490/(M(3000)F2 + deltaM) - 316,
			xr = f/foF2, and
				a) x > 3.33 and xr >= 1: equation (14) (A1, B1, E1, F1, G, ds, a)
				b) x > 3.33 and xr < 1:  equation (15) (A2, B2, E2, F2, Z, df, b)
				c) x <= 3.33:            equation (16) (J, U)
			each limited to 800 km.

	 		INPUT
	 			struct PathData path - reads path.frequency (MHz) and path.SSN (R12, not limited)
	 			struct ControlPt CP - The control point of interest (CP.foF2, CP.foE in MHz,
					CP.M3kF2)
	 			double d - The hop length (km)

	 		OUTPUT
	 			returns the mirror reflection height hr (km), at most 800 km

			NOTES
				CP.foE must be non-zero (x = foF2/foE is not guarded).
	 
			SUBROUTINES
				None

	 */

	//Initialise to prevent - warning C4701: potentially uninitialized local variable 'hr' used
	double hr = 0; // Mirror reflection height

	// Temps
	double a, b, h, x, xr, y;
	double H, Z, J, U, G;
	double A1, A2, B1, B2, E1, E2, F1, F2;
	double deltaM;
	double ds, df;

	// Determine the critical frequency ratio
	x = CP.foF2/CP.foE;

	y = max(x, 1.8);

	// Delta M of equation (14) takes R12 as it is: P.533-14 section 3.4 limits R12
	// to 160 "in the case of foF2 only" (P.1240-2 Annex 2 gives the same unlimited
	// form). This clamped it to 160.
	deltaM = (0.18/(y - 1.4))+(0.096*(path.SSN - 25.0)/(150.0));

	xr = path.frequency/CP.foF2;

	H = (1490.0/(CP.M3kF2 + deltaM)) - 316.0;

	if((x > 3.33) && (xr >= 1.0)) { // a)
		E1 = -0.09707*pow(xr, 3) + 0.6870*xr*xr - 0.7506*xr + 0.6;

		if(xr <= 1.71) {
			F1 = -1.862*pow(xr, 4) + 12.95*pow(xr, 3) - 32.03*xr*xr + 33.50*xr - 10.91;
		}
		else { // (xr > 1.71)
			F1 = 1.21 + 0.2*xr;
		}

        // P.533-14 equation (14). This lacked the 90.47 xr term, so G fell
		// to about -315 at xr = 3.7 instead of meeting the constant 19.25
		// used beyond it, and the skip distance ds was wrong throughout.
		if(xr <= 3.7) {
			G = -2.102*pow(xr, 4) + 19.50*pow(xr, 3) - 63.15*xr*xr + 90.47*xr - 44.73;
		}
		else {
			G = 19.25;
		}

        ds = 160.0 + (H + 43.0)*G;
		a = (d - ds)/(H + 140.0);
		A1 = 140.0 + (H - 47.0)*E1;
		B1 = 150.0 + (H - 17.0)*F1 - A1;

		if((B1 >= 0.0) && (a >= 0.0)) {
			h = A1 + B1*pow(2.4, -a);
		}
		else {
			h = A1 + B1;
		}

        hr = min(h, 800.0);
	}
	else if ((x > 3.33) && (xr < 1.0)) { // b)
		Z = max(xr, 0.1);
		E2 = 0.1906*Z*Z + 0.00583*Z + 0.1936;
		A2 = 151.0 + (H - 47.0)*E2;
		F2 = 0.645*Z*Z + 0.883*Z + 0.162;
		B2 = 141.0 + (H - 24.0)*F2 - A2;
		df = min(0.115*d/(Z*(H + 140.0)), 0.65);
		b = -7.535*pow(df, 4) + 15.75*pow(df, 3) - 8.834*df*df - 0.378*df +1.0;

		if(B2 >= 0.0) {
			h = A2 + B2*b;
		}
		else {
			h = A2 + B2;
		}

        hr = min(h, 800.0);

	}
	else if(x <= 3.33) { // c

		J = -0.7126*pow(y, 3) + 5.863*y*y - 16.13*y + 16.07;
		U = 8.0E-5*(H - 80.0)*(1.0 + 11.0*pow(y, -2.2)) + 1.2E-3*H*pow(y, -3.6);
		hr = min((115.0 + H*J + U*d),800.0);

	}

    return hr;
}

double ElevationAngle(double dh, double hr) {

	/*

	 	ElevationAngle() - Determines the elevation angle from P.533-12 equation (13) Section 5.1 Elevation angle
	 		given the hop distance (dh) and the mirror reflection height (hr)
			(Verified: P.533-14 section 5.1, equation (13),
			 Delta = arctan(cot(d/2R0) - (R0/(R0 + hr)) cosec(d/2R0)), R0 = 6371 km.)

	 		INPUT
	 			double dh - Hop distance (km), > 0
	 			double hr - Reflection height (km)

	 		OUTPUT
	 			returns the elevation angle (radians). It is not limited: it goes negative when
				the hop is beyond the geometric horizon for hr, and a minimum elevation angle
				(e.g. MINELEANGLES) is the caller's business.
	 
	 		SUBROUTINES
				None

	 */

	double ele;

	ele = ((1.0/tan(dh/(2.0*R0))) - ((R0/(R0+hr))/sin((dh/(2.0*R0)))));
	ele = atan(ele);
	
	return ele;

}

double IncidenceAngle(double deltaf, double hr) {

	/*

	 	IncidenceAngle() Determine the angle of incidence from  P.533-12 equation (12).
	 		Section 4 E-layer maximum screening frequency (fs) given the
	 		mirror reflection height (hr) and the elevation angle (deltaf)
			(Verified: P.533-14 section 4, equation (12), i = arcsin(R0 cos(Delta_F)/(R0 + hr)),
			 R0 = 6371 km. Also used for i110 of equation (1) in MUFBasic() and there, with the
			 minimum elevation angle, to find the lowest-order F2 mode.)

	 		INPUT
	 			double deltaf - Elevation angle (radians)
	 			double hr - Height at which the angle of incidence is wanted (km); 110 km
					for equations (1) and (12)

	 		OUTPUT
	 			returns the incidence angle (radians)
	 
			SUBROUTINES
				None

	 */

	return asin(R0*cos(deltaf)/(R0 + hr));
}
