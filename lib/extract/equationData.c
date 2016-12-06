/*********************************************************************/
/**	@file equationData.c
 *
 * Routines to access Equation Database and manipulate objects used
 * in equations.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   e q u a t i o n D a t a . c                                        *
*                                                                      *
*   Routines to access Equation Database and manipulate objects        *
*   used in equations                                                  *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#define EQUATION_DATA		/* To initialize defined constants and */
							/*  functions in equation.h file       */

#include "equation.h"

#include <environ/environ.h>
#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_macros.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG_EQUATION		/* Turn on/off internal debug printing */
	static	LOGICAL	DebugMode = TRUE;
#else
	static	LOGICAL	DebugMode = FALSE;
#endif /* DEBUG_EQUATION */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                  */
/*  ... these are defined in equation.h */

/* Internal static functions (Equation Database) */
static void			init_equation_database_field(FpaEQTN_FLDS *);
static int			find_field_in_equation_database(FLD_DESCRIPT *);
static int			get_field_for_equation_database(FLD_DESCRIPT *);
static int			add_field_to_equation_database(FLD_DESCRIPT *, MAP_PROJ *,
							FIELD);
static FIELD		vector_field_from_equation_database(FLD_DESCRIPT *);
static FIELD		xycomp_field_from_equation_database(FLD_DESCRIPT *);
static FIELD		default_field_from_equation_database(FLD_DESCRIPT *);

/**********************************************************************
 ***                                                                ***
 *** c l e a r _ e q u a t i o n _ d a t a b a s e                  ***
 *** f i e l d _ f r o m _ e q u a t i o n _ d a t a b a s e        ***
 *** r e p l a c e _ f i e l d _ i n _ e q u a t i o n _            ***
 ***                                             d a t a b a s e    ***
 *** d e l e t e _ f i e l d _ i n _ e q u a t i o n _              ***
 ***                                             d a t a b a s e    ***
 ***                                                                ***
 **********************************************************************/

/* Storage locations for fields in Equation Database Objects */
static	int				NumEqtnFlds  = 0;
static	int				LastEqtnFld  = -1;
static	FpaEQTN_FLDS	*FpaEqtnFlds = NullPtr(FpaEQTN_FLDS *);


/*********************************************************************/
/** Initialize global storage for all fields used in equations.
 *********************************************************************/
void				clear_equation_database

	(
	)

	{
	int			inum;

	/* Return now if nothing to clear */
	if ( NumEqtnFlds <= 0 ) return;

	/* Initialize each FpaEQTN_FLDS storage structure in Equation Database */
	for ( inum=0; inum<NumEqtnFlds; inum++ )
		{
		(void) init_equation_database_field(&FpaEqtnFlds[inum]);
		}

	/* Now free all FpaEQTN_FLDS storage structures */
	FREEMEM(FpaEqtnFlds);
	FpaEqtnFlds = NullPtr(FpaEQTN_FLDS *);

	/* Reset counters */
	NumEqtnFlds = 0;
	LastEqtnFld = NumEqtnFlds - 1;
	}

/**********************************************************************/

/*********************************************************************/
/** Get a copy of a FIELD Object from global Equation Database.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a FIELD object.
 *********************************************************************/
