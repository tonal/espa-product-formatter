/*****************************************************************************
FILE: espa_geoloc.c
  
PURPOSE: Contains functions and defines for handling geolocation-related
conversions and computations, such as mapping from image space to geo space
and vice versa.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/23/2014    Gail Schmidt     Original development, borrowed code from space.c
                              in the LEDAPS application

NOTES:
1. Geodetic coordinates are the geodetic latitude and longitude of the point
   to be mapped. Geodetic cooridnates are in radians.
2. Only one space mapping definition (instance) should be used at any one
   time because the GCTP package is not re-entrant.
3. Image coordinates are in pixels with the origin (0.0, 0.0) at the center
   of the upper left corner pixel.  Sample coordinates are positive to the
   right and line coordinates are positive downward.
4. 'get_geoloc_info' is used to get the appropriate information from the XML
   file needed to setup the mapping structure.
5. 'setup_mapping' must be called before 'to_space', 'from_space', and
   'compute_bounds' routines.
6. GCTP stands for the General Cartographic Transformation Package and the
   library and associated code is available as a subdirectory delivered with
   the HDF-EOS code.
*****************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "espa_geoloc.h"

/* Constants */
#define MAX_PROJ (99)  /* Maximum map projection number */
#define GCTP_OK 0    /* Okay status return from the GCTP package */

/* Prototypes for initializing the GCTP projections */
int for_init (int outsys, int outzone, double *outparm, int outdatum, 
    char *fn27, char *fn83, int *iflg,
    int (*for_trans[])(double, double, double *, double *));
int inv_init (int insys, int inzone, double *inparm, int indatum,
    char *fn27, char *fn83, int *iflg,
    int (*inv_trans[])(double, double, double*, double*));
 

