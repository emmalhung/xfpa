/**********************************************************************/
/** @file presentation.c
 *
 * Routines to access the field presentation setup block and config
 * file and provide this information for each field.
 *
 * Version 8 &copy; Copyright 2009 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   p r e s e n t a t i o n . c                                        *
*                                                                      *
*   Routines to access the field presentation setup block and config   *
*   file and provide this information for each field.                  *
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
	short	nconspec;
	CONSPEC	*conspecs;
	short	ncatspec;
	CATSPEC	*catspecs;
	short	npltspec;
	PLTSPEC	*pltspecs;
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

static	LOGICAL	match_mtype(STRING, STRING);
static	LOGICAL	read_presentation_config(void);

/***********************************************************************
*                                                                      *
*     s e t u p _ m e t a f i l e _ p r e s e n t a t i o n            *
*     s e t u p _ f l d _ p r e s e n t a t i o n                      *
*     s e t u p _ s f c _ p r e s e n t a t i o n                      *
*     s e t u p _ s e t _ p r e s e n t a t i o n                      *
*     s e t u p _ p l o t _ p r e s e n t a t i o n                    *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Setup metafile presentation
 *
 *	@param[in]	meta	Metafile to present
 *	@param[in]	model	source of data (optional)
 * 	@return True if successful
 **********************************************************************/
LOGICAL		setup_metafile_presentation

	(
	METAFILE	meta,
	STRING		model
	)

	{
	int		ifld;
	FIELD	fld;

	/* Is there something there? */
	if ( !meta ) return FALSE;

	/* Setup presentation for each field */
	for ( ifld=0; ifld<meta->numfld; ifld++ )
		{
		fld = meta->fields[ifld];
		(void) setup_fld_presentation(fld, model);
		}

	(void) setup_metafile_presentation(meta->bgnd, model);
	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** setup field presentation.
 *
 *	@param[in]	fld    Field to present
 *	@param[in]	model	source of data (optional)
 * 	@return True if successful.
 **********************************************************************/
LOGICAL	setup_fld_presentation

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
	recall_fld_data(fld, &ftype, &fdata);
	if ( !fdata )       return FALSE;
	if ( blank(ftype) ) return FALSE;

	/* Get element and level */
	recall_fld_info(fld, &ent, &elem, &level);

	/* Set up the presentation for the appropriate kind of data */
	if ( same(ftype, "surface") )
		return setup_sfc_presentation((SURFACE)fdata, elem, level, model);
	else if ( same(ftype, "set") )
		return setup_set_presentation((SET)fdata, elem, level, model);
	else if ( same(ftype, "plot") )
		return setup_plot_presentation((PLOT)fdata, elem, level, model);
	else
		return FALSE;
	}

/**********************************************************************/

/**********************************************************************/
/** Setup Surface presentation.
 *
 *	@param[in]	sfc		Surface to present
 *	@param[in]	elem	element
 *	@param[in]	level	level (optional)
 *	@param[in]	model	source of data (optional)
 **********************************************************************/
LOGICAL	setup_sfc_presentation

	(
	SURFACE	sfc,
	STRING	elem,
	STRING	level,
	STRING	model
	)

	{
	int						nspec;
	CONSPEC					*specs;
	FpaConfigFieldStruct	*fdef;
	STRING					mtype;

#	ifdef DEBUG_PSPEC
	int						i;
#	endif /* DEBUG_PSPEC */

	if (!sfc)        return FALSE;
	if (blank(elem)) return FALSE;

	/* Must set up units first so contour values will be OK */
	(void) setup_sfc_units(sfc, elem, level, model);

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

	(void) pr_diag("Presentation", "Field Surface: %s %s %s %s\n",
			elem, level, model, mtype);

	nspec = get_conspecs(elem, level, model, mtype, NullString, &specs);

#	ifdef DEBUG_PSPEC
	if (pr_level("Presentation", 5))
		{
		for (i=0; i<nspec; i++)
			debug_conspec(specs+i, "conspec", 3);
		}
#	endif /* DEBUG_PSPEC */

	define_surface_conspecs(sfc, nspec, specs);
	invoke_surface_conspecs(sfc);
	return (LOGICAL) (nspec > 0);
	}

/**********************************************************************/

/**********************************************************************/
/** Setup Set presentation.
 *
 *	@param[in]	set     Set to present
 *	@param[in]	elem	element
 *	@param[in]	level	level (optional)
 *	@param[in]	model	source of data (optional)
 * 	@return True if successful.
 **********************************************************************/
LOGICAL	setup_set_presentation

	(
	SET		set,
	STRING	elem,
	STRING	level,
	STRING	model
	)

	{
	LOGICAL					xnode;
	int						ncspec, nxspec;
	CATSPEC					*cspecs, *xspecs;
	STRING					mtype;
	ITEM					item;
	FpaConfigFieldStruct	*fdef;

#	ifdef DEBUG_PSPEC
	int						i;
#	endif /* DEBUG_PSPEC */

	if (!set)        return FALSE;
	if (blank(elem)) return FALSE;

	mtype = set->type;
	xnode = (same(mtype, "lchain"))? TRUE: FALSE;

	fdef  = get_field_info(elem, level);
	if (fdef)
		{
		switch (fdef->element->fld_type)
			{
			case FpaC_DISCRETE:
					if (same(set->type, "area")) mtype = "discrete";
					break;

			case FpaC_WIND:
					if (same(set->type, "area")) mtype = "wind";
					break;

			case FpaC_SCATTERED:
					if (same(set->type, "spot")) mtype = "scattered";
					break;

			case FpaC_LCHAIN:
					xnode = TRUE;
					break;
			}
		}

	/* Get presentation for field */
	(void) pr_diag("Presentation", "Field Set: %s %s %s %s\n",
			elem, level, model, mtype);

	ncspec = get_catspecs(elem, level, model, mtype, NullString, &cspecs);

#	ifdef DEBUG_PSPEC
	if (pr_level("Presentation", 5))
		{
		for (i=0; i<ncspec; i++)
			debug_catspec(cspecs+i, "catspec", 3);
		}
#	endif /* DEBUG_PSPEC */

	/* Get secondary presentation for link chain nodes */
	if (xnode)
		{
		(void) pr_diag("Presentation", "Field Set: %s %s %s nodes\n",
				elem, level, model);

		nxspec = get_catspecs(elem, level, model, "nodes", NullString,
																&xspecs);

#		ifdef DEBUG_PSPEC
		if (pr_level("Presentation", 5))
			{
			for (i=0; i<nxspec; i++)
				debug_catspec(xspecs+i, "catspec", 3);
			}
#		endif /* DEBUG_PSPEC */
		}
	else
		{
		nxspec = 0;
		xspecs = (CATSPEC *) 0;
		}

	/* Set presentation for set */
	define_set_catspecs(set, ncspec, cspecs);
	define_set_secondary_catspecs(set, nxspec, xspecs);
	invoke_set_catspecs(set);
	return (LOGICAL) (ncspec > 0);
	}

