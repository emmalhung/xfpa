/*****************************************************************************
 ***                                                                       ***
 ***  u s e r _ r u l e s . c                                              ***
 ***                                                                       ***
 ***  This module is designed to access user defined routines to determine ***
 ***  attributes for meteorological fields.                                ***
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


/* Interface functions                            */
/*  ... these are defined in /lib/environ/rules.h */


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A1. Prototypes for user defined rules are added here, as in:        <*/
/*>                                                                     <*/
/*>         static ERULE_FUNC <rule_name>                               <*/
/*>                                                                     <*/
/*>      and added to the user defined functions search list, as in:    <*/
/*>                                                                     <*/
/*>         { <rule_name>, <config_file_name> },                        <*/
/*>                                                                     <*/
/*>      where:                                                         <*/
/*>         <rule_name> is the subroutine name for the user defined     <*/
/*>                             rule (in this file)                     <*/
/*>         <config_file_name> is the "entry_rules" or "type_rules"     <*/
/*>                             name in the config file                 <*/
/*>                                                                     <*/
/*>     Note that example functions are declared here.                  <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/* Define user defined rules for search list */
static	ERULE_FUNC	clds_and_wx_rule;
static	ERULE_FUNC	full_weather_rule;
static	ERULE_FUNC	full_cloud_rule;

/* Initialize user defined rule search list */
static	ERULE_TABLE	UserRules[] =
			{
				{ clds_and_wx_rule,  "user_clds_and_wx"  },
				{ full_weather_rule, "user_full_weather" },
				{ full_cloud_rule,   "user_full_cloud"   },
			};

/* Set number of user defined value functions in search list */
static	int		NumUserRules = (int) (sizeof(UserRules) / sizeof(ERULE_TABLE));


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>                                                                     <*/
/*> A2. No changes required in identify_user_rule_function()            <*/
/*>      or in display_user_rule_functions()                            <*/
/*>                                                                     <*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/***********************************************************************
*                                                                      *
*  i d e n t i f y _ u s e r _ r u l e _ f u n c t i o n               *
*                                                                      *
***********************************************************************/

ERULE	identify_user_rule_function
	(
	STRING	name
	)

	{
	int		i;

	if ( blank(name) ) return NullErule;

	/* Search user rules */
	for (i=0; i<NumUserRules; i++)
		{
		if ( same_ic(name, UserRules[i].label) )
			return UserRules[i].function;
		}

	/* If still not found, give up */
	return NullErule;
	}

/***********************************************************************
*                                                                      *
*  d i s p l a y _ u s e r _ r u l e _ f u n c t i o n s               *
*                                                                      *
***********************************************************************/

void	display_user_rule_functions
	(
	)

	{
	int		i;

	/* Display all user defined rules */
	(void) printf(" User Defined Rules");
	(void) printf(" ... from Config \"entry_rules\" or \"type_rules\" lines\n");
	for (i=0; i<NumUserRules; i++)
		{
		(void) printf("  %2d   Rule Name:  %s\n", i+1, UserRules[i].label);
		}
	}


/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (User Defined Rules)                     *
*                                                                      *
*     All the routines after this point are available only within      *
*      this file.                                                      *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*  c l d s _ a n d _ w x _ r u l e                                     *
*                                                                      *
***********************************************************************/

static	void	clds_and_wx_rule

	(
	CAL		cal
	)

	{
	STRING	val, cat;

	/* Compute "clds_and_wx" value if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("user_clds_and_wx", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "clds_and_wx") )
		{
		(void) pr_error("user_clds_and_wx", "No attribute: \"clds_and_wx\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("user_clds_and_wx", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("user_clds_and_wx", "No attribute: \"cloud_amount\"\n");
		return;
		}

	/* Set default "clds_and_wx" value */
	cat = "CLR";

	/* Set "clds_and_wx" value based on "weather" */
	val = CAL_get_attribute(cal, "weather");
	if (match(val, "TSRA"))      cat = "TSRA";
	else if (match(val, "TS"))   cat = "TS";
	else if (match(val, "FZRA")) cat = "FZRA";
	else if (match(val, "FZDZ")) cat = "FZDZ";
	else if (match(val, "RASN")) cat = "RASN";
	else if (match(val, "SHRA")) cat = "SHRA";
	else if (match(val, "SHSN")) cat = "SHSN";
	else if (match(val, "RA"))   cat = "RA";
	else if (match(val, "SN"))   cat = "SN";
	else if (match(val, "DZ"))   cat = "DZ";
	else if (match(val, "FG"))   cat = "FG";
	else if (match(val, "BR"))   cat = "BR";
	else
		{
		/* Set "clds_and_wx" value based on "cloud_amount" */
		val = CAL_get_attribute(cal, "cloud_amount");
		if (match(val, "CLR"))               cat = "CLR";
		else if (match(val, "SCT OCNL BKN")) cat = "SCT";
		else if (match(val, "SCT OCNL OVC")) cat = "SCT";
		else if (match(val, "SCT VRBL BKN")) cat = "BKN";
		else if (match(val, "SCT VRBL OVC")) cat = "OVC";
		else if (match(val, "BKN OCNL SCT")) cat = "BKN";
		else if (match(val, "BKN OCNL OVC")) cat = "BKN";
		else if (match(val, "BKN VRBL SCT")) cat = "BKN";
		else if (match(val, "BKN VRBL OVC")) cat = "OVC";
		else if (match(val, "OVC OCNL SCT")) cat = "OVC";
		else if (match(val, "OVC OCNL BKN")) cat = "OVC";
		else if (match(val, "OVC VRBL SCT")) cat = "OVC";
		else if (match(val, "OVC VRBL BKN")) cat = "OVC";
		else if (match(val, "SCT"))          cat = "SCT";
		else if (match(val, "BKN"))          cat = "BKN";
		else if (match(val, "OVC"))          cat = "OVC";
		}

	/* Set "clds_and_wx" value */
	CAL_set_attribute(cal, "clds_and_wx", cat);
	(void) pr_diag("user_clds_and_wx", "Clouds and weather: %s\n", cat);
	}

