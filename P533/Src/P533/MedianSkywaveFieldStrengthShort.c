#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

// Testing 
// This turns diagnostic prints off (FALSE) and on (TRUE)
#define BARF			FALSE
#define BARF_NOMODES	FALSE
#define TXEND			0
#define RXEND			1
// Testing

// Local Defines
// P.533-14 section 5.2: "Lz: term containing those effects in sky-wave
// propagation not otherwise included in this method. The present recommended
// value is 8.72 dB". This read 9.14.
#define NOIL			8.72
#define PEN				TRUE // Use penetration points for absorption
//#define PEN			FALSE // Use the absorption method in P.533-12
// End Local Defines

// Local prototypes
double DiurnalAbsorptionExponent(struct ControlPt CP, int month); 
double AbsorptionFactor(struct ControlPt CP, int month);
double AbsorptionLayerPenetrationFactor(double T);
double AbsorptionTerm(struct ControlPt CP, int month, double fv);
double FindLh(struct ControlPt CP, double D, double hour, int month);
double PenetrationPoints(struct PathData * path, double noh, double fv);
static double LongitudinalGyrofrequency(struct ControlPt P);
int WhatSeasonforLh(struct Location L, int month); 
int SmallestCPfoF2(struct PathData path);
// End local prototypes
 
