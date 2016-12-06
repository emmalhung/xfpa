/*=========================================================================*/
/*
*      File: radarSTAT_readConfig.c
*
*   Purpose: Reads the xml configuration file for the radar statistics
*            display and fills in the statcfg structure.
*
*
*------------------------------------------------------------------------------
*
*     Version 8 (c) Copyright 2013 Environment Canada
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
***************************************************************************
*   
*   Example configuration file. Note that this is just an example and will
*   need modification in a real environment!
*
<?xml version="1.0" encoding="UTF-8" ?>
<!-- 
USER: For those things that the user sees in the display interface.

	timeFormat:          How the user sees the valid time of the SCIT table in strftime
	                     format.

	labelAlignment:      The alignment of the column labels. One of "beginning","center",or "end"

	dataOffString:       What to display when the data in a cell is turned 'off'

	minTrendTime:        Minimum amount of time displayed on x-axis of the trend graph
	                     (not including forecasts).

	rankWeightHeight:    Height of the rank weight trend graph in pixels. Default 200.

	rankWeightMinMax:    The smallest maximum value that will be shown on the trend
	                     graph for rank weight. Default 10.

	rankWeightEditColor: The colour to display the forecast rank weight trend line when
	                     it is able to be edited.

	timeDisplayCompress: In the storm rank weight trend graph display dialog, time ranges
	                     above this value will reduce increase the distance between time
						 labels and introduce intermediate unlabeled grid lines. This is
						 to avoid overlapping axis numbers. Default 180.

FILEINFO: Information associated with searching for and displaying statistic files.

	fileNameMask:  How to do a generic search and get a list of SCIT files.
	               A dot '.' means accept any character. Anything else is taken as literal.

	fileTimeMask:  How to construct the file name with the time. The time is specified as
	               per the requirements of the strftime function (man strftime).

	fileTime:      The format of the valid time in the file (as per strftime)

	maxTimeDiff:   The maximum difference, in minutes, between the current file and the
	               previous file used to determine if a storm is new.

	timeInterval:  How many minutes between SCIT files. Default 10.

	forecastProgram: The program to run to generate trend forecasts of any of the
	                 elements. Since a program may require key words in its run
					 string the special keys [infile] and [outfile] are used and
					 are substituted for the the actual file names that the STAT
					 file display program needs. For example if the program is
					 "generate" and it requires the keys -i for input and -o for
					 output the line would be:

					 <forecastProgram>generate -i [infile] -o [outfile]</forecastProgram>

					 The default is to use the internal least square fitting functions.

	numForecasts:  How many forecast times to create. Default 3.

	numDataPoints: How many data points to use to create the forecasts. Default 3.
	
	rankWeightElementId: The element identifier for the rank weight element.
	                     Default "RankWeight".

	rankElementId: If there is a "Rank" element defined the identifier is specified
	               here. If not then enter "None". If specified the rank will
				   be modified if the rank weight is changed. Default is none.

THRESHOLD: Sets the colours used to identify the various thresholds.

	newStorm: The foreground and background colours to use to identify when a
	          new storm has appeared in the table.

	level<n>: The foreground and background colours to use for the specified
	          threshold level as in <level1>yellow</level1>.

ENVIRONMENTS: Defines a set of storm environments which define severe thresholds for
              various storm types. The environment identifier is used as the first xml
			  key under ENVIRONMENTS.  This identifier is appended to a "thresholds" id
		      in a element definition. If the identifier is "supercell" then there should
			  be a <thresholds.supercell> entry for each element. See the description
			  under ELEMENTS.

		 label: This is what the forecaster will see as the storm environment in the
		        viewer.


ELEMENTS: Sets configuration information on a per-element basis. The element identifier
          is used as the first xml key under ELEMENTS. See the configuration below.

    label:       This is what the user will see as column headers. Use "\n" to indicate
	             a new line.  This will create a multi-line label.

	dataAlign:   The alignment of the data within the cell. One of "beginning","center"
	             or "end"

	dataType:    one of "ignore","string",or "number". "ignore" means do not
	             display the element.

	numDecimals: Optional. The number of decimals to show in the STAT table spreadsheet
	             display and write into the STAT file. Default is 1.
	
	sortOrder:   Optional. The sort order of the data element in the storm spreadsheet.
	             One of "ascending" or "descending". Default "descending".

	trendLine:   The colour and line style of the line that is displayed in the element
	             trend graph as a comma separated triple of colour, data line style,
				 forecast line style. The colour must be a valid X colour name and the
				 line style one of: SOLID, DOTTED or WIDEDOT. As an example:
				 "red,SOLID,DOTTED". The elements can be left unspecified and the default
				 will be "white,SOLID,SOLID".

    thresholds:  Optional. This sets the default environment. There are two possible forms:
				 1. The name of another element whose thresholds are to be used. This is
				 normally used for synthetic elements like forecast rank weights.
				 2. Threshold levels (maximum 4) for the element used to override
	             threshold settings in the data file. The entry is a comma separated list
				 of values starting with level 1. The entry <thresholds>5,10,20</thresholds>
				 would set the level1 threshold at 5, level2 at 10 and level3 at 20 with no
				 level4 specified.

	thresholds.<identifier>: Optional. This sets thresholds identified with a specific
	                         environment. The threshold levels (maximum 4) are entered
							 as a comma separated list of values starting with level 1.
                             The entry <thresholds.NOS>5,10,20,30</thresholds.NOS>
				             would set the level1 threshold for the NOS environment at 5,
							 level2 at 10, level3 at 20 and level4 at 30. 

	flagSevere:  Optional. Show if the trend of the element will reach the severe level.
	             Value of 'yes' or 'no'. Does not apply to elements of dataType "string".
				 Default is yes for all others.

	rankWeightCalc: Optional. If this element is used for rank weight calculation this
		         entry must be a series of 3 to 6 comma separated numbers, a,b,c,d,m,f
				 to satisfy the equation (a*(m*x)^4 + b*(m*x)^3 + c*(m*x)^2 + d*(m*x)) * f 
				 If the list is 5 numbers then the equation is assumed to start at the
				 third power and if the list is 4 numbers the equation is assumed to start
				 at the second power and if 3 numbers it is assumed to start at d*(m*x).

	Note that there is a special element not found in the actual SCIT files with the key
	word <rankWeightElementId>xx, where xx is the T+ time of a forecast rank weight value. These
	values are calculated by the rankweightDaemon and inserted into the files.

VIEWS: These are lists of elements that will be shown. Usually used to reduce the number
       of elements being dispayed.
	   
	   Each view has the key <VIEW label="<label>">, where <label> is whatever is shown to
	   the user in the menu pulldown for the view selection, and <element>elem_id</element>
	   identifies each element in the list to be shown where elem_id is the element key word.

	   The default view is normally the entire list of elements in the order defined in this
	   configuration file. This can be overriden by defining a view with label="default".

MONITOR: Information used to monitor directories into which files from URP are put.
         When files with the same valid time arrive in all of the directories a
		 program is run. The property "program" holds the name of the program to 
		 run. It may contain time macros as per the strftime function which will be
		 replaced by the appropriate times. It may also contain the special key word
		 "%log" which will be replaced by the path of the logfile used by the monitor
		 program. It must be opened in append mode, written to and then closed.

	DIR: The directory to monitor with the directory indirectly given by the "keyword"
	     property. The key property must be a directory key word that is set in the
		 setup file directories block and associated with the directory to be monitored.
		   
		FILE: Masking information that defines the file monitored and how the time
		      is encoded into the file name.

			nameMask: How to do a search and get a list of the files of interest.
					  A dot '.' means accept any character. Anything else is taken as literal.

			timeMask: How to construct the file name with the time. The time is specified as
					  per the requirements of the strftime function (man strftime).
-->
<STATCONFIG>
	<USER>
		<timeFormat>%Y/%m/%d  %H:%M</timeFormat>
		<labelAlignment>center</labelAlignment>
		<dataOffString>***</dataOffString>
		<minTrendTime>80</minTrendTime>
		<rankWeightHeight>150</rankWeightHeight>
		<rankWeightMinMax>10</rankWeightMinMax>
		<rankWeightEditColor>magenta</rankWeightEditColor>
		<timeDisplayCompress>180</timeDisplayCompress>
	</USER>
	<FILEINFO>
		<nameMask>............:SCIT,VIEW_CONV:</nameMask>
		<timeMask>%Y%m%d%H%M:SCIT,VIEW_CONV:</timeMask>
		<fileTime>%Y%m%d%H%M</fileTime>
		<maxTimeDiff>30</maxTimeDiff>
		<forecastProgram>storm-trend-forecast [infile] [outfile]</forecastProgram>
		<numForecasts>3</numForecasts>
		<timeInterval>10</timeInterval>
		<numDataPoints>3</numDataPoints>
		<rankWeightElementId>RankWeight</rankWeightElementId>
	</FILEINFO>
	<THRESHOLD>
		<newStorm>
			<foreground>black</foreground>
			<background>white</background>
		</newStorm>
		<level1>
			<foreground>black</foreground>
			<background>green</background>
		</level1>
		<level2>
			<foreground>black</foreground>
			<background>yellow</background>
		</level2>
		<level3>
			<foreground>black</foreground>
			<background>red</background>
		</level3>
		<level4>
			<foreground>black</foreground>
			<background>magenta</background>
		</level4>
	</THRESHOLD>
	<ENVIRONMENTS>
		<default>
			<label>Default</label>
		</default>
		<LOT>
			<label>Low-Top</label>
		</LOT>
	</ENVIRONMENTS>
	<ELEMENTS>
		<REGION>
			<label>Region</label>
			<dataType>ignore</dataType>
		</REGION>
		<sNumber>
			<label>ID</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<flagSevere>no</flagSevere>
		</sNumber>
		<sEnviron>
			<label>Storm\nEnv</label>
			<dataAlign>beginning</dataAlign>
			<dataType>string</dataType>
			<flagSevere>no</flagSevere>
		</sEnviron>
		<RankWeight>
			<label>Rank\nWeight</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<flagSevere>no</flagSevere>
			<trendLine>white</trendLine>
			<thresholds.LOT>2,4,6,8</thresholds.LOT>
		</RankWeight>
		<RankWeight10>
			<label>Rank\nT+10</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<thresholds>RankWeight</thresholds>
			<flagSevere>no</flagSevere>
		</RankWeight10>
		<RankWeight20>
			<label>Rank\nT+20</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<thresholds>RankWeight</thresholds>
			<flagSevere>no</flagSevere>
		</RankWeight20>
		<RankWeight30>
			<label>Rank\nT+30</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<thresholds>RankWeight</thresholds>
			<flagSevere>no</flagSevere>
		</RankWeight30>
		<Meso_alert>
			<label>Meso\nAlert</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.4,1,0.25</rankWeightCalc>
			<trendLine>azure,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>3,5,10,15</thresholds.LOT>
		</Meso_alert>
		<Meso_max>
			<label>Meso\nMax</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.00009866,-0.011605,0.42981,1,0.25</rankWeightCalc>
			<trendLine>LightBlue,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>12,16,20,24</thresholds.LOT>
		</Meso_max>
		<Meso_ave>
			<label>Meso\nAve</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.00009866,-0.011605,0.42981,1,0.25</rankWeightCalc>
			<trendLine>LightBlue,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>4,8,12,16</thresholds.LOT>
		</Meso_ave>
		<Hail_mm>
			<label>Hail\n(mm)</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.0000015831,-0.00075025,0.11056,1,0.25</rankWeightCalc>
			<trendLine>magenta,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>13,20,35,50</thresholds.LOT>
		</Hail_mm>
		<MESH_mm>
			<label>MESH\n(mm)</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.0000015831,-0.00075025,0.11056,1,0.25</rankWeightCalc>
			<trendLine>LightSkyBlue,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>13,20,35,50</thresholds.LOT>
		</MESH_mm>
		<BWERvol_km3>
			<label>BWER\nVol</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>-0.000000021797,0.000010658,-0.0018033,0.130907,1,0.25</rankWeightCalc>
			<trendLine>green,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>0,10,25,50</thresholds.LOT>
		</BWERvol_km3>
		<BWERhgt_km>
			<label>BWER\nHgt</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.0000011311,-0.00056254,0.089627,10,0.25</rankWeightCalc>
			<trendLine>yellow,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>1,2,3,4</thresholds.LOT>
		</BWERhgt_km>
		<WDraft_kmh>
			<label>WDraft\n(km/h)</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>0.000012935,-0.0028382,0.19928,0.28,0.25</rankWeightCalc>
			<trendLine>white,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>36,54,72,90</thresholds.LOT>
		</WDraft_kmh>
		<VILDens>
			<label>VIL\nDens</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<rankWeightCalc>0.0000013262,-0.00063285,0.098483,10,0.25</rankWeightCalc>
			<trendLine>red,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>1,2,3,4</thresholds.LOT>
		</VILDens>
		<MaxZ>
			<label>MaxZ</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<rankWeightCalc>-0.000011869,0.001536,0.011393,1,0.25</rankWeightCalc>
			<trendLine>wheat,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>55,60,65,70</thresholds.LOT>
		</MaxZ>
		<ETop45_km>
			<label>ETop\n45km</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<rankWeightCalc>-0.0000016966,0.00045752,-0.0013906,10,0.25</rankWeightCalc>
			<trendLine>pink,SOLID,WIDEDOT</trendLine>
			<thresholds.LOT>7,8.5,10,12.5</thresholds.LOT>
		</ETop45_km>
		<Distance>
			<label>Dist</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<flagSevere>no</flagSevere>
		</Distance>
		<Dirn>
			<label>Dir</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<flagSevere>no</flagSevere>
		</Dirn>
		<Location>
			<label>Location</label>
			<dataAlign>beginning</dataAlign>
			<dataType>string</dataType>
		</Location>
		<MeanVel_kmh>
			<label>Mean Vel\n(Km/Hr)</label>
			<dataAlign>beginning</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>60,75,90,105</thresholds.LOT>
		</MeanVel_kmh>
		<Radar>
			<label>Radar</label>
			<dataAlign>beginning</dataAlign>
			<dataType>string</dataType>
		</Radar>
		<POSH_pct>
			<label>POSH\n(%)</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>70,80,90,100</thresholds.LOT>
		</POSH_pct>
		<CoreVolume_km3>
			<label>Core\nVol</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>100,300,500,1000</thresholds.LOT>
		</CoreVolume_km3>
		<ColderVolume_km3>
			<label>Colder\nVol</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>10000,30000,50000,100000</thresholds.LOT>
		</ColderVolume_km3>
		<LtgCount>
			<label>Ltg\nCount</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>10,20,30,40</thresholds.LOT>
		</LtgCount>
		<PosCGPct>
			<label>+CG\n(%)</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>25,50,75,100</thresholds.LOT>
		</PosCGPct>
		<MaxPosAmp>
			<label>MaxAmp\nPos CG</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<thresholds.LOT>|50|,|75|,|100|,|125|</thresholds.LOT>
		</MaxPosAmp>
		<MaxNegAmp>
			<label>MaxAmp\nNeg CG</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>0</numDecimals>
			<sortOrder>ascending</sortOrder>
			<thresholds.LOT>|50|,|75|,|100|,|125|</thresholds.LOT>
		</MaxNegAmp>
		<FlashDens>
			<label>Flash\nDens</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>2</numDecimals>
			<thresholds.LOT>0.1,0.25,0.5,0.75</thresholds.LOT>
		</FlashDens>
		<FlashTend>
			<label>Flash\nTend</label>
			<dataAlign>end</dataAlign>
			<dataType>numeric</dataType>
			<numDecimals>1</numDecimals>
			<thresholds.LOT>5,10,20,30</thresholds.LOT>
		</FlashTend>
	</ELEMENTS>
	<VIEWS>
		<VIEW label="Rank Weight Elements">
			<element>sNumber</element>
			<element>sEnviron</element>
			<element>RankWeight</element>
			<element>RankWeight10</element>
			<element>RankWeight20</element>
			<element>RankWeight30</element>
			<element>Meso_alert</element>
			<element>Meso_max</element>
			<element>Meso_ave</element>
			<element>Hail_mm</element>
			<element>MESH_mm</element>
			<element>BWERvol_km3</element>
			<element>BWERhgt_km</element>
			<element>WDraft_kmh</element>
			<element>VilDens</element>
			<element>MaxZ</element>
			<element>ETop_45km</element>
		</VIEW>
	</VIEWS>
</STATCONFIG>
*
*/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "global.h"
#include "radarSTAT.h"

