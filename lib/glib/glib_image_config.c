/**
 @file glib_image_config.c

 Functions to read an image configuration file and access the information.

@verbatim
 ================ Configuration File Documentation ===============

Information on the display of images is controlled through the use
of a configuration file. Note that:

1. Any line starting with a '#' character in this file is taken as a comment.

2. Any tag or item that has embedded spaces MUST be enclosed in quotes
   for proper parsing. For example: "1.5 Km CAPPI"

The file contains two types of information blocks for products and
images. The format of these blocks is:

>>>>> PRODUCT BLOCK <<<<<

A product is a group of images to which certain attributes apply. 
The product is identified by a "product tag" which is then
referred to in the image entry block.

product <product tag>
{
   key_word = <item list>
   key_word = <item list>
   ...
}

Key words are:

    label  - The product label as seen by the end user

    class  - The type of image the group is for. Either "radar", "satellite",
	         "overlay", "underlay" or "geographic".
			 
			 The "overlay" is an image of data that will appear over the
			 satellite and radar, while "underlay" will appear under satellite,
			 radar and overlay. 

			 Geographic images do not have a time component and are meant for
			 static images such as geography or political boundaries and such.

    ctable - Either the location of a colour look up table file to be applied to
             images in the product group or the special key 'DefinedByImage'.
			 
			 The key 'DefinedByImage' means that the images associated with the 
			 product must define their own colour tables. This means that they
			 can not be changed as there will not be just one colour table for
			 all of the images included in the product.
			 (See the entry for ctable in the image block section).
			 
			 There can be more than one colour table defined and they are expected
			 to be selectable by the user. Each entry is in the form of:

                ctable = label directory_key file_name <default>

                where: label         - label to give to the look up table
                       directory_key - directory key from setup file
                       file_name     - name of file to read
                       default       - optional. If specified the given
                                       ctable is set as the default.

         Notes: 1. Look up tables come in two forms, those which assign a
                   particular pixel value to a colour and those which are
                   used with data files to assign a range of values
                   (reflectivity, cloud top temperature, etc) to a colour.
                   See image_lut.c for details.

                2. If there is no lut entry in the product block or the
                   directory_key and file_name are either missing or set to
                   a dash ("-") then the display will be the one embedded
                   in the image itself (like png).

>>>>> IMAGE BLOCK <<<<<

A particular image is identified by an "image tag" and a valid time.
This file provides other necessary information for processing the image,
such as its location and filename, as well as geo-referencing information.

Note that the order of the images in the configuration file will determine
the display order. The first image will be displayed on top of the set of
images. The second will be displayed "underneath" the first and on top of
the third and so on.

Format:

image <image_tag>
{
   key_word = <item list>
   key_word = <item list>
   ...
}

Key words

    site or label - site:  the name of the radar site (eg. "King")
                    label: the satellite or data type that the image comes
					from (eg. "GOES West", "Lightning")

    product_tag - the product group to which this image belongs. See
                  product_tag above.

		 ctable - If the product ctable entry is 'DefinedByImage' then any images
		          that require a colour table must set it here. Note that unlike
				  the product entry only one ctable can be defined. The entry is:

                  ctable = directory_key file_name

                  where: directory_key - directory key from setup file
                         file_name     - name of file to read

        encode  - how the image is encoded. This may be one of (case insensitive):

                  "none"         - the image is not encoded but is a raw raster
                                   consisting of one byte per pixel (greyscale)
                  "rgb"          - the image is not encoded but is a raw raster
                                   consisting of three bytes per pixel (colour)
                                   in the form rgbrgbrgb...
                  "any"          - the library attempts to determine the image
                                   format from the file extension and the file
                                   contents.
                  "gif"          - stored as a GIF file
                  "tiff"         - Tagged Image Format File
				  "png"          - Portable Network Graphics file
                  "xwd"          - X window dump format
                  "xgl"          - the format native to this library. See below.
                  "gridded_data" - data is a raw grid of scaled data values as
                                   value = pixel  scale + offset (see below)
                  "urp_gridded"  - stored as a universal radar processor data
                                   in gridded format.
                  "urp_polar"    - stored as URP format in range-theta format
				  "FpaMetafile"  - the image or raster of scaled values is
				                   in an FPA metafile. The actual determination
								   as to which type is done from the meta data
								   in the file.

force_grayscale - "true" or "false" (default). If set the image is forced to
                  grayscale. This is provided in case the image (especially
                  satellite) had a colour label added. This would cause the
                  entire image to be treated as a colour image, which is not
                  usually wanted.

   aspect_ratio - "adjust" or "fixed" (default). The aspect ratio of the
                  image is allowed to change depending on the viewport setting
                  or must remained fixed.

For all image files (such as gif, png or tiff)

    transparent - <name> | <r> <g> <b> [closest]
	              Set the given colour to transparent. Either the name of a colour
				  or the red, green and blue values of the colour to be taken as
				  transparent followed by an optional keyword "closest". The
				  optional key is only used for image files with a 256 element 
				  colour table, like GIF, and will use the colour closest to the
				  requested transparent colour. Useful in cases when colours like
				  white are "sort of" white.
				  (example: transparent = white closest, transparent = 201 76 158).
				  More than one transparent entry is allowed and all of the
				  designated colours will be taken as transparent.

    print_cmaps - True or False. For images with 256 element colour tables this will
                  print the tables if in diagnostic mode. For temporary use to see
	 			  what colours are used in a image. Default False.

For all images except geographic types				  

    directory   - <dir_key> <dir> : The directory where the images are
	              to be found. Note that this does not apply to geographic
				  images (see geographic images below).

                  dir_key - the base directory key name of the image
                            directories as found in the setup file, or
							an absolute pathname of a directory (starts
							with the character '/').
                  dir     - directory where the images are to be found
				            starting from dir_key (so this could be an entry
							like radar/wkr).


    fname_mask  - The mask which is used to read the list of files from
                  the above directory. For example ............_WSO_CAPPI
                  will return all files starting with any characters in
                  the first 12 positions followed by the string _WSO_CAPPI.
                  Thus the file 200005132130_WKR_CAPPI_RAIN.GIF would be
                  a match. If not specified the default is all files ("").

    fname_time  - The parsing format for the time contained in the file
                  name in the form of <format> <parse keys> <Format> is the
                  way to parse the string in standard c sscanf style notation
                  and <parse keys> give the meaning of each of the elements
                  parsed. In the above example the file starts with the
                  date, so the entry %4d%2d%2d%2d%2d YYYY MM DD hh mm
                  will parse it. If there was a string in front of the
                  date this would need to be included in the format string.
                  For example if we had RT............_WSO_CAPPI then the
                  format would be RT%4d%2d%2d%2d%2d YYYY MM DD hh mm

                  The default is "%4d%2d%2d%2d%2d YYYY MM DD hh mm"

                  The recognized parse keys are:

                  YYYY - 4 digit year
                  YY   - 2 digit year (the last 2 digits of the year)
                  JJJ  - 3 digit julian day
                  MM   - 2 digit month
                  DD   - 2 digit day of month
                  hh   - 2 digit hour of the day
                  mm   - 2 digit minute of the hour

                  Note that the above are case sensitive.
				  

 time_frequency - The time between images. If no time information is required
                  enter "none" to avoid complains from the configuration reader.

                  The time frequence information consists of four entries:

                     1. the start time as an offset from 00 GMT
                     2. the cycle time
                     3. the time window before the expected time that
                        an image is considered to have arrived on time
                     4. the time window after the expected time that
                        an image is considered to have arrived on time

                  All time entries can be specfied in minutes (m) or hours and
                  minutes in the format h:m (for example 15 or 1:10).

                  The acceptance window is a time within which an image can
                  arrive and still be considered to be on time. Some satellite
                  images that nominally arrive on the half hour may actually
                  arrive at times like 10:32, thus an acceptance window of 2
                  minutes would consider this to be the 10:30 image. Typical
                  entries might be:

                     radar     -> 0:0 10 0 0
                     satellite -> 0:0 15 2 5

                  If a satellite repeated every half hour starting on the quarter
                  hour, plus or minus 2 minutes, the entry would be:

                     15 30 2 2


The following are required for radar, satellite and data images only (like GIF)
and satellite raw raster data files. If projection and mapdef are specified for
URP or TDF files, then the enties in this file will override the information
in the data file header. If not used, mapdef should be specified as "none" or
the configuration reader will complain.

    projection  - the map projection the image is in. The entries in this
                  line will depend on the projection but two common ones:

                  radar (URP projection)
                  polar_stereographic north 60N

    mapdef      - the map definition of the image. The entries are:

                  lat lon ref-lon llx lly urx ury units

                  lat     - latitude
                  lon     - longitude
                  ref-lat - reference latitude
                  llx     - lower left corner x value wrt center of map (Km)
                  lly     - lower left corner y value wrt center of map (Km)
                  urx     - upper right corner x value wrt center of map (Km)
                  ury     - upper right corner y value wrt center of map (Km)
                  units   - factor to turn measurement into km

                  Note that in radar numeric files the mapdef can be set to "none"
                  and the appropriate map definition information will be read from
                  the files.


Keys for radar image files only (like GIF)

    radar_overlay  - if the image has the geography in specific planes in the
                     image this can be used to remove them. Entries are
                     <flag> <mask>

                     flag - either "on" or "off" 
                     mask - the 8-bit mask indicating which bits in the image are
                            to be used or rejected. For example: 00111111

    radar_bgnd     - do we want the image background to be transparent or opaque.
                     values are "transparent" or "opaque"

    radar_bg_color - Either the name of a colour, like 'black' or the rgb (red,
			         green, blue) values of the colour to be taken as transparent
	                 if radar_bgnd is set to "transparent". The default is black.

    radar_extent   - display the entire image or just the part where the radar
                     scan is? Entries are "data" or "full"

    radar_centre   - where in the image the center of the radar scan can be found.
                     entries are <width> and <height> in pixels.

    radar_diameter - the diameter of the radar scan part of the image in pixels.

    range_rings    - Either "true" or "false". If true the library is allowed to
                     overlay range rings on the image. Many radar images have range
                     rings already in the image so normally this key word is either
                     left out or set to "false".

For satellite images only

   mapdef_file  - Some images, such as those received from polar obiting satellites
                  have a map definition that changes from one image to another. This
                  entry defines the name of the file that contains the map definition
                  amd/or projection. There are two forms the files can take:

                  1. The file contains the definitions for one image and the file name
                     identifies which image the information is for. The file name is
                     in the form of <format> <encode keys> <Format> is the way to
                     encode the file name in standard C sprintf style notation and
                     <encode keys> give the meaning of each of the elements. For example:

                         %2.2d%2.2d%2.2d%2.2d%2.2d_WSO_CAPPI.mapdef  YY MM DD hh mm

                     where the time is encoded according to the rules defined
                     above for fname_time. The file must contain the definitions
                     in the format given in the discussion of projection and mapdef
                     above although the equal ("-") sign is not required.

                  2. The file contains the definitions and/or projections for more
                     than one image. In this case the item list contains only one entry
                     which is the name of the file. For example:

                         mapdefs

                     Each line in the given file must be in the form of:

                         image_file_name key <paramters>

                     where image_file_name is of course the name of the image file,
                     key is one of "mapdef" or "projection", and the parameters are
                     as described in the discussion above.

                  The files are assumed to be in the same directory as the images.

                  Note that the map definition and projection must still be specified
                  in this file for use as the default if the file is not found or if
                  only the map definition is specified in the file.


For satellite or data with encode key "gridded_data" only.

    element         - the name of the element encoded into the grid
                      (eg. Temperature, Lightning)

    size            - <width> <height>

    bytes_per_pixel - the number of bytes (8-bit chunks) per pixel (must be
                      either 1, 2 or 3)

    byte_order      - The endian order of the image pixels. Either MSBFirst for
                      Most Significant Byte First or LSBFirst for Least
                      Significant Byte First. If not specified the default
                      is MSBFirst. Only required for bytes_per_pixel > 1.

    scale           - both scale and offset are used to turn the pixels into the
    offset            values represented by the pixels and are applied as

                         pixel  scale + offset.

                      For example if could top temperature was encoded as degrees
                      kelvin  100 and we wanted degrees celsius out then the scale
                      and offset values would be:

                       scale = .01 and offset = -273.15

					   

For encode key "none" only

    size       = <width> <height>

For geographic images only

	file = <dir_key> <file> : The file which contains the geographic image.

		  dir_key - the base directory key name of the image directories
		            as found in the setup file, or an absolute pathname
					of a directory (starts with the character '/').
		  file    - the full path name of the file starting from
		            dir_key.


XGL image file format.

This is the native format of this library. It is a very simple uncompressed raster
format and consists of a 24 byte header block followed by the image data. The file
starts with "xglraster". This is followed by a byte that is the type of image
storage. The options are 'G','R','P' and mean:

   'G' = 1 byte per pixel (grayscale)
   'R' = 3 bytes per pixel stored as RGB triplets: RGBRGBRGB...
   'P' = 3 bytes per pixel stored as RGB in plane order: RRRR...GGGG...BBBB

This is followed by a 14 byte block that holds the image size stored as width x height.
Note that the number of bytes in the image will be width  height * bytes per pixel.
Thus the file size (bytes) will be: 24 + width  height * bytes-per-pixel.

Header example: "xglrasterR1024x768      "



========= The following is an example of an image definition file ===================
  

# RADAR

product	cappi
{
	label = "1.5Km CAPPI"
	class = radar
	ctable = "Default" - - default
	ctable = "King" ctables KingEnhancement.tab
}

product cappi_numeric
{
	label  = "1.5Km Numeric CAPPI"
	class  = radar
	ctable = "MM/HR"    ctables RadarData_MMHR.tab default
	ctable = "> 1MM/HR"	ctables RadarData_HRR.tab
	ctable = "DBZ"		ctables RadarData_DBZ.tab
}

product cappi_range_theta
{
	label  = "1.5Km Range Theta CAPPI"
	class  = radar
	ctable = "MM/HR"	ctables RadarData_MMHR.tab default
	ctable = "> 1MM/HR"	ctables RadarData_HRR.tab
	ctable = "DBZ"		ctables RadarData_DBZ.tab
}

product doppler
{
	label  = "Doppler"
	class  = radar
	ctable = "M/S" ctables RadarData_VRPPI.tab
}


product	24_hr_precip
{
	label = "24 Hour Precipitation"
	class = radar
}


image	WKR_CAPPI
{
	label          = "King Radar 1.5 CAPPI"
	directory      = images radar/WKR
	fname_mask     = ............:CAPPI,1.5,AGL,MPRATE:WKR_CAPPI_RAIN
	fname_time     = %4d%2d%2d%2d%2d YYYY MM DD hh mm
	site           = King
	product_tag    = cappi
	encode         = gif
	radar_overlay  = on 00111111
	radar_bgnd     = transparent
	radar_extent   = data
	radar_centre   = 240 240
	radar_diameter = 480
	projection     = radar
	mapdef         = 43:57:50N 79:34:27W 79:34:27W -240 -240 240 240 1000
}

image	WKR_CAPPI_NUMERIC
{
	label          = "King 1.5 Km Numeric Data"
	directory      = images radar/WKR
	fname_mask     = ............:CAPPI,1.5,AGL,MPRATE:WKR_BINARY
	fname_time     = %4d%2d%2d%2d%2d YYYY MM DD hh mm
	site           = King
	product_tag    = cappi_numeric
	encode         = urp
	projection     = radar
	mapdef         = none
}

image	WKR_CAPPI_RANGE_THETA
{
	label          = "King 1.5 Km Range-Theta Data"
	directory      = images radar/WKR
	fname_mask     = ............:CAPPI,1.5,AGL,MPRATE.meta
	fname_time     = %4d%2d%2d%2d%2d YYYY MM DD hh mm
	site           = King
	product_tag    = cappi_range_theta
	encode         = urp
	projection     = radar
	mapdef         = none
}


image	WKR_DOPPLER
{
	label          = "King Doppler Data"
	directory      = images radar/WKR
	fname_mask     = ............:VRPPI,DOPVOL1_A,18.meta
	fname_time     = %4d%2d%2d%2d%2d YYYY MM DD hh mm
	site           = King
	product_tag    = doppler
	encode         = urp
	projection     = radar
	mapdef         = none
}


image	WKR_24hr
{
	label          = "King 24hr Precip (1.5 CAPPI)"
	directory      = images radar/WKR_24hr
	fname_mask     = 200005......_wkr_rfa_cappi
	fname_time     = %4d%2d%2d%2d%2d YYYY MM DD hh mm
	site           = King
	product_tag    = 24_hr_precip
	encode         = gif
	radar_overlay  = on 00111111
	radar_bgnd     = transparent
	radar_extent   = data
	radar_centre   = 240 240
	radar_diameter = 480
	projection     = radar
	mapdef         = 43:57:50N 79:34:27W 79:34:27W -240 -240 240 240 1000
}


# SATELLITE

product	visible
{
	label = "Visible"
	ctable = "Greyscale" ctables VISenhance.tab
}

product	ir
{
	label  = "IR"
	ctable = "Greyscale" ctables IRenhance.tab
	ctable = "Alternate VIS" ctables VISenhance.tab
}

product cct
{
	label  = "Cloud Top Temperature"
	ctable = "Greyscale" ctables SatelliteData_CCT.tab
}


image	east_canada_vis
{
	label          = "GOES-8 Eastern Canada"
	directory      = images GOES/vis
	fname_mask     = EC.............ch1
	fname_time     = EC.%2d%2d%2d.%2d%2d YY MM DD hh mm
	product_tag    = visible
	encode         = none
	size           = 640 480
	projection     = polar_stereographic north 60N
	mapdef         = 45N 75W 75W -1280 -960 1280 960 1000
}


image	east_canada_ir
{
	label          = "GOES-8 Eastern Canada"
	directory      = images GOES/ir
	fname_mask     = EC.............ch4
	fname_time     = EC.%2d%2d%2d.%2d%2d YY MM DD hh mm
	product_tag    = ir
	encode         = none
	size           = 640 480
	projection     = polar_stereographic north 60N
	mapdef         = 45N 75W 75W -1280 -960 1280 960 1000
}

image	cloud_top_temp
{
	label           = "GOES-8 Eastern Canada"
	directory       = images GOES/cloud_top_temp
	fname_mask      = IR.............ch4
	fname_time      = IR.%2d%2d%2d.%2d%2d YY MM DD hh mm
	product_tag     = cct
	encode          = none
	size            = 640 480
	bytes_per_pixel = 2
	scale           = 0.01
	offset          = -273.15
	projection      = polar_stereographic north 60N
	mapdef          = 45N 75W 75W -1280 -960 1280 960 1000
}

# GEOGRAPHICAL IMAGES

product geographic
{
	label = "Geographic Backgrounds"
	class = geographic
}

image terrainHeight
{
	label       = "Terrain Height"
	directory   = images geography/terrain_height.png
	product_tag = geographic
	encode      = png
	projection  = polar_stereographic north 60N
	mapdef      = 45N 75W 75W -1280 -960 1280 960 1000
}



=============== End of example file ===============
@endverbatim
*/

