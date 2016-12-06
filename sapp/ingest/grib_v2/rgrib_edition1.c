/***********************************************************************
*                                                                      *
*    r g r i b _ e d i t i o n 1 . c                                   *
*                                                                      *
*    Routines to decode GRIB Edition 1 format files                    *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

#define RGRIBED1_MAIN	/* To initialize defined constants and internal */
						/*  structures in rgrib_edition1.h  file        */

#include "rgrib_edition1.h"

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
/*  ... these are defined in rgrib_edition1.h */

/* Internal static functions (Identifier Translation) */
static	LOGICAL	E1_grib_models(E1_Product_definition_data, STRING);
static	LOGICAL	E1_grib_tstamps(E1_Product_definition_data, STRING, STRING,
						STRING);
static	LOGICAL	E1_grib_elements(E1_Product_definition_data, const STRING, STRING, STRING);
static	LOGICAL	E1_grib_levels(E1_Product_definition_data, const STRING, STRING);
static	LOGICAL	E1_grib_data_mapproj( GRIBFIELD *, MAP_PROJ *, LOGICAL *, LOGICAL *);
static  int		E1_grib_data_component_flag( GRIBFIELD );
static  LOGICAL	E1_extract_grib(void);
static  LOGICAL interpret_scan_mode ( LOGICAL *, LOGICAL *, LOGICAL *);

/* Internal static functions (Section Decodes) */
static	int		E1_section0decoder(FILE *, E1_Indicator_block *);
static	int		E1_section1decoder(FILE *, E1_Product_definition_data *);
static	int		E1_latlongdecoder(FILE *, E1_Grid_description_data *);
static	int		E1_gaussdecoder(FILE *, E1_Grid_description_data *);
static	int		E1_psdecoder(FILE *, E1_Grid_description_data *);
static	int		E1_lambertdecoder(FILE *, E1_Grid_description_data *);
static	int		E1_rotatedlatlongdecoder(FILE *, E1_Grid_description_data *);
static	int		E1_section2decoder(FILE *, E1_Grid_description_data *);
static	int		E1_gdbdefaultrtn(E1_Product_definition_data *,
						E1_Grid_description_data *);
static	int		E1_set_grid(E1_Product_definition_data *,
						E1_Grid_description_data *, unsigned int *,
						int *, int *, int *, int *);
static	int		E1_section3decoder(FILE *, E1_Grid_description_data *,
						E1_Bit_map_header *, unsigned int *, unsigned int *,
						LOGICAL **, LOGICAL *);
static	int		E1_section4decoder(FILE *, E1_Product_definition_data *,
						E1_Grid_description_data *, unsigned int *,
						unsigned int *, LOGICAL *, E1_Binary_data_header *,
						unsigned int *, float **, float *, float **);
static	int		E1_expand_data(E1_Product_definition_data *,
						E1_Grid_description_data *, int *, int *, float **);
static	int		E1_unmap_data(E1_Product_definition_data *,
						E1_Grid_description_data *, LOGICAL **,
						int *, int *, float **);
static	void	E1_fill_bitmap(int, int *, int, int, LOGICAL *);
static	int		E1_reorder_data(E1_Product_definition_data *,
						E1_Grid_description_data *, int *, int *,
						int *, int *, float **);
static	int		E1_add_pole_data(E1_Product_definition_data *,
						E1_Grid_description_data *, LOGICAL *, float *,
						int *, int *, float **);
static	void	E1_set_meridian_flag(E1_Grid_description_data *, int *, int *);
static	int		E1_add_meridian_data(E1_Product_definition_data *,
						E1_Grid_description_data *, int *, int *, float **);

/* Internal static functions (Setting Latitude/Longitude Increments) */
static	int		E1_set_lat_increment(int, long, long, short);
static	int		E1_set_lon_increment(int, long, long, short);

/* Internal static functions (Binary File Parsing) */
static	unsigned long	E1_extract_packed_datum(long, short, Octet *);

/* Internal static functions (Error Trap) */
static	void	errtrap(int);

/***********************************************************************
*                                                                      *
*    o p e n _ g r i b f i l e _ e d i t i o n 1                       *
*    n e x t _ g r i b f i e l d _ e d i t i o n 1                     *
*    g r i b f i e l d _ i d e n t i f i e r s _ e d i t i o n 1       *
*    c l o s e _ g r i b f i l e _ e d i t i o n 1                     *
*                                                                      *
*    Read the contents of the given GRIB file and store the decoded    *
*    information in a series of GRIBFIELD objects and translated       *
*    identifiers.                                                      *
*                                                                      *
*    open_gribfile_edition1() opens the given GRIB file for reading.   *
*                                                                      *
*    next_gribfield_edition1() extracts subsequent fields from the     *
*    opened GRIB file, saving each one in a local GRIBFIELD object.    *
*    As long as it finds a field, it returns TRUE and passes back the  *
*    GRIBFIELD object.  When the end of the GRIB file is reached, the  *
*    GRIB file is closed, and further calls to this function return    *
*    FALSE.                                                            *
*                                                                      *
*    gribfield_identifiers_edition1() extracts model, timestamp,       *
*    element, level, and unit identifiers from the local GRIBFIELD     *
*    object.                                                           *
*                                                                      *
*    close_gribfile_edition1() closes the currently open GRIB.         *
*                                                                      *
***********************************************************************/

/* Internal file pointers */
static	FILE		*GribFile    = NullPtr(FILE *);
static	long int	 GribPosition = 0;

/* Internal GRIB field buffer */
static	LOGICAL		 GribDecoded = FALSE;
static	GRIBFIELD	 GribFld     = { 0 };
static  DECODEDFIELD DecodedFld; 

/* Internal GRIB identifier buffers */
static	LOGICAL		GribValid                   = FALSE;
static	char		GribModel[GRIB_LABEL_LEN]   = "";
static	char		GribRTime[GRIB_LABEL_LEN]   = "";
static	char		GribVTimeb[GRIB_LABEL_LEN]  = "";
static	char		GribVTimee[GRIB_LABEL_LEN]  = "";
static	char		GribElement[GRIB_LABEL_LEN] = "";
static	char		GribLevel[GRIB_LABEL_LEN]   = "";
static	char		GribUnits[GRIB_LABEL_LEN]   = "";
/* Internal GRIB data buffers */
static MAP_PROJ 	MapProj;

/* Internal check to skip a field if numerical errors occur */
static	jmp_buf		GribEnv;

/* Internal checks for printing messages for coding errors */
static	LOGICAL		CMC_FirstGridIncrementError = TRUE;
static	LOGICAL		Korea_FirstLambertProjectionCentreProblem = TRUE;


LOGICAL				open_gribfile_edition1

	(
	STRING		name		/* GRIB filename */
	)

	{

	/* There will be no local GRIBFIELD upon opening */
	GribDecoded = FALSE;
	GribValid   = FALSE;

	/* If there already is an open GRIB file, close it now */
	(void) close_gribfile_edition1();

	/* Do nothing if GRIB file name not given */
	if ( blank(name) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition1] GRIB file name not given\n");
#		endif /* DEBUG */
		return FALSE;
		}

	/* See if the GRIB file exists */
	if ( !find_file(name) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition1] GRIB file not found: %s\n",
				name);
#		endif /* DEBUG */
		return FALSE;
		}

	/* Open the GRIB file */
	GribFile = fopen(name, "r");
	if ( IsNull(GribFile) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition1] GRIB file unreadable: %s\n",
				name);
#		endif /* DEBUG */
		return FALSE;
		}

	/* Set position to start of file and return TRUE if all OK */
	GribPosition = 0;
	return TRUE;
	}

LOGICAL				next_gribfield_edition1

	(
	DECODEDFIELD	**gribfld	/* pointer to local GRIBFIELD object */
	)

	{
	int		iret;
	int		ditmp, djtmp;

	/* Set default for no local GRIBFIELD */
	GribDecoded = FALSE;
	*gribfld    = NullPtr(DECODEDFIELD *);

	/* Return now if no current GRIB file */
	if ( IsNull(GribFile) ) return GribDecoded;

	/* Trap numerical errors in order to skip bad fields */
	(void) setjmp(GribEnv);
	set_num_trap(errtrap);

	/*** INDICATOR SECTION ***/
	iret = E1_section0decoder(GribFile, &GribFld.Isb);
	if ( iret != 0 && feof(GribFile) )
		{
		/* End of file in GRIB message */
		(void) close_gribfile_edition1();
		(void) unset_num_trap();
		return GribDecoded;
		}
	else if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/*** PRODUCT DEFINITION SECTION ***/
	iret = E1_section1decoder(GribFile, &GribFld.Pdd);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/* >>> Fix for CMC error in coding of surface parameters <<< */
	if ( GribFld.Pdd.layer.type == 100
			&& ((GribFld.Pdd.layer.top << 8) + GribFld.Pdd.layer.bottom) == 0 )
		{
		(void) fprintf(stderr, "  ...Correcting error in coding of");
		(void) fprintf(stderr, " surface as isobaric level at 0 hPa\n");
		GribFld.Pdd.layer.type = 1;
		}
	/* >>> End of fix <<< */

	/* >>> Fix for NMC error in coding of msl pressure <<< */
	if ( (GribFld.Pdd.parameter == 1)
			&& (GribFld.Pdd.layer.type == 102 ) )
		{
		(void) fprintf(stderr, "  ...Correcting error in coding of");
		(void) fprintf(stderr, " msl pressure as real_pressure at msl\n");
		GribFld.Pdd.parameter = 2;
		}
	/* >>> End of fix <<< */

	/*** GRID DESCRIPTION SECTION (OPTIONAL) ***/
	if ( GribFld.Pdd.block_flags.grid_description != 0 )
		{
		iret = E1_section2decoder(GribFile, &GribFld.Gdd);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition1();
				(void) unset_num_trap();
				return GribDecoded;
				}
			(void) unset_num_trap();
			(void) fprintf(stderr, " Next field will be processed\n");
			(void) fprintf(stderr, "==============================\n");
			return next_gribfield_edition1(gribfld);
			}

		/* >>> Fix for CMC coding of latitude-longitude grid increments <<< */
		if ( GribFld.Pdd.centre_id == 54
				&& GribFld.Gdd.dat_rep == LATLON_GRID
				&& GribFld.Gdd.defn.reg_ll.Di != GribFld.Gdd.defn.reg_ll.Dj )
			{
			/* Swap direction increments the right way */
			ditmp = GribFld.Gdd.defn.reg_ll.Dj;
			djtmp = GribFld.Gdd.defn.reg_ll.Di;
			GribFld.Gdd.defn.reg_ll.Di = ditmp;
			GribFld.Gdd.defn.reg_ll.Dj = djtmp;

			/* Print error message first time only! */
			if ( CMC_FirstGridIncrementError )
				{
				(void) fprintf(stderr, "  ...Correcting error in CMC coding of");
				(void) fprintf(stderr, " increments for lat/long grids\n");
				CMC_FirstGridIncrementError = FALSE;
				}
			}
		/* >>> End of fix <<< */

		/* >>> Fix for Korea coding of lambert conformal projection <<< */
		if ( GribFld.Pdd.centre_id == 40
				&& GribFld.Gdd.dat_rep == LAMBERTC_GRID
				&& GribFld.Gdd.defn.lambert.proj_centre.bipolar != 0 )
			{
			/* Reset the lambert projection to normal */
			GribFld.Gdd.defn.lambert.proj_centre.bipolar = 0;

			/* Print problem message first time only! */
			if ( Korea_FirstLambertProjectionCentreProblem )
				{
				(void) fprintf(stderr, "  ...Changing Korean coding");
				(void) fprintf(stderr, " of lambert conformal projection\n");
				(void) fprintf(stderr, "      from bi-polar to normal\n");
				Korea_FirstLambertProjectionCentreProblem = FALSE;
				}
			}
		/* >>> End of fix <<< */
		}

	/*** OR PREDEFINED GRID ***/
	else
		{
		iret = E1_gdbdefaultrtn(&GribFld.Pdd, &GribFld.Gdd);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition1();
				(void) unset_num_trap();
				return GribDecoded;
				}
			(void) unset_num_trap();
			(void) fprintf(stderr, " Next field will be processed\n");
			(void) fprintf(stderr, "==============================\n");
			return next_gribfield_edition1(gribfld);
			}
		}

	/*** REORDER BINARY DATA ARRAY ***/
	iret = E1_set_grid(&GribFld.Pdd, &GribFld.Gdd, &GribFld.NumGrid,
			&GribFld.Nii, &GribFld.Njj, &GribFld.Dii, &GribFld.Djj);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/*** BIT MAP SECTION (OPTIONAL) ***/
	if ( GribFld.Pdd.block_flags.bit_map != 0 )
		{
		iret = E1_section3decoder(GribFile, &GribFld.Gdd, &GribFld.Bmhd,
				&GribFld.NumGrid, &GribFld.NumBit, &GribFld.PBit,
				&GribFld.PoleBit);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition1();
				(void) unset_num_trap();
				return GribDecoded;
				}
			(void) unset_num_trap();
			(void) fprintf(stderr, " Next field will be processed\n");
			(void) fprintf(stderr, "==============================\n");
			return next_gribfield_edition1(gribfld);
			}
		}
	else
		{
		GribFld.NumBit  = 0;
		GribFld.PBit    = NullPtr(LOGICAL *);
		GribFld.PoleBit = FALSE;
		}

	/*** BINARY DATA SECTION ***/
	iret = E1_section4decoder(GribFile, &GribFld.Pdd, &GribFld.Gdd,
			&GribFld.NumGrid, &GribFld.NumBit, &GribFld.PoleBit,
			&GribFld.Bdhd, &GribFld.NumRaw, &GribFld.PRaw, &GribFld.PoleDatum,
			&GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/*** EXPAND QUASI-REGULAR DATA IN BINARY DATA ARRAY ***/
	iret = E1_expand_data(&GribFld.Pdd, &GribFld.Gdd,
			&GribFld.Nii, &GribFld.Njj, &GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/*** EXPAND BIT MAPPED DATA IN BINARY DATA ARRAY ***/
	if ( GribFld.Pdd.block_flags.bit_map != 0 )
		{
		iret = E1_unmap_data(&GribFld.Pdd, &GribFld.Gdd, &GribFld.PBit,
				&GribFld.Nii, &GribFld.Njj, &GribFld.PData);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition1();
				(void) unset_num_trap();
				return GribDecoded;
				}
			(void) unset_num_trap();
			(void) fprintf(stderr, " Next field will be processed\n");
			(void) fprintf(stderr, "==============================\n");
			return next_gribfield_edition1(gribfld);
			}
		}

	/*** ADD POLE DATA TO BINARY DATA ARRAY ***/
	iret = E1_add_pole_data(&GribFld.Pdd, &GribFld.Gdd, &GribFld.PoleBit,
			&GribFld.PoleDatum, &GribFld.Nii, &GribFld.Njj, &GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/* Set meridian flag for hemispheric data */
	(void) E1_set_meridian_flag(&GribFld.Gdd, &GribFld.Nii, &GribFld.Dii);

	/*** ADD MERIDIAN DATA TO BINARY DATA ARRAY ***/
	iret = E1_add_meridian_data(&GribFld.Pdd, &GribFld.Gdd,
			&GribFld.Nii, &GribFld.Njj, &GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition1();
			(void) unset_num_trap();
			return GribDecoded;
			}
		(void) unset_num_trap();
		(void) fprintf(stderr, " Next field will be processed\n");
		(void) fprintf(stderr, "==============================\n");
		return next_gribfield_edition1(gribfld);
		}

	/* Extract GRIB info into DECODEDFIELD object */
	/*
	GribDecoded = E1_extract_grib();
	*/
	GribDecoded = TRUE;
	(void) E1_extract_grib();
	/* Set GribDecoded to TRUE and return local GRIBFIELD object  */
	/*  if all parts of the GRIB message were extracted correctly */

	*gribfld    = &DecodedFld;
	(void) unset_num_trap();
	return GribDecoded;
	}

/* Grib Field Identifiers */
LOGICAL		gribfield_identifiers_edition1
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

void				close_gribfile_edition1

	(
	)

	{

	/* If there already is an open GRIB file, close it */
	if ( NotNull(GribFile) )
		{
		(void) fclose(GribFile);
		GribFile = NullPtr(FILE *);
		}

	/* Set position to start of file */
	GribPosition = 0;
	}
/***********************************************************************
*                                                                      *
*    p r i n t _ b l o c k 0 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 1 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 2 _ e d i t i o n 2                         *
*    p r i n t _ b l o c k 3 _ e d i t i o n 2                         *
*                                                                      *
*    Print out the relevant contents of the gribfield for gribtest.    *
*                                                                      *
*    Block 0 - Indicator                                               *
*    Block 1 - Identifier                                              *
*    Block 2 - Local Use                                               *
*    Block 3 - Grid Definition                                         *
*    Block 4 - Product Definition                                      *
*                                                                      *
***********************************************************************/
void print_block0_edition1 
	( 
	)
	{
	(void) fprintf(stdout, "\n   Block 0:\n");
	(void) fprintf(stdout, "     Total length    = %d \n", GribFld.Isb.length);
	(void) fprintf(stdout, "     Edition number  = %d \n", GribFld.Isb.edition);
	}

void print_block1_edition1 
	( 
	)
	{
	Octet			octet;
	int				iflags;
	(void) fprintf(stdout, "\n   Block 1:\n");
	(void) fprintf(stdout, "     PDB length      = %d \n", GribFld.Pdd.length);
	(void) fprintf(stdout, "     PDB edition     = %d \n", GribFld.Pdd.edition);
	(void) fprintf(stdout, "     PDB center      = %d \n", GribFld.Pdd.centre_id);
	(void) fprintf(stdout, "     PDB model_id    = %d \n", GribFld.Pdd.model_id);
	(void) fprintf(stdout, "     PDB grid_defn   = %d \n", GribFld.Pdd.grid_defn);
	octet = 0;
	if (GribFld.Pdd.block_flags.grid_description)
			SETBIT(octet, E1_block_flag_grid_desc);
	if (GribFld.Pdd.block_flags.bit_map)
			SETBIT(octet, E1_block_flag_bit_map);
	iflags = (int) octet;
	(void) fprintf(stdout, "     PDB block_flags = %d \n", iflags);
	(void) fprintf(stdout, "     PDB parameter   = %d \n", GribFld.Pdd.parameter);
	(void) fprintf(stdout, "     PDB level type  = %d \n", GribFld.Pdd.layer.type);
	(void) fprintf(stdout, "     PDB layer.top   = %d \n", GribFld.Pdd.layer.top);
	(void) fprintf(stdout, "     PDB layer.bottom= %d \n", GribFld.Pdd.layer.bottom);
	(void) fprintf(stdout, "     PDB year        = %d \n",
			GribFld.Pdd.forecast.reference.year);
	(void) fprintf(stdout, "     PDB month       = %d \n",
			GribFld.Pdd.forecast.reference.month);
	(void) fprintf(stdout, "     PDB day         = %d \n",
			GribFld.Pdd.forecast.reference.day);
	(void) fprintf(stdout, "     PDB hour        = %d \n",
			GribFld.Pdd.forecast.reference.hour);
	(void) fprintf(stdout, "     PDB minutes     = %d \n",
			GribFld.Pdd.forecast.reference.minute);
	(void) fprintf(stdout, "     PDB unit        = %d \n", GribFld.Pdd.forecast.units);
	(void) fprintf(stdout, "     PDB time1       = %d \n", GribFld.Pdd.forecast.time1);
	(void) fprintf(stdout, "     PDB time2       = %d \n", GribFld.Pdd.forecast.time2);
	(void) fprintf(stdout, "     PDB range_type  = %d \n",
			GribFld.Pdd.forecast.range_type);
	(void) fprintf(stdout, "     PDB no. average = %d \n",
			GribFld.Pdd.forecast.nb_averaged);
	(void) fprintf(stdout, "     PDB no. missing = %d \n",
			GribFld.Pdd.forecast.nb_missing);
	(void) fprintf(stdout, "     PDB century     = %d \n", GribFld.Pdd.forecast.century);
	(void) fprintf(stdout, "     PDB reserved    = %d \n", GribFld.Pdd.forecast.reserved);
	(void) fprintf(stdout, "     PDB factor_d    = %d \n",
			GribFld.Pdd.forecast.factor_d);
	}

