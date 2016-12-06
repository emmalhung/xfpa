/**********************************************************************/
/** @file rules.c
 *
 * Routines to handle the Controlled Attribute List
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r u l e s . c                                                      *
*                                                                      *
*   Routines to handle the Controlled Attribute List                   *
*                                                                      *
*     Version 4 (c) Copyright 1997 Environment Canada (AES)            *
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

#define _POSIX_SOURCE	/* required for fdopen() function declaration */
#define RULE_INIT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "calculation.h"
#include "rules.h"
#include "config_info.h"

#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>

#undef DEBUG_PYTHON_RULES

/***********************************************************************
*                                                                      *
*  ENTRY RULE FUNCTION SEARCH LIST:                                    *
*                                                                      *
***********************************************************************/

static	ERULE_FUNC	internal_rules_wx_label;
static	ERULE_FUNC	internal_rules_wx_category;
static	ERULE_FUNC	internal_rules_auto_category;
static	ERULE_FUNC	internal_rules_auto_label;
static	ERULE_FUNC	preset_FPA_mod100_spval;
static	ERULE_FUNC	preset_FPA_wx_label;
static	ERULE_FUNC	preset_FPA_wx_label_type;
static	ERULE_FUNC	preset_FPA_wx_category;
static	ERULE_FUNC	preset_FPA_clds_and_wx;
static	ERULE_FUNC	preset_FPA_full_weather;
static	ERULE_FUNC	preset_FPA_jet_wind;
static	ERULE_FUNC	preset_FPA_storm_cell;

static	ERULE_TABLE	EntryRules[] =
			{
				{ internal_rules_wx_label,		"weather_label"		},
				{ internal_rules_wx_category,	"weather_category"	},
				{ internal_rules_auto_label,	"auto_label"		},
				{ internal_rules_auto_category,	"auto_category"		},
				{ preset_FPA_mod100_spval,		"FPA_mod100_spval"	},
				{ preset_FPA_wx_label,			"FPA_wx_label"		},
				{ preset_FPA_wx_label_type,		"FPA_wx_label_type"	},
				{ preset_FPA_wx_category,		"FPA_wx_category"	},
				{ preset_FPA_clds_and_wx,		"FPA_clds_and_wx"	},
				{ preset_FPA_full_weather,		"FPA_full_weather"	},
				{ preset_FPA_jet_wind,			"FPA_jet_wind"		},
				{ preset_FPA_storm_cell,		"FPA_storm_cell"	},
			};

static	int		NumEntryRules =
			(sizeof(EntryRules) / sizeof(ERULE_TABLE));

/***********************************************************************
*                                                                      *
*  INTERFACE FUNCTIONS:                                                *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*  i d e n t i f y _ r u l e _ f u n c t i o n                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Identify a rule function given its name.
 *
 *	@param[in]	name	Function name
 * 	@return Pointer to an entry rule function structure.
 **********************************************************************/
ERULE	identify_rule_function
	(
	STRING	name
	)

	{
	int		i;
	ERULE	rule;

	if ( blank(name) ) return NullErule;

	/* Search user rules first */
	rule = identify_user_rule_function(name);
	if ( rule ) return rule;

	/* If not yet found, search internal rules */
	for (i=0; i<NumEntryRules; i++)
		{
		if ( same_ic(name, EntryRules[i].label) )
			return EntryRules[i].function;
		}

	/* If still not found, give up */
	return NullErule;
	}

/***********************************************************************
*                                                                      *
*  d i s p l a y _ r u l e _ f u n c t i o n s                         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Display all internal rules.
 **********************************************************************/
void	display_rule_functions
	(
	)

	{
	int		i;

	/* Display all internal rules */
	(void) printf(" Default Rules");
	(void) printf(" ... from Config \"entry_rules\" or \"type_rules\" lines\n");
	for (i=0; i<NumEntryRules; i++)
		{
		(void) printf("  %2d   Rule Name:  %s\n", i+1, EntryRules[i].label);
		}
	}

/***********************************************************************
*                                                                      *
*  C A L _ i n v o k e _ r u l e s                                     *
*  C A L _ i n v o k e _ e n t r y _ r u l e s _ b y _ n a m e         *
*  C A L _ i n v o k e _ l a b e l _ r u l e s _ b y _ n a m e         *
*  C A L _ i n v o k e _ l n o d e _ r u l e s _ b y _ n a m e         *
*  C A L _ i n v o k e _ a l l _ l c h a i n _ l n o d e _ r u l e s   *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Evaluate given entry rules.
 *
 *	@param[in]	cal 		Category attribute list
 *	@param[in]	nrules 		Size of rules list
 *	@param[in]	*rules		List of entry rules to evaluate
 **********************************************************************/
void	CAL_invoke_rules
	(
	CAL		cal,
	int		nrules,
	ERULE	*rules
	)

	{
	int		irule;

	if ( IsNull(cal) ) return;

	/* Evaluate given entry rules */
	if ( NotNull(rules) )
		{
		for (irule=0; irule<nrules; irule++)
			{
			if ( rules[irule] ) rules[irule](cal);
			}
		}

	/* Evaluate any default rules ... */
	/* >>> Try with no default rules to improve start-up speed <<< */

	/* Always ensure that CALautolabel has been set */
	/* internal_rules_auto_label(cal); */

	/* Always ensure that CALcategory has been set */
	/* internal_rules_auto_category(cal); */

	}

