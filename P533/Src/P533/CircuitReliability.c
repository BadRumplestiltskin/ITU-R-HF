#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End Local Includes

// Local prototypes
void ModeSort(struct Mode *M[MAXMDS], int order[MAXMDS], int criteria);
int NumberofModes(struct PathData path);
double DigitalModulationSignalandInterferers(struct PathData *path, int iS[MAXMDS], int iI[MAXMDS]);
void EquatorialScattering(struct PathData *path, int iS[MAXMDS]);
double FindFlambdad(struct ControlPt CP);
double FindFTl(struct ControlPt CP);
// End local prototypes

// Local defines
// Flags for digital reliability calculation
#define DOMINANT		0
#define SOONEST			1
#define NOTINDEX		99
// End local defines

void CircuitReliability(struct PathData *path) {

	/*

	  CircuitReliability() finds the basic circuit reliability, BCR, and the overall circuit reliability, OCR,
	 		for analog and digital systems.
	 		
	 		The BCR is calculated as described in Table 1 ITU-R P.842-4. 
	 		The calculation is broken down into 11 steps. Many of the parameters necessary for the computation have already been 
	 		calculated elsewhere in this project. The median available receiver power of the wanted signal, which is 
	 		Step 1 in Table 1 P.842-4, is calculated in the subroutine MedianAvailableReceiverPower(). The subroutine 
	 		MedianAvailableReceiverPower() implements the calculation in P.533-12 Section 6 Median available receiver.
	 		Table 1 P.842-4 Steps 2, 5 and 8 are calculated in subroutine Noise() and summarized below.
	 
	 				 P.842-4 Table 1 Step
	 							2			Median noise factors for atmospheric, galactic and man-made noise
	 							5			Lower decile deviation of atmospheric, galactic and Man-made noise
	 							8			Upper decile deviation of atmospheric, galactic and Man-made noise
	 
	 		This subroutine, CircuitReliability(), completes the remaining steps in P.842-4 Table 1.
	 
	 		The simplified approximate BCR method from section 9 "BCR for digital modulation systems" P.842-4 is 
	 		determined from the three probabilities:
	 				i) Probability that the required signal-to-noise ratio, SN0, is achieved
	 				ii) Probability that the required time spread, T0, at a level of -10 dB relative to the 
	 					peak signal amplitude is not exceeded
	 				iii) Probability that the required frequency dispersion f0 at a level of -10 dB relative to the 
	 					peak signal amplitude is not exceeded
	 
	 		This subroutine also determines the overall circuit reliability, OCR, via the method given in ITU-R P.P.842-4 Table 3
	 		for digital systems. 
	  
	 		This subroutine also determines the equatorial scattering occurrence probability and then calculates the OCR in the 
	 		presence of scattering
	 
	 			INPUT
	 				struct PathData *path
	 
	 			OUTPUT
	 				path->SNR - Signal-to-noise ratio
	 				path->DuSN - Upper decile deviation signal-to-noise ratio
	 				path->DlSN - Lower decile deviation signal-to-noise ratio
					path->SNRXX - Signal-to-noise ratio at the desired reliability 
	 				path->SIR - Signal-to-interference ratio
	 				path->DuSI - Upper decile deviation signal-to-interference ratio
	 				path->DlSI - Lower decile deviation signal-to-interference ratio
	 				path->BCR - Basic circuit reliability
	 				path->OCR - Overall circuit reliability without scattering
	 				path->OCRs - Overall circuit reliability with scattering
	 				path->MIR - Multimode interference ratio
	 				path->RSN - Probability that the required signal-to-noise ratio, SN0, is achieved 
	 				path->RT - Probability that the required time spread, T0, is not exceeded
	 				path->RF - Probability that the required frequency dispersion is not exceeded.
	 
	 				via DigitalModulationSignalandInterferers()
	 				path->Md_F2[].tau - F2 layer mode delay
	 				path->Md_E[].tau - E layer mode delay
	 
	 			SUBROUTINES
					DigitalModulationSignalandInterferers()
					GeomagneticCoords()
					EquatorialScattering()

	 */

	double x, y;	// Temps
	double S;
	double fBMUFR;	// Frequency to basic MUF ratio
	double DuSd;	// Signal upper decile deviation (day-to-day) (dB)
	double DlSd;	// Signal lower decile deviation (day-to-day) (dB)
	double DuSh;	// Signal upper decile deviation (hour-to-hour) (dB)
	double DlSh;	// Signal lower decile deviation (hour-to-hour) (dB)

	double DTu;		// Upper decile deviation of time spread (dB)
	double DTl;		// Lower decile deviation of time spread (dB)
	double DFu;		// Upper decile deviation of frequency spread (dB)
	double DFl;		// Lower decile deviation of frequency spread (dB)
	double DlIh;	// Lower decile deviation of interference (dB)
	double DuIh;	// Upper decile deviation of interference (dB)
	double Tm;		// Time spread
	double Fm;		// Frequency spread
	double D;		// path distance
	double Isum;	// Interference sum
	double Isuml;	// Interference sum lower decile
	double Isumu;	// Interference sum upper decile
	double ICR;		// Circuit reliability in the presence of interference only (%)

	// NORM is an array of independent variables that give 
	// the normal cdf from 0.5 to 0.99 in 0.01 increments
	// with a standard deviataion of one and mean zero
	double NORM[50] = { 0.0000000000, 0.0250689082, 0.0501535835, 0.075269862, 0.1004337206,
						0.1256613469, 0.1509692155, 0.1763741647, 0.201893479, 0.2275449764,
						0.2533471029, 0.2793190341, 0.3054807878, 0.331853346, 0.3584587930,
						0.3853204663, 0.4124631294, 0.4399131658, 0.467698799, 0.4958503478,
						0.5244005133, 0.5533847202, 0.5828415079, 0.612812991, 0.6433454057,
						0.6744897502, 0.7063025626, 0.7388468486, 0.772193213, 0.8064212461,
						0.8416212327, 0.8778962945, 0.9153650877, 0.954165253, 0.9944578841,
						1.036433391,  1.080319342,  1.12639113,   1.174986792, 1.226528119,
						1.281551564,  1.340755033,  1.405071561,  1.47579103,  1.554773595,
						1.644853625,  1.750686073,  1.880793606,  2.053748909, 2.326347874 };

	int n;
	int iI[MAXMDS];		// Interfence mode indicies
	int iS[MAXMDS];		// Signal mode indicies

	// Table 2 P.842-4
	double Table2LD[2][10] = {{  8.0, 12.0, 13.0, 10.0,  8.0,  8.0,  8.0, 7.0, 6.0, 5.0},
				              { 11.0, 16.0, 17.0, 13.0, 11.0, 11.0, 11.0, 9.0, 8.0, 7.0}};
	double Table2UD[2][10] = {{ 6.0,  8.0, 12.0, 13.0, 12.0, 9.0, 9.0, 8.0, 7.0, 7.0},
							  { 9.0, 11.0, 12.0, 13.0, 12.0, 9.0, 9.0, 8.0, 7.0, 7.0}};

	int gt60deg; // Index for greater than 60 degrees geomagnetic latitude
	int BMUF; // Basic MUF index for P.842-4 Table 2

	struct Location Geomag; // 

	// For readability
	struct NoiseParams noiseP = path->noiseP;

	// Begin BCR Calculation *********************************************************************

	// Step 1: "Median available receiver power of wanted signal (dBW)"
	//		See MedianAvailableReceiverPower()

	// Step 2: Median noise factor for atmospheric noise, galactic and man-made noise
	//		See Noise() [P372.dll]

	// Step 3: "Median resultant signal-to-noise ratio (dB) for bandwidth b (Hz)"
	// There is a difference between digial and analog modulation in how the signal is found
	// Once the signal is found for each modulation the BCR calculation is identical
	if(path->Modulation == ANALOG) {
		S = path->Pr; 
	}
	else { // path->Modulation == DIGITAL
		// The signal for digital modulation requires the calculation given in 
		// ITU-R P.533-12 Section 10.2.3 Reliability prediction procedure.
		S = DigitalModulationSignalandInterferers(path, iS, iI);
	}

    // Calculate the SNR
	path->SNR = S - 10.0*log10(pow(10.0, (noiseP.FaA/10.0)) + pow(10.0, (noiseP.FaM/10.0)) + pow(10.0, (noiseP.FaG/10.0)))
		          - 10.0*log10(path->BW) + 204;

	// Step 4 & 7: "Signal upper decile deviation (day-to-day) (dB)" & "Signal lower decile deviation (day-to-day) (dB)"		

	// Determine if the path crosses 60 degrees 
	// From note 1 Table 2 P.842.4
	// If any point which "lies between control points located 1000 km from each end of the path" crosses 
	// geomagnetic latitude 60 degrees then identify it. 
	// P.842-5 Table 2 note (1): "If any point on that part of the great circle which
	// passes through the transmitter and the receiver and which lies between control
	// points located 1 000 km from each end of the path, reaches a geomagnetic
	// latitude of 60 deg or more, the values of >= 60 deg have to be used (see
	// Recommendation ITU-R P.1239, Fig. 2)." This tested only the mid-path and the two
	// 1000 km points, compared the signed latitude (so no southern path qualified),
	// and used the Lh dipole of P.533 (78.5 N, 68.2 W). The segment is now sampled
	// every 50 km at most, the magnitude is tested, and the pole is P.1239's
	// (78.3 N, 69.0 W). Paths of 2000 km or less have no such segment.
	gt60deg = 0; // 0 means less than 60 degrees 
	if(path->distance > 2000.0) {
		int k, nk = (int)ceil((path->distance - 2000.0)/50.0);
		struct ControlPt pt;
		for(k=0; (k<=nk) && !gt60deg; k++) {
			GreatCirclePoint(path->L_tx, path->L_rx, &pt, path->distance,
							 (1000.0 + k*(path->distance - 2000.0)/nk)/path->distance);
			Geomag.lat = asin(sin(pt.L.lat)*sin(78.3*D2R) + cos(pt.L.lat)*cos(78.3*D2R)*cos(pt.L.lng + 69.0*D2R));
			if(fabs(Geomag.lat) >= 60.0*D2R) gt60deg = 1; // 1 means a point reaches 60 degrees
		}
	}

    // Find the basic MUF index to retrieve the values from P.842-4 Table 2
	fBMUFR = path->frequency/path->BMUF;

	BMUF = 9; // Initialize just in case

	if(0.8 >= fBMUFR) {
		BMUF = 0;
	}
	else if((0.8 < fBMUFR) && (fBMUFR <= 1.0)) {
		BMUF = 1;
	}
	else if((1.0 < fBMUFR) && (fBMUFR <= 1.2)) {
		BMUF = 2;
	}
	else if((1.2 < fBMUFR) && (fBMUFR <= 1.4)) {
		BMUF = 3;
	}
	else if((1.4 < fBMUFR) && (fBMUFR <= 1.6)) {
		BMUF = 4;
	}
	else if((1.6 < fBMUFR) && (fBMUFR <= 1.8)) {
		BMUF = 5;
	}
	else if((1.8 < fBMUFR) && (fBMUFR <= 2.0)) {
		BMUF = 6;
	}
	else if((2.0 < fBMUFR) && (fBMUFR <= 3.0)) {
		BMUF = 7;
	}
	else if((3.0 < fBMUFR) && (fBMUFR <= 4.0)) {
		BMUF = 8;
	}
	else if(4.0 < fBMUFR) {
		BMUF = 9;
	}

    // Deciles day-to-day
	DlSd = Table2LD[gt60deg][BMUF];
	DuSd = Table2UD[gt60deg][BMUF];

	// Deciles hour-to-hour
	DuSh = 5.0;
	DlSh = 8.0;


	// Step 6: "Upper decile deviation of resultant signal-to-noise ratio (dB)"
	x = pow(10.0, (noiseP.FaA/10.0)) + pow(10.0, (noiseP.FaM/10.0)) + pow(10.0, (noiseP.FaG/10.0));
	y = pow(10.0, ((noiseP.FaA-noiseP.DlA)/10.0)) + pow(10.0, ((noiseP.FaM- noiseP.DlM)/10.0)) + pow(10.0, ((noiseP.FaG- noiseP.DlG)/10.0));

	path->DuSN = sqrt(pow(10.0*log10(x/y),2) + pow(DuSd,2) + pow(DuSh,2));

	// Step 9: "Upper decile deviation of resultant signal-to-noise ratio (dB)"
	// The value in variable x can be reused from Step 6 above.
	y = pow(10.0, ((noiseP.FaA+ noiseP.DuA)/10.0)) + pow(10.0, ((noiseP.FaM+ noiseP.DuM)/10.0)) + pow(10.0, ((noiseP.FaG+ noiseP.DuG)/10.0));

	path->DlSN = sqrt(pow(10.0*log10(y/x),2) + pow(DlSd,2) + pow(DlSh,2));

	// Step 11: "Basic circuit reliability for S/N >= or < S/Nr (%)"
	if(path->SNR >= path->SNRr) {
		path->BCR = min(130.0 - 80.0/(1.0 + ((path->SNR-path->SNRr)/path->DlSN)), 100.0);
	}
	else { // (SNR < path->SNRr)
		path->BCR = max(80.0/(1.0 + ((path->SNRr-path->SNR)/path->DuSN)) - 30.0, 0.0);
	}

    // End of BCR Calculation *********************************************************************

	// Begin simplified approximate BCR calculation for digital modulation systems ****************

	// The following calculation is based on a method found in ITU-R P.842-4 
	// Section 9: "BCR for digital modulation systems". 

	if((path->Modulation == DIGITAL) && ((path->T0 != 0.0) && (path->F0 != 0.0))) {
		// Note: The following equivalences
		//		SNR0 = path->SNRr
		//		SNRm = path->SNR
		//		Dl = path->DlSN
		//		Du = path->DuSN

		// Time spread
		D = path->distance; // for readability
		if(D <= 2000.0) {
			Tm = min((2.5e7*(1.0 - pow((path->frequency/path->BMUF),2))*pow(D,-2)), (7.0 - 0.00175*D));	
		}
		else { // path->distance <= 2000.0
			Tm = min((4.27e-2*(1.0 - pow((path->frequency/path->BMUF),2))*pow(D,0.65)), 3.5);	
		}

        // Frequency spread
		Fm = 0.02*path->frequency*Tm;

		// Time deciles
		DTu = 0.15*Tm;
		DTl = 0.15*Tm;

		// Frequency deciles
		DFu = 0.1*Fm;
		DFl = 0.1*Fm;

		// Probability that the required signal-to-noise ratio is achieved
		if(path->SNR >= path->SNRr) {
			path->RSN = min(130.0 - 80.0/(1.0 + ((path->SNR - path->SNRr)/path->DlSN)), 100.0);
		}
		else { // (path->SNR < path->SNRr)
			path->RSN = max((80.0/(1.0 + ((path->SNRr - path->SNR)/path->DuSN)) - 30.0), 0.0);
		}

        // Probability that the required time spread, T0, at a level of -10 dB relative to the peak 
		// signal amplitude 
		if(Tm >= path->T0) {
			path->RT = min((130.0 - 80.0/(1.0 + (path->T0 - Tm)/DTl)), 100.0);
		}
		else { // (Tm < path->T0)
			path->RT = max((80.0/(1.0 + (Tm - path->T0)/DTu) - 30.0), 0.0);
		}

        // Probability that the required frequency spread f0 at a level of -10 dB relative to the peak 
		// signal amplitude
		if(Fm >= path->F0) {
			path->RF = min((130.0 - 80.0/(1.0 + (path->F0 - Fm)/DFl)), 100.0);
		}
		else { // (Fm < path->F0)
			path->RF = max((80.0/(1.0 + (Fm - path->F0)/DFu) - 30.0), 0.0);
		}
    } // path->Modulation == DIGITAL

	// End simplified approximate BCR calculation for digital modulation systems ******************

	// Begin OCR Calculation **********************************************************************

	if ((path->Modulation == DIGITAL) && (path->distance <= 9000.0)) {

		// Table 3 P.842-4

		// There are three sums for this calculation that can be accomplished in one loop:
		//		i) The interference sum with the protection ratio
		//		ii) The interference sum with the protection ratio and upper decile deviation
		//		iii) The interference sum with the protection ratio and lower decile deviation

		// First set all the deviations
		// Step 5: Set all the day-to-day deciles to 0 dB
		// Step 6: Upper decile deviation of the wanted signal, DuSh, was set to 5 dB above
		// the lower decile deviations of the interfering signal is set to 8 dB
		DlIh = 8.0;

		// Step 9: Lower decile deviation of the wanted signal, DlSh, was set to 8 dB above
		// the lower decile deviations of the interfering signal is set to 8 dB
		DuIh = 5.0;

		// Prepare the sums that will be used in the following steps:
		//		Step 4: Median resultant signal-to-interference signal (dB)
		//		Step 7: Upper decile deviation of resultant signal-to-interference ratio (dB)
		//		Step 10: Lower decile deviation of resultant signal-to-interference ratio (dB)
		// Initialize the sums
		Isum = 0.0;  // The interference sum with the protection ratio
		Isumu = 0.0; // The interference sum with the protection ratio and upper decile deviation
		Isuml = 0.0; // The interference sum with the protection ratio and lower decile deviation
		// iI[] came from the routine DigitalModulationSignalandInterferers() which grouped 
		// all E and F2 layers together. Consequently the E and F2 layers will have to be determined
		// separately here. Note: In the F2 layer loop the index is offset by 3 for the 3 E layer modes.
		// P.842-5 Table 3 step 4: S/I = S - 10 log sum 10^((Ii + Ri)/10), and P.533-14
		// section 10.2.3 step 5 replaces "the relative protection ratios of Step 3 of
		// Table 3 by the ratio A", so each interferer enters as Ii + A. It entered as
		// Ii - A, which put S/I (and the decile sums of steps 7 and 10) 2A dB high.
		// E layer loop
		for(n=0; n<MAXMDS; n++) {
			if(iI[n] != NOTINDEX) {  //
				if(iI[n] < MAXEMDS) { // E mode interference
					Isum += pow(10.0, ((path->Md_E[iI[n]].Prw + path->A)/10.0));
					Isumu += pow(10.0, ((path->Md_E[iI[n]].Prw + path->A + DuIh)/10.0));
					Isuml += pow(10.0, ((path->Md_E[iI[n]].Prw + path->A - DlIh)/10.0));
				}
				else { // F2 mode interference 
					Isum += pow(10.0, ((path->Md_F2[iI[n]-3].Prw + path->A)/10.0));
					Isumu += pow(10.0, ((path->Md_F2[iI[n]-3].Prw + path->A + DuIh)/10.0));
					Isuml += pow(10.0, ((path->Md_F2[iI[n]-3].Prw + path->A - DlIh)/10.0));
				}
            }
        }

        // With no interfering mode there is no interference to fail against:
		// the interference-only reliability of P.842 Table 3 step 12 is 100 %,
		// so MIR = 100 and OCR = BCR. This used to set MIR = 0, which reported
		// OCR = 0 for every digital circuit whose modes all fell inside the
		// time window, and returned before the scattering and SNRXX steps.
		if(Isum == 0.0) {
			// S/I is unbounded with nothing to interfere; report it as -TINYDB
			// (+307 dB) rather than leave TINYDB, which reads as the worst case.
			path->SIR = -TINYDB;
			path->DuSI = DuSh;
			path->DlSI = DlSh;
			path->MIR = 100.0;
			path->OCR = path->BCR;
			EquatorialScattering(path, iS);
		}
		else {

        // Step 4: Determine the signal-to-interference ratio
		path->SIR = S - 10.0*log10(Isum);

		// Step 7: Determine the upper decile deviation of the signal-to-interference ratio
		path->DuSI = sqrt(pow(DuSh,2) + pow(10.0*log10(Isum/Isuml),2));

		// Step 10: Determine the lower decile deviation of the signal-to-interference ratio
		path->DlSI = sqrt(pow(DlSh,2) + pow(10.0*log10(Isumu/Isum), 2));

		// Step 12: Circuit reliability in the presence of interference only for S/I >= or < S/Ir
		if(path->SIR >= path->SIRr) {
			ICR = min(130.0 - 80.0/(1.0 + ((path->SIR-path->SIRr)/path->DlSI)), 100.0);
		}
		else { // (SIR < path->SIRr)
			ICR = max((80.0/(1.0 + ((path->SIRr-path->SIR)/path->DuSI))) - 30.0, 0.0);
		}

        // Steps 13 and 14: P.842-5 Table 3 gives OCR = Min(ICR, BCR), and
		// P.533-14 section 10.2.3 step 5 takes that as the digital circuit
		// reliability, with MIR "the ratio of the values obtained for Step 14
		// to Step 13", so that DCR = BCR MIR / 100. This used to store ICR as
		// MIR and multiply, OCR = BCR ICR / 100, which is below the minimum
		// whenever both are under 100 % (BCR = ICR = 80 % gave 64 %).
		path->OCR = min(ICR, path->BCR);
		path->MIR = (path->BCR > 0.0) ? 100.0*path->OCR/path->BCR : 100.0;

		// Find the Overall Circuit reliability with scattering
		EquatorialScattering(path, iS);
		} // Isum != 0.0
	}

	else if(path->Modulation == DIGITAL) { // path->distance > 9000.0
		// P.533-14 section 10.2.3, for path lengths beyond 9000 km: the modes making up
		// the composite signal "are contained within a time delay spread of 3 ms at
		// 7 000 km, increasing linearly to 5 ms at 20 000 km. If the time window
		// specified for the system is smaller than this time delay spread, then it is
		// predicted that the system will not meet its performance requirements."
		// This range used to skip the OCR calculation altogether, leaving OCR = 0 and
		// MIR undefined for every digital circuit beyond 9000 km.
		double spread = 3.0 + 2.0*(path->distance - 7000.0)/13000.0; // ms
		if(path->TW >= spread) {
			path->MIR = 100.0;
			path->OCR = path->BCR;
		}
		else {
			path->MIR = 0.0;
			path->OCR = 0.0;
		}
		path->OCRs = path->OCR;
	}

    // End OCR Calculation ************************************************************************

	// SNR for the required reliability
	//
	// For details please see 
	// "CCIR Report 322 Noise Variation Parameters"
	// Technical Document 2813, June 1995
	// D. C. Lawrence
	// Naval Command, Control and Ocean Surveillance Center
	// RDT&E Division
	// http://www.dtic.mil/dtic/tr/fulltext/u2/a298722.pdf
	//
	// Note: The NORM[40] = 1.28 
	//		 SNRXX = SNR50 +- t(XX%)*(D_u,l/1.28)

	if(path->SNRXXp < 50) {
		path->SNRXX = path->SNR + path->DuSN*NORM[50-path->SNRXXp]/NORM[40]; 
	}
	else { // path->Relr >= 50 
		path->SNRXX = path->SNR - path->DlSN*NORM[path->SNRXXp - 50]/NORM[40];
	}

    return;

} // End CircuitReliability()

