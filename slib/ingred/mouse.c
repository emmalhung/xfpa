/***********************************************************************
*                                                                      *
*  m o u s e . c                                                       *
*                                                                      *
*  Interactive Graphics Editor module of FPA.                          *
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

/* >>>>> december addition <<<<< */
#include <X11/keysym.h>
/* >>>>> december addition <<<<< */

#undef DEBUG_INPUT
#undef DEBUG_EVENTS
#undef DEBUG_MOVE
#undef DEBUG_ROTATE
#undef DEBUG_SCULPT
#undef DEBUG_TIME

/* Tags for degree symbol */
#define Ssymbol  "@"
#define Dsymbol  "\260"

static	void	TrackHandler(Widget, XtPointer, XEvent *, LOGICAL *);
static	void	cursor_start(void);
static	void	cursor_next(void);
static	void	cursor_end(void);

/***********************************************************************
*                                                                      *
*     a l l o w _ o b s c u r e d _ i n p u t                          *
*                                                                      *
***********************************************************************/

void	allow_obscured_input

	(
	LOGICAL	state
	)

	{
	if (state && !IgnoreObsc)
		{
		IgnoreObsc = TRUE;
		if (!Visible)
			{
			AllowInput = TRUE;
			obscure_message(FALSE);
			obscure_cursor(FALSE);
			}
		}

	else if (!state && IgnoreObsc)
		{
		IgnoreObsc = FALSE;
		if (!Visible)
			{
			AllowInput = FALSE;
			obscure_message(TRUE);
			obscure_cursor(TRUE);
			}
		}
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ c i r c l e _ e c h o                              *
*     c i r c l e _ e c h o                                            *
*     c a l c _ p u c k                                                *
*                                                                      *
***********************************************************************/

static	SET		CircleEcho = NullSet;

/**********************************************************************/

void	define_circle_echo
	(
	float	radius1,
	float	radius2
	)

	{
	float	ang;
	POINT	pos;
	CURVE	curv;

	static	float	Radius1    = 0;
	static	float	Radius2    = 0;
	static	LINE	UnitCircle = NullLine;
	static	LINE	Circle1    = NullLine;
	static	LINE	Circle2    = NullLine;

	if (radius1 != Radius1 || radius2 != Radius2)
		{
		Radius1 = radius1;
		Radius2 = radius2;

		if (IsNull(CircleEcho))
			{
			CircleEcho = create_set("curve");

			curv = create_curve("", "", "");
			curv->line = create_line();
			define_lspec(&curv->lspec, 255, 0, NULL, False, 4.0, 0.0,
						 (HILITE) 2);
			add_item_to_set(CircleEcho, (ITEM)curv);
			Circle1 = curv->line;

			curv = create_curve("", "", "");
			curv->line = create_line();
			define_lspec(&curv->lspec, 255, 0, NULL, False, 1.0, 0.0,
						 (HILITE) 2);
			add_item_to_set(CircleEcho, (ITEM)curv);
			Circle2 = curv->line;

			UnitCircle = create_line();
			for (ang=0; ang<=360; ang+=10)
				{
				pos[X] = cosdeg(ang);
				pos[Y] = sindeg(ang);
				add_point_to_line(UnitCircle, pos);
				}
			}

		empty_line(Circle1);
		if (Radius1 > 0)
			{
			append_line(Circle1, UnitCircle);
			scale_line(Circle1, Radius1, Radius1);
			}

		empty_line(Circle2);
		if (Radius2 != Radius1 && Radius2 > 0)
			{
			append_line(Circle2, UnitCircle);
			scale_line(Circle2, Radius2, Radius2);
			}
		}
	}

/**********************************************************************/

void	circle_echo
	(
	LOGICAL	state
	)

	{
	state = (LOGICAL) (state && CircleEcho);
	(void) track_Xpointer(DnEdit, CircleEcho, state);
	}

/**********************************************************************/

void	calc_puck
	(
	float	*pr,
	float	*sr
	)

	{
	float   pfact, sfact, prad, srad;

	pfact = 0.01 * ModifyPuck;	sfact = 0.01 * ModifySmth;
	pfact = MAX(pfact, 0.0);	sfact = MAX(sfact, 0.0);
	pfact = MIN(pfact, 1.0);	sfact = MIN(sfact, 1.0);

	prad = SplineRes * (1 + 4*pfact);  /* between 1 and 5 */
	srad = sfact * prad;               /* between 0 and prad */

	if (NotNull(pr)) *pr = prad;
	if (NotNull(sr)) *sr = srad;
	}

/***********************************************************************
*                                                                      *
*     T r a c k H a n d l e r                                          *
*     t r a c k _ X p o i n t e r                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL		Tracking   = FALSE;
static	LOGICAL		TrackShown = FALSE;
static	DISPNODE	TrackDn    = NullDn;
static	SET			TrackSet   = NullSet;
static	XFORM		TrackXform = IDENT_XFORM;

/*ARGSUSED*/
static void	TrackHandler

	(
	Widget		w,
	XtPointer	unused,
	XEvent		*event,
	LOGICAL		*dispatch
	)

	{
	int		tx, ty;
	float	wx, wy;

	if (dispatch) *dispatch = TRUE;

#	ifdef DEBUG_EVENTS
	pr_diag("Editor.Events", "[TrackHandler] Event Type: %d\n", event->type);
#	endif /* DEBUG_EVENTS */

	switch(event->type)
		{
		/* Track cursor */
		case MotionNotify:
			if (!TrackShown) break;

			/* Erase cursor set */
			glLogicOp(glLO_XOR);
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();

			tx = event->xmotion.x;
			ty = event->xmotion.y;

			glScreen2Map(tx, ty, &wx, &wy);
			TrackXform[H][X] = wx;
			TrackXform[H][Y] = wy;

			/* Move and redraw cursor set */
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();
			glLogicOp(glLO_SRC);
			glFlush();
			sync_display();

			break;

		case EnterNotify:
			pr_diag("Editor.Events", "[TrackHandler] Entering\n");

			if (TrackShown) break;

			tx = event->xcrossing.x;
			ty = event->xcrossing.y;

			glScreen2Map(tx, ty, &wx, &wy);
			TrackXform[H][X] = wx;
			TrackXform[H][Y] = wy;

			/* Draw cursor set */
			glLogicOp(glLO_XOR);
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();
			glLogicOp(glLO_SRC);
			glFlush();
			sync_display();

			TrackShown = TRUE;
			break;

		case LeaveNotify:
			pr_diag("Editor.Events", "[TrackHandler] Leaving\n");

			if (!TrackShown) break;

			/* Erase cursor set */
			glLogicOp(glLO_XOR);
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();
			glLogicOp(glLO_SRC);
			glFlush();
			sync_display();

			TrackShown = FALSE;
			break;
		}

	}

LOGICAL		track_Xpointer

	(
	DISPNODE	dn,
	SET			set,
	LOGICAL		track_on
	)

	{
	int		rx, ry;
	int		tx, ty;
	float	wx, wy;
	Window	root, child;
	UNSIGN	mask;

	if (track_on)
		{
		if (Tracking) return TRUE;

		TrackDn  = dn;
		TrackSet = set;

		gxSetupTransform(dn);
		copy_xform(TrackXform, IdentXform);

		XQueryPointer(X_display, X_window, &root, &child, &rx, &ry, &tx, &ty,
						&mask);
#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", " Pointer: %d %d Child: %d\n", tx, ty, child);
#		endif /* DEBUG_EVENTS */
		/*
		if (child == 0)
			{
			TrackShown = FALSE;
			}
		else
		*/
			{
			glScreen2Map(tx, ty, &wx, &wy);
			TrackXform[H][X] = wx;
			TrackXform[H][Y] = wy;

			/* Draw the cursor set for the first time */
			glLogicOp(glLO_XOR);
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();
			glLogicOp(glLO_SRC);
			glFlush();
			sync_display();

			TrackShown = TRUE;
			}

		Tracking = TRUE;
		XtAddEventHandler(X_widget,
						  PointerMotionMask | EnterWindowMask | LeaveWindowMask,
						  FALSE, TrackHandler, NullPointer);
		}

	else
		{
		if (!Tracking) return TRUE;

		XtRemoveEventHandler(X_widget, XtAllEvents, TRUE, TrackHandler,
							NullPointer);
		Tracking = FALSE;

		if (TrackShown)
			{
			/* Erase cursor set */
			glLogicOp(glLO_XOR);
			glConcatMatrix(TrackXform);
			display_set(TrackSet);
			glPopMatrix();
			glLogicOp(glLO_SRC);
			glFlush();
			sync_display();

			TrackShown = FALSE;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c h e c k _ X p o i n t                                          *
*     r e a d y _ X p o i n t                                          *
*     i g n o r e _ X p o i n t                                        *
*     p i c k _ X p o i n t                                            *
*     u t r a c k _ X p o i n t                                        *
*     u t r a c k _ X s p a n                                          *
*     u t r a c k _ X n o d e _ a d d                                  *
*     u t r a c k _ X n o d e _ m o v e                                *
*     u r o t a t e _ X p o i n t                                      *
*                                                                      *
***********************************************************************/

static	void	rb_aspect(POINT, POINT, float);

/**********************************************************************/

LOGICAL		check_Xpoint

	(
	DISPNODE	dn,
	POINT		p,
	int			*butt
	)

	{
	int		button;
	long	mask;
	XEvent	event;

	if (ready_Xpoint(dn, p, butt)) return TRUE;

	mask = ButtonPressMask;
	if (!XCheckWindowEvent(X_display, X_window, mask, &event)) return FALSE;

#	ifdef DEBUG_EVENTS
	pr_diag("Editor.Events", "[check_Xpoint] Event Type: %d\n", event.type);
#	endif /* DEBUG_EVENTS */

	switch(event.type)
		{
		/* Wait for button press */
		case ButtonPress:
			button = event.xbutton.button;
			if (AllowInput && button == Button1)
				{
				ButtonDown = button;
				ButtonX    = event.xbutton.x;
				ButtonY    = event.xbutton.y;
				pr_diag("Editor.Events",
					"[check_Xpoint] Button %d pressed\n", button);
				}
			else
				{
				pr_diag("Editor.Events",
					"[check_Xpoint] Ignoring button %d press\n", button);
				}
			break;
		}

	return ready_Xpoint(dn, p, butt);
	}

/******************************************************************************/

LOGICAL		ready_Xpoint

	(
	DISPNODE	dn,
	POINT		p,
	int			*butt
	)

	{
	int		dx, dy;
	float	wx, wy;

	set_point(p, -1., -1.);
	if (NotNull(butt)) *butt = 0;

	if (ButtonDown <= NoButton)
		{

#		ifdef DEBUG_INPUT
		pr_diag("Editor.Events", "[ready_Xpoint] No Button Pressed.\n");
#		endif /* DEBUG_INPUT */

		return FALSE;
		}

	if (!AllowInput)
		{
		return FALSE;
		}

	gxSetupTransform(dn);
	dx = ButtonX;
	dy = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[ready_Xpoint] Button %d is Pressed (%g,%g).\n", ButtonDown, wx, wy);
#	endif /* DEBUG_INPUT */

	set_point(p, wx, wy);
	if (NotNull(butt)) *butt = ButtonDown;
	return TRUE;
	}

/******************************************************************************/

LOGICAL		ignore_Xpoint(void)

	{
	int		button;
	XEvent	event;
	long	mask;

	/* >>>>> december addition <<<<< */
	KeySym			keysym;
	char			buf[16];
	int				lbuf = 16;
	XComposeStatus	compose;
	/* >>>>> december addition <<<<< */

	if (ButtonDown == NoButton)
		{

#		ifdef DEBUG_INPUT
		pr_diag("Editor.Events", "[ignore_Xpoint] No Button Pressed.\n");
#		endif /* DEBUG_INPUT */

		return FALSE;
		}

	/* >>>>> december addition <<<<< */
	mask = ButtonReleaseMask | KeyPressMask;
	/* >>>>> december addition <<<<< */

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[ignore_Xpoint] Button %d is Pressed.\n", ButtonDown);
#	endif /* DEBUG_INPUT */

	stop_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[ignore_Xpoint] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* >>>>> december addition <<<<< */
			/* Look for Escape key to correct problems */
			case KeyPress:
				(void) XLookupString(&event.xkey, buf, lbuf, &keysym, &compose);
				if (keysym != XK_Escape) break;

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events", "[ignore_Xpoint] Escape Key Pressed!\n");
#				endif /* DEBUG_INPUT */

				/* Reset whatever parameters required to clean up and exit */
				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				stop_cursor(FALSE);

				/* Clear the event queue */
				XSync(X_display, TRUE);
				return FALSE;
			/* >>>>> december addition <<<<< */

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				if (button != ButtonDown) break;

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[ignore_Xpoint] Button %d Released.\n", ButtonDown);
#				endif /* DEBUG_INPUT */

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				stop_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		pick_Xpoint

	(
	DISPNODE	dn,
	int			type,
	POINT		p,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, rw=1;
	XEvent	event;
	long	mask;
	POINT	p0, p1, p2, p3;

	/* >>>>> december addition <<<<< */
	KeySym			keysym;
	char			buf[16];
	int				lbuf = 16;
	XComposeStatus	compose;
	/* >>>>> december addition <<<<< */

	static	CURVE	rband = NullCurve;
	static	MARK	mark  = NullMark;
	static	POINT	mpos = {0,0};

	if (IsNull(rband))
		{
		rband = create_curve("", "", "");
		define_lspec(&rband->lspec, 255, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 0.0, 0.0,
					 (HILITE) 3);
		}

	if (NotNull(butt)) *butt = 0;
	/* >>>>> december addition <<<<< */
	if (IsNull(p))
		{
		pr_diag("Editor.Events",
			"[pick_Xpoint] Missing point - contact System Administrator!\n");
		return FALSE;
		}
	/* >>>>> december addition <<<<< */
	set_point(p, -1., -1.);
	if (ButtonDown == Button1)
		{
		/* >>>>> december addition <<<<< */
		mask = Button1MotionMask | ButtonReleaseMask | KeyPressMask;
		/* >>>>> december addition <<<<< */
		}
	else
		{
		pr_diag("Editor.Events",
			"[pick_Xpoint] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[pick_Xpoint] Button %d is Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	set_point(p0, wx, wy);
	set_point(p1, wx, wy);
	set_point(p2, wx, wy);
	set_point(p3, wx, wy);
	if (NotNull(butt)) *butt = ButtonDown;

	switch (type)
		{
		/* Rubber band line  */
		case 1: /* Put a marker on the original point */
				define_mark_anchor(mark, p0, 0.0);
				mark->mspec.type = 2;
				display_mark(mark);
				/* NOTE: no break */

		/* Rubber band line or box */
		case 2:
		case 3:	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
				/* glSingleBuffer(); */
				glLogicOp(glLO_XOR);

				/* Draw the rubber band line or box for the first time */
				empty_curve(rband);
				add_point_to_curve(rband, p0);
				display_curve(rband);
				glFlush();
				sync_display();
				break;
		}

	/* Calculate window aspect ratio for constrained rubber band box */
	if (type == 3)
		{
		int		nx, ny;

		glGetWindowSize(&nx, &ny);
		rw = (float)ny / (float)nx;
		}

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[pick_Xpoint] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* >>>>> december addition <<<<< */
			/* Look for Escape key to correct problems */
			case KeyPress:
				(void) XLookupString(&event.xkey, buf, lbuf, &keysym, &compose);
				if (keysym != XK_Escape) break;

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events", "[pick_Xpoint] Escape Key Pressed!\n");
#				endif /* DEBUG_INPUT */

				/* Reset whatever parameters required to clean up and exit */
				dx = event.xbutton.x;
				dy = event.xbutton.y;
				glScreen2Map(dx, dy, &wx, &wy);
				set_point(p, wx, wy);
				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);

				/* Clear the event queue */
				XSync(X_display, TRUE);
				return FALSE;
			/* >>>>> december addition <<<<< */

			/* Track cursor while button pressed */
			case MotionNotify:
				/* Erase rubber band line or box */
				switch (type)
					{
					case 1:
					case 2:
					case 3:	display_curve(rband);
							glFlush();
							sync_display();
					}

				dx = event.xmotion.x;
				dy = event.xmotion.y;
				glScreen2Map(dx, dy, &wx, &wy);

				/* Fix aspect ratio for constrained rubber band box */
				if (type == 3)
					{
					set_point(p, wx, wy);
					rb_aspect(p0, p, rw);
					wx = p[X];
					wy = p[Y];
					}

				/* Adjust and redraw the rubber band line or box */
				p1[X] = wx;
				p2[X] = wx;
				p2[Y] = wy;
				p3[Y] = wy;
				switch (type)
					{
					case 1:	empty_curve(rband);
							add_point_to_curve(rband, p0);
							add_point_to_curve(rband, p2);
							display_curve(rband);
							glFlush();
							sync_display();
							break;

					case 2:
					case 3:	empty_curve(rband);
							add_point_to_curve(rband, p0);
							add_point_to_curve(rband, p1);
							add_point_to_curve(rband, p2);
							add_point_to_curve(rband, p3);
							add_point_to_curve(rband, p0);
							display_curve(rband);
							glFlush();
							sync_display();
							break;
					}

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[pick_Xpoint] Button %d Released.\n", button);

				/* Erase rubber band box and turn off abnormal drawing mode */
				switch (type)
					{
					case 1:
					case 2:
					case 3:	display_curve(rband);
							glLogicOp(glLO_SRC);
							glFlush();
							sync_display();
							/* glDoubleBuffer(); */
					}

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);

				/* Fix aspect ratio for constrained rubber band box */
				if (type == 3)
					{
					set_point(p, wx, wy);
					rb_aspect(p0, p, rw);
					wx = p[X];
					wy = p[Y];
					}

				set_point(p, wx, wy);
				switch (type)
					{
					case 1:
					case 2:
					case 3:	sync_display();
					}

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		utrack_Xpoint

	(
	DISPNODE	dn,
	SET			cset,
	POINT		p0,
	POINT		p1,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, wx0, wy0;
	XEvent	event;
	XFORM	xform;
	long	mask1, mask2;

	static	MARK	mark = NullMark;
	static	POINT	mpos = {0,0};

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 2.0, 0.0,
					 (HILITE) 3);
		}

	set_point(p0, -1., -1.);
	set_point(p1, -1., -1.);
	if (NotNull(butt)) *butt = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[utrack_Xpoint] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("utrack_Xpoint", "Button %d Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	set_point(p0, wx, wy);
	if (NotNull(butt)) *butt = ButtonDown;
	if (p0)
		{
		define_mark_anchor(mark, p0, 0.0);
		display_mark(mark);
		glFlush();
		sync_display();
		}

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the cursor set for the first time */
	cursor_start();
	wx0 = wx;	wy0 = wy;
	copy_xform(xform, IdentXform);
	glConcatMatrix(xform);
	display_set(cset);
	glFlush();
	sync_display();

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[utrack_Xpoint] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				/* Erase cursor set */
				display_set(cset);
				glPopMatrix();

				glScreen2Map(dx, dy, &wx, &wy);

				/* Switch drawing modes */
				cursor_next();

				/* Move and redraw cursor set */
				xform[H][X] = wx - wx0;
				xform[H][Y] = wy - wy0;
				glConcatMatrix(xform);
				display_set(cset);
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[utrack_Xpoint] Button %d Released.\n", button);

				/* Erase cursor set and turn off abnormal drawing mode */
				display_set(cset);
				glPopMatrix();
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[utrack_Xpoint] Button %d Released (%g,%g).\n",
						button, wx, wy);
#				endif /* DEBUG_INPUT */

				set_point(p1, wx, wy);
				if (p0)
					{
					sync_display();
					}

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		utrack_Xspan

	(
	DISPNODE	dn,
	POINT		p0,
	POINT		p1,
	float		dist,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, tang, xdist;
	/* >>>>>
	float	wx, wy, lat0, lon0, xang, tang, xdist;
	double	xx, yy;
	<<<<< */
	char	gcbuf[128];
	POINT	pos;
	XEvent	event;
	long	mask1, mask2;

	static	POINT	mpos  = {0,0};
	static	MARK	mark  = NullMark;
	static	CURVE	curv  = NullCurve;
	static	LABEL	label = NullLabel;

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 8.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(curv))
		{
		curv = create_curve("", "", "");
		define_lspec(&curv->lspec, 0, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(label))
		{
		label = create_label("", "", "", mpos, 0.0);
		define_tspec(&label->tspec, 0, SafeFont, False, LabelSize, 0.0,
					 Hc, Vc, (HILITE) 2);
		}

	if (p1) set_point(p1, -1., -1.);
	if (NotNull(butt)) *butt = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[utrack_Xspan] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	/* Get the end point */
	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);
	if (p1) set_point(p1, wx, wy);

#	ifdef DEBUG_INPUT
	pr_diag("utrack_Xspan", "Button %d Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	/* Determine the distance label halfway along the span */
	if (p0 && p1)
		{

		/* Determine bearing in degrees true */
		tang = great_circle_bearing(MapProj, p0, p1);

		/* Determine heading in radians on the current base map */
		/* >>>>>
		xx   = p1[X] - p0[X];
		yy   = p1[Y] - p0[Y];
		xang = (float) atan2deg(yy, xx);
		<<<<< */

		/* Convert heading to degrees true */
		/* >>>>>
		(void) pos_to_ll(MapProj, p0, &lat0, &lon0);
		tang = wind_dir_true(MapProj, lat0, lon0, xang);
		<<<<< */

		/* Reset the end point position if a distance is given */
		if (dist > 0.0)
			{

			/* Determine end point given start point and heading */
			if ( !great_circle_span(MapProj, p0, tang, dist, p1) )
				{
				pr_error("Span", "Error in great_circle_span()\n");
				return FALSE;
				}

			/* Reset the end position */
			wx = p1[X];
			wy = p1[Y];
			}

		/* Determine the great circle distance */
		xdist = great_circle_distance(MapProj, p0, p1);

		/* Set the distance label halfway along the span */
		(void) sprintf(gcbuf, "%d%s%d%s",
						NINT(xdist/1000.0), Ssymbol, NINT(tang), Dsymbol);
		pos[X] = (p0[X] + p1[X]) / 2.0;
		pos[Y] = (p0[Y] + p1[Y]) / 2.0;
		}

	/* Display the end point and the distance label */
	if (NotNull(butt)) *butt = ButtonDown;

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the mark, span and label for the first time */
	cursor_start();
	if (p0 && p1)
		{
		define_mark_anchor(mark, p1, 0.0);
		display_mark(mark);
		empty_curve(curv);
		add_point_to_curve(curv, p0);
		add_point_to_curve(curv, p1);
		display_curve(curv);
		define_label_anchor(label, pos, 0.0);
		define_label_value(label, "", "", gcbuf);
		display_label(label);
		}
	glFlush();
	sync_display();

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[utrack_Xspan] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				/* Erase the mark, span and label */
				if (p0 && p1)
					{
					display_mark(mark);
					display_curve(curv);
					display_label(label);
					}

				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);

				/* Determine the distance label halfway along the span */
				if (p0 && p1)
					{

					/* Determine bearing in degrees true */
					tang = great_circle_bearing(MapProj, p0, p1);

					/* Determine heading in radians on the current base map */
					/* >>>>>
					xx   = p1[X] - p0[X];
					yy   = p1[Y] - p0[Y];
					xang = (float) atan2deg(yy, xx);
					<<<<< */

					/* Convert heading to degrees true */
					/* >>>>>
					(void) pos_to_ll(MapProj, p0, &lat0, &lon0);
					tang = wind_dir_true(MapProj, lat0, lon0, xang);
					<<<<< */

					/* Reset the end point position if a distance is given */
					if (dist > 0.0)
						{

						/* Determine end point given start point and heading */
						if ( !great_circle_span(MapProj, p0, tang, dist, p1) )
							{
							pr_error("Span", "Error in great_circle_span()\n");
							return FALSE;
							}

						/* Reset the end position */
						wx = p1[X];
						wy = p1[Y];
						}

					/* Determine the great circle distance */
					xdist = great_circle_distance(MapProj, p0, p1);

					/* Set the distance label halfway along the span */
					(void) sprintf(gcbuf, "%d%s%d%s",
									NINT(xdist/1000.0), Ssymbol,
									NINT(tang), Dsymbol);
					pos[X] = (p0[X] + p1[X]) / 2.0;
					pos[Y] = (p0[Y] + p1[Y]) / 2.0;
					}

				/* Switch drawing modes */
				cursor_next();

				/* Move and redraw the mark, span and label */
				if (p0 && p1)
					{
					define_mark_anchor(mark, p1, 0.0);
					display_mark(mark);
					empty_curve(curv);
					add_point_to_curve(curv, p0);
					add_point_to_curve(curv, p1);
					display_curve(curv);
					define_label_anchor(label, pos, 0.0);
					define_label_value(label, "", "", gcbuf);
					display_label(label);
					}
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[utrack_Xspan] Button %d Released.\n", button);

				/* Erase the mark, span and label      */
				/*  and turn off abnormal drawing mode */
				if (p0 && p1)
					{
					display_mark(mark);
					display_curve(curv);
					display_label(label);
					}
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[utrack_Xspan] Button %d Released (%g,%g).\n",
						button, wx, wy);
#				endif /* DEBUG_INPUT */

				/* Reset the end point position if a distance is given */
				if (p0 && p1 && dist > 0.0)
					{

					/* Determine bearing in degrees true */
					tang = great_circle_bearing(MapProj, p0, p1);

					/* Determine heading in radians on the current base map */
					/* >>>>>
					xx   = p1[X] - p0[X];
					yy   = p1[Y] - p0[Y];
					xang = (float) atan2deg(yy, xx);
					<<<<< */

					/* Convert heading to degrees true */
					/* >>>>>
					(void) pos_to_ll(MapProj, p0, &lat0, &lon0);
					tang = wind_dir_true(MapProj, lat0, lon0, xang);
					<<<<< */

					/* Determine end point given start point and heading */
					if ( !great_circle_span(MapProj, p0, tang, dist, p1) )
						{
						pr_error("Span", "Error in great_circle_span()\n");
						return FALSE;
						}

					/* Reset the end position */
					wx = p1[X];
					wy = p1[Y];
					}

				if (p1) sync_display();

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		utrack_Xnode_add

	(
	DISPNODE	dn,
	POINT		p0,
	POINT		p1,
	int			ndelta,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, xdelta, tang, xdist, xspd;
	char	gcbuf[128];
	POINT	pos;
	XEvent	event;
	long	mask1, mask2;

	static	POINT	mpos  = {0,0};
	static	MARK	mark  = NullMark;
	static	CURVE	curv  = NullCurve;
	static	LABEL	label = NullLabel;

	/* Set the time delta */
	xdelta = (float) ndelta * 60.0;

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 8.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(curv))
		{
		curv = create_curve("", "", "");
		define_lspec(&curv->lspec, 0, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(label))
		{
		label = create_label("", "", "", mpos, 0.0);
		define_tspec(&label->tspec, 0, SafeFont, False, LabelSize, 0.0,
					 Hc, Vc, (HILITE) 3);
		}

	if (p1) set_point(p1, -1., -1.);
	if (NotNull(butt)) *butt = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[utrack_Xnode_add] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	/* Get the end point */
	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);
	if (p1) set_point(p1, wx, wy);

#	ifdef DEBUG_INPUT
	pr_diag("utrack_Xnode_add", "Button %d Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	/* Determine the speed label halfway along the span */
	if (p0 && p1)
		{

		/* Determine bearing in degrees true */
		if ( ndelta >= 0 )
			tang = great_circle_bearing(MapProj, p0, p1);
		else
			tang = great_circle_bearing(MapProj, p1, p0);

		/* Determine the great circle distance and speed (in m/s) */
		xdist = great_circle_distance(MapProj, p0, p1);
		xspd  = (xdist > 0.0 && ndelta != 0)? xdist / xdelta: 0.0;
		xspd  = (float) fabs((double) xspd);

		/* Set the speed label halfway along the span */
		(void) sprintf(gcbuf, "%d%s%d%s",
						NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
		pos[X] = (p0[X] + p1[X]) / 2.0;
		pos[Y] = (p0[Y] + p1[Y]) / 2.0;
		}

	/* Display the end point and the speed label */
	if (NotNull(butt)) *butt = ButtonDown;

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the mark, span and label for the first time */
	cursor_start();
	if (p0 && p1)
		{
		define_mark_anchor(mark, p1, 0.0);
		display_mark(mark);
		empty_curve(curv);
		add_point_to_curve(curv, p0);
		add_point_to_curve(curv, p1);
		display_curve(curv);
		define_label_anchor(label, pos, 0.0);
		define_label_value(label, "", "", gcbuf);
		display_label(label);
		}
	glFlush();
	sync_display();

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events",
			"[utrack_Xnode_add] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				/* Erase the mark, span and label */
				if (p0 && p1)
					{
					display_mark(mark);
					display_curve(curv);
					display_label(label);
					}

				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);

				/* Determine the speed label halfway along the span */
				if (p0 && p1)
					{

					/* Determine bearing in degrees true */
					if ( ndelta >= 0 )
						tang = great_circle_bearing(MapProj, p0, p1);
					else
						tang = great_circle_bearing(MapProj, p1, p0);

					/* Determine the great circle distance and speed (in m/s) */
					xdist = great_circle_distance(MapProj, p0, p1);
					xspd  = (xdist > 0.0 && ndelta != 0)? xdist / xdelta: 0.0;
					xspd  = (float) fabs((double) xspd);

					/* Set the speed label halfway along the span */
					(void) sprintf(gcbuf, "%d%s%d%s",
									NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
					pos[X] = (p0[X] + p1[X]) / 2.0;
					pos[Y] = (p0[Y] + p1[Y]) / 2.0;
					}

				/* Switch drawing modes */
				cursor_next();

				/* Move and redraw the mark, span and label */
				if (p0 && p1)
					{
					define_mark_anchor(mark, p1, 0.0);
					display_mark(mark);
					empty_curve(curv);
					add_point_to_curve(curv, p0);
					add_point_to_curve(curv, p1);
					display_curve(curv);
					define_label_anchor(label, pos, 0.0);
					define_label_value(label, "", "", gcbuf);
					display_label(label);
					}
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[utrack_Xnode_add] Button %d Released.\n", button);

				/* Erase the mark, span and label      */
				/*  and turn off abnormal drawing mode */
				if (p0 && p1)
					{
					display_mark(mark);
					display_curve(curv);
					display_label(label);
					}
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[utrack_Xnode_add] Button %d Released (%g,%g).\n",
						button, wx, wy);
#				endif /* DEBUG_INPUT */

				if (p0 && p1)
					{
					sync_display();
					}

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		utrack_Xnode_move

	(
	DISPNODE	dn,
	POINT		lpos,
	int			ldelta,
	POINT		npos,
	POINT		fpos,
	int			fdelta,
	POINT		p0,
	POINT		p1,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, wx0, wy0, dwx, dwy, xdell, xdelf, tang, xdist, xspd;
	char	gcbufl[128], gcbuff[128];
	POINT	pos, posl, posf;
	XEvent	event;
	long	mask1, mask2;

	static	POINT	mpos  = {0,0};
	static	MARK	mark  = NullMark;
	static	CURVE	curvl = NullCurve;
	static	LABEL	labl  = NullLabel;
	static	CURVE	curvf = NullCurve;
	static	LABEL	labf  = NullLabel;

	/* Set the time deltas */
	xdell = (float) ldelta * 60.0;
	xdelf = (float) fdelta * 60.0;

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 8.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(curvl))
		{
		curvl = create_curve("", "", "");
		define_lspec(&curvl->lspec, 0, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(labl))
		{
		labl = create_label("", "", "", mpos, 0.0);
		define_tspec(&labl->tspec, 0, SafeFont, False, LabelSize, 0.0,
					 Hc, Vc, (HILITE) 3);
		}

	if (IsNull(curvf))
		{
		curvf = create_curve("", "", "");
		define_lspec(&curvf->lspec, 0, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 2);
		}

	if (IsNull(labf))
		{
		labf = create_label("", "", "", mpos, 0.0);
		define_tspec(&labf->tspec, 0, SafeFont, False, LabelSize, 0.0,
					 Hc, Vc, (HILITE) 3);
		}

	if (p0) set_point(p1, -1., -1.);
	if (p1) set_point(p1, -1., -1.);
	if (NotNull(butt)) *butt = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[utrack_Xnode_move] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	/* Get the start point */
	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);
	if (p0) set_point(p0, wx, wy);
	if (NotNull(butt)) *butt = ButtonDown;
	wx0 = wx;	wy0 = wy;

#	ifdef DEBUG_INPUT
	pr_diag("utrack_Xnode_move", "Button %d Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	/* Set mark at the initial node location */
	if (npos)
		{
		(void) copy_point(pos, npos);

#		ifdef DEBUG_MOVE
		pr_diag("Xnode Move Start", "pos: %.2f/%.2f\n", pos[X], pos[Y]);
#		endif /* DEBUG_MOVE */
		}

	/* Determine the speed label halfway along the previous span */
	if (lpos && npos)
		{

		/* Determine bearing in degrees true */
		if ( ldelta >= 0 )
			tang = great_circle_bearing(MapProj, lpos, pos);
		else
			tang = great_circle_bearing(MapProj, pos, lpos);

		/* Determine the great circle distance and speed (in m/s) */
		xdist = great_circle_distance(MapProj, lpos, pos);
		xspd  = (xdist > 0.0 && ldelta != 0)? xdist / xdell: 0.0;
		xspd  = (float) fabs((double) xspd);

		/* Set the speed label halfway along the span */
		(void) sprintf(gcbufl, "%d%s%d%s",
						NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
		posl[X] = (lpos[X] + pos[X]) / 2.0;
		posl[Y] = (lpos[Y] + pos[Y]) / 2.0;

#		ifdef DEBUG_MOVE
		pr_diag("Xnode Move Start",
			"tang: %.2f  xdist/xspd: %.2f/%.2f  gcbufl: %s  posl: %.2f/%.2f\n",
			tang, xdist, xspd, gcbufl, posl[X], posl[Y]);
#		endif /* DEBUG_MOVE */
		}

	/* Determine the speed label halfway along the next span */
	if (npos && fpos)
		{

		/* Determine bearing in degrees true */
		if ( fdelta >= 0 )
			tang = great_circle_bearing(MapProj, pos, fpos);
		else
			tang = great_circle_bearing(MapProj, fpos, pos);

		/* Determine the great circle distance and speed (in m/s) */
		xdist = great_circle_distance(MapProj, pos, fpos);
		xspd  = (xdist > 0.0 && fdelta != 0)? xdist / xdelf: 0.0;
		xspd  = (float) fabs((double) xspd);

		/* Set the speed label halfway along the span */
		(void) sprintf(gcbuff, "%d%s%d%s",
						NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
		posf[X] = (pos[X] + fpos[X]) / 2.0;
		posf[Y] = (pos[Y] + fpos[Y]) / 2.0;

#		ifdef DEBUG_MOVE
		pr_diag("Xnode Move Start",
			"tang: %.2f  xdist/xspd: %.2f/%.2f  gcbuff: %s  posf: %.2f/%.2f\n",
			tang, xdist, xspd, gcbuff, posf[X], posf[Y]);
#		endif /* DEBUG_MOVE */
		}

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the mark, spans and labels for the first time */
	cursor_start();
	if (npos)
		{
		define_mark_anchor(mark, pos, 0.0);
		display_mark(mark);
		}
	if (lpos && npos)
		{
		empty_curve(curvl);
		add_point_to_curve(curvl, lpos);
		add_point_to_curve(curvl, pos);
		display_curve(curvl);
		define_label_anchor(labl, posl, 0.0);
		define_label_value(labl, "", "", gcbufl);
		display_label(labl);
		}
	if (npos && fpos)
		{
		empty_curve(curvf);
		add_point_to_curve(curvf, pos);
		add_point_to_curve(curvf, fpos);
		display_curve(curvf);
		define_label_anchor(labf, posf, 0.0);
		define_label_value(labf, "", "", gcbuff);
		display_label(labf);
		}
	glFlush();
	sync_display();

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events",
			"[utrack_Xnode_move] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				/* Erase the mark, spans and labels */
				if (npos)
					{
					display_mark(mark);
					}
				if (lpos && npos)
					{
					display_curve(curvl);
					display_label(labl);
					}

				if (npos && fpos)
					{
					display_curve(curvf);
					display_label(labf);
					}

				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);
				dwx = wx - wx0;
				dwy = wy - wy0;

				/* Set translated mark location */
				if (npos)
					{
					pos[X] = npos[X] + dwx;
					pos[Y] = npos[Y] + dwy;

#					ifdef DEBUG_MOVE
					pr_diag("Xnode Move", "pos: %.2f/%.2f\n", pos[X], pos[Y]);
#					endif /* DEBUG_MOVE */
					}

				/* Determine the speed label halfway along the previous span */
				if (lpos && npos)
					{

					/* Determine bearing in degrees true */
					if ( ldelta >= 0 )
						tang = great_circle_bearing(MapProj, lpos, pos);
					else
						tang = great_circle_bearing(MapProj, pos, lpos);

					/* Determine the great circle distance and speed (in m/s) */
					xdist = great_circle_distance(MapProj, lpos, pos);
					xspd  = (xdist > 0.0 && ldelta != 0)? xdist / xdell: 0.0;
					xspd  = (float) fabs((double) xspd);

					/* Set the speed label halfway along the span */
					(void) sprintf(gcbufl, "%d%s%d%s",
									NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
					posl[X] = (lpos[X] + pos[X]) / 2.0;
					posl[Y] = (lpos[Y] + pos[Y]) / 2.0;

#					ifdef DEBUG_MOVE
					pr_diag("Xnode Move",
						"tang: %.2f  xdist/xspd: %.2f/%.2f  gcbufl: %s  posl: %.2f/%.2f\n",
						tang, xdist, xspd, gcbufl, posl[X], posl[Y]);
#					endif /* DEBUG_MOVE */
					}

				/* Determine the speed label halfway along the next span */
				if (npos && fpos)
					{

					/* Determine bearing in degrees true */
					if ( fdelta >= 0 )
						tang = great_circle_bearing(MapProj, pos, fpos);
					else
						tang = great_circle_bearing(MapProj, fpos, pos);

					/* Determine the great circle distance and speed (in m/s) */
					xdist = great_circle_distance(MapProj, pos, fpos);
					xspd  = (xdist > 0.0 && fdelta != 0)? xdist / xdelf: 0.0;
					xspd  = (float) fabs((double) xspd);

					/* Set the speed label halfway along the span */
					(void) sprintf(gcbuff, "%d%s%d%s",
									NINT(xspd), Ssymbol, NINT(tang), Dsymbol);
					posf[X] = (pos[X] + fpos[X]) / 2.0;
					posf[Y] = (pos[Y] + fpos[Y]) / 2.0;

#					ifdef DEBUG_MOVE
					pr_diag("Xnode Move",
						"tang: %.2f  xdist/xspd: %.2f/%.2f  gcbuff: %s  posf: %.2f/%.2f\n",
						tang, xdist, xspd, gcbuff, posf[X], posf[Y]);
#					endif /* DEBUG_MOVE */
					}

				/* Switch drawing modes */
				cursor_next();

				/* Redraw the mark, spans and labels */
				if (npos)
					{
					define_mark_anchor(mark, pos, 0.0);
					display_mark(mark);
					}
				if (lpos && npos)
					{
					empty_curve(curvl);
					add_point_to_curve(curvl, lpos);
					add_point_to_curve(curvl, pos);
					display_curve(curvl);
					define_label_anchor(labl, posl, 0.0);
					define_label_value(labl, "", "", gcbufl);
					display_label(labl);
					}
				if (npos && fpos)
					{
					empty_curve(curvf);
					add_point_to_curve(curvf, pos);
					add_point_to_curve(curvf, fpos);
					display_curve(curvf);
					define_label_anchor(labf, posf, 0.0);
					define_label_value(labf, "", "", gcbuff);
					display_label(labf);
					}
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[utrack_Xnode_move] Button %d Released.\n", button);

				/* Erase the mark, spans and labels    */
				/*  and turn off abnormal drawing mode */
				if (npos)
					{
					display_mark(mark);
					}
				if (lpos && npos)
					{
					display_curve(curvl);
					display_label(labl);
					}

				if (npos && fpos)
					{
					display_curve(curvf);
					display_label(labf);
					}
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);
				if (p1) set_point(p1, wx, wy);

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[utrack_Xnode_move] Button %d Released (%g,%g).\n",
						button, wx, wy);
#				endif /* DEBUG_INPUT */

				if (p0 && p1)
					{
					sync_display();
					}

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/******************************************************************************/

LOGICAL		urotate_Xpoint

	(
	DISPNODE	dn,
	SET			cset,
	POINT		pc,
	POINT		p0,
	POINT		p1,
	float		*angle,
	int			*butt
	)

	{
	int		button, dx, dy;
	float	wx, wy, wx0, wy0;
	float	sx0, sy0, sx, sy, ds0, fact, ang0, ang, tx, ty, sa, ca;
	float	angl, angn, dang;
	XEvent	event;
	XFORM	xform;
	long	mask1, mask2;

	static	MARK	mark = NullMark;
	static	POINT	mpos = {0,0};
	static	CURVE	curv = NullCurve;

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
					 (HILITE) 3);
		}

	if (IsNull(curv))
		{
		curv = create_curve("", "", "");
		define_lspec(&curv->lspec, 0, 0, NULL, False, 2.0, 0.0,
					 (HILITE) 3);
		}

	set_point(p0, -1., -1.);
	set_point(p1, -1., -1.);
	if (NotNull(angle)) *angle = 0.0;
	if (NotNull(butt))  *butt  = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[urotate_Xpoint] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[urotate_Xpoint] Button %d Pressed (%g,%g).\n", button, wx, wy);
#	endif /* DEBUG_INPUT */

	set_point(p0, wx, wy);
	if (NotNull(butt)) *butt = ButtonDown;
	if (p0)
		{
		define_mark_anchor(mark, p0, 0.0);
		display_mark(mark);
		glFlush();
		sync_display();
		}

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the cursor set for the first time */
	cursor_start();
	tx   = pc[X];
	ty   = pc[Y];
	wx0  = wx;
	wy0  = wy;
	sx0  = wx0 - tx;
	sy0  = wy0 - ty;
	ds0  = hypot(sx0, sy0);
	ang0 = atan2(sy0, sx0);
	angl = ang0;
	copy_xform(xform, IdentXform);
	glConcatMatrix(xform);
	if (p0)
		{
		empty_curve(curv);
		add_point_to_curve(curv, pc);
		add_point_to_curve(curv, p0);
		display_curve(curv);
		}
	display_set(cset);
	glFlush();
	sync_display();

	picking_cursor(TRUE);
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[urotate_Xpoint] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{

			/* Track cursor while button pressed */
			case MotionNotify:

				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				/* Erase cursor set */
				if (p0) display_curve(curv);
				display_set(cset);
				glPopMatrix();

				glScreen2Map(dx, dy, &wx, &wy);
				if (fcompare(wx, tx, 0.0, 1e-5) && fcompare(wy, ty, 0.0, 1e-5))
					{
					wx = wx0;
					wy = wy0;
					}

				/* Determine angle of rotation */
				sx   = wx - tx;
				sy   = wy - ty;
				angn = atan2(sy, sx);

				/* Ensure angle of rotation does not jump quadrants */
				dang = fabs((double) (angn - angl));
				if (dang > PI && angl < 0.0)      angn -= 2*PI;
				else if (dang > PI && angl > 0.0) angn += 2*PI;

				/* Set rotation from starting position */
				ang  = angn - ang0;
				angl = angn;
				ca   = cos((double) ang);
				sa   = sin((double) ang);

#				ifdef DEBUG_ROTATE
				pr_diag("Rotate", "ang0: %.3f  angn: %.3f  ang: %.3f\n",
					ang0, angn, ang);
#				endif /* DEBUG_ROTATE */

				/* Switch drawing modes */
				cursor_next();

				/* Rotate and redraw cursor set */
				xform[X][X] = ca;
				xform[X][Y] = sa;
				xform[Y][X] = -sa;
				xform[Y][Y] = ca;
				xform[H][X] = -ca*tx + sa*ty + tx;
				xform[H][Y] = -sa*tx - ca*ty + ty;
				glConcatMatrix(xform);
				if (p0) display_curve(curv);
				display_set(cset);
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[urotate_Xpoint] Button %d Released.\n", button);

				/* Erase cursor set and turn off abnormal drawing mode */
				if (p0) display_curve(curv);
				display_set(cset);
				glPopMatrix();
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);
				if (fcompare(wx, tx, 0.0, 1e-5) && fcompare(wy, ty, 0.0, 1e-5))
					{
					wx = wx0;
					wy = wy0;
					}

				/* Determine final angle of rotation */
				sx   = wx - tx;
				sy   = wy - ty;
				angn = atan2(sy, sx);

				/* Ensure angle of rotation does not jump quadrants */
				dang = fabs((double) (angn - angl));
				if (dang > PI && angl < 0.0)      angn -= 2*PI;
				else if (dang > PI && angl > 0.0) angn += 2*PI;

				/* Set rotation from starting position */
				ang  = angn - ang0;
				ang  = fmod((double) ang, 2*PI);

#				ifdef DEBUG_ROTATE
				pr_diag("Rotate", "Final ang0: %.3f  angn: %.3f  ang: %.3f\n",
					ang0, angn, ang);
#				endif /* DEBUG_ROTATE */

				/* Determine location of rotated reference point */
				if (sx==0.0 && sy==0.0) fact = 0.0;
				else                    fact = ds0 / hypot(sx, sy);
				sx   = tx + sx*fact;
				sy   = ty + sy*fact;

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[urotate_Xpoint] Button %d Released (%g,%g).\n",
						button, wx, wy);
#				endif /* DEBUG_INPUT */

				set_point(p1, sx, sy);
				if (angle) *angle = ang;
				if (p0)
					{
					sync_display();
					}

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				picking_cursor(FALSE);
				return TRUE;
			}
		}
	}

/**********************************************************************/

static	void	rb_aspect
	(
	POINT	pa,
	POINT	pb,
	float	rw
	)

	{
	float	r, dx, dy;

	pb[X] = MAX(pb[X], 0);
	pb[X] = MIN(pb[X], MapProj->definition.xlen);

	pb[Y] = MAX(pb[Y], 0);
	pb[Y] = MIN(pb[Y], MapProj->definition.ylen);

	dx = pb[X] - pa[X];
	dy = pb[Y] - pa[Y];
	r  = fabs((double)(dy / dx));
	if (r > rw)
		{
		/* Too tall */
		pb[Y] = pa[Y] + dx*(SIGN(dy)/SIGN(dx))*rw;
		}
	else if (r < rw)
		{
		/* Too wide */
		pb[X] = pa[X] + dy*(SIGN(dx)/SIGN(dy))/rw;
		}
	}

/***********************************************************************
*                                                                      *
*     u e d i t _ X c u r v e                                          *
*                                                                      *
***********************************************************************/

static	LOGICAL	bump_start(LINE, LOGICAL, LOGICAL);
static	LOGICAL	bump_set(float, float);
static	LOGICAL	bump_range(float, float);
static	LOGICAL	bump_line(LINE, SUBAREA, POINT, float, float, POINT, int,
						LOGICAL, LOGICAL, LOGICAL *);
static	LOGICAL	exit_circle(LINE, POINT, float, float, float, LOGICAL,
						LOGICAL, LOGICAL, LOGICAL, POINT, float *, float *);
static	LOGICAL	push_circle(LINE, POINT, float, int, LOGICAL,
						POINT, float *, POINT, float *);

/**********************************************************************/

static	AREA	Tool  = NullArea;
static	LINE	Tbndy = NullLine;
static	AREA	Ring  = NullArea;
static	LINE	Rbndy = NullLine;
static	float	Dang  = 0;

LOGICAL		uedit_Xcurve

	(
	DISPNODE	dn,			/* working window */
	LINE		line,		/* line to modify */
	SUBAREA		sub,		/* subarea containing modified line */
	float		rd,			/* radius of puck */
	float		spread,		/* smoothing radius around puck */
	int			*butt,		/* which button was pressed */
	LOGICAL		*mright		/* which side of line was modified */
	)

	{
	int		button, dx, dy, iseg;
	float	sd, wx, wy, dist, ang;
	XEvent	event;
	POINT	p, pos, ppos;
	long	mask1, mask2;
	LOGICAL	right, closed, lright;
	XFORM	xform;
	LOGICAL	modified = FALSE;

	static	CURVE	curv = NullCurve;

	if (IsNull(curv))
		{
		curv = create_curve("", "", "");
		define_lspec(&curv->lspec, 255, 0, NULL, False, 5.0, 0.0,
					 (HILITE) 1);
		}

	if (IsNull(Tool))
		{
		Tool = create_area("", "", "");
		define_lspec(&Tool->lspec, 255, 0, NULL, False, 0.0, 0.0,
					 (HILITE) -1);
		define_fspec(&Tool->fspec, 255, 1, NULL, False, False, 0.0, 0.0,
					 (HILITE) 2);

		Ring = create_area("", "", "");
		define_lspec(&Ring->lspec, 255, 2, NULL, False, 0.0, 0.0,
					 (HILITE) 2);
		}

	/* Create the circular tool at the given size */
	empty_area(Tool);
	empty_area(Ring);
	Dang = 360 * SplineRes / (2*PI*rd);
	Dang = MIN(Dang, 45);
	Tbndy = create_line();
	for (ang=0; ang<360; ang+=Dang)
		{
		p[X] = fpa_cosdeg(ang);
		p[Y] = fpa_sindeg(ang);
		add_point_to_line(Tbndy, p);
		}
	close_line(Tbndy);
	Rbndy = copy_line(Tbndy);
	sd = rd + spread;
	scale_line(Tbndy, rd, rd);
	scale_line(Rbndy, sd, sd);
	define_area_boundary(Tool, Tbndy);
	define_area_boundary(Ring, Rbndy);
	copy_xform(xform, IdentXform);

	if (NotNull(butt))   *butt   = 0;
	if (NotNull(mright)) *mright = TRUE;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[uedit_Xcurve] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[uedit_Xcurve] Button %d Pressed (%g,%g).\n",
		button, wx, wy);
#	endif /* DEBUG_INPUT */
	if (NotNull(butt)) *butt = ButtonDown;

	/* Find closest point */
	set_point(pos, wx, wy);
	line_test_point(line, pos, &dist, ppos, &iseg, NULL, &right);
	if (NotNull(mright)) *mright = right;
	closed = line_closed(line);
	if (right) pr_diag("Sculpt", "Start orientation ... Right\n");
	else       pr_diag("Sculpt", "Start orientation ... Left\n");

	/* Puck is already in contact with line! */
	if (dist < rd)
		{
		/*
		dwx = pos[X] - ppos[X];
		dwy = pos[Y] - ppos[Y];
		*/

		/* >>> modify <<< */
		/* Modify the line */
#		ifdef DEBUG_SCULPT
		if (right)
			pr_diag("Sculpt",
				"Start bump_line() ... Right ... %.4f from: %.4f %.4f\n",
				dist, pos[X], pos[Y]);
		else
			pr_diag("Sculpt",
				"Start bump_line() ... Left ... %.4f from: %.4f %.4f\n",
				dist, pos[X], pos[Y]);
#		endif /* DEBUG_SCULPT */
		bump_start(line, right, closed);
		if (!bump_line(line, sub, pos, rd, spread, ppos, iseg, right, closed,
				&lright))
			{

			pr_diag("Sculpt", "Error return from bump_line() at start!\n");
			put_message("sculpt-error");
			(void) sleep(1);

			/* Empty the area boundary or line */
			empty_line(line);
			ButtonDown = NoButton;
			ButtonX    = 0;
			ButtonY    = 0;
			if (NotNull(mright)) *mright = lright;
			return FALSE;
			}
		modified = TRUE;

		/* Check for area/line removal */
		if (line_too_short(line, rd))
			{

#			ifdef DEBUG_INPUT
			if (closed)
				pr_diag("Editor.Events",
					"[uedit_Xcurve] Deleting area at start!\n");
			else
				pr_diag("Editor.Events",
					"[uedit_Xcurve] Deleting line at start!\n");
#			endif /* DEBUG_INPUT */

			/* Empty the area boundary or line */
			empty_line(line);
			ButtonDown = NoButton;
			ButtonX    = 0;
			ButtonY    = 0;
			if (NotNull(mright)) *mright = lright;
			return FALSE;
			}
		}

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the curve for the first time */
	cursor_start();
	xform[H][X] = wx;
	xform[H][Y] = wy;
	glConcatMatrix(xform);
	display_area(Tool);
	if (spread > 0) display_area(Ring);
	glPopMatrix();
	curv->line = line;
	display_curve(curv);
	glFlush();
	sync_display();

	/* picking_cursor(TRUE); */
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[uedit_Xcurve] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				glScreen2Map(dx, dy, &wx, &wy);

				/* Erase curve and tool */
				display_curve(curv);
				glConcatMatrix(xform);
				display_area(Tool);
				if (spread > 0) display_area(Ring);
				glPopMatrix();

				/* Switch drawing modes */
				cursor_next();

				/* Find closest point */
				set_point(pos, wx, wy);
				line_test_point(line, pos, &dist, ppos, &iseg, NULL, &right);
				if (NotNull(mright)) *mright = right;

				/* Puck is in contact with line! */
				if (dist < rd)
					{
					/*
					dwx = pos[X] - ppos[X];
					dwy = pos[Y] - ppos[Y];
					*/
					/* >>> modify <<< */

#					ifdef DEBUG_SCULPT
					if (right)
						pr_diag("Sculpt",
							"Track bump_line() ... Right ... %.4f from: %.4f %.4f\n",
							dist, pos[X], pos[Y]);
					else
						pr_diag("Sculpt",
							"Track bump_line() ... Left ... %.4f from: %.4f %.4f\n",
							dist, pos[X], pos[Y]);
#					endif /* DEBUG_SCULPT */

					/* Modify the line */
					if (!modified) bump_start(line, right, closed);
					if (!bump_line(line, sub, pos, rd, spread,
							ppos, iseg, right, closed, &lright))
						{

						/* Erase curve and turn off abnormal drawing mode */
						display_curve(curv);
						glConcatMatrix(xform);
						display_area(Tool);
						if (spread > 0) display_area(Ring);
						glPopMatrix();
						cursor_end();
						glFlush();
						sync_display();

						pr_diag("Sculpt",
							"Error return from bump_line() at track!\n");
						put_message("sculpt-error");
						(void) sleep(1);

						/* Empty the area boundary or line */
						empty_line(line);
						ButtonDown = NoButton;
						ButtonX    = 0;
						ButtonY    = 0;
						/* picking_cursor(FALSE); */
						curv->line = NullLine;
						if (NotNull(mright)) *mright = lright;
						return FALSE;
						}
					modified = TRUE;

					/* Check for area/line removal */
					if (line_too_short(line, rd))
						{

						/* Erase curve and turn off abnormal drawing mode */
						display_curve(curv);
						glConcatMatrix(xform);
						display_area(Tool);
						if (spread > 0) display_area(Ring);
						glPopMatrix();
						cursor_end();
						glFlush();
						sync_display();

	#					ifdef DEBUG_INPUT
						if (closed)
							pr_diag("Editor.Events",
								"[uedit_Xcurve] Deleting area at track!\n");
						else
							pr_diag("Editor.Events",
								"[uedit_Xcurve] Deleting line at track!\n");
	#					endif /* DEBUG_INPUT */

						/* Empty the area boundary or line */
						empty_line(line);
						ButtonDown = NoButton;
						ButtonX    = 0;
						ButtonY    = 0;
						/* picking_cursor(FALSE); */
						curv->line = NullLine;
						if (NotNull(mright)) *mright = lright;
						return FALSE;
						}
					}

				/* Redraw the curve and tool */
				xform[H][X] = wx;
				xform[H][Y] = wy;
				glConcatMatrix(xform);
				display_area(Tool);
				if (spread > 0) display_area(Ring);
				glPopMatrix();
				display_curve(curv);
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[uedit_Xcurve] Button %d Released.\n", button);

				/* Erase curve and turn off abnormal drawing mode */
				display_curve(curv);
				glConcatMatrix(xform);
				display_area(Tool);
				if (spread > 0) display_area(Ring);
				glPopMatrix();
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);

				/* Find closest point */
				set_point(pos, wx, wy);
				line_test_point(line, pos, &dist, ppos, &iseg, NULL, &right);
				if (NotNull(mright)) *mright = right;

				/* Puck is in contact with line! */
				if (dist < rd)
					{
					/*
					dwx = pos[X] - ppos[X];
					dwy = pos[Y] - ppos[Y];
					*/
					/* >>> modify <<< */

#					ifdef DEBUG_SCULPT
					if (right)
						pr_diag("Sculpt",
							"Release bump_line() ... Right ... %.4f from: %.4f %.4f\n",
							dist, pos[X], pos[Y]);
					else
						pr_diag("Sculpt",
							"Release bump_line() ... Left ... %.4f from: %.4f %.4f\n",
							dist, pos[X], pos[Y]);
#					endif /* DEBUG_SCULPT */

					/* Modify the line */
					if (!modified) bump_start(line, right, closed);
					if (!bump_line(line, sub, pos, rd, spread,
							ppos, iseg, right, closed, &lright))
						{
						pr_diag("Sculpt",
							"Error return from bump_line() at release!\n");
						put_message("sculpt-error");
						(void) sleep(1);

						/* Empty the area boundary or line */
						empty_line(line);
						ButtonDown = NoButton;
						ButtonX    = 0;
						ButtonY    = 0;
						/* picking_cursor(FALSE); */
						curv->line = NullLine;
						if (NotNull(mright)) *mright = lright;
						return FALSE;
						}
					modified = TRUE;

					/* Check for area/line removal */
					if (line_too_short(line, rd))
						{

	#					ifdef DEBUG_INPUT
						if (closed)
							pr_diag("Editor.Events",
								"[uedit_Xcurve] Deleting area at release!\n");
						else
							pr_diag("Editor.Events",
								"[uedit_Xcurve] Deleting line at release!\n");
	#					endif /* DEBUG_INPUT */

						/* Empty the area boundary or line */
						empty_line(line);
						ButtonDown = NoButton;
						ButtonX    = 0;
						ButtonY    = 0;
						/* picking_cursor(FALSE); */
						curv->line = NullLine;
						if (NotNull(mright)) *mright = lright;
						return FALSE;
						}
					}

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[uedit_Xcurve] Button %d Released (%g,%g).\n",
					button, wx, wy);