/***********************************************************************
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/
#include <ctype.h>
#include <sys/stat.h>
#include <environ/environ.h>
#include "glib_private.h"

/* This is the keyword used in the setup file to recognize the image
 * configuration file.
 */
#define CONFIG_KEY "image"


/* Set the center point and diameter of the actual radar imaging section of a
*  standard AES radar image as sent in a gif file.
*/
#define RADAR_X     240
#define RADAR_Y     240
#define RADAR_DIAM  480
#define RADAR_MASK	0x3F

/* For use by product LUT */
#define DEFINED_BY_IMAGE	INT_MAX


/* This structure defines the list of key words used in the config
 * file that are associated with the available image encodings. The
 * encodings are defined in glib.h
 */
typedef struct {
	STRING              key;
	enum IMAGE_ENCODING encoding;
} ENCODESTRUCT;

static const ENCODESTRUCT EncodingList[] = {
	{"any",               ImageEncodingAny        },
	{"none",              ImageEncodingNone       },
	{"none_rgb",          ImageEncodingRGB        },
	{"rgb",               ImageEncodingRGB        },
	{"gif",               ImageEncodingGIF        },
	{"png",               ImageEncodingPNG        },
	{"tiff",              ImageEncodingTIFF       },
	{"tif",               ImageEncodingTIFF       },
	{"xwd",               ImageEncodingXWD        },
	{"xgl",               ImageEncodingXGL        },
	{"gridded",           ImageEncodingGridded    },
	{"gridded_data",      ImageEncodingGridded    },
	{"GriddedData",       ImageEncodingGridded    },
	{"FpaMetafile",       ImageEncodingFpaMetafile},
	{"Metafile",          ImageEncodingFpaMetafile},
	{"DataPNG",           ImageEncodingDataPNG    },
	{"data_png",          ImageEncodingDataPNG    },
	{"GriddedPNG",        ImageEncodingDataPNG    },
	{"gridded_png",       ImageEncodingDataPNG    },
	{"urp_gridded",       ImageEncodingGriddedURP },
	{"UrpGridded",        ImageEncodingGriddedURP },
	{"gridded_radar",     ImageEncodingGriddedURP },
	{"GriddedRadar",      ImageEncodingGriddedURP },
	{"urp_polar",         ImageEncodingPolarURP   },
	{"UrpPolar",          ImageEncodingPolarURP   },
	{"polar_radar",       ImageEncodingPolarURP   },
	{"PolarRadar",        ImageEncodingPolarURP   },
	{"gridded_satellite", ImageEncodingGridded    },
	{"GriddedSatellite",  ImageEncodingGridded    }
};

static const int NumEncodingList = (int)(sizeof(EncodingList)/sizeof(ENCODESTRUCT));
/* Only the first 19 are allowed for geographic images */
static const int NumGeographicEncoding = 19;


/* Time element keys for the fparm variable.
 */
enum { T_NONE, T_YYYY, T_YY, T_JJJ, T_MM, T_DD, T_hh, T_mm };

/* Global variables to hold image keys and related info from the image */
/* control file */
static	LOGICAL    CfileReady   = FALSE;
static  LOGICAL    CfileMissing = FALSE;
static  int        NumProdDefs  = 0;
static  IMPRODDEF *ProdDefs     = (IMPRODDEF *)0;
static	int        NumImdefs    = 0;
static	IMDEF     *Imdefs       = (IMDEF *)0;

/* forward local function definitions */
static int        date_to_minutes        (int, int, int, int);
static void       find_valid_times       (IMDEF*);
static IMPRODDEF *get_product_definition (STRING);
static STRING	  parse_image_tstamp     (STRING, STRING, int *);
static void 	  read_image_config      (void);





/*============== PUBLIC FUNCTIONS ===================*/


/******************************************************************************/
/**
*  @brief Return the image type.
*
*  @param[in] tag   The image or product tag or product identifier the type
*                   is wanted for.
*
*  @return The image type enumeration.
*/
/******************************************************************************/
enum IMAGE_TYPE glImageInfoGetType( STRING tag )
{
	IMPRODDEF *id;
	IMDEF     *im = _xgl_image_info_definition(tag);
	if (im) return im->prod->image_type;
	id = get_product_definition(tag);
	if (id) return id->image_type;
	return ImageTypeUnknown;
}


/******************************************************************************/
/**
*  @brief Return true if the image is a data type (can be sampled).
*
*  @param[in] tag   The image or product tag the type is wanted for.
*
*  @return True or False
*/
/******************************************************************************/
LOGICAL glImageInfoIsDataType( STRING tag )
{
	IMDEF *im = _xgl_image_info_definition(tag);
	if (im)
	{
		return (im->info.encoding == ImageEncodingGriddedURP ||
				im->info.encoding == ImageEncodingPolarURP   ||
				im->info.encoding == ImageEncodingGridded    ||
				im->info.encoding == ImageEncodingDataPNG      );
	}
	return FALSE;
}