/**********************************************************************/
/** Evaluate given python entry rules.
 *
 *	@param[in]	cal 		Category attribute list
 *	@param[in]	nrules 		Size of rules list
 *	@param[in]	*rules		List of entry rules names to evaluate
 **********************************************************************/
void	CAL_invoke_python_rules
	(
	CAL		cal,
	int		nrules,
	STRING	*rules
	)

	{
	int		irule, fd=-1, ii, jrule, jj;
	char	cmd[255], line[255], tempFileTemplate[255] = "/tmp/fpa_python_dict_XXXXXX";
	STRING	tempFileName, key, value; 

	FILE	*tempFile;
	CAL		rcal = CAL_create_empty();
	
	if ( IsNull(cal) ) return;


	/* Evaluate given entry rules */
	if ( NotNull(rules) )
		{
#		ifdef DEBUG_PYTHON_RULES
		(void) pr_diag("Rules", "PYTHON: Invoke %d python rules.\n", nrules);
#		endif /* DEBUG_PYTHON_RULES */

		for (irule=0; irule<nrules; irule++)
			{
			if ( rules[irule] ) 
				{
#				ifdef DEBUG_PYTHON_RULES
				(void) pr_diag("Rules",
					"PYTHON: Rule %d: %s\n", irule, rules[irule]);
#				endif /* DEBUG_PYTHON_RULES */

				tempFileName = strdup(tempFileTemplate);
				if ( ( -1 == (fd = mkstemp(tempFileName)) ) || 
					( NULL == (tempFile = fdopen(fd, "w")) ) )
					{
					if ( -1 != fd ) 
						{
						unlink(tempFileName);
						close(fd);
						FREEMEM(tempFileName);
						}
					(void) pr_error("Rules",
						"PYTHON: Could not open temp file: %s\n", tempFileName);
					}
				else
					{
					(void) fprintf(tempFile, "{ ");
					for (ii = 0; ii< cal->nattribs; ii++)
						(void) fprintf(tempFile, "'%s':'%s', ",
								cal->attribs[ii].name, cal->attribs[ii].value);
					(void) fprintf(tempFile, "} ");
					(void) fclose(tempFile);

#					ifdef DEBUG_PYTHON_RULES
					(void) pr_diag("Rules", "PYTHON: Output temp file.\n");
#					endif /* DEBUG_PYTHON_RULES */

					(void) sprintf(cmd, "%s %s", rules[irule], tempFileName);	/* call python script */

#					ifdef DEBUG_PYTHON_RULES
					(void) pr_diag("Rules", "PYTHON: Command line: %s.\n", cmd);
#					endif /* DEBUG_PYTHON_RULES */

					shrun(cmd, TRUE);									/* wait for return */
#					ifdef DEBUG_PYTHON_RULES
					(void) pr_diag("Rules", "PYTHON:  ... script returned.\n");
#					endif /* DEBUG_PYTHON_RULES */
					/* Clean up dictionary file */
					(void) remove(tempFileName);

					tempFileName[16]='l';
					tempFileName[17]='i';
					tempFileName[18]='s';
					tempFileName[19]='t';
					if ( NULL != ( tempFile = fopen(tempFileName, "r") ) )	
						{
#						ifdef DEBUG_PYTHON_RULES
						(void) pr_diag("Rules",
							"PYTHON: Reading input from tempFile.\n");
#						endif /* DEBUG_PYTHON_RULES */

						(void) fscanf(tempFile, "%d\n", &jrule);	/* how many key/value pairs should I expect */
						for (jj = 0; jj < jrule; jj++)		/* Add each key/value pair to the list */
							{
							getfileline(tempFile, line, sizeof(line));
							key   = strdup_arg(line);
							value = strdup_arg(line);
							CAL_add_attribute(rcal, key, value);
							FREEMEM(key);
							FREEMEM(value);
							}
						(void) fclose(tempFile);
						CAL_merge(cal, rcal, TRUE);	/* merge the return CAL into the Original structure */
						CAL_empty(rcal);			/* reset return CAL for next rule */
						}
					else 
						(void) pr_error("Rules",
							"PYTHON: Script '%s' failed to return CAL struct\n",
							tempFileName);
					/* Clean up list file */
					(void) remove(tempFileName);
					FREEMEM(tempFileName);
					}
				}
			}
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Invoke entry rules for a given element name and level name.
 *
 *	@param[in]	cal 	Category Attributes List
 *	@param[in]	elem 	Element name
 *	@param[in]	level	Level name
 **********************************************************************/
void	CAL_invoke_entry_rules_by_name
	(
	CAL		cal,
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigFieldStruct			*fid;
	FpaConfigElementStruct			*eid;
	FpaConfigElementEditorStruct	*ed;
	int								nrules, py_nrules;
	ERULE							*rules;
	STRING							*py_rules;

	if (!cal) return;

	if (blank(elem))  elem  = FpaCanyElement;
	if (blank(level)) level = FpaCanyLevel;

	fid = get_field_info(elem, level);
	eid = (fid)? fid->element: get_element_info(elem);
	if (!eid) return;

	ed = eid->elem_detail->editor;
	if (!ed) return;

	nrules    = 0;
	py_nrules = 0;
	rules     = NullPtr(ERULE *);
	py_rules  = NullPtr(STRING *);

	/* Search for rules based on type of field */
	switch (eid->fld_type)
		{
		case FpaC_CONTINUOUS:
				nrules    = ed->type.continuous->nrules;
				rules     = ed->type.continuous->entry_funcs;
				py_nrules = ed->type.continuous->py_nrules;
				py_rules  = ed->type.continuous->py_entry_rules;
				break;

		case FpaC_VECTOR:
				nrules    = ed->type.vector->nrules;
				rules     = ed->type.vector->entry_funcs;
				py_nrules = ed->type.vector->py_nrules;
				py_rules  = ed->type.vector->py_entry_rules;
				break;

		case FpaC_DISCRETE:
				nrules    = ed->type.discrete->nrules;
				rules     = ed->type.discrete->entry_funcs;
				py_nrules = ed->type.discrete->py_nrules;
				py_rules  = ed->type.discrete->py_entry_rules;
				break;

		case FpaC_WIND:
				nrules    = ed->type.wind->nrules;
				rules     = ed->type.wind->entry_funcs;
				py_nrules = ed->type.wind->py_nrules;
				py_rules  = ed->type.wind->py_entry_rules;
				break;

		case FpaC_LINE:
				nrules    = ed->type.line->nrules;
				rules     = ed->type.line->entry_funcs;
				py_nrules = ed->type.line->py_nrules;
				py_rules  = ed->type.line->py_entry_rules;
				break;

		case FpaC_LCHAIN:
				nrules    = ed->type.lchain->nrules;
				rules     = ed->type.lchain->entry_funcs;
				py_nrules = ed->type.lchain->py_nrules;
				py_rules  = ed->type.lchain->py_entry_rules;
				break;

		case FpaC_SCATTERED:
				(void) pr_warning("Rules",
					"Scattered fields should not call CAL_invoke_entry_rules_by_name()");
				return;

		default:
				(void) pr_warning("Rules",
					"Should be no entry rules for field \"%s %s\"\n",
					elem, level);
				return;
		}

	if (nrules > 0)
		(void) pr_diag("Rules",
			"Invoking %d entry rules for field \"%s %s\"\n",
			nrules, elem, level);

	if (py_nrules > 0) 
		(void) pr_diag("Rules",
			"Invoking %d python entry rules for field \"%s %s\"\n",
			py_nrules, elem, level);


	/* Invoke rules ... including default rules */
	CAL_invoke_rules(cal, nrules, rules);
	CAL_invoke_python_rules(cal, py_nrules, py_rules);
	}

/**********************************************************************/

/**********************************************************************/
/** Invoke Label rules given an element name and level name.
 *
 *	@param[in]	cal 	Category Attribute List
 *	@param[in]	elem	Element name
 *	@param[in]	level	Level name
 **********************************************************************/
void	CAL_invoke_label_rules_by_name
	(
	CAL		cal,
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigFieldStruct				*fid;
	FpaConfigElementStruct				*eid;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementLabellingStruct		*labelling;
	int									nrules, py_nrules;
	STRING							    *py_rules;
	ERULE								*rules;
	STRING								type;
	int									ii;

	if (!cal) return;

	if (blank(elem))  elem  = FpaCanyElement;
	if (blank(level)) level = FpaCanyLevel;

	fid = get_field_info(elem, level);
	eid = (fid)? fid->element: get_element_info(elem);
	if (!eid) return;

	/* Must invoke default rules at least */
	nrules    = 0;
	py_nrules = 0;
	rules     = NullPtr(ERULE *);
	py_rules  = NullPtr(STRING *);

	/* Search for rules based on type of field */
	switch (eid->fld_type)
		{

		/* Scattered type fields have special rules */
		case FpaC_SCATTERED:

				/* Find the default attribute to match */
				type = CAL_get_attribute(cal, AttribScatteredType);
				if (blank(type))
					{
					(void) pr_warning("Rules",
						"No default attribute \"%s\" for field \"%s %s\"\n",
						AttribScatteredType, elem, level);
					break;
					}

				/* Match the default attribute */
				ii = identify_scattered_type_by_name(elem, level, type,
						&stypes);
				if (IsNull(stypes))
					{
					(void) pr_warning("Rules",
						"No scattered_types block for field \"%s %s\"\n",
						elem, level);
					}
				else if (ii < 0)
					{
					(void) pr_diag("Rules",
						"No matching type \"%s\" in scattered_types block for field \"%s %s\"\n",
						type, elem, level);
					}
				else
					{
					nrules    = stypes->type_rules[ii].nrules;
					rules     = stypes->type_rules[ii].entry_funcs;
					py_nrules = stypes->type_rules[ii].py_nrules;
					py_rules  = stypes->type_rules[ii].py_entry_rules;
					}

				if (nrules > 0)
					(void) pr_diag("Rules",
						"Invoking %d rules for field \"%s %s\"  scattered type: %s\n",
						nrules, elem, level, type);
				if (py_nrules > 0)
					(void) pr_diag("Rules",
						"Invoking %d python rules for field \"%s %s\"  scattered type: %s\n",
						py_nrules, elem, level, type);
				break;

		/* All other fields use labelling rules */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_LCHAIN:
		default:

				/* Find the default attribute to match */
				type = CAL_get_attribute(cal, AttribLabelType);
				if (blank(type))
					{
					(void) pr_warning("Rules",
						"No default attribute \"%s\" for field \"%s %s\"\n",
						AttribLabelType, elem, level);
					break;
					}

				/* Match the default attribute */
				ii = identify_labelling_type_by_name(elem, level, type,
						&labelling);
				if (IsNull(labelling))
					{
					(void) pr_warning("Rules",
						"No labelling block for field \"%s %s\"\n",
						elem, level);
					}
				else if (ii < 0)
					{
					(void) pr_diag("Rules",
						"No matching type \"%s\" in labelling block for field \"%s %s\"\n",
						type, elem, level);
					}
				else
					{
					nrules    = labelling->type_rules[ii].nrules;
					rules     = labelling->type_rules[ii].entry_funcs;
					py_nrules = labelling->type_rules[ii].py_nrules;
					py_rules  = labelling->type_rules[ii].py_entry_rules;
					}

				if (nrules > 0)
					(void) pr_diag("Rules",
						"Invoking %d labelling rules for field \"%s %s\"  label type: %s\n",
						nrules, elem, level, type);
				if (py_nrules > 0)
					(void) pr_diag("Rules",
						"Invoking %d python labelling rules for field \"%s %s\"  label type: %s\n",
						py_nrules, elem, level, type);
				break;
		}

	/* Invoke rules ... including default rules */
	CAL_invoke_rules(cal, nrules, rules);
	CAL_invoke_python_rules(cal, py_nrules, py_rules);
	}

/**********************************************************************/

/**********************************************************************/
/** Invoke link chain node rules for a given element name and level name.
 *
 *	@param[in]	cal 	Category Attributes List
 *	@param[in]	elem 	Element name
 *	@param[in]	level	Level name
 **********************************************************************/
void	CAL_invoke_lnode_rules_by_name
	(
	CAL		cal,
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigFieldStruct			*fid;
	FpaConfigElementStruct			*eid;
	FpaConfigElementEditorStruct	*ed;
	int								nrules, py_nrules;
	ERULE							*rules;
	STRING							*py_rules;

	if (!cal) return;

	if (blank(elem))  elem  = FpaCanyElement;
	if (blank(level)) level = FpaCanyLevel;

	fid = get_field_info(elem, level);
	eid = (fid)? fid->element: get_element_info(elem);
	if (!eid) return;

	ed = eid->elem_detail->editor;
	if (!ed) return;

	nrules    = 0;
	py_nrules = 0;
	rules     = NullPtr(ERULE *);
	py_rules  = NullPtr(STRING *);

	/* Search for rules based on type of field */
	switch (eid->fld_type)
		{

		case FpaC_LCHAIN:
				nrules    = ed->type.lchain->nnode_rules;
				rules     = ed->type.lchain->node_entry_funcs;
				py_nrules = ed->type.lchain->py_nnode_rules;
				py_rules  = ed->type.lchain->py_node_entry_rules;
				break;

		default:
				return;
		}

	if (nrules > 0)
		(void) pr_diag("Rules",
			"Invoking %d link chain node rules for field \"%s %s\"\n",
			nrules, elem, level);
	if (py_nrules > 0)
		(void) pr_diag("Rules",
			"Invoking %d python link chain node rules for field \"%s %s\"\n",
			py_nrules, elem, level);

	/* Invoke rules ... including default rules */
	CAL_invoke_rules(cal, nrules, rules);
	CAL_invoke_python_rules(cal, py_nrules, py_rules);
	}

/**********************************************************************/

/**********************************************************************/
/** Invoke rules for all link chain nodes for a given element name
 * and level name.
 *
 *	@param[in]	chain 	Link chain
 *	@param[in]	elem 	Element name
 *	@param[in]	level	Level name
 **********************************************************************/
void	CAL_invoke_all_lchain_lnode_rules
	(
	LCHAIN	chain,
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigFieldStruct			*fid;
	FpaConfigElementStruct			*eid;
	FpaConfigElementEditorStruct	*ed;
	int								nrules, py_nrules, inode;
	ERULE							*rules;
	STRING							*py_rules;
	LNODE							lnode;

	if (!chain) return;

	if (blank(elem))  elem  = FpaCanyElement;
	if (blank(level)) level = FpaCanyLevel;

	fid = get_field_info(elem, level);
	eid = (fid)? fid->element: get_element_info(elem);
	if (!eid) return;

	ed = eid->elem_detail->editor;
	if (!ed) return;

	nrules    = 0;
	py_nrules = 0;
	rules     = NullPtr(ERULE *);
	py_rules  = NullPtr(STRING *);

	/* Search for rules based on type of field */
	switch (eid->fld_type)
		{

		case FpaC_LCHAIN:
				nrules    = ed->type.lchain->nnode_rules;
				rules     = ed->type.lchain->node_entry_funcs;
				py_nrules = ed->type.lchain->py_nnode_rules;
				py_rules  = ed->type.lchain->py_node_entry_rules;
				break;

		default:
				(void) pr_warning("Rules",
					"Should be no link chain node rules for field \"%s %s\"\n",
					elem, level);
				return;
		}

	if (nrules > 0)
		(void) pr_diag("Rules",
			"Invoking %d rules for all link chain nodes for field \"%s %s\"\n",
			nrules, elem, level);
	if (py_nrules > 0)
		(void) pr_diag("Rules",
			"Invoking %d python rules for all link chain nodes for field \"%s %s\"\n",
			py_nrules, elem, level);

	/* Invoke rules ... including default rules for all link nodes */
	for (inode=0; inode<chain->lnum; inode++)
		{
		lnode = chain->nodes[inode];
		if ( NotNull(lnode) && lnode->there )
			{
			CAL_invoke_rules(lnode->attrib, nrules, rules);
			CAL_invoke_python_rules(lnode->attrib, py_nrules, py_rules);
			}
		}
	}

/***********************************************************************
*                                                                      *
*  INTERNAL RULE FUNCTIONS:                                            *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*  i n t e r n a l _ r u l e s _ w x _ c a t e g o r y                 *
*  i n t e r n a l _ r u l e s _ w x _ l a b e l                       *
*  i n t e r n a l _ r u l e s _ a u t o _ l a b e l                   *
*  i n t e r n a l _ r u l e s _ a u t o _ c a t e g o r y             *
*                                                                      *
***********************************************************************/

static	void	internal_rules_wx_category

	(
	CAL		cal
	)

	{
	STRING	val, cat;

	/* Compute weather category if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("Rules", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("Rules", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("Rules", "No attribute: \"cloud_amount\"\n");
		return;
		}

	/* Set default weather category */
	cat = "none";

	/* Set weather category based on "weather" */
	val = CAL_get_attribute(cal, "weather");
	if (match(val, "FZ"))      cat = "freezing";
	else if (match(val, "SN")) cat = "frozen";
	else if (match(val, "RA")) cat = "precip";
	else if (match(val, "DZ")) cat = "precip";
	else if (match(val, "FG")) cat = "vis";
	else if (match(val, "BR")) cat = "vis";
	else if (match(val, "HZ")) cat = "vis";
	else if (match(val, "FU")) cat = "vis";
	else
		{
		/* Set weather category based on "cloud_amount" */
		val = CAL_get_attribute(cal, "cloud_amount");
		if (match(val, "OVC"))      cat = "cloud";
		else if (match(val, "BKN")) cat = "cloud";
		else if (match(val, "SCT")) cat = "cloud";
		}

	/* Set weather category */
	CAL_add_attribute(cal, CALcategory, cat);
	(void) pr_diag("Rules", "Category: %s\n", cat);
	}

static	void	internal_rules_wx_label

	(
	CAL		cal
	)

	{
	STRING	val, autolab, oldlab;

	static	STRING	deflab = "No Label";

	/* Compute weather autolabel if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("Rules", "No attribute structure\n");
		return;
		}

	val = CAL_get_attribute(cal, CALautolabel);
	oldlab = strdup(val);
	autolab = deflab;
	CAL_add_attribute(cal, CALautolabel, autolab);

	/* Set weather autolabel to special FoG string if available */
	if ( CAL_has_attribute(cal, "FoG_string") )
		{
		val = CAL_get_attribute(cal, "FoG_string");
		if ( !CAL_no_value(val) )
			{
			autolab = val;
			CAL_add_attribute(cal, CALautolabel, autolab);
			(void) pr_diag("Rules", "Auto Label: %s\n", autolab);
			FREEMEM(oldlab);
			return;
			}
		}

	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("Rules", "No attribute: \"weather\"\n");
		FREEMEM(oldlab);
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("Rules", "No attribute: \"cloud_amount\"\n");
		FREEMEM(oldlab);
		return;
		}

	/* Set default weather autolabel */
	autolab = "Clear";

	/* Set weather autolabel based on "weather" */
	val = CAL_get_attribute(cal, "weather");
	if (match(val, "TSRA"))      autolab = "Thunderstorms";
	else if (match(val, "TS"))   autolab = "Thunder";
	else if (match(val, "FZRA")) autolab = "Freezing Rain";
	else if (match(val, "FZDZ")) autolab = "Freezing Drizzle";
	else if (match(val, "RASN")) autolab = "Rain and Snow";
	else if (match(val, "SHSN")) autolab = "Flurries";
	else if (match(val, "+SN"))  autolab = "Heavy Snow";
	else if (match(val, "SN"))   autolab = "Snow";
	else if (match(val, "SHRA")) autolab = "Showers";
	else if (match(val, "+RA"))  autolab = "Heavy Rain";
	else if (match(val, "RA"))   autolab = "Rain";
	else if (match(val, "DZ"))   autolab = "Drizzle";
	else if (match(val, "FG"))   autolab = "Fog";
	else if (match(val, "BR"))   autolab = "Mist";
	else if (match(val, "HZ"))   autolab = "Haze";
	else if (match(val, "FU"))   autolab = "Smoke";
	else
		{
		/* Set weather autolabel based on "cloud_amount" */
		val = CAL_get_attribute(cal, "cloud_amount");
		if (match(val, "CB"))       autolab = "CB";
		else if (match(val, "TCU")) autolab = "Towering CU";
		else if (match(val, "OVC")) autolab = "Cloudy";
		else if (match(val, "BKN")) autolab = "Mostly Cloudy";
		else if (match(val, "SCT")) autolab = "Partly Cloudy";
		else if (match(val, "FEW")) autolab = "Few Clouds";
		}

	/* Set weather autolabel */
	CAL_add_attribute(cal, CALautolabel, autolab);
	(void) pr_diag("Rules", "Auto Label: %s\n", autolab);

	if ( CAL_has_attribute(cal, CALuserlabel) )
		{
		val = CAL_get_attribute(cal, CALuserlabel);
		if (same(val, oldlab)) CAL_add_attribute(cal, CALuserlabel, autolab);
		}
	FREEMEM(oldlab);
	}

static	void	internal_rules_auto_label

	(
	CAL		cal
	)

	{
	STRING	autolab, userlab;

	static	STRING	deflab = "No Label";

	if ( IsNull(cal) ) return;

	/* Compute "AutoLabel" if not set already */
	autolab = deflab;
	if ( CAL_has_attribute(cal, CALautolabel) )
		{
		autolab = CAL_get_attribute(cal, CALautolabel);
		if ( CAL_no_value(autolab) || same(autolab, deflab) )
			{
			autolab = deflab;
			CAL_add_attribute(cal, CALautolabel, autolab);
			}
		}

	/* Compute "UserLabel" if not set manually */
	if ( CAL_has_attribute(cal, CALuserlabel) )
		{
		userlab = CAL_get_attribute(cal, CALuserlabel);
		if ( CAL_no_value(userlab) || same(userlab, deflab) )
			{
			userlab = autolab;
			if ( CAL_no_value(userlab) ) userlab = deflab;
			CAL_add_attribute(cal, CALuserlabel, userlab);
			}
		}
	}

static	void	internal_rules_auto_category

	(
	CAL		cal
	)

	{
	STRING	cat;

	static	STRING	defcat = "default";

	/* Compute default category if possible */
	if ( IsNull(cal) )                          return;
	if ( !CAL_has_attribute(cal, CALcategory) ) return;

	cat = CAL_get_attribute(cal, CALcategory);
	if ( CAL_no_value(cat) )
		{
		CAL_add_attribute(cal, CALcategory, defcat);
		pr_diag("Rules", "Category: %s\n", defcat);
		}
	}

/***********************************************************************
*                                                                      *
*  p r e s e t _ F P A _ m o d 1 0 0 _ s p v a l                       *
*                                                                      *
***********************************************************************/

static	void	preset_FPA_mod100_spval

	(
	CAL		cal
	)

	{
	STRING	val;
	double	dval, dmodval;
	int		imodval;
	char	cat[128];

	/* Compute short form of evaluated spline value if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_mod100_spval", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "EVAL_spval") &&
		 !CAL_has_attribute(cal, "EVAL_contour") )
		{
		(void) pr_error("FPA_mod100_spval",
			"No attribute: \"EVAL_spval\" or \"EVAL_contour\"\n");
		return;
		}

	/* Get spline value (if available) */
	val = CAL_get_attribute(cal, "EVAL_spval");
	if (blank(val)) val = CAL_get_attribute(cal, "EVAL_contour");

	/* Determine short form of spline value */
	(void) strcpy(cat, "");
	if ( !blank(val) )
		{
		dval    = atof(val);
		dmodval = fmod(dval, 100.0);
		imodval = NINT(dmodval);
		(void) sprintf(cat, "%.2d", imodval);
		}

	/* Set "EVAL_mod100_spval" value */
	CAL_add_attribute(cal, "EVAL_mod100_spval", cat);
	(void) pr_diag("FPA_mod100_spval", "Spline value: %s\n", cat);
	}

/***********************************************************************
*                                                                      *
*  p r e s e t _ F P A _ w x _ l a b e l                               *
*  p r e s e t _ F P A _ w x _ l a b e l _ t y p e                     *
*  p r e s e t _ F P A _ w x _ c a t e g o r y                         *
*  p r e s e t _ F P A _ c l d s _ a n d _ w x                         *
*  p r e s e t _ F P A _ f u l l _ w e a t h e r                       *
*                                                                      *
***********************************************************************/

static	void	preset_FPA_wx_label

	(
	CAL		cal
	)

	{
	STRING	val, autolab, oldlab;

	static	STRING	deflab = "No Label";

	/* Compute weather autolabel if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_wx_label", "No attribute structure\n");
		return;
		}

	val = CAL_get_attribute(cal, CALautolabel);
	oldlab = strdup(val);
	autolab = deflab;
	CAL_add_attribute(cal, CALautolabel, autolab);

	/* Set weather autolabel to special FoG string if available */
	if ( CAL_has_attribute(cal, "FoG_string") )
		{
		val = CAL_get_attribute(cal, "FoG_string");
		if ( !CAL_no_value(val) )
			{
			autolab = val;
			CAL_add_attribute(cal, CALautolabel, autolab);
			(void) pr_diag("FPA_wx_label", "Auto Label: %s\n", autolab);
			FREEMEM(oldlab);
			return;
			}
		}

	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("FPA_wx_label", "No attribute: \"weather\"\n");
		FREEMEM(oldlab);
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("FPA_wx_label", "No attribute: \"cloud_amount\"\n");
		FREEMEM(oldlab);
		return;
		}

	/* Set default weather autolabel */
	autolab = "Clear";

	/* Set weather autolabel based on "weather" */
	val = CAL_get_attribute(cal, "weather");
	if (match(val, "TSRA"))      autolab = "Thunderstorms";
	else if (match(val, "TS"))   autolab = "Thunder";
	else if (match(val, "FZRA")) autolab = "Freezing Rain";
	else if (match(val, "FZDZ")) autolab = "Freezing Drizzle";
	else if (match(val, "RASN")) autolab = "Rain and Snow";
	else if (match(val, "SHSN")) autolab = "Flurries";
	else if (match(val, "+SN"))  autolab = "Heavy Snow";
	else if (match(val, "SN"))   autolab = "Snow";
	else if (match(val, "SHRA")) autolab = "Showers";
	else if (match(val, "+RA"))  autolab = "Heavy Rain";
	else if (match(val, "RA"))   autolab = "Rain";
	else if (match(val, "DZ"))   autolab = "Drizzle";
	else if (match(val, "FG"))   autolab = "Fog";
	else if (match(val, "BR"))   autolab = "Mist";
	else if (match(val, "HZ"))   autolab = "Haze";
	else if (match(val, "FU"))   autolab = "Smoke";
	else
		{
		/* Set weather autolabel based on "cloud_amount" */
		val = CAL_get_attribute(cal, "cloud_amount");
		if (match(val, "CB"))       autolab = "CB";
		else if (match(val, "TCU")) autolab = "Towering CU";
		else if (match(val, "OVC")) autolab = "Cloudy";
		else if (match(val, "BKN")) autolab = "Mostly Cloudy";
		else if (match(val, "SCT")) autolab = "Partly Cloudy";
		else if (match(val, "FEW")) autolab = "Few Clouds";
		}

	/* Set weather autolabel */
	CAL_add_attribute(cal, CALautolabel, autolab);
	(void) pr_diag("FPA_wx_label", "Auto Label: %s\n", autolab);

	if ( CAL_has_attribute(cal, CALuserlabel) )
		{
		val = CAL_get_attribute(cal, CALuserlabel);
		if (same(val, oldlab)) CAL_add_attribute(cal, CALuserlabel, autolab);
		}
	FREEMEM(oldlab);
	}