#				endif /* DEBUG_INPUT */

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				/* picking_cursor(FALSE); */
				curv->line = NullLine;
				if (NotNull(mright)) *mright = lright;
				return modified;
			}
		}
	}

/**********************************************************************/

static	LINE	NewLn      = NullLine;

static	LINE	PrevLine   = NullLine;
static	LOGICAL	PrevRight  = FALSE;
static	LOGICAL	PrevClosed = FALSE;
static	float	PrevBeg    = 0.0;
static	float	PrevEnd    = 0.0;
static	float	PrevInc    = 0.5;
static	LOGICAL	PrevSet    = FALSE;

/**********************************************************************/

static	LOGICAL	bump_start

				(
				LINE	line,
				LOGICAL	right,
				LOGICAL	closed
				)

	{
	PrevLine   = line;
	PrevRight  = right;
	PrevClosed = closed;
	PrevBeg    = 0.0;
	PrevEnd    = 0.0;
	PrevSet    = FALSE;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	bump_set

				(
				float	idxs,
				float	idxe
				)

	{
	if (IsNull(PrevLine)) return FALSE;

	PrevBeg = idxs;
	PrevEnd = idxe;
	PrevSet = TRUE;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	bump_range

				(
				float	newbeg,
				float	newend
				)

	{
	if (IsNull(PrevLine)) return FALSE;
	if (!PrevSet)         return FALSE;

	/* Fine if new begin or end point is inside prev range */
	if (PrevBeg<=PrevEnd)
		{
		/* Prev end follows begin */
		if (PrevBeg-PrevInc<=newbeg && newbeg<=PrevEnd+PrevInc) return TRUE;
		if (PrevBeg-PrevInc<=newend && newend<=PrevEnd+PrevInc) return TRUE;
		}
	else
		{
		/* Prev begin follows end */
		/* Makes no sense for open lines */
		if (!PrevClosed) return FALSE;

		/* For closed lines, implies wrap around endpoint */
		if (PrevBeg-PrevInc<=newbeg || newbeg<=PrevEnd+PrevInc) return TRUE;
		if (PrevBeg-PrevInc<=newend || newend<=PrevEnd+PrevInc) return TRUE;
		}

	/* Also fine if prev begin or end point is inside new range */
	if (newbeg<=newend)
		{
		/* New end follows begin */
		if (newbeg<=PrevBeg && PrevBeg<=newend) return TRUE;
		if (newbeg<=PrevEnd && PrevEnd<=newend) return TRUE;
		}
	else
		{
		/* New begin follows end */
		/* Makes no sense for open lines */
		if (!PrevClosed) return FALSE;

		/* For closed lines, implies wrap around endpoint */
		if (newbeg<=PrevBeg || PrevBeg<=newend) return TRUE;
		if (newbeg<=PrevEnd || PrevEnd<=newend) return TRUE;
		}

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"---------- ERROR  PrevBeg/End: %.2f %.2f  newbeg/newend: %.2f %.2f\n",
		PrevBeg, PrevEnd, newbeg, newend);
#	endif /* DEBUG_SCULPT */

	return FALSE;
	}

/**********************************************************************/

static	LOGICAL	bump_line

				(
				LINE	line,
				SUBAREA	sub,
				POINT	centre,
				float	rd,
				float	spread,
				POINT	ppos,
				int		iseg,
				LOGICAL	right,
				LOGICAL	closed,
				LOGICAL	*lright
				)

	{
	int		nlp, npc, lpc, ip, ipl, jp, jpl, nl;
	LINE	*lines, cdiv, divln, tline;
	float	dx, dy, sd, sdx, ds, de, dp, idxs, idxe, ra, rb, anga, angb, angt;
	float	idx, idxa, idxb, dist, idxx, jdx, jdxx;
	POINT	pos, posa, posb;
	LOGICAL	atstart, bothin, nearbeg, smoothed, rt, pushok;
	LOGICAL	outbeg, outend, closebeg, closeend;
	int		ips, ipe, ipx, jpx;
	POINT	spos, epos;
	float	spfact;
	DIVSTAT	dstat;
	STRING	msgkey;

	/* Variables relating to the inner circle */
	int		ipcrbeg, ipcrend;
	float	idxrbeg, idxrend, angrbeg=0, angrend=0, angrmid=0;
	POINT	prbeg, prend;
	LOGICAL	frbeg, frend;

	/* Variables relating to the outer (smoothing) circle */
	float	idxsbeg, idxsend, angsbeg=0, angsend=0;
	POINT	psbeg, psend;
	LOGICAL	fsbeg, fsend;

#	ifdef DEBUG_SCULPT
	int		ii;
#	endif /* DEBUG_SCULPT */

	if (NotNull(lright)) *lright = right;

	if (IsNull(line) || line->numpts < 2)
		{
		(void) pr_error("Sculpt", "No line to sculpt!\n");
		return FALSE;
		}

	if (iseg < 0)
		{
		(void) pr_error("Sculpt", "No line segment to sculpt!\n");
		return FALSE;
		}

	if (closed && NotNull(sub))
		{
		pr_warning("Sculpt", "Cannot clip areas to a subarea boundary!\n");
		}

#	ifdef DEBUG_TIME
	if (NotNull(sub))
		(void) printf("[bump_line] Begin divide at: %d  Points: %d\n",
			(long) clock(), line->numpts);
	else
		(void) printf("[bump_line] Begin at: %d  Points: %d\n",
			(long) clock(), line->numpts);
#	endif /* DEBUG_TIME */

	/* Set initial parameters */
	sd  = rd + spread;
	sdx = 0.75 * sd;
	nlp = line->numpts - 1;
	npc = Tbndy->numpts;
	lpc = npc - 1;
	atstart  = (LOGICAL) (iseg < nlp/2);
	smoothed = (LOGICAL) (spread > 0);

	/* Set start/end line locations to check crossover orientation */
	dx = centre[X] - line->points[0][X];
	dy = centre[Y] - line->points[0][Y];
	ds = hypot(dx, dy);
	dx = centre[X] - line->points[nlp][X];
	dy = centre[Y] - line->points[nlp][Y];
	de = hypot(dx, dy);
	bothin  = (ds < rd && de < rd)? TRUE: FALSE;
	nearbeg = (ds < de)? TRUE: FALSE;

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt", "----------\n");
#	endif /* DEBUG_SCULPT */

	/* Figure out index of ppos on line */
	dx   = ppos[X] - line->points[iseg][X];
	dy   = ppos[Y] - line->points[iseg][Y];
	dp   = hypot(dx, dy);
	idxs = line_index(line, iseg, dp);

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"---------- iseg/nlp: %d %d  dx/dy/dp: %.2f %.2f %.2f  idxs: %.2f\n",
		iseg, nlp, dx, dy, dp, idxs);
#	endif /* DEBUG_SCULPT */

	/* If the cursor is moving quickly, it is possible to jump over to the */
	/*  other side of the line.  We want to interpret this as a big push   */
	/*  from the original side, rather than a push back from the new side  */
	if (PrevSet)
		{

		/* Ensure that we are still checking wrt original span of line */
		/*  ... to avoid problems with sharply turning lines           */
		tline = create_line();
		ipx = floor(idxs);
		if (ipx == nlp) ipx--;
		add_point_to_line(tline, line->points[ipx]);
		add_point_to_line(tline, line->points[ipx+1]);
		line_test_point(tline, centre, NullFloat, NullPoint, NullInt,
						NullLogical, &rt);
		(void) destroy_line(tline);

		/* Ignore motion if on opposite side of original span */
		if ((right && !rt) || (!right && rt))
			{
#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Return from bump_line() ... switch sides in testing line\n");
#			endif /* DEBUG_SCULPT */

			if (NotNull(lright)) *lright = rt;
			return TRUE;
			}

		/* Switch to the proper side */
		if (PrevRight && !right)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "Switch R->L - Correcting\n");
#			endif /* DEBUG_SCULPT */

			right    = PrevRight;
			}
		else if (!PrevRight && right)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "Switch L->R - Correcting\n");