/***********************************************************************
*                                                                      *
*  f u l l _ w e a t h e r _ r u l e                                   *
*                                                                      *
***********************************************************************/

static	void	full_weather_rule

	(
	CAL		cal
	)

	{
	STRING	valv, valw, valm, valr;
	char	cat[128];

	/* Compute "full_weather" values if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("user_full_weather", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "visibility") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"visibility\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather_modifier") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"weather_modifier\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "visibility_secondary") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"visibility_secondary\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather_secondary") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"weather_secondary\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "wx_remarks") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"wx_remarks\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "full_weather") )
		{
		(void) pr_error("user_full_weather", "No attribute: \"full_weather\"\n");
		return;
		}

	/* Set default "full_weather" value */
	(void) strcpy(cat, "");

	/* Build the primary weather from "visibility" and "weather" */
	/* Note that "weather" begins with a blank!                  */
	valv = CAL_get_attribute(cal, "visibility");
	valw = CAL_get_attribute(cal, "weather");
	if ( !blank(valv) )
		{
		(void) strcat(cat, valv);
		(void) strcat(cat, valw);
		}

	/* Add the secondary weather from "weather_modifier", */
	/*  "visibility_secondary" and "weather_secondary"    */
	/* Note that "weather_secondary" begins with a blank! */
	valm = CAL_get_attribute(cal, "weather_modifier");
	valv = CAL_get_attribute(cal, "visibility_secondary");
	valw = CAL_get_attribute(cal, "weather_secondary");
	if ( !blank(valm) && !blank(valv) )
		{
		if ( !blank(cat) ) (void) strcat(cat, " ");
		(void) strcat(cat, valm);
		(void) strcat(cat, " ");
		(void) strcat(cat, valv);
		(void) strcat(cat, valw);
		}

	/* Add the weather remarks from "wx_remarks" */
	valr = CAL_get_attribute(cal, "wx_remarks");
	if ( !blank(valr) )
		{
		if ( !blank(cat) ) (void) strcat(cat, " ");
		(void) strcat(cat, valr);
		}

	/* Set "full_weather" value */
	CAL_set_attribute(cal, "full_weather", cat);
	(void) pr_diag("user_full_weather", "Full weather: %s\n", cat);
	}

/***********************************************************************
*                                                                      *
*  f u l l _ c l o u d _ r u l e                                       *
*                                                                      *
***********************************************************************/

static	void	full_cloud_rule

	(
	CAL		cal
	)

	{
	STRING	valb, vala, valt;
	char	cat1[128], cat2[128];

	/* Compute "full_cloud" values if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("user_full_cloud", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_base") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_base\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_amount\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_top") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_top\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_base_2") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_base_2\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount_2") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_amount_2\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_top_2") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"cloud_top_2\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "full_cloud_1") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"full_cloud_1\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "full_cloud_2") )
		{
		(void) pr_error("user_full_cloud", "No attribute: \"full_cloud_2\"\n");
		return;
		}

	/* Set default "full_cloud" values */
	(void) strcpy(cat1, "");
	(void) strcpy(cat2, "");

	/* Build "full_cloud_1" from "cloud_base", "cloud_amount", */
	/*  and "cloud_top"                                        */
	valb = CAL_get_attribute(cal, "cloud_base");
	vala = CAL_get_attribute(cal, "cloud_amount");
	valt = CAL_get_attribute(cal, "cloud_top");
	if ( !blank(valb) )
		{
		(void) strcpy(cat1, valb);
		(void) strcat(cat1, " ");
		(void) strcat(cat1, vala);
		(void) strcat(cat1, " ");
		(void) strcat(cat1, valt);
		}

	/* Build "full_cloud_2" from "cloud_base_2", "cloud_amount_2", */
	/*  and "cloud_top_2"                                          */
	valb = CAL_get_attribute(cal, "cloud_base_2");
	vala = CAL_get_attribute(cal, "cloud_amount_2");
	valt = CAL_get_attribute(cal, "cloud_top_2");
	if ( !blank(valb) )
		{
		(void) strcpy(cat2, valb);
		(void) strcat(cat2, " ");
		(void) strcat(cat2, vala);
		(void) strcat(cat2, " ");
		(void) strcat(cat2, valt);
		}

	/* Set "full_cloud" values */
	CAL_set_attribute(cal, "full_cloud_1", cat1);
	CAL_set_attribute(cal, "full_cloud_2", cat2);
	(void) pr_diag("user_full_cloud", "Full cloud 1: %s\n", cat1);
	(void) pr_diag("user_full_cloud", "Full cloud 2: %s\n", cat2);
	}
