/*****************************************************************************
FILE: parse_metadata.c
  
PURPOSE: Contains functions for parsing the ESPA internal metadata files.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development
2/25/2014    Gail Schmidt     Added support for source and category attributes
                              for the band metadata

NOTES:
  1. The XML metadata format parsed or written via this library follows the
     ESPA internal metadata format found in ESPA Raw Binary Format v1.0.doc.
     The schema for the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
  2. This code relies on the libxml2 library developed for the Gnome project.
*****************************************************************************/

#include "espa_metadata.h"

/******************************************************************************
MODULE:  add_global_metadata_proj_info_albers

PURPOSE: Add the ALBERS projection elements node to the global metadata
projection information structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the projection_info elements
SUCCESS         Successful parse of the projection_info values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata_proj_info_albers
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata_proj_info_albers"; /* func name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */

    /* Make sure the projection type specified matches the projection
       parameters type */
    if (gmeta->proj_info.proj_type != GCTP_ALBERS_PROJ)
    {
        sprintf (errmsg, "Projection type is not ALBERS so the fact that "
            "albers_proj_params exists is a mismatch in the "
            "projection_information.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Process the siblings in the projection information */
    for (cur_node = a_node->children; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Set up the child pointer */
        child_node = cur_node->children;

        if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "standard_parallell1"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.standard_parallel1 =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "standard_parallell2"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.standard_parallel2 =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "central_meridian"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.central_meridian =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "origin_latitude"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.origin_latitude =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "false_easting"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.false_easting =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "false_northing"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:albers_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.false_northing =
                atof ((const char *) child_node->content);
        }
        else
        {
            sprintf (errmsg, "Unknown albers_proj_params element: %s",
                cur_node->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_global_metadata_proj_info_ps

PURPOSE: Add the Polar Stereographic projection elements node to the global
metadata projection information structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the projection_info elements
SUCCESS         Successful parse of the projection_info values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata_proj_info_ps
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata_proj_info_ps"; /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */

    /* Make sure the projection type specified matches the projection
       parameters type */
    if (gmeta->proj_info.proj_type != GCTP_PS_PROJ)
    {
        sprintf (errmsg, "Error: projection type is not PS so the fact that "
            "ps_proj_params exists is a mismatch in projection_information.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Process the siblings in the projection information */
    for (cur_node = a_node->children; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Set up the child pointer */
        child_node = cur_node->children;

        if (xmlStrEqual (cur_node->name, (const xmlChar *) "longitude_pole"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:ps_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.longitude_pole =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "latitude_true_scale"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:ps_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.latitude_true_scale =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "false_easting"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:ps_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.false_easting =
                atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "false_northing"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Error processing global_metadata:"
                    "projection_information:ps_proj_params element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
    
            /* Copy the content of the child node into the value for this
               field */
            gmeta->proj_info.false_northing =
                atof ((const char *) child_node->content);
        }
        else
        {
            sprintf (errmsg, "Unknown ps_proj_params element: %s",
                cur_node->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_global_metadata_proj_info_utm

PURPOSE: Add the UTM projection elements node to the global metadata projection
information structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the projection_info elements
SUCCESS         Successful parse of the projection_info values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata_proj_info_utm
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata_proj_info_utm"; /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */

    /* Make sure the projection type specified matches the projection
       parameters type */
    if (gmeta->proj_info.proj_type != GCTP_UTM_PROJ)
    {
        sprintf (errmsg, "Projection type is not UTM so the fact that "
            "utm_proj_params exists is a mismatch in the "
            "projection_information.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Set up the current and child pointers */
    cur_node = a_node->children;
    child_node = cur_node->children;

    /* Process the zone code for UTM */
    if (xmlStrEqual (cur_node->name, (const xmlChar *) "zone_code"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Error processing global_metadata:"
                "projection_information:utm_proj_params element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this
           field */
        gmeta->proj_info.utm_zone = atoi ((const char *) child_node->content);
    }
    else
    {
        sprintf (errmsg, "Unknown utm_proj_params element: %s",
            cur_node->name);
        error_handler (false, FUNC_NAME, errmsg);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_global_metadata_proj_info

PURPOSE: Add the projection elements node to the global metadata structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the projection_info elements
SUCCESS         Successful parse of the projection_info values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata_proj_info
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata_proj_info"; /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlAttrPtr attr = NULL;       /* pointer to the element attributes */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    xmlNsPtr ns = NULL;           /* pointer to the namespace */
    xmlChar *attr_val = NULL;     /* attribute value */
    bool is_ul = false;           /* is this the UL corner */
    bool is_lr = false;           /* is this the LR corner */
    double x, y;                  /* x/y values */
    int count;                    /* number of chars copied in snprintf */

    /* Set up the current and child pointers */
    cur_node = a_node;
    ns = cur_node->nsDef;
    child_node = cur_node->children;

    /* Verify the namespace of this node is our ESPA namespace.  If it isn't
       then the element won't be added to the metadata structure. */
    if (!xmlStrEqual (ns->href, (const xmlChar *) ESPA_NS))
    {
        sprintf (errmsg, "Skipping %s since it is not in the ESPA namespace",
            cur_node->name);
        error_handler (false, FUNC_NAME, errmsg);
        return (SUCCESS);
    }

    /* Handle the element attributes */
    for (attr = cur_node->properties; attr != NULL; attr = attr->next)
    {
        attr_val = xmlGetProp (cur_node, attr->name);
        if (xmlStrEqual (attr->name, (const xmlChar *) "projection"))
        {
            if (xmlStrEqual (attr_val, (const xmlChar *) "UTM"))
                gmeta->proj_info.proj_type = GCTP_UTM_PROJ;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "PS"))
                gmeta->proj_info.proj_type = GCTP_PS_PROJ;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "ALBERS"))
                gmeta->proj_info.proj_type = GCTP_ALBERS_PROJ;
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "sphere_code"))
            gmeta->proj_info.sphere_code = atoi ((const char *) attr_val);
        else if (xmlStrEqual (attr->name, (const xmlChar *) "units"))
        {
            count = snprintf (gmeta->proj_info.units,
                sizeof (gmeta->proj_info.units), "%s", (const char *) attr_val);
            if (count < 0 || count >= sizeof (gmeta->proj_info.units))
            {
                sprintf (errmsg, "Overflow of gmeta->proj_info.units string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else
        {
            sprintf (errmsg, "Unknown attribute for element (%s): %s",
                cur_node->name, attr->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
        xmlFree (attr_val);
    }

    /* Process the siblings in the projection information */
    for (cur_node = a_node->children; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Set up the child pointer */
        child_node = cur_node->children;

        /* Check for the element nodes within the projection_information
           element */
        if (xmlStrEqual (cur_node->name, (const xmlChar *) "corner_point"))
        {
            /* Handle the element attributes */
            x = -9999.0;
            y = -9999.0;
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                attr_val = xmlGetProp (cur_node, attr->name);
                if (xmlStrEqual (attr->name, (const xmlChar *) "location"))
                {
                    is_ul = false;
                    is_lr = false;
                    if (xmlStrEqual (attr_val, (const xmlChar *) "UL"))
                        is_ul = true;
                    else if (xmlStrEqual (attr_val, (const xmlChar *) "LR"))
                        is_lr = true;
                    else
                    {
                        sprintf (errmsg, "Unknown corner_point location "
                            "specified (%s). UL and LR expected.", attr_val);
                        error_handler (false, FUNC_NAME, errmsg);
                    }
                }
                else if (xmlStrEqual (attr->name, (const xmlChar *) "x"))
                    x = atof ((const char *) attr_val);
                else if (xmlStrEqual (attr->name, (const xmlChar *) "y"))
                    y = atof ((const char *) attr_val);
                else
                {
                    sprintf (errmsg, "unknown attribute for element (%s): %s",
                        cur_node->name, attr->name);
                    error_handler (false, FUNC_NAME, errmsg);
                }
                xmlFree (attr_val);
            }

            /* Populate the correct corner point */
            if (is_ul)
            {
                gmeta->proj_info.ul_corner[0] = x;
                gmeta->proj_info.ul_corner[1] = y;
            }
            else if (is_lr)
            {
                gmeta->proj_info.lr_corner[0] = x;
                gmeta->proj_info.lr_corner[1] = y;
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "grid_origin"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing global_metadata:"
                    "projection_information element: %s.", cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into the value for this
               field */
            count = snprintf (gmeta->proj_info.grid_origin,
                sizeof (gmeta->proj_info.grid_origin), "%s", 
                (const char *) child_node->content);
            if (count < 0 || count >= sizeof (gmeta->proj_info.grid_origin))
            {
                sprintf (errmsg, "Overflow of gmeta->proj_info.grid_origin");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "utm_proj_params"))
        {
            /* Handle the projection-specific parameters */
            if (add_global_metadata_proj_info_utm (cur_node, gmeta))
            {
                sprintf (errmsg, "Processing projection_information:"
                    "utm_proj_params elements");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "ps_proj_params"))
        {
            /* Handle the projection-specific parameters */
            if (add_global_metadata_proj_info_ps (cur_node, gmeta))
            {
                sprintf (errmsg, "Processing projection_information:"
                    "ps_proj_params elements");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "albers_proj_params"))
        {
            /* Handle the projection-specific parameters */
            if (add_global_metadata_proj_info_albers (cur_node, gmeta))
            {
                sprintf (errmsg, "Processing projection_information:"
                    "albers_proj_params elements");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else
        {
            sprintf (errmsg, "Unknown projection information element: %s",
                cur_node->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
    }  /* end for cur_node */

    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_global_metadata_bounding_coords

PURPOSE: Add the bounding coords elements node to the global metadata structure

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the bounding_coords elements
SUCCESS         Successful parse of the bounding_coords values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata_bounding_coords
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata_bounding_coords";/* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    xmlNsPtr ns = NULL;           /* pointer to the namespace */
    int indx;                     /* index into the bounding coords array */

    /* Set up the current and child pointers */
    cur_node = a_node;
    ns = cur_node->nsDef;
    child_node = cur_node->children;

    /* Verify the namespace of this node is our ESPA namespace.  If it isn't
       then the element won't be added to the metadata structure. */
    if (!xmlStrEqual (ns->href, (const xmlChar *) ESPA_NS))
    {
        sprintf (errmsg, "Skipping %s since it is not in the ESPA namespace",
            cur_node->name);
        error_handler (false, FUNC_NAME, errmsg);
        return (SUCCESS);
    }

    /* Look for the ESPA global metadata bounding coordinates elements and
       process them */
    indx = 0;
    if (xmlStrEqual (cur_node->name, (const xmlChar *) "west"))
        indx = ESPA_WEST;
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "east"))
        indx = ESPA_EAST;
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "north"))
        indx = ESPA_NORTH;
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "south"))
        indx = ESPA_SOUTH;
    else
    {
        sprintf (errmsg, "Unknown bounding coords element: %s", cur_node->name);
        error_handler (false, FUNC_NAME, errmsg);
    }

    /* Expect the child node to be a text node containing the value of
       this field */
    if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
    {
        sprintf (errmsg, "Processing global_metadata element: %s.",
            cur_node->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Copy the content of the child node into the value for this field */
    gmeta->bounding_coords[indx] = atof ((const char *) child_node->content);

    return (SUCCESS);
}

/******************************************************************************
MODULE:  add_global_metadata

PURPOSE: Add the current element node to the global metadata structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the global_metadata elements
SUCCESS         Successful parse of the global_metadata values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int add_global_metadata
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
)
{
    char FUNC_NAME[] = "add_global_metadata";   /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlAttrPtr attr = NULL;       /* pointer to the element attributes */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    xmlNsPtr ns = NULL;           /* pointer to the namespace */
    xmlChar *attr_val = NULL;     /* attribute value */
    bool is_ul = false;           /* is this the UL corner */
    bool is_lr = false;           /* is this the LR corner */
    double latitude, longitude;   /* lat/long values */
    int count;                    /* number of chars copied in snprintf */

    /* Set up the current and child pointers */
    cur_node = a_node;
    ns = cur_node->nsDef;
    child_node = cur_node->children;

    /* Verify the namespace of this node is our ESPA namespace.  If it isn't
       then the element won't be added to the metadata structure. */
    if (!xmlStrEqual (ns->href, (const xmlChar *) ESPA_NS))
    {
        sprintf (errmsg, "Skipping %s since it is not in the ESPA namespace",
            cur_node->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (SUCCESS);
    }

    /* Look for the ESPA global metadata elements and process them */
    if (xmlStrEqual (cur_node->name, (const xmlChar *) "data_provider"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->data_provider, sizeof (gmeta->data_provider),
            "%s", (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->data_provider))
        {
            sprintf (errmsg, "Overflow of gmeta->data_provider string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "satellite"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->satellite, sizeof (gmeta->satellite), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->satellite))
        {
            sprintf (errmsg, "Overflow of gmeta->satellite string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "instrument"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->instrument, sizeof (gmeta->instrument), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->instrument))
        {
            sprintf (errmsg, "Overflow of gmeta->instrument string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "acquisition_date"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->acquisition_date,
            sizeof (gmeta->acquisition_date), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->acquisition_date))
        {
            sprintf (errmsg, "Overflow of gmeta->acquisition_date string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "scene_center_time"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->scene_center_time,
            sizeof (gmeta->scene_center_time), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->scene_center_time))
        {
            sprintf (errmsg, "Overflow of gmeta->scene_center_time string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "level1_production_date"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->level1_production_date,
            sizeof (gmeta->level1_production_date), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->level1_production_date))
        {
            sprintf (errmsg, "Overflow of gmeta->level1_production_date");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "solar_angles"))
    {
        /* Handle the element attributes */
        for (attr = cur_node->properties; attr != NULL; attr = attr->next)
        {
            attr_val = xmlGetProp (cur_node, attr->name);
            if (xmlStrEqual (attr->name, (const xmlChar *) "zenith"))
                gmeta->solar_zenith = atof ((const char *) attr_val);
            else if (xmlStrEqual (attr->name, (const xmlChar *) "azimuth"))
                gmeta->solar_azimuth = atof ((const char *) attr_val);
            else if (xmlStrEqual (attr->name, (const xmlChar *) "units"))
            {
                count = snprintf (gmeta->solar_units,
                    sizeof (gmeta->solar_units), "%s", (const char *) attr_val);
                if (count < 0 || count >= sizeof (gmeta->solar_units))
                {
                    sprintf (errmsg, "Overflow of gmeta->solar_units string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else
            {
                sprintf (errmsg, "WARNING: unknown attribute for element "
                    "(%s): %s\n", cur_node->name, attr->name);
                error_handler (false, FUNC_NAME, errmsg);
            }
            xmlFree (attr_val);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "wrs"))
    {
        /* Handle the element attributes */
        for (attr = cur_node->properties; attr != NULL; attr = attr->next)
        {
            attr_val = xmlGetProp (cur_node, attr->name);
            if (xmlStrEqual (attr->name, (const xmlChar *) "system"))
                gmeta->wrs_system = atoi ((const char *) attr_val);
            else if (xmlStrEqual (attr->name, (const xmlChar *) "path"))
                gmeta->wrs_path = atoi ((const char *) attr_val);
            else if (xmlStrEqual (attr->name, (const xmlChar *) "row"))
                gmeta->wrs_row = atoi ((const char *) attr_val);
            else
            {
                sprintf (errmsg, "WARNING: unknown attribute for element "
                    "(%s): %s\n", cur_node->name, attr->name);
                error_handler (false, FUNC_NAME, errmsg);
            }
            xmlFree (attr_val);
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "lpgs_metadata_file"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        count = snprintf (gmeta->lpgs_metadata_file,
            sizeof (gmeta->lpgs_metadata_file), "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= sizeof (gmeta->lpgs_metadata_file))
        {
            sprintf (errmsg, "Overflow of gmeta->lpgs_metadata_file string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name, (const xmlChar *) "corner"))
    {
        /* Handle the element attributes */
        latitude = -9999.0;
        longitude = -9999.0;
        for (attr = cur_node->properties; attr != NULL; attr = attr->next)
        {
            attr_val = xmlGetProp (cur_node, attr->name);
            if (xmlStrEqual (attr->name, (const xmlChar *) "location"))
            {
                is_ul = false;
                is_lr = false;
                if (xmlStrEqual (attr_val, (const xmlChar *) "UL"))
                    is_ul = true;
                else if (xmlStrEqual (attr_val, (const xmlChar *) "LR"))
                    is_lr = true;
                else
                {
                    sprintf (errmsg, "WARNING: unknown corner location "
                        "specified (%s). UL and LR expected.\n", attr_val);
                    error_handler (false, FUNC_NAME, errmsg);
                }
            }
            else if (xmlStrEqual (attr->name, (const xmlChar *) "latitude"))
                latitude = atof ((const char *) attr_val);
            else if (xmlStrEqual (attr->name, (const xmlChar *) "longitude"))
                longitude = atof ((const char *) attr_val);
            else
            {
                sprintf (errmsg, "WARNING: unknown attribute for element "
                    "(%s): %s\n", cur_node->name, attr->name);
                error_handler (false, FUNC_NAME, errmsg);
            }
            xmlFree (attr_val);
        }

        /* Populate the correct corner point */
        if (is_ul)
        {
            gmeta->ul_corner[0] = latitude;
            gmeta->ul_corner[1] = longitude;
        }
        else if (is_lr)
        {
            gmeta->lr_corner[0] = latitude;
            gmeta->lr_corner[1] = longitude;
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "bounding_coordinates"))
    {
        /* Process the siblings in the bounding coordinates */
        for (cur_node = a_node->children; cur_node;
             cur_node = xmlNextElementSibling (cur_node))
        {
            if (add_global_metadata_bounding_coords (cur_node, gmeta))
            {
                sprintf (errmsg, "Processing bounding_coordinates element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "projection_information"))
    {
        /* Process the elements within the projection information */
        if (add_global_metadata_proj_info (cur_node, gmeta))
        {
            sprintf (errmsg, "Processing projection_information elements");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (xmlStrEqual (cur_node->name,
        (const xmlChar *) "orientation_angle"))
    {
        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Processing global_metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the value for this field */
        gmeta->orientation_angle = atof ((const char *) child_node->content);
    }
    else
    {
        sprintf (errmsg, "Unknown element (%s) in the global_metadata",
            cur_node->name);
        error_handler (false, FUNC_NAME, errmsg);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_band_metadata_bitmap_description

PURPOSE: Adds the bit elements to the bitmap description to the band metadata
structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the bit elements
SUCCESS         Successful parse of the bit values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/27/2013   Gail Schmidt     Original development

NOTES:
1. Memory is allocated in the band metadata for the number of bits in the
   bitmap description.
******************************************************************************/
int add_band_metadata_bitmap_description
(
    xmlNode *a_node,            /* I/O: pointer to the element node to
                                        process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
)
{
    char FUNC_NAME[] = "add_band_metadata_bitmap_description"; /* func name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    int nbits = 0;                /* number of bits in the bitmap description */
    int count;                    /* number of chars copied in snprintf */

    /* Count the number of siblings which are bit descriptions */
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* If this is a bit element then count it */
        if (xmlStrEqual (cur_node->name, (const xmlChar *) "bit"))
            nbits++;
    }

    /* Allocate memory in the band structure for the number of bits */
    if (allocate_bitmap_metadata (bmeta, nbits) != SUCCESS)
    {
        sprintf (errmsg, "Allocating memory to the band structure for %d "
            "bits in the bitmap description.", nbits);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Process the siblings as long as they are bit elements.  Assume the XML
       file has a description for every bit number inclusive from 0 to nbits-1
       therefore the bit num attribute will not be stored. */
    nbits = 0;
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Set up the child pointer */
        child_node = cur_node->children;

        /* If this isn't a bit element then skip to the next one */
        if (!xmlStrEqual (cur_node->name, (const xmlChar *) "bit"))
            continue;

        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Error processing band metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the array of bits in the
           bitmap_description */
        count = snprintf (bmeta->bitmap_description[nbits], STR_SIZE, "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= STR_SIZE)
        {
            sprintf (errmsg, "Overflow of bmeta->bitmap_description[nbits]");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Increment the bit count */
        nbits++;
    }

    /* Successful processing */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_band_metadata_class_values

PURPOSE: Adds the class elements to the band metadata structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the class elements
SUCCESS         Successful parse of the class values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/27/2013   Gail Schmidt     Original development

NOTES:
1. Memory is allocated in the band metadata for the number of classes in the
   class values.
******************************************************************************/
int add_band_metadata_class_values
(
    xmlNode *a_node,            /* I/O: pointer to the element node to
                                        process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
)
{
    char FUNC_NAME[] = "add_band_metadata_class_values"; /* func name */
    char errmsg[STR_SIZE];        /* error message */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    xmlAttrPtr attr = NULL;       /* pointer to the element attributes */
    xmlChar *attr_val = NULL;     /* attribute value */
    int nclass = 0;               /* number of classes in the class values */
    int count;                    /* number of chars copied in snprintf */

    /* Count the number of siblings which are class descriptions */
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* If this is a class element then count it */
        if (xmlStrEqual (cur_node->name, (const xmlChar *) "class"))
            nclass++;
    }

    /* Allocate memory in the band structure for the number of classes */
    if (allocate_class_metadata (bmeta, nclass) != SUCCESS)
    {
        sprintf (errmsg, "Allocating memory to the band structure for %d "
            "classes in the class_values.", nclass);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Process the siblings as long as they are class elements */
    nclass = 0;
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Set up the child pointer */
        child_node = cur_node->children;

        /* If this isn't a class element then skip to the next one */
        if (!xmlStrEqual (cur_node->name, (const xmlChar *) "class"))
            continue;

        /* Handle the element attributes */
        for (attr = cur_node->properties; attr != NULL; attr = attr->next)
        {
            attr_val = xmlGetProp (cur_node, attr->name);
            if (xmlStrEqual (attr->name, (const xmlChar *) "num"))
                bmeta->class_values[nclass].class =
                    atoi ((const char *) attr_val);
            else
            {
                sprintf (errmsg, "WARNING: unknown attribute for element "
                    "(%s): %s\n", cur_node->name, attr->name);
                error_handler (false, FUNC_NAME, errmsg);
            }
            xmlFree (attr_val);
        }

        /* Expect the child node to be a text node containing the value of
           this field */
        if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
        {
            sprintf (errmsg, "Error processing band metadata element: %s.",
                cur_node->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Copy the content of the child node into the array of classes in the
           class_values */
        count = snprintf (bmeta->class_values[nclass].description,
            STR_SIZE, "%s",
            (const char *) child_node->content);
        if (count < 0 || count >= STR_SIZE)
        {
            sprintf (errmsg,
                "Overflow of bmeta->class_values[nclass].description");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Increment the class count */
        nclass++;
    }

    /* Successful processing */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  add_band_metadata

PURPOSE: Add the current band element node to the current band metadata
structure and process the childrend of this node.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the band metadata elements
SUCCESS         Successful parse of the band metadata values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/27/2013   Gail Schmidt     Original development
2/25/2014    Gail Schmidt     Added support for source and category attributes

NOTES:
******************************************************************************/
int add_band_metadata
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
)
{
    char FUNC_NAME[] = "add_band_metadata";   /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlAttrPtr attr = NULL;       /* pointer to the element attributes */
    xmlNode *cur_node = NULL;     /* pointer to the current node */
    xmlNode *child_node = NULL;   /* pointer to the child node */
    xmlNsPtr ns = NULL;           /* pointer to the namespace */
    xmlChar *attr_val = NULL;     /* attribute value */
    int count;                    /* number of chars copied in snprintf */

    /* Set up the current and child pointers */
    cur_node = a_node;
    ns = cur_node->nsDef;

    /* Verify the namespace of this node is our ESPA namespace.  If it isn't
       then the element won't be added to the metadata structure. */
    if (!xmlStrEqual (ns->href, (const xmlChar *) ESPA_NS))
    {
        sprintf (errmsg, "Skipping %s since it is not in the ESPA namespace",
            cur_node->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (SUCCESS);
    }

    /* Handle the element attributes for the band element */
    for (attr = cur_node->properties; attr != NULL; attr = attr->next)
    {
        attr_val = xmlGetProp (cur_node, attr->name);
        if (xmlStrEqual (attr->name, (const xmlChar *) "product"))
        {
            count = snprintf (bmeta->product, sizeof (bmeta->product),
                "%s", (const char *) attr_val);
            if (count < 0 || count >= sizeof (bmeta->product))
            {
                sprintf (errmsg, "Overflow of bmeta->product string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "source"))
        {
            count = snprintf (bmeta->source, sizeof (bmeta->source),
                "%s", (const char *) attr_val);
            if (count < 0 || count >= sizeof (bmeta->source))
            {
                sprintf (errmsg, "Overflow of bmeta->source string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "name"))
        {
            count = snprintf (bmeta->name, sizeof (bmeta->name),
                "%s", (const char *) attr_val);
            if (count < 0 || count >= sizeof (bmeta->name))
            {
                sprintf (errmsg, "Overflow of bmeta->name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "category"))
        {
            count = snprintf (bmeta->category, sizeof (bmeta->category),
                "%s", (const char *) attr_val);
            if (count < 0 || count >= sizeof (bmeta->category))
            {
                sprintf (errmsg, "Overflow of bmeta->category string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "data_type"))
        {
            if (xmlStrEqual (attr_val, (const xmlChar *) "INT8"))
                bmeta->data_type = ESPA_INT8;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "UINT8"))
                bmeta->data_type = ESPA_UINT8;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "INT16"))
                bmeta->data_type = ESPA_INT16;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "UINT16"))
                bmeta->data_type = ESPA_UINT16;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "INT32"))
                bmeta->data_type = ESPA_INT32;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "UINT32"))
                bmeta->data_type = ESPA_UINT32;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "FLOAT32"))
                bmeta->data_type = ESPA_FLOAT32;
            else if (xmlStrEqual (attr_val, (const xmlChar *) "FLOAT64"))
                bmeta->data_type = ESPA_FLOAT64;
        }
        else if (xmlStrEqual (attr->name, (const xmlChar *) "nlines"))
            bmeta->nlines = atoi ((const char *) attr_val);
        else if (xmlStrEqual (attr->name, (const xmlChar *) "nsamps"))
            bmeta->nsamps = atoi ((const char *) attr_val);
        else if (xmlStrEqual (attr->name, (const xmlChar *) "fill_value"))
            bmeta->fill_value = atoi ((const char *) attr_val);
        else if (xmlStrEqual (attr->name,
            (const xmlChar *) "saturate_value"))
            bmeta->saturate_value = atoi ((const char *) attr_val);
        else if (xmlStrEqual (attr->name, (const xmlChar *) "scale_factor"))
            bmeta->scale_factor = atof ((const char *) attr_val);
        else if (xmlStrEqual (attr->name, (const xmlChar *) "add_offset"))
            bmeta->add_offset = atof ((const char *) attr_val);
        else
        {
            sprintf (errmsg, "WARNING: unknown attribute for element (%s): "
                "%s\n", cur_node->name, attr->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
        xmlFree (attr_val);
    }

    /* Process the children of this node; start with the first child then
       process it's siblings */
    for (cur_node = a_node->children; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        child_node = cur_node->children;
        if (xmlStrEqual (cur_node->name, (const xmlChar *) "short_name"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->short_name, sizeof (bmeta->short_name),
                "%s", (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->short_name))
            {
                sprintf (errmsg, "Overflow of bmeta->short_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "long_name"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->long_name, sizeof (bmeta->long_name),
                "%s", (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->long_name))
            {
                sprintf (errmsg, "Overflow of bmeta->long_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "file_name"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->file_name, sizeof (bmeta->file_name),
                "%s", (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->file_name))
            {
                sprintf (errmsg, "Overflow of bmeta->file_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "pixel_size"))
        {
            /* Handle the element attributes */
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                attr_val = xmlGetProp (cur_node, attr->name);
                if (xmlStrEqual (attr->name, (const xmlChar *) "x"))
                    bmeta->pixel_size[0] = atof ((const char *) attr_val);
                else if (xmlStrEqual (attr->name, (const xmlChar *) "y"))
                    bmeta->pixel_size[1] = atof ((const char *) attr_val);
                else if (xmlStrEqual (attr->name, (const xmlChar *) "units"))
                {
                    count = snprintf (bmeta->pixel_units,
                        sizeof (bmeta->pixel_units), "%s",
                        (const char *) attr_val);
                    if (count < 0 || count >= sizeof (bmeta->pixel_units))
                    {
                        sprintf (errmsg, "Overflow of bmeta->pixel_units");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
                else
                {
                    sprintf (errmsg, "WARNING: unknown attribute for element "
                        "(%s): %s\n", cur_node->name, attr->name);
                    error_handler (false, FUNC_NAME, errmsg);
                }
                xmlFree (attr_val);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "data_units"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->data_units, sizeof (bmeta->data_units),
                "%s", (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->data_units))
            {
                sprintf (errmsg, "Overflow of bmeta->data_units string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "valid_range"))
        {
            /* Handle the element attributes */
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                attr_val = xmlGetProp (cur_node, attr->name);
                if (xmlStrEqual (attr->name, (const xmlChar *) "min"))
                    bmeta->valid_range[0] = atol ((const char *) attr_val);
                else if (xmlStrEqual (attr->name, (const xmlChar *) "max"))
                    bmeta->valid_range[1] = atol ((const char *) attr_val);
                else
                {
                    sprintf (errmsg, "WARNING: unknown attribute for element "
                        "(%s): %s\n", cur_node->name, attr->name);
                    error_handler (false, FUNC_NAME, errmsg);
                }
                xmlFree (attr_val);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "toa_reflectance"))
        {
            /* Handle the element attributes */
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                attr_val = xmlGetProp (cur_node, attr->name);
                if (xmlStrEqual (attr->name, (const xmlChar *) "gain"))
                    bmeta->toa_gain = atof ((const char *) attr_val);
                else if (xmlStrEqual (attr->name, (const xmlChar *) "bias"))
                    bmeta->toa_bias = atof ((const char *) attr_val);
                else
                {
                    sprintf (errmsg, "WARNING: unknown attribute for element "
                        "(%s): %s\n", cur_node->name, attr->name);
                    error_handler (false, FUNC_NAME, errmsg);
                }
                xmlFree (attr_val);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "calibrated_nt"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            bmeta->calibrated_nt = atof ((const char *) child_node->content);
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "app_version"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->app_version, sizeof (bmeta->app_version),
                "%s", (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->app_version))
            {
                sprintf (errmsg, "Overflow of bmeta->app_version string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "production_date"))
        {
            /* Expect the child node to be a text node containing the value of
               this field */
            if (child_node == NULL || child_node->type != XML_TEXT_NODE) 
            {
                sprintf (errmsg, "Processing band metadata element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Copy the content of the child node into value for this field */
            count = snprintf (bmeta->production_date,
                sizeof (bmeta->production_date), "%s",
                (const char *) child_node->content);
            if (count < 0 || count >= sizeof (bmeta->production_date))
            {
                sprintf (errmsg, "Overflow of bmeta->production_date string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name,
            (const xmlChar *) "bitmap_description"))
        {
            if (add_band_metadata_bitmap_description (cur_node->children,
                bmeta) != SUCCESS)
            {
                sprintf (errmsg, "Processing bitmap_description element: "
                    "%s.", cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else if (xmlStrEqual (cur_node->name, (const xmlChar *) "class_values"))
        {
            if (add_band_metadata_class_values (cur_node->children, bmeta) !=
                SUCCESS)
            {
                sprintf (errmsg, "Processing class_values element: %s.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        else
        {
            sprintf (errmsg, "Unknown element (%s) in the band metadata",
                cur_node->name);
            error_handler (false, FUNC_NAME, errmsg);
        }
    } /* for siblings */

    return (SUCCESS);
}


/******************************************************************************
MODULE:  parse_xml_into_struct

PURPOSE: Parse the XML document data into the ESPA internal metadata structure.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the metadata elements
SUCCESS         Successful parse of the metadata values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
1. Uses a stack of character strings to keep track of the nodes that have
   been parsed.  The stack must be allocated before calling this routine.
******************************************************************************/
int parse_xml_into_struct
(
    xmlNode *a_node,                  /* I: pointer to the current node */
    Espa_internal_meta_t *metadata,   /* I: ESPA internal metadata structure
                                            to be filled */
    int *top_of_stack,                /* I: pointer to top of the stack */
    char **stack                      /* I: stack to use for parsing */
)
{
    char FUNC_NAME[] = "parse_xml_into_struct";  /* function name */
    char errmsg[STR_SIZE];        /* error message */
    char *curr_stack_element = NULL;  /* element popped from the stack */
    xmlNode *cur_node = NULL;    /* pointer to the current node */
    xmlNode *sib_node = NULL;    /* pointer to the sibling node */
    static int nbands = 0;       /* number of bands in the XML structure */
    static bool global_metadata = false;  /* are we parsing the global metadata
                                    section of the ESPA metadata? */
    static bool bands_metadata = false;   /* are we parsing the bands metadata
                                    section of the ESPA metadata? */
    static int cur_band = 0;     /* current band being processed in the
                                    bands metadata section */
    bool skip_child;             /* boolean to specify the children of this
                                    node should not be processed */

    /* Start at the input node and traverse the tree, visiting all the children
       and siblings */
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Process the children of this element unless otherwise specified */
        skip_child = false;

        /* Only print the ELEMENT node types */
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
            /* Push the element to the stack and turn the booleans on if this
               is either the global_metadata or the bands elements */
            //printf ("***Pushed %s\n", cur_node->name); fflush (stdout);
            if (push (top_of_stack, stack, (const char *) cur_node->name))
            {
                sprintf (errmsg, "Pushing element '%s' to the stack.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Turn the boolean on if this is the global_metadata. Flag an
               issue if we have already traversed into the global metadata
               section. */
            if (xmlStrEqual (cur_node->name,
                (const xmlChar *) "global_metadata"))
            {
                if (global_metadata)
                {
                    sprintf (errmsg, "Current element node is '%s' however we "
                        "are already in the global_metadata section.",
                        cur_node->name);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
                global_metadata = true;
            }

            /* Turn the boolean on if this is the bands metadata. Flag an
               issue if we have already traversed into the bands section.
               Count the number of band elements in this structure, then
               allocate memory for the nbands. */
            if (xmlStrEqual (cur_node->name, (const xmlChar *) "bands"))
            {
                if (bands_metadata)
                {
                    sprintf (errmsg, "Current element node is '%s' however we "
                        "are already in the bands section.", cur_node->name);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
                bands_metadata = true;
                cur_band = 0;  /* reset to zero for start of band count */

                /* Count the number of siblings which are band elements */
                nbands = 0;
                for (sib_node = cur_node->children; sib_node;
                     sib_node = xmlNextElementSibling (sib_node))
                {
                    /* If this is a band element then count it */
                    if (xmlStrEqual (sib_node->name, (const xmlChar *) "band"))
                        nbands++;
                }

                if (allocate_band_metadata (metadata, nbands) != SUCCESS)
                {   /* Error messages already printed */
                    return (ERROR);
                }
            }

            /* Print out the name of the element */
            xmlAttrPtr attr;     /* pointer to the element attributes */
            //printf ("node type: Element, name: %s\n", cur_node->name);

            /* Print out the attribute properties for this element */
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                xmlChar *v = xmlGetProp (cur_node, attr->name);
                //printf (" @%s=%s ", attr->name, v);
                xmlFree (v);
            }
            //printf ("\n"); fflush (stdout);

            /* If we are IN the global metadata (don't process the actual
               global_metadata element) then consume this node and add the
               information to the global metadata structure */
            if (global_metadata && !xmlStrEqual (cur_node->name,
                (const xmlChar *) "global_metadata"))
            {
                if (add_global_metadata (cur_node, &metadata->global))
                {
                    sprintf (errmsg, "Consuming global_metadata element '%s'.",
                        cur_node->name);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                /* Skip processing the children of this node, since they
                   will be handled by the global metadata parser */
                skip_child = true;
            }

            /* If we are IN the bands metadata and at a band element, then
               consume this node and add the information to the band metadata
               structure for the current band */
            if (bands_metadata && xmlStrEqual (cur_node->name,
                (const xmlChar *) "band"))
            {
                if (cur_band >= nbands)
                {
                    sprintf (errmsg, "Number of bands consumed already "
                        "reached the total number of bands allocated for this "
                        "XML file (%d).", nbands);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                if (add_band_metadata (cur_node, &metadata->band[cur_band++]))
                {
                    sprintf (errmsg, "Consuming band metadata element '%s'.",
                        cur_node->name);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                /* Skip processing the children of this node, since they
                   will be handled by the global metadata parser */
                skip_child = true;
            }
        }
        else if (cur_node->type == XML_TEXT_NODE) 
        {
            /* Print out the text for the element */
            //printf ("   node type: Text, content: %s\n", cur_node->content);
        }

        /* Parse the children of this node if they haven't been consumed
           elsewhere */
        if (!skip_child)
        {
            if (parse_xml_into_struct (cur_node->children, metadata,
                top_of_stack, stack))
            {
                sprintf (errmsg, "Parsing the children of this element '%s'.",
                    cur_node->name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }

        /* Done with the element and its siblings so pop the element name off
           the stack */
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            curr_stack_element = pop (top_of_stack, stack);
            if (curr_stack_element == NULL)
            {
                sprintf (errmsg, "Popping elements off the stack.");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            //printf ("***Popped %s\n", curr_stack_element); fflush (stdout);

            if (!strcmp (curr_stack_element, "global_metadata"))
                global_metadata = false;
            if (!strcmp (curr_stack_element, "bands"))
                bands_metadata = false;
        }
    }  /* for cur_node */

    return (SUCCESS);
}


/******************************************************************************
MODULE:  parse_metadata

PURPOSE: Parse the input metadata file and populate the associated ESPA
internal metadata file.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error parsing the metadata elements
SUCCESS         Successful parse of the metadata values

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development

NOTES:
1. Uses a stack of character strings to keep track of the nodes that have beend
   found in the metadata document.
2. For debugging purposes
   xmlDocDump (stderr, doc);
   can be used to dump/print the XML doc to the screen.
******************************************************************************/
int parse_metadata
(
    char *metafile,                 /* I: input metadata file or URL */
    Espa_internal_meta_t *metadata  /* I: input metadata structure which has
                                          been initialized via
                                          init_metadata_struct */
)
{
    char FUNC_NAME[] = "parse_metadata";  /* function name */
    char errmsg[STR_SIZE];        /* error message */
    xmlTextReaderPtr reader;  /* reader for the XML file */
    xmlDocPtr doc = NULL;     /* document tree pointer */
    xmlNodePtr current=NULL;  /* pointer to the current node */
    int status;               /* return status */
    int nodeType;             /* node type (element, text, attribute, etc.) */
    int top_of_stack;         /* top of the stack */
    int count;                /* number of chars copied in snprintf */
    char **stack = NULL;      /* stack to keep track of elements in the tree */

    /* Establish the reader for this metadata file */
    reader = xmlNewTextReaderFilename (metafile);
    if (reader == NULL)
    {
        sprintf (errmsg, "Setting up reader for %s", metafile);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Use the reader to parse the XML file, looking at each of the nodes,
       until the entire file has been parsed.  Start by reading the first
       node in the file. */
    status = xmlTextReaderRead (reader);
    while (status == 1)
    {
        /* Determine what kind of node the reader is at (element, end element,
           attribute, text/white space) and handle the information as desired */
        nodeType = xmlTextReaderNodeType (reader);
        if (nodeType == -1)
        {
            sprintf (errmsg, "Getting node type");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        switch (nodeType)
        {
            case XML_READER_TYPE_ELEMENT:
            {  /* Node is an element (ex. <global_metadata> */
                xmlNodePtr node=NULL;
                if (doc==NULL)
                {
                    doc=xmlNewDoc (BAD_CAST "1.0");
                }

                /* Get the URI defining the namespace for this node */
                if (xmlTextReaderConstNamespaceUri (reader) != NULL)
                {
                    /* Search the namespace for a document */
                    xmlNsPtr ns = xmlSearchNs (doc, current,
                        xmlTextReaderConstNamespaceUri(reader));

                    /* Create a tree node for this element in the XML file
                       using the element name */
                    node = xmlNewNode (ns, xmlTextReaderConstName(reader));

                    /* If the namespace is empty (i.e. root) then create a
                       new namespace pointer with this node */
                    if (ns == NULL)
                    {
                        ns = xmlNewNs (node,
                            xmlTextReaderConstNamespaceUri(reader),
                            xmlTextReaderConstPrefix(reader));
                    }
                }
                else
                {
                    /* Create a tree node for this element in the XML file
                       using the element name */
                    node = xmlNewNode (0, xmlTextReaderConstName(reader));
                }

                /* Set the element as the root if appropriate otherwise add
                   it as a child to the previous element */
                if (current == NULL)
                {
                    xmlDocSetRootElement (doc, node);
                }
                else
                {
                    xmlAddChild (current, node);
                }
                current = node;

                /* If the element has attributes, then handle them */
                if (xmlTextReaderHasAttributes (reader))
                {
                    /* Get the number of attributes and then process each one */
                    int i;
                    int n_att = xmlTextReaderAttributeCount (reader);
                    for (i = 0; i < n_att; i++)
                    {
                        /* Read each attribute, obtain the name and value,
                           then add it as a property for this node in the
                           tree */
                        const xmlChar *k = NULL;
                        xmlChar *v = NULL;
                        xmlTextReaderMoveToAttributeNo (reader, i);
                        k = xmlTextReaderConstName (reader);
                        v = xmlTextReaderValue (reader);
                        if (xmlTextReaderConstNamespaceUri (reader) != NULL)
                        {
                            if (!xmlStrEqual (
                                xmlTextReaderConstNamespaceUri(reader),
                                BAD_CAST "http://www.w3.org/2000/xmlns/"))
                            {
                                /* Search the namespace for the document */
                                xmlNsPtr ns = xmlSearchNs (doc, current,
                                    xmlTextReaderConstNamespaceUri(reader));
                                if (ns == NULL)
                                {
                                    ns = xmlNewNs (node,
                                        xmlTextReaderConstNamespaceUri(reader),
                                        xmlTextReaderConstPrefix(reader));
                                }

                                /* Create a new property tagged with this
                                   namespace and carried by this node */
                                xmlNewNsProp (current, ns,
                                    xmlTextReaderConstLocalName(reader), v);
                            }
                         }
                         else
                         {
                            /* Add the attribute as a property of the node
                               in the tree */
                            xmlNewProp (current, k, v);
                         }

                         /* Free the XML value pointer */
                         xmlFree (v);
                    }

                    /* We are done with the attributes so go to the current
                       attribute node */
                    xmlTextReaderMoveToElement (reader);
                }

                /* If this is an empty element, then return to the parent */
                if (xmlTextReaderIsEmptyElement(reader))
                    current = current->parent;
                break;
            }  /* End: Node is an element */

            case XML_READER_TYPE_END_ELEMENT:
            {  /* Node is an end element (ex. </global_metadata>, so return
                  to the parent */
                current = current->parent;
                break;
            }

            case XML_READER_TYPE_TEXT:
            {  /* Node is text or white space */
                /* Read the value of the text and add it as text for the
                   node, which is then added as a child to the tree */
                const xmlChar *v = xmlTextReaderConstValue (reader);
                xmlNodePtr node = xmlNewDocText (doc, v);
                xmlAddChild (current, node);
                break;
            }
        }  /* end switch */

        /* Read the next node */
        status = xmlTextReaderRead (reader);
    }  /* end while */
    if (status != 0)
    {
        sprintf (errmsg, "Failed to parse %s", metafile);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* If the document tree is not NULL then send in the root node of the
       tree to be parsed and read into the ESPA metadata structure */
    if (doc != NULL)
    {
        /* Store the namespace for the overall metadata file */
        xmlNsPtr ns = xmlDocGetRootElement(doc)->nsDef;
        count = snprintf (metadata->meta_namespace,
            sizeof (metadata->meta_namespace), "%s", (const char *) ns->href);
        if (count < 0 || count >= sizeof (metadata->meta_namespace))
        {
            sprintf (errmsg, "Overflow of metadata->meta_namespace string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Initialize the stack to hold the elements */
        if (init_stack (&top_of_stack, &stack))
        {
            sprintf (errmsg, "Initializing the stack.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        //print_element_names (xmlDocGetRootElement (doc));

        /* Parse the XML document into our ESPA internal metadata structure */
        if (parse_xml_into_struct (xmlDocGetRootElement(doc), metadata,
            &top_of_stack, stack))
        {
            sprintf (errmsg, "Parsing the metadata file into the internal "
                "metadata structure.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Clean up the XML document and the stack */
        xmlFreeDoc (doc);
        free_stack (&stack);
    }

    /* Free the reader and associated memory */
    xmlFreeTextReader (reader);
    xmlCleanupParser();
    xmlMemoryDump();

    return (SUCCESS);
}