/**********************************************************************/

/**********************************************************************/
/** Setup plot presentation.
 *
 *	@param[in]	plot	Plot to present
 *	@param[in]	elem	element
 *	@param[in]	level	level (optional)
 *	@param[in]	model	source of data (optional)
 *  @return True if successful.
 **********************************************************************/
LOGICAL	setup_plot_presentation

	(
	PLOT	plot,
	STRING	elem,
	STRING	level,
	STRING	model
	)

	{
	int		nspec;
	PLTSPEC	*specs;
#	ifdef DEBUG_PSPEC
	int						i;
#	endif /* DEBUG_PSPEC */

	if (!plot)       return FALSE;
	if (blank(elem)) return FALSE;

	(void) pr_diag("Presentation", "Field Plot: %s %s %s\n",
			elem, level, model);

	nspec = get_pltspecs(elem, level, model, "plot", NullString, &specs);

#	ifdef DEBUG_PSPEC
	if (pr_level("Presentation", 5))
		{
		for (i=0; i<nspec; i++)
			debug_pltspec(specs+i, "pltspec", 3);
		}
#	endif /* DEBUG_PSPEC */

	define_plot_pltspecs(plot, nspec, specs);
	invoke_plot_pltspecs(plot);
	return (LOGICAL) (nspec > 0);
	}

/***********************************************************************
*                                                                      *
*   g e t _ c o n s p e c s                                            *
*   g e t _ c a t s p e c s                                            *
*   g e t _ p l t s p e c s                                            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Get Contour specifications for a given element, level, model,
 * member type and member name.
 *
 *	@param[in]	elem		Element name
 *	@param[in]	level		Level name
 *	@param[in]	model		Model name
 *	@param[in]	mtype		Member type
 *	@param[in]	mname		Member name
 *	@param[out]	**conspecs	a list of contour specifications
 *  @return The size of the conspecs list.
 **********************************************************************/
int		get_conspecs

	(
	STRING	elem,
	STRING	level,
	STRING	model,
	STRING	mtype,
	STRING	mname,
	CONSPEC	**conspecs
	)

	{
	int		i, j;
	FLD		*fld;
	FMEM	*fmem;
	STRING	enrml, lnrml, mnrml;

#	ifdef DEBUG_PSPEC
	int		n;
#	endif /* DEBUG_PSPEC */

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;

	/* Read setup ahead of config so that these are searched first */
	if (conspecs) *conspecs = NullPtr(CONSPEC *);
	if (!read_presentation_config()) return 0;

	/* Only certain types are known */
	if (   !blank(mtype)
		&& !same(mtype, "continuous")
		&& !same(mtype, "vector") ) return 0;

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
			&& !same(fld->elem, enrml)
			&& !same(fld->elem, "Any_Element") )  continue;

		if (   !blank(level)
			&& !same(fld->level, lnrml)
			&& !same(fld->level, "Any_Level") ) continue;

		if (   !blank(model)
			&& !same(fld->model, mnrml)
			&& !same(fld->model, "Any_Source") ) continue;

		for (j=0; j<fld->nmember; j++)
			{
			fmem = fld->members + j;

			if ( !match_mtype(mtype, fmem->type) ) continue;

			if (   !blank(mname)
				&& !same(fmem->name, mname) ) continue;

			/* Debug */
#			ifdef DEBUG_PSPEC
			(void) printf("[Presentation] Presentation for: %s %s %s %s %s\n",
					elem, level, model, mtype, mname);
			for (n=0; n<fmem->nconspec; n++)
				debug_conspec(&(fmem->conspecs[n]), "conspec:", 3);
#			endif /* DEBUG_PSPEC */

			/* Found a match */
			if (conspecs) *conspecs = fmem->conspecs;
			return fmem->nconspec;
			}
		}

	/* Didn't find requested field */
	(void) pr_info("Presentation", "No presentation for: %s %s %s %s %s\n",
			elem, level, model, mtype, mname);
	return 0;
	}

/**********************************************************************/

/**********************************************************************/
/** Get category specification for a given element, level, model
 * member type, and memeber name.
 *	@param[in]	elem		 Element name
 *	@param[in]	level		 Level name
 *	@param[in]	model		 Model name
 *	@param[in]	mtype		 Member type
 *	@param[in]	mname		 Member nam
 *	@param[out]	**catspecs	 category spec list
 *  @return size of catspecs list.
 **********************************************************************/
int		get_catspecs

	(
	STRING	elem,
	STRING	level,
	STRING	model,
	STRING	mtype,
	STRING	mname,
	CATSPEC	**catspecs
	)

	{
	int		i, j;
	FLD		*fld;
	FMEM	*fmem;
	STRING	enrml, lnrml, mnrml;

#	ifdef DEBUG_PSPEC
	int		n;
#	endif /* DEBUG_PSPEC */

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;

	/* Read setup ahead of config so that these are searched first */
	if (catspecs) *catspecs = NullPtr(CATSPEC *);
	if (!read_presentation_config()) return 0;

	/* Only certain types are known */
	if (   !blank(mtype)
		&& !same(mtype, "discrete")
		&& !same(mtype, "wind")
		&& !same(mtype, "area")
		&& !same(mtype, "barb")
		&& !same(mtype, "button")
		&& !same(mtype, "curve")
		&& !same(mtype, "label")
		&& !same(mtype, "mark")
		&& !same(mtype, "scattered")
		&& !same(mtype, "spot")
		&& !same(mtype, "lchain")
		&& !same(mtype, "nodes") ) return 0;

#	ifdef DEBUG_PSPEC
	(void) pr_diag("Presentation",
					"  Matching mtype: \"%s\"  mname: \"%s\"\n", mtype, mname);
#	endif /* DEBUG_PSPEC */

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
			&& !same(fld->elem, enrml)
			&& !same(fld->elem, "Any_Element") )  continue;

		if (   !blank(level)
			&& !same(fld->level, lnrml)
			&& !same(fld->level, "Any_Level") ) continue;

		if (   !blank(model)
			&& !same(fld->model, mnrml)
			&& !same(fld->model, "Any_Source") ) continue;

		for (j=0; j<fld->nmember; j++)
			{
			fmem = fld->members + j;

#			ifdef DEBUG_PSPEC
			(void) pr_diag("Presentation",
					"    Checking member type: \"%s\"  name: \"%s\"\n",
					fmem->type, fmem->name);
#			endif /* DEBUG_PSPEC */

			if ( !match_mtype(mtype, fmem->type) ) continue;

			if (   !blank(mname)
				&& !same(fmem->name, mname) ) continue;

			/* Debug */
#			ifdef DEBUG_PSPEC
			(void) printf("[Presentation] Presentation for: %s %s %s %s %s\n",
					elem, level, model, mtype, mname);
			for (n=0; n<fmem->ncatspec; n++)
				debug_catspec(&(fmem->catspecs[n]), "catspec:", 3);
#			endif /* DEBUG_PSPEC */

			/* Found a match */
			if (catspecs) *catspecs = fmem->catspecs;
			return fmem->ncatspec;
			}
		}

	/* Didn't find requested field */
	(void) pr_info("Presentation", "No presentation for: %s %s %s %s %s\n",
			elem, level, model, mtype, mname);
	return 0;
	}