#define pgm	"radarStatConfigReader"
#define CONFIG_VERSION	1	/* The required configuration file version */

/* Public variables */
STATCFG statcfg = {	/* configuration information */
    NULL,                 /* file                              */
    10,                   /* time_interval (minutes)           */
    3,                    /* num_forecasts                     */
    3,                    /* num_fcst_data_points              */
    20,                   /* max_file_time_diff (minutes)      */
    60.0,                 /* min_trend_time (minutes)          */
	180,                  /* time_compress_val                 */
    "OFF",                /* data_off_string                   */
    NULL,                 /* file_mask                         */
    NULL,                 /* file_time_mask                    */
    "%Y%m%d%H%M",         /* internal_time_format (strftime)   */
    "%Y/%m/%d %H:%M",     /* user_time_format (strftime)       */
    XmALIGNMENT_BEGINNING,/* label_alignment (XmLabel widget)  */
    {0,0,0,0,0},          /* threshold_fg                      */
    {0,0,0,0,0},          /* threshold_bg                      */
	NULL, 0,              /* environment, nenvironment         */
	NULL, 0,              /* element, nelement                 */
	{                     /* rankweight structure follows      */
		"RankWeight",     /* element_id                        */
		200,              /* trend_graph_height                */
		10.,              /* min_max                           */
		0,                /* edit_color                        */
		NULL, 0           /* elem, nelem                       */
	},
	{                     /* trend structure follows           */
		NULL,             /* program                           */
		5000,             /* timeout in milliseconds           */
		NULL,		      /* infile                            */
		NULL              /* outfile                           */
	}
}; 

