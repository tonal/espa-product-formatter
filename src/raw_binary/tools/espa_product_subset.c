/*****************************************************************************
FILE: espa_product_subset
  
PURPOSE: Contains functions for subsetting the ESPA raw binary file format,
particularly the XML file, to select bands with specified product types.

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
     http://espa.cr.usgs.gov/schema/espa_internal_metadata_v1_0.xsd.
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
    printf ("espa_product_subset subsets the bands with the specified "
            "product types from the input XML metadata file and creates a "
            "new XML metadata file containing only the specified bands.\n\n");
    printf ("usage: espa_product_subset "
            "--xml=input_metadata_filename "
            "--subset_xml=output_subset_metadata_filename "
            "--product=product_name (multiple --product options can be "
            "specified).\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the input XML metadata file which follows "
            "the ESPA internal raw binary schema\n");
    printf ("    -subset_xml: name of the output XML metadata file containing "
            "only the bands with the user-specified product types\n");
    printf ("    -product: name of the product type in the input XML file to "
            "be written to the subset XML file\n");
    printf ("\nExample: espa_product_subset "
            "--xml=LE70230282011250EDC00.xml "
            "--subset_xml=LE70230282011250EDC00_subset.xml "
            "--product L1G --product L1T --product surface_reflectance\n");
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
    int *nproducts,       /* O: number of product types in the subset */
    char products[][STR_SIZE]  /* O: array of product types to be subset */
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
        {"product", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* Loop through all the cmd-line options */
    *nproducts = 0;
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
     
            case 'p':  /* product type to be added */
                count = snprintf (products[*nproducts], 
                    sizeof (products[*nproducts]), "%s", optarg);
                if (count < 0 || count >= sizeof (products[*nproducts]))
                {
                    sprintf (errmsg, "Overflow of products[*nproducts] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                (*nproducts)++;
                if (*nproducts > MAX_TOTAL_PRODUCT_TYPES)
                {
                    sprintf (errmsg, "Maximum number of products (%d) has been "
                        "reached. Results might not be trustworthy as the "
                        "memory may have been trashed.",
                        MAX_TOTAL_PRODUCT_TYPES);
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

    if (*nproducts == 0)
    {
        sprintf (errmsg, "At least one product type must be specified for "
            "subsetting");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE:  Subsets the bands from the input XML metadata file based on the
specified product types and creates a new XML metadata file containing only
the specified bands.

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
  1. If no bands match the product type, then the global and projection
     information will still be copied to the subset XML file.
******************************************************************************/
int main (int argc, char** argv)
{
    char *xml_infile = NULL;          /* input XML filename */
    char *xml_subset_outfile = NULL;  /* output subset XML filename */
    char products[MAX_TOTAL_PRODUCT_TYPES][STR_SIZE];  /* array of nproducts
                                       product types */
    int nproducts;                   /* number of product types specified */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &xml_infile, &xml_subset_outfile, &nproducts,
        products) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (EXIT_FAILURE);
    }

    /* Subset the input XML metadata file with the specified product types and
       write to the output XML metadata file */
    if (subset_xml_by_product (xml_infile, xml_subset_outfile, nproducts,
        products) != SUCCESS)
    {  /* Error messages already written */
        exit (EXIT_FAILURE);
    }

    /* Free the pointers */
    free (xml_infile);
    free (xml_subset_outfile);

    /* Successful completion */
    exit (EXIT_SUCCESS);
}
