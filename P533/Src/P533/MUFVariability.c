#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

void MUFVariability(struct PathData *path) {
	/*

	 	MUFVariability() - Calculates the F2 and E Fprob, 50% MUF (MUF50), 90% MUF (MUF90) and 10% MUF (MUF10) 
	 		in accordance with P.533-12
	 		Section 3.7 "The path operational MUF" and
	 		Section 3.6 "Within the month probability of ionospheric propagation support" 
	 
			Verified against P.533-14 section 3.6: MUF(50) is the median (basic) MUF of
			sections 3.3 and 3.5; for F2 modes the decile ratios deltal = MUF(90)/MUF(50) and
			deltau = MUF(10)/MUF(50) are from P.1239 Tables 2 and 3 (P.1239-4 section 3.2);
			for E modes they are 0.95 and 1.05; Fprob from equation (9) when f < MUF(50),
			equation (10) otherwise.

	 		INPUT
	 			struct PathData *path - reads distance (km), frequency (MHz), BMUF and the
					mode BMUFs and cpMUF (MUFBasic()), CP[MP], CP[Td02], CP[Rd02] (location,
					ltime = UTC hour), month, SSN and the foF2var decile tables

	 		OUTPUT
	 			path->MUF50 = Path basic MUF 50% of a month (MHz), = path->BMUF
	 			path->MUF90 = Path basic MUF 90% of a month (MHz)
	 			path->MUF10 = Path basic MUF 10% of a month (MHz)
				(MUF90 and MUF10 are those of the mode that sets MUF50: the lowest-order F2
				 mode when its BMUF is the path BMUF, else the lowest-order E mode; unchanged
				 when neither exists)
	 			path->Md_E[].MUF50 = E layer 50% of the month MUF
	 			path->Md_F2[].MUF50 = F2 Layer 50% of the month MUF
	 			path->Md_E[].MUF90 = E layer 90% of the month MUF
	 			path->Md_F2[].MUF90 = F2 Layer 90% of the month MUF
	 			path->Md_E[].MUF10, path->Md_F2[].MUF10 = 10% of the month MUF
	 			path->Md_F2[].deltal = F2 layer lower decile deviation of the MUF
	 			path->Md_F2[].deltau = F2 layer upper decile deviation of the MUF
	 			path->Md_E[].deltal = E layer lower decile deviation of the MUF
	 			path->Md_E[].deltau = E layer upper decile deviation of the MUF
					(deltal and deltau are ratios to MUF(50), not dB)
	 			path->Md_F2[].Fprob = F2 layer within the month probability of ionospheric propagation support
	 			path->Md_E[].Fprob = E layer within the month probability of ionospheric propagation support
					(Fprob is a probability 0 to 1)
				Only modes with BMUF != 0 are set. Nothing is set for D > 9000 km.

			NOTES
				The F2 decile factors of each F2 mode are read at the control point whose value
				set its basic MUF (Md_F2[].cpMUF: mid-path up to dmax; beyond dmax the Table 1a)
				point T + d0/2 or R - d0/2 whose equation (3) or (7) value was selected): its
				local mean time (LocalMeanTime()), its geographic latitude and the P.1239-4
				season of its hemisphere (WhatSeason()). P.1239-4 section 3.2 says "the local
				time and geographic latitude at the control point"; the choice of point is the
				owner's ruling.


			SUBROUTINES
				FindfoF2var()

	 */

	int i;		// Index
	int decile; // Decile flag
	int season; // P.1239-4 season of the decile control point
	struct ControlPt *cp; // Control point at which the F2 decile factors are read

	struct Mode *setter;		// The mode whose MUF(50) is the path MUF(50)

	
	// Only do this subroutine if the path is less than or equal to 9000 km if not exit
	if(path->distance > 9000) return; 
	
	// Section 3.65 seems to imply that the basic MUF for the path and the 50% MUF are the same so for this calculation set them equal
	path->MUF50 = path->BMUF;

	// There potentially are 3 E layer modes and 6 F2 layer modes

	// The F2 layer modes
	for(i=0; i<MAXF2MDS; i++) { // There are 6 F2 layer modes 
		if(path->Md_F2[i].BMUF != 0.0) { // If the Basic MUF is set, non-zero, the layer exists

			// Section 3.6 P.533-12 indicates that the basic MUF and MUF(50) are the same.
			path->Md_F2[i].MUF50 = path->Md_F2[i].BMUF;

			// P.1239-4 section 3.2: the decile factors are "for the local time and geographic
			// latitude at the control point". By the owner's ruling that is the control point
			// whose value set the mode's basic MUF (mid-path up to dmax, the selected Table 1a)
			// point beyond), with the season of that point's hemisphere.
			cp = &path->CP[path->Md_F2[i].cpMUF];
			season = WhatSeason(cp->L, path->month);

			decile = DL; // Lower decile
			path->Md_F2[i].deltal = FindfoF2var(*path, season, LocalMeanTime(*cp), cp->L.lat, decile);
				
			decile = DU; // Upper decile
			path->Md_F2[i].deltau = FindfoF2var(*path, season, LocalMeanTime(*cp), cp->L.lat, decile);

			// Find the other MUFs
			path->Md_F2[i].MUF10 = path->Md_F2[i].deltau*path->Md_F2[i].MUF50;
			path->Md_F2[i].MUF90 = path->Md_F2[i].deltal*path->Md_F2[i].MUF50;

			// Now determine the probability that the mode can be supported
			if(path->frequency < path->Md_F2[i].MUF50) {
				path->Md_F2[i].Fprob = min(1.3 - (0.8/(1.0+((1.0 - (path->frequency/path->Md_F2[i].MUF50))/(1.0 - path->Md_F2[i].deltal)))), 1.0);
				//printf("Eq.9  Freq=%.2f MUF50=%.2f deltal=%.2f Fprob=%.2f\n", path->frequency, path->Md_F2[i].MUF50, path->Md_F2[i].deltal, path->Md_F2[i].Fprob);
			}
			else { // (path->frequency >= path->Md_F2[i].MUF50)
				path->Md_F2[i].Fprob = max((0.8/(1.0 + (((path->frequency/path->Md_F2[i].MUF50) - 1.0)/(path->Md_F2[i].deltau - 1.0)))) - 0.3, 0.0);
				//printf("Eq.10 Freq=%.2f MUF50=%.2f deltau=%.2f Fprob=%.2f\n", path->frequency, path->Md_F2[i].MUF50, path->Md_F2[i].deltau, path->Md_F2[i].Fprob);
			}
		}
	}

	// The following loop is the same as that for the F2 layer but the decile values are fixed according to Section 3.6
	// The lower decile delta is 0.95 and the upper is 1.05/
	// Find the E layer MUFs
	for(i=0; i<MAXEMDS; i++) { // There are 6 F2 layer modes 
		if(path->Md_E[i].BMUF != 0.0) { // If the Basic MUF is set, non-zero, the layer exists

			// Section 3.6 P.533-12 indicates that the basic MUF and MUF(50) are the same.
			path->Md_E[i].MUF50 = path->Md_E[i].BMUF;
			
			// deltal for E layer
			path->Md_E[i].deltal = 0.95;

			// deltau for the E layer 
			path->Md_E[i].deltau = 1.05;

			// Find the other MUFs
			path->Md_E[i].MUF10 = path->Md_E[i].deltau*path->Md_E[i].MUF50;
			path->Md_E[i].MUF90 = path->Md_E[i].deltal*path->Md_E[i].MUF50;

			// Now determine the probability that the mode can be supported
			if(path->frequency < path->Md_E[i].MUF50) {
				path->Md_E[i].Fprob = min(1.3 - (0.8/(1.0 + ((1.0 - (path->frequency/path->Md_E[i].MUF50))/(1.0 - path->Md_E[i].deltal)))), 1.0);
			}
			else { // (path->frequency >= path->Md_F2[i].MUF50)
				path->Md_E[i].Fprob = max((0.8/(1.0 + (((path->frequency/path->Md_E[i].MUF50) - 1.0)/(path->Md_E[i].deltau - 1.0)))) - 0.3, 0.0);
			}
		}
	}

	// Path MUF(90) and MUF(10): section 3.6 gives deciles per mode only. By the owner's
	// ruling they are those of the mode that sets the path MUF(50) = BMUF (section 3.1):
	// the lowest-order F2 mode when its basic MUF is the path basic MUF, otherwise the
	// lowest-order E mode, as for the path operational MUF (MUFOperational()). They were
	// the largest MUF90 and the largest MUF10 over all modes, taken separately.
	if((path->n0_F2 != NOLOWESTMODE) && (path->Md_F2[path->n0_F2].BMUF >= path->BMUF)) {
		setter = &path->Md_F2[path->n0_F2];
	}
	else if(path->n0_E != NOLOWESTMODE) {
		setter = &path->Md_E[path->n0_E];
	}
	else {
		return; // No mode: the path deciles keep their initial values
	}
	path->MUF90 = setter->MUF90;
	path->MUF10 = setter->MUF10;

	return;

}

