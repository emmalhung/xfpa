/*********************************************************************************/
/*
*    glib_private.h
*
*    The private header file for the glib library.
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

#ifndef _FPAGLIBP_H
#define _FPAGLIBP_H

#include <fpa_getmem.h>
#include <objects/objects.h>
#include <tools/tstamp.h>
#include "glib.h"


/* This replace is required as many allocations in the library depend on
 * being initialized to 0 for the code to work properly. Besides which any
 * macro called INIT... should initialize ;-)
 */
#undef INITMEM
#define INITMEM(TYPE,NUMBER) (TYPE *)calloc(memaddm(SIZE(NUMBER,TYPE)),1)

/* MEM does not initialize memory and ONEMEM just makes one of TYPE.
 */
#define MEM(TYPE,NUMBER) (TYPE *)malloc(memaddm(SIZE(NUMBER,TYPE)))
#define ONEMEM(TYPE)     (TYPE *)calloc(1,memaddm((size_t)sizeof(TYPE)))

/* The following beginning a line define a comment line in the config files
 */
#define CONFIG_COMMENT_LINE_START	"#!*"

/* For error and warning messages
 */
#define ActiveModule	Xgl.active_module


/* Math defines
 */
#ifndef MIN
#	define MIN(x,y)	((x)<(y)?(x):(y))
#endif
#ifndef MAX
#	define MAX(x,y)	((x)>(y)?(x):(y))
#endif
#ifndef ABS
#	define ABS(x) ((x)< 0?(-(x)):(x))
#endif

/* These will make it easier to redefine these functions if necessary
 * without going through all of the code.
 */
#define COSDEG(x)	_xgl_cos((x)*RAD)
#define COS(x)		_xgl_cos((x))
#define ACOS(x)		_xgl_acos((x))
#define SINDEG(x)	_xgl_sin((x)*RAD)
#define SIN(x)		_xgl_sin((x))
#define ASIN(x)		_xgl_asin((x))
#define TANDEG(x)	_xgl_tan((x)*RAD)
#define TAN(x)		_xgl_tan((x))
#define ATAN(x)		_xgl_atan((x))
#define SQRT(x)		_xgl_sqrt((x))


#ifndef NotNull
#	define NotNull(x) ((void*)x!=(void*)0)
#endif
#ifndef IsNull
#	define IsNull ((void*)x ==(void*)0)
#endif


#define BYTESIZE  			sizeof(UNCHAR)
#define BINARY_READ			"rb"
#define FORCE_GREYSCALE(x)	((x)!=NULL&&((x)->options&ForceGreyscale)!=0)


typedef unsigned int UNINT;


/* Set the number of windows to allocate upon initialization. Not all will
 * necessarly be used but this miniminizes reallocation.
 */
#define INITIAL_WINDOW_COUNT	3

/* Where the library will put its scratch file directory unless told otherwise.
 */
#define DEFAULT_WORKING_DIRECTORY	"/tmp"

/* Alias for readability.
*/
#define Xgl _glib_control_

#define W  	Xgl.active	 /* active window */

#define XR(x) 	 _xgl_XRscale(x)			/* rescaled x, rounded to next int */
#define YR(y) 	 _xgl_YRscale(y)			/* rescaled y, dito */
#define XS(x,y)  _xgl_Xscale(x,y)			/* rescaled and shifted x */
#define YS(x,y)  _xgl_Yscale(x,y)			/* rescaled and shifted and mirrored y */
#define XSC(x,y) _xgl_Xscale((W->xp=(x)),y)	/* return XS(x) and set cursor position */
#define YSC(x,y) _xgl_Yscale(x,(W->yp=(y)))	/* return YS(y) and set cursor position */

/* In the raster pixels are defined in groups of 3 conceutive bytes.
 * This macro defines this fact (Raster Bytes per Pixel).
 */
#define RASTER_BPP	3

/* Define transparent as the rgb colour 100. 
 */
