/******************************************************************************
MODULE:  deg_to_dms

PURPOSE:  Converts the decimal degree value to DMS in the form of DDDMMMSSS.ss.

RETURN VALUE:
Type = double
Value      Description
-----      -----------
{all}      Packed DMS value

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
7/24/2014    Gail Schmidt     Original Development

NOTES:
******************************************************************************/
double deg_to_dms
(
    double flt_deg   /* I: input decimal degree value */
)
{
    int deg;         /* integer degree portion of the input value */
    int min;         /* integer minute portion of the input value */
    float sec;       /* seconds portion of the input value */
    double dms;      /* packed degrees, minutes, seconds */

    /* Grab the integer degrees from the input value */
    deg = (int) flt_deg;

    /* Obtain the integer minutes from the input value */
    min = (int) ((flt_deg - deg) * 60.0);

    /* Obtain the seconds from the input value */
    sec = (flt_deg - deg - min / 60.0) * 3600.0;

    /* If the seconds or minutes are 60, handle that case */
    if (sec >= 60.0)
    {
        sec -= 60.0;
        min++;
    }
    if (min >= 60)
    {
        min -= 60;
        deg++;
    }

    /* Create the packed DMS value */
    dms = deg * 1000000.0 + min * 1000.0 + sec;

    return (dms);
}

