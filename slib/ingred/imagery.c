/***********************************************************************
*                                                                      *
*     i m a g e r y . c                                                *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all display and edit functions for imagery underlaid     *
*     on the current depiction.                                        *
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

#include "ingred_private.h"

#include <time.h>

#undef DEBUG_DISPLAY

/* Imagery save control */
typedef	struct tbuf_struct
	{
	STRING	tag;
	STRING	vtime;
	Image	image;
	} TBUF;
static	int		MaxTbuf = 0;
static	int		NumTbuf = 0;
static	TBUF	*Tbuf   = (TBUF *)0;

static	TBUF	*find_tbuf(STRING, STRING);
static	TBUF	*save_tbuf(STRING, STRING, Image);
static	TBUF	*make_tbuf(STRING, STRING);

/***********************************************************************
*                                                                      *
*     Functions which control the entire imagery module:               *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*     s h o w _ i m a g e r y                                          *
*     h i d e _ i m a g e r y                                          *
*     p r e s e n t _ i m a g e r y                                    *
*                                                                      *
***********************************************************************/

LOGICAL	show_imagery(void)

	{
	if (!ImageShown)
		{
		/* Set display state on */
		define_dn_vis(DnImage, TRUE);
		ImageShown = TRUE;
		}

	/* Set dispnode visibilities according to field state */
	/* to display pending selected fields */
	(void) sync_imagery();

	return present_all();
	}

/**********************************************************************/

LOGICAL	hide_imagery(void)

	{
	if (!ImageShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnImage, FALSE);
	ImageShown = FALSE;
	return present_all();
	}

/**********************************************************************/

