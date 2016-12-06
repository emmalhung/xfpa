/***********************************************************************
*                                                                      *
*     g x _ m a r k e r . c                                            *
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

/***********************************************************************
*                                                                      *
*   g x F i n d M a r k e r T y p e   - map marker type names          *
*                                                                      *
***********************************************************************/

MTYPE	gxFindMarkerType ( STRING name )
	{
	return (MTYPE) find_itbl_entry(MtypeDefs, NumMtype, name);
	}

/***********************************************************************
*                                                                      *
*    g x M a r k e r S i z e                                           *
*    g x M a r k e r A n g l e                                         *
*    g x D r a w M a r k e r                                           *
*                                                                      *
***********************************************************************/

static	const	float	MinSize = 0.002;
static	float			Msize   = 1.0;
static	float			Mangle  = 0.0;
static	float			Mcang   = 1.0;
static	float			Msang   = 0.0;

void	gxMarkerSize
	(
	float	size
	)
	{
	/* >>> Need to convert to map units <<< */
	Msize = MAX(size, MinSize) * gxGetMfact();
	}

void	gxMarkerAngle
	(
	float	angle
	)
	{
	Mangle = angle;
	Mcang  = cosdeg(angle);
	Msang  = sindeg(angle);
	}

void	gxDrawMarker
	(
	int		type,
	float	xo,
	float	yo
	)
	{
	float	x, y, cang, sang, Osize;
	LINE    poly;
	POINT   point;

	cang = Msize * Mcang;
	sang = Msize * Msang;

	switch (type)
		{
		case MarkerDot:
				glMove(xo, yo);
				glDraw(xo, yo);
				break;

		case MarkerCircle:
				glCircle(xo, yo, Msize);
				break;

		/* end angle > 360 to prevent line from appearing */
		case MarkerEllipse:
				if ( Mangle >= 90 && Mangle < 180 )
					glArcx(xo,yo,Msize,1.5*Msize,0,365);
				else
					glArcx(xo,yo,1.5*Msize,Msize,0,365);
				break;

		/* end angle > 360 to prevent line from appearing */
		case MarkerEllipseFill:
				glFillStyle(glPATTERN_SOLID);
				if ( Mangle >= 90 && Mangle < 180 )
					glFilledArcx(xo,yo,Msize,1.5*Msize,0,365);
				else
					glFilledArcx(xo,yo,1.5*Msize,Msize,0,365);
				break;

		case MarkerCircleFill:
				glFillStyle(glPATTERN_SOLID);
				glFilledCircle(xo, yo, Msize);
				break;

		case MarkerSquareFill:
				glFillStyle(glPATTERN_SOLID);
				poly = create_line();
				point[X] = xo + cang + sang;
				point[Y] = yo - sang + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang - sang;
				point[Y] = yo - sang - cang;
				add_point_to_line(poly, point);
				point[X] = xo - cang - sang;
				point[Y] = yo + sang - cang;
				add_point_to_line(poly, point);
				point[X] = xo - cang + sang;
				point[Y] = yo + sang + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang + sang;
				point[Y] = yo - sang + cang;
				add_point_to_line(poly, point);
				glFilledPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerSquare:
				poly = create_line();
				point[X] = xo + cang + sang;
				point[Y] = yo - sang + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang - sang;
				point[Y] = yo - sang - cang;
				add_point_to_line(poly, point);
				point[X] = xo - cang - sang;
				point[Y] = yo + sang - cang;
				add_point_to_line(poly, point);
				point[X] = xo - cang + sang;
				point[Y] = yo + sang + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang + sang;
				point[Y] = yo - sang + cang;
				add_point_to_line(poly, point);
				glPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerRect:
				if ( Mangle >= 90 && Mangle < 180 )
				{
					x = 1;
					y = 1.5;
				}
				else
				{
					x = 1.5;
					y = 1;
				}
				poly = create_line();
				point[X] = xo + x*cang + y*sang;
				point[Y] = yo - x*sang + y*cang;
				add_point_to_line(poly, point);
				point[X] = xo + x*cang - y*sang;
				point[Y] = yo - x*sang - y*cang;
				add_point_to_line(poly, point);
				point[X] = xo - x*cang - y*sang;
				point[Y] = yo + x*sang - y*cang;
				add_point_to_line(poly, point);
				point[X] = xo - x*cang + y*sang;
				point[Y] = yo + x*sang + y*cang;
				add_point_to_line(poly, point);
				point[X] = xo + x*cang + y*sang;
				point[Y] = yo - x*sang + y*cang;
				add_point_to_line(poly, point);
				glPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerRectFill:
				glFillStyle(glPATTERN_SOLID);
				if ( Mangle >= 90 && Mangle < 180 )
				{
					x = 1;
					y = 1.5;
				}
				else
				{
					x = 1.5;
					y = 1;
				}
				poly = create_line();
				point[X] = xo + x*cang + y*sang;
				point[Y] = yo - x*sang + y*cang;
				add_point_to_line(poly, point);
				point[X] = xo + x*cang - y*sang;
				point[Y] = yo - x*sang - y*cang;
				add_point_to_line(poly, point);
				point[X] = xo - x*cang - y*sang;
				point[Y] = yo + x*sang - y*cang;
				add_point_to_line(poly, point);
				point[X] = xo - x*cang + y*sang;
				point[Y] = yo + x*sang + y*cang;
				add_point_to_line(poly, point);
				point[X] = xo + x*cang + y*sang;
				point[Y] = yo - x*sang + y*cang;
				add_point_to_line(poly, point);
				glFilledPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerPlus:
				x = xo + sang;
				y = yo + cang;
				glMove(x, y);
				x = xo - sang;
				y = yo - cang;
				glDraw(x, y);

				x = xo + cang;
				y = yo - sang;
				glMove(x, y);
				x = xo - cang;
				y = yo + sang;
				glDraw(x, y);
				break;

		case MarkerTriangle:
				poly = create_line();
				point[X] = xo + sang;
				point[Y] = yo + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang*.866 - sang*.5;
				point[Y] = yo - sang*.866 - cang*.5;
				add_point_to_line(poly, point);
				point[X] = xo - cang*.866 - sang*.5;
				point[Y] = yo + sang*.866 - cang*.5;
				add_point_to_line(poly, point);
				point[X] = xo + sang;
				point[Y] = yo + cang;
				glPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerTriangleFill:
				glFillStyle(glPATTERN_SOLID);
				poly = create_line();
				point[X] = xo + sang;
				point[Y] = yo + cang;
				add_point_to_line(poly, point);
				point[X] = xo + cang*.866 - sang*.5;
				point[Y] = yo - sang*.866 - cang*.5;
				add_point_to_line(poly, point);
				point[X] = xo - cang*.866 - sang*.5;
				point[Y] = yo + sang*.866 - cang*.5;
				add_point_to_line(poly, point);
				point[X] = xo + sang;
				point[Y] = yo + cang;
				glFilledPolygon(poly->numpts, poly->points);
				destroy_line(poly);
				break;

		case MarkerDiamond:
				gxMarkerAngle(Mangle + 45);
				gxDrawMarker(MarkerSquare, xo, yo);
				gxMarkerAngle(Mangle - 45);
				break;

		case MarkerDiamondFill:
				gxMarkerAngle(Mangle + 45);
				gxDrawMarker(MarkerSquareFill, xo, yo);
				gxMarkerAngle(Mangle - 45);
				break;

		case MarkerCross:
				gxMarkerAngle(Mangle + 45);
				gxDrawMarker(MarkerPlus, xo, yo);
				gxMarkerAngle(Mangle - 45);
				break;

		case MarkerSquarePlus:
				gxDrawMarker(MarkerSquare, xo, yo);
				gxDrawMarker(MarkerPlus, xo, yo);
				break;

		case MarkerCirclePlus:
				gxDrawMarker(MarkerCircle, xo, yo);
				gxDrawMarker(MarkerPlus, xo, yo);
				break;

		case MarkerCircleTarget:
				glFilledCircle(xo, yo, Msize*0.6);
				glCircle(xo, yo, Msize);
				break;

		case MarkerAsterisk:
				gxDrawMarker(MarkerCross, xo, yo);
				gxDrawMarker(MarkerPlus, xo, yo);
				break;

		default:
				glMove(xo, yo);
				glDraw(xo, yo);
		}
	}

/***********************************************************************
*                                                                      *
*   g x M a r k e r S p e c   - invoke given marker spec               *
*                                                                      *
***********************************************************************/

void	gxMarkerSpec ( MSPEC *mspec )
	{
	float	size;

	/* Compute size */
	size = gxScaleSize(mspec->size/1000.0, mspec->scale);

	/* Use given marker if non-negative */
	if (mspec->type >= 0)
		{
		gxSetColorIndex(mspec->colour, mspec->hilite);
		glVdcLineWidth(0.0);
		glLineStyle(glSOLID);
		gxMarkerSize(size);
		gxMarkerAngle(mspec->angle);
		}

	/* Otherwise use a text symbol */
	else
		{
		gxSetColorIndex(mspec->colour, mspec->hilite);
		glSetFont(mspec->font);
		glSetVdcFontSize(size);
		glTextAngle(mspec->angle);
		gxTextAlignment(mspec->hjust, mspec->vjust);
		}
	}