FIELD				field_from_equation_database

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	int						fieldmacro;
	FpaConfigFieldStruct	*fdef;
	FLD_DESCRIPT			descript;

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) ) return NullFld;
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	/* Ensure that detailed field information has been read */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return NullFld;

	/* Set the type of field (if already defined) */
	if ( fdesc->fmacro != FpaCnoMacro ) fieldmacro = fdesc->fmacro;

	/* Otherwise, set default type of field from field information */
	else                                fieldmacro = fdef->element->fld_type;

	/* Enter detailied field information back into the field descriptor */
	(void) copy_fld_descript(&descript, fdesc);
	(void) set_fld_descript(&descript,
								FpaF_ELEMENT,     fdef->element,
								FpaF_LEVEL,       fdef->level,
								FpaF_FIELD_MACRO, fieldmacro,
								FpaF_END_OF_LIST);

	/* Check database for vector fields */
	if ( fieldmacro == FpaC_VECTOR
				&& xy_component_field(fdef->element->name) )
		{
		return vector_field_from_equation_database(&descript);
		}

	/* Check database for x/y component fields */
	else if ( fieldmacro == FpaC_CONTINUOUS
				&& xy_component_field(fdef->element->name) )
		{
		return xycomp_field_from_equation_database(&descript);
		}

	/* Check database for all other fields fields */
	else
		{
		return default_field_from_equation_database(&descript);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Supply a FIELD Object to add to the global Equation Database
 *  it will replace a matching FIELD if one exists.
 *
 *	@param[in]	*fdesc		Field descriptor
 *	@param[in]	*mproj		Map Projection
 *	@param[in]	fld			Field to replace.
 *********************************************************************/
void	replace_field_in_equation_database

	(
	FLD_DESCRIPT	*fdesc,
	MAP_PROJ		*mproj,
	FIELD			fld
	)

	{
	int						fieldmacro, inum;
	FpaConfigFieldStruct	*fdef;
	FLD_DESCRIPT			descript;

	/* Return if no field descriptor or map projection or field */
	if ( IsNull(fdesc) ) return;
	if ( IsNull(mproj) ) return;
	if ( IsNull(fld) )   return;

	/* Return if no valid field descriptor information */
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return;

	/* Ensure that detailed field information has been read */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return;

	/* Set the type of field (if already defined) */
	if ( fdesc->fmacro != FpaCnoMacro ) fieldmacro = fdesc->fmacro;

	/* Otherwise, set default type of field from field information */
	else                                fieldmacro = fdef->element->fld_type;

	/* Enter detailied field information back into the field descriptor */
	(void) copy_fld_descript(&descript, fdesc);
	(void) set_fld_descript(&descript,
								FpaF_ELEMENT,     fdef->element,
								FpaF_LEVEL,       fdef->level,
								FpaF_FIELD_MACRO, fieldmacro,
								FpaF_END_OF_LIST);

	/* If field is in Equation Database we will need to replace it */
	inum = find_field_in_equation_database(&descript);
	if ( inum >= 0 )
		{
		(void) copy_fld_descript(&FpaEqtnFlds[inum].descript, &descript);
		(void) copy_map_projection(&FpaEqtnFlds[inum].mproj, mproj);
		FpaEqtnFlds[inum].fld = fld;
		return;
		}

	/* Otherwise add it */
	(void) add_field_to_equation_database(&descript, mproj, fld);
	}

/**********************************************************************/

/*********************************************************************/
/** Remove the specified FIELD Object from the global Equation
 *  Database.
 *
 *	@param[in]	*fdesc		field descriptor
 *********************************************************************/
void	delete_field_in_equation_database

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	int						fieldmacro, inum;
	FpaConfigFieldStruct	*fdef;
	FLD_DESCRIPT			descript;

	/* Return if no field descriptor */
	if ( IsNull(fdesc) ) return;

	/* Return if no valid field descriptor information */
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return;

	/* Ensure that detailed field information has been read */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return;

	/* Set the type of field (if already defined) */
	if ( fdesc->fmacro != FpaCnoMacro ) fieldmacro = fdesc->fmacro;

	/* Otherwise, set default type of field from field information */
	else                                fieldmacro = fdef->element->fld_type;

	/* Enter detailied field information back into the field descriptor */
	(void) copy_fld_descript(&descript, fdesc);
	(void) set_fld_descript(&descript,
								FpaF_ELEMENT,     fdef->element,
								FpaF_LEVEL,       fdef->level,
								FpaF_FIELD_MACRO, fieldmacro,
								FpaF_END_OF_LIST);

	/* See if field is in Equation Database */
	inum = find_field_in_equation_database(&descript);
	if ( inum < 0 ) return;

	/* Remove it (empty it so it can't be found) */
	(void) init_equation_database_field(&FpaEqtnFlds[inum]);
	}

/**********************************************************************
 ***                                                                ***
 *** i n i t _ e q t n _ d a t a                                    ***
 *** f r e e _ e q t n _ d a t a                                    ***
 *** c o p y _ e q t n _ d a t a                                    ***
 *** c o n v e r t _ e q t n _ d a t a                              ***
 *** d e b u g _ e q t n _ d a t a                                  ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Initialize equation data.
 *
 *	@param[in]	type		type of Object in structure
 * 	@return Pointer to initialized structure for evaluations. You will
 * 			need to free this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*init_eqtn_data

	(
	short			type
	)

	{
	FpaEQTN_DATA	*pfld;

	/* Create structure */
	pfld = INITMEM(FpaEQTN_DATA, 1);

	/* Initialize structure based on type of Object */

	/* ... SCALAR Object */
	if ( type == FpaEQT_Scalar )
		{
		pfld->Type = FpaEQT_Scalar;
		(void) init_scalar(&pfld->Data.scalr);
		}

	/* ... GRID Object */
	else if ( type == FpaEQT_Grid )
		{
		pfld->Type = FpaEQT_Grid;
		(void) init_grid(&pfld->Data.gridd);
		}

	/* ... SPLINE Object */
	else if ( type == FpaEQT_Spline )
		{
		pfld->Type = FpaEQT_Spline;
		(void) init_spline(&pfld->Data.splne);
		}

	/* ... VLIST Object */
	else if ( type == FpaEQT_Vlist )
		{
		pfld->Type = FpaEQT_Vlist;
		(void) init_vlist(&pfld->Data.vlist);
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[init_eqtn_data] Unknown Object type: %d\n",
				type);
		FREEMEM(pfld);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return initialized structure */
	return pfld;
	}