#define T_RED	1
#define T_GREEN	0
#define T_BLUE	0
#define T_NAME  "rgb:0001/0000/0000"	/* colour as hex value in ascii */

/*  Check for an opaque pixel (rgb triple)
 */
#define OPAQUE_PIXEL(p)	(p[0]!=T_RED || p[1]!=T_GREEN || p[2]!=T_BLUE)

/* Operations on a glCOLOR structure
 */
#define OPAQUE_COLOR(c)	(c.red!=T_RED || c.green!=T_GREEN || c.blue!= T_BLUE)
#define SAME_COLOR(a,b)	(a.red==b.red && a.green==b.green && a.blue==b.blue)
/* careful here - the curly brackets {} around the define are important */
#define SET_TO_TRANSPARENT_COLOR(a)	{a.red=T_RED;a.green=T_GREEN;a.blue=T_BLUE;}

/* Mask operations. These assume that the mask is addressed starting at the
 * beginning of the mask buffer and that p is the number of the pixel in the
 * image raster that the mask is for. p>>3 is equivalent to p/8 and (~p)&0x7 is
 * equivalent to 7-p%8 and are used as these operations should be faster.
 *
 * 2006.11.15 - The macros were put into the fpa_macros.h header and thus may
 *              be defined before we get here, thus the test.
 */
#ifndef SET_MASK_BIT
#define SET_MASK_BIT(m,p)	(m[(p)>>3]|=(1<<((p)&0x7)))
#define UNSET_MASK_BIT(m,p)	(m[(p)>>3]&=(~(1<<((p)&0x7))))
#define MASK_BIT_SET(m,p)	(m[(p)>>3]&(1<<((p)&0x7)))
#define MASK_SIZE(w,h)		((((w)*(h))-1)/8+1)
#endif

/* For use with XColor structure
*/
#define DoRGB 	   (DoRed|DoGreen|DoBlue)

/* Define the standard rgb to greyscale conversion factors. Note that these must
 * add up to 32768
*/
#define CCIR_RED    6969
#define CCIR_GREEN  23434
#define CCIR_BLUE   2365

#define CONVERT_TO_GRAY_PIXEL(r,g,b) (UNCHAR)((CCIR_RED*(int)(r)+CCIR_GREEN*(int)(g)+CCIR_BLUE*(int)(b))/32768)
#define CONVERT_TO_GRAY_COLOR(c) (c.red=c.green=c.blue=(UNCHAR)((CCIR_RED*(int)(c.red)+CCIR_GREEN*(int)(c.green)+CCIR_BLUE*(int)(c.blue))/32768))

/* Required to define the machine and image byte order
 */
#define IMAGE_LITTLE_ENDIAN 12
#define IMAGE_BIG_ENDIAN    21
#define MACHINE_ENDIAN		Xgl.endian

/* Convienience defines
 */
#define valid_image(x)	(x>0 && x<Xgl.nimages && Xgl.images[x]!=(ImagePtr)0)
#define WHITESPACE		" \t\n\r\f"		/* characters to be taken as white space */
#define IS_RADAR(x)		((x)!=NULL&&(x)->prod!=NULL&&(x)->prod->image_type==ImageTypeRadar)
#define IS_RADAR(x)		((x)!=NULL&&(x)->prod!=NULL&&(x)->prod->image_type==ImageTypeRadar)
/* Float "very near zero" value */
#ifndef FLT_EPSILON
#define FLT_EPSILON  1e-6
#endif


/* This is the image type setting.
*/
#define FileImage		1
#define BlendedImage	2
#define CombinedImage	3
#define CompositeImage	4

/* Band type
 */
#define SingleBand	1
#define TripleBand	3

/* Image group
 */
#define GenericGroup    0	/* Images which are just pictures */
#define DataGroup		1	/* Images, such as satellite data arrays, which have encoded values */
#define RadarGroup		2	/* URP style radar images */
#define SyntheticGroup	3	/* Parent images with children but no actual associated file */

