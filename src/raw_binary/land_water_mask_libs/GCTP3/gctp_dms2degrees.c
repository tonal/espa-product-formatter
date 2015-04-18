/*******************************************************************************
Name: gctp_dms2degrees

Purpose: Converts a packed DMS angle to degrees.  The standard packed DMS
format is:

        degrees * 1000000 + minutes * 1000 + seconds

        Example:    angle = 120025045.25 yields
                deg = 120
                min = 25
                sec = 45.25

        The algorithm used for the conversion is as follows:

        1.  The absolute value of the angle is used.

        2.  The degrees are separated out:
            deg = angle/1000000   (fractional portion truncated)
    
        3.  The minutes are separated out:
            min = (angle - deg * 1000000) / 1000     (fractional
                            portion truncated)

        4.  The seconds are then computed:
            sec = angle - deg * 1000000 - min * 1000

        5.  The total angle in seconds is computed:
            sec = deg * 3600.0 + min * 60.0 + sec

        6.  The sign of sec is set to that of the input angle.

        7.  The total degrees are computed:
            degrees = sec / 3600.0

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Proffesional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include "local.h"
#include "cproj.h"

int gctp_dms2degrees
(
    double angle,       /* I: angle in DMS */
    double *degrees     /* O: angle in degrees */
)
{
    double fac;     /* sign flag */
    double deg;     /* degree variable */
    double min;     /* minute variable */
    double sec;     /* seconds variable */
    double tmp;     /* temporary variable */

    if (angle < 0.0)
        fac = -1;
    else
        fac = 1;

    /* find degrees */
    sec = fabs(angle);
    tmp = 1000000.0;
    deg = (int) (sec/tmp);
    if (deg > 360)
    {
        GCTP_PRINT_ERROR("Illegal DMS degrees field: %f", deg);
        return GCTP_ERROR;
    }

    /* find minutes */
    sec = sec - deg * tmp;
    tmp = 1000;
    min = (int) (sec / tmp);
    if (min > 60)
    {
        GCTP_PRINT_ERROR("Illegal DMS minutes field: %f", min);
        return GCTP_ERROR;
    }

    /* find seconds */
    sec = sec - min * tmp;
    if (sec > 60)
    {
        GCTP_PRINT_ERROR("Illegal DMS seconds field: %f", sec);
        return GCTP_ERROR;
    }
    else
        sec = fac * (deg * 3600.0 + min * 60.0 + sec);
    deg = sec / 3600.0;

    *degrees = deg;

    return GCTP_SUCCESS;
}
