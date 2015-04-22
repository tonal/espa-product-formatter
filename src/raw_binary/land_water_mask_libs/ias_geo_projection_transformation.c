/*****************************************************************************
Name: ias_geo_projection_transformation

Purpose: Provides a wrapper around GCTP.

Notes:
    - It might be useful add a routine to print out transformation info by
      echoing through to the gctp_print_transformation_info routine

*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "gctp.h"
#include "ias_logging.h"
#include "ias_lw_geo.h"
#include "ias_const.h"

/* Declare a private structure to store information about a projection
   transformation. */
struct ias_geo_proj_transformation
{
    GCTP_TRANSFORMATION *gctp_transform;/* Pointer to the GCTP transformation */
    int source_is_som;          /* Flag to indicate the source projection is
                                   SOM so the needed coordinate swapping can
                                   be performed */
    int target_is_som;          /* Flag to indicate the target projection is
                                   SOM so the needed coordinate swapping can
                                   be performed */
    int source_is_dms;          /* Flag to indicate the source units are DMS
                                   so they can be converted to degrees */
    int target_is_dms;          /* Flag to indicate the target units are DMS
                                   so they can be converted to degrees */
};

/*****************************************************************************
Name: copy_ias_proj_to_gctp_proj

Purpose: A helper routine to copy an IAS projection definition to a GCTP
    projection definition.

Returns: nothing

*****************************************************************************/
static void copy_ias_proj_to_gctp_proj
(
    const IAS_PROJECTION *ias_proj, /* I: source IAS projection */
    GCTP_PROJECTION *gctp_proj      /* O: target GCTP projection */
)
{
    int i;

    /* Copy the projection code, zone, units, and spheroid to the GCTP proj */
    gctp_proj->proj_code = ias_proj->proj_code;
    gctp_proj->zone = ias_proj->zone;
    gctp_proj->units = ias_proj->units;
    gctp_proj->spheroid = ias_proj->spheroid;

    /* Copy the projection parameters */
    for (i = 0; i < IAS_PROJ_PARAM_SIZE; i++)
        gctp_proj->parameters[i] = ias_proj->parameters[i];

}

/*****************************************************************************
Name: ias_geo_set_projection

Purpose: A routine to set the members into an IAS projection.

Returns: nothing

*****************************************************************************/
void ias_geo_set_projection
(
    int proj_code,          /* I: input projection code */
    int zone,               /* I: input zone */
    int units,              /* I: input units */
    int spheroid,           /* I: input spheroid */
    const double *parms,    /* I: input projection parameters */
    IAS_PROJECTION *proj    /* I: target projection structure */
)
{
    int i;

    /* Set the projection code, zone, units, and spheroid to the GCTP proj */
    proj->proj_code = proj_code;
    proj->zone = zone;
    proj->units = units;
    proj->spheroid = spheroid;

    /* Set the projection parameters */
    for (i = 0; i < IAS_PROJ_PARAM_SIZE; i++)
        proj->parameters[i] = parms[i];

}

/*****************************************************************************
Name: gctp_message_callback

Purpose: Handles the error messages from GCTP.  It does some simple translation
    of the message type and calls the IAS library logging routine.  This allows
    GCTP error messages to be formatted the same as any other message.

Returns: nothing

*****************************************************************************/
static void gctp_message_callback
(
    GCTP_MESSAGE_TYPE_ENUM message_type, /* I: message type */
    const char *filename,     /* I: source code file name for input */
    int line_number,          /* I: source code line number for input */
    const char *format, ...   /* I: format string for message */
)
{
    int ias_message_type;
    va_list ap;

    /* Convert the message type */
    if (message_type == GCTP_INFO_MESSAGE)
        ias_message_type = IAS_LOG_LEVEL_INFO;
    else if (message_type == GCTP_ERROR_MESSAGE)
        ias_message_type = IAS_LOG_LEVEL_ERROR;
    else
    {
        /* This shouldn't happen, but if it does, call it a warning */
        ias_message_type = IAS_LOG_LEVEL_WARN;
    }

    /* Call our log message routine */
    va_start(ap, format); 
    ias_log_message(ias_message_type, filename, line_number, format, ap);
    va_end(ap);
}

/*****************************************************************************
Name: ias_geo_create_proj_transformation

Purpose: Creates a projection transformation and returns it to the caller.

Returns: A pointer to the created projection or NULL if there is an error.

*****************************************************************************/
IAS_GEO_PROJ_TRANSFORMATION *ias_geo_create_proj_transformation
(
    const IAS_PROJECTION *source_projection, /* I: source projection */
    const IAS_PROJECTION *target_projection /* I: target projection */
)
{
    IAS_GEO_PROJ_TRANSFORMATION *trans; /* created projection */
    GCTP_TRANSFORMATION *gctp_trans;    /* associated GCTP projection */
    GCTP_PROJECTION source_proj;        /* source GCTP projection */
    GCTP_PROJECTION target_proj;        /* target GCTP projection */

    /* Redirect gctp output to our local callback so we can control the 
       formatting.  Note that this will be set for every projection created,
       but it doesn't really matter since it will always be set to the same
       routine. */
    gctp_set_message_callback(gctp_message_callback);

    /* Allocate space for the local transformation structure */
    trans = malloc(sizeof(*trans));
    if (!trans)
    {
        IAS_LOG_ERROR("Failed allocating memory for a projection "
            "transformation");
        return NULL;
    }

    /* Copy the source IAS projection to the GCTP version of a projection */
    copy_ias_proj_to_gctp_proj(source_projection, &source_proj);
    if (source_proj.units == DMS)
    {
        /* The IAS layer will translate DMS units to degrees, so set the
           GCTP source projection to DEGREE units and remember the source is
           in DMS */
        source_proj.units = DEGREE;
        trans->source_is_dms = 1;
    }
    else
        trans->source_is_dms = 0;

    /* Copy the target IAS projection to the GCTP version of a projection */
    copy_ias_proj_to_gctp_proj(target_projection, &target_proj);
    if (target_proj.units == DMS)
    {
        /* The IAS layer will translate DMS units to degrees, so set the
           GCTP target projection to DEGREE units and remember the target needs
           to be converted to DMS */
        target_proj.units = DEGREE;
        trans->target_is_dms = 1;
    }
    else
        trans->target_is_dms = 0;

    /* Set the flags for whether the SOM projection coordinate swapping is 
       required */
    if (source_proj.proj_code == SOM)
        trans->source_is_som = 1;
    else
        trans->source_is_som = 0;
    if (target_proj.proj_code == SOM)
        trans->target_is_som = 1;
    else
        trans->target_is_som = 0;

    /* Create the GCTP transformation */
    gctp_trans = gctp_create_transformation(&source_proj, &target_proj);
    if (!gctp_trans)
    {
        IAS_LOG_ERROR("Failed allocating memory for a GCTP projection "
            "transformation");
        free(trans);
        return NULL;
    }
    trans->gctp_transform = gctp_trans;

    return trans;
}