/******************************************************************************/
/**
*  @brief Return a list of products associated with a specific type of image.
*
*  @param[in]   class  The type of images that products are wanted for.
*  @param[out]  tags   A list of tags that identify the products
*  @param[out]  labels  A list of labels for the the products
*
*  @return  The number of products in the lists.        
*
*  @attention  Tags and products are returned in allocated arrays, which must
*              be freed when no longer needed by using glFree. The array
*              elements (eg. tags[n]) are NOT to be freed as they point to
*              internal static values.
*/
/******************************************************************************/
int glImageInfoGetProducts( enum IMAGE_TYPE class, STRING **tags, STRING **labels )
{
	int		i;
	int		np     = 0;
	STRING	*tlist = (STRING *)0;
	STRING	*plist = (STRING *)0;
	IMPRODDEF *id;

	if (tags)   *tags   = (STRING *)0;
	if (labels) *labels = (STRING *)0;

	if (CfileMissing) return 0;
	if (!CfileReady) read_image_config();

	for (i = 0; i < NumProdDefs; i++)
	{
		id = ProdDefs + i;
		if (class != id->image_type) continue;

		np++;
		if (tags)
		{
			tlist = GETMEM(tlist, STRING, np);
			tlist[np-1] = id->tag;
		}
		if (labels)
		{
			plist = GETMEM(plist, STRING, np);
			plist[np-1] = id->label;
		}
	}

	if (tags)   *tags   = tlist;
	if (labels) *labels = plist;
	return np;
}




/******************************************************************************/
/**
*  @brief Return a list of image tags and sites associated with a product.
*
*  @param[in]   prod_tag  The product tag
*  @param[out]  tags   A list of tags that identify the images
*  @param[out]  sites  A list of sites for the images
*
*  @return  The number of images in the lists.        
*
*  @attention  Tags and sites are returned in allocated arrays, which must
*              be freed when no longer needed by using glFree. The array
*              elements (eg. tags[n]) are NOT to be freed as they point to
*              internal static values.
*/
/******************************************************************************/
int glImageInfoGetProductImages( STRING prod_tag, STRING **tags, STRING **sites )
{
	int		i;
	int		np     = 0;
	STRING	*tlist = (STRING *)0;
	STRING	*slist = (STRING *)0;
	IMDEF   *im;

	if (tags)  *tags  = (STRING *)0;
	if (sites) *sites = (STRING *)0;

	if (CfileMissing) return 0;
	if (!CfileReady) read_image_config();

	for (i = 0; i < NumImdefs; i++)
	{
		im = Imdefs + i;
		if(!same(im->prod->tag, prod_tag)) continue;

		np++;
		if (tags)
		{
			tlist = GETMEM(tlist, STRING, np);
			tlist[np-1] = im->tag;
		}
		if (sites)
		{
			slist = GETMEM(slist, STRING, np);
			slist[np-1] = im->site;
		}
	}

	if (tags)  *tags  = tlist;
	if (sites) *sites = slist;
	return np;
}



/******************************************************************************/
/**
*  @brief   Return a list of sites associated with a specific type of image.
*
*  @param[in]  class  The type of image the site list is for.
*  @param[out] sites  A list of sites for the image type.
*
*  @return   The number of items in the returned list        
*
*  @attention  Sites are returned in an allocated array, which must be freed
*              when no longer needed by using glFree. The array elements
*              (eg. sites[n]) are NOT to be freed as they point to internal
*              static values.
*/
/******************************************************************************/
int glImageInfoGetSites( enum IMAGE_TYPE class, STRING **sites )
{
	int      i, j;
	int      ns     = 0;
	STRING  *slist = (STRING *)0;
	LOGICAL  found;
	IMDEF   *id;

	if (sites) *sites = (STRING *)0;

	if (CfileMissing) return 0;
	if (!CfileReady) read_image_config();

	for (i=0; i<NumImdefs; i++)
	{
		id = Imdefs + i;

		/* Paranoia check as this should never happen */
		if (!id->prod) continue;

		/* Make sure it matches the given class */
		if (class != id->prod->image_type) continue;

		/* Paranoia check as this should never happen */
		if (blank(id->site)) continue;

		/* Make sure it is unique */
		found = FALSE;
		for (j=0; j<ns; j++)
		{
			if (!same(id->site, slist[j])) continue;
			found = TRUE;
			break;
		}
		if (found) continue;

		ns++;
		if (sites)
		{
			slist = GETMEM(slist, STRING, ns);
			slist[ns-1] = id->site;
		}
	}

	if (sites) *sites = slist;
	return ns;
}


/******************************************************************************/
/**
*  @brief   Return a tag for an image defined by a product and site.
*
*  @param[in]  prod_tag  The product tag that the image is for.
*  @param[in]  site  The site that the image is for.
*
*  @return   The tag identifying the image.
*
*  @attention   The returned tag is an internal value and must not be freed
*               or modified.
*/
/******************************************************************************/
STRING glImageInfoGetTag(STRING prod_tag, STRING site )
{
	int    i;
	IMDEF *id;

	if (CfileMissing) return NULL;
	if (!CfileReady) read_image_config();

	if (blank(prod_tag)) return NULL;
	if (blank(site)) return NULL;

	for (i=0; i<NumImdefs; i++)
	{
		id = Imdefs + i;

		/* Make sure it matches the given product and site */
		if (!same(prod_tag, id->prod->tag)) continue;
		if (!same(site, id->site)) continue;

		return id->tag;
	}
	return NULL;
}


/******************************************************************************/
/**
*  @brief  Return the tag of the product associated with an image.
*
*  @param[in] tag  The image tag.
*
*  @return   The tag of the product the image is associated with.
*
*  @attention  The returned tag is an internal static, do not free or modify.
*/
/******************************************************************************/
STRING glImageInfoGetProductTag( STRING tag	/* image tag */ )
{
	int    i;
	IMDEF *id;

	if (CfileMissing) return NULL;
	if (!CfileReady) read_image_config();

	for (i=0; i<NumImdefs; i++)
	{
		id = Imdefs + i;
		if (same(tag, id->tag)) return id->prod->tag;
	}

	return NULL;
}


/******************************************************************************/
/**
*   @brief  Return the look-up-tables (lut) associated with a product or image.
*
*   @param[in]  tag      The product or image tag.
*   @param[out] lutptr   A list of lut identifiers.
*   @param[out] lutlbl   A list of labels associated with the luts.
*   @param[out] lutdef   The default lut for the product.
*
*   @return  The number if items in the lists.
*
*   @attention  The returned lists are internal static and must not be freed
*               or modified.
*/
/******************************************************************************/
int glImageInfoGetLuts( STRING tag, ImageLUT **lutptr, STRING **lutlbl, ImageLUT *lutdef )
{
	IMPRODDEF *id;

	if (lutptr) *lutptr = (ImageLUT*)0;
	if (lutlbl) *lutlbl = (STRING *)0;
	if (lutdef) *lutdef = glNoLUT;

	id = get_product_definition(tag);
	if (!id)
	{
		IMDEF *im = _xgl_image_info_definition(tag);
		if (im) id = im->prod;
	}
	if (!id) return 0;

	if (lutptr) *lutptr = id->lutptr;
	if (lutlbl) *lutlbl = id->lutlbl;
	if (lutdef) *lutdef = id->deflut;
	return id->nlut;
}


/******************************************************************************/
/**
*  @brief Set the image look-up-table.
*
*  Set the look up tables of all image definitions corresponding to the given
*  tag or all image definitions associated with a product with the given tag
*  to the lut table given by the ImageLUT parameter.
*
*  @param[in]  tag  The image or product tag.
*  @param[in]  lut  The lut to set to.
*/
/******************************************************************************/
void glImageInfoSetLuts( STRING tag, ImageLUT lut )
{
	int n;

	if (CfileMissing) return;
	if (!CfileReady) read_image_config();

	for( n = 0; n < NumImdefs; n++ )
	{
		IMDEF *id = Imdefs + n;
		if(!same(tag,id->tag) && !same(tag,id->prod->tag)) continue;
		if(lut == glDefaultLUT)
		{
			id->info.lut = id->info.dlut;
		}
		else
		{
			id->info.lut = lut;
		}
	}
}


/******************************************************************************/
/**
*  @brief Get a product or image default colour map look-up-table.
*
*  @param[in]  tag  The image or product tag.
*/
/******************************************************************************/
ImageLUT glImageInfoGetDefaultLut( STRING tag )
{
	int n;

	if (CfileMissing) return glNoLUT;
	if (!CfileReady) read_image_config();

	for( n = 0; n < NumImdefs; n++ )
	{
		if(same(tag,Imdefs[n].tag)) return Imdefs[n].info.dlut;
	}
	for( n = 0; n < NumProdDefs; n++)
	{
		if (same(tag,ProdDefs[n].tag)) return ProdDefs[n].deflut;
	}
	return glNoLUT;
}



/******************************************************************************/
/**
* @brief Find the valid times of a given image.
*
*   @param[in] tag		the image tag
*   @param[in] sort		the sort order of the returned list. One of
*						glSORT_DESCENDING or glSORT_ASCENDING.
*						glSORT_DESCENDING puts the most recent time
*						into vtimes[0] and glSORT_ASCENDING reversed this.
*   @param[out]  vtimes	an array pointing to the valid times.
*   @param[out]  ftimes	an array pointing to the actual times.
*
*	@return		The number of items in the time lists.        
*
*	@attention	The returned vtimes and ftimes are allocated memory and
*				must be freed using glFree, but the elements of the array 
*				(vtimes[i] and ftimes[i]) must NOT be freed as they point
*				to internal	static arrays.
*
*	@note There are two possibilities:
*
*  - If there is a valid time_frequencey key in the configuration file
*    the valid times are calculated and the vtimes array will contain
*    the times at which images should be available. The ftimes array
*    will then contain entries that correspond to the vtimes array but
*    contain the actual times of the images that are available within
*    the time window defined by the time_frequency key. If no real
*    image is available then the appropriate ftimes element will be
*    NULL.
*
*  - The time_frequency key is not defined. In this case both the
*    vtimes and ftimes arrays will be identical and contain a list of
*    the times at which images actually exist.
*/
/******************************************************************************/
int	glImageInfoFindValidTimes( STRING tag, const char sort, STRING **vtimes, STRING **ftimes )
{
	int     n;
	IMDEF  *id;
	STRING *alist  = NULL;

	if (vtimes) *vtimes = (STRING*)0;
	if (ftimes) *ftimes = (STRING*)0;

	id = _xgl_image_info_definition(tag);
	if(!id) return 0;
	if(id->prod->image_type == ImageTypeGeographic) return 0;

	/* Clear previous expected time info */
	FREEMEM(id->elist);
	id->net = 0;

	find_valid_times(id);
	if(id->nvt < 1) return 0;

	/* If no cycle information just return the valid times */
	if(!id->interval)
	{
		id->net   = id->nvt;
		id->elist = INITMEM(TSTAMP, id->net);
		alist     = INITMEM(STRING, id->net);
		for( n = 0; n < id->net; n++ )
		{
			(void) safe_strcpy(id->elist[n], id->vlist[n].time);
			alist[n] = id->vlist[n].time;
		}
	}
	else
	{
		int    yr, jd, hr, min, time_min, start_min, end_min, ndx;
		STRING *vtl;

		/* We want the series start time to be equal to or prior to our first image. */
		if(!parse_tstamp(id->vlist[0].time, &yr, &jd, &hr, &min, NULL, NULL)) return 0;
		time_min = date_to_minutes(yr, jd, hr, min);
		start_min = ((time_min - id->interval->offset)/id->interval->cycle) * id->interval->cycle +
						id->interval->offset;

		/* We want the series to end at or after the time of the last image */
		if(!parse_tstamp(id->vlist[id->nvt-1].time, &yr, &jd, &hr, &min, NULL, NULL)) return 0;
		time_min = date_to_minutes(yr, jd, hr, min);
		end_min = ((time_min - id->interval->offset + (id->interval->cycle - 1))/id->interval->cycle) *
					id->interval->cycle + id->interval->offset;

		/* Calculate the number of elements in the series */
		id->net = ((end_min - start_min) / id->interval->cycle) + 1;

		id->elist = INITMEM(TSTAMP, id->net);
		alist     = INITMEM(STRING, id->net);

		/* closest_tstamp needs valid times in an array */
		vtl = INITMEM(STRING, id->nvt);
		for( n = 0; n < id->nvt; n++ )
			vtl[n] = id->vlist[n].time;

		/* Convert the minutes since 1970 back to year format */
		yr  = 1970; jd  = 1; hr  = 0; min = start_min;
		tnorm(&yr, &jd, &hr, &min, NULL);

		/* Fill in the series */
		for( n = 0; n < id->net; n++ )
		{
			(void) safe_strcpy(id->elist[n], build_tstamp(yr, jd, hr, min, FALSE, TRUE));
			ndx = closest_tstamp(id->nvt, vtl, id->elist[n], 0, id->interval->btw, id->interval->atw, 0, 0);
			if(ndx >= 0) alist[n] = id->vlist[ndx].time;
			min += id->interval->cycle;
			tnorm(&yr, &jd, &hr, &min, NULL);
		}
		FREEMEM(vtl);
	}


	/* Put output arrays into requested order */
	if(id->net > 0)
	{
		STRING *etimes, *atimes;

		etimes = INITMEM(STRING, id->net);
		atimes = INITMEM(STRING, id->net);

		if( sort == glSORT_ASCENDING )
		{
			for(n = 0; n < id->net; n++ )
			{
				etimes[n] = id->elist[n];
				atimes[n] = alist[n];
			}
		}
		else
		{
			for(n = 0; n < id->net; n++ )
			{
				etimes[id->net-1-n] = id->elist[n];
				atimes[id->net-1-n] = alist[n];
			}
		}
		if (vtimes) *vtimes = etimes; else FREEMEM(etimes);
		if (ftimes) *ftimes = atimes; else FREEMEM(atimes);
	}
	FREEMEM(alist);
	return id->net;
}



