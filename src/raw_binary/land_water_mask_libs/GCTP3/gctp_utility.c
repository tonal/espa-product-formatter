/******************************************************************************
Name: gctp_utility.c

Purpose: Provides a number of simple utility routines.

******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "local.h"
#include "cproj.h"

/******************************************************************************
Name: gctp_print_transformation_info

Purpose: Prints out information about the two projections involved in the
    transformation.

Returns:
    nothing

******************************************************************************/
void gctp_print_transformation_info
(
    const GCTP_TRANSFORMATION *trans
)
{
    if (trans->forward.print_info)
    {
        GCTP_PRINT_INFO("Forward projection:");
        trans->forward.print_info(&trans->forward);
    }
    if (trans->inverse.print_info)
    {
        GCTP_PRINT_INFO("Inverse projection:");
        trans->inverse.print_info(&trans->inverse);
    }
}

/******************************************************************************
Name: gctp_get_input_proj

Purpose: Provides access to the input projection parameter information.

Returns:
    Pointer to the input projection parameters. 

******************************************************************************/
const GCTP_PROJECTION *gctp_get_input_proj
(
    const GCTP_TRANSFORMATION *trans
)
{
    return &trans->inverse.proj;
}

/******************************************************************************
Name: gctp_get_input_proj

Purpose: Provides access to the output projection parameter information.

Returns:
    Pointer to the output projection parameters. 

******************************************************************************/
const GCTP_PROJECTION *gctp_get_output_proj
(
    const GCTP_TRANSFORMATION *trans
)
{
    return &trans->forward.proj;
}

/******************************************************************************
Name: gctp_calc_utm_zone

Purpose: Calculate the UTM zone for a given longitude (in degrees).

Returns:
    The UTM zone (1 - 60)

******************************************************************************/
int gctp_calc_utm_zone
(
    double lon          /* I: longitude (in degrees) */
)
{
    return (int)(((lon + 180.0) / 6.0) + 1.0);
}

/******************************************************************************
Name: gctp_get_sign

Purpose: Get the sign of a number

Returns:
    -1 if number is < 0
    1 otherwise

******************************************************************************/
int gctp_get_sign
(
    double x
)
{
    if (x < 0.0)
        return -1;
    else
        return 1;
}

/******************************************************************************
Name: gctp_calc_e0|e1|e2|e3

Purpose: Functions to compute the constants e0, e1, e2, and e3 which are used
   in a series for calculating the distance along a meridian.  The input x
   represents the eccentricity squared.

Returns:
    Calculated value

******************************************************************************/
double gctp_calc_e0
(
    double x
)
{
    return 1.0-0.25*x*(1.0+x/16.0*(3.0+1.25*x));
}
double gctp_calc_e1
(
    double x
)
{
    return 0.375*x*(1.0+0.25*x*(1.0+0.46875*x));
}
double gctp_calc_e2
(
    double x
)
{
    return 0.05859375*x*x*(1.0+0.75*x);
}
double gctp_calc_e3
(
    double x
)
{
    return x*x*x*(35.0/3072.0);
}

/******************************************************************************
Name: gctp_calc_e4

Purpose: Function to compute the constant e4 from the input of the eccentricity
    of the spheroid, x.  This constant is used in the Polar Stereographic
    projection.

Returns:
    Calculated value

******************************************************************************/
double gctp_calc_e4
(
    double x
)
{
    double con;
    double com;
    con = 1.0 + x;
    com = 1.0 - x;
    return (sqrt((pow(con,con))*(pow(com,com))));
}

/******************************************************************************
Name: gctp_calc_dist_from_equator

Purpose: Function computes the value of M which is the distance along a
    meridian from the Equator to latitude phi.

Returns:
    Calculated value

******************************************************************************/
double gctp_calc_dist_from_equator
(
    double e0,
    double e1,
    double e2,
    double e3,
    double phi
)
{
    return e0*phi-e1*sin(2.0*phi)+e2*sin(4.0*phi)-e3*sin(6.0*phi);
}

/******************************************************************************
Name: gctp_calc_phi2

Purpose: Function to compute the latitude angle, phi2, for the inverse of the
    Lambert Conformal Conic and Polar Stereographic projections.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

******************************************************************************/
int gctp_calc_phi2
(
    double eccent,      /* I: Spheroid eccentricity */
    double ts,          /* I: Constant value t */
    double *phi2        /* O: calculated value of phi2 */
)
{
    double eccnth;
    double phi;
    double con;
    double dphi;
    double sinpi;
    long i;

    eccnth = .5 * eccent;
    phi = HALF_PI - 2 * atan(ts);
    for (i = 0; i <= 15; i++)
    {
        sinpi = sin(phi);
        con = eccent * sinpi;
        dphi = HALF_PI - 2 * atan(ts *(pow(((1.0 - con)/(1.0 + con)),eccnth))) 
            - phi;
        phi += dphi; 
        if (fabs(dphi) <= .0000000001)
        {
            *phi2 = phi;
            return GCTP_SUCCESS;
        }
    }

    GCTP_PRINT_ERROR("Failed to converge to a solution for phi2");
    return GCTP_ERROR;
}

/******************************************************************************
Name: gctp_calc_small_radius

Purpose: Function to compute the constant small m which is the radius of a
    parallel of latitude, phi, divided by the semimajor axis.

Returns:
    calculated value

******************************************************************************/
double gctp_calc_small_radius
(
    double eccent,
    double sinphi,
    double cosphi
)
{
    double con;

    con = eccent * sinphi;
    return (cosphi / (sqrt (1.0 - con * con)));
}

/******************************************************************************
Name: gctp_calc_small_t

Purpose: Function to compute the constant small t for use in the forward
    computations in the Lambert Conformal Conic and the Polar Stereographic
    projections.

Returns:
    calculated value

******************************************************************************/
double gctp_calc_small_t
(
    double eccent,	/* Eccentricity of the spheroid		*/
    double phi,		/* Latitude phi				*/
    double sinphi	/* Sine of the latitude			*/
)
{
    double con;
    double com;
  
    con = eccent * sinphi;
    com = .5 * eccent; 
    con = pow(((1.0 - con) / (1.0 + con)),com);
    return tan(.5 * (HALF_PI - phi))/con;
}
