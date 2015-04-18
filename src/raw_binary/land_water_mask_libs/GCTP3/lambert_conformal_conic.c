/*******************************************************************************
Name: LAMBERT CONFORMAL CONIC

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the Lambert Conformal Conic projection. The Easting and
    Northing values are in meters.  The longitude and latitude are in radians.

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
*******************************************************************************/
#include <stdlib.h>
#include <math.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"

/* structure to hold the setup data relevant to this projection */
struct lamcc_proj
{
    double r_major;                /* major axis */
    double r_minor;                /* minor axis */
    double es;                     /* eccentricity squared */
    double e;                      /* eccentricity */
    double lat1;                   /* first standard parallel */
    double lat2;                   /* second standard parallel */
    double center_lon;             /* center longitude */
    double lat_origin;             /* latitude of origin */
    double ns;                     /* ratio of angle between meridian */
    double f0;                     /* flattening of ellipsoid */
    double rh;                     /* height above ellipsoid */
    double false_easting;          /* x offset in meters */
    double false_northing;         /* y offset in meters */
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
    struct lamcc_proj *cache_ptr = (struct lamcc_proj *)trans->cache;

    gctp_print_title("LAMBERT CONFORMAL CONIC");
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
    gctp_print_stanparl(cache_ptr->lat1, cache_ptr->lat2);
    gctp_print_cenlonmer(cache_ptr->center_lon);
    gctp_print_origin(cache_ptr->lat_origin);
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
    double r_major;                 /* major axis */
    double r_minor;                 /* minor axis */
    double radius;                  /* earth radius */
    double lat1;                    /* first standard parallel */
    double lat2;                    /* second standard parallel */
    double center_lon;              /* center longitude */
    double lat_origin;              /* center latitude */
    double false_easting;           /* x offset in meters */
    double false_northing;          /* y offset in meters */
    double sin_po;                  /* sin value */
    double cos_po;                  /* cos value */
    double con;                     /* temporary variable */
    double ms1;                     /* small m 1 */
    double ms2;                     /* small m 2 */
    double temp;                    /* temporary variable */
    double ts0;                     /* small t 0 */
    double ts1;                     /* small t 1 */
    double ts2;                     /* small t 2 */

    const GCTP_PROJECTION *proj = &trans->proj;
    struct lamcc_proj *cache = NULL;
    int spheroid = proj->spheroid;

    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);
    false_easting  = proj->parameters[6];
    false_northing = proj->parameters[7];

    if (gctp_dms2degrees(proj->parameters[2], &lat1) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting standard parallel 1 in parameter 2 "
            "from DMS to degrees: %f", proj->parameters[2]);
        return GCTP_ERROR;
    }
    lat1 *= 3600 * S2R;

    if (gctp_dms2degrees(proj->parameters[3], &lat2) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting standard parallel 2 in parameter 3 "
            "from DMS to degrees: %f", proj->parameters[3]);
        return GCTP_ERROR;
    }
    lat2 *= 3600 * S2R;

    if (gctp_dms2degrees(proj->parameters[4], &center_lon) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center longitude in parameter 4 "
            "from DMS to degrees: %f", proj->parameters[4]);
        return GCTP_ERROR;
    }
    center_lon *= 3600 * S2R;

    if (gctp_dms2degrees(proj->parameters[5], &lat_origin) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center latitude in parameter 4 "
            "from DMS to degrees: %f", proj->parameters[5]);
        return GCTP_ERROR;
    }
    lat_origin *= 3600 * S2R;

    /* Standard Parallels cannot be equal and on opposite sides of the
       equator */
    if (fabs(lat1 + lat2) < EPSLN)
    {
        GCTP_PRINT_ERROR("Equal latitudes for Standard Parallels on opposite "
            "sides of equator");
        return GCTP_ERROR;
    }

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
    cache->lat1 = lat1;
    cache->lat2 = lat2;
    cache->center_lon = center_lon;
    cache->lat_origin = lat_origin;

    temp = r_minor / r_major;
    cache->es = 1.0 - SQUARE(temp);
    cache->e = sqrt(cache->es);

    sincos(lat1, &sin_po, &cos_po);
    con = sin_po;
    ms1 = gctp_calc_small_radius(cache->e, sin_po, cos_po);
    ts1 = gctp_calc_small_t(cache->e, lat1, sin_po);
    sincos(lat2, &sin_po, &cos_po);
    ms2 = gctp_calc_small_radius(cache->e, sin_po, cos_po);
    ts2 = gctp_calc_small_t(cache->e, lat2, sin_po);
    sin_po = sin(lat_origin);
    ts0 = gctp_calc_small_t(cache->e, lat_origin, sin_po);

    if (fabs(lat1 - lat2) > EPSLN)
        cache->ns = log(ms1/ms2)/ log(ts1/ts2);
    else
        cache->ns = con;
    cache->f0 = ms1 / (cache->ns * pow(ts1, cache->ns));
    cache->rh = r_major * cache->f0 * pow(ts0, cache->ns);

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
    struct lamcc_proj *cache_ptr = (struct lamcc_proj *)trans->cache;
    double rh1;         /* height above ellipsoid   */
    double con;         /* sign variable        */
    double ts;          /* small t          */
    double theta;           /* angle            */

    x -= cache_ptr->false_easting;
    y = cache_ptr->rh - y + cache_ptr->false_northing;
    if (cache_ptr->ns > 0)
    {
        rh1 = sqrt (x * x + y * y);
        con = 1.0;
    }
    else
    {
        rh1 = -sqrt (x * x + y * y);
        con = -1.0;
    }
    theta = 0.0;
    if (rh1 != 0)
        theta = atan2((con * x),(con * y));

    if ((rh1 != 0) || (cache_ptr->ns > 0.0))
    {
        con = 1.0/cache_ptr->ns;
        ts = pow((rh1/(cache_ptr->r_major * cache_ptr->f0)),con);
        if (gctp_calc_phi2(cache_ptr->e, ts, lat) != GCTP_SUCCESS)
            return GCTP_ERROR;
    }
    else
        *lat = -HALF_PI;

    *lon = adjust_lon(theta/cache_ptr->ns + cache_ptr->center_lon);

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to X,Y

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
    struct lamcc_proj *cache_ptr = (struct lamcc_proj *)trans->cache;
    double con;                     /* temporary angle variable */
    double rh1;                     /* height above ellipsoid */
    double sinphi;                  /* sin value */
    double theta;                   /* angle */
    double ts;                      /* small value t */

    con  = fabs( fabs(lat) - HALF_PI);
    if (con > EPSLN)
    {
        sinphi = sin(lat);
        ts = gctp_calc_small_t(cache_ptr->e, lat, sinphi);
        rh1 = cache_ptr->r_major * cache_ptr->f0 * pow(ts, cache_ptr->ns);
    }
    else
    {
        con = lat * cache_ptr->ns;
        if (con <= 0)
        {
            GCTP_PRINT_ERROR("Point can not be projected");
            return GCTP_ERROR;
        }
        rh1 = 0;
    }
    theta = cache_ptr->ns * adjust_lon(lon - cache_ptr->center_lon);
    *x = rh1 * sin(theta) + cache_ptr->false_easting;
    *y = cache_ptr->rh - rh1 * cos(theta) + cache_ptr->false_northing;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_lamcc_inverse_init

Purpose: Initializes the inverse Lambert Conformal Conic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_lamcc_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing lambert conformal conic inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_lamcc_forward_init

Purpose: Initializes the forward Lambert Conformal Conic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_lamcc_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing lambert conformal conic forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
