#ifndef PROJ_H
#define PROJ_H


/* The STPLN_TABLE unit value is specifically used for State Plane -- if units
   equals STPLN_TABLE and Datum is NAD83--actual units are retrieved from
   a table according to the zone.  If Datum is NAD27--actual units will be feet.
   An error will occur with this unit if the projection is not State Plane.  */
#define STPLN_TABLE 6

/* General code numbers */

#define IN_BREAK -2		/* Return status if the interupted projection
				    point lies in the break area */
#define COEFCT 15		/* projection coefficient count */
#define PROJCT (MAXPROJ + 1) /* count of supported projections */
#define SPHDCT 32		/* spheroid count */

#define MAXUNIT 5		/* Maximum unit code number */
#define GEO_TERM 0		/* Array index for print-to-term flag */
#define GEO_FILE 1		/* Array index for print-to-file flag */
#define GEO_TRUE 1		/* True value for geometric true/false flags */
#define GEO_FALSE -1		/*  False val for geometric true/false flags */

/* GCTP Function prototypes */

long alberforint
(
    double r_maj,
    double r_min,
    double lat1,
    double lat2,
    double lon0,
    double lat0,
    double false_east,
    double false_north
);

long alberfor
(
    double lon,
    double lat,
    double *x,
    double *y
);

long alberinvint
(
    double r_maj,               /* major axis                           */
    double r_min,               /* minor axis                           */
    double lat1,                /* first standard parallel              */
    double lat2,                /* second standard parallel             */
    double lon0,                /* center longitude                     */
    double lat0,                /* center lattitude                     */
    double false_east,          /* x offset in meters                   */
    double false_north          /* y offset in meters                   */
);

long alberinv
(
    double x,                   /* (O) X projection coordinate  */
    double y,                   /* (O) Y projection coordinate      */
    double *lon,                /* (I) Longitude                */
    double *lat                 /* (I) Latitude             */
);

long alconforint
(
    double r_maj,               /* Major axis                           */
    double r_min,               /* Minor axis                           */
    double false_east,          /* x offset in meters                   */
    double false_north          /* y offset in meters                   */
);

long alconfor
(
    double lon,                 /* (I) Longitude */
    double lat,                 /* (I) Latitude */
    double *x,                  /* (O) X projection coordinate */
    double *y                   /* (O) Y projection coordinate */
);

long alconinvint
(
    double r_maj,               /* Major axis                           */
    double r_min,               /* Minor axis                           */
    double false_east,          /* x offset in meters                   */
    double false_north          /* y offset in meters                   */
);

long alconinv
(
    double x,                   /* (O) X projection coordinate */
    double y,                   /* (O) Y projection coordinate */
    double *lon,                /* (I) Longitude */
    double *lat                 /* (I) Latitude */
);

long azimforint
(
    double r_maj,               /* major axis                   */
    double center_lon,          /* center longitude             */
    double center_lat,          /* center latitude              */
    double false_east,          /* x offset in meters           */
    double false_north          /* y offset in meters           */
);

long azimfor
(
    double lon,                 /* (I) Longitude                */
    double lat,                 /* (I) Latitude                 */
    double *x,                  /* (O) X projection coordinate  */
    double *y                   /* (O) Y projection coordinate  */
);

long aziminvint
(
    double r_maj,               /* major axis                   */
    double center_lon,          /* center longitude             */
    double center_lat,          /* center latitude              */
    double false_east,          /* x offset in meters           */
    double false_north          /* y offset in meters           */
);

long aziminv
(
    double x,                   /* (O) X projection coordinate  */
    double y,                   /* (O) Y projection coordinate  */
    double *lon,                /* (I) Longitude                */
    double *lat                 /* (I) Latitude                 */
);

void sincos
(
    double val,
    double *sin_val,
    double *cos_val
);

double asinz
(
    double con
);

double qsfnz
(
    double eccent,
    double sinphi
);

double phi1z
(
    double eccent,           /* Eccentricity angle in radians                */
    double qs,               /* Angle in radians                             */
    long *flag               /* Error flag number                            */
);

double phi3z
(
    double ml,               /* Constant                     */
    double e0,               /* Constant                     */
    double e1,               /* Constant                     */
    double e2,               /* Constant                     */
    double e3,               /* Constant                     */
    long *flag               /* Error flag number            */
);

double adjust_lon
(
    double x                 /* Angle in radians                     */
);

long eqconforint
(
    double r_maj,            /* major axis                   */
    double r_min,            /* minor axis                   */
    double lat1,             /* latitude of standard parallel*/
    double lat2,             /* latitude of standard parallel*/
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north,      /* y offset in meters           */
    long mode                /* which format is present A B  */
);

