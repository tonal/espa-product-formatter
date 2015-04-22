/*******************************************************************************
NAME                Projection support routines listed below.

PURPOSE:	The following functions are included in CPROJ.C.

		SINCOS:	  Calculates the sine and cosine.
		ASINZ:	  Eliminates roundoff errors.
		MSFNZ:	  Computes the constant small m for Oblique Equal Area.
		QSFNZ:	  Computes the constant small q for Oblique Equal Area.
		PHI1Z:	  Computes phi1 for Albers Conical Equal-Area.
		PHI2Z:	  Computes the latitude angle, phi2, for Lambert
			  Conformal Conic and Polar Stereographic.
		PHI3Z:	  Computes the latitude, phi3, for Equidistant Conic.
		PHI4Z:	  Computes the latitude, phi4, for Polyconic.
		PAKCZ:	  Converts a 2 digit alternate packed DMS format to
			  standard packed DMS format.
		PAKR2DM:  Converts radians to 3 digit packed DMS format.
		TSFNZ:	  Computes the small t for Lambert Conformal Conic and
			  Polar Stereographic.
		SIGN:	  Returns the sign of an argument.
		ADJUST_LON:  Adjusts a longitude angle to range -180 to 180.
		E0FN, E1FN, E2FN, E3FN:
			  Computes the constants e0,e1,e2,and e3 for
			  calculating the distance along a meridian.
		E4FN:	  Computes e4 used for Polar Stereographic.
		MLFN:	  Computes M, the distance along a meridian.
		CALC_UTM_ZONE:	Calculates the UTM zone number.

*******************************************************************************/
#include "cproj.h"
#include "local.h"

#define MAX_VAL 4
#define MAXLONG 2147483647.
#define DBLLONG 4.61168601e18

/* FIXME Temporarily disable the sincos function since newer versions of gcc
   seem to include it no matter what and it causes problems */
#if 0
/* Function to calculate the sine and cosine in one call.  Some computer
   systems have implemented this function, resulting in a faster implementation
   than calling each function separately.  It is provided here for those
   computer systems which don`t implement this function
  ----------------------------------------------------*/
void sincos
(
    double val,
    double *sin_val,
    double *cos_val
)
{
    *sin_val = sin(val);
    *cos_val = cos(val);
}
#endif

/* Function to eliminate roundoff errors in asin
----------------------------------------------*/
double asinz
(
    double con
)
{
    if (fabs(con) > 1.0)
    {
        if (con > 1.0)
            con = 1.0;
        else
            con = -1.0;
    }
    return(asin(con));
}

/* Function to compute constant small q which is the radius of a 
   parallel of latitude, phi, divided by the semimajor axis. 
------------------------------------------------------------*/
double qsfnz
(
    double eccent,
    double sinphi
)
{
    double con;

    if (eccent > 1.0e-7)
    {
        con = eccent * sinphi;
        return (( 1.0- eccent * eccent) * (sinphi /(1.0 - con * con) - 
                (.5/eccent) * log((1.0 - con)/(1.0 + con))));
    }
    else
        return(2.0 * sinphi);
}

/* Function to compute phi1, the latitude for the inverse of the
   Albers Conical Equal-Area projection.
-------------------------------------------*/
double phi1z
(
    double eccent,	/* Eccentricity angle in radians		*/
    double qs,		/* Angle in radians				*/
    long *flag	/* Error flag number				*/
)
{
    double eccnts;
    double dphi;
    double con;
    double com;
    double sinpi;
    double cospi;
    double phi;
    long i;

    phi = asinz(.5 * qs);
    if (eccent < EPSLN) 
         return(phi);
    eccnts = eccent * eccent; 
    for (i = 1; i <= 25; i++)
    {
        sincos(phi,&sinpi,&cospi);
        con = eccent * sinpi; 
        com = 1.0 - con * con;
        dphi = .5 * com * com / cospi * (qs / (1.0 - eccnts) - sinpi / com + 
               .5 / eccent * log ((1.0 - con) / (1.0 + con)));
        phi = phi + dphi;
        if (fabs(dphi) <= 1e-7)
            return(phi);
    }
    GCTP_PRINT_ERROR("Convergence error");
    *flag = 001;
    return(ERROR);
}

/* Function to compute latitude, phi3, for the inverse of the Equidistant
   Conic projection.
-----------------------------------------------------------------*/
double phi3z
(
    double ml,		/* Constant 			*/
    double e0,		/* Constant			*/
    double e1,		/* Constant			*/
    double e2,		/* Constant			*/
    double e3,		/* Constant			*/
    long *flag		/* Error flag number		*/
)
{
    double phi;
    double dphi;
    long i;

    phi = ml;
    for (i = 0; i < 15; i++)
    {
        dphi = (ml + e1 * sin(2.0 * phi) - e2 * sin(4.0 * phi) + e3 * 
                sin(6.0 * phi)) / e0 - phi;
        phi += dphi;
        if (fabs(dphi) <= .0000000001)
        {
            *flag = 0;
            return(phi);
        }
    }
    GCTP_PRINT_ERROR("Latitude failed to converge after 15 iterations");
    *flag = 3;
    return(3);
}

/* Function to adjust a longitude angle to range from -180 to 180 radians
   added if statments 
  -----------------------------------------------------------------------*/
double adjust_lon
(
    double x		/* Angle in radians			*/
)
{
    long count = 0;
    for(;;)
    {
        if (fabs(x)<=PI)
            break;
        else if (((long) fabs(x / PI)) < 2)
            x = x-(gctp_get_sign(x) *TWO_PI);
        else if (((long) fabs(x / TWO_PI)) < MAXLONG)
            x = x-(((long)(x / TWO_PI))*TWO_PI);
        else if (((long) fabs(x / (MAXLONG * TWO_PI))) < MAXLONG)
            x = x-(((long)(x / (MAXLONG * TWO_PI))) * (TWO_PI * MAXLONG));
        else if (((long) fabs(x / (DBLLONG * TWO_PI))) < MAXLONG)
            x = x-(((long)(x / (DBLLONG * TWO_PI))) * (TWO_PI * DBLLONG));
        else
            x = x-(gctp_get_sign(x) *TWO_PI);
        count++;
        if (count > MAX_VAL)
           break;
    }

    return(x);
}
