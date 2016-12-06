/***********************************************************************
*                                                                      *
*    r g r i b _ e d i t i o n 0 . c                                   *
*                                                                      *
*    Routines to decode GRIB Edition 0 format files                    *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#define RGRIBED0_MAIN	/* To initialize defined constants and internal */
						/*  structures in rgrib_edition0.h file         */

#include "rgrib_edition1.h"
#include "rgrib_edition0.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#undef DEBUG

#ifdef DEBUG_DECODE
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_DECODE */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                        */
/*  ... these are defined in rgrib_edition0.h */

/* Internal static functions (Identifier Translation) */
static	LOGICAL	E0_grib_models(E1_Product_definition_data, STRING);
static	LOGICAL	E0_grib_tstamps(E1_Product_definition_data, STRING, STRING,
						STRING);
static	LOGICAL	E0_grib_elements(E1_Product_definition_data, STRING, STRING);
static	LOGICAL	E0_grib_levels(E1_Product_definition_data, STRING);

/* Internal static functions (Section Decodes) */
static	int		E0_section0decoder(FILE *, E1_Indicator_block *);
static	int		E0_section1decoder(FILE *, E1_Product_definition_data *);
static	int		E0_latlongdecoder(FILE *, E1_Grid_description_data *);
static	int		E0_gaussdecoder(FILE *, E1_Grid_description_data *);
static	int		E0_psdecoder(FILE *, E1_Grid_description_data *);
static	int		E0_section2decoder(FILE *, E1_Grid_description_data *);
static	int		E0_gdbdefaultrtn(E1_Product_definition_data *,
						E1_Grid_description_data *);
static	int		E0_section3decoder(FILE *, E1_Bit_map_header *, Octet **);
static	int		E0_section4decoder(FILE *, E1_Product_definition_data *,
						E1_Grid_description_data *, E1_Binary_data_header *,
						unsigned int *, float **, float *,
						int *, int *, int *, int *);
static	int		E0_reorder_data(E1_Product_definition_data *,
						E1_Grid_description_data *, unsigned int *, float **,
						int *, int *, int *, int *, float **);
static	int		E0_add_pole_data(E1_Product_definition_data *,
						E1_Grid_description_data *, float *,
						int *, int *, float **);

/* Internal static functions (Binary File Parsing) */
static	unsigned long	E0_extract_packed_datum(long, short, Octet *);

/***********************************************************************
*                                                                      *
*    o p e n _ g r i b f i l e _ e d i t i o n 0                       *
*    n e x t _ g r i b f i e l d _ e d i t i o n 0                     *
*    g r i b f i e l d _ i d e n t i f i e r s _ e d i t i o n 0       *
*    c l o s e _ g r i b f i l e _ e d i t i o n 0                     *
*                                                                      *
*    Read the contents of the given GRIB file and store the decoded    *
*    information in a series of GRIBFIELD objects and translated       *
*    identifiers.                                                      *
*                                                                      *
*    open_gribfile_edition0() opens the given GRIB file for reading.   *
*                                                                      *
*    next_gribfield_edition0() extracts subsequent fields from the     *
*    opened GRIB file, saving each one in a local GRIBFIELD object.    *
*    As long as it finds a field, it returns TRUE and passes back the  *
*    GRIBFIELD object.  When the end of the GRIB file is reached, the  *
*    GRIB file is closed, and further calls to this function return    *
*    FALSE.                                                            *
*                                                                      *
*    gribfield_identifiers_edition0() extracts model, timestamp,       *
*    element, level, and unit identifiers from the local GRIBFIELD     *
*    object.                                                           *
*                                                                      *
*    close_gribfile_edition0() closes the currently open GRIB.         *
*                                                                      *
***********************************************************************/

/* Internal file pointers */
static	FILE		*GribFile    = NullPtr(FILE *);
static	long int	GribPosition = 0;

/* Internal GRIB field buffer */
static	LOGICAL		GribDecoded = FALSE;
static	GRIBFIELD	GribFld     = { 0 };

/* Internal GRIB identifier buffers */
static	LOGICAL		GribValid                   = FALSE;
static	char		GribModel[GRIB_LABEL_LEN]   = "";
static	char		GribRtime[GRIB_LABEL_LEN]   = "";
static	char		GribVtimeb[GRIB_LABEL_LEN]  = "";
static	char		GribVtimee[GRIB_LABEL_LEN]  = "";
static	char		GribElement[GRIB_LABEL_LEN] = "";
static	char		GribLevel[GRIB_LABEL_LEN]   = "";
static	char		GribUnits[GRIB_LABEL_LEN]   = "";


LOGICAL		open_gribfile_edition0

	(
	STRING		name
	)

	{

	/* There will be no local GRIBFIELD upon opening */
	GribDecoded = FALSE;
	GribValid   = FALSE;

	/* If there already is an open GRIB file, close it now */
	(void) close_gribfile_edition0();

	/* Do nothing if GRIB file name not given */
	if ( blank(name) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition0] GRIB file name not given\n");
#		endif /* DEBUG */
		return FALSE;
		}

	/* See if the GRIB file exists */
	if ( !find_file(name) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition0] GRIB file not found: %s\n",
				name);
#		endif /* DEBUG */
		return FALSE;
		}

	/* Open the GRIB file */
	GribFile = fopen(name, "r");
	if ( IsNull(GribFile) )
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[open_gribfile_edition0] GRIB file unreadable: %s\n",
				name);
#		endif /* DEBUG */
		return FALSE;
		}

	/* Set position to start of file and return TRUE if all OK */
	GribPosition = 0;
	return TRUE;
	}


