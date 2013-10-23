#! /usr/bin/env python

'''
License:
  "NASA Open Source Agreement 1.3"

Description:
  Parse the input metadata file, create the needed OMF and ODL parameter files,
  and run the associated DEM-related applications to generate a scene-based DEM
  for the input metadata file.

  The metadata file should be the LPGS MTL.txt file.

  All logging goes to STDOUT by default.

History:
  Created on January 25, 2013 by Gail Schmidt, USGS/EROS
  Updated on February 6, 2013 by Gail Schmidt, USGS/EROS
      Changed the script to update the map projection information in the GDAL
      header file to reflect the correct projection and not Geographic.
  Updated on March 20, 2013 by Gail Schmidt, USGS/EROS
      Modified to support Polar Stereographic projections.
  Updated Oct/2013 by Ron Dilley, USGS/EROS
      Renamed and modified for inclusion into the espa-common library.
      Modified to utilize espa-common functionality.
      Enhanced the running as a stand-alone application.
      Replaced using the depricated 'commands' with using 'subprocess'

Usage:
  do_create_dem.py --help prints the help message
'''

import sys
import os
import re
import subprocess
import datetime
from optparse import OptionParser

from espa_constants import *
from espa_logging import log

### ENVI projection numbers for UTM and PS ###
ENVI_GEO_PROJ = 1
ENVI_UTM_PROJ = 2
ENVI_PS_PROJ = 31

### GCTP projection numbers for UTM and PS ###
GCTP_GEO_PROJ = 0
GCTP_UTM_PROJ = 1
GCTP_PS_PROJ = 6

class SceneDEM():
    '''
    Defines the class object for scene based DEM generation/processing
    '''

    # class attributes
    west_bound_lon = 9999.0           # west bounding coordinate
    east_bound_lon = -9999.0          # east bounding coordinate
    north_bound_lat = -9999.0         # north bounding coordinate
    south_bound_lat = 9999.0          # south bounding coordinate
    ul_proj_x = -13.0                 # UL projection x coordinate
    ul_proj_y = -13.0                 # UL projection y coordinate
    pixsize = -13.0                   # pixel size
    path = 0                          # WRS path
    row = 0                           # WRS row
    map_proj = 'None'                 # map projection (UTM or PS)
    utm_zone = 99                     # UTM zone
    vert_lon_from_pole = -9999.0      # vertical longitude from pole (PS proj)
    true_scale_lat = -9999.0          # true scale latitude (PS proj)
    false_east = -9999.0              # false easting
    false_north = -9999.0             # false northing
    retrieve_elev_odl = 'retrieve_elevation.odl' # name of ODL file
    makegeomgrid_odl = 'makegeomgrid.odl'        # name of ODL file
    geomresample_odl = 'geomresample.odl'        # name of ODL file
    lsrd_omf = 'LSRD.omf'                        # OMF file for LPGS processing
    source_dem = 'lsrd_source_dem.h5'            # source DEM HDF filename
    scene_dem = 'lsrd_scene_based_dem.h5'        # scene based DEM HDF filename
    scene_dem_envi = 'lsrd_scene_based_dem.bin'  # scene based DEM ENVI filename
    scene_dem_hdr = 'lsrd_scene_based_dem.hdr'   # scene based DEM header file
    geomgrid = 'lsrd_geomgrid.h5'                # geometric grid HDF filename

    def __init__(self):
        pass

    def fixGdalHdr (self, gdal_hdr):
        '''
        Description:
          Parse the GDAL generated header file for the scene based DEM and
          update the map projection information to be correct, based on the
          projection information in the MTL file.

        Inputs:
          gdal_hdr - name of the gdal_hdr file to be parsed

        Returns: Nothing

        Notes:
          It is expected the GDAL header exists and contains the map info
          field along with all the other fields needed for the header.
        '''

        # open the GDAL header file for reading
        hdrf = open (gdal_hdr, 'r')

        # open a temporary new file for writing (copying the current GDAL
        # header information)
        temp_hdr = 'temp.hdr'
        tempf = open (temp_hdr, 'w')

        # read and process one line at a time, looking for the line with
        # "map info".  remove the trailing end of line and leading/trailing
        # white space
        for line in hdrf:
            myline = line.rstrip('\r\n').strip()
