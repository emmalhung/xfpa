/***********************************************************************
*                                                                      *
*     c m f _ t o _ p s . c                                            *
*                                                                      *
*     Routine to convert CMF format symbol files to Postscript         *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2001 Environment Canada (MSC)            *
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

#include "colour_utils.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

/* Set file information for CMF and Postscript symbol files */
#define	BufSize	256
static	FILE	*FP_In  = NullPtr(FILE *);
static	FILE	*FP_Out = NullPtr(FILE *);
static	const	STRING	CmfDirHome = "$HOME/setup/pdf/cormet/common/cmf";
static	const	STRING	CmfDirFpa  = "$FPA/setup/pdf/cormet/common/cmf";
static	const	STRING	PSDirHome  = "$HOME/setup/pdf/psmet/common/ps";
static	const	STRING	PSDirFpa   = "$FPA/setup/pdf/psmet/common/ps";

/* Routines for outputting "stroke" and "fill" commands */
#define	PNone	"none"
static	void	output_outline(STRING, STRING, STRING);
static	void	output_default_outline(STRING, STRING, STRING);
static	void	output_fill(STRING);
static	void	output_default_fill(STRING);

/* Trap for error situations */
static	void	error_trap(int);

/* Default message string */
static	char	MyLabel[BufSize];


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
	int			status, version_number;
	STRING		sbuf, xbuf;
	char		cmfname[BufSize], filecmf[BufSize];
	char		psname[BufSize], fileps[BufSize];
	char		inbuf[BufSize], tbuf[BufSize];
	char		outline[BufSize], lwidth[BufSize], lstyle[BufSize];
	char		fill[BufSize], font[BufSize];
	int			cc, mm, yy, kk, rr, gg, bb, radl, ndash, ofirst, just;
	float		xmin, xmax, ymin, ymax;
	float		xx1, yy1, xx2, yy2, xx3, yy3;
	float		sang, eang, rotn, sclx, scly, sclt;
	LOGICAL		default_outline, default_fill, outline_first;
	LOGICAL		inhome, argok, printline, textheader;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 3, 1);

	/* Validate run string parameters */
	if ( argc != 4 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   cmf_to_ps <input_cmf_file>");
		(void) fprintf(stderr, " <output_postscript_file> <type_of_file>\n");
		(void) fprintf(stderr, "Where <type_of_file> is one of:");
		(void) fprintf(stderr, " \"outline\" \"fill\" \"both\" \"none\"\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("product.graphic");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Set program label */
	(void) sprintf(MyLabel, "[%d] cmf_to_ps:", getpid());

	/* Open the input CMF file ... in $HOME or $FPA directories */
	(void) strcpy(cmfname, env_sub(argv[1]));
	if ( !strstr(cmfname, ".cmf") ) (void) strcat(cmfname, ".cmf");
	(void) strcpy(filecmf, pathname(env_sub(CmfDirHome), cmfname));
	inhome = TRUE;
	if ( IsNull( FP_In = fopen(filecmf, "r") ) )
		{
		inhome = FALSE;
		(void) strcpy(filecmf, pathname(env_sub(CmfDirFpa), cmfname));
		if ( IsNull( FP_In = fopen(filecmf, "r") ) )
			{
			(void) fprintf(stderr,
					"%s Cannot find input CMF file \"%s\" in directory \"%s\" or \"%s\"\n",
					MyLabel, cmfname, CmfDirHome, CmfDirFpa);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		}

	/* Open the output postscript file ... in matching directory! */
	(void) strcpy(psname, env_sub(argv[2]));
	if ( !strstr(psname, ".ps") ) (void) strcat(psname, ".ps");
	if ( inhome ) (void) strcpy(fileps, pathname(env_sub(PSDirHome), psname));
	else          (void) strcpy(fileps, pathname(env_sub(PSDirFpa),  psname));
	if ( IsNull( FP_Out = fopen(fileps, "w") ) )
		{
		(void) fprintf(stderr,
				"%s Cannot open output Postscript file \"%s\"\n",
				MyLabel, fileps);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set the parameters for using default outline and fill */
	if ( same_ic(argv[3], "outline") )
		{
		default_outline = TRUE;
		default_fill    = FALSE;
		}
	else if ( same_ic(argv[3], "fill") )
		{
		default_outline = FALSE;
		default_fill    = TRUE;
		}
	else if ( same_ic(argv[3], "both") )
		{
		default_outline = TRUE;
		default_fill    = TRUE;
		}
	else
		{
		default_outline = FALSE;
		default_fill    = FALSE;
		}

	/* Initialize internal buffers */
	(void) strcpy(outline, PNone);
	(void) strcpy(lwidth,  PNone);
	(void) strcpy(lstyle,  PNone);
	(void) strcpy(fill,    PNone);
	(void) strcpy(font,    "Times-Roman");
	outline_first = FALSE;

	/* Re-initialize internal outlines for solid lines */
	if ( !default_outline )
		{
		(void) strcpy(lwidth, "0.001 setlinewidth");
		(void) strcpy(lstyle, "[] 0 setdash");
		}

	/* Print startup message */
	(void) fprintf(stdout, " Converting CMF file: \"%s\"\n", filecmf);
	(void) fprintf(stdout, "  to postscript file: \"%s\"\n", fileps);

	/* Read the first line of the CMF file ... and convert the dimensions */
	(void) getfileline(FP_In, inbuf, (size_t) BufSize);
	if ( sscanf(inbuf, "@CorelMF %d %f %f %f %f",
			&version_number, &xmin, &ymax, &xmax, &ymin) != 5 )
		{
		(void) fprintf(stderr,
				"%s Cannot read dimensions from input CMF file \"%s\"\n",
				MyLabel, filecmf);
		return (-1);
		}
	xmin = xmin / 1000.0 * 72.0;
	xmax = xmax / 1000.0 * 72.0;
	ymin = ymin / 1000.0 * 72.0;
	ymax = ymax / 1000.0 * 72.0;

	/* Output the first line of the Postscript file */
	(void) fprintf(FP_Out,
			"%%!PS-Adobe-2.0-for-FPA-V5 PSMet_size[%.3f %.3f %.3f %.3f]",
			xmin, ymax, xmax, ymin);
	if ( default_outline && default_fill )
								(void) fprintf(FP_Out, " PSMet_both\n");
	else if ( default_outline )	(void) fprintf(FP_Out, " PSMet_outline\n");
	else if ( default_fill )	(void) fprintf(FP_Out, " PSMet_fill\n");
	else						(void) fprintf(FP_Out, "\n");

	/* Add a second line (for testing) */
	(void) fprintf(FP_Out, "		%% 144.000 144.000 translate");
	(void) fprintf(FP_Out, " %% ... for testing ...\n");

	/* Read the remaining lines of the CMF file ... and convert line writes */
	textheader = FALSE;
	while ( NotNull(getfileline(FP_In, inbuf, (size_t) BufSize)) )
		{

		/* Read the first part of the line */
		(void) strcpy(tbuf, inbuf);
		sbuf = string_arg(tbuf);

		/* Convert lines beginning with "@m" to "moveto" */
		if ( same_ic(sbuf, "@m") )
			{
			printline = TRUE;
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = yy1 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "newpath\n");
			(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xx1, yy1);
			}

		/* Convert lines beginning with "@l" or "@L" to "lineto" */
		else if ( same_ic(sbuf, "@l") )
			{
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = yy1 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx1, yy1);
			}

		/* Convert lines beginning with "@c" or "@C" to "curveto" */
		else if ( same_ic(sbuf, "@c") )
			{
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = yy1 / 1000.0 * 72.0;
			xx2 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx2 = xx2 / 1000.0 * 72.0;
			yy2 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy2 = yy2 / 1000.0 * 72.0;
			xx3 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx3 = xx3 / 1000.0 * 72.0;
			yy3 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy3 = yy3 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "%.3f %.3f %.3f %.3f %.3f %.3f curveto\n",
												xx1, yy1, xx2, yy2, xx3, yy3);
			}

		/* Convert lines beginning with "@cl" to "closepath" */
		else if ( same_ic(sbuf, "@cl") )
			{
			(void) fprintf(FP_Out, "closepath\n");
			if ( !printline ) continue;
			if ( default_outline && default_fill )
				{
				(void) output_default_fill(fill);
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_outline )
				{
				(void) output_fill(fill);
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_fill )
				{
				(void) output_default_fill(fill);
				(void) output_outline(outline, lwidth, lstyle);
				}
			else
				{
				(void) output_fill(fill);
				(void) output_outline(outline, lwidth, lstyle);
				}
			printline = FALSE;
			}

		/* Convert lines beginning with "@p" to "stroke" */
		else if ( same_ic(sbuf, "@p") )
			{
			if ( !printline ) continue;
			if ( default_outline )
				{
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else
				{
				(void) output_outline(outline, lwidth, lstyle);
				}
			printline = FALSE;
			}

		/* Convert lines beginning with "@e" ... ellipses */
		else if ( same_ic(sbuf, "@e") )
			{
			xx1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1  = xx1 / 1000.0 * 72.0;
			yy1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1  = yy1 / 1000.0 * 72.0;
			xx2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx2  = xx2 / 1000.0 * 72.0;
			yy2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy2  = yy2 / 1000.0 * 72.0;
			sclx = 1.0;
			scly = yy2 / xx2;
			xx1  = xx1 / sclx;
			yy1  = yy1 / scly;
			sang = float_arg(tbuf, &argok);  if ( !argok ) continue;
			sang = sang / 10.0;
			eang = float_arg(tbuf, &argok);  if ( !argok ) continue;
			eang = eang / 10.0;
			radl = int_arg(tbuf, &argok);    if ( !argok ) continue;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = rotn / 10.0;
			(void) fprintf(FP_Out, "gsave\n");
			(void) fprintf(FP_Out, "  newpath\n");
			(void) fprintf(FP_Out, "  %.3f rotate\n", rotn);
			(void) fprintf(FP_Out, "  %.3f %.3f scale\n", sclx, scly);
			if ( sang == eang )
				{
				(void) fprintf(FP_Out, "  %.3f %.3f %.3f 0.000 360.000 arc\n",
													xx1, yy1, xx2/2.0);
				}
			else if ( radl == 1 )
				{
				(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xx1, yy1);
				(void) fprintf(FP_Out, "  %.3f %.3f lineto\n", xx1, yy1);
				(void) fprintf(FP_Out, "  %.3f %.3f %.3f %.3f %.3f arc\n",
													xx1, yy1, xx2/2, sang, eang);
				(void) fprintf(FP_Out, "  %.3f %.3f lineto\n", xx1, yy1);
				}
			else
				{
				(void) fprintf(FP_Out, "  %.3f %.3f %.3f %.3f %.3f arc\n",
													xx1, yy1, xx2/2, sang, eang);
				}
			(void) fprintf(FP_Out, "  closepath\n");
			if ( default_outline && default_fill )
				{
				(void) output_default_fill(fill);
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_outline )
				{
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_fill )
				{
				(void) output_default_fill(fill);
				}
			else
				{
				(void) output_fill(fill);
				(void) output_outline(outline, lwidth, lstyle);
				}
			(void) fprintf(FP_Out, "grestore\n");
			}

		/* Convert lines beginning with "@r" ... rectangles */
		else if ( same_ic(sbuf, "@r") )
			{
			xx1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1  = xx1 / 1000.0 * 72.0;
			yy1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1  = yy1 / 1000.0 * 72.0;
			xx2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx2  = xx2 / 1000.0 * 72.0;
			yy2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy2  = yy2 / 1000.0 * 72.0;
			radl = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = rotn / 10.0;
			xx1  = xx1 - xx2/2.0;
			yy1  = yy1 - yy2/2.0;
			(void) fprintf(FP_Out, "gsave\n");
			(void) fprintf(FP_Out, "  newpath\n");
			(void) fprintf(FP_Out, "  %.3f rotate\n", rotn);
			(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xx1, yy1);
			(void) fprintf(FP_Out, "  0.000 %.3f rlineto\n", yy2);
			(void) fprintf(FP_Out, "  %.3f 0.000 rlineto\n", xx2);
			(void) fprintf(FP_Out, "  0.000 %.3f rlineto\n", -yy2);
			(void) fprintf(FP_Out, "  closepath\n");
			if ( default_outline && default_fill )
				{
				(void) output_default_fill(fill);
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_outline )
				{
				(void) output_default_outline(outline, lwidth, lstyle);
				}
			else if ( default_fill )
				{
				(void) output_default_fill(fill);
				}
			else
				{
				(void) output_fill(fill);
				(void) output_outline(outline, lwidth, lstyle);
				}
			(void) fprintf(FP_Out, "grestore\n");
			}

		/* Set parameter for lines beginning with "@uO" ... colour outline */
		else if ( same_ic(sbuf, "@uO") )
			{
			cc = int_arg(tbuf, &argok);  if ( !argok ) continue;
			mm = int_arg(tbuf, &argok);  if ( !argok ) continue;
			yy = int_arg(tbuf, &argok);  if ( !argok ) continue;
			kk = int_arg(tbuf, &argok);  if ( !argok ) continue;
			(void) cmyk_to_rgb(cc, mm, yy, kk, &rr, &gg, &bb);
			xx1 = (float) rr / 255.0;
			xx2 = (float) gg / 255.0;
			xx3 = (float) bb / 255.0;
			(void) sprintf(outline,
					"%.3f %.3f %.3f setrgbcolor %% ... for outline",
					xx1, xx2, xx3);
			if ( default_outline ) default_outline = FALSE;
			}

		/* Set parameter for lines beginning with "@xO" ... no outline */
		else if ( same_ic(sbuf, "@xO") )
			{
			(void) strcpy(outline, PNone);
			if ( default_outline ) default_outline = FALSE;
			}

		/* Set parameter for lines beginning with "@uF" ... colour fill */
		else if ( same_ic(sbuf, "@uF") )
			{
			cc = int_arg(tbuf, &argok);  if ( !argok ) continue;
			mm = int_arg(tbuf, &argok);  if ( !argok ) continue;
			yy = int_arg(tbuf, &argok);  if ( !argok ) continue;
			kk = int_arg(tbuf, &argok);  if ( !argok ) continue;
			(void) cmyk_to_rgb(cc, mm, yy, kk, &rr, &gg, &bb);
			xx1 = (float) rr / 255.0;
			xx2 = (float) gg / 255.0;
			xx3 = (float) bb / 255.0;
			(void) sprintf(fill,
					"%.3f %.3f %.3f setrgbcolor %% ... for fill",
					xx1, xx2, xx3);
			if ( default_fill ) default_fill = FALSE;
			}

		/* Set parameter for lines beginning with "@xF" ... no fill */
		else if ( same_ic(sbuf, "@xF") )
			{
			(void) strcpy(fill, PNone);
			if ( default_fill ) default_fill = FALSE;
			}

		/* Set parameter for lines beginning with "@FO" ... order of outline/fill */
		else if ( same_ic(sbuf, "@FO") )
			{
			ofirst = int_arg(tbuf, &argok);  if ( !argok ) continue;
			if ( ofirst == 0 ) outline_first = TRUE;
			else               outline_first = FALSE;
			}

		/* Set parameter for lines beginning with "@wd" ... line width */
		else if ( same_ic(sbuf, "@wd") )
			{
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			if ( xx1 < 0.001 ) (void) sprintf(lwidth, "0.001 setlinewidth");
			else               (void) sprintf(lwidth, "%.3f setlinewidth", xx1);
			}

		/* Set parameter for lines beginning with "@dt" ... line style */
		else if ( same_ic(sbuf, "@dt") )
			{
			ndash = int_arg(tbuf, &argok);  if ( !argok ) continue;
			if ( ndash == 0 ) (void) strcpy(lstyle, "[] 0 setdash");
			else              (void) sprintf(lstyle, "[%s ] 0 setdash", tbuf);
			}

		/* Convert lines beginning with "@f" ... font for text */
		else if ( same_ic(sbuf, "@f") )
			{
			xbuf = string_arg(tbuf);
			(void) sscanf(xbuf, "\"%s\"", font);
			}

		/* Convert lines beginning with "@t" ... text */
		else if ( same_ic(sbuf, "@t") )
			{
			xx1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1  = xx1 / 1000.0 * 72.0;
			yy1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1  = yy1 / 1000.0 * 72.0;
			sclt = float_arg(tbuf, &argok);  if ( !argok ) continue;
			sclt = sclt / 1000.0 * 72.0;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = rotn / 10.0;
			just = int_arg(tbuf, &argok);    if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			xbuf = string_arg(tbuf);

			/* Write definitions for justified text */
			if ( !textheader )
				{
				textheader = TRUE;
				(void) fprintf(FP_Out, "%%### Definitions for justified text\n");
				(void) fprintf(FP_Out, "/left_just {\n");
				(void) fprintf(FP_Out, "  pop\n");
				(void) fprintf(FP_Out, "  0 0 rmoveto\n");
				(void) fprintf(FP_Out, "  } def\n");
				(void) fprintf(FP_Out, "/centre_just {\n");
				(void) fprintf(FP_Out, "  dup stringwidth pop 2 div\n");
				(void) fprintf(FP_Out, "  neg 0 rmoveto\n");
				(void) fprintf(FP_Out, "  } def\n");
				(void) fprintf(FP_Out, "/right_just {\n");
				(void) fprintf(FP_Out, "  dup stringwidth pop\n");
				(void) fprintf(FP_Out, "  neg 0 rmoveto\n");
				(void) fprintf(FP_Out, "  } def\n");
				(void) fprintf(FP_Out, "%%### End definitions\n");
				}

			/* Write font and text */
			(void) fprintf(FP_Out, "/%s findfont\n",   font);
			(void) fprintf(FP_Out, "%.3f scalefont\n", sclt);
			(void) fprintf(FP_Out, "setfont\n");
			(void) fprintf(FP_Out, "gsave\n");
			(void) fprintf(FP_Out, "  newpath\n");
			(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xx1, yy1);
			(void) fprintf(FP_Out, "  %.3f rotate\n", rotn);
			if ( just == 1 )
				(void) fprintf(FP_Out, "  (%s) left_just\n",   xbuf);
			else if ( just == 2 )
				(void) fprintf(FP_Out, "  (%s) centre_just\n", xbuf);
			else if ( just == 3 )
				(void) fprintf(FP_Out, "  (%s) right_just\n",  xbuf);
			else
				(void) fprintf(FP_Out, "  (%s) left_just\n",   xbuf);
			(void) fprintf(FP_Out, "  (%s) true charpath\n",   xbuf);
			if ( outline_first )
				(void) output_outline(outline, lwidth, lstyle);
			(void) output_fill(fill);
			if ( !outline_first )
				(void) output_outline(outline, lwidth, lstyle);
			(void) fprintf(FP_Out, "grestore\n");
			}

		/* Skip lines beginning with "@u" or "@U" ... grouping */
		else if ( same(sbuf, "@u") )
			{
			continue;
			/* >>> (void) fprintf(FP_Out, "%% start group\n"); <<< */
			}
		else if ( same(sbuf, "@U") )
			{
			continue;
			/* >>> (void) fprintf(FP_Out, "%% end group\n"); <<< */
			}

		/* Append comment lines as is */
		else if ( same(sbuf, "%###") )
			{
			(void) fprintf(FP_Out, "%s\n", inbuf);
			}

		/* Append all other lines as comments */
		else
			{
			(void) fprintf(FP_Out, "		%% %s\n", inbuf);
			}
		}

	/* End with a "showpage" */
	(void) fprintf(FP_Out, "showpage\n");

	return 0;
	}

/***********************************************************************
*                                                                      *
*     o u t p u t _ o u t l i n e                                      *
*     o u t p u t _ d e f a u l t _ o u t l i n e                      *
*     o u t p u t _ f i l l                                            *
*     o u t p u t _ d e f a u l t _ f i l l                            *
*                                                                      *
***********************************************************************/

static	void	output_outline

	(
	STRING	outline,
	STRING	lwidth,
	STRING	lstyle
	)

	{

	/* Output outline (if required) */
	if ( !same(outline, PNone) )
		{
		(void) fprintf(FP_Out, "gsave\n");
		(void) fprintf(FP_Out, "  %s\n", outline);
		if ( !same(lwidth, PNone) )
			(void) fprintf(FP_Out, "  %s\n", lwidth);
		if ( !same(lstyle, PNone) )
			(void) fprintf(FP_Out, "  %s\n", lstyle);
		(void) fprintf(FP_Out, "  stroke\n");
		(void) fprintf(FP_Out, "grestore\n");
		}
	}

static	void	output_default_outline

	(
	STRING	outline,
	STRING	lwidth,
	STRING	lstyle
	)

	{

	/* No changes to default parameters */
	if ( same(outline, PNone)
			&& same(lwidth, PNone) && same(lstyle, PNone) )
		{
		(void) fprintf(FP_Out, "gsave\n");
		(void) fprintf(FP_Out, "  stroke\n");
		(void) fprintf(FP_Out, "grestore\n");
		}

	/* Set changes to default parameters */
	else
		{
		(void) fprintf(FP_Out, "gsave\n");
		if ( !same(outline, PNone) )
			(void) fprintf(FP_Out, "  %s\n", outline);
		if ( !same(lwidth, PNone) )
			(void) fprintf(FP_Out, "  %s\n", lwidth);
		if ( !same(lstyle, PNone) )
			(void) fprintf(FP_Out, "  %s\n", lstyle);
		(void) fprintf(FP_Out, "  stroke\n");
		(void) fprintf(FP_Out, "grestore\n");
		}
	}

static	void	output_fill

	(
	STRING	fill
	)

	{

	/* Output fill (if required) */
	if ( !same(fill, PNone) )
		{
		(void) fprintf(FP_Out, "gsave\n");
		(void) fprintf(FP_Out, "  %s\n", fill);
		(void) fprintf(FP_Out, "  fill\n");
		(void) fprintf(FP_Out, "grestore\n");
		}
	}

static	void	output_default_fill

	(
	STRING	fill
	)

	{

	/* No changes to default parameters */
	if ( same(fill, PNone) )
		{
		(void) fprintf(FP_Out, "gsave\n");
		(void) fprintf(FP_Out, "  fill\n");
		(void) fprintf(FP_Out, "grestore\n");
		}

	/* Set changes to default parameters */
	else
		{
		(void) fprintf(FP_Out, "gsave\n");
		(void) fprintf(FP_Out, "  %s\n", fill);
		(void) fprintf(FP_Out, "  fill\n");
		(void) fprintf(FP_Out, "grestore\n");
		}
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
