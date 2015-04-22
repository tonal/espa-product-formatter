/*****************************************************************************
FILE: create_land_water_mask
  
PURPOSE: Creates the land/water mask for the current scene.  The land/water
mask is generated from a static land-mass polygon.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/17/2015     Gail Schmidt     Original development

NOTES:
*****************************************************************************/
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "error_handler.h"
#include "envi_header.h"
#include "parse_metadata.h"
#include "write_metadata.h"
#include "raw_binary_io.h"
#include "generate_land_water_mask.h"

/******************************************************************************
MODULE: usage

PURPOSE: Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
4/17/2015     Gail Schmidt     Original development

NOTES:
******************************************************************************/
void usage ()
{
    printf ("create_land_water_mask creates the land/water mask for the "
            "input scene, based on a static land-mass polygon.\n\n");
    printf ("usage: create_land_water_mask "
            "--xml=input_metadata_filename\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the input XML metadata file which follows "
            "the ESPA internal raw binary schema\n");
    printf ("\nExample: create_land_water_mask "
            "--xml=LC80470272013287LGN00.xml\n");
}


/******************************************************************************
MODULE:  get_args

PURPOSE:  Gets the command-line arguments and validates that the required
arguments were specified.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error getting the command-line arguments or a command-line
                argument and associated value were not specified
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/17/2015     Gail Schmidt     Original development

NOTES:
  1. Memory is allocated for the input and output files.  All of these should
     be character pointers set to NULL on input.  The caller is responsible
     for freeing the allocated memory upon successful return.
******************************************************************************/
short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **xml_infile     /* O: address of input XML filename */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"xml", required_argument, 0, 'i'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* Loop through all the cmd-line options */
    opterr = 0;   /* turn off getopt_long error msgs as we'll print our own */
    while (1)
    {
        /* optstring in call to getopt_long is empty since we will only
           support the long options */
        c = getopt_long (argc, argv, "", long_options, &option_index);
        if (c == -1)
        {   /* Out of cmd-line options */
            break;
        }

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
     
            case 'h':  /* help */
                usage ();
                return (ERROR);
                break;

            case 'i':  /* XML infile */
            *xml_infile = strdup (optarg);
            break;

            case '?':
            default:
                sprintf (errmsg, "Unknown option %s", argv[optind-1]);
                error_handler (true, FUNC_NAME, errmsg);
                usage ();
                return (ERROR);
                break;
        }
    }

    /* Make sure the infiles and outfiles were specified */
    if (*xml_infile == NULL)
    {
        sprintf (errmsg, "XML input file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    return (SUCCESS);
}


#define MAX_DATE_LEN 28
/******************************************************************************
MODULE:  main

PURPOSE: Creates the land/water mask for the current scene and write it to the
output land/water mask file.  The land/water mask is generated from a static
land-mass polygon.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error creating the land/water mask
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/17/2015     Gail Schmidt     Original development
4/22/2015     Gail Schmidt     Make this band an intermediate_data product
                               since it won't be delivered

NOTES:
  1. The ESPA_LAND_MASS_POLYGON environment variable needs to be defined and
     contain the full path and filename of the land-mass polygon to be used
     to generate the land/water mask. It is recommended the land_no_buf.ply
     polygon is used, which is delivered with this source code.
  2. The land/water mask filename is the same as band 1 in the input XML file
     with the _B1.img replaced with _land_water_mask.img.
******************************************************************************/
int main (int argc, char** argv)
{
    char FUNC_NAME[] = "create_land_water_mask";  /* function name */
    char errmsg[STR_SIZE];       /* error message */
    char tmpstr[STR_SIZE];       /* temporary filename */
    char maskfile[STR_SIZE];     /* output land/water mask filename */
    char production_date[MAX_DATE_LEN+1]; /* current date/time for production */
    char *land_mass_polygon = NULL; /* filename of the land-mass polygon */
    char *espa_xml_file = NULL;  /* input ESPA XML metadata filename */
    char *cptr = NULL;           /* character pointer for the '_' in filename */
    int i;                       /* looping variable */
    int nlines;                  /* number of lines in the land/water mask */
    int nsamps;                  /* number of samples in the land/water mask */
    int refl_indx = -99;         /* index of band1 or first band */
    unsigned char *land_water_mask = NULL;  /* land/water mask buffer */
    time_t tp;                   /* time structure */
    struct tm *tm = NULL;        /* time structure for UTC time */
    FILE *fptr=NULL;             /* file pointer */
    Envi_header_t envi_hdr;      /* output ENVI header information */
    Espa_global_meta_t *gmeta = NULL; /* pointer to global metadata structure */
    Espa_band_meta_t *bmeta = NULL;   /* pointer to band metadata structure */
    Espa_band_meta_t *out_bmeta = NULL;/* band metadata for land-water mask */
    Espa_internal_meta_t out_meta;    /* output metadata for land-water mask */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure to be
                                populated by reading the MTL metadata file */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &espa_xml_file) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (ERROR);
    }

    /* Get the ESPA land/water mask environment variable which specifies the
       location of the land-mass polygon to be used */
    land_mass_polygon = getenv ("ESPA_LAND_MASS_POLYGON");
    if (land_mass_polygon == NULL)
    {
        sprintf (errmsg, "ESPA_LAND_MASS_POLYGON environment variable is "
            "not defined. Define the environment variable to contain the "
            "full path and filename of the land-mass polygon to be used "
            "to generate the land/water mask.\n");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
    printf ("Using land-mass polygon file: %s\n", land_mass_polygon);

    /* Validate the input metadata file */
    if (validate_xml_file (espa_xml_file) != SUCCESS)
    {  /* Error messages already written */
        exit (ERROR);
    }

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (espa_xml_file, &xml_metadata) != SUCCESS)
    {  /* Error messages already written */
        exit (ERROR);
    }
    gmeta = &xml_metadata.global;

    /* Generate the land/water mask for this scene. Memory is allocated for
       the land/water mask. */
    if (generate_land_water_mask (&xml_metadata, land_mass_polygon,
        &land_water_mask, &nlines, &nsamps) != SUCCESS)
    {  /* Error messages already written */
        exit (ERROR);
    }

    /* Use band 1 as the representative band in the XML */
    for (i = 0; i < xml_metadata.nbands; i++)
    {
        if (!strcmp (xml_metadata.band[i].name, "band1"))
        {
            /* this is the index we'll use for reflectance band info */
            refl_indx = i;
            break;
        }
    }

    /* Make sure the representative band was found in the XML file */
    if (refl_indx == -9)
    {
        sprintf (errmsg, "Band 1 (band1) was not found in the XML file");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Make sure the band 1 number of lines and samples matches what was used
       for creating the land/water mask, otherwise we will have a mismatch
       in the resolution and output XML information. */
    bmeta = &xml_metadata.band[refl_indx];
    if (nlines != bmeta->nlines || nsamps != bmeta->nsamps)
    {
        sprintf (errmsg, "Band 1 from this application does not match band 1 "
            "from the generate_land_water_mask function call.  Local nlines/"
            "nsamps: %d, %d   Returned nlines/nsamps: %d, %d", bmeta->nlines,
            bmeta->nsamps, nlines, nsamps);
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Initialize the output metadata structure.  The global metadata will
       not be used and will not be valid. */
    init_metadata_struct (&out_meta);

    /* Allocate memory for a single output band */
    if (allocate_band_metadata (&out_meta, 1) != SUCCESS)
    {
        sprintf (errmsg, "Cannot allocate memory for the land/water mask band");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
    out_bmeta = &out_meta.band[0];

    /* Set up the band metadata for the land/water mask */
    strcpy (out_bmeta->product, "intermediate_data");
    strcpy (out_bmeta->source, "level1");
    strcpy (out_bmeta->name, "land_water_mask");
    strcpy (out_bmeta->category, "qa");
    out_bmeta->data_type = ESPA_UINT8;
    out_bmeta->nlines = nlines;
    out_bmeta->nsamps = nsamps;
    strncpy (tmpstr, bmeta->short_name, 3);
    sprintf (out_bmeta->short_name, "%sLWMASK", tmpstr);
    strcpy (out_bmeta->long_name, "static land/water mask");
    out_bmeta->pixel_size[0] = bmeta->pixel_size[0];
    out_bmeta->pixel_size[1] = bmeta->pixel_size[1];
    strcpy (out_bmeta->pixel_units, bmeta->pixel_units);
    strcpy (out_bmeta->data_units, "quality/feature classification");
    out_bmeta->valid_range[0] = 0;
    out_bmeta->valid_range[1] = 1;
    sprintf (out_bmeta->app_version, "create_land_water_mask_%s",
        ESPA_COMMON_VERSION);

    /* Use the band1 filename to create the land/mask filename */
    strcpy (out_bmeta->file_name, bmeta->file_name);
    cptr = strchr (out_bmeta->file_name, '_');
    if (!cptr)
    {
        sprintf (errmsg, "Unable to find the _ in the band 1 filename for "
            "creating the land/water mask filename.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
    sprintf (cptr, "_land_water_mask.img");

    /* Set up the 2 classes for land (1) and water (0) */
    out_bmeta->nclass = 2;
    if (allocate_class_metadata (out_bmeta, 2) != SUCCESS)
    {
        sprintf (errmsg, "Cannot allocate memory for the classes");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
    out_bmeta->class_values[0].class = 0;
    out_bmeta->class_values[1].class = 1;
    strcpy (out_bmeta->class_values[0].description, "water");
    strcpy (out_bmeta->class_values[1].description, "land");

    /* Get the current date/time (UTC) for the production date of each band */
    if (time (&tp) == -1)
    {
        sprintf (errmsg, "Unable to obtain the current time.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
  
    tm = gmtime (&tp);
    if (tm == NULL)
    {
        sprintf (errmsg, "Converting time to UTC.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
  
    if (strftime (production_date, MAX_DATE_LEN, "%Y-%m-%dT%H:%M:%SZ", tm) == 0)
    {
        sprintf (errmsg, "Formatting the production date/time.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }
    strcpy (out_bmeta->production_date, production_date);

    /* Write the land/water mask file */
    strcpy (maskfile, out_bmeta->file_name);
    fptr = open_raw_binary (maskfile, "wb");
    if (!fptr)
    {
        sprintf (errmsg, "Unable to open the land/water mask file");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Write the data for this band */
    if (write_raw_binary (fptr, nlines, nsamps, sizeof (unsigned char),
        land_water_mask) != SUCCESS)
    {
        sprintf (errmsg, "Unable to write to the land/water mask file");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Close the file and free the pointer for this band */
    close_raw_binary (fptr);
    free (land_water_mask);

    /* Create the ENVI header using the representative band */
    if (create_envi_struct (out_bmeta, gmeta, &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Error creating the ENVI header file.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Write the ENVI header */
    sprintf (tmpstr, "%s", maskfile);
    sprintf (&tmpstr[strlen(tmpstr)-3], "hdr");
    if (write_envi_hdr (tmpstr, &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Writing the ENVI header file: %s.", tmpstr);
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Append the land/water mask band to the XML file */
    if (append_metadata (1, out_bmeta, espa_xml_file) != SUCCESS)
    {
        sprintf (errmsg, "Appending land/water mask to the XML file.");
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Free the input and output XML metadata */
    free_metadata (&xml_metadata);
    free_metadata (&out_meta);

    /* Free the pointers */
    free (espa_xml_file);

    /* Successful completion */
    exit (SUCCESS);
}