#			endif /* DEBUG_SCULPT */

			right    = PrevRight;
			}

		/* Ignore motion unless in same range as previous */
		if (!bump_range(idxs, idxs))
			{
#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "Return from bump_line() ... outside range\n");
#			endif /* DEBUG_SCULPT */

			/* >>>>> change return from false to true - Aug 2004 <<<<< */
			if (NotNull(lright)) *lright = right;
			return TRUE;
			}
		}
	bump_set(idxs, idxs);

	/* Find first point of line inside the inner circle */
	idxe  = (closed)? idxs+.01: 0;
	frbeg = exit_circle(line, centre, rd, idxs, idxe, closed, right, FALSE,
				TRUE, prbeg, &idxrbeg, &angrbeg);
	if (frbeg) bump_set(idxrbeg, idxs);

	/* If no exit, start of line is contained within the circle */
	if (!frbeg)
		{
		/* If editing a closed line then it is totally inside the circle */
		if (closed)
			{
			LOGICAL	cw;

			/* If editing a closed line from inside, expand to the circle */
			line_properties(line, NullChar, &cw, NullFloat, NullFloat);
			if ((right && cw) || (!right && !cw))
				{
				empty_line(line);
				append_line_dir(line, Tbndy, Not(cw));
				translate_line(line, centre[X], centre[Y]);
				PrevEnd = line->numpts - 1;

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"Return from bump_line() ... inside closed line\n");
#				endif /* DEBUG_SCULPT */

				if (NotNull(lright)) *lright = right;
				return TRUE;
				}

			/* Otherwise eat the line */
			empty_line(line);
			PrevLine = NullLine;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Return from bump_line() ... outside closed line ... removing\n");
#			endif /* DEBUG_SCULPT */

			/* >>>>> change return from false to true - Dec 2004 <<<<< */
			if (NotNull(lright)) *lright = right;
			return TRUE;
			}

		/* Otherwise, for an open line, we need to push the start of the */
		/* line to the edge of the circle (given by prbeg) */
		}