void MedianSkywaveFieldStrengthShort(struct PathData *path) {

	/*

	 	MedianSkywaveFieldStrengthShort() - Calculate the median skywave field strength as described in ITU-R P.533-14
	 		section 5.2 "Paths up to 9 000 km" (the only method below 7000 km; interpolated with section 5.3
			between 7000 and 9000 km). For each mode selected by section 5.2.1:
				Ew = 136.6 + Pt + Gt + 20 log f - Lb                                   equation (17)
				Lb = 32.45 + 20 log f + 20 log p' + Li + Lm + Lg + Lh + Lz             equation (18)
				p' from equation (19) with the elevation angle of equation (13)
				Li from equation (20) (with (21), (22), (23) and Figs. 1 - 3), see PenetrationPoints()
				Lm from equations (24), (25) (E modes) and (26) (F2 modes)
				Lg = 2(n - 1)                                                          equation (27)
				Lh from Table 2, mean over the Table 1d) control points, see FindLh()
				Lz = 8.72 dB
			and the resultant Es = 10 log10(sum 10^(Ew/10)) over the modes, equation (28).
			Mirror-reflection heights (section 5.2.1): 110 km for E modes; for F2 modes equation (2)'s hr at
			the mid-path control point (D <= dmax) or at the Table 1c) control point with the lowest foF2
			(dmax < D <= 9000 km).
	  		There is an unavoidable amount of redundancy in this routine. The same code statements are executed for the E and F2 layers. 
	  		This means that if alterations to the subroutine are necessary, the changes must be in the E layer and F2 layer loop. The advantage to 
	  		doing it this way is that a jumbling of the E and F2 layers is avoided. All of the nine modes could be treated together in an array. 
	  		This would be more efficient but less readable. This project goal was to write code that follows the standard flow as closely as possible.
	 
	 		INPUT
	 			struct PathData *path
					path->distance - D (km); nothing is done for D > 9000 km
					path->frequency - f (MHz), path->txpower - Pt (dB(1 kW)), path->SSN - R12 (not limited
						to MAXSSN in equation (20): section 3.4 limits R12 "in the case of foF2 only")
					path->month - 0-based month index
					path->dmax (km), path->n0_E, path->n0_F2, path->Md_E[].BMUF, path->Md_F2[].BMUF,
					path->Md_F2[].fs - from MUFBasic() and ELayerScreeningFrequency()
					path->CP[] - the Table 1 control points with their parameters at the current hour
					path->A_tx - transmit antenna pattern

	 		OUTPUT
	 			path->Md_E[].Lb - E mode basic transmission loss (dB), equation (18)
	 			path->Md_E[].Ew - E mode field strength (dB(1 uV/m)), equation (17)
	 			path->Md_F2[].Lb - F2 mode basic transmission loss (dB)
	 			path->Md_F2[].Ew - F2 mode field strength (dB(1 uV/m))
	 			path->Md_F2[n].ele - F2 mode elevation angle (radians)
	 			path->Md_E[n].ele - E mode elevation angle (radians)
				path->Md_E[].MC, path->Md_F2[].MC - TRUE for each mode included in Es
	 			path->Es - Median field strength with E layer screening, equation (28) (dB(1 uV/m));
						TINYDB (-307) if no mode is available
				path->Lz - 8.72 dB (NOIL)
				path->ptick - p' (km) of the last mode calculated (overwritten mode by mode)

			NOTES
				Mode n is the C index, so the mode has n+1 hops.
				Lh uses the mid-path local mean time for every control point (Table 2 is indexed by
				"Mid-path local time"). The absorption, fL and Lh averages over 1, 3 or 5 control points
				follow Table 1d) (D <= 2000, 2000 < D <= dmax, D > dmax); with PEN TRUE the absorption
				is instead evaluated at the 90 km penetration points (PenetrationPoints()), and the
				averaged fL is then only printed in diagnostics.
				The E-mode loop stops at the first slot that fails the mode test; the F2-mode loop
				tests every slot.

	 		SUBROUTINES
				SmallestCPfoF2()
				ElevationAngle()
				IncidenceAngle()
				PenetrationPoints()
				AbsorptionTerm() (only when PEN is FALSE)
				FindLh()
				AntennaGain()
				LocalMeanTime()

	 */

	// Temp
	double dh;			// Hop length
	double delta;		// Elevation angle
	double psi;			// Hop half angle
	double hr_E, hr_F2; // The mirror reflection points for the E and F2 layers
	double fv;			// Vertical-incidence wave frequency
	double Pt;			// Tx power (dB(1 kW))
	double Li;			// Absorption loss (dB) for the n-hop mode
	double Gt;			// Tx antenna gain in the desired direction (dB)
	double aoi110;		// Angle of incidence at 110 km
	double AT;			// Absorption term. There can be up to 5 absorption terms one for each control point
	double fL;			// Mean electron gyrofrequency at 100 km at the control point
	double Lm;			// "Above-the-MUF" loss
	double Lg;			// Ground reflection loss
	double Lh;			// Auroral and "Other" signal loss
	double Etw;			// Median field strength
	double SSN;			// Sun spot number

	int n;
	int m;		// Mode index for the BARF_NOMODES listings, which must not disturb n
	double mpltime;		// Midpath local time (h)
	// End Temp

	// Only do this subroutine if the path is less than or equal to 9000 km if not exit
	if(path->distance > 9000) return; 
	
	// 5.2 Paths up to 7000 km
	// 5.2.1 Modes considered
	//		The modes considered will be determined by if statements within the 
	//		E and F2 layer calculation loops
	// 5.2.2 Field strength determination
	
	// R12 in the absorption term (1 + 0.0067 R12) of equation (20) is not limited:
	// P.533-14 section 3.4 sets R12 to 160 "in the case of foF2 only". This
	// clamped it to 160, making Li low above R12 = 160 (11.5 % at R12 = 200).
	SSN = path->SSN;

	// This procedure applies only to paths less than 7000 km where it is the only method used and
	// paths greater than 7000 km but less than 9000 km where the field strength is interpolated with
	// the long path method
	
	// Initialize the mirror reflection height for the E-Layer 
	hr_E = 110.0;
		
	// Determine the mirror reflection height for the F2 layers
	// Use hr_F2 in this routine for readability
	if(path->distance > path->dmax) { // Note the path->distance is less than 9000 and path->distance is greater than dmax
		// Find the smallest foF2 amongst the control points  
		hr_F2 = MIN((1490.0/path->CP[SmallestCPfoF2(*path)].M3kF2) - 176.0, 500.0);
	}
	else { // (path->distance <= path->dmax)
		// Use the midpoint to determine the mirror reflection height
		hr_F2 = path->CP[MP].hr;
	}

    /*******************************************************************************************************/
	// Although the calculations for E and F2 layers are for the most part the 
	// same in order to not obscure the calculation relative to the standard
	// each layer will be dealt within its own loop. 
	/*******************************************************************************************************/

	// Determine the local time at the midpath point
	// Table 2 is indexed by "Mid-path local time, t". This used UTC plus the
	// longitude's whole time zone, (int)(lng/15), which is up to an hour out.
	mpltime = LocalMeanTime(path->CP[MP]);
	
	// Begin E modes median sky-wave field strength calculation
	// Does a low order E mode exist?
	if(path->n0_E != NOLOWESTMODE) { 
		// Determine the E modes that satisfy the criteria.
		for(n=path->n0_E; n<MAXEMDS; n++) {
			if(((n == path->n0_E) && (path->distance/(path->n0_E + 1.0) <= 2000.0)) // The lowest order mode is less than 2000 km.
										||
						((n > path->n0_E) && (path->Md_E[n].BMUF != 0.0))) { // higher order modes

				// Find the elevation angle from equation 13 Section 5.1 Elevation angle
				// ITU-R P.533-14
				delta = ElevationAngle(path->distance/(n+1.0), hr_E);
				
				// Store the elevation angle, but use delta elsewhere here for readability.
				path->Md_E[n].ele = delta;
			
				// angle of incidence at height hr = 110 km
				aoi110 = IncidenceAngle(delta, hr_E);

				// Vertical-incidence wave frequency
				fv = path->frequency*cos(aoi110);

				// Hop distance 
				dh = path->distance/(n+1.0);

				// Find the virtual slant range (19).
				psi = dh/(2.0*R0);

				path->ptick = fabs(2.0*R0*((sin(psi)/cos(delta + psi))))*(n+1.0);

				// Determine the number of control points.
				if(path->distance <= 2000.0) {

					if(PEN) {
						AT = PenetrationPoints(path, n, fv); 
					}
					else {
						// Find the loss due to all the absorption terms in Li
						// The absorption term includes loss from solar zenith angles, ATnoon and phin(fv/foE)
						AT = AbsorptionTerm(path->CP[MP], path->month, fv);
					}

                    // Determine the longitudinal gyrofrequency
					fL = fabs(path->CP[MP].fH[HR100km]*sin(path->CP[MP].dip[HR100km])); 

					// Determine auroral and other signal losses
					Lh = FindLh(path->CP[MP], path->distance, mpltime, path->month);
				}
				else { // (path->distance > 2000.0) There are three control points

					if(PEN) {
						AT = PenetrationPoints(path, n, fv); 
					}
					else {				
						// Find the loss due to all the absorption terms in Li
						// The absorption term includes loss from solar zenith angles, ATnoon and phin(fv/foE)
						AT = (AbsorptionTerm(path->CP[MP], path->month, fv) +
							  AbsorptionTerm(path->CP[T1k], path->month, fv) +
							  AbsorptionTerm(path->CP[R1k], path->month, fv))/3.0;
					}

                    // Determine the average longitudinal gyrofrequency
					fL = (fabs(path->CP[MP].fH[HR100km]*sin(path->CP[MP].dip[HR100km])) + 
						  fabs(path->CP[T1k].fH[HR100km]*sin(path->CP[T1k].dip[HR100km])) + 
						  fabs(path->CP[R1k].fH[HR100km]*sin(path->CP[R1k].dip[HR100km])))/3.0;

					// Determine auroral and other signal losses
					Lh = (FindLh(path->CP[MP], path->distance, mpltime, path->month) +
						  FindLh(path->CP[T1k], path->distance, mpltime, path->month) +
						  FindLh(path->CP[R1k], path->distance, mpltime, path->month))/3.0;
				} // (path->distance <= 2000.0)
	
				// All the variable have been calculated to determine
				// Absorption loss (dB) for an n-hop mode, Li
				if(PEN) {
					// AT is already the mean of ATj/(f + fLj)^2 over the penetration points
					Li = ((n+1.0)*(1.0 + 0.0067*SSN)*AT)/cos(aoi110);
				}
				else {
					Li = ((n+1.0)*(1.0 + 0.0067*SSN)*AT)/(pow((path->frequency + fL),2)*cos(aoi110));
				}

				// "Above-the-MUF" loss, P.533-14 equations (24) and (25).
				// Lm = 0 for f <= fb; for E modes above fb,
				//     Lm = 130[(f/fb) - 1]^2 dB, or 81 dB whichever is smaller.
				// This previously read 46[(f/fb) - 1]^0.5 + 5, capped at 58: the
				// wrong coefficient and exponent, and an additive constant that
				// made Lm step to 5 dB the instant f exceeded the basic MUF.
				if(path->frequency <= path->Md_E[n].BMUF) {
					Lm = 0.0;
				}
				else { // (path->frequency > path->Md_E[n].BMUF)
					Lm = MIN(130.0*pow(((path->frequency/path->Md_E[n].BMUF) - 1.0), 2.0), 81.0);
				}

                // Ground reflection loss
				Lg = 2.0*((n + 1.0) - 1.0); // n is a C index starting at 0 instead of 1 
				
				// "Not otherwise included" loss
				path->Lz = NOIL; 

				// The ray path basic transmission loss for the mode under consideration
				path->Md_E[n].Lb = 32.45 + 20.0*log10(path->frequency) + 20.0*log10(path->ptick) + Li + Lm + Lg + Lh + path->Lz;

				// Tx antenna gain in the desired direction (dB)
				Gt = AntennaGain(*path, path->A_tx, delta, TXTORX);

				// Transmit power
				Pt = path->txpower;

				path->Md_E[n].Ew = 136.6 + Pt + Gt + 20.0*log10(path->frequency) - path->Md_E[n].Lb;

				// Testing
				if(BARF) {
					printf("\nMSFSS: **** Start %dE mode Field Strength ********\n", (n+1));
					printf("MSFSS: Average over CPs of Absorption Terms \t\t%f\n", AT);
					printf("MSFSS: Average over CPs of longitudinal fH (fL) \t%f\n", fL);
					printf("MSFSS: Average over CPs Auroral etc loss (Lh) \t\t%f\n", Lh);
					printf("MSFSS: Absorption loss for %dE mode Li \t\t\t\t%f\n", (n+1), Li);
					printf("MSFSS: Angle of Arrival at 110 km \t\t%f\n", aoi110*R2D);
					printf("MSFSS: Oblique Frequency (fv) \t\t\t%f\n", fv);
					printf("MSFSS: Above-the-MUF loss (Lm) \t\t\t%f\n", Lm);
					printf("MSFSS: Ground-Reflection loss (Lg) \t\t%f\n", Lg);
					printf("MSFSS: \"Otherwise included\" loss (Lz) \t%f\n", path->Lz);
					printf("MSFSS: Antenna Gain (Gt) \t\t%f\n", Gt);
					printf("MSFSS: Transmitter power (Pt) \t%f\n", Pt);
					printf("MSFSS: Hop distance (dh) \t\t%f\n", dh);
					printf("MSFSS: Elevation angle (delta) \t%f\n", delta*R2D);
					printf("MSFSS: Hop angle (d/2R0) \t\t%f\n", psi*R2D);
					printf("MSFSS: Slant path (p') \t\t\t%f\n", path->ptick);
					printf("MSFSS: path->Md_E[%d].Lb \t\t%f\n", n, path->Md_E[n].Lb);
					printf("MSFSS: path->Md_E[%d].Ew \t\t%f\n", n, path->Md_E[n].Ew);
					printf("MSFSS: **** End %dE mode Field Strength **********\n\n", (n+1));
				}
                // Testing

			}
			else { 
				break; // There is no E modes that satisfiy the criteria
			} // The lowest order E mode is less than 2000 km and higher modes.
		}
    } // End E modes median sky-wave field strength calculation

	/******************************************************************************************************************/

	// Begin F2 modes median sky-wave field strength calculation
	// Does a low order F2 mode exist?
	if(path->n0_F2 != NOLOWESTMODE) { 
		// Determine the F2 mode that satisfy the criteria.
		for(n=path->n0_F2; n<MAXF2MDS; n++) {
			// Modes Considered:
			// Any mode that has an E-layer maximum screening frequency that is less than the operating frequency. 
			// The lowest order mode must also have hops that are less than dmax km 
			if(((n == path->n0_F2) && (path->distance/(path->n0_F2 + 1.0) <= path->dmax) && (path->Md_F2[n].fs < path->frequency)) 
										||
				(((n > path->n0_F2) && (path->Md_F2[n].BMUF != 0.0)) && (path->Md_F2[n].fs < path->frequency))) { // higher order modes
				
								        
				// Find the elevation angle from equation 13 Section 5.1 Elevation angle.
				// ITU-R P.533-14
				delta = ElevationAngle(path->distance/(n+1.0), hr_F2);

				// Store the elevation angle
				path->Md_F2[n].ele = delta;

				// angle of incidence at height hr = 110 km
				aoi110 = IncidenceAngle(delta, 110.0);

				// Vertical-incidence wave frequency
				fv = path->frequency*cos(aoi110);

				// Hop distance 
				dh = path->distance/(n+1.0);

				// Find the virtual slant range (19)
				psi = dh/(2.0*R0);

				// Calculate the slant range
				path->ptick = fabs(2.0*R0*((sin(psi)/cos(delta + psi))))*(n+1.0);
				
				// Determine the number of control points
				if(path->distance <= 2000.0) { // Use the mid-path control point

					if(PEN) {
						AT = PenetrationPoints(path, n, fv); 
					}
					else {
						// Find the loss due to all the absorption terms in Li
						// The absorption term includes loss from solar zenith angles, ATnoon and phin(fv/foE)
						AT = AbsorptionTerm(path->CP[MP], path->month, fv);
					}

                    // Determine the longitudinal gyrofrequency
					fL = fabs(path->CP[MP].fH[HR100km]*sin(path->CP[MP].dip[HR100km])); 
					
					// Determine auroral and other signal losses
					Lh = FindLh(path->CP[MP], path->distance, mpltime, path->month);
				}
				else if((2000.0 < path->distance) && (path->distance <= path->dmax)) { // There are three control points

					if(PEN) {
						AT = PenetrationPoints(path, n, fv); 
					}
					else {
						// Find the loss due to all the absorption terms in Li
						// The absorption term includes loss from solar zenith angles, ATnoon and phin(fv/foE)
						AT = (AbsorptionTerm(path->CP[MP], path->month, fv) +
							  AbsorptionTerm(path->CP[T1k], path->month, fv) +
							  AbsorptionTerm(path->CP[R1k], path->month, fv))/3.0;
					}

                    // Determine the average longitudinal gyrofrequency
					fL = (fabs(path->CP[MP].fH[HR100km]*sin(path->CP[MP].dip[HR100km])) + 
						  fabs(path->CP[T1k].fH[HR100km]*sin(path->CP[T1k].dip[HR100km])) + 
						  fabs(path->CP[R1k].fH[HR100km]*sin(path->CP[R1k].dip[HR100km])))/3.0;

					// Determine auroral and other signal losses
					Lh = (FindLh(path->CP[MP], path->distance, mpltime, path->month) +
						  FindLh(path->CP[T1k], path->distance, mpltime, path->month) +
						  FindLh(path->CP[R1k], path->distance, mpltime, path->month))/3.0;
				}
				else { // There are 5 control points.

					if(PEN) {
						AT = PenetrationPoints(path, n, fv); 
					}
					else {
						// Find the loss due to all the absorption terms in Li.
						// The absorption term includes loss from solar zenith angles, ATnoon and phin(fv/foE)
						AT = (AbsorptionTerm(path->CP[MP], path->month, fv)   +
							  AbsorptionTerm(path->CP[T1k], path->month, fv)  +
							  AbsorptionTerm(path->CP[R1k], path->month, fv)  +
							  AbsorptionTerm(path->CP[Td02], path->month, fv) +
							  AbsorptionTerm(path->CP[Rd02], path->month, fv))/5.0;
					}

                    // Find the average longitudinal gyrofrequency
					fL = (fabs(path->CP[MP].fH[HR100km]*sin(path->CP[MP].dip[HR100km])) + 
						  fabs(path->CP[T1k].fH[HR100km]*sin(path->CP[T1k].dip[HR100km])) + 
						  fabs(path->CP[R1k].fH[HR100km]*sin(path->CP[R1k].dip[HR100km])) + 
						  fabs(path->CP[Td02].fH[HR100km]*sin(path->CP[Td02].dip[HR100km])) +
						  fabs(path->CP[Rd02].fH[HR100km]*sin(path->CP[Rd02].dip[HR100km])))/5.0;
					
					// Determine auroral and other signal losses
					Lh = (FindLh(path->CP[MP], path->distance, mpltime, path->month)  +
						  FindLh(path->CP[T1k], path->distance, mpltime, path->month) +
						  FindLh(path->CP[R1k], path->distance, mpltime, path->month) +
						  FindLh(path->CP[Td02], path->distance, mpltime, path->month)+
						  FindLh(path->CP[Rd02], path->distance, mpltime, path->month))/5.0;
				} // (path->distance <= 2000.0)

				// All the variable have been calculated to determine
				// Absorption loss (dB) for an n-hop mode, Li.
				if(PEN) {
					// AT is already the mean of ATj/(f + fLj)^2 over the penetration points
					Li = ((n+1.0)*(1.0 + 0.0067*SSN)*AT)/cos(aoi110);
				}
				else {
					Li = ((n+1.0)*(1.0 + 0.0067*SSN)*AT)/(pow((path->frequency + fL),2)*cos(aoi110));
				}
	
				// "Above-the-MUF" loss, P.533-14 equations (24) and (26).
				// Lm = 0 for f <= fb; for F2 modes above fb,
				//     Lm = 36[(f/fb) - 1]^0.5 dB, or 62 dB whichever is smaller.
				// The Recommendation gives one expression for all distances. This
				// previously branched at 3000 km -- a split that appears nowhere
				// in P.533-14 -- and added 5 dB below it and 8 dB above, so Lm
				// stepped discontinuously as f crossed the basic MUF.
				//
				// KNOWN BIAS. Against the CCIR D1 databank (all 1613 cases,
				// macos-build/d1_absolute.py) the short model is biased high on
				// circuits operated above the path basic MUF fb:
				//
				//     f <= fb   n = 8083   bias +0.56 dB   RMS  8.19
				//     f  > fb   n = 3537   bias +5.74 dB   RMS 11.92
				//
				// (figures after the 2026-09 alignment of this code with the text of
				// P.533-14, P.842-5, P.1239-4 and P.1240-2; see git log.)
				//
				// macos-build/d1_bias.py breaks this down, and it is not simply
				// Lm being too weak:
				//
				//  - It starts below fb, where Lm = 0: +5.4 dB at 0.9 < f/fb <= 1,
				//    and no change to Lm moves it. Hour alignment is not the cause.
				//  - It depends on distance. Above fb: +5.1 dB up to 1000 km
				//    (2353 of the 3537), +13.2 dB at 3000-4000 km, +4.0 dB at
				//    4000-7000 km. The 3000-4000 km figure is mostly circuit 76
				//    (Teheran-Norddeich, 15.1 MHz), measured down to -55 dB(1 uV/m),
				//    which implies an Lm near 75 dB against the 62 dB cap.
				//  - 1F2 or 2F2 carries the field in 3259 of the 3537 hours.
				//
				// Bias/RMS for d <= 7000 km (all 11620 hours) under alternatives:
				//
				//     this expression                     +2.14 / 9.48
				//     the removed code (+5/+8, 70x+8)     -0.08 / 8.88
				//     this expression + 5 dB              +0.71 / 9.01
				//     fb scaled by 0.90 inside Lm only    +0.04 / 8.83
				//
				// Scaling fb fits best and is the only one that removes the bias
				// just below fb, which suggests the basic MUF acts 5-10% high for
				// field strength rather than Lm being too small. The removed code's
				// 70x+8 beyond 3000 km overshoots at 4000-7000 km (-5.7 dB). D1
				// cannot separate a MUF error from an Lm error; oblique-sounding MUF
				// data could.
				//
				// The Recommendation's expression is kept regardless: it is what
				// P.533-14 states, and Lz cannot absorb a bias confined to one
				// population. Resolving it is an SG3 matter, not a local fix.
				if(path->frequency <= path->Md_F2[n].BMUF) {
					Lm = 0.0;
				}
				else { // (path->frequency > path->Md_F2[n].BMUF)
					Lm = MIN(36.0*pow(((path->frequency/path->Md_F2[n].BMUF) - 1.0), 0.5), 62.0);
				}

                // Ground reflection loss
				Lg = 2.0*((n + 1.0) - 1.0); // n is a C index starting at 0 instead of 1  

				// "Not otherwise included" loss
				path->Lz = NOIL;

				// Calculate the total loss.
				path->Md_F2[n].Lb = 32.45 + 20.0*log10(path->frequency) + 20.0*log10(path->ptick) + Li + Lm + Lg + Lh + path->Lz;

				// Tx antenna gain in the desired direction (dB)
				Gt = AntennaGain(*path, path->A_tx, delta, TXTORX);

				Pt = path->txpower;

				path->Md_F2[n].Ew = 136.6 + Pt + Gt + 20.0*log10(path->frequency) - path->Md_F2[n].Lb;

				// Testing
				if(BARF) {
					printf("\nMSFSS: **** Start %dF2 mode Field Strength ********\n", (n+1));
					printf("MSFSS: Average over CPs of Absorption Terms \t\t%f\n", AT);
					printf("MSFSS: Average over CPs of longitudinal fH (fL) \t%f\n", fL);
					printf("MSFSS: Average over CPs Auroral etc loss (Lh) \t\t%f\n", Lh);
					printf("MSFSS: Absorption loss for %dF2 mode Li \t\t\t\t%f\n", (n+1), Li);
					printf("MSFSS: Angle of Arrival at 110 km \t\t%f\n", aoi110*R2D);
					printf("MSFSS: Oblique Frequency (fv) \t\t\t%f\n", fv);
					printf("MSFSS: Above-the-MUF loss (Lm) \t\t\t%f\n", Lm);
					printf("MSFSS: Ground-Reflection loss (Lg) \t\t%f\n", Lg);
					printf("MSFSS: \"Otherwise included\" loss (Lz) \t%f\n", path->Lz);
					printf("MSFSS: Antenna Gain (Gt) \t\t%f\n", Gt);
					printf("MSFSS: Transmitter power (Pt) \t%f\n", Pt);
					printf("MSFSS: Hop distance (dh) \t\t%f\n", dh);
					printf("MSFSS: Elevation angle (deg) (delta) \t%f\n", delta*R2D);
					printf("MSFSS: hop angle (deg) (d/2R0) \t\t%f\n", psi*R2D);
					printf("MSFSS: slant path (p') \t\t\t%f\n", path->ptick);
					printf("MSFSS: path->Md_F2[%d].Lb \t\t%f\n", n, path->Md_F2[n].Lb);
					printf("MSFSS: path->Md_F2[%d].Ew \t\t%f\n", n, path->Md_F2[n].Ew);
					printf("MSFSS: **** End %dF2 mode Field Strength **********\n\n", (n+1));
				}
                // Testing

			}
        }
    } // End F2 modes median sky-wave field strength calculation

	// Determine the overall resultant equivalent median sky-wave field strength, Es
	// See "Modes considered" Section 5.2.1 P.533-14
	// Es should be very small, reinitialize it for clarity.
	path->Es = TINYDB;
	// Initialize the other locals needed here.
	Etw = 0.0;

	// Do any E-layer modes exist if so proceed
	if(path->n0_E != NOLOWESTMODE) {
		// Sum the lowest-order E-layer mode with hop length up to 2000 km
		// and the next two higher-order E-layer modes that exist;
		for(n=path->n0_E; n<MAXEMDS; n++) {
			if(((n == path->n0_E) && (path->distance/(path->n0_E+1) <= 2000.0))
					                            ||
				((n != path->n0_E) && (path->Md_E[n].BMUF != 0.0))) { 
				Etw += pow(10.0, (path->Md_E[n].Ew/10.0));
				path->Md_E[n].MC = TRUE;
			}
        }
    }

    // Do any F2-layer modes exist if so proceed
	if(path->n0_F2 != NOLOWESTMODE) {
		// Mode Considered: The lowest-order F2-layer mode with a hop length up to dmax (km) and 
		// maximally the next five F2-layer higher-order modes.
		// If that mode's E layer screening frequency is less than the operating frequency
		for(n=path->n0_F2; n<MAXF2MDS; n++) {
			if(((n == path->n0_F2) && (path->distance/(path->n0_F2+1) <= path->dmax) && (path->Md_F2[n].fs < path->frequency)) 
				                                ||
				((n != path->n0_F2) && (path->Md_F2[n].BMUF != 0.0) && (path->Md_F2[n].fs < path->frequency))) {
			Etw += pow(10.0, (path->Md_F2[n].Ew/10.0));
			path->Md_F2[n].MC = TRUE;
			}
			// Testing
			else if (path->Md_F2[n].fs < path->frequency) {
				if(BARF_NOMODES) {
					printf("\nMSFSS: *** %dF2 modes are screened by the E layer ********\n", n);
					printf(  "MSFSS: Path ID: %s\n", path->name);
					printf(  "MSFSS: %s to %s\n", path->txname, path->rxname);
					printf(  "MSFSS: Hour %d Month %d \n", path->hour, path->month);
					for(m=0; m<MAXF2MDS; m++) printf("MSFSS: %dF2 mode E layer screening freq: %7.3f\n", m+1, path->Md_F2[m].fs);
					printf(  "MSFSS: **** %dF2 modes are screened by the E layer ********\n", n);
				}
            }
            // Testing
		}
    }

    if(BARF_NOMODES) {
		printf("\nMSFSS: **** No E modes and all foF2 modes are screened by the E layer ********\n");
		printf(  "MSFSS: Path ID: %s\n", path->name);
		printf(  "MSFSS: %s to %s\n", path->txname, path->rxname);
		printf(  "MSFSS: Hour %d Month %d \n", path->hour, path->month);
		for(m=0; m<MAXF2MDS; m++) printf("MSFSS: %dF2 mode E layer screening freq: %7.3f\n", m+1, path->Md_F2[m].fs);
		printf(  "MSFSS: **** No E modes and all foF2 modes are screened by the E layer ********\n");
	}

    // Find the field strength if there are any modes to consider
	// If there are no modes than path->Es will remain equal to TINYDB 
	if(Etw != 0.0) {
		path->Es = 10.0*log10(Etw); // Field strength with E layer screening
	}
	else { // All the modes have been screened 
		// Testing
		if(BARF_NOMODES) {
			printf("\nMSFSS: **** No E modes and all foF2 modes are screened by the E layer ********\n");
			printf(  "MSFSS: Path ID: %s\n", path->name);
			printf(  "MSFSS: %s to %s\n", path->txname, path->rxname);
			printf(  "MSFSS: Hour %d Month %d \n", path->hour, path->month);
			for(m=0; m<MAXF2MDS; m++) printf("MSFSS: %dF2 mode E layer screening freq: %7.3f\n", m+1, path->Md_F2[m].fs);
			printf(  "MSFSS: **** No E modes and all foF2 modes are screened by the E layer ********\n");
		}
        // Testing
	}

    return;

} // End Median Sky-wave Field Strength 


