/*****************************************************************************
FILE: espa_band_subset
  
PURPOSE: Contains functions for subsetting the ESPA raw binary file format,
particularly the XML file, to select the specified bands.

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
#include "subset_metadata.h"

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
    printf ("espa_band_subset subsets the specified bands from the input XML "
            "metadata file and creates a new XML metadata file containing "
            "only the specified bands.\n\n");
    printf ("usage: espa_band_subset "
            "--xml=input_metadata_filename "
            "--subset_xml=output_subset_metadata_filename "
            "[--band=band_name (multiple --band options can be specified)].\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the input XML metadata file which follows "
            "the ESPA internal raw binary schema\n");
    printf ("    -subset_xml: name of the output XML metadata file containing "
            "only the user-specified bands\n");

    printf ("\nwhere the following parameters are optional:\n");
    printf ("    -band: name of the band in the input XML file to be written "
            "to the subset XML file. If not specified, then only the global "
            "and projection metadata will be copied to the subset XML file.\n");
    printf ("\nExample: espa_band_subset "
            "--xml=LE70230282011250EDC00.xml "
            "--subset_xml=LE70230282011250EDC00_subset.xml "
            "--band band1 --band band4 --band band7\n");
    printf ("\nExample: espa_band_subset "
            "--xml=LE70230282011250EDC00.xml "
            "--subset_xml=LE70230282011250EDC00_subset.xml\n");
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
    char **xml_subset_outfile,  /* O: address of output subset XML filename */
    int *nbands,          /* O: number of bands in the subset */
    char bands[][STR_SIZE]  /* O: array of band names to be subset */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    int count;                       /* number of chars copied in snprintf */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"xml", required_argument, 0, 'i'},
        {"subset_xml", required_argument, 0, 'o'},
        {"band", required_argument, 0, 'b'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* Loop through all the cmd-line options */
    *nbands = 0;
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
     
            case 'o':  /* XML subset outfile */
                *xml_subset_outfile = strdup (optarg);
                break;
     
            case 'b':  /* band name to be added */
                count = snprintf (bands[*nbands], sizeof (bands[*nbands]),
                    "%s", optarg);
                if (count < 0 || count >= sizeof (bands[*nbands]))
                {
                    sprintf (errmsg, "Overflow of bands[*nbands] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                (*nbands)++;
                if (*nbands > MAX_TOTAL_BANDS)
                {
                    sprintf (errmsg, "Maximum number of bands (%d) has been "
                        "reached. Results might not be trustworthy as the "
                        "memory may have been trashed.", MAX_TOTAL_BANDS);
                    error_handler (false, FUNC_NAME, errmsg);
                }
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

    if (*xml_subset_outfile == NULL)
    {
        sprintf (errmsg, "XML subset output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    /* Warn the user if no bands were specified */
    if (*nbands == 0)
    {
        sprintf (errmsg, "No bands were specified, therefore only the "
            "global and projection metadata will be copied to the subset "
            "XML file.");
        error_handler (false, FUNC_NAME, errmsg);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE:  Subsets the specified bands from the input XML metadata file and
creates a new XML metadata file containing only the specified bands.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error doing the subsetting
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/14/2014    Gail Schmidt     Original development

NOTES:
  1. If nbands is 0 or no bands match the bands specified, then the global and
     projection information will still be copied to the subset XML file.
******************************************************************************/
int main (int argc, char** argv)
{
    char *xml_infile = NULL;          /* input XML filename */
    char *xml_subset_outfile = NULL;  /* output subset XML filename */
    char bands[MAX_TOTAL_BANDS][STR_SIZE];  /* array of nbands band names */
    int nbands;                       /* number of bands specified */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &xml_infile, &xml_subset_outfile, &nbands,
        bands) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (EXIT_FAILURE);
    }

    /* Subset the input XML metadata file with the specified bands and write
       to the output XML metadata file */
    if (subset_xml_by_band (xml_infile, xml_subset_outfile, nbands, bands) !=
        SUCCESS)
    {  /* Error messages already written */
        exit (EXIT_FAILURE);
    }

    /* Free the pointers */
    free (xml_infile);
    free (xml_subset_outfile);

    /* Successful completion */
    exit (EXIT_SUCCESS);
}