void print_block2_edition1 
	( 
	)
	{
	Octet			octet;
	int				iproj, icode;
	int				ii, jj, count, MaxCount = 10;

	(void) fprintf(stdout, "\n   Block 2:\n");
	(void) fprintf(stdout, "     GDB length      = %d \n", GribFld.Gdd.length);
	(void) fprintf(stdout, "     GDB Nv          = %d \n", GribFld.Gdd.nv);
	(void) fprintf(stdout, "     GDB pv or pl    = %d \n", GribFld.Gdd.pv_or_pl);
	(void) fprintf(stdout, "     GDB grid type   = %d \n", GribFld.Gdd.dat_rep);

	switch(GribFld.Gdd.dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			(void) fprintf(stdout, "     GDB Ni          = %d \n",
					GribFld.Gdd.defn.reg_ll.Ni);
			(void) fprintf(stdout, "     GDB Nj          = %d \n",
					GribFld.Gdd.defn.reg_ll.Nj);
			(void) fprintf(stdout, "     GDB La1         = %d \n",
					GribFld.Gdd.defn.reg_ll.La1);
			(void) fprintf(stdout, "     GDB Lo1         = %d \n",
					GribFld.Gdd.defn.reg_ll.Lo1);
			(void) fprintf(stdout, "     GDB resolution  = %d \n",
					GribFld.Gdd.defn.reg_ll.resltn);
			(void) fprintf(stdout, "     GDB La2         = %d \n",
					GribFld.Gdd.defn.reg_ll.La2);
			(void) fprintf(stdout, "     GDB Lo2         = %d \n",
					GribFld.Gdd.defn.reg_ll.Lo2);
			(void) fprintf(stdout, "     GDB Di          = %d \n",
					GribFld.Gdd.defn.reg_ll.Di);
			(void) fprintf(stdout, "     GDB Dj          = %d \n",
					GribFld.Gdd.defn.reg_ll.Dj);
			octet = 0;
			if (GribFld.Gdd.defn.reg_ll.scan_mode.west)
					SETBIT(octet, E1_scan_flag_west);
			if (GribFld.Gdd.defn.reg_ll.scan_mode.north)
					SETBIT(octet, E1_scan_flag_north);
			if (GribFld.Gdd.defn.reg_ll.scan_mode.horz_sweep)
					SETBIT(octet, E1_scan_flag_hsweep);
			icode = (int) octet;
			(void) fprintf(stdout, "     GDB scan mode   = %d \n", icode);
			(void) fprintf(stdout, "            (west)     = %d \n",
					GribFld.Gdd.defn.reg_ll.scan_mode.west);
			(void) fprintf(stdout, "            (north)    = %d \n",
					GribFld.Gdd.defn.reg_ll.scan_mode.north);
			(void) fprintf(stdout, "        (by longitude) = %d \n",
					GribFld.Gdd.defn.reg_ll.scan_mode.horz_sweep);
			(void) fprintf(stdout, "     GDB thin mode   = %d \n",
					GribFld.Gdd.defn.reg_ll.thin_mode);
			if ( GribFld.Gdd.defn.reg_ll.thin_mode > 0 )
				{
				if ( GribFld.Gdd.defn.reg_ll.thin_mode == 1 )
					ii = GribFld.Gdd.defn.reg_ll.Nj;
				else if ( GribFld.Gdd.defn.reg_ll.thin_mode == 2 )
					ii = GribFld.Gdd.defn.reg_ll.Ni;
				else
					ii = 0;
				(void) fprintf(stdout, "     GDB thin points = ");
				for ( count=0, jj=0; jj<ii; jj++ )
					{
					if (++count > MaxCount)
						{
						count = 1;
						(void) fprintf(stdout, "\n                       ");
						}
					(void) fprintf(stdout, "%3d ",
							GribFld.Gdd.defn.reg_ll.thin_pts[jj]);
					}
				(void) fprintf(stdout, "\n");
				}
			(void) fprintf(stdout, "     GDB pole extra  = %d \n",
					GribFld.Gdd.defn.reg_ll.pole_extra);
			(void) fprintf(stdout, "     GDB long extra  = %d \n",
					GribFld.Gdd.defn.reg_ll.meridian_extra);
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			(void) fprintf(stdout, "     GDB Ni          = %d \n",
					GribFld.Gdd.defn.guas_ll.Ni);
			(void) fprintf(stdout, "     GDB Nj          = %d \n",
					GribFld.Gdd.defn.guas_ll.Nj);
			(void) fprintf(stdout, "     GDB La1         = %d \n",
					GribFld.Gdd.defn.guas_ll.La1);
			(void) fprintf(stdout, "     GDB Lo1         = %d \n",
					GribFld.Gdd.defn.guas_ll.Lo1);
			(void) fprintf(stdout, "     GDB resolution  = %d \n",
					GribFld.Gdd.defn.guas_ll.resltn);
			(void) fprintf(stdout, "     GDB La2         = %d \n",
					GribFld.Gdd.defn.guas_ll.La2);
			(void) fprintf(stdout, "     GDB Lo2         = %d \n",
					GribFld.Gdd.defn.guas_ll.Lo2);
			(void) fprintf(stdout, "     GDB Di          = %d \n",
					GribFld.Gdd.defn.guas_ll.Di);
			(void) fprintf(stdout, "     GDB N           = %d \n",
					GribFld.Gdd.defn.guas_ll.N);
			octet = 0;
			if (GribFld.Gdd.defn.guas_ll.scan_mode.west)
					SETBIT(octet, E1_scan_flag_west);
			if (GribFld.Gdd.defn.guas_ll.scan_mode.north)
					SETBIT(octet, E1_scan_flag_north);
			if (GribFld.Gdd.defn.guas_ll.scan_mode.horz_sweep)
					SETBIT(octet, E1_scan_flag_hsweep);
			icode = (int) octet;
			(void) fprintf(stdout, "     GDB scan mode   = %d \n", icode);
			(void) fprintf(stdout, "            (west)     = %d \n",
					GribFld.Gdd.defn.guas_ll.scan_mode.west);
			(void) fprintf(stdout, "            (north)    = %d \n",
					GribFld.Gdd.defn.guas_ll.scan_mode.north);
			(void) fprintf(stdout, "        (by longitude) = %d \n",
					GribFld.Gdd.defn.guas_ll.scan_mode.horz_sweep);
			(void) fprintf(stdout, "     GDB thin mode   = %d \n",
					GribFld.Gdd.defn.guas_ll.thin_mode);
			if ( GribFld.Gdd.defn.guas_ll.thin_mode > 0 )
				{
				if ( GribFld.Gdd.defn.guas_ll.thin_mode == 1 )
					ii = GribFld.Gdd.defn.guas_ll.Nj;
				else
					ii = 0;
				(void) fprintf(stdout, "     GDB thin points = ");
				for ( count=0, jj=0; jj<ii; jj++ )
					{
					if ( ++count > MaxCount )
						{
						count = 1;
						(void) fprintf(stdout, "\n                       ");
						}
					(void) fprintf(stdout, "%3d ", GribFld.Gdd.defn.guas_ll.thin_pts[jj]);
					}
				(void) fprintf(stdout, "\n");
				}
			break;
		case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
			(void) fprintf(stdout, "     GDB Nx          = %d \n",
					GribFld.Gdd.defn.ps.Nx);
			(void) fprintf(stdout, "     GDB Ny          = %d \n",
					GribFld.Gdd.defn.ps.Ny);
			(void) fprintf(stdout, "     GDB La1         = %d \n",
					GribFld.Gdd.defn.ps.La1);
			(void) fprintf(stdout, "     GDB Lo1         = %d \n",
					GribFld.Gdd.defn.ps.Lo1);
			(void) fprintf(stdout, "     GDB compnt      = %d \n",
					GribFld.Gdd.defn.ps.compnt);
			(void) fprintf(stdout, "     GDB LoV         = %d \n",
					GribFld.Gdd.defn.ps.LoV);
			(void) fprintf(stdout, "     GDB Dx          = %d \n",
					GribFld.Gdd.defn.ps.Dx);
			(void) fprintf(stdout, "     GDB Dy          = %d \n",
					GribFld.Gdd.defn.ps.Dy);
			octet = 0;
			if (GribFld.Gdd.defn.ps.proj_centre.pole)
					SETBIT(octet, E1_proj_flag_pole);
			iproj = (int) octet;
			(void) fprintf(stdout, "     GDB proj center = %d \n", iproj);
			(void) fprintf(stdout, "          (south pole) = %d \n",
					GribFld.Gdd.defn.ps.proj_centre.pole);
			octet = 0;
			if (GribFld.Gdd.defn.ps.scan_mode.west)
					SETBIT(octet, E1_scan_flag_west);
			if (GribFld.Gdd.defn.ps.scan_mode.north)
					SETBIT(octet, E1_scan_flag_north);
			if (GribFld.Gdd.defn.ps.scan_mode.horz_sweep)
					SETBIT(octet, E1_scan_flag_hsweep);
			icode = (int) octet;
			(void) fprintf(stdout, "     GDB scan mode   = %d \n", icode);
			(void) fprintf(stdout, "          (negative x) = %d \n",
					GribFld.Gdd.defn.ps.scan_mode.west);
			(void) fprintf(stdout, "          (positive y) = %d \n",
					GribFld.Gdd.defn.ps.scan_mode.north);
			(void) fprintf(stdout, "          (by column)  = %d \n",
					GribFld.Gdd.defn.ps.scan_mode.horz_sweep);
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			(void) fprintf(stdout, "     GDB Nx          = %d \n",
					GribFld.Gdd.defn.lambert.Nx);
			(void) fprintf(stdout, "     GDB Ny          = %d \n",
					GribFld.Gdd.defn.lambert.Ny);
			(void) fprintf(stdout, "     GDB La1         = %d \n",
					GribFld.Gdd.defn.lambert.La1);
			(void) fprintf(stdout, "     GDB Lo1         = %d \n",
					GribFld.Gdd.defn.lambert.Lo1);
			(void) fprintf(stdout, "     GDB compnt      = %d \n",
					GribFld.Gdd.defn.lambert.compnt);
			(void) fprintf(stdout, "     GDB LoV         = %d \n",
					GribFld.Gdd.defn.lambert.LoV);
			(void) fprintf(stdout, "     GDB Dx          = %d \n",
					GribFld.Gdd.defn.lambert.Dx);
			(void) fprintf(stdout, "     GDB Dy          = %d \n",
					GribFld.Gdd.defn.lambert.Dy);
			octet = 0;
			if (GribFld.Gdd.defn.lambert.proj_centre.pole)
					SETBIT(octet, E1_proj_flag_pole);
			if (GribFld.Gdd.defn.lambert.proj_centre.bipolar)
					SETBIT(octet, E1_proj_flag_bipolar);
			iproj = (int) octet;
			(void) fprintf(stdout, "     GDB proj center = %d \n", iproj);
			(void) fprintf(stdout, "          (south pole) = %d \n",
					GribFld.Gdd.defn.lambert.proj_centre.pole);
			(void) fprintf(stdout, "          (bi-polar)   = %d \n",
					GribFld.Gdd.defn.lambert.proj_centre.bipolar);
			octet = 0;
			if (GribFld.Gdd.defn.lambert.scan_mode.west)
					SETBIT(octet, E1_scan_flag_west);
			if (GribFld.Gdd.defn.lambert.scan_mode.north)
					SETBIT(octet, E1_scan_flag_north);
			if (GribFld.Gdd.defn.lambert.scan_mode.horz_sweep)
					SETBIT(octet, E1_scan_flag_hsweep);
			icode = (int) octet;
			(void) fprintf(stdout, "     GDB scan mode   = %d \n", icode);
			(void) fprintf(stdout, "          (negative x) = %d \n",
					GribFld.Gdd.defn.lambert.scan_mode.west);
			(void) fprintf(stdout, "          (positive y) = %d \n",
					GribFld.Gdd.defn.lambert.scan_mode.north);
			(void) fprintf(stdout, "          (by column)  = %d \n",
					GribFld.Gdd.defn.lambert.scan_mode.horz_sweep);
			(void) fprintf(stdout, "     GDB Latin1      = %d \n",
					GribFld.Gdd.defn.lambert.Latin1);
			(void) fprintf(stdout, "     GDB Latin2      = %d \n",
					GribFld.Gdd.defn.lambert.Latin2);
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			(void) fprintf(stdout, "     GDB Ni          = %d \n",
					GribFld.Gdd.defn.rotate_ll.Ni);
			(void) fprintf(stdout, "     GDB Nj          = %d \n",
					GribFld.Gdd.defn.rotate_ll.Nj);
			(void) fprintf(stdout, "     GDB La1         = %d \n",
					GribFld.Gdd.defn.rotate_ll.La1);
			(void) fprintf(stdout, "     GDB Lo1         = %d \n",
					GribFld.Gdd.defn.rotate_ll.Lo1);
			(void) fprintf(stdout, "     GDB resolution  = %d \n",
					GribFld.Gdd.defn.rotate_ll.resltn);
			(void) fprintf(stdout, "     GDB La2         = %d \n",
					GribFld.Gdd.defn.rotate_ll.La2);
			(void) fprintf(stdout, "     GDB Lo2         = %d \n",
					GribFld.Gdd.defn.rotate_ll.Lo2);
			(void) fprintf(stdout, "     GDB Di          = %d \n",
					GribFld.Gdd.defn.rotate_ll.Di);
			(void) fprintf(stdout, "     GDB Dj          = %d \n",
					GribFld.Gdd.defn.rotate_ll.Dj);
			octet = 0;
			if (GribFld.Gdd.defn.rotate_ll.scan_mode.west)
					SETBIT(octet, E1_scan_flag_west);
			if (GribFld.Gdd.defn.rotate_ll.scan_mode.north)
					SETBIT(octet, E1_scan_flag_north);
			if (GribFld.Gdd.defn.rotate_ll.scan_mode.horz_sweep)
					SETBIT(octet, E1_scan_flag_hsweep);
			icode = (int) octet;
			(void) fprintf(stdout, "     GDB scan mode   = %d \n", icode);
			(void) fprintf(stdout, "            (west)     = %d \n",
					GribFld.Gdd.defn.rotate_ll.scan_mode.west);
			(void) fprintf(stdout, "            (north)    = %d \n",
					GribFld.Gdd.defn.rotate_ll.scan_mode.north);
			(void) fprintf(stdout, "        (by longitude) = %d \n",
					GribFld.Gdd.defn.rotate_ll.scan_mode.horz_sweep);
			(void) fprintf(stdout, "     GDB LaP         = %d \n",
					GribFld.Gdd.defn.rotate_ll.LaP);
			(void) fprintf(stdout, "     GDB LoP         = %d \n",
					GribFld.Gdd.defn.rotate_ll.LoP);
			(void) fprintf(stdout, "     GDB AngR        = %f \n",
					GribFld.Gdd.defn.rotate_ll.AngR);
			(void) fprintf(stdout, "     GDB thin mode   = %d \n",
					GribFld.Gdd.defn.rotate_ll.thin_mode);
			if ( GribFld.Gdd.defn.rotate_ll.thin_mode > 0 )
				{
				if ( GribFld.Gdd.defn.rotate_ll.thin_mode == 1 )
					ii = GribFld.Gdd.defn.rotate_ll.Nj;
				else if ( GribFld.Gdd.defn.rotate_ll.thin_mode == 2 )
					ii = GribFld.Gdd.defn.rotate_ll.Ni;
				else
					ii = 0;
				(void) fprintf(stdout, "     GDB thin points = ");
				for ( count=0, jj=0; jj<ii; jj++ )
					{
					if (++count > MaxCount)
						{
						count = 1;
						(void) fprintf(stdout, "\n                       ");
						}
					(void) fprintf(stdout, "%3d ",
							GribFld.Gdd.defn.rotate_ll.thin_pts[jj]);
					}
				(void) fprintf(stdout, "\n");
				}
			(void) fprintf(stdout, "     GDB pole extra  = %d \n",
					GribFld.Gdd.defn.rotate_ll.pole_extra);
			(void) fprintf(stdout, "     GDB long extra  = %d \n",
					GribFld.Gdd.defn.rotate_ll.meridian_extra);
			break;
		default :
			break;
		}
	}

void print_block3_edition1 
	( 
	)
	{
	int				isweep, ipole, ilatlon;
	int				ni, nj, ii, jj;
	LOGICAL			*xbits, xbit;

	if ( GribFld.Pdd.block_flags.bit_map )
		{
		(void) fprintf(stdout, "\n   Block 3:\n");
		(void) fprintf(stdout, "     BMB length      = %d \n", GribFld.Bmhd.length);
		(void) fprintf(stdout, "     BMB unused      = %d \n", GribFld.Bmhd.unused);
		(void) fprintf(stdout, "     BMB ntable      = %d \n", GribFld.Bmhd.ntable);

		switch(GribFld.Gdd.dat_rep)
			{
			case LATLON_GRID:			/* LATITUDE-LONGITUDE */
				ilatlon = 1;
				ni = GribFld.Gdd.defn.reg_ll.Ni;
				nj = GribFld.Gdd.defn.reg_ll.Nj;
				isweep = GribFld.Gdd.defn.reg_ll.scan_mode.horz_sweep;
				ipole  = GribFld.Gdd.defn.reg_ll.pole_extra;
				break;
			case GAUSS_GRID:			/* GAUSSIAN */
				ilatlon = 1;
				ni = GribFld.Gdd.defn.guas_ll.Ni;
				nj = GribFld.Gdd.defn.guas_ll.Nj;
				isweep = GribFld.Gdd.defn.guas_ll.scan_mode.horz_sweep;
				ipole  = 0;
				break;
			case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
				ilatlon = 1;
				ni = GribFld.Gdd.defn.rotate_ll.Ni;
				nj = GribFld.Gdd.defn.rotate_ll.Nj;
				isweep = GribFld.Gdd.defn.rotate_ll.scan_mode.horz_sweep;
				ipole  = GribFld.Gdd.defn.rotate_ll.pole_extra;
				break;
			case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
				ilatlon = 0;
				ni = GribFld.Gdd.defn.ps.Nx;
				nj = GribFld.Gdd.defn.ps.Ny;
				isweep = GribFld.Gdd.defn.ps.scan_mode.horz_sweep;
				ipole  = 0;
				break;
			case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
				ilatlon = 0;
				ni = GribFld.Gdd.defn.lambert.Nx;
				nj = GribFld.Gdd.defn.lambert.Ny;
				isweep = GribFld.Gdd.defn.lambert.scan_mode.horz_sweep;
				ipole  = 0;
				break;
			default :
				ilatlon = -1;
				break;
					}

		if ( ilatlon == -1 ) return;

		if ( ipole != 0 )
			{
			(void) fprintf(stdout, "\n   Bit map - Pole Bit\n");
			if ( GribFld.PoleBit ) (void) fprintf(stdout, "     B\n");
			else                    (void) fprintf(stdout, "     .\n");
			}

		/* Note that data may be ordered either by column or by row, */
		/*  but is printed out in ni columns for each of nj rows     */
		(void) fprintf(stdout, "\n   Bit map -");
		if ( ilatlon == 1 )
			{
			(void) fprintf(stdout, "  %d Longitudes for each of", ni);
			(void) fprintf(stdout, "  %d Latitudes", nj);
			}
		else
			{
			(void) fprintf(stdout, "  %d Columns for each of", ni);
			(void) fprintf(stdout, "  %d Rows", nj);
			}

		xbits = GribFld.PBit;
		for ( jj=0; jj<nj; jj++ )
			{
			(void) fprintf(stdout, "\n");
			for ( ii=0; ii<ni; ii++ )
				{
				if ( isweep == 0 ) xbit = xbits[jj*ni + ii];
				else               xbit = xbits[ii*nj + jj];

				if ( xbit ) (void) fprintf(stdout, "B");
				else        (void) fprintf(stdout, ".");
				}
			}
		(void) fprintf(stdout, "\n");
		}
	}

void print_block4_edition1 
	( 
	)
	{
	(void) fprintf(stdout, "\n   Block 4:\n");
	(void) fprintf(stdout, "     BDB length      = %d \n", GribFld.Bdhd.length);
	(void) fprintf(stdout, "     BDB flags       = %d \n", GribFld.Bdhd.flags);
	(void) fprintf(stdout, "     BDB unused      = %d \n", GribFld.Bdhd.unused);
	(void) fprintf(stdout, "     BDB scale       = %d \n", GribFld.Bdhd.scale);
	(void) fprintf(stdout, "     BDB reference   = %f \n", GribFld.Bdhd.reference);
	(void) fprintf(stdout, "     BDB bits/value  = %d \n", GribFld.Bdhd.bits_per_val);
	}