void ModeSort(struct Mode *M[MAXMDS], int order[MAXMDS], int criteria) {

	/*
	 
	  ModeSort() is used to return the index to the mode of interest.
	  
	 		INPUT
	 			struct Mode *M[MAXMDS] - 3 E modes and 6 F2 modes in one array
	 			int criteria - Flag that is either DOMINANT or SOONEST
	 
	 		OUTPUT
	 			int order[MAXMDS] - Index array of the modes in the order desired

			SUBROUTINES
				None
	  
	 */

	int n, m;
	int j; 

	// Initialize the order array
	m = 0; // This is so the array gets loaded from 0
	for(n=0; n<MAXMDS; n++) {
		// Find the modes that exist
		if (M[n]->BMUF != 0.0) {
			order[m++] = n; 
		}
    }

    if(criteria == DOMINANT) {
		for(n=0; n<MAXMDS; n++) {
			if(order[n] != NOTINDEX) { // The mode exists
				for(m=0; m<MAXMDS; m++) {
					if(order[m] != NOTINDEX) { // The mode exists
						if(M[order[n]]->Ew > M[order[m]]->Ew) {
							j = order[n];
							order[n] = order[m];
							order[m] = j;
						}
                    }
                }
            }
        }
    }
	else if(criteria == SOONEST) {
		for(n=0; n<MAXMDS; n++) {
			if(order[n] != NOTINDEX) { // The mode exists
				for(m=0; m<MAXMDS; m++) {
					if(order[m] != NOTINDEX) { // The mode exists
						if((M[order[n]]->BMUF != 0.0) && (M[order[n]]->tau < M[order[m]]->tau)) {
							j = order[n];
							order[n] = order[m];
							order[m] = j;
						}
                    }
                }
            }
        }
    }

    return;

} // End ModeSort()

