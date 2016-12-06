/***********************************************************************
*                                                                      *
*   i n g r e d _ p r i v a t e . h                                    *
*                                                                      *
*   Private include file for ingred library.                           *
*                                                                      *
*     Version 8 (c) Copyright 2011 Environment Canada                       *
*                                                                           *
*   This file is part of the Forecast Production Assistant (FPA).           *
*   The FPA is free software: you can redistribute it and/or modify it      *
*   under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation, either version 3 of the License, or       *
*   any later version.                                                      *
*                                                                           *
*   The FPA is distributed in the hope that it will be useful, but          *
*   WITHOUT ANY WARRANTY; without even the implied warranty of              *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
*   See the GNU General Public License for more details.                    *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.        *
*                                                                      *
***********************************************************************/

/* Only include if not already included */
#ifndef INGRED

/* Need these system include files */
#include <X11/Intrinsic.h>
#undef TRUE
#undef FALSE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
extern	int	errno;

/* Need FPA include files */
#include <fpa.h>
#include <fpa_math.h>
#include <graphics/graphics.h>

#undef DEBUG
#undef DEVELOPMENT

/***********************************************************************
*                                                                      *
*   Prototype assorted structures for global data.                     *
*                                                                      *
***********************************************************************/

#define NULLS {0}

/* Define VTIME structure to store time sequence information */
typedef	struct	VTIMEst
	{
	STRING	jtime;		/* valid time string "yyyy:ddd:hh" */
	STRING	mtime;		/* valid time string "yyyy/mm/dd hh" */
	int		year;		/* valid time components */
	int		jday;
	int		month;
	int		mday;
	int		hour;
	int		minute;
	int		tplus;		/* hours relative to first depiction */
	int		mplus;		/* minutes relative to first depiction */
	STRING	status;		/* status message */
	LOGICAL	local;		/* Is this a local time */
	LOGICAL	depict;		/* Is this a depiction time */
	} VTIME;

/* Define FRAME structure to store depiction or scratchpad image */
typedef	struct	FRAMEst
	{
	STRING		wfile;		/* working metafile name */
	STRING		bfile;		/* backup metafile name */
	STRING		owfile;		/* working metafile name (old format) */
	STRING		obfile;		/* backup metafile name (old format) */
	METAFILE	meta;		/* metafile structure */
	LOGICAL		modified;	/* has it been modified ? */
	LOGICAL		contoured;	/* has it been contoured ? */
	} FRAME;

/* Define GFRAME structure to store guidance image */
typedef	struct	GFRAMEst
	{
	STRING		jtime;		/* valid time string "yyyy:ddd:hh" */
	METAFILE	meta;		/* metafile structure */
	LOGICAL		show;		/* do we want to show this chart? */
	LOGICAL		contoured;	/* has it been contoured ? */
	} GFRAME;

/* Define GLIST structure to store guidance field sequences */
typedef	struct	GLISTst
	{
	DISPNODE	dn;			/* panel for field */
	GFRAME		*charts;	/* field chart sequence */
	int			nchart;		/* number of charts in sequence */
	STRING		tag;		/* internal name for this field (blank if free) */
	LOGICAL		erase;		/* field erased (on/off) */
	STRING		elem;		/* field element name */
	STRING		level;		/* field level name */
	STRING		source;		/* field source name */
	STRING		subsrc;		/* field subsource name */
	STRING		rtime;		/* run time stamp */
	COLOUR		colour;		/* field colour (if modified) */
	LSTYLE		style;		/* field line style (if modified) */
	float		width;		/* field line width (if modified) */
	int			labpos;		/* label position indicator */
	} GLIST;

/* Define field visibility states */
typedef	enum
	{
	SHOW_ACTIVE_FLD,	/* Show only when the field is active */
	SHOW_ACTIVE_GRP,	/* Show only when the group is active */
	SHOW_ALWAYS			/* Show always */
	} SHOW;

/* Define link types */
typedef	enum
	{
	LINK_REG,	/* regular (depiction time) link node */
	LINK_CTL,	/* control (non-depiction time) link node */
	LINK_INT	/* interpolated (visual) link node */
	} LTYPE;

/* Define LCTRL structure to store lists of intermediate link controls */
typedef	struct	LCTRLst
	{
	int			lnum;		/* number of intermediate link nodes */
	int			ncont;		/* number of active intermediate link nodes */
	LOGICAL		*cthere;	/* is this an active intermediate link node? */
	int			*mplus;		/* time for each intermediate link node */
	int			*lcnum;		/* number of control nodes for each active node */
	POINT		**cnode;	/* control node locations */
	int			**ilink;	/* control node link chain */
	} LCTRL;

/* Define PERIOD structure to store daily period information */
typedef	struct	PERIODst
	{
	LOGICAL		local;		/* is period given in GMT or chart local time? */
	int			shrs;		/* start of period (in hours) */
	int			ehrs;		/* end of period (in hours) */
	int			vhrs;		/* normal valid time (in hours) */
	} PERIOD;

/* Define timelink/interpolation states */
/* Replaces ->linked, ->interp, etc. from previous version */
#ifdef NEW_STUFF
typedef	enum
	{
	TM_NA,			/* link/interp status is irrelevant */
	TM_LINK_NONE,	/* Field has no links at all */
	TM_LINK_SOME,	/* Field has some links, but not enough to interpolate */
	TM_LINK_SUFF,	/* Field has enough links to interpolate */
	TM_INTERP_FLD,	/* Field has been interpolated, but not the labels */
	TM_INTERP_ALL	/* Field has been interpolated, including the labels */
	} TMSTAT;
#endif

/* Define DFLIST structure to store field/sequence data */
typedef	struct	DFLISTst
	{
	STRING		element;	/* element */
	STRING		level;		/* level (sfc, pressure, sigma, theta...) */
	STRING		entity;		/* entity (a, b, c, d) */
	STRING		group;		/* group that this field belongs to */
	int			editor;		/* editor (continuous, discrete, line...) */
	LOGICAL		bgset;		/* do we have a background? */
	CAL			bgcal;		/* background attributes */
	CAL			defcal;		/* default background attributes */
	LOGICAL		there;		/* is this field present? */
	LOGICAL		showgrp;	/* is the group visible at all times? */
	SHOW		showdep;	/* whether to show this field in the depiction */
	LOGICAL		showmov;	/* whether to show this field for animation */
	LOGICAL		doedit;		/* can we edit the field */
	LOGICAL		dohilo;		/* can we put labels at highs and lows */
	LOGICAL		dowind;		/* can we use wind barbs as labels */
	LOGICAL		dolink;		/* can/should we link/interpolate this one */
	LOGICAL		reorder;	/* can/should we allow this one to be reordered */
	LOGICAL		required;	/* is this a required field? */
	int			tdep;		/* time-dependence (normal, daily, static) */
	LOGICAL		daily;		/* is this a field with a daily valid period? */
	PERIOD		period;		/* daily valid period description */
	int			labpos;		/* label position for static/daily fields */
	DISPNODE	dn;			/* panel for field */
	FRAME		*frames;	/* sequence of charts */
	FIELD		*fields;	/* sequence of working fields */
	SET			*flabs;		/* .. corresponding generic labels */
	FIELD		*snaps;		/* "interpolated" snapshot of special fields */
	SET			*slabs;		/* .. corresponding generic labels */
	FIELD		*tweens;	/* interpolated sequence of regular fields */
	SET			*tlabs;		/* .. corresponding generic labels */
	struct DFLISTst
				*linkto;	/* pointer to another DFLIST for borrowing links */
	int			nchain;		/* number of link chains */
	LCHAIN		*chains;	/* set of link chains */
#ifdef NEW_STUFF
	TMSTAT		tmstat;		/* link/interp status */
#else
	LOGICAL		linked;		/* has it been linked enough to interpolate? */
	LOGICAL		interp;		/* has it been interpolated? */
	LOGICAL		intlab;		/* have the labels also been interpolated? */
#endif
	LOGICAL		saved;		/* has it been saved? */
	LOGICAL		reported;	/* has it been reported to interface? */
	} DFLIST;

