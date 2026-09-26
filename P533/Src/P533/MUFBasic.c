#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

void MUFBasic(struct PathData *path) {

	/*

	  MUFBasic() Determines the Basic MUF as described in ITU-R P.533-12 Section 3.5 F2-layer basic MUF.
	 		and Section 3.3 E-layer basic MUF. This routine stores the F2 and E mode information to the path structure.
			Verified against P.533-14:
				section 3.5.1.1  equation (2) hr at mid-path; equation (3) with (4)-(6) for the
				                 lowest-order mode, dmax limited to 4000 km
				section 3.5.1.2  D > dmax: the lower of F2(dmax)MUF (equation (3) with d = dmax)
				                 at T + d0/2 and R - d0/2 (Table 1a))
				section 3.5.2.1  higher-order F2 modes, D <= dmax, equation (3) at mid-path, d = D/n
				section 3.5.2.2  higher-order F2 modes, D > dmax, equations (7) and (8), dmax
				                 recalculated (unlimited) at each control point
				section 3.3      E modes, equation (1) nE(D)MUF = foE sec i110, foE at the
				                 Table 1a) control points (the lower one for 2000-4000 km)
				section 3.1      path basic MUF = the higher of the lowest-order E and F2 MUFs
				section 5.2.1    which modes: lowest-order F2 and the next five (NHIGHERF2),
				                 lowest-order E and the next two (NHIGHERE), E only to 4000 km
			Paths longer than 9000 km are skipped (they use section 5.3).

	 		INPUT
	 			struct PathData *path - reads path->distance (km) and
					path->CP[MP] (and CP[T1k], CP[R1k] for D >= 2000 km) from InitializePath()

	 		OUTPUT
	 			path->CP[MP].hr - equation (2) mirror-reflection height (km), min(1490/M(3000)F2 - 176, 500)
	 			path->n0_F2 - index (hops - 1) of the lowest-order F2 mode, or NOLOWESTMODE (99)
	 			path->n0_E - index (hops - 1) of the lowest-order E mode: 0 for D <= 2000 km,
					1 for 2000 < D <= 4000 km; unchanged (NOLOWESTMODE) beyond 4000 km
	 			path->dmax - dmax at mid-path limited to 4000 km (km); only set if an F2 mode exists
	 			path->BMUF - path basic MUF (MHz); TOOBIG (DBL_MAX) if neither an E nor an F2 mode
					exists; not touched (initial 99.9) when D > 9000 km
	 			path->Md_F2[].BMUF - F2 mode basic MUFs (MHz) for n0_F2 .. n0_F2+5; others stay 0.0
	 			path->Md_E[].BMUF - E mode basic MUFs (MHz) for n0_E .. n0_E+2; others stay 0.0
	 			path->Md_E[].hr - 110 km for those E modes
				path->CP[].x - foF2/foE or 2 (set by CalcB() at every control point it is used on)

	 			via CalculateCPParameters()
	 			path->CP[Td02] - The parameters for the T + d0/2 control point are determined.
	 			path->CP[Rd02] - The parameters for the R - d0/2 control point are determined.
				(only when D > dmax)

			NOTES
				- The lowest-order F2 mode is found geometrically: the longest hop is the one
				  reaching the ground at MINELEANGLES (3 deg) elevation from height hr, limited
				  to 4000 km, and n0 is the fewest hops (at most MAXN0F2+1) whose hop is
				  shorter than that. P.533-14 only says n0 "is determined by geometrical
				  considerations"; the 3 deg minimum is this implementation's choice.
				- Owner ruling, section 3.5.2.2: for D > dmax the higher-order mode MUF is
				  formed as F2(dmax)MUF x Mn/Mn0 at each of T + d0/2 and R - d0/2 and the lower
				  of the two products is taken.
				- The lowest-order E mode is 1E up to 2000 km and 2E beyond (the body cites
				  P.1240-2 section 2 for this; P.1240 text not provided, unverified), from
				  "the lowest-order mode with hop length up to 2 000 km" of section 5.2.1.

			SUBROUTINES
				IncidenceAngle()
				ElevationAngle()
				Calcdmax()
				CalcF2DMUF()
				CalcB()
				GreatCirclePoint()
				CalculateCPParameters()
			
	 */

	int n0; // order
	int n; // dummy
	
	double hr; // Mirror reflection height
	double minele; // Min elevation in P533 was 3 degrees in radians
	double dh; // The hop distance
	//double dhmax; // The maximum hop distance at a 500km reflection height
	double aoi; // Angle of incidence
	double n0F2DMUF; // Lowest-order mode basic MUF
	double F2DMUF[2] = {0.0, 0.0}; // F2(dmax)MUF at T + d0/2 and R - d0/2, set and read only for D > dmax
	double Mn0[2]; // MUF Factor of the lowest-order mode
	double Mn[2]; // MUF Factor for higher-order modes
	double fracd; // Fractional distance
	//double psi; // The angle associated with the hop length d, d = R*psi.
	double i110; // The incident angle for a reflection height of 110 km.
	double dmax; // path->dmax for readability
	double delta; // Elevation angle

	// Only do this subroutine if the path is less than or equal to 9000 km if not exit
	if(path->distance > 9000) return; 
	
	// First, determine the F2 layer Basic MUF
	// Determine the mirror reflection height ( hr ) at the midpoint.
	hr = min(1490.0/path->CP[MP].M3kF2 - 176.0, 500.0);

	// Store hr to the control point but use hr in this routine for readability.
	path->CP[MP].hr = hr;

	// The mode is just determined by geometry. I will assume that the earth is a sphere. So find the lowest mode by geometry.
	// The mirror reflection height will tell you if multiple hops are necessary.
	// Determine where the horizon is. If the horizon is less than half the distance you must have multiple hops.
	// Try the other modes with a max of 6.
	// There is a minimum elevation angle is MINELEANGLE degrees for the short model(See p533.h).
	minele = MINELEANGLES*D2R;
	// Find the hop distance to the horizon (dh) 
	// The angle of incidence for the minimum elevation (minele) and mirror reflection height (hr)
	aoi = IncidenceAngle(minele, hr);
	// Now solve for the arc length of the half-hop length and multiply by the earth's radius. 
	// Then to find the total hop length (dh) multiply by 2.0
	dh = (PI - aoi - ((PI/2.0) + minele))*R0*2.0;
	
	// *************************** Not Used ******************************************
	// Determine the maximum hop lenght for the path if the reflection height is 500 km
	//dhmax = (PI - IncidenceAngle(minele, 500.0) - ((PI/2.0) + minele))*R0*2.0;
	//*********************************************************************************
	
	// The hop distance cannot be longer than 4000 km.
	dh = min(dh, 4000.0);
	
	for( n0 = 0; n0 <= MAXN0F2; n0++ ) {
		if(dh > path->distance/(n0+1)) { // Is the mirror reflection height horizon less than the n0 hop distance?
			// At this point lowest-order mode is known – store it.
			path->n0_F2 = n0;
			break; // You have found the lowest-order mode. Jump out of the loop.
		}
    }

    // Check to see if a low order mode exists F2 layers. If it does not skip Sections 3.5.1 and 3.5.2 
	// since they are dependenton the lowest order F2 mode existing
	if(path->n0_F2 != NOLOWESTMODE) {

		// There will be several calculations that follow that require dmax and B calculated at the midpoint.
		// Both will be calculated here for clarity.
		// The calculation of dmax is dependent on the calculation of B, Eqn (6) in 
		// Section 3.5.1.1 Paths up to dmax (km) ITU-R P.533-12.
		// The dmax is determined at mid-path and then stored to the path structure. The path->dmax
		// will be used elsewhere in p533().

		// Limit dmax to 4000 km
		path->dmax = min(Calcdmax(&path->CP[MP]), 4000.0);
	
		// For readability use a local variable.
		dmax = path->dmax;

		// 3.5.1 Lowest-order mode
		if(path->distance <= path->dmax ) { // 3.5.1.1 Paths up to dmax (km)
		
			// For this calculation the hop distance is path->distance/(n0+1). 
			// The path->dmax has already been calculated at the midpoint for the remainder of these calculations.
			n0F2DMUF = CalcF2DMUF(&path->CP[MP], path->distance/(n0+1.0), path->dmax, CalcB(&path->CP[MP])); 

			// The lowest-order F2 mode has been determined. Load it into the path structure
			path->Md_F2[path->n0_F2].BMUF = n0F2DMUF; // lowest-order mode basic MUF

			// The n0F2DMUF is the basic F2 MUF of the path.
			path->BMUF = n0F2DMUF;

			// NOTE: There will be no T + d0/2 and R - d0/2 control points for this path
		}
		else { // 3.5.1.2 Paths longer than dmax (km)
			// In this case we have to load two new control points at T - d sub 0/2 and R - d sub 0/2. 
			// To find these new locations,
			// first determine the fractional distances and then find the point on the great circle between tx and rx.
			fracd = (1.0/(2.0*(n0+1))); // T + d sub 0/2 as a fraction of the total path length
			GreatCirclePoint(path->L_tx, path->L_rx, &(path->CP[Td02]), path->distance, fracd);
			fracd = (1.0 - (1.0/(2.0*(n0+1)))); // R - d sub 0/2 as a fraction of the total path length
			GreatCirclePoint(path->L_tx, path->L_rx, &(path->CP[Rd02]), path->distance, fracd);
			// All distances for the control points are relative to the tx.
	
			// Find foF2, M(3000)F2 and foE these control points.
			CalculateCPParameters(path, &path->CP[Td02]);
			CalculateCPParameters(path, &path->CP[Rd02]);

			// Determine the F2 basic MUF at each control point
			// P.533-14 section 3.5.1.2: "the lower of the F2(dmax)MUF values
			// determined from equation (3) for the two control points", i.e.
			// equation (3) with d = dmax. This passed the hop length D/(n0+1)
			// instead, which is shorter than dmax whenever n0 > 0 and gave a
			// path basic MUF 2-10 % low for every path beyond dmax.
			F2DMUF[0] = CalcF2DMUF(&path->CP[Td02], dmax, dmax, CalcB(&path->CP[Td02]));
			F2DMUF[1] = CalcF2DMUF(&path->CP[Rd02], dmax, dmax, CalcB(&path->CP[Rd02]));

			path->Md_F2[n0].BMUF = min(F2DMUF[0], F2DMUF[1]); // Basic MUF is the lower of the two control point MUFs.

			// The path MUF is small of the two MUFs calculated at the control points T + d0/2 and R - d0/2 
			path->BMUF = path->Md_F2[n0].BMUF;

		}

        // 3.5.2 Higher-order modes (paths up to 9 000 km)
		if(path->distance <= 9000) {
			for(n = n0+1; (n < MAXF2MDS) && (n <= n0+NHIGHERF2); n++) { // the next five higher-order modes, section 5.2.1
				if(path->distance <= path->dmax) { // 3.5.2.1 Paths up to dmax (km)
					path->Md_F2[n].BMUF = CalcF2DMUF(&path->CP[MP], path->distance/(n+1.0), dmax, CalcB(&path->CP[MP]));
				}
				else { //3.5.2.2 Paths longer than dmax (km)
					// The higher-order modes calculated here imply that the lowest-order mode has already set up the 
					// two new control points and the basic MUF has been determined. The path->BMUF is then n0F2(D)MUF.
					// Now find the mode basic MUF as a function of F2(dmax[CP[MP]])MUF and a scaling factor. 
					// The scaling factor is 
					//     Mn/Mn0 = nF2(D)MUF/n0F2(D)MUF
					//        where d is the hop length of the mode and D is the total distance.
					//**************************************************************
					// Note: For this calculation dmax is allowed to exceed 4000 km
					//**************************************************************

					Mn0[0] = CalcF2DMUF(&path->CP[Td02], path->distance/(n0+1.0), Calcdmax(&path->CP[Td02]), CalcB(&path->CP[Td02])); 
					Mn0[1] = CalcF2DMUF(&path->CP[Rd02], path->distance/(n0+1.0), Calcdmax(&path->CP[Rd02]), CalcB(&path->CP[Rd02]));

					Mn[0] = CalcF2DMUF(&path->CP[Td02], path->distance/(n+1.0), Calcdmax(&path->CP[Td02]), CalcB(&path->CP[Td02])); 
					Mn[1] = CalcF2DMUF(&path->CP[Rd02], path->distance/(n+1.0), Calcdmax(&path->CP[Rd02]), CalcB(&path->CP[Rd02]));
				
					// Equation (7), nF2(D)MUF = n0F2(dmax)MUF x Mn/Mn0: "The lower of
					// the values calculated at the two control points of Table 1a) is
					// selected." The product is formed at each control point and the
					// lower taken. This multiplied the lower F2(dmax)MUF (path->BMUF)
					// by the lower ratio, which can come from the other control point.
					path->Md_F2[n].BMUF = min(F2DMUF[0]*Mn[0]/Mn0[0], F2DMUF[1]*Mn[1]/Mn0[1]);
			
				}
            }
        }
    }
    // End calculation for F2 Layer Basic MUF

	// E layer basic MUF calculation
	// P.533-14 section 5.2.1: E modes "for paths up to 4 000 km", and section 4
	// screens F2 modes "for paths up to 4 000 km"; the rest of this block and
	// ELayerScreeningFrequency() already include 4000 km. This test excluded it.
	if(path->distance <= 4000.0) {

		// Determine the lowest order E mode
		hr = 110.0; // The mirror reflection height is 110 km for the E layer basic MUF calculation

		// P.533-14 section 5.2.1 considers "E modes - the lowest-order mode with hop
		// length up to 2 000 km, and the next two higher-order modes", and P.1240-2
		// section 2 lists 1E for 0-2 000 km and 2E for 2 000-4 000 km. The lowest-order
		// E mode is therefore one hop up to 2000 km and two beyond. This found it from
		// a 3 degree minimum elevation angle instead, which limits one hop to 1776 km,
		// so paths of 1776-2000 km took 2E (a basic MUF about 27 % low) and paths of
		// 3552-4000 km took 3E.
		n0 = (path->distance <= 2000.0) ? 0 : 1;
		path->n0_E = n0;

        // Is there a lowest order E mode?
		if(path->n0_E != NOLOWESTMODE) {
			// The lowest-order E mode and the next two, section 5.2.1.
			for(n=n0; (n<MAXEMDS) && (n <= n0+NHIGHERE); n++) {
				// Save the reflection height, although for E layers it is always 110.0 km.
				path->Md_E[n].hr = hr;
				// Find the hop length for this mode.
				dh = min(path->distance/(n+1), 4000.0);
				// The angle of incidence for hr = 110 km.
				// First, find the angle associated with the half-hop distance, psi.
				//psi = dh/(2.0*R0);
				// Find the elevation angle, delta.
				delta = ElevationAngle(dh, hr);
				// The incident angle i110 is
				i110 = IncidenceAngle(delta, hr);
				// The control point is the midpoint
				if(path->distance < 2000.0) {
					path->Md_E[n].BMUF = path->CP[MP].foE/cos(i110);
				}
				else if(path->distance >= 2000.0 && path->distance <= 4000.0) {
					path->Md_E[n].BMUF = min(path->CP[R1k].foE, path->CP[T1k].foE)/cos(i110);
				}
				else { // path->distance > 4000.0
					path->Md_E[n].BMUF = 0.0; // The basic MUF is zero past 4000 km. 
					// This is redundant but to make the point set the basic E layer MUF to 0.0.
				}

                // Determine the lowest-order E layer mode.
				if((path->Md_E[n].BMUF != 0.0) && (path->n0_E == NOLOWESTMODE)) {
					path->n0_E = n;
				}
            }
		}
		else { // path->n0_E == NOLOWESTMODE
			// No E mode reaches this distance within the minimum elevation angle.
		}
    }
    // End calculation for the E layer Basic MUF.

	// Finally, determine the path basic MUF.
	if(path->n0_E != NOLOWESTMODE) {
		if(path->n0_F2 != NOLOWESTMODE) { // F2 and E modes exist
			path->BMUF = max(path->Md_E[path->n0_E].BMUF, path->Md_F2[path->n0_F2].BMUF);
		}
		else { // E modes exist. F2 mode do not.
			path->BMUF = path->Md_E[path->n0_E].BMUF;
		}
    }
	else { // path->n0_E == NOLOWESTMODE
		if(path->n0_F2 != NOLOWESTMODE) { // F2 modes exist. E modes do not.
			path->BMUF = path->Md_F2[path->n0_F2].BMUF;
		}
		else {
			// This is an error condition
			path->BMUF = TOOBIG; 
		}
    }

    return;

}