/**********************************************************************/

/*********************************************************************/
/** Free memory for structure for evaluations.
 *
 *	@param[in]	*pfld		Equation Data structure
 *********************************************************************/
void				free_eqtn_data

	(
	FpaEQTN_DATA	*pfld
	)

	{
	/* Do nothing if null */
	if ( IsNull(pfld) ) return;

	/* Free memory in structure based on type of Object */

	/* ... SCALAR Object */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) free_scalar(&pfld->Data.scalr);
		}

	/* ... GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		(void) free_grid(&pfld->Data.gridd);
		}

	/* ... SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		(void) free_spline(&pfld->Data.splne);
		}

	/* ... VLIST Object */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) free_vlist(&pfld->Data.vlist);
		}

	/* Free structure and set pointer to Null */
	FREEMEM(pfld);
	pfld = NullPtr(FpaEQTN_DATA *);
	}

/**********************************************************************/

/*********************************************************************/
/** Copy the given equation data.
 *
 *	@param[in]	*pfld		structure to be copied
 * 	@return Pointer to an exact copy of a structure for evaluations.
 * 			You will need to free this memory when you are finished
 * 			with it.
 *********************************************************************/
FpaEQTN_DATA		*copy_eqtn_data

	(
	FpaEQTN_DATA	*pfld
	)

	{
	FpaEQTN_DATA	*pcopy;

	/* Return Null pointer if no structure passed */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Initialize structure to be copied into based on type of Object */
	pcopy = init_eqtn_data(pfld->Type);
	if ( IsNull(pcopy) ) return NullPtr(FpaEQTN_DATA *);

	/* Copy data into structure based on type of Object */

	/* ... SCALAR Object */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) copy_scalar(&pcopy->Data.scalr, &pfld->Data.scalr);
		}

	/* ... GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		(void) copy_grid(&pcopy->Data.gridd, &pfld->Data.gridd);
		}

	/* ... SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		(void) copy_spline(&pcopy->Data.splne, &pfld->Data.splne);
		}

	/* ... VLIST Object */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) copy_vlist(&pcopy->Data.vlist, &pfld->Data.vlist);
		}

	/* Return the copy of the structure */
	return pcopy;
	}

/**********************************************************************/

/*********************************************************************/
/** Get a converted copy of a structure for evaluations.
 *
 * NOTE: FpaEQT_Grid and FpaEQT_Spline cannot be converted FpaEQT_Scalar
 *
 * NOTE: FpaEQT_Vlist cannot be converted to any other type
 *
 *	@param[in]	type		type of Object to convert into
 *	@param[in]	*pfld		structure to be copied
 * 	@return Pointer to a converted copy of a structure.
 * 			You will need to free this memory when you are finished
 * 			with it.
 *********************************************************************/