#            print "DEBUG: *%s*" % myline

            # if the current line contains "map info" then we want to modify
            # this line with the new projection info
            if myline.find ('map info') >= 0:
                if (self.map_proj == 'UTM'):
                    if self.utm_zone > 0:
                        map_info_str = "map info = {UTM, 1.000, 1.000, %f," \
                            " %f, %f, %f, %d, North, WGS-84, units=Meters}\n" \
                            % (self.ul_proj_x, self.ul_proj_y, self.pixsize,
                               self.pixsize, self.utm_zone)
                    else:
                        map_info_str = "map info = {UTM, 1.000, 1.000, %f," \
                            " %f, %f, %f, %d, South, WGS-84, units=Meters}\n" \
                            % (self.ul_proj_x, self.ul_proj_y, self.pixsize,
                               self.pixsize, -self.utm_zone)
                    tempf.write (map_info_str)
                elif (self.map_proj == "PS"):
                    map_info_str = "map info = {Polar Stereographic, 1.000," \
                        " 1.000, %f, %f, %f, %f, WGS-84, units=Meters}\n" \
                        % (self.ul_proj_x, self.ul_proj_y, self.pixsize,
                           self.pixsize)
                    tempf.write (map_info_str)
                    proj_info_str = "projection info = {%d, 6378137.0," \
                        " 6356752.314245179, %f, %f, %f, %f, WGS-84, Polar" \
                        " Stereographic, units=Meters}\n" \
                        % (ENVI_PS_PROJ, self.true_scale_lat,
                           self.vert_lon_from_pole, self.false_east,
                           self.false_north)
                    tempf.write (proj_info_str)
            else:
                # copy this line as-is to the temporary header file
                tempf.write (line)

        # done with the header file and temp file
        hdrf.close()
        tempf.close()

        # remove the header file
        os.remove (gdal_hdr)

        # rename the temporary file to the header file
        os.rename (temp_hdr, gdal_hdr)
    # END of fixGdalHdr


    def parseMeta (self, metafile):
        '''
        Description:
          Parse the input metadata file, search for the desired fields in the
          metadata file, and set the associated class variables with the
          values read for that field.

        Inputs:
          metafile - name of the metadata file to be parsed

        Returns:
          ERROR - error occurred while parsing the metadata file
          SUCCESS - successful processing of the metadata file

        Notes:
          It is expected the input metadata file is an LPGS _MTL.txt file and
          follows the format (contains the same fields) of those files.
        '''

        # open the metadata file for reading
        metaf = open (metafile, 'r')

        # read and process one line at a time, using the = sign as the
        # deliminator for the desired parameters.  remove the trailing
        # end of line and leading/trailing white space
        for line in metaf:
            myline = line.rstrip('\r\n').strip()
#            print "DEBUG: *%s*" % myline
            # break out if at the end of the metadata in the metadata file
            if myline == "END":
                break

            # process the myline which should be parameter = value strings.
            # some strings may need the "s stripped from the value itself
            (param, value) = myline.split ("=", 1)
            param = param.strip()
            value = value.strip()