void print_block4_raw_edition1 
	( 
	)
	{
	int				ipole;
	int				ii, count, MaxCount = 10;
	float			*gvals;

	switch(GribFld.Gdd.dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			ipole = GribFld.Gdd.defn.reg_ll.pole_extra;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ipole = GribFld.Gdd.defn.rotate_ll.pole_extra;
			break;
		default :
			ipole = 0;
			break;
		}

	if ( ipole != 0 )
		{
		(void) fprintf(stdout, "\n   Raw GRIB data - Pole Datum\n");
		(void) fprintf(stdout, "%10.2f\n", GribFld.PoleDatum);
		}

	(void) fprintf(stdout, "\n   Raw GRIB data -");
	(void) fprintf(stdout, "  %d Data values\n", GribFld.NumRaw);
	gvals = GribFld.PRaw;
	for ( count=0, ii=0; ii<GribFld.NumRaw; ii++ )
		{
		if ( ++count > MaxCount )
			{
			count = 1;
			(void) fprintf(stdout, "\n");
			}
		(void) fprintf(stdout, "%10.2f ", *gvals++);
		}
	(void) fprintf(stdout, "\n");
	}

void print_block4_data_edition1 
	( 
	)
	{
	int				ilatlon;
	int				ii, jj, count, MaxCount = 10;
	float			*gvals;

	switch(GribFld.Gdd.dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
		case GAUSS_GRID:			/* GAUSSIAN */
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ilatlon = 1;
			break;
		case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			ilatlon = 0;
			break;
		default :
			ilatlon = 0;
			break;
		}

	/* Note that data is ordered by Nii columns in each of Njj rows */
	(void) fprintf(stdout, "\n   Processed GRIB data -");
	if ( ilatlon == 1 )
		{
		(void) fprintf(stdout, "  %d Longitudes for each of", GribFld.Nii);
		(void) fprintf(stdout, "  %d Latitudes", GribFld.Njj);
		}
	else
		{
		(void) fprintf(stdout, "  %d Columns for each of", GribFld.Nii);
		(void) fprintf(stdout, "  %d Rows", GribFld.Njj);
		}

	gvals = GribFld.PData;
	for ( jj=0; jj<GribFld.Njj; jj++ )
		{
		(void) fprintf(stdout, "\n");
		for ( count=0, ii=0; ii<GribFld.Nii; ii++ )
			{
			if ( ++count > MaxCount )
				{
				count = 1;
				(void) fprintf(stdout, "\n");
				}
			(void) fprintf(stdout, "%10.2f ", *gvals++);
			}
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

/**********************************************************************
 ***                                                                ***
 *** E 1 _ e x t r a c t _ g r i b                                  ***
 *** E 1 _ g r i b _ m o d e l s                                    ***
 *** E 1 _ g r i b _ t s t a m p s                                  ***
 *** E 1 _ g r i b _ e l e m e n t s                                ***
 *** E 1 _ g r i b _ l e v e l s                                    ***
 *** E 1 _ g r i b _ d a t a _ m a p p r o j                        ***
 *** E 1 _ g r i b _d a t a _ c o m p o n e n t _ f l a g           ***
 ***                                                                ***
 *** Identify model label, run and valid timestamps, element/units  ***
 *** labels, or level label from GRIB product definition data.      ***
 ***                                                                ***
 *** These routines are based on Tables in the 1991 GRIB Edition 1  ***
 *** document WMO Code FM 92-VIII Ext. entitled "The WMO Format For ***
 *** The Storage Of Weather Product Information And The Exchange Of ***
 *** Weather Product Messages In Gridded Binary Form" editted by    ***
 *** John D. Stackpole of the U.S. Department of Commerce, NMC.     ***
 *** (Further information is available in the 1988 Edition of WMO   ***
 *** #306, "Manual on Codes, Volume 1, Part B, Binary Codes",       ***
 *** Section c. List Of Binary Codes With Their Specifications And  ***
 *** Associated Code Tables, FM92-IX Ext. GRIB (gridded binary).)   ***
 ***                                                                ***
 *** The  model  definitions are from Table 0 and Table A.          ***
 *** The  tstamp  definitions are from Table 4 and Table 5.         ***
 *** The  element/units  definitions are from Table 2.              ***
 *** The  level  defintions are from Table 3 and Table 3a.          ***
 ***                                                                ***
 *** Note that model and element/unit definitions which are not     ***
 *** recognized in rgrib.h are returned as a default name tag which ***
 *** could be decoded at a later time.  For example, gribmodel:1:15 ***
 *** would identify an unrecognized model "15" from Melbourne WMC,  ***
 *** and gribelement:25 would identify an unrecognized element      ***
 *** "temperature_anomaly" in "degreesK".                           ***
 ***                                                                ***
 **********************************************************************/

static LOGICAL	E1_extract_grib
	(
	)
	{

	LOGICAL left, bottom, west, north, isweep;
	/* Set defaults for GRIB identifiers */
	GribValid               = FALSE;

	/* Return now if no current GRIB file or no local GRIBFIELD object */
	if ( IsNull(GribFile) || !GribDecoded ) return GribValid;

	/* Set model label from originating center and model */
	if ( !E1_grib_models(GribFld.Pdd, GribModel) ) return GribValid;

	/* Set run and valid timestamps from date and time information */
	if ( !E1_grib_tstamps(GribFld.Pdd, GribRTime, GribVTimeb, GribVTimee) )
			return GribValid;

	/* Set element and units labels from element code */
	if ( !E1_grib_elements(GribFld.Pdd, GribModel, GribElement, GribUnits) )
			return GribValid;

	/* Set level label from level code and values */
	if ( !E1_grib_levels(GribFld.Pdd, GribModel, GribLevel) )
			return GribValid;

	/* Set projection, map definition and grid definition */
	if ( !E1_grib_data_mapproj(&GribFld, &MapProj, &left, &bottom) )
		return GribValid;

	/* Set scan mode bits */
	if ( !interpret_scan_mode(&west, &north, &isweep) ) return GribValid;

	/* Set GribValid to TRUE and return requested identifiers */
	/*  if all identifiers were correctly translated          */
	GribValid                 = TRUE;
	DecodedFld.model	      = GribModel;
	DecodedFld.rtime	      = GribRTime;
	DecodedFld.vtimeb	      = GribVTimeb;
	DecodedFld.vtimee	      = GribVTimee;
	DecodedFld.element	      = GribElement;
	DecodedFld.units	      = GribUnits;
	DecodedFld.level	      = GribLevel;
	DecodedFld.mproj_orig	  = DecodedFld.mproj = &MapProj;
	DecodedFld.data_orig	  = DecodedFld.data	 = GribFld.PData;
	DecodedFld.bmap	          = GribFld.PBit;
	DecodedFld.component_flag =	E1_grib_data_component_flag( GribFld );
	DecodedFld.west			  = west;
	DecodedFld.north		  = north;
	DecodedFld.left			  = left;
	DecodedFld.bottom		  = bottom;
	DecodedFld.isweep		  = isweep;
	DecodedFld.rsweep		  = FALSE;

	/* Set flags for data processing */
	DecodedFld.filled         = FALSE;
	DecodedFld.reordered      = FALSE;
	DecodedFld.wrapped        = FALSE;

	return GribValid;
	}

static	LOGICAL		E1_grib_models

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		model		/* string containing model label */
	)

	{
	int				num, numc, numm;
	static char		did[GRIB_LABEL_LEN];
	static char		cid[GRIB_LABEL_LEN];
	static char     mid[GRIB_LABEL_LEN];
	/* Internal checks for centre and element ids */
	int					nn;
	static int			new=0;
	static short int	*newc=NullShort, *newm=NullShort;
	/* Parameter list */
	int id[2];
	id[0] = pdd.centre_id;
	id[1] = pdd.model_id;

	/* Initialise model */
	(void) strcpy(model, "");

	if ( ingest_grib_models(1, id, model) ) return TRUE;

	/* Set default for unrecognized model */
	(void) strcpy(did, "gribmodel");
	num = strlen(did);
	numc = int_string((int) pdd.centre_id, cid, (size_t) GRIB_LABEL_LEN);
	numm = int_string((int) pdd.model_id, mid, (size_t) GRIB_LABEL_LEN);
	if ( num > 0 && numc > 0 && numm >0 && (num+1+numc+1+numm) < GRIB_LABEL_LEN )
		{
		(void) strcpy(model, did);
		(void) strcat(model, ":");
		(void) strcat(model, cid);
		(void) strcat(model, ":");
		(void) strcat(model, mid);

		/* Check if centre and model ids have been saved */
		for (nn=0; nn<new; nn++)
			{
			if ( newc[nn] == pdd.centre_id && newm[nn] == pdd.model_id)
					return TRUE;
			}

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E1_grib_models] Processing unrecognized");
		(void) fprintf(stderr, " model parameter: %d %d\n", pdd.centre_id, pdd.model_id);
		(void) fprintf(stderr, "     Edit your Ingest config file to add");
		(void) fprintf(stderr, " this parameter\n");

		/* Save centre and model ids (to prevent multiple messages) */
		new++;
		newc = GETMEM(newc, short int, new);
		newm = GETMEM(newm, short int, new);
		newc[new-1] = pdd.centre_id;
		newm[new-1] = pdd.model_id;
		return TRUE;
		}

	/* Error return for unrecognizable model parameter */
	(void) fprintf(stderr, "[E1_grib_models] Unrecognizable");
	(void) fprintf(stderr, " model parameter: %d %d\n", pdd.centre_id, pdd.model_id);
	return FALSE;
	}

static	LOGICAL		E1_grib_tstamps

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		rtime,		/* string containing run timestamp */
	STRING		vtimeb,		/* string containing begin valid timestamp */
	STRING		vtimee		/* string containing end valid timestamp */
	)

	{
	int		year, month, mday, jday, hour, minute, second=0;
	int		vyearb, vjdayb, vhourb, vminb, vsecb=0, ptimeb;
	int		vyeare, vjdaye, vhoure, vmine, vsece=0, ptimee;
	int		vmonth, vmday;

	/* Initialise timestamps */
	(void) strcpy(rtime,  "");
	(void) strcpy(vtimeb, "");
	(void) strcpy(vtimee, "");

	/* Get the reference date and time, and set the run timestamp */
	year   = 100 * (pdd.forecast.century - 1)
					+ pdd.forecast.reference.year;
	month  = pdd.forecast.reference.month;
	mday   = pdd.forecast.reference.day;
	(void) jdate(&year, &month, &mday, &jday);
	hour   = pdd.forecast.reference.hour;
	minute = pdd.forecast.reference.minute;
	(void) tnorm(&year, &jday, &hour, &minute, &second);
	(void) strcpy(rtime, build_tstamp(year, jday, hour, minute, FALSE, TRUE));

	/* Error return for unrecognizable date or time */
	if ( blank(rtime) )
		{
		(void) fprintf(stderr, "[E1_grib_tstamps] Unrecognizable");
		(void) fprintf(stderr, " century: %d", pdd.forecast.century);
		(void) fprintf(stderr, "  or year: %d", pdd.forecast.reference.year);
		(void) fprintf(stderr, "  or month: %d", pdd.forecast.reference.month);
		(void) fprintf(stderr, "  or day: %d", pdd.forecast.reference.day);
		(void) fprintf(stderr, "  or hour: %d", pdd.forecast.reference.hour);
		(void) fprintf(stderr, "  or minute: %d\n", pdd.forecast.reference.minute);
		return FALSE;
		}

	/* Interpret the valid time(s) */
	switch ( pdd.forecast.range_type )
		{
		/* 0 = forecast or analysis valid at run time + time1 */
		case 0:		ptimeb = pdd.forecast.time1;
					ptimee = 0;
					break;

		/* 1 = analysis valid at run time where time1=0) */
		case 1:		ptimeb = 0;
					ptimee = 0;
					break;

		/* 2 = valid time ranging from run time + time1 to run time + time2 */
		/* 3 = average from run time + time1 to run time + time2            */
		/* 4 = accumulation from run time + time1 to run time + time2       */
		/* 5 = difference of run time + time2 minus run time + time1        */
		case 2:
		case 3:
		case 4:
		case 5:		ptimeb = pdd.forecast.time1;
					ptimee = pdd.forecast.time2;
					break;

		/* 10 = forecast valid at run time plus time1+time2 */
		case 10:	ptimeb = (pdd.forecast.time1 << 8) + pdd.forecast.time2;
					ptimee = 0;
					break;

		/* Error return for unrecognizable time range indicator */
		default:	(void) fprintf(stderr, "[E1_grib_tstamps] Unrecognizable");
					(void) fprintf(stderr, " time range indicator: %d\n",
								pdd.forecast.range_type);
					return FALSE;
		}

	/* Interpret valid time in terms of time units */
	vyearb = vyeare = year;
	vjdayb = vjdaye = jday;
	vhourb = vhoure = hour;
	vminb  = vmine  = minute;
	switch ( pdd.forecast.units )
		{
		/* 254 = seconds (truncated to minutes for now) */
		case 254:	if (ptimeb > 0) vminb += ptimeb/60;
					if (ptimee > 0) vmine += ptimee/60;
					break;

		/* 0   = minutes */
		case 0:		if (ptimeb > 0) vminb += ptimeb;
					if (ptimee > 0) vmine += ptimee;
					break;

		/* 1   = hours */
		case 1:		if (ptimeb > 0) vhourb += ptimeb;
					if (ptimee > 0) vhoure += ptimee;
					break;

		/* 2   = days */
		case 2:		if (ptimeb > 0) vjdayb += ptimeb;
					if (ptimee > 0) vjdaye += ptimee;
					break;

		/* 3   = months */
		case 3:		if (ptimeb > 0)
						{
						vmonth = month + ptimeb;
						vmday  = mday;
						(void) jdate(&vyearb, &vmonth, &vmday, &vjdayb);
						}
					if (ptimee > 0)
						{
						vmonth = month + ptimee;
						vmday  = mday;
						(void) jdate(&vyeare, &vmonth, &vmday, &vjdaye);
						}
					break;

		/* 4   = years */
		case 4:		if (ptimeb > 0) vyearb += ptimeb;
					if (ptimee > 0) vyeare += ptimee;
					break;

		/* 5   = decades */
		case 5:		if (ptimeb > 0) vyearb += ptimeb*10;
					if (ptimee > 0) vyeare += ptimee*10;
					break;

		/* 6   = 30 years */
		case 6:		if (ptimeb > 0) vyearb += ptimeb*30;
					if (ptimee > 0) vyeare += ptimee*30;
					break;

		/* 7   = centuries */
		case 7:		if (ptimeb > 0) vyearb += ptimeb*100;
					if (ptimee > 0) vyeare += ptimee*100;
					break;

		/* Error return for unrecognizable time unit */
		default:	(void) fprintf(stderr, "[E1_grib_tstamps] Unrecognizable");
					(void) fprintf(stderr, " forecast time unit: %d\n",
								pdd.forecast.units);
					return FALSE;
		}

	/* Build valid time stamps */
	if (ptimeb <= 0)
			(void) strcpy(vtimeb, rtime);
	else	{
			(void) tnorm(&vyearb, &vjdayb, &vhourb, &vminb, &vsecb);
			(void) strcpy(vtimeb,
					build_tstamp(vyearb, vjdayb, vhourb, vminb, FALSE, TRUE));
			}
	if (ptimee <= 0)
			(void) strcpy(vtimee, vtimeb);
	else	{
			(void) tnorm(&vyeare, &vjdaye, &vhoure, &vmine, &vsece);
			(void) strcpy(vtimee,
					build_tstamp(vyeare, vjdaye, vhoure, vmine, FALSE, TRUE));
			}

	/* Error return for unrecognizable valid times */
	if ( blank(vtimeb) || blank(vtimee) )
		{
		(void) fprintf(stderr, "[E1_grib_tstamps] Unrecognizable");
		(void) fprintf(stderr, " forecast times:");
		(void) fprintf(stderr, " %d", pdd.forecast.time1);
		(void) fprintf(stderr, "  %d\n", pdd.forecast.time2);
		(void) fprintf(stderr, "  Time range");
		(void) fprintf(stderr, " indicator: %d", pdd.forecast.range_type);
		(void) fprintf(stderr, "  Forecast time");
		(void) fprintf(stderr, " unit: %d\n", pdd.forecast.units);
		return FALSE;
		}

	/* Return for acceptable timestamps found */
	return TRUE;
	}

static	LOGICAL		E1_grib_elements

	(
	E1_Product_definition_data
						pdd,		/* GRIB product definition data */
	const STRING		source,		/* string containing source label */
	STRING				element,	/* string containing element label */
	STRING				units		/* string containing units label */
	)

	{
	int				num, nume, nump;
	static char		did[GRIB_LABEL_LEN];
	static char		eid[GRIB_LABEL_LEN];
	static char		pid[GRIB_LABEL_LEN];

	/* Internal checks for centre and element ids */
	int					nn;
	static int			new=0;
	static short int	*newc=NullShort, *newe=NullShort, *newp=NullShort;
	/*  id[1] = { pTable version, parameter }; */
	int	id[2];
	id[0] = pdd.edition;
	id[1] = pdd.parameter;

	/* Initialise element and units labels */
	(void) strcpy(element, "");
	(void) strcpy(units,   "");

	 if ( ingest_grib_elements(1, source, id, element, units ) ) return TRUE;

	/* Set default for unrecognized element */
	(void) strcpy(did, "gribelement");
	num = strlen(did);
	nume = int_string((int) pdd.edition, eid, (size_t) GRIB_LABEL_LEN);
	nump = int_string((int) pdd.parameter, pid, (size_t) GRIB_LABEL_LEN);
	if ( num > 0 && nume > 0 && nump > 0 && (num+1+nume+1+nump) < GRIB_LABEL_LEN )
		{
		(void) strcpy(element, did);
		(void) strcat(element, ":");
		(void) strcat(element, eid);
		(void) strcat(element, ":");
		(void) strcat(element, pid);

		/* Check if centre and element ids have been saved */
		for (nn=0; nn<new; nn++)
			{
			if ( newc[nn] == pdd.centre_id && newe[nn] == pdd.edition 
					&& newp[nn] == pdd.parameter 
					)
					return TRUE;
			}

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E1_grib_elements] Processing unrecognized");
		(void) fprintf(stderr, " element parameter: %d %d\n", pdd.edition, pdd.parameter);
		(void) fprintf(stderr, "     Edit your Ingest config file to add");
		(void) fprintf(stderr, " this parameter.\n");

		/* Save centre and element ids (to prevent multiple messages) */
		new++;
		newc = GETMEM(newc, short int, new);
		newe = GETMEM(newe, short int, new);
		newp = GETMEM(newp, short int, new);
		newc[new-1] = pdd.centre_id;
		newe[new-1] = pdd.edition;
		newp[new-1] = pdd.parameter;
		return TRUE;
		}

	/* Error return for unrecognizable element parameter */
	(void) fprintf(stderr, "[E1_grib_elements] Unrecognizable");
	(void) fprintf(stderr, " element parameter: %d %d\n",pdd.edition, pdd.parameter);
	return FALSE;
	}

static	LOGICAL	level_label

	(
	STRING		label,
	short int	itop,
	short int	ibot,
	float		scale,
	float		offset,
	STRING		tag
	)

	{
	int		ilev, size;
	float	flev;
	char	lab[GRIB_LABEL_LEN];

	ilev = (itop<<8) + ibot;
	flev = ilev*scale + offset;
	ilev = NINT(flev);
	size = GRIB_LABEL_LEN - strlen(tag);

	if ( int_string(ilev, lab, (size_t) size) )
		{
		(void) strcpy(label, lab);
		(void) strcat(label, tag);
		return TRUE;
		}

	return FALSE;
	}

