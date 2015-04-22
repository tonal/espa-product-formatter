/*******************************************************************************
Name: STATE PLANE

Purpose: Provides the transformations between Easting/Northing and longitude/
    latitude for the state plane projection.  The Easting and Northing
    values are in meters.  The longitude and latitude are in radians. 

Notes:
- The "state plane" projection is really nothing more than a way to use a
  "zone" to indicate one of 134 pre-defined projections with different 
  projection parameters.  The "zones" pick between the Transverse Mercator,
  Lambert Conformal Conic, Polyconic, and Oblique Mercator projections.

- This is implemented by creating a child projection transformation and
  tracking it in this module.

Algorithm References

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"

typedef struct state_plane_zone_data
{
    int zone_id;        /* Zone id */
    int proj_id;        /* projection id:  1 = Transverse Mercator,
                           2 = Lambert Conformal Conic, 3 = Polyconic,
                           4 = Oblique Mercator.  0 = not a valid zone */
    double table[9];    /* Array of 9 projection parameters.  Meaning varies
                           based on the proj_id. */
    char name[33];      /* Zone name */
} STATE_PLANE_ZONE_DATA;

/* Include the nad27_data and nad83_data tables */
#include "state_plane_table.h"


#if 0
/* Note that the following table appeared in the original GCTP code with code
   similar to:

   if ((*inspheroid == 0) && (*insys == SPCS) && (*inunit == STPLN_TABLE)) 
        unit = FEET;
    if ((*inspheroid == 8) && (*insys == SPCS) && (*inunit == STPLN_TABLE))
        unit = NADUT[(*inzone)/100];

    if (*insys == SPCS)
       *inunit = unit;

   And the same for the output projection.  So, essentially if the input
   units were STPLN_TABLE for the state plane projection, it would
   automatically select the units to use and return those units to the calling
   routine.  That feature has not been implemented here since nothing really
   appears to use it and it is very strange behavior.  If needed, this 
   feature should be implemented as a routine to return the units that
   should be used for a given zone and let the calling routines deal with it. */
*/

/* Table of unit codes as specified by state laws as of 2/1/92 for NAD 1983
   State Plane projection, 1 = U.S. Survey Feet, 2 = Meters, 5 = International
   Feet */
static long NADUT[134] = {1, 5, 1, 1, 5, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 2, 2,
              1, 1, 5, 2, 1, 2, 5, 1, 2, 2, 2, 1, 1, 1, 5, 2, 1, 5,
              2, 2, 5, 2, 1, 1, 5, 2, 2, 1, 2, 1, 2, 2, 1, 2, 2, 2};

#endif

/* structure to hold the setup data relevant to this projection */
struct state_plane_proj
{
    TRANSFORMATION child_transform;

    int zone;       /* state plane zone number */
    int sphere;     /* 0 = NAD27, 8 = GRS 1980 for NAD83 */
    int proj_id;    /* projection id: 1 = Transverse Mercator,
                       2 = Lambert Conformal Conic, 3 = Polyconic,
                       4 = Oblique Mercator */
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
    struct state_plane_proj *cache_ptr 
        = (struct state_plane_proj *)trans->cache;
    int nadval;

    gctp_print_title("STATE PLANE");
    gctp_print_genrpt_long(cache_ptr->zone, "Zone:     ");

    if (cache_ptr->sphere == 0)
       nadval = 27;
    else
       nadval = 83;

    gctp_print_genrpt_long(nadval, "Datum:     NAD");

    cache_ptr->child_transform.print_info(&cache_ptr->child_transform);
}

