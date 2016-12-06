/*****************************************************************************
 ***                                                                       ***
 ***  u s e r _ c o n f i r m . c                                          ***
 ***                                                                       ***
 ***  This module is designed to confirm that the correct user defined     ***
 ***  library is being accessed.                                           ***
 ***                                                                       ***
 ***  Version 5 (c) Copyright 2000 Environment Canada (MSC)                ***
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


/* Interface functions                            */
/*  ... these are defined in /lib/environ/read_setup.h */


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A1. Set a message to identify that functions in this library are    <*/
/*>     indeed being invoked.  Be creative!                             <*/
/*>                                                                     <*/
/*>     Note that userlib_verify() is always accessed.                  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

static	STRING	UserLibMessage
					= "The local user-defined library is connected!";

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A2. Set flags for displaying rules, value functions, and wind       <*/
/*>     functions.  Set to "FALSE" if display not required!             <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

static	LOGICAL	DisplayRules  = TRUE;
static	LOGICAL	DisplayValues = TRUE;
static	LOGICAL	DisplayWinds  = TRUE;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A3. No changes required in userlib_verify()                         <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/***********************************************************************
*                                                                      *
*  u s e r l i b _ v e r i f y                                         *
*                                                                      *
***********************************************************************/

void	userlib_verify (void)

	{
	static	LOGICAL	First = TRUE;

	if (First)
		{
		(void) printf("%s\n", UserLibMessage);
		if (DisplayRules)
			{
			(void) display_user_rule_functions();
			(void) display_rule_functions();
			}
		if (DisplayValues)
			{
			(void) display_user_value_functions();
			(void) display_value_functions();
			}
		if (DisplayWinds)
			{
			(void) display_user_wind_functions();
			(void) display_wind_functions();
			}
		First = FALSE;
		}
	}
