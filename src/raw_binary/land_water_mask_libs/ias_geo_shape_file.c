/* Standard Library Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* IAS Library Includes */
#include "ias_lw_geo.h"
#include "ias_logging.h"

/****************************************************************************
NAME:  ias_geo_get_polygon_offsets

PURPOSE:  Compute the number of bytes used for the polygon structures.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
static void ias_geo_get_polygon_offsets
(
    const IAS_POLYGON_LINKED_LIST *polygon, /* I: Polygon linked list */
    int64_t *offset,                        /* I/O : Polygon offset array */
    int64_t *byte_count                     /* I/O : Byte counter */
)
{
    while (polygon)
    {
        /* Set the polygon offset */
        offset[polygon->id] = *byte_count;

        /* Polygons contain the following fixed-count members: ID,
           child ID, number of points - 1 (one subtracted since the final
           point is a copy of the first and not written to the file), number of
           segments, min x, max x, min y, max y. Polygon contain the following
           variable-count arrays: point_x(num_points), point_y(num_points), 
           poly_seg(num_segs) */
        *byte_count += 48 + 16 * (polygon->num_points - 1) + 
            polygon->num_segs * sizeof(IAS_POLYGON_SEGMENT);

        /* Get offsets for children */
        if (polygon->child)
            ias_geo_get_polygon_offsets(polygon->child, offset, byte_count);

        polygon = polygon->next;
    }

    /* Account for final zero value marking the end of the group */
    *byte_count += 4;
}