/******************************************************************************/
/**
*   @brief  Return a list of directories in which the images are to be found.
*
*   @param[out]  list   A returned list of directories which contain images.
*
*   @return   The number of elements in the list.
*   
*   @attention   The returned directory list is allocated and must be freed
*                using glFree. The list elements must NOT be freed as they
*                point to an internal static array elements.
*
*   @note        If the list parameter is NULL then the function will just
*                return the number of directories.
*/
/******************************************************************************/
int glImageInfoGetDirectories(STRING **list)
{
	static int    nl   = 0;
	static STRING *lst = NULL;

	if (!CfileMissing && !lst)
	{
		int i, n;

		if (!CfileReady) read_image_config();

		lst = INITMEM(STRING, NumImdefs);

		/* Check for repititions in the directories when making our list
		 */
		for(nl = 0, i = 0; i < NumImdefs; i++)
		{
			for(n = 0; n < nl; n++)
			{
				if(same(lst[n], Imdefs[i].dir)) break;
			}
			if( n >= nl && !blank(Imdefs[i].dir))
			{
				lst[nl] = Imdefs[i].dir;
				nl++;
			}
		}
	}
	if (list) *list = lst;
	return nl;
}



/*==================== Internal Library Functions ======================*/


/* Return the file path to the file containing the image or NULL if
 * no path can be made. This will be the case if tag is unrecognized
 * or if the valid time does not exist.
 *
 * Note: The filename returned is an allocated string, which must be
 *       freed when no longer needed.
 */
static int timefind( const void *a1, const void *a2 )
{
	return (compare_tstamps((STRING)a1, ((IMVTLS *)a2)->time, 0));
}

STRING _xgl_make_image_file_path( STRING tag, STRING vtime )
{
	STRING	path;
	IMVTLS *tm;
	IMDEF  *id;

	id = _xgl_image_info_definition(tag);
	if (!id) return NULL;
	if (id->prod->image_type == ImageTypeGeographic)
		return safe_strdup(id->dir);

	if (id->nvt < 1) find_valid_times(id);

	tm = (IMVTLS*)bsearch(vtime, id->vlist, (size_t)id->nvt, sizeof(IMVTLS), timefind);
	if(!tm) return NULL;

	path = pathname(id->dir, tm->file);
	return safe_strdup(path);
}


IMDEF *_xgl_image_info_definition( STRING tag )
{
	int    i;
	IMDEF *id;

	if (CfileMissing) return (IMDEF*) 0;
	if (!CfileReady) read_image_config();

	for (i=0; i<NumImdefs; i++)
	{
		id = Imdefs + i;
		if (same(tag, id->tag)) return id;
	}

	return (IMDEF*) 0;
}


LOGICAL _xgl_parse_projection(STRING proj, MAP_PROJ *mproj)
{
	char    line[201];
	char    ptype[50];
	char    p1[25], p2[25], p3[25], p4[25], p5[25];
	LOGICAL ok;

	line[200] = '\0';
	(void) strncpy(line, proj, 200);

	/* Read the projection type */
	(void) strncpy_arg(ptype, 50, line, &ok);
	if (!ok) return FALSE;

	/* Read the projection parameters */
	(void) strncpy_arg(p1, 25, line, &ok);
	(void) strncpy_arg(p2, 25, line, &ok);
	(void) strncpy_arg(p3, 25, line, &ok);
	(void) strncpy_arg(p4, 25, line, &ok);
	(void) strncpy_arg(p5, 25, line, &ok);

	/* Build the projection */
	if(!define_projection_by_name(&(mproj->projection), ptype, p1, p2, p3, p4, p5))
	{
		pr_error("[parse_projection]", "Unrecognized projection \"%s\"\n", ptype);
		return FALSE;
	}
	return TRUE;
}



/* Parse a string containing a map definition. See glib_image_config.c for details.
 * The input "none" is a special case that is legal for data files that have a map
 * definition embedded in them.
 */
LOGICAL _xgl_parse_map_def(STRING mdef, MAP_PROJ *mproj)
{
	char    lval[50];
	char    line[201];
	STRING  ptr;
	float   olat, olon, lref;
	float   xmax, xmin, ymin, ymax;
	float   units;
	LOGICAL ok;

	line[200] = '\0';
	(void) strncpy(line, mdef, 200);

	ptr = string_arg(line);
	if(blank(ptr)) return FALSE;

	if(same_ic(ptr,"none")) return TRUE;

	(void) strcpy(lval, ptr);
	olat = read_lat(lval, &ok);			if (!ok) return FALSE;

	(void) strncpy_arg(lval, 50, line, &ok);	if (!ok) return FALSE;
	olon = read_lon(lval, &ok);			if (!ok) return FALSE;

	(void) strncpy_arg(lval, 50, line, &ok);	if (!ok) return FALSE;
	lref = read_lon(lval, &ok);			if (!ok) return FALSE;

	if (olon <= -180) olon +=360;
	if (lref <= -180) lref +=360;

	xmin  = float_arg(line, &ok); if (!ok) return FALSE;
	ymin  = float_arg(line, &ok); if (!ok) return FALSE;
	xmax  = float_arg(line, &ok); if (!ok) return FALSE;
	ymax  = float_arg(line, &ok); if (!ok) return FALSE;
	units = float_arg(line, &ok); if (!ok) return FALSE;

	if(mproj)
	{
		mproj->definition.olat  = olat;
		mproj->definition.olon  = olon;
		mproj->definition.lref  = lref;
		mproj->definition.xorg  = -xmin;
		mproj->definition.yorg  = -ymin;
		mproj->definition.xlen  = xmax - xmin;
		mproj->definition.ylen  = ymax - ymin;
		mproj->definition.units = units;
	}
	return TRUE;	
}



/* Some satellite images, such as those of polar orbiting satellites, change their map
 * definition on each image (at each valid time). This information has to be read from
 * a file associated with the image. Either there is one file per image or there is one
 * file containing the information for all of the images.
 *
 * Inputs: module - the name of the module to use for error reporting.
 *         im     - image definition pointer
 *         vtime  - valid time of the image
 *
 * Return: A pointer to the map projection structure if there is a valid file and
 *         projection definition, else NULL.
 */
LOGICAL _xgl_read_image_map_projection_file( STRING module, ImagePtr im, MAP_PROJ *mproj)
{
	int      i;
	char     line[1000], oline[250];
	LOGICAL  foundOne = FALSE;
	STRING   key, path, ptr, fname;
	FILE     *fp;

	static STRING mapmsg = "map definition file";

	/* If we do not have a file defined we exit */
	if(!im->imdef) return FALSE;
	if(blank(im->imdef->piffmt)) return FALSE;

	copy_map_projection(mproj, &(im->imdef->mproj));

	/* Our file name and what information we need will depend on how the
	 * map definition is stored.
	 */
	if(im->imdef->options & MultipleImagePIF)
	{
		/* We have one file for all images, so we will need the file name
		 * associated with the given valid time to use as the first key in
		 * the data file.
		 */
		IMVTLS *tm = (IMVTLS*)bsearch(im->vtime, im->imdef->vlist, (size_t)im->imdef->nvt, sizeof(IMVTLS), timefind);
		fname = (tm)? tm->file : NULL;

		/* The file name is as given in the piffmt parameter */
		path = pathname(im->imdef->dir, im->imdef->piffmt);
	}
	else
	{
		/* We have one file per image so we need to construct the name of
		 * the file from the piffmt parameter and the time parameters.
		 */
		int yr, jd, hr, min, month, day, parm[5];
		(void) parse_tstamp(im->vtime, &yr, &jd, &hr, &min, NULL, NULL);
		mdate(&yr, &jd, &month, &day);

		for( i = 0; i < 5; i++ )
		{
			switch(im->imdef->pifparm[i])
			{
				case T_YYYY: parm[i] = yr;     break;
				case T_YY:   parm[i] = yr%100; break;
				case T_JJJ:  parm[i] = jd;     break;
				case T_MM:   parm[i] = month;  break;
				case T_DD:   parm[i] = day;    break;
				case T_hh:   parm[i] = hr;     break;
				case T_mm:   parm[i] = min;    break;
				default:     parm[i] = 0;
			}
		}
		(void) snprintf(line, sizeof(line), im->imdef->piffmt, parm[0], parm[1], parm[2], parm[3], parm[4]);
		path = pathname(im->imdef->dir, line);
	}

	fp = fopen(path, "r");
	if(!fp)
	{
		pr_error(module, "Unable to open %s \"%s\"\n", mapmsg, path);
		return FALSE;
	}

	while(getvalidline(fp, line, 250, CONFIG_COMMENT_LINE_START))
	{
		(void) safe_strcpy(oline, line);

		/* If a multiple image file the first parameter will be the image
		 * file name.
		 */
		if(im->imdef->options & MultipleImagePIF)
		{
			key = string_arg(line);
			if(!same(key, fname)) continue;
			foundOne = TRUE;
		}

		/* The "=" is actually not needed for parsing */
		ptr = strchr(line,'=');
		if (ptr) *ptr = ' ';

		/* The rest of the line reading logic is the same for either type of
		 * map definition file.
		 */
		key = string_arg(line);
		if(same_ic(key, "projection"))
		{
			if(!_xgl_parse_projection(line, mproj)) goto errdef;
		}
		else if(same_ic(key, "mapdef"))
		{
			/* Read the origin info */
			if(!_xgl_parse_map_def(line, mproj)) goto errdef;
		}
		else
		{
			pr_warning(module, "Unrecognized key \"%s\" in %s \"%s\"\n", key, mapmsg, path);
		}

	}
	(void) fclose(fp);

	if(!foundOne && (im->imdef->options & MultipleImagePIF))
	{
		pr_warning(module,"Entry for image \"%s\" not found in %s \"%s\"\n", fname, mapmsg, path);
		return FALSE;
	}

	return TRUE;

errdef:
	(void) fclose(fp);
	pr_error(module, "Parse error in %s \"%s\". Line: \"%s\"\n", mapmsg, path, oline);
	return FALSE;
}



/*================ Internal Functions =======================*/


/* Find the rgb value from a colour name or a rgb triplet. The name value
 * is read from the rgb.txt file. Note that the name in the rgb.txt file
 * can contain white space.
 */
