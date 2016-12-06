/*****************************************************************************
 ***                                                                       ***
 ***  u s e r _ w i n d s . c                                              ***
 ***                                                                       ***
 ***  This module is designed to access user-defined routines to extract   ***
 ***  wind speed and direction from fields of meteorological data.         ***
 ***                                                                       ***
 ***  Three example routines are given.                                    ***
 ***  The routine "winds_from_equations()" gives an example of winds       ***
 ***  calculated using a Generic equation from the configuration files.    ***
 ***  The routine "winds_from_values()" gives an example of winds          ***
 ***  calculated by passing values to a function that the user would       ***
 ***  supply (given here by "winds_from_function_call()").                 ***
 ***  The routine "gradient_wind_function()" gives an alternative way to   ***
 ***  calculate a gradient wind, using a more complicated function call    ***
 ***  rather than the equations given in the configuration files.  This    ***
 ***  function also allows a cross isobaric estimate to be added.          ***
 ***                                                                       ***
 ***  Version 4 (c) Copyright 1997 Environment Canada (AES)                ***
 ***  Version 5 (c) Copyright 1998 Environment Canada (AES)                ***
 ***  Version 6 (c) Copyright 2001 Environment Canada (MSC)                ***
 ***  Version 7 (c) Copyright 2006 Environment Canada                      ***
 ***  Version 8 (c) Copyright 2011 Environment Canada                      ***
 ***                                                                       ***
 ***  This file is part of the Forecast Production Assistant (FPA).        ***
 ***  The FPA is free software: you can redistribute it and/or modify it   ***
 ***  under the terms of the GNU General Public License as published by    ***
 ***  the Free Software Foundation, either version 3 of the License, or    ***
 ***  any later version.                                                   ***
 ***                                                                       ***
 ***  The FPA is distributed in the hope that it will be useful, but       ***
 ***  WITHOUT ANY WARRANTY; without even the implied warranty of           ***
 ***  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                 ***
 ***  See the GNU General Public License for more details.                 ***
 ***                                                                       ***
 ***  You should have received a copy of the GNU General Public License    ***
 ***  along with the FPA.  If not, see <http://www.gnu.org/licenses/>.     ***
 ***                                                                       ***
 *****************************************************************************/

/* define or undef checking parameter to determine calculation times */
#define TIME_WINDS

/* define or undef parameter to estimate cross isobaric correction */
/*  in gradient_wind_function()                                    */
#undef VR_CROSS_ISO

/* FPA library definitions */
#include <fpa.h>

/* Standard library definitions */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>


/* Interface functions                            */
/*  ... these are defined in /lib/extract/winds.h */


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A1. Routines accessed internally are defined here.                  <*/
/*>                                                                     <*/
/*>     Note that internal example routines are defined here.           <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Internal static functions (User Defined Routines) */
static	void		winds_from_function_call(float *, float *, float *, float *,
								float *, float *, float *, float *);


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A2. Generic equations cross-referenced in configuration file        <*/
/*>      are added here, as in:                                         <*/
/*>                                                                     <*/
/*>         static const STRING UgPres = "ugpres";                      <*/
/*>                                                                     <*/
/*>     Note that the example equations are set to "geostrophic" wind.  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Generic equations cross-referenced in configuration file */
static	const	STRING	UEqtn1 = "ugpres";
static	const	STRING	VEqtn1 = "vgpres";
static	const	STRING	UEqtn2 = "ughght";
static	const	STRING	VEqtn2 = "vghght";


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A3. Elements/Levels/Constants/Units cross-referenced in             <*/
/*>      configuration files are added here, as in:                     <*/
/*>                                                                     <*/
/*>         static const STRING Temperature = "temperature";            <*/
/*>                                                                     <*/
/*>     Note that parameters required by the example functions are set  <*/
/*>      here.                                                          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Elements cross-referenced in configuration file */
static	const	STRING	Pressure       = "pressure";
static	const	STRING	Temperature    = "temperature";
static	const	STRING	SeaTemperature = "sea_temperature";
static	const	STRING	Height         = "height";

/* Levels cross-referenced in configuration file */
static	const	STRING	Msl            = "msl";
static	const	STRING	Sfc            = "surface";

/* Constants cross-referenced in configuration file */
static	const	STRING	Rad            = "RAD";
static	const	STRING	Rho            = "ROSTD";
static	const	STRING	Grav           = "GRAV";
static	const	STRING	Cor            = "COR";

/* Units cross-referenced in configuration file */
static	const	STRING	MKS            = "MKS";
static	const	STRING	MperS          = "m/s";
static	const	STRING	Mb             = "mb";
static	const	STRING	DegC           = "degreesC";


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A4. Prototypes for user defined functions are added here, as in:    <*/
/*>                                                                     <*/
/*>         static WINDFUNC_FUNC <function_name>                        <*/
/*>                                                                     <*/
/*>      and added to the user defined functions search list, as in:    <*/
/*>                                                                     <*/
/*>         { <config_file_name>, <function_name>, <n_fields> },        <*/
/*>                                                                     <*/
/*>      where:                                                         <*/
/*>         <config_file_name> is the "wind_function" name in the       <*/
/*>                             config file                             <*/
/*>         <function_name> is the subroutine name for the user         <*/
/*>                             defined function (in this file)         <*/
/*>         <n_fields> is the number of fields required by the          <*/
/*>                             user defined function                   <*/
/*>                                                                     <*/
/*>     Note that example functions are declared here.                  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Define user defined wind functions for search list */
static	WINDFUNC_FUNC	winds_from_equations;
static	WINDFUNC_FUNC	winds_from_values;
static	WINDFUNC_FUNC	gradient_wind_function;

