/*************************************************************************

NAME: ias_geo_handle_180

PURPOSE: Defines routines to be used in handling the issues with crossing the 
         180 degree boundary. 

NOTES:
The unit should be the numerical representation of the angular unit as
returned by ias_geo_get_units. If using a the defines from gctp.h they
are defined as single, the strings passed to ias_geo_get_units are plural:

 unit   unit string   define from gctp.h
  0      RADIANS      RADIAN     (is implemented)
  1      FEET         FEET       (not implemented)
  2      METERS       METER      (not implemented)
  3      SECONDS      SECOND     (is implemented)
  4      DEGREES      DEGREE     (is implemented)
  5      DMS          DMS        (is implemented)

**************************************************************************/
#include <stdio.h>
#include <math.h>
#include "ias_lw_geo.h"
#include "ias_logging.h"
#include "gctp.h"

static double ias_math_get_pi();
static int can_wrap_around ( int unit );
static void add_once_around_to_dms ( double *lon );
static double determine_once_around ( int unit );

/******************************************************************************
Name: ias_math_get_pi

Purpose: Returns the calculated value of Pi.

Returns: The value of Pi

******************************************************************************/
static double ias_math_get_pi()
{
    static double pi;
    static int initialized = 0;

    if (!initialized)
    {
        pi = 4.0 * atan(1.0);

        initialized = 1;
    }

    return pi;
}


/******************************************************************************
NAME:           can_wrap_around

PURPOSE:
Detemines if the specified unit is one that is currently handled by this
module.  Currently, only a subset of possible units are handled by this
routine.

RETURN VALUE:   
Type = int
Value    Description
-----    -----------
 1       if the unit is handled
 0       if the unit is not handled
-1       if the unit is not recognized


******************************************************************************/

static int can_wrap_around ( int unit )
{
    if ((unit == RADIAN) || (unit == SECOND) 
         || (unit == DEGREE) || (unit == DMS))
        return 1;

    if ((unit == FEET) || (unit == METER))
        return 0;

    IAS_LOG_WARNING("Unit %d not recognized", unit);
    return -1;
}


/******************************************************************************
NAME:           ias_geo_does_cross_180

PURPOSE:
Attempts to determine if the specified points are from a scene that crosses
the 180 degree boundary.  The points passed in should be the x coordinates at
the corner points.  The unit is the value returned by ias_geo_get_units.  Note
that currently not all types of units are supported.

RETURN VALUE:   
Type = int
Value    Description
-----    -----------
 1       if the scene appears to cross the 180 degree boundary
 0       if the scene does not cross the 180 degree boundary

******************************************************************************/

int ias_geo_does_cross_180
(
    int unit,             /* I: The angular unit of the angles passed in */
    double const corner_longitudes[4]    
                          /* I: The longitude for each corner of the scene */
)
{
    double once_around;  /* the distance around the earth in unit units */
    int positive_count = 0; /* the number of x's that are positive */
    int i, j;            /* loop variables */

    /* determine if we can handle the specified unit */
    if (can_wrap_around(unit) != 1)
    {
        IAS_LOG_DEBUG("Unable to have a 180 degree crossing");
        return 0;
    }

    /* determine how many units it takes to circle the globe */
    once_around = determine_once_around(unit);

    /* Count the number of corners that are positive.  If none or all of them
       are positive, the 180 degree boundary is not crossed. */
    for ( i = 0; i < 4; i++ )
    {
        if ( corner_longitudes[i] > 0 )
            positive_count++;
    }

    if ((positive_count == 0) || (positive_count == 4))
        return 0;

    /* For a scene that does not cross 180, fabs(x1 - x2)/once_around
       will be very nearly 0.0 for all corner x-coordinates.  For a scene that
       crosses 180, fabs(x1 - x2)/once_around will be closer to 1 for some pair
       of corner x-coordinates. */
    for (i = 0; i < 4-1; i++)
    {
        for (j = i+1; j < 4; j++)
        {
            /* fraction is the fraction of the earth's circumference that
               is traversed by travelling from corner_longitudes[i] 
               to corner_longitudes[j] */
            double fraction = fabs(corner_longitudes[i] - corner_longitudes[j])
                               / once_around;
            if (fraction > 0.5)
                return 1;
        }
    }

    return 0;
}

/******************************************************************************
NAME:           determine_once_around

PURPOSE:
Determines how many units it takes to circle the globe.

Uses the same units as does_cross_180.

RETURN VALUE:   
Type = double
Value    Description
-----    -----------
 0.0     if unknown or non-implemented unit,
 x       if specified unit is implemented, then returns the number of units
         needed to travel around the world (eg: 360 degrees, 2 pi radians,
         etc.)

******************************************************************************/

static double determine_once_around
(
    int unit   /* I: The angular unit being used. */
)
{
    double once_around = 0.0;

    if (unit == RADIAN)  once_around = 2 * ias_math_get_pi();  /* Radians */
    else if (unit == SECOND)  once_around = 360.0 * 3600.0;    /* Seconds */
    else if (unit == DEGREE)  once_around = 360.0;             /* Degrees */
    else if (unit == DMS)  once_around = 360000000.0;          /* DMS */

    return once_around;
}

/******************************************************************************
NAME:           ias_geo_add_once_around

PURPOSE:
Adds to the longitude value so that the new value will refer to the same place
on earth, but the longitude value will be greater than the old value by one
trip around the earth.

Uses the same units as does_cross_180.

RETURN VALUE:   
Type = int
Value    Description
-----    -----------
 0        if the unit specified is not implemented for handling by this module
 1        if the value was updated as requested

******************************************************************************/

int ias_geo_add_once_around
(
    int unit,        /* I: The angular unit to use to interpret lon */
    double *lon      /* I/O: The angle to be adjusted */
)
{
    if (can_wrap_around(unit) != 1)
        return 0;

    if (unit == DMS)
    {
        add_once_around_to_dms(lon);
        return 1;
    }

    *lon += determine_once_around(unit);

    return 1;
}

/******************************************************************************
NAME:           add_once_around_to_dms

PURPOSE:
Computes the dms value for longitude that represents the same line as the
input longitude, but is greater than the current value by 360 degrees.

RETURN VALUE:   
Type = none

NOTES:
Meant only to be called by add_once_around.

******************************************************************************/

static void add_once_around_to_dms
(
    double *lon      /* I/O: The angle to be adjusted */
)
{
    double easy_soln; /* simple approach */

    double angle;  /* used in calculating tdeg, tmin, and tsec */
    int status;

    /* First, we try a simple approach in which we simply add 360 degrees to
       the input value.  If we didn't cross zero, then we are done; that will
       be the value returned.  If we do cross zero, then we have to play some
       games with the individual values stored in the packed input. */
    easy_soln = *lon + determine_once_around(DMS);

    if (((easy_soln < 0) && (*lon < 0)) || ((easy_soln >= 0) && (*lon >= 0)))
    {
        *lon = easy_soln;
        return;
    }

    status = ias_geo_convert_dms2deg(*lon, &angle, "DEGREES");
    angle += determine_once_around(DEGREE);
    status = ias_geo_convert_deg2dms(angle, lon, "DEGREES");
    
}