String  stat_data_dir = NULL;	/* stat file data directory */
VIEWS   *elem_view = NULL;		/* The list of element views as obtained from config file */
int     nelem_view = 0;			/* number of views */
ELEMCFG **elemp = NULL;			/* pointer to active elem_view item */
int     nelemp  = 0;			/* number of elemp in active item */
STORM   *storm_list = NULL;		/* storm information array */
int     storm_list_len = 0;		/* number of storms */

/* Private variables */
static String errkey = NULL;	/* for error messages. Combines dialog and config file name */


/************* Start read configuration file section *******************/


/* Determine the alignment macro from the key string. */
static UCHAR get_alignment_from_string(String align, Boolean *ok)
{
	*ok = True;
	if(same_ic(align,"beginning")) return XmALIGNMENT_BEGINNING;
	if(same_ic(align,"center")   ) return XmALIGNMENT_CENTER;
	if(same_ic(align,"centre")   ) return XmALIGNMENT_CENTER;
	if(same_ic(align,"end")      ) return XmALIGNMENT_END;
	*ok = False;
	return XmALIGNMENT_END;
}


/* Determine data type from string. Some of the entries are for
 * backwards compatibility.
 */
static int get_data_type_from_string(String type, Boolean *ok)
{
	*ok = True;
	if(same_ic(type,"string") ) return DT_STRING;
	if(same_ic(type,"int")    ) return DT_NUMERIC;
	if(same_ic(type,"float")  ) return DT_NUMERIC;
	if(same_ic(type,"number") ) return DT_NUMERIC;
	if(same_ic(type,"numeric")) return DT_NUMERIC;
	if(same_ic(type,"abs")    ) return DT_NUMERIC;
	if(same_ic(type,"none")   ) return DT_NONE;
	if(same_ic(type,"ignore") ) return DT_NONE;
	*ok = False;
	return DT_STRING;
}