/* Image operational status 
*/
#define ImageNotVisible	0
#define ImageGenerate	1
#define ImageExists		2
#define ImageOnDisk     3

/* Raster data type
 */
#define ByteType	0
#define FloatType	1

/* For use with options in the IMDEF structure.
 */
#define ForceGreyscale		(1)		/* Force the image to greyscale */
#define AdjustAspect		(1<<1)	/* Allow aspect ratio to change */
#define MultipleImagePIF	(1<<2)	/* One file contains multiple image map definitions */

/*  Original image retrieval type. This determines the function that is
 *  used to get the original image.
 */
#define ORT_GIF	 1	/* Graphics Interchange Format */
#define ORT_PNG	 2	/* Portable Network Graphics */
#define ORT_TIF  3	/* TIFF files */
#define ORT_GTF  4	/* GeoTIFF file - NOT USED AT THE MOMENT */
#define ORT_XWD	 5	/* X Windows Dump */
#define ORT_XGL	 6	/* Xgl Library format */
#define ORT_RSB	 7	/* Raw (uncompressed) Single Band */
#define ORT_RGB	 8	/* Raw (uncompressed) RGB Triple Band */
#define ORT_DAT	 9	/* Multi-byte data images */
#define ORT_DPG 10	/* Multi-byte data images from a PNG file */
#define ORT_GUR	11	/* gridded urp data file */
#define ORT_PUR	12	/* polar (r-theta) urp data file */
#define ORT_RAD	13	/* Radar */
#define ORT_FMF 14	/* RGB Image or data raster Metafile */
#define ORT_RPG	15	/* Reprojected Image File */


/* Structure to hold image interval time information
 */
typedef struct {
	int     offset;	/* start time offset from 00Z in minutes */
	int     cycle;	/* repitition cycle in minutes */
	char    btw[8];	/* before time acceptance window */
	char    atw[8];	/* after time acceptance window */
} IMTIMES;
/*
 * Radar data file information.
 *
 * The table_id is the name in the radar data file associated with the data
 * and the item_id are the labels of the data columns in the file. For example
 * we could have "PrecipitationRate-Reflectivity" as the tabel_label and "DBZ",
 * "MM/HR","DBZ_SNOW","CM/HR" as the item labels for the columns of item_values.
 * There are 256 item values coresponding to the maximum number of pixels possible
 * in the image.
 *
 * There are data tables defined in lut.c that relate a range of values for a 
 * specific item to a colour (rgb) (called radar data look-up tables). As we do not
 * know what value is associated with a pixel until we read up the radar image, a
 * lut for any image cannot be assigned until the image is read.
 *
 * lut_xref cross references each of the radar data look-up tables to the information
 * contained in this specific data structure. Each of these look-up table/data table
 * conbinations will result in a unique rgb colour table entry and this is what is
 * stored in lut_xref. All radar luts are < 0 as they refer to this cross reference
 * list and not to an actual lut. Thus a radar lut of -2 could map to an rgb colour
 * table of 12 for the data as stored in this structure.
 */
#define	Indexed		0		/* values are binary bytes and index into a data array */
#define	AsciiFloat	1		/* values are float stored as comma separated ascii */

typedef struct {
	int      encode;		/* how the data is stored, see defines */
	int      range;			/* max range of range-theta data (pixels) */
	int      theta;			/* number of angles in range-theta data */
	float    rscale;		/* range scale (km/pixel) */
	float    tscale;		/* theta scale (degree/theta) */
	STRING   table_id;		/* id of the data table (called label internally in URP) */
	int      nitems;		/* number of columns in the table */
	STRING   *item_id;		/* labels associated with the table columns */
	float    *item_values;	/* values in the table */
	int      nlut_xref;		/* number of colour look up table cross references */
	ImageLUT *lut_xref;		/* look up table cross refs */
} IMRADDATA;
/*
 * Gridded Data image information structure. The scaling factor is that required
 * to scale the pixel value to the actual value. Thus a scaling factor of 0.1
 * would scale a pixel value of 89 to 8.9. The offset translates the value. An
 * offset of -10 would change the 8.9 to -1.1. Thus we have:
 *
 *    value = pixel * scale + offset
 */