#define tdep_normal(tdep)  ((LOGICAL) (tdep == FpaC_NORMAL))
#define tdep_static(tdep)  ((LOGICAL) (tdep == FpaC_STATIC))
#define tdep_daily(tdep)   ((LOGICAL) (tdep == FpaC_DAILY))
#define tdep_special(tdep) (tdep_daily(tdep) || tdep_static(tdep))

/* Define OVERLAY structure to store map overlays */
typedef	struct	OVERLAYst
	{
	DISPNODE	dn;		/* panel to display overlay in */
	STRING		name;	/* overlay name */
	} OVERLAY;

/* Define the principal editor states */
typedef	enum
	{
	EDIT_START_UP,
	EDIT_NORMAL,
	EDIT_SUSPEND
	} EDIT_STATE;

/* Define drawing modes */
typedef	enum
	{
	DRAW_CONT,
	DRAW_PTPT
	} DRAW_MODE;

/* Define merge modes */
typedef	enum
	{
	MERGE_FIELD,
	MERGE_FIELD_AND_LABELS
	} MERGE_MODE;

/* Define move modes */
typedef	enum
	{
	MOVE_FIELD,
	MOVE_FIELD_AND_LABELS
	} MOVE_MODE;

/* Define modification modes */
typedef	enum
	{
	MODIFY_CONT,
	MODIFY_PTPT,
	MODIFY_PUCK
	} MODIFY_MODE;

/* Define sculpting modes */
typedef	enum
	{
	SCULPT_REMOVE,
	SCULPT_PUSH
	} SCULPT_MODE;

/* Define stacking modes */
typedef	enum
	{
	STACK_TOP,
	STACK_BOTTOM
	} STACK_MODE;

/* Define link modes */
typedef	enum
	{
	LINK_NORMAL,
	LINK_INTERMEDIATE
	} LINK_MODE;

/* Define classes for identifying the current editor */
typedef	enum
	{
	MODULE_NONE,
	MODULE_DEPICT,
	MODULE_SCRATCH,
	MODULE_IMAGERY,
	MODULE_GUID,
	MODULE_LINK,
	MODULE_ANIM,
	MODULE_ZOOM
	} MODULE;
typedef	enum
	{
	EDITOR_NONE,
	EDITOR_EDIT,
	EDITOR_LABEL,
	EDITOR_SAMPLE,
	EDITOR_ZOOM,
	EDITOR_PAN,
	EDITOR_START,
	EDITOR_RESUME
	} EDITOR;
typedef enum
	{
	MOVIE_NONE,
	MOVIE_DEPICT,
	MOVIE_INTERP
	} MOVIE;

/***********************************************************************
*                                                                      *
*  Initialize global data in master module only.                       *
*  Import global data into other modules.                              *
*                                                                      *
***********************************************************************/