/* Parse the information for plotting the element trends on the
 * graph. There are 3 parts. In order they are the line colour,
 * the line style for data and the line style for the element
 * forecast.
 */
static void parse_trendline_config(Widget w, String value, int pos)
{
	String part;

	if(!(part = strtok(value,","))) return;

	statcfg.element[pos].trend_line_color = XuLoadColor(w, part);

	if(!(part = strtok(NULL,","))) return;

	if(same_ic(part,"SOLID"))
		statcfg.element[pos].trend_line_style = XmLINE_SOLID;
	else if(same_ic(part,"DOTTED"))
		statcfg.element[pos].trend_line_style = XmLINE_DOTTED;
	else if(same_ic(part,"WIDEDOT"))
		statcfg.element[pos].trend_line_style = XmLINE_WIDEDOT;
	else
		pr_error(errkey,"Unrecognized trend line style = %s\n",part);

	if(!(part = strtok(NULL,","))) return;

	if(same_ic(part,"SOLID"))
		statcfg.element[pos].fcst_trend_style = XmLINE_SOLID;
	else if(same_ic(part,"DOTTED"))
		statcfg.element[pos].fcst_trend_style = XmLINE_DOTTED;
	else if(same_ic(part,"WIDEDOT"))
		statcfg.element[pos].fcst_trend_style = XmLINE_WIDEDOT;
	else
		pr_error(errkey,"Unrecognized fcst trend line style = %s\n",part);
}


/* Parse the comma separated list of constants used in the calculation of
 * the rank weight contribution for the element.
 */
static void parse_rank_weight_calc(String value, int pos)
{
	statcfg.element[pos].type = ET_RANK_CALC;
	memset((void *) &statcfg.element[pos].rank_calc_const, 0, sizeof(struct _rcc));
	if(blank(value))
	{
		pr_error(pgm, "\'rankWeightCalc\' for element \'%s\' is empty",
				statcfg.element[pos].id);
	}
	else
	{
		int i, j, k = 0;
		double a1[6], a2[6];
		String ptr;
		/*
		 * Create a separate array of which elements are used for rank calculation.
		 * This makes processing when displaying graphs easier.
		 */
		statcfg.rankweight.elem = MoreMem(statcfg.rankweight.elem, ELEMCFG*, statcfg.rankweight.nelem+1);
		statcfg.rankweight.elem[statcfg.rankweight.nelem] = &statcfg.element[pos];
		statcfg.rankweight.nelem++;
		/*
		 * Read the constants for the curve fitting equations. They are put in order
		 * with unused constants simply not given. Thus the rearranging of the arrays.
		 * For each element the equation is (a*(m*x)⁴+b*(m*x)³+c*(m*x)²+d*(m*x))*f.
		 * Any of a,b,c and d can be 0. (x⁴ is not in most equations).
		 */
		ptr = strtok(value,",");
		while(k < 6 && ptr != NULL)
		{
			Boolean ok;
			a1[k++] = double_arg(ptr, &ok);
			if(!ok) pr_error(pgm,"Failed to read rankWeightCalc constant number %d for element %s",
				k, statcfg.element[pos].id);
			ptr = strtok(NULL,",");
		}
		memset((void*) a2, 0, 6*sizeof(double));
		for(j = 0, i = (6-k); i < 6; i++, j++)
			a2[i] = a1[j];
		statcfg.element[pos].rank_calc_const.a  = a2[0];
		statcfg.element[pos].rank_calc_const.b  = a2[1];
		statcfg.element[pos].rank_calc_const.c  = a2[2];
		statcfg.element[pos].rank_calc_const.d  = a2[3];
		statcfg.element[pos].rank_calc_const.m  = a2[4];
		statcfg.element[pos].rank_calc_const.f  = a2[5];
	}
}


/* Parse the value string that contains the comma separated list
 * of element severity thresholds.
 */
static void parse_thresholds(THRESH *thresholds, String value)
{
	int i;
	String val = strtok(value,",");

	for(i = 0; i < MAXTHRESH && val != NULL; i++)
	{
		char valbak[32];
		Boolean ok;

		no_white(val);
		strncpy(valbak, val, 32);
		valbak[31] = '\0';
		if((thresholds->isabs[i] = (*val == '|'))) val++;
		thresholds->value[i] = rs_float_parse(val, &ok);
		thresholds->cfgdef[i] = ok;
		if(!ok)
		{
			thresholds->value[i] = NO_THRESHOLD;
			pr_error(errkey, "Threshold value parse error. Threshold = %s\n", valbak);
		}
		val = strtok(NULL,",");
	}
}


/* Create 1 more threshold array element than indicated to hold
 * the SCIT file table default values.
 */