typedef struct {
	STRING    element;		/* name of the element coded into the grid */
	int       bpp;	    	/* bytes per pixel */
	int       packed;		/* packed value so apply the scale and offset factors? */
	float     scale;		/* pixel scaling factor */
	float     offset;   	/* pixel value offset from 0 */
	int       byte_order;	/* byte order of data in file */
} IMGRIDDAT;
/*
 * General image information structure.
 */
typedef struct {
	enum IMAGE_ENCODING encoding; /* ImageEncodingAny ... etc */
	int       width;              /* width of raster image  (faked for range-theta data)*/
	int       height;             /* height of raster image (faked for range-theta data)*/
	int       ntransparent;       /* number of transparent colours defined */
	glCOLOR   *transparent;       /* rgb values of cmap to set transparent (image files only) */
	LOGICAL   *closest_rgb;       /* use the rgb value closest to the given transparent rgb */
	ImageLUT  dlut;               /* default image color lookup table */
	ImageLUT  lut;                /* color lookup table to assign */
	LOGICAL   print_cmaps;        /* Normally for initial setup - print out colormaps */
} IMGENINFO;
/*
 * For radar option bits
 */
#define RadarCrop					(1)		/* set if just the radar scan part of image wanted */
#define RadarStripOverlayBits		(1<<1)	/* set if the overlay bits are to be stripped */
#define RadarBackgroundTransparent	(1<<2)	/* set if no_data_rgb to be rendered as transparent */
#define RadarRangeRings				(1<<3)	/* set if radar range ring overlay is allowed */
/*
 * Base image information for radar.
 */
typedef struct {
	int     orgx;			/* x location of radar center in pixels       (image files only) */
	int     orgy;			/* y location of radar center in pixels       (image files only) */
	int     diam;			/* diameter of radar scan part in pixels      (image files only) */
	int     options;			/* see define for radar option bits           (image files only) */
	UNCHAR  mask;			/* overlay masking bits                       (image files only) */
	glCOLOR no_data_rgb;	/* rgb value of background to set transparent (image files only) */
} IMRADINFO;
/*
 * Structure to hold product definition obtained from the Image config file
 */
typedef struct {
	STRING          tag;			/* product tag */
	STRING	        label;			/* the display label for the product */
	enum IMAGE_TYPE image_type;		/* type of the image */
	ImageLUT        deflut;			/* default lut */
	int		        nlut;			/* number of luts associated with the product */
	ImageLUT	   *lutptr;			/* lut identifier array */
	STRING	       *lutlbl;			/* lut label array associated with identifiers */
	float           bsf;			/* brightness scale factor */
} IMPRODDEF;
/*
 * image valid time list structure.
 */
typedef struct {
	TSTAMP time;
	STRING file;
} IMVTLS;
/*
 * Structure to hold image definition obtained from the Image config file
 */
typedef struct {
	STRING    tag;			/* image tag */
	STRING	  site;			/* site of the image */
	UNINT     options;		/* options specified as bits in the word */
	IMTIMES  *interval;		/* interval time between images */
	float     bsf;			/* brightness scale factor */
	IMPRODDEF *prod;		/* product image belongs to */
	IMGENINFO info;			/* general image information */
	IMRADINFO radar;		/* information if image is for radar */
	IMGRIDDAT grid;			/* info if image is encoded grid values */
	MAP_PROJ  mproj;		/* full map projection & definition */
	STRING	  imtab;		/* image manager style table */
	STRING	  imgrep[10];
	int		  impos;
	STRING	  dir;			/* directory where the images are stored */
	STRING    fmask;		/* mask used by ls function to get list of files */
	STRING	  ffmt;			/* file format for time extraction */
	int		  fparm[5];		/* time parameters used in ffmt */
	STRING    piffmt;		/* projection information file construction format */
	int       pifparm[5];	/* time parameters used in piffmt */
	int		  nvt;			/* number of valid times */
	IMVTLS   *vlist;		/* the valid time and file list */
	int       net;			/* number of expected times */
	TSTAMP   *elist;		/* times that image is expected to exist at */
} IMDEF;
/*
 * Information structure for synthetic images that are a merge of other images
 */
