/*********************************************************************************/
/*
*    FpaXgl.h
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/*********************************************************************************/

#ifndef _FPAXGL_H
#define _FPAXGL_H

/* Undefine true and false as the compiler complains about these
 * being redefined in Xlib.h and it is annoying. Undefine index
 * and rindex as compiler complains about these in Xos.h
 */
#undef True
#undef False
#undef index
#undef rindex

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

/* Now redefine True and False
 */
#ifndef True
#define True	1
#define False	0
#endif

/* For fonts
 */
#define glDefaultFontName	"default"

/* For colours
 */
#define UnallocatedColorIndex -1

/*For glLineStyle - any one member of a group may be or'ed with
                    any member of another group
*/
#define glSOLID         1
#define glDASH          2
#define glFILLED_DASH   3

#define glCAP_NOT_LAST     (1 << 3)
#define glCAP_BUTT         (2 << 3)
#define glCAP_ROUND        (3 << 3)
#define glCAP_PROJECTING   (4 << 3)

#define glJOIN_ROUND       (1 << 6)
#define glJOIN_MITER       (2 << 6)
#define glJOIN_BEVEL       (3 << 6)

/* for glFillStyle
*/
#define glPATTERN_SOLID        FillSolid
#define glPATTERN_TRANSPARENT  FillStippled
#define glPATTERN_BACKGROUND   FillOpaqueStippled
#define glPATTERN_TILE         FillTiled
#define glPATTERN_HATCH        1001                    /* Must be > 1000 */
#define glPATTERN_CROSS_HATCH  1002                    /* Must be > 1000 */

/* for glGetSnapshot
*/
enum { glSERVER_SIDE_PREFERENCE, glSERVER_SIDE_ONLY, glCLIENT_SIDE_ONLY, glCLIENT_SIDE_FILE };

/* for glLogicOp()
*/
enum logicalop{ glLO_ZERO, glLO_AND, glLO_ANDR, glLO_SRC, glLO_ANDI, glLO_DST, glLO_XOR,
                glLO_OR,  glLO_NOR, glLO_XNOR, glLO_NDST, glLO_ORR, glLO_NSRC, glLO_ORI,
                glLO_NAND, glLO_ONE };

/* For text orientation in relation to the cursor position.
 * The order of these is important - do not modify!
*/
enum { glNONE,   glTLEFT, glMLEFT, glBLEFT, glTCENTRE, glMCENTRE, glBCENTRE,
	   glTRIGHT, glMRIGHT, glBRIGHT };

/* For Clip Mode
*/
enum clipmode { glCLIP_OFF, glCLIP_ON };

/* For output raster types
 */
enum { glCOLORSCALE = 1, glGREYSCALE, glBW, glWB };

/* For output file types.
*/
enum { glXWD = 1, glGIF, glPNG, glTIFF, glXGL, glNATIVE };

/* Types
*/
typedef short  ColorIndex;
typedef short  Snapshot;

/* Drawing functions */
extern void  glClear ( void );

    /* Points */
extern void  glPoint ( Coord, Coord );
  
    /* Lines */
extern void  glMove      ( Coord, Coord );
extern void  glRMove     ( Coord, Coord );
extern void  glDraw      ( Coord, Coord );
extern void  glRDraw     ( Coord, Coord );
  
    /* Arcs & Circles */
extern void  glArc          ( Coord, Coord, Coord, Angle, Angle );
extern void  glFilledArc    ( Coord, Coord, Coord, Angle, Angle );
extern void  glArcx         ( Coord, Coord, Coord, Coord, Angle, Angle );
extern void  glFilledArcx   ( Coord, Coord, Coord, Coord, Angle, Angle );
extern void  glCircle       ( Coord, Coord, Coord );
extern void  glFilledCircle ( Coord, Coord, Coord );

    /* Rects & Boxes */
extern void  glRectangle       ( Coord, Coord, Coord, Coord );
extern void  glFilledRectangle ( Coord, Coord, Coord, Coord );
  
    /* Filled Polygons */
extern void  glSetConcave    ( LOGICAL );

extern void  glPolyLine      ( int, Coord (*p)[2] );
extern void  glPolygon       ( int, Coord (*p)[2] );
extern void  glFilledPolygon ( int, Coord (*p)[2] );
extern void  glPolygonHole   ( int, Coord (*p)[2] );

    /* Vertex graphics */
extern void  glBgnPoint          ( void );
extern void  glBgnLine           ( void );
extern void  glBgnPolygon        ( void );
extern void  glBgnFilledPolygon  ( void );
extern void  glBgnPolygonHole    ( void );
  
