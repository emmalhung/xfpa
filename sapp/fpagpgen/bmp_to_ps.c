/***********************************************************************
*                                                                      *
*     b m p _ t o _ p s . c                                            *
*                                                                      *
*     Routine to convert Bmp format symbol files to Postscript         *
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

/* Set file information for Bmp and Postscript symbol files */
#define	BufSize	256
static	FILE	*FP_In  = NullPtr(FILE *);
static	FILE	*FP_Out = NullPtr(FILE *);
static	const	STRING	BmpDirHome = "$HOME/setup/pdf/psmet/common/bmp";
static	const	STRING	BmpDirFpa  = "$FPA/setup/pdf/psmet/common/bmp";
static	const	STRING	PSDirHome  = "$HOME/setup/pdf/psmet/common/ps";
static	const	STRING	PSDirFpa   = "$FPA/setup/pdf/psmet/common/ps";

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
	int			status, width, height, xpos, ypos, xi, vw;
	Image       icon;
	char		bmpname[BufSize], filebmp[BufSize];
	char		psname[BufSize], fileps[BufSize];
	float		xmin, xmax, ymin, ymax;
	LOGICAL		inhome;
	unsigned char *raster;
	glCOLOR     *trans;

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
		(void) fprintf(stderr, "   bmp_to_ps <input_bmp_file>");
		(void) fprintf(stderr, " <output_postscript_file>\n");
		(void) fprintf(stderr, "Where <input_bmp_file> is a file of type:");
		(void) fprintf(stderr, " GIF, PNG, or TIF \n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("product.graphic");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Set program label */
	(void) sprintf(MyLabel, "[%d] bmp_to_ps:", getpid());

	/* Open the input Bmp file ... in $HOME or $FPA directories */
	(void) strcpy(bmpname, env_sub(argv[1]));
	(void) strcpy(filebmp, pathname(env_sub(BmpDirHome), bmpname));
	inhome = TRUE;
	
	/*>>>> CAN I DO THIS BETTER? <<<<*/
	if ( IsNull( FP_In = fopen(filebmp, "r") ) )
		{
		inhome = FALSE;
		(void) strcpy(filebmp, pathname(env_sub(BmpDirFpa), bmpname));
		if ( IsNull( FP_In = fopen(filebmp, "r") ) )
			{
			(void) fprintf(stderr,
					"%s Cannot find input Bmp file \"%s\" in directory \"%s\" or \"%s\"\n",
					MyLabel, bmpname, BmpDirHome, BmpDirFpa);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		}
	/*>>>> CAN I DO THIS BETTER?
	 * I'm only interested in finding where the file is I don't need it open for reading
	 <<<<*/

	fclose(FP_In);

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

	/* Print startup message */
	(void) fprintf(stdout, " Converting Bmp file: \"%s\"\n", filebmp);
	(void) fprintf(stdout, "  to postscript file: \"%s\"\n", fileps);

	/******************************************/
	/**** Open image and convert to raster ****/
	/******************************************/
	/* Need to create a virtual Window */
	(void) glVirtualInit();
	vw = glCreateVirtualWindow(900, 900);	
	/* Fetch the bitmap image */
    icon = glImageFetchFile(filebmp);
	trans = glImageTransparentPixel();
	/* Reset the image geometry to match the file size by default */
	glImageGeometry(icon, 0, 0, 0, 0);
	/* Convert to a raster */
	if (!glImageCreateRaster(icon, glImagePixelMajor, &raster, &xpos, &ypos, &width, &height))
	{
		glImageDestroy(icon);
		(void) glCloseWindow(vw);
		(void) glExit();
		(void) fprintf(stderr,
				"%s Cannot open output bitmap file \"%s\"\n",
				MyLabel, filebmp);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return(-1);
	}
	glImageDestroy(icon);
	(void) glCloseWindow(vw);
	(void) glExit();

	/* Output the first line of the Postscript file */
	(void) fprintf(FP_Out,
			"%%!PS-Adobe-2.0-for-FPA-V6 PSMet_size[ %.3f %.3f %.3f %.3f]\n",
			-width/2.0, height/2.0, width/2.0, -height/2.0);

	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "/DeviceRGB setcolorspace\n");
	/* Image is positioned wrt Lower Left corner of map area */
	(void) fprintf(FP_Out, "%d %d scale\n", width, height);
	(void) fprintf(FP_Out, "<<\n");
	/* Image Type 4 indicates Colour Key Masking */
	(void) fprintf(FP_Out, "	/ImageType 4\n");
	/* Indicate colour or range of colours to mask out as transparent */
	(void) fprintf(FP_Out, "	/MaskColor [%d %d %d]\n",
				   trans->red, trans->green, trans->blue);
	(void) fprintf(FP_Out, "	/Width %d\n", width);
	(void) fprintf(FP_Out, "	/Height %d\n", height);
	(void) fprintf(FP_Out, "	/BitsPerComponent 8\n");
	/* Decode array corresponds to /DeviceRGB colour space */
	(void) fprintf(FP_Out, "	/Decode [0 1 0 1 0 1]\n");
	/* Image origin is centre of raster */
	(void) fprintf(FP_Out, "	/ImageMatrix [%d 0 0 -%d  %.3f %.3f]\n",
				   width, height, width/2.0, height/2.0);
	(void) fprintf(FP_Out, "	/DataSource currentfile /ASCIIHexDecode filter\n");
	(void) fprintf(FP_Out, ">>\n");
	(void) fprintf(FP_Out, "image\n\n");
	/* Print 2 digit hex code for each colour at each pixel in image */
	for(xi = 0; xi < width*height*3; xi++)
	{
		(void) fprintf(FP_Out, "%.2X", raster[xi]);
		if (!( (xi + 1) % 36)) (void) fprintf(FP_Out, "\n");
	}
	(void) fprintf(FP_Out, "\n>\n");
	(void) fprintf(FP_Out, "grestore\n");
	(void) fprintf(FP_Out, "\n");
	/* Free the memeory for the raster */
	FREEMEM(raster);

	/* End with a "showpage" */
	(void) fprintf(FP_Out, "showpage\n");

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