FpaEQTN_DATA		*convert_eqtn_data

	(
	short			type,
	FpaEQTN_DATA	*pfld
	)

	{
	int				nnx, nny, nn;
	float			gridln;
	MAP_PROJ		mproj;
	FpaEQTN_DATA	*pgrid, *pcopy;

	/* Return Null pointer if no structure passed */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Set the default map projection */
	mproj = FpaEqtnDefs.mprojEval;

	/* Set the default grid dimensions */
	nnx = FpaEqtnDefs.mprojEval.grid.nx;
	nny = FpaEqtnDefs.mprojEval.grid.ny;
	gridln = FpaEqtnDefs.mprojEval.grid.gridlen;

	/* Error message if unacceptable default grid dimensions */
	if ( nnx <= 0 || nny <= 0 || gridln <= 0 )
		{
		(void) fprintf(stderr, "[convert_eqtn_data] Unacceptable default");
		(void) fprintf(stderr, " grid dimensions\n");
		(void) fprintf(stderr, "  nx: %d", FpaEqtnDefs.mprojEval.grid.nx);
		(void) fprintf(stderr, "  ny: %d", FpaEqtnDefs.mprojEval.grid.ny);
		(void) fprintf(stderr, "  gridlen: %f", FpaEqtnDefs.mprojEval.grid.gridlen);
		(void) fprintf(stderr, "  units: %f\n", FpaEqtnDefs.mprojEval.grid.units);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Copy data into structure based on type of structure to copy   */
	/*  and type of structure to convert structure into              */
	/* Note that all SCALAR Objects are converted to GRID Objects or */
	/*  SPLINE Objects at the default grid dimensions, and that all  */
	/*  SPLINE Objects are converted to GRID Objects at the default  */
	/*  grid dimensions                                              */

	if ( type == FpaEQT_Scalar && pfld->Type == FpaEQT_Scalar )
		{
		pcopy = copy_eqtn_data(pfld);
		}

	else if ( type == FpaEQT_Scalar && pfld->Type == FpaEQT_Grid )
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	else if ( type == FpaEQT_Scalar && pfld->Type == FpaEQT_Spline )
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	else if ( type == FpaEQT_Grid && pfld->Type == FpaEQT_Scalar )
		{
		pcopy = init_eqtn_data(FpaEQT_Grid);
		(void) scalar_grid(&pcopy->Data.gridd, gridln, nnx, nny,
				&pfld->Data.scalr);
		}

	else if ( type == FpaEQT_Grid && pfld->Type == FpaEQT_Grid )
		{
		pcopy = copy_eqtn_data(pfld);
		}

	else if ( type == FpaEQT_Grid && pfld->Type == FpaEQT_Spline )
		{
		pcopy = init_eqtn_data(FpaEQT_Grid);
		(void) spline_grid(&pcopy->Data.gridd, gridln, nnx, nny,
				&pfld->Data.splne);
		}

	else if ( type == FpaEQT_Spline && pfld->Type == FpaEQT_Scalar )
		{
		pgrid = init_eqtn_data(FpaEQT_Grid);
		(void) scalar_grid(&pgrid->Data.gridd, gridln, nnx, nny,
				&pfld->Data.scalr);
		pcopy = init_eqtn_data(FpaEQT_Spline);
		(void) grid_spline(&pcopy->Data.splne, gridln, nnx, nny,
				pgrid->Data.gridd.gval);
		(void) copy_map_projection(&pcopy->Data.splne.mp, &mproj);
		/* Free space used by work structure */
		(void) free_eqtn_data(pgrid);
		}

	else if ( type == FpaEQT_Spline && pfld->Type == FpaEQT_Grid )
		{
		pcopy = init_eqtn_data(FpaEQT_Spline);
		(void) grid_spline(&pcopy->Data.splne, pfld->Data.gridd.gridlen,
				pfld->Data.gridd.nx, pfld->Data.gridd.ny,
				pfld->Data.gridd.gval);
		(void) copy_map_projection(&pcopy->Data.splne.mp, &mproj);
		}

	else if ( type == FpaEQT_Spline && pfld->Type == FpaEQT_Spline )
		{
		pcopy = copy_eqtn_data(pfld);
		}

	else if ( type == FpaEQT_Vlist && pfld->Type == FpaEQT_Scalar )
		{
		pcopy = init_eqtn_data(FpaEQT_Vlist);
		/* Add scalar values to each point in vlist */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			(void) add_point_to_vlist(&pcopy->Data.vlist, FpaEqtnDefs.posEval[nn],
					pfld->Data.scalr.sval);
		}

	else if ( type == FpaEQT_Vlist && pfld->Type == FpaEQT_Vlist )
		{
		pcopy = copy_eqtn_data(pfld);
		}

	else if ( type == FpaEQT_Vlist || pfld->Type == FpaEQT_Vlist )
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[convert_eqtn_data] Unknown Object types\n");
		(void) fprintf(stderr, "  type: %d\n", type);
		(void) fprintf(stderr, "   pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return the converted copy of the structure */
	return pcopy;
	}

/**********************************************************************/

/*********************************************************************/
/** Display the contents of a structure for evaluations.
 *
 *	@param[in]	*pfld		pointer to structure
 *********************************************************************/
void				debug_eqtn_data

	(
	FpaEQTN_DATA	*pfld
	)

	{

	/* Do nothing if no structure passed */
	if ( IsNull(pfld) ) return;

	/* Display the contents based on the type of Object */

	/* ... SCALAR Object */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) debug_scalar(&pfld->Data.scalr);
		}

	/* ... GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		(void) debug_grid(&pfld->Data.gridd);
		}

	/* ... SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		(void) debug_spline(&pfld->Data.splne);
		}

	/* ... VLIST Object */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) debug_vlist(&pfld->Data.vlist);
		}

	/* ... unknown Object types */
	else
		{
		(void) fprintf(stderr, "\n");
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Equation Database)                      *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** i n i t _ e q u a t i o n _ d a t a b a s e _ f i e l d        ***
 ***                                                                ***
 *** initialize storage structure for global Equation Database      ***
 ***                                                                ***
 **********************************************************************/

