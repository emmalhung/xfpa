/*****************************************************************************
 ***                                                                       ***
 ***  u s e r _ v a l u e s . c                                            ***
 ***                                                                       ***
 ***  This module is designed to access user defined routines to extract   ***
 ***  values from fields of meteorological data.                           ***
 ***                                                                       ***
 ***  One example routine is given.                                        ***
 ***  The routine "values_from_values()" gives an example of data          ***
 ***  calculated by passing values to a function that the user would       ***
 ***  supply (given here by "values_from_function_call()").                ***
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

/* FPA library definitions */
#include <fpa.h>

/* Standard library definitions */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>


/* Interface functions                             */
/*  ... these are defined in /lib/extract/values.h */


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
static	void		values_from_function_call(float *, float *, float *,
								float *, float *, float *, float *);


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A2. Elements/Levels/Constants/Units cross-referenced in             <*/
/*>      configuration files are added here, as in:                     <*/
/*>                                                                     <*/
/*>         static const STRING AirTemp = "temperature";                <*/
/*>                                                                     <*/
/*>     Note that parameters required by the example functions are set  <*/
/*>      here.                                                          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Elements cross-referenced in configuration file */
static	const	STRING	Pressure = "pressure";
static	const	STRING	AirTemp  = "temperature";
static	const	STRING	SeaTemp  = "sea_temperature";

/* Levels cross-referenced in configuration file */
static	const	STRING	Msl      = "msl";
static	const	STRING	Sfc      = "surface";

/* Constants cross-referenced in configuration file */
static	const	STRING	Rad      = "RAD";

/* Units cross-referenced in configuration file */
static	const	STRING	MKS      = "MKS";
static	const	STRING	Mb       = "mb";
static	const	STRING	DegC     = "degreesC";


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A3. Prototypes for user defined functions are added here, as in:    <*/
/*>                                                                     <*/
/*>         static VALUEFUNC_FUNC <function_name>                       <*/
/*>                                                                     <*/
/*>      and added to the user defined functions search list, as in:    <*/
/*>                                                                     <*/
/*>         { <config_file_name>, <function_name>, <n_fields> },        <*/
/*>                                                                     <*/
/*>      where:                                                         <*/
/*>         <config_file_name> is the "value_function" name in the      <*/
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

/* Define user defined value functions for search list */
static	VALUEFUNC_FUNC	values_from_values;

/* Initialize user defined value function search list */
static	VALUEFUNC_TABLE	UserValueFuncs[] =
	{
		{ "ValuesFromValues",    values_from_values,     3 },
	};

/* Set number of user defined value functions in search list */
static	int		NumUserValueFuncs =
	(int) (sizeof(UserValueFuncs) / sizeof(VALUEFUNC_TABLE));


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A4. No changes required in identify_user_value_function()           <*/
/*>      or in display_user_value_functions()                           <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**********************************************************************
 ***                                                                ***
 *** i d e n t i f y _ u s e r _ v a l u e _ f u n c t i o n        ***
 ***                                                                ***
 *** identify a user defined config file value function name        ***
 ***                                                                ***
 **********************************************************************/

