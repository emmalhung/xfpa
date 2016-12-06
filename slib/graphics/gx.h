/****************************************************************************
*                                                                           *
*  File:        gx.h                                                        *
*                                                                           *
*     Version 8 (c) Copyright 2011 Environment Canada                       *
*                                                                           *
*   This file is part of the Forecast Production Assistant (FPA).           *
*   The FPA is free software: you can redistribute it and/or modify it      *
*   under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation, either version 3 of the License, or       *
*   any later version.                                                      *
*                                                                           *
*   The FPA is distributed in the hope that it will be useful, but          *
*   WITHOUT ANY WARRANTY; without even the implied warranty of              *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
*   See the GNU General Public License for more details.                    *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.        *
*                                                                           *
*****************************************************************************/

#ifndef GXDEFS
#define GXDEFS

#include <fpa.h>
#include <FpaXgl.h>

typedef	enum	{
				MarkerDot,
				MarkerCircle,
				MarkerSquare,
				MarkerPlus,
				MarkerTriangle,
				MarkerDiamond,
				MarkerCross,
				MarkerCirclePlus,
				MarkerSquarePlus,
				MarkerAsterisk,
				MarkerEllipse,
				MarkerCircleTarget,
				MarkerCircleFill,
				MarkerEllipseFill,
				MarkerSquareFill,
				MarkerDiamondFill,
				MarkerTriangleFill,
				MarkerRect,
				MarkerRectFill,
				MarkerRectTall,
				MarkerRectTallFill
				} MARKER_TYPE_LIST;

static	const	ITBL	MtypeDefs[] =
							{
							MarkerDot,			"dot",
							MarkerCircle,		"circle",
							MarkerSquare,		"square",
							MarkerPlus,			"plus",
							MarkerTriangle,		"triangle",
							MarkerDiamond,		"diamond",
							MarkerCross,		"cross",
							MarkerCirclePlus,	"circle_plus",
							MarkerSquarePlus,	"square_plus",
							MarkerAsterisk,		"asterisk",
							MarkerEllipse,		"ellipse",
							MarkerCircleTarget,	"circle_target",
							MarkerCircleFill,	"circle_fill",
							MarkerEllipseFill,	"ellipse_fill",
							MarkerSquareFill,	"square_fill",
							MarkerDiamondFill,	"diamond_fill",
							MarkerTriangleFill,	"triangle_fill",
							MarkerRect,			"rectangle",
							MarkerRectFill,		"rectangle_fill"
							};
static	const	int		NumMtype = ITBL_SIZE(MtypeDefs);

/* Functions in gx.c */
LOGICAL		gxFindPoption(STRING, STRING);
LOGICAL		gxOpenGraphics(Display *, Window);
LOGICAL		gxCloseGraphics(LOGICAL);
LOGICAL		gxOpenDump(int, int);
LOGICAL		gxCloseDump(void);

/* Functions in gx_colour.c */
COLOUR		gxFindColour(STRING);
Pixel		gxColorIndex(COLOUR, HILITE);
void		gxSetColorIndex(COLOUR, HILITE);

/* Functions in gx_line.c */
LSTYLE		gxFindLineStyle(STRING);
float		gxFindLineWidth(STRING);
void		gxLineStyle(LSTYLE, float, float);
void		gxLineSpec(LSPEC *);

/* Functions in gx_fill.c */
FSTYLE		gxFindFillStyle(STRING);
void		gxFillStyle(FSTYLE, LOGICAL);
LOGICAL		gxNeedFill(FSTYLE);
LOGICAL		gxNeedPreFill(FSTYLE, HILITE);
void		gxFillSpec(FSPEC *, LOGICAL);

/* Functions in gx_marker.c */
MTYPE		gxFindMarkerType(STRING);
void		gxMarkerSpec(MSPEC *);
void		gxMarkerSize(float);
void		gxMarkerAngle(float);
void		gxDrawMarker(int, float, float);

/* Functions in gx_barb.c */
BTYPE		gxFindBarbType(STRING);
void		gxBarbSpec(BSPEC *);

/* Functions in gx_text.c */
LOGICAL     gxAddFont(STRING, STRING);
FONT		gxFindFont(STRING);
float		gxFindFontSize(STRING);
void        gxReplacePredefinedFonts(STRING);
void		gxTextSpec(TSPEC *);
void		gxTextAlignment(HJUST, VJUST);

/* Functions in gx_trans.c */
void		gxSetMproj(MAP_PROJ *);
MAP_PROJ	*gxGetMproj(void);
void		gxSetZoomMproj(MAP_PROJ *);
MAP_PROJ	*gxGetZoomMproj(void);
void		gxSetPatchInfo(SURFACE, int, int);
SURFACE		gxGetPatchInfo(int *, int *);
void		gxSetWindParms(float, float, STRING, STRING);
void		gxGetWindParms(float *, float *, FpaConfigUnitStruct **,
														FpaConfigUnitStruct **);
float		gxGetMfact(void);
float		gxGetPixelSize(void);
float		gxScaleSize(float, LOGICAL);
void		gxSetupTransform(DISPNODE);
void		gxPushTransform(DISPNODE);
void		gxPopTransform(DISPNODE);

#endif