#undef GLOBAL
#ifdef INGRED_MASTER
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

	/* Global info for graphics devices */
	GLOBAL( XtAppContext, X_appcon   , 0 );		/* Xt application context */
	GLOBAL( Widget		, X_widget   , 0 );		/* Widget for drawing window */
	GLOBAL( Display		, *X_display , NULL );	/* X-display */
	GLOBAL( Window		, X_window   , 0 );		/* X-window */

	/* Global info for graphics panels */
	GLOBAL( DISPNODE	, DnRoot      , NULL );	/* root panel */
	GLOBAL( DISPNODE	, DnMap       , NULL );	/* map panel */
	GLOBAL( DISPNODE	, DnBgnd      , NULL );	/* actual map background */
	GLOBAL( DISPNODE	, DnImage     , NULL );	/* master imagery panel */
	GLOBAL( DISPNODE	, DnDepict    , NULL );	/* master depiction panel */
	GLOBAL( DISPNODE	, DnOrig      , NULL );	/* depiction edit panel */
	GLOBAL( DISPNODE	, DnEdit      , NULL );	/* depiction edit panel */
	GLOBAL( DISPNODE	, DnLink      , NULL );	/* time link panel */
	GLOBAL( DISPNODE	, DnGuid      , NULL );	/* master guidance panel */
	GLOBAL( DISPNODE	, DnScratch   , NULL );	/* master scratchpad panel */
	GLOBAL( DISPNODE	, DnExtrap    , NULL );	/* link start/end menu panel */
	GLOBAL( DISPNODE	, DnLNode     , NULL );	/* link node menu panel */
	GLOBAL( DISPNODE	, DnTemp      , NULL );	/* temporary edit panel */
	GLOBAL( DISPNODE	, DnSample    , NULL );	/* temporary sample panel */
	GLOBAL( DISPNODE	, DnBlank     , NULL );	/* temporary blank panel */
	GLOBAL( METAFILE	, ScratchMeta , NULL );	/* metafile in scratchpad */
	GLOBAL( METAFILE	, OrigMeta    , NULL );	/* metafile in orig panel */
	GLOBAL( METAFILE	, EditMeta    , NULL );	/* metafile in edit panel */
	GLOBAL( METAFILE	, LinkMeta    , NULL );	/* metafile in link panel */
	GLOBAL( METAFILE	, ExtrapMeta  , NULL );	/* metafile in extrap menu */
	GLOBAL( METAFILE	, LNodeMeta   , NULL );	/* metafile in link node menu */
	GLOBAL( METAFILE	, TempMeta    , NULL );	/* metafile in temp panel */
	GLOBAL( METAFILE	, SampleMeta  , NULL );	/* metafile in sample panel */
	GLOBAL( METAFILE	, BlankMeta   , NULL );	/* metafile in blank panel */

	/* Module state variables */
	GLOBAL( EDIT_STATE	, EditState     , EDIT_START_UP );	/* edit state */
	GLOBAL( LOGICAL		, ViewOnly      , TRUE );	/* view only mode? */
	GLOBAL( LOGICAL		, ConserveMem   , TRUE );	/* careful with memory? */
	GLOBAL( LOGICAL		, SequenceReady , FALSE );	/* depictions read? */
	GLOBAL( LOGICAL		, DepictVis     , FALSE );	/* depict vis changed? */
	GLOBAL( LOGICAL		, DepictShown   , FALSE );	/* depictions shown? */
	GLOBAL( LOGICAL		, DepictReady   , FALSE );	/* depict already read? */
	GLOBAL( LOGICAL		, EditShown     , FALSE );	/* edit fld shown? */
	GLOBAL( LOGICAL		, LinkShown     , FALSE );	/* timelinks shown? */
	GLOBAL( LOGICAL		, ScratchShown  , FALSE );	/* scratchpad shown? */
	GLOBAL( LOGICAL		, GuidShown     , FALSE );	/* guidance shown? */
	GLOBAL( LOGICAL		, ImageShown    , FALSE );	/* imagery shown? */
	GLOBAL( LOGICAL		, ExtrapShown   , FALSE );	/* extrap menu shown? */
	GLOBAL( LOGICAL		, LNodeShown    , FALSE );	/* link node menu shown? */
	GLOBAL( LOGICAL		, TempShown     , TRUE );	/* temporary panel shown? */
	GLOBAL( LOGICAL		, SampleShown   , TRUE );	/* sample panel shown? */
	GLOBAL( LOGICAL		, BlankShown    , FALSE );	/* blank panel shown? */
	GLOBAL( LOGICAL		, Spawned       , FALSE );	/* have we been spawned? */
	GLOBAL( LOGICAL		, Posting       , FALSE );	/* filter incoming cmds? */

	/* Global buffers for metafile sequences */
	GLOBAL( int		, MaxGuid     , 20 );	/* allocated guidance fields */
	GLOBAL( int		, NumDfld     , 0 );	/* number of depiction fields */
	GLOBAL( int		, NumMaster   , 0 );	/* number master links */
	GLOBAL( int		, NumTime     , 0 );	/* number of valid times */
	GLOBAL( int		, NumTween    , 0 );	/* number of interpolation times */
	GLOBAL( int		, DTween      , 60 );	/* interpolation time interval */
	GLOBAL( int		, EditTime    , -1 );	/* valid time of edit field */
	GLOBAL( int		, GrpTime     , -1 );	/* valid time of fields in group */
	GLOBAL( int		, ViewTime    , -1 );	/* valid time of other fields */
	GLOBAL( int		, LinkVtime   , -1 );	/* old active time while linking */
	GLOBAL( int		, LinkEtime   , -1 );	/* old edit time while linking */
	GLOBAL( int		, LinkGtime   , -1 );	/* old group time while linking */
	GLOBAL( int		, LinkDir     , 0 );	/* link direction +=fwd -=bkwd */
	GLOBAL( LOGICAL	, LinkRead    , FALSE );/* have links been read in yet */
	GLOBAL( VTIME	, *TimeList   , NULL );	/* full sequence of valid times */
	GLOBAL( FRAME	, ScratchPad  , {0} );	/* scratchpad frame */
	GLOBAL( GLIST	, *GuidFlds   , NULL );	/* guidance field sequences */
	GLOBAL( DFLIST	, *ActiveDfld , NULL );	/* current depiction field */
	GLOBAL( DFLIST	, *DfldList   , NULL );	/* list of depiction field info */
	GLOBAL( DFLIST	, *MasterLinks, NULL );	/* list of master links */

	/* Current edit objects in active depiction */
	GLOBAL( STRING		, CurrElement , NULL );	/* current element */
	GLOBAL( STRING		, CurrLevel   , NULL );	/* current level */
	GLOBAL( STRING		, CurrGroup   , NULL );	/* current field group */
	GLOBAL( int			, CurrEditor  , 0 );	/* current editor type */
	GLOBAL( METAFILE	, ActiveMeta  , NULL );	/* depiction metafile to edit */
	GLOBAL( FIELD		, ActiveField , NULL );	/* fields to edit */
	GLOBAL( FIELD		, LabField    , NULL );
	GLOBAL( SURFACE		, OldSurface  , NULL );	/* original unedited fields */
	GLOBAL( SET			, OldAreas    , NULL );
	GLOBAL( SET			, OldCurves   , NULL );
	GLOBAL( SET			, OldPoints   , NULL );
	GLOBAL( SET			, OldLchains  , NULL );
	GLOBAL( SET			, OldLabs     , NULL );
	GLOBAL( SURFACE		, NewSurface  , NULL );	/* copies of fields to edit */
	GLOBAL( SET			, NewAreas    , NULL );
	GLOBAL( SET			, NewCurves   , NULL );
	GLOBAL( SET			, NewPoints   , NULL );
	GLOBAL( SET			, NewLchains  , NULL );
	GLOBAL( SET			, NewLabs     , NULL );

	/* Current edit objects for all modules */
	GLOBAL(	FLD_DESCRIPT, EditFd      , {0} );	/* General field descriptor */
	GLOBAL(	FLD_DESCRIPT, ValFd       , {0} );	/* Value field descriptor */
	GLOBAL(	FLD_DESCRIPT, WindFd      , {0} );	/* Wind field descriptor */
	GLOBAL(	LOGICAL		, WindXref    , 0 );	/* Is wind a cross ref? */
	GLOBAL(	STRING		, WindType    , NULL );	/* Which wind cross ref? */
	GLOBAL(	LOGICAL		, EditUndoable, TRUE );	/* Can we undo the edit? */
	GLOBAL(	LOGICAL		, EditRetain  , FALSE);	/* Should we use active field? */
	GLOBAL(	SURFACE		, EditSfc     , NULL );	/* Surface to edit */
	GLOBAL(	SET			, EditAreas   , NULL );	/* Area set to edit */
	GLOBAL(	SET			, EditCurves  , NULL );	/* Curve set to edit */
	GLOBAL(	SET			, EditPoints  , NULL );	/* Spot set to edit */
	GLOBAL(	SET			, EditLchains , NULL );	/* Link chain set to edit */
	GLOBAL(	SET			, EditLabs    , NULL );	/* Generic labels */
	GLOBAL(	SET			, EditMarks   , NULL );	/* Generic marks */
	GLOBAL(	COLOUR		, EditColour  , -1 );	/* Label/sample colour */
	GLOBAL(	LSTYLE		, EditStyle   , -1 );	/* Label/sample line style */
	GLOBAL(	float		, EditWidth   , -1 );	/* Label/sample line width */
	GLOBAL(	FONT		, EditFont    , -1 );	/* Label/sample text font */
	GLOBAL(	float		, EditLsize   , -1 );	/* Label/sample label size */
	GLOBAL(	float		, EditBsize   , -1 );	/* Label/sample barb size */
	GLOBAL(	float		, EditXoff    ,  0 );	/* Label/sample x-offset */
	GLOBAL(	float		, EditYoff    ,  0 );	/* Label/sample y-offset */
	GLOBAL(	float		, EditFact    ,  0 );	/* Label/sample scale */
	GLOBAL(	int			, EditNumP    ,  0 );	/* Label/sample list count */
	GLOBAL(	POINT		, *EditPlist  , NULL );	/* Label/sample list */
	GLOBAL(	LOGICAL		, EditUseList , FALSE);	/* Label/sample list ready */
	GLOBAL(	LOGICAL		, EditFullSam , TRUE );	/* Use full sample */
	GLOBAL(	LOGICAL		, EditDoHiLo  , FALSE);	/* Use hi/lo labels */

	/* Editor control info */
	GLOBAL( MODULE	, Module          , MODULE_NONE );	/* Current module */
	GLOBAL( EDITOR	, Editor          , EDITOR_NONE );	/* Current editor */
	GLOBAL( LOGICAL	, (*EditFunc)()   , NULL );	/* Current edit function */
	GLOBAL( char	, EditMode[20]    , "" );	/* Current edit mode */
	GLOBAL( LOGICAL	, EditResume      , FALSE);	/* Resume previous edit mode? */
	GLOBAL( char	, EditVal[10][128], {0} );	/* Current edit parameters */
	GLOBAL( int		, nEditVal        , 10 );	/* Max edit parameters */
	GLOBAL( CAL		, EditCal         , NullCal );	/* Edit value struct */
	GLOBAL( CAL		, EditCalX        , NullCal );	/* Secondary edit value struct */

	/* Timelink display colours */
	GLOBAL(	COLOUR	, LinkColour      , -1 );	/* Colour for link chains */
	GLOBAL(	COLOUR	, LinkTextColour  , -1 );	/* Colour for link labels */
	GLOBAL(	COLOUR	, LinkGuessColour , -1 );	/* Colour for guess links */

	/* Timelink display options */
	GLOBAL( LOGICAL	, DisplayTime,  TRUE );	/* display depiction link times */
	GLOBAL( LOGICAL	, DisplayEarly, TRUE );	/* display early/late link times */
	GLOBAL( LOGICAL	, DisplayCtrl,  TRUE );	/* display control link times */
	GLOBAL( LOGICAL	, DisplaySpeed, TRUE );	/* display link chain speeds */
	GLOBAL( char	, SpdUnits[64], "knots" );	/* display units for speeds */
	GLOBAL( char	, SpdLabel[64], "kt" );		/* label for speed units */

	/* Imagery control */
	GLOBAL( LOGICAL		, ImageBlended, FALSE );	/* blend imagery? */
	GLOBAL( int			, ImageBlend  , 0 );		/* blend imagery? */
	GLOBAL( Image		, ActiveImage , (Image)0 );	/* image to sample */

	/* Animation control */
	GLOBAL( LOGICAL	, MovieShown, FALSE );		/* is the movie menu active? */
	GLOBAL( LOGICAL	, MovieGoing, FALSE );		/* is there a movie running? */
	GLOBAL( LOGICAL	, MovieOK   , FALSE );		/* is the movie showable? */
	GLOBAL( MOVIE	, MovieMode , MOVIE_NONE );	/* type of movie */
	GLOBAL( STRING	, MovieMsg  , NULL );		/* movie message */
	GLOBAL( int		, MovieWait , 0 );			/* frame delay time */

	/* Map background info */
	GLOBAL( char	, MapName[256], "" );	/* map background name */
	GLOBAL( MAP_PROJ, *MapProj    , NULL );	/* target map proj ptr */
	GLOBAL( BOX		, *MapView    , NULL );	/* geometry of map ptr */
	GLOBAL( int		, NumOverlay  , 0 );	/* number of map overlays */
	GLOBAL( OVERLAY	, *Overlays   , NULL );	/* map overlays */
	GLOBAL( int		, LocalOffset , 0 );	/* difference from GMT */
	GLOBAL( COLOUR	, GeogLand    , -1 );	/* land colour */
	GLOBAL( COLOUR	, GeogWater   , -1 );	/* water colour */
	GLOBAL( COLOUR	, GeogCoast   , -1 );	/* coast colour */
	GLOBAL( COLOUR	, GeogBorder  , -1 );	/* border colour */
	GLOBAL( COLOUR	, GeogLatlon  , -1 );	/* latlon colour */
	GLOBAL( COLOUR	, GeogFarea   , -1 );	/* forecast area colour */
	GLOBAL( COLOUR	, GeogFbord   , -1 );	/* forecast area border colour */

	/* Graphics editor parameters */
	/* Some of these should be taken from presentation info */
	GLOBAL( float		, FilterRes , 0 );	/* drawing filter resolution */
	GLOBAL( float		, SplineRes , 0 );	/* curve spline resolution */
	GLOBAL( float		, PickTol   , 0 );	/* tolerance for picking */
	GLOBAL( float		, LinkRes   , 0 );	/* link interpolation resolution */
	GLOBAL( float		, LabelSize , 25 );	/* label size */
	GLOBAL( float		, LgndSize  , 30 );	/* guidance legend label size */
	GLOBAL( float		, WbarbSize , 40 );	/* wind barb size */
	GLOBAL( float		, LmarkSize , 20 );	/* link marker size */
	GLOBAL( int			, LabPos    , 1 );	/* guidance label position */
	GLOBAL( DRAW_MODE	, DrawMode  , DRAW_CONT );		/* drawing mode */
	GLOBAL( MERGE_MODE	, MergeMode , MERGE_FIELD );	/* merge mode */
	GLOBAL( MOVE_MODE	, MoveMode  , MOVE_FIELD );		/* move mode */
	GLOBAL( MODIFY_MODE	, ModifyMode, MODIFY_CONT );	/* modification mode */
	GLOBAL( SCULPT_MODE	, SculptMode, SCULPT_PUSH );	/* sculpting mode */
	GLOBAL( STACK_MODE	, StackMode , STACK_TOP );		/* stacking mode */
	GLOBAL( int			, LchainDelta  , 60 );			/* link chain    */
														/*  interp delta */
	GLOBAL( LINK_MODE	, LinkMode  , LINK_NORMAL );	/* timelink mode */
	GLOBAL( float		, DrawSmth  , 50 );	/* drawing smooth factor */
	GLOBAL( float		, ModifySmth, 50 );	/* modify smooth factor */
	GLOBAL( float		, ModifyPuck, 50 );	/* modify puck size */
	GLOBAL( float		, SpreadFact, 50 );	/* spread factor */
	GLOBAL(	float		, MaxSpread ,  0 );	/* Maximum spline edit spread */
	GLOBAL(	float		, LimSpread , 10 );	/* Limit spline edit spread */
	GLOBAL( float		, SplineSmth,  1 );	/* spline smoothing amount */

	/* Event handler states */