static	void		init_equation_database_field

	(
	FpaEQTN_FLDS	*edata		/* pointer to storage structure in */
								/*  global Equation Database       */
	)

	{

	/* Return if no storage structure */
	if ( IsNull(edata) ) return;

	/* Reinitialize field descriptor in storage structure */
	(void) init_fld_descript(&edata->descript);

	/* Reinitialize map projection in storage structure */
	(void) copy_map_projection(&edata->mproj, &NoMapProj);

	/* Free space used by FIELD Object in storage structure */
	edata->fld = destroy_field(edata->fld);
	}

/**********************************************************************
 ***                                                                ***
 *** f i n d _ f i e l d _ i n _ e q u a t i o n _ d a t a b a s e  ***
 ***                                                                ***
 *** returns location of a field in the global Equation Database    ***
 ***                                                                ***
 *** g e t _ f i e l d _ f o r _ e q u a t i o n _ d a t a b a s e  ***
 ***                                                                ***
 *** gets a field from a metafile, adds the field to the global     ***
 *** Equation Database, and returns the location of the field in    ***
 *** the global Equation Database                                   ***
 ***                                                                ***
 *** a d d _ f i e l d _ t o _ e q u a t i o n _ d a t a b a s e    ***
 ***                                                                ***
 *** adds a field to the global Equation Database, and returns the  ***
 *** location of the field in the global Equation Database          ***
 ***                                                                ***
 **********************************************************************/

static int		find_field_in_equation_database

	(
	FLD_DESCRIPT	*fdesc	/* pointer to field descriptor */
	)

	{
	int		inum, istart;

	/* Return if no field descriptor */
	if ( IsNull(fdesc) ) return -1;

	/* Check fields in Equation Database */
	/* Begin with last one used */
	/*  ... then check to the end of the list */
	/*  ... the start at the beginning if still not found */
	/* First time - start at the beginning */
	istart = ( LastEqtnFld < 0 )? 0: LastEqtnFld;

	/* Start from last one */
	for ( inum=istart; inum<NumEqtnFlds; inum++ )
		{
		if ( same_fld_descript_no_map(&FpaEqtnFlds[inum].descript, fdesc) )
			{
			LastEqtnFld = inum;
			return inum;
			}
		}

	/* Start from the beginning */
	for ( inum=0; inum<istart; inum++ )
		{
		if ( same_fld_descript_no_map(&FpaEqtnFlds[inum].descript, fdesc) )
			{
			LastEqtnFld = inum;
			return inum;
			}
		}

	/* Not found */
	LastEqtnFld = -1;
	return -1;
	}

/**********************************************************************/

static int		get_field_for_equation_database

	(
	FLD_DESCRIPT	*fdesc	/* pointer to field descriptor */
	)

	{
	int				inum;
	STRING			metapath;
	MAP_PROJ		smproj;
	METAFILE		meta;
	FIELD			fld;

	/* Return if no information in field descriptor */
	if ( IsNull(fdesc) ) return -1;
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return -1;

	/* Check for metafile identified by field descriptor */
	metapath = find_meta_filename(fdesc);
	if ( blank(metapath) ) return -1;

	/* Read the named metafile and extract the required field */
	/* Note that the metafile map projection will have a map  */
	/*  definition but no grid definition!                    */
	dprintf(stdout, "  Reading metafile: \"%s\"\n", SafeStr(metapath));
	meta = read_metafile(metapath, NullMapProj);

	/* Error message if problem reading named metafile */
	if ( IsNull(meta) )
		{
		(void) fprintf(stderr, "[get_field_for_equation_database]");
		(void) fprintf(stderr, " Problem reading metafile: \"%s\"\n",
				metapath);
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		return -1;
		}

	/* Set the metafile map projection */
	(void) copy_map_projection(&smproj, &meta->mproj);

	/* Check for given element/level in METAFILE Object */
	/*  ... extract the FIELD Object (if found)         */
	switch ( fdesc->fmacro )
		{

		/* Continuous and vector fields */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
			fld = take_mf_field(meta, "surface", NullString, NullString,
					fdesc->edef->name, fdesc->ldef->name);
			break;

		/* Discrete and wind fields */
		case FpaC_DISCRETE:
		case FpaC_WIND:
			fld = take_mf_field(meta, "set", "area", NullString,
					fdesc->edef->name, fdesc->ldef->name);
			break;

		/* Line fields */
		case FpaC_LINE:
			fld = take_mf_field(meta, "set", "curve", NullString,
					fdesc->edef->name, fdesc->ldef->name);
			break;

		/* Scattered fields */
		case FpaC_SCATTERED:
			if (PreferPlot)
				{
				fld = take_mf_field(meta, "plot", NullString, NullString,
						fdesc->edef->name, fdesc->ldef->name);
				PreferPlot = FALSE;
				}
			else
				{
				fld = take_mf_field(meta, "set", "spot", NullString,
						fdesc->edef->name, fdesc->ldef->name);
				}
			break;

		/* Link chain fields */
		case FpaC_LCHAIN:
			fld = take_mf_field(meta, "set", "lchain", NullString,
					fdesc->edef->name, fdesc->ldef->name);
			break;

		/* All other fields */
		default:
			fld = take_mf_field(meta, NullString, NullString, NullString,
					fdesc->edef->name, fdesc->ldef->name);
			break;
		}

	/* Return if no field found in METAFILE Object */
	meta = destroy_metafile(meta);
	if ( IsNull(fld) ) return -1;

	/* Save the field in the Equation Database */
	inum = add_field_to_equation_database(fdesc, &smproj, fld);
	if ( inum < 0 )
		{
		/* This should only fail if arguments above are NULL */
		(void) destroy_field(fld);
		return -1;
		}

	/* Return the location of the field in the Equation Database */
	return inum;
	}