LOGICAL		next_gribfield_edition0

	(
	GRIBFIELD	**gribfld		/* pointer to local GRIBFIELD object */
	)

	{
	int		iret;
	Octet	*pbit_data;
	int		ditmp, djtmp;

	/* Set default for no local GRIBFIELD */
	GribDecoded = FALSE;
	*gribfld    = NullPtr(GRIBFIELD *);

	/* Return now if no current GRIB file */
	if ( IsNull(GribFile) ) return GribDecoded;

	/*** INDICATOR SECTION ***/
	iret = E0_section0decoder(GribFile, &GribFld.Isb);
	if ( iret != 0 && feof(GribFile) )
		{
		/* End of file in GRIB message */
		(void) close_gribfile_edition0();
		return GribDecoded;
		}
	else if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition0();
			return GribDecoded;
			}
		return next_gribfield_edition0(gribfld);
		}

	/*** PRODUCT DEFINITION SECTION ***/
	iret = E0_section1decoder(GribFile, &GribFld.Pdd);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition0();
			return GribDecoded;
			}
		return next_gribfield_edition0(gribfld);
		}

	/* Fix for CMC error in coding of surface parameters */
	if ( GribFld.Pdd.layer.type == 100
			&& ((GribFld.Pdd.layer.top << 8) + GribFld.Pdd.layer.bottom) == 0 )
		{
		(void) fprintf(stderr, "  ...Correcting error in coding of");
		(void) fprintf(stderr, " surface as isobaric level at 0 hPa\n");
		GribFld.Pdd.layer.type = 1;
		}
	/* End of fix */

	/*** GRID DESCRIPTION SECTION (OPTIONAL) ***/
	if ( GribFld.Pdd.block_flags.grid_description != 0 )
		{
		iret = E0_section2decoder(GribFile, &GribFld.Gdd);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition0();
				return GribDecoded;
				}
			return next_gribfield_edition0(gribfld);
			}

		/* Fix for CMC coding of latitude/longitude grid increments */
		if ( (GribFld.Pdd.centre_id == 54) && (GribFld.Gdd.dat_rep == 0) )
			{
			/* Swap direction increments the right way */
			ditmp = GribFld.Gdd.defn.reg_ll.Dj;
			djtmp = GribFld.Gdd.defn.reg_ll.Di;
			GribFld.Gdd.defn.reg_ll.Di = ditmp;
			GribFld.Gdd.defn.reg_ll.Dj = djtmp;
			}
		}

	/*** OR PREDEFINED GRID ***/
	else
		{
		iret = E0_gdbdefaultrtn(&GribFld.Pdd, &GribFld.Gdd);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition0();
				return GribDecoded;
				}
			return next_gribfield_edition0(gribfld);
			}
		}

	/*** BIT MAP SECTION (OPTIONAL) ***/
	if ( GribFld.Pdd.block_flags.bit_map != 0 )
		{
		iret = E0_section3decoder(GribFile, &GribFld.Bmhd, &pbit_data);
		if ( iret != 0 )
			{
			/* Reset position due to error in GRIB message */
			if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
				{
				(void) close_gribfile_edition0();
				return GribDecoded;
				}
			return next_gribfield_edition0(gribfld);
			}
		}

	/*** BINARY DATA SECTION ***/
	iret = E0_section4decoder(GribFile, &GribFld.Pdd, &GribFld.Gdd,
			&GribFld.Bdhd,
			&GribFld.NumRaw, &GribFld.PRaw, &GribFld.PoleDatum,
			&GribFld.Nii, &GribFld.Njj, &GribFld.Dii, &GribFld.Djj);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition0();
			return GribDecoded;
			}
		return next_gribfield_edition0(gribfld);
		}

	/*** REORDER BINARY DATA ARRAY ***/
	iret = E0_reorder_data(&GribFld.Pdd, &GribFld.Gdd,
			&GribFld.NumRaw, &GribFld.PRaw,
			&GribFld.Nii, &GribFld.Njj, &GribFld.Dii, &GribFld.Djj,
			&GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition0();
			return GribDecoded;
			}
		return next_gribfield_edition0(gribfld);
		}

	/*** ADD POLE DATA TO BINARY DATA ARRAY ***/
	iret = E0_add_pole_data(&GribFld.Pdd, &GribFld.Gdd,
			&GribFld.PoleDatum, &GribFld.Nii, &GribFld.Njj, &GribFld.PData);
	if ( iret != 0 )
		{
		/* Reset position due to error in GRIB message */
		if ( fseek(GribFile, GribPosition, SEEK_SET) != 0 )
			{
			(void) close_gribfile_edition0();
			return GribDecoded;
			}
		return next_gribfield_edition0(gribfld);
		}

	/* Set GribDecoded to TRUE and return local GRIBFIELD object  */
	/*  if all parts of the GRIB message were extracted correctly */
	GribDecoded = TRUE;
	*gribfld    = &GribFld;
	return GribDecoded;
	}


LOGICAL		gribfield_identifiers_edition0

	(
	STRING		*model,		/* string containing model label */
	STRING		*rtime,		/* string containing run timestamp */
	STRING		*vtimeb,	/* string containing begin valid timestamp */
	STRING		*vtimee,	/* string containing end valid timestamp */
	STRING		*element,	/* string containing field element label */
	STRING		*level,		/* string containing field level label */
	STRING		*units		/* string containing field units label */
	)

	{

	/* Set defaults for GRIB identifiers */
	GribValid               = FALSE;
	if ( NotNull(model) )   *model   = NullString;
	if ( NotNull(rtime) )   *rtime   = NullString;
	if ( NotNull(vtimeb) )  *vtimeb  = NullString;
	if ( NotNull(vtimee) )  *vtimee  = NullString;
	if ( NotNull(element) ) *element = NullString;
	if ( NotNull(level) )   *level   = NullString;
	if ( NotNull(units) )   *units   = NullString;

	/* Return now if no current GRIB file or no local GRIBFIELD object */
	if ( IsNull(GribFile) || !GribDecoded ) return GribValid;

	/* Set model label from originating center and model */
	if ( !E0_grib_models(GribFld.Pdd, GribModel) ) return GribValid;

	/* Set run and valid timestamps from date and time information */
	if ( !E0_grib_tstamps(GribFld.Pdd, GribRtime, GribVtimeb, GribVtimee) )
			return GribValid;

	/* Set element and units labels from element code */
	if ( !E0_grib_elements(GribFld.Pdd, GribElement, GribUnits) )
			return GribValid;

	/* Set level label from level code and values */
	if ( !E0_grib_levels(GribFld.Pdd, GribLevel) )
			return GribValid;

	/* Set GribValid to TRUE and return requested identifiers */
	/*  if all identifiers were correctly translated          */
	GribValid               = TRUE;
	if ( NotNull(model) )   *model   = GribModel;
	if ( NotNull(rtime) )   *rtime   = GribRtime;
	if ( NotNull(vtimeb) )  *vtimeb  = GribVtimeb;
	if ( NotNull(vtimee) )  *vtimee  = GribVtimee;
	if ( NotNull(element) ) *element = GribElement;
	if ( NotNull(level) )   *level   = GribLevel;
	if ( NotNull(units) )   *units   = GribUnits;
	return GribValid;
	}


