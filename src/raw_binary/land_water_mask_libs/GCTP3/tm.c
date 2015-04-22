/*******************************************************************************
Name: TRANSVERSE MERCATOR and UNIVERSAL TRANSVERSE MERCATOR

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for both the Universal Transverse Mercator and Transverse Mercator
    projections since the UTM projection is just a special case of the TM
    projection.  The Easting and Northing are in meters.  The longitude and
    latitude are in radians.

Algorithm References:

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdlib.h>
#include <math.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"


/* structure to hold the setup data relevant to this projection */
struct tm_proj
{
    double r_major;          /* major axis */
    double r_minor;          /* minor axis */
    double scale_factor;     /* scale factor */
    double lon_center;       /* Center longitude (projection center) */
    double lat_origin;       /* center latitude */
    double false_northing;   /* y offset in meters */
    double false_easting;    /* x offset in meters */
    double e0;               /* eccentricity constants */
    double e1;
    double e2;
    double e3;
    double es;
    double esp;
    double ml0;              /* small value m */
    int is_sphere;           /* sphere flag value */
};


/*****************************************************************************
Name: utm_print_info

Purpose: Prints a summary of information about the UTM projection.

Returns:
    nothing

*****************************************************************************/
static void utm_print_info
(
    const TRANSFORMATION *trans
)
{
    struct tm_proj *cache_ptr = (struct tm_proj *)trans->cache;

    gctp_print_title("UNIVERSAL TRANSVERSE MERCATOR (UTM)"); 
    gctp_print_genrpt_long(trans->proj.zone, "Zone:     ");
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
    gctp_print_genrpt(cache_ptr->scale_factor,
        "Scale Factor at C. Meridian:     ");
    gctp_print_cenlonmer(cache_ptr->lon_center);
}

/*****************************************************************************
Name: tm_print_info

Purpose: Prints a summary of information about the TM projection.

Returns:
    nothing

*****************************************************************************/
static void tm_print_info
(
    const TRANSFORMATION *trans
)
{
    struct tm_proj *cache_ptr = (struct tm_proj *)trans->cache;

    gctp_print_title("TRANSVERSE MERCATOR (TM)"); 
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
    gctp_print_genrpt(cache_ptr->scale_factor,
        "Scale Factor at C. Meridian:     ");
    gctp_print_cenlonmer(cache_ptr->lon_center);
    gctp_print_origin(cache_ptr->lat_origin);
    gctp_print_offsetp(cache_ptr->false_easting, cache_ptr->false_northing);
}

/*******************************************************************************
Name: common_init

Purpose: Initialization routine for initializing the projection information
    that is common to both the TM/UTM forward and inverse transformations.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*******************************************************************************/
static int common_init
(
    TRANSFORMATION *trans,
    double r_major,
    double r_minor,
    double scale_factor,
    double center_long,
    double lat_origin,
    double false_easting,
    double false_northing
)
{
    struct tm_proj *cache = NULL;
    double temp;

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
    cache->scale_factor = scale_factor;
    cache->lat_origin = lat_origin;
    cache->lon_center = center_long;

    cache->false_easting = false_easting;
    cache->false_northing = false_northing;

    temp = r_minor / r_major;
    cache->es = 1.0 - SQUARE(temp);
    cache->e0 = gctp_calc_e0(cache->es);
    cache->e1 = gctp_calc_e1(cache->es);
    cache->e2 = gctp_calc_e2(cache->es);
    cache->e3 = gctp_calc_e3(cache->es);
    cache->ml0 = r_major * gctp_calc_dist_from_equator(cache->e0, cache->e1,
                    cache->e2, cache->e3, cache->lat_origin);
    cache->esp = cache->es / (1.0 - cache->es);

    /* Set the flag indicating the transformation can use the spherical form
       if the eccentricity is small enough */
    if (cache->es < .00001)
        cache->is_sphere = 1;
    else 
        cache->is_sphere = 0;

    return GCTP_SUCCESS;
}

/*******************************************************************************
Name: tm_common_init

Purpose: Initialization routine for initializing the TM projection information
    that is common to both the forward and inverse transformations.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*******************************************************************************/
static int tm_common_init
(
    TRANSFORMATION *trans   /* I/O: transformation to initialize */
)
{
    double r_major;         /* major axis */
    double r_minor;         /* minor axis */
    double radius;          /* earth radius */
    double scale_factor;    /* scale factor */
    double center_long;
    double lat_origin;
    double false_easting;
    double false_northing;
    const GCTP_PROJECTION *proj = &trans->proj;
    int spheroid = proj->spheroid;

    /* Get the parameters from the input parameters */
    scale_factor = proj->parameters[2];
    false_easting = proj->parameters[6];
    false_northing = proj->parameters[7];
    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);

    if (gctp_dms2degrees(proj->parameters[4], &center_long) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting longitude in parameter 4 from "
            "DMS to degrees: %f", proj->parameters[4]);
        return GCTP_ERROR;
    }
    center_long *= 3600 * S2R;

    if (gctp_dms2degrees(proj->parameters[5], &lat_origin) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting latitude in parameter 5 from "
            "DMS to degrees: %f", proj->parameters[5]);
        return GCTP_ERROR;
    }
    lat_origin *= 3600 * S2R;
 
    trans->print_info = tm_print_info;

    return common_init(trans, r_major, r_minor, scale_factor, center_long,
                lat_origin, false_easting, false_northing);
}