LOGICAL	present_imagery

	(
	LOGICAL	all
	)

	{
	if (!ImageShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnImage);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     a c t i v e _ i m a g e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	active_image

	(
	 STRING tag,
	 STRING vtime
	)

	{
	TBUF	*tbuf;

	pr_diag("Image", "Find active image: %s %s\n", tag, vtime);

	tbuf = find_tbuf(tag, vtime);
	if (IsNull(tbuf)) return FALSE;

	pr_diag("Image", "Set active image: %s %s\n", tag, vtime);

	ActiveImage = tbuf->image;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d i s p l a y _ i m a g e r y                                    *
*     r e m o v e _ i m a g e r y                                      *
*     b l e n d _ i m a g e r y                                        *
*     l u t _ i m a g e r y                                            *
*     i m a g e r y _ c h e c k                                        *
*     i m a g e r y _ s a m p l e                                      *
*     s y n c _ i m a g e r y                                          *
*                                                                      *
***********************************************************************/

/* Imagery buffers */
static	int		NumP1   = 0;
static	int		MaxP1   = 0;
static	Image	*Plist1 = (Image *)0;
static  Image   Plane1  = (Image)0;
static	LOGICAL	Blend   = FALSE;
static	int		Ratio   = 0;
static	LOGICAL	ImReset = TRUE;

/**********************************************************************/

/* XXX Note that for now the plane reference is ignored. Once the
 * static background display plane is implemented the plane will
 * once more have meaning.
 */
LOGICAL	display_imagery

	(
	int		plane,
	STRING	tag,
	STRING	vtime
	)
	{

	TBUF	*tbuf;

	if (ImReset)
		{
		ImReset = FALSE;
		NumP1 = 0;
		}

	tbuf = make_tbuf(tag, vtime);
	if (IsNull(tbuf)) return FALSE;
	if (!tbuf->image) return FALSE;

	NumP1++;
	if (NumP1 > MaxP1)
		{
		MaxP1 = NumP1;
		Plist1 = GETMEM(Plist1, Image, NumP1);
		}
	Plist1[NumP1-1] = tbuf->image;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	remove_imagery

	(
	STRING	tag
	)

	{
	int	 i, count;
	TBUF *tb;

	if (ImReset)
		{
		ImReset = FALSE;
		NumP1 = 0;
		}

	/* remove tbuf entries for this tag */
	for (count=0, i=0; i<NumTbuf; i++)
		{
		if (same(tag, Tbuf[i].tag))
			{
			glImageDestroy(Tbuf[i].image);
			FREEMEM(Tbuf[i].tag);
			FREEMEM(Tbuf[i].vtime);
			}
		else
			{
			Tbuf[count].image = Tbuf[i].image;
			Tbuf[count].tag   = Tbuf[i].tag;
			Tbuf[count].vtime = Tbuf[i].vtime;
			count++;
			}
		}
	NumTbuf = count;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	blend_imagery

	(
	LOGICAL	state,
	int		ratio
	)

	{
	Blend = state;
	Ratio = ratio;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	lut_imagery

	(
	 STRING tag,
	 STRING lutstr
	)

	{
	ImageLUT	lut;

	if(same_ic(lutstr,"DEFAULT"))
	{
		glImageSetTagLUT(tag, glNoLUT);
	}
	else if(same_ic(lutstr,"NONE"))
	{
		glImageSetTagLUT(tag, glNoLUT);
	}
	else
	{
		if(sscanf(lutstr, "%d", &lut) != 1) return FALSE;
		glImageSetTagLUT(tag, lut);
	}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	imagery_check(void)

	{
	if (!ImageShown) return FALSE;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	imagery_sample

	(
	 STRING mode
	)

	{
	LOGICAL	valid;
	FONT	fnt;
	float	sz;
	COLOUR	clr;

	/* Handle "normal" sample */
	if (same(EditMode, "NORMAL"))
	    {
		valid = eval_list_reset();
		if (!valid) return FALSE;
		}

	/* Handle "grid" sample */
	else if (same(EditMode, "GRID"))
	    {
		valid = eval_list_grid(EditVal[0], EditVal[1]);
		move_edit_vals(0, 2);
		if (!valid) return FALSE;
		}

	/* Handle "list" sample */
	else if (same(EditMode, "LIST"))
	    {
		if (same(EditVal[0], "GO"))
			{
			move_edit_vals(0, 1);
			}
		else
			{
			valid = eval_list_add(EditVal[0], EditVal[1]);
			move_edit_vals(0, 2);
			if (!valid) return FALSE;
			return FALSE;
			}
		}

	/* Unknown command */
	else
	    {
		put_message("image-unsupported", EditMode, "");
		return FALSE;
	    }

	/* Came here from "NORMAL", "GRID" or "LIST GO" */

	/* Check the image to be sampled */
	if (!ActiveImage) return FALSE;
	pr_diag("Image", "Sampling active image\n");

	/* Determine display attributes */
	pr_diag("Image", "Sampling image appearance: %s %s %s\n",
		EditVal[2], EditVal[3], EditVal[4]);
	if (blank(EditVal[2]))
		{
		(void) strcpy(EditVal[2], "bold");
		(void) strcpy(EditVal[3], "4%");
		(void) strcpy(EditVal[4], "yellow");
		}
	fnt = find_font(EditVal[2], &valid);
	sz  = find_size(EditVal[3], &valid);
	clr = find_colour(EditVal[4], &valid);
	sample_appearance(fnt, sz, clr);

	/* Resume in normal mode */
	(void) strcpy(EditMode, "NORMAL");

	(void) sample_image(mode, ActiveImage, EditVal[0], EditVal[1]);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	sync_imagery(void)

	{
	LOGICAL	susp;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	ImReset = TRUE;

	if (!ImageShown) return TRUE;

	if (NumP1<=0)
		{
		pr_diag("Image", "No images selected\n");
		define_dn_data(DnImage, "special:image", (POINTER) 0);
		/* present_all(); */
		return TRUE;
		}

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	susp = (LOGICAL) (Plane1 == 0);
	if (susp) suspend_zoom();
	gxSetupTransform(DnImage);

	Plane1 = glImageComposite(Plane1, MapProj, Plist1, NumP1, Blend? Ratio:100);

	if (susp) resume_zoom(FALSE);

	DnImage->data.ptr = NullPointer;
	define_dn_data(DnImage, "special:image", (POINTER) ((long)Plane1));

#	ifdef DEBUG_DISPLAY
	tnxt = (long) clock();
	if (tnxt - tbgn > tdiff)
		printf("[sync_imagery] Imagery: %d to %d\n", tbgn, tnxt);
#	endif /* DEBUG_DISPLAY */

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     f i n d _ t b u f                                                *
*     s a v e _ t b u f                                                *
*     m a k e _ t b u f                                                *
*                                                                      *
***********************************************************************/

static	TBUF	*find_tbuf(STRING tag, STRING vtime)
	{
	int	i;

	for (i=0; i<NumTbuf; i++)
		{
		if (!same(tag, Tbuf[i].tag))     continue;
		if (!same(vtime, Tbuf[i].vtime)) continue;
		return Tbuf+i;
		}
	return (TBUF *)0;
	}

/**********************************************************************/

static	TBUF	*save_tbuf(STRING tag, STRING vtime, Image image)
	{
	TBUF	*tbuf;

	tbuf = find_tbuf(tag, vtime);
	if (NotNull(tbuf))
		{
		/* if (NotNull(tbuf->image)) gxReleaseImage(tbuf->image); */
		tbuf->image = image;
		return tbuf;
		}

	NumTbuf++;
	if(NumTbuf > MaxTbuf)
		{
		MaxTbuf = NumTbuf;
		Tbuf = GETMEM(Tbuf, struct tbuf_struct, MaxTbuf);
		}
	tbuf = Tbuf + NumTbuf-1;
	tbuf->tag   = strdup(tag);
	tbuf->vtime = strdup(vtime);
	tbuf->image = image;
	return tbuf;
	}

/**********************************************************************/

static	TBUF	*make_tbuf(STRING tag, STRING vtime)
	{
	TBUF	*tbuf;
	Image	image;

	tbuf = find_tbuf(tag, vtime);
	if (NotNull(tbuf)) return tbuf;

	gxSetupTransform(DnImage);
	image = glImageFetch(tag, vtime, MapProj);

	return save_tbuf(tag, vtime, image);
	}
