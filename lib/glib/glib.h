/*********************************************************************************/
/*
*    glib.h
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
*/
/*********************************************************************************/

#ifndef _FPAGLIB_H
#define _FPAGLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <objects/objects.h>
/*
 * Data missing values
 */
#define glDATA_MISSING		-999
#define glNORMAL_BRIGHTNESS	-999
#define glNoLUT				0
#define glDefaultLUT		-1
#define glNoImage			0
/*
 * For "Data PNG" images
 */
#define PNGTAG_ORIGINLOCATION	"OriginLocation"
#define		PNGVAL_UPPERLEFT	"UpperLeft"
#define		PNGVAL_LOWERLEFT	"LowerLeft"
#define PNGTAG_PROJECTION		"Projection"
#define PNGTAG_MAPDEF			"MapDef"
#define PNGTAG_IMAGETYPE		"ImageType"
#define		PNGVAL_DATAIMAGE	"DataImage"
#define PNGTAG_DATASCALE		"DataScale"
#define PNGTAG_DATAOFFSET		"DataOffset"

/*
 * For memory allocation freeing consistency. This library uses
 * malloc and calloc in macros, while the function that use this
 * library are Xt based. This macro uses free to be consistent 
 * with the above allocation routines.
 */
#define glFree(x) {if(x){free((void*)(x));(x)=NULL;}}
/*
 * Types
*/
typedef float  Angle;
typedef float  Coord;
typedef float  Matrix[3][2];
typedef short  Screencoord;
typedef int    ImageLUT;
typedef int    Image, *ImageList;
typedef struct { float lat; float lon; } LatLonPoint;
typedef struct { Coord x; Coord y; } CoordPoint;
/*
 * Image types.  Note: If this list changes the static buffer image_type_bsf[] in glib_image.c
 *                     will need to be modified for the number of items in this list.
 */
enum IMAGE_TYPE {
	ImageTypeUnknown,		/* Unknown type */
    ImageTypeRadar,
    ImageTypeSatellite,
	ImageTypeOverlay,		/* Images that go over all other image types */
	ImageTypeUnderlay,		/* Images that go under all other imate types */
	ImageTypeGeographic		/* Images with no time dependence */
};
/*
 * The encoding used in the image file
 */
enum IMAGE_ENCODING {
	ImageEncodingAny,			/* Determine which format it is */
	ImageEncodingNone,			/* Image is in raw uncompressed greyscale raster format */
	ImageEncodingRGB,			/* Image is in raw uncompressed rgb pixel raster format */
	ImageEncodingGIF,			/* GIF file */
	ImageEncodingPNG,			/* PNG (Portable Network Graphics) format */
	ImageEncodingDataPNG,		/* PNG storing gridded data values along with data encoding information */
	ImageEncodingTIFF,			/* TIFF format */
	ImageEncodingXWD,			/* X window dump format */
	ImageEncodingXGL,			/* Library format file */
	ImageEncodingGridded,		/* Gridded data values stored as a raw uncompressed raster */
	ImageEncodingGriddedURP,	/* Gridded URP radar data file */
	ImageEncodingPolarURP,		/* Polar coordinate URP radar data file */
	ImageEncodingFpaMetafile,	/* RGB image or data raster in an FPA metafile */
	ImageEncodingReprojected	/* special used internally only */
};
/*
 * Raster output format
 */
enum IMAGE_RASTER_TYPE {
	glImagePixelMajor = 1,		/* rgb, rgb, ... */
	glImagePixelMajorOpaque,	/* transparent pixels converted to black (0,0,0) */
	glAlphaPixelMajor,			/* rgba, rgba, ... */
	glImagePlaneMajor,			/* rrr...ggg...bbb */
	glImagePlaneMajorOpaque,	/* transparent pixels converted to black (0,0,0) */
	glAlphaPlaneMajor			/* rrr...ggg...bbb...aaa */
};
/*
 * Colour structure
 */
