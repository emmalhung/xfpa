/***********************************************************************
*                                                                      *
*     s v t o p s . c                                                  *
*                                                                      *
*     Program to convert SVG vector code to PS format.                 *
*                                                                      *
*   Usage:  svtops <input >output                                      *
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

/* FPA library definitions */
#include <fpa.h>

/* Standard library definitions */
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

#define	BufSize	256
static	FILE	*FP_In  = NullPtr(FILE *);
static	FILE	*FP_Out = NullPtr(FILE *);
static	const	STRING	SvgDirHome = "$HOME/setup/pdf/svgmet/common/svg";
static	const	STRING	SvgDirFpa  = "$FPA/setup/pdf/svgmet/common/svg";
static	const	STRING	PSDirHome  = "$HOME/setup/pdf/psmet/common/ps";
static	const	STRING	PSDirFpa   = "$FPA/setup/pdf/psmet/common/ps";
/**********************************************************************/

/* Program title used in messaging */
static	const	STRING	MyTitle = "SVG to PS";
static			STRING	MyLabel = "";

/**********************************************************************/

/* Local functions */
static	void	error_trap();	/* Trap for error situations */
		LOGICAL	find_svg_size(FILE *, float *, float *);

/**********************************************************************/

/***********************************************************************
*                                                                      *
*     m a i n                                                          *
*                                                                      *
***********************************************************************/