double AbsorptionTerm(struct ControlPt CP, int month, double fv) {

	/*

	 	AbsorptionTerm() - Determines the absorption term from the three factors:
	 		i) Absorption factor at local noon
	 		ii) Absorption layer penetration factor
	 		iii) Diurnal absorption exponent
	 		These three factors can be seen in Figures 1, 2, and 3 of P.533-14
	 		section 5.2.2 "Field strength determination". The term returned is the j-th summand of
			equation (20) without its 1/(f + fLj)^2 divisor:
				ATjnoon * F(chi_j)/F(chi_jnoon) * phi_n(fv/foE_j)
			with F(chi) = cos^p(0.881 chi) or 0.02, whichever is greater (equation (21)), chi_j the
			solar zenith angle at the point "or 102 degrees whichever is the smaller", chi_jnoon its
			value at local noon, ATnoon from Fig. 1, phi_n from Fig. 2 and p from Fig. 3.

	 		INPUT
	 			struct ControlPt CP - the Control point (or penetration point) of interest, after
						CalculateCPParameters(): L (radians), foE (MHz), dip[HR100km] (radians),
						Sun.sza (radians) at the current hour and Sun.lsn (local solar noon, UTC hours)
	 			int month - The 0-based month index
	 			double fv - Vertical-incidence wave frequency fv = f cos(i), equation (22) (MHz)

	 		OUTPUT
	 			returns the absorption term used to calculate Li in Eqn (20) (dimensionless apart
				from the units of ATnoon)

			NOTES
				chi_jnoon is the solar zenith angle computed by SolarParameters() at the point's local
				solar noon Sun.lsn (which includes the equation of time for the middle of the month).
	 
	 		SUBROUTINES
				ZeroCP()
				DiurnalAbsorptionExponent()
				SolarParameters()
				AbsorptionFactor()
				AbsorptionLayerPenetrationFactor()

	 */
	
	double p;			// Diurnal absorption exponent
	double chij;		// Solar zenith angle at the j-th control point
	double chijnoon;	// Solar zenith angle at the j-th control point at noon
	double Fchij;		// Eqn (21) with chij argument
	double Fchijnoon;	// Eqn (21) with chijnoon argument
	double ATnoon;		// Absorption factor at local noon and R12 = 0
	double phin;		// Absorption layer penetration factor

	struct ControlPt CP_0; // Temp control point

	// Zero the local control point
	ZeroCP(&CP_0);
	
	// Find the diurnal absorption exponent, p.
	p = DiurnalAbsorptionExponent(CP, month);

	// The solar zenith angle for the control point
	// Make sure that it doesn't exceed 102 degrees
	chij = min(CP.Sun.sza, 102.0*D2R);

	Fchij = max(pow(cos(0.881*chij), p), 0.02);

	// Determine when noon is local time, then find the solar zenith angle.
	// To make this calculation, a temporary control point must be used 
	// where the only variable necessary to initialize is the longitude.
	CP_0 = CP;
	// The hour that gets passed to SolarParameters() is a UTC fractional hour 
	// The local noon in UTC has already been calculated and stored by execution 
	// CalculateCPParameters()
	// The month is in the path structure thus: 
	// The hour for this calculation is CP[].Sun.lsn and
	// the month for this calculation is path->month.
	SolarParameters(&CP_0, month, CP.Sun.lsn);

	// The solar zenith angle for the control point at noon local time.
	chijnoon = CP_0.Sun.sza;

	Fchijnoon = max(pow(cos(0.881*chijnoon), p), 0.02);

	// Find the remaining absorption parameters.
	ATnoon = AbsorptionFactor(CP, month);

	phin = AbsorptionLayerPenetrationFactor(fv/CP.foE);

	// Testing
	if(BARF) {
		printf("\nMSFSS: Control Point CP.L.lat %f CP.L.lng %f\n", CP.L.lat, CP.L.lng);
		printf("MSFSS: Absorption term (ATnoon*phin*Fchij/Fchijnoon) %f\n", ATnoon*phin*Fchij/Fchijnoon);
		printf("MSFSS: ATnoon %f\n", ATnoon);
		printf("MSFSS: phin %f\n", phin);
		printf("MSFSS: Fchij %f\n", Fchij);
		printf("MSFSS: Fchijnoon %f\n", Fchijnoon);
	}
    // Testing

	return ATnoon*phin*Fchij/Fchijnoon;
}


