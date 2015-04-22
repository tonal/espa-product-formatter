/****************************************************************************
Name: gctp_transform

Purpose: Performs a coordinate transformation with the given previously
    created transformation.

****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "gctp.h"
#include "local.h"
#include "cproj.h"

/****************************************************************************
Name: call_gctp

Purpose: Helper routine to perform a transformation by calling the old gctp
    interface.  Note that this routine should be eliminated when all of the
    projections have been converted to the new interface.

Returns: GCTP_SUCCESS, GCTP_ERROR or GCTP_IN_BREAK

****************************************************************************/
static int call_gctp
(
    const GCTP_TRANSFORMATION *trans, /* I: transformation to use */
    const double *in_coor, /* I: array of (lon, lat) or (x, y) */
    double *out_coor       /* O: array of (x, y) or (lon, lat) */
)
{
    /* Set up local variables for calling the old gctp routine */
    const GCTP_PROJECTION *in_proj = &trans->inverse.proj;
    const GCTP_PROJECTION *out_proj = &trans->forward.proj;
    long insys = in_proj->proj_code;
    long inzone = in_proj->zone;
    long inunit = in_proj->units;
    long inspheroid = in_proj->spheroid;
    long outsys = out_proj->proj_code;
    long outzone = out_proj->zone;
    long outunit = out_proj->units;
    long outspheroid = out_proj->spheroid;
    long iflg = 0;

    /* Call the original gctp routine */
    gctp(in_coor, &insys, &inzone, in_proj->parameters, &inunit, &inspheroid,
        out_coor, &outsys, &outzone, out_proj->parameters, &outunit,
        &outspheroid, &iflg);

    /* Convert the error flag */
    if (iflg == OK)
        return GCTP_SUCCESS;
    else if (iflg == IN_BREAK)
        return GCTP_IN_BREAK;
    else
        return GCTP_ERROR;
}

/****************************************************************************
Name: gctp_transform

Purpose: Performs a coordinate transformation with the given previously
    created transformation.

Returns: GCTP_SUCCESS, GCTP_ERROR or GCTP_IN_BREAK

****************************************************************************/
int gctp_transform
(
    const GCTP_TRANSFORMATION *trans, /* I: transformation to use */
    const double *in_coor, /* I: array of (lon, lat) or (x, y) */
    double *out_coor       /* O: array of (x, y) or (lon, lat) */
)
{
    double lon;
    double lat;
    double x;
    double y;

    /* Verify the transformation provided is valid */
    if (!trans)
    {
        GCTP_PRINT_ERROR("Invalid transformation provided");
        return GCTP_ERROR;
    }

    /* If the use_gctp flag is set, fall back to using gctp */
    /* TODO - remove this after all the projections have been converted */
    if (trans->use_gctp)
    {
        return call_gctp(trans, in_coor, out_coor);
    }

    /* Convert the input coordinate into the correct units for this
       transformation since the transforms always operate in radians or meters
       and the caller may have provided the coordinate in different units */
    x = in_coor[0] * trans->inverse.unit_conversion_factor;
    y = in_coor[1] * trans->inverse.unit_conversion_factor;

    if (trans->inverse.transform)
    {
        int status;

        status = trans->inverse.transform(&trans->inverse, x, y, &lon, &lat);
        if (status != GCTP_SUCCESS)
        {
            if (status == IN_BREAK)
            {
                /* In a break area, so return that indication */
                return GCTP_IN_BREAK;
            }
            GCTP_PRINT_ERROR("Error in inverse transformation");
            return GCTP_ERROR;
        }
    }
    else
    {
        /* no inverse transform, so copy input to lat/lon */
        lon = x;
        lat = y;
    }

    if (trans->forward.transform)
    {
        if (trans->forward.transform(&trans->forward, lon, lat,
                &out_coor[0], &out_coor[1]) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error in forward transformation");
            return GCTP_ERROR;
        }
    }
    else
    {
        /* no forward transform, so copy input to temp */
        out_coor[0] = lon;
        out_coor[1] = lat;
    }

    /* Convert the output coordinate into the correct units for this
       transformation since the transforms always operate in radians or meters
       and the caller may have requested different units */
    out_coor[0] *= trans->forward.unit_conversion_factor;
    out_coor[1] *= trans->forward.unit_conversion_factor;

    return GCTP_SUCCESS;
}
