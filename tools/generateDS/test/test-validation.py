#! /usr/bin/env python

import os
import sys
from lxml import etree
from argparse import ArgumentParser

import metadata_api

if __name__ == '__main__':
    # Create a command line argument parser
    description = "Validate an ESPA XML using two methods"
    parser = ArgumentParser (description=description)

    parser.add_argument ('--xml-file',
        action='store', dest='xml_file', required=True,
        help="ESPA XML file to validate")

    # Parse the command line arguments
    args = parser.parse_args()

    xml = metadata_api.parse (args.xml_file, silence=True)


    # Export with validation
    f = open ('val_01-' + args.xml_file, 'w')
    # Create the file and specify the namespace/schema
    metadata_api.export (f, xml)
    f.close()


    # LXML - Validation Example
    try:
        f = open ('../../../htdocs/schema/espa_internal_metadata_v1_0.xsd')
        schema_root = etree.parse (f)
        f.close()
        schema = etree.XMLSchema (schema_root)

        tree = etree.parse ('val_01-' + args.xml_file)

        schema.assertValid (tree)
    except Exception, e:
       print "lxml Validation Error: %s" % e
       print str (e)

    # Terminate SUCCESS
    sys.exit(0)

