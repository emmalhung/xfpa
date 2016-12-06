/**************************************************************************
*                                                                         *
*     t e x t _ p a n e l . c                                             *
*                                                                         *
*     Routines to manipulate a "text" dispnode.                           *
*                                                                         *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)               * 
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
*                                                                         *
**************************************************************************/

#include "panel.h"
#include "display.h"
#include "gx.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static			SET		Tset  = NULL;
static			LABEL	Tlab  = NULL;
static	const	POINT	Tpos  = { 20.0, 3.0 };
static	const	TSPEC	Tspec = { 0, -1, -1, 4, True, 25, 0, Hl, VB, 0 };

/***********************************************************************
*                                                                      *
*     p r i n t _ t e x t                                              *
*     b l a n k _ t e x t                                              *
*     d e f i n e _ t e x t                                            *
*                                                                      *
***********************************************************************/

void	print_text

	(
	DISPNODE	tnode,	/* text window */
	COLOUR		colour,	/* colour of text */
	STRING		text	/* text to be printed */
	)

	{
	define_text(tnode, colour, text);
	display_dispnode(tnode);
	}

/*************************************************************************/

void		blank_text

	(
	DISPNODE	tnode	/* text window */
	)

	{
	define_text(tnode, 0, NULL);
	display_dispnode(tnode);
	}

/*************************************************************************/

void		define_text

	(
	DISPNODE	tnode,	/* text window */
	COLOUR		colour,	/* colour of text */
	STRING		text	/* text to be printed */
	)

	{
	STRING	type;
	POINTER	data;

	if (!Tlab)
		{
		Tset = create_set("label");
		Tlab = create_label("message", "", text, Tpos, 0.0);
		add_item_to_set(Tset, (ITEM) Tlab);
		copy_tspec(&Tlab->tspec, &Tspec);
		}
	else
		{
		define_label_value(Tlab, "message", "", text);
		}
	Tlab->tspec.colour = colour;

	if (!tnode) return;
	delete_dn_subtree(tnode);
	recall_dn_data(tnode, &type, &data);
	if (data == (POINTER)Tset) return;
	define_dn_data(tnode, "set", (POINTER) Tset);
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ t e x t                                              *
*     y e s n o _ t e x t                                              *
*                                                                      *
***********************************************************************/

STRING		build_text

	(
	DISPNODE	tnode,	/* text window */
	COLOUR		colour,	/* colour of text */
	STRING		prompt,	/* prompt string */
	STRING		dtext,	/* current string */
	UNCHAR		key	/* key to add */
	)

	{
	static	char	Message[256] = "";
	static	char	Ctext[128]   = "";
	static	char	Ptext[128]   = "";
	static	int		Code         = 0;

	/* Initialize string buffers if starting from scratch */
	if (Code == 0)
		{
		(void) safe_strcpy(Ctext, dtext);
		(void) safe_strcpy(Ptext, dtext);
		}

	/* Interpret the given key and add to the current string if appropriate */
	Code = build_string(Ctext, key, 128);

	/* If 'escaped' toggle current text with default text */
	if (Code < 0)
		{
		if (same (Ctext, dtext))
			{
			/* Already swapped - swap back */
			(void) safe_strcpy(Ctext, Ptext);
			(void) safe_strcpy(Ptext, dtext);
			}
		else
			{
			(void) safe_strcpy(Ptext, Ctext);
			(void) safe_strcpy(Ctext, dtext);
			}
		}

	/* Display the full text */
	(void) sprintf(Message, "%s: %s_", SafeStr(prompt), SafeStr(Ctext));
	print_text(tnode, colour, Message);

	if (Code == 0) return Ctext;
	else           return NULL;
	}

/*************************************************************************/

STRING		yesno_text

	(
	DISPNODE	tnode,	/* text window */
	COLOUR		colour,	/* colour of text */
	STRING		prompt,	/* prompt string */
	STRING		yes,	/* string to match and return for yes */
	STRING		no,		/* string to match and return for no */
	LOGICAL		dstate,	/* default state */
	UNCHAR		key		/* key to add */
	)

	{
	static	char	Message[256] = "";
	static	char	Dummy[3]     = "a";
	static	int		State        = FALSE;
	static	int		Code         = 0;

	/* Initialize string buffers if starting from scratch */
	if (Code == 0)
		{
		State = dstate;
		}

	/* Interpret the given key - use a dummy string just to get the code */
	(void) strcpy(Dummy, "a");
	Code = build_string(Dummy, key, 3);

	/* If 'escaped' toggle current state with default state */
	if (Code < 0)
		{
		State = !State;
		}
	
	/* Or set the current state if the given key matches the first letter */
	/* of one of the choices */
	else if (Code > 0)
		{
		if (!blank(yes) && key == toupper(yes[0])) State = TRUE;
		if (!blank(yes) && key == tolower(yes[0])) State = TRUE;
		if (!blank(no)  && key == toupper(no[0]))  State = FALSE;
		if (!blank(no)  && key == tolower(no[0]))  State = FALSE;
		}

	/* Display the full text */
	(void) sprintf(Message, "%s: %s_", SafeStr(prompt), (State)? yes: no);
	print_text(tnode, colour, Message);

	if (Code == 0) return (State)? yes: no;
	else           return NULL;
	}