typedef struct {
	Image                ndx;          /* Index into the Xgl.images array */
	enum IMAGE_TYPE      image_type;   /* of the synthetic image */
	int                  blend_ratio;  /* ratio for blending operation */
	int                  maxsrc;	   /* current allocated size of src buffer */
	int                  nsrc; 	       /* number of source images */
	struct _image_struct **src; 	   /* source images */
	LOGICAL              src_used[6];  /* used by the composite image function */
} IMSYNDATA;
/*
 * The structure that holds the image data. This structure opaque to external functions.
 */
#define FIDLEN	7
typedef struct _image_struct {
	char     type;           /* FileImage, BlendedImage, etc */
	char     group;          /* DataGroup, RadarGroup, SyntheticGroup */
	char     ort;            /* sets the original image retreval type */
	char     opstat;         /* status of image: ImageGenerate ... (see define) */
	char     rast_fmt;       /* raster data format: ByteType, ... (see define) */
	char     reprowd;        /* index of working directory where reprojected image is stored */
	char     bands;          /* original image bands - SingleBand or TripleBand */
	char     encoding;       /* original image encoding */
	LOGICAL  changed;        /* did the image change during retreval (generated or fetched from disk) */
	char     fid[FIDLEN];    /* unique storage file identification string */
	int      ow, oh;         /* original image width and height */
	STRING   vtime;          /* valid time of image - also used to hold file name if mdef is NULL */
	IMDEF   *imdef;          /* image definition for this image */
	MAP_PROJ mproj;		     /* map projection used to display in map window */
	MAP_PROJ mproj_org;      /* unaltered map projection of the image */
	union {
		IMGRIDDAT *grid;     /* gridded image info */
		IMSYNDATA *synth;    /* synthetic image info */
		IMRADDATA *radar;    /* radar data metafile info */
	} info;                  /* image type specific data */
	ImageLUT lut;            /* pointer to LUT data, glNoLUT if none */
	ImageLUT dlut;           /* lut data if available from the image file (eg GIF) */
	UNCHAR  *lut_remap;      /* array to remap image file lut to lut assigned to dlut */
	Angle    ra;             /* rotation angle to apply to image */
	Coord    rx, ry;         /* point about which to rotate image */
	Coord    mx, my, mw, mh; /* map coord location and size of display image */
	float    sx, sy;         /* scale factors */
	int      tx, ty;         /* translation factors */
	int      ix, iy, iw, ih; /* map coord converted into screen coord */
	int      dx, dy, dw, dh; /* location of drawn origin and its size */
	UNCHAR  *raster;         /* image raster */
} ImageStruct, *ImagePtr;


/*=============== End Image Initialization ==================*/

/* This enumerated type applies to the window text structure height_units item.
*/
enum { PIX_UNITS, VDC_UNITS, MAP_UNITS };

typedef struct {
	Matrix	matrix;
	float   Sx, Sy, ra;
} MATRIXSTACK;

