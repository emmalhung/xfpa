/**********************************************************************/
/** @file units.c
 *
 * Routines to access the units information in the field
 * presentation, setup block and config file and provide this
 * information for each field.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   u n i t s . c                                                      *
*                                                                      *
*   Routines to access the units information in the field              *
*   presentation, setup block and config file and provide this         *
*   information for each field.                                        *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (AES)            *
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

#include "presentation.h"
#include "config_structs.h"
#include "config_info.h"
#include "read_config.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/* Structure to hold info about a field member */
typedef	struct
	{
	STRING	name;
	STRING	type;
	USPEC	units;
	} FMEM;

/* Structure to hold info about a field */
typedef	struct
	{
	STRING	elem;
	STRING	level;
	STRING	model;
	STRING	type;
	short	nmember;
	FMEM	*members;
	} FLD;

/* Global variables to hold presentation info */
static	int		Cready  = FALSE;
static	int		Nfields = 0;
static	FLD		*Fields = NullPtr(FLD *);

/* Internal static functions */
static	LOGICAL	read_presentation_config(void);
static	LOGICAL	interpret_presentation_line(STRING);

/***********************************************************************
*                                                                      *
*     s e t u p _ m e t a f i l e _ u n i t s                          *
*     s e t u p _ f l d _ u n i t s                                    *
*     s e t u p _ s f c _ u n i t s                                    *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Setup units information for each field in the metafile.
 *
 *	@param[in]	meta	The metafile
 *	@param[in]	model	source of data
 * 	@return True if successful.
 **********************************************************************/
LOGICAL		setup_metafile_units

	(
	METAFILE	meta,
	STRING		model
	)

	{
	int		ifld;
	FIELD	fld;

	/* Is there something there? */
	if ( !meta ) return FALSE;

	/* Setup units for each field */
	for ( ifld=0; ifld<meta->numfld; ifld++ )
		{
		fld = meta->fields[ifld];
		(void) setup_fld_units(fld,model);
		}

	return TRUE;
	}

/**********************************************************************/
/** Setup units information for the appropriate kind of data.
 *
 *	@param[in]	fld		Field to setup
 *	@param[in]	model	source of data
 * 	@return True if successful.
 **********************************************************************/
LOGICAL	setup_fld_units

	(
	FIELD	fld,
	STRING	model
	)

	{
	STRING	ftype, ent, elem, level;
	POINTER	fdata;

	/* Is there something there? */
	if ( !fld ) return FALSE;

	/* See what type of field we have */
	recall_fld_data(fld,&ftype,&fdata);
	if ( !fdata )       return FALSE;
	if ( blank(ftype) ) return FALSE;

	/* Get element and level */
	recall_fld_info(fld,&ent,&elem,&level);

	/* Set up the units for the appropriate kind of data */
	if ( same(ftype,"surface") )
		return setup_sfc_units((SURFACE)fdata,elem,level,model);
	else if ( same(ftype,"set") )
		return FALSE;
	else if ( same(ftype,"plot") )
		return FALSE;
	else
		return FALSE;
	}

/**********************************************************************/
/** Setup units information for continuous and vector fields.
 *
 *	@param[in]	sfc 	surface to setup
 *	@param[in]	elem 	element
 *	@param[in]	level 	pressure level
 *	@param[in]	model	source of data
 *  @return True if successful.
 **********************************************************************/