static	LOGICAL	layer_label

	(
	STRING		label,
	short int	itop,
	short int	ibot,
	float		scale_top,
	float		offset_top,
	float		scale_bot,
	float		offset_bot,
	STRING		tag
	)

	{
	int		jtop, jbot, size;
	float	flev;
	char	ltop[GRIB_LABEL_LEN], lbot[GRIB_LABEL_LEN];

	flev = itop*scale_top + offset_top;
	jtop = NINT(flev);
	flev = ibot*scale_bot + offset_bot;
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

static	LOGICAL		E1_grib_levels

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	const STRING source,	/* Source Label */
	STRING		level		/* string containing level label */
	)

	{
	int				num, numl;
	static char		did[GRIB_LABEL_LEN];
	static char     lid[GRIB_LABEL_LEN];
	/* Internal checks for centre and level ids */
	int					nn;
	static int			new=0;
	static short int	*newl=NullShort;

	LOGICAL			valid;
	int				method;
	float			scale, offset, scale2, offset2;
	static char		tag[GRIB_LABEL_LEN]	= "";

	/* Initialise level */
	(void) strcpy(level, "");

	/* Check through list of levels definitions */
	if ( !ingest_grib_levels(1, source, pdd.layer.type, 
				&method, &scale, &offset, &scale2, &offset2, tag) )
		{
		/* Set default for unrecognized level */
		(void) strcpy(did, "griblevel");
		num = strlen(did);
		numl = int_string((int) pdd.layer.type, lid, (size_t) GRIB_LABEL_LEN);
		if ( num > 0 && numl >0 && (num+1+numl) < GRIB_LABEL_LEN )
			{
			(void) strcpy(level, did);
			(void) strcat(level, ":");
			(void) strcat(level, lid);
	
			/* Check if centre and model ids have been saved */
			for (nn=0; nn<new; nn++)
				{
				if ( newl[nn] == pdd.layer.type ) return TRUE;
				}
	
			/* Write message to request update from FPA development group */
			(void) fprintf(stderr, "[E1_grib_levels] Processing unrecognized");
			(void) fprintf(stderr, " level type: %d\n", pdd.layer.type);
			(void) fprintf(stderr, "     Edit your Ingest config file to add");
			(void) fprintf(stderr, " this parameter.\n");

			/* Save centre and model ids (to prevent multiple messages) */
			new++;
			newl = GETMEM(newl, short int, new);
			newl[new-1] = pdd.layer.type;
			return TRUE;
			}

		/* Error return for unrecognizable level type or values */
		(void) fprintf(stderr, "[E1_grib_levels] Unrecognizable");
		(void) fprintf(stderr, " level type: %d\n", pdd.layer.type);
		return FALSE;
		}

	/* Level definition found in list */
	/* Special levels use no values */
	if ( method == 0 )
			{
			(void) strcpy(level, tag);
			return TRUE;
			}

	/* Single levels use 1 value */
	else if ( method == 1 )
		{
		valid = level_label(level, pdd.layer.top, pdd.layer.bottom,
						scale, offset, tag);
		if ( valid ) return TRUE;
		}

	/* Layers use 2 values */
	else if ( method == 2 )
		{
		valid = layer_label(level, pdd.layer.top, pdd.layer.bottom,
						scale, offset, scale2, offset2, tag);
		if ( valid ) return TRUE;
		}

	/* Error return for unrecognizable level type or values */
	(void) fprintf(stderr, "[E1_grib_levels] Unrecognizable");
	(void) fprintf(stderr, " method: %d", method);
	(void) fprintf(stderr, " for level type: %d\n", pdd.layer.type);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    g r i b _ d a t a _ m a p p r o j                                 *
*                                                                      *
*    Obtain a map projection which describes the data coverage of      *
*    given GRIBFIELD grid definition block.                            *
*                                                                      *
*    This routine is based on Section 2 and Table 1 in the 1991 GRIB   *
*    Edition 1 document WMO Code FM 92-VIII Ext. entitled "The WMO     *
*    Format For The Storage Of Weather Product Information And The     *
*    Exchange Of Weather Product Messages In Gridded Binary Form"      *
*    editted by John D. Stackpole of the U.S. Department of Commerce,  *
*    NMC.                                                              *
*                                                                      *
*    The  resltn  definitions are from Table 7.                        *
*    The  scan_mode  definitions are from Table 8.                     *
*    The  origin/delta  definitions are from Table C.                  *
*                                                                      *
*    Note Edition 1 polar stereographic projections are true at the    *
*    60 degree latitude closest to the pole of projection.             *
*                                                                      *
***********************************************************************/
static	LOGICAL	E1_grib_data_mapproj

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	MAP_PROJ		*mproj,		/* constructed map projection */
	LOGICAL			*left,
	LOGICAL			*bottom
	)

	{
	E1_Grid_description_data	*gdd;
	PROJ_DEF					proj;
	MAP_DEF						map;
	GRID_DEF					grid;
	float						splat, splon, rot;
	float						plat, tlat, slat;
	int							nn;
	STRING						pbuf, mbuf;

	/* Set origins and deltas based on type of GRIB grid */
	gdd = &gribfld->Gdd;
	switch (gdd->dat_rep)
		{

		case LATLON_GRID:			/* LATITUDE-LONGITUDE */

			define_projection(&proj, ProjectLatLon, 0., 0., 0., 0., 0.);

			/* Error message if no increments given for Lat/Long grid */
			if ( !(0x80 & gdd->defn.reg_ll.resltn) )
				{
				(void) pr_error("Grib-Meta", " No increments given for Lat/Long grid\n");
				(void) pr_error(" ", "  La1 = %d  Lo1 = %d  La2 = %d  Lo2 = %d\n",
						gdd->defn.reg_ll.La1, gdd->defn.reg_ll.Lo1,
						gdd->defn.reg_ll.La2, gdd->defn.reg_ll.Lo2);
				(void) pr_error(" ", "  resltn = %d  Ni = %d  Nj = %d  Di = %d  Dj = %d\n",
						gdd->defn.reg_ll.resltn,
						gdd->defn.reg_ll.Ni, gdd->defn.reg_ll.Nj,
						gdd->defn.reg_ll.Di, gdd->defn.reg_ll.Dj);
				return FALSE;
				}

			/* Determine the location of the first grid point */
			if ( gdd->defn.reg_ll.scan_mode.west == 0 )
				*left = (LOGICAL) (gribfld->Dii > 0);
			else
				*left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.reg_ll.scan_mode.north == 0 )
				*bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				*bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive East and positive North */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToDegrees;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToDegrees;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = 1.0;

			/* Set map definition */
			map.olat  = (float) gdd->defn.reg_ll.La1 / GribToDegrees;
			map.olon  = (float) gdd->defn.reg_ll.Lo1 / GribToDegrees;
			map.lref  = 0.0;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (*left)?   0.0: map.xlen;
			map.yorg  = (*bottom)? 0.0: map.ylen;
			map.units = 1.0;

			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */

			/* Set projection to north or south polar stereographic */
			/* >>> assummed to be true at 60N or 60S! <<< */
			if (gdd->defn.ps.proj_centre.pole == 0)
				{
				plat = 90.0;
				tlat = 60.0;
				}
			else
				{
				plat = -90.0;
				tlat = -60.0;
				}
			define_projection(&proj, ProjectPolarSt, plat, tlat, 0., 0., 0.);

			/* Determine the location of the first grid point */
			if ( gdd->defn.ps.scan_mode.west == 0 )
				*left = (LOGICAL) (gribfld->Dii > 0);
			else
				*left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.ps.scan_mode.north == 0 )
				*bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				*bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive right and positive up */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToMeters;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToMeters;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = MetersPerUnit;

			/* Set map definition */
			/* Set origins from latitude/longitude of first location */
			/*  ... if latitude/longitude is given!                  */
			if ( labs(gdd->defn.ps.La1) <= MaxLatitude
				&& labs(gdd->defn.ps.Lo1) <= MaxLongitude )
				{
				map.olat = (float) gdd->defn.ps.La1 / GribToDegrees;
				map.olon = (float) gdd->defn.ps.Lo1 / GribToDegrees;
				}

			/* Otherwise, set origins from location of pole */
			else
				{
				MAP_PROJ	mp;
				POINT		origin;

				origin[X] = -((float) gdd->defn.ps.pole_i) * grid.xgrid
							/ GribToPolePos;
				origin[Y] = -((float) gdd->defn.ps.pole_j) * grid.ygrid
							/ GribToPolePos;
				if (gdd->defn.ps.proj_centre.pole == 0)
					map.olat = 90.0;
				else
					map.olat = -90.0;
				map.olon  = 0.0;
				map.lref  = (float) gdd->defn.ps.LoV / GribToDegrees;
				map.xlen  = grid.xgrid * (grid.nx - 1);
				map.ylen  = grid.ygrid * (grid.ny - 1);
				map.xorg  = (*left)?   0.0: map.xlen;
				map.yorg  = (*bottom)? 0.0: map.ylen;
				map.units = MetersPerUnit;

				define_map_projection(&mp, &proj, &map, &grid);
				pos_to_ll(&mp, origin, &map.olat, &map.olon);
				}

			/* Set reference longitude in degrees East, and */
			/*  parallel to the y axis                      */
			map.lref  = (float) gdd->defn.ps.LoV / GribToDegrees;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (*left)?   0.0: map.xlen;
			map.yorg  = (*bottom)? 0.0: map.ylen;
			map.units = MetersPerUnit;

			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */

			/* Check for unsuppoorted modes */
			if (gdd->defn.lambert.proj_centre.bipolar != 0)
				{
				/* Bi-polar not supported */
				(void) pr_error("Grib-Meta",
					" Bi-polar form of Lambert conformal grid not supported\n");
				return FALSE;
				}

			/* Set projection to N or S Lambert conformal with given */
			/* tangent latitude */
			if (gdd->defn.lambert.proj_centre.pole == 0)
				{
				plat = 90.0;
				/* Check if cone bends north */
				}
			else
				{
				plat = -90.0;
				/* Check if cone bends south */
				}
			tlat = (float) gdd->defn.lambert.Latin1 / GribToDegrees;
			slat = (float) gdd->defn.lambert.Latin2 / GribToDegrees;
			define_projection(&proj, ProjectLambertConf,
						tlat, slat, 0., 0., 0.);

			/* Determine the location of the first grid point */
			if ( gdd->defn.lambert.scan_mode.west == 0 )
				*left = (LOGICAL) (gribfld->Dii > 0);
			else
				*left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.lambert.scan_mode.north == 0 )
				*bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				*bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive right and positive up */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToMeters;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToMeters;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = MetersPerUnit;

			/* Set map definition */
			/* Set origins from latitude/longitude of first location */
			/*  ... if latitude/longitude is given!                  */
			if ( labs(gdd->defn.lambert.La1) <= MaxLatitude
				&& labs(gdd->defn.lambert.Lo1) <= MaxLongitude )
				{
				map.olat = (float) gdd->defn.lambert.La1 / GribToDegrees;
				map.olon = (float) gdd->defn.lambert.Lo1 / GribToDegrees;
				}

			/* Otherwise, set origins from location of pole */
			else
				{
				/* Not supported */
				(void) pr_error("Grib-Meta",
					" Setting origin from pole location for Lambert conformal grid not supported\n");
				return FALSE;
				}

			/* Set reference longitude in degrees East, and */
			/*  parallel to the y axis                      */
			map.lref  = (float) gdd->defn.lambert.LoV / GribToDegrees;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (*left)?   0.0: map.xlen;
			map.yorg  = (*bottom)? 0.0: map.ylen;
			map.units = MetersPerUnit;

			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE GRID */

			/* Set rotated latitude-longitude projection */
			splat = (float) gdd->defn.rotate_ll.LaP / GribToDegrees;
			splon = (float) gdd->defn.rotate_ll.LoP / GribToDegrees;
			rot   = (float) gdd->defn.rotate_ll.AngR / GribToDegrees;

			define_projection(&proj, ProjectLatLonAng, splat, splon, rot,
								0., 0.);

			/* Error message if no increments given for rotated Lat/Long grid */
			if ( !(0x80 & gdd->defn.rotate_ll.resltn) )
				{
				(void) pr_error("Grib-Meta", " No increments given for rotated Lat/Long grid\n");
				(void) pr_error(" ", "  La1 = %d  Lo1 = %d  La2 = %d  Lo2 = %d\n",
						gdd->defn.rotate_ll.La1, gdd->defn.rotate_ll.Lo1,
						gdd->defn.rotate_ll.La2, gdd->defn.rotate_ll.Lo2);
				(void) pr_error(" ", "  resltn = %d  Ni = %d  Nj = %d  Di = %d  Dj = %d\n",
						gdd->defn.rotate_ll.resltn,
						gdd->defn.rotate_ll.Ni, gdd->defn.rotate_ll.Nj,
						gdd->defn.rotate_ll.Di, gdd->defn.rotate_ll.Dj);
				return FALSE;
				}

			/* Determine the location of the first grid point */
			if ( gdd->defn.rotate_ll.scan_mode.west == 0 )
				*left = (LOGICAL) (gribfld->Dii > 0);
			else
				*left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.rotate_ll.scan_mode.north == 0 )
				*bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				*bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive East and positive North */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToDegrees;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToDegrees;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = 1.0;

			/* Set map definition wrt rotated latitude-longitude grid */
			map.olat  = (float) gdd->defn.rotate_ll.La1 / GribToDegrees;
			map.olon  = (float) gdd->defn.rotate_ll.Lo1 / GribToDegrees;
			map.lref  = 0.0;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (*left)?   0.0: map.xlen;
			map.yorg  = (*bottom)? 0.0: map.ylen;
			map.units = 1.0;

			break;

		default:

			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if (gdd->dat_rep == GRIBGridLabels[nn].ident)
					{
					(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if (nn >= NumGRIBGridLabels)
				{
				(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return FALSE;
		}

	/* One last step ... */
	/* This map projection may be compared with another map projection read */
	/*  from an FPA metafile (such as combining u/v component fields).      */
	/* Therefore, we will "write" and "re-read" the projection information  */
	/*  exactly as is done in "write_metafile()" and "read_metafile()" to   */
	/*  ensure that the map projections will compare exactly!               */
	pbuf = format_metafile_projection(&proj);
	mbuf = format_metafile_mapdef(&map, MaxDigits);
	if ( !parse_metafile_projection(pbuf, &proj) ) return FALSE;
	if ( !parse_metafile_mapdef(mbuf, &map) ) return FALSE;

	/* Now define the map projection */
	define_map_projection(mproj, &proj, &map, &grid);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    g r i b _ d a t a _ c o m p o n e n t _ f l a g                   *
*                                                                      *
*    Return flag for determining components of vector fields from      *
*    GRIB data.                                                        *
*                                                                      *
*    The component flag definitions are from Table 7.                  *
*                                                                      *
*    GRIB data expressed in terms of the East and North coordinate     *
*    system returns a component flag of 0.                             *
*    GRIB data expressed in terms of the usual x/y coordinates of      *
*    the GRIB map projection returns a component flag of 1.            *
*                                                                      *
***********************************************************************/
static int		E1_grib_data_component_flag

	(
	GRIBFIELD		gribfld	/* decoded GRIB data */
	)

	{
	E1_Grid_description_data	*gdd;
	int							nn;

	/* Set component flag based on type of GRIB grid */
	gdd = &gribfld.Gdd;
	switch (gdd->dat_rep)
		{

		case LATLON_GRID:			/* LATITUDE-LONGITUDE */

			/* Components set in terms of East and North coordinates */
			/* (Note that x and y are also East and North!)          */
			return 0;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */

			/* Components set in terms of x and y coordinates */
			if ( 0x8 & gdd->defn.ps.compnt ) return 1;

			/* Components set in terms of East and North coordinates */
			else                             return 0;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */

			/* Components set in terms of x and y coordinates */
			if ( 0x8 & gdd->defn.lambert.compnt ) return 1;

			/* Components set in terms of East and North coordinates */
			else                                  return 0;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE GRID */

			/* Components set in terms of rotated x and y coordinates */
			if ( 0x8 & gdd->defn.rotate_ll.resltn) return 1;

			/* Components set in terms of East and North coordinates */
			else                                   return 0;

		default:

			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if (gdd->dat_rep == GRIBGridLabels[nn].ident)
					{
					(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if (nn >= NumGRIBGridLabels)
				{
				(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -1;
		}
	}
/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Section Decodes):                       *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_section0decoder()                                 */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to scan for the string 'GRIB' in a file              */
/*                                                                    */
/***usage      - E1_section0decoder(ip_file, isb);                    */
/*               ip_file is a file pointer                            */
/*               isb is a E1_Indicator_block structure                */
/*                                                                    */
/**********************************************************************/

static	int			E1_section0decoder

	(
	FILE						*ip_file,
	E1_Indicator_block			*isb
	)

	{
	char	ip_char, *required_char;
	int		nb_found, totalchar;

	dprintf(stderr, "\n\n");

	/* Extract Section0 header string */
	totalchar = 0;
	nb_found = 0;
	required_char = GRIB_HEADER;
	while(nb_found < GRIB_HEADER_LENGTH)
		{
		ip_char = fgetc(ip_file);

		/* Set position to next character in this GRIB message      */
		/* This is used to reposition if error occurs during decode */
		GribPosition = ftell(ip_file);

		/* Check for end-of-file or error in search for header string */
		if ( feof(ip_file) )
			{
			if ( totalchar <= 0 )
				dprintf(stderr, "\n  End of GRIB file\n");
			else
				(void) fprintf(stderr, "\n  End-of-file in Section0 header\n");
			return 1;
			}
		if ( ferror(ip_file) )
			{
			(void) fprintf(stderr, "\n  Error in Section0 header\n");
			return -1;
			}

		if ( ++totalchar > 80 )
			{
			totalchar = 1;
			dprintf(stderr, "\n");
			}
		dprintf(stderr, "%c",ip_char);

		/* Check for next character in GRIB header string */
		if ( ip_char == *required_char )
			{
			++nb_found;
			++required_char;
			}
		/* Check for first character in GRIB header string */
		else
			{
			nb_found = 0;
			required_char = GRIB_HEADER;
			if ( ip_char == *required_char )
				{
				++nb_found;
				++required_char;
				}
			}
		}

	/* Extract Section0 data */
	isb->length = fget3c(ip_file);
	isb->edition = fgetc(ip_file);

	/* Check for end-of-file or error in Section0 data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section0 data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section0 data\n");
		return -1;
		}

	dprintf(stderr, "\n  Completed Section0 decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_section1decoder()                                 */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the product definition section            */
/*               and to convert data into integer if required         */
/*                                                                    */
/***usage      - E1_section1decoder(ip_file, pdd);                    */
/*               ip_file is a file pointer                            */
/*               pdd is a Product_definition_data structure           */
/*                                                                    */
/**********************************************************************/

static	int			E1_section1decoder

	(
	FILE						*ip_file,
	E1_Product_definition_data	*pdd
	)

	{
	int		ii;
	Octet	octet;

	/*** GRIB PRODUCT DEFINITION SECTION ***/

	/* Extract Section1 data */
	pdd->length = fget3c(ip_file);
	pdd->edition = fgetc(ip_file);
	pdd->centre_id = fgetc(ip_file);
	pdd->model_id = fgetc(ip_file);
	pdd->grid_defn = fgetc(ip_file);
	octet = fgetc(ip_file);  /* TABLE 1 FLAGS ACCESSED */
	pdd->block_flags.grid_description = GETBIT(octet, E1_block_flag_grid_desc);
	pdd->block_flags.bit_map = GETBIT(octet, E1_block_flag_bit_map);
	pdd->parameter = fgetc(ip_file);
	pdd->layer.type = fgetc(ip_file);
	pdd->layer.top = fgetc(ip_file);
	pdd->layer.bottom = fgetc(ip_file);
	pdd->forecast.reference.year = fgetc(ip_file);
	pdd->forecast.reference.month = fgetc(ip_file);
	pdd->forecast.reference.day = fgetc(ip_file);
	pdd->forecast.reference.hour = fgetc(ip_file);
	pdd->forecast.reference.minute = fgetc(ip_file);
	pdd->forecast.units = fgetc(ip_file);
	pdd->forecast.time1 = fgetc(ip_file);
	pdd->forecast.time2 = fgetc(ip_file);
	pdd->forecast.range_type = fgetc(ip_file);
	pdd->forecast.nb_averaged = fget2c(ip_file);
	pdd->forecast.nb_missing = fgetc(ip_file);
	pdd->forecast.century = fgetc(ip_file);
	pdd->forecast.reserved = fgetc(ip_file);
	pdd->forecast.factor_d = fget2c(ip_file);
	if (0x8000 & pdd->forecast.factor_d)
		pdd->forecast.factor_d = 0 - (pdd->forecast.factor_d & 0x7fff);

	/* Check for proper length of Section1 data */
	/*  ... and read unused bytes               */
	if ( pdd->length < PDB_LENGTH || pdd->length > PDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section1 length: %d\n",
				pdd->length);
		return -2;
		}
	for ( ii = PDB_LENGTH; ii < pdd->length; ii++ ) (void) fgetc(ip_file);

	/* Check for end-of-file or error in Section1 data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section1 data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section1 data\n");
		return -1;
		}

	dprintf(stderr, "  Completed Section1 decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_latlongdecoder()                                  */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the description section for a lat-long    */
/*               grid and to convert data into integer if required    */
/*                                                                    */
/***usage      - E1_latlongdecoder(ip_file, gdd);                     */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_latlongdecoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	Octet				octet;
	int					e1_scancode, icode;
	int					ii, ipts, iloc;
	static short int	*thinpts = NullShort;

	/* Extract Section2 latlong grid data */

	/*  ... Octet 7-8, 9-10 */
	gdd->defn.reg_ll.Ni = fget2c(ip_file);
	gdd->defn.reg_ll.Nj = fget2c(ip_file);

	/*  ... Octet 11-13, 14-16 */
	gdd->defn.reg_ll.La1 = fget3c(ip_file);
	if (gdd->defn.reg_ll.La1 >= 0x800000)
		gdd->defn.reg_ll.La1 = 0x800000 - gdd->defn.reg_ll.La1;
	gdd->defn.reg_ll.Lo1 = fget3c(ip_file);
	if (gdd->defn.reg_ll.Lo1 >= 0x800000)
		gdd->defn.reg_ll.Lo1 = 0x800000 - gdd->defn.reg_ll.Lo1;

	/*  ... Octet 17 */
	gdd->defn.reg_ll.resltn = fgetc(ip_file);

	/*  ... Octet 18-20, 21-23 */
	gdd->defn.reg_ll.La2 = fget3c(ip_file);
	if (gdd->defn.reg_ll.La2 >= 0x800000)
		gdd->defn.reg_ll.La2 = 0x800000 - gdd->defn.reg_ll.La2;
	gdd->defn.reg_ll.Lo2 = fget3c(ip_file);
	if (gdd->defn.reg_ll.Lo2 >= 0x800000)
		gdd->defn.reg_ll.Lo2 = 0x800000 - gdd->defn.reg_ll.Lo2;

	/*  ... Octet 24-25, 26-27 */
	gdd->defn.reg_ll.Di = fget2c(ip_file);
	gdd->defn.reg_ll.Dj = fget2c(ip_file);

	/*  ... Octet 28 */
	octet = fgetc(ip_file);
	gdd->defn.reg_ll.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.reg_ll.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.reg_ll.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* The following test on i and scan code is an adjustment of */
	/*  errors made by the encoder of other countries   */
	icode = (int) octet;
	e1_scancode = (int) octet >> 5;
	if ( e1_scancode == 0 && icode != 0 )
		{
		(void) fprintf(stderr, "\n*****WARNING: ERROR IN LOWER FIVE BITS");
		(void) fprintf(stderr, " OF SCAN CODE ****\n");
		}

	/* Set flag for extra pole datum (only found in pre-defined grids) */
	gdd->defn.reg_ll.pole_extra = 0;

	/* Initialize flag for extra meridian data */
	gdd->defn.reg_ll.meridian_extra = 0;

	/* Check for proper length of Section2 latitude-longitude grids */
	if ( gdd->length < GDB_LATLON_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 latlong length: %d\n",
				gdd->length);
		return -2;
		}

	/* Process regular latitude-longitude grids */
	if ( gdd->pv_or_pl == 255 )
		{
		/* Set indicator for regular latitude-longitude grids */
		gdd->defn.reg_ll.thin_mode = 0;
		gdd->defn.reg_ll.thin_pts = NullShort;

		/* Read unused bytes for regular latitude-longitude grids */
		/*   ... Octet 29 - gdd->length                           */
		for ( ii = GDB_LATLON_LENGTH; ii < gdd->length; ii++ )
				(void) fgetc(ip_file);
		}

	/* Process quasi-regular latitude-longitude grids */
	/*  and/or vertical coordinate parameters         */
	else if ( gdd->pv_or_pl < gdd->length )
		{
		/* Read unused bytes before additional parameters */
		/*   ... Octet 29 - (gdd->pv_or_pl - 1)           */
		for ( ii = GDB_LATLON_LENGTH; ii < (gdd->pv_or_pl - 1); ii++ )
				(void) fgetc(ip_file);

		/* Set parameters for quasi-regular rows */
		if ( gdd->defn.reg_ll.Ni == 65535
				&& gdd->defn.reg_ll.Di == 65535 )
			{
			ipts = gdd->defn.reg_ll.Nj;
			gdd->defn.reg_ll.thin_mode = 1;
			}

		/* Set parameters for quasi-regular columns */
		else if ( gdd->defn.reg_ll.Nj == 65535
				&& gdd->defn.reg_ll.Dj == 65535 )
			{
			ipts = gdd->defn.reg_ll.Ni;
			gdd->defn.reg_ll.thin_mode = 2;
			}

		/* Set parameters for regular grid */
		else
			{
			ipts = 0;
			gdd->defn.reg_ll.thin_mode = 0;
			gdd->defn.reg_ll.thin_pts = NullShort;
			}

		/* First read vertical co-ordinate parameters */
		if ( gdd->nv > 0 )
			{

			/* >>> Vertical co-ordinate parameters not saved!!! <<< */
			for ( ii = 0; ii < gdd->nv; ii++ )
				{
				(void) dfget4c(ip_file);
				}
			}

		/* Now read number of points in each row/column of quasi-regular grid */
		if ( ipts > 0 )
			{
			thinpts = GETMEM(thinpts, short int, ipts);
			for ( ii = 0; ii < ipts; ii++ ) thinpts[ii] = fget2c(ip_file);
			gdd->defn.reg_ll.thin_pts = thinpts;
			}

		/* Read all remaining unused bytes */
		iloc = gdd->pv_or_pl + (gdd->nv)*4 + ipts*2 - 1;
		for ( ii = iloc; ii < gdd->length; ii++ ) (void) fgetc(ip_file);
		}

	else
		{
		(void) fprintf(stderr, "\n***WARNING*** No data for quasi-regular");
		(void) fprintf(stderr, " lat-long grid (or vertical coordinates)!\n");
		return -4;
		}

	/* Check for end-of-file or error in Section2 latlong grid data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 latlong grid data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 latlong grid data\n");
		return -1;
		}

	dprintf(stderr, "    Completed latlong grid decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_gaussecoder()                                     */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the description section for a gaussian    */
/*               grid and to convert data into integer if required    */
/*                                                                    */
/***usage      - E1_gaussdecoder(ip_file, gdd);                       */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/***notes :                                                           */
/*          this routine has not been tested!                         */
/*                                                                    */
/**********************************************************************/

static	int			E1_gaussdecoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	Octet				octet;
	int					ii, ipts, iloc;
	static short int	*thinpts = NullShort;

	/* Extract Section2 gaussian grid data */

	/*  ... Octet 7-8, 9-10 */
	gdd->defn.guas_ll.Ni = fget2c(ip_file);
	gdd->defn.guas_ll.Nj = fget2c(ip_file);

	/*  ... Octet 11-13, 14-16 */
	gdd->defn.guas_ll.La1 = fget3c(ip_file);
	if (gdd->defn.guas_ll.La1 >= 0x800000)
		gdd->defn.guas_ll.La1 = 0x800000 - gdd->defn.guas_ll.La1;
	gdd->defn.guas_ll.Lo1 = fget3c(ip_file);
	if (gdd->defn.guas_ll.Lo1 >= 0x800000)
		gdd->defn.guas_ll.Lo1 = 0x800000 - gdd->defn.guas_ll.Lo1;

	/*  ... Octet 17 */
	gdd->defn.guas_ll.resltn = fgetc(ip_file);

	/*  ... Octet 18-20, 21-23 */
	gdd->defn.guas_ll.La2 = fget3c(ip_file);
	if (gdd->defn.guas_ll.La2 >= 0x800000)
		gdd->defn.guas_ll.La2 = 0x800000 - gdd->defn.guas_ll.La2;
	gdd->defn.guas_ll.Lo2 = fget3c(ip_file);
	if (gdd->defn.guas_ll.Lo2 >= 0x800000)
		gdd->defn.guas_ll.Lo2 = 0x800000 - gdd->defn.guas_ll.Lo2;

	/*  ... Octet 24-25, 26-27 */
	gdd->defn.guas_ll.Di = fget2c(ip_file);
	gdd->defn.guas_ll.N = fget2c(ip_file);

	/*  ... Octet 28 */
	octet = fgetc(ip_file);
	gdd->defn.guas_ll.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.guas_ll.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.guas_ll.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* Check for proper length of Section2 gaussian grids */
	if ( gdd->length < GDB_GAUSS_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 gaussian length: %d\n",
				gdd->length);
		return -2;
		}

	/* Process regular gaussian grids */
	if ( gdd->pv_or_pl == 255 )
		{
		/* Set indicator for regular gaussian grids */
		gdd->defn.guas_ll.thin_mode = 0;
		gdd->defn.guas_ll.thin_pts = NullShort;

		/* Read unused bytes for regular gaussian grids */
		/*   ... Octet 29 - gdd->length                 */
		for ( ii = GDB_GAUSS_LENGTH; ii < gdd->length; ii++ )
				(void) fgetc(ip_file);
		}

	/* Process quasi-regular gaussian grids   */
	/*  and/or vertical coordinate parameters */
	else if ( gdd->pv_or_pl < gdd->length )
		{
		/* Read unused bytes before additional parameters */
		/*   ... Octet 29 - (gdd->pv_or_pl - 1)           */
		for ( ii = GDB_GAUSS_LENGTH; ii < (gdd->pv_or_pl - 1); ii++ )
				(void) fgetc(ip_file);

		/* Set parameters for quasi-regular rows */
		if ( gdd->defn.guas_ll.Ni == 65535
				&& gdd->defn.guas_ll.Di == 65535 )
			{
			ipts = gdd->defn.guas_ll.Nj;
			gdd->defn.guas_ll.thin_mode = 1;
			}

		/* Set parameters for regular grid */
		else
			{
			ipts = 0;
			gdd->defn.guas_ll.thin_mode = 0;
			gdd->defn.guas_ll.thin_pts = NullShort;
			}

		/* First read vertical co-ordinate parameters */
		if ( gdd->nv > 0 )
			{

			/* >>> Vertical co-ordinate parameters not saved!!! <<< */
			for ( ii = 0; ii < gdd->nv; ii++ )
				{
				(void) dfget4c(ip_file);
				}
			}

		/* Now read number of points in each row/column of quasi-regular grid */
		if ( ipts > 0 )
			{
			thinpts = GETMEM(thinpts, short int, ipts);
			for ( ii = 0; ii < ipts; ii++ ) thinpts[ii] = fget2c(ip_file);
			gdd->defn.guas_ll.thin_pts = thinpts;
			}


		/* Read all remaining unused bytes */
		iloc = gdd->pv_or_pl + (gdd->nv)*4 + ipts*2 - 1;
		for ( ii = iloc; ii < gdd->length; ii++ ) (void) fgetc(ip_file);
		}

	else
		{
		(void) fprintf(stderr, "\n***WARNING*** No data for quasi-regular");
		(void) fprintf(stderr, " guassian grid (or vertical coordinates)!\n");
		return -4;
		}

	/* Check for end-of-file or error in Section2 gaussian grid data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 gaussian grid data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 gaussian grid data\n");
		return -1;
		}

	dprintf(stderr, "    Completed gaussian decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_psdecoder()                                       */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the description section for a polar       */
/*               stereographic grid and to convert data into integer  */
/*               if required                                          */
/*                                                                    */
/***usage      - E1_psdecoder(ip_file, gdd);                          */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_psdecoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	Octet				octet;
	int		ii, iloc;

	/* Extract Section2 polar steriographic grid data */

	/*  ... Octet 7-8, 9-10 */
	gdd->defn.ps.Nx = fget2c(ip_file);
	gdd->defn.ps.Ny = fget2c(ip_file);

	/*  ... Octet 11-13, 14-16 */
	gdd->defn.ps.La1 = fget3c(ip_file);
	if (gdd->defn.ps.La1 >= 0x800000)
		gdd->defn.ps.La1 = 0x800000 - gdd->defn.ps.La1;
	gdd->defn.ps.Lo1 = fget3c(ip_file);
	if (gdd->defn.ps.Lo1 >= 0x800000)
		gdd->defn.ps.Lo1 = 0x800000 -  gdd->defn.ps.Lo1;

	/*  ... Octet 17 */
	gdd->defn.ps.compnt = fgetc(ip_file);

	/*  ... Octet 18-20 */
	gdd->defn.ps.LoV = fget3c(ip_file);
	if (gdd->defn.ps.LoV >= 0x800000)
		gdd->defn.ps.LoV = 0x800000 -  gdd->defn.ps.LoV;

	/*  ... Octet 21-23, 24-26 */
	gdd->defn.ps.Dx = fget3c(ip_file);
	gdd->defn.ps.Dy = fget3c(ip_file);

	/*  ... Octet 27, 28 */
	octet = fgetc(ip_file);
	gdd->defn.ps.proj_centre.pole = GETBIT(octet, E1_proj_flag_pole);
	octet = fgetc(ip_file);
	gdd->defn.ps.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.ps.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.ps.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* Check for proper length of Section2 polar stereographic grids */
	if ( gdd->length < GDB_PSTEREO_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 pstereo length: %d\n",
				gdd->length);
		return -2;
		}

	/* Read unused bytes for polar stereographic grids */
	/*   ... Octet 29 - gdd->length                    */
	if ( gdd->pv_or_pl == 255 )
		{
		for ( ii = GDB_PSTEREO_LENGTH; ii < gdd->length; ii++ )
				(void) fgetc(ip_file);
		}

	/* Read vertical coordinate parameters for polar stereographic grids */
	else if ( gdd->pv_or_pl < gdd->length )
		{
		/* Read unused bytes before additional parameters */
		/*   ... Octet 29 - (gdd->pv_or_pl - 1)           */
		for ( ii = GDB_PSTEREO_LENGTH; ii < (gdd->pv_or_pl - 1); ii++ )
				(void) fgetc(ip_file);

		/* >>> Vertical co-ordinate parameters not saved!!! <<< */
		for ( ii = 0; ii < gdd->nv; ii++ )
			{
			(void) dfget4c(ip_file);
			}

		/* Read all remaining unused bytes */
		iloc = gdd->pv_or_pl + (gdd->nv)*4 - 1;
		for ( ii = iloc; ii < gdd->length; ii++ ) (void) fgetc(ip_file);
		}

	else
		{
		(void) fprintf(stderr, "\n***WARNING*** No data for ");
		(void) fprintf(stderr, " pstereo grid vertical coordinates!\n");
		return -4;
		}

	/* Check for end-of-file or error in Section2 pstereo grid data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 pstereo grid data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 pstereo grid data\n");
		return -1;
		}

	dprintf(stderr, "    Completed pstereo decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_lambertdecoder()                                  */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the description section for a Lambert     */
/*               Conformal grid and to convert data into integer if   */
/*               required                                             */
/*                                                                    */
/***usage      - E1_lambertdecoder(ip_file, gdd);                     */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_lambertdecoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	Octet				octet;
	int		ii, iloc;

	/* Extract Section2 Lambert conformal grid data */

	/*  ... Octet 7-8, 9-10 */
	gdd->defn.lambert.Nx = fget2c(ip_file);
	gdd->defn.lambert.Ny = fget2c(ip_file);

	/*  ... Octet 11-13, 14-16 */
	gdd->defn.lambert.La1 = fget3c(ip_file);
	if (gdd->defn.lambert.La1 >= 0x800000)
		gdd->defn.lambert.La1 = 0x800000 - gdd->defn.lambert.La1;
	gdd->defn.lambert.Lo1 = fget3c(ip_file);
	if (gdd->defn.lambert.Lo1 >= 0x800000)
		gdd->defn.lambert.Lo1 = 0x800000 -  gdd->defn.lambert.Lo1;

	/*  ... Octet 17 */
	gdd->defn.lambert.compnt = fgetc(ip_file);

	/*  ... Octet 18-20 */
	gdd->defn.lambert.LoV = fget3c(ip_file);
	if (gdd->defn.lambert.LoV >= 0x800000)
		gdd->defn.lambert.LoV = 0x800000 -  gdd->defn.lambert.LoV;

	/*  ... Octet 21-23, 24-26 */
	gdd->defn.lambert.Dx = fget3c(ip_file);
	gdd->defn.lambert.Dy = fget3c(ip_file);

	/*  ... Octet 27, 28 */
	octet = fgetc(ip_file);
	gdd->defn.lambert.proj_centre.pole    = GETBIT(octet, E1_proj_flag_pole);
	gdd->defn.lambert.proj_centre.bipolar = GETBIT(octet, E1_proj_flag_bipolar);
	octet = fgetc(ip_file);
	gdd->defn.lambert.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.lambert.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.lambert.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/*  ... Octet 29-31, 32-34 */
	gdd->defn.lambert.Latin1 = fget3c(ip_file);
	if (gdd->defn.lambert.Latin1 >= 0x800000)
		gdd->defn.lambert.Latin1 = 0x800000 - gdd->defn.lambert.Latin1;
	gdd->defn.lambert.Latin2 = fget3c(ip_file);
	if (gdd->defn.lambert.Latin2 >= 0x800000)
		gdd->defn.lambert.Latin2 = 0x800000 - gdd->defn.lambert.Latin2;

	/* Check for proper length of Section2 lambert conformal grids */
	if ( gdd->length < GDB_LAMBERTC_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 lambertc length: %d\n",
				gdd->length);
		return -2;
		}

	/* Read unused bytes for Lambert conformal grids */
	/*   ... Octet 35 - gdd->length                  */
	if ( gdd->pv_or_pl == 255 )
		{
		for ( ii = GDB_LAMBERTC_LENGTH; ii < gdd->length; ii++ )
				(void) fgetc(ip_file);
		}

	/* Read vertical coordinate parameters for Lambert conformal grids */
	else if ( gdd->pv_or_pl < gdd->length )
		{
		/* Read unused bytes before additional parameters */
		/*   ... Octet 29 - (gdd->pv_or_pl - 1)           */
		for ( ii = GDB_LAMBERTC_LENGTH; ii < (gdd->pv_or_pl - 1); ii++ )
				(void) fgetc(ip_file);

		/* >>> Vertical co-ordinate parameters not saved!!! <<< */
		for ( ii = 0; ii < gdd->nv; ii++ )
			{
			(void) dfget4c(ip_file);
			}

		/* Read all remaining unused bytes */
		iloc = gdd->pv_or_pl + (gdd->nv)*4 - 1;
		for ( ii = iloc; ii < gdd->length; ii++ ) (void) fgetc(ip_file);
		}

	else
		{
		(void) fprintf(stderr, "\n***WARNING*** No data for ");
		(void) fprintf(stderr, " lambertc grid vertical coordinates!\n");
		return -4;
		}

	/* Check for end-of-file or error in Section2 lambertc grid data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 lambertc grid data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 lambertc grid data\n");
		return -1;
		}

	dprintf(stderr, "    Completed lambertc decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_rotatedlatlongdecoder()                           */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the description section for a rotated     */
/*               lat-long grid and to convert data into integer if    */
/*               required                                             */
/*                                                                    */
/***usage      - E1_rotatedlatlongdecoder(ip_file, gdd);              */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_rotatedlatlongdecoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	Octet				octet;
	int					e1_scancode, icode;
	int					ii, ipts, iloc;
	static short int	*thinpts = NullShort;

	/* Extract Section2 rotated latlong grid data */

	/*  ... Octet 7-8, 9-10 */
	gdd->defn.rotate_ll.Ni = fget2c(ip_file);
	gdd->defn.rotate_ll.Nj = fget2c(ip_file);

	/*  ... Octet 11-13, 14-16 */
	gdd->defn.rotate_ll.La1 = fget3c(ip_file);
	if (gdd->defn.rotate_ll.La1 >= 0x800000)
		gdd->defn.rotate_ll.La1 = 0x800000 - gdd->defn.rotate_ll.La1;
	gdd->defn.rotate_ll.Lo1 = fget3c(ip_file);
	if (gdd->defn.rotate_ll.Lo1 >= 0x800000)
		gdd->defn.rotate_ll.Lo1 = 0x800000 - gdd->defn.rotate_ll.Lo1;

	/*  ... Octet 17 */
	gdd->defn.rotate_ll.resltn = fgetc(ip_file);

	/*  ... Octet 18-20, 21-23 */
	gdd->defn.rotate_ll.La2 = fget3c(ip_file);
	if (gdd->defn.rotate_ll.La2 >= 0x800000)
		gdd->defn.rotate_ll.La2 = 0x800000 - gdd->defn.rotate_ll.La2;
	gdd->defn.rotate_ll.Lo2 = fget3c(ip_file);
	if (gdd->defn.rotate_ll.Lo2 >= 0x800000)
		gdd->defn.rotate_ll.Lo2 = 0x800000 - gdd->defn.rotate_ll.Lo2;

	/*  ... Octet 24-25, 26-27 */
	gdd->defn.rotate_ll.Di = fget2c(ip_file);
	gdd->defn.rotate_ll.Dj = fget2c(ip_file);

	/*  ... Octet 28 */
	octet = fgetc(ip_file);
	gdd->defn.rotate_ll.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.rotate_ll.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.rotate_ll.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* The following test on i and scan code is an adjustment of */
	/*  errors made by the encoder of other countries   */
	icode = (int) octet;
	e1_scancode = (int) octet >> 5;
	if ( e1_scancode == 0 && icode != 0 )
		{
		(void) fprintf(stderr, "\n*****WARNING: ERROR IN LOWER FIVE BITS");
		(void) fprintf(stderr, " OF SCAN CODE ****\n");
		}

	/* Skip unused bits */
	/*  ... Octet 29-32  */
	(void) fget4c(ip_file);

	/*  ... Octet 33-35, 36-38 */
	gdd->defn.rotate_ll.LaP = fget3c(ip_file);
	if (gdd->defn.rotate_ll.LaP >= 0x800000)
		gdd->defn.rotate_ll.LaP = 0x800000 - gdd->defn.rotate_ll.LaP;
	gdd->defn.rotate_ll.LoP = fget3c(ip_file);
	if (gdd->defn.rotate_ll.LoP >= 0x800000)
		gdd->defn.rotate_ll.LoP = 0x800000 - gdd->defn.rotate_ll.LoP;

	/*  ... Octet 39-42 */
	gdd->defn.rotate_ll.AngR = dfget4c(ip_file);

	/* Set flag for extra pole datum (only found in pre-defined grids) */
	gdd->defn.rotate_ll.pole_extra = 0;

	/* Initialize flag for extra meridian data */
	gdd->defn.rotate_ll.meridian_extra = 0;

	/* Check for proper length of Section2 rotated latitude-longitude grids */
	if ( gdd->length < GDB_ROTATED_LATLON_LENGTH
			|| gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 rotated latlong length: %d\n",
				gdd->length);
		return -2;
		}

	/* Process regular rotated latitude-longitude grids */
	if ( gdd->pv_or_pl == 255 )
		{
		/* Set indicator for regular latitude-longitude grids */
		gdd->defn.rotate_ll.thin_mode = 0;
		gdd->defn.rotate_ll.thin_pts = NullShort;

		/* Read unused bytes for regular rotated latitude-longitude grids */
		/*  ... Octet 43 - gdd->length                                    */
		for ( ii = GDB_ROTATED_LATLON_LENGTH; ii < gdd->length; ii++ )
				(void) fgetc(ip_file);
		}

	/* Process quasi-regular rotated latitude-longitude grids */
	/*  and/or vertical coordinate parameters                 */
	else if ( gdd->pv_or_pl < gdd->length )
		{
		/* Read unused bytes before additional parameters */
		/*   ... Octet 43 - (gdd->pv_or_pl - 1)           */
		for ( ii = GDB_ROTATED_LATLON_LENGTH; ii < (gdd->pv_or_pl - 1); ii++ )
				(void) fgetc(ip_file);

		/* Set parameters for quasi-regular rows */
		if ( gdd->defn.rotate_ll.Ni == 65535
				&& gdd->defn.rotate_ll.Di == 65535 )
			{
			ipts = gdd->defn.rotate_ll.Nj;
			gdd->defn.rotate_ll.thin_mode = 1;
			}

		/* Set parameters for quasi-regular columns */
		else if ( gdd->defn.rotate_ll.Nj == 65535
				&& gdd->defn.rotate_ll.Dj == 65535 )
			{
			ipts = gdd->defn.rotate_ll.Ni;
			gdd->defn.rotate_ll.thin_mode = 2;
			}

		/* Set parameters for regular grid */
		else
			{
			ipts = 0;
			gdd->defn.rotate_ll.thin_mode = 0;
			gdd->defn.rotate_ll.thin_pts = NullShort;
			}

		/* First read vertical co-ordinate parameters */
		if ( gdd->nv > 0 )
			{

			/* >>> Vertical co-ordinate parameters not saved!!! <<< */
			for ( ii = 0; ii < gdd->nv; ii++ )
				{
				(void) dfget4c(ip_file);
				}
			}

		/* Now read number of points in each row/column of quasi-regular grid */
		if ( ipts > 0 )
			{
			thinpts = GETMEM(thinpts, short int, ipts);
			for ( ii = 0; ii < ipts; ii++ ) thinpts[ii] = fget2c(ip_file);
			gdd->defn.rotate_ll.thin_pts = thinpts;
			}

		/* Read all remaining unused bytes */
		iloc = gdd->pv_or_pl + (gdd->nv)*4 + ipts*2 - 1;
		for ( ii = iloc; ii < gdd->length; ii++ ) (void) fgetc(ip_file);
		}

	else
		{
		(void) fprintf(stderr, "\n***WARNING*** No data for quasi-regular");
		(void) fprintf(stderr, " rotated lat-long grid (or vertical coordinates)!\n");
		return -4;
		}

	/* Check for end-of-file or error in Section2 rotated latlong grid data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 rotated latlong grid data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 rotated latlong grid data\n");
		return -1;
		}

	dprintf(stderr, "    Completed rotated latlong grid decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_section2decoder()                                 */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the grid description section              */
/*               and to convert data into integer if required         */
/*                                                                    */
/***usage      - E1_section2decoder(ip_file, gdd);                    */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_section2decoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd
	)

	{
	int		iret = 0, nn;

	/* Extract Section2 data */
	gdd->length = fget3c(ip_file);
	gdd->nv = fgetc(ip_file);
	gdd->pv_or_pl = fgetc(ip_file);
	gdd->dat_rep = fgetc(ip_file);

	/* Check for end-of-file or error in Section2 data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section2 data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section2 data\n");
		return -1;
		}

	/* Extract Section2 grid data */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE GRID */
			iret = E1_latlongdecoder(ip_file,gdd);
			break;

		case GAUSS_GRID:			/* GAUSSIAN GRID */
			iret = E1_gaussdecoder(ip_file,gdd);
			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC GRID */
			iret = E1_psdecoder(ip_file,gdd);
			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL GRID */
			iret = E1_lambertdecoder(ip_file,gdd);
			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE GRID */
			iret = E1_rotatedlatlongdecoder(ip_file,gdd);
			break;

		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " Section2 grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized Section2 grid type: %d\n",
						gdd->dat_rep);
				}
			iret = 2;
			break;
		}

	if ( iret != 0 ) return iret;

	dprintf(stderr, "  Completed Section2 decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_gdbdefaultrtn()                                   */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - This routine is called when GDB is not available     */
/*               and the grid is one of the standard grid.            */
/*               It will fill in the GDB information                  */
/*                                                                    */
/***usage      - i = E1_gdbdefaultrtn(pdd, gdd);                      */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*                                                                    */
/**********************************************************************/

static	int			E1_gdbdefaultrtn

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd
	)

	{
	/* Pointers into predefined grid tables */
	const E1_ll_grid_predefinition	*predef_ll = NullPtr(E1_ll_grid_predefinition *);
	const E1_ps_grid_predefinition	*predef_ps = NullPtr(E1_ps_grid_predefinition *);
	int								predef_index;

	/* Set default grid description data */
	gdd->length = 0;
	gdd->nv = 0;
	gdd->pv_or_pl = 255;

	/* Set grid description for latitude-longitude grids */
	for(predef_index = 0; predef_index < E1_nb_predef_ll_grids; predef_index++)
		if ( E1_predef_ll_grids[predef_index].grid_defn == pdd->grid_defn )
			{
			/** LATITUDE-LONGITUDE **/

			predef_ll = E1_predef_ll_grids + predef_index;

			gdd->dat_rep = predef_ll->dat_rep;

			gdd->defn.reg_ll.Ni = predef_ll->Ni;
			gdd->defn.reg_ll.Nj = predef_ll->Nj;
			gdd->defn.reg_ll.La1 = predef_ll->La1;
			gdd->defn.reg_ll.Lo1 = predef_ll->Lo1;
			gdd->defn.reg_ll.resltn = predef_ll->resltn;
			gdd->defn.reg_ll.La2 = predef_ll->La2;
			gdd->defn.reg_ll.Lo2 = predef_ll->Lo2;
			gdd->defn.reg_ll.Di = predef_ll->Di;
			gdd->defn.reg_ll.Dj = predef_ll->Dj;

			gdd->defn.reg_ll.scan_mode.west       = predef_ll->scan_mode.west;
			gdd->defn.reg_ll.scan_mode.north      = predef_ll->scan_mode.north;
			gdd->defn.reg_ll.scan_mode.horz_sweep = predef_ll->scan_mode.horz_sweep;

			/* Set flag for regular latitude-longitude grid */
			gdd->defn.reg_ll.thin_mode = 0;
			gdd->defn.reg_ll.thin_pts = NullShort;

			/* Set flag for extra pole datum */
			gdd->defn.reg_ll.pole_extra = predef_ll->pole_extra;

			/* Initialize flag for extra meridian data */
			gdd->defn.reg_ll.meridian_extra = 0;
			}

	/* Return now if latitude-longitude grid was found */
	if ( NotNull(predef_ll) ) return 0;

	/* Set grid description for polar stereographic grids */
	for(predef_index = 0; predef_index < E1_nb_predef_ps_grids; predef_index++)
		if ( E1_predef_ps_grids[predef_index].grid_defn == pdd->grid_defn )
			{
			/** POLAR STEREOGRAPHIC **/

			predef_ps = E1_predef_ps_grids + predef_index;

			gdd->dat_rep = predef_ps->dat_rep;
			gdd->defn.ps.Nx = predef_ps->Nx;
			gdd->defn.ps.Ny = predef_ps->Ny;
			gdd->defn.ps.La1 = predef_ps->La1;
			gdd->defn.ps.Lo1 = predef_ps->Lo1;
			gdd->defn.ps.compnt = predef_ps->compnt;
			gdd->defn.ps.LoV = predef_ps->LoV;
			gdd->defn.ps.Dx = predef_ps->Dx;
			gdd->defn.ps.Dy = predef_ps->Dy;

			gdd->defn.ps.proj_centre.pole     = predef_ps->proj_centre.pole;

			gdd->defn.ps.scan_mode.west       = predef_ps->scan_mode.west;
			gdd->defn.ps.scan_mode.north      = predef_ps->scan_mode.north;
			gdd->defn.ps.scan_mode.horz_sweep = predef_ps->scan_mode.horz_sweep;

			gdd->defn.ps.pole_i = predef_ps->pole_i;
			gdd->defn.ps.pole_j = predef_ps->pole_j;
			}

	/* Return now if polar stereographic grid was found */
	if ( NotNull(predef_ps) ) return 0;

	/* Error return if no grid definition found */
	(void) fprintf(stderr, "No grid definition for grid number %d\n",
			pdd->grid_defn);
	return 1;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_set_grid()                                        */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to set the grid parameters for GRIB data             */
/*                                                                    */
/***usage      - E1_set_grid(pdd, gdd, numgrid, nii, njj, dii, djj);  */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               numgrid is the number of grid points in GRIB data    */
/*               nii is the column dimension of data                  */
/*               njj is the row dimension of data                     */
/*               dii is the column spacing of data                    */
/*               djj is the row spacing of data                       */
/*                                                                    */
/**********************************************************************/

static	int			E1_set_grid

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	unsigned int				*numgrid,
	int							*nii,
	int							*njj,
	int							*dii,
	int							*djj
	)

	{

	int				ni, nj, nn, di, dj;
	short int		ithin, npts;
	unsigned int	num_vals;

	/* Initialize pointers for returned grid dimensions */
	*numgrid = 0;
	*nii = 0;
	*njj = 0;
	*dii = 0;
	*djj = 0;

	/* Set grid dimensions */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			ni = gdd->defn.reg_ll.Ni;
			nj = gdd->defn.reg_ll.Nj;
			di = gdd->defn.reg_ll.Di;
			dj = gdd->defn.reg_ll.Dj;
			ithin = gdd->defn.reg_ll.thin_mode;

			/* Regular latitude-longitude grids */
			if ( ithin == 0 )
				{
				/* Set number of data values */
				num_vals = ni * nj;

				/* Check column increment */
				if ( di == 65535 )
					{
					di = E1_set_lon_increment(ni,
							gdd->defn.reg_ll.Lo1, gdd->defn.reg_ll.Lo2,
							gdd->defn.reg_ll.scan_mode.west);
					}

				/* Check row increment */
				if ( dj == 65535 )
					{
					dj = E1_set_lat_increment(nj,
							gdd->defn.reg_ll.La1, gdd->defn.reg_ll.La2,
							gdd->defn.reg_ll.scan_mode.north);
					}
				}

			/* Quasi-regular latitude-longitude grids */
			else if ( ithin == 1 )
				{
				/* Set maximum number of columns and number of data values */
				for (ni=0, num_vals=0, nn=0; nn<nj; nn++)
					{
					npts = gdd->defn.reg_ll.thin_pts[nn];
					if ( npts > ni ) ni = npts;
					num_vals += npts;
					}

				/* Set column increment */
				di = E1_set_lon_increment(ni,
						gdd->defn.reg_ll.Lo1, gdd->defn.reg_ll.Lo2,
						gdd->defn.reg_ll.scan_mode.west);

				/* Check row increment */
				if ( dj == 65535 )
					{
					dj = E1_set_lat_increment(nj,
							gdd->defn.reg_ll.La1, gdd->defn.reg_ll.La2,
							gdd->defn.reg_ll.scan_mode.north);
					}
				}

			else if ( ithin == 2 )
				{
				/* Set maximum number of rows and number of data values */
				for (nj=0, num_vals=0, nn=0; nn<ni; nn++)
					{
					npts = gdd->defn.reg_ll.thin_pts[nn];
					if ( npts > nj ) nj = npts;
					num_vals += npts;
					}

				/* Check column increment */
				if ( di == 65535 )
					{
					di = E1_set_lon_increment(ni,
							gdd->defn.reg_ll.Lo1, gdd->defn.reg_ll.Lo2,
							gdd->defn.reg_ll.scan_mode.west);
					}

				/* Set row increment */
				dj = E1_set_lat_increment(nj,
						gdd->defn.reg_ll.La1, gdd->defn.reg_ll.La2,
						gdd->defn.reg_ll.scan_mode.north);
				}

			break;

		case GAUSS_GRID:			/* GAUSSIAN */
			ni = gdd->defn.guas_ll.Ni;
			nj = gdd->defn.guas_ll.Nj;
			di = gdd->defn.guas_ll.Di;
			dj = gdd->defn.guas_ll.N;
			ithin = gdd->defn.guas_ll.thin_mode;

			/* Regular gaussian grids */
			if ( ithin == 0 )
				{
				/* Set number of data values */
				num_vals = ni * nj;

				/* Check column increment */
				if ( di == 65535 )
					{
					di = E1_set_lon_increment(ni,
							gdd->defn.guas_ll.Lo1, gdd->defn.guas_ll.Lo2,
							gdd->defn.guas_ll.scan_mode.west);
					}
				}

			/* Quasi-regular gaussian grids */
			else if ( ithin == 1 )
				{
				/* Set maximum number of columns and number of data values */
				for (ni=0, num_vals=0, nn=0; nn<nj; nn++)
					{
					npts = gdd->defn.guas_ll.thin_pts[nn];
					if ( npts > ni ) ni = npts;
					num_vals += npts;
					}

				/* Set column increment */
				di = E1_set_lon_increment(ni,
						gdd->defn.guas_ll.Lo1, gdd->defn.guas_ll.Lo2,
						gdd->defn.guas_ll.scan_mode.west);
				}

			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			ni = gdd->defn.ps.Nx;
			nj = gdd->defn.ps.Ny;
			di = gdd->defn.ps.Dx;
			dj = gdd->defn.ps.Dy;
			num_vals = ni * nj;
			ithin = 0;
			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			ni = gdd->defn.lambert.Nx;
			nj = gdd->defn.lambert.Ny;
			di = gdd->defn.lambert.Dx;
			dj = gdd->defn.lambert.Dy;
			num_vals = ni * nj;
			ithin = 0;
			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ni = gdd->defn.rotate_ll.Ni;
			nj = gdd->defn.rotate_ll.Nj;
			di = gdd->defn.rotate_ll.Di;
			dj = gdd->defn.rotate_ll.Dj;
			ithin = gdd->defn.rotate_ll.thin_mode;

			/* Regular rotated latitude-longitude grids */
			if ( ithin == 0 )
				{
				/* Set number of data values */
				num_vals = ni * nj;

				/* Check column increment */
				if ( di == 65535 )
					{
					di = E1_set_lon_increment(ni,
							gdd->defn.rotate_ll.Lo1, gdd->defn.rotate_ll.Lo2,
							gdd->defn.rotate_ll.scan_mode.west);
					}

				/* Check row increment */
				if ( dj == 65535 )
					{
					dj = E1_set_lat_increment(nj,
							gdd->defn.rotate_ll.La1, gdd->defn.rotate_ll.La2,
							gdd->defn.rotate_ll.scan_mode.north);
					}
				}

			/* Quasi-regular rotated latitude-longitude grids */
			else if ( ithin == 1 )
				{
				/* Set maximum number of columns and number of data values */
				for (ni=0, num_vals=0, nn=0; nn<nj; nn++)
					{
					npts = gdd->defn.rotate_ll.thin_pts[nn];
					if ( npts > ni ) ni = npts;
					num_vals += npts;
					}

				/* Set column increment */
				di = E1_set_lon_increment(ni,
						gdd->defn.rotate_ll.Lo1, gdd->defn.rotate_ll.Lo2,
						gdd->defn.rotate_ll.scan_mode.west);

				/* Check row increment */
				if ( dj == 65535 )
					{
					dj = E1_set_lat_increment(nj,
							gdd->defn.rotate_ll.La1, gdd->defn.rotate_ll.La2,
							gdd->defn.rotate_ll.scan_mode.north);
					}
				}

			else if ( ithin == 2 )
				{
				/* Set maximum number of rows and number of data values */
				for (nj=0, num_vals=0, nn=0; nn<ni; nn++)
					{
					npts = gdd->defn.rotate_ll.thin_pts[nn];
					if ( npts > nj ) nj = npts;
					num_vals += npts;
					}

				/* Check column increment */
				if ( di == 65535 )
					{
					di = E1_set_lon_increment(ni,
							gdd->defn.rotate_ll.Lo1, gdd->defn.rotate_ll.Lo2,
							gdd->defn.rotate_ll.scan_mode.west);
					}

				/* Set row increment */
				dj = E1_set_lat_increment(nj,
						gdd->defn.rotate_ll.La1, gdd->defn.rotate_ll.La2,
						gdd->defn.rotate_ll.scan_mode.north);
				}

			break;

		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Error message if bit map AND quasi-regular data! */
	if ( pdd->block_flags.bit_map != 0 && ithin != 0 )
		{
		(void) fprintf(stderr, "\n  Cannot process bit-map quasi-regular data!\n");
		return -3;
		}

	/* Set pointers for returned grid dimensions */
	*numgrid = num_vals;
	*nii = ni;
	*njj = nj;
	*dii = di;
	*djj = dj;

	dprintf(stderr, "  Completed setting grid dimensions\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_section3decoder()                                 */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the bit map header data and bit map       */
/*               and to convert data into integer if required         */
/*                                                                    */
/***usage      - E1_section3decoder(ip_file, gdd, bmhd, numgrid,      */
/*                                 numbit, pbit_data, ppole_bit);     */
/*               ip_file is a file pointer                            */
/*               gdd is a Grid_description_data structure             */
/*               bmhd is a Bitmap_data_header structure               */
/*               numgrid is the number of grid points in GRIB data    */
/*               numbit is the number of bit map points in GRIB data  */
/*               pbit_data is an array of bit map flags               */
/*               ppole_bit is a bit map flag for the pole             */
/*                                                                    */
/**********************************************************************/

static	int			E1_section3decoder

	(
	FILE						*ip_file,
	E1_Grid_description_data	*gdd,
	E1_Bit_map_header			*bmhd,
	unsigned int				*numgrid,
	unsigned int				*numbit,
	LOGICAL						**pbit_data,
	LOGICAL						*ppole_bit
	)

	{
	Octet			octet;
	int             nn;
	unsigned int	words_in_grib, vals_in_grib;
	unsigned int	nbit, wcount, vcount, bcount;
	short int		ipole;

	/* Input/Output data parameters */
	static Octet	*bit_raw  = NullPtr(Octet *);
	static LOGICAL	*bit_temp = NullPtr(LOGICAL *);
	static LOGICAL	*bit_data = NullPtr(LOGICAL *);
	LOGICAL			pole_bit;

	/* Initialize pointer for returned bit map data */
	*numbit    = 0;
	*pbit_data = NullPtr(LOGICAL *);
	*ppole_bit = FALSE;

	/* Extract Section3 header data */
	bmhd->length = fget3c(ip_file);
	bmhd->unused = fgetc(ip_file);
	bmhd->ntable = fget2c(ip_file);

	/* Check for proper length of Section3 data */
	if ( bmhd->length < BMB_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section3 header length: %d\n",
				bmhd->length);
		return -2;
		}

	/* Check for end-of-file or error in Section3 header data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section3 header data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section3 header data\n");
		return -1;
		}

	/* Extract Section3 bit map data */
	if ( bmhd->ntable == 0 )
		{

		/* Set number of values in remainder of Section3 */
		words_in_grib =  bmhd->length - BMB_LENGTH;
		vals_in_grib  = 8 * words_in_grib;

		/* Check for error in number of values in bit map data */
		if ( *numgrid > vals_in_grib )
			{
			(void) fprintf(stderr, "\n  Error in number of Section3 bit map values\n");
			(void) fprintf(stderr, "    grid_values: %d", *numgrid);
			(void) fprintf(stderr, "    bit_map_words: %d\n", words_in_grib);
			return -9;
			}

		/* Read the Section3 bit map data */
		bit_raw = GETMEM(bit_raw, Octet, words_in_grib);
		(void) fread((void *)bit_raw,(size_t)words_in_grib,1,ip_file);

		/* Check for end-of-file or error in Section3 bit map data */
		if ( feof(ip_file) )
			{
			(void) fprintf(stderr, "\n  End-of-file in Section3 bit map data\n");
			return 1;
			}
		if ( ferror(ip_file) )
			{
			(void) fprintf(stderr, "\n  Error in Section3 bit map data\n");
			return -1;
			}
		}

	/* Pre-defined bit maps are not available */
	else
		{
		(void) fprintf(stderr, "\n***WARNING*** Pre-defined GRIB bit maps");
		(void) fprintf(stderr, " are not available!\n");
		return -3;
		}

	/* Set pole data flag */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			ipole = gdd->defn.reg_ll.pole_extra;
			break;

		case GAUSS_GRID:			/* GAUSSIAN */
			ipole = 0;
			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			ipole = 0;
			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			ipole = 0;
			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ipole = gdd->defn.rotate_ll.pole_extra;
			break;

		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Extract bit map flags from the raw data */
	bit_temp = GETMEM(bit_temp, LOGICAL, vals_in_grib);
	for (wcount=0, vcount=0; wcount<words_in_grib; wcount++)
		{
		octet = bit_raw[wcount];
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_1) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_2) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_3) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_4) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_5) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_6) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_7) != 0 );
		bit_temp[vcount++] = (LOGICAL) ( GETBIT(octet,E1_bitmap_flag_8) != 0 );
		}

	/* Set bit map flags and pole data bit (if necessary) */
	bit_data = GETMEM(bit_data, LOGICAL, vals_in_grib);
	pole_bit = FALSE;
	bcount   = 0;

	/* Extract pole data bit from first bit */
	if ( ipole == -1 )
		{
		pole_bit = bit_temp[bcount++];
		}

	/* Set bit map flags for all grid locations */
	/*  and count number of bit mapped points   */
	for (nbit=0, vcount=0; vcount<*numgrid; vcount++)
		{
		bit_data[vcount] = bit_temp[bcount++];
		if ( bit_data[vcount] ) nbit++;
		}

	/* Extract data pole bit from last bit */
	if ( ipole == 1 )
		{
		pole_bit = bit_temp[bcount++];
		}

	/* Set pointer for returned data */
	*numbit    = nbit;
	*pbit_data = bit_data;
	*ppole_bit = pole_bit;
	dprintf(stderr, "  Completed Section3 decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_section4decoder()                                 */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to read in the binary data section header            */
/*               and to convert data into integer if required         */
/*                                                                    */
/***usage      - E1_section4decoder(ip_file, pdd, gdd, numgrid,       */
/*                                 numbit, ppole_bit, bdhd,           */
/*                                 numraw, praw_data, ppole_datum,    */
/*                                 pgrib_data);                       */
/*               ip_file is a file pointer                            */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               numgrid is the number of grid points in GRIB data    */
/*               numbit is the number of bit map points in GRIB data  */
/*               ppole_bit is a bit map flag for the pole             */
/*               bdhd is a Binary_data_header structure               */
/*               numraw is the number of data values in the GRIB data */
/*               praw_data is the extracted GRIB data                 */
/*               ppole_datum is an extracted GRIB datum for the pole  */
/*               pgrib_data is the extracted GRIB data before further */
/*                 processing (equivalent to praw_data)               */
/*                                                                    */
/**********************************************************************/

static	int			E1_section4decoder

	(
	FILE						*ip_file,
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	unsigned int				*numgrid,
	unsigned int				*numbit,
	LOGICAL						*ppole_bit,
	E1_Binary_data_header		*bdhd,
	unsigned int				*numraw,
	float						**praw_data,
	float						*ppole_datum,
	float						**pgrib_data
	)

	{
	Octet			octet;
	int				nn;
	LOGICAL			constant_data = FALSE;
	short int		ipole;
	unsigned int	words_in_grib, vals_in_grib;
	unsigned int	num_vals, total_vals, count, ecount;
	float			tentopowermd;

	/* Input/Output data parameters */
	static Octet	*binary_ip;
	long			bit_cursor;
	unsigned long	packed_datum;
	char			*required_char, ip_char;
	static float	*grib_data = NullFloat;
	static float	pole_datum = 0.0;

	/* Initialize pointers for returned GRIB data and pole datum */
	*numraw      = 0;
	*praw_data   = NullFloat;
	*ppole_datum = 0.0;
	*pgrib_data  = NullFloat;

	/* Extract Section4 header data */
	bdhd->length = fget3c(ip_file);
	octet = fgetc(ip_file);
	bdhd->flags  = E1_shift_r4(octet);
	bdhd->unused = E1_mask_l4(octet);
	if ( bdhd->flags != 0 )
		{
		(void) fprintf(stderr, "\n***WARNING*** Only simple packing for");
		(void) fprintf(stderr, " grid point data currently supported!\n");
		return -3;
		}
	bdhd->scale = fget2c(ip_file);
	if (0x8000 & bdhd->scale) bdhd->scale = 0 - (bdhd->scale & 0x7fff);
	bdhd->reference = dfget4c(ip_file);
	bdhd->bits_per_val = fgetc(ip_file);

	/* Check for proper length of Section4 data */
	if ( bdhd->length < BDH_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section4 header length: %d\n",
				bdhd->length);
		return -2;
		}

	/* Check for end-of-file or error in Section4 header data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section4 header data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section4 header data\n");
		return -1;
		}

	/* Set special case for constant data fields! */
	if ( bdhd->bits_per_val == 0 ) constant_data = TRUE;

	/* Make sure we can process the bit stream */
	if ( bdhd->bits_per_val > 26
			&& ( bdhd->bits_per_val != 28 && bdhd->bits_per_val != 32 ) )
		{
		(void) fprintf(stderr, "\n  Cannot process %d bits per value!",
				bdhd->bits_per_val);
		(void) fprintf(stderr, "\n  Only 1-26, 28 or 32 bits per value allowed!\n");
		return -3;
		}

	/* Set number of words and values in remainder of Section4     */
	/* Note that special case is allowed for constant data fields! */
	words_in_grib = bdhd->length - BDH_LENGTH;
	if ( constant_data ) vals_in_grib = 1;
	else                 vals_in_grib = 8 * words_in_grib / bdhd->bits_per_val;

	/* Set pole data flag */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			ipole = gdd->defn.reg_ll.pole_extra;
			break;

		case GAUSS_GRID:			/* GAUSSIAN */
			ipole = 0;
			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			ipole = 0;
			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			ipole = 0;
			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ipole = gdd->defn.rotate_ll.pole_extra;
			break;

		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Set number of values for bit map data */
	/*  (and reset pole flag if required!)   */
	if ( pdd->block_flags.bit_map != 0 )
		{
		num_vals = *numbit;
		if ( !(*ppole_bit) ) ipole = 0;
		}

	/* Otherwise, set number of values from grid data */
	else
		{
		num_vals = *numgrid;
		}

	/* Check for error in number of values in GRIB data */
	if ( !constant_data )
		{
		total_vals = num_vals + abs((int) ipole);
		if ( total_vals > vals_in_grib )
			{
			(void) fprintf(stderr, "\n  Error in number of Section4 data values\n");
			(void) fprintf(stderr, "    total_vals: %d", total_vals);
			(void) fprintf(stderr, "    vals_in_grib: %d\n", vals_in_grib);
			return -9;
			}
		}

	/* Read the Section4 data */
	binary_ip = GETMEM(binary_ip, Octet, words_in_grib);
	(void) fread((void *) binary_ip, (size_t)words_in_grib, 1, ip_file);

	/* Check for end-of-file or error in Section4 data */
	if ( feof(ip_file) )
		{
		(void) fprintf(stderr, "\n  End-of-file in Section4 data\n");
		return 1;
		}
	if ( ferror(ip_file) )
		{
		(void) fprintf(stderr, "\n  Error in Section4 data\n");
		return -1;
		}

	/* Set the data values for constant data fields! */
	if ( constant_data )
		{

		/* Allocate space for grid data */
		grib_data = GETMEM(grib_data, float, num_vals);

		/* Set the pole value */
		if ( ipole == -1 || ipole == 1 )
			{
			pole_datum = bdhd->reference;
			}

		/* Set grid values */
		for(count=0; count<num_vals; count++)
			{
			grib_data[count] = bdhd->reference;
			}
		}

	/* Extract the data values for all other fields */
	else
		{

		/* Allocate space for grid data */
		grib_data = GETMEM(grib_data, float, num_vals);
		tentopowermd = pow(10.0, (double) -pdd->forecast.factor_d);
		bit_cursor = 0;

		/* Get pole value from first value */
		if ( ipole == -1 )
			{
			packed_datum = E1_extract_packed_datum(bit_cursor,
				bdhd->bits_per_val, binary_ip);
			pole_datum = tentopowermd *
				(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
			bit_cursor+=bdhd->bits_per_val;
			}

		/* Get grid values from interior values */
		for(count=0; count<num_vals; count++)
			{
			packed_datum = E1_extract_packed_datum(bit_cursor,
				bdhd->bits_per_val, binary_ip);
			grib_data[count] = tentopowermd *
				(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
			bit_cursor+=bdhd->bits_per_val;
			}

		/* Get pole value from last value */
		if ( ipole == 1 )
			{
			packed_datum = E1_extract_packed_datum(bit_cursor,
				bdhd->bits_per_val, binary_ip);
			pole_datum = tentopowermd *
				(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
			bit_cursor+=bdhd->bits_per_val;
			}
		}

	/* Extract Section4 trailer string */
	required_char = GRIB_TRAILER;
	for (ecount = 0, count = 0; count < GRIB_TRAILER_LENGTH; count++)
		{
		ip_char = fgetc(ip_file);

		/* Check for end-of-file or error in search for trailer string */
		if ( feof(ip_file) )
			{
			(void) fprintf(stderr, "\n  End-of-file in Section4 trailer\n");
			return 1;
			}
		if ( ferror(ip_file) )
			{
			(void) fprintf(stderr, "\n  Error in Section4 trailer\n");
			return -1;
			}

		/* Check for bad character in Section4 trailer string */
		if ( ip_char != *required_char )
			{
			ecount++;
			(void) fprintf(stderr, "\n  Bad character in Section4 trailer\n");
			(void) fprintf(stderr,   "  Character \"%2x\" at location %d",
					ip_char, count+1);
			(void) fprintf(stderr,   " ... End string should be \"7777\"\n");
			}

		required_char++;
		}

	/* Error return if Section4 trailer not complete */
	if ( ecount > 0 ) return -99;

	/* Set pointers for returned data */
	*numraw      = num_vals;
	*praw_data   = grib_data;
	*ppole_datum = pole_datum;
	*pgrib_data  = grib_data;

	dprintf(stderr, "  Completed Section4 decode\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_expand_data()                                     */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to expand quasi-regular grid data to a regular grid  */
/*                                                                    */
/***usage      - E1_expand_data(pdd, gdd, nii, njj, pgrib_data);      */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               nii is the column dimension of data                  */
/*               njj is the row dimension of data                     */
/*               pgrib_data is an array of extracted GRIB data        */
/*                 on a regular grid (input/output)                   */
/*                                                                    */
/**********************************************************************/

static	int			E1_expand_data

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	int							*nii,
	int							*njj,
	float						**pgrib_data
	)

	{
	int				ii, ni, jj, nj, npts;
	short int		isweep, ithin, *pnpts;
	unsigned int	num_vals;
	double			dpos;

	/* Input/Output data parameters */
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;
	static double	*posqr = NullDouble, *valqr = NullDouble;
	static double	*pstns = NullDouble, *values = NullDouble;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;

	/* Set flags for expansion to regular grid */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			isweep = gdd->defn.reg_ll.scan_mode.horz_sweep;
			ithin = gdd->defn.reg_ll.thin_mode;
			pnpts = gdd->defn.reg_ll.thin_pts;
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			isweep = gdd->defn.guas_ll.scan_mode.horz_sweep;
			ithin = gdd->defn.guas_ll.thin_mode;
			pnpts = gdd->defn.guas_ll.thin_pts;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			isweep = gdd->defn.rotate_ll.scan_mode.horz_sweep;
			ithin = gdd->defn.rotate_ll.thin_mode;
			pnpts = gdd->defn.rotate_ll.thin_pts;
			break;
		default:					/* No quasi-regular grids are presently */
									/*  supported for other grid types      */
			return 0;
		}

	/* Return now if extracted GRIB data is on a regular grid */
	if ( ithin == 0 )
		{
		dprintf(stderr, "  Do not need to expand to regular grid\n");
		return 0;
		}

	/* Ensure that isweep and ithin are consistent */
	if ( isweep == 0 )
		{
		if ( ithin != 1 )
			{
			(void) fprintf(stderr, "\n  Inconsistent GRIB parameters");
			(void) fprintf(stderr, " for quasi-regular grid\n");
			(void) fprintf(stderr, "      isweep: %d   ithin:%d\n",
					isweep, ithin);
			return -2;
			}
		}
	else
		{
		if ( ithin != 2 )
			{
			(void) fprintf(stderr, "\n  Inconsistent GRIB parameters");
			(void) fprintf(stderr, " for quasi-regular grid\n");
			(void) fprintf(stderr, "      isweep: %d   ithin:%d\n",
					isweep, ithin);
			return -2;
			}
		}

	/* Initialize pointer to input data values */
	value_in = *pgrib_data;

	/* Set final grid dimensions for expanded regular grid */
	num_vals = ni * nj;

	/* Initialize space to hold data values at each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	value_out = grib_data;

	/* Quasi-regular rows */
	if ( ithin == 1 )
		{

		/* Set positions on each row of regular grid */
		pstns  = GETMEM(pstns,  double, ni);
		values = GETMEM(values, double, ni);
		for(ii=0; ii<ni; ii++) pstns[ii] = (double) ii;

		/* Determine data from quasi-regular data on each row */
		for(jj=0; jj<nj; jj++)
			{
			/* Set spacing of quasi-regular data wrt regular grid */
			npts = (int) *(pnpts + jj);
			dpos = (double) (ni-1) / (double) (npts-1);

			/* Set positions and values from quasi-regular grid */
			posqr = GETMEM(posqr, double, npts);
			valqr = GETMEM(valqr, double, npts);
			for(ii=0; ii<npts; ii++)
				{
				posqr[ii] = (double) ii * dpos;
				valqr[ii] = (double) *value_in++;
				}

			/* Determine values at all positions on regular grid */
			(void) Tween1(npts, posqr, valqr, ni, pstns, values, NullDouble);
			for(ii=0; ii<ni; ii++)
				*value_out++ = (float) values[ii];
			}
		}

	/* Quasi-regular columns */
	else
		{

		/* Set positions on each column of regular grid */
		pstns  = GETMEM(pstns,  double, nj);
		values = GETMEM(values, double, nj);
		for(jj=0; jj<nj; jj++)
			pstns[jj] = (double) jj;

		/* Determine data from quasi-regular data on each column */
		for(ii=0; ii<ni; ii++)
			{
			/* Set locations of quasi-regular data wrt regular grid */
			npts = (int) *(pnpts + ii);
			dpos = (double) (nj-1) / (double) (npts-1);

			/* Set positions and values from quasi-regular grid */
			posqr = GETMEM(posqr, double, npts);
			valqr = GETMEM(valqr, double, npts);
			for(jj=0; jj<npts; jj++)
				{
				posqr[jj] = (double) jj * dpos;
				valqr[jj] = (double) *value_in++;
				}

			/* Determine values at all positions on regular grid */
			(void) Tween1(npts, posqr, valqr, nj, pstns, values, NullDouble);
			for(jj=0; jj<nj; jj++)
				*value_out++ = (float) values[jj];
			}
		}

	/* Set pointer for returned GRIB data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed expansion to regular grid\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_unmap_data()                                      */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to expand bit mapped data to a regular grid          */
/*                                                                    */
/***usage      - E1_unmap_data(pdd, gdd, pbit_data, nii, njj,         */
/*                              pgrib_data);                          */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               pbit_data is an array of bit map data                */
/*               nii is the column dimension of data                  */
/*               njj is the row dimension of data                     */
/*               pgrib_data is an array of extracted GRIB data        */
/*                 on a regular grid (input/output)                   */
/*                                                                    */
/**********************************************************************/

static	int			E1_unmap_data

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	LOGICAL						**pbit_data,
	int							*nii,
	int							*njj,
	float						**pgrib_data
	)

	{
	int				ii, ni, jj, nj;
	short int		isweep;
	unsigned int	num_vals;

	/* Input/Output data parameters */
	static float	*grib_data        = NullFloat;
	LOGICAL			*bit_in;
	float			*val_in, *val_out;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;

	/* Set flags for expansion from bit mapped data */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			isweep = gdd->defn.reg_ll.scan_mode.horz_sweep;
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			isweep = gdd->defn.guas_ll.scan_mode.horz_sweep;
			break;
		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			isweep = gdd->defn.ps.scan_mode.horz_sweep;
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			isweep = gdd->defn.lambert.scan_mode.horz_sweep;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			isweep = gdd->defn.rotate_ll.scan_mode.horz_sweep;
			break;
		default:					/* Bit-mapped data is not presently */
									/*  supported for other grid types  */
			return 0;
		}

	/* Initialize pointer to input bit map flags and data values */
	bit_in = *pbit_data;
	val_in = *pgrib_data;

	/* Set final grid dimensions for expanded regular grid */
	num_vals = ni * nj;

	/* Initialize space to hold bit map flags and data values */
	/*  at all grid points                                    */
	grib_data = GETMEM(grib_data, float, num_vals);
	val_out   = grib_data;

	/* Initialize regular grid of bit map flags and data values */
	/*  for data with i'th direction incrementing first         */
	if ( isweep == 0 )
		{
		for(jj=0; jj<nj; jj++)
			for(ii=0; ii<ni; ii++)
				{
				if ( *bit_in++ )
					*val_out++ = *val_in++;
				else
					*val_out++ = 0.0;
				}
		}

	/* Initialize regular grid of bit map flags and data values */
	/*  for data with j'th direction incrementing first         */
	else
		{
		for(ii=0; ii<ni; ii++)
			for(jj=0; jj<nj; jj++)
				{
				if ( *bit_in++ )
					*val_out++ = *val_in++;
				else
					*val_out++ = 0.0;
				}
		}

	/* Set pointer for returned GRIB data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed expansion from bit mapped data\n");
	return 0;
}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_fill_bitmap()                                     */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to fill in missing locations on a bit map            */
/*                                                                    */
/***usage      - E1_fill_bitmap(npts, bitloc, bitex, nout, bitsout);  */
/*               npts is the number of "TRUE" bits in the bit map row */
/*                 or column                                          */
/*               bitloc is the location of each "TRUE" bit in the     */
/*                 bit map row or column                              */
/*               bitex is the number of bits to extrapolate           */
/*               nout is the number of bits in the output bit map row */
/*                 or column                                          */
/*               bitsout is the "TRUE/FALSE" value of each bit        */
/*                in the output bit map row or column                 */
/*                                                                    */
/**********************************************************************/

static	void		E1_fill_bitmap

	(
	int							npts,
	int							*bitloc,
	int							bitex,
	int							nout,
	LOGICAL						*bitsout
	)

	{
	int				nn, nnx, nbgn, ntrue;

	/* Initialize output bit map */
	for (nn=0; nn<nout; nn++) bitsout[nn] = TRUE;

	/* Return immediately if output bit map row or column is almost complete */
	if ( (nout - npts) <= (bitex * 2) ) return;

	/* Loop to reset output bit map for locations beyond extrapolation limit */
	for (nbgn=0, nn=0; nn<npts; nn++)
		{
		ntrue = bitloc[nn];
		if ( (ntrue - nbgn) > bitex )
			{
			for (nnx=nbgn; nnx<(ntrue-bitex); nnx++) bitsout[nnx] = FALSE;
			}
		nbgn = ntrue + bitex + 1;
		}
	if ( nbgn < nout )
		{
		for (nnx=nbgn; nnx<nout; nnx++) bitsout[nnx] = FALSE;
		}

	/* Return output bit map */
	return;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_reorder_data()                                    */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to reorder a grid of extracted GRIB data by row      */
/*                                                                    */
/***usage      - E1_reorder_data(pdd, gdd,                            */
/*                                nii, njj, dii, djj, pgrib_data);    */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               nii is the column dimension of data                  */
/*               njj is the row dimension of data                     */
/*               dii is the column spacing of data                    */
/*               djj is the row spacing of data                       */
/*               pgrib_data is an array of extracted GRIB data        */
/*                 on a regular grid ordered by row (input/output)    */
/*                                                                    */
/**********************************************************************/

static	int			E1_reorder_data

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	int							*nii,
	int							*njj,
	int							*dii,
	int							*djj,
	float						**pgrib_data
	)

	{
	int				ii, ni, di, jj, nj, dj, nn;
	short int		iwest, inorth, isweep;
	LOGICAL			left, bottom;
	unsigned int	num_vals;

	/* Input/Output data parameters */
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;
	di = *dii;
	dj = *djj;

	/* Set flags for data ordering */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			iwest  = gdd->defn.reg_ll.scan_mode.west;
			inorth = gdd->defn.reg_ll.scan_mode.north;
			isweep = gdd->defn.reg_ll.scan_mode.horz_sweep;
			if ( iwest == 0 )  left   = (LOGICAL) (di>0);
			else               left   = (LOGICAL) (di<0);
			if ( inorth == 0 ) bottom = (LOGICAL) (dj<0);
			else               bottom = (LOGICAL) (dj>0);
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			iwest  = gdd->defn.guas_ll.scan_mode.west;
			inorth = gdd->defn.guas_ll.scan_mode.north;
			isweep = gdd->defn.guas_ll.scan_mode.horz_sweep;
			if ( iwest == 0 )  left   = (LOGICAL) (di>0);
			else               left   = (LOGICAL) (di<0);
			if ( inorth == 0 ) bottom = (LOGICAL) (dj<0);
			else               bottom = (LOGICAL) (dj>0);
			break;
		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			iwest  = gdd->defn.ps.scan_mode.west;
			inorth = gdd->defn.ps.scan_mode.north;
			isweep = gdd->defn.ps.scan_mode.horz_sweep;
			if ( iwest == 0 )  left   = (LOGICAL) (di>0);
			else               left   = (LOGICAL) (di<0);
			if ( inorth == 0 ) bottom = (LOGICAL) (dj<0);
			else               bottom = (LOGICAL) (dj>0);
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			iwest  = gdd->defn.lambert.scan_mode.west;
			inorth = gdd->defn.lambert.scan_mode.north;
			isweep = gdd->defn.lambert.scan_mode.horz_sweep;
			if ( iwest == 0 )  left   = (LOGICAL) (di>0);
			else               left   = (LOGICAL) (di<0);
			if ( inorth == 0 ) bottom = (LOGICAL) (dj<0);
			else               bottom = (LOGICAL) (dj>0);
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			iwest  = gdd->defn.rotate_ll.scan_mode.west;
			inorth = gdd->defn.rotate_ll.scan_mode.north;
			isweep = gdd->defn.rotate_ll.scan_mode.horz_sweep;
			if ( iwest == 0 )  left   = (LOGICAL) (di>0);
			else               left   = (LOGICAL) (di<0);
			if ( inorth == 0 ) bottom = (LOGICAL) (dj<0);
			else               bottom = (LOGICAL) (dj>0);
			break;
		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Set final grid dimensions */
	num_vals = ni * nj;

	/* Set the data values for each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	value_out = grib_data;

	/* Transfer data to output array                                       */
	/*  ... ordered by column (left to right) for each row (bottom to top) */
	for(jj=0; jj<nj; jj++)
		for(ii=0; ii<ni; ii++)
			{

			/* Data with i'th direction incrementing first */
			if ( isweep == 0 )
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *pgrib_data + (jj*ni) + ii;
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + ((nj-jj-1)*ni) + ii;
						}
					}

				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *pgrib_data + (jj*ni) + (ni-ii-1);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + ((nj-jj-1)*ni) + (ni-ii-1);
						}
					}
				}

			/* Data with j'th direction incrementing first */
			else
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *pgrib_data + jj + (ii*nj);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + (nj-jj-1) + (ii*nj);
						}
					}

				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *pgrib_data + jj + ((ni-ii-1)*nj);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + (nj-jj-1) + ((ni-ii-1)*nj);
						}
					}
				}

			/* Set value in output data array */
			*value_out++ = *value_in;
			}

	/* Set pointer for returned data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed reorder of data\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_add_pole_data()                                   */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to extend pole data from preset lat-long grids to an */
