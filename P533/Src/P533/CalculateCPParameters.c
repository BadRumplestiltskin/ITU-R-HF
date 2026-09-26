#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

void CalculateCPParameters( struct PathData *path, struct ControlPt *here) {

	/*
	 
	  CalculateCPParameters() finds the ionosheric parameters foF2 and M3kF2 for the high (SSN = 100) and low (SSN = 0), the gyrofrequency and magnetic dip at 100 and 300 km
	 		and the solar parameters for the control point. The ionospheric parameters are determined by the bi-linear interpolation method in P.1144-3 (2001)
	 		and linear interpolation by the desired SSN (Found in path->SSN - the user selected SSN) the routine determines the foF2 and M3F2 at the point of interest.
	 		This routine further determines foE at the point of interest.
			It supplies the control-point quantities of P.533-14 section 3.2 (foE "as defined in
			Recommendation ITU-R P.1239") and section 3.4 (foF2 and M(3000)F2 from the P.1239
			numerical maps, magnetic field evaluated at 300 km), for the control points of Table 1.
			The four subroutines are called in this order: IonosphericParameters(),
			SolarParameters(), FindfoE() (which needs the solar zenith angle and declination),
			magfit() at 100 km and at 300 km.

	 		INPUT
	 			struct PathData *path - reads path->foF2 and path->M3kF2 (the month's maps),
					path->hour (hour index 0-23, used both as the map hour and as the UTC
					hour), path->month (0-11) and path->SSN (R12)
	 			struct ControlPt *here - This is a pointer to the control point of interest.
					here->L (lat, lng in radians) must already be set (GreatCirclePoint()).

	 		OUTPUT
	 			There are four subroutines in this program that calculate the parameters for
	 			the control point. Which subroutine calulates which parameter is summarized below

	 			via IonosphericParameters()
	 				here->foF2 - Critical frequency of the F2 layer (MHz)
	 				here->M3kF2 - M(3000)F2, the propagation factor MUF(3000)F2/foF2 (dimensionless)
	 			via FindFoE()
	 				here->foE - Critical frequency fo the E layer (MHz)
	 			via SolarParameters()
	 				here->ltime - The UTC hour (not local time, despite the field name)
	 			via magfit()
	 				here->dip[2] - Magnetic dip calculated at 100 and 300 km (radians)
	 				here->fH[2] - Gyrofreqency calculated at 100 and 300 km (MHz)
	 			via SolarParameters() 
	 				here->Sun.ha - Hour angle (radians)
	 				here->Sun.sha -  Sunrise/Sunset hour angle (radians) for a 90.833 deg zenith angle; PI = Sun never sets, 0 = never rises
	 				here->Sun.sza - Solar zenith angle (radians)
	  				here->Sun.decl - Solar declination (radians)
	  				here->Sun.eot- Equation of time (minutes)
	 				here->Sun.lsr - local sunrise (hours UTC, 0-24)
	 				here->Sun.lsn - local solar noon (hours UTC, 0-24)
	 				here->Sun.lss - local sunset (hours UTC, 0-24)
	 
	 		SUBROUTINES
				IonosphericParameters()
				SolarParameters()
				FindfoE()
				magfit()

	 */

	/*
	 * Find the ionospheric parameters foF2 and M3kF2 at the control point here.
	 * If here is not on a grid point then use bilinear interpolation.
	 */
	IonosphericParameters(here, path->foF2, path->M3kF2, path->hour, path->SSN);

	/*
	 * Calculate the solar parameters. 
	 * These parametres are used in the MedianSkywaveFieldStrengthLong() calculation. Because this 
	 * routine determines the control point parameters for all methods, find the solar parameters now
	 * before entering the conditional loop for the foE calculation.
	 */
	// Find the solar parameters for the control point.
	SolarParameters(here, path->month, (double)path->hour);
		
	/*
	 * Calculate foE by the method outlined in P.1239-2 (now P.1239-4 section 4, equations (12)-(18)).
	 */
	FindfoE(here, path->month, path->hour, path->SSN);

	/* 
	 * At each control point the gyrofrequency and magnetic dip must also be calculated.
	 * The calculation is done at two heights: 
	 *		height = 300 km is for the determination of MUF and the long path (> 9000 km).
     *      height = 100 km is used in the determination of absorption on the int paths (< 9000 km).
	 */

	magfit(here, 100.00);
	magfit(here, 300.00);

	return;
}