/*
  The following four programs perform the calculation necessary to determine the F2(d)MUF from Section 3.5.1.1 Paths up to dmax (km)
  in ITU-R P.533-12. These programs execute eqn (3) thru (6). They are separated due to necessity. Throughout p533(), differing other 
  procedures use these calculations in various ways. By separating the calculation of F2(d)MUF in this way, dmax and F2(D)MUF can be 
  explicitly determined. A single routine complicates these calculations unnecessarily. 
  The table below shows how these routines relate to each other and the targeted equation in P.533-12.
 			
 	P.533-12 Equation	Routine			Dependencies
 		(6)				CalcB()			
 		(5)				Calcdmax()		(6)
  		(4)				CalcCd()		(5)(6)
 		(3)				CalcF2DMUF()	(4)(5)(6)

  The equation numbers are unchanged in P.533-14 section 3.5.1.1 (verified).

 */

double Calcdmax(struct ControlPt *CP) {

	/*

	  Calcdmax() Finds the dmax for the path. The dmax can be calculated to be greater than 4000 km.
			Presently, June 2013, dmax can be greater than 4000 km for the calculation of higher order MUFs.
			all other locations it will typically be restricted to 4000 km. 
	 
			P.533-14 section 3.5.1.1 equation (5):
			dmax = 4780 + (12610 + 2140/x^2 - 49720/x^4 + 688900/x^6)(1/B - 0.303),
			with B from equation (6) and x = max(foF2/foE, 2).

	 		INPUT
	 			struct ControlPt *CP - Control point of interest (reads foF2, foE, M3kF2)

	 		OUTPUT
	 			returns result of Eqn (5) P.533-12 dmax (km), NOT limited to 4000 km: callers
				apply the limit where section 3.5.1.1 requires it
				CP->x is set as a side effect (via CalcB())

			SUBROUTINES
				CalcB()
	 
	 */

	double B;
	double dmax;
	
	// Now you can find B
	B = CalcB(CP);

	// Determine d sub max
	dmax = 4780.0 + (12610.0 + (2140.0/pow(CP->x, 2.0)) - (49720.0/pow(CP->x, 4.0)) + (688900.0/pow(CP->x, 6.0)))*((1.0/B) - 0.303);
	
	return dmax;
	
}

