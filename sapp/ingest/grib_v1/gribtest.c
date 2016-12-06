/***********************************************************************
*                                                                      *
*  g r i b t e s t . c                                                 *
*                                                                      *
*  GRIB Data Testing Program                                           *
*                                                                      *
*  This program reads the given GRIB file and prints out the contents  *
*  of the decoded structure.                                           *
*                                                                      *
*  Decoded GRIB data from Block 4 can be displayed.  Both the original *
*  (raw) and processed data can be displayed.  The processed data has  *
*  been (if required) expanded from a quasi-regular grid or a bit      *
*  mapped array of data points to a regular grid, has had a full row   *
*  of data added for the pole, and has had the first meridian copied   *
*  after the last meridian to produce a full hemisphere of data.       *
*                                                                      *
*   Usage:  gribtest  <setup_file>  <grib_file>  <grib_edition>        *
*                       <diagnostic_code>  (<max_fields>)              *
*                                                                      *
*     where  <setup_file>        is the usual local setup file name    *
*            <grib_file>         is the GRIB file name                 *
*            <grib_edition>      is the GRIB edition number (1 or 0)   *
*            <diagnostic_code>   is the code for output diagnostics    *
*            <max_fields>        is the maximum number of fields to    *
*                                 decode (optional)                    *
*                                                                      *
*     Allowed values for  <diagnostic_code>  are:                      *
*        0 - Output information from Block 0                           *
*        1 - Output information from Block 0 and 1                     *
*        2 - Output information from Block 0, 1, and 2                 *
*        3 - Output information from Block 0, 1, 2, and 3              *
*        4 - Output information from Block 0, 1, 2, 3, and 4           *
*       10 - Output raw data from Block 4                              *
*       50 - Output processed data from Block 4                        *
*             (If required, data has been expanded from a quasi-       *
*             regular grid or expanded from a bit mapped array of      *
*             data points, has had a full row of data added for the    *
*             pole, and has had the first meridian copied after the    *
*             last meridian to produce a full hemisphere of data)      *
*      100 - Output GRIB data processed into FPA Metafile format       *
*                                                                      *
*     Note that the  <diagnostic_code>  can be a combination, so that  *
*      162 would output information from Block 0, 1, and 2 as well as  *
*      the raw and processed data from Block 4 and the GRIB data in    *
*      FPA Metafile format.                                            *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#include "gribs.h"
#include "rgrib_edition1.h"
#include "rgrib_edition0.h"
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
	LOGICAL			DebugB4Raw  = FALSE;
	LOGICAL			DebugB4Data = FALSE;
	LOGICAL			DebugMetaff = FALSE;

	/* GRIB information */
	GRIBFIELD		*gribfld;
	int				nflds = 0, MaxFlds = 0;
	STRING			model, rtime, btime, etime;
	STRING			element, level, units;
	int				iflags, iproj, icode, isweep, ipole, ilatlon;
	int				ni, nj, ii, jj, count, MaxCount = 10;
	LOGICAL			*gbits, gbit;
	float			*gvals;
	Octet			octet;

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
		(void) fprintf(stderr, "        10 - Raw Data from Block 4\n");
		(void) fprintf(stderr, "        50 - Processed Data from Block 4\n");
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
	if ( !read_complete_gribs_file() )
		{
		(void) fprintf(stderr, "[gribtest]");
		(void) fprintf(stderr, " Fatal problem with Gribs Files\n");
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
	if ( fmod((double) argv4,  50.0) >= 10.0 ) DebugB4Raw  = TRUE;
	if ( fmod((double) argv4, 100.0) >= 50.0 ) DebugB4Data = TRUE;
	if ( argv4 >= 100.0 )                      DebugMetaff = TRUE;

	/* Set the maximum number of fields to decode */
	if ( argc == 6 ) MaxFlds = atoi(argv[5]);

	/* Testing GRIB Edition 1 decode                           */
	/* Read GRIB fields until end-of-file or error encountered */
	while ( argv3 == 1 )
		{

		/* Open GRIB file for first field */
		if ( nflds == 0 )
			{
			(void) fprintf(stdout, "\n===================================");
			(void) fprintf(stdout, "\n== Testing GRIB Edition 1 decode ==");
			(void) fprintf(stdout, "\n===================================\n");

			/* Open GRIB file for Edition 1 decode */
			if ( !open_gribfile_edition1(gribname) )
				{
				(void) fprintf(stderr, "\n[gribtest]");
				(void) fprintf(stderr, " Error opening GRIB file %s\n",
						gribname);
				return (-10);
				}
			}

		/* Process each GRIB field */
		(void) fprintf(stdout, "\n===================================\n");
		if ( !next_gribfield_edition1(&gribfld) ) break;
		nflds++;

		/* Check for maximum number of fields */
		if ( MaxFlds > 0 && nflds > MaxFlds )
			{
			nflds--;
			break;
			}

		/* Print the identifiers */
		(void) fprintf(stdout, "\n  Information from GRIB field: %i\n", nflds);
		if ( gribfield_identifiers_edition1(&model, &rtime, &btime, &etime,
				&element, &level, &units) )
			{
			(void) replace_grib_default_model(&model);
			(void) replace_grib_default_element(&element, &units);
			(void) fprintf(stdout, "\n   Field Identifiers:\n");
			(void) fprintf(stdout, "     model           = %s \n", model);
			(void) fprintf(stdout, "     run time        = %s \n", rtime);
			(void) fprintf(stdout, "     begin valid time= %s \n", btime);
			(void) fprintf(stdout, "     end valid time  = %s \n", etime);
			(void) fprintf(stdout, "     element         = %s \n", element);
			(void) fprintf(stdout, "     level           = %s \n", level);
			(void) fprintf(stdout, "     units           = %s \n", units);
			}

		/* Print Edition 1 Block 0 information */
		if ( DebugBlock0 )
			{
			(void) fprintf(stdout, "\n   Block 0:\n");
			(void) fprintf(stdout, "     Total length    = %d \n",
					gribfld->Isb.length);
			(void) fprintf(stdout, "     Edition number  = %d \n",
					gribfld->Isb.edition);
			}

		/* Print Edition 1 Block 1 information */
		if ( DebugBlock1 )
			{
			(void) fprintf(stdout, "\n   Block 1:\n");
			(void) fprintf(stdout, "     PDB length      = %d \n",
					gribfld->Pdd.length);
			(void) fprintf(stdout, "     PDB edition     = %d \n",
					gribfld->Pdd.edition);
			(void) fprintf(stdout, "     PDB center      = %d \n",
					gribfld->Pdd.centre_id);
			(void) fprintf(stdout, "     PDB model_id    = %d \n",
					gribfld->Pdd.model_id);
			(void) fprintf(stdout, "     PDB grid_defn   = %d \n",
					gribfld->Pdd.grid_defn);
			octet = 0;
			if (gribfld->Pdd.block_flags.grid_description)
					SETBIT(octet, E1_block_flag_grid_desc);
			if (gribfld->Pdd.block_flags.bit_map)
					SETBIT(octet, E1_block_flag_bit_map);
			iflags = (int) octet;
			(void) fprintf(stdout, "     PDB block_flags = %d \n",
					iflags);
			(void) fprintf(stdout, "     PDB parameter   = %d \n",
					gribfld->Pdd.parameter);
			(void) fprintf(stdout, "     PDB level type  = %d \n",
					gribfld->Pdd.layer.type);
			(void) fprintf(stdout, "     PDB layer.top   = %d \n",
					gribfld->Pdd.layer.top);
			(void) fprintf(stdout, "     PDB layer.bottom= %d \n",
					gribfld->Pdd.layer.bottom);
			(void) fprintf(stdout, "     PDB year        = %d \n",
					gribfld->Pdd.forecast.reference.year);
			(void) fprintf(stdout, "     PDB month       = %d \n",
					gribfld->Pdd.forecast.reference.month);
			(void) fprintf(stdout, "     PDB day         = %d \n",
					gribfld->Pdd.forecast.reference.day);
			(void) fprintf(stdout, "     PDB hour        = %d \n",
					gribfld->Pdd.forecast.reference.hour);
			(void) fprintf(stdout, "     PDB minutes     = %d \n",
					gribfld->Pdd.forecast.reference.minute);
			(void) fprintf(stdout, "     PDB unit        = %d \n",
					gribfld->Pdd.forecast.units);
			(void) fprintf(stdout, "     PDB time1       = %d \n",
					gribfld->Pdd.forecast.time1);
			(void) fprintf(stdout, "     PDB time2       = %d \n",
					gribfld->Pdd.forecast.time2);
			(void) fprintf(stdout, "     PDB range_type  = %d \n",
					gribfld->Pdd.forecast.range_type);
			(void) fprintf(stdout, "     PDB no. average = %d \n",
					gribfld->Pdd.forecast.nb_averaged);
			(void) fprintf(stdout, "     PDB no. missing = %d \n",
					gribfld->Pdd.forecast.nb_missing);
			(void) fprintf(stdout, "     PDB century     = %d \n",
					gribfld->Pdd.forecast.century);
			(void) fprintf(stdout, "     PDB reserved    = %d \n",
					gribfld->Pdd.forecast.reserved);
			(void) fprintf(stdout, "     PDB factor_d    = %d \n",
					gribfld->Pdd.forecast.factor_d);
			}

		/* Print Edition 1 Block 2 information */
		if ( DebugBlock2 )
			{
			(void) fprintf(stdout, "\n   Block 2:\n");
			(void) fprintf(stdout, "     GDB length      = %d \n",
					gribfld->Gdd.length);
			(void) fprintf(stdout, "     GDB Nv          = %d \n",
					gribfld->Gdd.nv);
			(void) fprintf(stdout, "     GDB pv or pl    = %d \n",
					gribfld->Gdd.pv_or_pl);
			(void) fprintf(stdout, "     GDB grid type   = %d \n",
					gribfld->Gdd.dat_rep);

			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:			/* LATITUDE-LONGITUDE */
					(void) fprintf(stdout, "     GDB Ni          = %d \n",
							gribfld->Gdd.defn.reg_ll.Ni);
					(void) fprintf(stdout, "     GDB Nj          = %d \n",
							gribfld->Gdd.defn.reg_ll.Nj);
					(void) fprintf(stdout, "     GDB La1         = %d \n",
							gribfld->Gdd.defn.reg_ll.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.reg_ll.Lo1);
					(void) fprintf(stdout, "     GDB resolution  = %d \n",
							gribfld->Gdd.defn.reg_ll.resltn);
					(void) fprintf(stdout, "     GDB La2         = %d \n",
							gribfld->Gdd.defn.reg_ll.La2);
					(void) fprintf(stdout, "     GDB Lo2         = %d \n",
							gribfld->Gdd.defn.reg_ll.Lo2);
					(void) fprintf(stdout, "     GDB Di          = %d \n",
							gribfld->Gdd.defn.reg_ll.Di);
					(void) fprintf(stdout, "     GDB Dj          = %d \n",
							gribfld->Gdd.defn.reg_ll.Dj);
					octet = 0;
					if (gribfld->Gdd.defn.reg_ll.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.reg_ll.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.reg_ll.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "            (west)     = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.west);
					(void) fprintf(stdout, "            (north)    = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.north);
					(void) fprintf(stdout, "        (by longitude) = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.horz_sweep);
					(void) fprintf(stdout, "     GDB thin mode   = %d \n",
							gribfld->Gdd.defn.reg_ll.thin_mode);
					if ( gribfld->Gdd.defn.reg_ll.thin_mode > 0 )
						{
						if ( gribfld->Gdd.defn.reg_ll.thin_mode == 1 )
							ii = gribfld->Gdd.defn.reg_ll.Nj;
						else if ( gribfld->Gdd.defn.reg_ll.thin_mode == 2 )
							ii = gribfld->Gdd.defn.reg_ll.Ni;
						else
							ii = 0;
						(void) fprintf(stdout, "     GDB thin points = ");
						for ( count=0, jj=0; jj<ii; jj++ )
							{
							if (++count > MaxCount)
								{
								count = 1;
								(void) fprintf(stdout, "\n                       ");
								}
							(void) fprintf(stdout, "%3d ",
									gribfld->Gdd.defn.reg_ll.thin_pts[jj]);
							}
						(void) fprintf(stdout, "\n");
						}
					(void) fprintf(stdout, "     GDB pole extra  = %d \n",
							gribfld->Gdd.defn.reg_ll.pole_extra);
					(void) fprintf(stdout, "     GDB long extra  = %d \n",
							gribfld->Gdd.defn.reg_ll.meridian_extra);
					break;
				case GAUSS_GRID:			/* GAUSSIAN */
					(void) fprintf(stdout, "     GDB Ni          = %d \n",
							gribfld->Gdd.defn.guas_ll.Ni);
					(void) fprintf(stdout, "     GDB Nj          = %d \n",
							gribfld->Gdd.defn.guas_ll.Nj);
					(void) fprintf(stdout, "     GDB La1         = %d \n",
							gribfld->Gdd.defn.guas_ll.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.guas_ll.Lo1);
					(void) fprintf(stdout, "     GDB resolution  = %d \n",
							gribfld->Gdd.defn.guas_ll.resltn);
					(void) fprintf(stdout, "     GDB La2         = %d \n",
							gribfld->Gdd.defn.guas_ll.La2);
					(void) fprintf(stdout, "     GDB Lo2         = %d \n",
							gribfld->Gdd.defn.guas_ll.Lo2);
					(void) fprintf(stdout, "     GDB Di          = %d \n",
							gribfld->Gdd.defn.guas_ll.Di);
					(void) fprintf(stdout, "     GDB N           = %d \n",
							gribfld->Gdd.defn.guas_ll.N);
					octet = 0;
					if (gribfld->Gdd.defn.guas_ll.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.guas_ll.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.guas_ll.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "            (west)     = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.west);
					(void) fprintf(stdout, "            (north)    = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.north);
					(void) fprintf(stdout, "        (by longitude) = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.horz_sweep);
					(void) fprintf(stdout, "     GDB thin mode   = %d \n",
							gribfld->Gdd.defn.guas_ll.thin_mode);
					if ( gribfld->Gdd.defn.guas_ll.thin_mode > 0 )
						{
						if ( gribfld->Gdd.defn.guas_ll.thin_mode == 1 )
							ii = gribfld->Gdd.defn.guas_ll.Nj;
						else
							ii = 0;
						(void) fprintf(stdout, "     GDB thin points = ");
						for ( count=0, jj=0; jj<ii; jj++ )
							{
							if ( ++count > MaxCount )
								{
								count = 1;
								(void) fprintf(stdout, "\n                       ");
								}
							(void) fprintf(stdout, "%3d ",
									gribfld->Gdd.defn.guas_ll.thin_pts[jj]);
							}
						(void) fprintf(stdout, "\n");
						}
					break;
				case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
					(void) fprintf(stdout, "     GDB Nx          = %d \n",
							gribfld->Gdd.defn.ps.Nx);
					(void) fprintf(stdout, "     GDB Ny          = %d \n",
							gribfld->Gdd.defn.ps.Ny);
					(void) fprintf(stdout, "     GDB La1         = %d \n",
							gribfld->Gdd.defn.ps.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.ps.Lo1);
					(void) fprintf(stdout, "     GDB compnt      = %d \n",
							gribfld->Gdd.defn.ps.compnt);
					(void) fprintf(stdout, "     GDB LoV         = %d \n",
							gribfld->Gdd.defn.ps.LoV);
					(void) fprintf(stdout, "     GDB Dx          = %d \n",
							gribfld->Gdd.defn.ps.Dx);
					(void) fprintf(stdout, "     GDB Dy          = %d \n",
							gribfld->Gdd.defn.ps.Dy);
					octet = 0;
					if (gribfld->Gdd.defn.ps.proj_centre.pole)
							SETBIT(octet, E1_proj_flag_pole);
					iproj = (int) octet;
					(void) fprintf(stdout, "     GDB proj center = %d \n",
							iproj);
					(void) fprintf(stdout, "          (south pole) = %d \n",
							gribfld->Gdd.defn.ps.proj_centre.pole);
					octet = 0;
					if (gribfld->Gdd.defn.ps.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.ps.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.ps.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "          (negative x) = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.west);
					(void) fprintf(stdout, "          (positive y) = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.north);
					(void) fprintf(stdout, "          (by column)  = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.horz_sweep);
					break;
				case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
					(void) fprintf(stdout, "     GDB Nx          = %d \n",
							gribfld->Gdd.defn.lambert.Nx);
					(void) fprintf(stdout, "     GDB Ny          = %d \n",
							gribfld->Gdd.defn.lambert.Ny);
					(void) fprintf(stdout, "     GDB La1         = %d \n",
							gribfld->Gdd.defn.lambert.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.lambert.Lo1);
					(void) fprintf(stdout, "     GDB compnt      = %d \n",
							gribfld->Gdd.defn.lambert.compnt);
					(void) fprintf(stdout, "     GDB LoV         = %d \n",
							gribfld->Gdd.defn.lambert.LoV);
					(void) fprintf(stdout, "     GDB Dx          = %d \n",
							gribfld->Gdd.defn.lambert.Dx);
					(void) fprintf(stdout, "     GDB Dy          = %d \n",
							gribfld->Gdd.defn.lambert.Dy);
					octet = 0;
					if (gribfld->Gdd.defn.lambert.proj_centre.pole)
							SETBIT(octet, E1_proj_flag_pole);
					if (gribfld->Gdd.defn.lambert.proj_centre.bipolar)
							SETBIT(octet, E1_proj_flag_bipolar);
					iproj = (int) octet;
					(void) fprintf(stdout, "     GDB proj center = %d \n",
							iproj);
					(void) fprintf(stdout, "          (south pole) = %d \n",
							gribfld->Gdd.defn.lambert.proj_centre.pole);
					(void) fprintf(stdout, "          (bi-polar)   = %d \n",
							gribfld->Gdd.defn.lambert.proj_centre.bipolar);
					octet = 0;
					if (gribfld->Gdd.defn.lambert.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.lambert.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.lambert.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "          (negative x) = %d \n",
							gribfld->Gdd.defn.lambert.scan_mode.west);
					(void) fprintf(stdout, "          (positive y) = %d \n",
							gribfld->Gdd.defn.lambert.scan_mode.north);
					(void) fprintf(stdout, "          (by column)  = %d \n",
							gribfld->Gdd.defn.lambert.scan_mode.horz_sweep);
					(void) fprintf(stdout, "     GDB Latin1      = %d \n",
							gribfld->Gdd.defn.lambert.Latin1);
					(void) fprintf(stdout, "     GDB Latin2      = %d \n",
							gribfld->Gdd.defn.lambert.Latin2);
					break;
				case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
					(void) fprintf(stdout, "     GDB Ni          = %d \n",
							gribfld->Gdd.defn.rotate_ll.Ni);
					(void) fprintf(stdout, "     GDB Nj          = %d \n",
							gribfld->Gdd.defn.rotate_ll.Nj);
					(void) fprintf(stdout, "     GDB La1         = %d \n",
							gribfld->Gdd.defn.rotate_ll.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.rotate_ll.Lo1);
					(void) fprintf(stdout, "     GDB resolution  = %d \n",
							gribfld->Gdd.defn.rotate_ll.resltn);
					(void) fprintf(stdout, "     GDB La2         = %d \n",
							gribfld->Gdd.defn.rotate_ll.La2);
					(void) fprintf(stdout, "     GDB Lo2         = %d \n",
							gribfld->Gdd.defn.rotate_ll.Lo2);
					(void) fprintf(stdout, "     GDB Di          = %d \n",
							gribfld->Gdd.defn.rotate_ll.Di);
					(void) fprintf(stdout, "     GDB Dj          = %d \n",
							gribfld->Gdd.defn.rotate_ll.Dj);
					octet = 0;
					if (gribfld->Gdd.defn.rotate_ll.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.rotate_ll.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.rotate_ll.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "            (west)     = %d \n",
							gribfld->Gdd.defn.rotate_ll.scan_mode.west);
					(void) fprintf(stdout, "            (north)    = %d \n",
							gribfld->Gdd.defn.rotate_ll.scan_mode.north);
					(void) fprintf(stdout, "        (by longitude) = %d \n",
							gribfld->Gdd.defn.rotate_ll.scan_mode.horz_sweep);
					(void) fprintf(stdout, "     GDB LaP         = %d \n",
							gribfld->Gdd.defn.rotate_ll.LaP);
					(void) fprintf(stdout, "     GDB LoP         = %d \n",
							gribfld->Gdd.defn.rotate_ll.LoP);
					(void) fprintf(stdout, "     GDB AngR        = %f \n",
							gribfld->Gdd.defn.rotate_ll.AngR);
					(void) fprintf(stdout, "     GDB thin mode   = %d \n",
							gribfld->Gdd.defn.rotate_ll.thin_mode);
					if ( gribfld->Gdd.defn.rotate_ll.thin_mode > 0 )
						{
						if ( gribfld->Gdd.defn.rotate_ll.thin_mode == 1 )
							ii = gribfld->Gdd.defn.rotate_ll.Nj;
						else if ( gribfld->Gdd.defn.rotate_ll.thin_mode == 2 )
							ii = gribfld->Gdd.defn.rotate_ll.Ni;
						else
							ii = 0;
						(void) fprintf(stdout, "     GDB thin points = ");
						for ( count=0, jj=0; jj<ii; jj++ )
							{
							if (++count > MaxCount)
								{
								count = 1;
								(void) fprintf(stdout, "\n                       ");
								}
							(void) fprintf(stdout, "%3d ",
									gribfld->Gdd.defn.rotate_ll.thin_pts[jj]);
							}
						(void) fprintf(stdout, "\n");
						}
					(void) fprintf(stdout, "     GDB pole extra  = %d \n",
							gribfld->Gdd.defn.rotate_ll.pole_extra);
					(void) fprintf(stdout, "     GDB long extra  = %d \n",
							gribfld->Gdd.defn.rotate_ll.meridian_extra);
					break;
				default :
					break;
				}
			}

		/* Print Edition 1 Block 3 information */
		if ( DebugBlock3 )
			{
			if ( gribfld->Pdd.block_flags.bit_map )
				{
				(void) fprintf(stdout, "\n   Block 3:\n");
				(void) fprintf(stdout, "     BMB length      = %d \n",
						gribfld->Bmhd.length);
				(void) fprintf(stdout, "     BMB unused      = %d \n",
						gribfld->Bmhd.unused);
				(void) fprintf(stdout, "     BMB ntable      = %d \n",
						gribfld->Bmhd.ntable);

				switch(gribfld->Gdd.dat_rep)
					{
					case LATLON_GRID:			/* LATITUDE-LONGITUDE */
						ilatlon = 1;
						ni = gribfld->Gdd.defn.reg_ll.Ni;
						nj = gribfld->Gdd.defn.reg_ll.Nj;
						isweep = gribfld->Gdd.defn.reg_ll.scan_mode.horz_sweep;
						ipole  = gribfld->Gdd.defn.reg_ll.pole_extra;
						break;
					case GAUSS_GRID:			/* GAUSSIAN */
						ilatlon = 1;
						ni = gribfld->Gdd.defn.guas_ll.Ni;
						nj = gribfld->Gdd.defn.guas_ll.Nj;
						isweep = gribfld->Gdd.defn.guas_ll.scan_mode.horz_sweep;
						ipole  = 0;
						break;
					case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
						ilatlon = 1;
						ni = gribfld->Gdd.defn.rotate_ll.Ni;
						nj = gribfld->Gdd.defn.rotate_ll.Nj;
						isweep = gribfld->Gdd.defn.rotate_ll.scan_mode.horz_sweep;
						ipole  = gribfld->Gdd.defn.rotate_ll.pole_extra;
						break;
					case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
						ilatlon = 0;
						ni = gribfld->Gdd.defn.ps.Nx;
						nj = gribfld->Gdd.defn.ps.Ny;
						isweep = gribfld->Gdd.defn.ps.scan_mode.horz_sweep;
						ipole  = 0;
						break;
					case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
						ilatlon = 0;
						ni = gribfld->Gdd.defn.lambert.Nx;
						nj = gribfld->Gdd.defn.lambert.Ny;
						isweep = gribfld->Gdd.defn.lambert.scan_mode.horz_sweep;
						ipole  = 0;
						break;
					default :
						ilatlon = -1;
						break;
					}

				if ( ilatlon == -1 ) break;

				if ( ipole != 0 )
					{
					(void) fprintf(stdout, "\n   Bit map - Pole Bit\n");
					if ( gribfld->PoleBit ) (void) fprintf(stdout, "     B\n");
					else                    (void) fprintf(stdout, "     .\n");
					}

				/* Note that data may be ordered either by column or by row, */
				/*  but is printed out in ni columns for each of nj rows     */
				(void) fprintf(stdout, "\n   Bit map -");
				if ( ilatlon == 1 )
					{
					(void) fprintf(stdout, "  %d Longitudes for each of", ni);
					(void) fprintf(stdout, "  %d Latitudes", nj);
					}
				else
					{
					(void) fprintf(stdout, "  %d Columns for each of", ni);
					(void) fprintf(stdout, "  %d Rows", nj);
					}

				gbits = gribfld->PBit;
				for ( jj=0; jj<nj; jj++ )
					{
					(void) fprintf(stdout, "\n");
					for ( ii=0; ii<ni; ii++ )
						{
						if ( isweep == 0 ) gbit = gbits[jj*ni + ii];
						else               gbit = gbits[ii*nj + jj];

						if ( gbit ) (void) fprintf(stdout, "B");
						else        (void) fprintf(stdout, ".");
						}
					}
				(void) fprintf(stdout, "\n");
				}
			}

		/* Print Edition 1 Block 4 information */
		if ( DebugBlock4 )
			{
			(void) fprintf(stdout, "\n   Block 4:\n");
			(void) fprintf(stdout, "     BDB length      = %d \n",
					gribfld->Bdhd.length);
			(void) fprintf(stdout, "     BDB flags       = %d \n",
					gribfld->Bdhd.flags);
			(void) fprintf(stdout, "     BDB unused      = %d \n",
					gribfld->Bdhd.unused);
			(void) fprintf(stdout, "     BDB scale       = %d \n",
					gribfld->Bdhd.scale);
			(void) fprintf(stdout, "     BDB reference   = %f \n",
					gribfld->Bdhd.reference);
			(void) fprintf(stdout, "     BDB bits/value  = %d \n",
					gribfld->Bdhd.bits_per_val);
			}

		/* Print Edition 1 Raw GRIB data */
		if ( DebugB4Raw )
			{
			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:			/* LATITUDE-LONGITUDE */
					ipole = gribfld->Gdd.defn.reg_ll.pole_extra;
					break;
				case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
					ipole = gribfld->Gdd.defn.rotate_ll.pole_extra;
					break;
				default :
					ipole = 0;
					break;
				}

			if ( ipole != 0 )
				{
				(void) fprintf(stdout, "\n   Raw GRIB data - Pole Datum\n");
				(void) fprintf(stdout, "%10.2f\n", gribfld->PoleDatum);
				}

			(void) fprintf(stdout, "\n   Raw GRIB data -");
			(void) fprintf(stdout, "  %d Data values\n", gribfld->NumRaw);
			gvals = gribfld->PRaw;
			for ( count=0, ii=0; ii<gribfld->NumRaw; ii++ )
				{
				if ( ++count > MaxCount )
					{
					count = 1;
					(void) fprintf(stdout, "\n");
					}
				(void) fprintf(stdout, "%10.2f ", *gvals++);
				}
			(void) fprintf(stdout, "\n");
			}

		/* Print Edition 1 Processed GRIB data */
		if ( DebugB4Data )
			{
			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:			/* LATITUDE-LONGITUDE */
				case GAUSS_GRID:			/* GAUSSIAN */
				case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE */
					ilatlon = 1;
					break;
				case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
				case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */
					ilatlon = 0;
					break;
				default :
					ilatlon = 0;
					break;
				}

			/* Note that data is ordered by Nii columns in each of Njj rows */
			(void) fprintf(stdout, "\n   Processed GRIB data -");
			if ( ilatlon == 1 )
				{
				(void) fprintf(stdout, "  %d Longitudes for each of",
						gribfld->Nii);
				(void) fprintf(stdout, "  %d Latitudes", gribfld->Njj);
				}
			else
				{
				(void) fprintf(stdout, "  %d Columns for each of",
						gribfld->Nii);
				(void) fprintf(stdout, "  %d Rows", gribfld->Njj);
				}

			gvals = gribfld->PData;
			for ( jj=0; jj<gribfld->Njj; jj++ )
				{
				(void) fprintf(stdout, "\n");
				for ( count=0, ii=0; ii<gribfld->Nii; ii++ )
					{
					if ( ++count > MaxCount )
						{
						count = 1;
						(void) fprintf(stdout, "\n");
						}
					(void) fprintf(stdout, "%10.2f ", *gvals++);
					}
				}
			(void) fprintf(stdout, "\n");
			}

		/***** Testing for  gribfield_to_metafile  *****/
		if ( gribfield_identifiers_edition1(&model, &rtime, &btime, &etime,
				&element, &level, &units) && DebugMetaff )
			{
			(void) replace_grib_default_model(&model);
			(void) replace_grib_default_element(&element, &units);

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
								FpaF_ELEMENT,	components->comp_edefs[iout],
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

	/* Close GRIB file for Edition 1 decode */
	if ( argv3 == 1 ) (void) close_gribfile_edition1();

	/* Exit if Edition 1 decode processed one or more fields */
	if ( nflds > 0 )
		{
		(void) fprintf(stdout, "\n=======================================");
		(void) fprintf(stdout, "\n== Total Edition 1 GRIB fields: %4i ==",
				nflds);
		(void) fprintf(stdout, "\n=======================================\n");
		return 0;
		}

	/* Testing GRIB Edition 0 decode                           */
	/* Read GRIB fields until end-of-file or error encountered */
	while ( argv3 == 0 )
		{

		/* Open GRIB file for first field */
		if ( nflds == 0 )
			{
			(void) fprintf(stdout, "\n===================================");
			(void) fprintf(stdout, "\n== Testing GRIB Edition 0 decode ==");
			(void) fprintf(stdout, "\n===================================\n");

			/* Open GRIB file for Edition 0 decode */
			if ( !open_gribfile_edition0(gribname) )
				{
				(void) fprintf(stderr, "\n[gribtest]");
				(void) fprintf(stderr, " Error opening GRIB file %s\n",
						gribname);
				return (-10);
				}
			}

		/* Process each GRIB field */
		(void) fprintf(stdout, "\n===================================\n");
		if ( !next_gribfield_edition0(&gribfld) ) break;
		nflds++;

		/* Check for maximum number of fields */
		if ( MaxFlds > 0 && nflds > MaxFlds )
			{
			nflds--;
			break;
			}

		/* Print the identifiers */
		(void) fprintf(stdout, "\n  Information from GRIB field: %i\n", nflds);
		if ( gribfield_identifiers_edition0(&model, &rtime, &btime, &etime,
				&element, &level, &units) )
			{
			(void) fprintf(stdout, "\n   Field Identifiers:\n");
			(void) fprintf(stdout, "     model           = %s \n", model);
			(void) fprintf(stdout, "     run time        = %s \n", rtime);
			(void) fprintf(stdout, "     begin valid time= %s \n", btime);
			(void) fprintf(stdout, "     end valid time  = %s \n", etime);
			(void) fprintf(stdout, "     element         = %s \n", element);
			(void) fprintf(stdout, "     level           = %s \n", level);
			(void) fprintf(stdout, "     units           = %s \n", units);
			}

		/* Print Edition 0 Block 0 information */
		if ( DebugBlock0 )
			{
			(void) fprintf(stdout, "\n   Block 0:\n");
			(void) fprintf(stdout, "     Edition number  = %d \n",
					gribfld->Isb.edition);
			}

		/* Print Edition 0 Block 1 information */
		if ( DebugBlock1 )
			{
			(void) fprintf(stdout, "\n   Block 1:\n");
			(void) fprintf(stdout, "     PDB length      = %d \n",
					gribfld->Pdd.length);
			(void) fprintf(stdout, "     PDB edition     = %d \n",
					gribfld->Pdd.edition);
			(void) fprintf(stdout, "     PDB center      = %d \n",
					gribfld->Pdd.centre_id);
			(void) fprintf(stdout, "     PDB model_id    = %d \n",
					gribfld->Pdd.model_id);
			(void) fprintf(stdout, "     PDB grid_defn   = %d \n",
					gribfld->Pdd.grid_defn);
			octet = 0;
			if (gribfld->Pdd.block_flags.grid_description)
					SETBIT(octet, E1_block_flag_grid_desc);
			if (gribfld->Pdd.block_flags.bit_map)
					SETBIT(octet, E1_block_flag_bit_map);
			iflags = (int) octet;
			(void) fprintf(stdout, "     PDB block_flags = %d \n",
					iflags);
			(void) fprintf(stdout, "     PDB parameter   = %d \n",
					gribfld->Pdd.parameter);
			(void) fprintf(stdout, "     PDB level type  = %d \n",
					gribfld->Pdd.layer.type);
			(void) fprintf(stdout, "     PDB layer.top   = %d \n",
					gribfld->Pdd.layer.top);
			(void) fprintf(stdout, "     PDB layer.bottom= %d \n",
					gribfld->Pdd.layer.bottom);
			(void) fprintf(stdout, "     PDB year        = %d \n",
					gribfld->Pdd.forecast.reference.year);
			(void) fprintf(stdout, "     PDB month       = %d \n",
					gribfld->Pdd.forecast.reference.month);
			(void) fprintf(stdout, "     PDB day         = %d \n",
					gribfld->Pdd.forecast.reference.day);
			(void) fprintf(stdout, "     PDB hour        = %d \n",
					gribfld->Pdd.forecast.reference.hour);
			(void) fprintf(stdout, "     PDB minutes     = %d \n",
					gribfld->Pdd.forecast.reference.minute);
			(void) fprintf(stdout, "     PDB unit        = %d \n",
					gribfld->Pdd.forecast.units);
			(void) fprintf(stdout, "     PDB time1       = %d \n",
					gribfld->Pdd.forecast.time1);
			(void) fprintf(stdout, "     PDB time2       = %d \n",
					gribfld->Pdd.forecast.time2);
			(void) fprintf(stdout, "     PDB range_type  = %d \n",
					gribfld->Pdd.forecast.range_type);
			(void) fprintf(stdout, "     PDB no. average = %d \n",
					gribfld->Pdd.forecast.nb_averaged);
			(void) fprintf(stdout, "     PDB no. missing = %d \n",
					gribfld->Pdd.forecast.nb_missing);
			(void) fprintf(stdout, "     PDB century     = %d \n",
					gribfld->Pdd.forecast.century);
			(void) fprintf(stdout, "     PDB reserved    = %d \n",
					gribfld->Pdd.forecast.reserved);
			(void) fprintf(stdout, "     PDB factor_d    = %d \n",
					gribfld->Pdd.forecast.factor_d);
			}

		/* Print Edition 0 Block 2 information */
		if ( DebugBlock2 )
			{
			(void) fprintf(stdout, "\n   Block 2:\n");
			(void) fprintf(stdout, "     GDB length      = %d \n",
					gribfld->Gdd.length);
			(void) fprintf(stdout, "     GDB Nv          = %d \n",
					gribfld->Gdd.nv);
			(void) fprintf(stdout, "     GDB pv or pl    = %d \n",
					gribfld->Gdd.pv_or_pl);
			(void) fprintf(stdout, "     GDB grid type   = %d \n",
					gribfld->Gdd.dat_rep);

			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:			/* LATITUDE-LONGITUDE */
					(void) fprintf(stdout, "     GDB Ni          = %d \n",
							gribfld->Gdd.defn.reg_ll.Ni);
					(void) fprintf(stdout, "     GDB Nj          = %d \n",
							gribfld->Gdd.defn.reg_ll.Nj);
					(void) fprintf(stdout, "     GDB la1         = %d \n",
							gribfld->Gdd.defn.reg_ll.La1);
					(void) fprintf(stdout, "     GDB lo1         = %d \n",
							gribfld->Gdd.defn.reg_ll.Lo1);
					(void) fprintf(stdout, "     GDB resolution  = %d \n",
							gribfld->Gdd.defn.reg_ll.resltn);
					(void) fprintf(stdout, "     GDB la2         = %d \n",
							gribfld->Gdd.defn.reg_ll.La2);
					(void) fprintf(stdout, "     GDB lo2         = %d \n",
							gribfld->Gdd.defn.reg_ll.Lo2);
					(void) fprintf(stdout, "     GDB Di          = %d \n",
							gribfld->Gdd.defn.reg_ll.Di);
					(void) fprintf(stdout, "     GDB Dj          = %d \n",
							gribfld->Gdd.defn.reg_ll.Dj);
					octet = 0;
					if (gribfld->Gdd.defn.reg_ll.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.reg_ll.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.reg_ll.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "            (west)     = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.west);
					(void) fprintf(stdout, "            (north)    = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.north);
					(void) fprintf(stdout, "        (by longitude) = %d \n",
							gribfld->Gdd.defn.reg_ll.scan_mode.horz_sweep);
					(void) fprintf(stdout, "     GDB pole extra  = %d \n",
							gribfld->Gdd.defn.reg_ll.pole_extra);
					break;
				case GAUSS_GRID:			/* GAUSSIAN */
					(void) fprintf(stdout, "     GDB Ni          = %d \n",
							gribfld->Gdd.defn.guas_ll.Ni);
					(void) fprintf(stdout, "     GDB Nj          = %d \n",
							gribfld->Gdd.defn.guas_ll.Nj);
					(void) fprintf(stdout, "     GDB la1         = %d \n",
							gribfld->Gdd.defn.guas_ll.La1);
					(void) fprintf(stdout, "     GDB lo1         = %d \n",
							gribfld->Gdd.defn.guas_ll.Lo1);
					(void) fprintf(stdout, "     GDB resolution  = %d \n",
							gribfld->Gdd.defn.guas_ll.resltn);
					(void) fprintf(stdout, "     GDB la2         = %d \n",
							gribfld->Gdd.defn.guas_ll.La2);
					(void) fprintf(stdout, "     GDB lo2         = %d \n",
							gribfld->Gdd.defn.guas_ll.Lo2);
					(void) fprintf(stdout, "     GDB Di          = %d \n",
							gribfld->Gdd.defn.guas_ll.Di);
					(void) fprintf(stdout, "     GDB N           = %d \n",
							gribfld->Gdd.defn.guas_ll.N);
					octet = 0;
					if (gribfld->Gdd.defn.guas_ll.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.guas_ll.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.guas_ll.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "            (west)     = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.west);
					(void) fprintf(stdout, "            (north)    = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.north);
					(void) fprintf(stdout, "        (by longitude) = %d \n",
							gribfld->Gdd.defn.guas_ll.scan_mode.horz_sweep);
					break;
				case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
					(void) fprintf(stdout, "     GDB Nx          = %d \n",
							gribfld->Gdd.defn.ps.Nx);
					(void) fprintf(stdout, "     GDB Ny          = %d \n",
							gribfld->Gdd.defn.ps.Ny);
					(void) fprintf(stdout, "     GDB la1         = %d \n",
							gribfld->Gdd.defn.ps.La1);
					(void) fprintf(stdout, "     GDB Lo1         = %d \n",
							gribfld->Gdd.defn.ps.Lo1);
					(void) fprintf(stdout, "     GDB compnt      = %d \n",
							gribfld->Gdd.defn.ps.compnt);
					(void) fprintf(stdout, "     GDB LoV         = %d \n",
							gribfld->Gdd.defn.ps.LoV);
					(void) fprintf(stdout, "     GDB Dx          = %d \n",
							gribfld->Gdd.defn.ps.Dx);
					(void) fprintf(stdout, "     GDB Dy          = %d \n",
							gribfld->Gdd.defn.ps.Dy);
					octet = 0;
					if (gribfld->Gdd.defn.ps.proj_centre.pole)
							SETBIT(octet, E1_proj_flag_pole);
					iproj = (int) octet;
					(void) fprintf(stdout, "     GDB proj center = %d \n",
							iproj);
					(void) fprintf(stdout, "          (south pole) = %d \n",
							gribfld->Gdd.defn.ps.proj_centre.pole);
					octet = 0;
					if (gribfld->Gdd.defn.ps.scan_mode.west)
							SETBIT(octet, E1_scan_flag_west);
					if (gribfld->Gdd.defn.ps.scan_mode.north)
							SETBIT(octet, E1_scan_flag_north);
					if (gribfld->Gdd.defn.ps.scan_mode.horz_sweep)
							SETBIT(octet, E1_scan_flag_hsweep);
					icode = (int) octet;
					(void) fprintf(stdout, "     GDB scan mode   = %d \n",
							icode);
					(void) fprintf(stdout, "          (negative x) = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.west);
					(void) fprintf(stdout, "          (positive y) = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.north);
					(void) fprintf(stdout, "          (by column)  = %d \n",
							gribfld->Gdd.defn.ps.scan_mode.horz_sweep);
					break;
				default :
					break;
				}
			}

		/* Print Edition 0 Block 4 information */
		if ( DebugBlock4 )
			{
			(void) fprintf(stdout, "\n   Block 4:\n");
			(void) fprintf(stdout, "     BDB length      = %d \n",
					gribfld->Bdhd.length);
			(void) fprintf(stdout, "     BDB flags       = %d \n",
					gribfld->Bdhd.flags);
			(void) fprintf(stdout, "     BDB unused      = %d \n",
					gribfld->Bdhd.unused);
			(void) fprintf(stdout, "     BDB scale       = %d \n",
					gribfld->Bdhd.scale);
			(void) fprintf(stdout, "     BDB reference   = %f \n",
					gribfld->Bdhd.reference);
			(void) fprintf(stdout, "     BDB bits/value  = %d \n",
					gribfld->Bdhd.bits_per_val);
			}

		/* Print Edition 0 Raw GRIB data */
		if ( DebugB4Raw )
			{
			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:		/* LATITUDE-LONGITUDE */
					ipole = gribfld->Gdd.defn.reg_ll.pole_extra;
					break;
				default :
					ipole = 0;
					break;
				}
			if ( ipole != 0 )
				{
				(void) fprintf(stdout, "\n   Raw GRIB data - Pole Datum\n");
				(void) fprintf(stdout, "%10.2f\n", gribfld->PoleDatum);
				}
			(void) fprintf(stdout, "\n   Raw GRIB data -");
			(void) fprintf(stdout, "  %d Data values\n", gribfld->NumRaw);
			gvals = gribfld->PRaw;
			for ( count=0, ii=0; ii<gribfld->NumRaw; ii++ )
				{
				if ( ++count > MaxCount )
					{
					count = 1;
					(void) fprintf(stdout, "\n");
					}
				(void) fprintf(stdout, "%10.2f ", *gvals++);
				}
			(void) fprintf(stdout, "\n");
			}

		/* Print Edition 0 Processed GRIB data */
		if ( DebugB4Data )
			{
			(void) fprintf(stdout, "\n   Processed GRIB data -");
			switch(gribfld->Gdd.dat_rep)
				{
				case LATLON_GRID:			/* LATITUDE-LONGITUDE */
				case GAUSS_GRID:			/* GAUSSIAN */
					ilatlon = 1;
					break;
				case PSTEREO_GRID:			/* POLAR STERIOGRAPHIC */
					ilatlon = 0;
					break;
				default :
					ilatlon = 0;
					break;
				}

			/* Note that data is ordered by Nii columns in each of Njj rows */
			if ( ilatlon == 1 )
				{
				(void) fprintf(stdout, "  %d Longitudes for each of",
						gribfld->Nii);
				(void) fprintf(stdout, "  %d Latitudes", gribfld->Njj);
				}
			else
				{
				(void) fprintf(stdout, "  %d Columns for each of",
						gribfld->Nii);
				(void) fprintf(stdout, "  %d Rows", gribfld->Njj);
				}

			gvals = gribfld->PData;
			for ( jj=0; jj<gribfld->Njj; jj++ )
				{
				(void) fprintf(stdout, "\n");
				for ( count=0, ii=0; ii<gribfld->Nii; ii++ )
					{
					if ( ++count > MaxCount )
						{
						count = 1;
						(void) fprintf(stdout, "\n");
						}
					(void) fprintf(stdout, "%10.2f ", *gvals++);
					}
				}
			(void) fprintf(stdout, "\n");
			}

		/***** Testing for  gribfield_to_metafile  *****/
		if ( gribfield_identifiers_edition0(&model, &rtime, &btime, &etime,
				&element, &level, &units) && DebugMetaff )
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
								FpaF_ELEMENT,	components->comp_edefs[iout],
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

	/* Close GRIB file for Edition 0 decode */
	if ( argv3 == 0 ) (void) close_gribfile_edition0();

	/* Exit if Edition 0 decode processed one or more fields */
	if ( nflds > 0 )
		{
		(void) fprintf(stdout, "\n=======================================");
		(void) fprintf(stdout, "\n== Total Edition 0 GRIB fields: %4i ==",
				nflds);
		(void) fprintf(stdout, "\n=======================================\n");
		return 0;
		}

	return (-99);
	}
