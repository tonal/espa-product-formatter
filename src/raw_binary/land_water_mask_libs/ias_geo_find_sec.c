/****************************************************************************
NAME: ias_geo_find_sec

PURPOSE: Extracts sec portions of an angle.

RETURNS: void

ALGORITHM DESCRIPTION:
        Extract seconds portion of angle
        Return

*****************************************************************************/
#include <math.h>
#include "ias_lw_geo.h"

void ias_geo_find_sec 
(
    double angle,                 /* I: Angle in total degrees */
    double *second                /* O: Second portion of the angle */
)
{
    int temp_sec;

    *second = fabs(angle);
    *second -= (int)(*second);
    *second *= 60.0;
    *second -= (int)(*second);
    *second *= 60.0;
    if (*second > 60.0) 
        *second -= 60.0;
    temp_sec = (int) (*second * 1000);      /* Truncate to 0.001 sec */
    *second = temp_sec / 1000.0;
}