/****************************************************************************
NAME:  ias_geo_write_polygons

PURPOSE:  Write the individual polygons out to a file.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
static int ias_geo_write_polygons
(
    FILE *fp,                        /* I: File pointer to dump results to */
    const IAS_POLYGON_LINKED_LIST *polygon /* I: Polygon linked list */
)
{
    unsigned int zero = 0; /* Zero for writing end of polygon */

    /* Write out the polygons */
    while (polygon)
    {
        /* Write one less than the number of points since the last point
           is a copy of the first point and is not written to the file */
        int local_points = polygon->num_points - 1;

        /* Writing out the polygon id */
        if (fwrite(&polygon->id, sizeof(unsigned int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out polygon id %d", polygon->id);
            return ERROR;
        }

        /* Writing out the number of points */
        if (fwrite(&local_points, sizeof(unsigned int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out the number of points for polygon id %d",
                polygon->id);
            return ERROR;
        }

        /* Writing out the x vertices */
        if (fwrite(polygon->point_x, sizeof(double), local_points, fp)
            != local_points)
        {
            IAS_LOG_ERROR("Writing out X vertices for polygon id %d",
                polygon->id);
            return ERROR;
        }

        /* Writing out the y vertices */
        if (fwrite(polygon->point_y, sizeof(double), local_points, fp)
            != local_points)
        {
            IAS_LOG_ERROR("Writing out Y vertices for polygon id %d",
                polygon->id);
            return ERROR;
        }

        /* Writing out the minimum x bounds */
        if (fwrite(&polygon->min_x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out minimum X bounds for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Writing out the maximum x bounds */
        if (fwrite(&polygon->max_x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out maximum X bounds for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Writing out the minimum y bounds */
        if (fwrite(&polygon->min_y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out minimum Y bounds for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Writing out the maximum y bounds */
        if (fwrite(&polygon->max_y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out maximum Y bounds for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Writing out the number of polygon segments */
        if (fwrite(&polygon->num_segs, sizeof(unsigned int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out number of segments for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Writing out the polygon segments */
        if (fwrite(polygon->poly_seg, sizeof(IAS_POLYGON_SEGMENT), 
            polygon->num_segs, fp) != polygon->num_segs)
        {
            IAS_LOG_ERROR("Writing out the polygon segments for polygon id %d", 
                polygon->id);
            return ERROR;
        }

        /* Write out the children if present */
        if (polygon->child)
        {
            /* Write out the polygons child id */
            if (fwrite(&polygon->child->id, sizeof(unsigned int), 1, fp) != 1)
            {
                IAS_LOG_ERROR("Writing out the child %d of polygon %d", 
                    polygon->child->id, polygon->id);
                return ERROR;
            }

            /* Dump out the child polygon */
            if (ias_geo_write_polygons(fp, polygon->child) != SUCCESS)
            {
                IAS_LOG_ERROR("Writing out the polygons");
                return ERROR;
            }
        }
        else
        {
            /* Write out end of polygon */
            if (fwrite(&zero, sizeof(unsigned int), 1, fp) != 1)
            {
                IAS_LOG_ERROR("Writing out end of polygon for %d",
                    polygon->id);
                return ERROR;
            }
        }

        /* Advance to the next polygon */
        polygon = polygon->next;
    }

    /* Write out a final ID of zero. This is used by the load function to
       determine whether we've reached the end of the group. */
    if (fwrite(&zero, sizeof(unsigned int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Writing out end of polygon for %d",
            polygon->id);
        return ERROR;
    }

    return SUCCESS;
}

/*****************************************************************************
NAME:  ias_geo_dump_polygon

PURPOSE:  Dump the polygon list out to a file in the parent/child structure.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
int ias_geo_dump_polygon
(
    FILE *fp,                              /* I: File pointer to dump results */
    const IAS_POLYGON_LINKED_LIST *polygon,/* I: Polygon linked list */
    unsigned int npolygons,                /* I: Number of polygons */
    unsigned int nparent_polygons          /* I: Number of parent polygons */
)
{
    const IAS_POLYGON_LINKED_LIST *head = polygon;/* First polygon in list */
    int64_t *offset;                       /* Polygon offset in binary list */
    int64_t byte_count;                    /* Polygon list byte counter */
    unsigned int zero = 0;                 /* Zero for writing end of polygon */
    unsigned int count;                    /* Count of polygons */ 

    /* Compute file offsets for polygons. The initial offset is for the
       "header" information consisting of the number of parent polygons and
       their bounding boxes. Each parent block consists of the polygon ID,
       byte offset, and bounding box range */
    offset = malloc((npolygons + 1) * sizeof(int64_t));
    if (offset == NULL)
    {
        IAS_LOG_ERROR("Allocating space for polygon offsets");
        return ERROR;
    }

    byte_count = 2 * sizeof(unsigned int) +
        nparent_polygons * (sizeof(unsigned int) + sizeof(int64_t) +
        4 * sizeof(double));
    ias_geo_get_polygon_offsets(polygon, offset, &byte_count);

    /* Write out the "parent" polygon bounding boxes and offsets.
       Verify the number of parent polygons found agrees with the
       number reported. */
    if (fwrite(&nparent_polygons, sizeof(unsigned int), 1 , fp) != 1)
    {
        IAS_LOG_ERROR("Writing out parent polygons %d", nparent_polygons);
        free(offset);
        return ERROR;
    }
    count = 0;

    /* Write out the polygons. */
    while (polygon)
    {
        /* Writing out the polygon id */
        if (fwrite(&polygon->id, sizeof(unsigned int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out polygon id %d", polygon->id);
            free(offset);
            return ERROR;
        }

        /* Writing output the offset */
        if (fwrite(&offset[polygon->id], sizeof(int64_t), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out offset for polygon id %d", polygon->id);
            free(offset);
            return ERROR;
        }

        /* Writing out the minimum x bounds */
        if (fwrite(&polygon->min_x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out minimum X bounds for polygon id %d", 
                polygon->id);
            free(offset);
            return ERROR;
        }

        /* Writing out the maximum x bounds */
        if (fwrite(&polygon->max_x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out maximum X bounds for polygon id %d", 
                polygon->id);
            free(offset);
            return ERROR;
        }

        /* Writing out the minimum y bounds */
        if (fwrite(&polygon->min_y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out minimum Y bounds for polygon id %d", 
                polygon->id);
            free(offset);
            return ERROR;
        }

        /* Writing out the maximum y bounds */
        if (fwrite(&polygon->max_y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Writing out maximum Y bounds for polygon id %d", 
                polygon->id);
            free(offset);
            return ERROR;
        }

        /* Advance to the next polygon */
        polygon = polygon->next;
        count++;
    } /* End of while loop */
    free(offset);
    offset = NULL;

    if (count != nparent_polygons)
    {
        IAS_LOG_ERROR("Number of parents found (%d) doesn't agree "
            "with the number reported (%d).", count, nparent_polygons);
        return ERROR;
    }

    /* Write out a final ID of zero. This is used by the load function to
       determine whether we 've reached the end of the group */
    if (fwrite(&zero, sizeof(unsigned int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Writing out end of file");
        return ERROR;
    }

    /* Write out the polygons */ 
    if (ias_geo_write_polygons(fp, head) != SUCCESS)
    {
       IAS_LOG_ERROR("Writing out the polygons");
       return ERROR;
    }
    return SUCCESS;
}

/****************************************************************************
NAME:  ias_geo_read_polygon

PURPOSE:  Read a polygon and its children from a file. The newly read polygon
          is read into the polygon buffer provided and the children are placed
          in a linked list under the child pointer.
          Returns the ID or ERROR

RETURN VALUE:
Type = int
Value    Description
-----    -----------
id       Successful completion
ERROR    Operation failed
*****************************************************************************/
static int ias_geo_read_polygon
(
    FILE *fp,                       /* I: Output file pointer */
    IAS_POLYGON_LINKED_LIST *polygon  /* I/O: Polygon pointer */
)
{
    int id;
    int child_id;
    IAS_POLYGON_LINKED_LIST *child_tail = NULL;

    if (fread(&id, sizeof(int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading the polygon ID");
        return ERROR;
    }

    /* If the ID is zero, we're done */
    if (id == 0)
        return id;

    polygon->id = id;
    if (fread(&polygon->num_points, sizeof(unsigned int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading number of points");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    /* Need a final point that duplicates the first point. */
    polygon->point_x = malloc((polygon->num_points + 1) * sizeof(double));
    if (polygon->point_x == NULL)
    {
        IAS_LOG_ERROR("Allocating memory");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    polygon->point_y = malloc((polygon->num_points + 1) * sizeof(double));
    if (polygon->point_y == NULL)
    {
        IAS_LOG_ERROR("Allocating memory");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(polygon->point_x, sizeof(double), polygon->num_points, fp) != 
        polygon->num_points)
    {
        IAS_LOG_ERROR("Reading X vertices for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(polygon->point_y, sizeof(double), polygon->num_points, fp) !=
        polygon->num_points)
    {
        IAS_LOG_ERROR("Reading Y vertices for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(&polygon->min_x, sizeof(double), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading minimum X bounds for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(&polygon->max_x, sizeof(double), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading maximum X bounds for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(&polygon->min_y, sizeof(double), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading minimum Y bounds for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(&polygon->max_y, sizeof(double), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading maximum Y bounds for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(&polygon->num_segs, sizeof(unsigned int), 1, fp) !=1 )
    {
        IAS_LOG_ERROR("Reading number of segments for polygon");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    polygon->poly_seg = malloc(polygon->num_segs * sizeof(IAS_POLYGON_SEGMENT));
    if (polygon->poly_seg == NULL)
    {
        IAS_LOG_ERROR("Allocating memory");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    if (fread(polygon->poly_seg, sizeof(IAS_POLYGON_SEGMENT), 
        polygon->num_segs, fp) != polygon->num_segs)
    {
        IAS_LOG_ERROR("Reading the polygon segments");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    /* Duplicate the first point as the last point. */
    polygon->point_x[polygon->num_points] = polygon->point_x[0];
    polygon->point_y[polygon->num_points] = polygon->point_y[0];
    polygon->num_points++;

    /* Read any children. */
    if (fread(&child_id, sizeof(int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading child id");
        ias_geo_free_polygon_linked_list(polygon);
        return ERROR;
    }

    while (child_id != 0)
    {
        IAS_POLYGON_LINKED_LIST *child = NULL; /* new child polygon */

        /* Allocate memory for the next child polygon */
        child = calloc(1, sizeof(IAS_POLYGON_LINKED_LIST));
        if (child == NULL)
        {
            IAS_LOG_ERROR("Allocating memory for linked list");
            return ERROR;
        }

        /* Read the next child polygon */
        child_id = ias_geo_read_polygon(fp, child);
        if (child_id == ERROR)
        {
            IAS_LOG_ERROR("Reading children polygons");
            ias_geo_free_polygon_linked_list(polygon);
            return ERROR;
        }

        /* If a child was read, add it to the tail of the child list */
        if (child_id != 0)
        {
            if (!polygon->child)
            {
                /* First child, so it starts the child list */
                polygon->child = child;
                child_tail = child;
            }
            else
            {
                /* Add the child to the end of the existing list */
                child_tail->next = child;
                child->prev = child_tail;
                child_tail = child;
            }
        }
        else
        {
            /* Reached the end of the children, so free the unused node */
            free(child);
        }
    }

    return id;
}

/*****************************************************************************
NAME:  ias_geo_load_polygon

PURPOSE:  Read the polygons in from a file in the parent/child structure. 

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
int ias_geo_load_polygon
(
    FILE *fp,                       /* I: Input file pointer */
    double min_x,                   /* I: Minimum x value of interest */
    double max_x,                   /* I: Maximum x value of interest */
    double min_y,                   /* I: Minimum y value of interest */
    double max_y,                   /* I: Maximum y value of interest */
    IAS_POLYGON_LINKED_LIST **head  /* O: Polygon pointer */
)
{
    IAS_POLYGON_LINKED_LIST *polygon; /* Current polygon in loop */
    IAS_POLYGON_LINKED_LIST *list_tail = NULL;
                                /* Pointer to the tail of the polygon list */
    int id;                           /* Current id of polygon */
    unsigned int nparent_polygons;    /* Number of parent polygons */
    unsigned int i;                   /* Generic counter */
    int64_t *offset;                  /* Polygon offset in binary list */
    int error_occured = FALSE;        /* Error tracking flag */
    IAS_DBL_XY *bb_max;               /* Bounding box max x/y values */
    IAS_DBL_XY *bb_min;               /* Bounding box min x/y values */

    /* Assume no polygons will be read */
    *head = NULL;

    /* Read the number of "parent" polygons. */
    if (fread(&nparent_polygons, sizeof(unsigned int), 1, fp) != 1)
    {
        IAS_LOG_ERROR("Reading the number of parent polygons");
        return ERROR;
    }

    if (nparent_polygons == 0)
    {
        IAS_LOG_ERROR("Only one parent polygon reported");
        return ERROR;
    }

    /* Allocate memory for offsets and bounding boxes */
    offset = malloc(nparent_polygons * sizeof(int64_t));
    if (offset == NULL)
    {
        IAS_LOG_ERROR("Allocating space for polygon offsets");
        return ERROR;
    }

    bb_max = malloc(nparent_polygons * sizeof(IAS_DBL_XY));
    if (bb_max == NULL)
    {
        IAS_LOG_ERROR("Allocating space for the bouding box maximum values");
        free(offset);
        return ERROR;
    }

    bb_min = malloc(nparent_polygons * sizeof(IAS_DBL_XY));
    if (bb_min == NULL)
    {
        IAS_LOG_ERROR("Allocating space for the bounding box minimum values");
        free(offset);
        free(bb_max);
        return ERROR;
    }

    /* Read the parent polygon offsets and bounding boxes. Verify that the
       number read agrees with the number reported */
    for (i = 0; i < nparent_polygons; i++)
    {
        if (fread(&id, sizeof(int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading id");
            error_occured = TRUE;
            break;
        }

        if (id == 0)
            break;

        if (fread(&offset[i], sizeof(int64_t), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading offset");
            error_occured = TRUE;
            break;
        }

        if (fread(&bb_min[i].x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading minimum X bound");
            error_occured = TRUE;
            break;
        }

        if (fread(&bb_max[i].x, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading maximum X bound");
            error_occured = TRUE;
            break;
        }

        if (fread(&bb_min[i].y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading minimum Y bound");
            error_occured = TRUE;
            break;
        }

        if (fread(&bb_max[i].y, sizeof(double), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading maximum Y bound");
            error_occured = TRUE;
            break;
        }
    }

    /* Check for errors while reading the parent bounding boxes
       and offsets */
    if (error_occured)
    {
        IAS_LOG_ERROR("Reading parent bounding box and offsets");
        free(offset);
        free(bb_min);
        free(bb_max);
        return ERROR;
    }

    /* Verify that the number read agrees with the number reported.
       Need to read a final polygon ID (which should be zero) */
    if (i < nparent_polygons)
    {
        IAS_LOG_ERROR( "Number of parents found (%d) doesn't agree "
            "with the number reported (%d)", i, nparent_polygons);
        error_occured = TRUE;
    }
    else
    {
        if (fread(&id, sizeof(int), 1, fp) != 1)
        {
            IAS_LOG_ERROR("Reading id");
            error_occured = TRUE;
        }
        else if (id != 0)
        {
            IAS_LOG_ERROR("More parents found than reported (%d)",
                nparent_polygons);
            error_occured = TRUE;
        }
    }

    /* Check if error occured during the parent validation */
    if (error_occured)
    {
        IAS_LOG_ERROR("Validating the number of parent polygons");
        free(offset);
        free(bb_min);
        free(bb_max);
        return ERROR;
    }

    /* Read the polygons within the range of interest. */
    polygon = NULL;
    for (i = 0; i < nparent_polygons; i++)
    {
        if (bb_min[i].x > max_x || bb_max[i].x < min_x ||
            bb_min[i].y > max_y || bb_max[i].y < min_y)
            continue;

        if (fseeko(fp, offset[i], SEEK_SET) != 0)
        {
            IAS_LOG_ERROR("Using fseek to set the position in file pointer");
            ias_geo_free_polygon_linked_list(*head);
            *head = NULL;
            free(offset);
            free(bb_min);
            free(bb_max);
            return ERROR;
        }

        /* Allocate a node for the new polygon */
        polygon = calloc(1, sizeof(IAS_POLYGON_LINKED_LIST));
        if (polygon == NULL)
        {
            IAS_LOG_ERROR("Allocating memory for linked list");
            ias_geo_free_polygon_linked_list(*head);
            *head = NULL;
            free(offset);
            free(bb_min);
            free(bb_max);
            return ERROR;
        }

        /* Read the polygon and its children */
        if (ias_geo_read_polygon(fp, polygon) < 1)
        {
            IAS_LOG_ERROR("Reading polygons");
            ias_geo_free_polygon_linked_list(*head);
            *head = NULL;
            free(offset);
            free(bb_min);
            free(bb_max);
            return ERROR;
        }

        /* If this is the first polygon read, make it the list head */
        if (!*head)
            *head = polygon;

        if (list_tail)
        {
            /* The tail exists, so add this polygon to it */
            list_tail->next = polygon;
            polygon->prev = list_tail;
        }
        /* This polygon is now the tail */
        list_tail = polygon;

    }

    free(offset);
    free(bb_min);
    free(bb_max);

    return SUCCESS;
}

/*****************************************************************************
NAME:  ias_geo_free_polygon_linked_list

PURPOSE:  Free the polygon memory 

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
void ias_geo_free_polygon_linked_list
(
    IAS_POLYGON_LINKED_LIST *polygon    /* I: First polygon in list */
)
{
    IAS_POLYGON_LINKED_LIST *next;      /* Next polygon in list */

    while (polygon)
    {
        if (polygon->point_x)
        {
            free(polygon->point_x);
        }

        if (polygon->point_y)
        {
            free(polygon->point_y);
        }

        if (polygon->poly_seg)
        {
            free(polygon->poly_seg);
        }

        ias_geo_free_polygon_linked_list(polygon->child);
        next = polygon->next;
        free(polygon);
        polygon = next;
    }
}

/*****************************************************************************
NAME:  ias_geo_reduce_polygon

PURPOSE: Discard polygons outside a specified bounding box.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
SUCCESS  Successful completion
ERROR    Operation failed

*****************************************************************************/
int ias_geo_reduce_polygon
(
    IAS_POLYGON_LINKED_LIST **polygon_list, /* I/O: First polygon in list */
    double upper_left_x,                    /* I: Upper left x */
    double lower_right_x,                   /* I: Lower right x */
    double upper_left_y,                    /* I: Upper left y */
    double lower_right_y                    /* I: Lower right y */
)
{
    IAS_POLYGON_LINKED_LIST *polygon;/* Current polygon list pointer */
    IAS_POLYGON_LINKED_LIST *next;   /* Next polygon list pointer */
    unsigned int x_crossing;         /* Flag indicating whether a horizontal
                                        domain boundary is crossed; this is
                                        usually associated with +/-180 degree
                                        longitude boundaries */

    if (upper_left_x > lower_right_x)
    {
        IAS_LOG_ERROR(
            "Upper left longitude greater than lower right longitude");
        return ERROR;
    }

    if (upper_left_y < lower_right_y)
    {
        IAS_LOG_ERROR("Upper left latitude less than lower right latitude");
        return ERROR;
    }

    /* Set the boundary crossing flag. */
    if (lower_right_x >= upper_left_x)
    {
        x_crossing = 0;
    }
    else
    {
        x_crossing = 1;
    }

    polygon = *polygon_list;
    while (polygon)
    {
        /* Remove unnecessary children. */
        if (polygon->child)
        {
            ias_geo_reduce_polygon(&polygon->child, 
                upper_left_x, lower_right_x, upper_left_y, lower_right_y);
        }

        /* Keep track of next polygon */
        next = polygon->next;

        /* Check if polygon is outside the bounding box */
        if (polygon->min_y > upper_left_y || polygon->max_y < lower_right_y 
            || (!x_crossing && (polygon->min_x > lower_right_x || polygon->max_x
            < upper_left_x)) || (x_crossing && (polygon->min_x > lower_right_x 
            && polygon->max_x < upper_left_x)))
        {
            /* Check if the current polygon is the head of the list */
            if (polygon != *polygon_list)
            {
                /* Polygon is not the head of the list reset the links so 
                   the list isn't broken */
                polygon->prev->next = next;
                if (next != NULL)
                {
                    next->prev = polygon->prev;
                }
            }
            else
            {
                /* Polygon is the head of the list so change the head of the
                   list to the next polygon */
                *polygon_list = next;
                if (next != NULL)
                {
                    next->prev = NULL;
                }
            }

            /* Free up the polygon */
            polygon->next = NULL;
            ias_geo_free_polygon_linked_list(polygon);
        }

        polygon = next;
    }
    return SUCCESS;
}