#	ifdef DEBUG_SCULPT
	if (!frbeg)
		{
		pr_diag("Sculpt",
			"In bump_line() ... no start for line!\n");
		pr_diag("Sculpt",
			"     idxs/idxe/idxrbeg/angrbeg: %.4f %.4f %.4f %.4f\n",
			idxs, idxe, idxrbeg, angrbeg);
		}
#	endif /* DEBUG_SCULPT */

	/* Find last point of line inside the inner circle */
	idxe  = (closed)? idxs-.01: nlp;
	frend = exit_circle(line, centre, rd, idxs, idxe, closed, right, TRUE,
				TRUE, prend, &idxrend, &angrend);
	if (frend) bump_set(idxrbeg, idxrend);

#	ifdef DEBUG_SCULPT
	if (!frend)
		{
		pr_diag("Sculpt",
			"In bump_line() ... no exit for line!\n");
		pr_diag("Sculpt",
			"     idxs/idxe/idxrend/angrend: %.4f %.4f %.4f %.4f\n",
			idxs, idxe, idxrend, angrend);
		}
#	endif /* DEBUG_SCULPT */

	/* If no start and no exit, line is totally contained within the circle */
	/*  ... so either eat the line or push the line (depending on mode)     */
	if (!frbeg && !frend)
		{

		switch (SculptMode)
			{

			case SCULPT_REMOVE:

				/* Eat the line */
				empty_line(line);
				PrevLine = NullLine;

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"Return from bump_line() ... fully inside ... removing\n");
#				endif /* DEBUG_SCULPT */

				if (NotNull(lright)) *lright = right;
				return TRUE;

			/* Push the line */
			case SCULPT_PUSH:

				/* Push the line to the edge of the circle */
				pushok = push_circle(line, centre, rd, iseg, right,
										prbeg, &angrbeg, prend, &angrend);

				/* Problem with dividing line ... so eat the line */
				if (NotNull(sub) && !pushok)
					{
					empty_line(line);
					PrevLine = NullLine;

#					ifdef DEBUG_SCULPT
					pr_diag("Sculpt",
						"Return from bump_line() ... fully inside ... removing dividing\n");
#					endif /* DEBUG_SCULPT */

#					ifdef DEBUG_TIME
					(void) printf("[bump_line] End divide at: %d\n",
						(long) clock());
#					endif /* DEBUG_TIME */

					if (NotNull(lright)) *lright = right;
					return TRUE;
					}
				break;
			}

		/* Turn off smoothing */
		smoothed = FALSE;
		}

	/* If no exit, end of line is contained within the circle */
	else if (!frend)
		{

		/* For an open line, we need to push the end of the */
		/* line to the edge of the circle (given by prend) */
		}

	/* Find first and last point of line inside the outer circle */
	while (smoothed)
		{

		/* Find first point of line inside the outer circle */
		fsbeg = FALSE;
		if (frbeg)
			{
			idxs  = idxrbeg;
			idxe  = (closed)? idxrend: 0;
			fsbeg = exit_circle(line, centre, sd, idxs, idxe, closed, right,
						FALSE, FALSE, psbeg, &idxsbeg, &angsbeg);
			if (idxsbeg > idxrbeg
				&& ((idxsbeg < nlp/2 && idxrbeg < nlp/2)
					|| (idxsbeg > nlp/2 && idxrbeg > nlp/2)))
				{
				pr_diag("Sculpt",
					"Problem with first shoulder! idxsbeg/idxrbeg: %.3f %.3f\n",
						idxsbeg, idxrbeg);
				}
			if (fsbeg) bump_set(idxsbeg, idxrend);
			}

		/* If no exits from outer circle, turn off smoothing of closed lines */
		if (!fsbeg && closed)
			{
			smoothed = FALSE;
			break;
			}

		/* Find last point of line inside the outer circle */
		fsend = FALSE;
		if (frend)
			{
			idxs  = idxrend;
			idxe  = (closed)? idxrbeg: nlp;
			fsend = exit_circle(line, centre, sd, idxs, idxe, closed, right,
						TRUE, FALSE, psend, &idxsend, &angsend);
			if (idxsend < idxrend
				&& ((idxsend < nlp/2 && idxrend < nlp/2)
					|| (idxsend > nlp/2 && idxrend > nlp/2)))
				{
				pr_diag("Sculpt",
					"Problem with second shoulder! idxrend/idxsend: %.3f %.3f\n",
						idxrend, idxsend);
				}
			if (fsend) bump_set(idxsbeg, idxsend);
			}

		/* If no exits from outer circle, turn off smoothing */
		if (!fsbeg && !fsend)
			{
			smoothed = FALSE;
			break;
			}

		break;
		}

	/* Compute shoulder parameters (if using outer circle) */
	if (smoothed)
		{

		/* Angle between a point on the outer circle and the tangent point */
		/* on the inner circle */
		anga = fpa_acosdeg(rd/sd);
		angb = anga;
		if (!closed)
			{
			if (frbeg && !fsbeg)
				{
				dx = psbeg[X] - centre[X];
				dy = psbeg[Y] - centre[Y];
				ra = hypot(dx, dy);
				anga = fpa_acosdeg(rd/ra);
				}
			if (frend && !fsend)
				{
				dx = psend[X] - centre[X];
				dy = psend[Y] - centre[Y];
				rb = hypot(dx, dy);
				anga = fpa_acosdeg(rd/rb);
				}
			}

		/* Move exit point on inner circle to the point of tangency */
		/* if possible */
		if (right)
			{
#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Angles (R) sb/rb/re/se: %.3f %.3f %.3f %.3f  anga/angb: %.3f %.3f\n",
				angsbeg, angrbeg, angrend, angsend, anga, angb);
#			endif /* DEBUG_SCULPT */

			/* Put intersection angles in clockwise order */
			/* Largest should be angsbeg (or occasionally angrbeg) */
			if (fsbeg)
				{
				angt = angsbeg - angrbeg;
				if (angt < -180)     angrbeg -= 360;
				else if (angt > 180) angsbeg -= 360;
				}
			if (angrend > angrbeg)
				{
				angt = angrend - angrbeg;
				if      (!frbeg && angt < Dang) angrbeg  = angrend + Dang/100.0;
				else if (!frend && angt < Dang) angrend  = angrbeg - Dang/100.0;
				else if (!fsend)                angrend -= 360;
				else
					{
					angrend -= 360;
					angsend -= 360;
					}
				}
			if (fsbeg && fsend)
				{
				if (angsend > angsbeg) angsend -= 360;
				}
			/* >>>>> check angsend and angrend as well??? <<<<< */
			angrmid = (angrbeg + angrend) / 2;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Angles (R) sb/rb/re/se: %.3f %.3f %.3f %.3f  anga/angb: %.3f %.3f\n",
				angsbeg, angrbeg, angrend, angsend, anga, angb);
			if ((smoothed && frbeg && angsbeg<angrbeg) || angrbeg<angrend
				|| (smoothed && frend && angrend<angsend))
				{
				pr_diag("Sculpt",
					"Error with angles (R-decreasing-before): %.2f %.2f %.2f %.2f\n",
					angsbeg, angrbeg, angrend, angsend);
				}