typedef struct XglWindow_ {
	LOGICAL     inuse;              /* is this instance of the window in use */
    int         lw, ls, cs, js;     /* linewidth, linestyle, capstyle and joinstyle */
    int         fs;                 /* current fill style */
	float       ha, hs, hc;         /* hatch pattern angle, spacing and crossing angle */
    float       Sx, Sy;             /* scaling factor for macros XR(), YR()*/
    float       Tx, Ty;             /* scaled translation of coords for macros XS(), YS()*/
	float       RxCos, RxSin;       /* scaled rotation factors: macros XS(), YS() */
	float       RyCos, RySin;       /* scaled rotation factors: macros XS(), YS() */
	float       ra;                 /* rotation angle corresponding to RxCos and RySin */
    float       xp, yp;             /* graphics cursor position */
    UNINT       xm, ym;             /* width, height of window */
	LOGICAL     viewport;           /* TRUE if viewport clipping is activated */
    int         vl, vr, vb, vt;     /* viewport left, right, bottom, top */
	LOGICAL     clipping;           /* TRUE if clipping is activated */
	int         cl, cr, cb, ct;     /* clipping rectangle left, right, bottom, top */
    float       ol, or, ob, ot;     /* glOrtho   left, right, bottom, top */
	short       slen, send; 		/* end pos and length of transform matrix stack */
	MATRIXSTACK *stack;				/* transform matrix info stack */
	struct _xgl_xstuff_struct *x;   /* x specific window data */
} XglWindow;


/* Root data structure for those things common to all windows. The working directory
 * will be created. The directory structure will be removed upon library exit from the
 * first point where the library needs to create a directory. If the directory already
 * exists then it is not deleted.
 */
typedef struct {
	XglWindow *windows;                  /* list of windows */
	XglWindow *active;                   /* window currently active */
	int        active_ndx;               /* active window number */
	int        last_ndx;                 /* end of window list */
	STRING     active_module;            /* name of currently active module - used for messages */
	STRING     work_directory;           /* where to put scratch files and such */
	STRING     work_directory_root;      /* where in the work path we create our first directory */
	int        maximages;                /* number of image pointers allocated */
	int        nimages;                  /* number of images active */
	struct     _image_struct **images;   /* list of images */
	/*
	 * X specific variables. These are not used unless the non-virtual forms
	 * of initialization and window creation are used.
	 */
	struct _XDisplay *display;             /* default display on library initialization */
	struct _xgl_color_struct *Colors;      /* list of colour names */
	int   *ColorXRef;                      /* colour index to colour name cross ref array */
	int    LastColor;                      /* end of colour list */
	int    ReservedColors;                 /* number of reserved colours */
	int    ColorArrayLen;                  /* length of the allocated colour list */
	int    nsnapshot;                      /* number of stored snapshots */
	int    endian;                         /* One of IMAGE_LITTLE_ENDIAN or IMAGE_BIG_ENDIAN */
	float  short_limit;                    /* maximum value of a short int as a float  */
	struct _xgl_snapshot_struct *snapshot; /* list of snapshots */
	void   (*image_xlib_output)(struct _image_struct*); /* output image to screen */
	void   (*set_viewport)(XglWindow *w, Screencoord);
	void   (*close_xstuff)(XglWindow *w);  /* close x window components */
	void   (*exit_xstuff)(int);            /* called on exit to remove window components */
	void   (*x_exit)(void);                /* exit x library components */
} GLIB_CONTROL_STRUCT;


/* The above structure is initialized in glib_init.c where GLIBINIT is defined.
 */
#ifdef GLIBINIT
	GLIB_CONTROL_STRUCT Xgl = {
		0,0,0,1,NULL,NULL,NULL,0,0,0,0,0,0,0,0,0,0,0,0,0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0
	};
#else
	extern GLIB_CONTROL_STRUCT Xgl;
#endif


/* Functions internal to the library only.
 */
