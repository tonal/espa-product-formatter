## ESPA-PRODUCT_FORMATTER Version 1.4.0 Release Notes
Release Date: May 27, 2015

The product formatter project contains libraries and tools for working with the ESPA internal file format (raw binary with an XML metadata file). It currently supports Landsat 4-8.

This project is hosted by the US Geological Survey (USGS) Earth Resources Observation and Science (EROS) Land Satellite Data Systems (LSDS) Science Research and Development (LSRD) Project. For questions regarding this source code, please contact the Landsat Contact Us page and specify USGS CDR/ECV in the "Regarding" section. https://landsat.usgs.gov/contactus.php

### Downloads
espa-product-formatter source code

    git clone https://github.com/USGS-EROS/espa-product-formatter.git

See git tag [version_1.4.0]

### Dependencies
  * GCTP libraries
  * TIFF libraries
  * GeoTIFF libraries
  * HDF4 libraries
  * HDF-EOS2 libraries
  * JPEG libraries
  * XML2 libraries
  * Land/water static polygon

### Installation
  * Install dependent libraries - HDF-EOS GCTP (from HDF-EOS2), HDF4, HDF-EOS2, TIFF, GeoTIFF, JPEG, and XML2.

  * Set up environment variables.  Can create an environment shell file or add the following to your bash shell.  For C shell, use 'setenv VAR "directory"'.  Note: If the HDF library was configured and built with szip support, then the user will also need to add an environment variable for SZIP include (SZIPINC) and library (SZIPLIB) files.
  ```
    export HDFEOS_GCTPINC="path_to_HDF-EOS_GCTP_include_files"
    export HDFEOS_GCTPLIB="path_to_HDF-EOS_GCTP_libraries"
    export TIFFINC="path_to_TIFF_include_files"
    export TIFFLIB="path_to_TIFF_libraries"
    export GEOTIFF_INC="path_to_GEOTIFF_include_files"
    export GEOTIFF_LIB="path_to_GEOTIFF_libraries"
    export HDFINC="path_to_HDF4_include_files"
    export HDFLIB="path_to_HDF4_libraries"
    export HDFEOS_INC="path_to_HDFEOS2_include_files"
    export HDFEOS_LIB="path_to_HDFEOS2_libraries"
    export JPEGINC="path_to_JPEG_include_files"
    export JPEGLIB="path_to_JPEG_libraries"
    export XML2INC="path_to_XML2_include_files"
    export XML2LIB="path_to_XML2_libraries"
    export ESPAINC="path_to_format_converter_raw_binary_include_directory"
    export ESPALIB="path_to_format_converter_raw_binary_lib_directory"
  ```
  Define $PREFIX to point to the directory in which you want the executables, static data, etc. to be installed.
  ```
    export PREFIX="path_to_directory_for_format_converter_build_data"
   ```

  * Download the static land/water polygon from http://espa.cr.usgs.gov/downloads/auxiliaries/land_water_poly/land_no_buf.ply.gz. Unzip the file into $PREFIX/static_data.  Define the ESPA_LAND_MASS_POLYGON environment variable to point to the $PREFIX/static_data/land_no_buf.ply file in order to run the land/water mask code.
  ```
    export ESPA_LAND_MASS_POLYGON=$PREFIX/static_data/land_no_buf.ply
  ```
  
* Install ESPA product formatter libraries and tools by downloading the source from Downloads above.  Goto the src/raw\_binary directory and build the source code there. ESPAINC and ESPALIB above refer to the include and lib directories created by building this source code using make followed by make install. The ESPA raw binary conversion tools will be located in the $PREFIX/bin directory.

  Note: if the HDF library was configured and built with szip support, then the user will also need to add "-L$(SZIPLIB) -lsz" at the end of the library defines in the Makefiles.  The user should also add "-I$(SZIPINC)" to the include directory defines in the Makefile.

  Note: on some platforms, the JBIG library may be needed for the XML library support, if it isn't already installed.  If so, then the JBIGLIB environment variable needs to point to the location of the JBIG library.

### Linking these libraries for other applications
The following is an example of how to link these libraries into your
source code. Depending on your needs, some of these libraries may not
be needed for your application.
```
 -L$(ESPALIB) -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
 -L$(XML2LIB) -lxml2 \
 -L$(HDFEOS_LIB) -lhdfeos -L$(HDFEOS_GCTPLIB) -lGctp \
 -L$(HDFLIB) -lmfhdf -ldf -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm
```

### Verification Data

### User Manual

### Product Guide


## Changes From Previous Version
#### Updates on May 27, 2015 - USGS EROS
  * Created schema version 1.2 to include the reflectance gain/bias.  Changed toa\_reflectance gain/bias to radiance gain/bias.  This will better match the MTL file nomenclature.  Added thermal constants K1 and K2.  Added the earth-sun distance.  Updated the espa-common libraries and tools to read the MTL file to populate these new optional parameters and write them to the XML file.  The HDF global attributes no longer contain the gain/bias values.  Writing these as an array to the HDF attributes doesn’t really allow us to document what band each one applies to.  The gain/bias values for both radiance and reflectance will be in the XML file and they are much better documented there.  These coefficients and constants exist for TM  (if reprocessing is done), ETM+ (after March 17th), and OLI/TIRS.  Also verified that the scene center time parameter containing quotes or not containing quotes won’t be an issue.
  * Updated to include per-pixel solar/sensor angle libraries for Landsat 8.  A new tool in the tools directory will produce the solar/sensor angle bands for L8.
  * The IAS libraries in per-pixel angles code defined ERROR to be -1 vs. our previously defined ERROR of 1.  Switched ERROR in common.h to be -1.  Also renamed common.h to espa\_common.h to be more specific to ESPA.
  * Updated to install in $PREFIX/bin vs. $BIN
  * Developed a library to create the scene-based static land/water mask from the land mass polygon.  There is a create\_land\_water\_mask application which creates the land/water mask band {scenename\_land\_water\_mask.img} and appends it to the XML file.  This is being tested with L4, L5, L7, and L8.  This library/application combination requires that ESPA\_LAND\_MASS\_POLYGON be defined to point to the land\_no\_buf.ply file which gets installed in $PREFIX/static\_data.
  * Updated python metadata api for the schema changes.