#	define	NoButton     0
#	define	LeftButton   1
#	define	MiddleButton 2
#	define	RightButton  3
	GLOBAL( LOGICAL	, Visible    , TRUE);		/* drawing area unobscured */
	GLOBAL( LOGICAL	, IgnoreObsc , FALSE);		/* allow input while obscured */
	GLOBAL( LOGICAL	, AllowInput , TRUE);		/* allow input */
	GLOBAL( int		, ButtonDown , NoButton);	/* which button is pressed */
	GLOBAL( int		, ButtonX    , 0);			/* x locator value */
	GLOBAL( int		, ButtonY    , 0);			/* y locator value */
	GLOBAL( LOGICAL	, Drawing    , FALSE);		/* are we drawing? */
	GLOBAL( LOGICAL	, DrawWait   , FALSE);		/* continuous drawing */
	GLOBAL( LOGICAL	, DrawReady  , FALSE);		/* draw finished */
	GLOBAL( LOGICAL	, DrawGrab   , FALSE);		/* pointer grabbed */

	/* Miscellaneous */
	GLOBAL( STRING	, HomeDir    , NULL );		/* home directory */
	GLOBAL( char	, Msg[4096]  , "" );		/* temporary message buffer */
	GLOBAL( TSTAMP	, Stime      , "" );		/* Initial (T0) date-time */
	GLOBAL( int		, Syear      , 0 );
	GLOBAL( int		, Sjday      , 0 );
	GLOBAL( int		, Smonth     , 0 );
	GLOBAL( int		, Smday      , 0 );
	GLOBAL( int		, Shour      , 0 );
	GLOBAL( int		, Sminute    , 0 );

	/* Default units for wind parameters and display */
	GLOBAL( char	, WindUnits[64],   "knots" ); /* default units for winds */
	GLOBAL( char	, WindDisplay[64], "knots" ); /* display units for winds */

	/* Default number of significant digits in writing metafiles */
	GLOBAL( int		, MaxDigits  , 6 );

#define NoSamp NullPtr(FpaConfigSampleStruct *)
#define NoXref NullPtr(FpaConfigCrossRefStruct *)

#undef GLOBAL