double CalcB(struct ControlPt *CP) {

	/*
		CalcB() - Determines the intermediate values B from Eqn (6) P.533-12
			P.533-14 section 3.5.1.1 equation (6):
			B = M(3000)F2 - 0.124 + ([M(3000)F2]^2 - 4)(0.0215 + 0.005 sin(7.854/x - 1.9635)),
			x = foF2/foE or 2, whichever is the larger. The sine argument is taken in radians.

	 		INPUT
	 			struct ControlPt *CP - Control point of interest (reads foF2, foE (MHz), M3kF2)

	 		OUTPUT
	 			returns the intermediate value B (dimensionless)
				CP->x - set to x; if CP->foE is 0 (control point not computed) x = 2
	 
			SUBROUTINES
				None

	 */

	double B;

	// Determine x which is the ration of foF2 to foE at the midpoint. 
	if(CP->foE != 0.0) { // Check to see if foE exists
		CP->x = max(CP->foF2/CP->foE, 2.0);
	}
	else { // path->CP[MP].foE == 0.0
		CP->x = 2.0;
	}

    // Now you can find B
	B = CP->M3kF2 - 0.124 + (pow(CP->M3kF2, 2) - 4)*(0.0215 + 0.005*sin((7.854/CP->x) - 1.9635));
	
	return B;

}

