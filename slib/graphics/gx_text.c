/***********************************************************************
*                                                                      *
*     g x _ t e x t . c                                                *
*                                                                      *
*     Useful extensions to the FpaXgl library.                         *
*                                                                      *
*     (c) Copyright 1996-2008 Environment Canada (EC)                  *
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

typedef struct
	{ 
	int ival; 
	STRING name; 
	STRING subName;
	} FONTPRETBL;
#define FONTPRETBL_SIZE(list) (sizeof(list)/sizeof(FONTPRETBL))

typedef	enum	{
				FontFixed,
				FontFixedBold,
				FontHelvetica,
				FontHelveticaBold,
				FontHelveticaItalic,
				FontHelveticaBoldItalic,
				FontTimes,
				FontTimesBold,
				FontTimesItalic,
				FontTimesBoldItalic
				} FONT_ID_LIST;

/* List of recognized font keywords */
static	const	ITBL	FontDefs[] =
							{
							FontFixed,				 "fixed",
							FontFixed,				 "stick",
							FontFixed,				 "computer",
							FontFixed,				 "simplex",
							FontFixedBold,			 "fixed_bold",
							FontFixedBold,			 "fixedbold",
							FontFixedBold,			 "triplex",
							FontFixedBold,			 "bold",
							FontHelvetica,			 "helvetica",
							FontHelvetica,			 "sans_serif",
							FontHelvetica,			 "sansserif",
							FontHelveticaBold,		 "helvetica_bold",
							FontHelveticaBold,		 "helveticabold",
							FontHelveticaBold,		 "sans_serif_bold",
							FontHelveticaBold,		 "sansserifbold",
							FontHelveticaItalic,	 "helvetica_italic",
							FontHelveticaItalic,	 "helveticaitalic",
							FontHelveticaItalic,	 "sans_serif_italic",
							FontHelveticaItalic,	 "sansserifitalic",
							FontHelveticaBoldItalic, "helvetica_bold_italic",
							FontHelveticaBoldItalic, "helveticabolditalic",
							FontHelveticaBoldItalic, "sans_serif_bold_italic",
							FontHelveticaBoldItalic, "sansserifbolditalic",
							FontTimes,				 "times",
							FontTimes,				 "times_roman",
							FontTimes,				 "timesroman",
							FontTimes,				 "serif",
							FontTimesBold,			 "times_bold",
							FontTimesBold,			 "timesbold",
							FontTimesBold,			 "times_roman_bold",
							FontTimesBold,			 "timesromanbold",
							FontTimesBold,			 "serif_bold",
							FontTimesBold,			 "serifbold",
							FontTimesBold,			 "bold_serif",
							FontTimesBold,			 "boldserif",
							FontTimesItalic,		 "times_italic",
							FontTimesItalic,		 "timesitalic",
							FontTimesItalic,		 "times_roman_italic",
							FontTimesItalic,		 "timesromanitalic",
							FontTimesItalic,		 "serif_italic",
							FontTimesItalic,		 "serifitalic",
							FontTimesItalic,		 "italic_serif",
							FontTimesItalic,		 "italicserif",
							FontTimesBoldItalic,	 "times_bold_italic",
							FontTimesBoldItalic,	 "timesbolditalic",
							FontTimesBoldItalic,	 "times_roman_bold_italic",
							FontTimesBoldItalic,	 "timesromanbolditalic",
							FontTimesBoldItalic,	 "serif_bold_italic",
							FontTimesBoldItalic,	 "serifbolditalic",
							FontTimesBoldItalic,	 "bold_italic_serif",
							FontTimesBoldItalic,	 "bolditalicserif"
							};
static	const	int		NumFont = ITBL_SIZE(FontDefs);

/* List of pre-recognized font names */
static	FONTPRETBL	FontPre[] =
							{
							FontFixed,				 "misc-fixed-medium-r", NULL,
							FontFixedBold,			 "misc-fixed-bold-r",   NULL,
							FontHelvetica,			 "helvetica-medium-r",  NULL,
							FontHelveticaBold,		 "helvetica-bold-r",    NULL,
							FontHelveticaItalic,	 "helvetica-medium-o",  NULL,
							FontHelveticaBoldItalic, "helvetica-bold-o",    NULL,
							FontTimes,				 "times-medium-r",      NULL,
							FontTimesBold,			 "times-bold-r",        NULL,
							FontTimesItalic,		 "times-medium-i",      NULL,
							FontTimesBoldItalic,	 "times-bold-i",        NULL
							};
static	int		NumPre = FONTPRETBL_SIZE(FontPre);

