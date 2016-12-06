/***********************************************************************
*                                                                      *
*    r g r i b _ e d i t i o n 2 . c                                   *
*                                                                      *
*    Routines to decode GRIB Edition 2 format files                    *
*                                                                      *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

#define RGRIBED2_MAIN	/* To initialize defined constants and internal */
						/*  structures in rgrib_edition2.h  file        */

#include <grib2.h>
#include "rgrib_edition2.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

#undef DEBUG

#ifdef DEBUG_DECODE
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_DECODE */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                        */
/*  ... these are defined in rgrib_edition2.h */

	/* Internal static function (Identifier Translation) */
static LOGICAL	E2_extract_grib( void );
static LOGICAL	E2_grib_models( STRING );
static LOGICAL	E2_grib_tstamps( STRING, STRING, STRING );
static LOGICAL	E2_grib_elements( const STRING, STRING, STRING );
static LOGICAL	E2_grib_levels( const STRING, STRING );
static LOGICAL	E2_grib_data_mapproj( MAP_PROJ *, int *, LOGICAL *, LOGICAL *);
static LOGICAL  E2_grib_bit_map ( GRID_DEF, LOGICAL **);
static	unsigned long	E2_extract_packed_datum ( long,	short, unsigned char *);
static LOGICAL  interpret_scan_mode( LOGICAL *, LOGICAL *, LOGICAL *, LOGICAL *);

/* Open Gribfile Edition 2 */ 
static FILE		*GribFile		= NullPtr(FILE *);
static long int GribPosition	= 0;
static long int GribPositionErr	= 0;
static long int GribFieldNumber = 0;

/* Internal GRIB field buffer */
static LOGICAL		GribDecoded	= FALSE;
static gribfield	*GribFld = NullPtr(gribfield *);
static DECODEDFIELD DecodedFld; 

/* Internal GRIB identifier buffers */
static LOGICAL	GribValid					= FALSE;
static char		GribModel[GRIB_LABEL_LEN]	= "";
static char		GribRTime[GRIB_LABEL_LEN]	= "";
static char		GribVTimeb[GRIB_LABEL_LEN]	= "";
static char		GribVTimee[GRIB_LABEL_LEN]	= "";
static char		GribElement[GRIB_LABEL_LEN]	= "";
static char		GribLevel[GRIB_LABEL_LEN]	= "";
static char		GribUnits[GRIB_LABEL_LEN]	= "";
/* Internal GRIB data buffers */
static MAP_PROJ MapProj;
static float    *DataGrid;
static LOGICAL  *DataBitmap;
static int      ComponentFlag;

unsigned char   *cgrib	  = NullPtr(unsigned char *);
static g2int	listsec0[3];
static g2int	listsec1[13];
static g2int	numlocal  = 0;
static g2int	numfields = 0;

/***********************************************************************
*                                                                      *
*    o p e n _ g r i b f i l e _ e d i t i o n 2                       *
*    n e x t _ g r i b f i e l d _ e d i t i o n 2                     *
*    g r i b f i e l d _ i d e n t i f i e r s _ e d i t i o n 2       *
*    c l o s e _ g r i b f i l e _ e d i t i o n 2                     *
*                                                                      *
*    open_gribfile_edition2()                                          *
*    next_gribfield_edition2()                                         *
*    gribfield_identifiers_edition2()                                  *
*    close_gribfile_edition2()                                         *
*                                                                      *
***********************************************************************/
LOGICAL open_gribfile_edition2
	(
 	STRING	name	/* GRIB filename */
	)

	{
	/* There will be no local GRIBFIELD upon opening */
	GribDecoded	= FALSE;
	GribValid   = FALSE;

	/* If there already is an open GRIB file close it now */
	(void) close_gribfile_edition2();
	
	/* Do nothing if GRIB file name not given */
	if ( blank(name) )
		{
		dprintf(stderr, "[open_gribfile_edition2] GRIB file name not given\n");
		return FALSE;
		}
	/* See if the GRIB file exists */
	if ( !find_file(name) )
		{
		dprintf(stderr, "[open_gribfile_edition2] GRIB file not found: %s\n",
					   name);
		return FALSE;
		}

	/* Open the GRIB file */
	GribFile = fopen(name, "r");
	if ( IsNull(GribFile) )
		{
		dprintf(stderr, "[open_gribfile_edition2] GRIB file unreadable: %s\n",
					   name);
		return FALSE;
		}

	/* Set position to start of file and return TRUE if all was OK */
	GribPosition = 0;
	GribPositionErr = 0;
	GribFieldNumber = 0;
	return TRUE;
	}

