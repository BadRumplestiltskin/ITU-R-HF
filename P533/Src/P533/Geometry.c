#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local Includes
#include "Common.h"
#include "P533.h"

// ClampUnit() - Rounding can leave a sine or cosine a few ulp outside [-1, 1], where asin()
// and acos() return NaN. Clamp such arguments back into the domain.
//		INPUT	x - value that should lie in [-1, 1]
//		OUTPUT	returns x limited to [-1, 1]
static double ClampUnit(double x) {
	if(x > 1.0) return 1.0;
	if(x < -1.0) return -1.0;
	return x;
}

P533_API double GreatCircleDistance(struct Location here, struct Location there) {

/* 

  GreatCircleDistance() determines the short-way great-circle distance in km between here and
		there, by the haversine formula on a sphere of radius R0 = 6371 km (the Earth radius
		P.533-14 section 4 gives with equation (12)). P.533-14 section 2 assumes propagation
		along the great-circle path; the Recommendation gives no formula for its length.

 		INPUT
 			here - Origin: lat and lng in radians (N and E positive)
 			there - Terminus: lat and lng in radians (N and E positive)

 		OUTPUT
 			returns the short-path distance (km) between here and there, 0 to PI*R0 (about 20015 km).
			The long-path distance (2*PI*R0 minus this) is formed by the caller, InitializePath().

		NOTES
			The haversine is clamped to [0, 1] (ClampUnit()) so that (near) antipodal points
			give PI*R0 rather than NaN.

		SUBROUTINES
			ClampUnit()

 */
	double h; // Haversine of the central angle, in [0, 1]

	h = pow((sin((here.lat-there.lat)/2.0)),2.0) + cos(here.lat)*cos(there.lat)*pow((sin((here.lng - there.lng)/2.0)),2.0);

	// For (near) antipodal points h can round a few ulp above 1: asin(sqrt(h)) would be NaN
	return 2.0*R0*asin(sqrt(ClampUnit(h))); 

} //  GreatCircleDistance()

P533_API void GreatCirclePoint(struct Location here, struct Location there, struct ControlPt *midpnt, double distance, double fraction) {

/*

 	GreatCirclePoint() determines the lat and long of a point on a great circle path of length distance from here to there.
 		The point is determined at fraction*distance from here, by spherical linear interpolation
		(sphere of radius R0 = 6371 km). It is used to place the control points of P.533-14
		section 2, Table 1 (mid-path, T + 1000 km, R - 1000 km, T + d0/2, R - d0/2) and the
		points sampled for the P.842-5 Table 2 note (1) test in CircuitReliability().

 		INPUT
 			here - Location origin of calculation (lat, lng in radians)
 			there - Location point terminus of calculation (lat, lng in radians)
 			distance - Distance from here to there along the path in use (km). For a long path
				this is the long-path distance (greater than PI*R0) and the point is placed
				on the long way round.
 			fraction - Fraction (0 to 1) of distance from here at which to place the point

 		OUTPUT
 			midpnt->L.lat - Point latitude (radians)
 			midpnt->L.lng - Point longitude (radians, -PI to PI)
 			midpnt->distance - Distance of the point from here (km), fraction*distance.
				Only L and distance are written; the other ControlPt fields are untouched.

		NOTES
			distance == 0 returns here itself. When here and there are antipodal (d = PI) or
			coincident on a full-circumference long path (d = 2*PI), the interpolation is
			undefined (sin(d) = 0) and the point is found along the azimuth Bearing() gives
			instead (see the comment in the body).

		SUBROUTINES
			Bearing()
			ClampUnit()

*/
	
	double A, B, d, x, y, z; // temp variables
	double theta, delta, lat; // bearing, angular distance and latitude for the degenerate case
	
	if (distance != 0.0) {
		midpnt->distance = distance*fraction;
		d = distance / R0;
		if ((d > 1.0) && (fabs(sin(d)) < 1.0e-9)) {
			// here and there are antipodal (d = PI) or coincident on a long path of one
			// full circumference (d = 2*PI). Every great circle through here then passes
			// through there, so the interpolation below divides by sin(d) = 0 and returns
			// an arbitrary point. Instead follow the great circle leaving here on the
			// azimuth that Bearing() gives, which is the azimuth the rest of the engine
			// uses for this path (for example the antenna bearings).
			theta = Bearing(here, there, (d > PI) ? LONGPATH : SHORTPATH);
			delta = fraction*d;
			lat = asin(ClampUnit(sin(here.lat)*cos(delta) + cos(here.lat)*sin(delta)*cos(theta)));
			midpnt->L.lat = lat;
			midpnt->L.lng = here.lng + atan2(sin(theta)*sin(delta)*cos(here.lat), cos(delta) - sin(here.lat)*sin(lat));
			midpnt->L.lng = atan2(sin(midpnt->L.lng), cos(midpnt->L.lng)); // Wrap to [-PI, PI]
			return;
		}
		A = sin((1 - fraction)*d) / sin(d);
		B = sin(fraction*d) / sin(d);
		x = A*cos(here.lat)*cos(here.lng) + B*cos(there.lat)*cos(there.lng);
		y = A*cos(here.lat)*sin(here.lng) + B*cos(there.lat)*sin(there.lng);
		z = A*sin(here.lat) + B*sin(there.lat);
		midpnt->L.lat = atan2(z, sqrt(pow(x, 2) + pow(y, 2)));
		midpnt->L.lng = atan2(y, x);
	}
	else if (distance == 0) {
		midpnt->distance = 0.0;
		midpnt->L.lat = here.lat;
		midpnt->L.lng = here.lng;
	}

return;

} // GreatCirclePoint()