double CalcCd(double d, double dmax) {

	/*

	 	CalcCd() - Performs Eqn (4) P.533-12
			P.533-14 section 3.5.1.1 equation (4):
			Cd = 0.74 - 0.591 Z - 0.424 Z^2 - 0.090 Z^3 + 0.088 Z^4 + 0.181 Z^5 + 0.096 Z^6,
			Z = 1 - 2d/dmax.

	 		INPUT
	 			double d - hop distance (km)
	 			double dmax - maximum hop distance (km), non-zero

	 		OUTPUT
	 			returns the result of Eqn (4) (dimensionless)

			SUBROUTINES
				None
	 
	 */

	double Z;
	double Cd;

	Z = 1.0 - 2.0*d/dmax;

	Cd = 0.74 - 0.591*Z - 0.424*pow(Z,2) - 0.090*pow(Z,3) + 0.088*pow(Z,4) + 0.181*pow(Z,5) + 0.096*pow(Z,6);

	return Cd;
}

double CalcF2DMUF(struct ControlPt *CP, double distance, double dmax, double B) {

	/*

		CalcF2DMUF() - Determines the F2 Layer MUF from Eqn (3) P.533-12
			P.533-14 section 3.5.1.1 equation (3):
			nF2(D)MUF = (1 + (Cd/C3000)(B - 1)) foF2 + (fH/2)(1 - d/dmax),
			Cd from equation (4) at d, C3000 = Cd at d = 3000 km, fH at 300 km.

	 		INPUT
	 			struct ControlPt *CP - Control point of interest (reads foF2 (MHz), fH[HR300km] (MHz))
	 			double distance - Hop distance d (km)
	 			double dmax - Maximum hop distance (km), as the caller wants it (limited to
					4000 km or not)
	 			double B - Intermediate value B (equation (6), CalcB())

	 		OUTPUT
	 			return the F2 layer MUF (MHz)

			NOTES
				If distance > dmax, Cd is evaluated at dmax, but the gyrofrequency term still
				uses distance/dmax and so becomes negative. Section 3.5.1.2 callers pass
				distance = dmax; in section 3.5.2.2 the Mn/Mn0 hops D/n are compared with the
				recalculated (unlimited) dmax.
	 
	 		SUBROUTINES
				CalcCd()

	 */

	double d;	// Temp
	double C3k; // Cd at 3000 km
	double Cd;	// Cd at D
	double F2DMUF;

	// If the distance is less than dmax use the distance
	if(distance <= dmax) {
		d = distance;
	}
	else {
		d = dmax;
	}

    // ITU-R P.533-12 Eqn (4)
	Cd = CalcCd(d, dmax);

	d = 3000.0; // From ITU-R P.533-12 "C sub 3000 : value of Cd for D = 3 000 km" 

	C3k = CalcCd(d, dmax);

	F2DMUF = (1.0 + (Cd/C3k)*(B - 1.0))*CP->foF2 + (CP->fH[HR300km]/2.0)*(1.0 - (distance/dmax));

	return F2DMUF;
 
}