static THRESH *create_thresholds(int num)
{
	int i, n;
	THRESH *threshold;
	
	num++;
	threshold = NewMem(THRESH, num);
	for(n = 0; n < num; n++)
	{
		for(i = 0; i < MAXTHRESH; i++)
		{
			threshold[n].cfgdef[i] = False;
			threshold[n].isabs[i]  = False;
			threshold[n].value[i]  = NO_THRESHOLD;
		}
	}
	return threshold;
}


/* Read the ELEMENTS block of the radarSTAT configuration file.
 */
static void read_element_config(String fname, Widget wid, xmlNodePtr node)
{
	int  i, n, len;
	xmlNodePtr top = NULL, cur = NULL, data = NULL;

	/* Scan the tree and allocate and initialize the element data array.
	 * This is necessary as some element thresholds cross reference the
	 * threshold arrays of other elements. This is why threshold does not
	 * directly point to the environ_threshold element but has the values
	 * copied in because other elements could point to it.
	 */
	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,ELEMENTS)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			n = statcfg.nelement;
			statcfg.nelement++;
			statcfg.element = MoreMem(statcfg.element, ELEMCFG, statcfg.nelement);
			memset((void *) &statcfg.element[n], 0, sizeof(ELEMCFG));
			statcfg.element[n].ndx = n;
			statcfg.element[n].id = XtNewString((String) cur->name);
			statcfg.element[n].sort_ascending = False;
			statcfg.element[n].flag_severe = True;
			statcfg.element[n].data_type = DT_STRING;
			statcfg.element[n].ndecimals = 1;
			statcfg.element[n].alignment = XmALIGNMENT_END;
			statcfg.element[n].trend_line_color = XuLoadColor(wid, "white");
			statcfg.element[n].trend_line_style = XmLINE_SOLID;
			statcfg.element[n].fcst_trend_style = XmLINE_SOLID;
			statcfg.element[n].thresholds = create_thresholds(statcfg.nenvironment);
			/*
			 * Assign element types from the element id
			 */
			if(same_start(statcfg.element[n].id, statcfg.rankweight.element_id))
			{
				/* get the time from the id (in minutes, 0 = data) */
				len = strlen(statcfg.rankweight.element_id);
				if(atoi(statcfg.element[n].id+len) > 0)
					statcfg.element[n].type = ET_RANK_FCST;
				else
					statcfg.element[n].type = ET_RANK_ELEM;
			}
			else if(same(statcfg.element[n].id, STORM_NUMBER))
			{
				statcfg.element[n].type = ET_STORM_NUMBER;
			}
			else if(same(statcfg.element[n].id, STORM_ENV_ID))
			{
				statcfg.element[n].type = ET_ENVIRON;
			}
			else
			{
				statcfg.element[n].type = ET_VIEW;
			}
		}
		break;
	}


	/* Scan the tree again and load in the element configuration information.
	 */
	for(n = 0, top = node; top; top = top->next)
	{
		if(!KEYNODE(top,ELEMENTS)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if (cur->type != XML_ELEMENT_NODE) continue;
			for(data = cur->children; data; data = data->next)
			{
				String value = xmlNodeGetContent(data);
				if(!value) continue;
				no_white(value);

				if(KEYNODE(data,"label"))
				{
					String p;
					statcfg.element[n].header = XtNewString(value);
					p = statcfg.element[n].header;
					while(*p && (p = strstr(p,"\\n")))	/* check for line breaks */
					{
						*p = '\n';
						p++;
						memmove(p, p+1, strlen(p));
					}
					/*
					 * The trend graph title is the same as the matrix column
					 * header but without any line breaks.
					 */
					statcfg.element[n].trend_title = XtNewString(statcfg.element[n].header);
					while((p = strchr(statcfg.element[n].trend_title,'\n')))
						*p = ' ';
				}
				else if(KEYNODE(data,"dataAlign"))
				{
					Boolean ok;
					statcfg.element[n].alignment = get_alignment_from_string(value, &ok);
					if(!ok) pr_error(errkey, "%s - Unrecognized alignment key \'%s\' for element \'%s\'\n",
								fname, value, statcfg.element[n].id);
				}
				else if(KEYNODE(data,"dataType"))
				{
					Boolean ok;
					statcfg.element[n].data_type = get_data_type_from_string(value, &ok);
					if(!ok) pr_error(errkey,"%s - Unrecognized dataType \'%s\' for element \'%s\'\n",
								fname, value, statcfg.element[n].id);
				}
				else if(KEYNODE(data,"numDecimals"))
				{
					Boolean ok;
					i = rs_int_parse(value, &ok);
					if(ok && i >= 0) statcfg.element[n].ndecimals = i;
					else pr_error(errkey,"%s - Parse failure for \'numDecimals\' for element \'%s\'\n",
							fname, statcfg.element[n].id);
				}
				else if(KEYNODE(data,"sortOrder"))
				{
					statcfg.element[n].sort_ascending = same_start_ic(value,"as");
				}
				else if(KEYNODE(data,"flagSevere"))
				{
					statcfg.element[n].flag_severe = (strchr("YyTt",*value) != NULL);
				}
				else if(KEYNODE(data,"trendLine"))
				{
					parse_trendline_config(wid, value, n);
				}
				else if(KEYNODE(data,"rankWeightCalc"))
				{
					parse_rank_weight_calc(value, n);
				}
				else if(KEYNODE(data,"thresholds") || KEYNODE(data,"thresholds.default"))
				{
					/* First see if the entry cross references to another element. If
					* it does free it's thresholds array and point it to the
					* array of the other element.
					*/
					for(i = 0; i < statcfg.nelement; i++)
					{
						int k;
						if(!same(value, statcfg.element[i].id)) continue;
						FreeItem(statcfg.element[n].thresholds);
						statcfg.element[n].thresholds = statcfg.element[i].thresholds;
						statcfg.element[n].has_threshold = True;
						break;
					}
					if(!statcfg.element[n].has_threshold)
					{
						statcfg.element[n].has_threshold = True;
						parse_thresholds(statcfg.element[n].thresholds, value);
					}
				}
				else if(KEYSTARTNODE(data,"thresholds."))	/* Thresholds associated with storm environments */
				{
					for(i = 0; i < statcfg.nenvironment; i++)
					{
						if(xmlStrcmp(data->name+11,statcfg.environment[i].key)) continue;
						parse_thresholds(statcfg.element[n].thresholds+i, value);
						break;
					}
					if(i >= statcfg.nenvironment)
					{
						pr_error(errkey,"%s - Unrecognized environment key in element item \'%s\'\n",
							fname, data);
					}
				}
				else if(data->type == XML_ELEMENT_NODE)
				{
					pr_error(errkey,"%s - Unrecognized key \'%s\' for element \'%s\'\n",
							fname, data->name, statcfg.element[n].id);
				}
				xmlFree(value);
			}
			/*
			 * Consistency checks
			 */
			if(statcfg.element[n].data_type == DT_STRING)
				statcfg.element[n].flag_severe = False;
			n++;
		}
		return;
	}
	pr_error(errkey,"Did not find the \"ELEMENTS\" block in the config file \'%s\'.\n", fname);
}