/* Next Grib Message (helper to next_gribfield_edition2) */
LOGICAL		next_gribmessage
	(
	)
	{
	long	lskip, lgrib;
	int		ierr;

	while (1)
		{
		seekgb(GribFile, GribPosition, 32000, &lskip, &lgrib);
		if ( 0 == lgrib) return GribDecoded; /* End of file or problem */

		/* Get enough space for the entire message */
		if ( IsNull(cgrib) || sizeof(cgrib)/sizeof(unsigned char) < lgrib )
			cgrib = GETMEM(cgrib, unsigned char, lgrib);

		/* Skip to the begining of the message */
		if ( fseek(GribFile, lskip, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition2();
			return GribDecoded;
			}

		/* Read in a grib message */
		(void) fread(cgrib, sizeof(unsigned char), (size_t)lgrib, GribFile);

		/* Reset file, point to the end of the current grib message */
		GribPosition = lskip + lgrib;
		/* If we encounter an error only andvance past the GRIB marker */
		GribPositionErr = lskip + 4;

		/* Look at message header */
		ierr = g2_info(cgrib, listsec0, listsec1, &numfields, &numlocal);
		if ( ierr != 0 )
			{
			(void) pr_error("[next_gribmessage_edition2]", "%s\n", g2_infoErrors[ierr]);
			GribPosition = GribPositionErr;
			continue; 
			}
		else break;
		}
	/* Reset to first field in message */
	GribFieldNumber = 1;

	return TRUE;
	}

LOGICAL		next_gribfield_edition2
	(
 	DECODEDFIELD	**ffld /* pointer to local DECODEDFIELD object */
	)
	{
	int	ierr, unpack=1, expand=1;
	
	/* Set default for no local GRIBFIELD */
	GribDecoded	= FALSE;
	*ffld       = NullPtr(DECODEDFIELD *);
	
	/* Keep getting the next field until you get a field without errors */
	while(1)
		{
		/* Free up GribFld in case it's already in use */
		if ( NotNull(GribFld) ) 
			{
			g2_free(GribFld);
			GribFld = NullPtr(gribfield *);
			}
	
		/* If there are no more fields in current message, get the next message */
		if ( (GribFieldNumber > numfields) || GribFieldNumber < 1 ) 
			if ( !next_gribmessage( ) ) return GribDecoded;
	
		/* Return now if no current GRIB message */
		if ( IsNull(cgrib) )	return GribDecoded;
	
		/* Extract next field and increment counter */
		ierr = g2_getfld(cgrib, GribFieldNumber++, unpack, expand, &GribFld);
		if ( ierr != 0 )
			{
			(void) pr_error("[next_gribfield_edition2]", "%s\n", g2_getfldErrors[ierr]);
			GribPosition = GribPositionErr;
			continue;
			}
	
		/* Extract GRIB info into DECODEDFIELD object */
		GribDecoded = TRUE;
		(void) E2_extract_grib();
		*ffld = &DecodedFld;
		return GribDecoded;
		}
	}

/* Grib Field Identifiers */
LOGICAL		gribfield_identifiers_edition2
	(
 	STRING		*model,		/* string containing model label */
 	STRING      *rtime,		/* ... run timestamp             */
 	STRING		*vtimeb,	/* ... begin valid timestamp     */
 	STRING		*vtimee,	/* ... end valid timestamp       */
 	STRING		*element,	/* ... field element label       */
 	STRING		*level,		/* ... field level label         */
 	STRING		*units		/* ... field units label         */
	)
	{
	if (!GribDecoded ) return GribDecoded;
	/* Set defaults for GRIB identifiers */
	if ( NotNull(model) )	*model   = NullString;
	if ( NotNull(rtime) )	*rtime   = NullString;
	if ( NotNull(vtimeb) )	*vtimeb  = NullString;
	if ( NotNull(vtimee) )	*vtimee  = NullString;
	if ( NotNull(element) )	*element = NullString;
	if ( NotNull(level) )	*level   = NullString;
	if ( NotNull(units) )	*units   = NullString;

	if ( NotNull(model) )	*model	 = DecodedFld.model;
	if ( NotNull(rtime) )	*rtime	 = DecodedFld.rtime;
	if ( NotNull(vtimeb) )	*vtimeb	 = DecodedFld.vtimeb;
	if ( NotNull(vtimee) )	*vtimee	 = DecodedFld.vtimee;
	if ( NotNull(element) )	*element = DecodedFld.element;
	if ( NotNull(level) )	*level	 = DecodedFld.level;
	if ( NotNull(units) )	*units	 = DecodedFld.units;
	return GribValid;
	}


/* Close Grib File */
void	close_gribfile_edition2
	(
	)
	{
	/* If there already is an open GRIB file, close it */
	if (NotNull(GribFile) )
		{
		(void) fclose(GribFile);
		GribFile = NullPtr(FILE *);
		}

	/* If there is a open Grib Message, free it now */
	if ( NotNull(cgrib) )
		{
		FREEMEM(cgrib);
		cgrib	= NullPtr(unsigned char *);
		}

	/* If there is an open Grib Field, free it now */
	if ( NotNull(GribFld) ) 
		{
		g2_free(GribFld);
		GribFld = NullPtr(gribfield *);
		}

	/* Set position to start of file */
	GribPosition = 0;
	GribFieldNumber = 0;
	}

/***********************************************************************
*                                                                      *
*    p r i n t _ b l o c k 0 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 1 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 2 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 3 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 4 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 5 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 6 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 7 _ e d i t i o n 2                         *
*                                                                      *
*    Print out the relevant contents of the gribfield for gribtest.    *
*                                                                      *
*    Block 0 - Indicator                                               *
*    Block 1 - Identifier                                              *
*    Block 2 - Local Use                                               *
*    Block 3 - Grid Definition                                         *
*    Block 4 - Product Definition                                      *
*    Block 5 - Data Representation                                     *
*    Block 6 - Bit-map                                                 *
*    Block 7 - Data                                                    *
*                                                                      *
***********************************************************************/
void print_block0_edition2
	(
	)
	{
	(void) fprintf(stdout, "\n   Block 0: Indicator Section\n");
	(void) fprintf(stdout, "     Total length      = %d\n", listsec0[2]);
	(void) fprintf(stdout, "     Edition number    = %d\n", listsec0[1]);
	(void) fprintf(stdout, "     Discipline number = %d\n", listsec0[0]);
	}

void print_block1_edition2
	(
	)
	{
	(void) fprintf(stdout, "\n   Block 1: Identification Section\n");
	(void) fprintf(stdout, "     IDS length        = %d\n", GribFld->idsectlen);
	(void) fprintf(stdout, "     IDS center        = %d\n", GribFld->idsect[0]);
	(void) fprintf(stdout, "     IDS sub-centre    = %d\n", GribFld->idsect[1]);
	(void) fprintf(stdout, "     IDS master tbl #  = %d\n", GribFld->idsect[2]);
	(void) fprintf(stdout, "     IDS local tbl #   = %d\n", GribFld->idsect[3]);

	switch ( GribFld->idsect[4] )
		{
		case 0: (void) fprintf(stdout, "     IDS ref type      = Analysis\n");
				break;
		case 1: (void) fprintf(stdout, "     IDS ref type      = Start of forecast\n");
				break;
		case 2: (void) fprintf(stdout, 
					       "     IDS ref type      = Verifying time of forecast\n");
				break;
		case 3: (void) fprintf(stdout, "     IDS ref type      = Observation time\n");
				break;
		}

	(void) fprintf(stdout, "     IDS ref time      = %04d/%02d/%02d %02d:%02d:%02d\n",
			GribFld->idsect[5], GribFld->idsect[6], GribFld->idsect[7],
			GribFld->idsect[8], GribFld->idsect[9], GribFld->idsect[10]);

	switch ( GribFld->idsect[11] )
		{
		case 0: (void) fprintf(stdout, "     IDS prod status   = Operational\n");
				break;
		case 1: (void) fprintf(stdout, "     IDS prod status   = Operational test\n");
				break;
		case 2: (void) fprintf(stdout, "     IDS prod status   = Research\n");
				break;
		case 3: (void) fprintf(stdout, "     IDS prod status   = Re-analysis\n");
				break;
		}
	switch ( GribFld->idsect[12] )
		{
		case 0: (void) fprintf(stdout, "     IDS data type     = Analysis\n");
				break;
		case 1: (void) fprintf(stdout, "     IDS data type     = Forecast\n");
				break;
		case 2: (void) fprintf(stdout, "     IDS data type     = Analysis and forecast\n");
				break;
		case 3: (void) fprintf(stdout, "     IDS data type     = Control forecast\n");
				break;
		case 4: (void) fprintf(stdout, "     IDS data type     = Perturbed forecast\n");
				break;
		case 5: (void) fprintf(stdout, 
						   "     IDS data type     = Control and perturbed forecast\n");
				break;
		case 6: (void) fprintf(stdout, 
						   "     IDS data type     = Processed satellite observations\n");
				break;
		case 7: (void) fprintf(stdout, 
						   "     IDS data type     = Processed radar observations\n");
				break;
		}
	}

/* I need to re-visit this function, the only example I've tried does not look right */
void print_block2_edition2
	(
	)
	{
	short           bits_per_val;
	long            bit_cursor;
	int				num_of_words, ii, count;
	unsigned char	*binary_ip, packed_datum;

	(void) fprintf(stdout, "\n   Block 2: Local Use Section\n");
	if (GribFld->locallen == 0)
		(void) fprintf(stdout, "     LUS local use     = none\n");
	else	/* This works okay for NDFD wx but I'm not sure how I can generalize it. */
		{
		bits_per_val = 7;
		num_of_words = ((GribFld->locallen-15)*8)/bits_per_val;
		(void) fprintf(stdout, "     LUS length        = %d\n", num_of_words);
		(void) fprintf(stdout, "     LUS bits per val  = %d\n", bits_per_val);
		(void) fprintf(stdout, "     LUS extra chars   = ");
		for (ii=0;ii<15; ii++) (void)fprintf(stdout, " %d", GribFld->local[ii]); 
		(void) fprintf(stdout, "\n");
		(void) fprintf(stdout, "     LUS local use     =\n		");

		binary_ip = GribFld->local+15;
		count = 0;
		bit_cursor   = 0; 
		for ( ii = 0; ii<num_of_words; ii++, bit_cursor+=bits_per_val)
			{
			packed_datum = (unsigned char)E2_extract_packed_datum(bit_cursor, bits_per_val, binary_ip);
			if ( packed_datum != '\0' )
				(void) fprintf(stdout, "%c", packed_datum);
			else
				(void) fprintf(stdout, "\n		");
			}
		}
	}

void print_block3_edition2
	(
	)
	{
	float units;
	LOGICAL west, north, hsweep, rsweep;

	(void) fprintf(stdout, "\n   Block 3: Grid Definition Section\n");
	(void) fprintf(stdout, "     GDS length        = %d\n", GribFld->igdtlen);
	(void) fprintf(stdout, "     GDS type          = %d\n", GribFld->igdtnum);

	if (!interpret_scan_mode(&west, &north, &hsweep, &rsweep)) return;

	switch(GribFld->igdtnum)
		{
		case GT_LATLON:
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dj            = %.3f\n",
						   GribFld->igdtmpl[17]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			break;
		case GT_ROTATED_LATLON:
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dj            = %.3f\n",
						   GribFld->igdtmpl[17]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			(void) fprintf(stdout, "     GDS  lat pole     = %.3f\n",
						   (GribFld->igdtmpl[19])/GribToDegrees); 
			(void) fprintf(stdout, "     GDS  lon pole     = %.3f\n",
						   (GribFld->igdtmpl[20])/GribToDegrees);
			(void) fprintf(stdout, "     GDS  rotation     = %d\n", (GribFld->igdtmpl[21])); 
			break;
		case GT_STRETCHED_LATLON:
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dj            = %.3f\n",
						   GribFld->igdtmpl[17]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			(void) fprintf(stdout, "          lat pole     = %.3f\n",
						   (GribFld->igdtmpl[19])/GribToDegrees); 
			(void) fprintf(stdout, "          lon pole     = %.3f\n",
						   (GribFld->igdtmpl[20])/GribToDegrees);
			(void) fprintf(stdout, "          stretch      = %d\n", (GribFld->igdtmpl[21])); 
			break;
		case GT_MERCATOR:
			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[9]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[10]/GribToDegrees);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[11]);
			(void) fprintf(stdout, "     GDS LaD           = %.3f\n",
						   GribFld->igdtmpl[12]/GribToDegrees);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[13]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[14]/GribToDegrees);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[15]);
			(void) fprintf(stdout, "     GDS orientation   = %d\n", GribFld->igdtmpl[16]);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[17]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dj            = %.3f\n",
						   GribFld->igdtmpl[18]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", MetersPerUnit);
			break;
		case GT_PSTEREO:
			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Nx            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Ny            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[9]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[10]/GribToDegrees);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[11]);
			(void) fprintf(stdout, "     GDS LaD           = %.3f\n",
						   GribFld->igdtmpl[12]/GribToDegrees);
			(void) fprintf(stdout, "     GDS LoV           = %.3f\n",
						   GribFld->igdtmpl[13]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Dx            = %.3f\n",
						   GribFld->igdtmpl[14]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dy            = %.3f\n",
						   GribFld->igdtmpl[15]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", MetersPerUnit);
			(void) fprintf(stdout, "     GDS proj centre   = %d\n", GribFld->igdtmpl[16]);
			(void) fprintf(stdout, "          (south pole)  = %d\n",  
						   GETBIT(GribFld->igdtmpl[16], E2_pole_centre_south));
			(void) fprintf(stdout, "          (bi-polar)    = %d\n",  
						   GETBIT(GribFld->igdtmpl[16], E2_pole_centre_bipolar));
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[17]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			break;
		case GT_LAMBERT:
			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Nx            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Ny            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[9]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[10]/GribToDegrees);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[11]);
			(void) fprintf(stdout, "     GDS LaD           = %.3f\n",
						   GribFld->igdtmpl[12]/GribToDegrees);
			(void) fprintf(stdout, "     GDS LoV           = %.3f\n",
						   GribFld->igdtmpl[13]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Dx            = %.3f\n",
						   GribFld->igdtmpl[14]/GribToKMeters);
			(void) fprintf(stdout, "     GDS Dy            = %.3f\n",
						   GribFld->igdtmpl[15]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", MetersPerUnit);
			(void) fprintf(stdout, "     GDS proj centre   = %d\n", GribFld->igdtmpl[16]);
			(void) fprintf(stdout, "          (south pole)  = %d\n",  
						   GETBIT(GribFld->igdtmpl[16], E2_pole_centre_south));
			(void) fprintf(stdout, "          (bi-polar)    = %d\n",  
						   GETBIT(GribFld->igdtmpl[16], E2_pole_centre_bipolar));
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[17]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			(void) fprintf(stdout, "     GDS Latin 1       = %.3f\n",
						   GribFld->igdtmpl[18]/GribToDegrees);
			(void) fprintf(stdout, "     GDS Latin 2       = %.3f\n",
						   GribFld->igdtmpl[19]/GribToDegrees);
			(void) fprintf(stdout, "     GDS LaP proj      = %.3f\n",
						   GribFld->igdtmpl[20]/GribToDegrees);
			(void) fprintf(stdout, "     GDS LoP proj      = %.3f\n",
						   GribFld->igdtmpl[21]/GribToDegrees);
			break;
		case GT_GAUSS:
			(void) fprintf(stdout, "\n   Unsupported grid definition: Gaussian\n");
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS N parallels   = %d\n", GribFld->igdtmpl[17]);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			break;
		case GT_ROTATED_GAUSS:
			(void) fprintf(stdout, "\n   Unsupported grid definition: Rotated Gaussian\n");
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			units = ((float)GribFld->igdtmpl[9])/(GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS N parallels   = %d\n", GribFld->igdtmpl[17]);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			(void) fprintf(stdout, "     GDS laP proj      = %.3f\n",
						   GribFld->igdtmpl[19]/GribToDegrees);
			(void) fprintf(stdout, "     GDS loP proj      = %.3f\n",
						   GribFld->igdtmpl[20]/GribToDegrees);
			(void) fprintf(stdout, "     GDS rotation      = %d\n", GribFld->igdtmpl[21]);
			break;
		case GT_STRETCHED_GAUSS:
			/* The ratio of the basic angle and the subdivisions number */
			(void) fprintf(stdout, "\n   Unsupported grid definition: Stretched Gaussian\n");
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			(void) fprintf(stdout, "     GDS ShapeOfEarth  = %d\n", GribFld->igdtmpl[0]);
			(void) fprintf(stdout, "     GDS  Spherical\n");
			(void) fprintf(stdout, "           ScaleFactor = %d\n", GribFld->igdtmpl[1]);
			(void) fprintf(stdout, "           ScaleValue  = %d\n", GribFld->igdtmpl[2]);
			(void) fprintf(stdout, "     GDS  OblateSphereoid\n");
			(void) fprintf(stdout, "           MajorFactor = %d\n", GribFld->igdtmpl[3]);
			(void) fprintf(stdout, "           MajorValue  = %d\n", GribFld->igdtmpl[4]);
			(void) fprintf(stdout, "           MinorFactor = %d\n", GribFld->igdtmpl[5]);
			(void) fprintf(stdout, "           MinorValue  = %d\n", GribFld->igdtmpl[6]);

			(void) fprintf(stdout, "     GDS Ni            = %d\n", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "     GDS Nj            = %d\n", GribFld->igdtmpl[8]);
			units = ((float)GribFld->igdtmpl[9])/(GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS basic angle   = %d\n", GribFld->igdtmpl[9]);
			(void) fprintf(stdout, "     GDS angle subdiv  = %d\n", GribFld->igdtmpl[10]);
			(void) fprintf(stdout, "     GDS La1           = %.3f\n",
						   GribFld->igdtmpl[11]/units);
			(void) fprintf(stdout, "     GDS Lo1           = %.3f\n",
						   GribFld->igdtmpl[12]/units);
			(void) fprintf(stdout, "     GDS compnt        = %d\n", GribFld->igdtmpl[13]);
			(void) fprintf(stdout, "     GDS La2           = %.3f\n",
						   GribFld->igdtmpl[14]/units);
			(void) fprintf(stdout, "     GDS Lo2           = %.3f\n",
						   GribFld->igdtmpl[15]/units);
			(void) fprintf(stdout, "     GDS Di            = %.3f\n",
						   GribFld->igdtmpl[16]/GribToKMeters);
			(void) fprintf(stdout, "     GDS units         = %.3f\n", 1.0);
			(void) fprintf(stdout, "     GDS N parallels   = %d\n", GribFld->igdtmpl[17]);
			(void) fprintf(stdout, "     GDS scan mode     = %d\n", GribFld->igdtmpl[18]);
			(void) fprintf(stdout, "          (negative x)  = %d\n", west);
			(void) fprintf(stdout, "          (positive y)  = %d\n", north);
			(void) fprintf(stdout, "          (by column)   = %d\n", hsweep);
			(void) fprintf(stdout, "          (rev sweep)   = %d\n", rsweep);
			(void) fprintf(stdout, "     GDS laP proj      = %3.f\n",
						   GribFld->igdtmpl[19]/GribToDegrees);
			(void) fprintf(stdout, "     GDS loP proj      = %3.f\n",
						   GribFld->igdtmpl[20]/GribToDegrees);
			(void) fprintf(stdout, "     GDS stretch       = %d\n", GribFld->igdtmpl[21]);
			break;
		default:
			(void) fprintf(stdout, "\n   Unsupported grid definition\n");
		}
	}