/******************************************************************************
MODULE:  setup_mapping

PURPOSE:  Sets up the geolocation data structure and initializes the forward
and inverse mapping functions via GCTP.

RETURN VALUE:
Type = Geoloc_t *
Value      Description
-----      -----------
NULL       Error occurred in the setup
not-NULL   Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/23/2014     Gail Schmidt     Original Development (based on input routines
                               from the LEDAPS lndsr application)
4/25/2014     Gail Schmidt     Updated to support additional projections

NOTES:
1. Memory is allocated for the Geoloc_t pointer.  It is up to the calling
   routine to free the memory for this pointer.
2. Make sure the corners Space_def_t are reported as the upper left of the
   the since that's what the other routines are expecting.
******************************************************************************/
Geoloc_t *setup_mapping
(
    Space_def_t *space_def     /* I: space definition structure */
)
{
    char FUNC_NAME[] = "setup_mapping"; /* function name */
    char errmsg[STR_SIZE];              /* error message */
    char file27[] = "FILE27";           /* file for NAD27 (only for State Plane)
                                           so just use something fake for now */
    char file83[] = "FILE83";           /* file for NAD83 (only for State Plane)
                                           so just use something fake for now */
    Geoloc_t *this = NULL;              /* pointer to the space structure */
    double temp1, temp2;                /* temp variables for PS projection */
    int i;                              /* looping variable */
    int iflag;                          /* return status from GCTP */
    int (*for_trans[MAX_PROJ + 1])();   /* forward transformation function */
    int (*inv_trans[MAX_PROJ + 1])();   /* inverse transformation function */
  
    /* Verify some of the space definition parameters */
    if (space_def->img_size.l < 1) 
    {
        sprintf (errmsg, "Invalid number of lines: %d", space_def->img_size.l);
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }
    if (space_def->img_size.s < 1) 
    {
        sprintf (errmsg, "Invalid number of samples per line: %d",
            space_def->img_size.s);
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }

    if (space_def->pixel_size[0] <= 0.0 || space_def->pixel_size[1] <= 0.0)
    {
        sprintf (errmsg, "Invalid pixel size: %lf %lf",
            space_def->pixel_size[0], space_def->pixel_size[1]);
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }

    if (space_def->proj_num < 0  ||  space_def->proj_num > MAX_PROJ)
    {
        sprintf (errmsg, "Invalid projection number: %d", space_def->proj_num);
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }
  
    /* Create the geolocation data structure */
    this = malloc (sizeof (Geoloc_t));
    if (this == NULL) 
    {
        sprintf (errmsg, "Allocating geolocation data structure");
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }
  
    /* Copy the space definition fields */
    this->def.pixel_size[0] = space_def->pixel_size[0];
    this->def.pixel_size[1] = space_def->pixel_size[1];
    this->def.ul_corner.x = space_def->ul_corner.x;
    this->def.ul_corner.y = space_def->ul_corner.y;
    this->def.img_size.l = space_def->img_size.l;
    this->def.img_size.s = space_def->img_size.s;
    this->def.proj_num = space_def->proj_num;
    this->def.zone = space_def->zone;
    this->def.spheroid = space_def->spheroid;
    this->def.orientation_angle = space_def->orientation_angle;
    for (i = 0; i < NPROJ_PARAM; i++) 
        this->def.proj_param[i] = space_def->proj_param[i];
  
    /* Calculate the orientation cosine/sine */
    this->sin_orien = sin (space_def->orientation_angle);
    this->cos_orien = cos (space_def->orientation_angle);
  
    /* Convert angular projection parameters to DMS the necessary projections */
    if (this->def.proj_num == GCTP_PS_PROJ ||
        this->def.proj_num == GCTP_ALBERS_PROJ)
    {
        if (!degdms (&this->def.proj_param[4], &temp1, "DEG", "LON" ) ||
            !degdms (&this->def.proj_param[5], &temp2, "DEG", "LAT" ))
        {
            free (this);
            sprintf (errmsg, "Converting PS or ALBERS angular parameters from "
                "degrees to DMS");
            error_handler (true, FUNC_NAME, errmsg);
            return (NULL);
        }
        this->def.proj_param[4] = temp1;
        this->def.proj_param[5] = temp2;
    }
    else if (this->def.proj_num == GCTP_SIN_PROJ)
    {
        if (!degdms (&this->def.proj_param[4], &temp1, "DEG", "LON" ))
        {
            free (this);
            sprintf (errmsg, "Converting SIN angular parameters from degrees "
                "to DMS");
            error_handler (true, FUNC_NAME, errmsg);
            return (NULL);
        }
        this->def.proj_param[4] = temp1;
    }
     
    /* Setup the forward transform */
    for_init (this->def.proj_num, this->def.zone, this->def.proj_param, 
        this->def.spheroid, file27, file83, &iflag, for_trans);
    if (iflag)
    {
        free (this);
        sprintf (errmsg, "Error returned from for_init");
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }
    this->for_trans = for_trans[this->def.proj_num];
  
    /* Setup the inverse transform */
    inv_init (this->def.proj_num, this->def.zone, this->def.proj_param, 
        this->def.spheroid, file27, file83, &iflag, inv_trans);
    if (iflag)
    {
        free (this);
        sprintf (errmsg, "Error returned from for_init");
        error_handler (true, FUNC_NAME, errmsg);
        return (NULL);
    }
    this->inv_trans = inv_trans[this->def.proj_num];
  
    /* Successful completion */
    return (this);
}


/******************************************************************************
MODULE:  to_space

PURPOSE:  Maps a point from geodetic coordinates to line, sample space.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred in the mapping
true       Successful mapping

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/23/2014     Gail Schmidt     Original Development (based on input routines
                               from the LEDAPS lndsr application)

NOTES:
1. Report image coordinates for the UL corner of the pixel.
******************************************************************************/
bool to_space
(
    Geoloc_t *this,          /* I: geolocation structure; for_trans function
                                   is used for the forward mapping */
    Geo_coord_t *geo,        /* I: geodetic coordinates (radians) */
    Img_coord_float_t *img   /* O: image coordinates (for UL corner of pixel) */
)
{
    char FUNC_NAME[] = "to_space";  /* function name */
    char errmsg[STR_SIZE];          /* error message */
    Map_coord_t map;                /* coordinate in projection space */
    double dx, dy;                  /* delta x, y values */
    double dl, ds;                  /* delta line, sample values */

    /* If this coordinate is fill then skip */
    img->is_fill = true;
    if (geo->is_fill)
    {
        sprintf (errmsg, "Forward mapping called with geodetic coordinate "
            "that is fill.");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }

    /* Do the forward mapping */
    if (this->for_trans (geo->lon, geo->lat, &map.x, &map.y) != GCTP_OK) 
    {
        sprintf (errmsg, "Geodetic coordinate failed the forward mapping.");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }

    /* Determine the line, sample location from the projection space */
    dx = map.x - this->def.ul_corner.x;
    dy = map.y - this->def.ul_corner.y;

    dl = (dx * this->sin_orien) - (dy * this->cos_orien);
    ds = (dx * this->cos_orien) + (dy * this->sin_orien);

    img->l = dl / this->def.pixel_size[1];
    img->s = ds / this->def.pixel_size[0];
    img->is_fill = false;

    /* Successful completion */
    return (true);
}


