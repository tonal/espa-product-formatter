#! /usr/bin/env python

import datetime

import metadata_api


# Create the top-level espa_metadata object
xml = metadata_api.espa_metadata()


# Create the global_metadata object
global_metadata = metadata_api.global_metadataType()

# Assign the global metadata values
global_metadata.set_data_provider ('USGS/EROS')
global_metadata.set_satellite ('LANDSAT_5')
global_metadata.set_instrument ('TM')

# ---
date = datetime.datetime.strptime ('2002-02-11', '%Y-%m-%d')
global_metadata.set_acquisition_date (date)
date = datetime.datetime.strptime ('18:34:40.566000Z', '%H:%M:%S.%fZ')
global_metadata.set_scene_center_time (date)
date = datetime.datetime.strptime ('2014-01-13T06:49:56', '%Y-%m-%dT%H:%M:%S')
global_metadata.set_level1_production_date (date)

# ---
solar_angles = metadata_api.solar_angles (zenith="64.810051", azimuth="150.464523", units="degrees")
global_metadata.set_solar_angles (solar_angles)
wrs = metadata_api.wrs(system="2", path="46", row="28")
global_metadata.set_wrs (wrs)

# ---
global_metadata.set_lpgs_metadata_file ('LT50460282002042EDC01_MTL.txt')

# ---
corner = metadata_api.corner (location="UL", latitude="47.018340", longitude="-124.013170")
global_metadata.add_corner (corner)
corner = metadata_api.corner (location="LR", latitude="45.027950", longitude="-120.949910")
global_metadata.add_corner (corner)

# ---
bounding_coordinates = metadata_api.bounding_coordinates()
bounding_coordinates.set_west (-124.013367)
bounding_coordinates.set_east (-120.875356)
bounding_coordinates.set_north (47.022955)
bounding_coordinates.set_south (45.027814)
global_metadata.set_bounding_coordinates (bounding_coordinates)

# ---
#projection_information = metadata_api.projection_information (projection="UTM", datum="WGS84", units="meters")
projection_information = metadata_api.projection_information (projection="UTM", datum="WGS84", units="meters")

corner_point = metadata_api.corner_point(location="UL", x="423000.000000", y="5207700.000000")
projection_information.add_corner_point (corner_point)
corner_point = metadata_api.corner_point(location="LR", x="661500.000000", y="4988100.000000")
projection_information.add_corner_point (corner_point)

projection_information.set_grid_origin ('CENTER')

utm_proj_params = metadata_api.utm_proj_params()
utm_proj_params.set_zone_code (10)
projection_information.set_utm_proj_params (utm_proj_params)
del projection_information.utm_proj_params
projection_information.utm_proj_params = None

global_metadata.set_projection_information (projection_information)

# ---
global_metadata.set_orientation_angle (0)

# Add the global metadata to the top-level object
xml.set_global_metadata (global_metadata)


# Create bands object
bands = metadata_api.bandsType()

# Create a band
band = metadata_api.band (product="RDD", name="band1", category="image",
    data_type="UINT8", nlines="7321", nsamps="7951", fill_value="0")

band.set_short_name ("LT5DN")
band.set_long_name ("band 1 digital numbers")
band.set_file_name ("LT50460282002042EDC01_B1.img")

pixel_size = metadata_api.pixel_size ("30.000000", 30, "meters")
band.set_pixel_size (pixel_size)

band.set_data_units ("digital numbers")

valid_range = metadata_api.valid_range (min="1", max=255)
band.set_valid_range (valid_range)

toa_reflectance = metadata_api.toa_reflectance (gain=1.448, bias="-4.28819")
band.set_toa_reflectance (toa_reflectance)

band.set_app_version ("LPGS_12.3.1")

production_date = \
    datetime.datetime.strptime ('2014-01-13T06:49:56', '%Y-%m-%dT%H:%M:%S')
band.set_production_date (production_date)

bands.add_band (band)

# Add the bands to the top-level object
xml.set_bands (bands)


# Create the output file **WITH** validation
f = open ('test-2-with-validation.xml', 'w')
metadata_api.export (f, xml)
f.close()