double DiurnalAbsorptionExponent(struct ControlPt CP, int month) {

	/*

	 	 DiurnalAbsorptionExponent() Determine the diurnal absorption exponent, p, used in equation (21).
	 		The exponent, p, is a function of the month and the modified magnetic dip at 100 km.
	 		The p vs modified magnetic dip graph is shown as Figure 3 ITU-R P.533-14, which refers to
			Recommendation ITU-R P.1239, Annex 1, for the modified dip; P.1239-4 equation (4) gives it as
			X = arctan(I / sqrt(cos(lat))). The figure is represented by two degree-6 polynomials per
			month (below and above a month-dependent break dip, ppt[]), in a normalised dip; the
			coefficients come from REC533 and cannot be checked against the figure's text.

	 		INPUT
	 			struct ControlPt CP - control point of interest: L.lat (radians), dip[HR100km] (radians)
	 			int month - 0-based month index; for southern latitudes it is shifted by six months,
						the second (Southern) month axis of Fig. 3

	 		OUTPUT
	 			return p the diurnal absorption exponent (dimensionless)

			NOTES
				The modified dip is taken as its absolute value and limited to 70 degrees (the top of
				Fig. 3, "70-90").
	 
			SUBROUTINES
				None

	 	This is based on CONTP() in REC533
	 
		For latitude = 3.4464, magnetic dip = 2.89 and month = 4 the diurnal 
		absorption exponent will be	p = 0.689
	 
	 */

	double ppt[12] = {30.0, 30.0, 30.0, 27.5, 32.5, 35.0, 37.5, 35.0, 32.5 , 30.0, 30.0, 30.0};

    double pval1[6][2][7] = {
        {
            {1.510, -0.353, -0.090, 0.191, 0.133, -0.067, -0.053},
            {1.400, -0.365, -1.212, -0.049, 1.187, 0.119, -0.400}
        },
        {
            {1.490, -0.348, -0.055, 0.164, 0.160, -0.041, -0.080},
            {1.450, -0.119, -0.913, -0.640, 0.347, 0.458, 0.107}
        },
        {
            {1.520, -0.410, -0.138, 0.308, 0.267, -0.113, -0.133},
            {1.500, -0.492, -0.958, 0.216, 0.267, -0.029, 0.187}
        },
        {
            {1.580, -0.129, -0.228, -0.192, 0.200, 0.116, -0.027},
            {1.530, -0.468, -1.312, 0.096, 0.973, 0.057, -0.187}
        },
        {
            {1.590, 0.002, -0.102, -0.579, -0.467, 0.522, 0.613},
            {1.490, -0.937, -1.622, 1.365, 1.720, -0.873, -0.453}
        },
        {
            {1.600, -0.060, -0.175, -0.037, 0.147, -0.008, -0.027},
            {1.460, -0.881, -1.595, 0.901, 2.133, -0.395, -0.933}
        }
    };

    double pval2[6][2][7] = {
        {
            {1.60, -0.030, -0.135, -0.137, 0.053, 0.072, 0.027},
            {1.43, -0.902, -1.667, 0.905, 2.480, -0.383, -1.173}
        },
        {
            {1.59, -0.032, -0.083, -0.119, 0.000, 0.031, 0.053},
            {1.46, -0.831, -1.653, 0.708, 2.320, -0.257, -1.067}
        },
        {
            {1.59, -0.060, -0.180, -0.181, 0.267, 0.081, -0.107},
            {1.51, -0.809, -1.740, 0.750, 2.240, -0.301, -0.960}
        },
        {
            {1.57, -0.189, -0.207, -0.005, 0.293, 0.004, -0.107},
            {1.52, -0.433, -1.015, -0.017, 0.440, 0.115, 0.080}
        },
        {
            {1.55, -0.292, -0.275, 0.093, 0.427, -0.026, -0.187},
            {1.44, -0.279, -0.770, -0.266, 0.053, 0.245, 0.267}
        },
        {
            {1.51, -0.347, -0.082, 0.160, 0.093, -0.048, -0.027},
            {1.40, -0.355, -1.212, -0.102, 1.187, 0.172, -0.400}
        }
    };

	
	double p;	// ?
	double PP;	// ?
	double SX;	// ?
	double A;	// ?
	double moddip; // Modified magnetic dip (or latitude) (degrees)

	int i,j;	// Loop indices

	// Initialize the exponent p
    p = 0.0;

	// Initialize the modified magnetic dip angle (degrees)
	moddip = fabs(atan2(CP.dip[HR100km], sqrt(cos(CP.L.lat))));
    
	if(moddip > 70.0*D2R) { 
		moddip = 70.0*D2R;
	}
    
	if(CP.L.lat < 0.0) {
		month = month + 6;
		if(month > 11) month = month - 12;
	}

    PP = ppt[month]*D2R;
    
	if(moddip > PP) {
		i = 1;
		moddip = -1.0 + 2.0*(moddip - PP)/(70.0*D2R - PP);
	}
	else {
		i = 0;
		moddip = -1.0 + 2.0*moddip/PP;
	}

    SX = 1.0;
    
	for(j=0; j<7; j++) {
		if(month <= 5) {
			A = pval1[month][i][j];
		}
		else {
			A = pval2[month-6][i][j];
		}

        p = p + A*SX;
		SX = SX*moddip;
	}

    // Testing
	if(BARF) {
		printf("\nMSFSS: p %f CP.L.lat %f CP.L.lng %f \n", p, CP.L.lat, CP.L.lng);
	}
    // Testing

	return p;
		
	}

