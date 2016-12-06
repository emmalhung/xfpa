/***********************************************************************
*                                                                      *
*     f p a g p g e n . c                                              *
*                                                                      *
*     Main routine for FpaGPgen graphics product generator application *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#define  FPAGPGEN_MAIN

#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

/* Define FpaGPgen program types */
static const PROGRAM_INFO GPGprogramTypes[] =
	{
		{ "psmet",      GPG_PSMet,   "psmet",   "psout",
										"Version 8.1 PSMet",    60.0,  100.0 },
		{ "svgmet",    GPG_SVGMet,   "svgmet",   "svgout",
										"Version 8.1 SVGMet",   60.0,  100.0 },
		{ "cormet",    GPG_CorMet,  "cormet",  "corout",
										"Version 8.1 CorMet",   60.0,  100.0 },
		{ "texmet",    GPG_TexMet,  "texmet",  "texout",
										"Version 8.1 TexMet",  100.0,  100.0 },
		{ "fpagpgen",   GPG_PSMet,   "psmet",   "psout",
										"Version 8.1 PSMet",    60.0,  100.0 },
		/* >>> For alternate "default" versions of fpagpgen ...
		{ "fpagpgen",  GPG_SVGMet,   "svgmet",   "svgout",
										"Version 8.1 SVGMet",   60.0,  100.0 },
		{ "fpagpgen",  GPG_CorMet,  "cormet",  "corout",
										"Version 8.1 CorMet",   60.0,  100.0 },
		{ "fpagpgen",  GPG_TexMet,  "texmet",  "texout",
										"Version 8.1 TexMet",  100.0,  100.0 },
		<<< */
	};

/* Set number of predefined program types */
static const int NumGPGprogramTypes =
	(int) (sizeof(GPGprogramTypes) / sizeof(PROGRAM_INFO));

/* Trap for error situations */
static	void	error_trap(int);