/******************************************************************************
MODULE:  from_space

PURPOSE:  Maps a point from line, sample space to geodetic coordinates.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred in the mapping
true       Successful mapping

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/23/2014     Gail Schmidt     Original Development (based on input routines
                               from the LEDAPS lndsr application)

NOTES:
1. Report image coordinates for the UL corner of the pixel.
******************************************************************************/
bool from_space
(
    Geoloc_t *this,          /* I: geolocation structure; inv_trans function
                                   is used for the inverse mapping */
    Img_coord_float_t *img,  /* I: image coordinates (for UL corner of pixel) */
    Geo_coord_t *geo         /* O: geodetic coordinates (radians) */
)
{
    char FUNC_NAME[] = "from_space";  /* function name */
    char errmsg[STR_SIZE];            /* error message */
    Map_coord_t map;                  /* coordinate in projection space */
    double dx, dy;                    /* delta x, y values */
    double dl, ds;                    /* delta line, sample values */

    /* If this coordinate is fill then skip */
    geo->is_fill = true;
    if (img->is_fill)
    {
        sprintf (errmsg, "Inverse mapping called with line, sample coordinate "
            "that is fill.");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }

    /* Determine the line,sample location in projection space */
    dl = img->l * this->def.pixel_size[1];
    ds = img->s * this->def.pixel_size[0];

    dy = (ds * this->sin_orien) - (dl * this->cos_orien);
    dx = (ds * this->cos_orien) + (dl * this->sin_orien);

    map.y = this->def.ul_corner.y + dy;
    map.x = this->def.ul_corner.x + dx;

    /* Do the inverse mapping */
    if (this->inv_trans (map.x, map.y, &geo->lon, &geo->lat) != GCTP_OK) 
    {
        sprintf (errmsg, "Projection coordinate failed the inverse mapping.");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }
    geo->is_fill = false;

    /* Successful completion */
    return (true);
}