/* Initialize user defined wind function search list */
static	WINDFUNC_TABLE	UserWindFuncs[] =
	{
		{ "WindsFromEquations",    winds_from_equations,     1 },
		{ "WindsFromValues",       winds_from_values,        3 },
		{ "GradientFunction",      gradient_wind_function,   1 },
	};

/* Set number of user defined wind functions in search list */
static	int		NumUserWindFuncs =
	(int) (sizeof(UserWindFuncs) / sizeof(WINDFUNC_TABLE));

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A5. No changes required in identify_user_wind_function()            <*/
/*>      or in display_user_wind_functions()                            <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**********************************************************************
 ***                                                                ***
 *** i d e n t i f y _ u s e r _ w i n d _ f u n c t i o n          ***
 ***                                                                ***
 *** identify a user defined config file wind function name         ***
 ***                                                                ***
 **********************************************************************/

LOGICAL				identify_user_wind_function

	(
	STRING			name,		/* config file wind function name */
	WINDFUNC		*func,		/* pointer to function */
	int				*nreq		/* number of fields required by function */
	)

	{
	int				inum;

	/* Initialize return values */
	if ( NotNull(func) ) *func = NullWindFunc;
	if ( NotNull(nreq) ) *nreq = 0;

	/* Return FALSE if no wind function name passed */
	if ( blank(name) ) return FALSE;

	/* Search internal user defined wind functions */
	for ( inum=0; inum<NumUserWindFuncs; inum++ )
		{

		if ( same(name, UserWindFuncs[inum].name) )
			{
			if ( NotNull(func) ) *func = UserWindFuncs[inum].func;
			if ( NotNull(nreq) ) *nreq = UserWindFuncs[inum].nreq;
			return TRUE;
			}
		}

	/* Return FALSE if wind function name not found */
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** d i s p l a y _ u s e r _ w i n d _ f u n c t i o n s          ***
 ***                                                                ***
 *** display user defined wind function names for config files      ***
 ***                                                                ***
 **********************************************************************/

void				display_user_wind_functions

	(
	)

	{
	int				inum;

	/* Display all user defined wind functions */
	(void) printf(" User Defined Wind Functions");
	(void) printf(" ... from Config \"wind_function\" lines\n");
	for ( inum=0; inum<NumUserWindFuncs; inum++ )
		{
		(void) printf("  %2d   Wind Function Name:  %s\n",
				inum+1, UserWindFuncs[inum].name);
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (User Defined Functions)                 *
*                                                                      *
*     All the routines after this point are available only within      *
*      this file.                                                      *
*                                                                      *
***********************************************************************/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> The following two routines can be used as templates for extracting  <*/
/*>  winds from FPA fields, by using GENERIC equations or by accessing  <*/
/*>  user-supplied code.                                                <*/
/*> Note that the "winds_from_equations()" function is cross-referenced <*/
/*>  to the equations for "geostrophic" wind.                           <*/
/*> Note that the "winds_from_values()" function extracts information   <*/
/*>  to call a function "winds_from_function_call()" that requires      <*/
/*>  derivatives of msl pressure, surface temperature, and sea surface  <*/
/*>  temperature.                                                       <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**********************************************************************
 ***                                                                ***
 *** w i n d s _ f r o m _ e q u a t i o n                          ***
 ***                                                                ***
 *** determine wind speed and direction from a GENERIC equation     ***
 ***                                                                ***
 **********************************************************************/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> B1. GENERIC wind equation information (required by the function)    <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Global variables to hold GENERIC wind equation information */
static	LOGICAL					EqtnSet   = FALSE;
static	LOGICAL					EqtnValid = FALSE;
static	FpaConfigElementStruct	*ueqtn1   = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*veqtn1   = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*ueqtn2   = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*veqtn2   = NullPtr(FpaConfigElementStruct *);

static	LOGICAL		winds_from_equations

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							nn, ipos;
	float						wlat, wlon;
	double						degtorad, uval, vval, dang, sval;
	LOGICAL						Uvalid, Vvalid;
	STRING						Uunits, Vunits;
	FpaConfigElementStruct		*edef, *ucomp, *vcomp;
	STRING						Ueqtnunits, Veqtnunits;
	char						Ueqtnbuf[MAX_BCHRS], Veqtnbuf[MAX_BCHRS];

	/* Internal buffers for u/v component values extracted at each position */
	static	float		*Uvalues = NullFloat;
	static	float		*Vvalues = NullFloat;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B2. Error checking for data structures (no changes required)    <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[winds_from_equations] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B3. Data initialization (no changes required)                   <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B4. Set all the GENERIC equations that could be used by this    <*/
	/*>      function.  (The actual equations used will be set from one <*/
	/*>      of the following parameters!)                              <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Set pointers to GENERIC equations only once! */
	if ( !EqtnSet )
		{
		EqtnValid = TRUE;

		/* Generic equations for winds based on pressure */
		ueqtn1 = get_element_info(UEqtn1);
		if ( IsNull(ueqtn1) || IsNull(ueqtn1->elem_detail->equation)
				|| blank(ueqtn1->elem_detail->equation->eqtn)
				|| IsNull(ueqtn1->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[winds_from_equations] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", UEqtn1);
			EqtnValid = FALSE;
			}
		veqtn1 = get_element_info(VEqtn1);
		if ( IsNull(veqtn1) || IsNull(veqtn1->elem_detail->equation)
				|| blank(veqtn1->elem_detail->equation->eqtn)
				|| IsNull(veqtn1->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[winds_from_equations] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", VEqtn1);
			EqtnValid = FALSE;
			}

		/* Generic equations for winds based on height/thickness */
		ueqtn2 = get_element_info(UEqtn2);
		if ( IsNull(ueqtn2) || IsNull(ueqtn2->elem_detail->equation)
				|| blank(ueqtn2->elem_detail->equation->eqtn)
				|| IsNull(ueqtn2->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[winds_from_equations] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", UEqtn2);
			EqtnValid = FALSE;
			}
		veqtn2 = get_element_info(VEqtn2);
		if ( IsNull(veqtn2) || IsNull(veqtn2->elem_detail->equation)
				|| blank(veqtn2->elem_detail->equation->eqtn)
				|| IsNull(veqtn2->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[winds_from_equations] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", VEqtn2);
			EqtnValid = FALSE;
			}

		EqtnSet = TRUE;
		}

	/* Check for valid GENERIC equations */
	if ( !EqtnValid )
		{
		(void) fprintf(stderr, "[winds_from_equations] Error in GENERIC equations\n");
		return FALSE;
		}

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B5. Set any constants required by the wind calculations.        <*/
	/*>     The  get_winds_constant()  function sets the numerical      <*/
	/*>      value of a constant from the "Constants" block of the      <*/
	/*>      configuration files (in this case, the value of "RAD" in   <*/
	/*>      units of "MKS").                                           <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Get constant for conversion of degrees to radians */
	if ( !get_winds_constant(Rad, MKS, &degtorad) ) return FALSE;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B6. Construct the equations required by the wind calculations.  <*/
	/*>     The following section allows for usage of the same "type"   <*/
	/*>      of wind for more than one equation.  (In this case,        <*/
	/*>      "geostrophic" wind can be used to calculate wind from both <*/
	/*>      "Pressure" type fields as well as from "Height" or         <*/
	/*>      "Thickness" type fields.)                                  <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Ensure that detailed element information has been read */
	edef = get_element_info(fdescs[0].edef->name);
	if ( IsNull(edef) ) return FALSE;

	/* Set pointers to u and v components of wind based on wind class */
	switch ( edef->elem_detail->wd_class )
		{

		/* "Pressure" type fields */
		case FpaC_PRESSURE:
			ucomp = ueqtn1;
			vcomp = veqtn1;
			break;

		/* "Height" or "Thickness" type fields */
		case FpaC_HEIGHT:
		case FpaC_THICKNESS:
			ucomp = ueqtn2;
			vcomp = veqtn2;
			break;

		/* Default for all other type fields */
		default:
			(void) fprintf(stderr, "[winds_from_equations] Missing wind_class");
			(void) fprintf(stderr, " for element: \"%s\"\n",
					SafeStr(fdescs[0].edef->name));
			return FALSE;
		}

	/* Set units and construct equation for u component of winds */
	Ueqtnunits = ucomp->elem_detail->equation->units->name;
	(void) sprintf(Ueqtnbuf, "%s<@%s>", SafeStr(ucomp->name),
			SafeStr(fdescs[0].edef->name));

	/* Set units and construct equation for v component of winds */
	Veqtnunits = vcomp->elem_detail->equation->units->name;
	(void) sprintf(Veqtnbuf, "%s<@%s>", SafeStr(vcomp->name),
			SafeStr(fdescs[0].edef->name));

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B7. Evaluate the component winds.                               <*/
	/*>     The  extract_surface_value_by_equation()  function is used  <*/
	/*>      to evaluate u and v component winds using the constructed  <*/
	/*>      equations.                                                 <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Allocate space for extracted u/v component values */
	Uvalues = GETMEM(Uvalues, float, npos);
	Vvalues = GETMEM(Vvalues, float, npos);

	/* Extract u and v components of winds at all locations */
	Uvalid = extract_surface_value_by_equation(Ueqtnunits, Ueqtnbuf,
			&(fdescs[0]), matched, npos, ppos, clon, Uvalues, &Uunits);
	Vvalid = extract_surface_value_by_equation(Veqtnunits, Veqtnbuf,
			&(fdescs[0]), matched, npos, ppos, clon, Vvalues, &Vunits);

	/* Return FALSE if u or v components cannot be found */
	if ( !Uvalid || !Vvalid )
		{
		(void) fprintf(stderr, "[winds_from_equations] Cannot");
		(void) fprintf(stderr, " retrieve data for  \"%s\"  or  \"%s\"\n",
				Ueqtnbuf, Veqtnbuf);
		return FALSE;
		}

	/* Return FALSE if u or v components cannot be converted to m/s */
	if ( !convert_value(Uunits, 1.0, MperS, NullDouble)
			|| !convert_value(Vunits, 1.0, MperS, NullDouble) )
		{
		(void) fprintf(stderr, "[winds_from_equations] Error in units");
		(void) fprintf(stderr, " for u wind  \"%s\"  or v wind  \"%s\"\n",
				Uunits, Vunits);
		return FALSE;
		}

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B8. Calculate the winds from the u/v component winds.           <*/
	/*>     The  convert_value()  function converts the u/v component   <*/
	/*>      winds to units of m/s.                                     <*/
	/*>     The  pos_to_ll()  function sets the latitude and longitude  <*/
	/*>      for each position.                                         <*/
	/*>     The  wind_dir_true()  function converts wind direction from <*/
	/*>      map coordinates to coordinates on the Earth.               <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Convert u and v component winds to m/s */
		(void) convert_value(Uunits, (double) Uvalues[ipos], MperS, &uval);
		(void) convert_value(Vunits, (double) Vvalues[ipos], MperS, &vval);

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&(fdescs[0].mproj), ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		if ( NotNull(wdirs) )
			{
			dang = (fpa_atan2(vval, uval) / degtorad) + 180.0;
			wdirs[ipos] = wind_dir_true(&(fdescs[0].mproj), wlat, wlon,
											(float) dang);
			}
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			sval = hypot(uval, vval);
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) sval;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) sval;
				}
			}
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "winds_from_equations: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** w i n d s _ f r o m _ v a l u e s                              ***
 ***                                                                ***
 *** determine wind speed and direction from calculations using     ***
 ***  a function call (though only a stub function is used!)        ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		winds_from_values

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int				nn, ipos;
	float			wlat, wlon;
	float			xprs, yprs, tmp, sst, uval, vval;
	double			degtorad, dvalue, du, dv, dang, sval;
	char			Eqtnbuf[MAX_BCHRS];
	LOGICAL			Xvalid, Yvalid, Tvalid, Svalid;
	STRING			Xunits, Yunits, Tunits, Sunits;
	FLD_DESCRIPT	descript;

	/* Internal buffers for values extracted at each position */
	static	float		*Xvalues = NullFloat;
	static	float		*Yvalues = NullFloat;
	static	float		*Tvalues = NullFloat;
	static	float		*Svalues = NullFloat;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> C1. Error checking for data structures (no changes required)    <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Check for correct number of field descriptors */
	if ( nfds < 3 )
		{
		(void) fprintf(stderr, "[winds_from_values] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Three field descriptors required (for");
		(void) fprintf(stderr, " pressure, temperature, sea temperature)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> C2. Data initialization (no changes required)                   <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> C3. Set any constants required by the wind calculations.        <*/
	/*>     The  get_winds_constant()  function sets the numerical      <*/
	/*>      value of a constant from the "Constants" block of the      <*/
	/*>      configuration files (in this case, the value of "RAD" in   <*/
	/*>      units of "MKS").                                           <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Get constant for conversion of degrees to radians */
	if ( !get_winds_constant(Rad, MKS, &degtorad) ) return FALSE;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> C4. Evaluate the values required to calculate the winds.        <*/
	/*>     The  copy_fld_descript()  and  set_fld_descript()           <*/
	/*>      functions ensure that ordinary values will be extracted.   <*/
	/*>     The  extract_surface_value_by_equation()  function is used  <*/
	/*>      to evaluate the x and y derivatives of Msl Pressure, from  <*/
	/*>      the constructed equations.  (Note that this function       <*/
	/*>      returns the derivatives in MKS units of Pa/gridunits!)     <*/
	/*>     The  extract_surface_value()  function is used to evaluate  <*/
	/*>      Surface Temperature and Sea Surface Temperature.           <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Allocate space for extracted values */
	Xvalues = GETMEM(Xvalues, float, npos);
	Yvalues = GETMEM(Yvalues, float, npos);
	Tvalues = GETMEM(Tvalues, float, npos);
	Svalues = GETMEM(Svalues, float, npos);

	/* Extract x and y derivatives of mean sea level pressure,            */
	/*  surface temperature, and sea surface temperature at all locations */
	Xvalid = Yvalid = Tvalid = Svalid = FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{

		/* Initialize field descriptor for extracting values */
		(void) copy_fld_descript(&descript, &fdescs[nn]);
		(void) set_fld_descript(&descript,
						FpaF_VALUE_FUNCTION_NAME, FpaDefaultValueFunc,
						FpaF_END_OF_LIST);

		/* Extract x and y derivatives of mean sea level pressure */
		if ( equivalent_element_definitions(descript.edef->name, Pressure)
				&& equivalent_level_definitions(descript.ldef->name, Msl) )
			{

			/* Set equation for x derivative of pressure */
			(void) sprintf(Eqtnbuf, "ddx[%s<%s>]", Pressure, Msl);
			Xvalid = extract_surface_value_by_equation(MKS, Eqtnbuf,
					&descript, matched, npos, ppos, clon, Xvalues, &Xunits);

			/* Set equation for y derivative of pressure */
			(void) sprintf(Eqtnbuf, "ddy[%s<%s>]", Pressure, Msl);
			Yvalid = extract_surface_value_by_equation(MKS, Eqtnbuf,
					&descript, matched, npos, ppos, clon, Yvalues, &Yunits);
			}

		/* Extract surface temperature */
		else if ( equivalent_element_definitions(descript.edef->name,
																Temperature)
				&& equivalent_level_definitions(descript.ldef->name, Sfc) )
			{
			Tvalid = extract_surface_value(1, &descript, matched,
					npos, ppos, clon, Tvalues, &Tunits);
			}

		/* Extract sea surface temperature */
		else if ( equivalent_element_definitions(descript.edef->name,
																SeaTemperature)
				&& equivalent_level_definitions(descript.ldef->name, Sfc) )
			{
			Svalid = extract_surface_value(1, &descript, matched,
					npos, ppos, clon, Svalues, &Sunits);
			}

		/* Error if matching field not found */
		else
			{
			(void) fprintf(stderr, "[winds_from_values] Unrecognized field for");
			(void) fprintf(stderr, " wind: \"%s %s\"\n",
					SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
			return FALSE;
			}
		}

	/* Error if any required data not found */
	if ( !Xvalid || !Yvalid || !Tvalid || !Svalid )
		{
		if ( !Xvalid || !Yvalid )
			{
			(void) fprintf(stderr, "[winds_from_values] Missing data for x or y");
			(void) fprintf(stderr, " derivative of field: \"%s %s\"\n", Pressure, Msl);
			}
		if ( !Tvalid )
			{
			(void) fprintf(stderr, "[winds_from_values] Missing data for");
			(void) fprintf(stderr, " field: \"%s %s\"\n", Temperature, Sfc);
			}
		if ( !Svalid )
			{
			(void) fprintf(stderr, "[winds_from_values] Missing data for");
			(void) fprintf(stderr, " field: \"%s %s\"\n", SeaTemperature, Sfc);
			}
		return FALSE;
		}

	/* Return FALSE if x or y derivatives cannot be converted to mb */
	if ( !convert_value(Xunits, 1.0, Mb, NullDouble)
			|| !convert_value(Yunits, 1.0, Mb, NullDouble) )
		{
		(void) fprintf(stderr, "[winds_from_values] Error in units");
		(void) fprintf(stderr, " for x derivative  \"%s\"  or y derivative  \"%s\"\n",
				Xunits, Yunits);
		return FALSE;
		}

	/* Return FALSE if temperature or sea surface temperature */
	/*  cannot be converted to degrees C                      */
	if ( !convert_value(Tunits, 1.0, DegC, NullDouble)
			|| !convert_value(Sunits, 1.0, DegC, NullDouble) )
		{
		(void) fprintf(stderr, "[winds_from_values] Error in units");
		(void) fprintf(stderr, " for temperature  \"%s\"  or sea sfc temp  \"%s\"\n",
				Tunits, Sunits);
		return FALSE;
		}

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> C5. Calculate the winds from the data values extracted.         <*/
	/*>     The  convert_value()  function converts the data values to  <*/
	/*>      the units required by the function call.                   <*/
	/*>     The  pos_to_ll()  function sets the latitude and longitude  <*/
	/*>      for each position.                                         <*/
	/*>     The  wind_dir_true()  function converts wind direction from <*/
	/*>      map coordinates to coordinates on the Earth.               <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Convert all data to units used in function call             */
		/*  ... convert pressure derivatives from Pa/gridunits to mb/m */
		(void) convert_value(Xunits, (double) Xvalues[ipos], Mb, &dvalue);
		xprs = (float) dvalue / descript.mproj.grid.units;
		(void) convert_value(Yunits, (double) Yvalues[ipos], Mb, &dvalue);
		yprs = (float) dvalue / descript.mproj.grid.units;
		/*  ... convert air temperature to degC */
		(void) convert_value(Tunits, (double) Tvalues[ipos], DegC, &dvalue);
		tmp = (float) dvalue;
		/*  ... convert sea temperature to degC */
		(void) convert_value(Sunits, (double) Svalues[ipos], DegC, &dvalue);
		sst = (float) dvalue;

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&(descript.mproj), ppos[ipos], &wlat, &wlon) )
			continue;

		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
		/*>                                                             <*/
		/*> C6. Here is where the function call goes, called by value.  <*/
		/*>     For example, a FORTRAN function might be called by:     <*/
		/*>                                                             <*/
		/*>      (void) wind(&wlat, &wlon, &xprs, &yprs, &tmp, &sst,    <*/
		/*>                    &uval, &vval);                           <*/
		/*>                                                             <*/
		/*>     For this example, the stub function call returns both   <*/
		/*>      uval and vval in knots!                                <*/
		/*>                                                             <*/
		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

		/* Call function to return uval and vval (in knots) */
		(void) winds_from_function_call(&wlat, &wlon, &xprs, &yprs, &tmp, &sst,
				&uval, &vval);

		/* Convert all data returned by function to MKS units */
		(void) convert_value("knots", (double) uval, MKS, &du);
		(void) convert_value("knots", (double) vval, MKS, &dv);

		/* Set wind speed and direction */
		if ( NotNull(wdirs) )
			{
			dang = (fpa_atan2(dv, du) / degtorad) + 180.0;
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
											(float) dang);
			}
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			sval = hypot(du, dv);
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) sval;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) sval;
				}
			}
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "winds_from_values: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** g r a d i e n t _ w i n d _ f u n c t i o n                    ***
 ***                                                                ***
 *** determine gradient wind speed and direction from the mean sea  ***
 ***  level pressure field, and its gradients and curvature         ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		gradient_wind_function

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int				nn, ipos;
	LOGICAL			valid;
	double			corparm, rhostd, grav, fact;
	float			wlat, wlon, mpu;
	double			dpdx, dpdy, curv;
	double			corio, limcor, grad, radical, spd, dang;
	SURFACE			psfc = NullSfc;
	USPEC			*punits;
	FLD_DESCRIPT	pdesc;

	static double	limlat  = 15.0;
	static double	limcurv = 1.0e-8;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[gradient_wind_function] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   One field descriptor required");
		(void) fprintf(stderr, " (for pressure data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Get necessary constants */
	if ( !get_winds_constant(Rho,  MKS, &rhostd) )  return FALSE;
	if ( !get_winds_constant(Grav, MKS, &grav) )    return FALSE;
	if ( !get_winds_constant(Cor,  MKS, &corparm) ) return FALSE;

	/* Get SURFACE Object for mean sea level pressure,             */
	/* Note that valid time for each field is reset (if requested) */
	for ( nn=0; nn<nfds; nn++ )
		{

		/* Get SURFACE Object for mean sea level pressure */
		if ( equivalent_element_definitions(fdescs[nn].edef->name, Pressure)
				&& equivalent_level_definitions(fdescs[nn].ldef->name, Msl) )
			{
			(void) copy_fld_descript(&pdesc, &fdescs[nn]);
			if ( matched )
				{
				(void) matched_source_valid_time_reset(&pdesc,
						FpaC_TIMEDEP_ANY, pdesc.vtime);
				}
			psfc = retrieve_surface(&pdesc);
			fact = 1/rhostd;
			}

		/* Otherwise get SURFACE Object for height field */
		else if ( equivalent_element_definitions(fdescs[nn].edef->name, Height)
				)
			{
			(void) copy_fld_descript(&pdesc, &fdescs[nn]);
			if ( matched )
				{
				(void) matched_source_valid_time_reset(&pdesc,
						FpaC_TIMEDEP_ANY, pdesc.vtime);
				}
			psfc = retrieve_surface(&pdesc);
			fact = grav;
			}

		/* Error if matching field not found */
		else
			{
			(void) fprintf(stderr, "[gradient_wind_function] Unrecognized");
			(void) fprintf(stderr, " field for gradient wind: \"%s %s\"\n",
					SafeStr(fdescs[nn].edef->name),
					SafeStr(fdescs[nn].ldef->name));
			if ( NotNull(psfc) ) psfc = destroy_surface(psfc);
			return FALSE;
			}
		}

	/* Error if any required data not found */
	if ( IsNull(psfc) )
		{
		(void) fprintf(stderr, "[gradient_wind_function] Missing data for");
		(void) fprintf(stderr, " field: \"%s %s\"\n", Pressure, Msl);
		return FALSE;
		}

	/* Get map units for conversion to metres */
	mpu    = pdesc.mproj.definition.units;
	punits = &(psfc->units);

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Skip any location outside bounds of data */
		if ( !inside_surface_spline(psfc, ppos[ipos]) )
			continue;

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&pdesc.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Evaluate pressure/height gradients at given location */
		/* Return FALSE if problems with eval_sfc... */
		valid = eval_sfc_1st_deriv(psfc, ppos[ipos], &dpdx, &dpdy);
		if ( !valid )
			{
			(void) fprintf(stderr, "[gradient_wind_function] Problem");
			(void) fprintf(stderr, " evaluating pressure/height gradients");
			(void) fprintf(stderr, " with eval_sfc_1st_deriv\n");
			psfc = destroy_surface(psfc);
			return FALSE;
			}

		/* Calculate the wind speed */
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			/* Evaluate curvature of pressure/height field at given location */
			/* Return FALSE if problems with eval_sfc... */
			valid = eval_sfc_curvature(psfc, ppos[ipos], &curv, NullPoint);
			if ( !valid )
				{
				(void) fprintf(stderr, "[gradient_wind_function] Problem");
				(void) fprintf(stderr, " evaluating pressure/height curvature");
				(void) fprintf(stderr, " with eval_sfc_curvature\n");
				psfc = destroy_surface(psfc);
				return FALSE;
				}

			/* Convert gradients and curvature to MKS (Pa/m, m/m and 1/m) */
			grad  = hypot(dpdx, dpdy);
			grad  = convert_by_uspec(&MKS_UNITS, punits, grad) / mpu;
			curv /= mpu;

			/* The following "cross-isobaric" correction estimates the */
			/* gradient and curvature of the actual flow and ensures that */
			/* the critical curvature is never reached */
#			ifdef VR_CROSS_ISO
			angle = 0;
			if (curv < 0)
				{
				/* Critical curvature is the point beyond which the */
				/* gradient wind equation no longer has a solution */
				curvx = -corio*corio/4/grad/fact;
				curvf = curvx*(1 - exp(-a*curv/curvx));
				cang  = curvf/curv;
				angle = fpa_acosdeg(cang);
				grad *= cang;
				curv  = curvf;
				}
#			endif /* VR_CROSS_ISO */

			/* Calculate other parameters needed for the equation */
			corio = corparm * fpa_sindeg(wlat);

			/* Limiting the latitude in the Coriolis parameter prevents an */
			/* infinite wind at the equator (only in the super-critical case) */
			if ( fabs(wlat) < limlat )
				{
				limcor = corparm * fpa_sindeg(limlat*SIGN(wlat));
				}
			else
				{
				limcor = corio;
				}

			/* Adjust gradient and curvature in near-critical regions */
			/* (positive curvature and significant gradient) */

			/* Determine gradient winds from given data */
			/* Approximate by geostrophic if curvature is near zero */
			/* Cap at twice geostrophic if super-critical */
			radical = corio*corio/4 + grad*curv*fact;
			if ( fabs(curv) < limcurv )
				{
				spd = grad*fact/fabs(limcor);
				}
			else if ( radical <= 0 )
				{
				spd = 2*grad*fact/fabs(limcor);
				}
			else
				{
				spd = (-fabs(corio)/2 + sqrt(radical)) / curv;
				}

			/* Set the gradient winds and gusts */
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) spd;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) spd;
				}
			}

		/* Calculate the wind direction (degrees True) */
		if ( NotNull(wdirs) )
			{
			if (wlat >= 0) dang = fpa_atan2deg(dpdx, -dpdy) + 180.0;
			else           dang = fpa_atan2deg(-dpdx, dpdy) + 180.0;
			wdirs[ipos] = wind_dir_true(&(pdesc.mproj), wlat, wlon,
								(float) dang);
			}
		}

	/* Free space used by SURFACE Objects */
	psfc = destroy_surface(psfc);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "gradient_wind_function: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}


