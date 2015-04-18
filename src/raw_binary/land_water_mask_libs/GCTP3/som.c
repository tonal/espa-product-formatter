/*******************************************************************************
Name: SPACE OBLIQUE MERCATOR (SOM)

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the SOM projection.  The Easting and Northing are in meters.
    The longitude and latitude values are in radians.

Notes:
1.  The X axis is in the direction of satellite motion.  For most other
    projections, that is roughly the negative Y axis.  Therefore, the false
    northing is applied to the X coordinate and the false easting is applied to
    the Y coordinate.


Algorithm References:

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  "Software Documentation for GCTP General Cartographic Transformation
    Package", U.S. Geological Survey National Mapping Division, May 1982.
*******************************************************************************/
#include <stdlib.h>
#include <math.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"

#define LANDSAT_RATIO 0.5201613

/* structure to hold the setup data relevant to this projection */
struct som_proj
{
    double lon_center;
    double a;
    double b;
    double a2;
    double a4;
    double c1;
    double c3;
    double q;
    double t;
    double u;
    double w;
    double xj;
    double p21;
    double sa;
    double ca;
    double es;
    double false_easting;
    double false_northing;
    int is_somb;
    int path;
    int satnum;
    double inclination_angle;   /* inclination angle in radians */
    int start;
};