long eqconfor
(
    double lon,              /* (I) Longitude                */
    double lat,              /* (I) Latitude                 */
    double *x,               /* (O) X projection coordinate  */
    double *y                /* (O) Y projection coordinate  */
);

long eqconinvint
(
    double r_maj,            /* major axis                   */
    double r_min,            /* minor axis                   */
    double lat1,             /* latitude of standard parallel*/
    double lat2,             /* latitude of standard parallel*/
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north,      /* y offset in meters           */
    long mode                /* which format is present A B  */
);

long eqconinv
(
    double x,                /* (O) X projection coordinate  */
    double y,                /* (O) Y projection coordinate  */
    double *lon,             /* (I) Longitude                */
    double *lat              /* (I) Latitude                 */
);

long equiforint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double lat1,             /* latitude of true scale       */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long equifor
(
    double lon,              /* (I) Longitude                */
    double lat,              /* (I) Latitude                 */
    double *x,               /* (O) X projection coordinate  */
    double *y                /* (O) Y projection coordinate  */
);

long equiinvint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double lat1,             /* latitude of true scale       */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long equiinv
(
    double x,                /* (O) X projection coordinate  */
    double y,                /* (O) Y projection coordinate  */
    double *lon,             /* (I) Longitude                */
    double *lat              /* (I) Latitude                 */
);

void for_init
(
    long outsys,             /* output system code                           */
    long outzone,            /* output zone number                           */
    const double *outparm,   /* output array of projection parameters        */
    long outspheroid,        /* output spheroid                              */
    long *iflg,              /* status flag                                  */
    long (*for_trans[])(double,double,double*,double*)
                             /* forward function pointer                     */
);

void gctp
(
    const double *incoor,   /* input coordinates */
    const long *insys,      /* input projection code */
    const long *inzone,     /* input zone number */
    const double *inparm,   /* input projection parameter array */
    long *inunit,           /* input units */
    const long *inspheroid, /* input spheroid */
    double *outcoor,        /* output coordinates */
    const long *outsys,     /* output projection code */
    const long *outzone,    /* output zone */
    const double *outparm,  /* output projection array */
    long *outunit,          /* output units */
    const long *outspheroid,/* output spheroid */
    long *iflg              /* error flag */
);

long gnomforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long gnomfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long gnominvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long gnominv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long goodforint
(
    double r                 /* (I) Radius of the earth (sphere) */
);

long goodfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long goodinvint
(
    double r                 /* (I) Radius of the earth (sphere) */
);

long goodinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long gvnspforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double h,                /* height above sphere                  */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long gvnspfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long gvnspinvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double h,                /* height above sphere                  */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long gvnspinv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long hamforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long hamfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long haminvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long haminv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long imolwforint
(
    double r                 /* (I) Radius of the earth (sphere) */
);

long imolwfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long imolwinvint
(
    double r                 /* (I) Radius of the earth (sphere) */
);

long imolwinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

void inv_init
(
    long insys,              /* input system code                            */
    long inzone,             /* input zone number                            */
    const double *inparm,    /* input array of projection parameters         */
    long inspheroid,         /* input spheroid code                          */
    long *iflg,              /* status flag                                  */
    long (*inv_trans[])(double,double,double*,double*)
                             /* inverse function pointer                     */
);

long isinusforinit
( 
    double sphere,           /* (I) Radius of the earth (sphere) */
    double lon_cen_mer,	     /* (I) Longitude of central meridian (radians) */
    double false_east,	     /* (I) Easting at projection origin (meters) */
    double false_north,	     /* (I) Northing at projection origin (meters) */
    double dzone,	     /* (I) Number of longitudinal zones */
    double djustify	     /* (I) Justify (flag for rows w/odd # of columns)*/
);

long isinusfor
(
    double lon,	             /* (I) Longitude */
    double lat,	             /* (I) Latitude */
    double *x,	             /* (O) X projection coordinate */
    double *y	             /* (O) Y projection coordinate */
);

long isinusinvinit
(
    double sphere,	     /* (I) Radius of the earth (sphere) */
    double lon_cen_mer,	     /* (I) Longitude of central meridian (radians) */
    double false_east,	     /* (I) Easting at projection origin (meters) */
    double false_north,	     /* (I) Northing at projection origin (meters) */
    double dzone,	     /* (I) Number of longitudinal zones */
    double djustify	     /* (I) Justify (flag for rows w/odd # of columns)*/
);

long isinusinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long lamazforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long lamazfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long lamazinvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double center_lat,       /* (I) Center latitude                  */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long lamazinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long merforint
(
    double r_maj,            /* major axis                   */
    double r_min,            /* minor axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long merfor
(
    double lon,              /* (I) Longitude                */
    double lat,              /* (I) Latitude                 */
    double *x,               /* (O) X projection coordinate  */
    double *y                /* (O) Y projection coordinate  */
);

long merinvint
(
    double r_maj,            /* major axis                   */
    double r_min,            /* minor axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long merinv
(
    double x,                /* (O) X projection coordinate  */
    double y,                /* (O) Y projection coordinate  */
    double *lon,             /* (I) Longitude                */
    double *lat              /* (I) Latitude                 */
);

long millforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long millfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long millinvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long millinv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long molwforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long molwfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long molwinvint
(
    double r,                /* (I) Radius of the earth (sphere) */
    double center_long,      /* (I) Center longitude */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long molwinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long obleqforint
(
    double r,
    double center_long,
    double center_lat,
    double shape_m,
    double shape_n,
    double angle,
    double false_east,
    double false_north
);

long obleqfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long obleqinvint
(
    double r,
    double center_long,
    double center_lat,
    double shape_m,
    double shape_n,
    double angle,
    double false_east,
    double false_north
);

long obleqinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long orthforint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long orthfor
(
    double lon,              /* (I) Longitude                */
    double lat,              /* (I) Latitude                 */
    double *x,               /* (O) X projection coordinate  */
    double *y                /* (O) Y projection coordinate  */
);

long orthinvint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long orthinv
(
    double x,                /* (O) X projection coordinate  */
    double y,                /* (O) Y projection coordinate  */
    double *lon,             /* (I) Longitude                */
    double *lat              /* (I) Latitude                 */
);

double paksz
(
    double ang,              /* angle which in DMS           */
    long *iflg               /* error flag number            */
);

long robforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long robfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long robinvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long robinv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long sinforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long sinfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long sininvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long sininv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

void sphdz
(
    long isph,               /* spheroid code number                         */
    const double *parm,      /* projection parameters                        */
    double *r_major,         /* major axis                                   */
    double *r_minor,         /* minor axis                                   */
    double *radius           /* radius                                       */
);

long sterforint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long sterfor
(
    double lon,              /* (I) Longitude                */
    double lat,              /* (I) Latitude                 */
    double *x,               /* (O) X projection coordinate  */
    double *y                /* (O) Y projection coordinate  */
);

long sterinvint
(
    double r_maj,            /* major axis                   */
    double center_lon,       /* center longitude             */
    double center_lat,       /* center latitude              */
    double false_east,       /* x offset in meters           */
    double false_north       /* y offset in meters           */
);

long sterinv
(
    double x,                /* (O) X projection coordinate  */
    double y,                /* (O) Y projection coordinate  */
    double *lon,             /* (I) Longitude                */
    double *lat              /* (I) Latitude                 */
);

long untfz
(
    long inunit,
    long outunit,
    double *factor
);

long vandgforint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long vandgfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long vandginvint
(
    double r,                /* (I) Radius of the earth (sphere)     */
    double center_long,      /* (I) Center longitude                 */
    double false_east,       /* x offset in meters                   */
    double false_north       /* y offset in meters                   */
);

long vandginv
(
    double x,                /* (O) X projection coordinate */
    double y,                /* (O) Y projection coordinate */
    double *lon,             /* (I) Longitude */
    double *lat              /* (I) Latitude */
);

long wivforint
(
    double r,                /* (I) Radius of the earth (sphere) */
    double center_long,      /* (I) Center longitude */
    double false_east,       /* x offset                             */
    double false_north       /* y offset                             */
);

long wivfor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long wivinvint
(
    double r,                /* (I) Radius of the earth (sphere) */
    double center_long,      /* (I) Center longitude */
    double false_east,       /* x offset                             */
    double false_north       /* y offset                             */
);

long wivinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

long wviiforint
(
    double r,                /* (I) Radius of the earth (sphere) */
    double center_long,      /* (I) Center longitude */
    double false_east,       /* x offset                             */
    double false_north       /* y offset                             */
);

long wviifor
(
    double lon,              /* (I) Longitude */
    double lat,              /* (I) Latitude */
    double *x,               /* (O) X projection coordinate */
    double *y                /* (O) Y projection coordinate */
);

long wviiinvint
(
    double r,                /* (I) Radius of the earth (sphere) */
    double center_long,      /* (I) Center longitude */
    double false_east,       /* x offset                             */
    double false_north       /* y offset                             */
);

long wviiinv
(
    double x,                /* (I) X projection coordinate */
    double y,                /* (I) Y projection coordinate */
    double *lon,             /* (O) Longitude */
    double *lat              /* (O) Latitude */
);

#endif