/**********************************************************************/

static int		add_field_to_equation_database

	(
	FLD_DESCRIPT	*fdesc,
	MAP_PROJ		*mproj,
	FIELD			fld
	)

	{
	LastEqtnFld = -1;

	/* Return if no field descriptor or map projection or field */
	if ( IsNull(fdesc) ) return -1;
	if ( IsNull(mproj) ) return -1;
	if ( IsNull(fld) )   return -1;

	/* Save the field in the Equation Database */
	LastEqtnFld = NumEqtnFlds++;
	FpaEqtnFlds = GETMEM(FpaEqtnFlds, FpaEQTN_FLDS, NumEqtnFlds);

	(void) copy_fld_descript(&FpaEqtnFlds[LastEqtnFld].descript, fdesc);
	(void) copy_map_projection(&FpaEqtnFlds[LastEqtnFld].mproj, mproj);
	FpaEqtnFlds[LastEqtnFld].fld = fld;

	/* Add a grid definition to the map projection */
	/*  if the field is a surface                  */
	if ( fld->ftype == FtypeSfc )
		{
		(void) set_grid_from_surface(fld->data.sfc,
				&FpaEqtnFlds[LastEqtnFld].mproj);
		}

	/* Return the location of the field in the Equation Database */
	return LastEqtnFld;
	}

/**********************************************************************
 ***                                                                ***
 *** v e c t o r _ f i e l d _ f r o m _ e q u a t i o n _          ***
 ***                                             d a t a b a s e    ***
 ***                                                                ***
 *** x y c o m p _ f i e l d _ f r o m _ e q u a t i o n _          ***
 ***                                             d a t a b a s e    ***
 ***                                                                ***
 *** d e f a u l t _ f i e l d _ f r o m _ e q u a t i o n _        ***
 ***                                             d a t a b a s e    ***
 ***                                                                ***
 *** return copy of a FIELD Object from global Equation Database    ***
 ***                                                                ***
 **********************************************************************/