/* Currently registered fonts */
static	ITBL	*RegFontList = 0;
static	int		NumRegFont   = 0;

/* List of recognized font sizes */
static	const	ITBL	SizeDefs[] =
							{
							20,	"label",
							25,	"legend",
							40,	"hilo",
							5,	"tiny",
							10,	"small",
							30,	"medium",
							50,	"large",
							70,	"huge",
							90,	"giant",
							90,	"gigantic",
							90,	"enormous",
							90,	"humongous"
							};
static	const	int		NumSize = ITBL_SIZE(SizeDefs);

#define DEBUG_FONT

/***********************************************************************
*   g x R e p l a c e P r e d e f i n e d F o n t s - replace FontPre  *
*                                                                      *
*   Given a input string of the form font_key = descriptor;label,      *
*   font_key = descriptor;label, ...                                   *
*   where:                                                             *
*                                                                      *
*   font_key   - any of the pre-defined font key labels found in the   *
*                FontDefs array.                                       *
*   descriptor - a valid font descriptor                               *
*                                                                      *
***********************************************************************/
void gxReplacePredefinedFonts(STRING replacements)
	{
		int    fid;
		STRING str, eq, lsep; 
		STRING rep = NULL;
		STRING ptr = NULL;
		STRING key = NULL;
		STRING desc = NULL;

		const STRING module = "gxReplacePredefinedFonts";

		if(blank(replacements)) return;

		rep = str = strdup(replacements);
		while((key = strtok(str,",")))
			{
			str = NULL;
			FREEMEM(ptr);
			ptr = strdup(key);
			if(!(eq = strchr(key,'=')))
				{
				pr_error(module, "Unable to find '=' in defaultMapFont \"%s\"\n", ptr);
				continue;
				}
			if(blank(eq+1))
				{
				pr_error(module, "No information found after '=' in defaultMapFont \"%s\".\n", ptr);
				continue;
				}
			*eq = '\0';
			no_white(key);
			fid = find_itbl_entry(FontDefs, NumFont, key);
			if (fid < 0)
				{
					pr_error(module,"Font key in defaultMapFont \"%s\" is not valid.\n", ptr);
					continue;
				}
			desc = eq + 1;
			no_white(desc);
			FontPre[fid].subName = strdup(desc);
			}

		FREEMEM(rep);
		FREEMEM(ptr);
	}


/***********************************************************************
*                                                                      *
*   g x A d d F o n t  - add a font to the registered list             *
*                                                                      *
*   If the font id given by name already exists, then replace the      *
*   font with the one given by descriptor.                             *
*                                                                      *
*   name       - the identifier used to find the font                  *
*   descriptor - a standard X iso font descriptor ("helvetica-bold-r") *
*                                                                      *
***********************************************************************/

