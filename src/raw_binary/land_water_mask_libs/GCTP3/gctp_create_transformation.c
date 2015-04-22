/****************************************************************************
Name: gctp_create_transformation

Purpose: Given input and output projections creates a projection
    transformation object (GTCP_TRANSFORMATION) that is used when calling
    the gctp_transform to do the actual transformation.

Note: The GCTP_TRANSFORMATION includes the input and output units specified.
    If the same transformation is needed with different input or output
    units it requires the creation of a different GCTP_TRANSFORMATION.

****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "cproj.h"
#include "gctp.h"
#include "local.h"

/* Define a type for a projection initialization function pointer. */
typedef int (*INIT_ROUTINE)(TRANSFORMATION *trans);

/* Define a lookup table for the forward transform init routines
   (lat/lon to x/y) */
static INIT_ROUTINE forward_init[GCTP_MAX_PROJ_CODE + 1] =
    {
        gctp_geo_init,         /* 0 = Geographic */
        gctp_utm_forward_init, /* 1 = Universal Transverse Mercator (UTM) */
        gctp_state_plane_forward_init, /* 2 = State Plane Coordinates */
        NULL,           /* 3 = Albers Conical Equal Area */
        gctp_lamcc_forward_init, /* 4 = Lambert Conformal Conic */
        NULL,           /* 5 = Mercator */
        gctp_ps_forward_init,  /* 6 = Polar Stereographic */
        gctp_poly_forward_init,/* 7 = Polyconic */
        NULL,           /* 8 = Equidistant Conic */
        gctp_tm_forward_init,  /* 9 = Transverse Mercator */
        NULL,           /* 10 = Stereographic */
        NULL,           /* 11 = Lambert Azimuthal Equal Area */
        NULL,           /* 12 = Azimuthal Equidistant */
        NULL,           /* 13 = Gnomonic */
        NULL,           /* 14 = Orthographic */
        NULL,           /* 15 = General Vertical Near-Side Perspective */
        NULL,           /* 16 = Sinusiodal */
        NULL,           /* 17 = Equirectangular */
        NULL,           /* 18 = Miller Cylindrical */
        NULL,           /* 19 = Van der Grinten */
        gctp_om_forward_init,  /* 20 = (Hotine) Oblique Mercator */
        NULL,           /* 21 = Robinson */
        gctp_som_forward_init, /* 22 = Space Oblique Mercator (SOM) */
        NULL,           /* 23 = Alaska Conformal */
        NULL,           /* 24 = Interrupted Goode Homolosine */
        NULL,           /* 25 = Mollweide */
        NULL,           /* 26 = Interrupted Mollweide */
        NULL,           /* 27 = Hammer */
        NULL,           /* 28 = Wagner IV */
        NULL,           /* 29 = Wagner VII */
        NULL,           /* 30 = Oblated Equal Area */
        NULL,           /* 31 = Integerized Sinusiodal */
    };

/* Define a lookup table for the inverse transform init routines
   (x/y to lat/lon) */
static INIT_ROUTINE inverse_init[GCTP_MAX_PROJ_CODE + 1] = 
    {
        gctp_geo_init,         /* 0 = Geographic */
        gctp_utm_inverse_init, /* 1 = Universal Transverse Mercator (UTM) */
        gctp_state_plane_inverse_init, /* 2 = State Plane Coordinates */
        NULL,           /* 3 = Albers Conical Equal Area */
        gctp_lamcc_inverse_init, /* 4 = Lambert Conformal Conic */
        NULL,           /* 5 = Mercator */
        gctp_ps_inverse_init,  /* 6 = Polar Stereographic */
        gctp_poly_inverse_init,/* 7 = Polyconic */
        NULL,           /* 8 = Equidistant Conic */
        gctp_tm_inverse_init,  /* 9 = Transverse Mercator */
        NULL,           /* 10 = Stereographic */
        NULL,           /* 11 = Lambert Azimuthal Equal Area */
        NULL,           /* 12 = Azimuthal Equidistant */
        NULL,           /* 13 = Gnomonic */
        NULL,           /* 14 = Orthographic */
        NULL,           /* 15 = General Vertical Near-Side Perspective */
        NULL,           /* 16 = Sinusiodal */
        NULL,           /* 17 = Equirectangular */
        NULL,           /* 18 = Miller Cylindrical */
        NULL,           /* 19 = Van der Grinten */
        gctp_om_inverse_init,  /* 20 = (Hotine) Oblique Mercator */
        NULL,           /* 21 = Robinson */
        gctp_som_inverse_init, /* 22 = Space Oblique Mercator (SOM) */
        NULL,           /* 23 = Alaska Conformal */
        NULL,           /* 24 = Interrupted Goode Homolosine */
        NULL,           /* 25 = Mollweide */
        NULL,           /* 26 = Interrupted Mollweide */
        NULL,           /* 27 = Hammer */
        NULL,           /* 28 = Wagner IV */
        NULL,           /* 29 = Wagner VII */
        NULL,           /* 30 = Oblated Equal Area */
        NULL,           /* 31 = Integerized Sinusiodal */
    };