/*               entire row or column in the extracted GRIB data      */
/*                                                                    */
/***usage      - E1_add_pole_data(pdd, gdd, ppole_bit, ppole_datum,   */
/*                                 nii, njj, pgrib_data);             */
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               ppole_bit is a bit map flag for the pole             */
/*               ppole_datum is an extracted GRIB datum for the pole  */
/*               nii is the column dimension of data (input/output)   */
/*               njj is the row dimension of data including pole      */
/*                 (input/output)                                     */
/*               pgrib_data is an array of extracted GRIB data        */
/*                 including a row for the pole (if required)         */
/*                 (input/output)                                     */
/*                                                                    */
/**********************************************************************/

static	int			E1_add_pole_data

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	LOGICAL						*ppole_bit,
	float						*ppole_datum,
	int							*nii,
	int							*njj,
	float						**pgrib_data
	)

	{
	int				ii, ni, jj, nj, nn;
	short int		ipole;
	unsigned int	num_vals;

	/* Input/Output data parameters */
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;
	float			*value_frow, *value_lrow;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;

	/* Set flags for adding pole data */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			ipole = gdd->defn.reg_ll.pole_extra;
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			ipole = 0;
			break;
		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			ipole = 0;
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			ipole = 0;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			ipole = gdd->defn.rotate_ll.pole_extra;
			break;
		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Return now if no pole data to add */
	if ( ipole == 0 )
		{
		dprintf(stderr, "  Do not need to add pole data\n");
		return 0;
		}

	/* Set final grid dimensions (with pole data added) */
	*nii = ni;
	*njj = nj + abs((int) ipole);
	num_vals = *nii * *njj;

	/* Initialize space to hold data values at each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	value_out = grib_data;

	/* Initialize pointer to input data values  */
	/* Note that data has been reordered by row */
	value_in   = *pgrib_data;
	value_frow = *pgrib_data;
	value_lrow = *pgrib_data + (nj-1)*ni;

	/* Add row of pole data at start of data */
	if ( ipole == -1 )
		{
		/* Copy first row of data if bit map data not set! */
		if ( pdd->block_flags.bit_map && !(*ppole_bit) )
			for(ii=0; ii<ni; ii++)
				*value_out++ = *value_frow++;
		else
			for(ii=0; ii<ni; ii++)
				*value_out++ = *ppole_datum;
		}

	/* Transfer data to output */
	for(jj=0; jj<nj; jj++)
		for(ii=0; ii<ni; ii++)
			*value_out++ = *value_in++;

	/* Add row of pole data at end of data */
	if ( ipole == 1 )
		{
		/* Copy last row of data if bit map data not set! */
		if ( pdd->block_flags.bit_map && !(*ppole_bit) )
			for(ii=0; ii<ni; ii++)
				*value_out++ = *value_lrow++;
		else
			for(ii=0; ii<ni; ii++)
				*value_out++ = *ppole_datum;
		}

	/* Set pointer for returned GRIB data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed addition of pole data\n");
	return 0;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_set_meridian_flag()                               */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to set flag for hemispheric data which does not      */
/*               repeat the first meridian                            */
/*                                                                    */
/***usage      - E1_set_meridian_flag(gdd, nii, dii);                 */
/*               gdd is a Grid_description_data structure             */
/*               nii is the column dimension of data                  */
/*               dii is the column spacing of data                    */
/*                                                                    */
/**********************************************************************/

static	void		E1_set_meridian_flag

	(
	E1_Grid_description_data	*gdd,
	int							*nii,
	int							*dii
	)

	{
	long int	numlons, coverage;

	/* Set extra meridian flag only for latitude-longitude grids */
	if ( (gdd->dat_rep == LATLON_GRID) )
		{

		/* Set flag based on coverage of almost 360 degrees */
		numlons  = (long) *nii - 1;
		coverage = labs(numlons * (long) *dii);
		if ( coverage < MaxLongitude )
			{
			coverage += labs((long) *dii);
			if ( coverage >= MaxLongitude )
					gdd->defn.reg_ll.meridian_extra = 1;
			}
		}
	else if ( (gdd->dat_rep == ROTATED_LATLON_GRID) )
		{

		/* Set flag based on coverage of almost 360 degrees */
		numlons  = (long) *nii - 1;
		coverage = labs(numlons * (long) *dii);
		if ( coverage < MaxLongitude )
			{
			coverage += labs((long) *dii);
			if ( coverage >= MaxLongitude )
					gdd->defn.rotate_ll.meridian_extra = 1;
			}
		}
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_add_meridian_data()                               */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to copy an extra row or column in the extracted GRIB */
/*               data for hemispheric data which does not repeat the  */
/*               first meridian                                       */
/*                                                                    */
/***usage      - E1_add_meridian_data(pdd, gdd, nii, njj, pgrib_data);*/
/*               pdd is a Product_definition_data structure           */
/*               gdd is a Grid_description_data structure             */
/*               nii is the column dimension of data including        */
/*                an extra meridian (if required) (input/output)      */
/*               njj is the row dimension of data (input/output)      */
/*               pgrib_data is an array of extracted GRIB data        */
/*                 including an extra column for an extra meridian    */
/*                 (if required) (input/output)                       */
/*                                                                    */
/**********************************************************************/

static	int			E1_add_meridian_data

	(
	E1_Product_definition_data	*pdd,
	E1_Grid_description_data	*gdd,
	int							*nii,
	int							*njj,
	float						**pgrib_data
	)

	{
	int				ii, ni, jj, nj, nn;
	short int		imeridian;
	unsigned int	num_vals;

	/* Input/Output data parameters */
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;

	/* Set flags for adding meridian data */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			imeridian = gdd->defn.reg_ll.meridian_extra;
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			imeridian = 0;
			break;
		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			imeridian = 0;
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			imeridian = 0;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			imeridian = gdd->defn.rotate_ll.meridian_extra;
			break;
		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -2;
		}

	/* Return now if no meridian data needs to be added */
	if ( imeridian == 0 )
		{
		dprintf(stderr, "  Do not need to add meridian data\n");
		return 0;
		}

	/* Set final grid dimensions (with meridian data added) */
	*nii = ni + abs((int) imeridian);
	*njj = nj;
	num_vals = *nii * *njj;

	/* Initialize space to hold data values at each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	value_out = grib_data;

	for(jj=0; jj<nj; jj++)
		{

		/* Transfer row of data to output         */
		/* Note that data has been ordered by row */
		value_in = *pgrib_data + (jj * ni);
		for(ii=0; ii<ni; ii++)
			*value_out++ = *value_in++;

		/* Copy meridian data from first column to last column */
		if ( imeridian == 1 )
			{
			value_in = *pgrib_data + (jj * ni);
			*value_out++ = *value_in;
			}
		}

	/* Set pointer for returned GRIB data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed addition of meridian data\n");
	return 0;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Setting Latitude/Longitude Increments): *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_set_lat_increment()                               */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to set latitude increments from latitude ranges      */
/*                                                                    */
/***usage      - E1_set_lat_increment(njj, lbgn, lend, dir);          */
/*               njj is the row (latitude) dimension of data          */
/*               lbgn is the beginning latitude of data               */
/*               lend is the ending latitude of data                  */
/*               dir is the direction of latitude scanning            */
/*                 (0 is southwards, 1 is northwards from beginning)  */
/*                                                                    */
/**********************************************************************/

static	int			E1_set_lat_increment

	(
	int				njj,
	long int		lbgn,
	long int		lend,
	short int		dir
	)

	{
	int				dj;

	/* Error return for problem with row (latitude) dimension */
	if ( njj <= 1 || njj == 65535 ) return 0;

	/* Set row (latitude) increment based on direction of scanning */
	if ( dir == 1 )
		{
		dj = (lend - lbgn) / (njj - 1);
		}
	else
		{
		dj = (lbgn - lend) / (njj - 1);
		}

	/* Return row (latitude) increment */
	return dj;
	}

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_set_lon_increment()                               */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to set longitude increments from longitude ranges    */
/*                                                                    */
/***usage      - E1_set_lon_increment(nii, lbgn, lend, dir);          */
/*               nii is the column (longitude) dimension of data      */
/*               lbgn is the beginning longitude of data              */
/*               lend is the ending longitude of data                 */
/*               dir is the direction of longitude scanning           */
/*                 (0 is eastwards, 1 is westwards from beginning)    */
/*                                                                    */
/**********************************************************************/

static	int			E1_set_lon_increment

	(
	int				nii,
	long int		lbgn,
	long int		lend,
	short int		dir
	)

	{
	int				di;

	/* Error return for problem with column (longitude) dimension */
	if ( nii <= 1 || nii == 65535 ) return 0;

	/* Set column (longitude) increment based on direction of scanning */
	if ( dir == 0 )
		{
		if ( lend < lbgn ) lend += MaxLongitude;
		di = (lend - lbgn) / (nii - 1);
		}
	else
		{
		if ( lbgn < lend ) lbgn += MaxLongitude;
		di = (lbgn - lend) / (nii - 1);
		}

	/* Return column (longitude) increment */
	return di;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Binary File Parsing):                   *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/*                                                                    */
/***subroutine - E1_extract_packed_datum()                            */
/*                                                                    */
/***language   - c language                                           */
/*                                                                    */
/***purpose    - to extract GRIB data values from a bit stream that   */
/*               is not byte-aligned                                  */
/*                                                                    */
/***usage      - extract_packed_datum(first_bit, nb_bits, buffer);    */
/*                                                                    */
/***parameters -                                                      */
/*          buffer    : is the pointer to the bit stream              */
/*          first_bit : is the bit where the value begins             */
/*          nb_bits   : number of bits representing the value         */
/*                                                                    */
/**********************************************************************/

static	unsigned long	E1_extract_packed_datum

	(
	long	first_bit,	/* OFFSET OF BIT STRING BEGINNING */
						/*  (1st BIT IS 0)                */
	short	nb_bits,	/* LENGTH OF BIT STRING */
	Octet	*buffer		/* SOURCE DATA */
	)

	{
	long			last_bit;
	Octet			*first_byte, *last_byte;
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

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Error Trap):                            *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

static	void		errtrap

	(
	int				sig
	)

	{
	STRING		model, rtime, btime, etime, element, level, units;
	int			mplus;
	LOGICAL		ok;

	/* Reset error trap when error encountered */
	(void) unset_num_trap();
	(void) fprintf(stdout, "    %s encountered!!!\n", signal_name(sig));

	/* Extract identifiers for error message                   */
	/*  ... by temporarily setting GribDecoded as if it worked */
	GribDecoded = TRUE;
	if ( !gribfield_identifiers_edition1(&model, &rtime, &btime,
			&etime, &element, &level, &units) )
		{

		/* Print error message */
		(void) fprintf(stdout, " Skipping unrecognized field\n");
		(void) fprintf(stdout, " Next field will be processed\n");
		(void) fprintf(stdout, "==============================\n");
		}

	else
		{

		/* Set the prog time from run time and end valid time */
		mplus = calc_prog_time_minutes(rtime, etime, &ok);

		/* Print error message */
		(void) fprintf(stdout, " Skipping field: %s %s T%s %s %s\n",
				model, rtime, hour_minute_string(0, mplus), element, level);
		(void) fprintf(stdout, " Next field will be processed\n");
		(void) fprintf(stdout, "==============================\n");
		}

	/* Jump to next GRIB message */
	GribDecoded = FALSE;
	longjmp(GribEnv, 1);
	}

/*********************************************************************/
/**                                                                 **/
/** interpret_scan_mode - set flags based on scan mode              **/
/**                                                                 **/
/*********************************************************************/
static LOGICAL interpret_scan_mode
(
 LOGICAL	*iwest,		/* i direction is left to right (west to east) */
 LOGICAL	*inorth,	/* j direction is bottom to top (south to north) */
 LOGICAL	*isweep		/* sweep is in i direction */
)
	{
	int nn;
	E1_Grid_description_data	*gdd;
	gdd = &GribFld.Gdd;
	/* Obtain scan mode based on grid template */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:			/* LATITUDE-LONGITUDE */
			if (NotNull(iwest))	 *iwest  = gdd->defn.reg_ll.scan_mode.west;
			if (NotNull(inorth)) *inorth = gdd->defn.reg_ll.scan_mode.north;
			if (NotNull(isweep)) *isweep = gdd->defn.reg_ll.scan_mode.horz_sweep;
			break;
		case GAUSS_GRID:			/* GAUSSIAN */
			if (NotNull(iwest))  *iwest  = gdd->defn.guas_ll.scan_mode.west;
			if (NotNull(inorth)) *inorth = gdd->defn.guas_ll.scan_mode.north;
			if (NotNull(isweep)) *isweep = gdd->defn.guas_ll.scan_mode.horz_sweep;
			break;
		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */
			if (NotNull(iwest))  *iwest  = gdd->defn.ps.scan_mode.west;
			if (NotNull(inorth)) *inorth = gdd->defn.ps.scan_mode.north;
			if (NotNull(isweep)) *isweep = gdd->defn.ps.scan_mode.horz_sweep;
			break;
		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
			if (NotNull(iwest))  *iwest  = gdd->defn.lambert.scan_mode.west;
			if (NotNull(inorth)) *inorth = gdd->defn.lambert.scan_mode.north;
			if (NotNull(isweep)) *isweep = gdd->defn.lambert.scan_mode.horz_sweep;
			break;
		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
			if (NotNull(iwest))  *iwest  = gdd->defn.rotate_ll.scan_mode.west;
			if (NotNull(inorth)) *inorth = gdd->defn.rotate_ll.scan_mode.north;
			if (NotNull(isweep)) *isweep = gdd->defn.rotate_ll.scan_mode.horz_sweep;
			break;
		default:			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if ( gdd->dat_rep == GRIBGridLabels[nn].ident )
					{
					(void) fprintf(stderr, "\n  Cannot yet process");
					(void) fprintf(stderr, " GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if ( nn >= NumGRIBGridLabels )
				{
				(void) fprintf(stderr, "\n  Unrecognized GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return FALSE;
		}
	return TRUE;
	}