/***********************************************************************
*                                                                      *
*  Declare external functions in each module.                          *
*                                                                      *
***********************************************************************/

	/* Functions provided in ingred.c not already declared in ingred.h */
	void		read_edit_vals(int, STRING);
	void		move_edit_vals(int, int);
	void		set_edit_cal(CAL);
	void		set_edit_calx(CAL);

	/* Functions provided in feedback.c */
	void		define_feedback(void (*)(STRING, LOGICAL),
								void (*)(STRING, STRING),
								void (*)(STRING, CAL));
	void		suspend_feedback(void);
	void		resume_feedback(void);
	void		put_message(STRING, ...);
	void		string_message(STRING, STRING, ...);
	void		clear_message(void);
	void		obscure_message(LOGICAL);
	void		put_error_dialog(STRING);
	void		busy_cursor(LOGICAL);
	void		stop_cursor(LOGICAL);
	void		picking_cursor(LOGICAL);
	void		drawing_cursor(LOGICAL);
	void		obscure_cursor(LOGICAL);
	void		showing_chart(STRING);
	void		linking_chart(STRING);
	void		link_status(DFLIST *);
	void		end_control_link(void);
	void		interp_progress(DFLIST *, int, int, int, int);
	void		reset_background(DFLIST *);
	void		zoom_start(void);
	void		zoom_area(BOX *);
	void		zoom_pan_end(void);
	void		edit_menu(STRING);
	void		edit_select(CAL, LOGICAL);
	void		edit_select_hole(void);
	void		edit_select_node(CAL, LOGICAL);
	void		edit_adding_lchain(STRING);
	void		label_select(CAL);
	void		link_select(CAL, LOGICAL);
	void		field_times(DFLIST *);
	void		field_status(DFLIST *, int);
	void		field_create(DFLIST *, LOGICAL);
	void		guidance_legend_labels(STRING, COLOUR);
	void		sample_box(LOGICAL, CAL);
	void		edit_in_progress(void);
	void		edit_ignore(void);
	void		edit_pending(void);
	void		edit_complete(void);
	void		edit_allow_preset_outline(LOGICAL);
	void		edit_can_create(LOGICAL);
	void		edit_can_copy(LOGICAL);
	void		edit_can_paste(LOGICAL);
	void		edit_can_join(LOGICAL);
	void		edit_can_break(LOGICAL);
	void		edit_can_rejoin(LOGICAL);
	void		edit_can_proceed(LOGICAL);
	void		edit_can_clear(LOGICAL);
	void		edit_control(STRING, LOGICAL);
	void		edit_drawn_outline_posted(void);
	void		edit_moved_outline_posted(void);
	void		edit_stomp_outline_posted(void);
	void		edit_drawn_hole_posted(void);
	void		mode_draw_setable(LOGICAL);
	void		mode_control(STRING, LOGICAL);
	void		scratch_can_delete(LOGICAL);
	void		scratch_control(STRING, LOGICAL);
	void		drawing_control(LOGICAL);
	void		modifying_control(LOGICAL);
	void		interrupt_control(LOGICAL, LOGICAL);

	/* Functions provided in mouse.c */
	void		allow_obscured_input(LOGICAL);
	void		define_circle_echo(float, float);
	void		circle_echo(LOGICAL);
	void		calc_puck(float *, float *);
	LOGICAL		track_Xpointer(DISPNODE, SET, LOGICAL);
	LOGICAL		check_Xpoint(DISPNODE, POINT, int *);
	LOGICAL		ready_Xpoint(DISPNODE, POINT, int *);
	LOGICAL		ignore_Xpoint(void);
	LOGICAL		pick_Xpoint(DISPNODE, int, POINT, int *);
	LOGICAL		utrack_Xpoint(DISPNODE, SET, POINT, POINT, int *);
	LOGICAL		utrack_Xspan(DISPNODE, POINT, POINT, float, int *);
	LOGICAL		utrack_Xnode_add(DISPNODE, POINT, POINT, int, int *);
	LOGICAL		utrack_Xnode_move(DISPNODE, POINT, int, POINT, POINT, int,
							POINT, POINT, int *);
	LOGICAL		urotate_Xpoint(DISPNODE, SET, POINT, POINT, POINT, float *,
							int *);
	LOGICAL		uedit_Xcurve(DISPNODE, LINE, SUBAREA, float, float,
							int *, LOGICAL *);
	LOGICAL		uextend_Xcurve(DISPNODE, LINE, LOGICAL, float, float, int *);
	LOGICAL		set_Xcurve_modes(STRING);
	LOGICAL		drawing_Xcurve(void);
	LOGICAL		ready_Xcurve(void);
	LOGICAL		ignore_Xcurve(void);
	int			recall_Xcurve(LINE **);
	LOGICAL		draw_Xcurve(DISPNODE, float, float, LOGICAL);
	void		init_draw(LOGICAL, long);
	void		add_draw(int, int);
	void		end_draw(void);

	/* Functions provided in layout.c */
	LOGICAL		setup_panels(void);
	LOGICAL		reset_panels(void);
	METAFILE	meta_input(FLD_DESCRIPT *);
	LOGICAL		map_background(STRING, int);
	LOGICAL		map_overlay(int, STRING, STRING);
	LOGICAL		map_input(DISPNODE, STRING, LOGICAL);
	LOGICAL		map_palette(STRING);
	LOGICAL		link_palette(STRING);
	LOGICAL		present_all(void);
	LOGICAL		present_node(DISPNODE);
	LOGICAL		present_meta(DISPNODE);
	LOGICAL		present_field(DISPNODE, FIELD);
	LOGICAL		sync_display(void);
	LOGICAL		show_temp(void);
	LOGICAL		hide_temp(void);
	LOGICAL		present_temp(LOGICAL);
	LOGICAL		empty_temp(void);
	LOGICAL		show_sample(void);
	LOGICAL		hide_sample(void);
	LOGICAL		present_sample(LOGICAL);
	LOGICAL		empty_sample(void);
	LOGICAL		show_blank(void);
	LOGICAL		hide_blank(void);
	LOGICAL		present_blank(LOGICAL);
	LOGICAL		empty_blank(void);
	LOGICAL		global_display_state(LOGICAL);
	LOGICAL		default_geog_colours(COLOUR *, COLOUR *, COLOUR *, COLOUR *,
							COLOUR *, COLOUR *, COLOUR *);
	LOGICAL		printer_geog_colours(COLOUR *, COLOUR *, COLOUR *, COLOUR *,
							COLOUR *, COLOUR *, COLOUR *);
	LOGICAL		change_geog_colours(DISPNODE, COLOUR, COLOUR, COLOUR, COLOUR,
							COLOUR, COLOUR, COLOUR);
	LOGICAL		change_geog_catspec(CATSPEC *, STRING, COLOUR, COLOUR, COLOUR,
							COLOUR, COLOUR, COLOUR, COLOUR);
	LOGICAL		default_link_colours(COLOUR *, COLOUR *, COLOUR *);
	LOGICAL		change_link_colours(COLOUR, COLOUR, COLOUR);
	LOGICAL		default_temp_colours(COLOUR *, COLOUR *, COLOUR *, COLOUR *);
	LOGICAL		printer_temp_colours(COLOUR *, COLOUR *, COLOUR *, COLOUR *);

	/* Functions provided in sequence.c */
	LOGICAL		init_sequence(void);
	LOGICAL		read_sequence(STRING, STRING, STRING, LOGICAL);
	LOGICAL		insert_sequence(STRING, STRING, STRING, STRING, STRING);
	LOGICAL		delete_sequence(STRING);
	LOGICAL		save_sequence(STRING);
	LOGICAL		import_field(STRING, STRING, STRING, STRING, STRING, STRING,
							STRING);
	LOGICAL		delete_field(STRING, STRING, STRING);
	LOGICAL		save_field(STRING, STRING, STRING);
	LOGICAL		pick_sequence(STRING, STRING);
	LOGICAL		zero_sequence(STRING);
	int			find_valid_time(STRING);
	int			find_prog_time(int, LOGICAL);
	LOGICAL		make_depict_status(int, STRING, STRING);
	LOGICAL		tell_active_status(void);
	LOGICAL		give_active_status(void);
	LOGICAL		revise_depict_time(int);
	LOGICAL		check_depict_times(void);
	int			first_depict_time(void);
	int			last_depict_time(void);
	int			next_depict_time(int);
	int			prev_depict_time(int);
	int			which_depict_time(STRING);
	int			next_active_time(int);
	int			prev_active_time(int);
	LOGICAL		start_link(LOGICAL);
	LOGICAL		next_link(LCHAIN);
	LOGICAL		extrap_link(void);
	LOGICAL		new_link(void);
	LOGICAL		end_link(void);

	/* Functions provided in dfield.c */
	LOGICAL		pick_active_group(STRING);
	LOGICAL		pick_active_dfield(STRING, STRING);
	LOGICAL		set_group_state(STRING, STRING);
	LOGICAL		set_dfield_state(STRING, STRING, STRING);
	void		set_dfield_visibility(DFLIST *);
	LOGICAL		dfield_shown(DFLIST *);
	void		init_dfields(void);
	LOGICAL		extract_dfields(int);
	LOGICAL		extract_dfield(DFLIST *, int);
	LOGICAL		cleanup_dfields(void);
	LOGICAL		check_dfield_background(DFLIST *, int);
	DFLIST		*find_master_dfield(STRING);
	DFLIST		*find_dfield(STRING, STRING);
	DFLIST		*make_dfield(STRING, STRING);
	int			pick_dfield_frame(DFLIST *, int);

	/* Functions provided in animation.c */
	LOGICAL		init_animation(void);
	LOGICAL		suspend_animation(void);
	LOGICAL		exit_animation(void);
	LOGICAL		start_animation(void);
	LOGICAL		stop_animation(void);
	LOGICAL		animation_frame(STRING);
	LOGICAL		animation_mode(STRING, STRING, STRING);
	LOGICAL		animation_delay(int);
	LOGICAL		animation_dfield_state(STRING, STRING, STRING);

	/* Functions provided in depiction.c */
	LOGICAL			insert_depiction(STRING, STRING, STRING, STRING, int);
	LOGICAL			build_depiction(STRING, STRING, STRING, STRING, int);
	LOGICAL			find_depiction(STRING, STRING, STRING, STRING);
	LOGICAL			pick_depiction(void);
	LOGICAL			update_depiction(int);
	LOGICAL			backup_depiction(int);
	LOGICAL			delete_depiction(int);
	LOGICAL			import_depiction_field(FLD_DESCRIPT *, DFLIST *, int,
							LOGICAL, LOGICAL);
	FLD_DESCRIPT	*find_depiction_field(STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL			refit_depiction_field(DFLIST *, int);
	LOGICAL			label_depiction_field(DFLIST *, int);
	LOGICAL			pick_depiction_field(DFLIST *);
	LOGICAL			update_depiction_field(DFLIST *, int);
	LOGICAL			backup_depiction_field(DFLIST *, int);
	LOGICAL			delete_depiction_field(DFLIST *, int);
	LOGICAL			show_depiction(void);
	LOGICAL			hide_depiction(void);
	LOGICAL			present_depiction(LOGICAL);
	LOGICAL			present_depiction_field(DFLIST *);
	LOGICAL			set_background_value(STRING, STRING);
	LOGICAL			active_edit_group(STRING);
	LOGICAL			active_edit_field(STRING, STRING);
	LOGICAL			extract_edit_field(void);
	LOGICAL			release_edit_field(LOGICAL);
	LOGICAL			show_edit_field(void);
	LOGICAL			hide_edit_field(void);
	LOGICAL			present_edit_field(LOGICAL);
	LOGICAL			extract_special_tags(void);
	LOGICAL			release_special_tags(void);
	LOGICAL			depiction_check(void);
	LOGICAL			depiction_edit(STRING);
	LOGICAL			depiction_label(STRING);
	LOGICAL			depiction_sample(STRING);
	LOGICAL			edit_posted(void);
	void			post_mod(STRING);
	void			accept_mod(void);
	void			reject_mod(void);
	void			post_partial(LOGICAL);
	void			ignore_partial(void);
	void			cancel_partial(void);
	LOGICAL			copy_posted(STRING, STRING, STRING);
	void			post_spline_copy(STRING, STRING, SURFACE, CURVE,
							int, SPOT *);
	void			post_area_copy(STRING, STRING, int, AREA *,
							int, SPOT *, int *);
	void			post_line_copy(STRING, STRING, int, CURVE *,
							int, SPOT *, int *);
	void			post_point_copy(STRING, STRING, int, SPOT *);
	void			post_lchain_copy(STRING, STRING, int, LCHAIN *);
	void			post_lchain_node_copy(STRING, STRING, LMEMBER, LNODE);
	void			paste_spline_copy(STRING, STRING, SURFACE *, CURVE *,
							int *, SPOT **);
	void			paste_area_copy(STRING, STRING, int *, AREA **,
							int *, SPOT **, int **);
	void			paste_line_copy(STRING, STRING, int *, CURVE **,
							int *, SPOT **, int **);
	void			paste_point_copy(STRING, STRING, int *, SPOT **);
	void			paste_lchain_copy(STRING, STRING, int *, LCHAIN **);
	void			paste_lchain_node_copy(STRING, STRING, LMEMBER *, LNODE *);
	void			release_spline_copy(void);
	void			release_area_copy(void);
	void			release_line_copy(void);
	void			release_point_copy(void);
	void			release_lchain_copy(void);
	void			release_lchain_node_copy(void);
	LOGICAL			drawn_outline_posted(void);
	void			post_drawn_outline(CURVE);
	CURVE			retrieve_drawn_outline(void);
	CURVE			retrieve_named_outline(STRING);
	LOGICAL			moved_outline_posted(void);
	void			post_moved_outline(CURVE);
	CURVE			retrieve_moved_outline(void);
	LOGICAL			stomp_outline_posted(void);
	void			post_stomp_outline(CURVE);
	CURVE			retrieve_stomp_outline(void);
	LOGICAL			drawn_hole_posted(void);
	void			post_drawn_hole(CURVE);
	CURVE			retrieve_drawn_hole(void);
	int				retrieve_named_holes(STRING, CURVE **);

	/* Functions provided in imagery.c */
	LOGICAL		show_imagery(void);
	LOGICAL		hide_imagery(void);
	LOGICAL		present_imagery(LOGICAL);
	LOGICAL     active_image(STRING, STRING);
	LOGICAL		display_imagery(int, STRING, STRING);
	LOGICAL		remove_imagery(STRING);
	LOGICAL		blend_imagery(LOGICAL, int);
	LOGICAL     lut_imagery(STRING, STRING);
	LOGICAL		imagery_check(void);
	LOGICAL     imagery_sample(STRING);
	LOGICAL		sync_imagery(void);

	/* Functions provided in guidance.c */
	LOGICAL		init_guidance(void);
	LOGICAL		show_guidance(void);
	LOGICAL		hide_guidance(void);
	LOGICAL		sync_guidance(void);
	LOGICAL		present_guidance(LOGICAL);
	LOGICAL		release_guidance(void);
	LOGICAL		guidance_fld_register(STRING, STRING, STRING, STRING, STRING,
							STRING);
	LOGICAL		guidance_fld_deregister(STRING);
	LOGICAL		guidance_fld_visibility(STRING, STRING, STRING);
	GLIST		*register_gfield(STRING, STRING, STRING, STRING, STRING,
							STRING);
	GLIST		*deregister_gfield(STRING);
	GLIST		*find_gfield(STRING, LOGICAL);
	LOGICAL		check_gfield(GLIST *);
	LOGICAL		release_gfield(GLIST *, LOGICAL);
	LOGICAL		sync_gfield(GLIST *);
	LOGICAL		present_gfield(GLIST *, LOGICAL);
	GFRAME		*find_gfield_chart(GLIST *, STRING, LOGICAL);
	LOGICAL		check_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		read_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		release_gfield_chart(GLIST *, GFRAME *, LOGICAL);
	LOGICAL		show_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		hide_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		contour_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		label_gfield_chart(GLIST *, GFRAME *);
	LOGICAL		active_gfield_chart(STRING);
	LOGICAL		sample_gfield_chart(STRING, STRING);
	FIELD		gfield_chart_member(GLIST *, GFRAME *, STRING);
	LOGICAL		guidance_check(void);
	LOGICAL		guidance_edit(STRING);
	LOGICAL		guidance_label(STRING);
	LOGICAL		guidance_sample(STRING);
	LOGICAL		guid_fld_appearance(STRING, STRING);
	LOGICAL		guid_fld_erase(void);
	LOGICAL		guid_fld_restore(void);

	/* Functions provided in scratchpad.c */
	LOGICAL		show_scratch(void);
	LOGICAL		hide_scratch(void);
	LOGICAL		present_scratch(LOGICAL);
	LOGICAL		get_scratch(void);
	LOGICAL		update_scratch(void);
	LOGICAL		scratch_check(void);
	LOGICAL		scratch_edit(STRING);
	LOGICAL		edit_draw_generic_line(STRING, CAL);
	LOGICAL		edit_draw_generic_span(STRING, CAL, CAL, CAL, float);
	LOGICAL		edit_place_generic_label(STRING, CAL);
	LOGICAL		edit_select_generic_feature(STRING);

	/* Functions provided in timelink.c */
	LOGICAL		show_timelink(void);
	LOGICAL		hide_timelink(void);
	LOGICAL		present_timelink(LOGICAL);
	LOGICAL		define_link_resolution(void);
	LOGICAL		read_links(void);
	LOGICAL		save_links(void);
	LOGICAL		clear_links(LOGICAL);
	LOGICAL		clear_dfield_links(DFLIST *, LOGICAL);
	LOGICAL		interp_links(int, LOGICAL);
	LOGICAL		interp_dfield_links(DFLIST *, int, LOGICAL);
	LOGICAL		interp_link_node(LCHAIN, int, int, int, LOGICAL);
	LOGICAL		copy_dfield_links(DFLIST *, DFLIST *);
	LOGICAL		revise_links(void);
	LOGICAL		revise_dependent_links(DFLIST *);
	LOGICAL		extract_links(void);
	LOGICAL		release_links(void);
	LOGICAL		build_link_chain(DFLIST *, LCHAIN, LOGICAL);
	LOGICAL		prepare_link_chain(DFLIST *, LCHAIN);
	LOGICAL		remove_link_chain(DFLIST *, int);
	LOGICAL		remove_link_node(DFLIST *, int, int);
	LOGICAL		shuffle_link_chain(DFLIST *, int, int);
	LOGICAL		link_chains_posted(STRING, STRING, STRING);
	void		post_link_chains(STRING, STRING, STRING);
	void		replace_posted_chains(STRING, STRING, STRING);
	void		release_posted_chains(void);
	void		invoke_link_set_presentation(SET);
	void		invoke_link_chain_presentation(LCHAIN);
	LOGICAL		extract_unlinked(void);
	LOGICAL		release_unlinked(void);
	LOGICAL		build_extrap(LOGICAL, LOGICAL, int, int, int, POINT);
	LOGICAL		remove_extrap(void);
	LOGICAL		pick_extrap(POINT, int *);
	LOGICAL		present_extrap(LOGICAL);
	LOGICAL		build_ambiguous_nodes(LCHAIN, LTYPE, int, int *, int, POINT);
	LOGICAL		release_ambiguous_nodes(void);
	LOGICAL		pick_ambiguous_node(POINT, int *);
	LOGICAL		present_ambiguous_nodes(LOGICAL);
	STRING		link_frame_status(DFLIST *, int);
	LOGICAL		verify_links(LOGICAL);
	LOGICAL		verify_dfield_links(DFLIST *, LOGICAL);
	LOGICAL		ready_links(LOGICAL);
	LOGICAL		ready_dfield_links(DFLIST *, LOGICAL);
	LOGICAL		verify_interp(LOGICAL, LOGICAL);
	LOGICAL		verify_dfield_interp(DFLIST *, LOGICAL, LOGICAL);
	LOGICAL		release_interp(void);
	LOGICAL		release_dfield_interp(DFLIST *);
	LOGICAL		release_dfield_snaps(DFLIST *);
	LOGICAL		release_dfield_tweens(DFLIST *);
	LOGICAL		acquire_interp(void);
	LOGICAL		acquire_dfield_interp(DFLIST *, int);
	LOGICAL		acquire_dfield_snaps(DFLIST *, int);
	LOGICAL		acquire_dfield_tweens(DFLIST *, int);
	LOGICAL		prepare_interp(void);
	LOGICAL		prepare_dfield_interp(DFLIST *);
	LOGICAL		prepare_dfield_snaps(DFLIST *);
	LOGICAL		prepare_dfield_tweens(DFLIST *);
	LOGICAL		borrow_links(DFLIST *, STRING, STRING, LOGICAL);
	LOGICAL		timelink_check(void);
	LOGICAL		timelink_edit(STRING);
	int			closest_link_chain(int, POINT, DFLIST *, float, int *,
							LTYPE *, LOGICAL *, LOGICAL *);
	int			closest_link_end(LOGICAL, int, POINT, DFLIST *, float,
							LOGICAL *);
	LCTRL		*build_control_list(LCHAIN, int);
	LCTRL		*copy_control_list(LCTRL *);
	void		free_control_list(LCTRL *);
	void		add_nodes_to_control_list(LCTRL *, int, LCTRL *);
	void		post_interp_cancel(void);
	LOGICAL		interpolate(STRING, STRING);
	LOGICAL		interpolate_dfield(DFLIST *);
	LOGICAL		interp_needed(DFLIST *);
	LOGICAL		interp_static(DFLIST *, LOGICAL);

	/* Functions provided in zoom.c */
	LOGICAL		zoom_in(STRING);
	LOGICAL		zoom_out(void);
	LOGICAL		zoom_pan(STRING);
	LOGICAL		zoom_reset(STRING, STRING, STRING, STRING);
	LOGICAL		suspend_zoom(void);
	LOGICAL		resume_zoom(LOGICAL);
	LOGICAL		define_zoom(BOX *, LOGICAL, LOGICAL);
	void		zoom_dn_subtree(DISPNODE, MAP_DEF *, BOX *);
	LOGICAL		define_edit_resolution(void);
	float		zoom_factor(void);

	/* Functions provided in dump.c */
	LOGICAL		raster_dump(STRING, STRING, STRING, int, int, STRING, STRING,
							STRING);

	/* Functions provided in active.c */
	LOGICAL		eval_list_grid(STRING, STRING);
	LOGICAL		eval_list_add(STRING, STRING);
	LOGICAL		eval_list_reset(void);
	void		active_spline_fields(LOGICAL, SURFACE, SET);
	void		active_area_fields(LOGICAL, SET, SET);
	void		active_line_fields(LOGICAL, SET, SET);
	void		active_point_fields(LOGICAL, SET);
	void		active_lchain_fields(LOGICAL, SET);
	void		active_scratch_fields(LOGICAL, SET, SET, SET);
	void		label_appearance(COLOUR, LSTYLE, float, LOGICAL);
	void		sample_appearance(FONT, float, COLOUR);
	void		active_field_info(STRING,STRING, STRING,STRING, STRING,STRING);
	void		active_value_info(STRING);
	void		active_wind_info(LOGICAL, STRING);

	/* Functions provided in labels.c */
	LOGICAL		label_add(STRING, STRING, CAL);
	LOGICAL		label_modify(STRING, STRING, CAL);
	LOGICAL		label_move(STRING);
	LOGICAL		label_show(STRING);
	LOGICAL		label_delete(STRING);
	LOGICAL		make_depict_legend(SET, DFLIST *, STRING, COLOUR, float, float);
	LOGICAL		make_legend(SET, STRING, COLOUR, float, float, float);
	LOGICAL		generate_surface_labs(SURFACE, SET, float, float, float, float);
	LOGICAL		generate_areaset_labs(SET, SET, float);
	LOGICAL		generate_curveset_labs(SET, SET, float);
	LOGICAL		generate_spotset_labs(SET, SET, float);
	LOGICAL		recompute_surface_labs(SURFACE, SET, LOGICAL);
	LOGICAL		recompute_areaset_labs(SET, SET, LOGICAL);
	LOGICAL		recompute_curveset_labs(SET, SET, LOGICAL);
	LOGICAL		recompute_spotset_labs(SET, SET, LOGICAL);
	LOGICAL		dependent_winds_affected(DFLIST *, LOGICAL);
	LOGICAL		recompute_dependent_winds(DFLIST *, LOGICAL, int);
	double		convert_wind(double, STRING, LOGICAL *);
	void		check_labels(SET);

	/* Functions provided in samples.c */
	LOGICAL		sample_by_type(STRING, STRING, STRING,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_value(STRING,
							FpaConfigSampleStruct *,
							FpaConfigCrossRefStruct *,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_attribute(STRING, STRING,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_wind(STRING,
							FpaConfigSampleStruct *,
							FpaConfigCrossRefStruct *,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_field_label(STRING,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_lchain_node(STRING,
							STRING, STRING, STRING, STRING, STRING);
	LOGICAL		sample_image(STRING, Image, STRING, STRING);

	/* Functions provided in edit_*.c */
	LOGICAL		edit_ready_spline_field(void);
	LOGICAL		edit_poke_spline(STRING, float);
	LOGICAL		edit_poke_spline_2D(STRING, float, float);
	LOGICAL		edit_stomp_spline(STRING, STRING, float, float);
	LOGICAL		edit_stomp_spline_2D(STRING, STRING, float, float, float);
	LOGICAL		edit_drag_spline(STRING);
	LOGICAL		edit_drag_spline_2D(STRING);
	LOGICAL		edit_block_spline(STRING, STRING);
	LOGICAL		edit_block_spline_2D(STRING, STRING);
	LOGICAL		edit_merge_spline(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		edit_merge_spline_2D(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		edit_smooth_spline(STRING, STRING);
	LOGICAL		edit_smooth_spline_2D(STRING, STRING);
	LOGICAL		edit_grabcont_spline(STRING);
	LOGICAL		edit_ready_area_field(STRING);
	LOGICAL		edit_draw_area(STRING, CAL);
	LOGICAL		edit_addhole_area(STRING, STRING);
	LOGICAL		edit_move_area(STRING, STRING);
	LOGICAL		edit_background_area(STRING, CAL);
	LOGICAL		edit_modify_area(STRING, STRING, CAL);
	LOGICAL		edit_divide_area(STRING, CAL);
	LOGICAL		edit_merge_area(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		extract_area_order_tags(LOGICAL);
	LOGICAL		release_area_order_tags(LOGICAL);
	LOGICAL		edit_ready_line_field(STRING);
	LOGICAL		edit_draw_line(STRING, CAL);
	LOGICAL		edit_flip_line(STRING);
	LOGICAL		edit_move_line(STRING, STRING);
	LOGICAL		edit_modify_line(STRING, CAL);
	LOGICAL		edit_merge_line(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		edit_join_line(STRING);
	LOGICAL		edit_ready_point_field(STRING);
	LOGICAL		edit_draw_point(STRING, STRING, CAL);
	LOGICAL		edit_move_point(STRING, STRING);
	LOGICAL		edit_modify_point(STRING, STRING, CAL);
	LOGICAL		edit_merge_point(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		edit_ready_lchain_field(STRING);
	LOGICAL		edit_add_lchain(STRING, STRING, int, STRING, CAL, CAL);
	LOGICAL		edit_move_lchain(STRING, STRING);
	LOGICAL		edit_modify_lchain(STRING, STRING, int, int, CAL);
	LOGICAL		edit_merge_lchain(STRING, STRING, STRING, STRING, STRING,
							STRING, STRING);
	LOGICAL		edit_nodes_lchain(STRING, STRING, CAL);

	/* Functions provided in link_*.c */
	LOGICAL		link_master(STRING, LOGICAL);
	LOGICAL		mvlink_master(STRING);
	LOGICAL		mrglink_master(STRING, STRING, STRING, STRING, STRING);
	LOGICAL		delink_master(STRING, STRING);
	STRING		master_link_frame_status(DFLIST *, int);
	LOGICAL		verify_master_links(DFLIST *, LOGICAL);
	LOGICAL		ready_master_links(DFLIST *, LOGICAL);
	LOGICAL		link_spline(STRING, LOGICAL);
	LOGICAL		mvlink_spline(STRING);
	LOGICAL		mrglink_spline(STRING, STRING, STRING, STRING, STRING);
	LOGICAL		delink_spline(STRING, STRING);
	STRING		spline_link_frame_status(DFLIST *, int);
	LOGICAL		verify_spline_links(DFLIST *, LOGICAL);
	LOGICAL		ready_spline_links(DFLIST *, LOGICAL);
	LOGICAL		link_area(STRING, LOGICAL);
	LOGICAL		mvlink_area(STRING);
	LOGICAL		mrglink_area(STRING, STRING, STRING, STRING, STRING);
	LOGICAL		delink_area(STRING, STRING);
	LOGICAL		extract_unlinked_areas(void);
	LOGICAL		release_unlinked_areas(void);
	STRING		area_link_frame_status(DFLIST *, int);
	LOGICAL		verify_area_links(DFLIST *, LOGICAL);
	LOGICAL		ready_area_links(DFLIST *, LOGICAL);
	LOGICAL		post_area_link_nodes(DFLIST *, int);
	LOGICAL		reset_area_link_nodes(DFLIST *, int);
	LOGICAL		adjust_area_link_nodes(DFLIST *, int, int, int);
	LOGICAL		link_line(STRING, LOGICAL);
	LOGICAL		mvlink_line(STRING);
	LOGICAL		mrglink_line(STRING, STRING, STRING, STRING, STRING);
	LOGICAL		delink_line(STRING, STRING);
	LOGICAL		extract_unlinked_lines(void);
	LOGICAL		release_unlinked_lines(void);
	STRING		line_link_frame_status(DFLIST *, int);
	LOGICAL		verify_line_links(DFLIST *, LOGICAL);
	LOGICAL		ready_line_links(DFLIST *, LOGICAL);

	/* Functions provided in interp_*.c */
	extern	LOGICAL		interp_spline(DFLIST *, LOGICAL, LOGICAL);
	extern	LOGICAL		interp_spline_2D(DFLIST *, LOGICAL, LOGICAL);
	extern	LOGICAL		interp_area(DFLIST *, LOGICAL, LOGICAL);
	extern	LOGICAL		interp_line(DFLIST *, LOGICAL, LOGICAL);

	/* Functions provided in util.c */
	extern	LOGICAL		trunc_line(LINE, float, LOGICAL, LOGICAL);
	extern	LINE		smooth_line(LINE, float, float);
	extern	LINE		smjoin_lines(LINE, LINE, float, float);
	extern	LINE		smclose_line(LINE, float, float);
	extern	LINE		bridge_lines(LINE, LINE, float, float);
	extern	LOGICAL		spot_list_translate(int, SPOT *, float, float);
	extern	LOGICAL		spot_list_rotate(int, SPOT *, POINT, float);

	/* External functions from other FPA libraries */
	/* These should all be available in appropriate header files */
	STRING	XuGetMdbLine(STRING, STRING);
	LOGICAL	XuGetBooleanResource(STRING, LOGICAL);
	void	XuFree(void *);


/* Prevent from being included again */
#define INGRED
#endif
