/*****************************************************************************
FILE: espa_metadata.c
  
PURPOSE: Contains functions for reading/writing/appending the ESPA internal
metadata files along with inializing/freeing memory in the metadata structures.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/13/2013   Gail Schmidt     Original development

NOTES:
  1. The XML metadata format parsed or written via this library follows the
     ESPA internal metadata format found in ESPA Raw Binary Format v1.0.doc.
     The schema for the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
  2. This code relies on the libxml2 library developed for the Gnome project.
*****************************************************************************/

#include "espa_metadata.h"

/******************************************************************************
MODULE:  validate_xml_file

PURPOSE:  Validates the specified XML file with the specified schema file/URL.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           XML does not validate against the specified schema
SUCCESS         XML validates

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/13/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int validate_xml_file
(
    char *meta_file,          /* I: name of metadata file to be validated */
    char *schema_file         /* I: name of schema file or URL to be validated
                                    against */
)
{
    char FUNC_NAME[] = "validate_xml_file";   /* function name */
    char errmsg[STR_SIZE];        /* error message */
    int status;                   /* return status */
    xmlDocPtr doc = NULL;         /* resulting document tree */
    xmlSchemaPtr schema = NULL;   /* pointer to the schema */
    xmlSchemaParserCtxtPtr ctxt = NULL;  /* parser context for the schema */
    xmlSchemaValidCtxtPtr valid_ctxt = NULL;  /* pointer to validate from the
                                                 schema */

    /* Set up the schema parser and parse the schema file/URL */
    xmlLineNumbersDefault (1);
    ctxt = xmlSchemaNewParserCtxt (schema_file);
    xmlSchemaSetParserErrors (ctxt, (xmlSchemaValidityErrorFunc) fprintf,
        (xmlSchemaValidityWarningFunc) fprintf, stderr);
    schema = xmlSchemaParse (ctxt);

    /* Free the schema parser context */
    xmlSchemaFreeParserCtxt (ctxt);

    /* Load the XML file and parse it to the document tree */
    doc = xmlReadFile (meta_file, NULL, 0);
    if (doc == NULL)
    {
        sprintf (errmsg, "Could not parse %s", meta_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Identify the schema file as the validation source */
    valid_ctxt = xmlSchemaNewValidCtxt (schema);
    xmlSchemaSetValidErrors (valid_ctxt, (xmlSchemaValidityErrorFunc) fprintf,
        (xmlSchemaValidityWarningFunc) fprintf, stderr);

    /* Validate the XML metadata against the schema */
    status = xmlSchemaValidateDoc (valid_ctxt, doc);
    if (status > 0)
    {
        sprintf (errmsg, "%s fails to validate", meta_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    else if (status != 0)
    {
        sprintf (errmsg, "%s validation generated an internal error",
            meta_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Free the resources and clean up the memory */
    xmlSchemaFreeValidCtxt (valid_ctxt);
    xmlFreeDoc (doc);
    if (schema != NULL)
        xmlSchemaFree (schema);
    xmlSchemaCleanupTypes();
    xmlCleanupParser();   /* cleanup the XML library */
    xmlMemoryDump();      /* for debugging */

    /* Successful completion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  init_metadata_struct

PURPOSE:  Initializes the ESPA internal metadata structure, particularly the
pointers within each sub-structure.  Assigns field values to fill to make it
easier to detect if the values were parsed from the input metadata file or
assigned by the user.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/18/2013   Gail Schmidt     Original development
5/7/2014     Gail Schmidt     Updated for modis tiles

NOTES:
******************************************************************************/
void init_metadata_struct
(
    Espa_internal_meta_t *internal_meta   /* I: pointer to internal metadata
                                                structure to be initialized */
)
{
    Espa_global_meta_t *gmeta = &internal_meta->global;
                                 /* pointer to the global metadata structure */

    /* Initialze the number of bands */
    internal_meta->nbands = 0;
    internal_meta->band = NULL;

    /* Initialize the global metadata values to fill for use by the write
       metadata routines */
    strcpy (gmeta->data_provider, ESPA_STRING_META_FILL);
    strcpy (gmeta->satellite, ESPA_STRING_META_FILL);
    strcpy (gmeta->instrument, ESPA_STRING_META_FILL);
    strcpy (gmeta->acquisition_date, ESPA_STRING_META_FILL);
    strcpy (gmeta->scene_center_time, ESPA_STRING_META_FILL);
    strcpy (gmeta->level1_production_date, ESPA_STRING_META_FILL);
    gmeta->solar_zenith = ESPA_FLOAT_META_FILL;
    gmeta->solar_azimuth = ESPA_FLOAT_META_FILL;
    strcpy (gmeta->solar_units, ESPA_STRING_META_FILL);
    gmeta->wrs_system = ESPA_INT_META_FILL;
    gmeta->wrs_path = ESPA_INT_META_FILL;
    gmeta->wrs_row = ESPA_INT_META_FILL;
    gmeta->htile = ESPA_INT_META_FILL;
    gmeta->vtile = ESPA_INT_META_FILL;
    strcpy (gmeta->lpgs_metadata_file, ESPA_STRING_META_FILL);
    gmeta->ul_corner[0] = gmeta->ul_corner[1] = ESPA_FLOAT_META_FILL;
    gmeta->lr_corner[0] = gmeta->lr_corner[1] = ESPA_FLOAT_META_FILL;
    gmeta->bounding_coords[0] = ESPA_FLOAT_META_FILL;
    gmeta->bounding_coords[1] = ESPA_FLOAT_META_FILL;
    gmeta->bounding_coords[2] = ESPA_FLOAT_META_FILL;
    gmeta->bounding_coords[3] = ESPA_FLOAT_META_FILL;
    gmeta->proj_info.proj_type = ESPA_INT_META_FILL;
    gmeta->proj_info.datum_type = ESPA_NODATUM;
    gmeta->orientation_angle = ESPA_FLOAT_META_FILL;
}


/******************************************************************************
MODULE:  allocate_band_metadata

PURPOSE:  Allocates memory in the ESPA internal metadata structure for nbands.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error allocating memory for the nbands
SUCCESS         Successfully allocated memory

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/18/2013   Gail Schmidt     Original development
2/25/2014    Gail Schmidt     Added support for source and category attributes
                              for the band metadata

NOTES:
  1. Initializes the bitmap_description and class_values for each band to NULL
     and sets the nbits and nclass to 0.
******************************************************************************/
int allocate_band_metadata
(
    Espa_internal_meta_t *internal_meta,  /* I: pointer to internal metadata
                                                structure */
    int nbands                            /* I: number of bands to allocate
                                                for the band field in the
                                                internal_meta */
)
{
    char FUNC_NAME[] = "allocate_band_metadata";   /* function name */
    char errmsg[STR_SIZE];          /* error message */
    Espa_band_meta_t *bmeta = NULL; /* pointer to array of bands metadata */
    int i;                          /* looping variable */

    /* Allocate the number of bands to nbands and the associated pointers */
    internal_meta->nbands = nbands;
    internal_meta->band = calloc (nbands, sizeof (Espa_band_meta_t));
    if (internal_meta->band == NULL)
    {
        sprintf (errmsg, "Allocating ESPA band metadata for %d bands", nbands);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    bmeta = internal_meta->band;

    /* Set the nbits and nclass fields in the band metadata to 0 for each band
       and initialize the pointers to NULL.  Initialize the other fields to
       fill to make it easy to distinguish if they were populated by reading
       an input metadata file or assigned directly. */
    for (i = 0; i < nbands; i++)
    {
        bmeta[i].nbits = 0;
        bmeta[i].bitmap_description = NULL;
        bmeta[i].nclass = 0;
        bmeta[i].class_values = NULL;

        strcpy (bmeta[i].product, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].source, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].name, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].category, ESPA_STRING_META_FILL);
        bmeta[i].data_type = ESPA_UINT8;
        bmeta[i].nlines = ESPA_INT_META_FILL;
        bmeta[i].nsamps = ESPA_INT_META_FILL;
        bmeta[i].fill_value = ESPA_INT_META_FILL;
        bmeta[i].saturate_value = ESPA_INT_META_FILL;
        bmeta[i].scale_factor = ESPA_FLOAT_META_FILL;
        bmeta[i].add_offset = ESPA_FLOAT_META_FILL;
        strcpy (bmeta[i].short_name, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].long_name, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].file_name, ESPA_STRING_META_FILL);
        bmeta[i].pixel_size[0] = bmeta[i].pixel_size[1] = ESPA_FLOAT_META_FILL;
        strcpy (bmeta[i].pixel_units, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].data_units, ESPA_STRING_META_FILL);
        bmeta[i].valid_range[0] = bmeta[i].valid_range[1] = ESPA_INT_META_FILL;
        bmeta[i].toa_gain = ESPA_INT_META_FILL;
        bmeta[i].toa_bias = ESPA_INT_META_FILL;
        bmeta[i].calibrated_nt = ESPA_FLOAT_META_FILL;
        strcpy (bmeta[i].qa_desc, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].app_version, ESPA_STRING_META_FILL);
        strcpy (bmeta[i].production_date, ESPA_STRING_META_FILL);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  allocate_class_metadata

PURPOSE:  Allocates memory in the ESPA band metadata structure for nclasses.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error allocating memory for nclasses
SUCCESS         Successfully allocated memory

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/18/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int allocate_class_metadata
(
    Espa_band_meta_t *band_meta,  /* I: pointer to band metadata structure */
    int nclass                    /* I: number of classes to allocate for the
                                        band metadata */
)
{
    char FUNC_NAME[] = "allocate_class_metadata";   /* function name */
    char errmsg[STR_SIZE];        /* error message */

    /* Allocate the number of classes to nclass and the associated class_values
       pointer */
    band_meta->nclass = nclass;
    band_meta->class_values = calloc (nclass, sizeof (Espa_class_t));
    if (band_meta->class_values == NULL)
    {
        sprintf (errmsg, "Allocating ESPA band metadata for %d nclasses",
            nclass);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  allocate_bitmap_metadata

PURPOSE:  Allocates memory in the ESPA band metadata structure for nbits.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error allocating memory for nbits
SUCCESS         Successfully allocated memory

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/18/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
int allocate_bitmap_metadata
(
    Espa_band_meta_t *band_meta,  /* I: pointer to band metadata structure */
    int nbits                     /* I: number of bits to allocate for the
                                        bitmap metadata */
)
{
    char FUNC_NAME[] = "allocate_bitmap_metadata";   /* function name */
    char errmsg[STR_SIZE];        /* error message */
    int i;                        /* looping variable */

    /* Allocate the number of bits to nbits and the associated bitmap pointer */
    band_meta->nbits = nbits;
    band_meta->bitmap_description = calloc (nbits, sizeof (char *));
    if (band_meta->bitmap_description == NULL)
    {
        sprintf (errmsg, "Allocating ESPA bitmap description");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    for (i = 0; i < nbits; i++)
    {
        band_meta->bitmap_description[i] = calloc (STR_SIZE, sizeof (char));
        if (band_meta->bitmap_description[i] == NULL)
        {
            sprintf (errmsg, "Allocating ESPA band metadata for %d nbits",
                nbits);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  free_metadata

PURPOSE:  Frees memory in the ESPA internal metadata structure.

RETURN VALUE: N/A

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/19/2013   Gail Schmidt     Original development

NOTES:
******************************************************************************/
void free_metadata
(
    Espa_internal_meta_t *internal_meta   /* I: pointer to internal metadata
                                                structure */
)
{
    int i, b;                      /* looping variables */

    /* Free the pointers in the band metadata */
    for (i = 0; i < internal_meta->nbands; i++)
    {
        if (internal_meta->band[i].nbits > 0)
        {
            for (b = 0; b < internal_meta->band[i].nbits; b++)
                free (internal_meta->band[i].bitmap_description[b]);
            free (internal_meta->band[i].bitmap_description);
        }

        free (internal_meta->band[i].class_values);
    }

    /* Free the band pointer itself */
    if (internal_meta->band)
        free (internal_meta->band);
}


/******************************************************************************
MODULE:  print_element_names

PURPOSE:  Print the information for the elements in the document tree,
starting at the node provided.

RETURN VALUE:  N/A

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/23/2013   Gail Schmidt     Original development

NOTES:
  1. Prints to stdout.
******************************************************************************/
void print_element_names
(
    xmlNode *a_node   /* I: pointer to the current node in the tree to start
                            printing */
)
{
    xmlNode *cur_node = NULL;   /* pointer to the current node */

    /* Start at the input node and traverse the tree, visiting all the children
       and siblings */
    for (cur_node = a_node; cur_node;
         cur_node = xmlNextElementSibling (cur_node))
    {
        /* Only print the ELEMENT node types */
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
            /* Print out the name of the element */
            xmlAttrPtr attr;     /* pointer to the element attributes */
            printf ("node type: Element, name: %s", cur_node->name);

            /* Print out the namespace info as well */
            xmlNsPtr ns = cur_node->nsDef;
            while (ns != 0)
            {
                printf (" with namespace: %s %p\n", ns->href, ns->prefix);
                ns = ns->next;
            }
            printf("\n");

            /* Print out the attribute properties for this element */
            for (attr = cur_node->properties; attr != NULL; attr = attr->next)
            {
                xmlChar *v = xmlGetProp (cur_node, attr->name);
                if (attr->ns != NULL)
                {
                    if (attr->ns->prefix != NULL)
                    {
                        printf (" with namespace: %s %p\n", attr->ns->href,
                            attr->ns->prefix);
                    }
                    else
                    {
                        printf (" with namespace: %s\n", attr->ns->href);
                    }
                }
                printf (" @%s=%s ", attr->name, v);
                xmlFree (v);
            }
            printf ("\n");
        }
        else if (cur_node->type == XML_TEXT_NODE) 
        {
            /* Print out the text for the element */
            printf ("   node type: Text, content: %s\n", cur_node->content);
        }

        print_element_names (cur_node->children);
    }
}