int NumberofModes(struct PathData path) {

	/*
	 
	  NumberofModes() - Counts the number of modes in the path structure
	 
	 		INPUT
	 			struct PathData path
	 
	 		OUTPUT
	 			returns the number of modes that exist
	 
	 		SUBROUTINES
				None

	 */

	int n, count;

	count = 0;
	for(n=0; n<MAXEMDS; n++) {
		// Only count the modes that exist
		if(path.Md_E[n].BMUF != 0.0) {
			count += 1;
		}
    }
    for(n=0; n<MAXF2MDS; n++) {
		// Only cound the modes that exist
		if(path.Md_F2[n].BMUF != 0.0) {
			count += 1;
		}
    }

    return count;

} // End NumberofModes()

double DigitalModulationSignalandInterferers(struct PathData *path, int iS[MAXMDS], int iI[MAXMDS]) {

	/*
	 
	 	DigitalModulationSignalandInterferers() Returns the signal that satisfies the amplitude ratio, A, 
	 		and the time window, Tw. It also returns the indicies of the interfering modes in the array, iI[9]
	 		The latter is used in the calculation of overall circuit reliability, OCR
	 
	 		INPUT
	 			struct PathData *path
	 			
	 		OUTPUT
	 			int iS[MAXMDS] - Index array of the modes that meet the signal criteria
	 			int iI[MAXMDS] - Index array of the modes that meet the interference criteria
	 
	 		SUBROUTINES
				ElevationAngle()
				NumberofModes()
				ModeSort()

	 */

	int n,m,j;	// Temp

	double Ssum;	// Sum of the field strengths 
	double S;		// Signal used in Step 1 Table 1 and Table 3 P.842-4
	double dh;		// Hop distance
	double hr;		// Reflection height
	double delta;	// Elevation angle
	double psi;		// Half hop angle 
	double ptick;	// Slant range
	double deltat;	// Time window criteria
	double deltaA;  // A ratio criteria 
	double Xs, Xl;	// Equation (42) terms for 7000 - 9000 km

	struct Mode *M[MAXMDS];	// This array is so that all modes can be examined together independant of E or F2 layer
	// The following 2 arrays, iPrw and itau, are modes indicies arrays for the digital BCR, SIR and OCR calculation
	int iEw[9]; 
	int itau[9];

	// Initalize the order array
	for(n=0; n<MAXMDS; n++) { 
		iEw[n]	= NOTINDEX;
		itau[n] = NOTINDEX;
		iI[n]	= NOTINDEX;
		iS[n]	= NOTINDEX;
	}

    //*****************************************************************************************
	// Equation (47): tau = (p'/c) 10^3 ms, with p' the virtual slant range of
	// equation (19), 2 R0 sin(d/2R0) / cos(Delta + d/2R0) per hop. This used
	// cos(Delta - d/2R0), which is not p' and shrinks the delay differences
	// between modes (1F to 3F at 3000 km: 0.87 ms instead of 1.46 ms). tau is
	// stored in seconds: p' (km) * 1000 / VofL (m/s).
	// Although the slant range. ptick, was calculated in MedianSkywaveFieldStrengthShort() is was
	// calculated under the E layer screening condition which is not relevant here so ptick must be
	// calculated here. 
	if(path->distance <= 9000.0) {
		// Determine the time delay for all modes that exist
		// E layer modes
		// E layer reflection height
		hr = 110.0;
		for(n=path->n0_E; n<MAXEMDS; n++) {
			if(path->Md_E[n].BMUF != 0.0) { // Mode exists
				dh = path->distance/(n+1); // Hop distance
				delta = ElevationAngle(dh, hr);
				psi = dh/(2.0*R0);
				ptick = 2.0*R0*(sin(psi)/cos(delta + psi)); // equation (19)
				path->Md_E[n].tau = (n+1)*(ptick/VofL)*1000.0;
			}
        }

        // F2 layer modes
		// The reflection height for each F2 mode was calculated in ELayerScreeningFrequency()
		for(n=path->n0_F2; n<MAXF2MDS; n++) {
			if(path->Md_F2[n].BMUF != 0.0) { // Mode exists
				hr = path->Md_F2[n].hr;
				dh = path->distance/(n+1); // Hop distance
				delta = ElevationAngle(dh, hr);
				psi = dh/(2.0*R0);
				ptick = 2.0*R0*(sin(psi)/cos(delta + psi)); // equation (19)
				path->Md_F2[n].tau = (n+1)*(ptick/VofL)*1000.0;
			}
        }

        // Do the following if there are 2 or more modes 
		if(NumberofModes(*path) >= 2) {
			// For this calculation the layers don't matter so set up an array of all the modes
			// so that a single loop can be used
			// Point the M[] array at all of the modes in path
			for(n=0; n<MAXEMDS; n++) {
				M[n] = &path->Md_E[n];
			}
            for(n=0; n<MAXF2MDS; n++) {
				M[n+3] = &path->Md_F2[n];
			}

            // P.533-12 Section 10.2.3 Reliability prediction procedure
			// Step 1: Determination of the dominant mode, Ew
			ModeSort(M, iEw, DOMINANT); // iEw[0] is the dominant mode

			// Order the modes by time also			
			ModeSort(M, itau, SOONEST); // itau[0] is the earlest mode

			// Step 2: All other active modes with strengths exceeding (Ew - A (dB)) are identified.
			// Step 3: The first arriving mode is identified, and all modes within the time window, Tw, 
			// measured from the first arriving mode, are identified.
			// Step 4: For path lengths up to 7000 km, a power summation of the modes arriving within the
			// window is made, or for path lengths between 7000 and 9000 km the interpolation procedure is used.
			// the basic circuit reliability, BCR, is determined using the same method as in the analog modulation.
			// Step 2 gives an amplitude criteria and Step 3 gives a delay criteria and determine the modes which fulfill 
			// both of these criteria. Step 4 details what to do with the modes that meet the criteria.

			// The zeroth element in the iEw[] array is the dominant mode.
			// Step 2: "All other active modes with strengths exceeding (Ew - A (dB))".
			// A is defined on mode strengths (section 10.2.1: the ratio of the strength
			// of the dominant mode to that of a sub-dominant mode), so the test is on
			// the field strength Ew; it was made on the received power Prw, which
			// also carries each mode's receive antenna gain.
			deltaA = (M[iEw[0]]->Ew - path->A);

			// Step 3: "Of the modes identified in Steps 1 or 2, the first arriving
			// mode is identified". This took the earliest of every mode with a
			// basic MUF (itau[0]), including E-screened modes that carry no signal
			// (Prw = TINYDB) and modes below the amplitude ratio, so the window
			// could open on a mode that is not there. Only active modes -- those
			// summed into Es (MC) -- that pass the step 2 test are considered.
			deltat = DBL_MAX;
			for(n=0; n<MAXMDS; n++) {
				if((M[n]->BMUF != 0.0) && M[n]->MC && (M[n]->Ew >= deltaA) && (M[n]->tau < deltat)) {
					deltat = M[n]->tau;
				}
			}
			deltat += path->TW/1000.0; // The tau for each mode is in seconds where the time window, TW, is in mS

			// Initialize signal sum
			Ssum = 0.0;
			for(n=0; n<MAXMDS; n++) {
				// Sum the mode as signals that satisfy the following criteria:
				//		i) The mode is active: it exists and was summed into Es
				//		ii) The mode is within the A ratio of the dominant mode (step 2)
				//		iii) The mode arrives within TW of the earlest arriving mode (step 3)
				// Step 5: the interferers are "the active modes identified in Step 2 above
				// [that] have differential time delays beyond the time window". Every other
				// mode with a basic MUF used to count as an interferer, including E-screened
				// modes and modes weaker than Ew - A, so the interference sum was never 0.
				if((M[n]->BMUF != 0.0) && M[n]->MC && (M[n]->Ew >= deltaA)) { 
					if(M[n]->tau <= deltat) {

						Ssum += pow(10.0, M[n]->Prw/10.0);
						iS[n] = n;
					}
					else { // If the mode is not determined to be a signal then it is interference.
						// Store the index of the interfering modes for later use in the calculation of 
						// the signal-to-interference ratio (See Table 3 P.842-4).
						iI[n] = n;
					}
                }
            }

            // Step 4: "a power summation of the modes arriving within the window",
			// the in-window counterpart of equation (44): the sum of each mode's
			// available power Prw, which carries that mode's own receive antenna
			// gain. This took 10 log sqrt(sum (10^(Ew/10))^2) -- the root of the
			// summed squares of the field strengths, so two equal modes added
			// 1.5 dB rather than 3 dB -- and applied only the dominant mode's gain.
			if(Ssum > 0) {
				S = 10.0*log10(Ssum);
			}
			else {
				S = TINYDB;
			}

			// Step 4 continues: "or for path lengths between 7 000 and 9 000 km the
			// interpolation procedure given in section 5.4 is used". The in-window
			// short-path power is interpolated, with the equation (42) weights, towards
			// the long-path power El + Grw(0-8 deg) that section 6 uses at 9000 km
			// (path->Grw holds that gain in this range). This used the short-path
			// window sum alone at every distance up to 9000 km.
			if((7000.0 < path->distance) && (path->distance < 9000.0)) {
				Xs = pow(10.0, S/100.0);
				Xl = pow(10.0, (path->El + path->Grw - 20.0*log10(path->frequency) - 107.2)/100.0);
				S = 100.0*log10(Xs + ((path->distance - 7000.0)/2000.0)*(Xl - Xs));
			}

		}
		else { // (NumberofModes(*path) < 2) 
			// There is only one mode. 
			S = path->Pr;
		}
    } // if(path->distance <= 9000.0) 
	else { // (path->distance > 9000.0) 
		S = path->Pr;
	}

    // The iI[] and iS[] arrays need to be ordered.
	for(n=0; n<MAXMDS; n++) {
		for(m=0; m<MAXMDS; m++) {
			if(iI[n] < iI[m]) {
				j = iI[n];
				iI[n] = iI[m];
				iI[m] = j;
			}
            if(iS[n] < iS[m]) {
				j = iS[n];
				iS[n] = iS[m];
				iS[m] = j;
			}
        }
    }

    return S;

}

