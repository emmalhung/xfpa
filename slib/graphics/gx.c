/***********************************************************************
*                                                                      *
*     g x . c                                                          *
*                                                                      *
*     Useful extensions to the FpaXgl library.                         *
*                                                                      *
*     (c) Copyright 1996 Environment Canada (AES)                      *
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

#include   "gx.h"

/* Pre-defined presentation option names */
static const ITBL	PoptDefs[] =
						{
						FALSE,	"0",
						FALSE,	"f",
						FALSE,	"false",
						FALSE,	"dev",
						FALSE,	"device",
						FALSE,	"vdc",
						FALSE,	"window",
						FALSE,	"from",
						FALSE,	"dirfrom",
						FALSE,	"dir_from",
						FALSE,	"noval",
						FALSE,	"novalue",
						FALSE,	"nocross",
						FALSE,	"parallel",
						FALSE,	"parallel_hatch",
						TRUE,	"1",
						TRUE,	"t",
						TRUE,	"true",
						TRUE,	"map",
						TRUE,	"geog",
						TRUE,	"geography",
						TRUE,	"to",
						TRUE,	"dirto",
						TRUE,	"dir_to",
						TRUE,	"val",
						TRUE,	"value",
						TRUE,	"showval",
						TRUE,	"showvalue",
						TRUE,	"cross",
						TRUE,	"cross_hatch"
						};
static const int	NumPopt = ITBL_SIZE(PoptDefs);

/***********************************************************************
*                                                                      *
*   g x F i n d P o p t i o n   - map assorted presentation options    *
*                                                                      *
***********************************************************************/

LOGICAL	gxFindPoption ( STRING name, STRING type )
	{
	int		opt;

	if (blank(type)) return FALSE;

	opt = find_itbl_entry(PoptDefs, NumPopt, name);
	if (opt >= 0) return (LOGICAL) opt;
	else          return FALSE;
	}

/***********************************************************************
*                                                                      *
*     g x O p e n G r a p h i c s                                      *
*                                                                      *
*     Initialize the usual graphics devices.                           *
*                                                                      *
*     g x C l o s e G r a p h i c s                                    *
*                                                                      *
*     Shut down the usual graphics devices.                            *
*                                                                      *
***********************************************************************/

static	int		MainWid = 0;

/**********************************************************************/

LOGICAL	gxOpenGraphics

	(
	Display	*display,
	Window	window
	)

	{
	/* Initialize graphics and open the window */
	glInit();
	MainWid = glInitWindow(display, window);
	if (MainWid < 0)
		{
		(void) fprintf(stderr, "Cannot open drawing window for graphics\n");
		return FALSE;
		}

	/* Select the window and set up double buffering */
	glSetWindow(MainWid);
	glDoubleBuffer();

	/* Tell the presentation spec objects how to translate attributes */
	provide_colour_function  ( gxFindColour     );
	provide_lstyle_function  ( gxFindLineStyle  );
	provide_lwidth_function  ( gxFindLineWidth  );
	provide_fstyle_function  ( gxFindFillStyle  );
	provide_mtype_function   ( gxFindMarkerType );
	provide_font_function    ( gxFindFont       );
	provide_size_function    ( gxFindFontSize   );
	provide_btype_function   ( gxFindBarbType   );
	provide_poption_function ( gxFindPoption    );

	/* Tell the graphics pipe module how to translate attributes */
    provide_pipe_colour_fn ( gxSetColorIndex );
    provide_pipe_lstyle_fn ( gxLineStyle     );
    provide_pipe_move_fn   ( glMove          );
    provide_pipe_draw_fn   ( glDraw          );
    provide_pipe_msize_fn  ( gxMarkerSize    );
    provide_pipe_mangle_fn ( gxMarkerAngle   );
    provide_pipe_marker_fn ( gxDrawMarker    );
    provide_pipe_flush_fn  ( glFlush         );

	return TRUE;
	}

/**********************************************************************/

LOGICAL	gxCloseGraphics

	(
	LOGICAL	clear
	)

	{
	/* Shut down graphics display device */
	if (clear) glClear();
	glCloseWindow(MainWid);
	glExit();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g x O p e n D u m p                                              *
*                                                                      *
*     Initialize a background graphics area for screen dumps.          *
*                                                                      *
*     g x C l o s e D u m p                                            *
*                                                                      *
*     Shut down the background graphics area.                          *
*                                                                      *
***********************************************************************/

static	int		BgndWid = 0;

/**********************************************************************/

LOGICAL	gxOpenDump

	(
	int		nx,
	int		ny
	)

	{
	/* Open the dump window */
	BgndWid = glCreateBkgndWindow(nx, ny);
	if (BgndWid < 0)
		{
		(void) fprintf(stderr, "Cannot open background window for graphics\n");
		return FALSE;
		}

	/* Select the window and copy the main colour map */
	glSetWindow(BgndWid);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	gxCloseDump (void)

	{
	/* Shut down dump window and reselect the main window */
	glCloseWindow(BgndWid);
	glSetWindow(MainWid);

	return TRUE;
	}