static	void	preset_FPA_wx_label_type

	(
	CAL		cal
	)

	{
	STRING	vall, valw, vali, cat;

	/* Compute "wx_label_type" values if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_wx_label_type", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "wx_label_category") )
		{
		(void) pr_error("FPA_wx_label_type", "No attribute: \"wx_label_category\"\n");
		return;
		}

	/* Set default "wx_label_type" value */
	vall = CAL_get_attribute(cal, "wx_label_category");
	cat = "";

	/* Set the "wx_label_type" for Aviation weather labels */
	if ( same(vall, "Wx") )
		{
		valw = CAL_get_attribute(cal, "FPA_category");
		if (same(valw, "freezing"))    cat = "Freezing";
		else if (same(valw, "frozen")) cat = "Frozen";
		else if (same(valw, "precip")) cat = "Precip";
		else if (same(valw, "vis"))    cat = "Vis";
		else if (same(valw, "cloud"))  cat = "Cloud";
		else                           cat = "NoWx";
		}

	/* Set the "wx_label_type" for Aviation icing labels */
	else if ( same(vall, "Icg") )
		{
		vali = CAL_get_attribute(cal, "icing_intensity");
		if (same(vali, "LGT"))      cat = "LgtIcg";
		else if (same(vali, "MDT")) cat = "MdtIcg";
		else if (same(vali, "SVR")) cat = "SvrIcg";
		else                        cat = "NoIcg";
		}

	/* Set "wx_label_type" value */
	CAL_add_attribute(cal, "wx_label_type", cat);
	(void) pr_diag("FPA_wx_label_type", "Weather Label Type: %s\n", cat);
	}