/******************************************************************************
MODULE:  get_geoloc_info

PURPOSE:  Copy the geolocation information from the XML structure.

RETURN VALUE: N/A
Type = bool
Value      Description
-----      -----------
false      Error occurred getting geolocation information
true       Successfully obtained geolocation information

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/23/2014     Gail Schmidt     Original Development

NOTES:
1. Make sure UL corner pixel is UL corner of the pixel, since that's what
   setup_mapping expects.
******************************************************************************/
bool get_geoloc_info
(
    Espa_internal_meta_t *xml_metadata, /* I: XML metadata information */
    Space_def_t *geoloc_info            /* O: geolocation space information */
)
{
    char FUNC_NAME[] = "get_geoloc_info"; /* function name */
    char errmsg[STR_SIZE];                /* error message */
    int i;                                /* looping variable */
    int refl_indx = -99;                  /* index of band1 */
    Espa_global_meta_t *gmeta=&xml_metadata->global;  /* global metadata */

    /* Use band1 band-related metadata for the reflectance information */
    for (i = 0; i < xml_metadata->nbands; i++)
    {
        if (!strcmp (xml_metadata->band[i].name, "band1") &&
            !strncmp (xml_metadata->band[i].product, "L1", 2))  /* L1G or L1T */
        {
            /* this is the index we'll use for reflectance band info */
            refl_indx = i;
        }
    }

    if (refl_indx == -99)
    {
        sprintf (errmsg, "band1 not found in the input XML file");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }

    /* Copy the information from the XML file */
    geoloc_info->proj_num = gmeta->proj_info.proj_type;
    geoloc_info->pixel_size[0] = xml_metadata->band[refl_indx].pixel_size[0];
    geoloc_info->pixel_size[1] = xml_metadata->band[refl_indx].pixel_size[1];
    geoloc_info->img_size.l = xml_metadata->band[refl_indx].nlines;
    geoloc_info->img_size.s = xml_metadata->band[refl_indx].nsamps;

    /* If the grid origin is center, then adjust for the resolution.  The
       corners will be written for the UL of the corner. */
    if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
    {
        geoloc_info->ul_corner.x = gmeta->proj_info.ul_corner[0] -
            0.5 * xml_metadata->band[refl_indx].pixel_size[0];
        geoloc_info->ul_corner.y = gmeta->proj_info.ul_corner[1] +
            0.5 * xml_metadata->band[refl_indx].pixel_size[1];
    }
    else
    {
        geoloc_info->ul_corner.x = gmeta->proj_info.ul_corner[0];
        geoloc_info->ul_corner.y = gmeta->proj_info.ul_corner[1];
    }
    geoloc_info->ul_corner_set = true;
    
    /* Projection related info */
    geoloc_info->zone_set = false;
    for (i = 0; i < NPROJ_PARAM; i++)
        geoloc_info->proj_param[i] = 0.0;

    switch (gmeta->proj_info.proj_type)
    {
        case GCTP_GEO_PROJ:
            /* just use the already initialized zeros for the proj param */
            break;

        case GCTP_UTM_PROJ:
            geoloc_info->zone = gmeta->proj_info.utm_zone;
            geoloc_info->zone_set = true;
            break;

        case GCTP_ALBERS_PROJ:
            geoloc_info->proj_param[2] = gmeta->proj_info.standard_parallel1;
            geoloc_info->proj_param[3] = gmeta->proj_info.standard_parallel2;
            geoloc_info->proj_param[4] = gmeta->proj_info.central_meridian;
            geoloc_info->proj_param[5] = gmeta->proj_info.origin_latitude;
            geoloc_info->proj_param[6] = gmeta->proj_info.false_easting;
            geoloc_info->proj_param[7] = gmeta->proj_info.false_northing;
            break;

        case GCTP_PS_PROJ:
            geoloc_info->proj_param[4] = gmeta->proj_info.longitude_pole;
            geoloc_info->proj_param[5] = gmeta->proj_info.latitude_true_scale;
            geoloc_info->proj_param[6] = gmeta->proj_info.false_easting;
            geoloc_info->proj_param[7] = gmeta->proj_info.false_northing;
            break;

        case GCTP_SIN_PROJ:
            geoloc_info->proj_param[0] = gmeta->proj_info.sphere_radius;
            geoloc_info->proj_param[4] = gmeta->proj_info.central_meridian;
            geoloc_info->proj_param[6] = gmeta->proj_info.false_easting;
            geoloc_info->proj_param[7] = gmeta->proj_info.false_northing;
            break;

        default:
            sprintf (errmsg, "Unsupported projection type (%d).  GEO "
                "projection code (%d) or UTM projection code (%d) or "
                "ALBERS projection code (%d) or PS projection code (%d) or "
                "SIN projection code (%d) expected.",
                gmeta->proj_info.proj_type, GCTP_GEO_PROJ, GCTP_UTM_PROJ,
                GCTP_ALBERS_PROJ, GCTP_PS_PROJ, GCTP_SIN_PROJ);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
    }

    /* Get the proper spheroid for the current datum
       WGS84 datum <-- WGS84 spheroid, NAD27 datum <-- Clarke 86 spheroid,
       NAD83 datum <-- GRS80 spheroid */
    switch (gmeta->proj_info.datum_type)
    {
        case (ESPA_WGS84):
            geoloc_info->spheroid = GCTP_WGS84;
            break;
        case (ESPA_NAD27):
            geoloc_info->spheroid = GCTP_CLARKE_1866;
            break;
        case (ESPA_NAD83):
            geoloc_info->spheroid = GCTP_GRS80;
            break;
        case (ESPA_NODATUM):
            if (gmeta->proj_info.proj_type == GCTP_SIN_PROJ)
                geoloc_info->spheroid = GCTP_MODIS_SPHERE;
            else
            {
                sprintf (errmsg, "Unsupported datum/projection combination. "
                    "NoDatum is currently only supported with Sinusoidal.");
                error_handler (true, FUNC_NAME, errmsg);
                return (false);
            }
            break;
        default:
            sprintf (errmsg, "Unsupported datum. Currently only WGS84, NAD27, "
                "NAD83, and NODATUM (Sinusoidal) are supported.");
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
    }

    /* Convert the orientation angle to radians */
    geoloc_info->orientation_angle = gmeta->orientation_angle * RAD;
 
    /* Successful completion */
    return (true);
}