/* Default message string */
static	char	MyLabel[GPGLong];


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
	int				status, dlevel, dstyle, np, nslist;
	int				cyear, cjday, cmonth, cmday, chour, cmin, csec;
	STRING			rname, pname, sfile, *slist, vtime;
	MAP_PROJ		*mproj;
	STRING			pdf_file;

	static	char	PdfFile[GPGLong] = FpaCblank;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set default diagnostic mode */
	dlevel = 3;
	dstyle = 2;
	(void) diag_control(FALSE, dlevel, dstyle);

	/* Set run parameters based on program name */
	/* Check program name against each label in the GPGprogramTypes list */
	rname = base_name(argv[0], NullString);
	for ( np=0; np<NumGPGprogramTypes; np++ )
		{
		if ( same_ic(rname, GPGprogramTypes[np].program) )
			{
			Program = GPGprogramTypes[np];
			break;
			}
		}
	if ( np >= NumGPGprogramTypes )
		{
		(void) fprintf(stderr, "Error in program name \"%s\"\n", rname);
		(void) fprintf(stderr, "  Allowed programs are:");
		for ( np=0; np<NumGPGprogramTypes; np++ )
			{
			(void) fprintf(stderr, "  \"%s\"", GPGprogramTypes[np].program);
			}
		(void) fprintf(stderr, "\n");
		(void) fprintf(stdout, "Abort due to error in program name \"%s\"\n",
				rname);
		return (-1);
		}
	pname = strdup(GPGprogramTypes[np].program);

	/* Validate run string parameters */
	if ( argc != 5 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   %s <setup_file> <%s_sub_directory>",
				pname, pname);
		(void) fprintf(stderr, " <pdf_filename> <run_time>\n");
		(void) fprintf(stderr, "\n     <pdf_filename> does not need the");
		(void) fprintf(stderr, "  .fpdf  extension\n");
		(void) fprintf(stderr, "\n     <run_time> has the format YYYY:DDD:HH\n");
		(void) fprintf(stderr, "        where YYYY is the year\n");
		(void) fprintf(stderr, "              DDD  is the julian day\n");
		(void) fprintf(stderr, "              HH   is the hour of day\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("product.graphic");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Force allocation of patches only when necessary   */
	/* (the mode used in xfpa may cause memory problems) */
	(void) set_MMM(MMM_AllocateWhenNeeded);

	/* Set program pointers */
	switch ( Program.macro )
		{
		case GPG_PSMet:
			initialize_graphics_display       = initialize_psmet_display;
			close_graphics_file               = close_psmet_file;
			initialize_graphics_size          = initialize_psmet_size;
			write_graphics_comment            = write_psmet_comment;
			write_graphics_group              = write_psmet_group;
			write_graphics_bitmap             = write_psmet_bitmap;
			write_graphics_image              = write_psmet_image;
			write_graphics_box                = write_psmet_box;
			write_graphics_ellipse            = write_psmet_ellipse;
			write_graphics_underline          = write_psmet_underline;
			write_graphics_text               = write_psmet_text;
			write_graphics_lines              = write_psmet_lines;
			write_graphics_outlines           = write_psmet_outlines;
			write_graphics_boundaries         = write_psmet_boundaries;
			write_graphics_features           = write_psmet_features;
			write_graphics_symbol             = write_psmet_symbol;
			write_graphics_outline_mask       = write_psmet_outline_mask;
			write_graphics_boundary_mask      = write_psmet_boundary_mask;
			graphics_symbol_size              = psmet_symbol_size;
			default_graphics_presentation     = default_psmet_presentation;
			define_graphics_units             = define_psmet_units;
			define_graphics_placement         = define_psmet_placement;
			define_graphics_anchor            = define_psmet_anchor;
			check_graphics_keyword            = check_psmet_keyword;
			process_graphics_directive        = process_psmet_directive;
			break;
		case GPG_SVGMet:
			initialize_graphics_display       = initialize_svgmet_display;
			close_graphics_file               = close_svgmet_file;
			initialize_graphics_size          = initialize_svgmet_size;
			write_graphics_comment            = write_svgmet_comment;
			write_graphics_group              = write_svgmet_group;
			write_graphics_bitmap             = write_svgmet_bitmap;
			write_graphics_image              = write_svgmet_image;
			write_graphics_box                = write_svgmet_box;
			write_graphics_ellipse            = write_svgmet_ellipse;
			write_graphics_underline          = write_svgmet_underline;
			write_graphics_text               = write_svgmet_text;
			write_graphics_lines              = write_svgmet_lines;
			write_graphics_outlines           = write_svgmet_outlines;
			write_graphics_boundaries         = write_svgmet_boundaries;
			write_graphics_features           = write_svgmet_features;
			write_graphics_symbol             = write_svgmet_symbol;
			write_graphics_outline_mask       = write_svgmet_outline_mask;
			write_graphics_boundary_mask      = write_svgmet_boundary_mask;
			graphics_symbol_size              = svgmet_symbol_size;
			default_graphics_presentation     = default_svgmet_presentation;
			define_graphics_units             = define_svgmet_units;
			define_graphics_placement         = define_svgmet_placement;
			define_graphics_anchor            = define_svgmet_anchor;
			check_graphics_keyword            = check_svgmet_keyword;
			process_graphics_directive        = process_svgmet_directive;
			break;
		case GPG_CorMet:
			initialize_graphics_display       = initialize_cormet_display;
			close_graphics_file               = close_cormet_file;
			initialize_graphics_size          = initialize_cormet_size;
			write_graphics_comment            = write_cormet_comment;
			write_graphics_group              = write_cormet_group;
			write_graphics_bitmap             = write_cormet_bitmap;
			write_graphics_image              = 0;
			write_graphics_box                = write_cormet_box;
			write_graphics_ellipse            = write_cormet_ellipse;
			write_graphics_underline          = write_cormet_underline;
			write_graphics_text               = write_cormet_text;
			write_graphics_lines              = write_cormet_lines;
			write_graphics_outlines           = write_cormet_outlines;
			write_graphics_boundaries         = write_cormet_boundaries;
			write_graphics_features           = write_cormet_features;
			write_graphics_symbol             = write_cormet_symbol;
			write_graphics_outline_mask       = write_cormet_outline_mask;
			write_graphics_boundary_mask      = write_cormet_boundary_mask;
			graphics_symbol_size              = cormet_symbol_size;
			default_graphics_presentation     = default_cormet_presentation;
			define_graphics_units             = define_cormet_units;
			define_graphics_placement         = define_cormet_placement;
			define_graphics_anchor            = define_cormet_anchor;
			check_graphics_keyword            = check_cormet_keyword;
			process_graphics_directive        = process_cormet_directive;
			break;
		case GPG_TexMet:
			initialize_graphics_display       = initialize_texmet_display;
			close_graphics_file               = close_texmet_file;
			initialize_graphics_size          = initialize_texmet_size;
			write_graphics_comment            = write_texmet_comment;
			write_graphics_group              = write_texmet_group;
			write_graphics_bitmap             = 0;
			write_graphics_image              = 0;
			write_graphics_box                = 0;
			write_graphics_ellipse            = 0;
			write_graphics_underline          = 0;
			write_graphics_text               = write_texmet_text;
			write_graphics_lines              = 0;
			write_graphics_outlines           = 0;
			write_graphics_boundaries         = 0;
			write_graphics_features           = 0;
			write_graphics_symbol             = 0;
			write_graphics_outline_mask       = 0;
			write_graphics_boundary_mask      = 0;
			graphics_symbol_size              = 0;
			default_graphics_presentation     = default_texmet_presentation;
			define_graphics_units             = 0;
			define_graphics_placement         = 0;
			define_graphics_anchor            = define_texmet_anchor;
			check_graphics_keyword            = check_texmet_keyword;
			process_graphics_directive        = process_texmet_directive;
			break;
		}

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), Program.label);
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);
	(void) fprintf(stdout, "\n Run String: \"%s  %s  %s  %s  %s\"\n\n",
			rname, argv[1], argv[2], argv[3], argv[4]);

	/* Set creation time (rounded to nearest minute) */
	cmin += NINT((float) csec / 60.0);
	(void) tnorm(&cyear, &cjday, &chour, &cmin, NullInt);
	vtime = build_tstamp(cyear, cjday, chour, cmin, FALSE, TRUE);
	(void) safe_strcpy(TCstamp, vtime);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	sfile  = strdup(argv[1]);
	nslist = setup_files(sfile, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr, "%s Problem with setup file \"%s\"\n",
				MyLabel, sfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) safe_strcpy(SetupFile, sfile);

	/* Set diagnostic mode (through diag_control block in setup file) */
	(void) diag_control(True, -1, -1);

	/* Read the Config files */
	if ( !read_complete_config_file() )
		{
		(void) fprintf(stderr, "%s Problem with Config Files\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the target projection definition */
	mproj = get_target_map();
	if ( IsNull(mproj) )
		{
		(void) fprintf(stderr,
				"%s Target map not defined in setup file \"%s\"\n",
				MyLabel, sfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) copy_map_projection(&BaseMap, mproj);

	/* Print out the default projection, map, and grid definitions */
	(void) fprintf(stdout, "\n Default map projection ...");
	(void) fprintf(stdout, "  type: %s",
			which_projection_name(BaseMap.projection.type));
	(void) fprintf(stdout, "  ref[1-5]: %.1f %.1f %.1f %.1f %.1f\n",
			BaseMap.projection.ref[0], BaseMap.projection.ref[1],
			BaseMap.projection.ref[2], BaseMap.projection.ref[3],
			BaseMap.projection.ref[4]);
	(void) fprintf(stdout, " Default map definition ...");
	(void) fprintf(stdout, "  olat: %.1f  olon: %.1f  rlon: %.1f\n",
			BaseMap.definition.olat, BaseMap.definition.olon,
			BaseMap.definition.lref);
	(void) fprintf(stdout, "       xorg: %.0f  yorg: %.0f",
			BaseMap.definition.xorg, BaseMap.definition.yorg);
	(void) fprintf(stdout, "  xlen: %.0f  ylen: %.0f  map_units: %.0f\n",
			BaseMap.definition.xlen, BaseMap.definition.ylen,
			BaseMap.definition.units);
	(void) fprintf(stdout, " Default grid definition ...");
	(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
			BaseMap.grid.nx, BaseMap.grid.ny, BaseMap.grid.gridlen);
	(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
			BaseMap.grid.xgrid, BaseMap.grid.ygrid);
	(void) fprintf(stdout, "  map_units: %.0f\n",
			BaseMap.grid.units);

	/* Initialize the default field descriptor */
	(void) init_fld_descript(&Fdesc);
	(void) set_fld_descript(&Fdesc, FpaF_MAP_PROJECTION, mproj,
									FpaF_END_OF_LIST);

	/* Initialize run/valid time stamps */
	vtime = interpret_timestring(argv[4], NullString, 0.0);
	if ( IsNull(vtime) )
		{
		(void) fprintf(stderr,
				"%s Invalid T0 timestring \"%s\"\n",
				MyLabel, argv[4]);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) safe_strcpy(T0stamp, vtime);
	(void) safe_strcpy(TVstamp, vtime);

	/* Set the default source and time */
	(void) define_source("depict", "00");

	/* Initialize the default display */
	(void) initialize_graphics_display();

	/* Initialize the input and output directories */
	if ( !initialize_graphics_directories(argv[2], argv[3]) )
		{
		(void) fprintf(stderr, "%s Error in %s directories\n", MyLabel, pname);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Identify the base fpdf file */
	pdf_file = find_pdf_file(argv[3]);
	if ( blank(pdf_file) )
		{
		(void) fprintf(stderr, "%s Cannot find fpdf filename \"%s\"\n",
				MyLabel, argv[3]);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) strcpy(PdfFile, pdf_file);

	/* Process the fpdf file */
	if ( !process_pdf_file(PdfFile) )
		{
		(void) fprintf(stderr, "%s Error processing fpdf file \"%s\"\n",
				MyLabel, PdfFile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Close the ouput file */
	(void) close_graphics_file();

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);
	FREEMEM(pname);
	FREEMEM(sfile);

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