void print_block4_edition2
	(
	)
	{
	(void) fprintf(stdout, "\n   Block 4: Product Definition Section\n");
	(void) fprintf(stdout, "     PDS length        = %d\n", GribFld->ipdtlen);
	(void) fprintf(stdout, "     PDS type          = %d\n", GribFld->ipdtnum);
	switch ( GribFld->ipdtnum )
		{
		case 0:
			(void) fprintf(stdout, "     PDS category      = %d\n", GribFld->ipdtmpl[0]);
			(void) fprintf(stdout, "     PDS parameter     = %d\n", GribFld->ipdtmpl[1]);
			(void) fprintf(stdout, "     PDS gen type      = %d\n", GribFld->ipdtmpl[2]);
			(void) fprintf(stdout, "     PDS bkg process   = %d\n", GribFld->ipdtmpl[3]);
			(void) fprintf(stdout, "     PDS gen process   = %d\n", GribFld->ipdtmpl[4]);
			(void) fprintf(stdout, "     PDS time type     = %d\n", GribFld->ipdtmpl[7]);
			(void) fprintf(stdout, "     PDS fcst time     = %d\n", GribFld->ipdtmpl[8]);
			(void) fprintf(stdout, "     PDS sfc1 type     = %d\n", GribFld->ipdtmpl[9]);
			(void) fprintf(stdout, "     PDS sfc1 scale    = %d\n", GribFld->ipdtmpl[10]);
			(void) fprintf(stdout, "     PDS sfc1 value    = %d\n", GribFld->ipdtmpl[11]);
			(void) fprintf(stdout, "     PDS sfc2 type     = %d\n", GribFld->ipdtmpl[12]);
			(void) fprintf(stdout, "     PDS sfc2 scale    = %d\n", GribFld->ipdtmpl[13]);
			(void) fprintf(stdout, "     PDS sfc2 value    = %d\n", GribFld->ipdtmpl[14]);
			break;
		case 8:
			(void) fprintf(stdout, "     PDS category      = %d\n", GribFld->ipdtmpl[0]);
			(void) fprintf(stdout, "     PDS parameter     = %d\n", GribFld->ipdtmpl[1]);
			(void) fprintf(stdout, "     PDS gen type      = %d\n", GribFld->ipdtmpl[2]);
			(void) fprintf(stdout, "     PDS bkg process   = %d\n", GribFld->ipdtmpl[3]);
			(void) fprintf(stdout, "     PDS gen process   = %d\n", GribFld->ipdtmpl[4]);
			(void) fprintf(stdout, "     PDS time type     = %d\n", GribFld->ipdtmpl[7]);
			(void) fprintf(stdout, "     PDS fcst time     = %d\n", GribFld->ipdtmpl[8]);
			(void) fprintf(stdout, "     PDS sfc1 type     = %d\n", GribFld->ipdtmpl[9]);
			(void) fprintf(stdout, "     PDS sfc1 scale    = %d\n", GribFld->ipdtmpl[10]);
			(void) fprintf(stdout, "     PDS sfc1 value    = %d\n", GribFld->ipdtmpl[11]);
			(void) fprintf(stdout, "     PDS sfc2 type     = %d\n", GribFld->ipdtmpl[12]);
			(void) fprintf(stdout, "     PDS sfc2 scale    = %d\n", GribFld->ipdtmpl[13]);
			(void) fprintf(stdout, "     PDS sfc2 value    = %d\n", GribFld->ipdtmpl[14]);
			(void) fprintf(stdout, "     PDS end time      = %04d/%02d/%02d %02d:%02d:%02d\n",
					  GribFld->ipdtmpl[15], GribFld->ipdtmpl[16], GribFld->ipdtmpl[17],
					  GribFld->ipdtmpl[18], GribFld->ipdtmpl[19], GribFld->ipdtmpl[20]);
			break;
		default:
			(void) fprintf(stdout, "\n   Unsupported product definition\n");
		}
	}