static	void	preset_FPA_wx_category

	(
	CAL		cal
	)

	{
	STRING	val, cat;

	/* Compute weather category if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_wx_category", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("FPA_wx_category", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("FPA_wx_category", "No attribute: \"cloud_amount\"\n");
		return;
		}

	/* Set default weather category */
	cat = "none";

	/* Set weather category based on "weather" */
	val = CAL_get_attribute(cal, "weather");
	if (match(val, "FZ"))      cat = "freezing";
	else if (match(val, "SN")) cat = "frozen";
	else if (match(val, "RA")) cat = "precip";
	else if (match(val, "DZ")) cat = "precip";
	else if (match(val, "FG")) cat = "vis";
	else if (match(val, "BR")) cat = "vis";
	else if (match(val, "HZ")) cat = "vis";
	else if (match(val, "FU")) cat = "vis";
	else
		{
		/* Set weather category based on "cloud_amount" */
		val = CAL_get_attribute(cal, "cloud_amount");
		if (match(val, "OVC"))      cat = "cloud";
		else if (match(val, "BKN")) cat = "cloud";
		else if (match(val, "SCT")) cat = "cloud";
		}

	/* Set weather category */
	CAL_add_attribute(cal, CALcategory, cat);
	(void) pr_diag("FPA_wx_category", "Category: %s\n", cat);
	}

