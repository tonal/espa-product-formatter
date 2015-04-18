/****************************************************************************
NAME: ias_geo_find_deg

PURPOSE: Extracts deg portions of an angle.

RETURNS: void

ALGORITHM DESCRIPTION:
        Extract degree portion of angle
        Return

*****************************************************************************/
#include <math.h>
#include "ias_lw_geo.h"

void ias_geo_find_deg 
(
    double angle, /* I: Angle in total degrees */
    int *degree   /* O: Degree portion of the angle */
)
{
    int sign;     /* Sign of angle */
    int minute;   /* Minutes portion of angle */
    double sec;   /* Seconds portion of angle */

    *degree = (int) angle;
    sign = 1;
    if (*degree < 0) 
        sign = -1;
    *degree = (int) fabs (angle);
    minute =  (int) ((fabs (angle) - *degree) * 60.0);
    sec =  (((fabs (angle) - *degree) * 60.0) - minute) * 60.0;
    if (sec >= 60.0) 
        minute++;
    if (minute >= 60) 
        (*degree)++;
    *degree *= sign;
}