void EquatorialScattering(struct PathData *path, int iS[MAXMDS]) {

	/* 
	  EquatorialScattering() - Determines the equatorial scattering by the method described 
	 		in P.533-14 Section 10.3 "Equatorial scattering" and Attachment 1 to Annex 1
	 
	 		INPUT
	 			struct PathData *path
	 			int iS[MAXMDS] - Index array of the signal modes (0-2 E, 3-8 F2), from
	 				DigitalModulationSignalandInterferers(), NOTINDEX-terminated
	 
	 		OUTPUT
	 			path->probocc - Probability of scattering occurrence (%)
	 			path->OCRs - Overall circuit reliability with scattering, equation (48)
	 
			SUBROUTINES
				FindFlambdad()
				FindFTl()

		Attachment 1 gives the scattered power as a fraction 0.056 of the specular
		power of a mode (about -12.5 dB), falling off as a half-normal in delay
		(Tspread = 1 ms) and a normal in frequency (Fspread = 3 Hz). Every level
		here is in dBW, compared with the dominant mode's available power.

		This routine was rewritten because none of its steps worked: the time
		spread was evaluated at tau = Tw (ms, absolute) against mode delays in
		seconds, included E modes and read Md_F2[] without the offset of 3; pm was
		a field strength in dB multiplied by 0.056; the frequency spread was
		evaluated at FW - f (Hz, with f the carrier) and so was always 0; the
		step 9 test was reversed; and the control point was chosen by comparing
		time-spread values indexed by control point constants.

	 */

	double Tspread = 1.0e-3;	// Standard deviation of the time spread, 1 ms, in seconds as tau is
	double Fspread = 3.0;		// Standard deviation of the frequency spread (Hz)
	double k = 10.0*log10(0.056);	// The 0.056 scattered fraction, in dB
	double e = 10.0*log10(exp(1.0));	// dB per neper of power
	double tfirst, tedge;		// First arrival among the signal modes, and the window edge (s)
	double Pdom;				// Available power of the dominant mode (dBW)
	double level;				// Largest scattered level at a window edge (dBW)
	double pdomF;				// Available power of the dominant F region mode (dBW)
	double p, pmax;				// Probability of occurrence at a control point, and the largest
	double FR, FS;
	struct Mode *Md;
	int n, i, domF, ncp;
	int cps[2];

	path->probocc = 0.0;
	path->OCRs = path->OCR;

	if((path->distance > 9000.0) || (iS[0] == NOTINDEX)) return;

	// The window opens on the first arriving signal mode (section 10.2.3 step 3),
	// and the dominant mode is the strongest one (step 1).
	tfirst = DBL_MAX;
	Pdom = TINYDB;
	for(n=0; (n<MAXMDS) && (iS[n] != NOTINDEX); n++) {
		Md = (iS[n] < MAXEMDS) ? &path->Md_E[iS[n]] : &path->Md_F2[iS[n]-MAXEMDS];
		if(Md->tau < tfirst) tfirst = Md->tau;
		if(Md->Prw > Pdom) Pdom = Md->Prw;
	}
	tedge = tfirst + path->TW/1000.0;

	// Step 7: the time scattering function "applied to each F region mode within
	// the time window and the scattering strength pTspread, found at the edge of
	// the time window, Tw" (for tau greater than tau_m).
	level = TINYDB;
	domF = NOTINDEX;
	pdomF = TINYDB;
	for(n=0; (n<MAXMDS) && (iS[n] != NOTINDEX); n++) {
		if(iS[n] < MAXEMDS) continue; // F region modes only
		Md = &path->Md_F2[iS[n]-MAXEMDS];
		if(tedge > Md->tau) {
			level = max(level, Md->Prw + k - e*pow(tedge - Md->tau, 2)/(2.0*pow(Tspread, 2)));
		}
		if(Md->Prw > pdomF) {
			pdomF = Md->Prw;
			domF = iS[n];
		}
	}

	// Step 8: the frequency scattering function "applied to the dominant F region
	// mode and the frequency scattering strength is found symmetrically at the edges
	// of the frequency window, Fw". The edges are taken at +/- FW from the carrier,
	// as this routine always has; the function is symmetric, so one value serves.
	if(domF != NOTINDEX) {
		level = max(level, pdomF + k - e*pow(path->FW, 2)/(2.0*pow(Fspread, 2)));
	}

	// Step 9: "If the value of any pTspread and/or pFspread at the edges of the
	// windows exceeds (Ew - A) the probability of occurrence of scattering should
	// be determined at the control points for the F region modes ... Where more
	// than one control point is considered for a propagation mode, the largest
	// probability should be taken."
	if(level <= Pdom - path->A) return;

	// F2 modes use the control points of Table 1a): mid-path up to dmax, otherwise
	// T + d0/2 and R - d0/2, for every mode.
	if(path->distance <= path->dmax) {
		cps[0] = MP;
		ncp = 1;
	}
	else {
		cps[0] = Td02;
		cps[1] = Rd02;
		ncp = 2;
	}

	// P.533-14 Attachment 1: "FR = (0.1 + 0.008R12) or 1, whichever is
	// the smaller". This read 0.1 + 0.008*max(SSN, 160), so FR was never
	// below 1.38 and probocc was overstated for every circuit.
	FR = min(0.1 + 0.008*path->SSN, 1.0);
	FS = 0.55 + 0.45*sin(60.0*D2R*((path->month+1.0) - 1.5));

	pmax = 0.0;
	for(i=0; i<ncp; i++) {
		p = FindFlambdad(path->CP[cps[i]])*FindFTl(path->CP[cps[i]])*FR*FS;
		if(p > pmax) pmax = p;
	}

	// Attachment 1 gives probocc as a probability, 0 to 1. It is kept in percent,
	// like BCR and MIR and as the report labels it, so that the line below is
	// equation (48), OCRs = BCR MIR (1 - probocc)/100.
	path->probocc = 100.0*pmax;

	// Find the overall circuit reliabilty with scattering
	path->OCRs = path->BCR*path->MIR*(100.0 - path->probocc)/10000.0;

	return;

}