LOGICAL	setup_sfc_units

	(
	SURFACE	sfc,
	STRING	elem,
	STRING	level,
	STRING	model
	)

	{
	STRING					mtype;
	FpaConfigFieldStruct	*fdef;
	USPEC					*units;

	if (!sfc)        return FALSE;
	if (blank(elem)) return FALSE;

	mtype = "continuous";
	fdef  = get_field_info(elem, level);
	if (fdef)
		{
		switch (fdef->element->fld_type)
			{
			case FpaC_CONTINUOUS:
					mtype = "continuous";
					break;

			case FpaC_VECTOR:
					mtype = "vector";
					break;
			}
		}

	if (get_unitspec(elem, level, model, mtype, NullString, &units))
		{
		change_surface_units(sfc, units);
		return TRUE;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   g e t _ u n i t s p e c                                            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Search for the units specified for a particular element, level,
 * source, member type, and member name.
 *
 *	@param[in]	elem	Element
 *	@param[in]	level	Level
 *	@param[in]	model	Source of data
 *	@param[in]	mtype	Member type
 *	@param[in]	mname	Member name
 *	@param[out]	**units	units
 * @return True if successful.
 **********************************************************************/
int		get_unitspec

	(
	STRING	elem,
	STRING	level,
	STRING	model,
	STRING	mtype,
	STRING	mname,
	USPEC	**units
	)

	{
	int		i, j;
	FLD		*fld;
	FMEM	*fmem;
    STRING  enrml, lnrml, mnrml;

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;

	/* Read setup ahead of config so that these are searched first */
	if (units) *units = NullPtr(USPEC *);
	if (!read_presentation_config()) return FALSE;

	/* Only certain types are known */
	if (   !blank(mtype)
		&& !same(mtype,"continuous")
		&& !same(mtype,"vector") ) return FALSE;

	edef = identify_element(elem);
	ldef = identify_level(level);
	sdef = identify_source(model, FpaCblank);
	enrml = (edef)? edef->name: elem;
	lnrml = (ldef)? ldef->name: level;
	mnrml = (sdef)? sdef->name: model;

	/* Scan the list of field presentations */
	for (i=0; i<Nfields; i++)
		{
		fld = Fields + i;

		if (   !blank(elem)
			&& !same(fld->elem,enrml)
			&& !same(fld->elem,"Any_Element") )  continue;

		if (   !blank(level)
			&& !same(fld->level,lnrml)
			&& !same(fld->level,"Any_Level") ) continue;

		if (   !blank(model)
			&& !same(fld->model,mnrml)
			&& !same(fld->model,"Any_Source") ) continue;

		for (j=0; j<fld->nmember; j++)
			{
			fmem = fld->members + j;

			if (   !blank(mtype)
				&& !same(fmem->type,mtype) ) continue;

			if (   !blank(mname)
				&& !same(fmem->name,mname) ) continue;

			/* Found a match */
			if (units) *units = &fmem->units;
			return TRUE;
			}
		}

	/* Didn't find requested field */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   STATIC (LOCAL) ROUTINES:                                           *
*                                                                      *
*   All routines after this point are available only within this file. *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ p r e s e n t a t i o n _ c o n f i g                    *
*                                                                      *
***********************************************************************/

typedef	enum { UxOrder, UxFtype, UxSupport, UxInvalid, UxCmd } UXMODE;

static	LOGICAL complain(UXMODE, STRING, STRING, FLD *, FMEM *);

/**********************************************************************/

static	LOGICAL	read_presentation_config(void)

	{
	STRING	line;
	FILE	*fp;

	/* Do nothing if already input */
	if (Cready) return TRUE;

	/* Open the presentation config file */
	if ( !first_config_file_open("presentation", &fp) )
		{
		(void) printf("Presentation config file unknown or not found\n");
		return FALSE;
		}

	/* Read the file */
	while ( line = read_config_file_line(&fp) )
		{
		(void) interpret_presentation_line(line);
		}

	/* Debug */
#	ifdef DEBUG_CONFIG
	for (i=0; i<Nfields; i++)
		{
		print_spec(Fields+i);
		}
#	endif /* DEBUG_CONFIG */

	return (Cready = TRUE);
	}

/**********************************************************************/

static	LOGICAL	complain
	(
	UXMODE mode,
	STRING need,
	STRING key,
	FLD *fld,
	FMEM *fmem)

	{
	STRING	fname;
	char	types[100];

	(void) config_file_location(NULL, &fname, NullLong);
	/* It would be nice to know which line of config file */
	/* but that info is buried in read_config_file_line() */

	pr_error("Presentation","In file: %s\n", fname);
	if (NotNull(fld))
		{
		pr_error("Presentation","> In field: %s %s %s\n",
				fld->elem, fld->level, fld->model);
		}
	if (NotNull(fmem))
		{
		pr_error("Presentation","> In member: %s %s\n",
				fmem->type, fmem->name);
		}

	switch (mode)
		{
		case UxOrder:
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Must follow \"%s\"\n",
				key, need);
			break;

		case UxFtype:
			(void) strcpy(types, ">");
			if (strchr(need, 'c')) (void) strcat(types, " continuous");
			if (strchr(need, 'v')) (void) strcat(types, " vector");
			if (strchr(need, 'd')) (void) strcat(types, " discrete");
			if (strchr(need, 'w')) (void) strcat(types, " wind");
			if (strchr(need, 'l')) (void) strcat(types, " line");
			if (strchr(need, 's')) (void) strcat(types, " scattered");
			if (strchr(need, 'x')) (void) strcat(types, " spot");
			if (strchr(need, 'p')) (void) strcat(types, " plot");
			(void) strcat(types, "\n");
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Field type must be one of:\n", key);
			pr_error("Presentation", types);
			break;

		case UxSupport:
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Field type must support %s\n",
				key, need);
			break;

		case UxInvalid:
			pr_error("Presentation",
				"> Invalid parameter value \"%s\" for \"%s\"\n",
				key, need);
			break;

		case UxCmd:
			pr_error("Presentation",
				"> Invalid command \"%s\" %s\n",
				need, need);
			break;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   i n t e r p r e t _ p r e s e n t a t i o n _ l i n e              *
*                                                                      *
***********************************************************************/

static	LOGICAL	interpret_presentation_line

	(
	STRING	line
	)

	{
	static	FLD		*fld       = NullPtr(FLD *);
	static	FMEM	*fmem      = NullPtr(FMEM *);
	static	USPEC	*units     = NullPtr(USPEC *);
	static	LOGICAL	contours   = FALSE;
	static	LOGICAL	categories = FALSE;
	static	LOGICAL	subfields  = FALSE;

	STRING	cmd, uname, ename, lname, mname;
	STRING	enrml, lnrml, mnrml;

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;
	FpaConfigUnitStruct		*udef;

	/* Return error if line is empty or null */
	if (blank(line)) return FALSE;

	/* The first argument on the line tells what to do */
	cmd = string_arg(line);

	/* Check for "field" specifier */
	if (same(cmd,"field"))
		{
		/* Unset all pointers to current structures */
		fld        = NullPtr(FLD *);
		fmem       = NullPtr(FMEM *);
		units      = NullPtr(USPEC *);
		contours   = FALSE;
		categories = FALSE;
		subfields  = FALSE;

		/* Add a new field */
		Nfields++;
		Fields       = GETMEM(Fields,FLD,Nfields);
		fld          = Fields + (Nfields-1);
        ename  = strdup_arg(line);
        lname  = strdup_arg(line);
        mname  = strdup_arg(line);

		edef = identify_element(ename);
		ldef = identify_level(lname);
		sdef = identify_source(mname, FpaCblank);
		/*
        if (same(ename, "ALL")) enrml = "-";
        if (same(lname, "ALL")) lnrml = "-";
        if (same(mname, "ALL")) mnrml = "-";
		*/
		enrml = (edef)? edef->name: ename;
		lnrml = (ldef)? ldef->name: lname;
		mnrml = (sdef)? sdef->name: mname;

        fld->elem    = strdup(enrml);
        fld->level   = strdup(lnrml);
        fld->model   = strdup(mnrml);
        fld->nmember = 0;
        fld->members = NullPtr(FMEM *);
        FREEMEM(ename);
        FREEMEM(lname);
        FREEMEM(mname);
		}

	/* Check for "member" specifier */
	else if (same(cmd,"member"))
		{
		/* Unset pointers to current field member */
		fmem       = NullPtr(FMEM *);
		units      = NullPtr(USPEC *);
		contours   = FALSE;
		categories = FALSE;
		subfields  = FALSE;
		if (!fld) return complain(UxOrder, "field", cmd, fld, fmem);

		/* Add a new member to the current field */
		fld->nmember++;
		fld->members   = GETMEM(fld->members,FMEM,fld->nmember);
		fmem           = fld->members + (fld->nmember-1);
		fmem->type     = strdup_arg(line);
		fmem->name     = strdup_arg(line);
		init_uspec(&fmem->units);

		if ( same(fmem->type,"continuous") ) contours   = TRUE;
		if ( same(fmem->type,"vector") )     contours   = TRUE;
		if ( same(fmem->type,"discrete") )   categories = TRUE;
		if ( same(fmem->type,"wind") )       categories = TRUE;
		if ( same(fmem->type,"area") )       categories = TRUE;
		if ( same(fmem->type,"barb") )       categories = TRUE;
		if ( same(fmem->type,"button") )     categories = TRUE;
		if ( same(fmem->type,"curve") )      categories = TRUE;
		if ( same(fmem->type,"label") )      categories = TRUE;
		if ( same(fmem->type,"mark") )       categories = TRUE;
		/*
		if ( same(fmem->type,"scattered") )  spotclass  = TRUE;
		if ( same(fmem->type,"spot") )       spotclass  = TRUE;
		*/
		if ( same(fmem->type,"plot") )       subfields  = TRUE;
		}

	/* Check for "units" specifier */
	else if (same(cmd,"units"))
		{
		/* Unset pointers to current units spec */
		units = NullPtr(USPEC *);
		if (!fld)      return complain(UxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(UxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(UxFtype, "cv", cmd, fld, fmem);

		units = &fmem->units;
		uname = strdup_arg(line);
		udef  = identify_unit(uname);
		if (udef)
			{
			define_uspec(units,uname,udef->factor,udef->offset);
			}
        FREEMEM(uname);
		}


	/* Ignore all other commands */
	else
		{
		return FALSE;
		}

	/* Successful */
	return TRUE;
	}