double AbsorptionFactor(struct ControlPt CP, int month) {

	/*

	  AbsorptionFactor() Calculates the absorption factor ATnoon as shown Figure 1 ITU-R P.533-14
			("absorption factor at local noon for the j-th penetration point and R12 = 0 given as a
			function of geographic latitude and month from Fig. 1"), used in equation (20).
			The figure is held as a table ATNO[9][29]: nine month columns and 29 latitudes from 0 to
			70 degrees in 2.5 degree steps, linearly interpolated in latitude. May and August share a
			column, as do June and July, and December and January (see the month map below). The
			values come from REC533 and cannot be checked against the figure's text.

	 		INPUT
	 			struct ControlPt CP - Control point of interest: L.lat (radians)
	 			int month - 0-based month index; shifted by six months for southern latitudes (the
						Southern month axis of Fig. 1)

	 		OUTPUT
	 			returns ATnoon - The absorption factor at local noon and R12 = 0.

			NOTES
				|latitude| is limited to just below 70 degrees (the "70-90" top of Fig. 1).
	 
	 		SUBROUTINES
				None
	 
	 This routine is based on CONTAT() in REC533.
	 
	 */

	double X;		// ?
	double ATnoon;	// Absorption factor at local noon and R12 = 0 

	int i;			// Month index
	int j;			// Index

    double ATNO[9][29] = {
        {
            323.9, 297.5, 274.5, 256.4, 244.2, 235.0, 229.5, 226.1, 226.8, // Win
            229.0, 232.5, 237.0, 243.4, 249.9, 258.1, 267.5, 277.5, 283.3, 283.2, // Win
            273.1, 257.0, 232.1, 201.4, 171.5, 146.0, 123.0, 103.1, 83.0, 66.6 // Win
        }, 
        {
            312.1, 285.1, 263.1, 251.8, 249.5, 250.9, 254.5, 260.3, 266.7, 272.3, // Feb
            277.8, 280.3, 283.9, 284.5, 284.4, 283.0, 278.6, 273.0, 265.7, 256.3, // Feb
            244.8, 232.0, 218.1, 204.5, 189.9, 172.3, 155.3, 135.5, 116.2 // Feb
        },
        {
            347.7, 321.9, 302.5, 293.8, 291.4, 289.3, 292.1, 296.6, 304.3, 313.0, // Mar
            321.7, 333.8, 342.6, 349.6, 355.2, 355.6, 352.2, 341.7, 327.3, 308.4, // Mar
            286.0, 265.0, 244.1, 223.8, 202.8, 181.8, 160.8, 141.6, 123.4 // Mar
        },
        {
            338.0, 313.2, 297.0, 290.2, 292.1, 299.4, 308.0, 320.4, 331.6, 340.7, // Apr
            347.8, 353.8, 357.0, 360.0, 359.8, 358.3, 355.8, 350.8, 344.5, 332.7, // Apr
            316.4, 292.5, 266.1, 236.4, 214.0, 193.8, 177.5, 165.0, 155.9 // Apr
        },
        {
            328.1, 303.8, 287.7, 282.5, 284.4, 289.4, 294.8, 303.6, 312.9, 322.7, // Su_Eq
            332.3, 343.8, 350.6, 358.7, 364.3, 365.8, 362.4, 356.0, 346.7, 333.0, // Su_Eq
            318.8, 299.7, 282.1, 260.5, 240.5, 220.6, 203.9, 186.3, 173.0 // Su_Eq
        },
        {
            305.1, 288.5, 275.2, 273.7, 278.6, 288.9, 302.5, 319.3, 333.6, 346.3, // Summ
            356.3, 364.7, 371.7, 373.6, 374.2, 373.1, 370.5, 365.1, 358.5, 347.7, // Summ
            335.0, 320.3, 299.1, 276.6, 253.2, 230.7, 214.0, 196.6, 185.3 // Summ
        },
        {
            345.4, 319.4, 298.7, 290.1, 290.0, 291.8, 296.3, 302.9, 312.1, 320.1, // Sep
            327.8, 334.1, 340.2, 343.3, 345.7, 346.5, 345.3, 341.1, 334.5, 321.7, // Sep
            304.2, 286.8, 265.9, 244.8, 224.1, 204.5, 183.6, 164.1, 145.2 // Sep
        },
        {
            341.9, 314.8, 295.3, 277.9, 265.0, 258.2, 254.4, 255.8, 257.3, 262.9, // Oct
            268.5, 279.0, 287.5, 295.2, 299.6, 300.2, 298.9, 291.5, 279.0, 262.6, // Oct
            245.7, 227.0, 203.6, 182.3, 163.2, 147.1, 133.9, 119.9, 110.8 // Oct
        },
        {
            318.8, 293.3, 268.3, 251.7, 240.4, 233.1, 229.4, 228.8, 230.5, 235.5, // Nov
            239.7, 242.6, 245.4, 247.5, 248.9, 249.9, 248.5, 244.4, 237.3, 225.6, // Nov
            213.5, 195.2, 172.7, 151.3, 131.1, 113.1, 100.1, 89.0, 80.0 // Nov
        }
    };

	/*
		Note: the month index into the array is as follows:
				Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
			i =  0   1   2   3   4   5   5   4   6   7   8   0
	 */
	i = month;

	// P.533-14 Fig. 1 has a second month axis for the Southern Hemisphere,
	// shifted by six months (Southern July reads the Northern January column).
	// DiurnalAbsorptionExponent() already applied this; this routine did not,
	// so southern ATnoon was read from the wrong season.
	if(CP.L.lat < 0.0) {
		i = (i + 6) % 12;
	}

	switch(i) {
		case JUL:
			i = 5;
			break;
		case AUG:
			i = 4;
			break;
		case SEP:
			i = 6;
			break;
		case OCT:
			i = 7;
			break;
		case NOV:
			i = 8;
			break;
		case DEC:
			i = 0;
			break;	
	}

    X = fabs(CP.L.lat*R2D);
	if(X >= 70.0) X = 69.99; // This is for the (int) casting of X so that j is not >= 28.
	X = X/2.5; 
	j = (int)X;
	X = X - j;
    ATnoon = ATNO[i][j+1]*X + ATNO[i][j]*(1.0-X);

	return ATnoon;
  }

