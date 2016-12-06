/**********************************************************************/
/** @file gribtest.c
 *
 *  GRIB Data Testing Program
 *
 *  This program reads the given GRIB file and prints out the contents
 *  of the decoded structure.
 *
 *  Decoded GRIB data from Block 4 can be displayed.  Both the original
 *  (raw) and processed data can be displayed.  The processed data has
 *  been (if required) expanded from a quasi-regular grid or a bit
 *  mapped array of data points to a regular grid, has had a full row
 *  of data added for the pole, and has had the first meridian copied
 *  after the last meridian to produce a full hemisphere of data.
 *
 *   Usage:  gribtest  @<setup_file@>  @<grib_file@>  @<grib_edition@> 
 *   						@<diagnostic_code@>  (@<max_fields@>
 *
 *     where:
 *     -	@<setup_file@>        is the usual local setup file name
 *     -	@<grib_file@>         is the GRIB file name
 *     -	@<grib_edition@>      is the GRIB edition number (1 or 0)
 *     -	@<diagnostic_code@>   is the code for output diagnostics
 *     -	@<max_fields@>        is the maximum number of fields to
 *                                 decode (optional)
 *
 *     Allowed values for  @<diagnostic_code@>  are:
 *     -	0 - Output information from Block 0
 *     -	1 - Output information from Block 0 and 1
 *     -	2 - Output information from Block 0, 1, and 2
 *     -	3 - Output information from Block 0, 1, 2, and 3
 *     -	4 - Output information from Block 0, 1, 2, 3, and 4
 *     -	10 - Output raw data from Block 4
 *     -	50 - Output processed data from Block 4
 *             (If required, data has been expanded from a quasi-
 *             regular grid or expanded from a bit mapped array of
 *             data points, has had a full row of data added for the
 *             pole, and has had the first meridian copied after the
 *             last meridian to produce a full hemisphere of data)
 *     -	100 - Output GRIB data processed into FPA Metafile format
 *
 *     Note that the  @<diagnostic_code@>  can be a combination, so that
 *      162 would output information from Block 0, 1, and 2 as well as
 *      the raw and processed data from Block 4 and the GRIB data in
 *      FPA Metafile format.
 **********************************************************************/
/***********************************************************************
*                                                                      *
*  g r i b t e s t . c                                                 *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

/* We need FPA definitions */
#include <fpa.h>

/* #include "gribs.h" */
#include "rgrib.h"
#include "gribmeta.h"

#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*  m a i n                                                             *
*                                                                      *
***********************************************************************/