void print_block5_edition2
	(
	)
	{
	float value;
	rdieee(GribFld->idrtmpl, &value, 1);
	(void) fprintf(stdout, "\n   Block 5: Data Representation Section\n");
	(void) fprintf(stdout, "     DRS length        = %d\n", GribFld->idrtlen);
	(void) fprintf(stdout, "     DRS reference val = %.3f\n", value);
	(void) fprintf(stdout, "     DRS binary scale  = %d\n", GribFld->idrtmpl[1]);
	(void) fprintf(stdout, "     DRS decimal scale = %d\n", GribFld->idrtmpl[2]);
	(void) fprintf(stdout, "     DRS bits per val  = %d\n", GribFld->idrtmpl[3]);
	if	(GribFld->idrtmpl[4] == 0)
		(void) fprintf(stdout, "     DRS field type    = floating point\n");
	else
		(void) fprintf(stdout, "     DRS field type    = integer\n");
	
	(void) fprintf(stdout, "     DRS type          = %d:", GribFld->idrtnum);
	switch (GribFld->idrtnum)
		{
		case 0: (void) fprintf(stdout, " Grid point data - simple packing\n"); break;
		case 1: (void) fprintf(stdout, " Matrix value - simple packing\n"); break;
		case 2: (void) fprintf(stdout, " Grid point data - complex packing\n"); break;
		case 3: (void) fprintf(stdout,
					" Grid point data - complex packing and spatial differencing\n"); break;
		case 40000:
		case 40: (void) fprintf(stdout,
					" Grid point data - JPEG 2000 Code Stream Format\n"); break;
		case 40010:
		case 41: (void) fprintf(stdout, 
					" Grid point data - Portable Network Graphics (PNG)\n"); break;
		case 50: (void) fprintf(stdout, " Spectral data - simple packing\n"); break;
		case 51: (void) fprintf(stdout, " Spherical harmonics data - complex packing\n"); break;
		}
	}

void print_block6_edition2
	(
	)
	{
	g2int 			ni, nj, ii, jj;
	LOGICAL 		hsweep;
	LOGICAL			*xbits, xbit;


	(void) fprintf(stdout,"\n   Block 6: Bit-map Section\n");
	if (!interpret_scan_mode(NULL, NULL, &hsweep, NULL)) return;

	ni = GribFld->igdtmpl[7];
	nj = GribFld->igdtmpl[8];
	if ( NotNull(DecodedFld.bmap) ) 
		{
		xbits = DecodedFld.bmap;
		for (jj = 0; jj < nj; jj++)
			{
			(void) fprintf(stdout, "\n");
			for (ii = 0; ii < ni; ii++)
				{
				if ( hsweep == 0 ) xbit = xbits[jj*ni + ii];
				else               xbit = xbits[ii*nj + jj];

				if ( xbit ) (void) fprintf(stdout, "B");
				else        (void) fprintf(stdout, ".");
				}
			}
		(void) fprintf(stdout, "\n");
		}
	else if ( 255 == GribFld->ibmap ) (void) fprintf(stdout,"     BMS none\n");
	else (void) fprintf(stdout,"    BMS: predefined\n");
	}

void print_block7_edition2
	(
	)
	{
	g2int 			ni, nj, ii, jj, count=0, MaxCount=8;
	LOGICAL 		hsweep;
	g2float			*gdata, gdatum;


	(void) fprintf(stdout,"\n   Block 7: Data Section\n");
	if (!interpret_scan_mode(NULL, NULL, &hsweep, NULL)) return;
	ni = GribFld->igdtmpl[7];
	nj = GribFld->igdtmpl[8];

	(void) fprintf(stdout, "\n   Processed GRIB data -");
	switch(GribFld->igdtnum)
		{
		case GT_LATLON:
		case GT_ROTATED_LATLON:
		case GT_STRETCHED_LATLON:
		case GT_MERCATOR:
		case GT_GAUSS:
		case GT_ROTATED_GAUSS:
		case GT_STRETCHED_GAUSS:
			(void) fprintf(stdout, "  %d Longitudes for each of", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "  %d Latitudes", GribFld->igdtmpl[8]);
			break;
		case GT_PSTEREO:
		case GT_LAMBERT:
			(void) fprintf(stdout, "  %d Columns for each of", GribFld->igdtmpl[7]);
			(void) fprintf(stdout, "  %d Rows", GribFld->igdtmpl[8]);
			break;
		}

	gdata = GribFld->fld;
	(void) fprintf(stdout, "\n! ");
	for (jj = 0; jj < nj; jj++)
		{
		for (ii = 0; ii < ni; ii++)
			{
			if ( hsweep == 0 ) gdatum = gdata[jj*ni + ii];
			else               gdatum = gdata[ii*nj + jj];

			if ( ++count > MaxCount )
				{
				count = 1;
				(void) fprintf(stdout, "\n  ");
				}
			(void) fprintf(stdout, "%10.2f ", gdatum);
			}
		count = 0;
		if ( jj < nj-1 ) (void) fprintf(stdout, "\n! ");
		}
	(void) fprintf(stdout, "\n");
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Identifier Translation)                 *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/
/* e x t r a c t _ g r i b _ e d i t i o n 2  */
static LOGICAL		E2_extract_grib
	(
	)
	{

	LOGICAL	left, bottom, west, north, isweep, rsweep;
	/* Set defaults for GRIB identifiers */
	GribValid		= FALSE;

	strcpy(GribModel, "");
	strcpy(GribRTime, "");
	strcpy(GribVTimeb, "");
	strcpy(GribVTimee, "");
	strcpy(GribElement, "");
	strcpy(GribUnits, "");
	strcpy(GribLevel, "");

	/* Set model label from originating center and model */
	if ( !E2_grib_models(GribModel) ) return GribValid;

	/* Set run and valid timestamps from date and time information */
	if ( !E2_grib_tstamps(GribRTime, GribVTimeb, GribVTimee) ) return GribValid;

	/* Set element and units labels from element code */
	if ( !E2_grib_elements(GribModel, GribElement, GribUnits) ) return GribValid;

	/* Set level label from level code and values */
	if ( !E2_grib_levels(GribModel, GribLevel) ) return GribValid;

	/* Set projection, map definition and grid definition */
	if ( !E2_grib_data_mapproj(&MapProj, &ComponentFlag, &left, &bottom) ) return GribValid;

	/* Set data grid */
	if ( IsNull(DataGrid = GribFld->fld) ) return GribValid;

	/* Set bit map */
	if ( !E2_grib_bit_map(MapProj.grid, &DataBitmap)) return GribValid;

	/* Set scan mode bits */
	if ( !interpret_scan_mode(&west, &north, &isweep, &rsweep) ) return GribValid;

	/* Set GribValid to TRUE and return decoded field */
	/*	if entire field was correctly translated	  */
	GribValid			      = TRUE;
	DecodedFld.model	      = GribModel;
	DecodedFld.rtime	      = GribRTime;
	DecodedFld.vtimeb	      = GribVTimeb;
	DecodedFld.vtimee	      = GribVTimee;
	DecodedFld.element	      = GribElement;
	DecodedFld.units	      = GribUnits;
	DecodedFld.level	      = GribLevel;
	DecodedFld.mproj_orig	  = DecodedFld.mproj = &MapProj;
	DecodedFld.data_orig	  = DecodedFld.data	 = DataGrid;
	DecodedFld.projection     = GribFld->igdtnum;
	DecodedFld.bmap	          = DataBitmap;
	DecodedFld.component_flag = ComponentFlag;
	DecodedFld.west			  = west;
	DecodedFld.north		  = north;
	DecodedFld.left			  = left;
	DecodedFld.bottom		  = bottom;
	DecodedFld.isweep		  = isweep;
	DecodedFld.rsweep		  = rsweep;

	/* Set flags for data processing */
	DecodedFld.filled         = FALSE;
	DecodedFld.reordered      = FALSE;
	DecodedFld.wrapped        = FALSE;

	return GribValid;
	}