/**********************************************************************/

/**********************************************************************/
/** Get plot specifications for a given element, level, model,
 * member type, member name.
 *
 * 	@param[in]	elem		Element name
 * 	@param[in]	level		Level name
 * 	@param[in]	model		Model name
 * 	@param[in]	mtype		Member type
 * 	@param[in]	mname		Member name
 * 	@param[in]	**pltspecs	list of plot specifications
 *  @return The size of the pltspecs list.
 **********************************************************************/
int		get_pltspecs

	(
	STRING	elem,
	STRING	level,
	STRING	model,
	STRING	mtype,
	STRING	mname,
	PLTSPEC	**pltspecs
	)

	{
	int		i, j;
	FLD		*fld;
	FMEM	*fmem;
	STRING	enrml, lnrml, mnrml;

#	ifdef DEBUG_PSPEC
	int		n;
#	endif /* DEBUG_PSPEC */

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;

	/* Read setup ahead of config so that these are searched first */
	if (pltspecs) *pltspecs = NullPtr(PLTSPEC *);
	if (!read_presentation_config()) return 0;

	/* Only certain types are known */
	if (   !blank(mtype)
		&& !same(mtype, "plot") ) return 0;

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
			&& !same(fld->elem, enrml)
			&& !same(fld->elem, "Any_Element") )  continue;

		if (   !blank(level)
			&& !same(fld->level, lnrml)
			&& !same(fld->level, "Any_Level") ) continue;

		if (   !blank(model)
			&& !same(fld->model, mnrml)
			&& !same(fld->model, "Any_Source") ) continue;

		for (j=0; j<fld->nmember; j++)
			{
			fmem = fld->members + j;

			if ( !match_mtype(mtype, fmem->type) ) continue;

			if (   !blank(mname)
				&& !same(fmem->name, mname) ) continue;

			/* Debug */
#			ifdef DEBUG_PSPEC
			(void) printf("[Presentation] Presentation for: %s %s %s %s %s\n",
					elem, level, model, mtype, mname);
			for (n=0; n<fmem->npltspec; n++)
				debug_pltspec(&(fmem->pltspecs[n]), "pltspec:", 3);
#			endif /* DEBUG_PSPEC */

			/* Found a match */
			if (pltspecs) *pltspecs = fmem->pltspecs;
			return fmem->npltspec;
			}
		}

	/* Didn't find requested field */
	(void) pr_info("Presentation", "No presentation for: %s %s %s %s %s\n",
			elem, level, model, mtype, mname);
	return 0;
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
*   m a t c h _ m t y p e                                              *
*                                                                      *
***********************************************************************/