/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (User Defined Routines)                 *
*                                                                      *
*     All the routines after this point are available only within      *
*      this file.                                                      *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** w i n d s _ f r o m _ f u n c t i o n _ c a l l                ***
 ***                                                                ***
 *** stub function to return u and v components of wind using       ***
 ***  x and y derivatives of pressure, surface temperature, and     ***
 ***  sea surface temperature                                       ***
 ***                                                                ***
 **********************************************************************/

static	void		winds_from_function_call

	(
	float			*wlat,		/* latitude (in degrees) */
	float			*wlon,		/* longitude (in degrees) */
	float			*xprs,		/* x derivative of msl pressure (in mb/m) */
	float			*yprs,		/* y derivative of msl pressure (in mb/m) */
	float			*tmp,		/* surface temperature (in degrees C) */
	float			*sst,		/* sea surface temperature (in degrees C) */
	float			*uval,		/* u component of wind (in knots) - returned */
	float			*vval		/* v component of wind (in knots) - returned */
	)

	{

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> Note that this is merely a stub function ...                    <*/
	/*>  uval and vval are arbitrarily set and returned!                <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	*uval = *wlat / 10.0;
	*vval = *wlon / 100.0;

	}


/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#if defined USER_WINDS_STANDALONE

/**********************************************************************
 *** routine to test extract_awind                                  ***
 **********************************************************************/

static	void		test_extract_awind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* input positions on field */
	float			clon		/* center longitude for fields */
	)

	{
	int			nn;
	float		*wspds, *wgsts, *wdirs;
	STRING		units;

	/* Return if no information in field descriptors */
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) ) return;
		}

	(void) fprintf(stdout, "\n   Source: %s %s",
			fdescs[0].sdef->name, fdescs[0].subdef->name);
	(void) fprintf(stdout, "   Runtime: %s   Validtime: %s\n",
			fdescs[0].rtime, fdescs[0].vtime);
	(void) fprintf(stdout, "   Function name: %s", fdescs[0].wind_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);

	wspds = INITMEM(float, npos);
	wgsts = INITMEM(float, npos);
	wdirs = INITMEM(float, npos);

	if ( extract_awind(nfds, fdescs, matched, npos, ppos, clon,
			wdirs, wspds, wgsts, &units) )
		{
		(void) fprintf(stdout, "\n");
		for ( nn=0; nn<npos; nn++ )
			{
			(void) fprintf(stdout, "  Point: %7.1f  %7.1f",
					ppos[nn][X], ppos[nn][Y]);
			(void) fprintf(stdout, "    Speed/Gust: %7.2f  %7.2f  %s",
					wspds[nn], wgsts[nn], units);
			(void) fprintf(stdout, "   Direction (degrees): %7.2f\n",
					wdirs[nn]);
			}
		}

	else
		{
		(void) fprintf(stdout, "\n  Error in call to extract_awind\n");
		}

	FREEMEM(wspds);
	FREEMEM(wgsts);
	FREEMEM(wdirs);
	}