/*********************************************************************
***                                                                ***
*** E 2 _ g r i b _ m o d e l s                                    ***
*** E 2 _ g r i b _ t s t a m p s                                  ***
*** E 2 _ g r i b _ e l e m e n t s                                ***
*** E 2 _ g r i b _ l e v e l s                                    ***
***                                                                ***
*** Identify model label, run and valid timestamps, element/units  ***
*** labels, or level label from GRIB product definition data.      ***
***                                                                ***
*** These routines are based on Tables in the 2001 GRIB Edition 2  ***
***                                                                ***
*** The  model   definitions are from                              ***
***		http://www.nco.ncep.noaa.gov/pmb/docs/on388/table0.html    ***
*** and	http://www.nco.ncep.noaa.gov/pmb/docs/on388/tablea.html    ***
*** The  tstamp  definitions are from Table 4.4.                   ***
*** The  element/units  definitions are from Table 4.2.            ***
*** The  level  defintions are from Table 4.5.                     ***
***                                                                ***
**********************************************************************/
static LOGICAL	E2_grib_models
	(
 	STRING model
	)
	{
	int				num, numc, nums, numt, numy, nump, numb;
	static char		did[GRIB_LABEL_LEN];
	static char		cid[GRIB_LABEL_LEN];
	static char     sid[GRIB_LABEL_LEN];
	static char     tid[GRIB_LABEL_LEN];
	static char     yid[GRIB_LABEL_LEN];
	static char     pid[GRIB_LABEL_LEN];
	static char     bid[GRIB_LABEL_LEN];
	/* Internal checks for centre and element ids */
	int					nn;
	static int			new=0;
	static short int	*newc=NullShort, *news=NullShort;
	static short int	*newt=NullShort, *newy=NullShort;
	static short int	*newp=NullShort, *newb=NullShort;
	
	/* Parameter list */
	int id[6];
	id[0] =	GribFld->idsect[0];		/* centre          */
	id[1] =	GribFld->idsect[1];		/* sub_centre      */
	id[2] =	GribFld->ipdtnum;		/* template        */
	id[3] =	GribFld->ipdtmpl[2];	/* type of process */
	id[4] =	GribFld->ipdtmpl[4];	/* process         */
	id[5] =	GribFld->ipdtmpl[3];	/* bkgd_process    */

	/* Initialize model */
	(void) strcpy(model, "");

	/* Return if model was found */
	if ( ingest_grib_models(2, id, model) ) return TRUE;
	
	/* Set default for unrecognized model */
	(void) strcpy(did, "gribmodel");
	num = strlen(did);
	numc = int_string((int) id[0], cid, (size_t) GRIB_LABEL_LEN);
	nums = int_string((int) id[1], sid, (size_t) GRIB_LABEL_LEN);
	numt = int_string((int) id[2], tid, (size_t) GRIB_LABEL_LEN);
	numy = int_string((int) id[3], yid, (size_t) GRIB_LABEL_LEN);
	nump = int_string((int) id[4], pid, (size_t) GRIB_LABEL_LEN);
	numb = int_string((int) id[5], bid, (size_t) GRIB_LABEL_LEN);
	if ( (num > 0) && (numc > 0) && (nums > 0)  
			&& (numt > 0) && (numy > 0) && (nump > 0) && (numb > 0) 
			&& (6+num+numc+nums+numt+numy+nump+numb) < GRIB_LABEL_LEN )
		{
		(void) strcpy(model, did);
		(void) strcat(model, ":");
		(void) strcat(model, cid);
		(void) strcat(model, ":");
		(void) strcat(model, sid);
		(void) strcat(model, ":");
		(void) strcat(model, tid);
		(void) strcat(model, ":");
		(void) strcat(model, yid);
		(void) strcat(model, ":");
		(void) strcat(model, pid);
		(void) strcat(model, ":");
		(void) strcat(model, bid);

		/* Check if centre and model ids have been saved */
		for (nn=0; nn<new; nn++)
			{
			if ( newc[nn] == id[0] && news[nn] == id[1] &&
				 newt[nn] == id[2] && newy[nn] == id[3] &&
				 newp[nn] == id[4] && newb[nn] == id[5] )
					return TRUE;
			}

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E2_grib_models] Processing unrecognized");
		(void) fprintf(stderr, " model parameters: %d %d %d %d %d %d\n", 
					   id[0], id[1], id[2], id[3], id[4], id[5] );
		(void) fprintf(stderr, "     Edit your Ingest config file to add");
		(void) fprintf(stderr, " this parameter\n");

		/* Save centre and model ids (to prevent multiple messages) */
		new++;
		newc = GETMEM(newc, short int, new);
		news = GETMEM(news, short int, new);
		newt = GETMEM(newt, short int, new);
		newy = GETMEM(newy, short int, new);
		newp = GETMEM(newp, short int, new);
		newb = GETMEM(newb, short int, new);
		newc[new-1] = id[0];
		news[new-1] = id[1];
		newt[new-1] = id[2];
		newy[new-1] = id[3];
		newp[new-1] = id[4];
		newb[new-1] = id[5];
		return TRUE;
		}

	/* Error return for unrecognizable model parameter */
	(void) fprintf(stderr, "[E2_grib_models] Unrecognizable");
	(void) fprintf(stderr, " model parameters: %d %d %d %d %d %d\n", 
				   id[0], id[1], id[2], id[3], id[4], id[5] );
	return FALSE;
	}

/*********************************************************************/
/**	E 2 _ g r i b _ t s t a m p s                                   **/
/**	                                                                **/
/** Extract run and valid time information                          **/
/*********************************************************************/
static LOGICAL	E2_grib_tstamps
	(
 	STRING	rtime,  /* run timestamp         */
 	STRING 	vtimeb, /* begin valid timestamp */
 	STRING	vtimee  /* end valid timestamp   */
	)
	{
	int year,   month,   mday, jday, hour, minute, second;
	int vyearb, vjdayb, vhourb, vminb, vsecb=0;
	int vyeare, vjdaye, vhoure, vmine, vsece=0;
	int vmonth, vmday;

	/* Initialize timestamps */
	(void) strcpy(rtime, "");
	(void) strcpy(vtimeb, "");
	(void) strcpy(vtimee, "");

	/* Get the reference date and time, and set the run timestamp */
	year	= GribFld->idsect[5];
	month   = GribFld->idsect[6];
	mday    = GribFld->idsect[7];
	(void) jdate(&year, &month, &mday, &jday);
	hour    = GribFld->idsect[8];
	minute  = GribFld->idsect[9];
	second  = GribFld->idsect[10];
	(void) tnorm(&year, &jday, &hour, &minute, &second);
	(void) strcpy(rtime, build_tstamp(year, jday, hour, minute, FALSE, TRUE));

	/* Error return for unrecognizable date or time */
	if ( blank (rtime) )
		{
		(void) fprintf(stderr, "[E2_grib_tstamps] Unrecognizable");
		(void) fprintf(stderr, " year: %d",         GribFld->idsect[5]);
		(void) fprintf(stderr, "  or month: %d",    GribFld->idsect[6]);
		(void) fprintf(stderr, "  or day: %d",      GribFld->idsect[7]);
		(void) fprintf(stderr, "  or hour: %d",     GribFld->idsect[8]);
		(void) fprintf(stderr, "  or minute: %d",   GribFld->idsect[9]);
		(void) fprintf(stderr, "  or second: %d\n", GribFld->idsect[10]);
		return FALSE;
		}

	/* Interpret the valid time(s) */
	/*  set starting values for begin and end valid timestamps */
	vyearb  = vyeare = year;
	vjdayb  = vjdaye = jday;
	vhourb  = vhoure = hour;
	vminb   = vmine  = minute;
	vsecb   = vsece  = second;

	/* Interpret the begin valid timestamp */
	switch ( GribFld->ipdtmpl[7] )
		{
		case 0:	vminb += GribFld->ipdtmpl[8]; 		/* Minutes */
			break;
		case 1:	vhourb += GribFld->ipdtmpl[8];		/* Hours */
			break;
		case 2:	vjdayb += GribFld->ipdtmpl[8];		/* Days */
			break;
		case 3:	vmonth += GribFld->ipdtmpl[8];		/* Months */
				vmday = mday;
				(void) jdate(&vyearb, &vmonth, &vmday, &vjdayb);
			break;
		case 4: vyearb += GribFld->ipdtmpl[8];		/* Years */
			break;
		case 5: vyearb += 10*GribFld->ipdtmpl[8];	/* Decades (10 years)*/
			break;
		case 6: vyearb += 30*GribFld->ipdtmpl[8];	/* Normal (30 years) */
			break;
		case 7: vyearb += 100*GribFld->ipdtmpl[8];	/* Century (100 years) */
			break;
		case 10: vhourb += 3*GribFld->ipdtmpl[8];	/* 3 hours */
			break;
		case 11: vhourb += 6*GribFld->ipdtmpl[8];	/* 6 hours */
			break;
		case 12: vhourb += 12*GribFld->ipdtmpl[8];	/* 12 hours */
			break;
		case 13: vsecb += GribFld->ipdtmpl[8];		/* Seconds */
			break;
		default:	/* Unknown Unit */
			(void) fprintf(stderr, "[E2_grib_tstamps] Unrecognizable");
			(void) fprintf(stderr, " forecast time unit: %d\n",
						   GribFld->ipdtmpl[7]);
			return FALSE;
		}
	(void) tnorm(&vyearb, &vjdayb, &vhourb, &vminb, &vsecb);

	/* Interpret the end valid timestamp */
	switch ( GribFld->ipdtnum )
		{
		case 0: 
				vyeare  = vyearb;
				vjdaye  = vjdayb;
				vhoure  = vhourb;
				vmine   = vminb;
				vsece   = vsecb;
				break;
		case 8:
				vyeare  = GribFld->ipdtmpl[15];
				vmonth  = GribFld->ipdtmpl[16];
				vmday   = GribFld->ipdtmpl[17];
				vhoure  = GribFld->ipdtmpl[18];
				vmine   = GribFld->ipdtmpl[19];
				vsece   = GribFld->ipdtmpl[20];
				(void) jdate(&vyeare, &vmonth, &vmday, &vjdaye);
				break;
		default:
			(void) fprintf(stderr, "[E2_grib_tstamps] Product Definition Template");
			(void) fprintf(stderr, " %d not supported\n", GribFld->ipdtnum);
			return FALSE;
		}
	(void) tnorm(&vyeare, &vjdaye, &vhoure, &vmine, &vsece);

	/* Build valid time stamps */
	(void) strcpy(vtimeb, build_tstamp(vyearb, vjdayb, vhourb, vminb, FALSE, TRUE));
	(void) strcpy(vtimee, build_tstamp(vyeare, vjdaye, vhoure, vmine, FALSE, TRUE));

	/* Return for acceptable timestamps found */
	return TRUE;
	}