/* Flag to only allow threadsafe transforms to be created.  This is "temporary"
   and can be removed when all the projection transformations have been updated
   to be threadsafe. */
static int only_threadsafe = 0;

/* Routine to get the conversion factor for converting between the input and
   output units. */
static int get_unit_conversion_factor
(
    int in_units,
    int out_units,
    double *factor
)
{
    /* Define an array of conversion factors between the in_units (first index
       in the array) and the out_units (second index of the array) */
    static const double factors[6][6] =
       {{1.0, 0.0, 0.0, 206264.8062470963, 57.295779513082323, 0.0},
        {0.0, 1.0, .3048006096012192, 0.0, 0.0, 1.000002000004},
        {0.0, 3.280833333333333, 1.0, 0.0, 0.0, 3.280839895013124},
        {.484813681109536e-5, 0.0, 0.0, 1.0, .27777777777778e-3, 0.0}, 
        {.01745329251994329, 0.0, 0.0, 3600, 1.0, 0.0},
        {0.0, .999998, .3048, 0.0, 0.0, 1.0}};

    if ((out_units >= 0) && (out_units <= MAXUNIT) && (in_units >= 0)
            && (in_units <= MAXUNIT))
    {
        *factor = factors[in_units][out_units];

        /* Angle units can not be converted to length units */
        if (*factor == 0.0)
        {
            GCTP_PRINT_ERROR("Incompatable unit codes: %d and %d",
                in_units, out_units);
            return GCTP_ERROR;
        }
    }
    else
    {
        GCTP_PRINT_ERROR("Illegal source or target unit code: %d and %d",
            in_units, out_units);
        return GCTP_ERROR;
    }

    return GCTP_SUCCESS;
}

