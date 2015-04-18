/*******************************************************************************
NAME                           FOR_INIT 

PURPOSE:	Initializes forward projection transformation parameters

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include "cproj.h"
#include "gctp.h"

void for_init
(
    long outsys,		/* output system code				*/
    long outzone,		/* output zone number				*/
    const double *outparm, /* output array of projection parameters	*/
    long outspheroid,	/* output spheroid				*/
    long *iflg,		/* status flag					*/
    long (*for_trans[])(double, double, double*, double*)
                    /* forward function pointer			*/
)
{
double angle;		/* rotation anlge				*/
double lat1 = 0.0;		/* 1st standard parallel			*/
double lat2 = 0.0;		/* 2nd standard parallel			*/
double center_long;	/* center longitude				*/
double center_lat;	/* center latitude				*/
double h;		/* height above sphere				*/
double lat_origin;	/* latitude at origin				*/
double r_major;		/* major axis in meters				*/
double r_minor;		/* minor axis in meters				*/
double false_easting;	/* false easting in meters			*/
double false_northing;	/* false northing in meters			*/
double shape_m;		/* constant used for Oblated Equal Area		*/
double shape_n;		/* constant used for Oblated Equal Area		*/
double radius;		/* radius of sphere				*/
long mode;		/* which initialization method  to use A or B	*/
double dzone;       /* number of longitudinal zones in ISG          */
double djustify;    /* justify flag in ISG projection               */

	/* Function declarations for function pointer use
	-----------------------------------------------*/
long alberfor();
long merfor();
long eqconfor();
long sterfor();
long lamazfor();
long azimfor();
long gnomfor();
long orthfor();
long gvnspfor();
long sinfor();
long equifor();
long millfor();
long vandgfor();
long hamfor();
long robfor();
long goodfor();
long molwfor();
long imolwfor();
long alconfor();
long wivfor();
long wviifor();
long obleqfor();
long isinusfor();

/* Initialize forward transformations
-----------------------------------*/
  /* find the correct major and minor axis
  --------------------------------------*/
  sphdz(outspheroid,outparm,&r_major,&r_minor,&radius);
  false_easting  = outparm[6];
  false_northing = outparm[7];

  if ((outsys == HOM)
      || (outsys == LAMCC)
      || (outsys == UTM)
      || (outsys == TM)
      || (outsys == POLYC)
      || (outsys == PS)
      || (outsys == SOM)
      || (outsys == SPCS))
    {
    /* For projections no longer supported via the old interface, consider it
       an error */
    *iflg = ERROR;
    return;
    }
  else
  if (outsys == ALBERS)
    {
    /* this is the call to initialize ALBERS CONICAL EQUAL AREA 
    ----------------------------------------------------------*/
    lat1 = paksz(outparm[2],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat2 = paksz(outparm[3],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat_origin = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = alberforint(r_major,r_minor,lat1,lat2,center_long,lat_origin,
		       false_easting, false_northing);
    for_trans[outsys] = alberfor;
    }
  else
  if (outsys == MERCAT)
    {
    /* this is the call to initialize MERCATOR
    ----------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat1   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = merforint(r_major,r_minor,center_long,lat1,false_easting,
		     false_northing);
    for_trans[outsys] = merfor;
    }
  else
  if (outsys == EQUIDC)
    {
    /* this is the call to initialize EQUIDISTANT CONIC 
    -------------------------------------------------*/
    lat1 = paksz(outparm[2],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat2 = paksz(outparm[3],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat_origin   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    if (outparm[8] == 0)
       mode = 0;
    else 
       mode = 1;
    *iflg = eqconforint(r_major,r_minor,lat1,lat2,center_long,lat_origin,
		false_easting,false_northing,mode);
    for_trans[outsys] = eqconfor;
    }
  else
  if (outsys == STEREO)
    {
    /* this is the call to initialize STEREOGRAPHIC
    ---------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = sterforint(radius,center_long,center_lat,false_easting,
		      false_northing); 
    for_trans[outsys] = sterfor;
    }
  else
  if (outsys == LAMAZ)
    {
    /* this is the call to initialize LAMBERT AZIMUTHAL
    -------------------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat  = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = lamazforint(radius,center_long, center_lat,false_easting,
		       false_northing);
    for_trans[outsys] = lamazfor;
    }
  else
  if (outsys == AZMEQD)
    {
    /* this is the call to initialize AZIMUTHAL EQUIDISTANT
    -----------------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = azimforint(radius,center_long,center_lat,false_easting,
		      false_northing); 
    for_trans[outsys] = azimfor;
    }
  else
  if (outsys == GNOMON)
    {
    /* this is the call to initialize GNOMONIC 
    ----------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = gnomforint(radius,center_long,center_lat,false_easting,
		      false_northing);
    for_trans[outsys] = gnomfor;
    }
  else
  if (outsys == ORTHO)
    {
    /* this is the call to initalize ORTHOGRAPHIC
    -------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = orthforint(radius,center_long,center_lat,false_easting,
		      false_northing); 
    for_trans[outsys] = orthfor;
    }
  else
  if (outsys == GVNSP)
    {
    /* this is the call to initalize GENERAL VERTICAL NEAR-SIDE PERSPECTIVE
    ----------------------------------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    h = outparm[2];
    *iflg = gvnspforint(radius,h,center_long,center_lat,false_easting,
		       false_northing);
    for_trans[outsys] = gvnspfor;
    }
  else
  if (outsys == SNSOID)
    {
    /* this is the call to initialize SINUSOIDAL 
    -------------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = sinforint(radius, center_long,false_easting,false_northing);
    for_trans[outsys] = sinfor;
    }
  else
  if (outsys == EQRECT)
    {
    /* this is the call to initialize EQUIRECTANGULAR
    -----------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    lat1   = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = equiforint(radius,center_long,lat1,false_easting,false_northing); 
    for_trans[outsys] = equifor;
    }
  else
  if (outsys == MILLER)
    {
    /* this is the call to initialize MILLER CYLINDRICAL 
    --------------------------------------------------*/
    center_long  = paksz(outparm[4],iflg) * 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = millforint(radius, center_long,false_easting,false_northing); 
    for_trans[outsys] = millfor;
    }
  else
  if (outsys == VGRINT)
    {
    /* this is the call to initialize VAN DER GRINTEN 
    -----------------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = vandgforint(radius, center_long,false_easting,false_northing); 
    for_trans[outsys] = vandgfor;
    }
  else
  if (outsys == HAMMER)
    {
    /* this is the call to initialize HAMMER 
    --------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = hamforint(radius,center_long,false_easting,false_northing); 
    for_trans[outsys] = hamfor;
    }
  else
  if (outsys == ROBIN)
    {
    /* this is the call to initialize ROBINSON 
    ----------------------------------------*/
    center_long  = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = robforint(radius,center_long,false_easting,false_northing); 
    for_trans[outsys] = robfor;
    }
  else
  if (outsys == GOOD)
    {
    /* this is the call to initialize GOODE'S HOMOLOSINE
    ---------------------------------------------------*/
    *iflg = goodforint(radius);
    for_trans[outsys] = goodfor;
    }
  else
  if (outsys == MOLL)
    {
    /* this is the call to initialize MOLLWEIDE
    ------------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = molwforint(radius, center_long,false_easting,false_northing);
    for_trans[outsys] = molwfor;
    }
  else
  if (outsys == IMOLL)
    {
    /* this is the call to initialize INTERRUPTED MOLLWEIDE
    -----------------------------------------------------*/
    *iflg = imolwforint(radius);
    for_trans[outsys] = imolwfor;
    }
  else
  if (outsys == ALASKA)
    {
    /* this is the call to initialize ALASKA CONFORMAL 
    ------------------------------------------------*/
    *iflg = alconforint(r_major,r_minor,false_easting,false_northing);
    for_trans[outsys] = alconfor;
    }
  else
  if (outsys == WAGIV)
    {
    /* this is the call to initialize WAGNER IV 
    -----------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = wivforint(radius, center_long,false_easting,false_northing);
    for_trans[outsys] = wivfor;
    }
  else
  if (outsys == WAGVII)
    {
    /* this is the call to initialize WAGNER VII 
    ------------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = wviiforint(radius, center_long,false_easting,false_northing);
    for_trans[outsys] = wviifor;
    }
  else
  if (outsys == OBEQA)
    {
    /* this is the call to initialize OBLATED EQUAL AREA 
    ---------------------------------------------------*/
    center_long = paksz(outparm[4],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    center_lat  = paksz(outparm[5],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    shape_m = outparm[2];
    shape_n = outparm[3];
    angle = paksz(outparm[8],iflg)* 3600 * S2R;
    if (*iflg != 0)
       return;
    *iflg = obleqforint(radius,center_long,center_lat,shape_m, shape_n, 
		angle,false_easting,false_northing);
    for_trans[outsys] = obleqfor;
    }
   
  else if ( outsys == ISIN )
    {
    /* this is the call to initialize INTEGERIZED SINUSOIDAL GRID
    ------------------------------------------------------------ */
    center_long = paksz( outparm[4], iflg ) * 3600 * S2R;
    if ( *iflg != 0 )
        return;

    dzone = outparm[8];
    djustify = outparm[10];

    *iflg = isinusforinit( radius, center_long, false_easting, false_northing,
                           dzone, djustify );
    for_trans[outsys] = isinusfor;
    }
}