/*********************************************************************/
/**	E 2 _ g r i b _ e l e m e n t s                                 **/
/**	                                                                **/
/** Extract element information and create a label                  **/
/*********************************************************************/
static 	LOGICAL		E2_grib_elements
	(
	const	STRING	source,  	/* Source Label  */
 	STRING			element, 	/* Element Label */
 	STRING			units    	/* Units Label   */
	)
	{
	int				num, numd, numc, nump, numt;
	static char		eid[GRIB_LABEL_LEN];
	static char		did[GRIB_LABEL_LEN];
	static char		cid[GRIB_LABEL_LEN];
	static char		pid[GRIB_LABEL_LEN];
	static char		tid[GRIB_LABEL_LEN];

	/* Internal checks for centre and element ids */
	int					nn;
	static int			new=0;
	static short int	*newd=NullShort, *newc=NullShort;
	static short int	*newp=NullShort, *newt=NullShort;

	/*  parameters */
	int	id[4];
	id[0] =	GribFld->discipline; 	/* discipline */
	id[1] =	GribFld->ipdtmpl[0]; 	/* category   */
	id[2] =	GribFld->ipdtmpl[1];	/* parameter  */
	id[3] =	GribFld->ipdtnum;		/* template   */

	/* Initialise element and units labels */
	(void) strcpy(element, "");
	(void) strcpy(units,   "");

	if ( ingest_grib_elements(2, source, id, element, units) ) return TRUE;
	
	/* Set default for unrecognized element */
	(void) strcpy(eid, "gribelement");
	num = strlen(eid);
	numd = int_string((int) id[0], did, (size_t) GRIB_LABEL_LEN);
	numt = int_string((int) id[3], tid, (size_t) GRIB_LABEL_LEN);
	numc = int_string((int) id[1], cid, (size_t) GRIB_LABEL_LEN);
	nump = int_string((int) id[2], pid, (size_t) GRIB_LABEL_LEN);
	if ( (num > 0) && (numd > 0) && (numt > 0) && (numc > 0) && (nump > 0) 
			&& (4+num+numd+numt+numc+nump) < GRIB_LABEL_LEN )
		{
		(void) strcpy(element, eid);
		(void) strcat(element, ":");
		(void) strcat(element, did);
		(void) strcat(element, ":");
		(void) strcat(element, tid);
		(void) strcat(element, ":");
		(void) strcat(element, cid);
		(void) strcat(element, ":");
		(void) strcat(element, pid);

		/* Check if centre and element ids have been saved */
		for (nn=0; nn<new; nn++)
			{
			if ( newd[nn] == id[0] && newt[nn] == id[3]
					&& newc[nn] == id[1] && newp[nn] == id[2])
					return TRUE;
			}

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E2_grib_elements] Processing unrecognized");
		(void) fprintf(stderr, " element parameters: %d %d %d %d\n", 
					   id[0], id[3], id[1], id[2]);
		(void) fprintf(stderr, "     Edit your Ingest config file to add");
		(void) fprintf(stderr, " this parameter.\n");

		/* Save centre and element ids (to prevent multiple messages) */
		new++;
		newd = GETMEM(newd, short int, new);
		newt = GETMEM(newt, short int, new);
		newc = GETMEM(newc, short int, new);
		newp = GETMEM(newp, short int, new);
		newd[new-1] = id[0];
		newt[new-1] = id[3];
		newc[new-1] = id[1];
		newp[new-1] = id[2];
		return TRUE;
		}

	/* Error return for unrecognizable element parameter */
	(void) fprintf(stderr, "[E2_grib_elements] Unrecognizable");
	(void) fprintf(stderr, " element parameters: %d %d %d %d\n", 
				   id[0], id[3], id[1], id[2]);
	return FALSE;
	}

/*********************************************************************/
/** Internal use functions for E2_grib_levels                       **/
/**	level_label - Construct a lable for a single level              **/
/** layer_label - Construct a lable for a layer                     **/
/** These functions will fail if the label is a Rational number.    **/
/** How should we tackle this problem?                              **/
/*********************************************************************/
static	LOGICAL	level_label

		(
		STRING		label,
		float		fpa_scale,
		float		fpa_offset,
		float		factor,
		float		scaled_value,
		STRING		tag
		)

	{
	int		ilev, size;
	float	flev;
	char	lab[GRIB_LABEL_LEN];

	flev  = fpa_scale * (scaled_value * pow(10, -1 * factor)) + fpa_offset;
	ilev = NINT(flev);
	size = GRIB_LABEL_LEN - strlen(tag);

	if ( int_string(ilev, lab, (size_t) size) )
		{
		(void) safe_strcpy(label, lab);
		(void) safe_strcat(label, tag);
		return TRUE;
		}

	return FALSE;
	}

static  LOGICAL layer_label
	(
 	STRING     label,
 	float		fpa_scale,
 	float		fpa_offset,
 	float		factor1,
 	float		scaled_value1,
 	float		factor2,
 	float		scaled_value2,
 	STRING     tag
	)
	{
	int     jtop, jbot, size;
	float   flev;
	char    ltop[GRIB_LABEL_LEN], lbot[GRIB_LABEL_LEN];

	flev  = fpa_scale * (scaled_value1 * pow(10, -1 * factor1)) + fpa_offset;
	jtop = NINT(flev);
	flev  = fpa_scale * (scaled_value2 * pow(10, -1 * factor2)) + fpa_offset;
	jbot = NINT(flev);
	size = (GRIB_LABEL_LEN - strlen(tag) - 1) / 2;

	if ( int_string(jtop, ltop, (size_t) size)
			&& int_string(jbot, lbot, (size_t) size) )
		{
		(void) strcpy(label, ltop);
		(void) strcat(label, "-");
		(void) strcat(label, lbot);
		(void) strcat(label, tag);
		return TRUE;
		}

	return FALSE;
	}