P533_API void GeomagneticCoords(struct Location here, struct Location *there) {
	
/* 

	GeomagneticCoords() - Conversion from geographic coordinates to geomagnetic coordinates (lat, lng)
		for an Earth-centred dipole with its north pole at 78.5 N, 68.2 W, the dipole P.533-14
		section 5.2.2 specifies for the geomagnetic latitude Gn used with Table 2 (Lh, the
		auroral and other signal losses). Its caller is FindLh() (MedianSkywaveFieldStrengthShort.c),
		which uses only the latitude.

		INPUT
	 		here - Geographic location (lat, lng in radians, N and E positive)

	 	OUTPUT
	 		there->lat - Geomagnetic latitude (radians, -PI/2 to PI/2, N positive)
	 		there->lng - Geomagnetic longitude (radians). It is found with asin(), so it is
				only resolved to -PI/2 to PI/2 (the quadrant is lost); do not use it where
				the full longitude is needed.

		NOTES
			This dipole is not the one P.1239-4 section 5 gives for foF1 (78.3 N, 69.0 W), which
			CircuitReliability() uses for the P.842-5 Table 2 note (1) test.

		SUBROUTINES
			ClampUnit()

*/

	struct Location GeoMagNPole;

	// This was the pole location when the coefficients Lh were determined.
	// In 1955 the Geomagnetic North Pole was at 78.5N	69.2W,
	//				while the South Pole was at  78.5S  110.8E
	// Note: P.533-14 section 5.2.2 states the Lh dipole pole as 78.5 N, 68.2 W, which is
	// the value used below; the 69.2 W above is not from the Recommendation.
	GeoMagNPole.lat = 78.5*D2R; 
	GeoMagNPole.lng = -68.2*D2R;

	there->lat = asin(ClampUnit(sin(here.lat)*sin(GeoMagNPole.lat) + cos(here.lat)*cos(GeoMagNPole.lat)*cos(here.lng - GeoMagNPole.lng)));
	there->lng = asin(ClampUnit(cos(here.lat)*sin(here.lng - GeoMagNPole.lng)/cos(there->lat)));

	return;

}

P533_API double Bearing(struct Location here, struct Location there, int direction) {

/*

  Bearing() - Determines the initial great-circle bearing (azimuth) from here towards there,
		the short or the long way round. Used for the antenna azimuths and by GreatCirclePoint()
		in the degenerate case. Not part of the P.533-14 text.

 		INPUT
 			here - Location of the origin of calculation (lat, lng in radians)
 			there - Location of the terminus of the calculation (lat, lng in radians)
			direction - SHORTPATH or LONGPATH (P533.h). LONGPATH adds PI.

 		OUTPUT
 			returns the bearing from here to there in radians, clockwise from true north,
			0 to 2*PI

		NOTES
			When here and there coincide or are antipodal (every direction is a great circle)
			0 (due north) is returned for both directions, so results do not depend on the
			platform's atan2() of rounding noise.

		SUBROUTINES
			None

 */

	// N and E are positive
	// S and W are negative

	double numerator, denominator;
	double bearing;

	numerator = sin(there.lng - here.lng)*cos(there.lat);
	denominator = cos(here.lat)*sin(there.lat) - sin(here.lat)*cos(there.lat)*cos(there.lng - here.lng);

	// When the two points coincide, or are antipodal, every direction is a great
	// circle between them and both terms are rounding noise. atan2() of that noise
	// gave 0 on macOS and PI on Linux for the same full-circle path, so the
	// antennas, and every result, differed by platform. Take due north, in both
	// directions.
	if ((fabs(numerator) < 1.0E-12) && (fabs(denominator) < 1.0E-12)) {
		return 0.0;
	}

	bearing = atan2(numerator, denominator);

	bearing = fmod((2.0 * PI + bearing), 2.0 * PI);

	if (direction == LONGPATH) {
        // Flip the bearing around since you are looking at the long way round
		// Then modulo 2*PI
		bearing = fmod((2.0 * PI + (bearing + PI)), 2.0 * PI);
	}

return bearing;
}

double LocalMeanTime(struct ControlPt CP) {

	/*
	  LocalMeanTime() - The local (mean solar) time at a control point, in hours 0 to 24:
			the UTC hour held in CP.ltime plus the longitude at 15 degrees an hour.
			P.533-14 and P.1239 index several quantities by local time -- the foF2
			decile factors, F_T of Attachment 1 and the mid-path local time of Table 2 --
			and CP.ltime itself carries the UTC hour (sunrise and sunset are in UTC).
			Used for: P.1239-4 section 3.2 / Tables 2 and 3 ("the local time ... at the
			control point", FindfoF2var() via MUFVariability()), P.533-14 Attachment 1 section 3
			("Tl: local time at the control point (h)", FindFTl()) and the local time t of
			P.533-14 Table 2 (Lh, in MedianSkywaveFieldStrengthShort.c).

			INPUT
				CP.ltime - UTC hour (the path hour index 0-23, as SolarParameters() stores it)
				CP.L.lng - Longitude (radians, E positive)

			OUTPUT
				returns the local mean time in hours, in [0, 24). No equation-of-time
				correction is applied (it is mean, not apparent, solar time).
	 */

	return fmod(fmod(CP.ltime + CP.L.lng*R2D/15.0, 24.0) + 24.0, 24.0);
}