/* Read the ENVIRONMENTS block of the configuration file. This sets up the
 * list of storm environments recognized by the system.
 */
static void read_environments_config(String fname, xmlNodePtr node)
{
	xmlNodePtr top = NULL, cur = NULL, data = NULL;

	/* Create the default environments. If a default is defined in
	 * the config file it is automatically put in the [0] location
	 * and replaces the default label below.
	 */
	statcfg.nenvironment = 1;
	statcfg.environment = NewMem(SENV, statcfg.nenvironment);
	statcfg.environment[0].key = XtNewString("default");
	statcfg.environment[0].label = XtNewString("Default");

	/* Scan looking for selectable environments */
	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,"ENVIRONMENTS")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			String label = NULL;

			if(cur->type != XML_ELEMENT_NODE) continue;
			for(data = cur->children; data; data = data->next)
			{
				if(data->type != XML_ELEMENT_NODE) continue;
				if(NODENAME(data,"label"))
				{
					String val = xmlNodeGetContent(data);
					label = XtNewString(val);
					xmlFree(val);
				}
			}
			if(label)
			{
				if(xmlStrcmp(cur->name,statcfg.environment[0].key))
				{
					int n = statcfg.nenvironment;
					statcfg.nenvironment++;
					statcfg.environment = MoreMem(statcfg.environment, SENV, statcfg.nenvironment);
					statcfg.environment[n].key = XtNewString((String) cur->name);
					statcfg.environment[n].label = label;
				}
				else
				{
					XtFree(statcfg.environment[0].label);
					statcfg.environment[0].label = label;
				}
			}
			else
			{
				pr_error(errkey,"%s - Environment \'%s\' label missing.\n", fname, cur->name);
			}
		}
		break;
	}
	/*
	 * The SCIT file defaults set in the statcfg.nenvironment array location.
	 */
	statcfg.environment = MoreMem(statcfg.environment, SENV, statcfg.nenvironment+1);
	statcfg.environment[statcfg.nenvironment].label = XtNewString("Default");
	statcfg.environment[statcfg.nenvironment].key = XtNewString("scit");
}


/* Read the THRESHOLD block of the configuration file. Here the foreground and
 * background colours for the colour code associated with the threshold values
 * are defined for each threshold level.
 */
static void read_threshold_config(String fname, Widget w, xmlNodePtr node)
{
	int level;
	Pixel color;
	xmlNodePtr top = NULL, cur = NULL, data = NULL;

	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,THRESHOLD)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			if(     NODENAME(cur,"level1")) level = 0;
			else if(NODENAME(cur,"level2")) level = 1;
			else if(NODENAME(cur,"level3")) level = 2;
			else if(NODENAME(cur,"level4")) level = 3;
			else if(NODENAME(cur,"newStorm")) level = MAXTHRESH;
			else
			{
				pr_error(errkey,"%s - Threshold level \'%s\' unrecognized.\n", fname, cur->name);
				continue;
			}
			for(data = cur->children; data; data = data->next)
			{
				if(data->type != XML_ELEMENT_NODE) continue;
				if(NODENAME(data,"foreground"))
				{
					String color = xmlNodeGetContent(data);
					if(color) statcfg.threshold_fg[level] = XuLoadColor(w, color);
					xmlFree(color);
				}
				else if(NODENAME(data,"background"))
				{
					String color = xmlNodeGetContent(data);
					if(color) statcfg.threshold_bg[level] = XuLoadColor(w, color);
					xmlFree(color);
				}
				else
				{
					pr_error(errkey,"%s - Unknown colour key \'%s\'\n", fname, cur->name);
				}
			}
		}
		return;
	}
	pr_error(errkey,"Did not find the \"THRESHOLD\" block in the config file \'%s\'.\n", fname);
}


/* Read the FILEINFO block of the file. The file and time masks allow
 * searching the directory for a list of STAT files and for getting a
 * file at a specific time.
 */