static LOGICAL find_rgb(STRING inbuf, glCOLOR *rgb)
{
	int r = -1, g = -1, b = -1;
	char line[200], name[50];
	STRING arg, rgbfile;
	LOGICAL ok;

	/* This is the default location of the rbg.txt file.
	 * Different operating systems may have a different location
	 * which can be reset using the RGB_TXT_FILE environment variable.
	 */
	const STRING default_rgbfile = "/usr/share/X11/rgb.txt";

	arg = string_arg(inbuf);
	if (blank(arg)) return FALSE;
	(void) snprintf(name, sizeof(name), "%s", arg);

	/* None of the colour names starts with a digit so if it does the
	 * input is taken as the first number of an rgb triplet.
	 */
	if(isdigit((int)name[0]))
	{
		r = atoi(name);
		g = int_arg(inbuf, &ok); if(!ok) g = -1;
		b = int_arg(inbuf, &ok); if(!ok) b = -1;
	}
	else
	{
		LOGICAL found;
		FILE    *fp;

		/* Try for a colour name from the rgb.txt file. */
		/* Use the RGB_TXT_FILE environment variable path (if available) */
		rgbfile = getenv("RGB_TXT_FILE");
		if(blank(rgbfile))
		{
			pr_error("[read_image_config]", "Environment variable RGB_TXT_FILE not set! Using default location.\n");
			/* Use the default location */
			rgbfile = default_rgbfile;
		}

		/* Open the rgb.txt file */
		fp = fopen(rgbfile,"r");
		if(!fp)
		{
			pr_error("[read_image_config]", "Unable to open %s. Use rgb values instead of colour names.\n", rgbfile);
			return FALSE;
		}

		/* Find the colour name */
		found = FALSE;
		while(getvalidline(fp, line, sizeof(line), "!"))
		{
			r = int_arg(line, &ok); if(!ok) continue;
			g = int_arg(line, &ok); if(!ok) continue;
			b = int_arg(line, &ok); if(!ok) continue;
			no_white(line); /* The colour name can have embedded spaces so an arg function cannot be used */
			if(!same_ic(name, line)) continue;
			found = TRUE;
			break;
		}
		fclose(fp);
		if(!found)
		{
			pr_error("[read_image_config]", "Unable to find the colour name '%s' in %s.\n", name, rgbfile);
			return FALSE;
		}
	}

	if( r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 ) return FALSE;

	rgb->red   = (UNCHAR) r;
	rgb->green = (UNCHAR) g;
	rgb->blue  = (UNCHAR) b;

	return TRUE;
}


/*  Fill in the valid time list structures for the given image.
 *  This list is for the times that images actually exist in
 *  the image directory.
 */
static void find_valid_times( IMDEF *id )
{
	int     i;
	STRING  tstamp;
	int     nvt   = 0;
	STRING *flist = (STRING*)0;
	STRING *vlist = (STRING*)0;

	for(i = 0; i < id->nvt; i++)
	{
		FREEMEM(id->vlist[i].file);
	}
	FREEMEM(id->vlist);
	id->nvt = 0;

	/* If there is an image manager valid time table provided we must
	 * use this, else we scan the directory for available times.
	 */
	if (id->imtab)
	{
		int		it;
		char	buf[257]="\0", *cp;
		STRING	path;
		LOGICAL	matches;
		FILE	*fd;

		/* Open the IM directory table */
		fd = fopen(id->imtab, "r");
		if (fd)
		{
			/* Check each line in the IM table */
			while ( fread((void *)buf, sizeof(char), 256, fd) > 0 )
			{
				/* Terminate the buffer */
				buf[256] = '\0';

				/* Filter based on given match strings */
				matches = FALSE;
				for(i=0; i<10; i++)
				{
					if(blank(id->imgrep[i]))
					{
						matches = TRUE;
						break;
					}
					cp = strstr(buf, id->imgrep[i]);
					if (cp == NULL) break;
				}
				if (!matches) continue;

				/* Replace non-printing characters */
				/* Leave NULL after filename */
				it = 0;
				for(i=0; i<256; i++)
				{
					if (buf[i] == '\0')
					{
						buf[i] = '@';
						if (it==0 && i>85) it = i;
					}
					if (!isprint((int)buf[i])) buf[i] = '~';
				}
				if (it>0) buf[it] = '\0';

				/* Parse valid time from the buffer */
				tstamp = parse_image_tstamp(buf+id->impos, id->ffmt, id->fparm);
				path   = pathname(id->dir, buf+85);
				pr_diag("Image", "Tstamp %s,  Path %s\n", tstamp, path);
				if (blank(tstamp) || blank(path))
				{
					pr_diag("Image", " - Incomplete info\n");
					continue;
				}
				if (!find_file(path)) 
				{
					pr_diag("Image", " - Cannot find %s\n", path);
					continue;
				}

				/* Save in buffer */
				nvt++;
				vlist = GETMEM(vlist, STRING, nvt);
				flist = GETMEM(flist, STRING, nvt);
				vlist[nvt-1] = safe_strdup(tstamp);
				flist[nvt-1] = safe_strdup(path);
			}
			(void) fclose(fd);
		}
	}
	else
	{
		int    nf;
		STRING *dlist;

		nf = dirlist(id->dir, id->fmask, &dlist);
		if(nf > 0)
		{
			/* Parse valid time from filename */
			vlist = INITMEM(STRING, nf);
			flist = INITMEM(STRING, nf);
			for(i = 0; i < nf; i++)
			{
				/* eliminate all small files - usually 0 */
				struct stat sb;
				if( stat(pathname(id->dir,dlist[i]), &sb) != 0 || sb.st_size < 10) continue;
				tstamp = parse_image_tstamp(dlist[i], id->ffmt, id->fparm);
				if(blank(tstamp))
				{
					pr_error("find_file_valid_time",
						"Unable to parse valid time from file name: file = \'%s\'  time format = \'%s\'\n",
						dlist[i], id->ffmt);
				}
				else
				{
					flist[nvt] = safe_strdup(dlist[i]);
					vlist[nvt] = safe_strdup(tstamp);
					nvt++;
				}
			}
		}
		else
		{
			pr_error("find_image_valid_times",
				"No files found in directory \'%s\' using search mask \'%s\'\n",
				id->dir, id->fmask);
		}
	}

	/* Put the values into our structure element in sorted order
	 */
	if(nvt > 0)
	{
		id->vlist = INITMEM(IMVTLS, nvt);
		(void) safe_strcpy(id->vlist[0].time, vlist[0]);
		id->vlist[0].file = flist[0];
		id->nvt = 1;

		for(i = 1; i < nvt; i++)
		{
			int n, j;
			for(n = 0; n < id->nvt; n++)
			{
				if(compare_tstamps(vlist[i], id->vlist[n].time, 0) > 0) continue;
				for(j = id->nvt; j > n; j--)
				{
					(void) safe_strcpy(id->vlist[j].time, id->vlist[j-1].time);
					id->vlist[j].file = id->vlist[j-1].file;
				}
				break;
			}
			(void) safe_strcpy(id->vlist[id->nvt].time, vlist[i]);
			id->vlist[id->nvt].file = flist[i];
			id->nvt++;
		}
	}
	FREEMEM(flist);
	FREELIST(vlist, nvt);
}


/* Convert a date into minutes since 00:00:00 GMT 1 January 1970.
 * Using minutes in the function below greatly simplifies the
 * logic for finding the series limits.
 */
static int date_to_minutes( int yr, int jd, int hr, int min )
{
	int cval = jd - 1;
	while (--yr >= 1970) cval += ndyear(yr); /* number of days since beginning */
	cval *= 1440;
	cval += hr * 60;
	cval += min;
	return cval;
}



/* Keys for the cfgerror function
 */
enum IMCFG_ERR {
	EndErr = 1, NewErr, OldErr, BadErr, BadValErr, DateErr, DateErrs, JMErr, NoClassErr,
    ClassErr, TagErr, DupTagErr, DupProdTagErr, HeadErr, EqualErr, ProdLabelErr, ProdParseErr,
	ParseErr, ProjErr, FileMaskErr, ImProdErr, SiteErr, MapErr, BadValsErr, ProdErr, ElemErr,
	TimeFreqErr, DirErr, GeoFileErr, AccessErr, EmptyMdefErr, NoMaskErr, ImageBadErr, DupEntry,
	DupProdEntry, ImageCtableDup, ImageCtableInvalid
};


/* Error output message function. All messages generated while reading the config
 * file are handled here as we can bracket the errors with the header and trailing
 * messages which put the errors into a block.
 */
static void cfgerror( enum IMCFG_ERR mode, IMDEF *im, STRING arg1, STRING arg2 )
{
	static STRING  module = "[read_image_config]";
	static LOGICAL error_found = FALSE;

	STRING tag = (im && im->tag)? im->tag:"";

	if (!error_found && mode != EndErr)
	{
		error_found = TRUE;
		pr_error(module,">>> The following are errors in the image configuration file: %s\n",
			config_file_name(CONFIG_KEY));
	}

	switch(mode)
	{
		case EndErr:
			if(error_found) pr_error(module,">>> Finished image configuration file errors.\n");
			error_found = FALSE;
			break;

		case NewErr:
			pr_error(module, "Tag block: \"%s\" - Change keyword \"%s\" to \"%s\"\n", tag, arg1, arg2);
			break;

		case OldErr:
			pr_error(module, "Tag block: \"%s\" - Old keyword \"%s\" funcionality moved to \"%s\"\n",
					tag, arg1, arg2);
			break;

		case BadErr:
			pr_error(module, "Tag block: \"%s\" - Unknown keyword \"%s\"\n", arg1, arg2);
			break;

		case ImageBadErr:
			pr_error(module, "Image block: \"%s\" - Unknown keyword \"%s\" for product type \"%s\"\n", tag, arg1, arg2);
			break;

		case BadValErr:
			pr_error(module, "Tag block: \"%s\" Keyword \"%s\" - Bad value \"%s\"\n", tag, arg1, arg2);
			break;

		case BadValsErr:
			pr_error(module, "Tag block: \"%s\" Keyword: \"%s\" - Bad values \"%s\"\n", tag, arg1, arg2);
			break;

		case ProdErr:
			pr_error(module,"Tag block: %s - Unrecognized product_tag: \"%s\"\n", tag, arg1);
			break;

		case DateErr:
			pr_error(module, "Unknown date format key \"%s\"\n", arg1);
			break;

		case DateErrs:
			pr_error(module, "The \"%s\" date key is specified more than once.\n", arg1);
			break;

		case JMErr:
			pr_error(module, "A julian day and a month or day have been specified together.\n");
			break;

		case ClassErr:
			pr_error(module, "Unrecognized class type specifier: \"%s\" in product block \"%s\"\n", arg1, arg2);
			break;

		case NoClassErr:
			pr_error(module, "Class specifier missing from product block \"%s\".\n", arg1);
			break;

		case TagErr:
			pr_error(module, "\"%s\" block encountered which does not have a valid tag.\n", arg1);
			break;

		case DupTagErr:
			pr_error(module, "\"%s\" definition encountered which has the same tag \"%s\" as another.\n", arg1, arg2);
			break;

		case DupProdTagErr:
			pr_error(module, "Image definition encountered which has the same tag \"%s\" as a product.\n", arg1, arg2);
			break;

		case HeadErr:
			pr_error(module, "Start of block encountered without \"%s\" header.\n", arg1);
			break;

		case EqualErr:
			pr_error(module, "No \"=\" found after keyword \"%s\" - skipping line.\n", arg1);
			break;

		case ProdLabelErr:
			pr_error(module, "Label missing from product \"%s\" block.\n", arg1);
			break;

		case ProdParseErr:
			pr_error(module, "Product = \"%s\" : Parsing error in line: \"%s\"\n", arg1, arg2);
			break;

		case ParseErr:
			pr_error(module, "Parsing error in line: \"%s\"\n", arg1);
			break;

		case ProjErr:
			pr_error(module, "Unrecognized projection \"%s\" for image \"%s\"\n", arg1, tag);
			break;

		case ImProdErr:
			pr_error(module, "No valid product associated with image tag \"%s\".\n", tag);
			break;

		case SiteErr:
			pr_error(module, "No site defined for image tag \"%s\".", tag);
			break;

		case MapErr:
			pr_error(module, "No map definition for image tag: \"%s\"\n", tag);
			break;

		case ElemErr:
			pr_error(module, "\"element\" undefined for %s image type with tag: %s\n", arg1, tag);
			break;

		case TimeFreqErr:
			pr_error(module, "\"time_frequency\" key not specified for image with tag: %s\n", tag);
			break;

		case FileMaskErr:
			pr_error(module, "Images \"%s\" and \"%s\" have same data directory \"%s\" and file name mask \"%s\".\n",
				tag, arg1, im->dir, im->fmask+1);
			break;

		case NoMaskErr:
			pr_error(module, "Image \"%s\" keyword \"%s\" was not specified. A default of \"%s\" was assigned.\n", tag, arg1, arg2);
			break;

		case DirErr:
			if (arg1) pr_error(module, "Directory \"%s\" is not accessable for image tag \"%s\".\n", arg1, tag);
			break;

		case AccessErr:
			if (arg1) pr_error(module, "File \"%s\" is not accessable for image tag \"%s\".\n", arg1, tag);
			break;

		case EmptyMdefErr:
			pr_error(module, "Image \"%s\" has no map projection defined but is not a data image type.\n", tag);
			break;

		case DupEntry:
			pr_error(module, "Duplicate entry for key \"%s\" in image block \"%s\".\n", arg1, tag);
			break;

		case DupProdEntry:
			pr_error(module, "Duplicate entry for key \"%s\" in product block \"%s\".\n", arg1, arg2);
			break;

		case GeoFileErr:
			pr_error(module, "Geographic image \"%s\" points to the same file as \"%s\".\n", tag, arg1);
			break;

		case ImageCtableDup:
			pr_error(module, "More than one ctable key specified for image: %s\n", tag);
			break;

		case ImageCtableInvalid:
			pr_error(module, "Image \"%s\" has ctable key when product defines a colour table.\n", tag);
			break;
	}
}