static FIELD	vector_field_from_equation_database
	(
	FLD_DESCRIPT	*fdesc		/* pointer to field descriptor */
	)

	{
	int				inum, icmp;
	COMPONENT		tcomp;
	MAP_PROJ		tmproj;
	FLD_DESCRIPT	descript;
	FIELD			fld = NullFld;

	STRING			uname, vname;
	MAP_PROJ		umproj, vmproj;
	FIELD			ufld = NullFld;
	FIELD			vfld = NullFld;

	FpaConfigElementComponentStruct	*components;

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) ) return NullFld;
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	/* Check for fields in Equation Database */
	inum = find_field_in_equation_database(fdesc);

	/* Field not found in Equation Database  */
	/*  ... so try to get it from a metafile */
	if ( inum < 0 )
		{
		inum = get_field_for_equation_database(fdesc);
		}

	/* Return the saved map projection and a copy of the field (if found) */
	if ( inum >= 0 )
		{
		(void) copy_map_projection(&FpaEqtnDefs.mprojRead,
				&FpaEqtnFlds[inum].mproj);
		return copy_field(FpaEqtnFlds[inum].fld);
		}

	/* Set the target map projection */
	(void) copy_map_projection(&tmproj, &fdesc->mproj);

	/* Search for components of vector field if no metafile found */
	components = fdesc->edef->elem_detail->components;
	if ( IsNull(components) || components->ncomp <= 0 ) return NullFld;

	/* Check for each component in Equation Database or in metafiles */
	for ( icmp=0; icmp<components->ncomp; icmp++ )
		{

		/* Reset field descriptor for this component */
		(void) copy_fld_descript(&descript, fdesc);
		(void) set_fld_descript(&descript,
							FpaF_ELEMENT, components->comp_edefs[icmp],
							FpaF_END_OF_LIST);

		/* Check for each component in the Equation Database */
		inum = find_field_in_equation_database(&descript);

		/* Field not found in Equation Database  */
		/*  ... so try to get it from a metafile */
		if ( inum < 0 )
			{
			inum = get_field_for_equation_database(&descript);
			}

		/* Set the components if field was found */
		if ( inum >= 0 )
			{

			/* Determine which component was found */
			tcomp = components->comp_types[icmp];

			/* X component found */
			if ( tcomp == X_Comp )
				{
				uname = descript.edef->name;
				ufld  = copy_field(FpaEqtnFlds[inum].fld);
				(void) copy_map_projection(&umproj, &FpaEqtnFlds[inum].mproj);
				}

			/* Y component found */
			else if ( tcomp == Y_Comp )
				{
				vname = descript.edef->name;
				vfld  = copy_field(FpaEqtnFlds[inum].fld);
				(void) copy_map_projection(&vmproj, &FpaEqtnFlds[inum].mproj);
				}

			/* Other components not recognized */
			else
				{
				(void) fprintf(stderr, "[vector_field_from_equation_database]");
				(void) fprintf(stderr, " Problem with type of components\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(descript.sdef->name),
						SafeStr(descript.subdef->name));
				(void) fprintf(stderr, "  at runtime: \"%s\"",
						SafeStr(descript.rtime));
				(void) fprintf(stderr, "  validtime: \"%s\"\n",
						SafeStr(descript.vtime));
				(void) fprintf(stderr, "   for element: \"%s\"",
						SafeStr(descript.edef->name));
				(void) fprintf(stderr, "  level: \"%s\"\n",
						SafeStr(descript.ldef->name));
				}
			}
		}

	/* Return if either component not found */
	if ( IsNull(ufld) || IsNull(vfld) )
		{
		(void) destroy_field(ufld);
		(void) destroy_field(vfld);
		return NullFld;
		}

	/* Check if metafile map projections do not match */
	if ( !same_map_projection(&umproj, &vmproj) )
		{
		(void) fprintf(stderr, "[vector_field_from_equation_database]");
		(void) fprintf(stderr, " Mismatched component map projections\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for elements: \"%s  %s\"",
				SafeStr(uname), SafeStr(vname));
		(void) fprintf(stderr, "  level: \"%s\"\n",
				SafeStr(fdesc->ldef->name));
		(void) destroy_field(ufld);
		(void) destroy_field(vfld);
		return NullFld;
		}

	/* Build a field from the two components */
	fld = build_field_2D(entity_from_field_type(fdesc->fmacro),
							fdesc->edef->name, fdesc->ldef->name,
							ufld, vfld, &umproj, &tmproj);

	/* Return the target map projection and the component field */
	(void) copy_map_projection(&FpaEqtnDefs.mprojRead, &tmproj);
	(void) destroy_field(ufld);
	(void) destroy_field(vfld);
	return fld;
	}

/**********************************************************************/

