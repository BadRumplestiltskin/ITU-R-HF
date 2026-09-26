#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Local Includes
#include "Common.h"
#include "P533.h"

// Rounding can leave a sine or cosine a few ulp outside [-1, 1], where asin()
// and acos() return NaN. Clamp such arguments back into the domain.
static double ClampUnit(double x) {
	if(x > 1.0) return 1.0;
	if(x < -1.0) return -1.0;
	return x;
}

P533_API double GreatCircleDistance(struct Location here, struct Location there) {

/* 

  GreatCircleDistance() determines the distance in km between here and there on the great circle.
 
 		INPUT
 			here - Control point origin of calculation
 			there - Control point terminus of calculation
 
 		OUTPUT 
 			returns the distance (km) between here and there

		SUBROUTINES
			None

 */
	double h; // Haversine of the central angle, in [0, 1]

	h = pow((sin((here.lat-there.lat)/2.0)),2.0) + cos(here.lat)*cos(there.lat)*pow((sin((here.lng - there.lng)/2.0)),2.0);

	// For (near) antipodal points h can round a few ulp above 1: asin(sqrt(h)) would be NaN
	return 2.0*R0*asin(sqrt(ClampUnit(h))); 

} //  GreatCircleDistance()

P533_API void GreatCirclePoint(struct Location here, struct Location there, struct ControlPt *midpnt, double distance, double fraction) {

/*

 	GreatCirclePoint() determines the lat and long of a point on a great circle path of length distance from here to there.
 		The point is determined at fraction*distance from here. 
 
 		INPUT
 			here - Location origin of calculation
 			there - Location point terminus of calculation
 			distance - Distance from here to there 
 			fraction - Fraction of the distance from here to there to determine the mid-path
 
 		OUTPUT
 			midpnt->L.lat - Mid-point latitude
 			midpnt->L.lng - Mid-point longitude
 			midpnt->distance - Mid-point distance from here

		SUBROUTINES
			None
 			
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
	 
		INPUT
	 		here - Location origin of calculation
	 
	 	OUTPUT 
	 		there - Location with geomagnetic location of here 

		SUBROUTINES
			None

*/ 

	struct Location GeoMagNPole;

	// This was the pole location when the coefficients Lh were determined.
	// In 1955 the Geomagnetic North Pole was at 78.5N	69.2W,  
	//				while the South Pole was at  78.5S  110.8E
	GeoMagNPole.lat = 78.5*D2R; 
	GeoMagNPole.lng = -68.2*D2R;

	there->lat = asin(ClampUnit(sin(here.lat)*sin(GeoMagNPole.lat) + cos(here.lat)*cos(GeoMagNPole.lat)*cos(here.lng - GeoMagNPole.lng)));
	there->lng = asin(ClampUnit(cos(here.lat)*sin(here.lng - GeoMagNPole.lng)/cos(there->lat)));

	return;

}

P533_API double Bearing(struct Location here, struct Location there, int direction) {

/*

  Bearing() - Determines the bearing of a Location
 
 		INPUT
 			here - Location of the origin of calculation
 			there - Location of the terminus of the calculation
			direction - Either the short or long way round
 
 		OUTPUT
 			returns the bearing from here to there

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
	 */

	return fmod(fmod(CP.ltime + CP.L.lng*R2D/15.0, 24.0) + 24.0, 24.0);
}