double FindFlambdad(struct ControlPt CP) {

	/*
	 
	  FindFlambdad() - Determines F sub lambda sub d in P.533-12 Appendix 1
	 		to Annex 1 "A model for scattering of HF signals"
	 
	 	INPUT 
	 		Control point to determine F sub lambda sub d
	 
	 	OUTPUT
	 		returns F sub lambda sub d
	 
	 	SUBROUTINES
			None

	 */

	double lambdad;

	// Magnetic dip parameter, in degrees as Attachment 1 states the bands and
	// the formula. The dip is held in radians; it was used unconverted in the
	// 15-25 degree formula, which then returned about -11.7 across the band.
	lambdad = fabs(CP.dip[HR100km])*R2D;
	if((0.0 <= lambdad) && (lambdad < 15.0)) {
		return 1.0;
	}
	else if((15.0 <= lambdad) && (lambdad < 25.0)) {
		return pow(((25.0 - lambdad)/10.0), 2) * ((lambdad - 10.0)/5.0);
	}
	else if((25.0 <= lambdad) && (lambdad <= 90.0)) {
		return 0.0;
	}

    return 0.0;

}

double FindFTl(struct ControlPt CP) {

	/*
	 
	 
	  FindTl() - Determines the F sub T sub l parameter in in P.533-12 Appendix 1
	 		to Annex 1 "A model for scattering of HF signals"
	 
	 	INPUT 
	 		Control point of interest
	 
	 	OUTPUT 
	 		returns time parameter F sub T sub l for the calculation of Prob sub occ
	 
	 	SUBROUTINES
			None

	 */

	double Tl;

	// Time parameter
	// Attachment 1: "Tl: local time at the control point (h)". CP.ltime is the UTC
	// hour, which is what this used.
	Tl = LocalMeanTime(CP);
	// FTl is 1 on both sides of midnight (20 < Tl < 24 and 00 < Tl < 03), so
	// Tl = 0 exactly is 1 as well; it fell through to 0 here.
	if((0.0 <= Tl) && (Tl <= 3.0)) {
		return 1.0;
	}
	else if((3.0 < Tl) && (Tl <= 7.0)) {
		return pow(((7.0 - Tl)/4.0), 2) * ((Tl - 1.0)/2.0);
	}
	else if((7.0 < Tl) && (Tl <= 19.0)) {
		return 0.0;
	}
	else if((19.0 < Tl) && (Tl <= 20.0)) {
		return pow((Tl - 19.0), 2)*(41.0 - 2.0*Tl);
	}
	else if((20.0 < Tl) && (Tl <= 24.0)) {
		return 1.0;
	}

    return 0.0;
}
