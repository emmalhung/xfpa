#ifndef RADAR_STAT_H
#define RADAR_STAT_H

#include <Xm/SciPlot.h>
#include <libxml/tree.h>

#define RADAR_STAT		"radar_stat"/* setup file key */
#define OFF_PROP		"off"		/* node property for storing off state prior value */
#define RECOVER_PROP	"recover"	/* node property for storing unedited file data value */
#define RATIO_PROP		"ratio"		/* node property for ratio between original and set value */
#define VIEW_LABEL_PROP	"label"		/* label properly of view list */
#define ERROR_COLOR		"IndianRed"	/* Colour of errors shown to user */
#define NO_THRESHOLD	1000000000
#define SCIT_ENVIRON	statcfg.nenvironment	/* where the SCIT file thresholds are stored */
/*
 * Keys found in SCIT files
 */
#define ROOT_ID			"SCIT"		/* identifier of root xml node */
#define STORM_NUMBER	"sNumber"
#define ELEMENTS		"ELEMENTS"
#define THRESHOLDS		"THRESHOLDS"
#define THRESHOLD		"THRESHOLD"
#define VALIDTIME		"VALIDTIME"
#define STORM_KEY		"STORM"
#define DATA_NA			"N/A"
/*
 * Special non urp keys. Note that these must agree with the ones
 * found in the rankweightDaemon header file rankweight.h.
 */
#define TYPE_PROP		"type"		/* root node property */	
#define FCST_KEY		"forecast"	/* property key to indicate forecast file */
#define OVERRIDE		"override"	/* property to indicate manual override */
#define STORM_ENV_ID	"sEnviron"	/* For the environment pseudo-element */
/*
 * Keys used in files passed to and received from an external program that
 * calulates forecast trend values for elements and rank weight.
 */
#define ELEMENT_FCST	"ElementForecast"
#define RANKWEIGHT_FCST	"RankWeightForecast"
#define NFORECASTS		"NFORECASTS"
#define TIMEINTERVAL	"TIMEINTERVAL"
#define NTIMES			"NTIMES"
#define TIMEDELTA		"TIMEDELTA"
/*
 * The maximum number of element threshold values.
 */
#define MAXTHRESH	4
/*
 * The size of the array needed to hold the rank weight calculation constants.
 */
#define NRANK_CALC_CONSTANTS	6
/*
 * Index into threshold array that holds the severe threshold value
 */
#define SEVERE_THRESHOLD_NDX	2
/*
 * for rs_user_mesage function
 */
enum { UM_STATUS, UM_ERROR };
/*
 * For data_type variable in the config structure
 */
typedef enum {
	DT_NONE,	/* No data type - ignore this element */
	DT_STRING,	/* data is string */
	DT_NUMERIC	/* data is number */
} data_t;
/*
 * For the element type variable in the element structure
 */
typedef enum {
	ET_VIEW,		/* Element is viewable only */
	ET_RANK_CALC,	/* Element is involved in rank weight calculation and can be edited */
	ET_RANK_ELEM,	/* This is rank weight and is calculated */
	ET_RANK_FCST,	/* Forecast rank weight. Can be edited. */
	ET_ENVIRON,		/* Storm environment */
	ET_STORM_NUMBER	/* Storm number */
} elem_t;


/* Handy define */
#define UCHAR	unsigned char

/* These defines replaces a very common check for key word node elements. */
#define KEYNODE(knp,knn) \
		(knp->type==XML_ELEMENT_NODE && xmlStrcasecmp(knp->name,(const xmlChar*)knn)==0)
#define KEYSTARTNODE(knp,knn) \
		(knp->type==XML_ELEMENT_NODE && xmlStrncasecmp(knp->name,(const xmlChar*)knn,\
													   xmlStrlen((const xmlChar*)knn))==0)
#define NODENAME(knp,knn) \
		(xmlStrcasecmp(knp->name,(const xmlChar*)knn)==0)


/*
 * Structure for element value information
 */
typedef struct {
	float   value;	/* numerical value of element */
	Boolean off;	/* is element turned off */
	Boolean na;		/* is element not available */
	Boolean usermod;/* was element modified directly by the forecaster */
} DELEM;
/*
 * Structure holding data for each storm instance
 */
typedef struct {
	time_t vtime;	/* time of the storm instance in minutes */
	DELEM  *elem;	/* list of element value information in same order as element array */
} SDATA;
/* 
 * Information for each storm in the SCIT file. If no changes are made then the data
 * array values will be the same as the stat array values.
 */
typedef struct {
	int        num;		/* storm number */
	time_t     vtime;	/* most recent storm instance valid time. Same as hist[0].vtime */
	DELEM      *data;	/* most recent storm instance data array. Same as hist[0].elem */
	DELEM      *stat;	/* array of currently valid STAT file values */
	DELEM      *unmod;	/* array of original unmodified STAT file values */
	xmlNodePtr *nodes;	/* xml data node associated with each data array value */
	int        nhist;	/* number of historical storm instances found */
	SDATA      *hist;	/* data of each historical storm instance */
	SDATA      *fcst;	/* statcfg.num_forecasts forecast instances */
} STORM;
/* 
 * Structure to hold threshold info
 */
typedef struct {
	Boolean cfgdef[MAXTHRESH];	/* threshold is pre-defined in configuration file */
	Boolean isabs[MAXTHRESH];	/* is threshold comparison an absolute value? */
	float   value[MAXTHRESH];	/* threshold value */
} THRESH;
/*
 * Structure to store element configuration information
 */