double FindfoF2var(struct PathData path, int season, double hour, double lat, int decile) { 

	/*

	  FindfoF2var() - Determines the variation in foF2 given the season, hour, latitude ss and decile
	 		This routine uses the bilinear interpolation method in ITU-R P.1144-5
	 
			The table is P.1239-4 Table 2 (lower decile) or Table 3 (upper decile), "for the
			local time and geographic latitude at the control point", in three ranges of R12
			and three seasons (P.1239-4 section 3.2), interpolated bilinearly in latitude
			(5 degree rows) and hour (1 hour columns) as section 3.2 permits.

	 		INPUT
	 			struct PathData path - reads path.SSN and
					path.foF2var[season][hour 0-23][|lat| index 0-18 in 5 deg steps][R12 range][decile]
	 			int season - P.1239-4 season (WINTER/EQUINOX/SUMMER) of the point's hemisphere,
					from WhatSeason()
	 			double hour - Hour of interest: local time, hours 0 to < 24 (callers pass
					LocalMeanTime()); hour 23 to 24 interpolates towards hour 0
	 			double lat - Latitude of interest (radians); only |lat| is used, the tables
					being given for latitude without hemisphere
	 			int decile - Upper or lower decile index: DL (0, Table 2) or DU (1, Table 3)

	 		OUTPUT
	 			return interpolated value: the decile ratio foF2(decile)/foF2(median),
				dimensionless

			NOTES
				R12 range index: R12 < 50 -> 0, 50 <= R12 <= 100 -> 1, R12 > 100 -> 2, matching
				the P.1239-4 table headings. The negative-index and above-18 rollovers in the
				body cannot occur for |lat| <= 90 deg and are kept only as guards.

			SUBROUTINES
				BilinearInterpolation()
	 
	 */

	double UR, UL, LL, LR; // Neighboring values

	int latU, latL; // lower and upper lat
	int hourU, hourL; // lower and upper hour
	int ssn;		// sunspot number index

	double c; // Fractional column
	double r; // Fractional row

	double Irc; // Interpolated value

	lat = fabs(lat/(5.0*D2R)); // 5 degree increments

	// Determine the fractional column and row
	// The distance between indices is 1.0
	r = lat - (int)lat; // The fractional part of the row
	c = hour - (int)hour; // The fractional part of the column

	latL = (int)floor(lat);
	latU = (int)ceil(lat);
	
	if(latL < 0) {
		latL = 18; // rollunder
		// The sense of the fractional row is reversed when negative so fix it
		r = 1.0 - r;
	}
    if(latU > 18) {
		latU = 0; // rollover 
	}

    hourL = (int)floor(hour);
	hourU = (int)ceil(hour);

	if(hourL < 0) {
		hourL = 23; // rollunder
		// The sense of the fractional column is reversed when negative so fix it
		c = 1.0 - c;
	}

    if(hourU > 23) {
		hourU = 0; // rollover hour
	}

    // Determine the sunspot number index ssn. 
	if(path.SSN < 50) {
		ssn = 0;
	}
	else if((50 <= path.SSN) && (path.SSN <=100)) { 
		ssn = 1;	
	}
	else { // path-SSN > 100
		ssn = 2;
	}

    // Find the neighbors
	LL = path.foF2var[season][hourL][latL][ssn][decile];
	LR = path.foF2var[season][hourU][latL][ssn][decile];
	UL = path.foF2var[season][hourL][latU][ssn][decile];
	UR = path.foF2var[season][hourU][latU][ssn][decile];

	Irc = BilinearInterpolation(LL, LR, UL, UR, r, c);

	return Irc;

}

