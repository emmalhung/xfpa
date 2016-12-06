/***********************************************************************
*                                                                      *
*     c m f _ t o _ s v g . c                                          *
*                                                                      *
*     Routine to convert CMF format symbol files to Scalable Vector    *
*     Graphics (SVG)                                                   *
*                                                                      *
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
static	const	STRING	SVGDirHome  = "$HOME/setup/pdf/svgmet/common/svg";
static	const	STRING	SVGDirFpa   = "$FPA/setup/pdf/svgmet/common/svg";

#define	PNone	"none"

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
	char		svgname[BufSize], filesvg[BufSize];
	char		inbuf[BufSize], tbuf[BufSize];
	char		outline[BufSize], lwidth[BufSize], lstyle[BufSize];
	char		fill[BufSize], font[BufSize];
	int			cc, mm, yy, kk, rr, gg, bb, radl, ndash, ofirst, just;
	float		xmin, xmax, ymin, ymax;
	float		xx1, yy1, xx2, yy2, xx3, yy3;
	float		sang, eang, rotn, sclx, scly, sclt;
	LOGICAL		display_outline, display_fill, outline_first;
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
	if ( argc != 3 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   cmf_to_svg <input_cmf_file>");
		(void) fprintf(stderr, " <output_svg_file>\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("product.graphic");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Set program label */
	(void) sprintf(MyLabel, "[%d] cmf_to_svg:", getpid());

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

	/* Open the output svg file ... in matching directory! */
	(void) strcpy(svgname, env_sub(argv[2]));
	if ( !strstr(svgname, ".svg") ) (void) strcat(svgname, ".svg");
	if ( inhome ) (void) strcpy(filesvg, pathname(env_sub(SVGDirHome), svgname));
	else          (void) strcpy(filesvg, pathname(env_sub(SVGDirFpa),  svgname));
	if ( IsNull( FP_Out = fopen(filesvg, "w") ) )
		{
		(void) fprintf(stderr,
				"%s Cannot open output SVG file \"%s\"\n",
				MyLabel, filesvg);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set the parameters for using outline and fill */
	display_outline = TRUE;
	display_fill    = TRUE;
	outline_first    = FALSE;

	/* Initialize internal buffers */
	(void) strcpy(outline, PNone);
	(void) strcpy(lwidth,  PNone);
	(void) strcpy(lstyle,  PNone);
	(void) strcpy(fill,    PNone);
	(void) strcpy(font,    "Times-Roman");

	/* Print startup message */
	(void) fprintf(stdout, " Converting CMF file: \"%s\"\n", filecmf);
	(void) fprintf(stdout, " to SVG file: \"%s\"\n", filesvg);

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
	/* Output the first linesof the SVG file 
	 * Will need to either remove these or ignore them when placing in another svg file.
	 * */
	/* >>> EMMA TESTING NEW HEADER FORMAT
	(void) fprintf(FP_Out,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>");
	(void) fprintf(FP_Out,"<!-- SVGMet_size[%.3f %.3f %.3f %.3f] -->",
				   xmin, ymax, xmax, ymin);
	(void) fprintf(FP_Out,"<svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%.3f\" height=\"%.3f\">\n",xmax-xmin, ymax-ymin);
	*/
	(void) fprintf(FP_Out, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?> <!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"> <svg version=\"1.1\" baseProfile=\"full\" width=\"100%%\" height=\"100%%\" viewBox=\"%.3f %.3f %.3f %.3f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" >" , xmin, ymin, xmax-xmin, ymax-ymin);
	
	/* Read the remaining lines of the CMF file ... and convert line writes */
	textheader = FALSE;
	while ( NotNull(getfileline(FP_In, inbuf, (size_t) BufSize)) ) 
	{ 
	  
	  /* Read the first part of the line */ 
	  (void) strcpy(tbuf, inbuf); sbuf = string_arg(tbuf); 
		/* Convert lines beginning with "@m" to "moveto" */
		if ( same_ic(sbuf, "@m") )
			{
			printline = TRUE;
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = -yy1 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "<path d=\"");
			(void) fprintf(FP_Out, "\n  M %.3f %.3f", xx1, yy1);
			}

		/* Convert lines beginning with "@l" or "@L" to "lineto" */
		else if ( same_ic(sbuf, "@l") )
			{
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = -yy1 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "\n  L %.3f %.3f", xx1, yy1);
			}

		/* Convert lines beginning with "@c" or "@C" to "curveto" */
		else if ( same_ic(sbuf, "@c") )
			{
			xx1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1 = xx1 / 1000.0 * 72.0;
			yy1 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1 = -yy1 / 1000.0 * 72.0;
			xx2 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx2 = xx2 / 1000.0 * 72.0;
			yy2 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy2 = -yy2 / 1000.0 * 72.0;
			xx3 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx3 = xx3 / 1000.0 * 72.0;
			yy3 = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy3 = -yy3 / 1000.0 * 72.0;
			(void) fprintf(FP_Out, "\n  C %.3f %.3f %.3f %.3f %.3f %.3f",
												xx1, yy1, xx2, yy2, xx3, yy3);
			}

		/* Convert lines beginning with "@cl" to "closepath" */
		else if ( same_ic(sbuf, "@cl") )
			{
			(void) fprintf(FP_Out, "\" ");/* Close the path list */
			if ( !printline )
				{
			  	(void) fprintf(FP_Out, "/>\n");
			  	continue;
				}
		
			if ( display_outline )
			{
			  if ( !same(outline,PNone))
			  	(void) fprintf(FP_Out, "%s", outline);
			  if ( !same(lwidth,PNone))
			  	(void) fprintf(FP_Out, "%s", lwidth);
			  if ( !same(lstyle,PNone))
			  	(void) fprintf(FP_Out, "%s", lstyle);
			}
			else 
			  	(void) fprintf(FP_Out, "stroke=\"none\" ");
			
			if ( display_fill )
			{
			 if (!same(fill, PNone))
			  	(void) fprintf(FP_Out, "%s", fill);
			}
			else 
			  	(void) fprintf(FP_Out, "fill=\"none\" ");
			
			(void) fprintf(FP_Out, "/>\n"); /* Close style list and path.*/
			printline = FALSE;
			}

		/* Convert lines beginning with "@p" to "stroke" */
		else if ( same_ic(sbuf, "@p") )
			{
			if ( !printline ) continue;
			
			(void) fprintf(FP_Out, "\" fill=\"none\" ");/* Begin style list */
			
			if ( display_outline )
			{
			  if (!same(outline,PNone))
				(void) fprintf(FP_Out, "%s",outline);
			  if (!same(lwidth,PNone))
				(void) fprintf(FP_Out, "%s",lwidth);
			  if (!same(lstyle,PNone))
				(void) fprintf(FP_Out, "%s",lstyle);
			}
			else
				(void) fprintf(FP_Out, "stroke=\"none\" ");
			
			(void) fprintf(FP_Out, "/>\n"); /* Close style list and tag.*/
			printline = FALSE;
			}

		/* Convert lines beginning with "@e" ... ellipses */
		else if ( same_ic(sbuf, "@e") )
			{
			int large_arc, sweep;
			float extent, cx, cy, rx, ry;
			float c_r, s_r, s_nr,s_s, c_s, s_e, c_e;
			
			xx1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			cx  = xx1 / 1000.0 * 72.0;
			yy1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			cy  = -yy1 / 1000.0 * 72.0;
			xx2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rx  = xx2 / 1000.0 * 72.0/2.0;
			yy2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			ry  = yy2 / 1000.0 * 72.0/2.0;
			sang = float_arg(tbuf, &argok);  if ( !argok ) continue;
			sang = -sang / 10.0;
			eang = float_arg(tbuf, &argok);  if ( !argok ) continue;
			eang = -eang / 10.0;
			radl = int_arg(tbuf, &argok);    if ( !argok ) continue;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = -rotn / 10.0;
			
			/* Compute large_arc and sweep flags */
			extent    = eang - sang;
			large_arc = (abs(extent) > 180)?1:0;
			sweep     = (extent > 0 )?1:0;
			
			/* Convert to Radians */
			s_r  = sin(RAD * rotn);
			c_r  = cos(RAD * rotn);
			s_nr = sin(RAD * (-rotn));
			s_s  = sin(RAD * sang);
			c_s  = cos(RAD * sang);
			s_e  = sin(RAD * eang);
			c_e  = cos(RAD * eang);
			
			/* Calculate start and end points of arc. */
			xx1 = cx + c_r * rx * c_s + s_nr * ry * s_s;
			yy1 = cy + s_r * rx * c_s + c_r  * ry * s_s;
			xx2 = cx + c_r * rx * c_e + s_nr * ry * s_e;
			yy2 = cy + s_r * rx * c_e + c_r  * ry * s_e;
			
			if ( sang == eang )
				{
				(void) fprintf(FP_Out, 
							   "<ellipse cx=\"%.3f\" cy=\"%.3f\" rx=\"%.3f\" ry=\"%.3f\" ",
							   cx,cy,rx,ry);   
				(void) fprintf(FP_Out,"transform=\"rotate(%.3f)\" ",rotn); 
				}
			else if ( radl == 1 )
				{
				(void) fprintf(FP_Out, "<path d=\"M %.3f %.3f ", cx, cy);
				(void) fprintf(FP_Out, "L %.3f %.3f ", xx1, yy1);
				(void) fprintf(FP_Out, "A %.3f %.3f %.3f %d %d %.3f %.3f ", 
						   rx, ry, rotn, large_arc, sweep, xx2, yy2);
				(void) fprintf(FP_Out, "L %.3f %.3f\" ", cx, cy);
				}
			else
				{
				(void) fprintf(FP_Out, "<path d=\"M %.3f %.3f ", xx1, yy1);
				(void) fprintf(FP_Out, "A %.3f %.3f %.3f %d %d %.3f %.3f\" ", 
						   rx, ry, rotn, large_arc, sweep, xx2, yy2);
				}
			
			if ( display_outline )
			{
			  if (!same(outline,PNone))
			  	(void) fprintf(FP_Out, "%s", outline);
			  if (!same(lwidth,PNone))
			  	(void) fprintf(FP_Out, "%s", lwidth);
			  if (!same(lstyle,PNone))
			  	(void) fprintf(FP_Out, "%s", lstyle);
			}
			else 
			  	(void) fprintf(FP_Out, "stroke=\"none\" ");
			
			if ( display_fill )
			{
			 if (!same(fill, PNone))
			  	(void) fprintf(FP_Out, "%s", fill);
			}
			else 
			  	(void) fprintf(FP_Out, "fill=\"none\" ");
			
			(void) fprintf(FP_Out, "/>\n"); /* Close style list and tag.*/
			printline = FALSE;
			}

		/* Convert lines beginning with "@r" ... rectangles */
		else if ( same_ic(sbuf, "@r") )
			{
			xx1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx1  = xx1 / 1000.0 * 72.0;
			yy1  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy1  = -yy1 / 1000.0 * 72.0;
			xx2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			xx2  = xx2 / 1000.0 * 72.0;
			yy2  = float_arg(tbuf, &argok);  if ( !argok ) continue;
			yy2  = yy2 / 1000.0 * 72.0;
			radl = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = rotn / 10.0;
			(void) fprintf(FP_Out, 
						   "<rect x=\"%.3f\" y=\"%.3f\" width=\"%.3f\" height=\"%.3f\" ",
						   xx1, yy1, xx2, yy2);
			(void) fprintf(FP_Out,"transform=\"rotate(%.3f)\" ",rotn); 
			
			if ( display_outline )
			{
			  if (!same(outline,PNone))
			  	(void) fprintf(FP_Out, "%s", outline);
			  if (!same(lwidth,PNone))
			  	(void) fprintf(FP_Out, "%s", lwidth);
			  if (!same(lstyle,PNone))
			  	(void) fprintf(FP_Out, "%s", lstyle);
			}
			else 
			  	(void) fprintf(FP_Out, "stroke=\"none\" ");
			
			if ( display_fill )
			{
			  if (!same(fill,PNone))
			  	(void) fprintf(FP_Out, "%s", fill);
			}
			else 
			  	(void) fprintf(FP_Out, "fill=\"none\" ");
			
			(void) fprintf(FP_Out, "/>\n"); /* Close style list and tag.*/
			}

		/* Set parameter for lines beginning with "@uO" ... colour outline */
		else if ( same_ic(sbuf, "@uO") )
			{
			display_outline = TRUE;
			cc = int_arg(tbuf, &argok);  if ( !argok ) continue;
			mm = int_arg(tbuf, &argok);  if ( !argok ) continue;
			yy = int_arg(tbuf, &argok);  if ( !argok ) continue;
			kk = int_arg(tbuf, &argok);  if ( !argok ) continue;
			(void) cmyk_to_rgb(cc, mm, yy, kk, &rr, &gg, &bb);
			(void) sprintf(outline, "stroke=\"rgb(%d,%d,%d)\" ", rr,gg,bb);
			}

		/* Set parameter for lines beginning with "@xO" ... no outline */
		else if ( same_ic(sbuf, "@xO") )
			{
			display_outline = FALSE;
			}

		/* Set parameter for lines beginning with "@uF" ... colour fill */
		else if ( same_ic(sbuf, "@uF") )
			{
			display_fill = TRUE;
			cc = int_arg(tbuf, &argok);  if ( !argok ) continue;
			mm = int_arg(tbuf, &argok);  if ( !argok ) continue;
			yy = int_arg(tbuf, &argok);  if ( !argok ) continue;
			kk = int_arg(tbuf, &argok);  if ( !argok ) continue;
			(void) cmyk_to_rgb(cc, mm, yy, kk, &rr, &gg, &bb);
			(void) sprintf(fill,
					"fill=\"rgb(%d,%d,%d)\" ", rr,gg,bb);
			}

		/* Set parameter for lines beginning with "@xF" ... no fill */
		else if ( same_ic(sbuf, "@xF") )
			{
			display_fill = FALSE;
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
			if ( xx1 < 0.001 ) (void) sprintf(lwidth, "stroke-width=\"0.001\" ");
			else               (void) sprintf(lwidth, "stroke-width=\"%.3f\" ", xx1);
			}

		/* Set parameter for lines beginning with "@dt" ... line style */
		else if ( same_ic(sbuf, "@dt") )
			{
			ndash = int_arg(tbuf, &argok);  if ( !argok ) continue;
			if ( ndash == 0 ) (void) strcpy(lstyle, "stroke-dasharray=\"none\" ");
			else              (void) sprintf(lstyle, "stroke-dasharray=\"%s\" ", tbuf);
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
			yy1  = -yy1 / 1000.0 * 72.0;
			sclt = float_arg(tbuf, &argok);  if ( !argok ) continue;
			sclt = sclt / 1000.0 * 72.0;
			rotn = float_arg(tbuf, &argok);  if ( !argok ) continue;
			rotn = -rotn / 10.0;
			just = int_arg(tbuf, &argok);    if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			(void) int_arg(tbuf, &argok);     if ( !argok ) continue;
			xbuf = string_arg(tbuf);

			/* Write font and text */
			(void) fprintf(FP_Out, "<text x=\"%.3f\" y=\"%.3f\" ", xx1, yy1);
			(void) fprintf(FP_Out, "transform=\"rotate(%.3f)\"",rotn);
			(void) fprintf(FP_Out, "font-family=\"%s\" ",font);
			(void) fprintf(FP_Out, "font-size=\"%.3f\" ",sclt);
			switch (just)
			{
			  case 2:
			    (void) fprintf(FP_Out, "text-anchor=\"middle\" ");
			    break;
			  case 3:
			    (void) fprintf(FP_Out, "text-anchor=\"end\" ");
				break;
			  default:
			    (void) fprintf(FP_Out, "text-anchor=\"start\" ");
				break;
			}
			
			if ( display_outline )
			{
			  if (!same(outline,PNone))
			  	(void) fprintf(FP_Out, "%s", outline);
			  if (!same(lwidth,PNone))
			  	(void) fprintf(FP_Out, "%s", lwidth);
			  if (!same(lstyle,PNone))
			  	(void) fprintf(FP_Out, "%s", lstyle);
			}
			else 
			  	(void) fprintf(FP_Out, "stroke=\"none\" ");
			
			if ( display_fill )
			{
			  if (!same(fill,PNone))
			  	(void) fprintf(FP_Out, "%s", fill);
			}
			else 
			  	(void) fprintf(FP_Out, "fill=\"none\" ");
			(void) fprintf(FP_Out, ">%s</text>\n", xbuf);
			}

		/* lines beginning with "@u" or "@U" ... grouping */
		else if ( same(sbuf, "@u") )
			{
			(void) fprintf(FP_Out, "<g>\n");
			}
		else if ( same(sbuf, "@U") )
			{
			(void) fprintf(FP_Out, "</g>\n");
			}

		/* Append comment lines as is */
		else if ( same(sbuf, "%###") )
			{
			(void) fprintf(FP_Out, "<!-- %s -->\n", inbuf);
			}

		/* Append all other lines as comments */
		else
			{
			(void) fprintf(FP_Out, "<!-- %s -->\n", inbuf);
			}
		}

	/* End with a "</svg>" */
	(void) fprintf(FP_Out, "</svg>\n");

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