int		main

	(
	int		argc,
	STRING	argv[]
	)

	{

	/* Setup information */
	int				argv3, argv4, nsetup;
	STRING			setupfile, *setuplist, dir;
	char			home[MAX_BCHRS], work[MAX_BCHRS], gribname[MAX_BCHRS];
	char			cwd[256];
	MAP_PROJ		*mproj;
	FLD_DESCRIPT	fdesc;

	/* Diagnostic flags */
	LOGICAL			DebugBlock0 = FALSE;
	LOGICAL			DebugBlock1 = FALSE;
	LOGICAL			DebugBlock2 = FALSE;
	LOGICAL			DebugBlock3 = FALSE;
	LOGICAL			DebugBlock4 = FALSE;
	LOGICAL			DebugBlock5 = FALSE;
	LOGICAL			DebugBlock6 = FALSE;
	LOGICAL			DebugB4Raw  = FALSE;
	LOGICAL			DebugB4Data = FALSE;
	LOGICAL			DebugMetaff = FALSE;

	/* GRIB information */
	DECODEDFIELD	*gribfld;
	int				nflds = 0, MaxFlds = 0;
	STRING			model, rtime, btime, etime;
	STRING			element, level, units;
	LOGICAL			iret;

	/* Metafile information */
	int				iout;
	METAFILE		meta = NullMeta;

	/* Component information */
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementComponentStruct	*components;
	COMPONENT						compin;

	(void) setvbuf(stdout, NullString, _IOLBF, 0);
	(void) setvbuf(stderr, NullString, _IOLBF, 0);

	/* Validate run string parameters */
	if ( argc < 5 )
		{
		(void) fprintf(stderr, "\nUsage:\n");
		(void) fprintf(stderr, "   gribtest <setup_file> <grib_file>");
		(void) fprintf(stderr, " <grib_edition> <diagnostic_code>");
		(void) fprintf(stderr, " (<max_fields>)\n");
		(void) fprintf(stderr, "\n      <grib_edition> is usually \"1\"\n");
		(void) fprintf(stderr, "\n      <diagnostic_code> can be combination of:\n");
		(void) fprintf(stderr, "         0 - Information from Block 0\n");
		(void) fprintf(stderr, "         1 - Information from Block 0 and 1\n");
		(void) fprintf(stderr, "         2 - Information from Block 0, 1,");
		(void) fprintf(stderr, " and 2\n");
		(void) fprintf(stderr, "         3 - Information from Block 0, 1, 2,");
		(void) fprintf(stderr, " and 3\n");
		(void) fprintf(stderr, "         4 - Information from Block 0, 1, 2,");
		(void) fprintf(stderr, " 3, and 4\n");
		(void) fprintf(stderr, "         5 - Information from Block 0, 1, 2,");
		(void) fprintf(stderr, " 3, 4, and 5\n");
		(void) fprintf(stderr, "         6 - Information from Block 0, 1, 2,");
		(void) fprintf(stderr, " 3, 4, 5, and 6\n");
		(void) fprintf(stderr, "        10 - Raw Data\n");
		(void) fprintf(stderr, "        50 - Processed Data\n");
		(void) fprintf(stderr, "              (If required, processed data");
		(void) fprintf(stderr, " has been expanded from a quasi-regular\n");
		(void) fprintf(stderr, "              grid or a bit mapped array,");
		(void) fprintf(stderr, " has had a full row of data added for\n");
		(void) fprintf(stderr, "              the pole, and has had the");
		(void) fprintf(stderr, " first meridian copied after the last\n");
		(void) fprintf(stderr, "              meridian to produce a full");
		(void) fprintf(stderr, " hemisphere of data)\n");
		(void) fprintf(stderr, "       100 - GRIB data processed into FPA");
		(void) fprintf(stderr, " Metafile format\n");
		(void) fprintf(stderr, "\n      <diagnostic_code> \"162\" would print");
		(void) fprintf(stderr, " information from Block 0, 1, and 2; raw and\n");
		(void) fprintf(stderr, "        processed data from Block 4; and GRIB");
		(void) fprintf(stderr, " data processed into FPA Metafile format\n\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("ingest");

	/* Save the current directory */
	(void) getcwd(cwd, 256);

	/* Read the Setup file */
	setupfile = strdup(argv[1]);
	(void) fprintf(stdout, "Setup File: %s\n", setupfile);
	nsetup = setup_files(setupfile, &setuplist);
	if ( !define_setup(nsetup, setuplist) )
		{
		(void) fprintf(stderr, "[gribtest]");
		(void) fprintf(stderr, " Fatal problem with Setup File: %s\n",
				setupfile);
		return (-2);
		}

	/* Read the Config files */
	if ( !read_complete_config_file() )
		{
		(void) fprintf(stderr, "[gribtest]");
		(void) fprintf(stderr, " Fatal problem with Config Files\n");
		return (-3);
		}

	/* Read the Gribs configuration files */
	if ( !read_complete_ingest_file() )
		{
		(void) fprintf(stderr, "[gribtest]");
		(void) fprintf(stderr, " Fatal problem with Ingest Files\n");
		return (-4);
		}

	/* Get Default Map Projection */
	mproj = get_target_map();
	if ( !mproj )
		{
		(void) fprintf(stderr, "[gribtest]");
		(void) fprintf(stderr, " Fatal problem with Default Map Projection\n");
		return (-5);
		}

	(void) fprintf(stdout, "\n\nBasemap  olat: %f", mproj->definition.olat);
	(void) fprintf(stdout, "  olon: %f", mproj->definition.olon);
	(void) fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
	(void) fprintf(stdout, "         xorg: %f", mproj->definition.xorg);
	(void) fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
	(void) fprintf(stdout, "         xlen: %f", mproj->definition.xlen);
	(void) fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
	(void) fprintf(stdout, "  units: %f\n", mproj->definition.units);

	(void) fprintf(stdout, "\nGrid definition  nx: %d", mproj->grid.nx);
	(void) fprintf(stdout, "  ny: %d", mproj->grid.ny);
	(void) fprintf(stdout, "  gridlen: %f", mproj->grid.gridlen);
	(void) fprintf(stdout, "  units: %f\n", mproj->grid.units);

	/* Retrieve the operating directory */
	dir = home_directory();
	(void) strcpy(home, dir);
	dir = get_directory("ingest.src");
	if ( blank(dir) )
		dir = getenv("FPA_LOCAL_GRIB");
	if ( !blank(dir) ) (void) strcpy(work, dir);
	else               (void) strcpy(work, home);

	/* Initialize the field descriptors for output fields */
	(void) init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, home,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "\n[gribtest]");
		(void) fprintf(stderr, " Error initializing field descriptor\n");
		return (-6);
		}

	/* Get the GRIB file name (check the current directory first) */
	(void) strcpy(gribname, pathname(cwd, argv[2]));
	if (!find_file(gribname))
		{
		(void) strcpy(gribname, pathname(work, argv[2]));
		if (!find_file(gribname))
			{
			(void) fprintf(stderr, "\n[gribtest]");
			(void) fprintf(stderr, " Cannot find GRIB file: %s\n", argv[2]);
			return (-7);
			}
		}
	(void) fprintf(stdout, "\nGRIB File: %s\n", gribname);

	/* Set the GRIB edition number */
	argv3 = atoi(argv[3]);

	/* Set the diagnostic flags */
	argv4 = atoi(argv[4]);
	DebugBlock0 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  1.0 ) DebugBlock1 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  2.0 ) DebugBlock2 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  3.0 ) DebugBlock3 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  4.0 ) DebugBlock4 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  5.0 ) DebugBlock5 = TRUE;
	if ( fmod((double) argv4,  10.0) >=  6.0 ) DebugBlock6 = TRUE;
	if ( fmod((double) argv4,  50.0) >= 10.0 ) DebugB4Raw  = TRUE;
	if ( fmod((double) argv4, 100.0) >= 50.0 ) DebugB4Data = TRUE;
	if ( argv4 >= 100.0 )                      DebugMetaff = TRUE;

	/* Set the maximum number of fields to decode */
	if ( argc == 6 ) MaxFlds = atoi(argv[5]);

	/* ========================================================*/
	/* Testing GRIB Edition 0, 1 & 2 decode                    */
	/* Read GRIB fields until end-of-file or error encountered */
	/* ========================================================*/
	while ( 1 )
		{

		/* Open GRIB file for first field */
		if ( nflds == 0 )
			{
			(void) fprintf(stdout, "\n===================================");
			(void) fprintf(stdout, "\n== Testing GRIB Edition %d decode ==", argv3);
			(void) fprintf(stdout, "\n===================================\n");

			/* Open GRIB file for Edition 0, 1 or 2 decode */
			switch (argv3)
				{	
				case 0: iret = open_gribfile_edition0(gribname); break;
				case 1: iret = open_gribfile_edition1(gribname); break;
				case 2: iret = open_gribfile_edition2(gribname); break;
				default: iret = FALSE;
				}
			if ( !iret )
				{
				(void) fprintf(stderr, "\n[gribtest]");
				(void) fprintf(stderr, " Error opening GRIB file %s\n",
						gribname);
				return (-10);
				}
			}

		/* Process each GRIB field */
		(void) fprintf(stdout, "\n===================================\n");
		switch (argv3)
			{
			case 0: iret = next_gribfield_edition0(&gribfld); break;
			case 1: iret = next_gribfield_edition1(&gribfld); break;
			case 2: iret = next_gribfield_edition2(&gribfld); break;
			default: iret = FALSE;
			}
		if (!iret) break;	/* No more fields left */

		nflds++;

		/* Check for maximum number of fields */
		if ( MaxFlds > 0 && nflds > MaxFlds )
			{
			nflds--;
			break;
			}

		/* Print the identifiers */
		(void) fprintf(stdout, "\n  Information from GRIB field: %i\n", nflds);
		switch (argv3)
			{
			case 0: 
				iret = gribfield_identifiers_edition0(&model, &rtime, &btime, 
						&etime, &element, &level, &units);
				break;
			case 1: 
				iret = gribfield_identifiers_edition1(&model, &rtime, &btime, 
						&etime, &element, &level, &units);
				break;
			case 2: 
				iret = gribfield_identifiers_edition2(&model, &rtime, &btime, 
						&etime, &element, &level, &units);
				break;
			default: iret = FALSE;
			}

			if (iret)
			{
			/* (void) replace_grib_default_model(&model);
			(void) replace_grib_default_element(&element, &units); */
			(void) fprintf(stdout, "\n   Field Identifiers:\n");
			(void) fprintf(stdout, "     model           = %s \n", model);
			(void) fprintf(stdout, "     run time        = %s \n", rtime);
			(void) fprintf(stdout, "     begin valid time= %s \n", btime);
			(void) fprintf(stdout, "     end valid time  = %s \n", etime);
			(void) fprintf(stdout, "     element         = %s \n", element);
			(void) fprintf(stdout, "     level           = %s \n", level);
			(void) fprintf(stdout, "     units           = %s \n", units);
			}

			switch (argv3)
				{
				case 0:	/* Print Edition 0 */
					/* Block 0 */
					if ( DebugBlock0 ) (void) print_block0_edition0();
					/* Block 1 */
					if ( DebugBlock1 ) (void) print_block1_edition0();
					/* Block 2 */
					if ( DebugBlock2 ) (void) print_block2_edition0();
					/* Block 4 */
					if ( DebugBlock4 ) (void) print_block4_edition0();
					/* Raw GRIB data */
					if ( DebugB4Raw )  (void) print_block4_raw_edition0();
					/* Raw GRIB data */
					if ( DebugB4Data )  (void) print_block4_data_edition0();
					break;
				case 1:	/* Print Edition 1 */
					/* Block 0 */
					if ( DebugBlock0 ) (void) print_block0_edition1();
					/* Block 1 */
					if ( DebugBlock1 ) (void) print_block1_edition1();
					/* Block 2 */
					if ( DebugBlock2 ) (void) print_block2_edition1();
					/* Block 3 */
					if ( DebugBlock3 ) (void) print_block3_edition1();
					/* Block 4 */
					if ( DebugBlock4 ) (void) print_block4_edition1();
					/* Raw GRIB data */
					if ( DebugB4Raw )  (void) print_block4_raw_edition1();
					/* Raw GRIB data */
					if ( DebugB4Data )  (void) print_block4_data_edition1();
					break;
				case 2:	/* Print Edition 2 */
					/* Block 0 (Indicator) */
					if ( DebugBlock0 ) (void) print_block0_edition2();
					/* Block 1 (Indentification) */
					if ( DebugBlock1 ) (void) print_block1_edition2();
					/* Block 2 (Local Use) */
					if ( DebugBlock2 ) (void) print_block2_edition2();
					/* Block 3 (Grid Definition) */
					if ( DebugBlock3 ) (void) print_block3_edition2();
					/* Block 4 (Product Definition) */
					if ( DebugBlock4 ) (void) print_block4_edition2();
					/* Block 5 (Data Representation) */
					if ( DebugBlock5 ) (void) print_block5_edition2();
					/* Block 6 (Bit Map) */
					if ( DebugBlock6 ) (void) print_block6_edition2();
					/* Processed GRIB data (Data) */
					if ( DebugB4Data ) (void) print_block7_edition2();
					break;
				}
			if (!iret) continue;
		/***** Testing for  gribfield_to_metafile  *****/
			if ( DebugMetaff ) 
				{
				if ( set_fld_descript(&fdesc,
							FpaF_SOURCE_NAME,		model,
							FpaF_RUN_TIME,			rtime,
							FpaF_VALID_TIME,		etime,
							FpaF_ELEMENT_NAME,		element,
							FpaF_LEVEL_NAME,		level,
							FpaF_END_OF_LIST) )
					{

					/* Process GRIB fields with x/y components */
					if ( xy_component_field(fdesc.edef->name) )
						{

						/* Ensure that detailed field information has been read */
						/*  and is entered back into the field descriptor       */
						fdef = get_field_info(fdesc.edef->name, fdesc.ldef->name);
						(void) set_fld_descript(&fdesc,
												FpaF_ELEMENT,	fdef->element,
												FpaF_LEVEL,		fdef->level,
												FpaF_END_OF_LIST);

						/* Set component parameters */
						components = fdef->element->elem_detail->components;
						compin     = which_components(fdef->element->name,
								NullStringPtr, NullPtr(COMPONENT *));

					/* Identify components for output */
						switch ( compin )
							{
							case X_Comp:
								(void) fprintf(stdout, "\nMetafiles for x component: %s",
											   fdesc.edef->name);
								(void) fprintf(stdout, "     Output components:");
								for ( iout=0; iout<components->ncomp; iout++ )
									{
									(void) fprintf(stdout, " %s ",
												   components->comp_edefs[iout]->name);
									}
								(void) fprintf(stdout, "\n");
								break;
							case Y_Comp:
								(void) fprintf(stdout, "\nMetafiles for y component: %s",
											   fdesc.edef->name);
								(void) fprintf(stdout, "     Output components:");
								for ( iout=0; iout<components->ncomp; iout++ )
									{
									(void) fprintf(stdout, " %s ",
												   components->comp_edefs[iout]->name);
									}
								(void) fprintf(stdout, "\n");
								break;
							default:
								break;
							}

						/* Determine metafiles for all output components */
						for ( iout=0; iout<components->ncomp; iout++ )
							{
							(void) set_fld_descript(&fdesc,
													FpaF_ELEMENT,
													components->comp_edefs[iout],
													FpaF_END_OF_LIST);
							meta = gribfield_to_metafile_by_comp(gribfld, 
									&fdesc, units, compin,
									components->comp_types[iout]);
							if ( meta )
								{

								/* Output formatted metafile for testing */
								(void) fprintf(stdout, "\nMetafile (Formatted)\n");
								(void) write_metafile("DEBUG", meta, MaxDigits);

								/* Free space used by METAFILE Object */
								meta = destroy_metafile(meta);
								}
							}
						}

					/* Process all other GRIB fields */
					else
						{
						meta = gribfield_to_metafile(gribfld, &fdesc, units);
						if ( meta )
							{

							/* Output formatted metafile for testing */
							(void) fprintf(stdout, "\nMetafile (Formatted)\n");
							(void) write_metafile("DEBUG", meta, MaxDigits);

							/* Free space used by METAFILE Object */
							meta = destroy_metafile(meta);
							}
						}
					}
				}
		}

	/* Close GRIB file for Edition 0,1 or 2 decode */
	switch (argv3)
		{
		case 0: (void) close_gribfile_edition0(); break;
		case 1: (void) close_gribfile_edition1(); break;
		case 2: (void) close_gribfile_edition2(); break;
		}

	/* Exit if Edition 0,1 or 2 decode processed one or more fields */
	if ( nflds > 0 )
		{
		(void) fprintf(stdout, "\n=======================================");
		(void) fprintf(stdout, "\n== Total Edition %d GRIB fields: %4i ==", argv3, nflds);
		(void) fprintf(stdout, "\n=======================================\n");
		return 0;
		}
	}