typedef struct {
	UNCHAR red;
	UNCHAR green;
	UNCHAR blue;
} glCOLOR;
/*
 * This structure provides one element of a image data colour table lookup.
 */
typedef struct {
	float   lower_bound;	/* lower bound of the range taken as >= */
	float   upper_bound;	/* upper bound of the range taken as <  */
	UNCHAR  red;			/* red component of rgb assignment      */
	UNCHAR  green;			/* green component of rgb assignment    */
	UNCHAR  blue;			/* blue component of rgb assignment     */
	UNCHAR  alpha;			/* alpha value for future consideration */
} glLUTCOLOR;
/*
 *  Structure to hold colormap lookup table information.
 */
typedef struct {
	STRING     path;		/* path name of cmap file */
	STRING     id;			/* identifier used to identify contents */
	STRING     label;		/* data table label */
	STRING     item;		/* data table item label */
	int        ncells;		/* number of data-rgb specifications */
	glLUTCOLOR *cells;		/* data-rgb lut specification */
} glLUT;

/*
 * Order of returned times in arrays.
 */
#define glSORT_ASCENDING	'a'
#define glSORT_DESCENDING	'd'

/*
 * Function prototypes
 */
extern void  glSetWindow           ( int );
extern int   glGetWindowId         ( void );
extern void  glGetMapPixelSize     ( float*, float* );
extern void  glVdcUnitPerMapUnit   ( float*, float* );
extern void  glMapUnitPerVdcUnit   ( float*, float* );
extern void  glOrtho               ( Coord, Coord, Coord, Coord );
extern void  glGetOrtho            ( Coord*, Coord*, Coord*, Coord* );
extern void  glViewport            ( Screencoord, Screencoord, Screencoord, Screencoord );
extern void  glMapViewport         ( Coord, Coord, Coord, Coord );
extern void  glVdcViewport         ( Coord, Coord, Coord, Coord );
extern void  glGetViewport         ( Screencoord*, Screencoord*, Screencoord*, Screencoord* );
extern void  glHatchAngle          ( float );
extern void  glHatchCrossAngle     ( float );
extern void  glHatchSpacing        ( int );
extern void  glMapHatchSpacing     ( float );
extern void  glVdcHatchSpacing     ( float );
extern void  glScreen2Map          ( Screencoord, Screencoord, Coord*, Coord* );
extern void  glMap2Screen          ( Coord, Coord, Screencoord*, Screencoord* );
extern void  glVdc2Map             ( Coord, Coord, Coord*, Coord* );
extern void  glMap2Vdc             ( Coord, Coord, Coord*, Coord* );
extern void  glPushWindow          ( void );
extern void  glPopWindow           ( void );
extern void  glSetClipRectangle    ( Screencoord, Screencoord, Screencoord, Screencoord );
extern void  glGetClipRectangle    ( Screencoord*, Screencoord*, Screencoord*, Screencoord* );
extern void  glSetMapClipRectangle ( Coord, Coord, Coord, Coord );
extern void  glSetVdcClipRectangle ( Coord, Coord, Coord, Coord );

  
/* Creation and initialization */
extern void  glVirtualInit         ( void );
extern int   glCreateVirtualWindow ( int, int );
extern void  glCloseWindow         ( int );
extern void  glExit                ( void );
extern void  glSetWorkingDirectory ( char* );