/******************************************************************************
MODULE:  compute_bounds

PURPOSE:  Computes the boundary corners of the output image.  For ascending
scenes and scenes in the polar regions, the scenes are flipped upside down.
The bounding coords will be correct in North represents the northernmost
latitude and South represents the southernmost latitude.  However, the UL
corner in this case would be more south than the LR corner.  Comparing the UL
and LR corners will allow the user to determine if the scene is flipped.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred in the computation or mapping
true       Successfully computed the image bounds

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/23/2014     Gail Schmidt     Original Development (based on input routines
                               from the LEDAPS lndsr application)
NOTES:
1. Memory is allocated for the Geoloc_t pointer.  It is up to the calling
   routine to free the memory for this pointer.
2. This assumes the setup mapping was setup using the UL of the UL pixel.
   It then loops around the outer edges of each pixel as it loops around the
   outer edges of the entire image.
******************************************************************************/
bool compute_bounds
(
    Geoloc_t *space,          /* I: geolocation structure which contains the
                                    information used for forward/inverse
                                    mapping */
    int nlines,               /* I: number of lines in the scene */
    int nsamps,               /* I: number of samples in the scene */
    Geo_bounds_t *bounds      /* O: output boundary for the scene */
)
{
    char FUNC_NAME[] = "compute_bounds";  /* function name */
    char errmsg[STR_SIZE];            /* error message */
    Img_coord_float_t img;            /* image coordinates for current pixel */
    Geo_coord_t geo;                  /* geodetic coordinates (note radians) */
    int ix, iy;                       /* current x,y coordinates */

    /* Initialize the bounding coordinates with the upper left of the UL
       corner */
    img.l = 0.0;
    img.s = 0.0;
    img.is_fill = false;
    if (!from_space (space, &img, &geo))
    {
        sprintf (errmsg, "Mapping line, sample pixel to lat/long");
        error_handler (true, FUNC_NAME, errmsg);
        return (false);
    }

    bounds->max_lat = geo.lat * DEG;
    bounds->min_lat = geo.lat * DEG;
    bounds->max_lon = geo.lon * DEG;
    bounds->min_lon = geo.lon * DEG;

    /* Determine the bounding coords by looping around the edges of the image
       in line, sample space and converting to lat/long space. Remember that
       the to/from space mappings are initialized using the UL of the UL corner
       of the image. Thus we need to go an extra pixel to the right and bottom
       of the image to get the true outer extents. */
    /** top -- go to (nsamps-1) + 1 to get to the far right edge of the
        image **/
    for (ix = 0; ix <= nsamps; ix++)
    {
        iy = 0;
        img.l = (double) iy;
        img.s = (double) ix;
        img.is_fill = false;
        if (!from_space (space, &img, &geo))
        {
            sprintf (errmsg, "Mapping top line of the image to lat/long");
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        bounds->max_lat = max (bounds->max_lat, geo.lat*DEG);
        bounds->min_lat = min (bounds->min_lat, geo.lat*DEG);
        bounds->max_lon = max (bounds->max_lon, geo.lon*DEG);
        bounds->min_lon = min (bounds->min_lon, geo.lon*DEG);
    }

    /** bottom -- go to (nsamps-1) + 1 to get to the far right edge of the
        image **/
    for (ix = 0; ix <= nsamps; ix++)
    {
        iy = nlines;  /* this is actually (nlines-1) + 1 to get to the very
                         bottom edge of the image */
        img.l = (double) iy;
        img.s = (double) ix;
        img.is_fill = false;
        if (!from_space (space, &img, &geo))
        {
            sprintf (errmsg, "Mapping top line of the image to lat/long");
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        bounds->max_lat = max (bounds->max_lat, geo.lat*DEG);
        bounds->min_lat = min (bounds->min_lat, geo.lat*DEG);
        bounds->max_lon = max (bounds->max_lon, geo.lon*DEG);
        bounds->min_lon = min (bounds->min_lon, geo.lon*DEG);
    }

    /** left -- go to (nlines-1) + 1 to get to the bottom edge of the image **/
    for (iy = 0; iy <= nlines; iy++)
    {
        ix = 0;
        img.l = (double) iy;
        img.s = (double) ix;
        img.is_fill = false;
        if (!from_space (space, &img, &geo))
        {
            sprintf (errmsg, "Mapping top line of the image to lat/long");
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        bounds->max_lat = max (bounds->max_lat, geo.lat*DEG);
        bounds->min_lat = min (bounds->min_lat, geo.lat*DEG);
        bounds->max_lon = max (bounds->max_lon, geo.lon*DEG);
        bounds->min_lon = min (bounds->min_lon, geo.lon*DEG);
    }

    /** right -- go to (nlines-1) + 1 to get to the bottom edge of the image **/
    for (iy = 0; iy <= nlines; iy++)
    {
        ix = nsamps;  /* this is actually (nsamps-1) + 1 to get to the far
                         right edge of the image */
        img.l = (double) iy;
        img.s = (double) ix;
        img.is_fill = false;
        if (!from_space (space, &img, &geo))
        {
            sprintf (errmsg, "Mapping top line of the image to lat/long");
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        bounds->max_lat = max (bounds->max_lat, geo.lat*DEG);
        bounds->min_lat = min (bounds->min_lat, geo.lat*DEG);
        bounds->max_lon = max (bounds->max_lon, geo.lon*DEG);
        bounds->min_lon = min (bounds->min_lon, geo.lon*DEG);
    }

    /* Successful completion */
    return (true);
}


/******************************************************************************
MODULE:  degdms

PURPOSE:  Converts decimal degrees (or minutes or seconds) to DMS or validates
the DMS value.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error occurred in the conversion or DMS value is not valid
true       Successful conversion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/24/2014     Gail Schmidt     Original Development (based on input routines
                               from LEDAPS)

NOTES:
1. The input 'deg' variable can contain the angle in degrees (code=DEG),
   minutes (code=MIN), or seconds (code=SEC) when converting to DMS.
2. The input 'deg' variable can contain an angle in DMS (code=DMS) in which
   the DMS angle value will then be validated.  'dms' variable in this case
   will simply be a copy of the 'deg' variable.
******************************************************************************/
bool degdms
(
    double *deg,     /* I: input angular value in degrees, minutes, or seconds
                           to be converted to DMS; or input angle in DMS to be
                           validated (no conversion occurs) */
    double *dms,     /* O: output angular value in DMS converted from degrees,
                           minutes, or seconds; or copy of 'deg' value
                           (code=DMS) */
    char *code,      /* I: code to identify if the input angle is in degrees,
                           minutes, or seconds (DEG, MIN, SEC) for conversion
                           to DMS; or code to identify the input 'deg' value
                           is in DMS in which case the DMS value will simply
                           be validated and not converted */
    char *check      /* I: angle type to check/validate (LAT, LON, OTHER) */
)
{
    char FUNC_NAME[] = "degdms";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    double tsec;              /* temporary seconds value */
    double MAXDMS;            /* maximum DMS value based on angle type */
    double MAXMIN = 60060.0;  /* maximum minutes value */
    double MAXSEC = 60.0;     /* maximum seconds value */
    double MINDMS;            /* minimum DMS value */
    double MINMIN = -60060.0; /* minimum minutes value */
    double MINSEC = -60.0;    /* minimum seconds value */
    double tempmin;           /* temporary minutes value */
    double tempsec;           /* temporary seconds value */
    long tdeg;                /* temporary degrees value */
    long tmin;                /* temporary minutes value */
    int sign;                 /* sign of the angle */

    /* Setup the max/min DMS value based on the type of angle to be processed */
    if (strcmp (check, "LAT") == 0)
    {  /* -90.0 to 90.0 */
        MAXDMS = 90000000.0;
        MINDMS = -90000000.0;
    }
    else if (strcmp (check, "LON") == 0)
    {  /* -180.0 to 180.0 */
        MAXDMS = 180000000.0;
        MINDMS = -180000000.0;
    }
    else
    {  /* 0.0 to 360.0 */
        MAXDMS = 360000000.0;
        MINDMS = 0.0;
    }

    /* Take care of the conversion from DEG to DMS or DMS to DEG */
    if (strcmp (code, "DMS"))
    {   /* Input is in DEG, MIN, or SEC */
        /* Convert minutes to degrees */
        if (!strcmp (code, "MIN"))
            *deg = *deg / 60.0;

        /* Convert seconds to degrees */
        else if (!strcmp (code, "SEC"))
            *deg = *deg / 3600.0;

        /* Convert to DMS */
        tdeg = (long) find_deg (*deg);
        tmin = (long) find_min (*deg);
        tsec = find_sec (*deg);
        sign = 1;
        if (*deg < 0)
            sign = -1;
        tdeg = abs (tdeg);
        *dms = (tdeg * 1000000 + tmin * 1000 + tsec) * sign;

        /* Check to make sure DMS value is valid */
        if (*dms > MAXDMS || *dms < MINDMS)
        {
            sprintf (errmsg, "Invalid input coordinate value: %f.  DMS "
                "conversion (%f).", *deg, *dms);
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }
    }
    else
    {   /* Handle conversion from DMS to deg */
        *dms = *deg;

        /* Check to be sure coordinate is valid */
        if (*dms > MAXDMS || *dms < MINDMS)
        {
            sprintf (errmsg, "Invalid DMS coordinate value: %f", *dms);
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        /* If it's not a LAT or LON value then DMS must fall within 0-360 */
        if (strcmp (check, "LAT") && strcmp (check, "LON"))
        {
            if (*dms <= 0)
            {
                sprintf (errmsg, "Invalid DMS coordinate value: %f", *dms);
                error_handler (true, FUNC_NAME, errmsg);
                return (false);
            }
        }

        /* Parse out the minutes value from DMS and check against MAXMIN */
        tempmin = *dms - (((int) (*dms / 1000000)) * 1000000);
        if (tempmin > MAXMIN || tempmin < MINMIN)
        {
            sprintf (errmsg, "Invalid DMS coordinate value: %f", *dms);
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }

        /* Parse out the seconds value from DMS and check against MAXSEC */
        tempsec = *dms - (((int) (*dms / 1000)) * 1000);
        if (tempsec > MAXSEC || tempsec < MINSEC)
        {
            sprintf (errmsg, "Invalid DMS coordinate value: %f", *dms);
            error_handler (true, FUNC_NAME, errmsg);
            return (false);
        }
    }

    return (true);
}


/******************************************************************************
MODULE:  find_deg

PURPOSE:  Extracts the degree portion of the input angle.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
deg        Degree portion of the input angle

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/24/2014     Gail Schmidt     Original Development (based on input routines
                               from LEDAPS)

NOTES:
******************************************************************************/
int find_deg
(
    double angle    /* I: angle in total degrees */
)
{
    int sign;            /* sign of angle */
    int deg;             /* degrees portion of angle */
    int minute;          /* minutes portion of angle */
    double sec;          /* seconds portion of angle */

    /* Get the sign of the angle */
    sign = 1;
    if (angle < 0)
        sign = -1;

    /* Split angle into deg, minutes, seconds */
    deg = (int) fabs (angle);
    minute = (int) ((fabs (angle) - deg) * 60.0);
    sec = (((fabs (angle) - deg) * 60.0) - minute) * 60.0;

    /* Keep seconds and minutes in a valid range */
    if (sec >= 59.999)
        minute++;
    if (minute >= 60)
        deg++;

    /* Return the signed degree portion */
    deg *= sign;
    return (deg);
}


/******************************************************************************
MODULE:  find_min

PURPOSE:  Extracts the minute portion of the input angle.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
min        Minutes portion of the input angle

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/24/2014     Gail Schmidt     Original Development (based on input routines
                               from LEDAPS)

NOTES:
******************************************************************************/
int find_min
(
    double angle    /* I: angle in total degrees */
)
{
    int minute;          /* minutes portion of angle */
    double sec;          /* seconds portion of angle */

    /* Split angle into minutes and seconds */
    angle = fabs (angle);
    angle -= (int) angle;
    minute = (int) (angle * 60.0);
    sec = ((angle * 60.0) - minute) * 60.0;

    /* Keep seconds and minutes in a valid range */
    if (sec > 59.999)
        minute++;
    if (minute >= 60)
        minute -= 60;

    /* Return the minute portion */
    return (minute);
}


/******************************************************************************
MODULE:  find_sec

PURPOSE:  Extracts the seconds portion of the input angle.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
sec        Seconds portion of the input angle

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
1/24/2014     Gail Schmidt     Original Development (based on input routines
                               from LEDAPS)

NOTES:
******************************************************************************/
double find_sec
(
    double angle    /* I: angle in total degrees */
)
{
    int temp_angle;       /* temporary angle value */

    /* Split angle into seconds */
    angle = fabs (angle);
    angle -= (int) angle;
    angle *= 60.0;
    angle -= (int) angle;
    angle *= 60.0;

    /* Keep minutes in a valid range */
    if (angle > 59.999)
        angle -= 60.0;
    temp_angle = (int) (angle * 1000.0);    /* truncate to 0.001 sec */
    angle = temp_angle / 1000.0;

    /* Return the second portion */
    return (angle);
}
