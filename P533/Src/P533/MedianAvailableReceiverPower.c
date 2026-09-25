#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End local includes

// Local prototypes
void DominantMode(struct PathData *path);
double SumModePowers(struct PathData *path, int dominant);
// End local prototypes

void MedianAvailableReceiverPower(struct PathData *path) {

	/*	
	  MedianAvailableReceiverPower() - Calculates the median available receiver power Pr (dBW) by the
	 		method in ITU-R P.533-14 section 6 "Median available receiver power":
				D <= 7000 km:        equation (43) for each mode considered in section 5.2.1,
				                     Prw = Ew + Grw - 20 log10 f - 107.2, with Grw the receive gain at
				                     that mode's elevation angle, power-summed by equation (44).
				7000 < D < 9000 km:  "the power is determined from equation (42) using the powers
				                     corresponding to Es and El": the equation (44) sum of the short
				                     modes (Ps) and equation (43) applied to El with the largest receive
				                     gain at 0 - 8 degrees (Pl), interpolated as in equation (42),
				                     Pr = 100 log10(Xs + ((D - 7000)/2000)(Xl - Xs)), X = 10^(0.01 P).
				D >= 9000 km:        equation (43) with Ew = El and Grw the largest receive gain at the
				                     required azimuth in the elevation range 0 - 8 degrees.
	 
	 		INPUT
	 			struct PathData *path
					path->distance - path length D (km)
					path->frequency - f (MHz)
					path->Md_E[], path->Md_F2[] - Ew (dB(1 uV/m)) and elevation angle (radians) of each mode
					path->Es, path->Ei, path->El - field strengths from sections 5.2, 5.4, 5.3 (dB(1 uV/m))
					path->A_rx - receive antenna pattern (dBi)
	 
	 		OUTPUT
	 			path->Md_F2[].Prw, path->Md_E[].Prw - mode available power (dBW), equation (43), and
				path->Md_F2[].Grw, path->Md_E[].Grw - the receive gain used (dBi); set for D < 9000 km
	 			path->Pr - Median available receiver power (dBW). TINYDB (-307) when D <= 7000 km and
						   no mode is available (no E mode and every F2 mode screened).
	 			path->Ep - The path field strength (dB(1 uV/m)): Es, Ei or El by distance.
				path->Grw, path->ele - receive gain (dBi) and elevation angle (radians): those of the
						   dominant mode for D <= 7000 km (DominantMode()), otherwise the 0 - 8 degree
						   maximum and the elevation (whole degree, in radians) at which it occurs.
				path->DMptr, path->DMidx - dominant (strongest Prw) mode, D <= 7000 km only.
	 
			NOTES
				The distance bands are D <= 7000, 7000 < D < 9000 and D >= 9000 km; section 6 does not
				assign the boundary values themselves, this code puts 7000 km with the short method and
				9000 km with the long one. No dominant mode is set for D > 7000 km.

			SUBROUTINE
				AntennaGain()
				DominantMode()
				AntennaGain08()
				SumModePowers()
	 
	 */
	
	double SumPr;	// Summation of individual mode powers
	double Grw;		// Mode gain paths < 9000 km and the overall receiver gain paths > 9000 km
	double Ps, Pl;	// Short- and long-model powers for 7000 - 9000 km (dBW)
	double Xs, Xl;	// Their equation (42) terms
	
	double elevation;	// Antenna elevation

	// Intialize 
	elevation = 2.0*PI;

	// In each case the receiver gain, Grw, must be determined. Grw is calculated at the receiver elevation angles for 
	// paths less than 9000 km. While for paths >= 9000 km the largest gain between 0 and 8 degrees is used.

	if(path->distance <= 7000.0) {
		
		// For each mode power to the total received power sum as given in Eqn (44) P.533-14
		// This can be done as each mode power is calculated
		SumPr = SumModePowers(path, TRUE);

        // Now that the modes are calculated, set the path parameters. 
		// Find the total received power. 
		// If the SumPr is 0 then set the path->Pr to something small
		if((SumPr != 0.0) && (path->DMptr != NULL)) {
			path->Pr = 10.0*log10(SumPr);
			// The dominant mode is known so set any values in the path structure that are relevant.
			DominantMode(path);
		}
		else {
			// There are no E modes and all the F2 modes are screened
			path->Pr = TINYDB;
		}

        // Save the path field strength. For this distance select Es, which includes E layer screening.
		path->Ep = path->Es;

	}
	else if((7000.0 < path->distance) && (path->distance < 9000.0)) {
		// Determine the receiver gain.
		Grw = AntennaGain08(*path, path->A_rx, RXTOTX, &elevation);

		// P.533-14 section 6: "In the intermediate range 7 000 to 9 000 km, the power
		// is determined from equation (42) using the powers corresponding to Es and
		// El": the power sum of the short-path modes, each with its own receive gain
		// (equation (44)), and El with the largest 0-8 degree gain (equation (43)),
		// interpolated as equation (42) interpolates Es and El. This interpolated
		// the field strengths and then applied the single 0-8 degree gain, so the
		// short-path end never saw the receive gains of its own modes. No dominant
		// mode is set here, as before.
		SumPr = SumModePowers(path, FALSE);
		Ps = (SumPr > 0.0) ? 10.0*log10(SumPr) : TINYDB;
		Pl = path->El + Grw - 20.0*log10(path->frequency) - 107.2;
		Xs = pow(10.0, Ps/100.0);
		Xl = pow(10.0, Pl/100.0);
		path->Pr = 100.0*log10(Xs + ((path->distance - 7000.0)/2000.0)*(Xl - Xs));

		// The path receiver gain Grw.
		path->Grw = Grw;

		// Save the path field strength, which at this distance is Ei.
		path->Ep = path->Ei;

		// Set the rx antenna elevation to the  long path rx elevation
		path->ele = elevation;

	}
	else { // path->distance >= 9000.0)
		// Determine the receiver gain.
		Grw = AntennaGain08(*path, path->A_rx, RXTOTX, &elevation);

		// Use the combined mode power, El, and the antenna gain between 0 and 8 degrees, Grw.
		path->Pr = path->El + Grw - 20.0*log10(path->frequency) - 107.2;

		// The path receiver gain Grw.
		path->Grw = Grw;

		// Save the path field strength, which at this distance is El.
		path->Ep = path->El;

		// Set the rx antenna elevation to the  long path rx elevation
		path->ele = elevation;
	}

    return;
}