void IonosphericParameters(struct ControlPt *here, float ****foF2, float ****M3kF2, int hour, int SSN){

	/*
	 
	  IonosphericParameters() - Finds foF2 and M(3000)F2 by Bilinear Interpolation.
			The monthly median foF2 and M(3000)F2 are read from the P.1239 grid-point maps
			(1.5 degree grid, R12 = 0 and 100), bilinearly interpolated to the control point
			(P.1239-4 section 3.1 refers to P.1144 Annex 1 for this; P.1144 text not provided)
			and then linearly interpolated or extrapolated in R12 (P.533-14 section 3.4,
			P.1239-4 section 3.1).

	  	INPUTS
	 		struct ControlPt *here - here->L (lat, lng in radians) is read
	  		float ****foF2 - foF2 map, indexed [hour][lng index 0-240][lat index 0-120][R12 index 0/1]
	 		float ****M3kF2 - M(3000)F2 map, same indexing
	 		int hour - Hour index 0-23 into the maps (path->hour)
	 		int SSN - R12, 12-month smoothed sunspot number

	 	OUTPUT
	 		here->foF2 - foF2 (MHz)
	 		here->M3KF2 - M(3000)F2 (dimensionless)

		NOTES
			R12 is limited to MAXSSN = 160 for both foF2 and M(3000)F2. P.533-14 section 3.4
			states the limit "in the case of foF2 only", but P.1239-4 section 3.1 also takes
			M(3000)F2 "to be the value obtained for R12 = 160" above 160, which is what the
			code does. Grid indices are clamped to the map, so points at the edges (poles,
			180 degrees) use the edge row or column.

		SUBROUTINES
			BilinearInterpolation()

 		I am indebted to Peter Suessman for the use of his program, iongrid ver 1.80, which was used extensively
 		to verify the method in this routine. 

	*/
	 	
	struct Neighbor {
		struct Location L; // the physical location of the point
		double foF2[2], M3kF2[2]; // the values for the ionospheric parameters at the point
		int j, k; // the j (lng) and k (lat) indexes into the gridpoint map. 
	} UL, UR, LR, LL; // UL = upper left, UR = upper right, LR = lower right and LL = lower left
	
	struct Neighbor ned; //  
    
	// Temporary Varibles
	int m; // Temp the SSN index
	int n; // Temp hours index

	
	
	int		lng, lat;	// gridmap maxima

	// For the gridpoint maps at an increment of 1.5 degrees in lat and long
	double inc;			// Increment for the gridpoint maps (1.5 * pi) / 180 = 0.0261799388
	double fracj, frack;// Fractional "column" j and fractional "row" k

	// This routine is dependent on the 1.5 degree increment
	inc = 1.5*D2R;

	lng = 241;	// 241 Longitudes at 1.5 degree increments
	lat = 121;	// 121 lngitudes at 1.5 degree increments
	 
	/*
	 * Find the neighborhood around the point of interest.
	 * In the foF2 and M3kF2 arrays the 0,0 point is the south-west corner (90 S, 180 W) and the
	 * indices increase north (k) and east (j) in 1.5 degree steps. The lower-left neighbour is the
	 * grid point at or below and west of the point, found with floor(), and the fractional row and
	 * column are measured from it, as BilinearInterpolation() and P.1144 require.
	 *
	 * This used (int) truncation in four quadrant branches. Truncation rounds toward zero, so in the
	 * southern and western hemispheres the grid line it found was the nearer-to-zero one -- the upper
	 * or right neighbour -- while the fraction was still applied as if measured from the lower or left
	 * one: the point was mirrored within its cell (the MATLAB port's D27). foF2 and M(3000)F2 were
	 * wrong by up to the change across one 1.5 degree cell there; the NE quadrant was unaffected.
	 */
	{
		double jf = (here->L.lng + PI)/inc;		// fractional longitude index
		double kf = (here->L.lat + PI/2.0)/inc;	// fractional latitude index
		int j0 = (int)floor(jf), k0 = (int)floor(kf);
		if(j0 < 0) j0 = 0;
		if(j0 > lng-1) j0 = lng-1;
		if(k0 < 0) k0 = 0;
		if(k0 > lat-1) k0 = lat-1;
		fracj = min(max(jf - j0, 0.0), 1.0);
		frack = min(max(kf - k0, 0.0), 1.0);
		LL.j = j0;							LL.k = k0;
		LR.j = (j0 < lng-1) ? j0+1 : j0;	LR.k = k0;
		UL.j = j0;							UL.k = (k0 < lat-1) ? k0+1 : k0;
		UR.j = LR.j;						UR.k = UL.k;
	}

    // Determine the lat and lng in degrees
	UL.L.lat = UL.k*inc - PI/2.0;
	UL.L.lng = UL.j*inc - PI;
	LL.L.lat = LL.k*inc - PI/2.0;
	LL.L.lng = LL.j*inc - PI;
	UR.L.lat = UR.k*inc - PI/2.0;
	UR.L.lng = UR.j*inc - PI;
	LR.L.lat = LR.k*inc - PI/2.0;
	LR.L.lng = LR.j*inc - PI;

	/*
	 * At this point you have the neighborhood around the point now you can populate the foF2 and
	 * M3kF2 for each of the adjacent points
	 */
	n = hour;

	for(m=0; m<2; m++) { // SSN
		// Upper Left
		UL.foF2[m] = foF2[n][UL.j][UL.k][m];
		UL.M3kF2[m] = M3kF2[n][UL.j][UL.k][m];
		// Upper Right
		UR.foF2[m] = foF2[n][UR.j][UR.k][m];
		UR.M3kF2[m] = M3kF2[n][UR.j][UR.k][m];
		// Lower Left
		LL.foF2[m] = foF2[n][LL.j][LL.k][m];
		LL.M3kF2[m] = M3kF2[n][LL.j][LL.k][m];
		// Lower Right
		LR.foF2[m] = foF2[n][LR.j][LR.k][m];
		LR.M3kF2[m] = M3kF2[n][LR.j][LR.k][m];
	}

    // Now you are ready to interpolate the value at the point of interest
	// determine the fractional "column" j and fractional "row" k for the bilinear interpolation calculation
	// frack and fracj, the fractional row and column from the lower-left neighbour, were set above.
	for(m=0; m<2; m++) {
		ned.foF2[m] = BilinearInterpolation(LL.foF2[m], LR.foF2[m], UL.foF2[m], UR.foF2[m], frack, fracj);
		ned.M3kF2[m] = BilinearInterpolation(LL.M3kF2[m], LR.M3kF2[m], UL.M3kF2[m], UR.M3kF2[m], frack, fracj);
	}

    // End of calculation for foF2 and M3kF2

	/*
	 * Now interpolate by the SSN. Note the SSN maximum has been restricted to a maximm of 160 ITU-R P.533-12 (P.533-14 section 3.4 for foF2; P.1239-4 section 3.1 for M(3000)F2 too).
	 * "For most purposes it is adequate to assume a linear relationship with R12 for both foF2 and M(3000)F2." 
	 * ITU-R P.1239-2 (10-2009); the same sentence is in P.1239-4 section 3.1
	 * Note the index on foF2 and M3kF2 in the neighbor structure is for the SSN = 0 (index = 0) and SSN = 100 (index = 1)
	 */
	SSN = min(SSN, MAXSSN);
	here->foF2 = (ned.foF2[1]*SSN + ned.foF2[0]*(100.0 - SSN))/100.0;
	here->M3kF2 = (ned.M3kF2[1]*SSN + ned.M3kF2[0]*(100.0 - SSN))/100.0;

	// End of calculation for foF2 and M3kF2

	return;

}


