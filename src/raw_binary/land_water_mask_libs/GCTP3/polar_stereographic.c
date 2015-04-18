/*******************************************************************************
Name: POLAR STEREOGRAPHIC 

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the Polar Stereographic projection.  The Easting and Northing
    values are in meters.  The longitude and latitude are in radians. 

Algorithm References

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdlib.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"

/* structure to hold the setup data relevant to this projection */
struct ps_proj
{
    double r_major;         /* major axis */
    double r_minor;         /* minor axis */
    double e;               /* eccentricity */
    double e4;              /* e4 calculated from eccentricity */
    double center_lon;      /* center longitude */
    double center_lat;      /* center latitude */
    double fac;             /* sign variable */
    double ind;             /* flag variable */
    double mcs;             /* small value m */
    double tcs;             /* small value t */
    double false_northing;  /* y offset in meters */
    double false_easting;   /* x offset in meters */
};

/*****************************************************************************
Name: print_info

Purpose: Prints a summary of information about this projection.

Returns:
    nothing

*****************************************************************************/
static void print_info
(
    const TRANSFORMATION *trans
)
{
    struct ps_proj *cache_ptr = (struct ps_proj *)trans->cache;

    gctp_print_title("POLAR STEREOGRAPHIC");
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
    gctp_print_cenlon(cache_ptr->center_lon);
    gctp_print_offsetp(cache_ptr->false_easting, cache_ptr->false_northing);
}

/*******************************************************************************
Name: common_init

Purpose: Initialization routine for initializing the projection information
    that is common to both the forward and inverse transformations.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*******************************************************************************/
static int common_init
(
    TRANSFORMATION *trans   /* I/O: transformation to initialize */
)
{
    double r_major;             /* major axis */
    double r_minor;             /* minor axis */
    double radius;
    double center_lon;          /* center longitude */
    double center_lat;          /* center latitude */
    double false_easting;       /* x offset in meters */
    double false_northing;      /* y offset in meters */

    double temp;                /* temporary variable */
    double con1;                /* temporary angle */
    double sinphi;              /* sin value */
    double cosphi;              /* cos value */
    double es;                  /* eccentricity squared */

    const GCTP_PROJECTION *proj = &trans->proj;
    struct ps_proj *cache = NULL;
    int spheroid = proj->spheroid;

    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);
    false_easting  = proj->parameters[6];
    false_northing = proj->parameters[7];

    if (gctp_dms2degrees(proj->parameters[4], &center_lon) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center longitude in parameter 4 "
            "from DMS to degrees: %f", proj->parameters[4]);
        return GCTP_ERROR;
    }
    center_lon *= 3600 * S2R;

    if (gctp_dms2degrees(proj->parameters[5], &center_lat) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center latitude in parameter 4 "
            "from DMS to degrees: %f", proj->parameters[5]);
        return GCTP_ERROR;
    }
    center_lat *= 3600 * S2R;

    /* Allocate a structure for the cached info */
    cache = malloc(sizeof(*cache));
    if (!cache)
    {
        GCTP_PRINT_ERROR("Error allocating memory for cache buffer");
        return GCTP_ERROR;
    }
    trans->cache = cache;

    /* Save the information to the cache */
    cache->r_major = r_major;
    cache->r_minor = r_minor;
    cache->false_northing = false_northing;
    cache->false_easting = false_easting;
    temp = r_minor / r_major;
    es = 1.0 - SQUARE(temp);
    cache->e = sqrt(es);
    cache->e4 = gctp_calc_e4(cache->e);
    cache->center_lon = center_lon;
    cache->center_lat = center_lat;
    if (center_lat < 0)
        cache->fac = -1.0;
    else
        cache->fac = 1.0;
    cache->ind = 0;
    cache->mcs = 0.0;
    cache->tcs = 0.0;
    if (fabs(fabs(center_lat) - HALF_PI) > EPSLN)
    {
        cache->ind = 1;
        con1 = cache->fac * center_lat; 
        sincos(con1, &sinphi, &cosphi);
        cache->mcs = gctp_calc_small_radius(cache->e, sinphi, cosphi);
        cache->tcs = gctp_calc_small_t(cache->e, con1, sinphi);
    }

    trans->print_info = print_info;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: inverse_transform

Purpose: Transforms X,Y to lat,long

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
static int inverse_transform
(
    const TRANSFORMATION *trans, /* I: transformation information */
    double x,       /* I: X projection coordinate */
    double y,       /* I: Y projection coordinate */
    double *lon,    /* O: Longitude */
    double *lat     /* O: Latitude */
)
{
    struct ps_proj *cache_ptr = (struct ps_proj *)trans->cache;
    double rh;          /* height above ellipsiod */
    double ts;          /* small value t */
    double temp;        /* temporary variable */
    double phi2;

    x = (x - cache_ptr->false_easting) * cache_ptr->fac;
    y = (y - cache_ptr->false_northing) * cache_ptr->fac;
    rh = sqrt(x * x + y * y);
    if (cache_ptr->ind != 0)
        ts = rh * cache_ptr->tcs/(cache_ptr->r_major * cache_ptr->mcs);
    else
        ts = rh * cache_ptr->e4 / (cache_ptr->r_major * 2.0);
    if (gctp_calc_phi2(cache_ptr->e, ts, &phi2) != GCTP_SUCCESS)
        return GCTP_ERROR;

    *lat = cache_ptr->fac * phi2;
    if (rh == 0)
        *lon = cache_ptr->fac * cache_ptr->center_lon;
    else
    {
        temp = atan2(x, -y);
        *lon = adjust_lon(cache_ptr->fac * temp + cache_ptr->center_lon);
    }

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to polar stereographic X,Y

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
static int forward_transform
(
    const TRANSFORMATION *trans, /* I: transformation information */
    double lon,         /* I: Longitude */
    double lat,         /* I: Latitude */
    double *x,          /* O: X projection coordinate */
    double *y           /* O: Y projection coordinate */
)
{
    struct ps_proj *cache_ptr = (struct ps_proj *)trans->cache;

    double con1;            /* adjusted longitude */
    double con2;            /* adjusted latitude */
    double rh;              /* height above ellipsoid */
    double sinphi;          /* sin value */
    double ts;              /* value of small t */

    con1 = cache_ptr->fac * adjust_lon(lon - cache_ptr->center_lon);
    con2 = cache_ptr->fac * lat;
    sinphi = sin(con2);
    ts = gctp_calc_small_t(cache_ptr->e, con2, sinphi);
    if (cache_ptr->ind != 0)
        rh = cache_ptr->r_major * cache_ptr->mcs * ts / cache_ptr->tcs;
    else
        rh = 2.0 * cache_ptr->r_major * ts / cache_ptr->e4;
    *x = cache_ptr->fac * rh * sin(con1) + cache_ptr->false_easting;
    *y = -cache_ptr->fac * rh * cos(con1) + cache_ptr->false_northing;;

    return GCTP_SUCCESS;
}


/*****************************************************************************
Name: gctp_ps_inverse_init

Purpose: Initializes the inverse polar stereographic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_ps_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing polar stereographic inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_ps_forward_init

Purpose: Initializes the forward polar stereographic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_ps_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing polar stereographic forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