/* Given either the product identifier or tag return the product structure pointer */
static IMPRODDEF *get_product_definition( STRING tag )
{
	int		i;
	IMPRODDEF *id;

	if (CfileMissing) return (IMPRODDEF*) 0;
	if (!CfileReady) read_image_config();

	for (i=0; i<NumProdDefs; i++)
	{
		id = ProdDefs + i;
		if (same(tag, id->tag)) return id;
	}

	return (IMPRODDEF *)0;
}



static STRING parse_image_tstamp( STRING buf, STRING fmt, int *parms )
{
	int		year, month, mday, jday, hour, minute;
	int		np, ip, p[6];

	static	TSTAMP	tstamp;

	/* Fix 20050818 - set the year, month and day to default from
	 * the system clock and set everything else to a default of 0.
	 */
	systime(&year, &jday, NULL, NULL, NULL);
	mdate(&year, &jday, &month, &mday);

	jday   = 0;
	hour   = 0;
	minute = 0;

	np = sscanf(buf, fmt, p+0, p+1, p+2, p+3, p+4, p+5);
	if (np <= 0) return (STRING)0;

	for (ip=0; ip<np; ip++)
	{
		if (parms[ip] == T_NONE)
		{
			continue;
		}
		else if (parms[ip] == T_YYYY)
		{
			year = p[ip];
		}
		else if (parms[ip] == T_YY)
		{
			year = full_year(p[ip], 0);
		}
		else if (parms[ip] == T_MM)
		{
			month = p[ip];
			jday  = 0;
		}
		else if (parms[ip] == T_DD)
		{
			mday = p[ip];
			jday  = 0;
		}
		else if (parms[ip] == T_JJJ)
		{
			jday = p[ip];
		}
		else if (parms[ip] == T_hh)
		{
			hour = p[ip];
		}
		else if (parms[ip] == T_mm)
		{
			minute = p[ip];
		}
	}

	if (jday <= 0) jdate(&year, &month, &mday, &jday);
	(void) safe_strcpy(tstamp, build_tstamp(year, jday, hour, minute, FALSE, TRUE));
	return tstamp;
}


static LOGICAL parse_time_keys(STRING line, int *parm)
{
	int     i, vals[5];
	int     count[6] = {0,0,0,0,0,0};
	STRING  p, label[6] = {"year","julian day","month","day","hour","minute"};

	for( i = 0; i < 5; i++ )
	{
		vals[i] = T_NONE;
		parm[i] = T_NONE;
	}

	for( i = 0; i < 5; i++ )
	{
		if(blank(p = string_arg(line))) break;

		if(same(p,"YYYY"))
		{
			vals[i] = T_YYYY;
			count[0]++;
		}
		else if(same(p,"YY")  )
		{
			vals[i] = T_YY;
			count[0]++;
		}
		else if(same(p,"JJJ") )
		{
			vals[i] = T_JJJ;
			count[1]++;
		}
		else if(same(p,"MM")  )
		{
			vals[i] = T_MM;
			count[2]++;
		}
		else if(same(p,"DD")  )
		{
			vals[i] = T_DD;
			count[3]++;
		}
		else if(same(p,"hh")  )
		{
			vals[i] = T_hh;
			count[4]++;
		}
		else if(same(p,"mm")  )
		{
			vals[i] = T_mm;
			count[5]++;
		}
		else
		{
			cfgerror(DateErr, NULL, p, NULL);
			return FALSE;
		}
	}
	for(i = 0; i< 6; i++)
	{
		if(count[i] > 1)
		{
			cfgerror(DateErrs, NULL, label[i], NULL);
		}
	}
	if(count[1] > 0 && (count[2] > 0 || count[3] > 0))
	{
		cfgerror(JMErr, NULL, NULL, NULL);
	}

	for( i = 0; i < 5; i++ )
		parm[i] = vals[i];

	return TRUE;
}


/* The function interpret_hour_minute_string assumes that any input
 * not containing a colon is in hours. This function is a front end
 * that assumes that any input without a colon is in minutes.
 */
static int hour_minute_string_value(STRING line)
{
	TSTAMP dt;
	STRING arg = string_arg(line);
	if (!arg) return -1;

	if(strchr(arg,':')) {
		if(strlen(arg) > sizeof(TSTAMP)-1) return -1;
		(void) strcpy(dt, arg);
	} else {
		if(strlen(arg) > sizeof(TSTAMP)-2) return -1;
		(void) strcpy(dt, ":");
		(void) strcat(dt, arg);
	}
	return interpret_hour_minute_string(dt);
}


static STRING image_type_as_string(enum IMAGE_TYPE type)
{
	switch(type)
	{
		case ImageTypeRadar:      return ("radar");
		case ImageTypeSatellite:  return ("satellite");
		case ImageTypeOverlay:    return ("overlay");
		case ImageTypeUnderlay:   return ("underlay");
		case ImageTypeGeographic: return ("geographic");
	}
	return ("unknown");
}


static void assign_key(IMDEF *im, STRING *item, STRING key, STRING value)
{
	if(*item)
		cfgerror(DupEntry, im, key, NULL);
	else
		*item = safe_strdup(value);
}

/*  Read the configuration file.
 */