static void read_file_info_config(String fname, xmlNodePtr node)
{
	xmlNodePtr top = NULL, cur = NULL, data = NULL;
	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,"FILEINFO")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if (cur->type != XML_ELEMENT_NODE) continue;
			String value = xmlNodeGetContent(cur);
			if(value)
			{
				no_white(value);
				if(KEYNODE(cur,"nameMask"))
				{
					statcfg.file_mask = XtMalloc(strlen(value)+2);
					strcpy(statcfg.file_mask, "^");
					strcat(statcfg.file_mask, value);
				}
				else if(KEYNODE(cur,"timeMask"))
				{
					statcfg.file_time_mask = XtNewString(value);
				}
				else if(KEYNODE(cur,"fileTime"))
				{
					statcfg.internal_time_format = XtNewString(value);
				}
				else if(KEYNODE(cur,"maxTimeDiff"))
				{
					int dt = atoi(value);
					if(dt > 0) statcfg.max_file_time_diff = dt;
				}
				else if(KEYNODE(cur,"timeInterval"))
				{
					int dt = atoi(value);
					if(dt > 0) statcfg.time_interval = dt;
				}
				else if(KEYNODE(cur,"trendProgramTimeout"))
				{
					unsigned long timeout = (unsigned long)(atof(value)*1000.0);
					if(timeout > 0) statcfg.trend.timeout = timeout;
					else pr_error(errkey,"trendProgramTimeout = %s - must be > 0\n", value);
				}
				else if(KEYNODE(cur,"trendProgram"))
				{
					/* The keyword "default" is used to indicate the use of the internal
					 * functions. As the user might put in "default [infile] [outfile]" or
					 * some such look for a start of "default".
					 */
					if(same_start_ic(value,"default"))
						statcfg.trend.program = NULL;
					else if(!strstr(value,"[infile]"))
						pr_error(errkey,"trendProgram: [infile] not in run string.\n");
					else if(!strstr(value,"[outfile]"))
						pr_error(errkey,"trendProgram: [outfile] not in run string.\n");
					else
					{
						size_t len;
						String p, pbuf, buf1, buf2;
						pbuf = env_sub(value);
						statcfg.trend.infile  = tempnam(GV_working_directory,"tin-");
						statcfg.trend.outfile = tempnam(GV_working_directory,"tout-");
						len = safe_strlen(pbuf)+safe_strlen(statcfg.trend.infile)+safe_strlen(statcfg.trend.outfile);
						buf1 = NewMem(char, len);
						buf2 = NewMem(char, len);
						p = strstr(pbuf,"[infile]");
						strncpy(buf1, pbuf, p-pbuf);
						strcat(buf1, statcfg.trend.infile);
						strcat(buf1, p + 8);
						p = strstr(buf1, "[outfile]");
						strncpy(buf2, buf1, p-buf1);
						strcat(buf2, statcfg.trend.outfile);
						strcat(buf2, p+9);
						/*
						 * If present LOGFILE is replaced with a dash indicating that
						 * standard out should be used.
						 */
						if((p = strstr(buf2, "[LOGFILE]")))
						{
							strncpy(buf1, buf2, p-buf2);
							buf1[p-buf2] = '\0';
							strcat(buf1, "-");
							strcat(buf1, p+9);
							statcfg.trend.program = XtNewString(buf1);
						}
						else
						{
							statcfg.trend.program = XtNewString(buf2);
						}
						FreeItem(buf1);
						FreeItem(buf2);
					}
				}
				else if(KEYNODE(cur,"numForecasts"))
				{
					int nr = atoi(value);
					if(nr > 0) statcfg.num_forecasts = nr;
				}
				else if(KEYNODE(cur,"numDataPoints"))
				{
					int nr = atoi(value);
					if(nr > 0) statcfg.num_fcst_data_points = nr;
				}
				else if(KEYNODE(cur,"rankWeightElementId"))
				{
					statcfg.rankweight.element_id = XtNewString(value);
				}
				else
				{
					pr_error(errkey,"%s - Unknown FILEINFO key \'%s\'\n", fname, cur->name);
				}
				xmlFree(value);
			}
			else
			{
				pr_error(errkey,"%s - Missing value for \'%s\'\n", fname, cur->name);
			}
		}
		/*
		 * Data checks
		 */
		if(statcfg.max_file_time_diff < statcfg.time_interval)
			statcfg.max_file_time_diff = statcfg.time_interval;
		return;
	}
	pr_error(errkey,"Did not find the \"FILE_MASK\" block in the config file \'%s\'.\n", fname);
}


/* Read the user block of the config file.
 */
static void read_user_config(String fname, Widget w, xmlNodePtr node)
{
	int ndx;
	xmlNodePtr top = NULL, cur = NULL, data = NULL;
	statcfg.user_time_format = "%Y%m%d%H%M";

	/* Set the default colour for the trend line of the editable
	 * forecast rank weight.
	 */
	if((ndx = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id)) >= 0)
		statcfg.rankweight.edit_color = statcfg.element[ndx].trend_line_color;
	else
		statcfg.rankweight.edit_color = XuLoadColor(w,"white");

	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,"USER")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if (cur->type != XML_ELEMENT_NODE) continue;
			String sval = xmlNodeGetContent(cur);
			if(sval)
			{
				no_white(sval);
				if(KEYNODE(cur,"timeFormat"))
				{
					statcfg.user_time_format = XtNewString(sval);
				}
				else if(KEYNODE(cur,"dataOffString"))
				{
					statcfg.data_off_string = XtNewString(sval);
				}
				else if(KEYNODE(cur,"rankWeightHeight"))
				{
					Boolean ok;
					int ival = rs_int_parse(sval, &ok);
					if(ok)
						statcfg.rankweight.trend_graph_height = ival;
					else
						pr_error(errkey,"Unable to parse rankWeightHeight value in config file.\n");
				}
				else if(KEYNODE(cur,"minTrendTime"))
				{
					Boolean ok;
					float dt = rs_float_parse(sval, &ok);
					if(ok)
						statcfg.min_trend_time = dt;
					else
						pr_error(errkey,"Unable to parse minTrendTime value in config file.\n");
				}
				else if(KEYNODE(cur,"rankWeightMinMax"))
				{
					Boolean ok;
					float mm = rs_float_parse(sval, &ok);
					if(ok)
						statcfg.rankweight.min_max = mm;
					else
						pr_error(errkey,"Unable to parse rankWeightMinMax value in config file.\n");
				}
				else if(KEYNODE(cur,"labelAlignment"))
				{
					Boolean ok;
					statcfg.label_alignment = get_alignment_from_string(sval, &ok);
					if(!ok) pr_error(errkey, "%s - Unrecognized label alignment \'%s\'\n", fname, sval);
				}
				else if(KEYNODE(cur,"timeDisplayCompress"))
				{
					Boolean ok;
					float dt = rs_float_parse(sval, &ok);
					if(ok)
						statcfg.time_compress_val = dt;
					else
						pr_error(errkey,"Unable to parse rankWeightMinMax value in config file.\n");
				}
				else if(KEYNODE(cur,"rankWeightEditColor"))
				{
					statcfg.rankweight.edit_color = XuLoadColor(w,sval);
				}
				else
				{
					pr_error(errkey,"%s - Unknown FILE_SEARCH key \'%s\'\n", fname, cur->name);
				}
			}
			else
			{
				pr_error(errkey,"%s - Missing value for \'%s\'\n", fname, cur->name);
			}
			xmlFree(sval);
		}
		return;
	}
	pr_error(errkey,"Did not find the \"USER\" block in the config file \'%s\'.\n", fname);
}