static	void	preset_FPA_clds_and_wx

	(
	CAL		cal
	)

	{
	STRING	val, cat;

	/* Compute "clds_and_wx" value if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_clds_and_wx", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("FPA_clds_and_wx", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "cloud_amount") )
		{
		(void) pr_error("FPA_clds_and_wx", "No attribute: \"cloud_amount\"\n");
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
	else if (match(val, "HZ"))   cat = "HZ";
	else if (match(val, "FU"))   cat = "FU";
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
	CAL_add_attribute(cal, "clds_and_wx", cat);
	(void) pr_diag("FPA_clds_and_wx", "Clouds and weather: %s\n", cat);
	}

static	void	preset_FPA_full_weather

	(
	CAL		cal
	)

	{
	STRING	valv, valw, valm, valr;
	char	cat[128];

	/* Compute "full_weather" values if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_full_weather", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "visibility") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"visibility\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"weather\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather_modifier") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"weather_modifier\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "visibility_secondary") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"visibility_secondary\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "weather_secondary") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"weather_secondary\"\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "wx_remarks") )
		{
		(void) pr_error("FPA_full_weather", "No attribute: \"wx_remarks\"\n");
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
	CAL_add_attribute(cal, "full_weather", cat);
	(void) pr_diag("FPA_full_weather", "Full weather: %s\n", cat);
	}

/***********************************************************************
*                                                                      *
*  p r e s e t _ F P A _ j e t _ w i n d                               *
*                                                                      *
***********************************************************************/

static	void	preset_FPA_jet_wind

	(
	CAL		cal
	)

	{
	STRING		spd, dir, wval;
	double		dspd, ddir;
	WIND_VAL	wv;

	/* Compute wind label for jet wind if possible */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_jet_wind", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, "jet_core_speed") ||
		 !CAL_has_attribute(cal, AttribLineDirection) )
		{
		(void) pr_error("FPA_jet_wind",
			"No attribute: \"jet_core_speed\" or \"%s\"\n",
			AttribLineDirection);
		return;
		}

	/* Get jet core speed ... which must be in knots */
	spd = CAL_get_attribute(cal, "jet_core_speed");
	if ( !blank(spd) ) dspd = atof(spd);

	/* Get line direction at location */
	/*  ... which must be in degrees true in a direction towards */
	dir = CAL_get_attribute(cal, AttribLineDirection);
	if ( !blank(dir) )
		{
		ddir = atof(dir) - 180;
		if ( ddir < 0.0 ) ddir += 360.0;
		}

	/* Build the wind value string */
	if ( !blank(spd) && !blank(dir) )
		{
		wv.dir   = (float) ddir;
		wv.dunit = "degrees_true";
		wv.speed = (float) dspd;
		wv.gust  = 0.0;
		wv.sunit = "knots";
		wval     = build_wind_value_string(&wv);
		}
	else
		{
		wval     = strdup("");
		}

	/* Set "EVAL_jet_wind" value */
	CAL_add_attribute(cal, "jet_wind", wval);
	(void) pr_diag("FPA_jet_wind", "Jet wind: %s\n", wval);
	FREEMEM(wval);
	}