/*****************************************************************************
Name: ias_geo_destroy_proj_transformation

Purpose: Releases the resources allocated by the creation of a projection
    transformation

Returns: nothing

*****************************************************************************/
void ias_geo_destroy_proj_transformation
(
    IAS_GEO_PROJ_TRANSFORMATION *trans
)
{
    if (trans)
    {
        gctp_destroy_transformation(trans->gctp_transform);
        trans->gctp_transform = NULL;
        free(trans);
    }
}

/****************************************************************************
Name: ias_geo_only_allow_threadsafe_transforms

Purpose: Many of the transformations haven't been made threadsafe yet.  Until
    they are, there needs to be a way to allow threaded applications to
    indicate that they are threaded and therefore only threadsafe
    transformations should be allowed.  This routine should be called from any
    application that uses threading.  After calling this, non-threadsafe
    projections will cause the transformation creation to return an error.

Returns:
    nothing

****************************************************************************/
void ias_geo_only_allow_threadsafe_transforms()
{
    gctp_only_allow_threadsafe_transforms();
}

/*****************************************************************************
Name: ias_geo_transform_coordinate

Purpose: Using a projection transformation, convert the input coordinates
    from the source projection to the target projection.

Returns: SUCCESS or ERROR

*****************************************************************************/
int ias_geo_transform_coordinate
(
    const IAS_GEO_PROJ_TRANSFORMATION *trans, /* I: transformation to use */
    double inx,             /* I: Input X projection coordinate */
    double iny,             /* I: Input Y projection coordinate */
    double *outx,           /* O: Output X projection coordinate */
    double *outy            /* O: Output Y projection coordinate */
)
{
    int status;          /* Status code from call to GCTP */
    double incoor[2];    /* Input coordinates */
    double outcoor[2];   /* Output coordinates */

    /* Verify the transformation provided is valid */
    if (trans == NULL)
    {
        IAS_LOG_ERROR("Invalid transformation provided");
        return ERROR;
    }

    /* Swap X & Y if the source projection is SOM */
    if (trans->source_is_som)
    {
        incoor[0] = -iny;
        incoor[1] = inx;
    }
    else
    {
        /* Pack input coordinates into a GCTP compatible array */
        incoor[0] = inx;
        incoor[1] = iny;
    }

    /* Since the GCTP doesn't work with DMS units, convert to degrees if DMS 
       units are being used */
    if (trans->source_is_dms)
    {
        double coord;

        if (ias_geo_convert_dms2deg(incoor[0], &coord, "LON") != SUCCESS) 
        {
            IAS_LOG_ERROR("Failed converting input DMS longitude (%f) to "
                          "degrees", incoor[0]);
            return ERROR;
        }
        incoor[0] = coord;
        if (ias_geo_convert_dms2deg(incoor[1], &coord, "LAT") != SUCCESS)
        {
            IAS_LOG_ERROR("Failed converting input DMS latitude (%f) to "
                          "degrees", incoor[1]);
            return ERROR;
        }
        incoor[1] = coord;
    }

    /* Call the GCTP transformation routine */
    status = gctp_transform(trans->gctp_transform, incoor, outcoor);
    if (status != GCTP_SUCCESS)
    {
        if (status == GCTP_IN_BREAK)
        {
            /* We don't support any projections that can have break areas, so
               just include some rudimentary support for it, but consider it an
               error for now */
            IAS_LOG_ERROR("In projection break");
            return ERROR;
        }
        IAS_LOG_ERROR("Failed converting between coordinate systems in GCTP");
        return ERROR;
    }

    /* Unpack transformed coordinates */
    *outx = outcoor[0];
    *outy = outcoor[1];

    /* If target units are requested in DMS, do the conversion */
    if (trans->target_is_dms)
    {
        if (ias_geo_convert_deg2dms(outcoor[0], outx, "LON") != SUCCESS) 
        {
            IAS_LOG_ERROR("Failed converting output degrees longitude (%f) to "
                          "DMS", outcoor[0]);
            return ERROR;
        }
        if (ias_geo_convert_deg2dms(outcoor[1], outy, "LAT") != SUCCESS)
        {
            IAS_LOG_ERROR("Failed converting output degrees latitude (%f) to "
                          "DMS", outcoor[1]);
            return ERROR;
        }
    }

    /* If the target projection is SOM, swap the X & Y coordinates */
    if (trans->target_is_som)
    {
        double temp;         /* Temp storage variable */

        temp = *outx;
        *outx = *outy;
        *outy = -temp;
    }

    return SUCCESS;
}