extern void  glEndPoint         ( void );
extern void  glEndLine          ( void );
extern void  glEndPolygon       ( void );
extern void  glEndFilledPolygon ( void );
extern void  glEndPolygonHole   ( void );
  
extern void  glVert ( float[2] );
  
/* Control functions */
extern void  glSingleBuffer    ( void );
extern void  glDoubleBuffer    ( void );
extern void  glSwapBuffers     ( void );
  
extern void  glFlush           ( void );
extern void  glResetWindow     ( void );
extern void  glResetViewport   ( void );

extern Display *glGetDisplay  ( void );
extern Drawable glGetDrawable ( void );

extern int   glGetPlanes       ( void );

extern void  glClipMode            ( int );

extern void  glLineWidth    ( int );
extern void  glVdcLineWidth ( float );
extern void  glMapLineWidth ( float );
extern int   glGetLineWidth ( void );

extern void  glLineStyle    ( int );

extern void  glDashStyle    ( int, int[] );
extern void  glVdcDashStyle ( int, float[] );
extern void  glMapDashStyle ( int, float[] );

extern void glFillStyle       ( int );
extern void glFillPattern     ( int , int, unsigned char* );
extern void glTilePattern     ( int , int, char** );

extern void  glLogicOp ( int );  

/* Text */
extern void        glDrawString            ( Coord, Coord, char * );
extern int         glGetFont               ( void );
extern LOGICAL     glIsFont                ( char* );
extern int         glLoadFont              ( char* );
extern void        glSetDefaultFont        ( char* );
extern void        glSetFont               ( int );
extern void        glSetFontSize           ( int );
extern void        glSetMapFontSize        ( float );
extern void        glSetVdcFontSize        ( float );
extern void        glTextAlignment         ( int );
extern void        glTextAngle             ( Angle );

  
/* Colour */
extern Pixel      glGetPixelFromColorIndex ( ColorIndex );
extern void       glLoadColor              ( ColorIndex ndx, char *color_name);
extern ColorIndex glReservedColorIndex     ( char *color_name );
extern ColorIndex glSetColor               ( char* );
extern ColorIndex glSetLineColor           ( char* );
extern ColorIndex glSetFillColor           ( char* );
extern ColorIndex glSetTextColor           ( char* );
extern ColorIndex glSetBgColor             ( char* );
extern ColorIndex glSetLineBgColor         ( char* );
extern ColorIndex glSetFillBgColor         ( char* );
extern ColorIndex glSetTextBgColor         ( char* );
extern void       glSetColorIndex          ( ColorIndex );
extern void       glSetLineColorIndex      ( ColorIndex );
extern void       glSetFillColorIndex      ( ColorIndex );
extern void       glSetTextColorIndex      ( ColorIndex );
extern void       glSetBgColorIndex        ( ColorIndex );
extern void       glSetBgLineColorIndex    ( ColorIndex );
extern void       glSetBgFillColorIndex    ( ColorIndex );
extern void       glSetBgTextColorIndex    ( ColorIndex );
extern void       glGetColormap            ( XColor**, int*);

/* Creation and initialization */
extern void  glInit                     ( void );
extern int   glInitWindow               ( Display*, Window );
extern int   glCreatePixmapFromXWindow  ( Display*, Window, int, int );
extern int   glCreatePixmap             ( void );
extern int   glCreateBkgndWindow        ( int, int );
extern void  glGetWindowSize            ( int *, int * );
extern void  glResetDisplayConnection   ( void );
extern void  glSetWindowAttributes      ( unsigned long, XSetWindowAttributes* );
extern void  glGetVdcPixelSize          ( float*, float* );

/* Output */
extern void glFileToWindow ( char*, LOGICAL );

/* Motif interface */
extern int  glInitWidgetWindow (Widget);

/* Snapshots */
extern void     glClearAllSnapshots      ( void );
extern void     glClearSnapshot          ( Snapshot );
extern Snapshot glGetSnapshot            ( int );
extern Snapshot glGetBkgndWindowSnapshot ( int, int );
extern void     glPutSnapshot            ( Snapshot );

/* File output */
extern void  glSetGrayscaleNameConversion            ( char**, Pixel*, int );
extern void  glSetGrayscaleColorIndexConversion      ( ColorIndex*, Pixel*, int );
extern void  glWindowToFile                          ( char*, int, int );

  
#endif /* _FPAXGL_H */
