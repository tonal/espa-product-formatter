/*****************************************************************************
FILE: convert_hdf_to_old_hdf
  
PURPOSE: Contains functions for converting the HDF_EOS2 file format to the
old HDF-EOS2 format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
8/7/2014     Gail Schmidt     Original development

NOTES:
  1. The XML metadata format parsed or written via this library follows the
     ESPA internal metadata format found in ESPA Raw Binary Format v1.0.doc.
     The schema for the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/
#include <getopt.h>
#include "convert_hdf_to_old_hdf.h"

/******************************************************************************
MODULE: usage

PURPOSE: Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
8/7/2014     Gail Schmidt     Original Development

NOTES:
******************************************************************************/
void usage ()
{
    printf ("convert_hdf_to_old_hdf converts the ESPA HDF-EOS2 format (and "
            "associated XML metadata file) to the old style HDF-EOS2 (HDF4).  "
            "The input XML file needs to be processed through ESPA with "
            "surface reflectance and brightness temperature processing. "
            "If the fmask band exists it will also be supported.\n\n");
    printf ("usage: convert_hdf_to_old_hdf "
            "--xml=input_metadata_filename_for_espa_hdf "
            "--hdf=output_oldstyle_hdf_filename\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the XML metadata file which was delivered "
            "with the ESPA HDF-EOS2 file\n");
    printf ("    -hdf: filename of the output old-style HDF-EOS2 file\n");
    printf ("\nExample: convert_hdf_to_old_hdf "
            "--xml=LE70230282011250EDC00_hdf.xml "
            "--hdf=lndsr.LE70230282011250EDC00.hdf\n");
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
    char **hdf_outfile    /* O: address of output HDF filename */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"xml", required_argument, 0, 'i'},
        {"hdf", required_argument, 0, 'o'},
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
     
            case 'o':  /* HDF outfile */
                *hdf_outfile = strdup (optarg);
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

    if (*hdf_outfile == NULL)
    {
        sprintf (errmsg, "HDF output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE:  Converts the ESPA HDF-EOS2 (and associated XML metadata file) to
the old HDF-EOS2 format.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error doing the conversion
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
8/7/2014     Gail Schmidt     Original development

NOTES:
******************************************************************************/
int main (int argc, char** argv)
{
    char *xml_infile = NULL;     /* input XML filename */
    char *hdf_outfile = NULL;    /* output HDF filename */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &xml_infile, &hdf_outfile) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (EXIT_FAILURE);
    }

    /* Convert the ESPA HDF-EOS2 product to the old HDF-EOS2 with external
       SDSs */
    if (convert_hdf_to_old_hdf (xml_infile, hdf_outfile) != SUCCESS)
    {  /* Error messages already written */
        exit (EXIT_FAILURE);
    }

    /* Free the pointers */
    free (xml_infile);
    free (hdf_outfile);

    /* Successful completion */
    exit (EXIT_SUCCESS);
}