#            print "    DEBUG: param *%s*" % param
#            print "    DEBUG: value *%s*" % value.strip('\"')
            if   (param == 'CORNER_UL_LAT_PRODUCT'):
                if (float(value) > self.north_bound_lat):
                    self.north_bound_lat = float(value)
            elif (param == 'CORNER_UL_LON_PRODUCT'):
                if (float(value) < self.west_bound_lon):
                    self.west_bound_lon = float(value)
            elif (param == 'CORNER_UR_LAT_PRODUCT'):
                if (float(value) > self.north_bound_lat):
                    self.north_bound_lat = float(value)
            elif (param == 'CORNER_UR_LON_PRODUCT'):
                if (float(value) > self.east_bound_lon):
                    self.east_bound_lon = float(value)
            elif (param == 'CORNER_LL_LAT_PRODUCT'):
                if (float(value) < self.south_bound_lat):
                    self.south_bound_lat = float(value)
            elif (param == 'CORNER_LL_LON_PRODUCT'):
                if (float(value) < self.west_bound_lon):
                    self.west_bound_lon = float(value)
            elif (param == 'CORNER_LR_LAT_PRODUCT'):
                if (float(value) < self.south_bound_lat):
                    self.south_bound_lat = float(value)
            elif (param == 'CORNER_LR_LON_PRODUCT'):
                if (float(value) > self.east_bound_lon):
                    self.east_bound_lon = float(value)
            elif (param == 'CORNER_UL_PROJECTION_X_PRODUCT'):
                self.ul_proj_x = float(value)
            elif (param == 'CORNER_UL_PROJECTION_Y_PRODUCT'):
                self.ul_proj_y = float(value)
            elif (param == 'GRID_CELL_SIZE_REFLECTIVE'):
                self.pixsize = float(value)
            elif (param == 'WRS_PATH'):
                self.path = int(value)
            elif (param == 'WRS_ROW'):
                self.row = int(value)
            elif (param == 'MAP_PROJECTION'):
                self.map_proj = value.strip('\"')
            elif (param == 'UTM_ZONE'):
                self.utm_zone = int(value)
            elif (param == 'VERTICAL_LON_FROM_POLE'):
                self.vert_lon_from_pole = float(value)
            elif (param == 'TRUE_SCALE_LAT'):
                self.true_scale_lat = float(value)
            elif (param == 'FALSE_EASTING'):
                self.false_east = float(value)
            elif (param == 'FALSE_NORTHING'):
                self.false_north = float(value)
        # END for line in metaf

        # done with the metadata file
        metaf.close()

        # validate input
        if (self.north_bound_lat == -9999.0) or \
           (self.south_bound_lat == 9999.0) or  \
           (self.east_bound_lon == -9999.0) or \
           (self.west_bound_lon == 9999.0):
            msg = "Error: obtaining north/south and/or east/west bounding" \
               " metadata fields from: %s" % metafile
            log (msg)
            return ERROR

        if (self.ul_proj_x == -13.0) or \
           (self.ul_proj_y == -13.0):
            msg = "Error: obtaining UL project x/y fields from: %s" % metafile
            log (msg)
            return ERROR

        if (self.pixsize == -13.0):
            msg = "Error: obtaining reflective grid cell size field from:" \
               " %s" % metafile
            log (msg)
            return ERROR

        if (self.path == 0) or (self.row == 0):
            msg = "Error: obtaining path/row metadata fields from: %s" \
                % metafile
            log (msg)
            return ERROR

        if (self.map_proj != "UTM") and (self.map_proj != "PS"):
            msg = "Error: obtaining map projection metadata field from: %s." \
              "  Only UTM and PS are supported." % metafile
            log (msg)
            return ERROR

        if (self.map_proj == "UTM"):
            if (self.utm_zone == 0):
                msg = "Error: obtaining UTM zone metadata field from: %s" \
                    % metafile
                log (msg)
                return ERROR

        if (self.map_proj == "PS"):
            if (self.vert_lon_from_pole == -9999.0) or \
               (self.true_scale_lat == -9999.0) or \
               (self.false_east == -9999.0) or \
               (self.false_north == -9999.0):
                msg = "Error: obtaining Polar Stereographic metadata fields" \
                    " (vertical longitude from pole, true scale latitude," \
                    " false easting, and/or false northing from: %s" % metafile
                log (msg)
                return ERROR

        # convert the UL corner to represent the UL corner of the pixel
        # instead of the center of the pixel.  the coords in the MTL file
        # represent the center of the pixel.
        self.ul_proj_x -= 0.5 * self.pixsize
        self.ul_proj_y += 0.5 * self.pixsize

        # successful completion
        return SUCCESS
    # END of parseMeta


    def writeParams (self, metafile):
        '''
        Description:
            Creates and writes the necessary ODL and OMF parameter files for
            running retrieve_elevation, makegeomgrid, and geomresample using
            the parameter values previously obtained.

        Inputs:
          metafile - name of the metadata file to be used for processing

        Returns: Nothing
        '''

        # open the OMF file for writing
        omff = open (self.lsrd_omf, 'w')

        # build the OMF file
        omf_list = []
        omf_list.append ("OBJECT = OMF\n")
        omf_list.append ("  SATELLITE = 8\n")
        msg = "  UL_BOUNDARY_LAT_LON = (%f, %f)\n" \
            % (self.north_bound_lat, self.west_bound_lon)
        omf_list.append (msg)
        msg = "  UR_BOUNDARY_LAT_LON = (%f, %f)\n" \
            % (self.north_bound_lat, self.east_bound_lon)
        omf_list.append (msg)
        msg = "  LL_BOUNDARY_LAT_LON = (%f, %f)\n" \
            % (self.south_bound_lat, self.west_bound_lon)
        omf_list.append (msg)
        msg = "  LR_BOUNDARY_LAT_LON = (%f, %f)\n" \
            % (self.south_bound_lat, self.east_bound_lon)
        omf_list.append (msg)
        msg = "  TARGET_WRS_PATH = %d\n" % self.path
        omf_list.append (msg)
        msg = "  TARGET_WRS_ROW = %d\n" % self.row
        omf_list.append (msg)

        if (self.map_proj == 'UTM'):
            omf_list.append ("  TARGET_PROJECTION = 1\n")
            msg = "  UTM_ZONE = %d\n" % self.utm_zone
            omf_list.append (msg)
        elif (self.map_proj == 'PS'):
            # GAIL finish!!  Add support for the projection params, which are
            # then read by makegeomgrid/get_proj_info_toa_refl.c
            omf_list.append ("  TARGET_PROJECTION = 6\n")

        msg = "  GRID_FILENAME_PASS_1 = \"%s\"\n\n" % metafile
        omf_list.append (msg)
        omf_list.append ("END_OBJECT = OMF\n")
        omf_list.append ("END\n")
        omff.writelines (omf_list)
        
        # done with the OMF file
        omff.close()

        # open the retrieve_elevation ODL file for writing
        odlf = open (self.retrieve_elev_odl, 'w')

        # build the ODL file
        odl_string = \
            "OBJECT = SCA\n" \
            "  WO_DIRECTORY = \".\"\n" \
            "  WORK_ORDER_ID = LSRD\n" \
            "  DEM_FILENAME = \"%s\"\n" \
            "END_OBJECT = SCA\n" \
            "END\n" % self.source_dem
        odlf.write (odl_string)

        # done with the ODL file
        odlf.close()

        # open the makegeomgrid ODL file for writing
        odlf = open (self.makegeomgrid_odl, 'w')

        # build the ODL file
        odl_string = \
            "OBJECT = GRID_TERRAIN\n" \
            "  WO_DIRECTORY = \".\"\n" \
            "  WORK_ORDER_ID = LSRD\n" \
            "  CELL_LINES = 50\n" \
            "  CELL_SAMPLES = 50\n" \
            "  GEOM_GRID_FILENAME = \"%s\"\n" \
            "  PROCESSING_PASS = 1\n" \
            "  SOURCE_BAND_NUMBER_LIST = 1\n" \
            "  SOURCE_IMAGE_TYPE = 0\n" \
            "  TARGET_BAND_NUMBER_LIST = 1\n" \
            "END_OBJECT = GRID_TERRAIN\n" \
            "END\n" % self.geomgrid
        odlf.write (odl_string)

        # done with the ODL file
        odlf.close()

        # open the geomresample ODL file for writing
        odlf = open (self.geomresample_odl, 'w')

        # build the ODL file
        odl_string = \
            "OBJECT = RESAMPLE_TERRAIN\n" \
            "  WO_DIRECTORY = \".\"\n" \
            "  WORK_ORDER_ID = LSRD\n" \
            "  BACKGRND = 0.000000\n" \
            "  BAND_LIST = 1\n" \
            "  MINMAX_OUTPUT = (-500.000000,9000.000000)\n" \
            "  ODTYPE = \"I*2\"\n" \
            "  OUTPUT_IMAGE_FILENAME = \"%s\"\n" \
            "  PCCALPHA = -0.500000\n" \
            "  PROCESSING_PASS = 1\n" \
            "  RESAMPLE = BI\n" \
            "  SOURCE_IMAGE_TYPE = 0\n" \
            "  WINDOW_FLAG = 0\n" \
            "END_OBJECT = RESAMPLE_TERRAIN\n" \
            "END\n" % self.scene_dem
        odlf.write (odl_string)

        # done with the ODL file
        odlf.close()
    # END of writeParams


    def createDEM (self, metafile=None, usebin=None):
        '''
        Description:
          Use the parameter passed for the LPGS metadata file (*_MTL.txt) to
          parse the necessary fields to create the OMF and ODL files needed
          for running retrieve_elevation, makegeomgrid, and geomresample in
          order to generate a scene-based DEM.

        Inputs:
          metafile - name of the Landsat metadata file to be processed
          usebin - this specifies if the DEM exes reside in the $BIN directory;
                   if None then the exes are expected to be in the PATH

        Returns:
          ERROR - error running the DEM applications
          SUCCESS - successful processing

        Notes:
          The script obtains the path of the metadata file and changes
          directory to that path for running the DEM code.  If the metafile
          directory is not writable, then this script exits with an error.
        '''

        # if no parameters were passed then get the info from the command line
        if metafile == None:
            # get the command line argument for the metadata file
            parser = OptionParser()
            parser.add_option ('-f', '--metafile', type='string',
                dest='metafile',
                help="name of Landsat LPGS MTL file", metavar='FILE')
            parser.add_option ('--usebin', dest='usebin', default=False,
                action='store_true',
                help="use BIN environment variable as the location of DEM apps")
            (options, args) = parser.parse_args()
    
            # validate the command-line options
            usebin = options.usebin          # should $BIN directory be used
            metafile = options.metafile      # name of the metadata file
            if metafile == None:
                parser.error ("missing metafile command-line argument");
                return ERROR
        
        # open the log file if it exists and the log handler wasn't already
        # specified as a parameter; use line buffering for the output; if the
        # log handler was passed as a parameter, then don't close the log
        # handler.  that will be up to the calling routine.
        msg = "Processing scene-based DEMs for Landsat metadata file: %s" \
            % metafile
        log (msg)
        
        # should we expect the DEM applications to be in the PATH or in the
        # BIN directory?
        if usebin:
            # get the BIN dir environment variable
            bin_dir = os.environ.get('BIN')
            bin_dir = bin_dir + '/'
            msg = "BIN environment variable: %s" % bin_dir
            log (msg)
        else:
            # don't use a path to the DEM applications, they are expected
            # to be in the PATH
            bin_dir = ""
            msg = "DEM executables expected to be in the PATH"
            log (msg)
        
        # make sure the metadata file exists
        if not os.path.isfile(metafile):
            msg = "Error: metadata file does not exist or is not" \
                " accessible: %s" % metafile
            log (msg)
            return ERROR

        # use the base metadata filename and not the full path.
        base_metafile = os.path.basename (metafile)
        msg = "Processing metadata file: %s" % base_metafile
        log (msg)
        
        # get the path of the MTL file and change directory to that location
        # for running this script.  save the current working directory for
        # return to upon error or when processing is complete.  Note: use
        # abspath to handle the case when the filepath is just the filename
        # and doesn't really include a file path (i.e. the current working
        # directory).
        mydir = os.getcwd()
        metadir = os.path.dirname (os.path.abspath (metafile))
        if not os.access(metadir, os.W_OK):
            msg = "Path of metadata file is not writable: %s.  DEM apps need" \
                " write access to the metadata directory." % metadir
            log (msg)
            return ERROR
        msg = "Changing directories for DEM processing: %s" % metadir
        log (msg)
        os.chdir (metadir)

        # parse the metadata file to get the necessary parameters
        return_value = self.parseMeta (base_metafile)
        if return_value != SUCCESS:
            msg = "Error parsing the metadata. Processing will terminate."
            log (msg)
            os.chdir (mydir)
            return ERROR

        # create the OMF and ODL files
        self.writeParams (base_metafile)

        # run DEM modules, checking the return status of each module.
        # exit if any errors occur.
        cmd_list = ['%sretrieve_elevation' % bin_dir,
                    '%s' % self.retrieve_elev_odl]
        try:
            log ("Executing: %s" % ' '.join(cmd_list))
            (output) = subprocess.check_output (cmd_list)
        except subprocess.CalledProcessError, e:
            msg = "Error running retrieve_elevation. Processing will terminate."
            log (msg)
            os.chdir (mydir)
            return ERROR
        log (output)

        cmd_list = ['%smakegeomgrid' % bin_dir,
                    '%s' % self.makegeomgrid_odl]
        try:
            log ("Executing: %s" % ' '.join(cmd_list))
            (output) = subprocess.check_output (cmd_list)
        except subprocess.CalledProcessError, e:
            msg = "Error running makegeomgrid. Processing will terminate."
            log (msg)
            os.chdir (mydir)
            return ERROR
        log (output)
        
        cmd_list = ['%sgeomresample' % bin_dir,
                    '%s' % self.geomresample_odl]
        try:
            log ("Executing: %s" % ' '.join(cmd_list))
            (output) = subprocess.check_output (cmd_list)
        except subprocess.CalledProcessError, e:
            msg = "Error running geomresample. Processing will terminate."
            log (msg)
            os.chdir (mydir)
            return ERROR
        log (output)
        
        cmd_list = ['gdal_translate',
                    '-of',
                    'ENVI',
                    'HDF5:\"%s\"://B01' % self.scene_dem,
                    '%s' % self.scene_dem_envi]
        try:
            log ("Executing: %s" % ' '.join(cmd_list))
            (output) = subprocess.check_output (cmd_list)
        except subprocess.CalledProcessError, e:
            msg = "Error running gdal_translate, which is expected to be in" \
                " your PATH. Processing will terminate."
            log (msg)
            os.chdir (mydir)
            return ERROR
        log (output)
        
        # modify the gdal output header since it doesn't contain the correct
        # projection information; instead it just flags the image as being
        # in the Geographic projection
#        print "DEBUG: Updating the GDAL header file"
        self.fixGdalHdr (self.scene_dem_hdr)

        # successful completion.  return to the original directory.
        os.chdir (mydir)
        msg = "Completion of scene based DEM generation."
        log (msg)
        return SUCCESS
    # END of createDEM
# END of SceneDEM class

if __name__ == '__main__':
    return_value = sys.exit (SceneDEM().createDEM())

    if (return_value != SUCCESS):
        sys.exit (EXIT_FAILURE)
    else:
        sys.exit (EXIT_SUCCESS)