/*********************************************************************/
/** E 2 _ g r i b _ l e v e l s                                     **/
/**                                                                 **/
/** Extract level information and create a label                    **/
/*********************************************************************/
static LOGICAL	E2_grib_levels
	(
	const STRING 	source,		/* Source Label */
 	STRING 			level		/* Level Label */
	)
	{
	int				num, numl;
	static char		did[GRIB_LABEL_LEN];
	static char		lid[GRIB_LABEL_LEN];
	/* Internal checks for centre and element ids */
	int					nn;
	static int			new=0;
	static short int	*newl=NullShort;

	LOGICAL			valid;
	int				method;
	float			scale, offset;
	static char		tag[GRIB_LABEL_LEN]	= "";

	/* First Fixed Level */
	g2int	type1   = GribFld->ipdtmpl[9];
	g2int	factor1 = GribFld->ipdtmpl[10];
	g2int	scaled1 = GribFld->ipdtmpl[11];

	/* Second Fixed Level */
	g2int	type2   = GribFld->ipdtmpl[12];
	g2int	factor2 = GribFld->ipdtmpl[13];
	g2int	scaled2 = GribFld->ipdtmpl[14];

	/* Initialize level */
	(void) strcpy(level, "");

	if ( type2 != MISSING && type2 != type1 )
		{
		(void) fprintf(stderr, 
					   "[E2_grib_levels] Missmatched fixed surface types %d and %d.\n",
					   type1, type2);
		return FALSE;
		}

	/* Check through list of level definitions */
	if ( !ingest_grib_levels(2, source, type1, &method, &scale, &offset, 
				NullFloat, NullFloat, tag) )
		{
		/* Set default for unrecognized model */
		(void) strcpy(did, "griblevel");
		num = strlen(did);
		numl = int_string((int) type1, lid, (size_t) GRIB_LABEL_LEN);

		if ( (num > 0) && (numl > 0) && (1+num+numl) < GRIB_LABEL_LEN )
			{
			(void) strcpy(level, did);
			(void) strcat(level, ":");
			(void) strcat(level, lid);
	
			/* Check if centre and model ids have been saved */
			for (nn=0; nn<new; nn++)
				{
				if ( newl[nn] == type1 ) return TRUE;
				}
	
			/* Write message to request update from FPA development group */
			(void) fprintf(stderr, "[E2_grib_levels] Processing unrecognized");
			(void) fprintf(stderr, " level type: %d\n", type1);
			(void) fprintf(stderr, "     Edit your Ingest config file to add");
			(void) fprintf(stderr, " this parameter.\n");
	
			/* Save centre and model ids (to prevent multiple messages) */
			new++;
			newl = GETMEM(newl, short int, new);
			newl[new-1] = type1;
			return TRUE;
			}
		return FALSE;
		}

	if ( method == 0 ) 			/* Single value level */
		{
		(void) safe_strcpy(level, tag);
		return TRUE;
		}
	if ( type2 == MISSING )			/* Single Level */
		{
			valid = level_label(level, scale, offset, (float)factor1, (float)scaled1, tag);
			if ( valid ) return TRUE;
		}
	else /* ( type2 != MISSING )*/ 	/* Layer */
		{
			valid = layer_label(level, scale, offset, (float)factor1, (float)scaled1, 
					(float)factor2, (float)scaled2, tag);
			if ( valid ) return TRUE;
		}
	/* Something went wrong. I should never get here. */
	(void) fprintf(stderr, "[E2_grib_levels] Error in level processing\n");
	return FALSE;
	}

/*********************************************************************/
/**                                                                 **/
/** E 2 _ g r i b _ b i t _ m a p                                   **/
/** - fetch the bitmap or create one if necessary                   **/ 
/**                                                                 **/
/*********************************************************************/
static LOGICAL E2_grib_bit_map
	(
	 GRID_DEF	grid,
	 LOGICAL	**bitmap		/* Return pointer to bitmap */
	)
	{
	static LOGICAL *bmap;
	float	missing1, missing2;	
	int		ni, nj, ii, jj;
	LOGICAL *pbit;
	float   *pdata_in;

	if ( NotNull(bitmap) ) 	*bitmap = NullPtr(LOGICAL *);

	ni = grid.nx;
	nj = grid.ny;
	switch ( GribFld->ibmap )
		{
		case 0: 
		case 254:
			bmap   = GETMEM(bmap, LOGICAL , ( ni * nj ) );
			*bitmap = bmap;
			for (ii = 0; ii< ni*nj; ii++) bmap[ii] = (LOGICAL)GribFld->bmap[ii];
			return TRUE;
		case 255: 
			/* A bitmap does not apply, if MISSING values apply then build a bitmap */
			if ( GribFld->idrtnum == 2 || GribFld->idrtnum == 3 ) 
				{
				/* Determin missing values (if any) */
				switch ( GribFld->idrtmpl[6] )
					{
					case 0: 
						return TRUE;	/* No missing values */
					case 1: 
						if ( GribFld->idrtmpl[4] == 0 )
							rdieee(GribFld->idrtmpl+7, &missing1, 1);
						else missing1 = GribFld->idrtmpl[7];

						missing2 = missing1;
						break;
					case 2:
						if ( GribFld->idrtmpl[4] == 0 )
							rdieee(GribFld->idrtmpl+7, &missing1, 1);
						else missing1 = GribFld->idrtmpl[7];
						if ( GribFld->idrtmpl[4] == 0 )
							rdieee(GribFld->idrtmpl+8, &missing2, 1);
						else missing2 = GribFld->idrtmpl[8];
					default: 
						(void) fprintf(stderr, "E2_grib_bit_map: Unknown");
						(void) fprintf(stderr, " missing value management %d", 
									   GribFld->idrtmpl[6]);
						return FALSE;	/* Don't know what to do */
					}
				/* We know the missing value so let's build a bitmap */

				bmap      = GETMEM(bmap, LOGICAL , ( ni * nj ) );

				*bitmap   = bmap;
				pbit      = bmap;
				pdata_in  = GribFld->fld;
				for (jj=0; jj<(nj*ni); jj++, pdata_in++)
					*pbit++ = (*pdata_in != missing1 && *pdata_in != missing2)?TRUE:FALSE;
				}
				return TRUE;
		default:
			(void) fprintf(stderr, "E2_grib_bit_map: Unknown predefined bimap.\n");
			return FALSE;
		}
	}
