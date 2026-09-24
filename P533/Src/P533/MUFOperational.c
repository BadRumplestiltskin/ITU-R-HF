#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

void MUFOperational(struct PathData *path) {

	/*

	 	 MUFOperational() - Calculates the operational MUF from P.533-12 Section 3.7 "The path operational MUF".
	 
	 		INPUT
	 			struct PathData *path
	 
	 		OUTPUT
	 			path->Md_F2[].OPMUF - Operational MUF
	 			path->Md_F2[i].OPMUF10 - 10% of the month Operational MUF
	 			path->Md_F2[i].OPMUF90 - 90% of the month Operational MUF
	 			path->Md_E[].OPMUF - Operational MUF
	 			path->Md_E[i].OPMUF10 - 10% of the month Operational MUF
	 			path->Md_E[i].OPMUF90 - 90% of the month Operational MUF
	 			path->OPMUF - Path operational MUF
	 			path->OPMUF10 - Path operational MUF for 10% of the month 
	 			path->OPMUF90 - Path operational MUF for 90% of the month 

			SUBROUTINES
				None
	 
	 */


	// Initialize Table 1 ITU-R P.1240-1 "Ratio of the median operational MUF to the median
	// basic MUF for an F2-mode, Rop". 
    double Rop[2][3][2] = {
        {
            {1.20, 1.30},
            {1.15, 1.25},
            {1.10, 1.20}
        },
        {
            {1.25, 1.35},
            {1.20, 1.30},
            {1.15, 1.25}
        }
    };
	// P.1240-2 Table 1, indexed [EIRP <= 30 or > 30 dBW][WINTER, EQUINOX, SUMMER][DAY, NIGHT]:
	//     <= 30 dBW   summer 1.10/1.20, equinox 1.15/1.25, winter 1.20/1.30 (day/night)
	//      > 30 dBW   summer 1.15/1.25, equinox 1.20/1.30, winter 1.25/1.35
	// The > 30 dBW row had its winter and summer entries exchanged.
	
	double OPEMUF, OPEMUF10, OPEMUF90;		// E layer operational MUFs
	double OPF2MUF, OPF2MUF10, OPF2MUF90;	// F2 layer operational MUFa
	
	int time;	// Day/night index to the Rop array
	int power;	// Power index

	int i;		// Index
	
	// Only do this subroutine if the path is less than or equal to 9000 km if not exit
	if(path->distance > 9000) return; 

	// Determine if the time of interest is night or day at the mid-path point.
	// ltime, lsr and lss are all hours UTC folded into 0-24, so where sunset
	// in UTC falls before sunrise -- roughly beyond 90 degrees of longitude --
	// the day is the interval that wraps through midnight. The earlier test
	// required lsr < ltime < lss and so classed every such hour as night,
	// taking the night Rop by day. Without a sunrise at all (sha is NaN) the
	// Sun is up all day when the point and the Sun share a hemisphere.
	{
		double t = path->CP[MP].ltime, sr = path->CP[MP].Sun.lsr, ss = path->CP[MP].Sun.lss;
		int isday;
		if(isnan(path->CP[MP].Sun.sha)) {
			isday = (path->CP[MP].L.lat*path->CP[MP].Sun.decl > 0.0);
		}
		else if(sr < ss) {
			isday = (sr < t) && (t < ss);
		}
		else {
			isday = (t > sr) || (t < ss);
		}
		time = isday ? DAY : NIGHT;
	}

    // Determine the EIRP index power.
	// path->EIRP was never computed (it stayed at its TINYDB initial value) and
	// both branches here set power = 0, so the > 30 dBW row of P.1240 Table 1 was
	// never used and the operational MUF was 5 % low above 30 dBW e.i.r.p. The
	// e.i.r.p. is Pt (dB(1 kW) + 30 = dBW) plus the transmit antenna gain; P.1240
	// names no elevation for that gain, so it is taken along the ray of the
	// lowest-order F2 mode, the mode whose basic MUF Rop scales.
	path->EIRP = path->txpower + 30.0;
	if(path->n0_F2 != NOLOWESTMODE) {
		path->EIRP += AntennaGain(*path, path->A_tx,
					ElevationAngle(path->distance/(path->n0_F2 + 1.0), path->CP[MP].hr), TXTORX);
	}
	if(path->EIRP <= 30.0) {
		power = 0;
	}
	else { // (path->EIRP > 30.0)
		power = 1;
	}

    // Initialize the OPMUF extrema for the F2 Layer
	OPF2MUF = 0.0;
	OPF2MUF10 = 0.0;
	OPF2MUF90 = 0.0;

	// 6 F2 Modes OPMUF. 
	// Set the OPMUF for the ith F2 mode then determine if it are the largest for all existant F2 modes
	for(i=0; i<MAXF2MDS; i++) {
		if(path->Md_F2[i].BMUF != 0.0) {
			// The mode exists
			path->Md_F2[i].OPMUF = path->Md_F2[i].MUF50*Rop[power][path->season][time];
			path->Md_F2[i].OPMUF10 = path->Md_F2[i].OPMUF*path->Md_F2[i].deltau;
			path->Md_F2[i].OPMUF90 = path->Md_F2[i].OPMUF*path->Md_F2[i].deltal;

			// Now assume that the same procedure that applies to the basic MUF and variability MUF applies to the Operation MUF.
			// As was done in the MUFBasic(), determine the largest of the OPMUF, OPMUF10 and OPMUF90. 
			// Pick the MUF extrema for the path for the F2 layer.
			if(path->Md_F2[i].OPMUF > OPF2MUF) OPF2MUF = path->Md_F2[i].OPMUF;
			if(path->Md_F2[i].OPMUF90 > OPF2MUF90) OPF2MUF90 = path->Md_F2[i].OPMUF90;
			if(path->Md_F2[i].OPMUF10 > OPF2MUF10) OPF2MUF10 = path->Md_F2[i].OPMUF10;
		}
    }

    // Initialize the OPMUF extrema for the E Layers
	OPEMUF = 0.0;
	OPEMUF10 = 0.0;
	OPEMUF90 = 0.0;

	// 3 E Modes OPMUF. 
	// Set the OPMUF for the ith E mode then determine if it is the largest amongst all existant E modes
	for(i=0; i<MAXEMDS; i++) {
		if(path->Md_E[i].BMUF != 0.0) {
			// The mode exists
			path->Md_E[i].OPMUF = path->Md_E[i].BMUF;
			path->Md_E[i].OPMUF10 = path->Md_E[i].OPMUF*path->Md_E[i].deltau;
			path->Md_E[i].OPMUF90 = path->Md_E[i].OPMUF*path->Md_E[i].deltal;

			// Now assume that the same procedure that applies to the basic MUF and variability MUF applies to the Operation MUF.
			// As was done in the MUFBasic(), determine the largest of the OPMUF, OPMUF10 and OPMUF90. 
			// Pick the MUF extrema for the path for the E layer.
			if(path->Md_E[i].OPMUF > OPEMUF) OPEMUF = path->Md_E[i].OPMUF;
			if(path->Md_E[i].OPMUF90 > OPEMUF90) OPEMUF90 = path->Md_E[i].OPMUF90;
			if(path->Md_E[i].OPMUF10 > OPEMUF10) OPEMUF10 = path->Md_E[i].OPMUF10;
		}
    }

    // Now find the largest OPMUF of all existant modes
	path->OPMUF = max(OPEMUF, OPF2MUF); // largest OPMUF
	path->OPMUF90 = max(OPEMUF90, OPF2MUF90); // largest 90% OPMUF
	path->OPMUF10 = max(OPEMUF10, OPF2MUF10); // largest 10% OPMUF

	return;
	
}
