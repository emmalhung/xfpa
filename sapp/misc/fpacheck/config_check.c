/***********************************************************************
*                                                                      *
*     c o n f i g _ c h e c k . c                                      *
*                                                                      *
*     Routine to read Config and Presentation files for error checking *
*                                                                      *
*     Version 5 (c) Copyright 1999 Environment Canada (AES)            *
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

#define  CONFIG_CHECK_MAIN

/* We need FPA library definitions */
#include <fpa.h>

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

/* Trap for error situations */
static	void	error_trap(int);

/* Default message string */
static	STRING	MyLabel = "config_check";


/***********************************************************************
*                                                                      *
*    m a i n                                                           *
*                                                                      *
***********************************************************************/

int				main

	(
	int			argc,
	STRING		argv[]
	)

	{
	int						status, nslist;
	STRING					sfile, *slist;
	LOGICAL					list_names, OKarg;
	int						ii, jj;
	int						numsrc, numelem, numfld;
	int						nummks, numunit, numconst, numlvl;
	int						numgrp, numxref, numsamp;
	FpaConfigSourceStruct	**sdefs, *sdef;
	FpaConfigElementStruct	**edefs, *edef;
	FpaConfigFieldStruct	**fdefs, *fdef;
	FpaConfigUnitStruct		**udefs, **udefsx;
	FpaConfigConstantStruct	**cdefs;
	FpaConfigLevelStruct	**ldefs;
	FpaConfigGroupStruct	**gdefs;
	FpaConfigCrossRefStruct	**xdefs;
	FpaConfigSampleStruct	**pdefs;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Validate run string parameters */
	if ( argc < 2 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   config_check <setup_file>");
		(void) fprintf(stderr, " <list_names? (Y/N)>\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("generic");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	sfile  = strdup(argv[1]);
	nslist = setup_files(sfile, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr, "%s: Problem with setup file \"%s\"\n",
				MyLabel, sfile);
		(void) fprintf(stdout, "%s: Aborted\n", MyLabel);
		return (-1);
		}

	/* Set the name listing check */
	if ( argc < 3 ) list_names = FALSE;
	else            list_names = yesno_arg(argv[2], &OKarg);

	/* Read and check the Config files */
	(void) fprintf(stderr, "\n********************\n");
	(void) fprintf(stderr, "Begin checking Config Files\n\n");

	/* Check standard information from Config files */
	if ( !read_complete_config_file() )
		{
		(void) fprintf(stderr, "%s: Problem with Config Files\n", MyLabel);
		(void) fprintf(stdout, "%s: Aborted\n", MyLabel);
		return (-1);
		}

	/* Now check detailed information from Config files */
	(void) fprintf(stderr, "\nChecking detailed information");
	numsrc = identify_sources_by_type(FpaC_SRC_ANY, &sdefs);
	(void) fprintf(stderr, " for  %d  Source members\n", numsrc);
	for ( ii=0; ii<numsrc; ii++ )
		{
		if ( DebugMode )
			(void) fprintf(stderr, "  Source: \"%s\"\n", sdefs[ii]->name);
		sdef = get_source_info(sdefs[ii]->name, FpaCblank);
		}
	numsrc = identify_sources_by_type_free(&sdefs, numsrc);

	(void) fprintf(stderr, "\nChecking detailed information");
	numelem = identify_elements_by_group(FpaCanyGroup, &edefs);
	(void) fprintf(stderr, " for  %d  Element members\n", numelem);
	for ( ii=0; ii<numelem; ii++ )
		{
		if ( DebugMode )
			(void) fprintf(stderr, "  Element: \"%s\"\n", edefs[ii]->name);
		edef = get_element_info(edefs[ii]->name);
		}
	numelem = identify_elements_by_group_free(&edefs, numelem);

	(void) fprintf(stderr, "\nChecking detailed information");
	numfld = identify_fields_by_group(FpaCanyGroup, &fdefs);
	(void) fprintf(stderr, " for  %d  Field members\n", numfld);
	for ( ii=0; ii<numfld; ii++ )
		{
		if ( DebugMode )
			(void) fprintf(stderr, "  Field: \"%s %s\"\n",
					fdefs[ii]->element->name, fdefs[ii]->level->name);
		fdef = get_field_info(fdefs[ii]->element->name, fdefs[ii]->level->name);
		}
	numfld = identify_fields_by_group_free(&fdefs, numfld);

	/* Config file checks complete */
	(void) fprintf(stderr, "\nEnd checking Config Files");
	(void) fprintf(stderr, "\n********************\n");

	/* List config file names (if requested) */
	if ( list_names )
		{
		(void) fprintf(stderr, "\n********************\n");

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Sources (by type)\n");
		(void) fprintf(stderr, "  Source Type: \"Depiction\"\n");
		numsrc = identify_sources_by_type(FpaC_DEPICTION, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);
		(void) fprintf(stderr, "  Source Type: \"Guidance\"\n");
		numsrc = identify_sources_by_type(FpaC_GUIDANCE, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);
		(void) fprintf(stderr, "  Source Type: \"Allied\"\n");
		numsrc = identify_sources_by_type(FpaC_ALLIED, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);
		(void) fprintf(stderr, "  Source Type: \"Maps\"\n");
		numsrc = identify_sources_by_type(FpaC_MAPS, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);
		(void) fprintf(stderr, "  Source Type: \"Direct\"\n");
		numsrc = identify_sources_by_type(FpaC_DIRECT, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);
		(void) fprintf(stderr, "  Source Type: \"NotUsed\"\n");
		numsrc = identify_sources_by_type(FpaC_SRC_NOTUSED, &sdefs);
		for ( ii=0; ii<numsrc; ii++ )
			{
			(void) fprintf(stderr, "    Source: \"%s\"\n", sdefs[ii]->name);
			}
		numsrc = identify_sources_by_type_free(&sdefs, numsrc);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Elements (by group)\n");
		numgrp = identify_groups_for_elements(&gdefs);
		for ( ii=0; ii<numgrp; ii++ )
			{
			(void) fprintf(stderr, "  Element Group: \"%s\"\n",
					gdefs[ii]->name);
			numelem = identify_elements_by_group(gdefs[ii]->name, &edefs);
			for ( jj=0; jj<numelem; jj++ )
				{
				(void) fprintf(stderr, "    Element: \"%s\"\n",
						edefs[jj]->name);
				}
			numelem = identify_elements_by_group_free(&edefs, numelem);
			}
		numgrp = identify_groups_for_elements_free(&gdefs, numgrp);

		/* Add "special" element groups */
		(void) fprintf(stderr, "  Element Group: \"%s\"\n", FpaCnotDisplayed);
		numelem = identify_elements_by_group(FpaCnotDisplayed, &edefs);
		for ( jj=0; jj<numelem; jj++ )
			{
			(void) fprintf(stderr, "    Element: \"%s\"\n", edefs[jj]->name);
			}
		numelem = identify_elements_by_group_free(&edefs, numelem);
		(void) fprintf(stderr, "  Element Group: \"%s\"\n", FpaCgenericEqtn);
		numelem = identify_elements_by_group(FpaCgenericEqtn, &edefs);
		for ( jj=0; jj<numelem; jj++ )
			{
			(void) fprintf(stderr, "    Element: \"%s\"\n", edefs[jj]->name);
			}
		numelem = identify_elements_by_group_free(&edefs, numelem);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Levels (by type)\n");
		(void) fprintf(stderr, "  Level Type: \"Msl\"\n");
		numlvl = identify_levels_by_type(FpaC_MSL, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"Surface\"\n");
		numlvl = identify_levels_by_type(FpaC_SURFACE, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"Level\"\n");
		numlvl = identify_levels_by_type(FpaC_LEVEL, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"Layer\"\n");
		numlvl = identify_levels_by_type(FpaC_LAYER, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"Geography\"\n");
		numlvl = identify_levels_by_type(FpaC_GEOGRAPHY, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"Annotation\"\n");
		numlvl = identify_levels_by_type(FpaC_ANNOTATION, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);
		(void) fprintf(stderr, "  Level Type: \"NotUsed\"\n");
		numlvl = identify_levels_by_type(FpaC_LVL_NOTUSED, &ldefs);
		for ( ii=0; ii<numlvl; ii++ )
			{
			(void) fprintf(stderr, "    Level: \"%s\"\n", ldefs[ii]->name);
			}
		numlvl = identify_levels_by_type_free(&ldefs, numlvl);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Fields (by group)\n");
		numgrp = identify_groups_for_fields(&gdefs);
		for ( ii=0; ii<numgrp; ii++ )
			{
			(void) fprintf(stderr, "  Field Group: \"%s\"\n",
					gdefs[ii]->name);
			numfld = identify_fields_by_group(gdefs[ii]->name, &fdefs);
			for ( jj=0; jj<numfld; jj++ )
				{
				(void) fprintf(stderr, "    Field: \"%s %s\"\n",
						fdefs[jj]->element->name, fdefs[jj]->level->name);
				}
			numfld = identify_fields_by_group_free(&fdefs, numfld);
			}
		numgrp = identify_groups_for_fields_free(&gdefs, numgrp);

		/* Add "special" field groups */
		(void) fprintf(stderr, "  Field Group: \"%s\"\n", FpaCnotDisplayed);
		numfld = identify_fields_by_group(FpaCnotDisplayed, &fdefs);
		for ( jj=0; jj<numfld; jj++ )
			{
			(void) fprintf(stderr, "    Field: \"%s %s\"\n",
					fdefs[jj]->element->name, fdefs[jj]->level->name);
			}
		numfld = identify_fields_by_group_free(&fdefs, numfld);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Wind Crossrefs\n");
		numxref = identify_crossrefs_for_winds(&xdefs);
		for ( ii=0; ii<numxref; ii++ )
			{
			(void) fprintf(stderr, "  Wind Crossref: \"%s\"\n",
					xdefs[ii]->name);
			}
		numxref = identify_crossrefs_for_winds_free(&xdefs, numxref);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Value Crossrefs\n");
		numxref = identify_crossrefs_for_values(&xdefs);
		for ( ii=0; ii<numxref; ii++ )
			{
			(void) fprintf(stderr, "  Value Crossref: \"%s\"\n",
					xdefs[ii]->name);
			}
		numxref = identify_crossrefs_for_values_free(&xdefs, numxref);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Wind Samples\n");
		numsamp = identify_samples_for_winds(&pdefs);
		for ( ii=0; ii<numsamp; ii++ )
			{
			(void) fprintf(stderr, "  Wind Sample: \"%s\"\n", pdefs[ii]->name);
			}
		numsamp = identify_samples_for_winds_free(&pdefs, numsamp);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Value Samples\n");
		numsamp = identify_samples_for_values(&pdefs);
		for ( ii=0; ii<numsamp; ii++ )
			{
			(void) fprintf(stderr, "  Value Sample: \"%s\"\n", pdefs[ii]->name);
			}
		numsamp = identify_samples_for_values_free(&pdefs, numsamp);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Constants\n");
		numconst = identify_constants_by_group(FpaCany, &cdefs);
		for ( ii=0; ii<numconst; ii++ )
			{
			(void) fprintf(stderr, "  Constant: \"%s\"\n", cdefs[ii]->name);
			}
		numconst = identify_constants_by_group_free(&cdefs, numconst);

		(void) fprintf(stderr, "\nConfig File Names ...");
		(void) fprintf(stderr, " Units (by MKS equivalent values)\n");
		nummks = identify_mks_units(&udefs);
		for ( ii=0; ii<nummks; ii++ )
			{
			(void) fprintf(stderr, "  MKS units: \"%s\"\n", udefs[ii]->name);
			numunit = identify_units_by_mks(udefs[ii]->name, &udefsx);
			for ( jj=0; jj<numunit; jj++ )
				{
				(void) fprintf(stderr, "    Equivalent Units: \"%s\"\n",
						udefsx[jj]->name);
				}
			numunit = identify_units_by_mks_free(&udefsx, numunit);
			}
		nummks = identify_mks_units_free(&udefs, nummks);

		(void) fprintf(stderr, "\n********************\n");
		}

	/* Read and check the Presentation files */
	(void) fprintf(stderr, "\n********************\n");
	(void) fprintf(stderr, "Begin checking Presentation Files\n\n");

	/* <<< No Presentation file checking available yet! >>> */
	(void) fprintf(stderr, "No Presentation File checking available yet!\n");

	/* Presentation file checks complete */
	(void) fprintf(stderr, "\nEnd checking Presentation Files");
	(void) fprintf(stderr, "\n********************\n");

	return 0;
	}

/***********************************************************************
*                                                                      *
*     e r r o r _ t r a p                                              *
*                                                                      *
***********************************************************************/

static	void	error_trap

	(
	int		sig
	)

	{
	char	*sname;

	/* Ignore all further signals */
	(void) set_error_trap(SIG_IGN);
	(void) signal(sig, SIG_IGN);

	/* Get the signal name if possible */
	sname = signal_name(sig);

	/* Provide a message */
	(void) fprintf(stdout, "%s !!! %s Has Occurred - Terminating\n",
			MyLabel, sname);

	/* Die gracefully */
	(void) exit(1);
	}