/*******************************************************************************
Name: utm_common_init

Purpose: Initialization routine for initializing the UTM projection information
    that is common to both the forward and inverse transformations.

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*******************************************************************************/
static int utm_common_init
(
    TRANSFORMATION *trans   /* I/O: transformation to initialize */
)
{
    double r_major;         /* major axis */
    double r_minor;         /* minor axis */
    double radius;          /* earth radius */
    double scale_factor;    /* scale factor */
    double center_long;
    double lat_origin;
    double false_easting;
    double false_northing;
    int zone;               /* zone number */
    const GCTP_PROJECTION *proj = &trans->proj;
    int spheroid = proj->spheroid;


    /* set Clarke 1866 spheroid if negative spheroid code */
    if (spheroid < 0)
        spheroid = 0;

    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);

    zone = proj->zone;
    if (zone == 0)
    {
        /* The zone is zero, so calculate it from the first two projection
           parameters */
        double lon1;
        double lat1;

        /* Convert the longitude and latitude from DMS to degrees */
        if (gctp_dms2degrees(proj->parameters[0], &lon1) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting longitude in parameter 0 from "
                "DMS to degrees: %f", proj->parameters[0]);
            return GCTP_ERROR;
        }
        lon1 *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[1], &lat1) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting latitude in parameter 1 from "
                "DMS to degrees: %f", proj->parameters[1]);
            return GCTP_ERROR;
        }
        lat1 *= 3600 * S2R;

        /* Calculate the zone from the longitude */
        zone = gctp_calc_utm_zone(lon1 * R2D);

        /* Use the convention of a negative zone for the southern hemisphere */
        if (lat1 < 0)
            zone = -zone;
    }
    scale_factor = .9996;

    /* Verify the zone is a legal value */
    if ((abs(zone) < 1) || (abs(zone) > 60))
    {
        GCTP_PRINT_ERROR("Illegal zone number: %d", zone);
        return GCTP_ERROR;
    }

    lat_origin = 0.0;
    center_long = ((6 * abs(zone)) - 183) * D2R;
    false_easting = 500000.0;
    false_northing = (zone < 0) ? 10000000.0 : 0.0;

    trans->print_info = utm_print_info;

    return common_init(trans, r_major, r_minor, scale_factor, center_long,
                lat_origin, false_easting, false_northing);
}