typedef struct {
	int     ndx;				/* position of this element in the full element array */
	String  id;					/* element id as found in stat table */
	String  header;				/* label to display to the user */
	String  trend_title;		/* trend graph title */
	Pixel   trend_line_color;	/* line colour of element on the trend graph */
	int     trend_line_style;	/* line style of element on the trend graph */
	int     fcst_trend_style;	/* line style of forecast portion of the trend line */
	elem_t  type;
	data_t  data_type;			/* from defines above */
	int     ndecimals;			/* number of decimals to output to xml file */
	UCHAR   alignment;			/* alignment of data within the column */
	Boolean sort_ascending;		/* True if the element is sorted in ascending order */
	Boolean has_data;			/* was some data found for this element */
	Boolean flag_severe;		/* show trend indicator? */
	Boolean has_threshold;		/* Are thresholds defined for this element */
	THRESH  *thresholds;		/* threshold values for all of the environments */
	/*
	 * Structure holding information for the curve fitting equations. For
	 * each element the equation is (a*(m*x)⁴+b*(m*x)³+c*(m*x)²+d*(m*x))*f.
	 * Any of a,b,c and d can be 0. (x⁴ is not in most equations).
	 */
	struct _rcc {
		double a,b,c,d,m,f;
	} rank_calc_const;
} ELEMCFG;
/*
 * Structure to hold storm environments info.
 */
typedef struct {
	String key;		/* the end part of the thresholds.xxx keyword in the elements config block */
	String label;	/* what to show the forecaster in a selection list */
} SENV;
/*
 * Structure that holds pointers to the list of data views
 * as defined in the configuration file.
 */
typedef struct {
	String  label;
	int     nelemp;
	ELEMCFG **elemp;
} VIEWS;


/* Structure containing those things read from the config file. Note that elemp is
 * an array of pointers into the element structure. This was done to allow the order
 * of the elements in the matrix to be different from the element structure and allow
 * this order to be modified dynamically
 */
typedef struct {
	String  file;					/* the config file name */
	int     time_interval;			/* time interval between SCIT files in minutes */
	int     num_forecasts;			/* number of forecasts to make */
	int     num_fcst_data_points;	/* number of data points required to do least square fit */
	int     max_file_time_diff;		/* maximum age of file used to determine new storm in minutes */
	float   min_trend_time;			/* minimum time displayed on trend grapy x-axis */
	float   time_compress_val;		/* x-axis time range over which the number of numbers is reduced */
	String  data_off_string;		/* what goes in the cell for the off state */
	String  file_mask;				/* file mask for file matching in directory search */
	String  file_time_mask;			/* file mask with strtime style time format */
	String  internal_time_format;	/* time format as used internally in the file */
	String  user_time_format;		/* time format as seen by user */
	UCHAR   label_alignment;
	Pixel   threshold_fg[MAXTHRESH+1];/* threshold colours (threshold_fg[MAXTHRESH] is for new storm) */
	Pixel   threshold_bg[MAXTHRESH+1];
	SENV    *environment;			/* Storm environment list */
	int     nenvironment;
	ELEMCFG *element;				/* per element config info */
	int     nelement;
	struct {
		String element_id;			/* element identifier for rank weight */
		int    trend_graph_height;	/* height of the trend graph */
		float  min_max;				/* min value of the max y-axis value allowed */
		Pixel  edit_color;			/* colour of editable forecast trend */
		ELEMCFG **elem;				/* array of elements used to calculate rank weight */
		int     nelem;				/* number of elements */
	} rankweight;
	struct {
		String program;				/* name of external program for forecasting trends */
		unsigned long timeout;		/* time to wait for program in milliseconds */
		String infile;				/* input file to program */
		String outfile;				/* output file from program */
	} trend;
} STATCFG;
/*
 * General public functions
 */
extern void    ACTIVATE_radarStatDialog      (Widget);
extern void    ACTIVATE_stormTrendDialog     (STORM*, Boolean);
extern void    InitRadarStatSystem           (void);
extern Boolean RadarStatSystemInitialized    (void);
extern void    RadarStatSetTime              (String);
/*
 * Used between radarSTAT modules
 */
extern void    rs_modify_forecast_rankweights   (int, int*, float*);
extern THRESH *rs_active_thresholds (ELEMCFG*, STORM*, int);
extern Boolean rs_validate_config    (void);
extern Boolean rs_read_config        (Widget);
extern int     rs_get_element_array_pos_from_id (String);
extern int     rs_get_rankweight_element_array_pos_from_time (int);
extern void    rs_calc_storm_T0_rankweight (STORM*);
extern void    rs_calc_storm_rankweights (STORM*);
extern void    rs_make_all_storms_element_trend_forecasts (void);
extern void    rs_make_storm_element_trend_forecast (STORM*, ELEMCFG*, int);
extern void    rs_make_rankweight_forecast (STORM*);
extern float   rs_float_parse (String, Boolean*);
extern int     rs_int_parse (String, Boolean*);
extern void    rs_user_message (int, String, ...);
extern STORM   *rs_get_storm_ptr_from_node (xmlNodePtr);
extern STORM   *rs_get_storm_ptr_from_storm_number (int);

#endif /* RADAR_STAT_H */
