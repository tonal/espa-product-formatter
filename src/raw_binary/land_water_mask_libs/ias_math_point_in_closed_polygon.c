/* IAS Library Includes */
#include "ias_logging.h"
#include "ias_types.h"  
#include "ias_structures.h"
#include "ias_math.h"
#include "ias_const.h"

/*****************************************************************************
NAME:  ias_math_point_in_closed_polygon

PURPOSE: Determine if a point is located within the bounds of a polygon. 

RETURN VALUE:
Type = int
Value    Description
-----    -----------
TRUE        Point inside polygon
FALSE       Point not inside polygon (or unable to compute)
ERROR       Unable to compute

ALGORITHM:
    Based on W. Randolph Franklin's algorithm:
        http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    See also the Wikipedia page:
        http://eni.wikipedia.org/wiki/Point_in_polygon

Notes:  There shoud be one more point in vertex arrays than the number
        of sides. This method is used to close the polygon instead of wrapping 
        around to the first point of the polygon.

*****************************************************************************/
int ias_math_point_in_closed_polygon
(
    unsigned int num_sides,             /* I: Number of sides in polygon */
    const double *vert_x,               /* I: Vertices of polygon */
    const double *vert_y,               /* I: Vertices of polygon */
    double point_x,                     /* I: X coordinate of point */
    double point_y,                     /* I: Y coordinate of point */
    unsigned int num_segs,              /* I: Number of polygon segments */
    const IAS_POLYGON_SEGMENT *poly_seg /* I: Array of polygon segments */
)
{
    unsigned int point;         /* Point loop counter */
    unsigned int segment;       /* Segment loop counter */
    int intflag = 0;            /* Flag denoting even (0) or odd (1) 
                                    number of polygon side intersections */

    if (num_sides < 3) 
    {
        IAS_LOG_ERROR("Need at least three sides for polygon.");
        return ERROR;
    }

    /* If polygon segments have been specified, make use of them. */
    if (num_segs > 0)
    {
        for (segment = 0; segment < num_segs; segment++)
        {
            if (poly_seg[segment].min_x > point_x || poly_seg[segment].max_x 
                < point_x || poly_seg[segment].max_y < point_y)
            {
                continue;
            }
            /* Loop through the points in this segment checking the
               number of intersections */
            for (point = poly_seg[segment].first_point; 
                 point < poly_seg[segment].last_point; point++)
            {
                if (((vert_x[point] > point_x) != (vert_x[point + 1] > point_x))
                    && (point_y < (vert_y[point + 1] - vert_y[point]) 
                    * (point_x - vert_x[point]) / (vert_x[point + 1] 
                    - vert_x[point]) + vert_y[point]))
                {
                    intflag = !intflag;
                }
            }
        }
    }
    else
    {
        /* Loop through the points checking the number of intersections */
        for (point = 0; point < num_sides; point++)
        {
            if (((vert_x[point] > point_x) != (vert_x[point + 1] > point_x))
                && (point_y < (vert_y[point + 1] - vert_y[point]) * (point_x
                - vert_x[point]) / (vert_x[point + 1] - vert_x[point])
                + vert_y[point]))
            {
                intflag = !intflag;
            }
        }
    }

    /* If the number of intersections is even, the point is outside the
       polygon.  If the number is odd, the point is inside the polygon. */
    return intflag;
}