void DominantMode(struct PathData *path) {

	/* 

	  DominantMode() - Stores values that are associated with the dominant mode
	 		to the path structure. These are strictly not part of the standard 
	 		P.533-14 but are provided for continuity in the analysis. The dominant mode
			is the mode with the largest available power Prw (equation (43)).
	 
	 		INPUT
	 			struct PathData *path
					path->DMptr - must point at the dominant mode (set by SumModePowers() with
								  dominant == TRUE); it is dereferenced without a NULL check
	 
	 		OUTPUT
	 			path->Grw - receive antenna gain of the dominant mode (dBi)
	 			path->ele - elevation angle of the dominant mode (radians)
	 
			SUBROUTINES
				None

	 */

		// Select the dominant mode to represent the median path behavior.
		// The path receiver gain is the dominant mode receiver gain.
		path->Grw = path->DMptr->Grw;

		// The path elevation angle is the dominant mode elevation angle.
		path->ele = path->DMptr->ele;

}

double SumModePowers(struct PathData *path, int dominant) {

	/*
	  SumModePowers() - Sets the available power Prw of each mode considered in section 5.2.1
			(equation (43)) and returns the sum of their powers, the argument of equation (44).
			When dominant is TRUE it also points path->DMptr at the strongest mode.
			P.533-14 section 6, equations (43) and (44); mode selection per section 5.2.1.

			INPUT
				struct PathData *path
				int dominant - TRUE to record the strongest mode in path->DMptr and path->DMidx

			OUTPUT
				returns sum over the modes of 10^(Prw/10) (linear, W relative to 1 W); 0.0 when no
					mode qualifies. The caller takes 10 log10() for Pr in dBW.
				path->Md_E[i].Grw, .Prw and path->Md_F2[i].Grw, .Prw for each mode summed
				path->DMptr, path->DMidx (dominant == TRUE only). DMidx is the E-mode slot i
					(0 .. MAXEMDS-1) or MAXEMDS + the F2-mode slot.

			NOTES
				The mode tests repeat those of MedianSkywaveFieldStrengthShort(): the lowest-order E
				mode with hop length <= 2000 km and higher E modes with a basic MUF; the lowest-order
				F2 mode with hop length <= dmax and higher F2 modes with a basic MUF, F2 modes only if
				their E-layer screening frequency fs is below the operating frequency (section 4).
				Slots with BMUF == 0.0 do not exist (see MUFBasic()).

			SUBROUTINES
				AntennaGain()
	 */

	double SumPr = 0.0;	// Summation of individual mode powers
	double Prw = TINYDB;	// Greatest receive mode power
	int i;


	// Calculate the available signal power Prw (dBW) for each mode from 
	// sky-wave field strength Ew (dB(1 µV/m)), frequency f (MHz) and Grw
	// lossless receiving antenna of gain.

	// Do any E-layer modes exist if so proceed
	// See "Modes considered" Section 5.2.1 P.533-14

	if(path->n0_E != NOLOWESTMODE) {
		for(i=path->n0_E; i<MAXEMDS; i++) {
			if(((i == path->n0_E) && (path->distance/(path->n0_E+1) <= 2000.0))
				                             ||
			   ((i != path->n0_E) && (path->Md_E[i].BMUF != 0.0))) {

				// Find the receiver gain for this mode.
				path->Md_E[i].Grw = AntennaGain(*path, path->A_rx, path->Md_E[i].ele, RXTOTX);

				path->Md_E[i].Prw = path->Md_E[i].Ew + path->Md_E[i].Grw 
									- 20.0*log10(path->frequency) - 107.2;

				// Determine if this is the greatest received power
				// If this is first time in the loop i == path->n0_E then initialize Prw
				if(dominant && (Prw < path->Md_E[i].Prw)){
					// Prw is the greatest power
					Prw = path->Md_E[i].Prw;

					// Point to the dominant mode and set the dominant mode index.
					path->DMptr = &path->Md_E[i];
					path->DMidx = i;

				}

                    // Add this mode to the sum.
				SumPr += pow(10.0, path->Md_E[i].Prw/10.0); 
			}
            }
        }
        // F2 modes
	// Do any F2-layer modes exist if so proceed
	if(path->n0_F2 != NOLOWESTMODE) {
		for(i=path->n0_F2; i<MAXF2MDS; i++) {
			if(((i == path->n0_F2) && (path->distance/(path->n0_F2+1) <= path->dmax) && (path->Md_F2[i].fs < path->frequency)) 
												   ||
			   ((i != path->n0_F2) && (path->Md_F2[i].BMUF != 0.0) && (path->Md_F2[i].fs < path->frequency))) {
				// Find the receiver gain for this mode.
				path->Md_F2[i].Grw = AntennaGain(*path, path->A_rx, path->Md_F2[i].ele, RXTOTX);

				path->Md_F2[i].Prw = path->Md_F2[i].Ew + path->Md_F2[i].Grw 
									- 20.0*log10(path->frequency) - 107.2;

				// Determine if this is the greatest received power.
				// If there was an E mode then Prw is already set to that power
				if(dominant && (Prw < path->Md_F2[i].Prw)) {
					// Prw is the greatest power.
					Prw = path->Md_F2[i].Prw;
				
					// Point to the dominant mode and set the dominant mode index.
					path->DMptr = &path->Md_F2[i];
					path->DMidx = i + MAXEMDS;

				}

                    // Add this mode to the sum.
				SumPr += pow(10.0, path->Md_F2[i].Prw/10.0); 
			}
            }
        }

	return SumPr;
}