/***********************************************************************
*                                                                      *
*  p r e s e t _ F P A _ s t o r m _ c e l l                           *
*                                                                      *
***********************************************************************/

static	void	preset_FPA_storm_cell

	(
	CAL		cal
	)

	{
	STRING		stype, splus;
	int			mplus;

	/* Reset storm cell type based on time */
	if ( IsNull(cal) )
		{
		(void) pr_error("FPA_storm_cell", "No attribute structure\n");
		return;
		}
	if ( !CAL_has_attribute(cal, AttribLnodeType) ||
		 !CAL_has_attribute(cal, AttribLnodeTime) ||
		 !CAL_has_attribute(cal, AttribLabelType) )
		{
		(void) pr_error("FPA_storm_cell",
			"No attribute: \"%s\" or \"%s\" or \"%s\"\n",
			AttribLnodeType, AttribLnodeTime, AttribLabelType);
		return;
		}

	/* Attributes of interpolated nodes are not changed */
	stype = CAL_get_attribute(cal, AttribLnodeType);
	if ( blank(stype) || same(stype, FpaNodeClass_Unknown) ||
		 same(stype, FpaNodeClass_Interp) ) return;

	/* Get the storm cell time ... which will be in minutes */
	splus = CAL_get_attribute(cal, AttribLnodeTime);
	mplus = (!blank(splus))? atoi(splus): 0;

	/* Get the storm cell label type */
	stype = CAL_get_attribute(cal, AttribLabelType);
	if ( !blank(stype) )
		{
		/* Storm cells with time less than zero must be "hist" */
		if ( mplus < 0 && !same(stype, "hist") )
			{
			(void) pr_diag("FPA_storm_cell",
				"Modify \"%s\" from \"%s\" to \"hist\" for time: %d\n",
				AttribLabelType, stype, mplus);
			CAL_set_attribute(cal, AttribLabelType, "hist");
			}

		/* Storm cells with time of zero must be "hist_end" or "prestorm_start" */
		else if ( mplus == 0 && !same(stype, "hist_end")
								&& !same(stype, "prestorm_start") )
			{
			/* Modify prestorm cell types */
			if ( same_start(stype, "prestorm") )
				{
				(void) pr_diag("FPA_storm_cell",
					"Modify \"%s\" from \"%s\" to \"prestorm_start\" for time: %d\n",
					AttribLabelType, stype, mplus);
				CAL_set_attribute(cal, AttribLabelType, "prestorm_start");
				}

			/* Modify all other storm cell types */
			else
				{
				(void) pr_diag("FPA_storm_cell",
					"Modify \"%s\" from \"%s\" to \"hist_end\" for time: %d\n",
					AttribLabelType, stype, mplus);
				CAL_set_attribute(cal, AttribLabelType, "hist_end");
				}
			}

		/* Storm cells with time greater than zero must be "fcst" or "prestorm" */
		else if ( mplus > 0 && !same_start(stype, "fcst")
								&& !same(stype, "prestorm") )
			{
			/* Modify prestorm cell types */
			if ( same_start(stype, "prestorm") )
				{
				(void) pr_diag("FPA_storm_cell",
					"Modify \"%s\" from \"%s\" to \"prestorm\" for time: %d\n",
					AttribLabelType, stype, mplus);
				CAL_set_attribute(cal, AttribLabelType, "prestorm");
				}

			/* Modify all other storm cell types */
			else
				{
				(void) pr_diag("FPA_storm_cell",
					"Modify \"%s\" from \"%s\" to \"fcst\" for time: %d\n",
					AttribLabelType, stype, mplus);
				CAL_set_attribute(cal, AttribLabelType, "fcst");
				}
			}
		}
	}