static void read_image_config (void)
{
	int n;
	int nimdefs = 0;
	STRING line = NULL;
	STRING arg = NULL;
	STRING path = NULL;
	LOGICAL sat_type = FALSE;
	LOGICAL rad_type = FALSE;
	LOGICAL geo_type = FALSE;
	LOGICAL okblock  = FALSE;
	LOGICAL inblock  = FALSE;
	LOGICAL nomapdef = TRUE;
	LOGICAL noproj   = TRUE;
	LOGICAL notimefreq = TRUE;
	LOGICAL argOK;
	IMDEF *id = (IMDEF *)0;
	IMPRODDEF *pd = (IMPRODDEF *)0;
	char key[256], lbuf[300];
	FILE *cfd;

	/* Parse chars */
	static const STRING	openbr  = "{";
	static const STRING	closbr  = "}";

	if (CfileMissing) return;
	if (CfileReady) return;
	if (!first_config_file_open(CONFIG_KEY, &cfd))
	{
		CfileMissing = TRUE;
		pr_warning("read_image_config", "Image configuration file \"%s\" not available.\n",
				config_file_name(CONFIG_KEY));
		return;
	}

	CfileReady = TRUE;

	/* Read the file first scanning the product blocks */
	while ((line = read_config_file_line(&cfd)))
	{
		/* Keep a copy of the line for error reporting */
		(void) safe_strcpy(lbuf, line);

		/* The equal sign is removed for parsing.
		 */
		if((arg = strchr(line,'='))) *arg = ' ';

		(void) strncpy_arg(key, 256, line, &argOK);
		if (!argOK || blank(key)) continue;

		if (!inblock)
		{
			if (same_ic(key, "product"))
			{
				if(blank((arg = string_arg(line))))
				{
					cfgerror(TagErr, NULL, key, NULL);
					(void) skip_config_file_block(&cfd);
					continue;
				}
				/* Look for duplicate product tag */
				for(n = 0; n < NumProdDefs; n++)
				{
					if(!same(arg,ProdDefs[n].tag)) continue;
					cfgerror(DupTagErr, NULL, key, arg);
				}
				if(n < NumProdDefs) continue;

				okblock = TRUE;
				NumProdDefs++;
				ProdDefs = GETMEM(ProdDefs, IMPRODDEF, NumProdDefs);
				pd = ProdDefs + NumProdDefs - 1;

				pd->tag        = safe_strdup(arg);
				pd->label      = (STRING)0;
				pd->image_type = ImageTypeUnknown;
				pd->deflut     = DEFINED_BY_IMAGE;
				pd->bsf        = glNORMAL_BRIGHTNESS;
				pd->nlut       = 0;
				pd->lutptr     = (ImageLUT*)0;
				pd->lutlbl     = (STRING *)0;
			}
			else if (same_ic(key, "image"))
			{
				(void) skip_config_file_block(&cfd);
			}
			else if (*key == *openbr)
			{
				if (okblock)
				{
					inblock = TRUE;
				}
				else
				{
					cfgerror(HeadErr, NULL, "product", NULL);
					(void) skip_to_end_of_block(&cfd);
				}
			}
		}
		else
		{
			if (same_ic(key, "class"))
			{
				arg = string_arg(line);

				/* Only looking at the first character avoids issues with
				 * the spelling of the keywords.
				 */
				if (same_start_ic(arg, "s"))
				{
					pd->image_type = ImageTypeSatellite;
				}
				else if (same_start_ic(arg, "r"))
				{
					pd->image_type = ImageTypeRadar;
				}
				else if (same_start_ic(arg, "o"))
				{
					pd->image_type = ImageTypeOverlay;
				}
				else if (same_start_ic(arg, "u"))
				{
					pd->image_type = ImageTypeUnderlay;
				}
				else if (same_start_ic(arg, "g"))
				{
					pd->image_type = ImageTypeGeographic;
				}
			}
			else if (same_ic(key, "label"))
			{
				/* Read the label */
				if (pd->label)
					cfgerror(DupProdEntry, NULL, key, pd->tag);
				else
					pd->label = strdup_arg(line);
			}
			else if (same_ic(key, "ctable"))
			{
				STRING ctab, lab, dkey, dsub;

				lab = strtok_arg(line);
				if(same_ic(lab,"DefinedByImage")) continue;

				dkey = strtok_arg(NULL);
				ctab = strtok_arg(NULL);
				dsub = strtok_arg(NULL);
				if(blank(lab) || blank(dkey) || blank(ctab))
				{
					cfgerror(ProdParseErr, NULL, pd->tag, lbuf);
					continue;
				}

				if(same(dkey,"-") || same(ctab,"-"))
				{
					int ilut = pd->nlut++;
					pd->lutptr = GETMEM(pd->lutptr, ImageLUT, pd->nlut);
					pd->lutlbl = GETMEM(pd->lutlbl, STRING,   pd->nlut);
					pd->lutlbl[ilut] = safe_strdup(lab);
					pd->lutptr[ilut] = glNoLUT;
				}
				else
				{
					ImageLUT lut = glImageReadLUT(get_path(dkey, ctab));
					if( lut != glNoLUT)
					{
						int ilut = pd->nlut++;
						pd->lutptr = GETMEM(pd->lutptr, ImageLUT, pd->nlut);
						pd->lutlbl = GETMEM(pd->lutlbl, STRING,   pd->nlut);
						pd->lutlbl[ilut] = safe_strdup(lab);
						pd->lutptr[ilut] = lut;
						if(same_start_ic(dsub,"def")) pd->deflut = lut;
					}
				}
			}
			else if (same(key, closbr))
			{
				if(pd->image_type == ImageTypeUnknown)
				{
					cfgerror(NoClassErr, NULL, pd->tag, NULL);
					FREEMEM(pd->tag);
					NumProdDefs--;
				}
				else if(blank(pd->label))
				{
					cfgerror(ProdLabelErr, NULL, pd->tag, NULL);
					FREEMEM(pd->tag);
					NumProdDefs--;
				}
				pd = (IMPRODDEF *)0;
				inblock = FALSE;
				okblock = FALSE;
			}
			else
			{
				cfgerror(BadErr, NULL, pd->tag, key);
			}
		}
	}

	/* Reopen the config file as it is automatically closed on eof.
	 */
	if (!first_config_file_open(CONFIG_KEY, &cfd))
	{
		CfileMissing = TRUE;
		return;
	}

	/* Scan the image blocks for the first time to set up the structures and to get the
	 * associated product. The image_type of the product will tell us what image config
	 * items are legitimate for a given image.
	 */
	while ((line = read_config_file_line(&cfd)))
	{
		(void) safe_strcpy(lbuf, line);

		/* The equal sign is removed for parsing.
		 */
		if((arg = strchr(line,'='))) *arg = ' ';

		(void) strncpy_arg(key, 256, line, &argOK);
		if (!argOK || blank(key)) continue;

		if (!inblock)
		{
			if (same_ic(key, "image"))
			{
				arg = string_arg(line);
				if(blank(arg))
				{
					cfgerror(TagErr, NULL, key, NULL);
					(void) skip_config_file_block(&cfd);
					continue;
				}

				/* Check for a duplicate of a product tag and warn */
				for(n = 0; n < NumProdDefs; n++)
				{
					if(!same(arg,ProdDefs[n].tag)) continue;
					cfgerror(DupProdTagErr, NULL, arg, ProdDefs[n].tag);
					break;
				}

				/* Check for a duplicate image tag */
				for(n = 0; n < NumImdefs; n++)
				{
					if(!same(arg,Imdefs[n].tag)) continue;
					cfgerror(DupTagErr, NULL, key, arg);
					break;
				}
				if(n < NumImdefs) continue;
				okblock = TRUE;

				/* Allocate memory and initialize */
				NumImdefs++;
				Imdefs = GETMEM(Imdefs, IMDEF, NumImdefs);
				id = Imdefs + NumImdefs - 1;
				(void) memset((void *)id, 0, sizeof(IMDEF));

				id->tag             = safe_strdup(arg);
				id->bsf             = glNORMAL_BRIGHTNESS;
				id->info.encoding   = ImageEncodingAny;
				id->info.dlut       = glNoLUT;
				id->info.lut        = glNoLUT;
				id->radar.orgx      = RADAR_X;
				id->radar.orgy      = RADAR_Y;
				id->radar.diam      = RADAR_DIAM;
				id->radar.mask      = RADAR_MASK;
				id->radar.options   = RadarCrop | RadarStripOverlayBits | RadarBackgroundTransparent;
				id->grid.bpp        = 1;
				id->grid.scale      = 1.0;
				id->grid.byte_order = IMAGE_BIG_ENDIAN;
				id->fparm[0]        = T_YYYY;
				id->fparm[1]        = T_MM;
				id->fparm[2]        = T_DD;
				id->fparm[3]        = T_hh;
				id->fparm[4]        = T_mm;
				id->pifparm[0]      = T_YYYY;
				id->pifparm[1]      = T_MM;
				id->pifparm[2]      = T_DD;
				id->pifparm[3]      = T_hh;
				id->pifparm[4]      = T_mm;
				define_map_projection(&(id->mproj), &NoProjDef, &NoMapDef, &NoGridDef);
			}
			else if (same_ic(key, "product"))
			{
				(void) skip_config_file_block(&cfd);
			}
			else if (same(key, openbr))
			{
				if (okblock)
					inblock = TRUE;
				else
				{
					cfgerror(HeadErr, NULL, "image", NULL);
					(void) skip_to_end_of_block(&cfd);
				}
			}
		}
		else
		{
			if (same_ic(key, "product_tag"))
			{
				arg = string_arg(line);
				if (blank(arg))
					cfgerror(ParseErr, NULL, lbuf, NULL);
				else if(!( pd = get_product_definition(arg)))
					cfgerror(ProdErr, id, arg, NULL);
				else
					id->prod = pd;
			}
			else if (same(key, closbr))
			{
				if (!id->prod)
				{
					cfgerror(ImProdErr, id, NULL, NULL);
					FREEMEM(id->tag);
					NumImdefs--;
				}
				id = (IMDEF *)0;
				inblock = FALSE;
				okblock = FALSE;
			}
		}
	}


	/* Go through the images a second time now that we have the product assignment.
	 * The image type as defined in the product is used to limit what keywords are
	 * recognized as valid for the various types and produce more error reports if
	 * extra keys are used.
	 */
	if (!first_config_file_open(CONFIG_KEY, &cfd))
	{
		CfileMissing = TRUE;
		return;
	}

	inblock = FALSE;
	okblock = FALSE;

	while ((line = read_config_file_line(&cfd)))
	{
		(void) safe_strcpy(lbuf, line);

		/* The equal sign is removed for parsing.
		 */
		if((arg = strchr(line,'='))) *arg = ' ';

		(void) strncpy_arg(key, 256, line, &argOK);
		if (!argOK || blank(key)) continue;

		if (!inblock)
		{
			if (same_ic(key, "image"))
			{
				/* The image must be in the list created above */
				arg = string_arg(line);
				for(n = 0; n < NumImdefs; n++)
					if(same(Imdefs[n].tag,arg)) break;
				if(n >= NumImdefs)
				{
					(void) skip_config_file_block(&cfd);
					continue;
				}
				okblock = TRUE;
				id = Imdefs + nimdefs++;

				sat_type = (id->prod->image_type == ImageTypeSatellite);
				rad_type = (id->prod->image_type == ImageTypeRadar);
				geo_type = (id->prod->image_type == ImageTypeGeographic);
				/* geographic type does not have a time component */
				if (geo_type) notimefreq = FALSE;
			}
			else if (same_ic(key, "product"))
			{
				(void) skip_config_file_block(&cfd);
			}
			else if (same(key, openbr))
			{
				if (okblock)
					inblock = TRUE;
				else
					(void) skip_to_end_of_block(&cfd);
			}
		}
		else
		{
			if (same_ic(key, "site") || same_ic(key, "label"))
			{
				arg = string_arg(line);
				if (blank(arg)) goto parseErr;
				assign_key(id, &id->site, key, arg);
			}
			else if (same_ic(key, "product_tag"))
			{
				/* The colour table is only taken from the product if the
				 * entry was not 'DefinedByImage'.
				 */
				if(id->prod->deflut == DEFINED_BY_IMAGE) continue;
				if(id->prod->deflut != glNoLUT)
					id->info.dlut = id->prod->deflut;
				else if(id->prod->nlut > 0 && id->prod->lutptr[0] != glNoLUT)
					id->info.dlut = id->prod->lutptr[0];
				id->info.lut = id->info.dlut;
			}
			else if (same_ic(key, "ctable"))
			{
				if(id->prod->deflut != DEFINED_BY_IMAGE)
				{
					cfgerror(ImageCtableInvalid, id, NULL, NULL);
				}
				else if(id->info.lut != glNoLUT)
				{
					cfgerror(ImageCtableDup, id, NULL, NULL);
				}
				else
				{
					STRING dkey, ctab;
					dkey = strtok_arg(line);
					ctab = strtok_arg(NULL);
					id->info.dlut = glImageReadLUT(get_path(dkey, ctab));
					id->info.lut = id->info.dlut;
					if(id->info.lut == glNoLUT)
						cfgerror(BadValErr, id, key, line);
				}
			}
			else if(same_ic(key,"transparent"))
			{
				glCOLOR trans;
				int ndx = id->info.ntransparent;
				if(!find_rgb(line, &trans)) goto parseErr;
				id->info.ntransparent++;
				id->info.transparent = GETMEM(id->info.transparent, glCOLOR, id->info.ntransparent);
				id->info.transparent[ndx].red   = trans.red;
				id->info.transparent[ndx].green = trans.green;
				id->info.transparent[ndx].blue  = trans.blue;
				arg = string_arg(line);
				id->info.closest_rgb = GETMEM(id->info.closest_rgb, LOGICAL, id->info.ntransparent);
				id->info.closest_rgb[ndx] = (same_start_ic(arg,"cl") || same_start_ic(arg,"ne"));
			}
			else if(same_start_ic(key,"print_cmap"))
			{
				arg = string_arg(line);
				id->info.print_cmaps = (!blank(arg) && (strchr("YyTt", *arg) != NULL));
			}
			else if (same_start_ic(key, "encod")) /* gets encode or encoding */
			{
				enum IMAGE_ENCODING code;
				int num = (geo_type)? NumGeographicEncoding : NumEncodingList;

				arg = string_arg(line);
				for (n = 0; n < num; n++)
					if(same_ic(arg, EncodingList[n].key)) break;
				if (n >= num) goto badValErr;

				id->info.encoding = code = EncodingList[n].encoding;

				/* Turn on range rings if dealing with URP data files */
				if (code == ImageEncodingGriddedURP || code == ImageEncodingPolarURP)
					id->radar.options |= RadarRangeRings;

				/* Any one of these do not need a map projection */
				if (code == ImageEncodingGriddedURP || code == ImageEncodingPolarURP || code == ImageEncodingDataPNG)
					noproj = FALSE;
			}
			else if (same_ic(key, "size"))
			{
				int xp, yp;
				/* Read the pixel dimensions */
				xp = int_arg(line, &argOK);	if (!argOK) goto parseErr;
				yp = int_arg(line, &argOK);	if (!argOK) goto parseErr;

				if (xp>=0 && yp>=0)
				{
					id->info.width  = xp;
					id->info.height = yp;
				}
				else
				{
					(void) snprintf(line, sizeof(line), "%d %d", xp, yp);
					cfgerror(BadValsErr, id, key, line);
				}
			}
			/* use "force_gr" only to allow for different spellings of gray (grey) */
			else if (same_start_ic(key,"force_gr"))
			{
				arg = string_arg(line);
				if(same_ic(arg,"true") || same_ic(arg,"on"))
					id->options |= ForceGreyscale;
				else if(same_ic(arg,"false") && same_ic(arg,"off"))
					id->options &= ~ForceGreyscale;
				else
					goto badValErr;
			}
			else if (same_ic(key,"aspect_ratio"))
			{
				arg = string_arg(line);
				if(same_ic(arg,"adjust"))
					id->options |= AdjustAspect;
				else if(same_ic(arg,"fixed"))
					id->options &= ~AdjustAspect;
				else
					goto badValErr;
			}
			else if (rad_type && same_ic(key, "radar_centre"))
			{
				int xc, yc;
				/* Read the pixel co-ordinates of the image centre */
				xc = int_arg(line, &argOK);	if (!argOK) goto parseErr;
				yc = int_arg(line, &argOK);	if (!argOK) goto parseErr;

				if (xc>=0 && yc>=0)
				{
					id->radar.orgx = xc;
					id->radar.orgy = yc;
				}
				else
				{
					(void) snprintf(line, sizeof(line), "%d %d", xc, yc);
					cfgerror(BadValsErr, id, key, line);
				}
			}
			else if (rad_type && same_ic(key, "radar_diameter"))
			{
				/* Read the pixel co-ordinates of the image centre */
				int diam = int_arg(line, &argOK);	if (!argOK) goto parseErr;

				if (diam > 0)
					id->radar.diam = diam;
				else
				{
					(void) snprintf(line, sizeof(line), "%d", diam);
					cfgerror(BadValErr, id, key, line);
				}
			}
			else if (rad_type && same_ic(key, "radar_overlay"))
			{
				/* Read the masking instructions (on/off) */
				arg = string_arg(line);
				if (same_ic(arg, "on"))
					id->radar.options &= ~RadarStripOverlayBits;
				else if (same_ic(arg, "off"))
					id->radar.options |= RadarStripOverlayBits;
				else
					goto badValErr;

				/* See if a bitmask is given (default is 00111111) */
				if (!blank(line))
				{
					/* Read the bitmask for the image colour table */
					UNCHAR mask = (UNCHAR) ubase_arg(line, 2, &argOK);
					if (!argOK) goto parseErr;
					id->radar.mask = mask;
				}
			}
			else if (rad_type && (same_ic(key, "radar_bgnd") || (same_ic(key, "radar_background"))))
			{
				/* Read the background mode (transparent/opaque) */
				arg = string_arg(line);
				if (same_start_ic(arg, "t"))
					id->radar.options |= RadarBackgroundTransparent;
				else if (same_start_ic(arg, "o"))
					id->radar.options &= ~RadarBackgroundTransparent;
				else
					goto badValErr;
			}
			else if (rad_type && same_start_ic(key, "radar_bg_col")) /* color or colour */
			{
				if(!find_rgb(line, &id->radar.no_data_rgb)) goto parseErr;
			}
			else if (rad_type && same_ic(key, "radar_extent"))
			{
				/* Read the data display mode (data/full) */
				arg = string_arg(line);
				if (same_ic(arg, "data"))
					id->radar.options |= RadarCrop;
				else if (same_ic(arg, "full"))
					id->radar.options &= ~RadarCrop;
				else
					goto badValErr;
			}
			else if (rad_type && same_ic(key,"range_rings"))
			{
				/* Read the data display mode (data/full) */
				arg = string_arg(line);
				if (same_start_ic(arg, "t"))
					id->radar.options |= RadarRangeRings;
				else if (same_start_ic(arg, "f"))
					id->radar.options &= ~RadarRangeRings;
				else
					goto badValErr;
			}
			else if (same_ic(key,"element"))
			{
				/* read the name of the gridded data element */
				arg = string_arg(line);  if(blank(arg)) goto parseErr;
				assign_key(id, &id->grid.element, key, arg);
			}
			else if (same_ic(key,"byte_order"))
			{
				/* Read the data byte order (MSBFirst or LSBFirst) */
				arg = string_arg(line);
				if (same_ic(arg, "msbfirst"))
					id->grid.byte_order = IMAGE_BIG_ENDIAN;
				else if (same_ic(arg, "lsbfirst"))
					id->grid.byte_order = IMAGE_LITTLE_ENDIAN;
				else
					goto badValErr;
			}
			else if (same_ic(key,"bytes_per_pixel"))
			{
				int val = int_arg(line, &argOK);  if(!argOK) goto parseErr;
				if(val > 0 && val < 5)
				{
					id->grid.bpp = val;
				}
				else
				{
					(void) snprintf(line, sizeof(line), "%d", val);
					cfgerror(BadValErr, id, key, line);
				}
			}
			else if (same_ic(key,"scale"))
			{
				float scale = float_arg(line, &argOK);  if(!argOK) goto parseErr;
				id->grid.scale = scale;
				if (scale != 1.0) id->grid.packed = TRUE;
			}
			else if (same_ic(key,"offset"))
			{
				float offset = float_arg(line, &argOK);  if(!argOK) goto parseErr;
				id->grid.offset = offset;
				if (offset != 0.0) id->grid.packed = TRUE;
			}
			else if (!geo_type && same_ic(key, "directory"))
			{
				char dkey[256], dsub[256];
				struct stat sb;
				(void) strncpy_arg(dkey, 256, line, &argOK);	if (!argOK) goto parseErr;
				(void) strncpy_arg(dsub, 256, line, &argOK);
				if(*dkey == '/')
					path = pathname(dkey, dsub);
				else
					path = get_path(dkey, dsub);
				if(!blank(path))
				{
					assign_key(id, &id->dir, key, path);
					if(stat(id->dir,&sb) || !S_ISDIR(sb.st_mode))
						cfgerror(DirErr, id, id->dir, NULL);
				}
			}
			else if (geo_type && same_ic(key, "file"))
			{
				char dkey[256], dsub[256];
				(void) strncpy_arg(dkey, 256, line, &argOK);	if (!argOK) goto parseErr;
				(void) strncpy_arg(dsub, 256, line, &argOK);
				if(*dkey == '/')
					path = pathname(dkey, dsub);
				else
					path = get_path(dkey, dsub);
				assign_key(id, &id->dir, key, path);
				if(access(id->dir,R_OK))
					cfgerror(AccessErr, id, id->dir, NULL);
			}
			else if (!geo_type && same_ic(key, "fname_mask"))
			{
				/* Read the file mask and put the special search character in front */
				char fmask[256];
				(void) strcpy(fmask, "^");
				(void) strncpy_arg(fmask+1, 255, line, &argOK); if (!argOK) goto parseErr;
				assign_key(id, &id->fmask, key, fmask);
			}
			else if (!geo_type && same_ic(key, "fname_time"))
			{
				char ffmt[256];
				/* Read the file format */
				(void) strncpy_arg(ffmt, 256, line, &argOK); if (!argOK) goto parseErr;
				assign_key(id, &id->ffmt, key, ffmt);
				if(!parse_time_keys(line, id->fparm)) goto parseErr;
			}
			/* Note that interval_time is for backwards compatability for older key */
			else if (!geo_type && (same_ic(key,"time_frequency") || same_ic(key,"interval_time")))
			{
				notimefreq = FALSE;
				no_white(line);
				if(!same_ic(line,"none"))
				{
					int  oset, cycle, btw, atw;

					/* the image must start sometime during a day */
					oset = hour_minute_string_value(line);
					if(oset < 0 || oset > 1440) goto parseErr;

					/* cycle time must be non zero and no less than once a day */
					cycle = hour_minute_string_value(line);
					if(cycle < 1 || cycle > 1440) goto parseErr;

					/* window must be less than cycle time */
					btw = hour_minute_string_value(line);
					if(btw < 0 || btw > cycle) goto parseErr;

					atw = hour_minute_string_value(line);
					if(atw < 0 || atw > cycle) goto parseErr;

					id->interval = ONEMEM(IMTIMES);
					id->interval->offset = oset;
					id->interval->cycle  = cycle;
					(void) snprintf(id->interval->btw, sizeof(id->interval->btw), ":%d", btw);
					(void) snprintf(id->interval->atw, sizeof(id->interval->atw), ":%d", atw);
				}
			}
			else if (sat_type && same_ic(key, "mapdef_file"))
			{
				char fmask[256];
				/* Read the file name format */
				(void) strncpy_arg(fmask, 256, line, &argOK);	if (!argOK) goto parseErr;
				assign_key(id, &id->piffmt, key, fmask);

				/* Check for a multiple image file or get the data/time parameters */
				if(blank(line))
					id->options |= MultipleImagePIF;
				else if(parse_time_keys(line, id->pifparm))
					id->options &= ~MultipleImagePIF;
				else
					goto parseErr;
			}
			else if (sat_type && same_ic(key, "im_table"))
			{
				/* Read the directory key and filename */
				char fnam[256], dkey[256];
				(void) strncpy_arg(dkey, 256, line, &argOK);	if (!argOK) goto parseErr;
				(void) strncpy_arg(fnam, 256, line, &argOK);
				path = get_path(dkey, fnam);
				assign_key(id, &id->imtab, key, path);
			}
			else if (sat_type && same_ic(key, "im_match"))
			{
				id->imgrep[0] = strdup_arg(line);
				id->imgrep[1] = strdup_arg(line);
				id->imgrep[2] = strdup_arg(line);
				id->imgrep[3] = strdup_arg(line);
				id->imgrep[4] = strdup_arg(line);
				id->imgrep[5] = strdup_arg(line);
				id->imgrep[6] = strdup_arg(line);
				id->imgrep[7] = strdup_arg(line);
				id->imgrep[8] = strdup_arg(line);
				id->imgrep[9] = strdup_arg(line);
			}
			else if (sat_type && same_ic(key, "im_time"))
			{
				char ffmt[256];
				/* Read the starting byte position */
				int ipos = int_arg(line, &argOK);	if (!argOK) goto parseErr;

				/* Read the date format */
				(void) strncpy_arg(ffmt, 256, line, &argOK);	if (!argOK) goto parseErr;
				id->impos = ipos;
				assign_key(id, &id->ffmt, key, ffmt);
				if(!parse_time_keys(line, id->fparm)) goto parseErr;
			}
			else if (same_ic(key, "projection"))
			{
				if(!_xgl_parse_projection(line, &id->mproj)) goto parseErr;
				noproj = FALSE;
			}
			else if (same_ic(key, "mapdef"))
			{
				if(!_xgl_parse_map_def(line, &id->mproj)) goto parseErr;
				nomapdef = FALSE;
			}
			else if (same(key, closbr))
			{
				/* End of the image block, so some error checking can be done */
				if (!id->site)
				{
					cfgerror(SiteErr, id, NULL, NULL);
					id->site = "???";
				}

				if (!rad_type && id->info.encoding == ImageEncodingGridded && blank(id->grid.element))
				{
					cfgerror(ElemErr, id, image_type_as_string(id->prod->image_type), NULL);
					id->grid.element = safe_strdup("Data");
				}

				if(!id->fmask && !geo_type)
				{
					id->fmask = safe_strdup("*");
					cfgerror(NoMaskErr, id, "fname_mask", id->fmask);
				}

				if(!id->ffmt && !geo_type)
				{
					id->ffmt = safe_strdup("%4d%2d%2d%2d%2d");
					cfgerror(NoMaskErr, id, "fname_time", id->ffmt);
				}

				if (noproj)     cfgerror(EmptyMdefErr, &Imdefs[n], NULL, NULL);
				if (nomapdef)   cfgerror(MapErr, id, NULL, NULL);
				if (notimefreq) cfgerror(TimeFreqErr, id, NULL, NULL);

				id = (IMDEF *)0;
				inblock    = FALSE;
				okblock    = FALSE;
				nomapdef   = TRUE;
				noproj     = TRUE;
				notimefreq = TRUE;
			}
			else
			{
				cfgerror(ImageBadErr, id, key, image_type_as_string(id->prod->image_type));
			}
		}
		continue;

parseErr:
		cfgerror(ParseErr, NULL, lbuf, NULL);
		continue;
badValErr:
		cfgerror(BadValErr, id, key, arg);
		continue;
	}

	/* Reset of product lut to no lut from the special value. */
	for( n = 0; n < NumProdDefs; n++)
	{
		if(ProdDefs[n].deflut == DEFINED_BY_IMAGE)
			ProdDefs[n].deflut = glNoLUT;
	}

	/* Error checking that must wait until the end so all data is available */
	for( n = 0; n < NumImdefs; n++ )
	{
		int i;
		if(Imdefs[n].prod->image_type == ImageTypeGeographic)
		{
			for( i = n+1; i < NumImdefs; i++ )
			{
				if(Imdefs[i].prod->image_type != ImageTypeGeographic) continue;
				if(!same(Imdefs[n].dir, Imdefs[i].dir)) continue;
				cfgerror(GeoFileErr, &Imdefs[i], Imdefs[n].tag, NULL);
			}
		}
		else
		{
			for( i = n+1; i < NumImdefs; i++ )
			{
				if(!same(Imdefs[n].fmask, Imdefs[i].fmask)) continue;
				if(!same(Imdefs[n].dir, Imdefs[i].dir)) continue;
				cfgerror(FileMaskErr, &Imdefs[i], Imdefs[n].tag, NULL);
			}
		}
	}

	cfgerror(EndErr, NULL, NULL, NULL);
	return;
}