/*****************************************************************************
Name: dms2to3

Purpose: Function to convert 2 digit alternate packed DMS format
    (+/-)DDDMMSS.SSS to 3 digit standard packed DMS format (+/-)DDDMMMSSS.SSS.

Returns:
    3 digit DMS value
*****************************************************************************/
static double dms2to3
(
    double dms_2digit   /* Angle in 2 digit packed DMS format   */
)
{
    double con;
    double secs;
    long degs,mins;
    char sgna;

    sgna = ' ';
    if (dms_2digit < 0.0) 
        sgna = '-';
    con = fabs (dms_2digit);
    degs = (long) ((con / 10000.0) + .001);
    con =  con  - degs * 10000;
    mins = (long) ((con / 100.0) + .001);
    secs = con  - mins * 100;
    con = (double) (degs) * 1000000.0 + (double) (mins) * 1000.0 + secs;
    if (sgna == '-') 
        con = - con;
    return(con); 
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
    int ind;        /* index for the zone */
    int i;          /* loop control variable */
    const double *table;  /* array containing the projection information */
    const GCTP_PROJECTION *proj = &trans->proj;
    int id;
    struct state_plane_proj *cache = NULL;
    const STATE_PLANE_ZONE_DATA *zone_data;

    int zone = proj->zone;
    int sphere = proj->spheroid;

    ind = -1;

    /* Find the index for the zone
       --------------------------*/
    if (zone > 0)
    {
        if (sphere == 0)
        {
            zone_data = nad27_data;
        }
        else if (sphere == 8)
        {
            zone_data = nad83_data;
        }
        else
        {
            GCTP_PRINT_ERROR("Illegal spheroid #%4d", sphere);
            return GCTP_ERROR;
        }

        for (i = 0; i < 134; i++)
        {
            if ((zone == zone_data[i].zone_id) && (zone_data[i].proj_id != 0))
            {
                ind = i;
                break;
            }
        }
    }
    if (ind == -1)
    {
        GCTP_PRINT_ERROR("Illegal zone #%4d  for spheroid #%4d", zone, sphere);
        return GCTP_ERROR;
    }
    table = zone_data[ind].table;
    id = zone_data[ind].proj_id;
    if ((id <= 0) || (id > 4))
    {
        GCTP_PRINT_ERROR("Illegal zone #%4d  for spheroid #%4d", zone, sphere);
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
    cache->proj_id = id;
    cache->zone = zone;
    cache->sphere = sphere;

    /* Build the child transform projection information */
    for (i = 0; i < GCTP_PROJECTION_PARAMETER_COUNT; i++)
        cache->child_transform.proj.parameters[i] = 0.0;
    cache->child_transform.proj.spheroid = sphere;
    cache->child_transform.proj.units = proj->units;
    cache->child_transform.proj.zone = 0;

    /* initialize proper projection */
    if (id == 1)
    {
        cache->child_transform.proj.proj_code = TM;
        /* scale factor */
        cache->child_transform.proj.parameters[2] = table[3];
        /* center longitude */
        cache->child_transform.proj.parameters[4] = dms2to3(table[2]);
        /* latitude of origin */
        cache->child_transform.proj.parameters[5] = dms2to3(table[6]);
        /* false easting */
        cache->child_transform.proj.parameters[6] = table[7];
        /* false northing */
        cache->child_transform.proj.parameters[7] = table[8];
    }
    else if (id == 2)
    {
        cache->child_transform.proj.proj_code = LAMCC;
        /* lat1 */
        cache->child_transform.proj.parameters[2] = dms2to3(table[5]);
        /* lat2 */
        cache->child_transform.proj.parameters[3] = dms2to3(table[4]);
        /* center longitude */
        cache->child_transform.proj.parameters[4] = dms2to3(table[2]);
        /* latitude of origin */
        cache->child_transform.proj.parameters[5] = dms2to3(table[6]);
        /* false easting */
        cache->child_transform.proj.parameters[6] = table[7];
        /* false northing */
        cache->child_transform.proj.parameters[7] = table[8];
    }
    else if (id == 3)
    {
        cache->child_transform.proj.proj_code = POLYC;
        /* center longitude */
        cache->child_transform.proj.parameters[4] = dms2to3(table[2]);
        /* center latitude */
        cache->child_transform.proj.parameters[5] = dms2to3(table[3]);
        /* false easting */
        cache->child_transform.proj.parameters[6] = table[4];
        /* false northing */
        cache->child_transform.proj.parameters[7] = table[5];
    }
    else if (id == 4)
    {
        cache->child_transform.proj.proj_code = HOM;
        /* scale factor */
        cache->child_transform.proj.parameters[2] = table[3];
        /* azimuth */
        cache->child_transform.proj.parameters[3] = dms2to3(table[5]);
        /* longitude of origin */
        cache->child_transform.proj.parameters[4] = dms2to3(table[2]);
        /* latitude of origin */
        cache->child_transform.proj.parameters[5] = dms2to3(table[6]);
        /* false easting */
        cache->child_transform.proj.parameters[6] = table[7];
        /* false northing */
        cache->child_transform.proj.parameters[7] = table[8];
        /* using format B */
        cache->child_transform.proj.parameters[12] = 1.0;
    }

    trans->print_info = print_info;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: destroy

Purpose: Releases the resources allocated by this projection.

Returns:
    nothing

*****************************************************************************/
static void destroy
(
    TRANSFORMATION *trans
)
{
    struct state_plane_proj *cache = (struct state_plane_proj *)trans->cache;
    free(cache->child_transform.cache);
    cache->child_transform.cache = NULL;
}
/*****************************************************************************
Name: inverse_transform

Purpose: Transforms UTM X,Y to lat,long

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
    struct state_plane_proj *cache_ptr
        = (struct state_plane_proj *)trans->cache;

    return cache_ptr->child_transform.transform(&cache_ptr->child_transform,
        x, y, lon, lat);
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
    struct state_plane_proj *cache_ptr
        = (struct state_plane_proj *)trans->cache;

    return cache_ptr->child_transform.transform(&cache_ptr->child_transform,
        lon, lat, x, y);
}

/*****************************************************************************
Name: gctp_state_plane_inverse_init

Purpose: Initializes the inverse state plane transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_state_plane_inverse_init
(
    TRANSFORMATION *trans
)
{
    struct state_plane_proj *cache;

    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing state plane inverse projection");
        return GCTP_ERROR;
    }

    cache = (struct state_plane_proj *)trans->cache;

    if (cache->proj_id == 1)
        gctp_tm_inverse_init(&cache->child_transform);
    else if (cache->proj_id == 2)
        gctp_lamcc_inverse_init(&cache->child_transform);
    else if (cache->proj_id == 3)
        gctp_poly_inverse_init(&cache->child_transform);
    else if (cache->proj_id == 4)
        gctp_om_inverse_init(&cache->child_transform);

    trans->transform = inverse_transform;
    trans->destroy = destroy;

    return GCTP_SUCCESS;
}

/*****************************************************************************
Name: gctp_state_plane_forward_init

Purpose: Initializes the forward state plane transformation

Returns:
    GCTP_SUCCESS or GCTP_ERROR

*****************************************************************************/
int gctp_state_plane_forward_init
(
    TRANSFORMATION *trans
)
{
    struct state_plane_proj *cache;

    /* Call the common routine used for the forward and inverse init */
    if (common_init(trans) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR(
            "Error initializing state plane forward projection");
        return GCTP_ERROR;
    }

    cache = (struct state_plane_proj *)trans->cache;

    if (cache->proj_id == 1)
        gctp_tm_forward_init(&cache->child_transform);
    else if (cache->proj_id == 2)
        gctp_lamcc_forward_init(&cache->child_transform);
    else if (cache->proj_id == 3)
        gctp_poly_forward_init(&cache->child_transform);
    else if (cache->proj_id == 4)
        gctp_om_forward_init(&cache->child_transform);

    trans->transform = forward_transform;
    trans->destroy = destroy;

    return GCTP_SUCCESS;
}