void FindfoE(struct ControlPt *here, int month, int hour, int SSN) {

	/* 
	 
	  FindFoE() - Determines the critical frequency of the E-layer, foE, by the method in
	  		ITU-R P.1239. This routine assumes that the solar parameters have been calculated
	 		for the control point before execution.
			Implements P.1239-4 section 4, equations (12) (foE^4 = A B C D), (13) A,
			(14) B, (15a)/(15b) the exponent m, (16a)-(16c) C, (17a)-(17e) D and (18) the
			night-time minimum, as required by P.533-14 section 3.2.

	 		INPUT
	 			struct ControlPt *here - reads here->L (radians), here->Sun.sza, .decl, .eot
					(set by SolarParameters())
	 			int month - 0-11; not used (kept for the interface)
	 			int hour - Hour index 0-23, used as the UTC hour for the hours-after-sunset h
					of equation (17d)
	 			int SSN - R12. Not limited to 160 here (see the note in the body).

	 		OUTPUT
	 			here->foE critical frequency of the E-layer determined for the control point here (MHz)

			NOTES
				- Phi12 is estimated from R12 by a quadratic that is not in the texts
				  available (P.1239-4 refers to P.371); unverified.
				- Latitude tests: |lat| < 32 deg uses (15a)/(16b), otherwise (15b)/(16c);
				  p = 1.31 for |lat| <= 12 deg, otherwise 1.20.
				- 73 deg < chi < 90 deg uses (17b)/(17c); chi >= 90 deg uses the greater of
				  (17d) and (17e), or (17e) alone in polar night. For (17d) sunset is found at
				  chi = 90 deg exactly, not at the 90.833 deg sunset in here->Sun.lss.
				- The result is the greater of equation (12) and equation (18) at all hours,
				  not only at night; since (18) is the night-time floor this only matters when
				  (12) falls below it.

	 		SUBROUTINES
				None

	 */
	 
	// Temps for the calculation of foE
	double A;			// solar activity factor
	double B;			// seasonal factor
	double phi;			// monthly mean 10.7 cm solar radio flux
	double M, N;

	//warning C4189: 'day15th': local variable is initialized but not referenced
	//int day15th[12] = {15,46,74,105,135,166,196,227,258,288,319,349}; // 15th day of each month;
	
	double C;			// main latitude factor
	double X, Y;		// temps
	double D;			// time-of-day factor
	double dsza;		// delta solar zenith angle
	int polarNight;		// the Sun does not rise (chi = 90 deg)
	double p, h;		// coefficients
	// End of Temporary Variables

	(void)month; // no longer needed: polar night is found from the Sun itself

	// R12 is not limited here. P.533-14 section 3.4 sets R12 to 160 "in the case of
	// foF2 only", and P.1239-4 gives foE (equations (12), (13) and (18)) in terms of
	// the 10.7 cm flux Phi12 estimated from R12 with no such limit. This clamped
	// R12 to 160 for foE as well.
	 
	/*
	 * Now find the foE for the control point "here"
	 * There are four parts to the calculation 
	 *		A = solar activity factor
	 *		B = seasonal factor
	 *		C = main latitude factor
	 *		D = time-of-day factor
	 */
	/* 
	 * Calculation for A : solar activity factor
	 * First find phi sub 12 (phi) by eqn (2) in P.1239-2 (2009)
	 * (P.1239-4 section 4 says only that Phi12 is estimated from R12 "see Recommendation
	 * ITU-R P.371"; this R12-to-Phi12 relation is not in the texts available, unverified)
	 */
	phi = 63.7 + 0.728*SSN + 0.00089*pow(SSN, 2);
	A = 1.0 + 0.0094*(phi - 66.0); 

	/*
	 * Calculation for B : seasonal factor
	 */

	if(fabs(here->L.lat) < 32.0*D2R) {
		M = - 1.93 + 1.92*cos(here->L.lat);
	}
	else { // fabs(here->L.lat) >= 32*D2R
		M = 0.11 - 0.49*cos(here->L.lat);
	}

    if(fabs(here->L.lat - here->Sun.decl) < 80.0*D2R) {
		N = here->L.lat - here->Sun.decl;
	}
	else {
		N = 80.0*D2R;
	}

    B = pow(cos(N), M);

	/* 
	 * Calculation for C : main latitude factor
	 */

	if(fabs(here->L.lat) < 32.0*D2R) { 
		X = 23.0;
		Y = 116.0;
	}
	else { // fabs(here->L.lat) >= 32*D2R
		X = 92.0;
		Y = 35.0;
	}
    C = X + Y*cos(here->L.lat);

	/*
	 * Calculation of D : time-of-day factor
	 */

	// In each case in determining D from the solar zentih angle (here->sza) the p exponent is determined the same
	if(fabs(here->L.lat) <= 12.0*D2R) {		
		p = 1.31;
	}
	else { // fabs(here->L.lat) > 12*D2R
		p = 1.2;
	}

    // Now calculate D conditional on the solar zenith angle (here->sza)
	if(here->Sun.sza <= 73.0*D2R) {
		D = pow(cos(here->Sun.sza),p);
	}
	else if ((here->Sun.sza > 73.0*D2R) && (here->Sun.sza  < PI/2.0)) { // Twilight is 90 degrees
		dsza = 6.27e-13*pow((here->Sun.sza*R2D - 50.0),8.0)*D2R;
		D = pow((cos(here->Sun.sza - dsza)),p);
	}
	else { // (here->sza >= 90.0*D2R )
		// In this case local sunset and sunrise must be known.
		// P.1239-4 (17d): "h is the number of hours after sunset (chi = 90)", and
		// "In polar winter conditions, when the Sun does not rise, equation (17e)
		// should be used". Both are at chi = 90 deg exactly, the geometric horizon.
		// This used Sun.lss and Sun.sha, which are for a 90.833 deg zenith angle
		// (refraction and the solar disc): sunset 4-11 minutes later, h short by
		// as much, night foE up to about 6 % off at 60 deg latitude. Those stay
		// for the operational-MUF day/night test. h is at the run's hour, the
		// same time as the solar zenith angle above.
		{
			double arg90 = -tan(here->L.lat)*tan(here->Sun.decl);
			if(arg90 > 1.0) {
				polarNight = TRUE;	// the Sun does not rise today
				h = 0.0;
			}
			else {
				double sha90 = acos(max(arg90, -1.0));
				double lss90 = fmod((720.0 + (-here->L.lng + sha90)*R2D*4.0 - here->Sun.eot)/60.0 + 48.0, 24.0);
				polarNight = FALSE;
				h = fmod(hour - lss90 + 48.0, 24.0);
			}
		}

		if(polarNight) {
			// Polar night
			D = pow(0.072,p)*exp(25.2 - 0.28*here->Sun.sza*R2D);
		}
		else {
			// Choose the larger of the two calculations
			D = max((pow(0.072,p)*exp(-1.4*h)), (pow(0.072,p)*exp(25.2 - 0.28*here->Sun.sza*R2D)));
		}
    }

    // Choose the larger of the foE calculations
	here->foE = max(pow(A*B*C*D, 0.25), pow(0.004*pow((1.0 + 0.021*phi), 2) , 0.25));
	
	return;
}