#			endif /* DEBUG_SCULPT */

			if (fsbeg)
				{
				angt    = angsbeg - anga;
				angrbeg = MAX(angt, angrmid);
				}

			if (fsend)
				{
				angt    = angsend + angb;
				angrend = MIN(angt, angrmid);
				}

#			ifdef DEBUG_SCULPT
			if ((smoothed && frbeg && angsbeg<angrbeg) || angrbeg<angrend
				|| (smoothed && frend && angrend<angsend))
				{
				pr_diag("Sculpt",
					"Error with angles (R-decreasing-after): %.2f %.2f %.2f %.2f\n",
					angsbeg, angrbeg, angrend, angsend);
				}
#			endif /* DEBUG_SCULPT */
			}
		else
			{
#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Angles (L) sb/rb/re/se: %.3f %.3f %.3f %.3f  anga/angb: %.3f %.3f\n",
				angsbeg, angrbeg, angrend, angsend, anga, angb);
#			endif /* DEBUG_SCULPT */

			/* Put intersection angles in counter-clockwise order */
			/* Largest should be angsend (or occasionally angrend) */
			if (fsend)
				{
				angt = angsend - angrend;
				if (angt < -180)     angrend -= 360;
				else if (angt > 180) angsend -= 360;
				}
			if (angrbeg > angrend)
				{
				angt = angrbeg - angrend;
				if      (!frbeg && angt < Dang) angrbeg  = angrend - Dang/100.0;
				else if (!frend && angt < Dang) angrend  = angrbeg + Dang/100.0;
				else if (!fsbeg)                angrbeg -= 360;
				else
					{
					angrbeg -= 360;
					angsbeg -= 360;
					}
				}
			/* >>>>> check angsbeg and angrbeg as well??? <<<<< */
			if (fsbeg && fsend)
				{
				if (angsbeg > angsend) angsbeg -= 360;
				}
			angrmid = (angrbeg + angrend) / 2;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"Angles (L) sb/rb/re/se: %.3f %.3f %.3f %.3f  anga/angb: %.3f %.3f\n",
				angsbeg, angrbeg, angrend, angsend, anga, angb);
			if ((smoothed && frbeg && angsbeg>angrbeg) || angrbeg>angrend
				|| (smoothed && frend && angrend>angsend))
				{
				pr_diag("Sculpt",
					"Error with angles (L-increasing-before): %.2f %.2f %.2f %.2f\n",
					angsbeg, angrbeg, angrend, angsend);
				}
#			endif /* DEBUG_SCULPT */

			if (fsbeg)
				{
				angt    = angsbeg + anga;
				angrbeg = MIN(angt, angrmid);
				}

			if (fsend)
				{
				angt    = angsend - angb;
				angrend = MAX(angt, angrmid);
				}

#			ifdef DEBUG_SCULPT
			if ((smoothed && frbeg && angsbeg>angrbeg) || angrbeg>angrend
				|| (smoothed && frend && angrend>angsend))
				{
				pr_diag("Sculpt",
					"Error with angles (L-increasing-after): %.2f %.2f %.2f %.2f\n",
					angsbeg, angrbeg, angrend, angsend);
				}
#			endif /* DEBUG_SCULPT */
			}

		/* Re-normalize intersection angles */
		while (angsbeg < 0) angsbeg += 360;
		while (angrbeg < 0) angrbeg += 360;
		while (angrend < 0) angrend += 360;
		while (angsend < 0) angsend += 360;

		prbeg[X] = centre[X] + rd*fpa_cosdeg(angrbeg);
		prbeg[Y] = centre[Y] + rd*fpa_sindeg(angrbeg);
		prend[X] = centre[X] + rd*fpa_cosdeg(angrend);
		prend[Y] = centre[Y] + rd*fpa_sindeg(angrend);
		}

	/* Set intersection angles */
	else
		{

		/* Re-normalize intersection angles */
		while (angrbeg < 0) angrbeg += 360;
		while (angrend < 0) angrend += 360;

		prbeg[X] = centre[X] + rd*fpa_cosdeg(angrbeg);
		prbeg[Y] = centre[Y] + rd*fpa_sindeg(angrbeg);
		prend[X] = centre[X] + rd*fpa_cosdeg(angrend);
		prend[Y] = centre[Y] + rd*fpa_sindeg(angrend);
		}

#	ifdef DEBUG_SCULPT
	if (smoothed && fsbeg && fsend)
		{
		pr_diag("Sculpt",
			"Indexes: idxsbeg/idxrbeg/idxrend/idxsend: %.3f %.3f %.3f %.3f\n",
			idxsbeg, idxrbeg, idxrend, idxsend);
		pr_diag("Sculpt",
			"Angles:  angsbeg/angrbeg/angrend/angsend: %.3f %.3f %.3f %.3f\n",
			angsbeg, angrbeg, angrend, angsend);
		}
	else if (smoothed && fsbeg)
		{
		pr_diag("Sculpt",
			"Indexes: idxsbeg/idxrbeg/idxrend/-: %.3f %.3f %.3f -\n",
			idxsbeg, idxrbeg, idxrend);
		pr_diag("Sculpt",
			"Angles:  angsbeg/angrbeg/angrend/-: %.3f %.3f %.3f -\n",
			angsbeg, angrbeg, angrend);
		}
	else if (smoothed && fsend)
		{
		pr_diag("Sculpt",
			"Indexes: -/idxrbeg/idxrend/idxsend: - %.3f %.3f %.3f\n",
			idxrbeg, idxrend, idxsend);
		pr_diag("Sculpt",
			"Angles:  -/angrbeg/angrend/angsend: - %.3f %.3f %.3f\n",
			angrbeg, angrend, angsend);
		}
	else
		{
		pr_diag("Sculpt",
			"Indexes: -/idxrbeg/idxrend/-: - %.3f %.3f -\n",
			idxrbeg, idxrend);
		pr_diag("Sculpt",
			"Angles:  -/angrbeg/angrend/-: - %.3f %.3f -\n",
			angrbeg, angrend);
		}
#	endif /* DEBUG_SCULPT */

	/* Build new line */
	if (IsNull(NewLn)) NewLn = create_line();
	else               empty_line(NewLn);
	idxa = 0.0;
	idxb = 0.0;

	/* Check for start of line outside */
	if (closed)
		{
		if (idxrbeg>idxrend)                  outbeg = FALSE;
		else if (smoothed && idxsbeg>idxsend) outbeg = FALSE;
		else                                  outbeg = TRUE;
		outend = outbeg;
		}
	else
		{
		outbeg = frbeg;
		outend = frend;
		}

	/* Determine parameters for start of line */
	if (outbeg || closed)
		{
		idx = (smoothed && fsbeg)? idxsbeg: idxrbeg;
		ip  = floor(idx);
		if (ip < 0)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "Adjusting ip: %d  to 0\n", ip);
#			endif /* DEBUG_SCULPT */

			ip = 0;
			}

		/* Check distance from start of this span */
		/* If too close, start of span will become before shoulder point */
		/*  and original line will end on previous span                  */
		closebeg = FALSE;
		dist = line_slen(line, (float) ip, idx);
		if (dist < sdx)
			{
			closebeg = TRUE;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  Close to start - idx/dist/sdx: %.3f/%.3f/%.3f\n",
				idx, dist, sdx);
#			endif /* DEBUG_SCULPT */
			}
		}

	/* Include start of line if outside */
	if (outbeg)
		{

		/* Set last point for start of line - reset if close to start of span */
		ipl = ip;
		if (closebeg) ipl--;

#		ifdef DEBUG_SCULPT
		if (ipl >= 0)
			{
			pr_diag("Sculpt", "L %d-%d\n", 0, ipl);
			pr_diag("Sculpt",
				"  Line - From Point:     %.2f %.2f\n",
					line->points[0][X], line->points[0][Y]);
			pr_diag("Sculpt",
				"  Line - To Point:       %.2f %.2f\n",
					line->points[ipl][X], line->points[ipl][Y]);
			}
		else
			{
			pr_diag("Sculpt", "L - No Start!\n");
			}
#		endif /* DEBUG_SCULPT */

		/* Add the start of the line */
		if (ipl >= 0) append_line_pdir(NewLn, line, 0, ipl, TRUE);

		/* Reset the start index */
		if (ipl >= 0)
			{
			idxa = ipl;
			copy_point(posa, NewLn->points[ipl]);
			}
		else
			{
			idxa = 0;
			copy_point(posa, NewLn->points[0]);
			}
		}

	/* Points will pass through smoother */
	reset_pipe();
	if (smoothed)
		{
		/* enable_filter(FilterRes, 0.0); */
		enable_spline(SplineRes, FALSE, 0., 0., 0.);
		}
	enable_save();

	/* Include starting shoulder if smoothed */
	if (smoothed && fsbeg)
		{
		if (outbeg || closed)
			{

			/* If close to start, start of span becomes before shoulder point */
			if (closebeg)
				{
				copy_point(pos, line->points[ip]);

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"  Start of Span Before Shoulder Point - ip: %d\n", ip);
#				endif /* DEBUG_SCULPT */
				}

			/* Otherwise, walk backwards to estimate before shoulder point */
			else
				{
				idxx = line_walk(line, idx, -sdx);
				copy_point(pos, line_pos(line, idxx, NullInt, NullFloat));

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"  Estimate Before Shoulder Point - idx/idxx: %.3f/%.3f\n",
					idx, idxx);
#				endif /* DEBUG_SCULPT */
				}

			/* Add before shoulder point */
			point_pipe(pos);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  Before Shoulder Point: %.2f %.2f\n", pos[X], pos[Y]);
#			endif /* DEBUG_SCULPT */
			}

		/* Add begin shoulder point */
		point_pipe(psbeg);

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt",
			"  Begin Shoulder Point:  %.2f %.2f (idxsbeg: %.3f)\n",
				psbeg[X], psbeg[Y], idxsbeg);
#		endif /* DEBUG_SCULPT */
		}

	/* Include starting point on inner circle */
	point_pipe(prbeg);

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"  Begin Circle Point:    %.2f %.2f (idxrbeg: %.3f)\n",
			prbeg[X], prbeg[Y], idxrbeg);
#	endif /* DEBUG_SCULPT */

	/* Include portion of innner circle */
	if (right)
		{
		ipcrbeg = floor(angrbeg/Dang);
		ipcrend = ceil(angrend/Dang);
		angt    = fabs((double) (angrbeg - angrend));

#		ifdef DEBUG_SCULPT
		if (angrbeg<0 || angrend<0 || ipcrbeg<0 || ipcrend<0 || angrbeg<angrend)
			{
			pr_diag("Sculpt", "C? %g->%d %g->%d\n",
				angrbeg, ipcrbeg, angrend, ipcrend);
			}
		if ((ipcrend%npc > (ipcrbeg%npc)+1) && (angt > Dang/2.0))
			{
			pr_diag("Sculpt", "C- %d-%d %d-%d\n", ipcrbeg, 0, lpc, ipcrend);
			}
		else
			{
			pr_diag("Sculpt", "C- %d-%d\n", ipcrbeg, ipcrend);
			}
#		endif /* DEBUG_SCULPT */

		if (ipcrend%npc > (ipcrbeg%npc)+1) ipcrbeg += npc;

#		ifdef DEBUG_SCULPT
		if (ipcrbeg<ipcrend)
			{
			pr_diag("Sculpt",
				" Angles for C- ... angrbeg/angrend/Dang: %.2f %.2f %.2f\n",
					angrbeg, angrend, Dang);
			}
#		endif /* DEBUG_SCULPT */

		/* Use part of the circle boundary where line is inside */
		for (ipx=ipcrbeg; ipx>=ipcrend; ipx--)
			{
			jpx = ipx%npc;	/* Skip last point if first used */
			if (jpx==lpc && ipx!=lpc) continue;
			pos[X] = Tbndy->points[jpx][X] + centre[X];
			pos[Y] = Tbndy->points[jpx][Y] + centre[Y];
			point_pipe(pos);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  Circle Point:          %.2f %.2f (cip:  %.2d)\n",
					pos[X], pos[Y], ipx);
#			endif /* DEBUG_SCULPT */
			}
		}
	else
		{
		ipcrbeg = ceil(angrbeg/Dang);
		ipcrend = floor(angrend/Dang);
		angt    = fabs((double) (angrbeg - angrend));

#		ifdef DEBUG_SCULPT
		if (angrbeg<0 || angrend<0 || ipcrbeg<0 || ipcrend<0 || angrbeg>angrend)
			{
			pr_diag("Sculpt", "C? %g->%d %g->%d\n",
				angrbeg, ipcrbeg, angrend, ipcrend);
			}
		if ((ipcrend%npc < (ipcrbeg%npc)-1) && (angt > Dang/2.0))
			{
			pr_diag("Sculpt", "C+ %d-%d %d-%d\n", ipcrbeg, lpc, 0, ipcrend);
			}
		else
			{
			pr_diag("Sculpt", "C+ %d-%d\n", ipcrbeg, ipcrend);
			}
#		endif /* DEBUG_SCULPT */

		if (ipcrend%npc < (ipcrbeg%npc)-1) ipcrend += npc;

#		ifdef DEBUG_SCULPT
		if (ipcrbeg>ipcrend)
			{
			pr_diag("Sculpt",
				" Angles for C+ ... angrbeg/angrend/Dang: %.2f %.2f %.2f\n",
					angrbeg, angrend, Dang);
			}
#		endif /* DEBUG_SCULPT */

		/* Use part of the circle boundary where line is inside */
		for (ipx=ipcrbeg; ipx<=ipcrend; ipx++)
			{
			jpx = ipx%npc;	/* Skip first point if last used */
			if (jpx==0 && ipx!=0) continue;
			pos[X] = Tbndy->points[jpx][X] + centre[X];
			pos[Y] = Tbndy->points[jpx][Y] + centre[Y];
			point_pipe(pos);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  Circle Point:          %.2f %.2f (cip:  %.2d)\n",
					pos[X], pos[Y], ipx);
#			endif /* DEBUG_SCULPT */
			}
		}

	/* Include ending point on inner circle */
	point_pipe(prend);

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"  End Circle Point:      %.2f %.2f (idxrend: %.3f)\n",
			prend[X], prend[Y], idxrend);