double AbsorptionLayerPenetrationFactor(double T) {

	/*

	 	AbsorptionLayerPenetrationFactor() - Determines the absorption layer penetration factor
	 		phi_n(fv/foE) shown in Figure 2 P.533-14, used in equation (20).
			The curve is represented piecewise: polynomials for 0 <= T <= 1 and 1 < T <= 2.2 (each
			capped at 0.53), a linear decrease to 0.34 at T = 10, and 0.34 beyond; the result is
			divided by 0.34. The fits come from REC533 and cannot be checked against the figure's text.

	 		INPUT
	 			T - the ratio fv/foE of the vertical-incidence wave frequency (equation (22)) and foE
						(dimensionless)

	 		OUTPUT
	 			returns phi the absorption layer penetration factor (dimensionless); 0 for T < 0.
				A non-finite T (foE = 0 at the point) fails every range test, falls through to 0.34
				and gives 1.0.
	 
	 		SUBROUTINES
				None

		This routine is based on PHIFUN() in REC533.

	 */

	double X; 
	double phi;

	 if(T <= 1.0) {
		 if(T < 0.0) {
			phi = 0.0;
		 }
		 else { 
			X=(T-0.475)/0.475;
			phi=(((((-0.093*X+0.04)*X+0.127)*X-.027)*X+0.044)*X+0.159)*X+0.225;
			phi= min(phi,0.53);
		 }
     }
	 else {
		 if(T <= 2.2) {
			X=(T-1.65)/0.55;
			phi=(((((.043*X-.07)*X-.027)*X+.034)*X+.054)*X-.049)*X+.375;
			phi= min(phi,0.53);
		 }
		 else {
			 if(T <= 10.0) {
				X=T;
				phi=0.34+(((10.0-X)*0.02)/7.8);
			 }
			 else {
				phi=0.34;
			 }
         }
     }

    // Multiply by the scaling factor.
	 phi = phi/0.34;

	 return phi; 
}


double FindLh(struct ControlPt CP, double D, double hour, int month) {

	/*	
	 *	FindLh() - Finds the value of Lh from Table 2 ITU-R P.533-14 "Values of Lh giving auroral and other signal losses".
	 *
	 *		P.533-14 section 5.2: "Each value is evaluated in terms of the geomagnetic
	 *		latitude Gn ... and local time t ...: mean values for the control points of
	 *		Table 1d) are taken." The caller averages the values this returns for each
	 *		control point; the season is that of the control point's own hemisphere.
	 *
	 *		Table 2 a) and b) are for "transmission ranges" up to and beyond 2 500 km.
	 *		The Recommendation uses "range" for the path length throughout and "hop
	 *		length" for D/n, so the sub-table is chosen by the path length D. It was
	 *		chosen by the hop length, which gave the modes of one path different
	 *		sub-tables.
	 *
	 *		Used for Lh in equation (18). Gn is the geomagnetic latitude for "an Earth-centred
	 *		dipole with pole at 78.5 N, 68.2 W" (GeomagneticCoords()). "For Gn < 42.5 degrees,
	 *		Lh = 0 dB". Seasons: "In the Northern Hemisphere, winter is taken as December-February,
	 *		equinox as March-May and September-November and summer as June-August. In the Southern
	 *		Hemisphere, the months for winter and summer are interchanged" (WhatSeasonforLh()).
	 *
	 *		INPUT
	 *			struct ControlPt CP - one Table 1d) control point: L (radians)
	 *			double D - path length (km), the "transmission range"
	 *			double hour - mid-path local (mean) time t (hours, 0 <= t < 24), the same for every
	 *				control point of the path
	 *			int month - 0-based month index
	 *
	 *		OUTPUT
	 *			returns the value of Lh (dB) for this control point
	 *
	 *		NOTES
	 *			The eight columns are 01-04, 04-07, ..., 19-22 and 22-01 h, lower bound inclusive.
	 *			|Gn| is used; a bound such as 77.5 degrees belongs to the higher band (Gn >= 77.5
	 *			is row 0). The 22-01 h column of Table 2 a) is cut off in the plain-text copy of
	 *			P.533-14 used when this header was written, so those eight values per season were
	 *			not checked.
	 *
	 */
	 	
	 /*	
	  * The upper three blocks of the array 
	  *	Lh[Transmission range][Season][Mid-path local time][Geomagnetic Latitude] is for 
	  *	transmission ranges less than or equal to 2500 km.
	  *	The lower three blocks are for transmission ranges greater than 2500 km
	  *	Each block of [8][8] is for one of three seasons: Winter, Equinox and Summer
	  *	in each block eight columns are for the mid-path local time, t, in 3-hour increments
	  *				  Eight rows are for the geomagnetic latitude, Gn
	  */	
								
	// a) Transmission ranges less than or equal to 2500 km

    double Lh[2][3][8][8] = {
        {
            {
                // Winter 
                {2.0, 6.6, 6.2, 1.5, 0.5, 1.4, 1.5, 1.0},
                {3.4, 8.3, 8.6, 0.9, 0.5, 2.5, 3.0, 3.0},
                {6.2, 15.6, 12.8, 2.3, 1.5, 4.6, 7.0, 5.0},
                {7.0, 16.0, 14.0, 3.6, 2.0, 6.8, 9.8, 6.6},
                {2.0, 4.5, 6.6, 1.4, 0.8, 2.7, 3.0, 2.0},
                {1.3, 1.0, 3.2, 0.3, 0.4, 1.8, 2.3, 0.9},
                {0.9, 0.6, 2.2, 0.2, 0.2, 1.2, 1.5, 0.6},
                {0.4, 0.3, 1.1, 0.1, 0.1, 0.6, 0.7, 0.3}
            },
            {
                // Equinox
                {1.4, 2.5, 7.4, 3.8, 1.0, 2.4, 2.4, 3.3},
                {3.3, 11.0, 11.6, 5.1, 2.6, 4.0, 6.0, 7.0},
                {6.5, 12.0, 21.4, 8.5, 4.8, 6.0, 10.0, 13.7},
                {6.7, 11.2, 17.0, 9.0, 7.2, 9.0, 10.9, 15.0},
                {2.4, 4.4, 7.5, 5.0, 2.6, 4.8, 5.5, 6.1},
                {1.7, 2.0, 5.0, 3.0, 2.2, 4.0, 3.0, 4.0},
                {1.1, 1.3, 3.3, 2.0, 1.4, 2.6, 2.0, 2.6},
                {0.5, 0.6, 1.6, 1.0, 0.7, 1.3, 1.0, 1.3}
            },
            {
                //Summer
                {2.2, 2.7, 1.2, 2.3, 2.2, 3.8, 4.2, 3.8},
                {2.4, 3.0, 2.8, 3.0, 2.7, 4.2, 4.8, 4.5},
                {4.9, 4.2, 6.2, 4.5, 3.8, 5.4, 7.7, 7.2},
                {6.5, 4.8, 9.0, 6.0, 4.8, 9.1, 9.5, 8.9},
                {3.2, 2.7, 4.0, 3.0, 3.0, 6.5, 6.7, 5.0},
                {2.5, 1.8, 2.4, 2.3, 2.6, 5.0, 4.6, 4.0},
                {1.6, 1.2, 1.6, 1.5, 1.7, 3.3, 3.1, 2.6},
                {0.8, 0.6, 0.8, 0.7, 0.8, 1.6, 1.5, 1.3}
            }
        },
        {
            {
                // b) Transmission ranges greater than 2500 km
                // Winter
                {1.5, 2.7, 2.5, 0.8, 0.0, 0.9, 0.8, 1.6},
                {2.5, 4.5, 4.3, 0.8, 0.3, 1.6, 2.0, 4.8},
                {5.5, 5.0, 7.0, 1.9, 0.5, 3.0, 4.5, 9.6},
                {5.3, 7.0, 5.9, 2.0, 0.7, 4.0, 4.5, 10.0},
                {1.6, 2.4, 2.7, 0.6, 0.4, 1.7, 1.8, 3.5},
                {0.9, 1.0, 1.3, 0.1, 0.1, 1.0, 1.5, 1.4},
                {0.6, 0.6, 0.8, 0.1, 0.1, 0.6, 1.0, 0.5},
                {0.3, 0.3, 0.4, 0.0, 0.0, 0.3, 0.5, 0.4}
            },
            {
                // Equinox
                {1.0, 1.2, 2.7, 3.0, 0.6, 2.0, 2.3, 1.6},
                {1.8, 2.9, 4.1, 5.7, 1.5, 3.2, 5.6, 3.6},
                {3.7, 5.6, 7.7, 8.1, 3.5, 5.0, 9.5, 7.3},
                {3.9, 5.2, 7.6, 9.0, 5.0, 7.5, 10.0, 7.9},
                {1.4, 2.0, 3.2, 3.8, 1.8, 4.0, 5.4, 3.4},
                {0.9, 0.9, 1.8, 2.0, 1.3, 3.1, 2.7, 2.0},
                {0.6, 0.6, 1.2, 1.3, 0.8, 2.0, 1.8, 1.3},
                {0.3, 0.3, 0.6, 0.6, 0.4, 1.0, 0.9, 0.6}
            },
            {
                //Summer
                {1.9, 3.8, 2.2, 1.1, 2.1, 1.2, 2.3, 2.4},
                {1.9, 4.6, 2.9, 1.3, 2.2, 1.3, 2.8, 2.7},
                {4.4, 6.3, 5.9, 1.9, 3.3, 1.7, 4.4, 4.5},
                {5.5, 8.5, 7.6, 2.6, 4.2, 3.2, 5.5, 5.7},
                {2.8, 3.8, 3.7, 1.4, 2.7, 1.6, 4.5, 3.2},
                {2.2, 2.4, 2.2, 1.0, 2.2, 1.2, 4.4, 2.5},
                {1.4, 1.6, 1.4, 0.6, 1.4, 0.8, 2.9, 1.6},
                {0.7, 0.8, 0.7, 0.3, 0.7, 0.4, 1.4, 0.8}
            }
        }
    };

	int season;		// season index
	int txrange;	// transmitter range index

	//Initialise to prevent warning C4701: potentially uninitialized local variable 'mplt' used
	int mplt = 0;		// mid-path local time index

	//Initialise to prevent warning C4701: potentially uninitialized local variable 'mplt' used
	int gmlat = 0;		// geomagnetic latitude index

	struct Location Gn;

	// Initialize the geomagnetic coordinates.
	Gn.lat = 0.0;
	Gn.lng = 0.0;

	// Find the geomagnetic coordinates for location of the control point.
	GeomagneticCoords(CP.L, &Gn);

	// Determine the season index for the Lh array.
	season = WhatSeasonforLh(CP.L, month);

	// Lh[Transmission range][season][geomagnetic latitude][mid-path local time]
	// Determine the indices
	// Transmission range index, from the path length
	if(D <= 2500.0) {
		txrange = 0;
	}
	else { // (D > 2500.0)
		txrange = 1;
	}

    // Midpoint local time index
	if((1 <= hour) && (hour < 4)) {
		mplt = 0;	
	}
	else if((4 <= hour) && (hour < 7)) {
		mplt = 1;	
	}
	else if((7 <= hour) && (hour < 10)) {
		mplt = 2;	
	}
	else if((10 <= hour) && (hour < 13)) {
		mplt = 3;	
	}
	else if((13 <= hour) && (hour < 16)) {
		mplt = 4;	
	}
	else if((16 <= hour) && (hour < 19)) {
		mplt = 5;	
	}
	else if((19 <= hour) && (hour < 22)) {
		mplt = 6;	
	}
	else if((22 <= hour) || (hour < 1)) {
		mplt = 7;	
	}

    Gn.lat = fabs(Gn.lat);
	
	// geomagnetic index
	if(77.5*D2R <= Gn.lat) {
		gmlat = 0;
	}
	else if((72.5*D2R <= Gn.lat) && (Gn.lat < 77.5*D2R)) {
		gmlat = 1;
	}
	else if((67.5*D2R <= Gn.lat) && (Gn.lat < 72.5*D2R)) {
		gmlat = 2;
	}
	else if((62.5*D2R <= Gn.lat) && (Gn.lat < 67.5*D2R)) {
		gmlat = 3;
	}
	else if((57.5*D2R <= Gn.lat) && (Gn.lat < 62.5*D2R)) {
		gmlat = 4;
	}
	else if((52.5*D2R <= Gn.lat) && (Gn.lat < 57.5*D2R)) {
		gmlat = 5;
	}
	else if((47.5*D2R <= Gn.lat) && (Gn.lat < 52.5*D2R)) {
		gmlat = 6;
	}
	else if((42.5*D2R <= Gn.lat) && (Gn.lat < 47.5*D2R)) {
		gmlat = 7;
	}
	else if(42.5*D2R > Gn.lat) {
		return 0.0; // Nothing more to do return 0.0 as Lh
	}
	
	// Testing
	if(BARF) {
		printf(  "\nMSFSS: Lh[%d][%d][%d][%d] %f CP.L.lat %f CP.L.lng %f \n", txrange, season, gmlat, mplt, Lh[txrange][season][gmlat][mplt], CP.L.lat, CP.L.lng);
		printf("MSFSS: Geomag Lat %f (deg)\n", Gn.lat*R2D);
		printf("MSFSS: MidPath hour %f\n", hour);
	}
    // Testing

	// Lh[transmission range][season][geomagnetic latitude][mid-path local time]
	return Lh[txrange][season][gmlat][mplt];
	
}