void		close_gribfile_edition0

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
*     STATIC (LOCAL) ROUTINES (Identifier Translation)                 *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** E 0 _ g r i b _ m o d e l s                                    ***
 *** E 0 _ g r i b _ t s t a m p s                                  ***
 *** E 0 _ g r i b _ e l e m e n t s                                ***
 *** E 0 _ g r i b _ l e v e l s                                    ***
 ***                                                                ***
 *** Identify model label, run and valid timestamps, element/units  ***
 *** labels, or level label from GRIB product definition data.      ***
 ***                                                                ***
 *** These routines are based on Tables in the 1989 GRIB Edition 0  ***
 *** document WMO Code FM 92-VIII Ext. entitled "The WMO Format For ***
 *** The Storage Of Weather Product Information And The Exchange Of ***
 *** Weather Product Messages In Gridded Binary Form" editted by    ***
 *** John D. Stackpole of the U.S. Department of Commerce, NMC.     ***
 ***                                                                ***
 *** Note: These routines have been modified from GRIB Edition 1    ***
 ***                                                                ***
 *** The  model  definitions are from Table 1.                      ***
 *** The  tstamp  definitions are from Table 8 and Table 8a.        ***
 *** The  element/units  definitions are from Table 5.              ***
 *** The  level  defintions are from Table 6 and Table 7.           ***
 ***                                                                ***
 *** Note that model and element/unit definitions which are not     ***
 *** recognized in rgrib_edition0.h are returned as a default name  ***
 *** tag which could be decoded at a later time.  For example,      ***
 *** gribmodel:74:15 would identify an unrecognized model "15" from ***
 *** UK Met Office,Bracknell, and gribelement:44 would identify an  ***
 *** unrecognized element "vertical_wind_shear" in "m/s/km".        ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		E0_grib_models

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		model		/* string containing model label */
	)

	{
	int		num, numc, numm;

	static	char	did[GRIB_LABEL_LEN];
	static	char	cid[GRIB_LABEL_LEN];
	static	char	mid[GRIB_LABEL_LEN];

	/* Initialise model */
	(void) strcpy(model, "");

	/* Check through list of model defintions for acceptable ones */
	for (num=0; num<NumModelDefs; num++)
		if ( ModelDefs[num].centre_id == pdd.centre_id
			&& ModelDefs[num].model_id == pdd.model_id )
				{

				/* Model label found */
				(void) strcpy(model, ModelDefs[num].model_label);
				return TRUE;
				}

	/* Set default for unrecognized center or model id */
	(void) strcpy(did, "gribmodel");
	num = strlen(did);
	numc = int_string(pdd.centre_id, cid, GRIB_LABEL_LEN);
	numm = int_string(pdd.model_id,  mid, GRIB_LABEL_LEN);
	if ( num > 0 && numc > 0 && numm > 0
			&& (num+1+numc+1+numm) < GRIB_LABEL_LEN )
		{
		(void) strcpy(model, did);
		(void) strcat(model, ":");
		(void) strcat(model, cid);
		(void) strcat(model, ":");
		(void) strcat(model, mid);

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E0_grib_models] Processing unrecognized");
		(void) fprintf(stderr, " centre id: %d", pdd.centre_id);
		(void) fprintf(stderr, "  or model id: %d\n", pdd.model_id);
		(void) fprintf(stderr, "     Contact FPA Development Group to add");
		(void) fprintf(stderr, " these identifiers to next FPA release\n");
		return TRUE;
		}

	/* Error return for unrecognizable center or model id */
	(void) fprintf(stderr, "[E0_grib_models] Unrecognizable");
	(void) fprintf(stderr, " centre id: %d", pdd.centre_id);
	(void) fprintf(stderr, "  or model id: %d\n", pdd.model_id);
	return FALSE;
}