extern int        _xgl_XRscale (float x);
extern int        _xgl_YRscale (float y);
extern int        _xgl_Xscale (float x, float y);
extern int        _xgl_Yscale (float x, float y);
extern void       _xgl_affine_coef (float dst[],float sx,float sy,Angle ra,int tx,int ty);
extern ImageLUT   _xgl_create_radar_data_lut (IMRADDATA*, int );
extern void       _xgl_free_reprojection_data(void);
extern void       _xgl_get_image_cmap(struct _image_struct*, glCOLOR* );
extern LOGICAL    _xgl_get_image_data_lut(struct _image_struct*, glLUTCOLOR**, size_t*);
extern LOGICAL    _xgl_rpg_raster(ImagePtr, UNCHAR**, UNCHAR**);
extern LOGICAL    _xgl_get_source_image(struct _image_struct*, UNCHAR**, UNCHAR**);
extern LOGICAL    _xgl_isGIF (STRING);
extern LOGICAL    _xgl_queryGIF (STRING,int*,int*,int*,glCOLOR*,int*,char**);
extern LOGICAL    _xgl_readGIF (ImagePtr,UNCHAR**);
extern void       _xgl_writeGIF (char*,int, int, UNCHAR*, UNCHAR*,UNCHAR*,UNCHAR*,int, char* );
extern LOGICAL    _xgl_isXWDFile (STRING);
extern LOGICAL    _xgl_queryXWD (STRING,int*,int*,int*,char**);
extern LOGICAL    _xgl_readXWD (ImagePtr,int*,int*,int*,UNCHAR**);
extern LOGICAL    _xgl_remap_image (ImagePtr, MAP_PROJ*);
extern void       _xgl_writeXWD (char*,int, int, UNCHAR*, UNCHAR*,UNCHAR*,UNCHAR*,int, char* );
extern LOGICAL    _xgl_isPNG (STRING);
extern LOGICAL    _xgl_queryPNG (STRING,IMDEF*,int*,int*,int*,glCOLOR*);
extern LOGICAL    _xgl_readPNG (ImagePtr,UNCHAR**,UNCHAR**);
extern LOGICAL    _xgl_isDataPNG (STRING);
extern LOGICAL    _xgl_queryDataPNG (STRING,int*,int*,IMDEF*);
extern LOGICAL    _xgl_readDataPNG (ImagePtr,UNCHAR**);
extern LOGICAL    _xgl_read_image_file (FILE*, UNCHAR**, int, int);
extern LOGICAL    _xgl_isTIFF (STRING);
extern LOGICAL    _xgl_queryTIFF (STRING,int*,int*,int*,glCOLOR*);
extern LOGICAL    _xgl_readTIFF (ImagePtr,UNCHAR**,UNCHAR**);
extern IMDEF     *_xgl_image_info_definition ( STRING );
extern LOGICAL    _xgl_isGridURP(STRING);
extern LOGICAL    _xgl_isPolarURP(STRING);
extern LOGICAL    _xgl_queryGridURP (STRING, glCOLOR*, IMDEF*, IMRADDATA**);
extern LOGICAL    _xgl_queryPolarURP (STRING,glCOLOR*,IMDEF*,IMRADDATA**);
extern LOGICAL    _xgl_get_gridded_urp_raster(ImagePtr, UNCHAR**);
extern LOGICAL    _xgl_get_polar_urp_raster(ImagePtr, UNCHAR**);
extern void       _xgl_draw_ellipse(UNCHAR*, int, int, int, int, int, int, UNCHAR*);
extern IMRADDATA *_xgl_create_radar_data_table(IMRADDATA*);
extern LOGICAL    _xgl_read_image_map_projection_file(STRING, ImagePtr, MAP_PROJ*);
extern FILE      *_xgl_open_image_file(ImagePtr);
extern ImageLUT   _xgl_create_lut(glCOLOR*, UNCHAR**, glLUT**);
extern LOGICAL    _xgl_parse_map_def(STRING, MAP_PROJ*);
extern LOGICAL    _xgl_parse_projection(STRING, MAP_PROJ*);
extern LOGICAL    _xgl_write_image_file (FILE*, UNCHAR*, int, int, int);
extern int        _xgl_make_image(int);
extern Image      _xgl_get_image(STRING, enum IMAGE_ENCODING, IMDEF*);
extern STRING     _xgl_make_image_file_path(STRING, STRING);

extern float _xgl_cos(float);
extern float _xgl_acos(float);
extern float _xgl_sin(float);
extern float _xgl_asin(float);
extern float _xgl_tan(float);
extern float _xgl_atan(float);
extern float _xgl_sqrt(float);

#endif /* _FPAGLIBP_H */