int WhatSeasonforLh(struct Location L, int month) {

	/*
	 *	WhatSeasonforLh() - Determines the index into the Lh array dependent on the month and latitude.
	 *		P.533-14 section 5.2.2 (Lh, Table 2): "In the Northern Hemisphere, winter is taken as
	 *		December-February, equinox as March-May and September-November and summer as
	 *		June-August. In the Southern Hemisphere, the months for winter and summer are
	 *		interchanged." These seasons differ from those of the P.1239 decile tables
	 *		(WhatSeason() in InitializePath.c).
	 *
	 *		INPUT
	 *			struct Location L - location of interest (radians); lat >= 0 (including the
	 *				equator) is taken as the Northern Hemisphere
	 *			int month - 0-based month index
	 *
	 *		OUTPUT
	 *			returns the season: WINTER (0), EQUINOX (1) or SUMMER (2)
	 */

	//Initialise to prevent warning C4701: potentially uninitialized local variable 'season' used
	int season = WINTER;
	
	if(L.lat >= 0) { // Northern hemisphere and the equator
		switch (month) {
			case DEC: case JAN: case FEB: 
				season = WINTER;
				break;
			case MAR: case APR: case MAY: case SEP: case OCT: case NOV:  
				season = EQUINOX;
				break;
			case JUN: case JUL: case AUG: 
				season = SUMMER;
				break;
		}
    }
	else { // Southern hemisphere 
		switch (month) {
			case JUN: case JUL: case AUG:
				season = WINTER;
				break;
			case MAR: case APR: case MAY: case SEP: case OCT: case NOV:   
				season = EQUINOX;
				break;
			case DEC: case JAN: case FEB: 
				season = SUMMER;
				break;
		}
    }
	
	return season;
	
}

int SmallestCPfoF2(struct PathData path) {

	/*
	 
	 	SmallestCPfoF2() - Determines the Table 1c) control point with the smallest foF2, whose
			M(3000)F2 gives the F2 mirror-reflection height hr of equation (2) for paths from dmax to
			9000 km (P.533-14 section 5.2.1).

	 		INPUT
	 			struct PathData path - path.CP[Td02], CP[MP], CP[Rd02] with foF2 (MHz) set

	 		OUTPUT
				returns the index (Td02, MP or Rd02) of the control point with the smallest foF2;
				on a tie the first in that order

			SUBROUTINES
				None

	 */

	// P.533-14 section 5.2.1: for paths from dmax to 9000 km the F2 mirror-reflection
	// height hr is from equation (2) "at the control point given in Table 1c) for which
	// foF2 has the lower value", and Table 1c) lists T + d0/2, M and R - d0/2. This
	// searched all five control points (T + 1000 and R - 1000 as well) with a double
	// loop that is not a sort and could never return index 0 (the MATLAB port's D15).
	int cp[3] = {Td02, MP, Rd02};
	int i, low = cp[0];

	for(i=1; i<3; i++) {
		if(path.CP[cp[i]].foF2 < path.CP[low].foF2) low = cp[i];
	}

	return low;

}