/**********************************************************************
 *** routine to test check_extract_wind                            ***
 **********************************************************************/

static	void		test_check_extract_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos		/* input positions on field */
	)

	{
	int			nn;

	/* Return if no information in field descriptors */
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) ) return;
		}

	(void) fprintf(stdout, "\n   Source: %s %s",
			fdescs[0].sdef->name, fdescs[0].subdef->name);
	(void) fprintf(stdout, "   Runtime: %s   Validtime: %s\n",
			fdescs[0].rtime, fdescs[0].vtime);
	(void) fprintf(stdout, "   Function name: %s", fdescs[0].wind_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);

	if ( check_extract_wind(nfds, fdescs, matched, npos, ppos) )
		{
		(void) fprintf(stdout, "\n   Winds can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, "\n   Missing data for winds\n");
		}
	}

/**********************************************************************
 *** main routine to test all static routines                       ***
 **********************************************************************/

int		main

(
)

{
int				nsetup;
STRING			setupfile, *setuplist, path;
MAP_PROJ		*mproj;
int				Inumx, Inumy, Npos, iix, iiy;
POINT			**Apstns, *Ppos;
float			Clon;
STRING			source, subsource, rtime, vtime;
STRING			windfunc, element, level;
FLD_DESCRIPT	fdescs[3];

/* Set Defaults for USER_WINDS_STANDALONE */

/* ... First set the default output units */
(void) setvbuf(stdout,	NullString,	_IOLBF,	0);
(void) setvbuf(stderr,	NullString,	_IOLBF,	0);

/* ... Next get a license */
(void) fpalib_license(FpaAccessRead);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D1. Set local setup file for testing                                <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

setupfile = "natwave";

(void) fprintf(stdout, "Setup File: %s\n", setupfile);
nsetup = setup_files(setupfile, &setuplist);
if ( !define_setup(nsetup, setuplist) )
	{
	(void) fprintf(stderr, "Fatal problem with Setup File: %s\n", setupfile);
	return -1;
	}

/* ... Next set Default Map Projection, positions for projection, */
/*      and center longitude for projection                       */
mproj = get_target_map();
if ( IsNull(mproj)
		|| !grid_positions(mproj, &Inumx, &Inumy, NullFloat,
				&Apstns, NullPtr(float ***), NullPtr(float ***))
		|| !grid_center(mproj, NullPointPtr, NullFloat, &Clon) )
	{
	(void) fprintf(stderr, "Fatal problem with Default Map Projection\n");
	return -1;
	}

/* Set positions at all locations */
Npos = Inumx * Inumy;
Ppos = INITMEM(POINT, Npos);
for ( iiy=0; iiy<Inumy; iiy++ )
	for ( iix=0; iix<Inumx; iix++ )
		{
		Ppos[iiy*Inumx + iix][X] = Apstns[iiy][iix][X];
		Ppos[iiy*Inumx + iix][Y] = Apstns[iiy][iix][Y];
		}

fprintf(stdout, "\n\nBasemap  olat: %f", mproj->definition.olat);
fprintf(stdout, "  olon: %f", mproj->definition.olon);
fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
fprintf(stdout, "         xorg: %f", mproj->definition.xorg);
fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
fprintf(stdout, "         xlen: %f", mproj->definition.xlen);
fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
fprintf(stdout, "  units: %f\n", mproj->definition.units);

fprintf(stdout, "\nGrid definition  nx: %d", mproj->grid.nx);
fprintf(stdout, "  ny: %d", mproj->grid.ny);
fprintf(stdout, "  gridlen: %f", mproj->grid.gridlen);
fprintf(stdout, "  units: %f\n", mproj->grid.units);

/* ... Next set Default Pathname (set to the Home directory) */
path = home_directory();

/* Initialize parameters for testing user-defined winds routines */
(void) clear_equation_database();
(void) init_fld_descript(&(fdescs[0]));


/* Testing for extract_awind() */
(void) fprintf(stdout, "\n\n *** Testing for extract_awind() ***\n");

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D2. Set source and times based on data in your directories          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

source   = "depict";	subsource = "";
rtime    = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
vtime    = strdup(build_tstamp(1992, 329, 12, 00, FALSE, FALSE));

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D3. Set wind function name <config_file_name> (from config files)   <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

windfunc = "WindsFromEquations";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D4. Set element(s) and level(s) required by <function_name>, load   <*/
/*>      parameters into field descriptor, and run the test routine     <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

element  = "pressure";
level    = "msl";

(void) set_fld_descript(&(fdescs[0]), FpaF_DIRECTORY_PATH, path,
						FpaF_MAP_PROJECTION, mproj,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);

(void) test_extract_awind(1, fdescs, TRUE, Npos, Ppos, Clon);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D5. Set wind function name <config_file_name> (from config files)   <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

windfunc = "WindsFromValues";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D6. Set element(s) and level(s) required by <function_name>, load   <*/
/*>      parameters into field descriptor, and run the test routine     <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

element  = "pressure";
level    = "msl";

(void) set_fld_descript(&(fdescs[0]),
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);

element  = "temperature";
level    = "surface";

(void) copy_fld_descript(&(fdescs[1]), &(fdescs[0]));
(void) set_fld_descript(&(fdescs[1]),
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);

element  = "sea_temperature";
level    = "surface";

(void) copy_fld_descript(&(fdescs[2]), &(fdescs[0]));
(void) set_fld_descript(&(fdescs[2]),
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);

(void) test_extract_awind(3, fdescs, TRUE, Npos, Ppos, Clon);


/* Testing for check_extract_wind() */
(void) fprintf(stdout, "\n\n *** Testing for check_extract_wind() ***\n");

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D7. Set source and times based on data in your directories          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

source   = "depict";	subsource = "";
rtime    = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
vtime    = strdup(build_tstamp(1992, 330, 12, 00, FALSE, FALSE));

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D8. Set wind function name <config_file_name> (from config files)   <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

windfunc = "WindsFromEquations";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> D9. Set element(s) and level(s) required by <function_name>, load   <*/
/*>      parameters into field descriptor, and run the test routine     <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

element  = "pressure";
level    = "msl";

(void) set_fld_descript(&(fdescs[0]), FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_MAP_PROJECTION, mproj,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_wind(1, fdescs, TRUE, Npos, Ppos);

/* Testing for ... */

return 0;
}

#endif /* USER_WINDS_STANDALONE */