/*****************************************************************************
Name: inverse_transform

Purpose: Transforms UTM/TM X,Y to lat,long

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
    struct tm_proj *cache_ptr = (struct tm_proj *)trans->cache;
    double con,phi;                 /* temporary angles */
    double delta_phi;               /* difference between longitudes */
    long i;                         /* counter variable */
    double sin_phi, cos_phi, tan_phi;   /* sin cos and tangent values */
    double c, cs, t, ts, n, r, d, ds;   /* temporary variables */
    double f, h, g, temp;           /* temporary variables */
    long max_iter = 6;              /* maximun number of iterations */

    /* Use the spherical form if possible */
    if (cache_ptr->is_sphere)
    {
        f = exp(x / (cache_ptr->r_major * cache_ptr->scale_factor));
        g = 0.5 * (f - 1.0/f);
        temp = cache_ptr->lat_origin + y
             /(cache_ptr->r_major * cache_ptr->scale_factor);
        h = cos(temp);
        con = sqrt((1.0 - h * h)/(1.0 + g * g));
        *lat = asinz(con);
        if (temp < 0)
            *lat = -*lat;
        if ((g == 0) && (h == 0))
        {
            *lon = cache_ptr->lon_center;
            return GCTP_SUCCESS;
        }
        else
        {
            *lon = adjust_lon(atan2(g,h) + cache_ptr->lon_center);
            return GCTP_SUCCESS;
        }
    }

    /* Inverse equations */
    x = x - cache_ptr->false_easting;
    y = y - cache_ptr->false_northing;

    con = (cache_ptr->ml0 + y / cache_ptr->scale_factor) / cache_ptr->r_major;
    phi = con;
    for (i = 0; ;i++)
    {
        delta_phi = ((con + cache_ptr->e1 * sin(2.0*phi) - cache_ptr->e2
                  * sin(4.0*phi) + cache_ptr->e3 * sin(6.0*phi))
                / cache_ptr->e0) - phi;
        phi += delta_phi;
        if (fabs(delta_phi) <= EPSLN)
            break;
        if (i >= max_iter) 
        { 
            GCTP_PRINT_ERROR("Latitude failed to converge in inverse "
                "transform");
            return GCTP_ERROR;
        }
    }
    if (fabs(phi) < HALF_PI)
    {
        sincos(phi, &sin_phi, &cos_phi);
        tan_phi = tan(phi);
        c    = cache_ptr->esp * SQUARE(cos_phi);
        cs   = SQUARE(c);
        t    = SQUARE(tan_phi);
        ts   = SQUARE(t);
        con  = 1.0 - cache_ptr->es * SQUARE(sin_phi); 
        n    = cache_ptr->r_major / sqrt(con);
        r    = n * (1.0 - cache_ptr->es) / con;
        d    = x / (n * cache_ptr->scale_factor);
        ds   = SQUARE(d);
        *lat = phi - (n * tan_phi * ds / r) * (0.5 - ds / 24.0 * (5.0 + 3.0
                * t + 10.0 * c - 4.0 * cs - 9.0 * cache_ptr->esp - ds / 30.0
                * (61.0 + 90.0 * t + 298.0 * c + 45.0 * ts - 252.0
                * cache_ptr->esp - 3.0 * cs)));
        *lon = adjust_lon(cache_ptr->lon_center + (d * (1.0 - ds / 6.0
                * (1.0 + 2.0 * t + c - ds / 20.0 * (5.0 - 2.0 * c + 28.0
                * t - 3.0 * cs + 8.0 * cache_ptr->esp + 24.0 * ts))) 
                / cos_phi));
    }
    else
    {
        *lat = HALF_PI * gctp_get_sign(y);
        *lon = cache_ptr->lon_center;
    }
    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to UTM/TM X,Y

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
    struct tm_proj *cache_ptr = (struct tm_proj *)trans->cache;
    double delta_lon;       /* Delta longitude (Given longitude - center) */
    double sin_phi, cos_phi;/* sin and cos value */
    double al, als;         /* temporary values */
    double b;               /* temporary values */
    double c, t, tq;        /* temporary values */
    double con, n, ml;      /* cone constant, small m */

    /* Forward equations */
    delta_lon = adjust_lon(lon - cache_ptr->lon_center);
    sincos(lat, &sin_phi, &cos_phi);

    /* Use the spherical form if possible */
    if (cache_ptr->is_sphere)
    {
        b = cos_phi * sin(delta_lon);
        if ((fabs(fabs(b) - 1.0)) < .0000000001)
        {
            GCTP_PRINT_ERROR("Point projects into infinity");
            return GCTP_ERROR;
        }
        else
        {
            *x = 0.5 * cache_ptr->r_major * cache_ptr->scale_factor
               * log((1.0 + b)/(1.0 - b));
            con = acos(cos_phi * cos(delta_lon)/sqrt(1.0 - b*b));
            if (lat < 0.0)
                con = - con;
            *y = cache_ptr->r_major * cache_ptr->scale_factor 
               * (con - cache_ptr->lat_origin); 
            return GCTP_SUCCESS;
        }
    }

    al  = cos_phi * delta_lon;
    als = SQUARE(al);
    c   = cache_ptr->esp * SQUARE(cos_phi);
    tq  = tan(lat);
    t   = SQUARE(tq);
    con = 1.0 - cache_ptr->es * SQUARE(sin_phi);
    n   = cache_ptr->r_major / sqrt(con);
    ml  = cache_ptr->r_major * gctp_calc_dist_from_equator(cache_ptr->e0,
            cache_ptr->e1, cache_ptr->e2, cache_ptr->e3, lat);

    *x  = cache_ptr->scale_factor * n * al * (1.0 + als / 6.0
        * (1.0 - t + c + als / 20.0 * (5.0 - 18.0 * t + SQUARE(t) + 72.0
        * c - 58.0 * cache_ptr->esp))) + cache_ptr->false_easting;

    *y  = cache_ptr->scale_factor * (ml - cache_ptr->ml0 + n * tq * (als
        * (0.5 + als / 24.0 * (5.0 - t + 9.0 * c + 4.0 * SQUARE(c) + als 
        / 30.0 * (61.0 - 58.0 * t + SQUARE(t) + 600.0 * c - 330.0 
        * cache_ptr->esp))))) + cache_ptr->false_northing;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_utm_inverse_init

Purpose: Initializes the inverse UTM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_utm_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (utm_common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing UTM inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_utm_forward_init

Purpose: Initializes the forward UTM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_utm_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (utm_common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing UTM forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_tm_inverse_init

Purpose: Initializes the inverse TM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_tm_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (tm_common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing TM inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_tm_forward_init

Purpose: Initializes the forward TM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_tm_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (tm_common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing TM forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