static	LOGICAL	match_mtype
	(
	STRING	mtype,
	STRING	ptype
	)

	{
	if (blank(mtype)) return TRUE;

	/* Look for exact match of requested type first */
	if (same(ptype, mtype)) return TRUE;

	/* Look for fallback types */
	if (same(mtype, "vector"))
		{
		if (same(ptype, "continuous")) return TRUE;
		return FALSE;
		}
	if (same(mtype, "wind"))
		{
		if (same(ptype, "discrete")) return TRUE;
		if (same(ptype, "area"))     return TRUE;
		return FALSE;
		}
	if (same(mtype, "discrete"))
		{
		if (same(ptype, "area"))     return TRUE;
		return FALSE;
		}
	if (same(mtype, "area"))
		{
		if (same(ptype, "discrete")) return TRUE;
		return FALSE;
		}
	if (same(mtype, "scattered"))
		{
		if (same(ptype, "spot"))      return TRUE;
		return FALSE;
		}
	if (same(mtype, "lchain"))
		{
		if (same(ptype, "track"))     return TRUE;
		if (same(ptype, "nodes"))     return TRUE;
		return FALSE;
		}

	/* Not a match */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   r e a d _ p r e s e n t a t i o n _ c o n f i g                    *
*                                                                      *
***********************************************************************/

typedef enum { PxOrder, PxFtype, PxSupport, PxInvalid, PxNeed, PxCmd,
														PxObsolete } PXMODE;

static	LOGICAL	complain(PXMODE, STRING, STRING, FLD *, FMEM *);
static	LOGICAL	interpret_presentation_line(STRING);

#ifdef DEBUG_PSPEC
static	void	debug_presentation(void);
#endif /* DEBUG_PSPEC */

/**********************************************************************/

static	LOGICAL	read_presentation_config(void)

	{
	STRING	line;
	FILE	*fp;

	/* Do nothing if already input */
	if (Cready) return TRUE;

	/* Force read the Fields config */
	(void) identify_element("doesn't matter");

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
#	ifdef DEBUG_PSPEC
	debug_presentation();
#	endif /* DEBUG_PSPEC */

	return (Cready = TRUE);
	}

/**********************************************************************/

static	LOGICAL	complain
	(
	PXMODE mode,
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
		case PxOrder:
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Must follow \"%s\"\n",
				key, need);
			break;

		case PxFtype:
			(void) strcpy(types, ">");
			if (strchr(need, 'c')) (void) strcat(types, " continuous");
			if (strchr(need, 'v')) (void) strcat(types, " vector");
			if (strchr(need, 'd')) (void) strcat(types, " discrete");
			if (strchr(need, 'w')) (void) strcat(types, " wind");
			if (strchr(need, 'l')) (void) strcat(types, " line");
			if (strchr(need, 's')) (void) strcat(types, " scattered");
			if (strchr(need, 'x')) (void) strcat(types, " spot");
			if (strchr(need, 'p')) (void) strcat(types, " plot");
			if (strchr(need, 'n')) (void) strcat(types, " lchain");
			(void) strcat(types, "\n");
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Field type must be one of:\n", key);
			pr_error("Presentation", types);
			break;

		case PxSupport:
			pr_error("Presentation",
				"> Invalid use of \"%s\" - Field type must support %s\n",
				key, need);
			break;

		case PxInvalid:
			pr_error("Presentation",
				"> Invalid parameter value \"%s\" for \"%s\"\n",
				key, need);
			break;

		case PxNeed:
			pr_error("Presentation",
				"> Need parameter \"%s\" for command \"%s\"\n",
				need, key);
			break;

		case PxCmd:
			pr_error("Presentation",
				"> Invalid command \"%s %s\"\n",
				key, need);
			break;

		case PxObsolete:
			pr_error("Presentation",
				"> Obsolete parameter \"%s\" - Replace with \"%s\"\n",
				key, need);
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
	static	CONSPEC	*conspec   = NullPtr(CONSPEC *);
	static	CATSPEC	*catspec   = NullPtr(CATSPEC *);
	static	PLTSPEC	*pltspec   = NullPtr(PLTSPEC *);
	static	LOGICAL	contours   = FALSE;
	static	LOGICAL	categories = FALSE;
	static	LOGICAL	spotclass  = FALSE;
	static	LOGICAL	spotmember = FALSE;
	static	LOGICAL	subfields  = FALSE;
	static	LOGICAL	nodeclass  = FALSE;
	static	float	Dwidth     = 0;
	static	LSTYLE	Dstyle     = -1;
	static	STRING	memclass   = NULL;
	static	STRING	memtype    = NULL;
	static	STRING	memname    = NULL;
	static	STRING	memattr    = NULL;

	STRING	class, cname, ctype, cattr, ccat, cval, symbol, pattern;
	char	cmd[100];
	char	symbuf[10];
	int		sym, vmult;
	float	cmin, cmax, cstd, cint, width, length, size, space, angle;
	STRING	ename, lname, mname, uname;
	STRING	enrml, lnrml, mnrml;
	LOGICAL	scale, cross, tofrom, doval;
	STRING	pname;
	COLOUR	colour, tcolour, bcolour;
	LSTYLE	lstyle;
	FSTYLE	fstyle;
	FONT	font;
	MTYPE	mtype;
	BTYPE	btype;
	LOGICAL	valid;
	POINT	offset;

	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigSourceStruct	*sdef;
	FpaConfigUnitStruct		*udef;

	/* Return error if line is empty or null */
	if (blank(line)) return FALSE;

	/* The first argument on the line tells what to do */
	strcpy_arg(cmd, line, &valid);

	/* Catch "debug" flag */
	if (same(cmd, "debug"))
		{
		printf("Debug in Presentation: %s\n", line);
		}

	/* Check for "field" specifier */
	else if (same(cmd, "field"))
		{
		/* Unset all pointers to current structures */
		fld        = NullPtr(FLD *);
		fmem       = NullPtr(FMEM *);
		units      = NullPtr(USPEC *);
		conspec    = NullPtr(CONSPEC *);
		catspec    = NullPtr(CATSPEC *);
		pltspec    = NullPtr(PLTSPEC *);
		contours   = FALSE;
		categories = FALSE;
		spotclass  = FALSE;
		spotmember = FALSE;
		subfields  = FALSE;
		nodeclass  = FALSE;

		/* Add a new field */
		Nfields++;
		Fields = GETMEM(Fields, FLD, Nfields);
		fld    = Fields + (Nfields-1);
        ename  = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
        lname  = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
        mname  = (!blank(line))? strdup_arg(line): strdup(FpaCblank);

		edef = identify_element(ename);
		ldef = identify_level(lname);
		sdef = identify_source(mname, FpaCblank);
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
	else if (same(cmd, "member"))
		{
		/* Unset pointers to current field member */
		fmem       = NullPtr(FMEM *);
		units      = NullPtr(USPEC *);
		conspec    = NullPtr(CONSPEC *);
		catspec    = NullPtr(CATSPEC *);
		pltspec    = NullPtr(PLTSPEC *);
		contours   = FALSE;
		categories = FALSE;
		spotclass  = FALSE;
		spotmember = FALSE;
		subfields  = FALSE;
		nodeclass  = FALSE;
		if (!fld) return complain(PxOrder, "field", cmd, fld, fmem);

		/* Add a new member to the current field */
		fld->nmember++;
		fld->members   = GETMEM(fld->members, FMEM, fld->nmember);
		fmem           = fld->members + (fld->nmember-1);
		fmem->type     = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		fmem->name     = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		init_uspec(&fmem->units);
		fmem->nconspec = 0;
		fmem->conspecs = NullPtr(CONSPEC *);
		fmem->ncatspec = 0;
		fmem->catspecs = NullPtr(CATSPEC *);
		fmem->npltspec = 0;
		fmem->pltspecs = NullPtr(PLTSPEC *);

		if ( same(fmem->type, "continuous") ) contours   = TRUE;
		if ( same(fmem->type, "vector") )     contours   = TRUE;
		if ( same(fmem->type, "discrete") )   categories = TRUE;
		if ( same(fmem->type, "wind") )       categories = TRUE;
		if ( same(fmem->type, "area") )       categories = TRUE;
		if ( same(fmem->type, "barb") )       categories = TRUE;
		if ( same(fmem->type, "button") )     categories = TRUE;
		if ( same(fmem->type, "curve") )      categories = TRUE;
		if ( same(fmem->type, "label") )      categories = TRUE;
		if ( same(fmem->type, "mark") )       categories = TRUE;
		if ( same(fmem->type, "scattered") )  spotclass  = TRUE;
		if ( same(fmem->type, "spot") )       spotclass  = TRUE;
		if ( same(fmem->type, "plot") )       subfields  = TRUE;
		if ( same(fmem->type, "lchain") )     categories = TRUE;
		if ( same(fmem->type, "nodes") )      nodeclass  = TRUE;
		}

	/* Check for "units" specifier */
	else if (same(cmd, "units"))
		{
		/* Unset pointers to current units spec */
		units = NullPtr(USPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		units = &fmem->units;
		uname = string_arg(line);
		udef  = identify_unit(uname);
		if (IsNull(udef)) return complain(PxInvalid, cmd, uname, fld, fmem);
		define_uspec(units, udef->name, udef->factor, udef->offset);
		}

	/* Check for "contour" specifier */
	else if (same(cmd, "contour"))
		{
		/* Unset pointers to current contour spec */
		conspec = NullPtr(CONSPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		/* What type of contour spec is it? */
		ctype = string_arg(line);

		/* 'range' . . . cmin cmax cstd cint */
		if (same(ctype, "range"))
			{
			fmem->nconspec++;
			fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
			conspec = fmem->conspecs + (fmem->nconspec-1);
			init_conspec(conspec);
			cmin = float_arg(line, &valid);	if (!valid) cmin = -MAXFLOAT;
			cmax = float_arg(line, &valid);	if (!valid) cmax = MAXFLOAT;
			cstd = float_arg(line, &valid);	if (!valid) cstd = 0.0;
			cint = float_arg(line, &valid);	if (!valid) cint = 0.0;
			define_conspec_range(conspec, cmin, cmax, cstd, cint);
			skip_lspec(&conspec->lspec);
			skip_fspec(&conspec->fspec);
			skip_tspec(&conspec->tspec);
			skip_bspec(&conspec->bspec);
			skip_mspec(&conspec->mspec);
			}

		/* 'list' . . . cval cval . . . */
		else if (same(ctype, "list"))
			{
			fmem->nconspec++;
			fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
			conspec = fmem->conspecs + (fmem->nconspec-1);
			init_conspec(conspec);
			define_conspec_list(conspec, 0, NullFloat, NullStringList);
			while (TRUE)
				{
				cstd = float_arg(line, &valid);
				if (!valid) break;
				add_cval_to_conspec(conspec, cstd, NullString);
				}
			skip_lspec(&conspec->lspec);
			skip_fspec(&conspec->fspec);
			skip_tspec(&conspec->tspec);
			skip_bspec(&conspec->bspec);
			skip_mspec(&conspec->mspec);
			}
		}

	/* Check for "vector" specifier */
	/* 'vector' . . . vmult */
	else if (same(cmd, "vector"))
		{
		/* Unset pointers to current contour spec */
		conspec = NullPtr(CONSPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		fmem->nconspec++;
		fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
		conspec = fmem->conspecs + (fmem->nconspec-1);
		init_conspec(conspec);
		vmult = int_arg(line, &valid);	if (!valid) vmult = 1;
		define_conspec_vector(conspec, vmult);
		skip_lspec(&conspec->lspec);
		skip_fspec(&conspec->fspec);
		skip_tspec(&conspec->tspec);
		skip_bspec(&conspec->bspec);
		skip_mspec(&conspec->mspec);
		conspec->mspec.type = 1;
		}

	/* Check for "maxima" specifier */
	/* 'maxima' . . . cmin cmax */
	else if (same(cmd, "maxima"))
		{
		/* Unset pointers to current contour spec */
		conspec = NullPtr(CONSPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		fmem->nconspec++;
		fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
		conspec = fmem->conspecs + (fmem->nconspec-1);
		init_conspec(conspec);
		cmin = float_arg(line, &valid);	if (!valid) cmin = -MAXFLOAT;
		cmax = float_arg(line, &valid);	if (!valid) cmax = MAXFLOAT;
		define_conspec_special(conspec, "maxima", cmin, cmax);
		skip_lspec(&conspec->lspec);
		skip_fspec(&conspec->fspec);
		skip_tspec(&conspec->tspec);
		skip_bspec(&conspec->bspec);
		skip_mspec(&conspec->mspec);
		conspec->mspec.type = 1;
		}

	/* Check for "minima" specifier */
	/* 'minima' . . . cmin cmax */
	else if (same(cmd, "minima"))
		{
		/* Unset pointers to current contour spec */
		conspec = NullPtr(CONSPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		fmem->nconspec++;
		fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
		conspec = fmem->conspecs + (fmem->nconspec-1);
		init_conspec(conspec);
		cmin = float_arg(line, &valid);	if (!valid) cmin = -MAXFLOAT;
		cmax = float_arg(line, &valid);	if (!valid) cmax = MAXFLOAT;
		define_conspec_special(conspec, "minima", cmin, cmax);
		skip_lspec(&conspec->lspec);
		skip_fspec(&conspec->fspec);
		skip_tspec(&conspec->tspec);
		skip_bspec(&conspec->bspec);
		skip_mspec(&conspec->mspec);
		conspec->mspec.type = 7;
		}

	/* Check for "saddle" specifier */
	/* 'saddle' . . . cmin cmax */
	else if (same(cmd, "saddle"))
		{
		/* Unset pointers to current contour spec */
		conspec = NullPtr(CONSPEC *);
		if (!fld)      return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)     return complain(PxOrder, "member", cmd, fld, fmem);
		if (!contours) return complain(PxFtype, "cv", cmd, fld, fmem);

		fmem->nconspec++;
		fmem->conspecs = GETMEM(fmem->conspecs, CONSPEC, fmem->nconspec);
		conspec = fmem->conspecs + (fmem->nconspec-1);
		init_conspec(conspec);
		cmin = float_arg(line, &valid);	if (!valid) cmin = -MAXFLOAT;
		cmax = float_arg(line, &valid);	if (!valid) cmax = MAXFLOAT;
		define_conspec_special(conspec, "saddle", cmin, cmax);
		skip_lspec(&conspec->lspec);
		skip_fspec(&conspec->fspec);
		skip_tspec(&conspec->tspec);
		skip_bspec(&conspec->bspec);
		skip_mspec(&conspec->mspec);
		conspec->mspec.type = 4;
		}

	/* Check for "class" specifier */
	/* 'class' . . . member_class */
	else if (same(cmd, "class"))
		{
		/* Unset pointers to current category spec */
		catspec = NullPtr(CATSPEC *);
		if (!fld)       return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)      return complain(PxOrder, "member", cmd, fld, fmem);
		if (!spotclass && !nodeclass)
			{
			if (!spotclass) return complain(PxFtype, "sx", cmd, fld, fmem);
			if (!nodeclass) return complain(PxFtype, "n",  cmd, fld, fmem);
			}

		FREEMEM(memclass);
		FREEMEM(memtype);
		FREEMEM(memname);
		FREEMEM(memattr);
		memclass = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		spotmember = TRUE;
		categories = FALSE;
		}

	/* Check for "class_member" specifier */
	/* 'class_member' . . . member_type member_name */
	else if (same(cmd, "class_member"))
		{
		/* Unset pointers to current category spec */
		catspec = NullPtr(CATSPEC *);
		if (!fld)        return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)       return complain(PxOrder, "member", cmd, fld, fmem);
		if (!spotmember) return complain(PxOrder, "class", cmd, fld, fmem);

		FREEMEM(memtype);
		FREEMEM(memname);
		FREEMEM(memattr);
		memtype = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		memname = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		memattr = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		categories = TRUE;
		}

	/* Check for "category" specifier */
	/* 'category' . . . cat_name */
	else if (same(cmd, "category"))
		{
		/* Unset pointers to current category spec */
		catspec = NullPtr(CATSPEC *);
		if (!fld)        return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)       return complain(PxOrder, "member", cmd, fld, fmem);
		if (!categories) return complain(PxSupport, "attributes", cmd, fld, fmem);

		/* What type of category spec is it? */
		class = (spotmember)? memclass: "";
		cname = (spotmember)? memname:  "";
		cattr = (spotmember)? memattr:  "";
		ctype = (spotmember)? memtype:  "";
		ccat  = "category";
		cval  = string_arg(line);

		fmem->ncatspec++;
		fmem->catspecs = GETMEM(fmem->catspecs, CATSPEC, fmem->ncatspec);
		catspec = fmem->catspecs + (fmem->ncatspec-1);
		init_catspec(catspec);
		define_catspec(catspec, class, cname, ctype, ccat, cval, cattr, ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		}

	/* Check for "attribute" specifier */
	/* 'attribute' . . . attrib_name attrib_value */
	else if (same(cmd, "attribute"))
		{
		/* Unset pointers to current category spec */
		catspec = NullPtr(CATSPEC *);
		if (!fld)        return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)       return complain(PxOrder, "member", cmd, fld, fmem);
		if (!categories) return complain(PxSupport, "attributes", cmd, fld, fmem);

		/* What type of category spec is it? */
		class = (spotmember)? memclass: "";
		cname = (spotmember)? memname:  "";
		cattr = (spotmember)? memattr:  "";
		ctype = (spotmember)? memtype:  "";
		ccat  = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		cval  = string_arg(line);

		fmem->ncatspec++;
		fmem->catspecs = GETMEM(fmem->catspecs, CATSPEC, fmem->ncatspec);
		catspec = fmem->catspecs + (fmem->ncatspec-1);
		init_catspec(catspec);
		define_catspec(catspec, class, cname, ctype, ccat, cval, cattr, ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);

		FREEMEM(ccat);
		}

	/* Check for "default" specifier */
	/* 'default' */
	else if (same(cmd, "default"))
		{
		/* Unset pointers to current category spec */
		catspec = NullPtr(CATSPEC *);
		if (!fld)        return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)       return complain(PxOrder, "member", cmd, fld, fmem);
		if (!categories) return complain(PxSupport, "attributes", cmd, fld, fmem);

		/* What type of category spec is it? */
		class = (spotmember)? memclass: "";
		cname = (spotmember)? memname:  "";
		cattr = (spotmember)? memattr:  "";
		ctype = (spotmember)? memtype:  "";
		ccat  = "default";
		cval  = "";

		fmem->ncatspec++;
		fmem->catspecs = GETMEM(fmem->catspecs, CATSPEC, fmem->ncatspec);
		catspec = fmem->catspecs + (fmem->ncatspec-1);
		init_catspec(catspec);
		define_catspec(catspec, class, cname, ctype, ccat, cval, cattr, ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		}

	/* Check for "subfield" specifier */
	/* 'subfield' . . . sub_type sub_name */
	else if (same(cmd, "subfield"))
		{
		/* Unset pointers to current subfield spec */
		pltspec = NullPtr(PLTSPEC *);
		if (!fld)       return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem)      return complain(PxOrder, "member", cmd, fld, fmem);
		if (!subfields) return complain(PxFtype, "p", cmd, fld, fmem);

		/* What type of subfield spec is it? */
		ctype = (!blank(line))? strdup_arg(line): strdup(FpaCblank);
		cname = string_arg(line);

		fmem->npltspec++;
		fmem->pltspecs = GETMEM(fmem->pltspecs, PLTSPEC, fmem->npltspec);
		pltspec = fmem->pltspecs + (fmem->npltspec-1);
		init_pltspec(pltspec);
		define_pltspec(pltspec, ctype, cname, ZeroPoint, 0.0);
		skip_lspec(&pltspec->lspec);
		skip_fspec(&pltspec->fspec);
		skip_tspec(&pltspec->tspec);
		skip_mspec(&pltspec->mspec);
		skip_bspec(&pltspec->bspec);

		FREEMEM(ctype);
		}

	/* Check for "offset" specifier */
	else if (same(cmd, "offset"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname     = string_arg(line);
		offset[X] = find_offset(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);
		pname     = string_arg(line);
		offset[Y] = find_offset(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->bspec.xvoff = offset[X];
			conspec->bspec.yvoff = offset[Y];
			}
		else if (categories && catspec)
			{
			copy_point(catspec->offset, offset);
			catspec->bspec.xvoff = offset[X];
			catspec->bspec.yvoff = offset[Y];
			}
		else if (subfields && pltspec)
			{
			copy_point(pltspec->offset, offset);
			pltspec->bspec.xvoff = offset[X];
			pltspec->bspec.yvoff = offset[Y];
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "bvaloff" specifier */
	else if (same(cmd, "bvaloff"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname     = string_arg(line);
		offset[X] = find_offset(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);
		pname     = string_arg(line);
		offset[Y] = find_offset(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->bspec.xvoff = offset[X];
			conspec->bspec.yvoff = offset[Y];
			}
		else if (categories && catspec)
			{
			catspec->bspec.xvoff = offset[X];
			catspec->bspec.yvoff = offset[Y];
			}
		else if (subfields && pltspec)
			{
			pltspec->bspec.xvoff = offset[X];
			pltspec->bspec.yvoff = offset[Y];
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "bfactor" specifier ... obsolete */
	else if (same(cmd, "bfactor"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		(void) complain(PxObsolete, "btype", cmd, fld, fmem);
		}

	/* Check for "pattern" specifier */
	else if (same(cmd, "pattern"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pattern = string_arg(line);
		if (blank(pattern)) return complain(PxInvalid, cmd, pattern, fld, fmem);

		if (contours && conspec)
			{
			conspec->lspec.pattern = STRMEM(conspec->lspec.pattern, pattern);
			}

		else if (categories && catspec)
			{
			catspec->lspec.pattern = STRMEM(catspec->lspec.pattern, pattern);
			}

		else if (subfields && pltspec)
			{
			pltspec->lspec.pattern = STRMEM(pltspec->lspec.pattern, pattern);
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "colour" specifier */
	else if (same(cmd, "colour"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname  = string_arg(line);
		colour = find_colour(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->lspec.colour = colour;
			conspec->tspec.colour = colour;
			conspec->bspec.colour = colour;
			conspec->mspec.colour = colour;
			}

		else if (categories && catspec)
			{
			catspec->lspec.colour = colour;
			catspec->tspec.colour = colour;
			catspec->mspec.colour = colour;
			catspec->bspec.colour = colour;
			}

		else if (subfields && pltspec)
			{
			pltspec->lspec.colour = colour;
			pltspec->tspec.colour = colour;
			pltspec->mspec.colour = colour;
			pltspec->bspec.colour = colour;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "bcolour" specifier */
	else if (same(cmd, "bcolour"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname   = string_arg(line);
		bcolour = find_colour(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname   = string_arg(line);
		tcolour = find_colour(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->bspec.colour = bcolour;
			conspec->tspec.colour = tcolour;
			}
		if (categories && catspec)
			{
			catspec->bspec.colour = bcolour;
			catspec->tspec.colour = tcolour;
			}
		else if (subfields && pltspec)
			{
			pltspec->bspec.colour = bcolour;
			pltspec->tspec.colour = tcolour;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "shadow" specifier */
	else if (same(cmd, "shadow"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname   = string_arg(line);
		tcolour = find_colour(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname   = string_arg(line);
		bcolour = find_colour(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.tcolour = tcolour;
			conspec->mspec.tcolour = tcolour;
			conspec->tspec.bcolour = bcolour;
			conspec->mspec.bcolour = bcolour;
			}

		else if (categories && catspec)
			{
			catspec->tspec.tcolour = tcolour;
			catspec->mspec.tcolour = tcolour;
			catspec->tspec.bcolour = bcolour;
			catspec->mspec.bcolour = bcolour;
			}

		else if (subfields && pltspec)
			{
			pltspec->tspec.tcolour = tcolour;
			pltspec->mspec.tcolour = tcolour;
			pltspec->tspec.bcolour = bcolour;
			pltspec->mspec.bcolour = bcolour;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "fill" specifier */
	else if (same(cmd, "fill"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		if (blank(line))
			{
			colour = find_colour(pname, &valid);
			if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

			fstyle = find_fstyle("solid_fill", &valid);
			if (!valid) return complain(PxInvalid, cmd, "solid_fill", fld, fmem);
			}
		else
			{
			fstyle = find_fstyle(pname, &valid);
			if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

			pname  = string_arg(line);
			colour = find_colour(pname, &valid);
			if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);
			}

		if (contours && conspec)
			{
			conspec->fspec.style  = fstyle;
			conspec->fspec.colour = colour;
			}

		else if (categories && catspec)
			{
			catspec->fspec.style  = fstyle;
			catspec->fspec.colour = colour;
			}

		else if (subfields && pltspec)
			{
			pltspec->fspec.style  = fstyle;
			pltspec->fspec.colour = colour;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "hatch" specifier */
	else if (same(cmd, "hatch"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		cross = find_poption(pname, "cross", &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname = string_arg(line);
		space = find_size(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		angle = float_arg(line, &valid);
		if (!valid) return complain(PxInvalid, cmd, line, fld, fmem);

		if (contours && conspec)
			{
			conspec->fspec.cross = cross;
			conspec->fspec.space = space;
			conspec->fspec.angle = angle;
			}

		else if (categories && catspec)
			{
			catspec->fspec.cross = cross;
			catspec->fspec.space = space;
			catspec->fspec.angle = angle;
			}

		else if (subfields && pltspec)
			{
			pltspec->fspec.cross = cross;
			pltspec->fspec.space = space;
			pltspec->fspec.angle = angle;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "style" specifier */
	else if (same(cmd, "style"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname  = string_arg(line);
		lstyle = find_lstyle(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->lspec.style = lstyle;
			if (conspec->lspec.width == SkipWidth)
				conspec->lspec.width = Dwidth;
			}

		else if (categories && catspec)
			{
			catspec->lspec.style = lstyle;
			if (catspec->lspec.width == SkipWidth)
				catspec->lspec.width = Dwidth;
			}

		else if (subfields && pltspec)
			{
			pltspec->lspec.style = lstyle;
			if (pltspec->lspec.width == SkipWidth)
				pltspec->lspec.width = Dwidth;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "btype" specifier */
	else if (same(cmd, "btype"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		btype = find_btype(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname  = string_arg(line);
		tofrom = find_poption(pname, "sense", &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname = string_arg(line);
		doval = find_poption(pname, "value", &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		pname = string_arg(line);
		if (blank(pname))
			{
			if (blank(fmem->units.name))
				return complain(PxNeed, "units", cmd, fld, fmem);
			uname = fmem->units.name;
			}
		else
			{
			udef  = identify_unit(pname);
			if (IsNull(udef)) return complain(PxInvalid, cmd, pname, fld, fmem);
			uname = udef->name;
			}

		if (contours && conspec)
			{
			conspec->bspec.type  = btype;
			conspec->bspec.sense = tofrom;
			conspec->bspec.value = doval;
			conspec->bspec.uname = STRMEM(conspec->bspec.uname, uname);
			}
		else if (categories && catspec)
			{
			catspec->bspec.type  = btype;
			catspec->bspec.sense = tofrom;
			catspec->bspec.value = doval;
			catspec->bspec.uname = STRMEM(catspec->bspec.uname, uname);
			}
		else if (subfields && pltspec)
			{
			pltspec->bspec.type  = btype;
			pltspec->bspec.sense = tofrom;
			pltspec->bspec.value = doval;
			pltspec->bspec.uname = STRMEM(pltspec->bspec.uname, uname);
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "width" specifier */
	else if (same(cmd, "width"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		width = find_lwidth(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);
		if (Dstyle < 0) Dstyle = find_lstyle("solid", &valid);

		if (contours && conspec)
			{
			if (conspec->lspec.style == SkipLstyle)
				conspec->lspec.style = Dstyle;
			conspec->lspec.width = width;
			conspec->bspec.width = width;
			}

		else if (categories && catspec)
			{
			if (catspec->lspec.style == SkipLstyle)
				catspec->lspec.style = Dstyle;
			catspec->lspec.width = width;
			catspec->bspec.width = width;
			}

		else if (subfields && pltspec)
			{
			if (pltspec->lspec.style == SkipLstyle)
				pltspec->lspec.style = Dstyle;
			pltspec->lspec.width = width;
			pltspec->bspec.width = width;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "length" specifier */
	else if (same(cmd, "length"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname  = string_arg(line);
		length = find_size(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->lspec.length = length;
			conspec->bspec.length = length;
			}

		else if (categories && catspec)
			{
			catspec->lspec.length = length;
			catspec->bspec.length = length;
			}

		else if (subfields && pltspec)
			{
			pltspec->lspec.length = length;
			pltspec->bspec.length = length;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "font" specifier */
	else if (same(cmd, "font"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		font  = find_font(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.font = font;
			conspec->mspec.font = font;
			}

		else if (categories && catspec)
			{
			catspec->tspec.font = font;
			catspec->mspec.font = font;
			}

		else if (subfields && pltspec)
			{
			pltspec->tspec.font = font;
			pltspec->mspec.font = font;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "scale" specifier */
	else if (same(cmd, "scale"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		scale = find_poption(pname, "scale", &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->lspec.scale = scale;
			conspec->fspec.scale = scale;
			conspec->tspec.scale = scale;
			conspec->bspec.scale = scale;
			conspec->mspec.scale = scale;
			}

		else if (categories && catspec)
			{
			catspec->lspec.scale = scale;
			catspec->fspec.scale = scale;
			catspec->tspec.scale = scale;
			catspec->mspec.scale = scale;
			catspec->bspec.scale = scale;
			}

		else if (subfields && pltspec)
			{
			pltspec->lspec.scale = scale;
			pltspec->fspec.scale = scale;
			pltspec->tspec.scale = scale;
			pltspec->mspec.scale = scale;
			pltspec->bspec.scale = scale;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "size" specifier */
	else if (same(cmd, "size"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		size = find_size(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.size = size;
			conspec->mspec.size = size;
			}

		else if (categories && catspec)
			{
			catspec->tspec.size = size;
			catspec->mspec.size = size;
			}

		else if (subfields && pltspec)
			{
			pltspec->tspec.size = size;
			pltspec->mspec.size = size;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "angle" specifier */
	else if (same(cmd, "angle"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		angle = float_arg(line, &valid);
		if (!valid) return complain(PxInvalid, cmd, line, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.angle = angle;
			conspec->mspec.angle = angle;
			}

		else if (categories && catspec)
			{
			catspec->angle = angle;
			catspec->tspec.angle = angle;
			catspec->mspec.angle = angle;
			}

		else if (subfields && pltspec)
			{
			pltspec->angle = angle;
			pltspec->tspec.angle = angle;
			pltspec->mspec.angle = angle;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "hjust" specifier */
	else if (same(cmd, "hjust"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		if (blank(pname)) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.hjust = pname[0];
			conspec->mspec.hjust = pname[0];
			}

		else if (categories && catspec)
			{
			catspec->tspec.hjust = pname[0];
			catspec->mspec.hjust = pname[0];
			}

		else if (subfields && pltspec)
			{
			pltspec->tspec.hjust = pname[0];
			pltspec->mspec.hjust = pname[0];
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "vjust" specifier */
	else if (same(cmd, "vjust"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		if (blank(pname)) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->tspec.vjust = pname[0];
			conspec->mspec.vjust = pname[0];
			}

		else if (categories && catspec)
			{
			catspec->tspec.vjust = pname[0];
			catspec->mspec.vjust = pname[0];
			}

		else if (subfields && pltspec)
			{
			pltspec->tspec.vjust = pname[0];
			pltspec->mspec.vjust = pname[0];
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "marker" specifier */
	else if (same(cmd, "marker"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		pname = string_arg(line);
		mtype = find_mtype(pname, &valid);
		if (!valid) return complain(PxInvalid, cmd, pname, fld, fmem);

		if (contours && conspec)
			{
			conspec->mspec.type = mtype;
			}

		else if (categories && catspec)
			{
			catspec->mspec.type = mtype;
			}

		else if (subfields && pltspec)
			{
			pltspec->mspec.type = mtype;
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Check for "symbol" specifier */
	else if (same(cmd, "symbol"))
		{
		if (!fld)  return complain(PxOrder, "field", cmd, fld, fmem);
		if (!fmem) return complain(PxOrder, "member", cmd, fld, fmem);

		symbol = string_arg(line);
		if (blank(symbol)) return complain(PxInvalid, cmd, symbol, fld, fmem);
		sym = symbol[0];
		if (sym == '\\')
			{
			(void) sscanf(symbol+1, "%o", &sym);
			(void) sprintf(symbuf, "%c", sym);
			symbol = symbuf;
			}

		if (contours && conspec)
			{
			conspec->mspec.type   = -1;
			conspec->mspec.symbol = STRMEM(conspec->mspec.symbol, symbol);
			}

		else if (categories && catspec)
			{
			catspec->mspec.type   = -1;
			catspec->mspec.symbol = STRMEM(catspec->mspec.symbol, symbol);
			}

		else if (subfields && pltspec)
			{
			pltspec->mspec.type   = -1;
			pltspec->mspec.symbol = STRMEM(pltspec->mspec.symbol, symbol);
			}

		else return complain(PxFtype, "cvdwp", cmd, fld, fmem);
		}

	/* Unknown command */
	else
		{
		return complain(PxCmd, line, cmd, fld, fmem);
		}

	/* Successful */
	return TRUE;
	}

/**********************************************************************/

#ifdef DEBUG_PSPEC
static	void	debug_presentation(void)

	{
	int		ii, jj, kk;
	FLD		*fld;
	FMEM	*fmem;

	for (ii=0; ii<Nfields; ii++)
		{
		fld = Fields + ii;
		(void) printf("Presentation field: %d  elem: %s  level: %s  model: %s\n",
				ii, fld->elem, fld->level, fld->model);
		for (jj=0; jj<fld->nmember; jj++)
			{
			fmem = fld->members + jj;
			(void) printf("  Member: %d  name: %s  type: %s  nspec con/cat/plt: %d/%d/%d\n",
					jj, fmem->name, fmem->type,
					fmem->nconspec, fmem->ncatspec, fmem->npltspec);
			for (kk=0; kk<fmem->nconspec; kk++)
				debug_conspec(&(fmem->conspecs[kk]), "conspec:", 3);
			for (kk=0; kk<fmem->ncatspec; kk++)
				debug_catspec(&(fmem->catspecs[kk]), "catspec:", 3);
			for (kk=0; kk<fmem->npltspec; kk++)
				debug_pltspec(&(fmem->pltspecs[kk]), "pltspec:", 3);
			}
		}
	}
#endif /* DEBUG_PSPEC */