static	LOGICAL		E0_grib_tstamps

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		rtime,		/* string containing run timestamp */
	STRING		vtimeb,		/* string containing begin valid timestamp */
	STRING		vtimee		/* string containing end valid timestamp */
	)

	{
	int		year, month, day, jday, hour, minute, second=0;
	int		vyear, vjday, vhour, vminute, vsecond=0;

	/* Initialise timestamps */
	(void) strcpy(rtime,  "");
	(void) strcpy(vtimeb, "");
	(void) strcpy(vtimee, "");

	/* Get the reference date and time, and set the run timestamp */
	/* Reference century assummed to be 1900                      */
	/**************************************************************
	*                                                             *
	*  Y2000 issue:  Optional 2-digit years (Potential problem)   *
	*                                                             *
	*  If necessary, use full_year() to convert 2-digit years.    *
	*  Preferably, drop support of abbreviated years.             *
	*                                                             *
	**************************************************************/
	year   = 1900 + pdd.forecast.reference.year;
	month  = pdd.forecast.reference.month;
	day    = pdd.forecast.reference.day;
	(void) jdate(&year, &month, &day, &jday);
	hour   = pdd.forecast.reference.hour;
	minute = pdd.forecast.reference.minute;
	(void) tnorm(&year, &jday, &hour, &minute, &second);
	(void) strcpy(rtime, build_tstamp(year, jday, hour, minute, FALSE, TRUE));

	/* Error return for unrecognizable date or time */
	if ( blank(rtime) )
		{
		(void) fprintf(stderr, "[E0_grib_tstamps] Unrecognizable");
		(void) fprintf(stderr, " century: %d", pdd.forecast.century);
		(void) fprintf(stderr, "  or year: %d", pdd.forecast.reference.year);
		(void) fprintf(stderr, "  or month: %d", pdd.forecast.reference.month);
		(void) fprintf(stderr, "  or day: %d", pdd.forecast.reference.day);
		(void) fprintf(stderr, "  or hour: %d", pdd.forecast.reference.hour);
		(void) fprintf(stderr, "  or minute: %d\n", pdd.forecast.reference.minute);
		return FALSE;
		}

	/* Determine the valid time, and set the valid timestamps        */
	/* Note that we will only recognize time range indicator:        */
	/*  0  (forecast or analysis valid at run time plus time1)       */
	/*  1  (analysis valid at run time where time1=0)                */
	/*  2  (valid time ranging from run time plus time1 to run time  */
	/*       plus time2)                                             */
	/*  3  (average from run time plus time1 to run time plus time2) */
	/*  4  (accumulation from run time plus time1 to run time plus   */
	/*       time2)                                                  */
	/*  5  (difference of run time plus time2 minus run time plus    */
	/*       time1)                                                  */
	/* Note that we will only recognize forecast time unit:          */
	/*  1  (hours)                                                   */

	if ( pdd.forecast.range_type == 0
			&& pdd.forecast.units == 1 )
		{
		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time1;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimeb,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));
		(void) strcpy(vtimee, vtimeb);
		}

	else if ( pdd.forecast.range_type == 1
			&& pdd.forecast.units == 1
			&& pdd.forecast.time1 == 0 )
		{
		(void) strcpy(vtimeb, rtime);
		(void) strcpy(vtimee, rtime);
		}

	else if ( pdd.forecast.range_type == 2
			&& pdd.forecast.units == 1 )
		{
		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time1;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimeb,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));

		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time2;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimee,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));
		}

	else if ( pdd.forecast.range_type == 3
			&& pdd.forecast.units == 1 )
		{
		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time1;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimeb,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));

		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time2;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimee,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));
		}

	else if ( pdd.forecast.range_type == 4
			&& pdd.forecast.units == 1 )
		{
		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time1;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimeb,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));

		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time2;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimee,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));
		}

	else if ( pdd.forecast.range_type == 5
			&& pdd.forecast.units == 1 )
		{
		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time1;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimeb,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));

		vyear   = year;
		vjday   = jday;
		vhour   = hour + pdd.forecast.time2;
		vminute = minute;
		(void) tnorm(&vyear, &vjday, &vhour, &vminute, &vsecond);
		(void) strcpy(vtimee,
				build_tstamp(vyear, vjday, vhour, vminute, FALSE, TRUE));
		}

	/* Error return for unrecognizable time range indicator or time unit */
	else
		{
		(void) fprintf(stderr, "[E0_grib_tstamps] Unrecognizable time range");
		(void) fprintf(stderr, " indicator: %d\n", pdd.forecast.range_type);
		(void) fprintf(stderr, "                or forecast time");
		(void) fprintf(stderr, " unit: %d\n", pdd.forecast.units);
		return FALSE;
		}

	/* Error return for unrecognizable valid times */
	if ( blank(vtimeb) || blank(vtimee) )
		{
		(void) fprintf(stderr, "[E0_grib_tstamps] Unrecognizable");
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

static	LOGICAL		E0_grib_elements

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		element,	/* string containing element label */
	STRING		units		/* string containing units label */
	)

	{
	int		num, nume;

	static	char	did[GRIB_LABEL_LEN];
	static	char	eid[GRIB_LABEL_LEN];

	/* Initialise element and units labels */
	(void) strcpy(element, "");
	(void) strcpy(units,   "");

	/* Check through list of element defintions for acceptable ones */
	for (num=0; num<NumElementDefs; num++)
		if ( ElementDefs[num].parameter == pdd.parameter )
			{
			(void) strcpy(element, ElementDefs[num].element_label);
			(void) strcpy(units, ElementDefs[num].units_label);
			return TRUE;
			}

	/* Set default for unrecognized element */
	(void) strcpy(did, "gribelement");
	num = strlen(did);
	nume = int_string(pdd.parameter, eid, GRIB_LABEL_LEN);
	if ( num > 0 && nume > 0 && (num+1+nume) < GRIB_LABEL_LEN )
		{
		(void) strcpy(element, did);
		(void) strcat(element, ":");
		(void) strcat(element, eid);

		/* Write message to request update from FPA development group */
		(void) fprintf(stderr, "[E0_grib_elements] Processing unrecognized");
		(void) fprintf(stderr, " element parameter: %d\n", pdd.parameter);
		(void) fprintf(stderr, "     Contact FPA Development Group to add");
		(void) fprintf(stderr, " this parameter to next FPA release\n");
		return TRUE;
		}

	/* Error return for unrecognizable element definition */
	(void) fprintf(stderr, "[E0_grib_elements] Unrecognizable");
	(void) fprintf(stderr, " element definition: %d\n", pdd.parameter);
	return FALSE;
	}

static	LOGICAL		E0_grib_levels

	(
	E1_Product_definition_data
				pdd,		/* GRIB product definition data */
	STRING		level		/* string containing level label */
	)

	{
	int		num, ilev, ilev1, ilev2;
	char	lev1[GRIB_LABEL_LEN];
	char	lev2[GRIB_LABEL_LEN];

	/* Initialise level */
	(void) strcpy(level, "");

	/* First check through list of predefined levels */
	for (num=0; num<NumLevelDefs; num++)
		if ( LevelDefs[num].type == pdd.layer.type )

				/* Level found in predefined list */
				{
				(void) strcpy(level, LevelDefs[num].level_label);
				return TRUE;
				}

	/* Determine the level (layer) if not found on predefined list */
	/* Note that we will only recognize level/layer indicator:     */
	/*  100  (Isobaric level in hPa)                               */
	/*  101  (Layer between two isobaric levels in hPa)            */
	/*  102  (Mean sea level)                                      */
	/*  107  (Sigma level times 100)                               */
	/*  108  (Layer between two sigma levels times 100)            */

	if ( pdd.layer.type == 100 )
		{
		ilev = (pdd.layer.top<<8) + pdd.layer.bottom;
		if ( int_string(ilev, lev1, GRIB_LABEL_LEN-2) )
			{
			(void) strcpy(level, lev1);
			(void) strcat(level, "mb");
			return TRUE;
			}
		}

	else if ( pdd.layer.type == 101 )
		{
		ilev1 = pdd.layer.top * 10;
		ilev2 = pdd.layer.bottom * 10;
		if ( int_string(ilev1, lev1, (GRIB_LABEL_LEN-3)/2)
				&& int_string(ilev2, lev2, (GRIB_LABEL_LEN-3)/2) )
			{
			(void) strcpy(level, lev1);
			(void) strcat(level, "-");
			(void) strcat(level, lev2);
			(void) strcat(level, "mb");
			return TRUE;
			}
		}

	else if ( pdd.layer.type == 102 )
		{
		(void) strcpy(level, "msl");
			return TRUE;
		}

	else if ( pdd.layer.type == 107 )
		{
		ilev = ((pdd.layer.top<<8) + pdd.layer.bottom) / 100;
		if ( int_string(ilev, lev1, GRIB_LABEL_LEN-5) )
			{
			(void) strcpy(level, lev1);
			(void) strcat(level, "sigma");
			return TRUE;
			}
		}

	else if ( pdd.layer.type == 108 )
		{
		ilev1 = pdd.layer.top;
		ilev2 = pdd.layer.bottom;
		if ( int_string(ilev1, lev1, (GRIB_LABEL_LEN-6)/2)
				&& int_string(ilev2, lev2, (GRIB_LABEL_LEN-6)/2) )
			{
			(void) strcpy(level, lev1);
			(void) strcat(level, "-");
			(void) strcat(level, lev2);
			(void) strcat(level, "sigma");
			return TRUE;
			}
		}

	/* Error return for unrecognizable level type or values */
	(void) fprintf(stderr, "[E0_grib_levels] Unrecognizable");
	(void) fprintf(stderr, " level type: %d", pdd.layer.type);
	(void) fprintf(stderr, "  or level values:");
	(void) fprintf(stderr, " %d  %d\n", pdd.layer.top, pdd.layer.bottom);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Section Decodes):                       *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