LOGICAL				identify_user_value_function

	(
	STRING			name,		/* config file value function name */
	VALUEFUNC		*func,		/* pointer to function */
	int				*nreq		/* number of fields required by function */
	)

	{
	int				inum;

	/* Initialize return values */
	if ( NotNull(func) ) *func = NullValueFunc;
	if ( NotNull(nreq) ) *nreq = 0;

	/* Return FALSE if no value function name passed */
	if ( blank(name) ) return FALSE;

	/* Search internal user defined value functions */
	for ( inum=0; inum<NumUserValueFuncs; inum++ )
		{

		if ( same(name, UserValueFuncs[inum].name) )
			{
			if ( NotNull(func) ) *func = UserValueFuncs[inum].func;
			if ( NotNull(nreq) ) *nreq = UserValueFuncs[inum].nreq;
			return TRUE;
			}
		}

	/* Return FALSE if value function name not found */
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** d i s p l a y _ u s e r _ v a l u e _ f u n c t i o n s        ***
 ***                                                                ***
 *** display user defined value function names for config files     ***
 ***                                                                ***
 **********************************************************************/

void				display_user_value_functions

	(
	)

	{
	int				inum;

	/* Display all user defined value functions */
	(void) printf(" User Defined Value Functions");
	(void) printf(" ... from Config \"value_function\" lines\n");
	for ( inum=0; inum<NumUserValueFuncs; inum++ )
		{
		(void) printf("  %2d   Value Function Name:  %s\n",
				inum+1, UserValueFuncs[inum].name);
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
/*> The following routine can be used as a template for extracting      <*/
/*>  values from FPA fields by accessing user supplied code.            <*/
/*> Note that the "values_from_values()" function extracts information  <*/
/*>  to call a function "values_from_function_call()" that requires     <*/
/*>  derivatives of msl pressure, surface temperature, and sea surface  <*/
/*>  temperature.                                                       <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**********************************************************************
 ***                                                                ***
 *** v a l u e s _ f r o m _ v a l u e s                            ***
 ***                                                                ***
 *** determine values from calculations using a function call       ***
 ***  (though only a stub function is used!)                        ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		values_from_values

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int				nn, ipos;
	float			wlat, wlon;
	float			xprs, yprs, tmp, sst, val;
	double			degtorad, dvalue;
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
	/*> B1. Error checking for <n_fields> data structures               <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Check for correct number of field descriptors */
	if ( nfds < 3 )
		{
		(void) fprintf(stderr, "[values_from_values] Error in");
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

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B2. Data initialization (no changes required)                   <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(values) ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B3. Set any constants required by the value calculations.       <*/
	/*>     The  get_values_constant()  function sets the numerical     <*/
	/*>      value of a constant from the "Constants" block of the      <*/
	/*>      configuration files (in this case, the value of "RAD" in   <*/
	/*>      units of "MKS").                                           <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Get constant for conversion of degrees to radians */
	if ( !get_values_constant(Rad, MKS, &degtorad) ) return FALSE;

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B4. Evaluate the values required to call the values function.   <*/
	/*>     The  copy_fld_descript()  and  set_fld_descript()           <*/
	/*>      functions ensure that ordinary values will be extracted.   <*/
	/*>     The  extract_surface_value_by_equation()  function is used  <*/
	/*>      to evaluate the x and y derivatives of Msl Pressure, from  <*/
	/*>      the constructed equations.  (Note that this function       <*/
	/*>      returns the derivatives in MKS units of Pa/gridunits!)     <*/
	/*>     The  extract_surface_value()  function is used to evaluate  <*/
	/*>      Surface Temperature and Sea Surface Temperature.           <*/
	/*>     Note that the order of the fields passed to the routine     <*/
	/*>      is arbitrary.                                              <*/
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
		else if ( equivalent_element_definitions(descript.edef->name, AirTemp)
				&& equivalent_level_definitions(descript.ldef->name, Sfc) )
			{
			Tvalid = extract_surface_value(1, &descript, matched,
					npos, ppos, clon, Tvalues, &Tunits);
			}

		/* Extract sea surface temperature */
		else if ( equivalent_element_definitions(descript.edef->name, SeaTemp)
				&& equivalent_level_definitions(descript.ldef->name, Sfc) )
			{
			Svalid = extract_surface_value(1, &descript, matched,
					npos, ppos, clon, Svalues, &Sunits);
			}

		/* Warning if matching field not found */
		else
			{
			(void) fprintf(stderr, "[values_from_values] Unrecognized field for");
			(void) fprintf(stderr, " value: \"%s %s\"\n",
					SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
			}
		}

	/* Error if any required data not found */
	if ( !Xvalid || !Yvalid || !Tvalid || !Svalid )
		{
		if ( !Xvalid || !Yvalid )
			{
			(void) fprintf(stderr, "[values_from_values] Missing data for");
			(void) fprintf(stderr, " x or y derivative of");
			(void) fprintf(stderr, " field: \"%s %s\"\n", Pressure, Msl);
			}
		if ( !Tvalid )
			{
			(void) fprintf(stderr, "[values_from_values] Missing data for");
			(void) fprintf(stderr, " field: \"%s %s\"\n", AirTemp, Sfc);
			}
		if ( !Svalid )
			{
			(void) fprintf(stderr, "[values_from_values] Missing data for");
			(void) fprintf(stderr, " field: \"%s %s\"\n", SeaTemp, Sfc);
			}
		return FALSE;
		}

	/* Return FALSE if x or y derivatives cannot be converted to mb */
	if ( !convert_value(Xunits, 1.0, Mb, NullDouble)
			|| !convert_value(Yunits, 1.0, Mb, NullDouble) )
		{
		(void) fprintf(stderr, "[values_from_values] Error in units");
		(void) fprintf(stderr, " for x derivative  \"%s\"", Xunits);
		(void) fprintf(stderr, "  or y derivative  \"%s\"\n", Yunits);
		return FALSE;
		}

	/* Return FALSE if temperature or sea surface temperature */
	/*  cannot be converted to degrees C                      */
	if ( !convert_value(Tunits, 1.0, DegC, NullDouble)
			|| !convert_value(Sunits, 1.0, DegC, NullDouble) )
		{
		(void) fprintf(stderr, "[values_from_values] Error in units");
		(void) fprintf(stderr, " for temperature  \"%s\"", Tunits);
		(void) fprintf(stderr, "  or sea sfc temp  \"%s\"\n", Sunits);
		return FALSE;
		}

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> B5. Calculate the values from the data values extracted.        <*/
	/*>     The  convert_value()  function converts the data values to  <*/
	/*>      the units required by the function call.                   <*/
	/*>     The  pos_to_ll()  function sets the latitude and longitude  <*/
	/*>      for each position.                                         <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	/* Loop through all positions and extract value parameters */
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
		/*> B6. Here is where the function call goes, called by value.  <*/
		/*>     For example, a FORTRAN function might be called by:     <*/
		/*>                                                             <*/
		/*>      (void) value(&wlat, &wlon, &xprs, &yprs, &tmp, &sst,   <*/
		/*>                    &val);                                   <*/
		/*>                                                             <*/
		/*>     For this example, the stub function call returns val    <*/
		/*>      in mb!                                                 <*/
		/*>                                                             <*/
		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
		/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

		/* Call function to return val (in mb) */
		(void) values_from_function_call(&wlat, &wlon, &xprs, &yprs, &tmp, &sst,
				&val);

		/* Convert all data returned by function to MKS units */
		(void) convert_value("mb", (double) val, MKS, &dvalue);

		/* Set return parameters */
		if ( NotNull(values) ) values[ipos] = (float) dvalue;
		}

	/* Set units to MKS and return TRUE if all went well */
	if ( NotNull(units) ) *units = MKS;
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
 *** v a l u e s _ f r o m _ f u n c t i o n _ c a l l              ***
 ***                                                                ***
 *** stub function to a value calculated from  x and y derivatives  ***
 ***  of pressure, surface temperature, and sea surface temperature ***
 ***                                                                ***
 **********************************************************************/

static	void		values_from_function_call

	(
	float			*wlat,		/* latitude (in degrees) */
	float			*wlon,		/* longitude (in degrees) */
	float			*xprs,		/* x derivative of msl pressure (in mb/m) */
	float			*yprs,		/* y derivative of msl pressure (in mb/m) */
	float			*tmp,		/* surface temperature (in degrees C) */
	float			*sst,		/* sea surface temperature (in degrees C) */
	float			*val		/* value (in mb) - returned */
	)

	{

	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>                                                                 <*/
	/*> Note that this is merely a stub function ...                    <*/
	/*>  val is arbitrarily set and returned!                           <*/
	/*>                                                                 <*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
	/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

	*val = 1000.0 + ( *tmp - *sst );

	}


/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#if defined USER_VALUES_STANDALONE

/**********************************************************************
 *** routine to test extract_surface_value                          ***
 **********************************************************************/

static	void		test_extract_surface_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* input positions on fields */
	float			clon		/* center longitude for fields */
	)

	{
	int			nn;
	float		*vals;
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
	(void) fprintf(stdout, "   Function name: %s", fdescs[0].value_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);

	vals = INITMEM(float, npos);

	if ( extract_surface_value(nfds, fdescs, matched, npos, ppos, clon,
			vals, &units) )
		{
		(void) fprintf(stdout, "\n");
		for ( nn=0; nn<npos; nn++ )
			{
			(void) fprintf(stdout, "  Point: %7.1f  %7.1f",
					ppos[nn][X], ppos[nn][Y]);
			(void) fprintf(stdout, "    Value: %f  %s\n", vals[nn], units);
			}
		}

	else
		{
		(void) fprintf(stdout, "\n  Error in call to extract_surface_value\n");
		}

	FREEMEM(vals);
	}

/**********************************************************************
 *** routine to test check_extract_value                            ***
 **********************************************************************/

static	void		test_check_extract_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos		/* input positions on fields */
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
	(void) fprintf(stdout, "   Function name: %s", fdescs[0].value_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);

	if ( check_extract_value(nfds, fdescs, matched, npos, ppos) )
		{
		(void) fprintf(stdout, "\n   Values can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, "\n   Missing data for values\n");
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
STRING			valuefunc, element, level;
FLD_DESCRIPT	fdescs[3];

/* Set Defaults for USER_VALUES_STANDALONE */

/* ... First set the default output units */
(void) setvbuf(stdout,	NullString,	_IOLBF,	0);
(void) setvbuf(stderr,	NullString,	_IOLBF,	0);

/* ... Next get a license */
(void) fpalib_license(FpaAccessRead);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C1. Set local setup file for testing                                <*/
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

/* Initialize parameters for testing user defined values routines */
(void) clear_equation_database();
(void) init_fld_descript(&(fdescs[0]));


/* Testing for extract_surface_value() */
(void) fprintf(stdout, "\n\n *** Testing for extract_surface_value() ***\n");

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C2. Set source and times based on data in your directories          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

source    = "depict";	subsource = "";
rtime     = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
vtime     = strdup(build_tstamp(1992, 329, 12, 00, FALSE, FALSE));

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C3. Set value function name <config_file_name> (from config files)  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

valuefunc = "ValuesFromValues";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C4. Set element(s) and level(s) required by <function_name>, load   <*/
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
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
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

(void) test_extract_surface_value(3, fdescs, TRUE, Npos, Ppos, Clon);


/* Testing for check_extract_value() */
(void) fprintf(stdout, "\n\n *** Testing for check_extract_value() ***\n");

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C5. Set source and times based on data in your directories          <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

source    = "depict";	subsource = "";
rtime     = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
vtime     = strdup(build_tstamp(1992, 330, 12, 00, FALSE, FALSE));

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C6. Set value function name <config_file_name> (from config files)  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

valuefunc = "ValuesFromValues";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> C7. Set element(s) and level(s) required by <function_name>, load   <*/
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
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
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

(void) test_check_extract_value(3, fdescs, TRUE, Npos, Ppos);

/* Testing for ... */

return 0;
}

#endif /* USER_VALUES_STANDALONE */
