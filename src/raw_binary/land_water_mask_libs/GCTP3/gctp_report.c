/*******************************************************************************
Name: gctp_report

Purpose: Provides some routines for printing out information about projection
    parameters.

*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cproj.h"
#include "local.h"

void gctp_print_title
(
    const char *proj_name
) 
{
    GCTP_PRINT_INFO("%s PROJECTION PARAMETERS:", proj_name); 
}

void gctp_print_radius
(
    double radius
)
{
    GCTP_PRINT_INFO("   Radius of Sphere:     %f meters", radius); 
}

void gctp_print_radius2
(
    double radius1,
    double radius2
)
{
    GCTP_PRINT_INFO("   Semi-Major Axis of Ellipsoid:     %f meters", radius1);
    GCTP_PRINT_INFO("   Semi-Minor Axis of Ellipsoid:     %f meters", radius2);
}

void gctp_print_cenlon
(
    double A
)
{ 
    GCTP_PRINT_INFO("   Longitude of Center:     %f degrees", A * R2D);
}
 
void gctp_print_cenlonmer
(
    double A
)
{ 
    GCTP_PRINT_INFO("   Longitude of Central Meridian:     %f degrees",
        A * R2D);
}

void gctp_print_cenlat
(
    double A
)
{
    GCTP_PRINT_INFO("   Latitude  of Center:     %f degrees", A * R2D);
}

void gctp_print_origin
(
    double A
)
{
    GCTP_PRINT_INFO("   Latitude of Origin:     %f degrees", A * R2D);
}

void gctp_print_stanparl
(
    double A,
    double B
)
{
    GCTP_PRINT_INFO("   1st Standard Parallel:     %f degrees", A * R2D);
    GCTP_PRINT_INFO("   2nd Standard Parallel:     %f degrees", B * R2D);
}

void gctp_print_stparl1
(
    double A
)
{
    GCTP_PRINT_INFO("   Standard Parallel:     %f degrees", A * R2D);
}

void gctp_print_offsetp
(
    double A,
    double B
)
{
    GCTP_PRINT_INFO("   False Easting:      %f meters", A);
    GCTP_PRINT_INFO("   False Northing:     %f meters", B);
}

void gctp_print_lat_zone
(
    double A
)
{
    GCTP_PRINT_INFO("Number of Latitudinal Zones:  %lf", A);
}

void gctp_print_justify_cols
(
    double A
)
{
    GCTP_PRINT_INFO("Right Justify Columns Flag:  %lf", A);
}

void gctp_print_genrpt
(
    double A,
    const char *S
)
{
    GCTP_PRINT_INFO("   %s %f", S, A);
}

void gctp_print_genrpt_long
(
    long A,
    const char *S
)
{
    GCTP_PRINT_INFO("   %s %ld", S, A);
}