/*********************************************************************/
/**                                                                 **/
/** E 2 _ g r i b _ m a p _ m a p p r o j                           **/ 
/** Extract projection, map and grid definition information         **/
/**                                                                 **/
/*********************************************************************/
static LOGICAL	E2_grib_data_mapproj
	(
 	MAP_PROJ	*mproj, 
	int			*cflag,
	LOGICAL		*left,
	LOGICAL		*bottom
	)
	{
	float   units;
	int     ii;
	LOGICAL  	west, north, hsweep, rsweep;
	PROJ_DEF	pdef;
	MAP_DEF		mdef;
	GRID_DEF	gdef;
	float		txgrid, tygrid;

	/* Projection components */
	char    projection[MAX_FCHRS] = "";
	char    ref1[MAX_NCHRS]       = "";
	char    ref2[MAX_NCHRS]       = "";
	char    ref3[MAX_NCHRS]       = "";
	char    ref4[MAX_NCHRS]       = "";
	char    ref5[MAX_NCHRS]       = "";
	/* Grid definition components */
	STRING  pbuf, mbuf;

	/* Interpret scan_mode */
	if ( !interpret_scan_mode(&west, &north, &hsweep, &rsweep) ) return FALSE;

	/* Set name of projection */
	for (ii=0; ii<NumFPAGridLabels; ii++) 
		if (FPAGridLabels[ii].ident == GribFld->igdtnum)
			{
			safe_strcpy(projection, FPAGridLabels[ii].label);
			break;
			}

	switch (GribFld->igdtnum) /* Grid template number */
		{
		case GT_PSTEREO:
			*cflag = 1;
			if ( GETBIT(GribFld->igdtmpl[16], E2_pole_centre_south) )
				(void) safe_strcpy(ref1,"south");
			else 
				(void) safe_strcpy(ref1,"north");
			sprintf(ref2,"%.3f", (float)(GribFld->igdtmpl[12])/GribToDegrees);
			mdef.olat    = (float)(GribFld->igdtmpl[9])/GribToDegrees;
			mdef.olon    = (float)(GribFld->igdtmpl[10])/GribToDegrees;
			mdef.lref    = (float)(GribFld->igdtmpl[13])/GribToDegrees;
			gdef.nx      = GribFld->igdtmpl[7];
			gdef.ny      = GribFld->igdtmpl[8];
			txgrid   	 = GribFld->igdtmpl[14]/GribToKMeters;
			tygrid       = GribFld->igdtmpl[15]/GribToKMeters;
			gdef.gridlen = 0.0;
			/* Grid lengths are in units of 10e-3 meters Grid 
			 * definition template 3.30 note (1)    */
			gdef.units = mdef.units  = MetersPerUnit;	

			break;
		case GT_LAMBERT:
			*cflag = 1;
			(void)sprintf(ref1,"%.3f", (float)(GribFld->igdtmpl[18])/GribToDegrees); 
			(void)sprintf(ref2,"%.3f", (float)(GribFld->igdtmpl[19])/GribToDegrees);
			mdef.olat    = (float)(GribFld->igdtmpl[9])/GribToDegrees;
			mdef.olon    = (float)(GribFld->igdtmpl[10])/GribToDegrees;
			mdef.lref    = (float)(GribFld->igdtmpl[13])/GribToDegrees;
			gdef.nx      = GribFld->igdtmpl[7];
			gdef.ny      = GribFld->igdtmpl[8];
			txgrid       = GribFld->igdtmpl[14]/GribToKMeters;
			tygrid       = GribFld->igdtmpl[15]/GribToKMeters;
			gdef.gridlen = 0.0;
			/* Grid lengths are in units of 10^(-3) meters Grid 
			 * definition template 3.30 note (1)    */
			gdef.units = mdef.units  = MetersPerUnit;	


			break;
		case GT_ROTATED_LATLON:
			(void)sprintf(ref1,"%.3f", (float)(GribFld->igdtmpl[19])/GribToDegrees);
			(void)sprintf(ref2,"%.3f", (float)(GribFld->igdtmpl[20])/GribToDegrees);
			(void)sprintf(ref3,"%.3f", (float)(GribFld->igdtmpl[21]));
			/* The rest is the same as normal LATLON */
		case GT_LATLON:
			*cflag = 0;
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			mdef.olat    = (float)(GribFld->igdtmpl[11])/units;
			mdef.olon    = (float)(GribFld->igdtmpl[12])/units;
			mdef.lref    = 0.0;
			gdef.nx      = GribFld->igdtmpl[7];
			gdef.ny      = GribFld->igdtmpl[8];
			txgrid       = GribFld->igdtmpl[16]/units;
			tygrid       = GribFld->igdtmpl[17]/units;
			gdef.gridlen = 0;
			gdef.units   = mdef.units = 1.0;

			break;
		case GT_GAUSS:
			*cflag = 0;
			/* The ratio of the basic angle and the subdivisions number */
			if (0 == GribFld->igdtmpl[9] || MISSING == GribFld->igdtmpl[9] ||
				0 == GribFld->igdtmpl[10] || MISSING == GribFld->igdtmpl[10]) 
				units = GribToDegrees;
			else
				units = ((float)GribFld->igdtmpl[10])/(GribFld->igdtmpl[9]);

			mdef.olat    = (float)(GribFld->igdtmpl[11])/units;
			mdef.olon    = (float)(GribFld->igdtmpl[12])/units;
			mdef.lref    = 0.0;
			gdef.nx      = GribFld->igdtmpl[7];
			gdef.ny      = GribFld->igdtmpl[8];
			txgrid       = GribFld->igdtmpl[16]/units;
			tygrid       = (GribFld->igdtmpl[14]/units - mdef.olat)/(gdef.ny-1);
			gdef.gridlen = 0;
			gdef.units   = mdef.units = 1.0;
			break;
		default:
			for (ii=0; ii<NumGRIBGridLabels; ii++) 
				if (GRIBGridLabels[ii].ident == GribFld->igdtnum) break;

				if (GRIBGridLabels[ii].ident == GribFld->igdtnum) 
					{
					(void)fprintf(stderr, "[E2_grib_data_mapproj]");
					(void)fprintf(stderr, " Grid definition \"%s\" is not supported yet.\n", 
								  GRIBGridLabels[ii].label);
					}
				else
					{
					(void)fprintf(stderr, "[E2_grib_data_mapproj]");
					(void)fprintf(stderr, " Unknown grid projection #%d\n", 
								  GribFld->igdtnum);
					}
				return FALSE;
		}
	*left   = (LOGICAL)( (west)? ( txgrid < 0 ):( txgrid > 0 ) );
	*bottom = (LOGICAL)( (north)?( tygrid > 0 ):( tygrid < 0 ) );
	gdef.xgrid = fabs(txgrid);
	gdef.ygrid = fabs(tygrid);
	mdef.xlen = fabs((float)(gdef.nx - 1) * gdef.xgrid);
	mdef.ylen = fabs((float)(gdef.ny - 1) * gdef.ygrid);
	mdef.xorg = (*left)?   0.0: mdef.xlen;
	mdef.yorg = (*bottom)? 0.0: mdef.ylen;

	/* Set projection, mapdef and griddef structures */
	if ( !define_projection_by_name(&pdef, projection, ref1, ref2, ref3, ref4, ref5) )
		{
		(void)fprintf(stderr,"[E2_grib_data_mapproj]");
		(void)fprintf(stderr," Error setting \"projection\" with type: \"%s\"\n",
					  projection);
		(void)fprintf(stderr,"   ref(1-5): %s %s %s %s %s\n",
					  ref1, ref2, ref3, ref4, ref5 );
		return FALSE;
		}
	/* One more step ... */
	/* This map projection may be compared with another map projection read */
	/*  from an FPA metafile (such as combining u/v component fields).      */
	/* Therefore, we will "write" and "re-read" the projection information  */
	/*  exactly as is done in "write_metafile()" and "read_metafile()" to   */
	/*  ensure that the map projections will compare exactly!               */
	pbuf = format_metafile_projection(&pdef);
	mbuf = format_metafile_mapdef(&mdef, MaxDigits);
	if ( !parse_metafile_projection(pbuf, &pdef) ) return FALSE;
	if ( !parse_metafile_mapdef(mbuf,&mdef) ) return FALSE;

	/* Now Define the map projection */
	define_map_projection(mproj, &pdef, &mdef, &gdef);
	return TRUE;
	}

/*********************************************************************/
/**                                                                 **/
/** interpret_scan_mode - set flags based on scan mode              **/
/**                                                                 **/
/*********************************************************************/
static LOGICAL interpret_scan_mode
(
 LOGICAL	*west,		/* i direction is left to right (west to east) */
 LOGICAL	*north,		/* j direction is bottom to top (south to north) */
 LOGICAL	*hsweep,	/* sweep is in i direction */
 LOGICAL	*rsweep		/* sweep reverses direction after each row */
)
	{
	int ii;
	g2int scan_mode;
	/* Obtain scan mode based on grid template */
	switch (GribFld->igdtnum)
		{
		case GT_LATLON: 
		case GT_ROTATED_LATLON: 
		case GT_STRETCHED_LATLON: 
		case GT_GAUSS:
		case GT_ROTATED_GAUSS:
		case GT_STRETCHED_GAUSS:
			scan_mode = GribFld->igdtmpl[18]; 
			break;
		case GT_MERCATOR:
			scan_mode = GribFld->igdtmpl[15];
			break;
		case GT_PSTEREO: 
		case GT_LAMBERT: 
			scan_mode = GribFld->igdtmpl[17]; 
			break;
		default:
			for (ii=0; ii<NumGRIBGridLabels; ii++) 
				if (GRIBGridLabels[ii].ident == GribFld->igdtnum) break;

				if (GRIBGridLabels[ii].ident == GribFld->igdtnum) 
					{
					(void)fprintf(stderr, "[interpret_scan_mode]");
					(void)fprintf(stderr, " Grid definition \"%s\" is not supported yet.\n", 
								  GRIBGridLabels[ii].label);
					}
				else
					{
					(void)fprintf(stderr, "[interpret_scan_mode]");
					(void)fprintf(stderr, " Unknown grid projection #%d\n", 
								  GribFld->igdtnum);
					}
				return FALSE;
		}
	if ( NotNull(west) )   *west   = GETBIT(scan_mode, E2_scan_flag_west);
	if ( NotNull(north) )  *north  = GETBIT(scan_mode, E2_scan_flag_north);
	if ( NotNull(hsweep) ) *hsweep = GETBIT(scan_mode, E2_scan_flag_hsweep);
	if ( NotNull(rsweep) ) *rsweep = GETBIT(scan_mode, E2_scan_flag_rsweep);
	return TRUE;
	}

static	unsigned long	E2_extract_packed_datum

	(
	long			first_bit,	/* OFFSET OF BIT STRING BEGINNING (1st BIT IS 0) */
	short			nb_bits,	/* LENGTH OF BIT STRING */
	unsigned char	*buffer		/* SOURCE DATA */
	)

	{
	long			last_bit;
	unsigned char	*first_byte, *last_byte;
	size_t			nbytes;
	unsigned long	result, mask;
	int				shift;

	/* Determine which bytes contain the required bits */
	last_bit   = first_bit + nb_bits - 1;
	first_byte = &buffer[first_bit/8];
	last_byte  = &buffer[last_bit/8];
	nbytes     = last_byte - first_byte + 1;	/* No more than 4 bytes! */

	/* Pack 4 bytes which bracket the desired bits into result */
	result   = *(first_byte);
	result <<= 8;
	result  |= *(first_byte+1);
	result <<= 8;
	result  |= *(first_byte+2);
	result <<= 8;
	result  |= *(first_byte+3);

	/* Shift back until last_bit is right justified */
	shift = 8 - (last_bit+1)%8;
	if ( shift == 8 ) shift = 0;
	shift += 8 * (sizeof(result) - nbytes);
	if ( shift > 0 ) result >>= shift;

	/* Truncate to required number of bits (if required) */
	if (nb_bits < 32)
		{
		mask    = (1 << nb_bits) - 1;
		result &= mask;
		}

	return result;
	}
