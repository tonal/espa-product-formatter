/*******************************************************************************
NAME                             ORTHOGRAPHIC 

PURPOSE:	Transforms input longitude and latitude to Easting and
		Northing for the Orthographic projection.  The
		longitude and latitude must be in radians.  The Easting
		and Northing values will be returned in meters.

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include "cproj.h"
#include "local.h"

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
static double r_major;		/* major axis 				*/
static double lon_center;	/* Center longitude (projection center) */
static double lat_origin;	/* center latitude			*/
static double false_northing;	/* y offset in meters			*/
static double false_easting;	/* x offset in meters			*/
static double sin_p14;		/* sin of center latitude		*/
static double cos_p14;		/* cos of center latitude		*/

/* Initialize the Orthographic projection
  -------------------------------------*/
long orthforint
(
    double r_maj,			/* major axis			*/
    double center_lon,		/* center longitude		*/
    double center_lat,		/* center latitude		*/
    double false_east,		/* x offset in meters		*/
    double false_north		/* y offset in meters		*/
)
{
/* Place parameters in static storage for common use
  -------------------------------------------------*/
r_major = r_maj;
lon_center = center_lon;
lat_origin = center_lat;
false_northing = false_north;
false_easting = false_east;

sincos(center_lat,&sin_p14,&cos_p14);

/* Report parameters to the user
  -----------------------------*/
gctp_print_title("ORTHOGRAPHIC"); 
gctp_print_radius(r_major);
gctp_print_cenlonmer(lon_center);
gctp_print_origin(lat_origin);
gctp_print_offsetp(false_easting,false_northing);
return(OK);
}


/* Orthographic forward equations--mapping lat,long to x,y
  ---------------------------------------------------*/
long orthfor
(
    double lon,			/* (I) Longitude 		*/
    double lat,			/* (I) Latitude 		*/
    double *x,			/* (O) X projection coordinate 	*/
    double *y			/* (O) Y projection coordinate 	*/
)
{
double sinphi, cosphi;	/* sin and cos value				*/
double dlon;		/* delta longitude value			*/
double coslon;		/* cos of longitude				*/
double ksp;		/* scale factor					*/
double g;		

/* Forward equations
  -----------------*/
dlon = adjust_lon(lon - lon_center);
sincos(lat,&sinphi,&cosphi);
coslon = cos(dlon);
g = sin_p14 * sinphi + cos_p14 * cosphi * coslon;
ksp = 1.0;
if ((g > 0) || (fabs(g) <= EPSLN))
  {
  *x = false_easting + r_major * ksp * cosphi * sin(dlon);
  *y = false_northing + r_major * ksp * (cos_p14 * sinphi -
					 sin_p14 * cosphi * coslon);
  }
else
  {
  GCTP_PRINT_ERROR("Point can not be projected");
  return(143);
  }
return(OK);
}