/* Read the VIEWS section of the config file. It is not an error if this
 * section does not exist as it is optional.
 */
static void read_elem_view_config(String fname, xmlNodePtr node)
{
	int i;
	String value, label;
	Widget pane;
	xmlNodePtr top = NULL, cur = NULL, data = NULL;

	/* Set the default element view. This is the same as the entire
	 * list of elements (minus those with no data type). Note that
	 * this list can be overriden through a VIEW with a label of 'default'.
	 */
	nelem_view = 1;
	elem_view = OneMem(VIEWS);
	elem_view[0].label  = "default";
	elem_view[0].nelemp = 0;
	elem_view[0].elemp  = NewMem(ELEMCFG*, statcfg.nelement);
	for(i = 0; i < statcfg.nelement; i++)
	{
		/* Exclude any elements with no data type defined */
		if(statcfg.element[i].data_type == DT_NONE) continue;
		elem_view[0].elemp[elem_view[0].nelemp++] = &statcfg.element[i];
	}

	/* Scan looking for the VIEWS section */
	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,"VIEWS")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			/* Process the VIEW blocks */
			if(!KEYNODE(cur,"VIEW")) continue;
			label = xmlGetProp(cur, "label");
			if(!label)
			{
				pr_error(errkey,"%s - Found a VIEW section in the config without a label property.\n", fname);
			}
			else
			{
				int n = 0;
				/*
				 * If the label is 'default' the default list is being redefined.
				 */
				if(!same_ic(label,"default"))
				{
					n = nelem_view++;
					elem_view = MoreMem(elem_view, VIEWS, nelem_view);
					elem_view[n].label = XtNewString(label);
					elem_view[n].elemp = NewMem(ELEMCFG*, statcfg.nelement);
				}
				elem_view[n].nelemp = 0;

				for(data = cur->children; data; data = data->next)
				{
					if(!KEYNODE(data,"element")) continue;
					value = xmlNodeGetContent(data);
					no_white(value);
					for(i = 0; i < statcfg.nelement; i++)
					{
						if(xmlStrcmp(value,statcfg.element[i].id)) continue;
						if(statcfg.element[i].data_type == DT_NONE) continue;
						elem_view[n].elemp[elem_view[n].nelemp++] = &statcfg.element[i];
						break;
					}
					xmlFree(value);
				}
				if(elem_view[n].nelemp == 0)
					pr_error(errkey,"%s - Config file VIEW section \'%s\' has no recognized elements.\n",
							fname, label);
				xmlFree(label);
			}
		}
	}
	nelemp = elem_view[0].nelemp;
	elemp  = elem_view[0].elemp;
}


/* Read the radar statistics configuration file.
 */
Boolean rs_read_config(Widget w)
{
	String fname = NULL;
	String version = NULL;
	xmlDocPtr stat = NULL;
    xmlNodePtr root = NULL, level1, level2, data;

	if(statcfg.element) return True;

	/* If the config file or data directory are not defined return
	 * without error as the setup not defining them is not an error
	 * unless the radar statistics package is used.
	 */
	stat_data_dir = get_directory(RADAR_STAT);
	if(IsNull(stat_data_dir)) return False;
	if(access(stat_data_dir,R_OK|W_OK))
	{
		pr_error(pgm, "Could not access SCIT file data directory \'%s\'\n", stat_data_dir);
		return False;
	}

	fname = config_file_name(RADAR_STAT);
	if(IsNull(fname)) return False;
	if(access(fname,R_OK))
	{
		pr_error(pgm,"Could not access configuration file \'%s\'.\n", pgm, fname);
		return False;
	}
	else
	{
		char buf[100];
		snprintf(buf, sizeof(buf), "%s [Config file = %s]", pgm, fname);
		errkey = XtNewString(buf);
	}

    /* This initializes the library and checks for potential
	 * ABI mismatches between the version it was compiled for
	 * and the actual shared library used.
     */
    LIBXML_TEST_VERSION

	if((stat = xmlReadFile(fname, NULL, 0)) == NULL)
	{
		pr_error(errkey,"Could not parse file %s\n", fname);
		return False;
	}

	root = xmlDocGetRootElement(stat);
	if(root == NULL)
	{
		pr_error(errkey,"File \'%s\' is empty.\n", fname);
		xmlFreeDoc(stat);
		return False;
	}
	if(!NODENAME(root,"STATCONFIG"))
	{
		pr_error(errkey,"File \'%s\' is not a STAT config file.\n", fname);
		xmlFreeDoc(stat);
		return False;
	}
	version = xmlGetProp(root,"version");
	if(!version)
		pr_error(errkey,"Unspecified configuration file version when version %d is required.\n",
				CONFIG_VERSION);
	else if(atoi(version) != CONFIG_VERSION)
		pr_error(errkey,"Version %s of configuration file found when version %d is required.\n",
			version, CONFIG_VERSION);
	xmlFree(version);

	read_file_info_config(fname, root->children);	/* must come first */
	read_environments_config(fname, root->children);
	read_element_config(fname, w, root->children);	/* must come after read_environments_config */
	read_threshold_config(fname, w, root->children);/* must come after read_environments_config */
	read_user_config(fname, w, root->children);
	read_elem_view_config(fname, root->children);	/* must come after read_element_config */

	xmlFreeDoc(stat);

	return True;
}


/* The config reader will not complain when it runs as it is quite possible
 * and legal for a setup file not to include the keys that define files and
 * directories that this dialog needs. Thus complaints are only made if the
 * dialog is activated and this function is only run on dialog creation.
 */
Boolean rs_validate_config(void)
{
	String fname, dir;

	if(statcfg.element) return True;

	fname = config_file_name(RADAR_STAT);
	if(!fname)
		pr_error(pgm,"Config file keyword \'%s\' not defined in setup file.\n", RADAR_STAT);
	else if(access(fname,R_OK))
		pr_error(pgm,"Config file \'%s\' not accessable for read.\n", fname);

	/* get_directory issues its own error message */
	dir = get_directory(RADAR_STAT);
	if(dir && access(dir,R_OK|W_OK))
		pr_error(pgm,"Data directory \'%s\' not accessable for read/write.\n", dir);

	return False;
}
