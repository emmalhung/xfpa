/****************************************************************************/
/*
*   File:     snapshot.c
*
*   Purpose:  Contains functions for grabbing an image of the current
*             drawable and then restoring the image to the current drawable.
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
/****************************************************************************/

#include "FpaXglP.h"


static int find_new_index(void)
{
	int ndx;

	/* Do we have an empty slot for a new snapshot?
	*/
	for(ndx = 0; ndx < Xgl.nsnapshot; ndx++)
	{
		if(Xgl.snapshot[ndx].type == SNAP_NONE) break;
	}

	/* If not we must assign memory. Allocate in blocks of 10.
	*/
	if(ndx >= Xgl.nsnapshot)
	{
		Xgl.nsnapshot += 10;
		Xgl.snapshot = GETMEM(Xgl.snapshot, SNAPSHOT, Xgl.nsnapshot);
		(void) memset((void*)&Xgl.snapshot[ndx], 0, 10 * sizeof(SNAPSHOT));
	}

	return ndx;
}


Snapshot glGetSnapshot(int do_what_key)
{
	int      ndx;
	size_t   size;
	UNLONG   plane_mask;
	SNAPSHOT *s;

	ndx = find_new_index();
	s = Xgl.snapshot + ndx;

	s->dpy = D;

	/* Determine the position and size of the area to snapshot from the
	*  current viewport parameters.
	*/
	s->x = W->vl;
	s->y = (int)W->ym - W->vt - 1;
	s->w = (UNINT) (W->vr - W->vl + 1);
	s->h = (UNINT) (W->vt - W->vb + 1);

	plane_mask = (UNLONG) ((1 << WX->depth)-1);
	XSync(D,0);
	switch(do_what_key)
	{
		case glSERVER_SIDE_PREFERENCE:

			s->data.px = XCreatePixmap(D, WX->draw, s->w, s->h, (UNINT)WX->depth);
			if(s->data.px)
			{
				XCopyArea(D, WX->draw, s->data.px, WX->miscgc, s->x, s->y, (UNINT)s->w, (UNINT)s->h, 0, 0);
				s->type = SNAP_PIXMAP;
			}
			else
			{
				s->data.ix = XGetImage(D, WX->draw, s->x, s->y, s->w, s->h, plane_mask, ZPixmap);
				if(!s->data.ix) return 0;
				s->type = SNAP_IMAGE;
			}
			break;

		case glSERVER_SIDE_ONLY:

			s->data.px = XCreatePixmap(D, WX->draw, s->w, s->h, (UNINT)WX->depth);
			if(!s->data.px) return 0;
			s->type = SNAP_PIXMAP;
			XCopyArea(D, WX->draw, s->data.px, WX->miscgc, s->x, s->y, (UNINT)s->w, (UNINT)s->h, 0, 0);
			break;

		case glCLIENT_SIDE_ONLY:

			s->data.ix = XGetImage(D, WX->draw, s->x, s->y, s->w, s->h, plane_mask, ZPixmap);
			if(!s->data.ix) return 0;
			s->type = SNAP_IMAGE;
			break;

		case glCLIENT_SIDE_FILE:
			s->data.ix = XGetImage(D, WX->draw, s->x, s->y, s->w, s->h, plane_mask, ZPixmap);
			if(!s->data.ix) return 0;
			s->type = SNAP_FILE;
			s->fp = tmpfile();
			size = (size_t) (s->data.ix->height * s->data.ix->bytes_per_line);
			(void)fwrite(s->data.ix->data, BYTESIZE, size, s->fp);
			XFREE(s->data.ix->data);
			break;
	}
	return (Snapshot) (ndx+1);
}