static FIELD	xycomp_field_from_equation_database

	(
	FLD_DESCRIPT	*fdesc		/* pointer to field descriptor */
	)

	{
	int				inum, jnum;
	MAP_PROJ		tmproj;
	LOGICAL			reproj;
	COMPONENT		tcomp, scomp;
	STRING			sname;
	FLD_DESCRIPT	descript;

	FIELD			ufld = NullFld;
	FIELD			vfld = NullFld;

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) ) return NullFld;
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	/* Check for fields in Equation Database */
	inum = find_field_in_equation_database(fdesc);

	/* Field not found in Equation Database  */
	/*  ... so try to get it from a metafile */
	if ( inum < 0 )
		{
		inum = get_field_for_equation_database(fdesc);
		}

	/* Return NullFld if no field can be found */
	if ( inum < 0 ) return NullFld;

	/* Set the target map projection */
	(void) copy_map_projection(&tmproj, &fdesc->mproj);

	/* Check if field is a component field that needs reprojection */
	reproj = check_reprojection_for_components(fdesc->edef->name,
										&FpaEqtnFlds[inum].mproj, &tmproj);

	/* Return the saved map projection and a copy of the field */
	/*  if no component reprojection required                  */
	if ( !reproj )
		{
		(void) copy_map_projection(&FpaEqtnDefs.mprojRead,
					&FpaEqtnFlds[inum].mproj);
		return copy_field(FpaEqtnFlds[inum].fld);
		}

	/* Determine which component has been found, make a copy of the */
	/*  field, and set a field descriptor for the other component   */
	tcomp = which_components(fdesc->edef->name, &sname, &scomp);

	/* X component found */
	if ( tcomp == X_Comp )
		{

		/* Copy X component field */
		ufld = copy_field(FpaEqtnFlds[inum].fld);

		/* Reset field descriptor to find Y component field */
		(void) copy_fld_descript(&descript, fdesc);
		(void) set_fld_descript(&descript, FpaF_ELEMENT_NAME, sname,
											FpaF_END_OF_LIST);
		}

	/* Y component found */
	else if ( tcomp == Y_Comp )
		{

		/* Copy Y component field */
		vfld = copy_field(FpaEqtnFlds[inum].fld);

		/* Reset field descriptor to find X component field */
		(void) copy_fld_descript(&descript, fdesc);
		(void) set_fld_descript(&descript, FpaF_ELEMENT_NAME, sname,
											FpaF_END_OF_LIST);
		}

	/* Other components not recognized */
	else
		{
		(void) fprintf(stderr, "[xycomp_field_from_equation_database]");
		(void) fprintf(stderr, " Problem with type of components\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		return NullFld;
		}

	/* Find second component field in Equation Database */
	jnum = find_field_in_equation_database(&descript);

	/* Field not found in Equation Database  */
	/*  ... so try to get it from a metafile */
	if ( jnum < 0 )
		{
		jnum = get_field_for_equation_database(&descript);
		}

	/* Return NullFld if second component cannot be found */
	if ( jnum < 0 )
		{
		(void) fprintf(stderr, "[xycomp_field_from_equation_database]");
		(void) fprintf(stderr, " Missing second component in database\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		(void) destroy_field(ufld);
		(void) destroy_field(vfld);
		return NullFld;
		}

	/* Error if database map projections do not match */
	if ( !same_map_projection(&FpaEqtnFlds[inum].mproj,
												&FpaEqtnFlds[jnum].mproj) )
		{
		(void) fprintf(stderr, "[xycomp_field_from_equation_database]");
		(void) fprintf(stderr, " Mismatched component map projections\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for elements: \"%s  %s\"",
				SafeStr(fdesc->edef->name), SafeStr(descript.edef->name));
		(void) fprintf(stderr, "  level: \"%s\"\n",
				SafeStr(fdesc->ldef->name));
		(void) destroy_field(ufld);
		(void) destroy_field(vfld);
		return NullFld;
		}

	/* Copy second component field */
	if ( tcomp == X_Comp ) vfld = copy_field(FpaEqtnFlds[jnum].fld);
	else                   ufld = copy_field(FpaEqtnFlds[jnum].fld);

	/* Now reproject the component fields */
	if ( !reproject_xy_fields(ufld, vfld, &FpaEqtnFlds[inum].mproj, &tmproj) )
		{
		(void) fprintf(stderr, "[xycomp_field_from_equation_database]");
		(void) fprintf(stderr, " Error reprojecting components\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		(void) destroy_field(ufld);
		(void) destroy_field(vfld);
		return NullFld;
		}

	/* Return the target map projection and reprojected component field */
	(void) copy_map_projection(&FpaEqtnDefs.mprojRead, &tmproj);
	if ( tcomp == X_Comp )
		{
		(void) destroy_field(vfld);
		return ufld;
		}
	else
		{
		(void) destroy_field(ufld);
		return vfld;
		}
	}

/**********************************************************************/

static FIELD	default_field_from_equation_database

	(
	FLD_DESCRIPT	*fdesc		/* pointer to field descriptor */
	)

	{
	int				inum;

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) ) return NullFld;
	if ( IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	/* Check for fields in Equation Database */
	inum = find_field_in_equation_database(fdesc);

	/* Field not found in Equation Database  */
	/*  ... so try to get it from a metafile */
	if ( inum < 0 )
		{
		inum = get_field_for_equation_database(fdesc);
		}

	/* Return NullFld if no field can be found */
	if ( inum < 0 ) return NullFld;

	/* Return the saved map projection and a copy of the field */
	(void) copy_map_projection(&FpaEqtnDefs.mprojRead,
				&FpaEqtnFlds[inum].mproj);
	return copy_field(FpaEqtnFlds[inum].fld);
	}
