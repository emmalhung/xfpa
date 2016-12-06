/*========================================================================*/
/*
*	File:		graphic.h
*
*   Purpose:    Header file for graphics*.c program files.
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
*/
/*========================================================================*/

#ifndef _GRAPHIC_H
#define _GRAPHIC_H

/* Define the macro recoginzed for directory substitution in the Metafile pdf.
*/
#define METADIR      "$METADIR"

/* Define the recognized graphics types
*/
typedef enum { GRAPHICS = 1, METAFILES, FPAMET } GR_TYPES;

/* Define the product definition structure. Note that not all of the parameters are
*  used by every product, but all are used by fpamet type products.
*/
typedef struct {
    GR_TYPES type;              /* type of product */
    int      pid;               /* product id from ProductStatusAddInfo */
    String   label;             /* product description seen by user */
    String   program;           /* name of product generation program */
    String   dir;               /* source directory(s) */
    String   pdf;               /* product definition file */
	Boolean  is_list_label;		/* is this actually a list label? */
    Boolean  editable;          /* can the product be edited */
    Boolean  viewable;          /* can the product be previewed */
    Boolean  printable;         /* is a print definition in the pdf */
    Boolean  generate;          /* generate this product? */
    String   generate_times;    /* list of depiction times to output */
    Boolean  print;             /* print thie product? */
    String   print_times;       /* list of depiction times to print */
    Boolean  running;           /* is the product being generated or printed? */
} GraphicProdStruct;

/* Prototype the functions local the the graphic functions.
*/
extern void GraphicPreviewShow				(GraphicProdStruct*);
extern void GraphicPreviewPopdown			(void);
extern void InitGraphicProductsDialog		(void);
extern void ACTIVATE_graphicProductsDialog	(Widget refw );

#endif /* _GRAPHIC_H */
