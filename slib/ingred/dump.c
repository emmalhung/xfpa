/***********************************************************************
*                                                                      *
*     d u m p . c                                                      *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     manages the raster dump of depiction images.                     *
*                                                                      *
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

#include "ingred_private.h"

#include <sys/signal.h>
#include <sys/wait.h>

/* Define panel fill and edge colours */
#define TempBgnd NullBox, NullBox, NULL, NULL, NULL, NULL

/* Trap for debug alarm */
static void (*prevfcn)(int);
static void trapfcn(int);
/*ARGSUSED*/
static void trapfcn(int unused) {}

/***********************************************************************
*                                                                      *
*     r a s t e r _ d u m p                                            *
*                                                                      *
***********************************************************************/

LOGICAL	raster_dump

	(
	STRING	fmt,
	STRING	mode,
	STRING	vtime,
	int		width,
	int		height,
	STRING	pad,
	STRING	dfile,
	STRING	template
	)

	{
	pid_t		pid;
	int			nx, ny, ivt;
	float		sx, sy;
	BOX			window;
	int			fmode;
	enum		{ Colour, GreyScale, Floyd } dmode;
	enum		{ NoPad, Pad } pmode;
	LOGICAL		active;
	METAFILE	tmeta;
	DISPNODE	tdn;
	FIELD		tfld;
	SET			tset;
	LABEL		tlab;
	int			ifld, imem;
	long		tissue, tvalid;
	char		cbuf[256];
	void		(*prevaction)();

	/*******************************************************************
	*                                                                  *
	*  Interpret the request.                                          *
	*                                                                  *
	*******************************************************************/

	/* Interpret the format mode parameter */
	if      (same_ic(fmt, "XWD"))  fmode = glXWD;
	else if (same_ic(fmt, "XGL"))  fmode = glXGL;
	else if (same_ic(fmt, "GIF"))  fmode = glGIF;
#ifdef MACHINE_PCLINUX
	else if (same_ic(fmt, "PNG"))  fmode = glPNG;
	else if (same_ic(fmt, "TIF"))  fmode = glTIFF;
	else if (same_ic(fmt, "TIFF")) fmode = glTIFF;
#endif
	else
		{
		(void) fprintf(stderr, "[raster_dump] Unsupported format: %s\n", fmt);
		return FALSE;
		}

	/* Interpret the colour mode parameter */
	if      (same_start_ic(mode, "DITHER"))    dmode = Floyd;
	else if (same_start_ic(mode, "FLOYD"))     dmode = Floyd;
	else if (same_start_ic(mode, "BW"))        dmode = Floyd;
	else if (same_start_ic(mode, "GREYSCALE")) dmode = GreyScale;
	else if (same_start_ic(mode, "GREY"))      dmode = GreyScale;
	else if (same_start_ic(mode, "GRAYSCALE")) dmode = GreyScale;
	else if (same_start_ic(mode, "GRAY"))      dmode = GreyScale;
	else if (same_start_ic(mode, "COLOUR"))    dmode = Colour;
	else if (same_start_ic(mode, "COLOR"))     dmode = Colour;
	else if (same_start_ic(mode, "CLR"))       dmode = Colour;
	else
		{
		(void) fprintf(stderr, "[raster_dump] Unsupported colour mode: %s\n",
					mode);
		return FALSE;
		}

	/* Interpret the valid time */
	active = same_ic(vtime, "ACTIVE");
	if (active)
		{
		/*
		ivt = ViewTime;
		if (ivt < 0)
			{
			(void) fprintf(stderr, "[raster_dump] No active depiction\n");
			return FALSE;
			}
		*/
		}
	else
		{
		ivt = find_valid_time(vtime);
		if (ivt < 0)
			{
			(void) fprintf(stderr, "[raster_dump] Non-existent depiction %s\n",
						vtime);
			return FALSE;
			}
		}

	/* Interpret the dimensions */
	if ((width<=0) || (height<=0))
		{
		(void) fprintf(stderr, "[raster_dump] Invalid dimensions %dx%d\n",
				width, height);
		return FALSE;
		}

	/* Interpret the padding mode */
	if (blank(pad))                 pmode = NoPad;
	else if (same_ic(pad, "NOPAD")) pmode = NoPad;
	else if (same_ic(pad, "PAD"))   pmode = Pad;
	else
		{
		(void) fprintf(stderr, "[raster_dump] Unsupported padding mode: %s\n", pad);
		return FALSE;
		}

	/* Adjust sizes to preserve the aspect ratio unless padding is requested */
	nx = width;
	ny = height;
	if (pmode != Pad)
		{
		float	xlen, ylen;
		/*
		sx = MapProj->definition.xlen / nx;
		sy = MapProj->definition.ylen / ny;
		if (sx >= sy) ny = (int) ceil(nx * MapProj->definition.ylen
						                 / MapProj->definition.xlen);
		else          nx = (int) ceil(ny * MapProj->definition.xlen
										 / MapProj->definition.ylen);
		*/
		xlen = DnMap->window.right - DnMap->window.left;
		ylen = DnMap->window.top - DnMap->window.bottom;
		sx = xlen / nx;
		sy = ylen / ny;
		if (sx >= sy) ny = (int) ceil(nx * ylen / xlen);
		else          nx = (int) ceil(ny * xlen / ylen);
		}

	/* Check the output file */
	if (blank(dfile))
		{
		(void) fprintf(stderr, "[raster_dump] No file name given\n");
		return FALSE;
		}

	/* Check the template file */
	tmeta = read_metafile(template, MapProj);
	if (NotNull(tmeta))
		{
		setup_metafile_presentation(tmeta, "FPA");
		tissue = encode_clock(Syear, Sjday, Shour, Sminute, 0);
		tvalid = encode_clock(TimeList[ivt].year, TimeList[ivt].jday,
							  TimeList[ivt].hour, TimeList[ivt].minute,
							  0);

		/* Interpret date-time format codes */
		for (ifld=0; ifld<tmeta->numfld; ifld++)
			{
			tfld = tmeta->fields[ifld];
			if (IsNull(tfld))               continue;
			if (tfld->ftype != FtypeSet)    continue;
			tset = tfld->data.set;
			if (IsNull(tset))               continue;
			if (!same(tset->type, "label")) continue;

			for (imem=0; imem<tset->num; imem++)
				{
				tlab = (LABEL) tset->list[imem];
				if (IsNull(tlab)) continue;
				if (strchr(tlab->label, '$'))
					{
					time_macro_substitute(cbuf, sizeof(cbuf), tlab->label,
										  tissue, tvalid);
					tlab->label = SETSTR(tlab->label, cbuf);
					}
				}
			}
		}

	/*******************************************************************
	*                                                                  *
	*  Launch the raster generator.                                    *
	*                                                                  *
	*******************************************************************/

	/* Fork the process so that we don't butcher the real data */
	prevaction = signal(SIGCLD, SIG_DFL);
	pid = fork();
	if (pid < 0)
		{
		perror("[raster_dump]");
		return FALSE;
		}
	if (pid > 0)
		{
		put_message("dump-format");
		while ( waitpid(pid, NullPtr(int *), 0) != pid)
			{
			}
		(void) signal(SIGCLD, prevaction);
		put_message("dump-ready");
		return TRUE;
		}

	/*******************************************************************
	*                                                                  *
	*  This is the raster generator.                                   *
	*                                                                  *
	*******************************************************************/

	/* Here we are in the forked process */
	/* XXX For some reason calling this function results in an endless loop XXX
	 * RDP 24 Oct 2002
	set_error_trap(SIG_IGN);
	*/
	pid = getpid();
	pr_diag("Raster.Dump", "Raster generator launched [%d]\n", pid);

	/* Provide ability to trap the raster generator in the debugger */
	if (getenv("FPA_DEBUG_DUMP") != NULL)
		{
		(void) printf("   Raster generator forked process [%d]\n", pid);
		(void) printf("   Debug mode is enabled.\n");
		(void) printf("   \n");
		(void) printf("   If you wish to continue, enter [RETURN].\n");
		(void) printf("   A timeout of 5 minutes is provided as a failsafe.\n");
		(void) printf("   \n");
		(void) printf("   If you wish to debug this sub-process:\n");
		(void) printf("      - run: xdb -P %d $FPA/bin/`sysdir`/xfpa,\n", pid);
		(void) printf("      - view the file dump.c,\n");
		(void) printf("      - set a breakpoint after the call to fgets(),\n");
		(void) printf("      - then continue.\n");
		(void) printf("   \n");
		(void) printf("   Waiting... Enter RETURN after setting breakpoint: ");

		/* Set a timeout, then wait for a debug process or a signal */
		prevfcn = signal(SIGALRM,trapfcn);
		(void) alarm(300);
		(void) fgets(cbuf, sizeof(cbuf), stdin);

		/* NOTE: Set a breakpoint after this point!!! */

		/* We have now returned from the pause reset the alarm */
		prevfcn = signal(SIGALRM,prevfcn);
		(void) alarm(0);
		XSynchronize(X_display, True);
		(void) printf("   Resuming...\n");
		}

	/* Open a memory device to match the size of the page */
	glResetDisplayConnection();
	if (!gxOpenDump(nx, ny))
		{
		pr_error("Raster.Dump", "Cannot open memory device\n");
		exit(1);
		}

	/* Transform the data to the page size */
	suspend_zoom();
	copy_box(&window, &UnitBox);
	window.right = (float) nx;
	window.top   = (float) ny;
	define_dn_xform(DnRoot, "root", NullBox, &window, NullMapProj, NullXform);
	define_dn_xform(DnMap, "map", &window, NullBox, MapProj, NullXform);
	free_dn_raster(DnBgnd);
	resume_zoom(FALSE);

	/* Redraw everything into the memory device */
	ViewOnly = TRUE;
	if (active)
		{
		update_screen(DnRoot);
		glFlush();
		}
	else
		{
		/* Turn off communication with the interface */
		Spawned = TRUE;

		/* Pick and draw the new depiction */
		(void) active_edit_field("NONE", "NONE");
		(void) pick_sequence(vtime, NULL);

		/* Add a template if specified */
		if (NotNull(tmeta))
			{
			tdn = init_panel(DnMap, "map", TempBgnd);
			define_dn_xform(tdn, "map", &tdn->viewport, NullBox, MapProj,
							NullXform);
			define_dn_data(tdn, "metafile", (POINTER) tmeta);
			present_all();
			}
		}

	/* For GreyScale and Floyd dithering, capture the frame buffer and */
	/* substitute "hardcopy" grey values for known colours */
	if (dmode == Floyd || dmode == GreyScale)
		{
		static	int		   nmap = 11;
		static	Pixel	   to[11];
		static  ColorIndex from[11];

		COLOUR		lcolour, wcolour, ccolour, bcolour, llcolour;
		COLOUR		facolour, fbcolour;
		COLOUR		lsub, wsub, csub, bsub, llsub, fasub, fbsub;
		COLOUR		tfcolour, tecolour, tlcolour, ttcolour;
		COLOUR		tfsub, tesub, tlsub, ttsub;

		/* Get current geography/template colours */
		lcolour  = GeogLand;
		wcolour  = GeogWater;
		ccolour  = GeogCoast;
		bcolour  = GeogBorder;
		llcolour = GeogLatlon;
		facolour = GeogFarea;
		fbcolour = GeogFbord;
		default_geog_colours(&lcolour, &wcolour, &ccolour, &bcolour, &llcolour,
							 &facolour, &fbcolour);
		default_temp_colours(&tfcolour, &tecolour, &tlcolour, &ttcolour);

		/* Get the hardcopy/greyscale geography/template colours */
		printer_geog_colours(&lsub, &wsub, &csub, &bsub, &llsub, &fasub,&fbsub);
		printer_temp_colours(&tfsub, &tesub, &tlsub, &ttsub);

		/* Map colours to greyscale */
		from[0]  = lcolour;		to[0]  = (Pixel) lsub;
		from[1]  = wcolour;		to[1]  = (Pixel) wsub;
		from[2]  = ccolour;		to[2]  = (Pixel) csub;
		from[3]  = bcolour;		to[3]  = (Pixel) bsub;
		from[4]  = llcolour;	to[4]  = (Pixel) llsub;
		from[5]  = facolour;	to[5]  = (Pixel) fasub;
		from[6]  = fbcolour;	to[6]  = (Pixel) fbsub;
		from[7]  = tfcolour;	to[7]  = (Pixel) tfsub;
		from[8]  = tecolour;	to[8]  = (Pixel) tesub;
		from[9]  = tlcolour;	to[9]  = (Pixel) tlsub;
		from[10] = ttcolour;	to[10] = (Pixel) ttsub;

		glSetGrayscaleColorIndexConversion(from, to, nmap);
		}

	/* Output the bitmap */
	switch (dmode)
		{
		case Colour:	glWindowToFile(dfile, fmode, glCOLORSCALE); break;
		case GreyScale:	glWindowToFile(dfile, fmode, glGREYSCALE);  break;
		case Floyd:		glWindowToFile(dfile, fmode, glBW);         break;
		}

	/* Close the temporary dump window */
	gxCloseDump();
	glExit();
	pr_diag("Raster.Dump", "Raster generator finished [%d]\n", pid);
	exit(0);

	/* Shouldn't get here */
	return TRUE;
	}
