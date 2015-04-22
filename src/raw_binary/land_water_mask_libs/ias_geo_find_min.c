/****************************************************************************
NAME: ias_geo_find_min

PURPOSE: Extracts min portions of an angle.

RETURNS: void

ALGORITHM DESCRIPTION:
        Extract minute portion of angle
        Return

*****************************************************************************/
#include <math.h>
#include "ias_lw_geo.h"

void ias_geo_find_min 
(
    double angle, /* I: Angle in total degrees */
    int *minute   /* O: Minute portion of the angle */
)
{
    double sec;   /* Seconds portion of angle */

    angle = fabs (angle);
    angle -= (int)angle;
    *minute = (int) (angle * 60.0);
    sec = ((angle * 60.0) - *minute) * 60.0;
    if (sec > 60.0) 
        (*minute)++;
    if (*minute >= 60) 
        *minute -= 60;
}