int		main

	(
	int		argc,
	STRING	argv[]
	)

	{
	int		status;
	LOGICAL	ok, got_path, got_data, inhome;
	char	svgname[BufSize], filesvg[BufSize], ftype[BufSize];
	char	psname[BufSize], fileps[BufSize];
	char	line[1000], work[1500], *cmd;
	char	*cp, *wp;
	float	x1, x2, x3, xc, xb;
	float	y1, y2, y3, yc, yb;
	float	ww, hh;

	/*************************************************************************/
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
		(void) fprintf(stderr, "   svg_to_ps <input_svg_file>");
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
	(void) sprintf(MyLabel, "[%d] svtops:", getpid());
	/**********************************************************************/

	/* Open the input SVG file ... in $HOME or $FPA directories */
	(void) strcpy(svgname, env_sub(argv[1]));
	(void) fprintf(stderr, "%s Input SVG file name \"%s\"\n",
			MyLabel, svgname);
	if ( !strstr(svgname, ".svg") ) (void) strcat(svgname, ".svg");
	(void) strcpy(filesvg, pathname(env_sub(SvgDirHome), svgname));
	inhome = TRUE;
	if ( IsNull( FP_In = fopen(filesvg, "r") ) )
		{
		inhome = FALSE;
		(void) strcpy(filesvg, pathname(env_sub(SvgDirFpa), svgname));
		if ( IsNull( FP_In = fopen(filesvg, "r") ) )
			{
			(void) fprintf(stderr,
					"%s Cannot find input SVG file \"%s\" in directory \"%s\" or \"%s\"\n",
					MyLabel, svgname, SvgDirHome, SvgDirFpa);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		}
	(void) fprintf(stderr, "%s Input SVG file \"%s\"\n",
			MyLabel, filesvg);

	/* Open the output postscript file ... in matching directory! */
	(void) strcpy(psname, env_sub(argv[2]));
	if ( !strstr(psname, ".ps") ) (void) strcat(psname, ".ps");
	if ( inhome ) (void) strcpy(fileps, pathname(env_sub(PSDirHome), psname));
	else          (void) strcpy(fileps, pathname(env_sub(PSDirFpa),  psname));
	if ( IsNull( FP_Out = fopen(fileps, "w") ) )
		{
		(void) fprintf(stderr, "%s Cannot open output Postscript file \"%s\"\n",
				MyLabel, fileps);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) fprintf(stderr, "%s Output Postscript file \"%s\"\n",
			MyLabel, fileps);

	/* Copy file type to a string */
	(void) strcpy(ftype, argv[3]);

	/* Search input file for width and height information */
	if ( !find_svg_size(FP_In, &ww, &hh) ) 
		{
		(void) fprintf(stderr,
				"%s Cannot find SVG dimensions for file \"%s\" in directory \"%s\"\n",
				MyLabel, svgname, inhome?SvgDirHome:SvgDirFpa);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	ww = ww/2;
	hh = hh/2;
	fprintf(FP_Out, "%%!PS-Adobe-2.0-for-FPA-V7 PSMet_size[%.3f %.3f %.3f %.3f]",
			-ww, hh, ww, -hh);
	if      ( same_ic(ftype, "outline") ) fprintf(FP_Out, " PSMet_outline\n");
	else if ( same_ic(ftype, "fill") )    fprintf(FP_Out, " PSMet_fill\n");
	else if ( same_ic(ftype, "both") )    fprintf(FP_Out, " PSMet_both\n");
	else	fprintf(FP_Out, "\n");

	/* Read and process each line */
	got_path = FALSE;
	got_data = FALSE;
	while (getfileline(FP_In, line, sizeof(line)))
		{
		/* Copy the line into the working buffer */
		cp = line;
		wp = work;
		while (*cp)
			{
			switch (*cp)
				{
				case ' ':
				case '\t':	break;

				case '<':	got_path = same_start_ic(cp, "<path");
							got_data = FALSE;
							if (got_path)
								{
								fprintf(FP_Out, "newpath\n");
								cp += 4;
								}
							break;
				case '>':	got_path = FALSE;
							got_data = FALSE;
							break;
				case '=':	if (got_path)
								{
								got_data = same_start_ic(cp-1, "d=");
								}
							else *wp++ = *cp;
							break;

				case '-':	if (got_data)
								{
								*wp++ = ' ';
								*wp++ = *cp;
								}
							else *wp++ = *cp;
							break;
				case ',':	if (got_data)
								{
								*wp++ = ' ';
								}
							else *wp++ = *cp;
							break;
				case 'z':
				case 'Z':
				case 'm':
				case 'M':
				case 'l':
				case 'L':
				case 'h':
				case 'H':
				case 'v':
				case 'V':
				case 'c':
				case 'C':
				case 's':
				case 'S':
				case 'q':
				case 'Q':
				case 't':
				case 'T':
				case 'a':
				case 'A':	if (got_data)
								{
								*wp++ = ' ';
								*wp++ = *cp;
								*wp++ = ' ';
								}
							else *wp++ = *cp;
							break;

				default:	*wp++ = *cp;
				}
			cp++;
			}
		*wp = '\0';

		/* Break the working line into individual commands and arguments */
		while (!blank(work))
			{
			cmd = string_arg(work);
			if (same_ic(cmd, "z"))
				{
				fprintf(FP_Out, "closepath\n");
				}
			else if (same(cmd, "M"))
				{
				x1 = float_arg(work, &ok);
				y1 = float_arg(work, &ok);
				xc = x1;
				yc = y1;
				fprintf(FP_Out, "%f %f moveto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "m"))
				{
				x1 = float_arg(work, &ok) + xc;
				y1 = float_arg(work, &ok) + yc;
				xc = x1;
				yc = y1;
				fprintf(FP_Out, "%f %f moveto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "L"))
				{
				x1 = float_arg(work, &ok);
				y1 = float_arg(work, &ok);
				xc = x1;
				yc = y1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "l"))
				{
				x1 = float_arg(work, &ok) + xc;
				y1 = float_arg(work, &ok) + yc;
				xc = x1;
				yc = y1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "H"))
				{
				x1 = float_arg(work, &ok);
				y1 = yc;
				xc = x1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "h"))
				{
				x1 = float_arg(work, &ok) + xc;
				y1 = yc;
				xc = x1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "V"))
				{
				x1 = xc;
				y1 = float_arg(work, &ok);
				yc = y1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "v"))
				{
				x1 = xc;
				y1 = float_arg(work, &ok) + yc;
				yc = y1;
				fprintf(FP_Out, "%f %f lineto\n", x1-ww, hh-y1);
				}
			else if (same(cmd, "C"))
				{
				x1 = float_arg(work, &ok);
				y1 = float_arg(work, &ok);
				x2 = float_arg(work, &ok);
				y2 = float_arg(work, &ok);
				x3 = float_arg(work, &ok);
				y3 = float_arg(work, &ok);
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "c"))
				{
				x1 = float_arg(work, &ok) + xc;
				y1 = float_arg(work, &ok) + yc;
				x2 = float_arg(work, &ok) + xc;
				y2 = float_arg(work, &ok) + yc;
				x3 = float_arg(work, &ok) + xc;
				y3 = float_arg(work, &ok) + yc;
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "S"))
				{
				x1 = xc + xc - xb;
				y1 = yc + yc - yb;
				x2 = float_arg(work, &ok);
				y2 = float_arg(work, &ok);
				x3 = float_arg(work, &ok);
				y3 = float_arg(work, &ok);
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "s"))
				{
				x1 = xc + xc - xb;
				y1 = yc + yc - yb;
				x2 = float_arg(work, &ok) + xc;
				y2 = float_arg(work, &ok) + yc;
				x3 = float_arg(work, &ok) + xc;
				y3 = float_arg(work, &ok) + yc;
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "Q"))
				{
				x1 = float_arg(work, &ok);
				y1 = float_arg(work, &ok);
				x2 = x1;
				y2 = y1;
				x3 = float_arg(work, &ok);
				y3 = float_arg(work, &ok);
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "q"))
				{
				x1 = float_arg(work, &ok) + xc;
				y1 = float_arg(work, &ok) + yc;
				x2 = x1;
				y2 = y1;
				x3 = float_arg(work, &ok) + xc;
				y3 = float_arg(work, &ok) + yc;
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "T"))
				{
				x1 = xc + xc - xb;
				y1 = yc + yc - yb;
				x2 = x1;
				y2 = y1;
				x3 = float_arg(work, &ok);
				y3 = float_arg(work, &ok);
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			else if (same(cmd, "t"))
				{
				x1 = xc + xc - xb;
				y1 = yc + yc - yb;
				x2 = x1;
				y2 = y1;
				x3 = float_arg(work, &ok) + xc;
				y3 = float_arg(work, &ok) + yc;
				xc = x3;
				yc = y3;
				xb = x2;
				yb = y2;
				fprintf(FP_Out, "%f %f %f %f %f %f curveto\n", 
						x1-ww, hh-y1, 
						x2-ww, hh-y2, 
						x3-ww, hh-y3);
				}
			}
		}
		/* finished with input file */
		fclose(FP_In);

		/* Finish off output file */
		if      ( same_ic(ftype, "outline") ) fprintf(FP_Out, "stroke\n");
		else if ( same_ic(ftype, "fill") )    fprintf(FP_Out, "fill\n");
		else if ( same_ic(ftype, "both") )    fprintf(FP_Out, "stroke\nfill\n");
		fprintf(FP_Out, "showpage\n");
		fclose(FP_Out);

	/**********************************************************************/

	/* Shutdown */
	return 0;
	}