LOGICAL	gxAddFont ( STRING name, STRING descriptor )
	{
	int    ndx, fid;
	STRING desc;

	/* Pre-register known fonts */
	if (!RegFontList)
		{
		NumRegFont  = NumPre;
		RegFontList = INITMEM(ITBL, NumRegFont);
		for (ndx=0; ndx<NumRegFont; ndx++)
			{
			if(FontPre[ndx].subName && glIsFont(FontPre[ndx].subName))
				{
				RegFontList[ndx].ival = glLoadFont(FontPre[ndx].subName);
				RegFontList[ndx].name = strdup(FontPre[ndx].subName);
				}
				else
				{
				RegFontList[ndx].ival = glLoadFont(FontPre[ndx].name);
				RegFontList[ndx].name = strdup(FontPre[ndx].name);
				}
			}
#		ifdef DEBUG_FONT
		for (ndx=0; ndx<NumRegFont; ndx++)
			(void) printf("[gxAddFont] Default font %d: %d %s\n",
					ndx, RegFontList[ndx].ival, RegFontList[ndx].name);
#		endif /* DEBUG_FONT */
		}

	/* Quit if no valid input */
	if(blank(name) || blank(descriptor)) return FALSE;

	/* copy as leading and trailing spaces need to be stripped */
	desc = strdup(descriptor);
	no_white(desc);

	/* if the descriptor is a dash then assume the default for name
	 * is wanted and thus name must be one of our internal fonts.
	 */
	if(same(desc,"-"))
		{
		(void) free(desc);
		return TRUE;
		}

	fid = glLoadFont(desc);
	(void) free(desc);

	/* glLoadFont returns 0 as the default if the font is not found */
	if(fid < 1) return FALSE;

	/* First see if the name is already in our registered font list.
	 * If it is, then we must replace the existing font id with a
	 * new one.
	 */
	for(ndx=0; ndx<NumRegFont; ndx++)
		{
		if(!same_ic(RegFontList[ndx].name, name)) continue;
		RegFontList[ndx].ival = fid;
		return TRUE;
		}

	/* The font is new so register it (using the next available index) */
	NumRegFont++;
	RegFontList = GETMEM(RegFontList, ITBL, NumRegFont);
	RegFontList[NumRegFont-1].ival = fid;
	RegFontList[NumRegFont-1].name = strdup(name);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   g x F i n d F o n t           - map font names                     *
*                                                                      *
***********************************************************************/

FONT	gxFindFont ( STRING name )
	{
	int		fid;

	/* Pre-register known fonts */
	if (!RegFontList)
		{
		NumRegFont  = NumPre;
		RegFontList = INITMEM(ITBL, NumRegFont);
		for (fid=0; fid<NumRegFont; fid++)
			{
			if(glIsFont(FontPre[fid].subName))
				{
				RegFontList[fid].ival = glLoadFont(FontPre[fid].subName);
				RegFontList[fid].name = strdup(FontPre[fid].subName);
				}
				else
				{
				RegFontList[fid].ival = glLoadFont(FontPre[fid].name);
				RegFontList[fid].name = strdup(FontPre[fid].name);
				}
			}
		}
	
	if (blank(name)) name = glDefaultFontName;

	/* Try the registered fonts first */
	fid = find_itbl_entry(RegFontList, NumRegFont, name);
	if (fid > 0) return (FONT) fid;

	/* Next try the keywords and see if it is registered */
	fid = find_itbl_entry(FontDefs, NumFont, name);
	if (fid >= 0 && fid < NumRegFont)
		{
		fid = find_itbl_entry(RegFontList, NumRegFont, RegFontList[fid].name);
		if (fid > 0) return (FONT) fid;
		}

	/* Register it (using the next available index) */
	NumRegFont++;
	RegFontList = GETMEM(RegFontList, ITBL, NumRegFont);
	RegFontList[NumRegFont-1].ival = glLoadFont(name);
	RegFontList[NumRegFont-1].name = strdup(name);
	return (FONT) RegFontList[NumRegFont-1].ival;
	}

/***********************************************************************
*                                                                      *
*   g x F i n d F o n t S i z e   - map font size names                *
*                                                                      *
***********************************************************************/

float	gxFindFontSize ( STRING name )
	{
	float	val;
	STRING  p;
	
	/* Interpret as an actual number first */
	val = (float) strtod(name, &p);
	if (p != name)
		{
		if (blank(p))        return (val);
		if (same(p, "%"))    return (val * 10);
		if (same_ic(p, "P")) return (val * 20);
		}

	val = (float) find_itbl_entry(SizeDefs, NumSize, name);
	if (val >= 0) return (val);

	return -1.0;
	}

/***********************************************************************
*                                                                      *
*   g x T e x t A l i g n m e n t   - set text alignment codes         *
*                                                                      *
***********************************************************************/

void	gxTextAlignment ( HJUST h, VJUST v )
	{
	int		align = glMCENTRE;

	switch (v)
		{
		case VT:
		case Vt:	switch (h)
						{
						case Hl:	align = glTLEFT;	break;
						case Hc:	align = glTCENTRE;	break;
						case Hr:	align = glTRIGHT;	break;
						default:	align = glTCENTRE;	break;
						}

		case Vb:
		case VB:	switch (h)
						{
						case Hl:	align = glBLEFT;	break;
						case Hc:	align = glBCENTRE;	break;
						case Hr:	align = glBRIGHT;	break;
						default:	align = glBCENTRE;	break;
						}

		case Vc:
		default:	switch (h)
						{
						case Hl:	align = glMLEFT;	break;
						case Hc:	align = glMCENTRE;	break;
						case Hr:	align = glMRIGHT;	break;
						default:	align = glMCENTRE;	break;
						}
		}

	glTextAlignment(align);
	}

/***********************************************************************
*                                                                      *
*   g x T e x t S p e c   - invoke given text spec                     *
*                                                                      *
***********************************************************************/

void	gxTextSpec ( TSPEC *tspec )
	{
	float	size;

	/* Compute size */
	size = gxScaleSize(tspec->size/1000.0, tspec->scale);

	gxSetColorIndex(tspec->colour, tspec->hilite);
	glSetFont(tspec->font);
	glSetVdcFontSize(size);
	glTextAngle(tspec->angle);
	gxTextAlignment(tspec->hjust, tspec->vjust);
	}
