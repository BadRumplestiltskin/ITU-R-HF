#ifndef COMMON_H
#define COMMON_H

// #defines

// Operating system preprocessor directives
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

// Define some constants to enhance readability
#define TRUE 1
#define FALSE 0
#define PI 3.14159265358979323846
// Radius of the Earth, P.533-14 section 4: "R0: radius of the Earth, 6 371 km".
// It was 6371.009 km (the IUGG mean radius), which put paths of exactly 2 500 km
// on the far side of the Table 2 a)/b) boundary. The magnetic field model uses
// its own radius, 6371.2 km (P.1239-4 equation (8)); see Magfit.c.
#define R0 6371.0 // km
#define D2R (PI/180.0)	// was 0.0174532925, truncated
#define R2D (180.0/PI)	// was 57.2957795, truncated
#define VofL 299792458.0 // Velocity of light (m/s)

// Numbers used as indicators
#define TINYDB DBL_MIN_10_EXP // Smallest number in dB
#define TOOBIG DBL_MAX // Large number typically an error 

// Double extreme
#define DBL_MANT_DIG 53
#define DBL_DIG 15
#define DBL_MIN_EXP -1021
#define DBL_MIN_10_EXP -307
#define DBL_MAX_EXP 1024
#define DBL_MAX_10_EXP 308
#define DBL_MAX 1.7976931348623157E+308
#define DBL_MIN 2.2250738585072014E-308
#define DBL_EPSILON 2.2204460492503131E-016

//////////////////////////////////////////////////////////////////////////////
//      Copyright  International Telecommunication Union (ITU) 2018         //
//                     All rights reserved.                                 //
// No part of this publication may be reproduced, by any means whatsoever,  //
//              without written permission of ITU                           //
//////////////////////////////////////////////////////////////////////////////
#endif // COMMON_H
