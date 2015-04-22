/* Standard Library Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* IAS Library Includes */
#include "ias_types.h"        
#include "ias_math.h"
#include "ias_lw_geo.h"          
#include "ias_logging.h" 
#include "ias_const.h"
#include "gctp.h"
#include "config.h"

/* Local Defines */
#define GRID_SIZE_HORZ 20
#define GRID_SIZE_VERT 20
#define ALL_BITS_SET 255
#define NO_BITS_SET 0

#ifndef HAVE_LITTLE_ENDIAN
#error("This code does not properly support big endian")
#endif

/*****************************************************************************
NAME:  convert_target_xy_to_input_line_sample

PURPOSE:  Converts an X/Y coordinate to input line/sample and confirms it falls
    within the image.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
TRUE     Pixel in mask
FALSE    Pixel not in mask
ERROR    Operation failed

*****************************************************************************/
static int convert_target_xy_to_input_line_sample
(
    const IAS_DBL_XY *current_pixel,   /* I: Pixel X/Y coordinate */
    IAS_GEO_PROJ_TRANSFORMATION *geographic_transformation,/* I: Transformation
                                                                 Projection */
    double min_lng,             /* I: Minimum image longitude */
    double max_lat,             /* I: Maximum image latitude */
    double delta_longitude,     /* I: Change in longitude from bounding box */
    double delta_latitude,      /* I: Change in latatiude from bounding box */
    int num_samples,            /* I: Number of samples in input image */
    int num_lines,              /* I: Number of lines in input image */
    IAS_DBL_LS *translated_pixel/* O: Translated to bit mask line/sample */
)
{
    IAS_DBL_LAT_LONG transformed_pixel; /* Pixel transformed to lat/long */
   
    /* Transform the X/Y coordinates to Lat/Long */
    if (ias_geo_transform_coordinate(geographic_transformation, 
        current_pixel->x, current_pixel->y, &transformed_pixel.lng, 
        &transformed_pixel.lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Converting to lat/long");
        return ERROR;
    }
            
    /* Translate lat/long to mask line/sample */  
    translated_pixel->samp = (transformed_pixel.lng - min_lng) 
        / delta_longitude;
    translated_pixel->line = (max_lat - transformed_pixel.lat) / delta_latitude;
            
    /* Check if the line sample falls within the image */
    if (translated_pixel->line >= 0 && translated_pixel->line < num_lines 
        && translated_pixel->samp >= 0 && translated_pixel->samp < num_samples)
    {
        return TRUE;
    } 

    return FALSE;
}

/*****************************************************************************
NAME:  ias_geo_shape_mask

PURPOSE:  Generate a mask image (per-bit buffer) based on a set of polygons.
          Values of zero denote locations outside the polygons, values of one
          represent locations inside a polygon.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
int ias_geo_shape_mask
(
    const char *polygon_file,   /* I: Polygon filename */
    unsigned int num_lines,     /* I: Number of lines in mask */
    unsigned int num_samples,   /* I: Number of samples in mask */
    double upper_left_lat,      /* I: Upper left latitude for mask */
    double lower_right_lat,     /* I: Lower right latitude for mask */
    double upper_left_long,     /* I: Upper left longitude for mask */
    double lower_right_long,    /* I: Lower right longitude for mask */
    unsigned char *mask         /* O: Mask buffer */
)
{
    unsigned int line;          /* Line counter */
    unsigned int index;         /* Generic counter */
    double delta_latitude;      /* Delta latitude */
    double delta_longitude;     /* Delta longitude */
    IAS_POLYGON_LINKED_LIST *polygon_list; /* Polygon linked list pointer */
    FILE *fp;                   /* Polygon file pointer */

    /* Open the polygon file. */
    if ((fp = fopen(polygon_file, "r")) == NULL)
    {
        IAS_LOG_ERROR("Unable to open %s for reading.", polygon_file);
        return ERROR;
    }

    /* Load the polygons. */
    if (ias_geo_load_polygon(fp, upper_left_long, lower_right_long,
        lower_right_lat, upper_left_lat, &polygon_list) != SUCCESS)
    {
        IAS_LOG_ERROR("Loading the polygon file %s", polygon_file);
        fclose(fp);
        return ERROR;
    }

    /* Close the polygon file. */
    fclose(fp);

    /* Discard polygons outside the bounding box. */
    if (ias_geo_reduce_polygon(&polygon_list, upper_left_long, lower_right_long,
        upper_left_lat, lower_right_lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Reducing the polygon");
        return ERROR;
    }

    /* Initialize the mask to all zeros. */
    memset(mask, 0, num_lines * num_samples / 8 + 1);

    /* Determine the mask value for each sample location. */
    delta_latitude = (upper_left_lat - lower_right_lat) / num_lines;
    if (lower_right_long >= upper_left_long)
    {
        delta_longitude = (lower_right_long - upper_left_long) / num_samples;
    }
    else
    {
        delta_longitude = (lower_right_long - upper_left_long + 360) 
            / num_samples;
    }

    /* Loop through each line */
    for (line = 0, index = 0; line < num_lines; line++)
    {
        unsigned int sample;        /* Sample counter */
        double latitude;            /* Latitude */

        latitude = upper_left_lat - delta_latitude * line;

        /* Loop through each sample */
        for (sample = 0; sample < num_samples; sample++, index++)
        {           
            IAS_POLYGON_LINKED_LIST *polygon_hit;  /* Polygon linked list 
                                                      pointer */
            double longitude;           /* Longitude */
            double distance;            /* Distance from point to polygon */
            int inside_flag;            /* Inside/Outside polygon flag */

            longitude = upper_left_long + delta_longitude * sample;

            /* Adjust for 180 crossing. */
            if (longitude >= 180)
            {
                longitude -= 360;
            }

            /* Initialize the flag and distances */
            inside_flag = 0;
            distance = 1e10;

            /* Determine if point is inside the shape */
            inside_flag = ias_geo_point_in_shape_distance(polygon_list,
                latitude, longitude, 0, &distance, &polygon_hit);
            
            /* Progress down the line using the distance provided by 
               point_in_shape_distance so we don't have to recalculate
               distance for each lat/long */
            while (sample < num_samples && distance > 0)
            {
                /* Set bit if point is inside the polygon */
                if (inside_flag)
                {
                    unsigned int byte;  /* Byte level indexing */
                    unsigned int bit;   /* Bit-level indexing */
                    byte = index / 8;
                    bit = 7 - index % 8;
                    mask[byte] |= 1 << bit;
                }
    
                /* Progress to the next sample and to the next mask
                   index */
                sample++;
                index++;

                /* Subtract the distance that we change each sample */
                distance -= delta_longitude;
            }

            sample--;
            index--;
        } /* longitude loop */
    } /* latitude loop */
    
    /* Free storage. */
    ias_geo_free_polygon_linked_list(polygon_list);

    return SUCCESS;
}

/*****************************************************************************
NAME:  ias_geo_shape_mask_projection

PURPOSE:  Generate a shape mask for a given region in a given projection.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

NOTES: Mask should already be initialized when passed to the routine. It should 
       be initialized with all zeros.
*****************************************************************************/
int ias_geo_shape_mask_projection
(
    const char *polygon_file,         /* I: Polygon filename */
    const IAS_IMAGE *image,           /* I: Input image struct pointer */
    const IAS_PROJECTION *projection, /* I: Input projection struct pointer */
    unsigned char *mask               /* O: Mask buffer */
)
{
    IAS_DBL_LAT_LONG corners[4];    /* Lat/Long corners: UL, UR, LL, LR */
    const IAS_CORNERS *corners_ptr; /* Image corners  */
    double lng[4];                  /* Corner longitudes */
    unsigned int min_lat = 0;       /* Minimum latitude */
    unsigned int max_lat = 0;       /* Maximum latitude */
    unsigned int min_lng = 0;       /* Minimum longitude */
    unsigned int max_lng = 0;       /* Maximum longitude */
    double delta_latitude;          /* Delta latitude */
    double delta_longitude;         /* Delta longitude */
    unsigned char *bit_mask = NULL; /* Bit mask */
    int num_horz_grids;             /* Number of horizontal grids for image */
    int num_vert_grids;             /* Number of vertical grids for image */
    int vgrid;                      /* Loop variable for current vert grid */
    int hgrid;                      /* Loop variable for current horz grid */
    unsigned int num_lines;         /* Number of lines in passed image */
    unsigned int num_samples;       /* Number of samples in passed image */
    unsigned int line;              /* Loop variable for lines in image */
    unsigned int sample;            /* Loop variable for samples in image */
    unsigned int index;             /* Loop variable for generic use */
    double oparm[IAS_PROJ_PARAM_SIZE];/* Output projection parameters */
    IAS_PROJECTION geographic_projection; /* Geographic projection struct */
    IAS_GEO_PROJ_TRANSFORMATION *geographic_transformation; /* Transformation
                                                               struct */ 

    /* Set up pointer to image members & grid values */
    corners_ptr = &image->corners;
    num_lines = image->nl;
    num_samples = image->ns;
    num_horz_grids = floor(num_samples / GRID_SIZE_HORZ);
    num_vert_grids = floor(num_lines / GRID_SIZE_VERT);

    /* Initalize output parameters to 0.0 */
    for (index = 0; index < IAS_PROJ_PARAM_SIZE; index++)
    {
        oparm[index] = 0.0;
    }

    /* Set up target projection */
    ias_geo_set_projection(GEO, NULLZONE, DEGREE, WGS84_SPHEROID, oparm,
        &geographic_projection);

    /* Create the transfomation */
    geographic_transformation = ias_geo_create_proj_transformation(projection, 
        &geographic_projection);
    if (!geographic_transformation)
    {
        IAS_LOG_ERROR("Creating projection transformation");
        return ERROR;
    }

    /* Convert the corner coordinates to lat/long. */
    if (ias_geo_transform_coordinate(geographic_transformation, 
            corners_ptr->upleft.x, corners_ptr->upleft.y, &corners[0].lng, 
            &corners[0].lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Error converting upper left projection parameters to "
                "lat/long.");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        return ERROR;
    }

    if (ias_geo_transform_coordinate(geographic_transformation, 
            corners_ptr->upright.x, corners_ptr->upright.y, &corners[1].lng, 
            &corners[1].lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Error converting upper right projection parameters to "
                "lat/long.");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        return ERROR;
    }

    if (ias_geo_transform_coordinate(geographic_transformation,
            corners_ptr->loleft.x, corners_ptr->loleft.y, &corners[2].lng, 
            &corners[2].lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Error converting lower left projection parameters to "
                "lat/long.");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        return ERROR;
    }

    if (ias_geo_transform_coordinate(geographic_transformation, 
            corners_ptr->loright.x, corners_ptr->loright.y, &corners[3].lng, 
            &corners[3].lat) != SUCCESS)
    {
        IAS_LOG_ERROR("Error converting lower right projection parameters to "
                "lat/long.");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        return ERROR;
    }

    /* Check whether we are crossing the dateline.  Adjust the corner
       longitudes, if necessary. */
    for (index = 0; index < 4; index++)
    {
        lng[index] = corners[index].lng;
    }
    
    if (ias_geo_does_cross_180(DEGREE, lng))
    {
        for (index = 0; index < 4; index++)
        {
            if (lng[index] < 0)
            {
                ias_geo_add_once_around(DEGREE, &lng[index]); 
            }
        }
    }
        
    /* Find the min/max bounding box. */
    for (index = 1; index < 4; index++)
    {
        if (corners[min_lat].lat > corners[index].lat)
        {
            min_lat = index;
        }
        else if (corners[max_lat].lat < corners[index].lat)
        {
            max_lat = index;
        }

        if (lng[min_lng] > lng[index])
        {    
            min_lng = index;
        }
        else if (lng[max_lng] < lng[index])
        {    
            max_lng = index;
        }
    }
    
    /* Allocate memory for the bit_mask */
    bit_mask = calloc(1, num_lines * num_samples / 8 + 1);
    if (!bit_mask)
    {
        IAS_LOG_ERROR("Allocating memory for the bit mask");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        return ERROR;
    }
    
    /* Creating the shapemask */
    if (ias_geo_shape_mask(polygon_file, num_lines, num_samples, 
        corners[max_lat].lat, corners[min_lat].lat, lng[min_lng],
        lng[max_lng], bit_mask) != SUCCESS)
    {
        IAS_LOG_ERROR("Creating the shape mask");
        ias_geo_destroy_proj_transformation(geographic_transformation);
        free(bit_mask);
        return ERROR;
    }
    
    /* Determine the delta latitude/longitude */
    delta_latitude = (corners[max_lat].lat - corners[min_lat].lat) 
        / num_lines;
    delta_longitude = (lng[max_lng] - lng[min_lng]) / num_samples;
    
    /* Loop through the grids */
    for (vgrid = 0; vgrid <= num_vert_grids; vgrid++)
    {
        int grid_lines = GRID_SIZE_VERT; /* Number of lines in grid */

        /* If it is the end of the image determine smaller grid */
        if (vgrid == num_vert_grids)
        {
            grid_lines = num_lines % GRID_SIZE_VERT;
            if (grid_lines == 0)
            {   
                continue;
            }
        }

        for (hgrid = 0; hgrid <= num_horz_grids; hgrid++)
        {
            IAS_DBL_LS translated_pixel[4];     /* Translated  line/samp */ 
            IAS_DBL_XY grid_corners[4];         /* UL LL UR LR */
            int grid_value = -1;                /* Grid match value */
            int bad_grid = 0;                   /* Boolean for bad grid check */
            int grid_samples = GRID_SIZE_HORZ;  /* Number of samples in grid */

            /* If it is the end of the image determine smaller grid */
            if (hgrid == num_horz_grids)
            {
                grid_samples = num_samples % GRID_SIZE_HORZ;
                if (grid_samples == 0)
                {   
                    continue;
                }
            }

            /* Determine corners for current grid square */
            grid_corners[0].y = corners_ptr->upleft.y - (GRID_SIZE_VERT 
                * vgrid * image->pixel_size_y);
            grid_corners[0].x = (GRID_SIZE_HORZ * hgrid 
                * image->pixel_size_x) + corners_ptr->upleft.x;

            grid_corners[1].y = grid_corners[0].y - (grid_lines
                * image->pixel_size_y);
            grid_corners[1].x = grid_corners[0].x;

            grid_corners[2].y = grid_corners[0].y;
            grid_corners[2].x = grid_corners[0].x + (grid_samples
                * image->pixel_size_x);

            grid_corners[3].y = grid_corners[1].y;
            grid_corners[3].x = grid_corners[2].x;
            
            /* Transform the grid corners to bit mask line/sample */
            for (index = 0; index < 4; index ++)
            {
                int status; /* Status placeholder */

                status = convert_target_xy_to_input_line_sample(
                    &grid_corners[index], geographic_transformation, 
                    lng[min_lng], corners[max_lat].lat, 
                    delta_longitude, delta_latitude, num_samples, 
                    num_lines, &translated_pixel[index]);
                if (status == ERROR)
                {
                    IAS_LOG_ERROR("Translating grid corners for grid line %d"
                        " sample %d ", vgrid * GRID_SIZE_VERT, hgrid 
                        * GRID_SIZE_HORZ);
                    free(bit_mask);
                    ias_geo_destroy_proj_transformation(
                        geographic_transformation);
                    return ERROR;
                }
                else if (!status)
                {
                    bad_grid = 1;
                }
            }

            /* If all corners are in bit_mask check bit_mask grid */
            if (!bad_grid)
            {
                int min_line = 0;   /* Max line index in bit_mask */
                int max_line = 0;   /* Min line index in bit_mask */
                int min_samp = 0;   /* Min sample index in bit_mask */
                int max_samp = 0;   /* Max sample index in bit_mask */
                IAS_LNG_LS max_ls;  /* Maximum line/sample */
                IAS_LNG_LS min_ls;  /* Minimum line/sample */

                /* Creating bounding box around bit_mask grid */
                for (index = 1; index < 4; index++)
                {
                    if (translated_pixel[min_line].line
                        > translated_pixel[index].line)
                    {
                        min_line = index;
                    }
                    else if (translated_pixel[max_line].line 
                             < translated_pixel[index].line)
                    {
                        max_line = index;
                    }   

                    if (translated_pixel[min_samp].samp 
                        > translated_pixel[index].samp)
                    {    
                        min_samp = index;
                    }

                    else if (translated_pixel[max_samp].samp 
                             < translated_pixel[index].samp)
                    {    
                        max_samp = index;
                    }
                }

                max_ls.line = translated_pixel[max_line].line + 1;
                max_ls.samp = translated_pixel[max_samp].samp + 1;
                min_ls.line = translated_pixel[min_line].line;
                min_ls.samp = translated_pixel[min_samp].samp;
 
                /* Make sure the max_ls is still in the image */
                if (max_ls.line >= num_lines || max_ls.samp >= num_samples)
                {
                    bad_grid = 1;
                }
                else
                {
                    /* Get the bounding box check value */
                    grid_value = bit_mask[(min_ls.line * num_samples 
                        + min_ls.samp) / 8];
                    if (grid_value != ALL_BITS_SET && grid_value != NO_BITS_SET)
                    {
                        bad_grid = 1;
                    }
                }

                if (!bad_grid)
                {
                    /* Check that all the values in the bounding box are 
                       identical*/
                    for (line = min_ls.line; line < max_ls.line; line++)
                    {
                        for (sample = min_ls.samp; sample < max_ls.samp; 
                             sample += 8)
                        {
                            int grid_index = (line * num_samples + sample) / 8;
                            if (bit_mask[grid_index] != grid_value)
                            {
                                bad_grid = 1;
                                break;  
                            }
                        }

                        if (bad_grid)
                        {
                            break;
                        }
                    }
                }
            }
         
            /* Grid is either all set bits or all empty bits */
            if (!bad_grid)
            {
                if (grid_value == NO_BITS_SET)
                {
                    continue;
                }

                for (line = GRID_SIZE_VERT * vgrid; line < GRID_SIZE_VERT 
                 * vgrid + grid_lines; line++)
                {    
                    for (sample = GRID_SIZE_HORZ * hgrid; sample 
                        < GRID_SIZE_HORZ * hgrid + grid_samples; sample++)
                    {
                        index = line * num_samples + sample;
                        mask[index] = IAS_GEO_SHAPE_MASK_VALID;
                    }
                }

                continue;
            }

            /* Loop through image converting each pixel to lat/long */
            for (line = GRID_SIZE_VERT * vgrid; line < GRID_SIZE_VERT 
                 * vgrid + grid_lines; line++)
            {    
                IAS_DBL_XY current_pixel;/* Current pixel in image using x/y */

                /* Calculate the Y coordinate */
                current_pixel.y = corners_ptr->upleft.y - (line 
                    * image->pixel_size_y);
    
                for (sample = GRID_SIZE_HORZ * hgrid; sample < GRID_SIZE_HORZ
                     * hgrid + grid_samples; sample++)
                {
                    int status; /* Status placeholder */
                    IAS_DBL_LS translated_pixel; /* Translated to line/samp */

                    /* Calculate the X Coordinate */
                    current_pixel.x = (sample * image->pixel_size_x) 
                        + corners_ptr->upleft.x;

                    /* Check if pixel is part of bit mask */
                    status = convert_target_xy_to_input_line_sample(
                        &current_pixel, geographic_transformation, 
                        lng[min_lng], corners[max_lat].lat, 
                        delta_longitude, delta_latitude, num_samples, 
                        num_lines, &translated_pixel);
                    if (status == ERROR)
                    {
                        IAS_LOG_ERROR("Translating pixel for line %d sample %d",
                            line, sample);
                        free(bit_mask);
                        ias_geo_destroy_proj_transformation(
                            geographic_transformation);
                        return ERROR;
                    }
                    else if (status) 
                    {
                        unsigned int byte; /* Byte level indexing */
                        unsigned int bit;  /* Bit level indexing */
                        int mask_index;
                        int nearest_line = round(translated_pixel.line);
                        int nearest_sample = round(translated_pixel.samp);

                        /* Clamp the line to the image after rounding up might
                           go off the edge */
                        if (nearest_line >= num_lines)
                            nearest_line = num_lines - 1;
                        if (nearest_sample >= num_samples)
                            nearest_sample = num_samples - 1;

                        mask_index = nearest_line * num_samples 
                            + nearest_sample;
                        byte = mask_index / 8;
                        bit = 7 - mask_index % 8;
                        index = line * num_samples + sample;
                        if (bit_mask[byte] & (1 << bit))
                        {
                            mask[index] = IAS_GEO_SHAPE_MASK_VALID;
                        }
                    } 
                }
            } 
        }
    }

    /* Free memory */
    free(bit_mask);
    ias_geo_destroy_proj_transformation(geographic_transformation);

    return SUCCESS;
}

/*****************************************************************************
NAME:  ias_geo_point_in_shape

PURPOSE:  Determine whether a given point is within a set of polygons.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
FALSE    Point outside the polygons
TRUE     Point inside a polygon
ERROR    Error

*****************************************************************************/
int ias_geo_point_in_shape
(
    IAS_POLYGON_LINKED_LIST *polygon_list,  /* I: Polygon list */
    double latitude,                        /* I: Point latitude (degrees) */
    double longitude                        /* I: Point longitude (degrees) */
)
{
    IAS_POLYGON_LINKED_LIST *polygon;       /* Polygon linked list pointer */
    int status;                             /* Return status */

    polygon = polygon_list;
    while (polygon)
    {
        /* Check the polygon bounding box. */
        if (polygon->min_y > latitude || polygon->max_y < latitude 
            || polygon->min_x > longitude || polygon->max_x < longitude)
        {
            polygon = polygon->next;
            continue;
        }
                
        /* Determine whether the point is inside or outside
           the polygon. */
        if (ias_math_point_in_closed_polygon(polygon->num_points - 1, 
                polygon->point_x, polygon->point_y, longitude, latitude, 
                polygon->num_segs, polygon->poly_seg))
        {
            /* If there are polygons within this one and we're inside a child
               polygon, then our point is considered to be outside the parent
               polygon. */
            if (polygon->child)
            {
                status = ias_geo_point_in_shape(polygon->child, latitude, 
                    longitude);
                if (status == ERROR)
                {
                    return ERROR;
                }
                else
                {
                    return !status;
                }
            }
            else
            {
                return TRUE;
            }
        }

        /* Point to next polygon. */
        polygon = polygon->next;
    }

    /* Not inside any polygons. */
    return FALSE;
}

/*****************************************************************************
NAME:  ias_geo_point_in_shape_distance

PURPOSE:  Determine whether a given point is within a set of polygons, and
          find the distance (in the latitude or longitude direction) to the
          nearest polygon boundary.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
FALSE    Point outside the polygons
TRUE     Point inside a polygon
ERROR    Unable to compute
*****************************************************************************/
int ias_geo_point_in_shape_distance
(
    IAS_POLYGON_LINKED_LIST *polygon_list,/* I: Polygon list */
    double latitude,        /* I: Point latitude (degrees) */
    double longitude,       /* I: Point longitude (degrees) */
    unsigned int direction, /* I: Direction to measure distance: 0 = x, 1 = y */
    double *distance,       /* I/O: Distance from point to polygon boundary in 
                                    specified direction */
    IAS_POLYGON_LINKED_LIST **polygon_hit /* O: Polygon hit */
)
{
    IAS_POLYGON_LINKED_LIST *polygon;   /* Polygon linked list pointer */
    IAS_POLYGON_LINKED_LIST *child_hit; /* Child polygon hit */
    double hit_distance;                /* Distance from point to polygon */
    int inside;                         /* Inside/outside polygon flag */

    polygon = polygon_list;
    while (polygon)
    {
        /* Check the bounding box, and only consider polygons with
           bounding boxes within the current minimum distance. */
        if (polygon->min_y > latitude || polygon->max_y < latitude 
            || polygon->max_x < longitude || polygon->min_x > longitude 
            + *distance)
        {
            polygon = polygon->next;
            continue;
        }
                
        /* Determine whether the point is inside or outside the polygon. */
        inside = ias_math_point_in_closed_polygon_distance(polygon->num_points 
            - 1, polygon->point_x, polygon->point_y, longitude, latitude,
            polygon->num_segs, polygon->poly_seg, direction, &hit_distance);
        if (inside == ERROR)
        {
            IAS_LOG_ERROR("Checking point in polygon distance ");
            return ERROR;
        }

        if (inside)
        {
            *polygon_hit = polygon;
            *distance = hit_distance;
            
            /* If there are polygons within this one and we're inside a child
               polygon, then our point is considered to be outside the parent
               polygon. */
            if (polygon->child)
            {
                inside = ias_geo_point_in_shape_distance(polygon->child, 
                    latitude, longitude, direction, &hit_distance, &child_hit);
                if (inside == ERROR)
                {
                    IAS_LOG_ERROR("Computing the point in shape distance");
                    return ERROR;
                }
                
                /* No error check the distance */
                if (inside || (hit_distance > 0 && hit_distance < *distance))
                {
                    *polygon_hit = child_hit;
                    *distance = hit_distance;

                    if (inside)
                    {
                        return FALSE;
                    }
                }
            }
            
            return TRUE;
        }
        else if (hit_distance > 0 && hit_distance < *distance)
        {
            *polygon_hit = polygon;
            *distance = hit_distance;
        }

        /* Point to next polygon. */
        polygon = polygon->next;
    }
    
    return FALSE;
}