/****************************************************************************
Name: gctp_create_transformation

Purpose: Given input and output projections creates a projection
    transformation.

Returns:
    A pointer to the transformation created, or NULL if there is an error.

Notes:
    - gctp_destroy_transformation should be called for transformations that
      are created to free any resources allocated by the creation process.

****************************************************************************/
GCTP_TRANSFORMATION *gctp_create_transformation
(
    const GCTP_PROJECTION *input_projection,
    const GCTP_PROJECTION *output_projection        
)
{
    GCTP_TRANSFORMATION *trans;
    INIT_ROUTINE inverse_init_func;
    INIT_ROUTINE forward_init_func;
    int units;

    /* Verify the proj codes are valid */
    if (input_projection->proj_code < 0
        || input_projection->proj_code > GCTP_MAX_PROJ_CODE)
    {
        GCTP_PRINT_ERROR("Invalid input projection code: %d",
               input_projection->proj_code);
        return NULL;
    }

    if (output_projection->proj_code < 0
        || output_projection->proj_code > GCTP_MAX_PROJ_CODE)
    {
        GCTP_PRINT_ERROR("Invalid output projection code: %d",
               input_projection->proj_code);
        return NULL;
    }

    /* Allocate memory for the transformation object */
    trans = malloc(sizeof(*trans));
    if (!trans)
    {
        GCTP_PRINT_ERROR("Error allocating memory for transformation object");
        return NULL;
    }

    /* Copy the projection information to the transformation object */
    trans->inverse.proj = *input_projection;
    trans->forward.proj = *output_projection;

    /* default to no forward or inverse transformation routines */
    trans->forward.transform = NULL;
    trans->forward.destroy = NULL;
    trans->forward.cache = NULL;
    trans->forward.print_info = NULL;

    trans->inverse.transform = NULL;
    trans->inverse.destroy = NULL;
    trans->inverse.cache = NULL;
    trans->inverse.print_info = NULL;

    /* Get the inverse and forward init routine pointers */
    inverse_init_func = inverse_init[input_projection->proj_code];
    forward_init_func = forward_init[output_projection->proj_code];

    /* If either of the init routines are not available, fall back to using
       GCTP for the transformations */
    /* TODO -  remove this when all transformations are supported with the
       new interface */
    trans->use_gctp = 0;
    if (!inverse_init_func || !forward_init_func)
    {
        /* If only threadsafe transforms are allowed, consider trying to use
           the old gctp interface an error since it isn't threadsafe */
        if (only_threadsafe)
        {
            GCTP_PRINT_ERROR("Error: trying to use a projection "
                "transformation that isn't threadsafe");
            free(trans);
            return NULL;
        }
        trans->use_gctp = 1;
        return trans;
    }

    /* Create the inverse transformation */
    if (inverse_init_func)
    {
        if (inverse_init_func(&trans->inverse) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error initializing inverse transformation");
            free(trans);
            return NULL;
        }
    }
    else
    {
        GCTP_PRINT_ERROR("Unsupported projection code: %d",
               input_projection->proj_code);
        free(trans);
        return NULL;
    }

    /* Create the forward transformation */
    if (forward_init_func)
    {
        if (forward_init_func(&trans->forward) != GCTP_SUCCESS)
        {
            GCTP_PRINT_ERROR("Error initializing forward transformation");
            gctp_destroy_transformation(trans);
            return NULL;
        }
    }
    else
    {
        GCTP_PRINT_ERROR("Unsupported projection code: %d",
               output_projection->proj_code);
        gctp_destroy_transformation(trans);
        return NULL;
    }

    /* find the factor unit conversions--all transformations are in radians
       or meters */
    if (input_projection->proj_code == GEO)
        units = RADIAN;
    else
        units = METER;
    if (get_unit_conversion_factor(input_projection->units, units,
            &trans->inverse.unit_conversion_factor) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error getting the input unit conversion factor");
        gctp_destroy_transformation(trans);
        return NULL;
    }

    if (output_projection->proj_code == GEO)
        units = RADIAN;
    else
        units = METER;
    if (get_unit_conversion_factor(units, output_projection->units,
            &trans->forward.unit_conversion_factor) != GCTP_SUCCESS)
    {
        GCTP_PRINT_ERROR("Error getting the output unit conversion factor");
        gctp_destroy_transformation(trans);
        return NULL;
    }

    /* Return the transformation */
    return trans;
}


/****************************************************************************
Name: gctp_destroy_transformation

Purpose: Releases the resources allocated when a transformation is created.
    After this is called, the transformation is no longer valid.

Returns:
    nothing

****************************************************************************/
void gctp_destroy_transformation
(
    GCTP_TRANSFORMATION *trans
)
{
    if (trans)
    {
        if (trans->forward.destroy)
            trans->forward.destroy(&trans->forward);
        if (trans->inverse.destroy)
            trans->inverse.destroy(&trans->inverse);
        free(trans->forward.cache);
        trans->forward.cache = NULL;
        free(trans->inverse.cache);
        trans->inverse.cache = NULL;
        free(trans);
    }
}

/****************************************************************************
Name: gctp_only_allow_threadsafe_transforms

Purpose: Many of the transformations haven't been made threadsafe yet.  Until
    they are, there needs to be a way to allow threaded applications to
    indicate that they are threaded and therefore only threadsafe
    transformations should be allowed.  This routine should be called from any
    application that uses threading.  After calling this, non-threadsafe
    projections will cause the transformation creation to return an error.

Returns:
    nothing

****************************************************************************/
void gctp_only_allow_threadsafe_transforms()
{
    only_threadsafe = 1;
}
