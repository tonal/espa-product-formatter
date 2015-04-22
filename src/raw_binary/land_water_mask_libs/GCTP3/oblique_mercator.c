/*******************************************************************************
Name: OBLIQUE MERCATOR (HOTINE)

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the Oblique Mercator projection.  The Easting and Northing
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
struct om_proj
{
    double r_major;             /* major axis */
    double r_minor;             /* minor axis */
    double scale_factor;        /* scale factor */
    double lon_origin;          /* center longitude */
    double lat_origin;          /* center latitude */
    double lon1;                /* first point to define central line */
    double lat1;                /* first point to define central line */
    double lon2;                /* second point to define central line */
    double lat2;                /* second point to define central line */
    double azimuth;             /* azimuth east of north */
    double false_northing;      /* y offset in meters */
    double false_easting;       /* x offset in meters */
    double e;                   /* eccentricity constants */
    double es;
    double sin_p20;             /* sin value */
    double cos_p20;             /* cos values */
    double bl;
    double al;
    double d;
    double el;
    double u;
    double singam;
    double cosgam;
    double sinaz;
    double cosaz;
    int mode;                   /* format type 0 = A, 1 = B */
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
    struct om_proj *cache_ptr = (struct om_proj *)trans->cache;

    gctp_print_title("OBLIQUE MERCATOR (HOTINE)"); 
    gctp_print_radius2(cache_ptr->r_major, cache_ptr->r_minor);
    gctp_print_genrpt(cache_ptr->scale_factor,
        "Scale Factor at C. Meridian:    ");
    gctp_print_offsetp(cache_ptr->false_easting, cache_ptr->false_northing);

    if (cache_ptr->mode == 0)
    {
        /* Format A parameters */
        gctp_print_genrpt(cache_ptr->lon1 * R2D,   
            "Longitude of First Point:    ");
        gctp_print_genrpt(cache_ptr->lat1 * R2D,   
            "Latitude of First Point:    ");
        gctp_print_genrpt(cache_ptr->lon2 * R2D,   
            "Longitude of Second Point:    ");
        gctp_print_genrpt(cache_ptr->lat2 * R2D,   
            "Latitude of Second Point:    ");
    }
    else
    {
        /* Format B parameters */
        gctp_print_genrpt(cache_ptr->azimuth * R2D,
            "Azimuth of Central Line:    ");
        gctp_print_cenlon(cache_ptr->lon_origin);
        gctp_print_cenlat(cache_ptr->lat_origin);

    }
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
    double lon_origin;          /* center longitude */
    double lat_origin;          /* center latitude */
    double false_easting;       /* x offset in meters */
    double false_northing;      /* y offset in meters */
    double scale_factor;        /* scale factor */
    double azimuth;             /* azimuth east of north */
    double lon1;                /* first point to define central line */
    double lat1;                /* first point to define central line */
    double lon2;                /* second point to define central line */
    double lat2;                /* second point to define central line */
    int mode;                   /* format A/B indicator */
    double temp;                /* temporary variable */
    double sinphi;              /* sin value */
    double con,com;
    double ts;
    double ts1,ts2;
    double h,l;
    double j,p,dlon;
    double f = 0.0;
    double g,gama;

    const GCTP_PROJECTION *proj = &trans->proj;
    struct om_proj *cache = NULL;
    int spheroid = proj->spheroid;

    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);
    scale_factor = proj->parameters[2];
    false_easting  = proj->parameters[6];
    false_northing = proj->parameters[7];

    if (gctp_dms2degrees(proj->parameters[5], &lat_origin) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error converting center latitude in parameter 4 "
                "from DMS to degrees: %f", proj->parameters[5]);
        return GCTP_ERROR;
    }
    lat_origin *= 3600 * S2R;

    if (proj->parameters[12] != 0)
        mode =  1;
    else
        mode = 0;

    if (mode == 0)
    {
        if (gctp_dms2degrees(proj->parameters[8], &lon1) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting first point Long1 in parameter "
                    "8 from DMS to degrees: %f", proj->parameters[8]);
            return GCTP_ERROR;
        }
        lon1 *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[9], &lat1) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting first point Lat1 in parameter "
                    "9 from DMS to degrees: %f", proj->parameters[9]);
            return GCTP_ERROR;
        }
        lat1 *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[10], &lon2) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting first point Long2 in parameter "
                    "10 from DMS to degrees: %f", proj->parameters[10]);
            return GCTP_ERROR;
        }
        lon2 *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[11], &lat2) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting first point Lat2 in parameter "
                    "11 from DMS to degrees: %f", proj->parameters[11]);
            return GCTP_ERROR;
        }
        lat2 *= 3600 * S2R;

        azimuth = 0.0;
        lon_origin = 0.0;
    }
    else
    {
        if (gctp_dms2degrees(proj->parameters[3], &azimuth) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting center azimuth angle in "
                    "parameter 3 from DMS to degrees: %f", proj->parameters[3]);
            return GCTP_ERROR;
        }
        azimuth *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[4], &lon_origin) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting center azimuth point in "
                    "parameter 4 from DMS to degrees: %f", proj->parameters[4]);
            return GCTP_ERROR;
        }
        lon_origin *= 3600 * S2R;

        lon1 = 0.0;
        lat1 = 0.0;
        lon2 = 0.0;
        lat2 = 0.0;
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
    cache->scale_factor = scale_factor;
    cache->lat_origin = lat_origin;
    cache->lon1 = lon1;
    cache->lat1 = lat1;
    cache->lon2 = lon2;
    cache->lat2 = lat2;
    cache->mode = mode;

    temp = r_minor / r_major;
    cache->es = 1.0 - SQUARE(temp);
    cache->e = sqrt(cache->es);

    sincos(lat_origin, &cache->sin_p20, &cache->cos_p20);
    con = 1.0 - cache->es * cache->sin_p20 * cache->sin_p20;
    com = sqrt(1.0 - cache->es);
    cache->bl = sqrt(1.0 + cache->es * pow(cache->cos_p20, 4.0)
        / (1.0 - cache->es));
    cache->al = r_major * cache->bl * cache->scale_factor * com / con;
    if (fabs(lat_origin) < EPSLN)
    {
        ts = 1.0;
        cache->d = 1.0;
        cache->el = 1.0;
    }
    else
    {
        ts = gctp_calc_small_t(cache->e, lat_origin, cache->sin_p20);
        con = sqrt(con);
        cache->d = cache->bl * com / (cache->cos_p20 * con);
        if ((cache->d * cache->d - 1.0) > 0.0)
        {
            if (lat_origin >= 0.0)
                f = cache->d + sqrt(cache->d * cache->d - 1.0);
            else
                f = cache->d - sqrt(cache->d * cache->d - 1.0);
        }
        else
            f = cache->d;
        cache->el = f * pow(ts, cache->bl);
    }

    if (mode != 0)
    {
        g = .5 * (f - 1.0/f);
        gama = asinz(sin(azimuth) / cache->d);
        lon_origin = lon_origin - asinz(g * tan(gama))/cache->bl;

        con = fabs(lat_origin);
        if ((con > EPSLN) && (fabs(con - HALF_PI) > EPSLN))
        {
            sincos(gama, &cache->singam, &cache->cosgam);
            sincos(azimuth, &cache->sinaz, &cache->cosaz);
            if (lat_origin >= 0)
            {
                cache->u =  (cache->al / cache->bl)
                    * atan(sqrt(cache->d * cache->d - 1.0) / cache->cosaz);
            }
            else
            {
                cache->u =  -(cache->al / cache->bl)
                    * atan(sqrt(cache->d * cache->d - 1.0) / cache->cosaz);
            }
        }
        else
        {
            GCTP_PRINT_ERROR("Input data error");
            free(cache);
            trans->cache = NULL;
            return GCTP_ERROR;
        }
    }
    else
    {
        sinphi = sin(lat1);
        ts1 = gctp_calc_small_t(cache->e, lat1, sinphi);
        sinphi = sin(lat2);
        ts2 = gctp_calc_small_t(cache->e, lat2, sinphi);
        h = pow(ts1, cache->bl);
        l = pow(ts2, cache->bl);
        f = cache->el / h;
        g = .5 * (f - 1.0/f);
        j = (cache->el * cache->el - l * h)/(cache->el * cache->el + l * h);
        p = (l - h) / (l + h);
        dlon = lon1 - lon2;
        if (dlon < -PI)
            lon2 = lon2 - 2.0 * PI;
        if (dlon > PI)
            lon2 = lon2 + 2.0 * PI;
        dlon = lon1 - lon2;
        lon_origin = .5 * (lon1 + lon2)
            - atan(j * tan(.5 * cache->bl * dlon) / p) / cache->bl;
        dlon  = adjust_lon(lon1 - lon_origin);
        gama = atan(sin(cache->bl * dlon) / g);
        azimuth = asinz(cache->d * sin(gama));

        if (fabs(lat1 - lat2) <= EPSLN)
        {
            GCTP_PRINT_ERROR("Input data error");
            free(cache);
            trans->cache = NULL;
            return GCTP_ERROR;
        }
        else
            con = fabs(lat1);
        if ((con <= EPSLN) || (fabs(con - HALF_PI) <= EPSLN))
        {
            GCTP_PRINT_ERROR("Input data error");
            free(cache);
            trans->cache = NULL;
            return GCTP_ERROR;
        }
        else if (fabs(fabs(lat_origin) - HALF_PI) <= EPSLN)
        {
            GCTP_PRINT_ERROR("Input data error");
            free(cache);
            trans->cache = NULL;
            return GCTP_ERROR;
        }

        sincos(gama, &cache->singam, &cache->cosgam);
        sincos(azimuth, &cache->sinaz, &cache->cosaz);
        if (lat_origin >= 0)
        {
            cache->u =  (cache->al / cache->bl)
                * atan(sqrt(cache->d * cache->d - 1.0)/cache->cosaz);
        }
        else
        {
            cache->u = -(cache->al / cache->bl)
                * atan(sqrt(cache->d * cache->d - 1.0)/cache->cosaz);
        }
    }

    cache->lon_origin = lon_origin;
    cache->azimuth = azimuth;

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
    struct om_proj *cache_ptr = (struct om_proj *)trans->cache;
    double theta;       /* angle */
    double t;           /* temporary values */
    double con;         /* cone constant, small m */
    double vs,us,q,s,ts1;
    double vl,ul;

    x -= cache_ptr->false_easting;
    y -= cache_ptr->false_northing;
    vs = x * cache_ptr->cosaz - y * cache_ptr->sinaz;
    us = y * cache_ptr->cosaz + x * cache_ptr->sinaz;
    us = us + cache_ptr->u;
    q = exp(-cache_ptr->bl * vs / cache_ptr->al);
    s = .5 * (q - 1.0/q);
    t = .5 * (q + 1.0/q);
    vl = sin(cache_ptr->bl * us / cache_ptr->al);
    ul = (vl * cache_ptr->cosgam + s * cache_ptr->singam)/t;
    if (fabs(fabs(ul) - 1.0) <= EPSLN)
    {
        *lon = cache_ptr->lon_origin;
        if (ul >= 0.0)
            *lat = HALF_PI;
        else
            *lat = -HALF_PI;
    }
    else
    {
        con = 1.0 / cache_ptr->bl;
        ts1 = pow((cache_ptr->el / sqrt((1.0 + ul) / (1.0 - ul))), con);
        if (gctp_calc_phi2(cache_ptr->e, ts1, lat) != GCTP_SUCCESS)
            return GCTP_ERROR;
        con = cos(cache_ptr->bl * us /cache_ptr->al);
        theta = cache_ptr->lon_origin - atan2((s * cache_ptr->cosgam - vl 
            * cache_ptr->singam), con) / cache_ptr->bl;
        *lon = adjust_lon(theta);
    }

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to oblique mercator X,Y

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
    struct om_proj *cache_ptr = (struct om_proj *)trans->cache;
    double sin_phi;     /* sin value */
    double t;           /* temporary values */
    double con;         /* cone constant, small m */
    double q,us,vl;
    double ul,vs;
    double s;
    double dlon;
    double ts1;

    sin_phi = sin(lat);
    dlon = adjust_lon(lon - cache_ptr->lon_origin);
    vl = sin(cache_ptr->bl * dlon);
    if (fabs(fabs(lat) - HALF_PI) > EPSLN)
    {
        ts1 = gctp_calc_small_t(cache_ptr->e, lat, sin_phi);
        q = cache_ptr->el / (pow(ts1, cache_ptr->bl));
        s = .5 * (q - 1.0 / q);
        t = .5 * (q + 1.0/ q);
        ul = (s * cache_ptr->singam - vl * cache_ptr->cosgam) / t;
        con = cos(cache_ptr->bl * dlon);
        if (fabs(con) < .0000001)
        {
            us = cache_ptr->al * dlon;
        }
        else
        {
            us = cache_ptr->al * atan((s * cache_ptr->cosgam + vl
                * cache_ptr->singam) / con)/cache_ptr->bl;
            if (con < 0)
                us = us + PI * cache_ptr->al / cache_ptr->bl;
        }
    }
    else
    {
        if (lat >= 0)
            ul = cache_ptr->singam;
        else
            ul = -cache_ptr->singam;
        us = cache_ptr->al * lat / cache_ptr->bl;
    }
    if (fabs(fabs(ul) - 1.0) <= EPSLN)
    {
        GCTP_PRINT_ERROR("Point projects into infinity");
        return GCTP_ERROR;
    }
    vs = .5 * cache_ptr->al * log((1.0 - ul)/(1.0 + ul)) / cache_ptr->bl;
    us = us - cache_ptr->u;
    *x = cache_ptr->false_easting + vs * cache_ptr->cosaz 
        + us * cache_ptr->sinaz;
    *y = cache_ptr->false_northing + us * cache_ptr->cosaz 
        - vs * cache_ptr->sinaz;

    return GCTP_SUCCESS;
}


/*****************************************************************************
Name: gctp_om_inverse_init

Purpose: Initializes the inverse oblique mercator transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_om_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing oblique mercator inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_om_forward_init

Purpose: Initializes the forward oblique mercator transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_om_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing oblique mercator forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
