/*****************************************************************************
FILE: convert_lpgs_to_espa
  
PURPOSE: Contains functions for converting the LPGS products to the ESPA
internal raw binary file format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/14/2014    Gail Schmidt     Original development

NOTES:
  1. The XML metadata format parsed or written via this library follows the
     ESPA internal metadata format found in ESPA Raw Binary Format v1.0.doc.
     The schema for the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/
#include <getopt.h>
#include "convert_lpgs_to_espa.h"

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
    printf ("convert_lpgs_to_espa converts the LPGS products (MTL file and "
            "associated GeoTIFF files) to the ESPA internal format (XML "
            "metadata file and associated raw binary files).\n\n");
    printf ("usage: convert_lpgs_to_espa "
            "--mtl=input_mtl_filename "
            "--xml=output_xml_filename\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -mtl: name of the input LPGS MTL metadata file\n");
    printf ("    -xml: name of the output XML metadata file which follows "
            "the ESPA internal raw binary schema\n");
    printf ("\nExample: convert_lpgs_to_espa "
            "--mtl=LE70230282011250EDC00_MTL.txt "
            "--xml=LE70230282011250EDC00.xml\n");
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
    char **mtl_infile,    /* O: address of input LPGS MTL filename */
    char **xml_outfile    /* O: address of output XML filename */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"mtl", required_argument, 0, 'i'},
        {"xml", required_argument, 0, 'o'},
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

            case 'i':  /* LPGS MTL infile */
                *mtl_infile = strdup (optarg);
                break;
     
            case 'o':  /* XML outfile */
                *xml_outfile = strdup (optarg);
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
    if (*mtl_infile == NULL)
    {
        sprintf (errmsg, "LPGS MTL input file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    if (*xml_outfile == NULL)
    {
        sprintf (errmsg, "XML output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE:  Converts the LPGS products (MTL file and associated GeoTIFF files) to
the ESPA internal format (XML metadata file and associated raw binary files).

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
    char *mtl_infile = NULL;      /* input LPGS MTL filename */
    char *xml_outfile = NULL;     /* output XML filename */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &mtl_infile, &xml_outfile) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (EXIT_FAILURE);
    }

    /* Convert the LPGS MTL and data to ESPA raw binary and XML */
    if (convert_lpgs_to_espa (mtl_infile, xml_outfile) != SUCCESS)
    {  /* Error messages already written */
        exit (EXIT_FAILURE);
    }

    /* Free the pointers */
    free (mtl_infile);
    free (xml_outfile);

    /* Successful completion */
    exit (EXIT_SUCCESS);
}
