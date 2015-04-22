/*****************************************************************************
FILE: convert_espa_to_gtif
  
PURPOSE: Contains functions for converting the ESPA raw binary file format
to GeoTIFF.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/14/2014    Gail Schmidt     Original development
4/2/2014     Gail Schmidt     Added a command-line flag to remove the source
                              files if specified

NOTES:
  1. The XML metadata format parsed or written via this library follows the
     ESPA internal metadata format found in ESPA Raw Binary Format v1.0.doc.
     The schema for the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/
#include <getopt.h>
#include "convert_espa_to_gtif.h"

/******************************************************************************
MODULE: usage

PURPOSE: Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
1/14/2014    Gail Schmidt     Original Development

NOTES:
******************************************************************************/
void usage ()
{
    printf ("convert_espa_to_gtif converts the ESPA internal format (raw "
            "binary and associated XML metadata file) to GeoTIFF.  Each "
            "band represented in the input XML file will be written to a "
            "single GeoTIFF file using the base filename provided followed "
            "by the band name with a .tif file extension.\n\n");
    printf ("usage: convert_espa_to_gtif "
            "--xml=input_metadata_filename "
            "--gtif=output_geotiff_base_filename "
            "[--del_src_files]\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the input XML metadata file which follows "
            "the ESPA internal raw binary schema\n");
    printf ("    -gtif: base filename of the output GeoTIFF files\n");
    printf ("    -del_src_files: if specified the source image and header "
            "files will be removed\n");
    printf ("\nExample: convert_espa_to_gtif "
            "--xml=LE70230282011250EDC00.xml "
            "--gtif=LE70230282011250EDC00\n");
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
1/14/2014    Gail Schmidt     Original development

NOTES:
  1. Memory is allocated for the input and output files.  All of these should
     be character pointers set to NULL on input.  The caller is responsible
     for freeing the allocated memory upon successful return.
******************************************************************************/
short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **xml_infile,    /* O: address of input XML filename */
    char **gtif_outfile,  /* O: address of output GeoTIFF base filename */
    bool *del_src         /* O: should source files be removed? */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static int del_flag = 0;         /* flag for removing the source files */
    static struct option long_options[] =
    {
        {"del_src_files", no_argument, &del_flag, 1},
        {"xml", required_argument, 0, 'i'},
        {"gtif", required_argument, 0, 'o'},
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
     
            case 'o':  /* GeoTIFF base outfile */
                *gtif_outfile = strdup (optarg);
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

    if (*gtif_outfile == NULL)
    {
        sprintf (errmsg, "GeoTIFF base output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    /* Check the delete source files flag */
    if (del_flag)
        *del_src = true;

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE:  Converts the ESPA internal format (raw binary and associated XML
metadata file) to GeoTIFF.  Each band represented in the input XML file will
be written to a single GeoTIFF file using the base filename provided followed
by the band name with a .tif file extension.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error doing the conversion
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/14/2014    Gail Schmidt     Original development

NOTES:
******************************************************************************/
int main (int argc, char** argv)
{
    char *xml_infile = NULL;     /* input XML filename */
    char *gtif_outfile = NULL;   /* output base GeoTIFF filename */
    bool del_src = false;        /* should source files be removed? */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &xml_infile, &gtif_outfile, &del_src) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (EXIT_FAILURE);
    }

    /* Convert the internal ESPA raw binary product to GeoTIFF */
    if (convert_espa_to_gtif (xml_infile, gtif_outfile, del_src) != SUCCESS)
    {  /* Error messages already written */
        exit (EXIT_FAILURE);
    }

    /* Free the pointers */
    free (xml_infile);
    free (gtif_outfile);

    /* Successful completion */
    exit (EXIT_SUCCESS);
}