/*****************************************************************************
Name: som_series

Purpose: Series to calculate a,b,c coefficients to convert from transform
    latitude,longitude to Space Oblique Mercator (SOM) rectangular coordinates
    Mathematical analysis by John Snyder 6/82

Returns:
    nothing

*****************************************************************************/
static void som_series
(
    struct som_proj *proj,  /* I/O: projection information */
    double *fb,     /* O */
    double *fa2,    /* O */
    double *fa4,    /* O */
    double *fc1,    /* O */
    double *fc3,    /* O */
    double dlam     /* I */
)
{
    double sd,sdsq,h,sq,fc;
    double s;
    double q_term;
    double w_term;

    dlam = dlam * 0.0174532925;               /* Convert dlam to radians */
    sd = sin(dlam);
    sdsq = sd * sd;
    q_term = 1.0 + proj->q * sdsq;
    w_term = 1.0 + proj->w * sdsq;
    s = proj->p21 * proj->sa * cos(dlam) * sqrt((1.0 + proj->t * sdsq) 
      / (w_term * q_term));
    h = sqrt(q_term / w_term) * ((w_term / (q_term * q_term))
        - proj->p21 * proj->ca);
    sq = sqrt(proj->xj * proj->xj + s * s);
    *fb = (h * proj->xj - s * s) / sq;
    *fa2 = *fb * cos(2.0 * dlam);
    *fa4 = *fb * cos(4.0 * dlam);
    fc = s * (h + proj->xj) / sq;
    *fc1 = fc * cos(dlam);
    *fc3 = fc * cos(3.0 * dlam);
}

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
    struct som_proj *cache_ptr = (struct som_proj *)trans->cache;

    gctp_print_title("SPACE OBLIQUE MERCATOR");
    gctp_print_radius2(cache_ptr->a, cache_ptr->b);
    if (cache_ptr->is_somb)
    {
        gctp_print_genrpt_long(cache_ptr->path,      "Path Number:    ");
        gctp_print_genrpt_long(cache_ptr->satnum,    "Satellite Number:    ");
    }
    gctp_print_genrpt(cache_ptr->inclination_angle * R2D,
        "Inclination of Orbit:    ");
    gctp_print_genrpt(cache_ptr->lon_center * R2D,
            "Longitude of Ascending Orbit:    ");
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

    double r_major;     /* major axis */
    double r_minor;     /* minor axis */
    double radius;      /* radius */
    int satnum;            /* Landsat satellite number (1,2,3,4,5) */
    int path;          /* Landsat path number */
    double lon_center;
    double false_easting;      /* x offset in meters           */
    double false_northing;     /* y offset in meters           */
    double time;
    int start1;
    int is_somb;
    const GCTP_PROJECTION *proj = &trans->proj;
    struct som_proj *cache = NULL;
    int spheroid = proj->spheroid;
    int i;
    double inclination_angle;
    double e2c,e2s,one_es;
    double dlam,fb,fa2,fa4,fc1,fc3,suma2,suma4,sumc1,sumc3,sumb;
    double p21;

    /* Get the projection parameters */
    gctp_get_spheroid(spheroid, proj->parameters, &r_major, &r_minor, &radius);
    false_easting = proj->parameters[6];
    false_northing = proj->parameters[7];
    start1 = (int)proj->parameters[10];
    is_somb = (int)proj->parameters[12];

    if (!is_somb)
    {
        satnum = 0;
        path = 0;

        if (gctp_dms2degrees(proj->parameters[3], &inclination_angle)
            != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting inclination angle parameter "
                "from DMS to degrees: %f", proj->parameters[3]);
            return GCTP_ERROR;
        }
        inclination_angle *= 3600 * S2R;

        if (gctp_dms2degrees(proj->parameters[4], &lon_center)
            != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error converting longitude of ascending orbit "
                "at equator parameter from DMS to degrees: %f",
                proj->parameters[4]);
            return GCTP_ERROR;
        }
        lon_center *= 3600 * S2R;

        time = proj->parameters[8];

        p21 = time / 1440.0;
    }
    else
    {
        satnum = (int)proj->parameters[2];
        path = (int)proj->parameters[3];

        if (satnum < 4)
        {
            inclination_angle = 99.092 * D2R;
            p21 = 103.2669323 / 1440.0;
            lon_center = (128.87 - (360.0 / 251.0 * path)) * D2R;
        }
        else
        {
            inclination_angle = 98.2 * D2R;
            p21 = 98.8841202 / 1440.0;
            lon_center = (129.30 - (360.0 / 233.0 * path)) * D2R;
        }
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
    cache->lon_center = lon_center;
    cache->false_easting = false_easting;
    cache->false_northing = false_northing;
    cache->is_somb = is_somb;
    cache->path = path;
    cache->satnum = satnum;
    cache->inclination_angle = inclination_angle;
    cache->start = start1;
    cache->p21 = p21;

    /* Place parameters in static storage for common use */
    cache->a = r_major;
    cache->b = r_minor;
    cache->es = 1.0 - SQUARE(r_minor / r_major);

    cache->ca = cos(inclination_angle);
    if (fabs(cache->ca) < 1.e-9)
        cache->ca = 1.e-9;
    cache->sa = sin(inclination_angle);
    e2c = cache->es * cache->ca * cache->ca;
    e2s = cache->es * cache->sa * cache->sa;
    cache->w = (1.0 - e2c) / (1.0 - cache->es);
    cache->w = cache->w * cache->w-1.0;
    one_es = 1.0 - cache->es;
    cache->q = e2s / one_es;
    cache->t = (e2s * (2.0 - cache->es)) / (one_es * one_es);
    cache->u = e2c / one_es;
    cache->xj = one_es * one_es * one_es;
    dlam = 0.0;
    som_series(cache, &fb,&fa2,&fa4,&fc1,&fc3, dlam);
    suma2 = fa2;
    suma4 = fa4;
    sumb = fb;
    sumc1 = fc1;
    sumc3 = fc3;
    for (i = 9; i <= 81; i += 18)
    {
        dlam = i;
        som_series(cache, &fb,&fa2,&fa4,&fc1,&fc3, dlam);
        suma2 = suma2 + 4.0 * fa2;
        suma4 = suma4 + 4.0 * fa4;
        sumb = sumb + 4.0 * fb;
        sumc1 = sumc1 + 4.0 * fc1;
        sumc3 = sumc3 + 4.0 * fc3;
    }
    for (i = 18; i <= 72; i += 18)
    {
        dlam = i;
        som_series(cache, &fb,&fa2,&fa4,&fc1,&fc3, dlam);
        suma2 = suma2 + 2.0 * fa2;
        suma4 = suma4 + 2.0 * fa4;
        sumb = sumb + 2.0 * fb;
        sumc1 = sumc1 + 2.0 * fc1;
        sumc3 = sumc3 + 2.0 * fc3;
    }

    dlam = 90.0;
    som_series(cache, &fb,&fa2,&fa4,&fc1,&fc3, dlam);
    suma2 = suma2 + fa2;
    suma4 = suma4 + fa4;
    sumb = sumb + fb;
    sumc1 = sumc1 + fc1;
    sumc3 = sumc3 + fc3;
    cache->a2 = suma2 / 30.0;
    cache->a4 = suma4 / 60.0;
    cache->b = sumb / 30.0;
    cache->c1 = sumc1 / 15.0;
    cache->c3 = sumc3 / 45.0;

    trans->print_info = print_info;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: inverse_transform

Purpose: Transforms SOM X,Y to lat,long

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
    struct som_proj *cache = (struct som_proj *)trans->cache;
    double tlon,conv,sav,sd,sdsq,blon,dif;
    double st,defac,actan,tlat,dd,bigk,bigk2,xlamt;
    double sl = 0.0;
    double scl = 0.0;
    double dlat = 0.0;
    double dlon;
    double s;
    int inumb;

    /* Apply false easting/northing.  Note that the axes of the SOM projection
       are defined differently than most projections.  The X axis is in the
       direction of satellite motion which is the inverse of the normal Y 
       axis.  So, the false northing is applied to the X axis. */
    y -= cache->false_easting;
    x -= cache->false_northing;

    /* Inverse equations. Begin inverse computation with approximation for
       tlon.  Solve for transformed long. */
    tlon = x / (cache->a * cache->b);
    conv = 1.e-9;
    for (inumb = 0; inumb < 50; inumb++)
    {
        sav = tlon;
        sd = sin(tlon);
        sdsq = sd * sd;
        s = cache->p21 * cache->sa * cos(tlon) * sqrt((1.0 + cache->t * sdsq)
            / ((1.0 + cache->w * sdsq) * (1.0 + cache->q * sdsq)));
        blon = (x / cache->a) + (y / cache->a) * s / cache->xj-cache->a2
            * sin(2.0 * tlon) - cache->a4 * sin(4.0 * tlon) - (s / cache->xj)
            * (cache->c1 * sin(tlon) + cache->c3 * sin(3.0 * tlon)); 
        tlon = blon / cache->b;
        dif = tlon - sav;
        if (fabs(dif) < conv)
            break; 
    }
    if (inumb >= 50)  
    {
        GCTP_PRINT_ERROR("Max iterations reached without converging");
        return GCTP_ERROR;
    }

    /* Compute transformed lat */
    st = sin(tlon);
    defac = exp(sqrt(1.0 + s * s / cache->xj / cache->xj) * (y 
        / cache->a - cache->c1 * st - cache->c3 * sin(3.0 * tlon)));
    actan = atan(defac);
    tlat = 2.0 * (actan - (PI / 4.0));

    /* Compute geodetic longitude */
    dd = st * st;
    if (fabs(cos(tlon)) < 1.e-7)
        tlon = tlon - 1.e-7;
    bigk = sin(tlat); 
    bigk2 = bigk * bigk;
    xlamt = atan(((1.0 - bigk2 / (1.0 - cache->es)) * tan(tlon) 
        * cache->ca - bigk * cache->sa * sqrt((1.0 + cache->q * dd)
        * (1.0 - bigk2) - bigk2 * cache->u) / cos(tlon)) / (1.0 - bigk2
        * (1.0 + cache->u)));

    /* Correct inverse quadrant */
    if (xlamt >= 0.0)
        sl = 1.0;
    if (xlamt < 0.0)
        sl = -1.0;
    if (cos(tlon) >= 0.0)
        scl = 1.0;
    if (cos(tlon) < 0.0)
        scl = -1.0;
    xlamt = xlamt - ((PI / 2.0) * (1.0 - scl) * sl);
    dlon = xlamt - cache->p21 * tlon;

    /* Compute geodetic latitude */
    if (fabs(cache->sa) < 1.e-7)
    {
        dlat = asin(bigk / sqrt((1.0 - cache->es) * (1.0 - cache->es)
            + cache->es * bigk2));
    }
    if (fabs(cache->sa) >= 1.e-7)
    {
        dlat = atan((tan(tlon) * cos(xlamt) - cache->ca * sin(xlamt)) 
            / ((1.0 - cache->es) * cache->sa));
    }
    *lon = adjust_lon(dlon + cache->lon_center);
    *lat = dlat;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: forward_transform

Purpose: Transforms lat,long to SOM X,Y

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
    struct som_proj *cache = (struct som_proj *)trans->cache;
    int n,l;
    double delta_lon;
    double rlm,tabs,tlam,xlam,c,xlamt,ab2,ab1,xlamp,sav;
    double d,sdsq,sd,tanlg,xtan,tphi,dp,rlm2;
    double scl = 0.0;
    double tlamp = 0.0;
    double conv,delta_lat,radlt,radln;
    double s;
    int done = 0;

    /* Forward equations */
    conv = 1.e-7;
    delta_lat = lat;
    delta_lon = lon-cache->lon_center;

    /* Test for latitude and longitude approaching 90 degrees */
    if (delta_lat>1.570796)
        delta_lat = 1.570796;
    if (delta_lat < -1.570796)
        delta_lat = -1.570796;
    radlt = delta_lat;
    radln = delta_lon;
    if (delta_lat >= 0.0)
        tlamp = PI / 2.0; 
    if (cache->start == 1)
        tlamp = 2.5 * PI;             /* Force to END */
    else if (cache->start == 0)
        tlamp = HALF_PI;   /* Force to START */
    if (delta_lat < 0.0)
        tlamp = 1.5 * PI;
    n = 0;

    while (1)
    {
        sav = tlamp;
        l = 0;
        xlamp = radln + cache->p21 * tlamp;
        ab1 = cos(xlamp);
        if (fabs(ab1) < conv)
            xlamp = xlamp - 1.e-7;
        if (ab1 >= 0.0)
            scl = 1.0;
        if (ab1 < 0.0)
            scl = -1.0;
        ab2 = tlamp - (scl) * sin(tlamp) * HALF_PI;

        while (l <= 50)
        {
            xlamt = radln + cache->p21 * sav;
            c = cos(xlamt);
            if (fabs(c) < 1.e-7)
                xlamt = xlamt - 1.e-7;
            xlam = (((1.0 - cache->es) * tan(radlt) * cache->sa) + sin(xlamt)
                * cache->ca) / c;
            tlam = atan(xlam);
            tlam = tlam + ab2;
            tabs = fabs(sav) - fabs(tlam);
            if (fabs(tabs) < conv)
            {
                /* Adjust for confusion at beginning and end of landsat 
                   orbits */
                rlm = PI * LANDSAT_RATIO;
                rlm2 = rlm + 2.0 * PI;
                n++;
                if ((n >= 3) || (tlam > rlm && tlam < rlm2))
                {
                    done = 1;
                    break;
                }
                if (tlam < rlm)
                {
                    tlamp = 2.50 * PI;
                    if (cache->start == 0)
                        tlamp = HALF_PI;
                }
                if (tlam >= rlm2) 
                {
                    tlamp = HALF_PI;
                    if (cache->start == 1)
                        tlamp = 2.50 * PI;
                }

                break;
            }

            l = l + 1;

            sav = tlam;
        }
        if (done)
            break;

        if (l > 50)
        {
            GCTP_PRINT_ERROR("Max iterations reached without converging");
            return GCTP_ERROR;
        }
    }

    /* tlam computed - now compute tphi */
    dp = sin(radlt);
    tphi = asin(((1.0 - cache->es) * cache->ca * dp - cache->sa * cos(radlt)
        * sin(xlamt)) / sqrt(1.0 - cache->es * dp * dp));

    /* compute x and y */
    xtan = (PI / 4.0) + (tphi / 2.0);
    tanlg = log(tan(xtan));
    sd = sin(tlam);
    sdsq = sd * sd;
    s = cache->p21 * cache->sa * cos(tlam) * sqrt((1.0 + cache->t * sdsq)
        / ((1.0 + cache->w * sdsq) * (1.0 + cache->q * sdsq)));
    d = sqrt(cache->xj * cache->xj + s * s);
    *x = cache->b * tlam + cache->a2 * sin(2.0 * tlam) + cache->a4
        * sin(4.0 * tlam) - tanlg * s / d;
    *x = cache->a * *x;
    *y = cache->c1 * sd + cache->c3 * sin(3.0 * tlam) + tanlg * cache->xj / d;
    *y = cache->a * *y;

    /* Apply false easting/northing.  Note that the axes of the SOM projection
       are defined differently than most projections.  The X axis is in the
       direction of satellite motion which is the inverse of the normal Y 
       axis.  So, the false northing is applied to the X axis. */
    *x += cache->false_northing;
    *y += cache->false_easting;
    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_som_inverse_init

Purpose: Initializes the inverse SOM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_som_inverse_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing SOM inverse projection");
        return GCTP_ERROR;
    }

    trans->transform = inverse_transform;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_som_forward_init

Purpose: Initializes the forward SOM transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_som_forward_init
(
    TRANSFORMATION *trans
)
{
    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error initializing SOM forward projection");
        return GCTP_ERROR;
    }

    trans->transform = forward_transform;

    return GCTP_SUCCESS;
}