/* Image */
extern void     glImageBlendRatio             ( Image, int );
extern Image    glImageBlend                  ( Image, MAP_PROJ*, Image, Image, int );
extern Image    glImageCombine                ( Image, MAP_PROJ*, ImageList, int );
extern Image    glImageComposite              ( Image, MAP_PROJ*, ImageList, int, int);
extern ImageLUT glImageCreateLUT              ( UNCHAR*, UNCHAR*, UNCHAR* );
extern ImageLUT glImageCreateTransparentLUT   ( UNCHAR*, UNCHAR*, UNCHAR*, UNCHAR* );
extern LOGICAL  glImageCreateRaster           ( Image, enum IMAGE_RASTER_TYPE, UNCHAR**, int*, int*, int*, int* );
extern void     glImageDestroy                ( Image );
extern void     glImageDestroyList            ( ImageList, int );
extern void     glImageDestroyAll             ( void );
extern void     glImageDisplay                ( Image );
extern Image    glImageFetch                  ( STRING, STRING, MAP_PROJ* );
extern Image    glImageFetchFile              ( STRING );
extern void     glImageGeometry               ( Image, Coord, Coord, Coord, Coord );
extern int      glImageGetDataItems           ( Image, char*** );
extern LOGICAL  glImageGetGeometry            ( Image, int*, int*, int*, int* );
extern glLUT   *glImageGetLUTInfo             ( ImageLUT );
extern int      glImageInfoFindValidTimes     ( STRING, const char, STRING**, STRING**);
extern ImageLUT glImageInfoGetDefaultLut      ( STRING );
extern int      glImageInfoGetLuts            ( STRING, ImageLUT**, STRING**, ImageLUT* );
extern void     glImageInfoSetLuts            ( STRING, ImageLUT );
extern int      glImageInfoGetProductImages   ( STRING, STRING**, STRING** );
extern int      glImageInfoGetProducts        ( enum IMAGE_TYPE, STRING**, STRING** );
extern int      glImageInfoGetSites           ( enum IMAGE_TYPE, STRING** );
extern STRING   glImageInfoGetTag             ( STRING, STRING );
extern enum IMAGE_TYPE glImageInfoGetType     ( STRING );
extern STRING   glImageInfoGetProductTag      ( STRING );
extern int      glImageInfoGetDirectories     ( STRING** );
extern LOGICAL  glImageInfoIsDataType         ( STRING );
extern LOGICAL  glImageIsDataLUT              ( ImageLUT );
extern LOGICAL  glImageLUTisValid             ( ImageLUT );
extern float    glImageSampleLatLonPoint      ( Image, STRING, LatLonPoint );
extern float   *glImageSampleLatLonArray      ( Image, STRING, LatLonPoint*, int);
extern float    glImageSamplePoint            ( Image, STRING, Coord, Coord );
extern float   *glImageSampleArray            ( Image, STRING, Coord*, Coord*, int );
extern LOGICAL  glImageIsGreyscale            ( Image );
extern LOGICAL  glImageIsDataImage            ( Image );
extern ImageLUT glImageReadLUT                ( STRING );
extern void     glImageRotation               ( Image, Angle, Coord, Coord );
extern void     glImageTagSetBrightness       ( STRING, float );
extern glCOLOR *glImageTransparentPixel       ( void );
extern LOGICAL  glImageToPNG                  ( Image, STRING );
extern STRING   glImageToBase64PNG            ( Image );
extern void     glImageTypeSetBrightness      ( enum IMAGE_TYPE, float );
extern void     glImageSetBlendingState       ( Image, LOGICAL );
extern void     glImageSetLUT                 ( Image, ImageLUT );
extern void     glImageSetTagLUT              ( STRING, ImageLUT );
extern void     glImageShowRadarRangeRings    ( LOGICAL, int );
extern void     glImageSetRadarRangeRingColor ( UNCHAR*, UNCHAR* );
extern void     glImageVdcRotation            ( Image, Angle, float, float );
extern void     glImageVdcGeometry            ( Image, float, float, float, float );


/* Matrix */
extern void glConcatMatrix   ( Matrix );
extern void glCopyMatrix     ( Matrix, Matrix );
extern void glBuildMatrix    ( Matrix, float, float, float, float, float );
extern void glGetMatrix      ( Matrix );
extern void glIdentityMatrix ( Matrix );
extern void glPopMatrix      ( void );
extern void glPushMatrix     ( Matrix );
extern void glRotate         ( Matrix, float );
extern void glScale          ( Matrix, float, float );
extern void glTranslate      ( Matrix, float, float );


#endif /* _FPAGLIB_H */
