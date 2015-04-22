/****************************************************************************
NAME: ias_geo_convert_deg2dms

PURPOSE:  To convert total degrees to packed degrees, minutes, seconds.

RETURN VALUE:   Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

ALGORITHM DESCRIPTION:
        Receive an angle degrees
        Convert it to DMS.
        The angle is then checked to be sure it is within the limits
          of its use (LAT, LON, or DEGREES).

*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "ias_logging.h"
#include "ias_lw_geo.h"

int ias_geo_convert_deg2dms 
(
    double deg,         /* I: Angle in seconds, minutes, or degrees */
    double *dms,        /* O: Angle converted to DMS */
    const char *check   /* I: Angle usage type (LAT, LON, or DEGREES) */ 
)
{
    double tsec;    /* Total seconds in the input angle */
    double maxdms;  /* Upper bound of the angle value for its use */
    double mindms;  /* Lower bound of the angle value for its use */

    int tdeg;      /* Degrees in the input angle */
    int tmin;      /* Minutes in the input angle */
    int sign;      /* Sign of the input angle */

    /* Find the upper and lower bound limits for the angle based on its
       usage type (LAT, LON, DEGREES).  */
    if (strcmp (check,"LAT") == 0)
    {
        maxdms = 90000000;
        mindms = -90000000;
    }
    else if (strcmp (check,"LON") == 0)
    {
        maxdms = 180000000;
        mindms = -180000000;
    }
    else
    { /* DEGREES */
        maxdms = 360000000;
        mindms = 0;
    }

    /* Convert the angle to DMS based on the coform in degrees.  */

    /* Extract the deg, min, and sec portions of the angle.  */
    ias_geo_find_deg(deg, &tdeg);
    ias_geo_find_min(deg, &tmin);
    ias_geo_find_sec(deg, &tsec);

    /* Find the sign of the angle based on the degrees.  */
    sign = 1;
    if (tdeg < 0)
        sign = -1;

    /* Calculate DMS from the degree, minutes, seconds values.  */
    tdeg = abs (tdeg);
    *dms = ((tdeg * 1000000) + (tmin * 1000) + tsec) * sign;

    /* Check to be sure the coordinate is within the bounds.  */
    if ((*dms > maxdms) || (*dms < mindms))
    {
        IAS_LOG_ERROR("Calculated DMS value of %f outside bounds of %f to %f",
            *dms, mindms, maxdms);
        return ERROR;
    }

    return SUCCESS;
}