*     All routines based on E1_ routines in rgrib_edition1.c           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/***subroutine - E0_section0decoder()                                 */
/**********************************************************************/

static	int			E0_section0decoder

	(
	FILE *ip_file,
	E1_Indicator_block *isb
	)

	{
	char ip_char, *required_char;
	int nb_found,totalchar;

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

	/* Initialize Section0 data not present in Edition 0 */
	isb->length = 0;
	isb->edition = 0;

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
/***subroutine - E0_section1decoder()                                 */
/**********************************************************************/

static	int			E0_section1decoder

	(
	FILE *ip_file,
	E1_Product_definition_data *pdd
	)

	{
	int		i;
	Octet	octet;

	/*** GRIB PRODUCT DEFINITION SECTION ***/

	/* Extract Section1 data */
	pdd->length = fget3c(ip_file);
	pdd->edition = fgetc(ip_file);
	pdd->centre_id = fgetc(ip_file);
	pdd->model_id = fgetc(ip_file);
	pdd->grid_defn = fgetc(ip_file);
	octet = fgetc(ip_file);  /* SECTION 2 & 3 FLAGS ACCESSED */
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

	/* Check for proper length of Section1 data */
	/*  ... and read unused bytes             */
	if ( pdd->length < PDBED0_LENGTH || pdd->length > PDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section1 length: %d\n",
				pdd->length);
		return -2;
		}
	for ( i = PDBED0_LENGTH; i < pdd->length; i++ )
			(void) fgetc(ip_file);

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
/***subroutine - E0_latlongdecoder()                                  */
/**********************************************************************/

static	int			E0_latlongdecoder

	(
	FILE *ip_file,
	E1_Grid_description_data *gdd
	)

	{
	Octet	octet;
	int e1_scancode, icode;
	int i;

	/* Extract Section2 latlong grid data */
	gdd->defn.reg_ll.Ni = fget2c(ip_file);
	gdd->defn.reg_ll.Nj = fget2c(ip_file);

	gdd->defn.reg_ll.La1 = fget3c(ip_file);
	if (gdd->defn.reg_ll.La1 >= 0x800000)
		gdd->defn.reg_ll.La1 = 0x800000 - gdd->defn.reg_ll.La1;
	gdd->defn.reg_ll.Lo1 = fget3c(ip_file);
	if (gdd->defn.reg_ll.Lo1 >= 0x800000)
		gdd->defn.reg_ll.Lo1 = 0x800000 - gdd->defn.reg_ll.Lo1;

	gdd->defn.reg_ll.resltn = fgetc(ip_file);

	gdd->defn.reg_ll.La2 = fget3c(ip_file);
	if (gdd->defn.reg_ll.La2 >= 0x800000)
		gdd->defn.reg_ll.La2 = 0x800000 - gdd->defn.reg_ll.La2;
	gdd->defn.reg_ll.Lo2 = fget3c(ip_file);
	if (gdd->defn.reg_ll.Lo2 >= 0x800000)
		gdd->defn.reg_ll.Lo2 = 0x800000 - gdd->defn.reg_ll.Lo2;

	gdd->defn.reg_ll.Di = fget2c(ip_file);
	gdd->defn.reg_ll.Dj = fget2c(ip_file);

	octet = fgetc(ip_file);
	gdd->defn.reg_ll.scan_mode.west = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.reg_ll.scan_mode.north = GETBIT(octet, E1_scan_flag_north);
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

	/* Check for proper length of Section2 latitude/longitude grid */
	/*  ... and read unused bytes                                */
	if ( gdd->length < GDB_LATLON_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 latlong length: %d\n",
				gdd->length);
		return -2;
		}
	for ( i = GDB_LATLON_LENGTH; i < gdd->length; i++ )
			(void) fgetc(ip_file);

	gdd->defn.reg_ll.pole_extra = 0;

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
/***subroutine - E0_gaussecoder()                                     */
/**********************************************************************/

static	int			E0_gaussdecoder

	(
	FILE *ip_file,
	E1_Grid_description_data *gdd
	)

	{
	Octet	octet;
	int i;

	/* Extract Section2 gaussian grid data */
	gdd->defn.guas_ll.Ni = fget2c(ip_file);
	gdd->defn.guas_ll.Nj = fget2c(ip_file);

	gdd->defn.guas_ll.La1 = fget3c(ip_file);
	if (gdd->defn.guas_ll.La1 >= 0x800000)
		gdd->defn.guas_ll.La1 = 0x800000 - gdd->defn.guas_ll.La1;
	gdd->defn.guas_ll.Lo1 = fget3c(ip_file);
	if (gdd->defn.guas_ll.Lo1 >= 0x800000)
		gdd->defn.guas_ll.Lo1 = 0x800000 - gdd->defn.guas_ll.Lo1;

	gdd->defn.guas_ll.resltn = fgetc(ip_file);

	gdd->defn.guas_ll.La2 = fget3c(ip_file);
	if (gdd->defn.guas_ll.La2 >= 0x800000)
		gdd->defn.guas_ll.La2 = 0x800000 - gdd->defn.guas_ll.La2;
	gdd->defn.guas_ll.Lo2 = fget3c(ip_file);
	if (gdd->defn.guas_ll.Lo2 >= 0x800000)
		gdd->defn.guas_ll.Lo2 = 0x800000 - gdd->defn.guas_ll.Lo2;

	gdd->defn.guas_ll.Di = fget2c(ip_file);
	gdd->defn.guas_ll.N = fget2c(ip_file);

	octet = fgetc(ip_file);
	gdd->defn.guas_ll.scan_mode.west = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.guas_ll.scan_mode.north = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.guas_ll.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* Check for proper length of Section2 gaussian grid */
	/*  ... and read unused bytes                      */
	if ( gdd->length < GDB_GAUSS_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 gaussian length: %d\n",
				gdd->length);
		return -2;
		}
	for ( i = GDB_GAUSS_LENGTH; i < gdd->length; i++ )
			(void) fgetc(ip_file);

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
/***subroutine - E0_psdecoder()                                       */
/**********************************************************************/

static	int			E0_psdecoder

	(
	FILE *ip_file,
	E1_Grid_description_data *gdd
	)

	{
	Octet	octet;
	int i;

	/* Extract Section2 polarsteriographic grid data */
	gdd->defn.ps.Nx = fget2c(ip_file);
	gdd->defn.ps.Ny = fget2c(ip_file);

	gdd->defn.ps.La1 = fget3c(ip_file);
	if (gdd->defn.ps.La1 >= 0x800000)
		gdd->defn.ps.La1 = 0x800000 - gdd->defn.ps.La1;
	gdd->defn.ps.Lo1 = fget3c(ip_file);
	if (gdd->defn.ps.Lo1 >= 0x800000)
		gdd->defn.ps.Lo1 = 0x800000 -  gdd->defn.ps.Lo1;

	gdd->defn.ps.compnt = fgetc(ip_file);

	gdd->defn.ps.LoV = fget3c(ip_file);
	if (gdd->defn.ps.LoV >= 0x800000)
		gdd->defn.ps.LoV = 0x800000 -  gdd->defn.ps.LoV;

	gdd->defn.ps.Dx = fget3c(ip_file);
	gdd->defn.ps.Dy = fget3c(ip_file);

	octet = fgetc(ip_file);
	gdd->defn.ps.proj_centre.pole = GETBIT(octet, E1_proj_flag_pole);
	octet = fgetc(ip_file);
	gdd->defn.ps.scan_mode.west       = GETBIT(octet, E1_scan_flag_west);
	gdd->defn.ps.scan_mode.north      = GETBIT(octet, E1_scan_flag_north);
	gdd->defn.ps.scan_mode.horz_sweep = GETBIT(octet, E1_scan_flag_hsweep);

	/* Check for proper length of Section2 polar stereographic grid */
	/*  ... and read unused bytes                                 */
	if ( gdd->length < GDB_PSTEREO_LENGTH || gdd->length > GDB_MAX_LENGTH )
		{
		(void) fprintf(stderr, "\n  Error in Section2 pstereo length: %d\n",
				gdd->length);
		return -2;
		}
	for ( i = GDB_PSTEREO_LENGTH; i < gdd->length; i++ )
			(void) fgetc(ip_file);

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
/***subroutine - E0_section2decoder()                                 */
/**********************************************************************/

static	int			E0_section2decoder

	(
	FILE *ip_file,
	E1_Grid_description_data *gdd
	)

	{
	int iret = 0;

	/* Extract Section2 data */
	gdd->length = fget3c(ip_file);
	gdd->nv = fgetc(ip_file);
	gdd->pv_or_pl = fgetc(ip_file);     /* UNUSED OCTET */
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
		case LATLON_GRID:		/* LATITUDE-LONGITUDE */
			iret = E0_latlongdecoder(ip_file,gdd);
			break;

		case GAUSS_GRID:		/* GAUSSIAN */
			iret = E0_gaussdecoder(ip_file,gdd);
			break;

		case PSTEREO_GRID:		/* POLAR STEREOGRAPHIC */
			iret = E0_psdecoder(ip_file,gdd);
			break;

		default :
			(void) fprintf(stderr, "\n  Unrecognizable Section2 grid type: %d\n",
					gdd->dat_rep);
			iret = 2;
			break;
		}

	if ( iret != 0 ) return iret;

	dprintf(stderr, "  Completed Section2 decode\n");
	return 0;
	}

/**********************************************************************/
/***subroutine - E0_gdbdefaultrtn()                                   */
/**********************************************************************/

static	int			E0_gdbdefaultrtn

	(
	E1_Product_definition_data *pdd,
	E1_Grid_description_data *gdd
	)

	{
	/* Pointers into predefined grid tables */
	const E1_ll_grid_predefinition	*predef_ll = NullPtr(E1_ll_grid_predefinition *);
	const E1_ps_grid_predefinition	*predef_ps = NullPtr(E1_ps_grid_predefinition *);
	int predef_index;

	/* Set default grid description data */
	gdd->length = 0;
	gdd->nv = 0;
	gdd->pv_or_pl = 255;

	/* Set grid description for latitude-longitude grids */
	for(predef_index = 0; predef_index < E1_nb_predef_ll_grids; predef_index++)
		if ( E1_predef_ll_grids[predef_index].grid_defn == pdd->grid_defn )
			{
			/** LATITUDE/LONGITUDE **/

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

			gdd->defn.reg_ll.scan_mode.west = predef_ll->scan_mode.west;
			gdd->defn.reg_ll.scan_mode.north = predef_ll->scan_mode.north;
			gdd->defn.reg_ll.scan_mode.horz_sweep =
					predef_ll->scan_mode.horz_sweep;

			gdd->defn.reg_ll.pole_extra = predef_ll->pole_extra;
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
/***subroutine - E0_section3decoder()                                 */
/**********************************************************************/

static	int			E0_section3decoder

	(
	FILE *ip_file,
	E1_Bit_map_header *bmhd,
	Octet **pbit_data
	)

	{
	static Octet *bit_data = NullPtr(Octet *);
	unsigned int words_in_grib;

	/* Initialize pointer for returned bitmap data */
	*pbit_data = NullPtr(Octet *);

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

	/* Extract Section3 bitmap data */
	if ( bmhd->ntable == 0 )
		{

		/* Set number of values in remainder of Section3 */
		words_in_grib =  bmhd->length - BMB_LENGTH;

		/* Read the Section3 bitmap data */
		bit_data = GETMEM(bit_data, Octet, words_in_grib);
		(void) fread((void *)bit_data,words_in_grib,1,ip_file);

		/* Check for end-of-file or error in Section3 bitmap data */
		if ( feof(ip_file) )
			{
			(void) fprintf(stderr, "\n  End-of-file in Section3 bitmap data\n");
			return 1;
			}
		if ( ferror(ip_file) )
			{
			(void) fprintf(stderr, "\n  Error in Section3 bitmap data\n");
			return -1;
			}
		}

	/* Set pointer for returned data */
	*pbit_data = bit_data;
	dprintf(stderr, "  Completed Section3 decode\n");
	return 0;
	}

/**********************************************************************/
/***subroutine - E0_section4decoder()                                 */
/**********************************************************************/

static	int			E0_section4decoder

	(
	FILE *ip_file,
	E1_Product_definition_data *pdd,
	E1_Grid_description_data *gdd,
	E1_Binary_data_header *bdhd,
	unsigned int *numraw,
	float **praw_data,
	float *ppole_datum,
	int *nii,
	int *njj,
	int *dii,
	int *djj
	)

	{

	/*** SECTION INPUT AREAS (EXTERNAL FORMAT) ***/
	Octet *binary_ip;
	long bit_cursor;
	unsigned long packed_datum;
	char *required_char, ip_char;

	/*** DECODE AREAS (INTERNAL FORMAT) ***/
	static float *grib_data = NullFloat;
	static float pole_datum = 0.0;
	float tentopowermd;
	float *value_cursor;

	/*** MISC ***/
	int ni, nj, di, dj;
	unsigned int num_vals, total_vals, count;
	unsigned int words_in_grib, vals_in_grib;
	int ipole;

	/* Initialize pointers for returned GRIB data, pole datum and dimensions */
	*numraw = 0;
	*praw_data = NullFloat;
	*ppole_datum = 0.0;
	*nii = 0;
	*njj = 0;
	*dii = 0;
	*djj = 0;

	/* Extract Section4 header data */
	bdhd->length = fget3c(ip_file);
	bdhd->flags = fgetc(ip_file);
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

	/* Set number of values in remainder of Section4 */
	words_in_grib = bdhd->length - BDH_LENGTH;
	vals_in_grib = 8 * words_in_grib / bdhd->bits_per_val;

	/* Set grid dimensions */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:		/* LATITUDE-LONGITUDE */
			ni = gdd->defn.reg_ll.Ni;
			nj = gdd->defn.reg_ll.Nj;
			di = gdd->defn.reg_ll.Di;
			dj = gdd->defn.reg_ll.Dj;
			num_vals = ni * nj;
			ipole = gdd->defn.reg_ll.pole_extra;
			break;
		case GAUSS_GRID:		/* GAUSSIAN */
			ni = gdd->defn.guas_ll.Ni;
			nj = gdd->defn.guas_ll.Nj;
			di = gdd->defn.guas_ll.Di;
			dj = gdd->defn.guas_ll.N;
			num_vals = ni * nj;
			ipole = 0;
			break;
		case PSTEREO_GRID:		/* POLAR STEREOGRAPHIC */
			ni = gdd->defn.ps.Nx;
			nj = gdd->defn.ps.Ny;
			di = gdd->defn.ps.Dx;
			dj = gdd->defn.ps.Dy;
			num_vals = ni * nj;
			ipole = 0;
			break;
		default:			/* Error message for all other grid types */
			(void) fprintf(stderr,"  Unacceptable GRIB grid type: %d\n",
					gdd->dat_rep);
			return -2;
		}

	/* Check for error in number of values in GRIB data */
	total_vals = num_vals + abs(ipole);
	if ( total_vals > vals_in_grib )
		{
		(void) fprintf(stderr, "\n  Error in number of Section4 data values\n");
		(void) fprintf(stderr, "    total_vals: %i", total_vals);
		(void) fprintf(stderr, "    vals_in_grib: %i\n", vals_in_grib);
		return -9;
		}

	/* Read the Section4 data */
	binary_ip = INITMEM(Octet, words_in_grib);
	(void) fread((void *)binary_ip,words_in_grib,1,ip_file);

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

	/* Set the data values for each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	tentopowermd = pow(10.0, (double) -pdd->forecast.factor_d);
	bit_cursor = 0;

	/* Get pole value from first value */
	if ( ipole == -1 )
		{
		packed_datum = E0_extract_packed_datum(bit_cursor, bdhd->bits_per_val,
			binary_ip);
		pole_datum = tentopowermd *
			(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
		bit_cursor+=bdhd->bits_per_val;
		}

	/* Get grid values from interior values */
	value_cursor = grib_data;
	for(count=0;count<num_vals;count++)
		{
		packed_datum = E0_extract_packed_datum(bit_cursor, bdhd->bits_per_val,
			binary_ip);
		*value_cursor = tentopowermd *
			(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
		bit_cursor+=bdhd->bits_per_val;
		value_cursor++;
		}

	/* Get pole value from last value */
	if ( ipole == 1 )
		{
		packed_datum = E0_extract_packed_datum(bit_cursor, bdhd->bits_per_val,
			binary_ip);
		pole_datum = tentopowermd *
			(bdhd->reference + ldexp((double)packed_datum,bdhd->scale));
		bit_cursor+=bdhd->bits_per_val;
		}

	/* Free space used by the Section4 data */
	FREEMEM(binary_ip);

	/* Extract Section4 trailer string */
	required_char = GRIB_TRAILER;
	for (count = 0; count < GRIB_TRAILER_LENGTH; count++)
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
			(void) fprintf(stderr, "\n  Bad character in Section4 trailer\n");
			return -99;
			}

		required_char++;
		}

	/* Set pointers for returned data and grid dimensions */
	*numraw = num_vals;
	*praw_data = grib_data;
	*ppole_datum = pole_datum;
	*nii = ni;
	*njj = nj;
	*dii = di;
	*djj = dj;

	dprintf(stderr, "  Completed Section4 decode\n");
	return 0;
	}

/**********************************************************************/
/***subroutine - E0_reorder_data()                                    */
/**********************************************************************/

static	int			E0_reorder_data

	(
	E1_Product_definition_data *pdd,
	E1_Grid_description_data *gdd,
	unsigned int *numraw,
	float **praw_data,
	int *nii,
	int *njj,
	int *dii,
	int *djj,
	float **pgrib_data
	)

	{
	int				ii, ni, di, jj, nj, dj;
	short int		isweep;
	LOGICAL			left, bottom;
	unsigned int	num_vals;
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;

	/* Initialize pointer for returned GRIB data */
	*pgrib_data = NullFloat;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;
	di = *dii;
	dj = *djj;

	/* Set flags for data ordering */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:		/* LATITUDE-LONGITUDE */
			isweep = gdd->defn.reg_ll.scan_mode.horz_sweep;
			if ( gdd->defn.reg_ll.scan_mode.west == 0 )  left   = (LOGICAL) (di>0);
			else                                         left   = (LOGICAL) (di<0);
			if ( gdd->defn.reg_ll.scan_mode.north == 0 ) bottom = (LOGICAL) (dj<0);
			else                                         bottom = (LOGICAL) (dj>0);
			break;
		case GAUSS_GRID:		/* GAUSSIAN */
			isweep = gdd->defn.guas_ll.scan_mode.horz_sweep;
			if ( gdd->defn.guas_ll.scan_mode.west == 0 )  left   = (LOGICAL) (di>0);
			else                                          left   = (LOGICAL) (di<0);
			if ( gdd->defn.guas_ll.scan_mode.north == 0 ) bottom = (LOGICAL) (dj<0);
			else                                          bottom = (LOGICAL) (dj>0);
			break;
		case PSTEREO_GRID:		/* POLAR STEREOGRAPHIC */
			isweep = gdd->defn.ps.scan_mode.horz_sweep;
			if ( gdd->defn.ps.scan_mode.west == 0 )  left   = (LOGICAL) (di>0);
			else                                     left   = (LOGICAL) (di<0);
			if ( gdd->defn.ps.scan_mode.north == 0 ) bottom = (LOGICAL) (dj<0);
			else                                     bottom = (LOGICAL) (dj>0);
			break;
		default:			/* Error message for all other grid types */
			(void) fprintf(stderr,"  Unacceptable GRIB grid type: %d\n",
					gdd->dat_rep);
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
						value_in = *praw_data + (jj*ni) + ii;
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *praw_data + ((nj-jj-1)*ni) + ii;
						}
					}

				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *praw_data + (jj*ni) + (ni-ii-1);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *praw_data + ((nj-jj-1)*ni) + (ni-ii-1);
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
						value_in = *praw_data + jj + (ii*nj);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *praw_data + (nj-jj-1) + (ii*nj);
						}
					}

				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom )
						{
						value_in = *praw_data + jj + ((ni-ii-1)*nj);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *praw_data + (nj-jj-1) + ((ni-ii-1)*nj);
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
/***subroutine - E0_add_pole_data()                                   */
/**********************************************************************/

static	int			E0_add_pole_data

	(
	E1_Product_definition_data *pdd,
	E1_Grid_description_data *gdd,
	float *ppole_datum,
	int *nii,
	int *njj,
	float **pgrib_data
	)

	{
	int				ii, ni, jj, nj;
	short int		ipole;
	unsigned int	num_vals;
	static float	*grib_data = NullFloat;
	float			*value_in, *value_out;

	/* Set grid dimensions */
	ni = *nii;
	nj = *njj;

	/* Set flags for adding pole data */
	switch (gdd->dat_rep)
		{
		case LATLON_GRID:		/* LATITUDE-LONGITUDE */
			ipole = gdd->defn.reg_ll.pole_extra;
			break;
		case GAUSS_GRID:		/* GAUSSIAN */
			ipole = 0;
			break;
		case PSTEREO_GRID:		/* POLAR STEREOGRAPHIC */
			ipole = 0;
			break;
		default:			/* Error message for all other grid types */
			(void) fprintf(stderr,"  Unacceptable GRIB grid type: %d\n",
					gdd->dat_rep);
			return -2;
		}

	/* Set final grid dimensions (with pole data added) */
	*nii = ni;
	*njj = nj + abs(ipole);
	num_vals = *nii * *njj;

	/* Set the data values for each grid point */
	grib_data = GETMEM(grib_data, float, num_vals);
	value_out = grib_data;

	/* Initialize pointer to input data values  */
	/* Note that data has been reordered by row */
	value_in = *pgrib_data;

	/* Add row of pole data at start of data */
	if ( ipole == -1 )
		{
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
		for(ii=0; ii<ni; ii++)
			*value_out++ = *ppole_datum;
		}

	/* Set pointer for returned data */
	*pgrib_data = grib_data;

	dprintf(stderr, "  Completed addition of pole data\n");
	return 0;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Binary File Parsing):                   *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
*     All routines based on E1_ routines in rgrib_edition1.c           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/*                                                                    */
/***subroutine - E0_extract_packed_datum()                            */
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

static	unsigned long	E0_extract_packed_datum

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
	unsigned long	result = 0, mask = 1;
	int				shift;

	/* Determine which bytes contain the required bits */
	last_bit   = first_bit + nb_bits - 1;
	first_byte = &buffer[first_bit/8];
	last_byte  = &buffer[last_bit/8];
	nbytes     = last_byte - first_byte + 1;	/* No more than 4 bytes! */

	/* Deposit the bytes which contain the desired bits into result */
	/* Note that "result" contains 4 bytes!                         */
	(void) memcpy((void *)&result, (void *)first_byte, nbytes);

	/* Shift back until last_bit is right justified */
	shift = 8 - (last_bit+1)%8;
	if ( shift == 8 ) shift = 0;
	shift += 8 * (sizeof(result) - nbytes);
	if ( shift > 0 ) result >>= shift;

	/* Truncate to required number of bits */
	result &= ((mask << nb_bits) - 1);

	return result;
	}