/* Function to take a snapshot of the background window. The pixmap that makes up
*  this window does not have a "real" colormap, but rather a synthetic one that is
*  just a number from 0 to ncolors and its depth is limited to 8 bits. This function
*  will snapshot this window, but it targets a specific "real" window with a real
*  colormap for output.
*/
Snapshot glGetBkgndWindowSnapshot(int target_window, int do_what_key)
{
	int        ndx, n, x, y;
	size_t     size;
	ColorIndex cndx;
	UNLONG     plane_mask;
	Pixel      array[256], transparent;
	Pixmap     px = (Pixmap)NULL, mask = (Pixmap)NULL;
	LOGICAL    need_mask;
	SNAPSHOT   *s;
	XImage     *ix = NULL, *pix = NULL, *mix = NULL;
	XglWindow  *wa, *tw;

	static String MyName = "glGetBkdngWindowSnapshot";

	if(WX->dbuf != BKG_BUFFERING)
	{
		pr_error(MyName, "Active window is not a background window\n", NULL);
		return (Snapshot)0;
	}

	(void) memset((void*)array, 0, 256);

	ndx = find_new_index();
	s = Xgl.snapshot + ndx;

	s->dpy = D;

	/* Determine the position and size of the area to snapshot from the
	*  current viewport parameters.
	*/
	s->x = W->vl;
	s->y = (int)W->ym - W->vt - 1;
	s->w = (UNINT)(W->vr - W->vl + 1);
	s->h = (UNINT)(W->vt - W->vb + 1);

	/* Get the background image from the active window.
	*/
	XSync(D,0);
	plane_mask = (UNLONG)((1 << WX->depth)-1);
	ix = XGetImage(D, WX->draw, s->x, s->y, s->w, s->h, plane_mask, ZPixmap);
	if(!ix) goto noxmem;

	/* Take an image of the target drawing window. This is an easy way to avoid the
	*  complication of creating an image directly.
	*/
	tw = &Xgl.windows[target_window];
	plane_mask = (UNLONG)((1 << tw->x->depth)-1);
	pix = XGetImage(D, tw->x->draw, 0, 0, s->w, s->h, plane_mask, ZPixmap);
	if(!pix) goto noxmem;
	FREE_PIXMAP(D, px);

	/* Create the colour table mapping. We Set the active window to the target
	*  window and use the library functions to find the pixel value for the colours
	*  found in the background window in the target window. Note that the background
	*  window pixels are synthetic and go from 0 to ncolors. Using this information
	*  simplifies the following code.
	*/
	wa = W;
	glPushWindow();
	glSetWindow(target_window);
	(void) memset((void*)array, 0, 256*sizeof(Pixel));
	need_mask = FALSE;
	cndx = _xgl_color_index_from_name(MyName, T_NAME, FALSE);
	transparent = _xgl_pixel_from_color_index(MyName, cndx);
	for( n = 0; n < wa->x->ncolors; n++ )
	{
		if(wa->x->colors[n].pixel != transparent)
			array[wa->x->colors[n].pixel] = _xgl_pixel_from_XColor(MyName, wa->x->colors[n]);
		else
			need_mask = TRUE;
	}
	glPopWindow();

	/* Create a mask? If so create a 1 bit deep pixmap and pre-fill it with
	*  1's (do not mask bits), take an image and destroy the pixmap.
	*/
	if(need_mask)
	{
		mask = XCreatePixmap(D, tw->x->draw, s->w, s->h, 1);
		if(!mask) goto nopmem;
		XSetForeground(D, tw->x->dep1gc, 1);
		XFillRectangle(D, mask, tw->x->dep1gc, 0, 0, (UNINT)s->w, (UNINT)s->h);
		XSetForeground(D, tw->x->dep1gc, 0);
		mix = XGetImage(D, mask, 0, 0, s->w, s->h, 1, XYPixmap);
		if(!mix) goto noxmem;
		FREE_PIXMAP(D, mask);
	}

	for(x = 0; x < (int) s->w; x++)
	{
		for(y = 0; y < (int) s->h; y++)
		{
			Pixel p = XGetPixel(ix,x,y);
			XPutPixel(pix, x, y, array[p]);
			if(p == transparent) XPutPixel(mix, x, y, 0);
		}
	}


	switch(do_what_key)
	{
		case glSERVER_SIDE_PREFERENCE:
			px = XCreatePixmap(D, tw->x->draw, s->w, s->h, (UNINT)tw->x->depth);
			if(px) mask = XCreatePixmap(D, px, s->w, s->h, 1);
			if(px != (Pixmap)NULL && mask != (Pixmap)NULL)
			{
				XPutImage(D, px, tw->x->miscgc, pix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
				FREE_XIMAGE(pix);
				XPutImage(D, mask, tw->x->dep1gc, mix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
				FREE_XIMAGE(mix);
				s->data.px = px;
				s->mask.px = mask;
				s->type = SNAP_PIXMAP;
				break;
			}
			FREE_PIXMAP(D, px);
			s->data.ix = pix;
			s->mask.ix = mix;
			s->type = SNAP_IMAGE;
			break;

		case glSERVER_SIDE_ONLY:
			px = XCreatePixmap(D, tw->x->draw, s->w, s->h, (UNINT)tw->x->depth);
			if(!px) goto nopmem;
			XPutImage(D, px, tw->x->miscgc, pix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
			FREE_XIMAGE(pix);
			mask = XCreatePixmap(D, px, s->w, s->h, 1);
			if(!mask) goto nopmem;
			XPutImage(D, mask, tw->x->dep1gc, mix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
			FREE_XIMAGE(mix);
			s->data.px = px;
			s->mask.px = mask;
			s->type = SNAP_PIXMAP;
			break;

		case glCLIENT_SIDE_ONLY:

			s->data.ix = pix;
			s->mask.ix = mix;
			s->type = SNAP_IMAGE;
			break;

		case glCLIENT_SIDE_FILE:
			s->data.ix = pix;
			s->mask.ix = mix;
			s->type = SNAP_FILE;
			s->fp = tmpfile();
			size = (size_t)(s->data.ix->height * s->data.ix->bytes_per_line);
			(void)fwrite(s->data.ix->data, BYTESIZE, size, s->fp);
			XFREE(s->data.ix->data);
			if(s->mask.ix)
			{
				size = (size_t)(s->data.ix->height * s->data.ix->bytes_per_line);
				(void)fwrite(s->mask.ix->data, BYTESIZE, size, s->fp);
				XFREE(s->mask.ix->data);
			}
			break;
	}
	return (Snapshot)(ndx+1);

noxmem:
	pr_error(MyName, "XImage memory allocation failure.\n", NULL);
	FREE_XIMAGE(ix);
	FREE_XIMAGE(pix);
	FREE_XIMAGE(mix);
	FREE_PIXMAP(D, px);
	FREE_PIXMAP(D, mask);
	return (Snapshot)0;

nopmem:
	pr_error(MyName, "Pixmap memory allocation failure.\n", NULL);
	FREE_XIMAGE(ix);
	FREE_XIMAGE(pix);
	FREE_XIMAGE(mix);
	FREE_PIXMAP(D, px);
	FREE_PIXMAP(D, mask);
	return (Snapshot)0;
}


void glPutSnapshot(Snapshot ndx)
{
	size_t   size;
	Pixmap   mask = (Pixmap)NULL;
	SNAPSHOT *s = Xgl.snapshot + ndx - 1;

	if( ndx < 1 || ndx > (Snapshot) Xgl.nsnapshot )
	{
		pr_error("glPutSnapshot", "invalid snapshot id: %d\n", ndx);
		return;
	}

	if( s->dpy != D )
	{
		pr_error("glPutSnapshot", "Snapshot Display and active Window Display are different.\n", NULL);
		return;
	}

	switch(s->type)
	{
		case SNAP_PIXMAP:
			if(s->mask.px)
			{
				XSetClipMask(D, WX->miscgc, s->mask.px);
				XSetClipOrigin(D, WX->miscgc, s->x, s->y);
			}
			XCopyArea(D, s->data.px, WX->draw, WX->miscgc, 0, 0, (UNINT)s->w, (UNINT)s->h, s->x, s->y);
			if(s->mask.px)
			{
				XSetClipMask(D, WX->miscgc, None);
				XSetClipOrigin(D, WX->miscgc, 0, 0);
			}
			break;

		case SNAP_IMAGE:
			if(s->mask.ix)
			{
				mask = XCreatePixmap(D, WX->draw, s->w, s->h, 1);
				if(mask)
				{
					XPutImage(D, mask, WX->dep1gc, s->mask.ix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
					XSetClipMask(D, WX->miscgc, mask);
					XSetClipOrigin(D, WX->miscgc, s->x, s->y);
				}
			}
			XPutImage(D, WX->draw, WX->miscgc, s->data.ix, 0, 0, s->x, s->y, (UNINT)s->w, (UNINT)s->h);
			if(s->mask.ix)
			{
				FREE_PIXMAP(D, mask);
				XSetClipMask(D, WX->miscgc, None);
				XSetClipOrigin(D, WX->miscgc, 0, 0);
			}
			break;

		case SNAP_FILE:
			FREEMEM(s->data.ix->data);
			size = (size_t)(s->data.ix->height * s->data.ix->bytes_per_line);
			s->data.ix->data = MEM(char, size);
			if(!s->data.ix->data) return;
			rewind(s->fp);
			(void)fread(s->data.ix->data, BYTESIZE, size, s->fp);
			if(s->mask.ix)
			{
				FREEMEM(s->mask.ix->data);
				size = (size_t)(s->mask.ix->height * s->mask.ix->bytes_per_line);
				s->mask.ix->data = MEM(char, size);
				if(s->mask.ix->data)
				{
					(void)fread(s->mask.ix->data, BYTESIZE, size, s->fp);
					mask = XCreatePixmap(D, WX->draw, s->w, s->h, 1);
					if(mask)
					{
						XPutImage(D, mask, WX->dep1gc, s->mask.ix, 0, 0, 0, 0, (UNINT)s->w, (UNINT)s->h);
						XSetClipMask(D, WX->miscgc, mask);
						XSetClipOrigin(D, WX->miscgc, s->x, s->y);
					}
					FREEMEM(s->mask.ix->data);
				}
			}
			XPutImage(D, WX->draw, WX->miscgc, s->data.ix, 0, 0, s->x, s->y, (UNINT)s->w, (UNINT)s->h);
			FREEMEM(s->data.ix->data);
			if(s->mask.ix)
			{
				FREE_PIXMAP(D, mask);
				XSetClipMask(D, WX->miscgc, None);
				XSetClipOrigin(D, WX->miscgc, 0, 0);
			}
			break;
	}
}


void glClearSnapshot(Snapshot ndx)
{
	if(ndx < 1 || ndx > (Snapshot) Xgl.nsnapshot ) return;
	ndx--;
	switch(Xgl.snapshot[ndx].type)
	{
		case SNAP_PIXMAP:
			FREE_PIXMAP(Xgl.snapshot[ndx].dpy, Xgl.snapshot[ndx].data.px);
			FREE_PIXMAP(Xgl.snapshot[ndx].dpy, Xgl.snapshot[ndx].mask.px);
			break;

		case SNAP_FILE:
			if(Xgl.snapshot[ndx].fp) (void) fclose(Xgl.snapshot[ndx].fp);
			Xgl.snapshot[ndx].fp = (FILE*)NULL;

		case SNAP_IMAGE:
			FREE_XIMAGE(Xgl.snapshot[ndx].data.ix);
			FREE_XIMAGE(Xgl.snapshot[ndx].mask.ix);
	}
	Xgl.snapshot[ndx].type = SNAP_NONE;
}


void glClearAllSnapshots(void)
{
	int   ndx;

	for(ndx = 0; ndx < Xgl.nsnapshot; ndx++)
	{
		switch(Xgl.snapshot[ndx].type)
		{
			case SNAP_PIXMAP:
				FREE_PIXMAP(Xgl.snapshot[ndx].dpy, Xgl.snapshot[ndx].data.px);
				FREE_PIXMAP(Xgl.snapshot[ndx].dpy, Xgl.snapshot[ndx].mask.px);
				break;

			case SNAP_FILE:
				if(Xgl.snapshot[ndx].fp) (void) fclose(Xgl.snapshot[ndx].fp);

			case SNAP_IMAGE:
				FREE_XIMAGE(Xgl.snapshot[ndx].data.ix);
				FREE_XIMAGE(Xgl.snapshot[ndx].mask.ix);
				break;

		}
	}
	FREEMEM(Xgl.snapshot);
	Xgl.nsnapshot = 0;
}