void SolarParameters(struct ControlPt *here, int month, double hour) {

	/*
	 
	 	SolarParameters() - Calculate the solar parmeters at the control point for the given
	 		time and month.
			The Recommendations give no formulae for these; this is a standard low-precision
			solar ephemeris (see the references below). It provides the solar zenith angle chi
			and declination for foE (P.1239-4 section 4, equations (14), (17a)-(17e)) and the
			solar zenith angle of the absorption term (P.533-14 equation (20)), for which
			P.533-14 section 5.2.2 says "The equation-of-time, for the middle of the month in
			question, is incorporated": the day is fixed at the 15th of the month.

	 		INPUT
	 			struct ControlPt *here - The control point of interest (here->L in radians)
	 			int month - Month index 0-11
	 			double hour - Decimal hours, UTC (callers pass the path hour index 0-23)

	 		OUTPUT
	 			here->Sun.ha - Hour angle (radians)
	 			here->Sun.sha -  Sunrise/Sunset hour angle (radians) for a 90.833 deg zenith angle; PI = Sun never sets, 0 = never rises
	 			here->Sun.sza - Solar zenith angle (radians)
	  			here->Sun.decl - Solar declination (radians)
	  			here->Sun.eot- Equation of time (minutes)
	 			here->Sun.lsr - local sunrise (hours UTC, 0-24)
	 			here->Sun.lsn - local solar noon (hours UTC, 0-24)
	 			here->Sun.lss - local sunset (hours UTC, 0-24)
				here->ltime - hour (the UTC hour, as passed in)

			NOTES
				Sunrise and sunset are for a zenith angle of 90.833 deg (refraction and the
				solar disc). In polar day or night sha is clamped to PI or 0 (see the body);
				MUFOperational() tests those two values.

			SUBROUTINES
				None

		Thanks to the following references
		See www.analemma.com/Pages/framesPage.html
		See holbert.faculty.asu.edu/eee463/SolarCalcs.pdf
		Although W is + and E is - and the time zones are also reversed 
		See www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF
	 
	 */

	double cosphi;	// cosine of the solar zenith angle
	double tst;		// True solar time
	double toffset;
	double ltime;	// Local time 
	double tzone;	// Time zone
	double lambda;
	double epsilon;	
	double nu;
	double beta;
	double A = 0.98565327;		// Average angle per day
	double B = 3.98891967;		// Minutes pre degree of Earth's rotation
	double S = sin(23.45*D2R);	// Earth's tile sine
	double C = cos(23.45*D2R);	// Earth's tile cosine
	double V = 78.746118*D2R;		// Value of nu on March 21st

	int day;
	double D;

	// The day of the year (doty) array allows us to determine the day count of the day of interest
	int doty[12] = { 0, 31, 59, 90, 120, 152, 181, 212, 243, 273, 304, 334 }; 

	// Determine the local time, hours, minutes, seconds and time zone
	ltime = hour + (int)(here->L.lng/(15.0*D2R)); // Local time 
	tzone = (int)(here->L.lng/(15.0*D2R)); // hours
	
	// At present this code only works for the 15th day of the month
	// If this changes a day field should be added to the path structure
	// and passed into this routine
	day = 15;

	D = doty[month] + day + hour/24.0;

	// Calculate the Equation-of-Time
	// First find the time due to the elliptic orbit of the Earth
	// The average day is 360 degrees / 365.25 days a year assuming a circular orbit equals 0.985653
	// Assume that the perihelion ( The Earth is closest to the sun ) is on January 2nd.
	// So in D days of the year the earth moves through lambda degrees
	lambda = A*D2R*(D - 2);

	// Determine the arc length due to an elliptical orbit
	// ( 360 degrees / PI ) * 0.016713 the shape factor of the elliptic equals 1.915169
	nu = lambda + 1.915169*D2R*sin(lambda);

	// Find the angles associated with the tile of the Earth
	// epsilon is the mean sun angle of the Earth after N - 80 days
	epsilon = A*D2R*(D - 80);

	// epsilon is +- PI/2
	if(epsilon >= 270*D2R) {
		epsilon -= 2.0*PI;
	}
	else if(epsilon >= 90*D2R) {
		epsilon -= PI;
	}

    // The angle of the true sun is beta
	beta = atan(C*tan(epsilon));

	// Equation of Time = tilt effect + eclliptic effect
	// Where 0.398892 is the minutes per degree of Earth's rotation 
	// 1440 minutes per day /361 degrees per day 
	here->Sun.eot = B*((epsilon - beta) + (lambda - nu))*R2D;

	// Solar declination in radians
	here->Sun.decl =  asin(S*sin((sin(A*(D-2)*D2R)*0.016713 + A*(D-2)*D2R) - V));
	
	// Find the hour angle which can be found from the solar time corrected for the local longitude and the eot
	toffset = (((here->L.lng/(15.0*D2R)) - tzone)*60.0 + here->Sun.eot); // minutes

	tst = ltime*60 + toffset; // Apparent/Local/True solar time in minutes

	here->Sun.ha = ((tst/4.0) - 180)*D2R; // radians
	
	// Hour angle at sunrise and sunset in radians
	// In polar day or night the acos() argument leaves [-1, 1] and acos() is NaN,
	// which made sunrise and sunset NaN. Take the limits instead: an argument
	// below -1 means the Sun never sets (sha = PI, sunrise and sunset 12 hours
	// either side of solar noon) and above 1 that it never rises (sha = 0, both at
	// solar noon). Consumers test sha against these two values for polar day and
	// night. Arguments in range are unchanged.
	{
		double arg = (cos(90.833*(D2R))/(cos(here->L.lat)*cos(here->Sun.decl))) - (tan(here->L.lat)*tan(here->Sun.decl));
		here->Sun.sha = acos(min(max(arg, -1.0), 1.0));
	}

	// The cosine of the solar zenith angle can be found
	cosphi = (sin(here->L.lat)*sin(here->Sun.decl)) + (cos(here->L.lat)*cos(here->Sun.decl)*cos(here->Sun.ha));

	 /* (watch out for the roundoff errors) */
    if ( fabs (cosphi) > 1.0 ) {
        if ( cosphi >= 0.0 )
            cosphi =  1.0;
        else
            cosphi = -1.0;
    }

	here->Sun.sza = acos(cosphi); // Solar zenith angle which will be positive even in the for southern latitudes

	// Switch the sign of the longitude for the time calculation
	// Local Sunrise relative to UTC in fractional hours
	here->Sun.lsr = (720.0 + (-here->L.lng - here->Sun.sha)*R2D*4.0 - here->Sun.eot)/60.0;
	
	// Local Sunset relative to UTC in fractional hours
	here->Sun.lss = (720.0 + (-here->L.lng + here->Sun.sha)*R2D*4.0 - here->Sun.eot)/60.0;

	// Local Solar noon relative to UTC in fractional hours
	here->Sun.lsn = (720.0 + (-here->L.lng)*R2D*4.0 - here->Sun.eot)/60.0;

	// Roll over the times. Note: add 24 because for the fmod(x, 24) x might be negative 
	here->Sun.lsr = fmod(here->Sun.lsr + 24.0, 24.0); 
	here->Sun.lss = fmod(here->Sun.lss + 24.0, 24.0);
	here->Sun.lsn = fmod(here->Sun.lsn + 24.0, 24.0);

	// Store the UTC time to here structure
	here->ltime = hour;

	return;

}

double BilinearInterpolation(double LL, double LR, double UL, double UR, double r, double c) {

	/*

	 BilinearInterpolation() - Interpolates a value given the Neighbors by
			the method in ITU-R P.1144-5 (10/09)
			(P.1144 text not provided: reference unverified. P.1239-4 section 3.1 refers to
			"the bi-linear interpolation procedure given in Recommendation ITU-R P.1144
			(Annex 1)" and section 3.2 allows "A bilinear interpolation process" for the
			decile tables.)

	 		INPUT
	 			double LL - Lower left neighbor
	 			double LR - Lower right neighbor
	 			double UL - Upper left neighbor
	 			double UR - Upper right neighbor
	 			double r - Fraction row, 0 to 1, measured from the lower (LL, LR) row
	 			double c - Fractional column, 0 to 1, measured from the left (LL, UL) column

	 		OUTPUT
	 			returns the interpolated value (units of the inputs)
				No range check: r or c outside [0, 1] extrapolates.

			SUBROUTINES
				None
	 
	 */
	
	return	LL*((1.0 - r)*(1.0 - c)) +
			UL*((r)*(1.0 - c)) +
			LR*((1.0 - r)*(c)) +
			UR*((r)*(c));

}



