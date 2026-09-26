#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local includes
#include "Common.h"
#include "P533.h"
// End Local includes

void Between7000kmand9000km(struct PathData *path) {

	/*

	  Between7000kmand9000km() Interpolates the median sky-wave field strength between the
			short-path value Es (section 5.2) and the long-path value El (section 5.3) for path
			lengths strictly between 7000 and 9000 km, and sets the path basic MUF for that range.
	 		Implements ITU-R P.533-14 section 5.4 "Paths between 7 000 and 9 000 km", equation (42):
				Ei = 100 log10(Xi),  Xi = Xs + ((D - 7000)/2000)(Xl - Xs),
				Xs = 10^(0.01 Es),   Xl = 10^(0.01 El)
			and the last paragraph of section 5.4: "The basic MUF for the path is equal to the
			lower of the basic MUF values given from equation (3) for the two control points noted
			in Table 1a)", i.e. T + d0/2 and R - d0/2 (path->CP[Td02], path->CP[Rd02]).

	 	INPUT
	 		struct PathData *path
				path->distance - great-circle path length D (km)
				path->Es - section 5.2 field strength, equation (28) (dB(1 uV/m)); must already be set
						   by MedianSkywaveFieldStrengthShort()
				path->El - section 5.3 field strength, equation (39) (dB(1 uV/m)); must already be set
						   by MedianSkywaveFieldStrengthLong()
				path->CP[Td02], path->CP[Rd02] - the Table 1a) control points, with foF2, M(3000)F2,
						   foE and fH already evaluated (MUFBasic())

	 	OUTPUT
	 		path->Ei - The interpolated path field strength (dB(1 uV/m)), equation (42)
	 		path->BMUF - The path basic MUF (MHz): the lower of F2(dmax)MUF at the two control points
			Nothing is changed when path->distance is outside the open interval (7000, 9000) km.

		NOTES
			dmax at each control point is taken from equation (5) and limited to 4000 km, as
			section 3.5.1 requires "For the calculation of the basic MUF". Equation (3) is then
			evaluated with the hop length d = dmax (the F2(dmax)MUF of section 3.5.1.2); these
			paths are always longer than dmax.
			If path->Es is the TINYDB sentinel (-307, all short-path modes screened) Xs is
			10^-3.07, effectively 0, so Ei follows El weighted by (D - 7000)/2000.

		SUBROUTINES
			CalcB()
			Calcdmax()
			CalcF2DMUF()

	 */

	double Xs;		// Linear field strength of path->Es
	double Xl;		// Linear field strength of path->El
	double Xi;		// Linear interpolated field strength
	double B;		// Intermediate value fo the MUF calculation
	double dmax;	// Maximum hop distance as calculated by Eqn (5) P.533-14
	double BMUF[2]; // Array of basic MUFs at two control points

	if((7000.0 < path->distance) && (path->distance < 9000.0)) {

		Xl = pow(10.0, path->El/100.0);
		Xs = pow(10.0, path->Es/100.0);

		Xi = Xs + ((path->distance - 7000.0)/2000.0)*(Xl - Xs);

		path->Ei = 100.0*log10(Xi); // Eqn (42) P.533-14

		// Calculate the basic MUF according to P.533-14 Section 5.4 "Paths between 7000 and 9000 km":
		// equation (3) at the two Table 1a) control points. These paths are
		// always longer than dmax, so as in section 3.5.1.2 that is
		// F2(dmax)MUF, equation (3) with d = dmax; this passed the hop length.
		B = CalcB(&path->CP[Td02]);
		dmax = min(Calcdmax(&path->CP[Td02]), 4000.0);
		BMUF[0] = CalcF2DMUF(&path->CP[Td02], dmax, dmax, B);
		
		B = CalcB(&path->CP[Rd02]);
		dmax = min(Calcdmax(&path->CP[Rd02]), 4000.0);
		BMUF[1] = CalcF2DMUF(&path->CP[Rd02], dmax, dmax, B);

		path->BMUF = min(BMUF[0], BMUF[1]);

	} // ((7000.0 < path->distance) && (path->distance < 9000.0))

	return;

}