LOGICAL	find_svg_size
	(
	 FILE  *fp, 	/* File pointer */
	 float *width, 	/* Width of image */
	 float *height	/* Height of image */
	)
	{
	char	line[1000],	*cp;
	LOGICAL got_svg=FALSE, got_width=FALSE, got_height=FALSE, ok;

	while (getfileline(fp, line, sizeof(line)))
		{
		cp = line;
		while ( *cp )
			{
			switch ( *cp )
				{
				case '<': got_svg = same_start_ic(cp,"<svg");
						  got_width  = FALSE;
						  got_height = FALSE;
						  cp += 4;
						  break;
				case '>': 
						  if ( got_svg && got_width && got_height ) 
						  	{
							rewind(fp);	
							return TRUE;
							}
						  else
						  	{
						  	got_svg    = FALSE;
						  	got_width  = FALSE;
						  	got_height = FALSE;
						  	cp++;
						  	}
						  break;
				case 'w':
				case 'W': if ( got_svg && same_start_ic(cp,"width=") )
						  	{
							cp += 6;
							*width    = float_arg(cp,&ok);
							got_width = ok;
						  	}
				case 'h':
				case 'H': if ( got_svg && same_start_ic(cp,"height=") )
						  	{
							cp += 7;
							*height    = float_arg(cp,&ok);
							got_height = ok;
						  	}
				default: cp++;
				}
			}
		}
	/* If we get here it should return FALSE */
	rewind(fp);	
	return (got_svg && got_width && got_height); 
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
	fprintf(stdout, "%s !!! %s Has Occurred - Terminating\n",
			MyLabel, sname);

	/* Die gracefully */
	exit(1);
	}