#	endif /* DEBUG_SCULPT */

	/* Determine parameters for end of line */
	if (outend || closed)
		{
		jdx = (smoothed && fsend)? idxsend: idxrend;
		jp  = ceil(jdx);

		if (jp > nlp)
			{

#			ifdef DEBUG_SCULPT
				pr_diag("Sculpt", "Adjusting jp: %d  to %d\n", jp, nlp);
#			endif /* DEBUG_SCULPT */

			jp = nlp;
			}

		/* Check distance to end of this span */
		/* If too close, end of span will become after shoulder point */
		/*  and original line will begin on next span                 */
		closeend = FALSE;
		dist = line_slen(line, jdx, (float) jp);
		if (dist < sdx)
			{
			closeend = TRUE;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  Close to end - jdx/dist/sdx: %.3f/%.3f/%.3f\n",
				jdx, dist, sdx);
#			endif /* DEBUG_SCULPT */
			}
		}

	/* Include ending shoulder if smoothed */
	if (smoothed && fsend)
		{

		/* Add end shoulder point */
		point_pipe(psend);

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt",
			"  End Shoulder Point:    %.2f %.2f (idxsend: %.3f)\n",
				psend[X], psend[Y], idxsend);
#		endif /* DEBUG_SCULPT */

		if (outend || closed)
			{

			/* If close to end, end of span becomes after shoulder point */
			if (closeend)
				{
				copy_point(pos, line->points[jp]);

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"  End of Span After Shoulder Point - jp: %d\n", jp);
#				endif /* DEBUG_SCULPT */
				}

			/* Otherwise, walk forwards to estimate after shoulder point */
			else
				{
				jdxx = line_walk(line, jdx, sdx);
				copy_point(pos, line_pos(line, jdxx, NullInt, NullFloat));

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"  Estimate After Shoulder Point - jdx/jdxx: %.3f/%.3f\n",
					jdx, jdxx);
#				endif /* DEBUG_SCULPT */

				}

			/* Add after shoulder point */
			point_pipe(pos);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  After Shoulder Point:  %.2f %.2f\n", pos[X], pos[Y]);
#			endif /* DEBUG_SCULPT */
			}
		}

	/* Now add the smoothed section back to the line */
	flush_pipe();
	nl = recall_save(&lines);
	if (nl > 0) append_line(NewLn, lines[0]);
	reset_pipe();

	/* Reset the end index */
	idxb = NewLn->numpts - 1;
	copy_point(posb, NewLn->points[NewLn->numpts-1]);

	/* Include end of line if outside */
	if (outend)
		{

		/* Set first point for end of line - reset if close to end of span */
		jpl = jp;
		if (closeend) jpl++;

#		ifdef DEBUG_SCULPT
		if (jpl <= nlp)
			{
			pr_diag("Sculpt", "L %d-%d\n", jpl, nlp);
			pr_diag("Sculpt",
				"  Line - From Point:     %.2f %.2f (jpl:   %d)\n",
					line->points[jpl][X], line->points[jpl][Y], jpl);
			pr_diag("Sculpt",
				"  Line - To Point:       %.2f %.2f (nlp:  %d)\n",
					line->points[nlp][X], line->points[nlp][Y], nlp);
			}
		else
			{
			pr_diag("Sculpt", "L - No End!\n");
			}
#		endif /* DEBUG_SCULPT */

		if (jpl <= nlp)
			{
			append_line_pdir(NewLn, line, jpl, nlp, TRUE);
			}
		}

	/* Otherwise include mid-section of closed line */
	else if (closed)
		{

		/* Set first and last point for mid-section of closed line */
		/*  - reset if close to start or close to end of spans     */
		ipl = ip;
		jpl = jp;
		if (jpl < ipl && closebeg) ipl--;
		if (jpl < ipl && closeend) jpl++;

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt", "LM %d-%d\n", jpl, ipl);
		pr_diag("Sculpt",
			"  Line - From Point:     %.2f %.2f (jpl:   %d)\n",
				line->points[jpl][X], line->points[jpl][Y], jpl);
		pr_diag("Sculpt",
			"  Line - To Point:       %.2f %.2f (ipl:   %d)\n",
				line->points[ipl][X], line->points[ipl][Y], ipl);
#		endif /* DEBUG_SCULPT */

		append_line_pdir(NewLn, line, jpl, ipl, TRUE);
		close_line(NewLn);
		}

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"---------- Endpoints: %.2f %.2f  to  %.2f %.2f\n",
		NewLn->points[0][X], NewLn->points[0][Y],
		NewLn->points[NewLn->numpts-1][X], NewLn->points[NewLn->numpts-1][Y]);
#	endif /* DEBUG_SCULPT */

	/* Clip dividing lines to the enclosing boundary (if required) */
	/* Note that clipping starts at the opposite end to sculpting! */
	if (!closed && NotNull(sub))
		{

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Clip the dividing line to the enclosing boundary */
		divln = clip_divline_to_subarea(sub, NewLn, !atstart, TRUE, &dstat);

		if (!divln)
			{
			switch (dstat)
				{
				case DivAreaRight:
				case DivAreaLeft:
					msgkey = "area-div-out1";	break;
				case DivTooShort:
					msgkey = "area-div-out2";	break;
				default:
					msgkey = "area-div-out3";	break;
				}
			put_message(msgkey);
			(void) sleep(1);

			/* Eat the line */
			empty_line(NewLn);
			empty_line(line);
			PrevLine = NullLine;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "Return from bump_line() ... clip divide ... removing\n");
#			endif /* DEBUG_SCULPT */

#			ifdef DEBUG_TIME
			(void) printf("[bump_line] After clip divide at: %d\n",
				(long) clock());
#			endif /* DEBUG_TIME */

			if (NotNull(lright)) *lright = right;
			return FALSE;
			}

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt",
			"---------- Clipped Endpoints: %.2f %.2f  to  %.2f %.2f\n",
			divln->points[0][X], divln->points[0][Y],
			divln->points[divln->numpts-1][X],
			divln->points[divln->numpts-1][Y]);
#		endif /* DEBUG_SCULPT */

#		ifdef DEBUG_TIME
		(void) printf("[bump_line] After clip divide at: %d  Points: %d\n",
			(long) clock(), divln->numpts);
#		endif /* DEBUG_TIME */

		/* Truncate the dividing line if it crosses itself */
		if ( looped_line_crossing(divln, spos, &ips, &ipx) )
			{

			/* A dividing line that crosses itself has been found */
			pr_warning("Sculpt", "Truncating dividing line cross over!\n");
			put_message("area-div-cross");
			(void) sleep(1);

			/* Find first cross over from the opposite end too */
			cdiv = copy_line(divln);
			reverse_line(cdiv);
			if ( !looped_line_crossing(cdiv, epos, &ipe, NullInt) )
				{
				pr_warning("Sculpt", "Problem with reverse cross over!\n");
				ipe = divln->numpts - 2 - ipx;
				copy_point(epos, spos);
				}

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  ----- Crossover at span %d of %d (from start)\n",
				ips, divln->numpts-1);
			pr_diag("Sculpt",
				"  ----- Reverse crossover at span %d of %d (from end)\n",
				ipe, cdiv->numpts-1);
#			endif /* DEBUG_SCULPT */

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"  ----- Before truncating ... line with %d points\n",
				divln->numpts);
			for (ii=0; ii<divln->numpts; ii++)
				pr_diag("Sculpt", "         %.2f %.2f\n",
					divln->points[ii][X], divln->points[ii][Y]);
#			endif /* DEBUG_SCULPT */

			/* Truncate dividing line to location of cross over */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ips < 2 )
				{
				divln->numpts = ips + 1;
				add_point_to_line(divln, spos);
				}
			else if ( ips < 3 )
				divln->numpts = ips + 1;
			else
				divln->numpts = ips;

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "  ----- Forward line with %d points\n",
				divln->numpts);
			for (ii=0; ii<divln->numpts; ii++)
				pr_diag("Sculpt", "         %.2f %.2f\n",
					divln->points[ii][X], divln->points[ii][Y]);
#			endif /* DEBUG_SCULPT */

			/* Truncate dividing line from the opposite end too */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ipe < 2 )
				{
				cdiv->numpts = ipe + 1;
				add_point_to_line(cdiv, epos);
				}
			else if ( ipe < 3 )
				cdiv->numpts = ipe + 1;
			else
				cdiv->numpts = ipe;
			reverse_line(cdiv);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "  ----- Reverse line with %d points\n",
				cdiv->numpts);
			for (ii=0; ii<cdiv->numpts; ii++)
				pr_diag("Sculpt", "         %.2f %.2f\n",
					cdiv->points[ii][X], cdiv->points[ii][Y]);
#			endif /* DEBUG_SCULPT */

			/* Join the two portions of the dividing line */
			if ( spfact > 1.0 )
				smjoin_lines(divln, cdiv, FilterRes/8, SplineRes/4);
			else
				append_line(divln, cdiv);
			cdiv = destroy_line(cdiv);

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt", "  ----- Final line with %d points\n",
				divln->numpts);
			for (ii=0; ii<divln->numpts; ii++)
				pr_diag("Sculpt", "         %.2f %.2f\n",
					divln->points[ii][X], divln->points[ii][Y]);
#			endif /* DEBUG_SCULPT */

			/* Check that crossover has not changed line orientation */
			dx  = centre[X] - divln->points[0][X];
			dy  = centre[Y] - divln->points[0][Y];
			ds  = hypot(dx, dy);
			dx  = centre[X] - divln->points[divln->numpts-1][X];
			dy  = centre[Y] - divln->points[divln->numpts-1][Y];
			de  = hypot(dx, dy);

			/* Crossover has changed line orientation  ... so eat the line */
			if (bothin && ((nearbeg && ds > de) || (!nearbeg && ds < de)))
				{

				empty_line(line);
				PrevLine = NullLine;

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"Return from bump_line() ... error in crossover ... removing\n");
#				endif /* DEBUG_SCULPT */

				if (NotNull(lright)) *lright = right;
				return TRUE;
				}
			}

#		ifdef DEBUG_TIME
		(void) printf("[bump_line] After looped divide at: %d  Points: %d\n",
			(long) clock(), divln->numpts);
#		endif /* DEBUG_TIME */

		/* Replace the original line */
		empty_line(line);
		append_line(line, divln);
		condense_line(line);
		divln = destroy_line(divln);
		}

	/* Otherwise just replace the original line */
	else
		{

#		ifdef DEBUG_TIME
		(void) printf("[bump_line] After sculpt at: %d  Points: %d\n",
			(long) clock(), NewLn->numpts);
#		endif /* DEBUG_TIME */

		empty_line(line);
		append_line(line, NewLn);
		condense_line(line);
		if (closed) close_line(line);
		}

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"---------- Points: 0 to %d  idxa/idxb: %.2f %.2f\n",
		line->numpts-1, idxa, idxb);
#	endif /* DEBUG_SCULPT */

	/* Reset the indexes (if required) */
	if (outbeg)
		idxa = (float) line_closest_point(line, posa, NullFloat, NullPoint);
	if (outend || closed)
		idxb = (float) line_closest_point(line, posb, NullFloat, NullPoint);
	else
		idxb = line->numpts - 1;
	bump_set(idxa, idxb);

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"---------- Reset: 0 to %d  idxa/idxb: %.2f %.2f\n",
		line->numpts-1, idxa, idxb);
	pr_diag("Sculpt", "Return from bump_line()\n");
	pr_diag("Sculpt",
		"  Bumped line - numpts: %d\n", line->numpts);
	for (ii=0; ii<line->numpts; ii++)
		pr_diag("Sculpt",
			"      Point: %d  %.4f %.4f\n",
			ii, line->points[ii][X], line->points[ii][Y]);
#	endif /* DEBUG_SCULPT */

	if (NotNull(lright)) *lright = right;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	exit_circle
	(
	LINE	line,	/* line to test */
	POINT	cpos,	/* centre of circle */
	float	crad,	/* radius of circle */
	float	idxbeg,	/* line index to start looking */
	float	idxend,	/* line index to stop looking */
	LOGICAL	closed,	/* is line closed? */
	LOGICAL	right,	/* edit on the right or left side? */
	LOGICAL	fwd,	/* look forward or backward? */
	LOGICAL	push,	/* push to edge if inside? */
	POINT	pint,	/* point of intersection if found */
	float	*idxint,/* line index of intersection point if found */
	float	*angint /* angle on circle of intersection point if found */
	)

	{
	int		ip, ipin, ipout, nlp, dp;
	float	r2, ccum, dx, dy, d2, ds, angn, rin, rout, fact;
	LOGICAL	fin, fout, dowrap, wrapped;
	int		ispbeg;
	float	dspbeg;
	POINT	pin, pout, pos;

	/* Find where the given line enters and exits the given circle */
	/* 1 - The beginning point (index) is guaranteed to be inside */
	/* 2 - Walk forward/backward along the line until we find a point outside */
	/* 3 - Find the actual intersection with the circle boundary */

	r2   = crad*crad;
	ccum = 5.0*crad;
	nlp  = line->numpts - 1;
	fin  = FALSE;
	fout = FALSE;
	if (fwd)
		{
		dp = 1;
		ip = ceil(idxbeg);
		}
	else
		{
		dp = -1;
		ip = floor(idxbeg);
		}

	dowrap = FALSE;
	if (closed)
		{
		if (fwd) dowrap = (LOGICAL) (idxend < idxbeg);
		else     dowrap = (LOGICAL) (idxbeg < idxend);
		}
	wrapped = Not(dowrap);

	/* Walk backwards/forwards from the given point until we find a */
	/* point outside */
	dx = dy = 0.0;
	for (;;)
		{
		/* Jump out when we get past the final point */
		/*  ... and handle wrapping of closed curves */
		if (fwd)
			{
			/* >>>>> here? <<<<< */
			if (wrapped && ip>idxend) break;
			if (ip>nlp) break;
			else if (ip==nlp && closed)
				{
				ip      = 0;
				wrapped = TRUE;
				}
			/* >>>>> or here? <<<<< */
			/* >>>>>
			if (wrapped && ip>idxend) break;
			<<<<< */
			}
		else
			{
			/* >>>>> here? <<<<< */
			if (wrapped && ip<idxend) break;
			if (ip<0) break;
			else if (ip==0 && closed)
				{
				ip      = nlp;
				wrapped = TRUE;
				}
			/* >>>>> or here? <<<<< */
			/* >>>>>
			if (wrapped && ip<idxend) break;
			<<<<< */
			}

		/* >>>>> this is probably paranoid ... mar19 <<<<< */
		if (ip<0 || ip>nlp)
			{
			pr_diag("Sculpt",
				"Error with line index: %d (0-%d)\n", ip, nlp);
			}
		/* >>>>> this is probably paranoid ... mar19 <<<<< */

		/* See if current point is still inside the circle */
		dx = line->points[ip][X] - cpos[X];
		dy = line->points[ip][Y] - cpos[Y];
		d2 = dx*dx + dy*dy;
		if (d2 > r2)
			{
			/* Went outside */
			fout  = TRUE;
			ipout = ip;
			break;
			}

		/* Keep track of the latest point inside the circle */
		fin   = TRUE;
		ipin  = ip;
		ip   += dp;
		}

	/* If no outside points were found, line never exits */
	if (!fout)
		{

		/* Set default location to start/end of line */
		/* >>>>> should be idxend, not nlp/0??? <<<<< */
		/* >>>>>
		if (fwd) ip = nlp;
		else     ip = 0;
		<<<<< */
		ip = idxend;
		/* >>>>> should be idxend, not nlp/0??? <<<<< */
		copy_point(pin, line->points[ip]);

		/* Set the position on the line */
		if (NotNull(idxint)) *idxint = (float) ip;

		/* For closed lines ... find projection of start/end on circle */
		if (closed)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"In exit_circle() No outside points ... closed line!\n");
#			endif /* DEBUG_SCULPT */

			if (NotNull(angint))
				{
				dx = pin[X] - cpos[X];
				dy = pin[Y] - cpos[Y];
				*angint = fpa_atan2deg(dy, dx);
				if (*angint < 0) *angint += 360;
#				ifdef DEBUG_SCULPT
				if (*angint == 0.0)
					{
					pr_diag("Sculpt",
						"In exit_circle() dx/dy: %.2f %.2f  angint: %.2f\n",
						dx, dy, *angint);
					}
#				endif /* DEBUG_SCULPT */
				}
			return FALSE;
			}

		/* Push the line away from end point to point on circle */
		else if (push && idxbeg == idxend)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"In exit_circle() No outside points ... same start/end!\n");
