/*********************************************************************/
/** @file equationOper.c
 *
 * Routines for non-UNIX functions and mathematical functions
 * used in equations.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    e q u a t i o n O p e r . c                                       *
*                                                                      *
*    Routines for non-UNIX functions and mathematical functions        *
*    used in equations                                                 *
*                                                                      *
*    Version 4 (c) Copyright 1996 Environment Canada (AES)             *
*    Version 5 (c) Copyright 1998 Environment Canada (AES)             *
*    Version 7 (c) Copyright 2006 Environment Canada                   *
*    Version 8 (c) Copyright 2011 Environment Canada                   *
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

#define EQUATION_OPER		/* To initialize defined constants and */
							/*  functions in equation.h file       */

#include "equation.h"

#include <environ/environ.h>
#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_macros.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include <fpa_math.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG_EQUATION		/* Turn on/off internal debug printing */
	static	LOGICAL	DebugMode = TRUE;
#else
	static	LOGICAL	DebugMode = FALSE;
#endif /* DEBUG_EQUATION */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                  */
/*  ... these are defined in equation.h */

/* Internal static functions (Non-UNIX Functions) */
static FpaEQTN_DATA	*oper_max(FpaEQTN_DATA *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_min(FpaEQTN_DATA *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_between(FpaEQTN_DATA *, FpaEQTN_DATA *,
														FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_outside(FpaEQTN_DATA *, FpaEQTN_DATA *,
														FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_lat();
static FpaEQTN_DATA	*oper_lon();
static FpaEQTN_DATA	*oper_ddx(FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_ddy(FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_curv(FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_ddt(int, int *, FpaEQTN_DATA **);
static FpaEQTN_DATA	*oper_laplc(FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_advct(FpaEQTN_DATA *, FpaEQTN_DATA *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_divrg(FpaEQTN_DATA *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_svprs(FpaEQTN_DATA *);
static FpaEQTN_DATA	*oper_lvlprs();
static FpaEQTN_DATA	*oper_uprprs();
static FpaEQTN_DATA	*oper_lwrprs();
static FpaEQTN_DATA	*oper_sunang();
static FpaEQTN_DATA	*oper_sundist();

/***********************************************************************
*                                                                      *
*     Routines to identify UNIX or user defined functions              *
*                                                                      *
***********************************************************************/

/* Initialize structure containing all defined functions     */
/* First define standard UNIX functions (some have fpalib    */
/* replacements to handle special cases)                     */
/*   sin[]  cos[]  tan[]  asin[]  acos[]  atan[]  atan2[]    */
/*   sinh[]  cosh[]  tanh[]  exp[]  log[]  log10[]  sqrt[]   */
/*   ceil[]  floor[]  fabs[]  fmod[]  copysign[]  hypot[]    */
/*   (Note that  abs[]  may be used in place of  fabs[])     */
/*   (Note that  mod[]  may be used in place of  fmod[])     */
/*   (Note that  pow[]  is defined as a mathematical symbol) */
/* Next define non-UNIX functions                            */
/*   max[]  min[]  between[]  outside[]  lat[]  lon[]        */
/*   ddx[]  ddy[]  curv[]  ddt[]  laplc[]  advct[]  divrg[]  */
/*   svprs[]  lvlprs[]  uprprs[]  lwrprs[]                   */
/*   sunang[]  sundist[]                                     */
/* Then define mathematical symbols                          */
/*   pow[]  plus[]  minus[]  mult[]  divn[]                  */

#ifdef MACHINE_SUN_NOT
	extern	double	copysign(double, double);
	extern	double	hypot(double, double);
#endif

static const FpaEQTN_FUNC Dfnd_Funcs[] =
	{
		{"sin",        1,   { TRUE },
				FpaEQF_UNIX,             fpa_sin,     (FpaENAM) 0},
		{"cos",        1,   { TRUE },
				FpaEQF_UNIX,             fpa_cos,     (FpaENAM) 0},
		{"tan",        1,   { TRUE },
				FpaEQF_UNIX,             fpa_tan,     (FpaENAM) 0},
		{"asin",       1,   { TRUE },
				FpaEQF_UNIX,             fpa_asin,    (FpaENAM) 0},
		{"acos",       1,   { TRUE },
				FpaEQF_UNIX,             fpa_acos,    (FpaENAM) 0},
		{"atan",       1,   { TRUE },
				FpaEQF_UNIX,             fpa_atan,    (FpaENAM) 0},
		{"atan2",      2,   { TRUE, TRUE },
				FpaEQF_UNIX,             fpa_atan2,   (FpaENAM) 0},
		{"sinh",       1,   { TRUE },
				FpaEQF_UNIX,             sinh,        (FpaENAM) 0},
		{"cosh",       1,   { TRUE },
				FpaEQF_UNIX,             cosh,        (FpaENAM) 0},
		{"tanh",       1,   { TRUE },
				FpaEQF_UNIX,             tanh,        (FpaENAM) 0},
		{"exp",        1,   { TRUE },
				FpaEQF_UNIX,             exp,         (FpaENAM) 0},
		{"log",        1,   { TRUE },
				FpaEQF_UNIX,             log,         (FpaENAM) 0},
		{"log10",      1,   { TRUE },
				FpaEQF_UNIX,             log10,       (FpaENAM) 0},
		{"sqrt",       1,   { TRUE },
				FpaEQF_UNIX,             fpa_sqrt,    (FpaENAM) 0},
		{"ceil",       1,   { TRUE },
				FpaEQF_UNIX,             ceil,        (FpaENAM) 0},
		{"floor",      1,   { TRUE },
				FpaEQF_UNIX,             floor,       (FpaENAM) 0},
		{"fabs",       1,   { TRUE },
				FpaEQF_UNIX,             fabs,        (FpaENAM) 0},
		{"abs",        1,   { TRUE },
				FpaEQF_UNIX,             fabs,        (FpaENAM) 0},
		{"fmod",       2,   { TRUE, TRUE },
				FpaEQF_UNIX,             fmod,        (FpaENAM) 0},
		{"mod",        2,   { TRUE, TRUE },
				FpaEQF_UNIX,             fmod,        (FpaENAM) 0},
		{"copysign",   2,   { TRUE, TRUE },
				FpaEQF_UNIX,             copysign,    (FpaENAM) 0},
		{"hypot",      2,   { TRUE, TRUE },
				FpaEQF_UNIX,             hypot,       (FpaENAM) 0},
		{"pow",        2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_power},
		{"plus",       2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_plus},
		{"minus",      2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_minus},
		{"mult",       2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_mult},
		{"divn",       2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_divn},
		{"max",        2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_max},
		{"min",        2,   { TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_min},
		{"between",    3,   { TRUE, TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_between},
		{"outside",    3,   { TRUE, TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_outside},
		{"lat",        0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_lat},
		{"lon",        0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_lon},
		{"ddx",        1,   { FALSE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_ddx},
		{"ddy",        1,   { FALSE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_ddy},
		{"curv",       1,   { FALSE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_curv},
		{"ddt",        1,   { TRUE },
				FpaEQF_TIMESERIES,      (FpaUNAM) 0,   oper_ddt},
		{"laplc",      1,   { FALSE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_laplc},
		{"advct",      3,   { FALSE, TRUE, TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_advct},
		{"divrg",      2,   { FALSE, FALSE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_divrg},
		{"svprs",      1,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_svprs},
		{"lvlprs",     0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_lvlprs},
		{"uprprs",     0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_uprprs},
		{"lwrprs",     0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_lwrprs},
		{"sunang",     0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_sunang},
		{"sundist",    0,   { TRUE },
				FpaEQF_ORDINARY,        (FpaUNAM) 0,   oper_sundist},
	};

/* Determine number of defined functions */
static const int Num_Dfnd_Funcs =
	(int) (sizeof(Dfnd_Funcs) / sizeof(FpaEQTN_FUNC));

/**********************************************************************
 ***                                                                ***
 *** i d e n t i f y _ f u n c t i o n                              ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Identify function by name.
 *
 *	@param[in]	buf			string containing function name
 * 	@return Pointer to parameters identified from a function name.
 *********************************************************************/
const FpaEQTN_FUNC	*identify_function

	(
	STRING			buf
	)

	{
	int				jnum;

	/* Return pointer to defined function ... ignoring case in comparison */
	for ( jnum=0; jnum<Num_Dfnd_Funcs; jnum++ )
		if ( same_ic(buf, Dfnd_Funcs[jnum].Name) ) return &Dfnd_Funcs[jnum];

	/* >>> Here is where call to user-defined functions will go <<< */

	/* Error return if function name not found */
	return NullPtr(FpaEQTN_FUNC *);
	}

/***********************************************************************
*                                                                      *
*     Routines to evaluate Mathematical Symbols                        *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** o p e r _ p o w e r                                            ***
 ***                                                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Calculate the first structure taken to the power of
 * the second structure.
 * @f$ {C} = {A} ^ {B} @f$
 *
 *	@param[in]	*plfld		first structure (A)
 *	@param[in]	*prfld		second structure (B)
 * 	@return Pointer to structure calculated from first structure
 * 			taken to the power of second structure (C). You will
 * 			need to free this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*oper_power

	(
	FpaEQTN_DATA	*plfld,
	FpaEQTN_DATA	*prfld
	)

	{
	int				iix, iiy, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform operation based on type of Object in each structure,  */
	/*  by copying one structure and overwriting the data.           */
	/* Note that all SPLINE Objects are converted to GRID Objects at */
	/*  the default grid dimensions                                  */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				pow((double) plfld->Data.scalr.sval,
					(double) prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						pow((double) plfld->Data.scalr.sval,
							(double) prfld->Data.gridd.gval[iiy][iix]);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_power(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						pow((double) plfld->Data.gridd.gval[iiy][iix],
							(double) prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_power] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_power(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							pow((double) plfld->Data.gridd.gval[iiy][iix],
								(double) prfld->Data.gridd.gval[iiy][iix]);
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_power(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_power(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_power(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_power(pltemp, prtemp);
		(void) free_eqtn_data(pltemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_power] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}

		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					pow((double) plfld->Data.vlist.val[nn],
						(double) prfld->Data.vlist.val[nn]);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					pow((double) plfld->Data.vlist.val[nn],
						(double) prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					pow((double) plfld->Data.scalr.sval,
						(double) prfld->Data.vlist.val[nn]);
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_power] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_power] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ p l u s                                              ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Add the second structure to the first structure.
 * @f$ {C} = {A} + {B} @f$
 *
 *	@param[in]	*plfld		first structure (A)
 *	@param[in]	*prfld		second structure (B)
 * 	@return Pointer to structure calculated from second structure
 * 			added to first structure (C). You will need to free
 * 			this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*oper_plus

	(
	FpaEQTN_DATA	*plfld,
	FpaEQTN_DATA	*prfld
	)

	{
	int				iix, iiy, iiu, iiv, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform addition based on type of Object in each structure, */
	/*  by copying one structure and overwriting the data.         */
	/* Note that all SPLINE Objects added to GRID Objects are      */
	/*  converted to GRID Objects at the default grid dimensions   */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				plfld->Data.scalr.sval + prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.scalr.sval +
						prfld->Data.gridd.gval[iiy][iix];
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.scalr.sval +
						prfld->Data.splne.cvs[iiu][iiv];
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.gridd.gval[iiy][iix] +
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_plus] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_plus(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							plfld->Data.gridd.gval[iiy][iix] +
							prfld->Data.gridd.gval[iiy][iix];
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_plus(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.splne.cvs[iiu][iiv] +
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_plus(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		/* Ensure that SPLINE Objects have same spline dimensions */
		if ( !same_spline_size(&plfld->Data.splne, &prfld->Data.splne) )
			{
			(void) fprintf(stderr, "[oper_plus] Spline dimensions do not agree\n");
			/* Convert both SPLINE Objects to GRID Objects if spline */
			/*  dimensions do not agree                              */
			pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
			pfield = oper_plus(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			/* Add SPLINE Objects if spline dimensions agree */
			pfield = copy_eqtn_data(plfld);
			for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
				for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
					pfield->Data.splne.cvs[iiu][iiv] =
							plfld->Data.splne.cvs[iiu][iiv] +
							prfld->Data.splne.cvs[iiu][iiv];
			}
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_plus] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] + prfld->Data.vlist.val[nn];
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] + prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.scalr.sval + prfld->Data.vlist.val[nn];
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_plus] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_plus] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ m i n u s                                            ***
 ***                                                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Subtract the second structure from the first structure.
 * @f$ {C} = {A} - {B} @f$
 *
 *	@param[in]	*plfld		first structure (A)
 *	@param[in]	*prfld		second structure (B)
 * 	@return pointer to structure calculated from second structure
 * 			subtracted from first structure (C). You will need to
 * 			free this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*oper_minus

	(
	FpaEQTN_DATA	*plfld,
	FpaEQTN_DATA	*prfld
	)

	{
	int				iix, iiy, iiu, iiv, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform subtraction based on type of Object in each structure, */
	/*  by copying one structure and overwriting the data.            */
	/* Note that all SPLINE Objects subtracted with GRID Objects are  */
	/*  converted to GRID Objects at the default grid dimensions      */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				plfld->Data.scalr.sval - prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.scalr.sval -
						prfld->Data.gridd.gval[iiy][iix];
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.scalr.sval -
						prfld->Data.splne.cvs[iiu][iiv];
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.gridd.gval[iiy][iix] -
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_minus] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_minus(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							plfld->Data.gridd.gval[iiy][iix] -
							prfld->Data.gridd.gval[iiy][iix];
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_minus(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.splne.cvs[iiu][iiv] -
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_minus(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		/* Ensure that SPLINE Objects have same spline dimensions */
		if ( !same_spline_size(&plfld->Data.splne, &prfld->Data.splne) )
			{
			(void) fprintf(stderr, "[oper_minus] Spline dimensions do not agree\n");
			/* Convert both SPLINE Objects to GRID Objects if spline */
			/*  dimensions do not agree                              */
			pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
			pfield = oper_minus(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			/* Subtract SPLINE Objects if spline dimensions agree */
			pfield = copy_eqtn_data(plfld);
			for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
				for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
					pfield->Data.splne.cvs[iiu][iiv] =
							plfld->Data.splne.cvs[iiu][iiv] -
							prfld->Data.splne.cvs[iiu][iiv];
			}
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_minus] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] - prfld->Data.vlist.val[nn];
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] - prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.scalr.sval - prfld->Data.vlist.val[nn];
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_minus] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_minus] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ m u l t                                              ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Multiply the first structure by the second structure.
 * @f$ {C} = {A}*{B} @f$
 *
 *	@param[in]	*plfld		first structure (A)
 *	@param[in]	*prfld		second structure (B)
 * 	@return Pointer to structure calculated from first structure
 * 			multiplied by second structure (C). You will need to
 * 			free this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*oper_mult

	(
	FpaEQTN_DATA	*plfld,
	FpaEQTN_DATA	*prfld
	)

	{
	int				iix, iiy, iiu, iiv, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform multiplication based on type of Object in each           */
	/*  structure, by copying one structure and overwriting the data    */
	/* Note that all SPLINE Objects multiplied with GRID/SPLINE Objects */
	/*  are converted to GRID Objects at the default grid dimensions    */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				plfld->Data.scalr.sval * prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.scalr.sval *
						prfld->Data.gridd.gval[iiy][iix];
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.scalr.sval *
						prfld->Data.splne.cvs[iiu][iiv];
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.gridd.gval[iiy][iix] *
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_mult] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_mult(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							plfld->Data.gridd.gval[iiy][iix] *
							prfld->Data.gridd.gval[iiy][iix];
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_mult(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.splne.cvs[iiu][iiv] *
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_mult(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_mult(pltemp, prtemp);
		(void) free_eqtn_data(pltemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_mult] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] * prfld->Data.vlist.val[nn];
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] * prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.scalr.sval * prfld->Data.vlist.val[nn];
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_mult] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_mult] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ d i v n                                              ***
 ***                                                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Divide the first structure by the second structure.
 * @f$ {C} = \frac{A}{B} @f$
 *
 *	@param[in]	*plfld		first structure (A)
 *	@param[in]	*prfld		second structure (B)
 * 	@return Pointer to structure calculated from first structure
 * 		 divided by second structure (C). You will need to free
 * 		 this memory when you are finished with it.
 *********************************************************************/
FpaEQTN_DATA		*oper_divn

	(
	FpaEQTN_DATA	*plfld,
	FpaEQTN_DATA	*prfld
	)

	{
	int				iix, iiy, iiu, iiv, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform division based on type of Object in each structure,   */
	/*  by copying one structure and overwriting the data.           */
	/* Note that all SPLINE Objects divided with GRID/SPLINE Objects */
	/*  are converted to GRID Objects at the default grid dimensions */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		if ( prfld->Data.scalr.sval == 0.0 )
			{
			(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				plfld->Data.scalr.sval / prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				if ( prfld->Data.gridd.gval[iiy][iix] == 0.0 )
					{
					(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
					(void) free_eqtn_data(pfield);
					return NullPtr(FpaEQTN_DATA *);
					}
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.scalr.sval /
						prfld->Data.gridd.gval[iiy][iix];
				}
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_divn(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		if ( prfld->Data.scalr.sval == 0.0 )
			{
			(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						plfld->Data.gridd.gval[iiy][iix] /
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_divn] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_divn(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					{
					if ( prfld->Data.gridd.gval[iiy][iix] == 0.0 )
						{
						(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
						(void) free_eqtn_data(pfield);
						return NullPtr(FpaEQTN_DATA *);
						}
					pfield->Data.gridd.gval[iiy][iix] =
							plfld->Data.gridd.gval[iiy][iix] /
							prfld->Data.gridd.gval[iiy][iix];
				}
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_divn(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		if ( prfld->Data.scalr.sval == 0.0 )
			{
			(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( iiu=0; iiu<pfield->Data.splne.m; iiu++ )
			for ( iiv=0; iiv<pfield->Data.splne.n; iiv++ )
				pfield->Data.splne.cvs[iiu][iiv] =
						plfld->Data.splne.cvs[iiu][iiv] /
						prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_divn(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_divn(pltemp, prtemp);
		(void) free_eqtn_data(pltemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_divn] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			{
			if ( prfld->Data.vlist.val[nn] == 0.0 )
				{
				(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] / prfld->Data.vlist.val[nn];
			}
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		if ( prfld->Data.scalr.sval == 0.0 )
			{
			(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					plfld->Data.vlist.val[nn] / prfld->Data.scalr.sval;
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			{
			if ( prfld->Data.vlist.val[nn] == 0.0 )
				{
				(void) fprintf(stderr, "[oper_divn] Divide by zero\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			pfield->Data.vlist.val[nn] =
					plfld->Data.scalr.sval / prfld->Data.vlist.val[nn];
			}
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_divn] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_divn] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Non-UNIX Functions)                     *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** o p e r _ m a x                                                ***
 ***                                                                ***
 *** return pointer to structure containing maximum value of first  ***
 ***  structure or second structure at each location                ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_max

	(
	FpaEQTN_DATA	*plfld,		/* pointer to first structure */
	FpaEQTN_DATA	*prfld		/* pointer to second structure */
	)

	{
	int				iix, iiy, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform operation based on type of Object in each structure,  */
	/*  by copying one structure and overwriting the data.           */
	/* Note that all SPLINE Objects are converted to GRID Objects at */
	/*  the default grid dimensions                                  */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				MAX(plfld->Data.scalr.sval, prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						MAX(plfld->Data.scalr.sval,
							prfld->Data.gridd.gval[iiy][iix]);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_max(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						MAX(plfld->Data.gridd.gval[iiy][iix],
							prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_max] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_max(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							MAX(plfld->Data.gridd.gval[iiy][iix],
								prfld->Data.gridd.gval[iiy][iix]);
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_max(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_max(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_max(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_max(pltemp, prtemp);
		(void) free_eqtn_data(pltemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_max] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}

		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MAX(plfld->Data.vlist.val[nn], prfld->Data.vlist.val[nn]);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MAX(plfld->Data.vlist.val[nn], prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MAX(plfld->Data.scalr.sval, prfld->Data.vlist.val[nn]);
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_max] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_max] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ m i n                                                ***
 ***                                                                ***
 *** return pointer to structure containing minimum value of first  ***
 ***  structure or second structure at each location                ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_min

	(
	FpaEQTN_DATA	*plfld,		/* pointer to first structure */
	FpaEQTN_DATA	*prfld		/* pointer to second structure */
	)

	{
	int				iix, iiy, nn;
	FpaEQTN_DATA	*pfield, *pltemp, *prtemp, *ptemp;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(plfld) || IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Perform operation based on type of Object in each structure,  */
	/*  by copying one structure and overwriting the data.           */
	/* Note that all SPLINE Objects are converted to GRID Objects at */
	/*  the default grid dimensions                                  */

	if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(prfld);
		pfield->Data.scalr.sval =
				MIN(plfld->Data.scalr.sval, prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(prfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						MIN(plfld->Data.scalr.sval,
							prfld->Data.gridd.gval[iiy][iix]);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_min(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				pfield->Data.gridd.gval[iiy][iix] =
						MIN(plfld->Data.gridd.gval[iiy][iix],
							prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Grid )
		{
		/* Ensure that GRID Objects have same grid dimensions */
		if ( !same_grid_size(&plfld->Data.gridd, &prfld->Data.gridd) )
			{
			(void) fprintf(stderr, "[oper_min] Grid dimensions do not agree\n");
			/* Convert both GRID Objects to default grid dimensions */
			ptemp = convert_eqtn_data(FpaEQT_Spline, plfld);
			pltemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			ptemp = convert_eqtn_data(FpaEQT_Spline, prfld);
			prtemp = convert_eqtn_data(FpaEQT_Grid, ptemp);
			(void) free_eqtn_data(ptemp);
			pfield = oper_min(pltemp, prtemp);
			(void) free_eqtn_data(pltemp);
			(void) free_eqtn_data(prtemp);
			}
		else
			{
			pfield = copy_eqtn_data(plfld);
			for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
				for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
					pfield->Data.gridd.gval[iiy][iix] =
							MIN(plfld->Data.gridd.gval[iiy][iix],
								prfld->Data.gridd.gval[iiy][iix]);
			}
		}

	else if ( plfld->Type == FpaEQT_Grid && prfld->Type == FpaEQT_Spline )
		{
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_min(plfld, prtemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Scalar )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_min(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Grid )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		pfield = oper_min(pltemp, prfld);
		(void) free_eqtn_data(pltemp);
		}

	else if ( plfld->Type == FpaEQT_Spline && prfld->Type == FpaEQT_Spline )
		{
		pltemp = convert_eqtn_data(FpaEQT_Grid, plfld);
		prtemp = convert_eqtn_data(FpaEQT_Grid, prfld);
		pfield = oper_min(pltemp, prtemp);
		(void) free_eqtn_data(pltemp);
		(void) free_eqtn_data(prtemp);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Vlist )
		{
		/* Ensure that VLIST Objects have same points */
		if ( !same_vlist_points(&plfld->Data.vlist, &prfld->Data.vlist) )
			{
			(void) fprintf(stderr, "[oper_min] Points in");
			(void) fprintf(stderr, " VLIST Objects do not match\n");
			return NullPtr(FpaEQTN_DATA *);
			}
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MIN(plfld->Data.vlist.val[nn], prfld->Data.vlist.val[nn]);
		}

	else if ( plfld->Type == FpaEQT_Vlist && prfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(plfld);
		for ( nn=0; nn<plfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MIN(plfld->Data.vlist.val[nn], prfld->Data.scalr.sval);
		}

	else if ( plfld->Type == FpaEQT_Scalar && prfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(prfld);
		for ( nn=0; nn<prfld->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] =
					MIN(plfld->Data.scalr.sval, prfld->Data.vlist.val[nn]);
		}

	/* Error message for VLIST Object mixed with GRID or SPLINE Object */
	else if ( plfld->Type == FpaEQT_Vlist || prfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_min] Error mixing VLIST Object");
		(void) fprintf(stderr, " with GRID or SPLINE Object\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message for unknown Object types */
	else
		{
		(void) fprintf(stderr, "[oper_min] Unknown Object types\n");
		(void) fprintf(stderr, "  plfld: %d\n", plfld->Type);
		(void) fprintf(stderr, "   prfld: %d\n", prfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ b e t w e e n                                        ***
 ***                                                                ***
 *** return pointer to structure that contains values from the      ***
 ***  input structure that are between a minimum and maximum range  ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_between

	(
	FpaEQTN_DATA	*pfld,		/* pointer to input structure */
	FpaEQTN_DATA	*pminfld,	/* pointer to minimum value structure */
	FpaEQTN_DATA	*pmaxfld	/* pointer to maximum value structure */
	)

	{
	int				iix, iiy, nn;
	float			xval, minval, maxval;
	FpaEQTN_DATA	*pfield, *ptemp, *ptmin, *ptmax, *ptex;

	/* Return Null if missing any structure */
	if ( IsNull(pfld) || IsNull(pminfld) || IsNull(pmaxfld) )
			return NullPtr(FpaEQTN_DATA *);

	/* Match minimum/maximum structures to type of Object in input structure */
	/* Note that all SPLINE Objects are converted to GRID Objects at the     */
	/*  default grid dimensions                                              */

	if ( pfld->Type == FpaEQT_Scalar )
		{

		if ( pminfld->Type == FpaEQT_Scalar && pmaxfld->Type == FpaEQT_Scalar )
			{
			}

		/* Error message for unmatched Objects */
		else
			{
			if ( pminfld->Type != FpaEQT_Scalar )
				{
				(void) fprintf(stderr, "[oper_between] Minimum for SCALAR field");
				(void) fprintf(stderr, " must be SCALAR!\n");
				}
			if ( pmaxfld->Type != FpaEQT_Scalar )
				{
				(void) fprintf(stderr, "[oper_between] Maximum for SCALAR field");
				(void) fprintf(stderr, " must be SCALAR!\n");
				}
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	else if ( pfld->Type == FpaEQT_Grid )
		{

		if ( pminfld->Type == FpaEQT_Grid && pmaxfld->Type == FpaEQT_Grid )
			{

			/* Ensure that GRID Objects have same grid dimensions */
			if ( !same_grid_size(&pfld->Data.gridd, &pminfld->Data.gridd)
					|| !same_grid_size(&pfld->Data.gridd, &pmaxfld->Data.gridd) )
				{
				(void) fprintf(stderr, "[oper_between] Grid dimensions do not agree\n");
				/* Convert all GRID Objects to default grid dimensions */
				ptex  = convert_eqtn_data(FpaEQT_Spline, pfld);
				ptemp = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				ptex  = convert_eqtn_data(FpaEQT_Spline, pminfld);
				ptmin = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				ptex  = convert_eqtn_data(FpaEQT_Spline, pmaxfld);
				ptmax = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				pfield = oper_between(ptemp, ptmin, ptmax);
				(void) free_eqtn_data(ptemp);
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return pfield;
				}
			}

		else
			{

			ptmin = convert_eqtn_data(FpaEQT_Grid, pminfld);
			ptmax = convert_eqtn_data(FpaEQT_Grid, pmaxfld);

			/* Error message for unmatched Objects */
			if ( IsNull(ptmin) || IsNull(ptmax) )
				{
				if ( IsNull(ptmin) )
					{
					(void) fprintf(stderr, "[oper_between] Minimum for GRID field");
					(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
					}
				if ( IsNull(ptmax) )
					{
					(void) fprintf(stderr, "[oper_between] Maximum for GRID field");
					(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
					}
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return NullPtr(FpaEQTN_DATA *);
				}

			pfield = oper_between(pfld, ptmin, ptmax);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return pfield;
			}
		}

	else if ( pfld->Type == FpaEQT_Spline )
		{

		ptemp = convert_eqtn_data(FpaEQT_Grid, pfld);
		ptmin = convert_eqtn_data(FpaEQT_Grid, pminfld);
		ptmax = convert_eqtn_data(FpaEQT_Grid, pmaxfld);

		/* Error message for unmatched Objects */
		if ( IsNull(ptmin) || IsNull(ptmax) )
			{
			if ( IsNull(ptmin) )
				{
				(void) fprintf(stderr, "[oper_between] Minimum for SPLINE field");
				(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
				}
			if ( IsNull(ptmax) )
				{
				(void) fprintf(stderr, "[oper_between] Maximum for SPLINE field");
				(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
				}
			(void) free_eqtn_data(ptemp);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return NullPtr(FpaEQTN_DATA *);
			}

		pfield = oper_between(ptemp, ptmin, ptmax);
		(void) free_eqtn_data(ptemp);
		(void) free_eqtn_data(ptmin);
		(void) free_eqtn_data(ptmax);
		return pfield;
		}

	else if ( pfld->Type == FpaEQT_Vlist )
		{

		if ( pminfld->Type == FpaEQT_Vlist && pmaxfld->Type == FpaEQT_Vlist )
			{

			/* Error message for unmatched VLIST Objects */
			if ( !same_vlist_points(&pfld->Data.vlist, &pminfld->Data.vlist)
					|| !same_vlist_points(&pfld->Data.vlist, &pmaxfld->Data.vlist) )
				{
				(void) fprintf(stderr, "[oper_between] Points in");
				(void) fprintf(stderr, " VLIST Objects do not match\n");
				return NullPtr(FpaEQTN_DATA *);
				}
			}

		else
			{

			ptmin = convert_eqtn_data(FpaEQT_Vlist, pminfld);
			ptmax = convert_eqtn_data(FpaEQT_Vlist, pmaxfld);

			/* Error message for unmatched Objects */
			if ( IsNull(ptmin) || IsNull(ptmax) )
				{
				if ( IsNull(ptmin) )
					{
					(void) fprintf(stderr, "[oper_between] Minimum for VLIST field");
					(void) fprintf(stderr, " must be SCALAR or VLIST!\n");
					}
				if ( IsNull(ptmax) )
					{
					(void) fprintf(stderr, "[oper_between] Maximum for VLIST field");
					(void) fprintf(stderr, " must be SCALAR or VLIST!\n");
					}
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return NullPtr(FpaEQTN_DATA *);
				}

			pfield = oper_between(pfld, ptmin, ptmax);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return pfield;
			}
		}

	/* Error message for unknown Object type for input structure */
	else
		{
		(void) fprintf(stderr, "[oper_between] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Limit values between minimum and maximum value based on type of     */
	/*  Object in structure, by copying structure and overwriting the data */

	if ( pfld->Type == FpaEQT_Scalar )
		{

		/* Limit value between minimum and maximum value */
		pfield = copy_eqtn_data(pfld);
		xval   = pfld->Data.scalr.sval;
		minval = pminfld->Data.scalr.sval;
		maxval = pmaxfld->Data.scalr.sval;
		xval   = MAX(xval, minval);
		xval   = MIN(xval, maxval);
		pfield->Data.scalr.sval = xval;
		}

	else if ( pfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(pfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{

				xval   = pfld->Data.gridd.gval[iiy][iix];
				minval = pminfld->Data.gridd.gval[iiy][iix];
				maxval = pmaxfld->Data.gridd.gval[iiy][iix];
				xval   = MAX(xval, minval);
				xval   = MIN(xval, maxval);
				pfield->Data.gridd.gval[iiy][iix] = xval;
				}
		}

	else if ( pfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			{
			xval   = pfld->Data.vlist.val[nn];
			minval = pminfld->Data.vlist.val[nn];
			maxval = pmaxfld->Data.vlist.val[nn];
			xval   = MAX(xval, minval);
			xval   = MIN(xval, maxval);
			pfield->Data.vlist.val[nn] = xval;
			}
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ o u t s i d e                                        ***
 ***                                                                ***
 *** return pointer to structure that contains values from the      ***
 ***  input structure that are less than a minimum value or more    ***
 ***  than a maximum value                                          ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_outside

	(
	FpaEQTN_DATA	*pfld,		/* pointer to input structure */
	FpaEQTN_DATA	*pminfld,	/* pointer to minimum value structure */
	FpaEQTN_DATA	*pmaxfld	/* pointer to maximum value structure */
	)

	{
	int				iix, iiy, nn;
	float			xval, minval, maxval, midval;
	FpaEQTN_DATA	*pfield, *ptemp, *ptmin, *ptmax, *ptex;

	/* Return Null if missing any structure */
	if ( IsNull(pfld) || IsNull(pminfld) || IsNull(pmaxfld) )
			return NullPtr(FpaEQTN_DATA *);

	/* Match minimum/maximum structures to type of Object in input structure */
	/* Note that all SPLINE Objects are converted to GRID Objects at the     */
	/*  default grid dimensions                                              */

	if ( pfld->Type == FpaEQT_Scalar )
		{

		if ( pminfld->Type == FpaEQT_Scalar && pmaxfld->Type == FpaEQT_Scalar )
			{
			}

		/* Error message for unmatched Objects */
		else
			{
			if ( pminfld->Type != FpaEQT_Scalar )
				{
				(void) fprintf(stderr, "[oper_outside] Minimum for SCALAR field");
				(void) fprintf(stderr, " must be SCALAR!\n");
				}
			if ( pmaxfld->Type != FpaEQT_Scalar )
				{
				(void) fprintf(stderr, "[oper_outside] Maximum for SCALAR field");
				(void) fprintf(stderr, " must be SCALAR!\n");
				}
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	else if ( pfld->Type == FpaEQT_Grid )
		{

		if ( pminfld->Type == FpaEQT_Grid && pmaxfld->Type == FpaEQT_Grid )
			{

			/* Ensure that GRID Objects have same grid dimensions */
			if ( !same_grid_size(&pfld->Data.gridd, &pminfld->Data.gridd)
					|| !same_grid_size(&pfld->Data.gridd, &pmaxfld->Data.gridd) )
				{
				(void) fprintf(stderr, "[oper_outside] Grid dimensions do not agree\n");
				/* Convert all GRID Objects to default grid dimensions */
				ptex  = convert_eqtn_data(FpaEQT_Spline, pfld);
				ptemp = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				ptex  = convert_eqtn_data(FpaEQT_Spline, pminfld);
				ptmin = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				ptex  = convert_eqtn_data(FpaEQT_Spline, pmaxfld);
				ptmax = convert_eqtn_data(FpaEQT_Grid, ptex);
				(void) free_eqtn_data(ptex);
				pfield = oper_outside(ptemp, ptmin, ptmax);
				(void) free_eqtn_data(ptemp);
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return pfield;
				}
			}

		else
			{

			ptmin = convert_eqtn_data(FpaEQT_Grid, pminfld);
			ptmax = convert_eqtn_data(FpaEQT_Grid, pmaxfld);

			/* Error message for unmatched Objects */
			if ( IsNull(ptmin) || IsNull(ptmax) )
				{
				if ( IsNull(ptmin) )
					{
					(void) fprintf(stderr, "[oper_outside] Minimum for GRID field");
					(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
					}
				if ( IsNull(ptmax) )
					{
					(void) fprintf(stderr, "[oper_outside] Maximum for GRID field");
					(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
					}
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return NullPtr(FpaEQTN_DATA *);
				}

			pfield = oper_outside(pfld, ptmin, ptmax);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return pfield;
			}
		}

	else if ( pfld->Type == FpaEQT_Spline )
		{

		ptemp = convert_eqtn_data(FpaEQT_Grid, pfld);
		ptmin = convert_eqtn_data(FpaEQT_Grid, pminfld);
		ptmax = convert_eqtn_data(FpaEQT_Grid, pmaxfld);

		/* Error message for unmatched Objects */
		if ( IsNull(ptmin) || IsNull(ptmax) )
			{
			if ( IsNull(ptmin) )
				{
				(void) fprintf(stderr, "[oper_outside] Minimum for SPLINE field");
				(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
				}
			if ( IsNull(ptmax) )
				{
				(void) fprintf(stderr, "[oper_outside] Maximum for SPLINE field");
				(void) fprintf(stderr, " must be SCALAR or GRID or SPLINE!\n");
				}
			(void) free_eqtn_data(ptemp);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return NullPtr(FpaEQTN_DATA *);
			}

		pfield = oper_outside(ptemp, ptmin, ptmax);
		(void) free_eqtn_data(ptemp);
		(void) free_eqtn_data(ptmin);
		(void) free_eqtn_data(ptmax);
		return pfield;
		}

	else if ( pfld->Type == FpaEQT_Vlist )
		{

		if ( pminfld->Type == FpaEQT_Vlist && pmaxfld->Type == FpaEQT_Vlist )
			{

			/* Error message for unmatched VLIST Objects */
			if ( !same_vlist_points(&pfld->Data.vlist, &pminfld->Data.vlist)
					|| !same_vlist_points(&pfld->Data.vlist, &pmaxfld->Data.vlist) )
				{
				(void) fprintf(stderr, "[oper_outside] Points in");
				(void) fprintf(stderr, " VLIST Objects do not match\n");
				return NullPtr(FpaEQTN_DATA *);
				}
			}

		else
			{

			ptmin = convert_eqtn_data(FpaEQT_Vlist, pminfld);
			ptmax = convert_eqtn_data(FpaEQT_Vlist, pmaxfld);

			/* Error message for unmatched Objects */
			if ( IsNull(ptmin) || IsNull(ptmax) )
				{
				if ( IsNull(ptmin) )
					{
					(void) fprintf(stderr, "[oper_outside] Minimum for VLIST field");
					(void) fprintf(stderr, " must be SCALAR or VLIST!\n");
					}
				if ( IsNull(ptmax) )
					{
					(void) fprintf(stderr, "[oper_outside] Maximum for VLIST field");
					(void) fprintf(stderr, " must be SCALAR or VLIST!\n");
					}
				(void) free_eqtn_data(ptmin);
				(void) free_eqtn_data(ptmax);
				return NullPtr(FpaEQTN_DATA *);
				}

			pfield = oper_outside(pfld, ptmin, ptmax);
			(void) free_eqtn_data(ptmin);
			(void) free_eqtn_data(ptmax);
			return pfield;
			}
		}

	/* Error message for unknown Object type for input structure */
	else
		{
		(void) fprintf(stderr, "[oper_outside] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Limit values outside minimum and maximum value based on type of     */
	/*  Object in structure, by copying structure and overwriting the data */

	if ( pfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(pfld);
		xval   = pfld->Data.scalr.sval;
		minval = pminfld->Data.scalr.sval;
		maxval = pmaxfld->Data.scalr.sval;
		midval = ( minval + maxval ) / 2.0;
		if ( xval > minval && xval < midval )   xval = minval;
		if ( xval < maxval && xval >=  midval ) xval = maxval;
		pfield->Data.scalr.sval = xval;
		}

	else if ( pfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(pfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{

				xval   = pfld->Data.gridd.gval[iiy][iix];
				minval = pminfld->Data.gridd.gval[iiy][iix];
				maxval = pmaxfld->Data.gridd.gval[iiy][iix];
				midval = ( minval + maxval ) / 2.0;
				if ( xval > minval && xval < midval )   xval = minval;
				if ( xval < maxval && xval >=  midval ) xval = maxval;
				pfield->Data.gridd.gval[iiy][iix] = xval;
				}
		}

	else if ( pfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			{
			xval   = pfld->Data.vlist.val[nn];
			minval = pminfld->Data.vlist.val[nn];
			maxval = pmaxfld->Data.vlist.val[nn];
			midval = ( minval + maxval ) / 2.0;
			if ( xval > minval && xval < midval )   xval = minval;
			if ( xval < maxval && xval >=  midval ) xval = maxval;
			pfield->Data.vlist.val[nn] = xval;
			}
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ l a t                                                ***
 ***                                                                ***
 *** return pointer to structure that contains the latitudes        ***
 ***  at the default grid dimensions on the default basemap         ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_lat

	(
	)

	{
	int				nn;
	float			xlat, xlon;
	int				Inumx, Inumy;
	float			Aglen, **Alats;
	float			*gvalblk;
	int				iix, iiy;
	FpaEQTN_DATA	*pfield;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the latitudes */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Evaluate the latitudes at each position (in degrees) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{
			if ( pos_to_ll(&FpaEqtnDefs.mprojEval, FpaEqtnDefs.posEval[nn],
					&xlat, &xlon) )
				(void) add_point_to_vlist(&pfield->Data.vlist,
						FpaEqtnDefs.posEval[nn], xlat);
			else
				{
				(void) fprintf(stderr, "[oper_lat] Error in latitude");
				(void) fprintf(stderr, " calculated from point evaluation\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Get latitudes for given map projection */
		if ( !grid_positions(&FpaEqtnDefs.mprojEval, &Inumx, &Inumy, &Aglen,
				NullPtr(POINT ***), &Alats, NullPtr(float ***)) )
					return NullPtr(FpaEQTN_DATA *);

		/* Initialize a structure to hold the latitudes */
		pfield = init_eqtn_data(FpaEQT_Grid);

		/* Define the basic attributes */
		pfield->Data.gridd.nx = Inumx;
		pfield->Data.gridd.ny = Inumy;
		pfield->Data.gridd.gridlen = Aglen;

		/* Allocate space for pointers and array of grid point data */
		gvalblk = INITMEM(float, Inumy*Inumx);
		pfield->Data.gridd.gval = INITMEM(float *, Inumy);

		/* Set pointers and set the latitude (in degrees) */
		/*  at each grid point                            */
		for ( iiy=0; iiy<Inumy; iiy++ )
			{
			pfield->Data.gridd.gval[iiy] = gvalblk + (iiy*Inumx);
			for ( iix=0; iix<Inumx; iix++ )
				{
				pfield->Data.gridd.gval[iiy][iix] = Alats[iiy][iix];
				}
			}
		}

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ l o n                                                ***
 ***                                                                ***
 *** return pointer to structure that contains the longitudes       ***
 ***  at the default grid dimensions on the default basemap         ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_lon

	(
	)

	{
	int				nn;
	float			xlat, xlon;
	int				Inumx, Inumy;
	float			Aglen, **Alons;
	float			*gvalblk;
	int				iix, iiy;
	FpaEQTN_DATA	*pfield;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the longitudes */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Evaluate the longitudes at each position (in degrees) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{
			if ( pos_to_ll(&FpaEqtnDefs.mprojEval, FpaEqtnDefs.posEval[nn],
					&xlat, &xlon) )
				(void) add_point_to_vlist(&pfield->Data.vlist,
						FpaEqtnDefs.posEval[nn], xlon);
			else
				{
				(void) fprintf(stderr, "[oper_lon] Error in longitude");
				(void) fprintf(stderr, " calculated from point evaluation\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Get longitudes for given map projection */
		if ( !grid_positions(&FpaEqtnDefs.mprojEval, &Inumx, &Inumy, &Aglen,
				NullPtr(POINT ***), NullPtr(float ***), &Alons) )
					return NullPtr(FpaEQTN_DATA *);

		/* Initialize a structure to hold the longitudes */
		pfield = init_eqtn_data(FpaEQT_Grid);

		/* Define the basic attributes */
		pfield->Data.gridd.nx = Inumx;
		pfield->Data.gridd.ny = Inumy;
		pfield->Data.gridd.gridlen = Aglen;

		/* Allocate space for pointers and array of grid point data */
		gvalblk = INITMEM(float, Inumy*Inumx);
		pfield->Data.gridd.gval = INITMEM(float *, Inumy);

		/* Set pointers and set the longitude (in degrees) */
		/*  at each grid point                             */
		for ( iiy=0; iiy<Inumy; iiy++ )
			{
			pfield->Data.gridd.gval[iiy] = gvalblk + (iiy*Inumx);
			for ( iix=0; iix<Inumx; iix++ )
				{
				pfield->Data.gridd.gval[iiy][iix] = Alons[iiy][iix];
				}
			}
		}

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ d d x                                                ***
 ***                                                                ***
 *** return pointer to structure that contains first derivative     ***
 ***  wrt x of input structure                                      ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_ddx

	(
	FpaEQTN_DATA	*pfld		/* pointer to input structure */
	)

	{
	int				iix, iiy, nn;
	double			cunits, deriv;
	POINT			pos;
	SURFACE			sfc = NullSfc;
	FpaEQTN_DATA	*pfield, *ptemp;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message for SCALAR Object in input structure */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) fprintf(stderr, "[oper_ddx] 1st derivative of SCALAR is 0\n");
		pfield = init_eqtn_data(FpaEQT_Scalar);
		pfield->Data.scalr.sval = 0.0;
		return pfield;
		}

	/* Get SPLINE work Object from input GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		ptemp = convert_eqtn_data(FpaEQT_Spline, pfld);
		}

	/* Copy SPLINE work Object from input SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		ptemp = copy_eqtn_data(pfld);
		}

	/* Error message for VLIST Object in input structure */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_ddx] 1st derivative of VLIST is 0\n");
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] = 0.0;
		return pfield;
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_ddx] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Define a SURFACE Object from the SPLINE work Object */
	sfc = create_surface();
	(void) define_surface_spline(sfc, ptemp->Data.splne.m,
			ptemp->Data.splne.n, &ptemp->Data.splne.mp,
			ptemp->Data.splne.origin, ptemp->Data.splne.orient,
			ptemp->Data.splne.gridlen, *ptemp->Data.splne.cvs,
			ptemp->Data.splne.n);

	/* Set conversion to units/meter */
	cunits = FpaEqtnDefs.mprojEval.grid.units;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the 1st derivative */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Determine 1st derivative ... ddx[fld] (at each point) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{

			/* Evaluate 1st derivative at position in units/meter */
			if ( !eval_sfc_1st_deriv(sfc, FpaEqtnDefs.posEval[nn],
					&deriv, NullDouble) )
				{
				/* Error message if 1st derivative not evaluated correctly */
				(void) fprintf(stderr, "[oper_ddx] Error in 1st derivative at");
				(void) fprintf(stderr, " position: %f  %f\n",
						FpaEqtnDefs.posEval[nn][X], FpaEqtnDefs.posEval[nn][Y]);
				}
			(void) add_point_to_vlist(&pfield->Data.vlist,
					FpaEqtnDefs.posEval[nn], (float)(deriv / cunits));
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Initialize a GRID Object to hold the 1st derivative        */
		/*  ... by converting the SPLINE work Object to a GRID Object */
		/*      at the default grid dimensions                        */
		pfield = convert_eqtn_data(FpaEQT_Grid, ptemp);

		/* Determine 1st derivative ... ddx[fld] */

		/* Evaluate 1st derivative at each grid point in units/meter */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				pos[X] = (float) iix * pfield->Data.gridd.gridlen;
				pos[Y] = (float) iiy * pfield->Data.gridd.gridlen;
				if ( !eval_sfc_1st_deriv(sfc, pos, &deriv, NullDouble) )
					{
					/* Error message if 1st derivative not evaluated correctly */
					(void) fprintf(stderr, "[oper_ddx] Error in 1st derivative at");
					(void) fprintf(stderr, " position: %f  %f\n", pos[X], pos[Y]);
					}
				pfield->Data.gridd.gval[iiy][iix] = (float)(deriv / cunits);
				}
		}

	/* Free space used by SURFACE Object and SPLINE Object */
	sfc = destroy_surface(sfc);
	(void) free_eqtn_data(ptemp);

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ d d y                                                ***
 ***                                                                ***
 *** return pointer to structure that contains first derivative     ***
 ***  wrt y of input structure                                      ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_ddy

	(
	FpaEQTN_DATA	*pfld		/* pointer to input structure */
	)

	{
	int				iix, iiy, nn;
	double			cunits, deriv;
	POINT			pos;
	SURFACE			sfc = NullSfc;
	FpaEQTN_DATA	*pfield, *ptemp;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message for SCALAR Object in input structure */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) fprintf(stderr, "[oper_ddy] 1st derivative of SCALAR is 0\n");
		pfield = init_eqtn_data(FpaEQT_Scalar);
		pfield->Data.scalr.sval = 0.0;
		return pfield;
		}

	/* Get SPLINE work Object from input GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		ptemp = convert_eqtn_data(FpaEQT_Spline, pfld);
		}

	/* Copy SPLINE work Object from input SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		ptemp = copy_eqtn_data(pfld);
		}

	/* Error message for VLIST Object in input structure */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_ddy] 1st derivative of VLIST is 0\n");
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] = 0.0;
		return pfield;
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_ddy] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Define a SURFACE Object from the SPLINE work Object */
	sfc = create_surface();
	(void) define_surface_spline(sfc, ptemp->Data.splne.m,
			ptemp->Data.splne.n, &ptemp->Data.splne.mp,
			ptemp->Data.splne.origin, ptemp->Data.splne.orient,
			ptemp->Data.splne.gridlen, *ptemp->Data.splne.cvs,
			ptemp->Data.splne.n);

	/* Set conversion to units/meter */
	cunits = FpaEqtnDefs.mprojEval.grid.units;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the 1st derivative */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Determine 1st derivative ... ddy[fld] (at each point) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{

			/* Evaluate 1st derivative at position in units/meter */
			if ( !eval_sfc_1st_deriv(sfc, FpaEqtnDefs.posEval[nn],
					NullDouble, &deriv) )
				{
				/* Error message if 1st derivative not evaluated correctly */
				(void) fprintf(stderr, "[oper_ddy] Error in 1st derivative at");
				(void) fprintf(stderr, " position: %f  %f\n",
						FpaEqtnDefs.posEval[nn][X], FpaEqtnDefs.posEval[nn][Y]);
				}
			(void) add_point_to_vlist(&pfield->Data.vlist,
					FpaEqtnDefs.posEval[nn], (float)(deriv / cunits));
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Initialize a GRID Object to hold the 1st derivative        */
		/*  ... by converting the SPLINE work Object to a GRID Object */
		/*      at the default grid dimensions                        */
		pfield = convert_eqtn_data(FpaEQT_Grid, ptemp);

		/* Determine 1st derivative ... ddy[fld] */

		/* Evaluate 1st derivative at each grid point in units/meter */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				pos[X] = (float) iix * pfield->Data.gridd.gridlen;
				pos[Y] = (float) iiy * pfield->Data.gridd.gridlen;
				if ( !eval_sfc_1st_deriv(sfc, pos, NullDouble, &deriv) )
					{
					/* Error message if 1st derivative not evaluated correctly */
					(void) fprintf(stderr, "[oper_ddy] Error in 1st derivative at");
					(void) fprintf(stderr, " position: %f  %f\n", pos[X], pos[Y]);
					}
				pfield->Data.gridd.gval[iiy][iix] = (float)(deriv / cunits);
				}
		}

	/* Free space used by SURFACE Object and SPLINE Object */
	sfc = destroy_surface(sfc);
	(void) free_eqtn_data(ptemp);

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ c u r v                                              ***
 ***                                                                ***
 *** return pointer to structure that contains curvature of input   ***
 ***  structure                                                     ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_curv

	(
	FpaEQTN_DATA	*pfld		/* pointer to input structure */
	)

	{
	int				nn;
	double			cunits, curv;
	int				iix, iiy;
	POINT			pos;
	SURFACE			sfc = NullSfc;
	FpaEQTN_DATA	*pfield, *ptemp;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message for SCALAR Object in input structure */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) fprintf(stderr, "[oper_curv] Curvature of SCALAR is 0\n");
		pfield = init_eqtn_data(FpaEQT_Scalar);
		pfield->Data.scalr.sval = 0.0;
		return pfield;
		}

	/* Get SPLINE work Object from input GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		ptemp = convert_eqtn_data(FpaEQT_Spline, pfld);
		}

	/* Copy SPLINE work Object from input SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		ptemp = copy_eqtn_data(pfld);
		}

	/* Error message for VLIST Object in input structure */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_curv] Curvature of VLIST is 0\n");
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] = 0.0;
		return pfield;
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_curv] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Define a SURFACE Object from the SPLINE work Object */
	sfc = create_surface();
	(void) define_surface_spline(sfc, ptemp->Data.splne.m,
			ptemp->Data.splne.n, &ptemp->Data.splne.mp,
			ptemp->Data.splne.origin, ptemp->Data.splne.orient,
			ptemp->Data.splne.gridlen, *ptemp->Data.splne.cvs,
			ptemp->Data.splne.n);

	/* Set conversion to units/meter */
	cunits = FpaEqtnDefs.mprojEval.grid.units;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the curvature */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Determine curvature ... curv[fld] (at each point) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{

			/* Evaluate curvature at position in 1/metres */
			if ( !eval_sfc_curvature(sfc, FpaEqtnDefs.posEval[nn],
					&curv, NullPoint) )
				{
				/* Error message if curvature not evaluated correctly */
				(void) fprintf(stderr, "[oper_curv] Error in curvature at");
				(void) fprintf(stderr, " position: %f  %f\n",
						FpaEqtnDefs.posEval[nn][X], FpaEqtnDefs.posEval[nn][Y]);
				}
			(void) add_point_to_vlist(&pfield->Data.vlist,
					FpaEqtnDefs.posEval[nn], (float)(curv / cunits));
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Initialize a GRID Object to hold the curvature             */
		/*  ... by converting the SPLINE work Object to a GRID Object */
		/*      at the default grid dimensions                        */
		pfield = convert_eqtn_data(FpaEQT_Grid, ptemp);

		/* Determine curvature ... curv[fld] */

		/* Evaluate curvature at each grid point in 1/metres */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				pos[X] = (float) iix * pfield->Data.gridd.gridlen;
				pos[Y] = (float) iiy * pfield->Data.gridd.gridlen;
				if ( !eval_sfc_curvature(sfc, pos, &curv, NullPoint) )
					{
					/* Error message if curvature not evaluated correctly */
					(void) fprintf(stderr, "[oper_curv] Error in curvature at");
					(void) fprintf(stderr, " position: %f  %f\n", pos[X], pos[Y]);
					}
				pfield->Data.gridd.gval[iiy][iix] = (float)(curv / cunits);
				}
		}

	/* Free space used by SURFACE Object and SPLINE Object */
	sfc = destroy_surface(sfc);
	(void) free_eqtn_data(ptemp);

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ d d t                                                ***
 ***                                                                ***
 *** return pointer to structure that contains first derivative     ***
 ***  wrt time from timeseries of structures                        ***
 ***                                                                ***
 *** Note that the times in the timeseries are always wrt the time  ***
 ***  of evaluation (in minutes), and that the evaluation time is   ***
 ***  always at 0 minutes                                           ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) timeseries          ***
 ***  structures are allowed and that a minimum of                  ***
 ***  FpaEQFF_MinFlds (2) timeseries structures are required        ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_ddt

	(
	int				jflds,		/* number of structures in timeseries */
	int				ptimes[],	/* times in timeseries (in minutes) */
	FpaEQTN_DATA	*pflds[]	/* pointer to timeseries structures */
	)

	{
	LOGICAL			allscalar, allgrid, allspline, allvlist;
	int				iix, iiy, jj, nn;
	FpaEQTN_DATA	*pfield, *pwork[FpaEQFF_MaxFlds], *ptemp;

	/* Internal buffers for timeseries values */
	static	double	*Times = NullPtr(double *);
	static	double	*Vals  = NullPtr(double *);

	/* Internal buffers for evaluations */
	static	LOGICAL	FirstCall = TRUE;
	static	int		ENum;
	static	double	*ETime;
	static	double	*EVal;
	static	double	*EDerv;

	/* Return Null if missing structures to evaluate */
	if ( jflds < 1 ) return NullPtr(FpaEQTN_DATA *);
	for ( jj=0; jj<jflds; jj++ )
		if ( IsNull(pflds[jj]) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message if too many timeseries structures */
	if ( jflds > FpaEQFF_MaxFlds )
		{
		(void) fprintf(stderr, "[oper_ddt] Too many timeseries structures: %d",
				jflds);
		(void) fprintf(stderr, "   (Max allowed: %d)\n", FpaEQFF_MaxFlds);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Error message if not enough timeseries structures */
	if ( jflds < FpaEQFF_MinFlds )
		{
		(void) fprintf(stderr, "[oper_ddt] Not enough timeseries structures: %d",
				jflds);
		(void) fprintf(stderr, "   (Min required: %d)\n", FpaEQFF_MinFlds);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Initialize internal buffers for evaluations */
	if ( FirstCall )
		{
		FirstCall = FALSE;
		ENum  = 1;
		ETime = INITMEM(double, ENum);
		EVal  = INITMEM(double, ENum);
		EDerv = INITMEM(double, ENum);
		for ( nn=0; nn<ENum; nn++ )
			{
			ETime[nn] = 0.0;
			EVal[nn]  = 0.0;
			EDerv[nn] = 0.0;
			}
		}

	/* Allocate work space for time derivative values */
	Times = GETMEM(Times, double, jflds);
	Vals  = GETMEM(Vals,  double, jflds);

	/* Set times for time derivative work array */
	for ( jj=0; jj<jflds; jj++ ) Times[jj] = (double) ptimes[jj];

	/* Debug output for times in timeseries */
	if ( DebugMode )
		{
		dprintf(stderr, "[oper_ddt] Number of times in series: %d", jflds);
		dprintf(stderr, "   Times (in minutes):");
		for ( jj=0; jj<jflds; jj++ ) dprintf(stderr, "  %g", Times[jj]);
		dprintf(stderr, "\n");
		}

	/* Debug output for structures in timeseries */
	if ( DebugMode )
		{
		dprintf(stderr, "[oper_ddt] Number of structures in series: %d\n",
				jflds);
		for ( jj=0; jj<jflds; jj++ )
			{
			dprintf(stderr, " Structure: %d   Type: %d   at: <%lx>\n",
					jj, pflds[jj]->Type, (unsigned long) pflds[jj]);
			}
		}

	/* Determine type of Objects in timeseries structures */
	/*  ... and ensure that they all match!               */
	allscalar = allgrid = allspline = allvlist = TRUE;
	for ( jj=0; jj<jflds; jj++ )
		{
		if ( pflds[jj]->Type == FpaEQT_Scalar )
				allgrid = allspline = allvlist = FALSE;
		else if ( pflds[jj]->Type == FpaEQT_Grid )
				allscalar = allspline = allvlist = FALSE;
		else if ( pflds[jj]->Type == FpaEQT_Spline )
				allscalar = allgrid = allvlist = FALSE;
		else if ( pflds[jj]->Type == FpaEQT_Vlist )
				allscalar = allgrid = allspline = FALSE;

		/* Error message if any Objects are of unacceptable type */
		else
			{
			(void) fprintf(stderr, "[oper_ddt] Unacceptable type of Object\n");
			(void) fprintf(stderr, "  Structure pflds[%d]", jj);
			(void) fprintf(stderr, "    Object type: %d\n", pflds[jj]->Type);
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	/* Error message if Objects do not match */
	if ( !allscalar && !allgrid && !allspline && !allvlist )
		{
		(void) fprintf(stderr, "[oper_ddt] Unmatched timeseries structures\n");
		for ( jj=0; jj<jflds; jj++ )
			{
			(void) fprintf(stderr, "  Structure pflds[%d]", jj);
			(void) fprintf(stderr, "    Object type: %d\n", pflds[jj]->Type);
			}
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Determine time derivative for SCALAR Objects */
	if ( allscalar )
		{

		/* Initialize structure from first argument structure */
		pfield = copy_eqtn_data(pflds[0]);

		/* Extract values for time derivative work array */
		for ( jj=0; jj<jflds; jj++ )
			{
			Vals[jj]  = (double) pflds[jj]->Data.scalr.sval;
			}

		/* Spline timeseries of values */
		(void) Tween1(jflds, Times, Vals, ENum, ETime, EVal, EDerv);

		/* Debug output for values in timeseries */
		if ( DebugMode )
			{
			dprintf(stderr, "[oper_ddt] Values:");
			for ( jj=0; jj<jflds; jj++ ) dprintf(stderr, "  %g", Vals[jj]);
			dprintf(stderr, "\n");
			dprintf(stderr, "[oper_ddt] Output Value: %g", EVal[0]);
			dprintf(stderr, "   Derivative: %g\n", EDerv[0]);
			}

		/* Overwrite structure data with time derivative in units/s */
		pfield->Data.scalr.sval = EDerv[0]/60.0;
		}

	/* Determine time derivative by looping on grid values */
	/*  for GRID or SPLINE Objects                         */
	else if ( allgrid || allspline )
		{

		/* Initialize pointers to work Objects */
		for ( jj=0; jj<jflds; jj++ ) pwork[jj] = NullPtr(FpaEQTN_DATA *);

		/* Convert each Object to GRID work Object */
		/*  at the default grid dimensions         */
		for ( jj=0; jj<jflds; jj++ )
			{

			/* Copy GRID Object to work GRID Object */
			if ( pflds[jj]->Type == FpaEQT_Grid )
				{
				/* Ensure that GRID Object has default grid dimensions */
				if ( pflds[jj]->Data.gridd.nx
										!= FpaEqtnDefs.mprojEval.grid.nx
						|| pflds[jj]->Data.gridd.ny
										!= FpaEqtnDefs.mprojEval.grid.ny
						|| pflds[jj]->Data.gridd.gridlen
										!= FpaEqtnDefs.mprojEval.grid.gridlen )
					{
					(void) fprintf(stderr, "[oper_ddt] Grid dimensions");
					(void) fprintf(stderr, " do not match default\n");
					/* Convert GRID Object to default grid dimensions */
					ptemp = convert_eqtn_data(FpaEQT_Spline, pflds[jj]);
					pwork[jj] = convert_eqtn_data(FpaEQT_Grid, ptemp);
					(void) free_eqtn_data(ptemp);
					}

				else
					{
					pwork[jj] = copy_eqtn_data(pflds[jj]);
					}
				}

			/* Convert SPLINE Object to work GRID Object */
			else if ( pflds[jj]->Type == FpaEQT_Spline )
				{
				pwork[jj] = convert_eqtn_data(FpaEQT_Grid, pflds[jj]);
				}
			}

		/* Initialize output structure from first work GRID Object */
		pfield = copy_eqtn_data(pwork[0]);

		/* Determine time derivative at each grid location */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{

				/* Extract values for time derivative work array */
				for ( jj=0; jj<jflds; jj++ )
					Vals[jj]  = (double) pwork[jj]->Data.gridd.gval[iiy][iix];

				/* Spline timeseries of values */
				(void) Tween1(jflds, Times, Vals, ENum, ETime, EVal, EDerv);

				/* Debug output for values in timeseries */
				if ( DebugMode )
					{
					dprintf(stderr, "[oper_ddt] Values:");
					for ( jj=0; jj<jflds; jj++ )
						dprintf(stderr, "  %g", Vals[jj]);
					dprintf(stderr, "\n");
					dprintf(stderr, "[oper_ddt] Output Value: %g", EVal[0]);
					dprintf(stderr, "   Derivative: %g\n", EDerv[0]);
					}

				/* Overwrite structure data with time derivative */
				/*  in units/s                                   */
				pfield->Data.gridd.gval[iiy][iix] = EDerv[0]/60.0;
				}

		/* Free space used by work Objects */
		for ( jj=0; jj<jflds; jj++ ) (void) free_eqtn_data(pwork[jj]);
		}

	/* Determine time derivative by looping on each position */
	/*  for VLIST Objects                                    */
	else if ( allvlist )
		{

		/* Compare point lists of all VLIST Objects */
		for ( pfield= NullPtr(FpaEQTN_DATA *), jj=0; jj<jflds; jj++ )
			{

			/* Initialize structure from first VLIST Object */
			if ( IsNull(pfield) ) pfield = copy_eqtn_data(pflds[jj]);

			/* Compare point lists of all other VLIST Objects */
			else if ( !same_vlist_points(&pfield->Data.vlist,
							&pflds[jj]->Data.vlist) )
				{
				(void) fprintf(stderr, "[oper_ddt] Points in");
				(void) fprintf(stderr, " VLIST Objects do not match\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			}

		/* Determine time derivative at each position */
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			{

			/* Extract values for time derivative work array */
			for ( jj=0; jj<jflds; jj++ )
				Vals[jj]  = (double) pflds[jj]->Data.vlist.val[nn];

			/* Spline timeseries of values */
			(void) Tween1(jflds, Times, Vals, ENum, ETime, EVal, EDerv);

			/* Debug output for values in timeseries */
			if ( DebugMode )
				{
				dprintf(stderr, "[oper_ddt] Values:");
				for ( jj=0; jj<jflds; jj++ )
					dprintf(stderr, "  %g", Vals[jj]);
				dprintf(stderr, "\n");
				dprintf(stderr, "[oper_ddt] Output Value: %g", EVal[0]);
				dprintf(stderr, "   Derivative: %g\n", EDerv[0]);
				}

			/* Overwrite structure data with time derivative */
			/*  in units/s                                   */
			pfield->Data.vlist.val[nn] = EDerv[0]/60.0;
			}
		}

	/* Return pointer to structure containing time derivative */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ l a p l c                                            ***
 ***                                                                ***
 *** return pointer to structure that contains laplacian of input   ***
 ***  structure                                                     ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_laplc

	(
	FpaEQTN_DATA	*pfld		/* pointer to input structure */
	)

	{
	int				iix, iiy, nn;
	double			cunits, xxdrv, yydrv;
	POINT			pos;
	SURFACE			sfc = NullSfc;
	FpaEQTN_DATA	*pfield, *ptemp;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(pfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message for SCALAR Object in input structure */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) fprintf(stderr, "[oper_laplc] Laplacian of SCALAR is 0\n");
		pfield = init_eqtn_data(FpaEQT_Scalar);
		pfield->Data.scalr.sval = 0.0;
		return pfield;
		}

	/* Get SPLINE work Object from input GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		ptemp = convert_eqtn_data(FpaEQT_Spline, pfld);
		}

	/* Copy SPLINE work Object from input SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		ptemp = copy_eqtn_data(pfld);
		}

	/* Error message for VLIST Object in input structure */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_laplc] Laplacian of VLIST is 0\n");
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] = 0.0;
		return pfield;
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_laplc] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Define a SURFACE Object from the SPLINE work Object */
	sfc = create_surface();
	(void) define_surface_spline(sfc, ptemp->Data.splne.m,
			ptemp->Data.splne.n, &ptemp->Data.splne.mp,
			ptemp->Data.splne.origin, ptemp->Data.splne.orient,
			ptemp->Data.splne.gridlen, *ptemp->Data.splne.cvs,
			ptemp->Data.splne.n);

	/* Set conversion to units/meter/meter */
	cunits = FpaEqtnDefs.mprojEval.grid.units * FpaEqtnDefs.mprojEval.grid.units;

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the laplacian */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Determine laplacian ... ddx[ddx[fld]] + ddy[ddy[fld]] */
		/*  (at each point)                                      */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{

			/* Evaluate 2nd derivative at position in units/meter/meter */
			if ( !eval_sfc_2nd_deriv(sfc, FpaEqtnDefs.posEval[nn],
											&xxdrv, NullDouble, &yydrv) )
				{
				/* Error message if 2nd derivative not evaluated correctly */
				(void) fprintf(stderr, "[oper_laplc] Error in 2nd derivative at");
				(void) fprintf(stderr, " position: %f  %f\n",
						FpaEqtnDefs.posEval[nn][X], FpaEqtnDefs.posEval[nn][Y]);
				}
			(void) add_point_to_vlist(&pfield->Data.vlist,
					FpaEqtnDefs.posEval[nn], (float)((xxdrv + yydrv) / cunits));
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Initialize a GRID Object to hold the laplacian             */
		/*  ... by converting the SPLINE work Object to a GRID Object */
		/*      at the default grid dimensions                        */
		pfield = convert_eqtn_data(FpaEQT_Grid, ptemp);

		/* Determine laplacian ... ddx[ddx[fld]] + ddy[ddy[fld]] */

		/* Evaluate 2nd derivative at each grid point in units/meter/meter */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				pos[X] = (float) iix * pfield->Data.gridd.gridlen;
				pos[Y] = (float) iiy * pfield->Data.gridd.gridlen;
				if ( !eval_sfc_2nd_deriv(sfc, pos,
											&xxdrv, NullDouble, &yydrv) )
					{
					/* Error message if 2nd derivative not evaluated correctly */
					(void) fprintf(stderr, "[oper_laplc] Error in 2nd derivative at");
					(void) fprintf(stderr, " position: %f  %f\n", pos[X], pos[Y]);
					}
				pfield->Data.gridd.gval[iiy][iix]
						= (float)((xxdrv + yydrv) / cunits);
				}
		}

	/* Free space used by SURFACE Object and SPLINE Object */
	sfc = destroy_surface(sfc);
	(void) free_eqtn_data(ptemp);

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ a d v c t                                            ***
 ***                                                                ***
 *** return pointer to structure that contains advection of input   ***
 ***  structure by the given wind component structures              ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_advct

	(
	FpaEQTN_DATA	*pfld,		/* pointer to structure to be advected */
	FpaEQTN_DATA	*puwnd,		/* pointer to structure containing */
								/*  u component of advecting wind  */
	FpaEQTN_DATA	*pvwnd		/* pointer to structure containing */
								/*  v component of advecting wind  */
	)

	{
	int				nn;
	FpaEQTN_DATA	*pfield, *ptemp, *pddx, *pddy, *pnadv, *pneg;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(pfld) || IsNull(puwnd) || IsNull(pvwnd) )
			return NullPtr(FpaEQTN_DATA *);

	/* Error message for SCALAR Object in structure to be advected */
	if ( pfld->Type == FpaEQT_Scalar )
		{
		(void) fprintf(stderr, "[oper_advct] Advection of SCALAR is 0\n");
		pfield = init_eqtn_data(FpaEQT_Scalar);
		pfield->Data.scalr.sval = 0.0;
		return pfield;
		}

	/* Get SPLINE work Object from input GRID Object */
	else if ( pfld->Type == FpaEQT_Grid )
		{
		ptemp = convert_eqtn_data(FpaEQT_Spline, pfld);
		}

	/* Copy SPLINE work Object from input SPLINE Object */
	else if ( pfld->Type == FpaEQT_Spline )
		{
		ptemp = copy_eqtn_data(pfld);
		}

	/* Error message for VLIST Object in structure to be advected */
	else if ( pfld->Type == FpaEQT_Vlist )
		{
		(void) fprintf(stderr, "[oper_advct] Advection of VLIST is 0\n");
		pfield = copy_eqtn_data(pfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			pfield->Data.vlist.val[nn] = 0.0;
		return pfield;
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_advct] Unknown Object type\n");
		(void) fprintf(stderr, "  pfld: %d\n", pfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Determine advection ... -(uwnd*ddx[fld] + vwnd*ddy[fld]) */

	/* Determine 1st derivative wrt x and y */
	pddx = oper_ddx(ptemp);
	pddy = oper_ddy(ptemp);

	/* Calculate the advection from wind components and derivatives */
	pnadv = oper_plus(oper_mult(puwnd, pddx), oper_mult(pvwnd, pddy));
	pneg = init_eqtn_data(FpaEQT_Scalar);
	pneg->Data.scalr.sval = -1.0;
	pfield = oper_mult(pneg, pnadv);

	/* Free space used by work structures */
	(void) free_eqtn_data(ptemp);
	(void) free_eqtn_data(pddx);
	(void) free_eqtn_data(pddy);
	(void) free_eqtn_data(pnadv);
	(void) free_eqtn_data(pneg);

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ d i v r g                                            ***
 ***                                                                ***
 *** return pointer to structure that contains divergence of given  ***
 ***  wind component structures                                     ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_divrg

	(
	FpaEQTN_DATA	*puwnd,		/* pointer to structure containing */
								/*  u component of wind            */
	FpaEQTN_DATA	*pvwnd		/* pointer to structure containing */
								/*  v component of wind            */
	)

	{
	FpaEQTN_DATA	*pfield;

	/* Return Null if missing structures to evaluate */
	if ( IsNull(puwnd) || IsNull(pvwnd) ) return NullPtr(FpaEQTN_DATA *);

	/* Determine divergence ... ddx[uwnd] + ddy[vwnd] */

	/* Calculate the divergence from derivatives of wind components */
	pfield = oper_plus(oper_ddx(puwnd), oper_ddy(pvwnd));

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ s v p r s                                            ***
 ***                                                                ***
 *** return pointer to structure that contains saturation vapour    ***
 ***  pressures derived from the given temperature structure        ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_svprs

	(
	FpaEQTN_DATA	*ptfld		/* pointer to structure containing */
								/*  temperature field              */
	)

	{
	int				iix, iiy, nn;
	float			rat, rati, expnt;
	float			triple = 273.16;
	FpaEQTN_DATA	*pfield, *ptemp;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(ptfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Determine saturation vapour pressure based on type of Object */
	/*  in structure, by copying structure and overwriting the data */
	/* Saturation vapour pressure is determined using the           */
	/*  Goff-Gratch formulae (over water in mb) given in            */
	/*  WMO #188 TP94, 1966  International Meteorological Tables,   */
	/*  Introduction to Tables 4.6 and 4.7                          */
	/* Note that saturation vapour pressure is returned in Pascals  */
	/* Note that all SPLINE Objects are converted to GRID Objects   */
	/*  at the default grid dimensions                              */

	if ( ptfld->Type == FpaEQT_Scalar )
		{
		pfield = copy_eqtn_data(ptfld);
		rat = ptfld->Data.scalr.sval / triple;
		rati = triple / ptfld->Data.scalr.sval;
		expnt = 10.79574 * (1.0 - rati)
			- 5.02800 * log10(rat)
			+ 1.50475e-04 * (1.0 - pow(10.0, (-8.2969 * (rat-1.0))))
			+ 0.42873e-03 * (pow(10.0, (4.76955*(1.0-rati))) - 1.0)
			+ 0.78614;
		pfield->Data.scalr.sval = 100.0 * pow(10.0, (double) expnt);
		return pfield;
		}

	else if ( ptfld->Type == FpaEQT_Grid )
		{
		pfield = copy_eqtn_data(ptfld);
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{
				rat = ptfld->Data.gridd.gval[iiy][iix] / triple;
				rati = triple / ptfld->Data.gridd.gval[iiy][iix];
				expnt = 10.79574 * (1.0 - rati)
					- 5.02800 * log10(rat)
					+ 1.50475e-04 * (1.0 - pow(10.0, (-8.2969 * (rat-1.0))))
					+ 0.42873e-03 * (pow(10.0, (4.76955*(1.0-rati))) - 1.0)
					+ 0.78614;
				pfield->Data.gridd.gval[iiy][iix] =
											100.0 * pow(10.0, (double) expnt);
				}
		}

	else if ( ptfld->Type == FpaEQT_Spline )
		{
		ptemp = convert_eqtn_data(FpaEQT_Grid, ptfld);
		pfield = oper_svprs(ptemp);
		(void) free_eqtn_data(ptemp);
		}

	else if ( ptfld->Type == FpaEQT_Vlist )
		{
		pfield = copy_eqtn_data(ptfld);
		for ( nn=0; nn<pfield->Data.vlist.numpts; nn++ )
			{
			rat = ptfld->Data.vlist.val[nn] / triple;
			rati = triple / ptfld->Data.vlist.val[nn];
			expnt = 10.79574 * (1.0 - rati)
				- 5.02800 * log10(rat)
				+ 1.50475e-04 * (1.0 - pow(10.0, (-8.2969 * (rat-1.0))))
				+ 0.42873e-03 * (pow(10.0, (4.76955*(1.0-rati))) - 1.0)
				+ 0.78614;
			pfield->Data.vlist.val[nn] = 100.0 * pow(10.0, (double) expnt);
			}
		}

	/* Error message for unknown Object type */
	else
		{
		(void) fprintf(stderr, "[oper_svprs] Unknown Object type\n");
		(void) fprintf(stderr, "  ptfld: %d\n", ptfld->Type);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ l v l p r s                                          ***
 *** o p e r _ u p r p r s                                          ***
 *** o p e r _ l w r p r s                                          ***
 ***                                                                ***
 *** return pointer to structure that contains the value of the     ***
 ***  pressure from the current level, or the value of the pressure ***
 ***  from the upper or lower level of the current layer            ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_lvlprs

	(
	)

	{
	float					pressure;
	FpaConfigLevelStruct	*ldef;
	FpaEQTN_DATA			*pfield;

	/* Initialize a structure to hold the current pressure value */
	pfield = init_eqtn_data(FpaEQT_Scalar);

	/* Identify the current level */
	ldef = identify_level(FpaEqtnDefs.lvl);
	if ( IsNull(ldef) ) return NullPtr(FpaEQTN_DATA *);

	/* Check that the current level is FpaC_LEVEL type */
	if ( ldef->lvl_type != FpaC_LEVEL )
		{
		(void) fprintf(stderr, "[oper_lvlprs] Cannot extract a");
		(void) fprintf(stderr, " pressure value from level \"%s\"\n",
				SafeStr(FpaEqtnDefs.lvl));
		(void) free_eqtn_data(pfield);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Extract the pressure based on the category of the current level */
	switch ( ldef->lev_lvls->lvl_category )
		{

		/* Category FpaC_LEVELS_PRESSURE ... constant pressure level */
		case FpaC_LEVELS_PRESSURE:
			pressure = (float) atof(ldef->lev_lvls->lvl);
			break;

		/* Error message for all other categories */
		default:
			(void) fprintf(stderr, "[oper_lvlprs] Cannot extract a");
			(void) fprintf(stderr, " pressure value from level \"%s\"\n",
					SafeStr(FpaEqtnDefs.lvl));
			(void) free_eqtn_data(pfield);
			return NullPtr(FpaEQTN_DATA *);
		}

	/* Store the pressure value ... in Pascals */
	pfield->Data.scalr.sval = pressure * 100.0;

	/* Return pointer to evaluated structure */
	return pfield;
	}

static	FpaEQTN_DATA	*oper_uprprs

	(
	)

	{
	float					pressure;
	FpaConfigLevelStruct	*ldef;
	FpaEQTN_DATA			*pfield;

	/* Initialize a structure to hold the current upper pressure value */
	pfield = init_eqtn_data(FpaEQT_Scalar);

	/* Identify the current upper level */
	ldef = identify_level(FpaEqtnDefs.uprlvl);
	if ( IsNull(ldef) ) return NullPtr(FpaEQTN_DATA *);

	/* Check that the current upper level is FpaC_LEVEL type */
	if ( ldef->lvl_type != FpaC_LEVEL )
		{
		(void) fprintf(stderr, "[oper_uprprs] Cannot extract an");
		(void) fprintf(stderr, " upper pressure value from level \"%s\"\n",
				SafeStr(FpaEqtnDefs.uprlvl));
		(void) free_eqtn_data(pfield);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Extract the pressure based on the category of the current upper level */
	switch ( ldef->lev_lvls->lvl_category )
		{

		/* Category FpaC_LEVELS_PRESSURE ... constant pressure level */
		case FpaC_LEVELS_PRESSURE:
			pressure = (float) atof(ldef->lev_lvls->lvl);
			break;

		/* Error message for all other categories */
		default:
			(void) fprintf(stderr, "[oper_uprprs] Cannot extract an");
			(void) fprintf(stderr, " upper pressure value from level \"%s\"\n",
					SafeStr(FpaEqtnDefs.uprlvl));
			(void) free_eqtn_data(pfield);
			return NullPtr(FpaEQTN_DATA *);
		}

	/* Store the pressure value ... in Pascals */
	pfield->Data.scalr.sval = pressure * 100.0;

	/* Return pointer to evaluated structure */
	return pfield;
	}

static	FpaEQTN_DATA	*oper_lwrprs

	(
	)

	{
	float					pressure;
	FpaConfigLevelStruct	*ldef;
	FpaEQTN_DATA			*pfield;

	/* Initialize a structure to hold the current lower pressure value */
	pfield = init_eqtn_data(FpaEQT_Scalar);

	/* Identify the current lower level */
	ldef = identify_level(FpaEqtnDefs.lwrlvl);
	if ( IsNull(ldef) ) return NullPtr(FpaEQTN_DATA *);

	/* Check that the current lower level is FpaC_LEVEL type */
	if ( ldef->lvl_type != FpaC_LEVEL )
		{
		(void) fprintf(stderr, "[oper_lwrprs] Cannot extract an");
		(void) fprintf(stderr, " lower pressure value from level \"%s\"\n",
				SafeStr(FpaEqtnDefs.lwrlvl));
		(void) free_eqtn_data(pfield);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Extract the pressure based on the category of the current lower level */
	switch ( ldef->lev_lvls->lvl_category )
		{

		/* Category FpaC_LEVELS_PRESSURE ... constant pressure level */
		case FpaC_LEVELS_PRESSURE:
			pressure = (float) atof(ldef->lev_lvls->lvl);
			break;

		/* Error message for all other categories */
		default:
			(void) fprintf(stderr, "[oper_lwrprs] Cannot extract an");
			(void) fprintf(stderr, " lower pressure value from level \"%s\"\n",
					SafeStr(FpaEqtnDefs.lwrlvl));
			(void) free_eqtn_data(pfield);
			return NullPtr(FpaEQTN_DATA *);
		}

	/* Store the pressure value ... in Pascals */
	pfield->Data.scalr.sval = pressure * 100.0;

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ s u n a n g                                          ***
 ***                                                                ***
 *** return pointer to structure that contains the field of solar   ***
 ***  zenith angles computed for the current valid time             ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_sunang

	(
	)

	{
	LOGICAL			local;
	int				year, jday, hour, min, seconds;
	float			slat, slon;
	int				nn;
	float			xlat, xlon, aang, zang;
	int				Inumx, Inumy;
	float			Aglen, **Alats, **Alons;
	float			*gvalblk;
	int				iix, iiy;
	FpaEQTN_DATA	*pfield;

	/* Interpret valid time stamp */
	(void) parse_tstamp(FpaEqtnDefs.vtime, &year, &jday, &hour, &min, &local,
						NullLogicalPtr);
	seconds  = hour * 3600;
	seconds += min * 60;
	sun_pos(year, jday, seconds, &slat, &slon);

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Initialize a structure to hold the sun angle values */
		pfield = init_eqtn_data(FpaEQT_Vlist);

		/* Evaluate the sun angle (in degrees) */
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			{
			if ( pos_to_ll(&FpaEqtnDefs.mprojEval, FpaEqtnDefs.posEval[nn],
					&xlat, &xlon) )
				{
				/* If this is a local time treat as GMT in Greenwich */
				if (local) (void) sun_pos_angle(slat, slon,
								xlat, 0.0,  &aang, &zang);
				else       (void) sun_pos_angle(slat, slon,
								xlat, xlon, &aang, &zang);
				(void) add_point_to_vlist(&pfield->Data.vlist,
						FpaEqtnDefs.posEval[nn], zang);
				}
			else
				{
				(void) fprintf(stderr, "[oper_sunang] Error in sun angle");
				(void) fprintf(stderr, " calculated from point evaluation\n");
				(void) free_eqtn_data(pfield);
				return NullPtr(FpaEQTN_DATA *);
				}
			}
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Get latitudes and longitudes for given map projection */
		if ( !grid_positions(&FpaEqtnDefs.mprojEval, &Inumx, &Inumy, &Aglen,
				NullPtr(POINT ***), &Alats, &Alons) )
					return NullPtr(FpaEQTN_DATA *);

		/* Initialize a structure to hold the sun angle values */
		pfield = init_eqtn_data(FpaEQT_Grid);

		/* Define the basic attributes */
		pfield->Data.gridd.nx = Inumx;
		pfield->Data.gridd.ny = Inumy;
		pfield->Data.gridd.gridlen = Aglen;

		/* Allocate space for pointers and array of grid point data */
		gvalblk = INITMEM(float, Inumy*Inumx);
		pfield->Data.gridd.gval = INITMEM(float *, Inumy);

		/* Set pointers and evaluate the sun angle (in degrees) */
		/*  at each grid point                                  */
		for ( iiy=0; iiy<Inumy; iiy++ )
			{
			pfield->Data.gridd.gval[iiy] = gvalblk + (iiy*Inumx);
			for ( iix=0; iix<Inumx; iix++ )
				{
				/* If this is a local time treat as GMT in Greenwich */
				if (local) (void) sun_pos_angle(slat, slon,
							Alats[iiy][iix], 0.0,             &aang, &zang);
				else       (void) sun_pos_angle(slat, slon,
							Alats[iiy][iix], Alons[iiy][iix], &aang, &zang);
				pfield->Data.gridd.gval[iiy][iix] = zang;
				}
			}
		}

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** o p e r _ s u n d i s t                                        ***
 ***                                                                ***
 *** return pointer to structure that contains the relative Earth-  ***
 ***  Sun distance computed for the current valid time              ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*oper_sundist

	(
	)

	{
	LOGICAL			local;
	int				year, jday, hour, min, seconds;
	FpaEQTN_DATA	*pfield;

	/* Initialize a structure to hold the value */
	pfield = init_eqtn_data(FpaEQT_Scalar);

	/* Interpret valid time stamp */
	/* If local time treat as GMT in Greenwich */
	(void) parse_tstamp(FpaEqtnDefs.vtime, &year, &jday, &hour, &min, &local,
						NullLogicalPtr);
	seconds  = hour * 3600;
	seconds += min * 60;

	/* Compute and store the value */
	pfield->Data.scalr.sval = (float) solar_dist(year, jday, seconds);

	/* Return pointer to evaluated structure */
	return pfield;
	}
