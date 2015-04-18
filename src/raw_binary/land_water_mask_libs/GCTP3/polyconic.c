/*******************************************************************************
Name: POLYCONIC

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the Polyconic projection.  The Easting and Northing
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
struct poly_proj
{
    double r_major;         /* major axis */
    double r_minor;         /* minor axis */
    double center_lon;      /* Center longitude (projection center) */
    double lat_origin;      /* center latitude */
    double e;               /* eccentricity constants */
    double e0;
    double e1;
    double e2;
    double e3;
    double es;
    double ml0;             /* small value m */
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
    struct poly_proj *cache_ptr = (struct poly_proj *)trans->cache;

    gctp_print_title("POLYCONIC");
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
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
    double r_major;             /* major axis */
    double r_minor;             /* minor axis */
    double radius;
    double center_lon;          /* center longitude */
    double lat_origin;          /* center latitude */
    double false_easting;       /* x offset in meters */
    double false_northing;      /* y offset in meters */

    double temp;                /* temporary variable */
    double es;                  /* eccentricity squared */

    const GCTP_PROJECTION *proj = &trans->proj;
    struct poly_proj *cache = NULL;
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

    if (gctp_dms2degrees(proj->parameters[5], &lat_origin) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center latitude in parameter 4 "
            "from DMS to degrees: %f", proj->parameters[5]);
        return GCTP_ERROR;
    }
    lat_origin *= 3600 * S2R;

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
    cache->center_lon = center_lon;
    cache->lat_origin = lat_origin;

    temp = r_minor / r_major;
    es = 1.0 - SQUARE(temp);
    cache->es = es;
    cache->e = sqrt(es);
    cache->e0 = gctp_calc_e0(es);
    cache->e1 = gctp_calc_e1(es);
    cache->e2 = gctp_calc_e2(es);
    cache->e3 = gctp_calc_e3(es);
    cache->ml0 = gctp_calc_dist_from_equator(cache->e0, cache->e1, cache->e2,
        cache->e3, lat_origin);

    trans->print_info = print_info;

    return GCTP_SUCCESS;
}

/*******************************************************************************
Name: compute_phi4z

Purpose: Function to compute, phi4, the latitude for the inverse of the
   Polyconic projection.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*******************************************************************************/
static int compute_phi4z
(
    double eccent,      /* Spheroid eccentricity squared    */
    double e0,
    double e1,
    double e2,
    double e3,
    double a,
    double b,
    double *c,
    double *phi
)
{
    double sinphi;
    double sin2ph;
    double tanphi;
    double ml;
    double mlp;
    double con1;
    double con2;
    double con3;
    double dphi;
    long i;

    *phi = a;
    for (i = 1; i <= 15; i++)
    {
        sinphi = sin(*phi);
        tanphi = tan(*phi);
        *c = tanphi * sqrt (1.0 - eccent * sinphi * sinphi);
        sin2ph = sin (2.0 * *phi);
        ml = e0 * *phi - e1 * sin2ph + e2 * sin (4.0 *  *phi) - e3 * 
            sin (6.0 *  *phi);
        mlp = e0 - 2.0 * e1 * cos (2.0 *  *phi) + 4.0 * e2 *
            cos (4.0 *  *phi) - 6.0 * e3 * cos (6.0 *  *phi);
        con1 = 2.0 * ml + *c * (ml * ml + b) - 2.0 * a *  (*c * ml + 1.0);
        con2 = eccent * sin2ph * (ml * ml + b - 2.0 * a * ml) / (2.0 * *c);
        con3 = 2.0 * (a - ml) * (*c * mlp - 2.0 / sin2ph) - 2.0 * mlp;
        dphi = con1 / (con2 + con3);
        *phi += dphi;
        if (fabs(dphi) <= .0000000001 )
            return GCTP_SUCCESS;
    }
    GCTP_PRINT_ERROR("Latitude failed to converge");
    return GCTP_ERROR;
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
    struct poly_proj *cache_ptr = (struct poly_proj *)trans->cache;
    double al;      /* temporary values */
    double b;       /* temporary values */
    double c;       /* temporary values */

    x -= cache_ptr->false_easting;
    y -= cache_ptr->false_northing;
    al = cache_ptr->ml0 + y/cache_ptr->r_major;
    if (fabs(al) <= .0000001)
    {
        *lon = x/cache_ptr->r_major + cache_ptr->center_lon;
        *lat = 0.0;
    }
    else
    {
        b = al * al + (x/cache_ptr->r_major) * (x/cache_ptr->r_major);
        if (compute_phi4z(cache_ptr->es, cache_ptr->e0, cache_ptr->e1,
            cache_ptr->e2, cache_ptr->e3, al, b, &c, lat) != GCTP_SUCCESS)
        {
            return GCTP_ERROR;
        }
        *lon = adjust_lon((asinz(x * c / cache_ptr->r_major) / sin(*lat))
             + cache_ptr->center_lon);
    }

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to polyconic X,Y

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
    struct poly_proj *cache_ptr = (struct poly_proj *)trans->cache;
    double sinphi, cosphi;      /* sin and cos value */
    double con, ml;             /* cone constant, small m */
    double ms;                  /* small m  */

    con = adjust_lon(lon - cache_ptr->center_lon);
    if (fabs(lat) <= .0000001)
    {
        *x = cache_ptr->false_easting + cache_ptr->r_major * con;
        *y = cache_ptr->false_northing - cache_ptr->r_major * cache_ptr->ml0;
    }
    else
    {
        sincos(lat,&sinphi,&cosphi);
        ml = gctp_calc_dist_from_equator(cache_ptr->e0, cache_ptr->e1,
                cache_ptr->e2, cache_ptr->e3, lat);
        ms = gctp_calc_small_radius(cache_ptr->e, sinphi, cosphi);
        con *= sinphi;
        *x = cache_ptr->false_easting + cache_ptr->r_major * ms * sin(con)
           / sinphi;
        *y = cache_ptr->false_northing + cache_ptr->r_major
           * (ml - cache_ptr->ml0 + ms * (1.0 - cos(con))/sinphi);
    }

    return GCTP_SUCCESS;
}


/*****************************************************************************
Name: gctp_poly_inverse_init

Purpose: Initializes the inverse polyconic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_poly_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing polyconic inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_poly_forward_init

Purpose: Initializes the forward polyconic transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_poly_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing polyconic forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