/*****************************************************************************
NAME:  ias_math_point_in_polygon_distance

PURPOSE: Determine if a point is located within the bounds of a polygon and 
         compute the polygon intersection distance 

RETURN VALUE:
Type = int
Value    Description
-----    -----------
TRUE        Point inside polygon
FALSE       Point not inside polygon (or unable to compute)
ERROR       Unable to compute

ALGORITHM:
    Based on W. Randolph Franklin's algorithm:
        http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    See also the Wikipedia page:
        http://eni.wikipedia.org/wiki/Point_in_polygon

Notes:  There shoud be one more point in vertex arrays than the number
        of sides. This method is used to close the polygon instead of wrapping 
        around to the first point of the polygon.

*****************************************************************************/
int ias_math_point_in_closed_polygon_distance
(
    unsigned int num_sides,             /* I: Number of sides in polygon */
    const double *vert_x,               /* I: Vertices of polygon */
    const double *vert_y,               /* I: Vertices of polygon */
    double point_x,                     /* I: X coordinate of point */
    double point_y,                     /* I: Y coordinate of point */
    unsigned int num_segs,              /* I: Number of polygon segments */
    const IAS_POLYGON_SEGMENT *poly_seg,/* I: Array of polygon segments */
    unsigned int direction, /* I: Direction to measure distance: 0 = x, 1 = y */
    double *distance        /* O: Distance from point to polygon boundary in
                                  specified direction */
)
{
    unsigned int segment;               /* Loop variable per segment */
    unsigned int point;                 /* Loop variable per point */
    double local_distance;              /* Distance */
    const double *local_vert_x = vert_x;/* Local vertex x */
    const double *local_vert_y = vert_y;/* Local vertex y */
    double local_x = point_x;           /* Local x coordinate */
    double local_y = point_y;           /* Local y coordinate */
    int intflag = 0;                    /* Flag denoting even (0) or odd (1)
                                           number of polygon side 
                                           intersections */

    /* Initialize the distance to a negative number. */
    *distance = -1;

    if (num_sides < 3) 
    {
        IAS_LOG_ERROR("Need at least three sides for polygon.");
        return ERROR;
    }

    /* Set up the local variables based on the specified 'look' direction.
       The algorithm is set up to look in the y direction.  If the direction
       is not 1 (x), then simply interchange the x/y values. */
    if (direction != 1)
    {
        local_vert_x = vert_y;
        local_vert_y = vert_x;
        local_x = point_y;
        local_y = point_x;
    }
    
    /* If polygon segments have been specified, make use of them. */
    if (num_segs > 0)
    {
        for (segment = 0; segment < num_segs; segment++)
        {
            /* Check the segment bounding box, taking the 'look' direction
               into consideration. */
            if ((direction == 0 && (poly_seg[segment].min_y > point_y || 
                poly_seg[segment].max_y < point_y || poly_seg[segment].max_x < 
                point_x)) || (direction == 1 && (poly_seg[segment].min_x > 
                point_x || poly_seg[segment].max_x < point_x || 
                poly_seg[segment].max_y < point_y)))
            {
                continue;
            }

            for (point = poly_seg[segment].first_point; 
                 point < poly_seg[segment].last_point; point++)
            {
                if ((local_vert_x[point] > local_x) == 
                    (local_vert_x[point + 1] > local_x))
                {
                    continue;
                }

                local_distance = (local_vert_y[point + 1] - 
                    local_vert_y[point]) * (local_x - 
                    local_vert_x[point]) / (local_vert_x[point + 1] -
                    local_vert_x[point]) + local_vert_y[point] - 
                    local_y;

                if (local_distance <= 0)
                {
                    continue;
                }

                intflag = !intflag;

                if (*distance > local_distance || *distance < 0)
                    *distance = local_distance;
            }
        }
    }
    else
    {
        for (point = 0; point < num_sides; point++)
        {
            if ((local_vert_x[point] > local_x) == 
                (local_vert_x[point + 1] > local_x))
            {
                continue;
            }
        
            local_distance = (local_vert_y[point + 1] - 
                local_vert_y[point]) * (local_x - local_vert_x[point]) 
                / (local_vert_x[point + 1] - local_vert_x[point]) + 
                local_vert_y[point] - local_y;

            if (local_distance <= 0)
            {
                continue;
            }

            intflag = !intflag;

            if (*distance > local_distance || *distance < 0)
                *distance = local_distance;
        }
    }

    /* If the number of intersections is even, the point is outside the
       polygon.  If the number is odd, the point is inside the polygon. */
    return intflag;
}