double AntennaGain(struct PathData path, struct Antenna Ant, double delta, int direction) {

	/*
		AntennaGain() - Finds the antenna gain at the desired elevation, delta, and the great-circle
			azimuth of the path. Gives Gt of equation (17) and Grw of equation (43) (P.533-14 sections
			5.2.2 and 6). The pattern is bilinearly interpolated between whole degrees of azimuth and
			elevation; for a multi-frequency pattern the block whose frequency is nearest to
			path.frequency is used (no interpolation in frequency).

			INPUT
				struct PathData path - supplies L_tx, L_rx, SorL (short or long great circle) and
					frequency (MHz)
				struct Antenna Ant - pattern[freq][azimuth 0-359][elevation 0-90] (dBi)
				double delta - elevation angle (radians)
				int direction - TXTORX: azimuth is the bearing from tx to rx; RXTOTX: from rx to tx

			OUTPUT
				returns the interpolated antenna gain (dBi) at the desired elevation, delta

			NOTES
				delta is clamped to 0 - 90 degrees (see the comment in the body). Any other value of
				direction leaves the bearing 0 (north).

			SUBROUTINES
				Bearing()
				BilinearInterpolation()

	*/


	// The structure Antenna Ant is used to tell the subroutine which antenna to calculate.
	//Initialise to prevent warning C4701: potentially uninitialized local variable 'B' used
	double B = 0.0;			// Bearing from transmitter to receiver
	double c;				// fractional column (azimuth)
	double r;				// fractional row (elevation)
	double LL, LR, UR, UL;	// Neighboring gain values
	double G;				// Interpolated gain

	int deltaL, deltaU;		// Upper and lower elevation indices
	int BL, BR;				// Left and right bearing indices

  int i, freqIndex;
  double minFreqDelta, freqDelta;
  
  freqIndex = 0;
  /* If we have pattern data for multiple frequencies, find the index of the
   * frequency closest to the path.frequency.
   */
  if (Ant.freqn > 1) {
    minFreqDelta = DBL_MAX;
    for (i=0; i<Ant.freqn; i++) {
      freqDelta = fabs(Ant.freqs[i] - path.frequency);
      if (freqDelta < minFreqDelta) {
        minFreqDelta = freqDelta;
        freqIndex = i;
      }
    }
  }
  
	// The elevations and azimuths have to be in degrees because the antenna pattern is indexed in degrees.

	// delta is in radians convert to degrees
	delta = delta*R2D;

	// The pattern holds elevations 0 to 90 degrees only. The lowest-order F2 mode
	// is chosen for at least MINELEANGLES at the midpoint hr, but the field
	// strength recomputes the elevation with the hr of another control point,
	// which can be low enough to give a negative angle. Clamp to the pattern so
	// the indices below stay in range.
	delta = min(max(delta, 0.0), 90.0);

	// Determine the bearing
	// From the tx to rx.
	if (direction == TXTORX) {
		B = Bearing(path.L_tx, path.L_rx, path.SorL)*R2D; // degrees
	}
	else if (direction == RXTOTX) {
		B = Bearing(path.L_rx, path.L_tx, path.SorL)*R2D; // degrees
	}

	// Now determine the gain at the elevation, delta
	// Find the indices to determine the neighbors for the gain interpolation.
	deltaU = (int)ceil(delta);
	deltaL = (int)floor(delta);

	// The bearing might wrap around.
	BR = (int)ceil(B)%360;
	BL = (int)floor(B)%360;

	// Identify the neighbors.
	LL = Ant.pattern[freqIndex][BL][deltaL];
	LR = Ant.pattern[freqIndex][BR][deltaL];
	UL = Ant.pattern[freqIndex][BL][deltaU];
	UR = Ant.pattern[freqIndex][BR][deltaU];

	// Determine the fractional column and row.
	// The distance between indices is fixed at 1 degree.
	r = delta - (int)delta; // The fractional part of the row
	c = B - (int)B;			// The fractional part of the column

	G = BilinearInterpolation(LL, LR, UL, UR, r, c);

	/*
	printf("\nPath freq: %.3fMHz Pattern.freq: %.3fMHz (Index = %d)\n", path.frequency, Ant.freqs[freqIndex], freqIndex);
	printf("Bearing: %.3f deg Elevation: %.3fdeg\n", B, delta);
	printf("LL:%.3f LR: %.3f UL: %.3f UR: %.3f\n", LL, LR, UL, UR);
	printf("Gain:%.3f\n", G);
	*/
	
	return G;

}

void ZeroCP(struct ControlPt *CP) {

	/*

		ZeroCP() - Initializes (zeros) the control point. Not part of the Recommendation.

			INPUT
	 			struct ControlPt *CP

	 		OUTPUT
				*CP with L, distance, foE, foF2, M3kF2, fH[], dip[], all of Sun, ltime and hr set to
				0.0. The member x is not touched.

			SUBROUTINES
				None
		
	*/

	CP->L.lat = 0.0;
	CP->L.lng = 0.0;
	CP->distance = 0.0;
	CP->foE = 0.0;
	CP->foF2 = 0.0;
	CP->M3kF2 = 0.0;
	CP->fH[HR300km] = 0.0;
	CP->dip[HR300km] = 0.0;
	CP->fH[HR100km] = 0.0;
	CP->dip[HR100km] = 0.0;
	CP->Sun.lsn = 0.0;
	CP->Sun.lsr = 0.0;
	CP->Sun.lss = 0.0;
	CP->Sun.sza = 0.0;
	CP->Sun.decl = 0.0;
	CP->Sun.eot = 0.0;
	CP->Sun.sha = 0.0;
	CP->Sun.ha = 0.0;
	CP->ltime = 0.0;
	CP->hr = 0.0;
		
}

/*
	PenetrationPoints() - Evaluates the summand of P.533-14 equation (20) at the 90 km penetration
		points of one mode and returns their mean:
			(1/m) sum_j  ATjnoon F(chi_j)/F(chi_jnoon) phi_n(fv/foE_j) / (f + fLj)^2
		with m = 2(noh + 1) points ("two penetration points per hop"), located for a fixed
		reflection height of 300 km and a penetration height of 90 km with the hop length D/(noh+1)
		(equation (13) for the elevation angle). fLj is equation (23) at each point
		(LongitudinalGyrofrequency()). The caller forms Li = (noh+1)(1 + 0.0067 R12) sec(i) * mean.

		INPUT
			struct PathData *path - path->distance (km), L_tx, L_rx, frequency f (MHz), month, and the
				fields CalculateCPParameters() needs to evaluate a point at the current hour
			double noh - the mode index n (the mode has noh+1 hops)
			double fv - vertical-incidence frequency f cos(i110) of the mode, equation (22) (MHz)

		OUTPUT
			returns the mean over the penetration points of ATj/(f + fLj)^2 (MHz^-2 times the units
			of ATnoon). This routine itself does not write to path.

		SUBROUTINES
			ElevationAngle(), IncidenceAngle(), ZeroCP(), GreatCirclePoint(),
			CalculateCPParameters(), AbsorptionTerm(), LongitudinalGyrofrequency()
*/
double PenetrationPoints(struct PathData * path, double noh, double fv) {

 	// The routine finds the penetration points as described initially in the long model
 	// As is done in the long model use the control points as penetration points
 	// There are twice as many penetration points as there are hops. 
	//
	// P.533-14 equation (20): "The penetration points are determined by assuming a
	// fixed reflection height of 300 km and a penetration height of 90 km (two
	// penetration points per hop)." This took the reflection height of the mode
	// instead -- 110 km for E modes, eq. (2)'s hr for F2 modes -- which moves the
	// E-mode points by several hundred km (the MATLAB port's D31).
	//
	// Equation (20) divides each point's term by (f + fLj)^2, with fLj "the value
	// of electron gyrofrequency, about the longitudinal component of the Earth's
	// magnetic field for a height of 100 km, determined at the j-th penetration
	// point" (23). The callers divided the mean of the terms by (f + fL)^2 with fL
	// averaged over the Table 1d) control points instead; on a 7 000 km southern
	// path at 2 MHz, where fLj runs from 0.42 to 1.38 MHz, that made Li 9.5 dB
	// low. The division is now made here, point by point, and the mean of
	// ATj/(f + fLj)^2 is returned.
	const double hr = 300.0;
 	
	struct ControlPt PP[2]; // Temp

	double dh;
	double delta;
	double aoi90;
	double phi;
	double dh90;
	//double fv;
	double ATSum = 0.0;
	double fracd;
	double f = path->frequency;

	int i;

 	// Hop distance
	dh = path->distance/(noh+1.0);  
 
 	// Determine the elevation angle
	delta = ElevationAngle(dh, hr);
 
 	// Determine the angle of incidence 90 km penetration points 
	aoi90 = IncidenceAngle(delta, 90.0); 
 
 	// Determine where the rays penetrate the 90 km height to calculate the dips.
 	// Find the 90-km height half-hop distance.
	phi = (PI/2.0 - delta - aoi90);
 	
	// Hop distance 
	dh90 = R0*phi;
 
 	// Vertical-incidence frequency
	//fv = path->frequency*cos(aoi90);
 
	ATSum = 0.0; // Declare and initialize the absorption term sum
	
	for(i=0; i <= noh; i++) { // 90-km penetration points 
 
 		// Calculate the penetration points in pairs because the fractional distance is
 		// determined a little different for each end.
		 
 		// Zero the elements of the two penetration points for ith hop
 		ZeroCP(&PP[TXEND]);
 		ZeroCP(&PP[RXEND]);

 		// There are two control points per hop.
 		// First the end nearest the tx for this hop.
		fracd = (i*dh + dh90)/path->distance;
 		GreatCirclePoint(path->L_tx, path->L_rx, &PP[TXEND], path->distance, fracd);
 		CalculateCPParameters(path, &PP[TXEND]);
 
		PP[TXEND].hr = 90.0;
 
 		// Calculate the absortion term for the ith hop penetration point
 		// closest to the transmitter and add it to the running absoption term sum	
 		ATSum += AbsorptionTerm(PP[TXEND], path->month, fv)/pow(f + LongitudinalGyrofrequency(PP[TXEND]), 2);
 			
 		// Next the end nearest to the receiver for this hop
 		fracd = ((i+1)*dh  - dh90)/path->distance;
 		GreatCirclePoint(path->L_tx, path->L_rx, &PP[RXEND], path->distance, fracd);
 		CalculateCPParameters(path, &PP[RXEND]);
 
 		PP[RXEND].hr = 90.0;
 
 		// Calculate the absortion term for the ith hop penetration point
 		// closest to the receiver and add it to the running absoption term sum	
 		ATSum += AbsorptionTerm(PP[RXEND], path->month, fv)/pow(f + LongitudinalGyrofrequency(PP[RXEND]), 2);


 		}

    // Return the mean of ATj/(f + fLj)^2 over all penetration points
	return ATSum/(2.0*(noh+1));
 
 }

/*
	LongitudinalGyrofrequency() - P.533-14 equation (23): fL = |fH sin(I)|, the
		electron gyrofrequency about the longitudinal component of the Earth's
		magnetic field at 100 km, at a point whose parameters are set.

		INPUT
			struct ControlPt P - a point after CalculateCPParameters()

		OUTPUT
			returns fL (MHz)
*/
static double LongitudinalGyrofrequency(struct ControlPt P) {

	return fabs(P.fH[HR100km]*sin(P.dip[HR100km]));

}