#			endif /* DEBUG_SCULPT */

			dx = pin[X] - cpos[X];
			dy = pin[Y] - cpos[Y];
			angn = fpa_atan2deg(dy, dx);
			pos[X] = cpos[X] + (crad * fpa_cosdeg(angn));
			pos[Y] = cpos[Y] + (crad * fpa_sindeg(angn));
			}

		else if (push)
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"In exit_circle() No outside points ... push!\n");
#			endif /* DEBUG_SCULPT */

			/* Project the line perpendicular to itself */
			/*  ... to beyond the edge of the circle    */
			/* >>>>> should be idxend, not nlp/0??? <<<<< */
			/* >>>>>
			if (fwd) copy_point(pos, line->points[nlp-1]);
			else     copy_point(pos, line->points[1]);
			<<<<< */
			if (fwd) ip--;
			else     ip++;
			copy_point(pos, line->points[ip]);
			/* >>>>> should be idxend, not nlp/0??? <<<<< */
			dx = pin[X] - pos[X];
			dy = pin[Y] - pos[Y];
			if ((right&&fwd) || (!right&&!fwd))
				{
				dx *=  ccum;
				dy *= -ccum;
				}
			else
				{
				dx *= -ccum;
				dy *= +ccum;
				}
			pout[X] = pin[X] + dy;
			pout[Y] = pin[Y] + dx;

			/* Find the intersection */
			rin    = hypot(pin[X]-cpos[X],  pin[Y]-cpos[Y]);
			rout   = hypot(pout[X]-cpos[X], pout[Y]-cpos[Y]);

			/* >>>>> this is probably paranoid ... mar19 <<<<< */
			if (rout == rin)
				{
				pr_diag("Sculpt",
					"---------- ERROR  Divide by 0!  dx/dy: %.2f %.2f\n", dx, dy);
				copy_point(pos, pout);
				}
			/* >>>>> this is probably paranoid ... mar19 <<<<< */

			else
				{
				fact   = (crad-rin) / (rout-rin);
				dx    *= fact;
				dy    *= fact;
				pos[X] = pin[X] + dx;
				pos[Y] = pin[Y] + dy;
				}
			}

		/* Wrap the line about the start/end point */
		else
			{

#			ifdef DEBUG_SCULPT
			pr_diag("Sculpt",
				"In exit_circle() No outside points ... wrap!\n");
#			endif /* DEBUG_SCULPT */

			dx = pin[X] - cpos[X];
			dy = pin[Y] - cpos[Y];
			pos[X] = pin[X] + dx;
			pos[Y] = pin[Y] + dy;
			}

		/* Set the intersection point */
		if (NotNull(pint)) copy_point(pint, pos);

		/* Where does this lie on the circle? */
		if (NotNull(angint))
			{
			dx = pos[X] - cpos[X];
			dy = pos[Y] - cpos[Y];
			*angint = fpa_atan2deg(dy, dx);
			if (*angint < 0) *angint += 360;
#			ifdef DEBUG_SCULPT
			if (*angint == 0.0)
				{
				pr_diag("Sculpt",
					"In exit_circle() dx/dy: %.2f %.2f  angint: %.2f\n",
					dx, dy, *angint);
				}
#			endif /* DEBUG_SCULPT */
			}

		return FALSE;
		}

	/* If no inside points were found, need to use the given start point */
	if (!fin)
		{

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt", "In exit_circle() No inside points!\n");
#		endif /* DEBUG_SCULPT */

		copy_point(pin, line_pos(line, idxbeg, &ispbeg, &dspbeg));
		copy_point(pout, line->points[ipout]);
		ipin = ispbeg;
		}

	/* Otherwise we are on the mid-section */
	else
		{

#		ifdef DEBUG_SCULPT
		pr_diag("Sculpt", "In exit_circle() On mid-section!\n");
#		endif /* DEBUG_SCULPT */

		copy_point(pin, line->points[ipin]);
		copy_point(pout, line->points[ipout]);
		ispbeg = ipin;
		dspbeg = 0;
		}

	/* Find the intersection */
	dx     = pout[X] - pin[X];
	dy     = pout[Y] - pin[Y];
	rin    = hypot(pin[X]-cpos[X], pin[Y]-cpos[Y]);
	rout   = hypot(pout[X]-cpos[X], pout[Y]-cpos[Y]);

	/* >>>>> this is probably paranoid ... mar19 <<<<< */
	if (rout == rin)
		{
		pr_diag("Sculpt",
			"---------- ERROR  Divide by 0!  dx/dy: %.2f %.2f\n", dx, dy);
		copy_point(pos, pout);
		}
	/* >>>>> this is probably paranoid ... mar19 <<<<< */

	else
		{
		fact   = (crad-rin) / (rout-rin);
		dx    *= fact;
		dy    *= fact;
		pos[X] = pin[X] + dx;
		pos[Y] = pin[Y] + dy;
		}

	/* Set the intersection point */
	if (NotNull(pint)) copy_point(pint, pos);

	/* Determine position on the line */
	if (NotNull(idxint) )
		{
		ds = hypot(dx, dy);
		if (fwd)
			{
			*idxint = line_index(line, ipin, dspbeg+ds);
			if (*idxint >= ((float) ipin+1))
				{
				pr_diag("Sculpt",
					"Adjusting idxint: %.3f to below: %d\n", *idxint, ipin+1);
				*idxint = ((float) ipin+1) - .01;
				}
			}
		else
			{
			*idxint = line_index(line, ipin, dspbeg-ds);
			if (*idxint <= ((float) ipin-1))
				{
				pr_diag("Sculpt",
					"Adjusting idxint: %.3f to above: %d\n", *idxint, ipin-1);
				*idxint = ((float) ipin-1) + .01;
				}
			}
		}

	/* Where does this lie on the circle? */
	if (NotNull(angint))
		{
		dx = pos[X] - cpos[X];
		dy = pos[Y] - cpos[Y];
		*angint = fpa_atan2deg(dy, dx);
		if (*angint < 0) *angint += 360;
#		ifdef DEBUG_SCULPT
		if (*angint == 0.0)
			{
			pr_diag("Sculpt",
				"In exit_circle() dx/dy: %.2f %.2f  angint: %.2f\n",
				dx, dy, *angint);
			}
#		endif /* DEBUG_SCULPT */
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	push_circle
	(
	LINE	line,		/* line to test */
	POINT	cpos,		/* centre of circle */
	float	crad,		/* radius of circle */
	int		iseg,		/* segment of line closed to centre of circle */
	LOGICAL	right,		/* edit on the right or left side? */
	POINT	pbeg,		/* point on circle to begin */
	float	*angbeg,	/* angle on circle to begin */
	POINT	pend,		/* point on circle to end */
	float	*angend		/* angle on circle to end */
	)

	{
	int		ip;
	float	dx, dy, ang, anglast, angmin, angmax, angb, ange;
	LOGICAL	lok, rt;
	POINT	*lpts, pos;
	LINE	xline;

	if (IsNull(line) || line->numpts < 2)
		{
		(void) pr_error("Sculpt", "No line to push!\n");
		return FALSE;
		}

	if (iseg < 0 || iseg >= (line->numpts - 1))
		{
		(void) pr_error("Sculpt", "No line segment to push!\n");
		return FALSE;
		}

	/* Initialize begin/end angles from closest span */
	lpts = line->points;
	if (right)
		{

		/* Largest angle should be start of segment */
		dx = lpts[iseg][X] - cpos[X];
		dy = lpts[iseg][Y] - cpos[Y];
		angmax = fpa_atan2deg(dy, dx);
		dx = lpts[iseg+1][X] - cpos[X];
		dy = lpts[iseg+1][Y] - cpos[Y];
		angmin = fpa_atan2deg(dy, dx);
		}
	else
		{

		/* Largest angle should be end of segment */
		dx = lpts[iseg][X] - cpos[X];
		dy = lpts[iseg][Y] - cpos[Y];
		angmin = fpa_atan2deg(dy, dx);
		dx = lpts[iseg+1][X] - cpos[X];
		dy = lpts[iseg+1][Y] - cpos[Y];
		angmax = fpa_atan2deg(dy, dx);
		}

	/* Adjust angles (if required) */
	if (angmin > angmax) angmin -= 360;
	angb = right? angmax: angmin;
	ange = right? angmin: angmax;

	/* Check segments from iseg to start of line */
	lok = TRUE;
	xline = create_line();
	anglast = angb;
	for (ip=iseg; ip>0; ip--)
		{

		/* Check centre of circle wrt each line segment */
		empty_line(xline);
		add_point_to_line(xline, lpts[ip-1]);
		add_point_to_line(xline, lpts[ip]);
		line_test_point(xline, cpos, NullFloat, NullPoint, NullInt,
						NullLogical, &rt);
		if ((right && !rt) || (!right && rt))
			{
			lok = FALSE;
			}

		/* Adjust maximum or minimum angle (if required) */
		dx = lpts[ip-1][X] - cpos[X];
		dy = lpts[ip-1][Y] - cpos[Y];
		ang = fpa_atan2deg(dy, dx);
		if      (ang > anglast && (ang - anglast) > 180) ang -=360;
		else if (ang < anglast && (anglast - ang) > 180) ang +=360;
		if      ( right &&  rt && ang > angmax) angmax = ang;
		else if ( right && !rt && ang < angmin) angmin = ang;
		else if (!right &&  rt && ang > angmax) angmax = ang;
		else if (!right && !rt && ang < angmin) angmin = ang;
		}

	/* Check segments from iseg to end of line */
	anglast = ange;
	for (ip=iseg+2; ip<line->numpts; ip++)
		{

		/* Check centre of circle wrt each line segment */
		empty_line(xline);
		add_point_to_line(xline, lpts[ip-1]);
		add_point_to_line(xline, lpts[ip]);
		line_test_point(xline, cpos, NullFloat, NullPoint, NullInt,
						NullLogical, &rt);
		if ((right && !rt) || (!right && rt))
			{
			lok = FALSE;
			}

		/* Adjust maximum or minimum angle (if required) */
		dx = lpts[ip][X] - cpos[X];
		dy = lpts[ip][Y] - cpos[Y];
		ang = fpa_atan2deg(dy, dx);
		if      (ang > anglast && (ang - anglast) > 180) ang -=360;
		else if (ang < anglast && (anglast - ang) > 180) ang +=360;
		if      ( right &&  rt && ang < angmin) angmin = ang;
		else if ( right && !rt && ang > angmax) angmax = ang;
		else if (!right &&  rt && ang < angmin) angmin = ang;
		else if (!right && !rt && ang > angmax) angmax = ang;
		}
	xline = destroy_line(xline);

	/* Set arc of circle */
	if (NotNull(pbeg))
		{
		ang = right? angmax: angmin;
		pos[X] = cpos[X] + (crad * fpa_cosdeg(ang));
		pos[Y] = cpos[Y] + (crad * fpa_sindeg(ang));
		copy_point(pbeg, pos);
		}
	if (NotNull(angbeg)) *angbeg = right? angmax: angmin;
	if (NotNull(pend))
		{
		ang = right? angmin: angmax;
		pos[X] = cpos[X] + (crad * fpa_cosdeg(ang));
		pos[Y] = cpos[Y] + (crad * fpa_sindeg(ang));
		copy_point(pend, pos);
		}
	if (NotNull(angend)) *angend = right? angmin: angmax;
	return lok;
	}

/***********************************************************************
*                                                                      *
*     u e x t e n d _ X c u r v e                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL	shorten_line(LINE, int, POINT, LOGICAL);
static	LOGICAL	lengthen_line(LINE, float, POINT, LOGICAL);
static	void	lengthen_line_reset(void);

LOGICAL		uextend_Xcurve

	(
	DISPNODE	dn,
	LINE		line,
	LOGICAL		atstart,
	float		tol,
	float		spread,
	int			*butt
	)

	{
	int		button, dx, dy, iseg, iend;
	float	wx, wy, dist, dend, dseg, tolx;
	XEvent	event;
	POINT	pos, pend, ppos;
	long	mask1, mask2;
	LOGICAL	near, nold;
	LOGICAL	modified = FALSE;

	static	MARK	mark = NullMark;
	static	POINT	mpos = {0,0};
	static	CURVE	curv = NullCurve;

	if (IsNull(mark))
		{
		mark = create_mark("", "", "", mpos, 0.0);
		define_mspec(&mark->mspec, 0, 2, NULL, 0, False, 0.0, 0.0,
					 (HILITE) 3);
		}

	if (IsNull(curv))
		{
		curv = create_curve("", "", "");
		define_lspec(&curv->lspec, 255, 0, NULL, False, 5.0, 0.0,
					 (HILITE) 1);
		}

	if (NotNull(butt)) *butt = 0;
	if (ButtonDown == Button1)
		{
		mask1 = Button1MotionMask | ButtonReleaseMask;
		mask2 = Button1MotionMask;
		}
	else
		{
		pr_diag("Editor.Events",
			"[uextend_Xcurve] Ignoring button %d press\n", ButtonDown);
		return FALSE;
		}

	gxSetupTransform(dn);
	button = ButtonDown;
	dx     = ButtonX;
	dy     = ButtonY;
	glScreen2Map(dx, dy, &wx, &wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events",
		"[uextend_Xcurve] Button %d Pressed (%g,%g).\n",
		button, wx, wy);
#	endif /* DEBUG_INPUT */
	if (NotNull(butt)) *butt = ButtonDown;

	/* Check for line removal */
	if (line_too_short(line, tol))
		{

#		ifdef DEBUG_INPUT
		pr_diag("Editor.Events", "[uextend_Xcurve] Deleting curve!\n");
#		endif /* DEBUG_INPUT */

		/* Empty the line */
		empty_line(line);
		ButtonDown = NoButton;
		ButtonX    = 0;
		ButtonY    = 0;
		return FALSE;
		}

	/* Find closest point */
	set_point(pos, wx, wy);
	iend = (atstart)? 0: line->numpts-1;
	copy_point(pend, line->points[iend]);
	dend = point_dist(pos, pend);

#	ifdef DEBUG_SCULPT
	pr_diag("Sculpt",
		"[uextend_Xcurve] Begin ... pos: %.2f %.2f  pend: %.2f %.2f  dend: %.2f\n",
		pos[0], pos[1], pend[0], pend[1], dend);
#	endif /* DEBUG_SCULPT */

	near = FALSE;
	tolx = tol*5.0;
	lengthen_line_reset();

	if (dend > tol)
		{

		/* Walk along line to determine how close we are to line */
		if (atstart) dseg = line_walk(line, (float) iend,  dend);
		else         dseg = line_walk(line, (float) iend, -dend);
		iseg  = (int) dseg;
		(void) copy_point(ppos, line_pos(line, dseg, NullInt, NullFloat));
		dist  = point_dist(pos, ppos);
		near  = (LOGICAL) (dist < tol);

#		ifdef DEBUG_SCULPT
		if (near && atstart)
			pr_diag("Sculpt",
				"[uextend_Xcurve] Begin shorten line-start ... dseg: %.2f  iseg: %d\n",
				dseg, iseg);
		else if (near)
			pr_diag("Sculpt",
				"[uextend_Xcurve] Begin shorten line-end ... dseg: %.2f  iseg: %d\n",
				dseg, iseg);
		else if (atstart)
			pr_diag("Sculpt",
				"[uextend_Xcurve] Begin extend line-start\n");
		else
			pr_diag("Sculpt",
				"[uextend_Xcurve] Begin extend line-end\n");
		pr_diag("Sculpt",
			"[uextend_Xcurve] Begin ... pos: %.2f %.2f  ppos: %.2f %.2f  dist: %.2f\n",
			pos[0], pos[1], ppos[0], ppos[1], dist);
#		endif /* DEBUG_SCULPT */

		if (near) (void) shorten_line(line, iseg, ppos, atstart);
		else      (void) lengthen_line(line, spread, pos, atstart);
		modified = TRUE;
		}

	/* Set up Exclusive-OR drawing mode so 2nd draw erases */
	/* Draw the curve for the first time */
	cursor_start();
	curv->line = line;
	display_curve(curv);
	glFlush();
	sync_display();

	if (near) picking_cursor(TRUE);
	else      drawing_cursor(TRUE);
	nold = near;
	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask1, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[uextend_Xcurve] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				/* Eat any additional movements - only handle the last one */
				dx = event.xmotion.x;
				dy = event.xmotion.y;
				while (XCheckWindowEvent(X_display, X_window, mask2, &event))
					{
					dx = event.xmotion.x;
					dy = event.xmotion.y;
					}

				glScreen2Map(dx, dy, &wx, &wy);

				/* Erase curve */
				display_curve(curv);

				/* Switch drawing modes */
				cursor_next();

				/* Check for line removal */
				if (line_too_short(line, tol))
					{

					/* Erase curve and turn off abnormal drawing mode */
					display_curve(curv);
					cursor_end();
					glFlush();
					sync_display();

#					ifdef DEBUG_INPUT
					pr_diag("Editor.Events",
						"[uextend_Xcurve] Deleting curve!\n");
#					endif /* DEBUG_INPUT */

					/* Empty the line */
					empty_line(line);
					ButtonDown = NoButton;
					ButtonX    = 0;
					ButtonY    = 0;
					if (nold) picking_cursor(FALSE);
					else      drawing_cursor(FALSE);
					curv->line = NullLine;
					return FALSE;
					}

				/* Find closest point */
				set_point(pos, wx, wy);
				iend = (atstart)? 0: line->numpts-1;
				copy_point(pend, line->points[iend]);
				dend = point_dist(pos, pend);

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"[uextend_Xcurve] Mid ... pos: %.2f %.2f  pend: %.2f %.2f  dend: %.2f\n",
					pos[0], pos[1], pend[0], pend[1], dend);
