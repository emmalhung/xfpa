#ifndef RANKWEIGHT_H
#define RANKWEIGHT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libxml/tree.h>
#include <fpa.h>

/*
 * Setup file key for config file and STAT data directory
 */
#define RADAR_STAT_KEY	"radar_stat"
/*
 * SCIT file keys
 */
#define ROOT_ID			"SCIT"		/* identifier of root xml node */
#define STORM_ID		"sNumber"	/* element name for storm identifier */
#define DATA_NA			"N/A"		/* value to assign to element when turned off */
#define VALID_TIME		"VALIDTIME"
#define STORM			"STORM"
#define THRESHOLDS		"THRESHOLDS"
#define THRESHOLD		"THRESHOLD"
#define LEVEL_PROP		"level"
#define NUM_FCSTS		"numForecasts"
#define TIME_INTERVAL	"timeInterval"
#define NDATA_POINTS	"numDataPoints"
#define RANK_WEIGHT_ID	"rankWeightElementId"
#define RANK_ID         "rankElementId"
#define STORM_ENV_ID    "sEnviron"
/*
 * Special non urp standard node properties. Note that these must
 * agree with the ones found in the xfpa header file radarSTAT.h.
 */
#define RECOVER_PROP	"recover"	/* element recovery property */
#define OFF_PROP		"off"		/* element turned off property */
#define TYPE_PROP		"type"		/* root node property */	
#define FCST_KEY		"forecast"	/* property key to indicate forecast file */
#define OVERRIDE_PROP	"override"	/* property to indicate manual element override */
#define RATIO_PROP		"ratio"		/* property giving ration between original
                                       data value and modified value */
/*
 * radarSTAT config file keys
 */
#define FILEINFO			"FILEINFO"
#define NAME_MASK			"nameMask"
#define TIME_MASK			"timeMask"
#define FILE_TIME_FMT		"fileTime"
#define EXTERNAL_PROGRAM	"trendProgram"
#define PROGRAM_TIMEOUT		"trendProgramTimeout"

/* Structure holding information for the curve fitting equations. For
 * each element the equation is (a*(m*x)⁴+b*(m*x)³+c*(m*x)²+d*(m*x))*f.
 * Any of a,b,c and d can be 0. (x⁴ is not in most equations). There is
 * also a scaling factor that is to be applied to the element value
 * when calculating. This will only be other than 1 if the forecaster has
 * changed the value. Everything after this is scaled by the change amount.
 */
typedef struct {
	STRING id;			/* element identifier */
	double a,b,c,d,m,f;	/* constants as above */
	int    ndecimals;	/* precision of the value */
} RW_ELEM;

/* Structure to hold info required for rank weight forecasts
 */
typedef struct {
	time_t minutes;
	int    ndecimals;
} RW_FCST;

extern void printlog              (STRING, ...);
extern void printdebug            (STRING, ...);
extern void time_sort_files       (STRING*, int);
extern void calculate_rankweight  (STRING*, int, LOGICAL);
extern void calculate_rankweights (STRING*, int, LOGICAL, STRING**, int*);
extern void process_manual_rankweight_override(STRING*, int);
extern void modify_elements_from_previous_file (STRING);

#endif /*RANKWEIGHT_H*/