#				endif /* DEBUG_SCULPT */

				if (dend > tol)
					{

					/* Walk along line to determine how close we are to line */
					if (atstart) dseg = line_walk(line, (float) iend,  dend);
					else         dseg = line_walk(line, (float) iend, -dend);
					iseg = (int) dseg;
					(void) copy_point(ppos,
									line_pos(line, dseg, NullInt, NullFloat));
					dist = point_dist(pos, ppos);

					if (nold) near = (LOGICAL) (dist < tolx);
					else      near = (LOGICAL) (dist < tol);
					if (near != nold)
						{
						if (nold) picking_cursor(FALSE);
						else      drawing_cursor(FALSE);
						if (near) picking_cursor(TRUE);
						else      drawing_cursor(TRUE);
						nold = near;
						}

#					ifdef DEBUG_SCULPT
					if (near && atstart)
						pr_diag("Sculpt",
							"[uextend_Xcurve] Mid shorten line-start ... dseg: %.2f  iseg: %d\n",
							dseg, iseg);
					else if (near)
						pr_diag("Sculpt",
							"[uextend_Xcurve] Mid shorten line-end ... dseg: %.2f  iseg: %d\n",
							dseg, iseg);
					else if (atstart)
						pr_diag("Sculpt",
							"[uextend_Xcurve] Mid extend line-start\n");
					else
						pr_diag("Sculpt",
							"[uextend_Xcurve] Mid extend line-end\n");
					pr_diag("Sculpt",
						"[uextend_Xcurve] Mid ... pos: %.2f %.2f  ppos: %.2f %.2f  dist: %.2f\n",
						pos[0], pos[1], ppos[0], ppos[1], dist);
#					endif /* DEBUG_SCULPT */

					if (near) (void) shorten_line(line, iseg, ppos, atstart);
					else      (void) lengthen_line(line, spread, pos, atstart);
					modified = TRUE;
					}

				/* Redraw the curve */
				display_curve(curv);
				glFlush();
				sync_display();

				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;
				if (button != ButtonDown) break;

				pr_diag("Editor.Events",
					"[uextend_Xcurve] Button %d Released.\n", button);

				/* Erase curve and turn off abnormal drawing mode */
				display_curve(curv);
				cursor_end();
				glFlush();
				sync_display();

				/* Set end location */
				glScreen2Map(dx, dy, &wx, &wy);

				/* Check for line removal */
				if (line_too_short(line, tol))
					{

#					ifdef DEBUG_INPUT
					pr_diag("Editor.Events",
						"[uextend_Xcurve] Deleting curve!\n");
#					endif /* DEBUG_INPUT */

					/* Empty the line */
					empty_line(line);
					ButtonDown = NoButton;
					ButtonX    = 0;
					ButtonY    = 0;
					if (nold) picking_cursor(FALSE);
					else      drawing_cursor(FALSE);
					curv->line = NullLine;
					return FALSE;
					}

				/* Find closest point */
				set_point(pos, wx, wy);
				iend = (atstart)? 0: line->numpts-1;
				copy_point(pend, line->points[iend]);
				dend = point_dist(pos, pend);

#				ifdef DEBUG_SCULPT
				pr_diag("Sculpt",
					"[uextend_Xcurve] End ... pos: %.2f %.2f  pend: %.2f %.2f  dend: %.2f\n",
					pos[0], pos[1], pend[0], pend[1], dend);
#				endif /* DEBUG_SCULPT */

				if (dend > tol)
					{

					/* Walk along line to determine how close we are to line */
					if (atstart) dseg = line_walk(line, (float) iend,  dend);
					else         dseg = line_walk(line, (float) iend, -dend);
					iseg = (int) dseg;
					(void) copy_point(ppos,
									line_pos(line, dseg, NullInt, NullFloat));
					dist = point_dist(pos, ppos);

					if (nold) near = (LOGICAL) (dist < tolx);
					else      near = (LOGICAL) (dist < tol);

#					ifdef DEBUG_SCULPT
					if (near && atstart)
						pr_diag("Sculpt",
							"[uextend_Xcurve] End shorten line-start ... dseg: %.2f  iseg: %d\n",
							dseg, iseg);
					else if (near)
						pr_diag("Sculpt",
							"[uextend_Xcurve] End shorten line-end ... dseg: %.2f  iseg: %d\n",
							dseg, iseg);
					else if (atstart)
						pr_diag("Sculpt",
							"[uextend_Xcurve] End extend line-start\n");
					else
						pr_diag("Sculpt",
							"[uextend_Xcurve] End extend line-end\n");
					pr_diag("Sculpt",
						"[uextend_Xcurve] End ... pos: %.2f %.2f  ppos: %.2f %.2f  dist: %.2f\n",
						pos[0], pos[1], ppos[0], ppos[1], dist);
#					endif /* DEBUG_SCULPT */

					if (near) (void) shorten_line(line, iseg, ppos, atstart);
					else      (void) lengthen_line(line, spread, pos, atstart);
					modified = TRUE;
					}

#				ifdef DEBUG_INPUT
				pr_diag("Editor.Events",
					"[uextend_Xcurve] Button %d Released (%g,%g).\n",
					button, wx, wy);
#				endif /* DEBUG_INPUT */

				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				if (nold) picking_cursor(FALSE);
				else      drawing_cursor(FALSE);
				curv->line = NullLine;
				return modified;
			}
		}
	}

/**********************************************************************/

static	LOGICAL	shorten_line

				(
				LINE	line,
				int		iseg,
				POINT	pos,
				LOGICAL	atstart
				)

	{
	int		lp;

	/* Build new line */
	if (IsNull(NewLn)) NewLn = create_line();
	else               empty_line(NewLn);

	lp = line->numpts - 1;
	if (atstart)
		{
		add_point_to_line(NewLn, pos);
		append_line_pdir(NewLn, line, iseg+1, lp, TRUE);
		}
	else
		{
		append_line_pdir(NewLn, line, 0, iseg, TRUE);
		add_point_to_line(NewLn, pos);
		}

	lengthen_line_reset();
	empty_line(line);
	append_line(line, NewLn);
	return TRUE;
	}

/**********************************************************************/

static	LINE	orgln = NullLine;

static	void	lengthen_line_reset(void)

	{
	if (NotNull(orgln)) orgln = destroy_line(orgln);
	}

static	LOGICAL	lengthen_line

				(
				LINE	line,
				float	spread,
				POINT	pos,
				LOGICAL	atstart
				)

	{
	int		nl, lp;
	LINE	*lines;

	/* Build copy of original line if needed */
	if (IsNull(orgln)) orgln = copy_line(line);

	/* Build new line */
	if (IsNull(NewLn)) NewLn = create_line();
	else               empty_line(NewLn);

	lp = orgln->numpts - 1;
	if (atstart)
		{
		/* Fit spline through new point and first 2 points of line */
		reset_pipe();
		enable_spline(SplineRes, FALSE, 0., 0., 0.);
		enable_save();
		put_pipe(pos[X], pos[Y]);
		put_pipe(orgln->points[0][X], orgln->points[0][Y]);
		put_pipe(orgln->points[1][X], orgln->points[1][Y]);
		flush_pipe();
		nl = recall_save(&lines);
		if (nl <= 0)
			{
			reset_pipe();
			return FALSE;
			}
		append_line(NewLn, lines[0]);
		reset_pipe();

		/* Add rest of line from second point on */
		append_line_pdir(NewLn, orgln, 2, lp, TRUE);
		}
	else
		{
		/* Add beginning of line up to second last point */
		append_line_pdir(NewLn, orgln, 0, lp-2, TRUE);

		/* Fit spline through last 2 points of line and new point */
		reset_pipe();
		enable_spline(SplineRes, FALSE, 0., 0., 0.);
		enable_save();
		put_pipe(orgln->points[lp-1][X], orgln->points[lp-1][Y]);
		put_pipe(orgln->points[lp][X], orgln->points[lp][Y]);
		put_pipe(pos[X], pos[Y]);
		flush_pipe();
		nl = recall_save(&lines);
		if (nl <= 0)
			{
			reset_pipe();
			return FALSE;
			}
		append_line(NewLn, lines[0]);
		reset_pipe();
		}

	empty_line(line);
	append_line(line, NewLn);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s e t _ X c u r v e _ m o d e s                                  *
*     d r a w i n g _ X c u r v e                                      *
*     r e a d y _ X c u r v e                                          *
*     i g n o r e _ X c u r v e                                        *
*     r e c a l l _ X c u r v e                                        *
*     d r a w _ X c u r v e                                            *
*                                                                      *
***********************************************************************/

static	LOGICAL	DoCont   = TRUE;
static	LOGICAL	DoSmooth = TRUE;

LOGICAL		set_Xcurve_modes(STRING	mode)

	{
	if (same(mode, "draw"))
		{
		switch (DrawMode)
			{
			case DRAW_CONT:		DoCont   = TRUE;
								DoSmooth = TRUE;
								return TRUE;

			case DRAW_PTPT:		DoCont   = FALSE;
								DoSmooth = (LOGICAL) (DrawSmth >= 30);
								return TRUE;
			}
		}

	else if (same(mode, "modify"))
		{
		switch (ModifyMode)
			{
			case MODIFY_CONT:	DoCont   = TRUE;
								DoSmooth = TRUE;
								return TRUE;

			case MODIFY_PTPT:	DoCont   = FALSE;
								DoSmooth = (LOGICAL) (ModifySmth >= 30);
								return TRUE;
			}
		}

	return FALSE;
	}

/******************************************************************************/


LOGICAL		drawing_Xcurve(void)

	{
	return Drawing;
	}

/******************************************************************************/

LOGICAL		ready_Xcurve(void)

	{
	return DrawReady;
	}

/******************************************************************************/

LOGICAL		ignore_Xcurve(void)

	{
	if (Drawing)
		{
		put_message("draw-cancel");
		(void) sleep(1);
		if (DrawWait) picking_cursor(FALSE);
		DrawWait = FALSE;
		end_draw();
		}

	if (DrawReady)
		{
		reset_pipe();
		DrawReady = FALSE;
		}

	return TRUE;
	}

/******************************************************************************/

int			recall_Xcurve

	(
	LINE	**lines
	)

	{
	if (!DrawReady)
		{
		if (lines) *lines = NULL;
		return 0;
		}

	DrawReady = FALSE;
	return recall_save(lines);
	}

/******************************************************************************/

static	DISPNODE	Dn;
static	MARK		Mark = NullMark;
static	POINT		Mpos = {0,0};
static	float		Fres;
static	float		Sres;
static	LOGICAL		Close;
static	LOGICAL		Pfirst = FALSE;
static	float		Px, Py;

LOGICAL		draw_Xcurve

	(
	DISPNODE	dn,
	float		fres,
	float		sres,
	LOGICAL		closeit
	)

	{
	int		button, dx, dy;
	XEvent	event;
	long	mask;

	Dn    = dn;
	Fres  = fres;
	Sres  = sres;
	Close = closeit;

	if (Drawing) return FALSE;

	mask = ButtonPressMask | ButtonReleaseMask
			| EnterWindowMask | LeaveWindowMask;
	switch (ButtonDown)
		{
		case Button1:	mask |= Button1MotionMask; break;
		default:		pr_diag("Editor.Events",
							"[draw_Xcurve] Ignoring button %d press\n",
							ButtonDown);
						return FALSE;
		}

#	ifdef DEBUG_EVENTS
	pr_diag("Editor",
		"[draw_Xcurve] In draw mode ... ButtonDown: %d\n", ButtonDown);
#	endif /* DEBUG_EVENTS */

	/* Point-by-point draw */
	if (!DoCont)
		{
		/* Initialize drawing ... and grab pointer to get points off screen */
		init_draw(TRUE, mask);

		/* Set point-by-point drawing mode */
		put_message("draw-add");
		picking_cursor(TRUE);
		DrawWait = TRUE;
		return FALSE;
		}

	/* Continuous draw only from here */
	/* Initialize drawing ... but do not grab pointer */
	init_draw(FALSE, mask);

	/* Add first point upon button down */
	add_draw(ButtonX, ButtonY);

	while (TRUE)
		{
		XWindowEvent(X_display, X_window, mask, &event);

#		ifdef DEBUG_EVENTS
		pr_diag("Editor.Events", "[draw_Xcurve] Event Type: %d\n", event.type);
#		endif /* DEBUG_EVENTS */

		switch(event.type)
			{
			/* Track cursor while button pressed */
			case MotionNotify:
				dx = event.xmotion.x;
				dy = event.xmotion.y;

				/* Add next point */
				add_draw(dx, dy);
				break;

			/* Wait for release */
			case ButtonRelease:
				button = event.xbutton.button;
				dx     = event.xbutton.x;
				dy     = event.xbutton.y;

				pr_diag("Editor.Events",
					"[draw_Xcurve] Button %d released\n", button);
				if (button != ButtonDown && ButtonDown != NoButton)
					{
					pr_diag("Editor.Events",
						"[draw_Xcurve] Button %d still pressed!\n", ButtonDown);
					break;
					}

				/* Add last point upon button up */
				add_draw(dx, dy);
				end_draw();
				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				return TRUE;

			/* Anything else */
#			ifdef DEBUG_EVENTS
			case LeaveNotify:
				pr_diag("Editor.Events", "[draw_Xcurve] Leaving\n");
				break;

			case EnterNotify:
				pr_diag("Editor.Events", "[draw_Xcurve] Entering\n");
				break;
#			endif /* DEBUG_EVENTS */
			}
		}
	}

/***********************************************************************
*                                                                      *
*     i n i t _ d r a w                                                *
*     a d d _ d r a w                                                  *
*     e n d _ d r a w                                                  *
*                                                                      *
***********************************************************************/

void	init_draw

	(
	LOGICAL grab,
	long	mask
	)

	{
	if (Drawing) return;

	if (IsNull(Mark))
		{
		Mark = create_mark("", "", "", Mpos, 0.0);
		define_mspec(&Mark->mspec, 0, 2, NULL, 0, False, 0.0, 0.0,
					 (HILITE) 3);
		}

	DrawReady = FALSE;
	gxSetupTransform(Dn);

	reset_pipe();
	if (DoSmooth)
		{
		enable_filter(Fres, 0.0);
		enable_spline(Sres, Close, 0., 0., 0.);
		}
	enable_save();

	mode_draw_setable(FALSE);
	drawing_cursor(TRUE);
	if (grab)
		{
		XtGrabPointer(X_widget, True, mask, GrabModeAsync, GrabModeAsync,
					None, None, CurrentTime);
		DrawGrab = TRUE;
		}
	Pfirst  = TRUE;
	Drawing = TRUE;
	interrupt_control(FALSE, FALSE); /* resume interrupts */
	}

/******************************************************************************/

void	add_draw

	(
	int		dx,
	int		dy
	)

	{
	float	wx, wy;
	POINT	p;

	if (!Drawing) return;

	glScreen2Map(dx, dy, &wx, &wy);
	put_pipe(wx, wy);

#	ifdef DEBUG_INPUT
	pr_diag("Editor.Events", "[add_draw] Add point (%g %g)\n", wx, wy);
#	endif /* DEBUG_INPUT */

	set_point(p, wx, wy);
	define_mark_anchor(Mark, p, 0.0);
	Mark->mspec.type = 3;
	display_mark(Mark);
	glFlush();
	sync_display();

	if (Pfirst)
		{
		Px = wx;
		Py = wy;
		Pfirst = FALSE;
		}
	}

/******************************************************************************/

void	end_draw(void)

	{
	if (!Drawing) return;

	if (DrawGrab)
		{
		XtUngrabPointer(X_widget, CurrentTime);
		DrawGrab = FALSE;
		}

	/* Close the curve if required */
	if (Close && !DoSmooth)
		{
		if (!Pfirst) put_pipe(Px, Py);
		}
	flush_pipe();
	sync_display();

	drawing_cursor(FALSE);
	mode_draw_setable(TRUE);
	Pfirst     = FALSE;
	Drawing    = FALSE;
	DrawReady  = TRUE;
	interrupt_control(TRUE, FALSE); /* suspend interrupts */
	}

/***********************************************************************
*                                                                      *
*     c u r s o r _ s t a r t                                          *
*     c u r s o r _ n e x t                                            *
*     c u r s o r _ e n d                                              *
*                                                                      *
***********************************************************************/

static	LOGICAL	DoXor     = TRUE;
static	LOGICAL	DoFlicker = FALSE;
static	LOGICAL	ModeSet   = FALSE;

/**********************************************************************/

static void	cursor_start(void)

	{
	STRING	cmode;

	if (!ModeSet)
		{
		ModeSet = TRUE;
		cmode   = get_feature_mode("Cursor.Display");
		if (blank(cmode))
			{
			DoXor     = TRUE;
			DoFlicker = FALSE;
			}
		else if (same_ic(cmode, "xor"))
			{
			DoXor     = TRUE;
			DoFlicker = FALSE;
			}
		else if (same_ic(cmode, "xnor"))
			{
			DoXor     = FALSE;
			DoFlicker = FALSE;
			}
		else if (same_ic(cmode, "flicker"))
			{
			DoXor     = TRUE;
			DoFlicker = TRUE;
			}
		else
			{
			pr_warning("Editor",
				"Supported Cursor.Display Modes: xor xnor flicker\n");
			DoXor     = TRUE;
			DoFlicker = FALSE;
			}
		}

	cursor_end();
	if (DoFlicker) DoXor = TRUE;
	if (DoXor) glLogicOp(glLO_XOR);
	else       glLogicOp(glLO_XNOR);
	}

/**********************************************************************/

static void	cursor_next(void)

	{
	if (DoFlicker)
		{
		if (DoXor) DoXor = FALSE;
		else       DoXor = TRUE;
		if (DoXor) glLogicOp(glLO_XOR);
		else       glLogicOp(glLO_XNOR);
		}
	}

/**********************************************************************/

static void	cursor_end(void)

	{
	glLogicOp(glLO_SRC);
	}
