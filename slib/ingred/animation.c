/***********************************************************************
*                                                                      *
*     a n i m a t i o n . c                                            *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles the animation of depiction and interpolated fields.      *
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

#include <ulimit.h>

static	int	Iframe = 0;
static	int	Ishown = 0;
static	int	Ifirst = 0;
static	int	Ilast  = 0;
static	int	Ibegin = 0;
static	int	Iend   = 0;

static	XtIntervalId	TimerHandle = 0;

static	LOGICAL	suspend = FALSE;

static	void	erase_movie_frame(void);
static	void	show_movie_frame(LOGICAL *);
static	void	show_next_frame(XtPointer, XtIntervalId *);
static	void	report_mem(STRING, STRING, LOGICAL);

/***********************************************************************
*                                                                      *
*     i n i t _ a n i m a t i o n                                      *
*     s u s p e n d _ a n i m a t i o n                                *
*     e x i t _ a n i m a t i o n                                      *
*                                                                      *
***********************************************************************/

LOGICAL	init_animation(void)

	{
	DFLIST	*dfld;
	int		idfld;

	MovieShown = TRUE;

	/* If we have suspended animation to zoom ... return immediately! */
	if (suspend)
		{
		suspend = FALSE;
		return TRUE;
		}

	/* Start memory tracking */
	report_mem("Animation", "Initial Memory", TRUE);

	/* Set the initial visibility of fields for animation equal to that */
	/* for normal viewing */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		dfld->showmov = dfield_shown(dfld);
		}

	/* Erase the depictions from the map area */
	hide_edit_field();
	show_blank();

	return TRUE;
	}

LOGICAL	suspend_animation(void)

	{
	suspend = TRUE;

	return TRUE;
	}

LOGICAL	exit_animation(void)

	{
	DFLIST	*dfld;
	int		idfld;

	MovieShown = FALSE;
	erase_movie_frame();
	present_node(DnMap);

	/* Restore visibility of fields to their normal viewing states */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		dfld->showmov = FALSE;
		set_dfield_visibility(dfld);

		if (ConserveMem) release_dfield_interp(dfld);
		}

	/* Restore the depictions on the map area */
	show_edit_field();
	hide_blank();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s t a r t _ a n i m a t i o n                                    *
*     s t o p _ a n i m a t i o n                                      *
*     a n i m a t i o n _ f r a m e                                    *
*     a n i m a t i o n _ m o d e                                      *
*     a n i m a t i o n _ d e l a y                                    *
*     a n i m a t i o n _ d f i e l d _ s t a t e                      *
*                                                                      *
***********************************************************************/

LOGICAL	start_animation(void)

	{
	if (MovieGoing) (void) stop_animation();

	/* Handle error conditions */
	if (!MovieOK)
		{
		put_message(MovieMsg);
		return FALSE;
		}

	/* Start the movie (i.e. show the first frame) */
#	ifdef DEBUG
	pr_diag("Editor", "[start_animation] %s\n", MovieMsg);
#	endif /* DEBUG */
	put_message(MovieMsg);
	MovieGoing = TRUE;
	show_next_frame((XtPointer) NULL, (XtIntervalId *) NULL);

	return MovieGoing;
	}

/**********************************************************************/

LOGICAL	stop_animation(void)

	{
	if (!MovieGoing) return FALSE;

	/* Remove the call-back */
	if (TimerHandle) XtRemoveTimeOut(TimerHandle);
	TimerHandle = 0;

	/* Clean up */
#	ifdef DEBUG
	pr_diag("Editor", "[stop_animation] Stopped\n");
#	endif /* DEBUG */
	MovieGoing = FALSE;
	put_message("movie-off");

	return TRUE;
	}

/**********************************************************************/

LOGICAL	animation_frame

	(
	STRING	vtime
	)

	{
	int		mplus, mpold, pframe, mfirst, itime;
	LOGICAL	valid;

	if (!MovieOK)   return FALSE;
	if (MovieGoing) return FALSE;

	/* Advance to next frame */
	if (same(vtime, "NEXT"))
		{
		switch (MovieMode)
			{
			case MOVIE_DEPICT:
				/* Find next depiction time */
				Iframe = next_depict_time(Ishown);
				if (Iframe < 0 || Iframe > Iend) Iframe = Ibegin;
				break;

			case MOVIE_INTERP:
				Iframe = Ishown+1;
				/* Check if we've used up all the real interpolations */
				if (Iframe > Iend) Iframe = Ibegin;
				break;
			}
		}

	/* Back up to previous frame */
	else if (same(vtime, "PREV"))
		{
		switch (MovieMode)
			{
			case MOVIE_DEPICT:
				/* Find previous depiction time */
				Iframe = prev_depict_time(Ishown);
				if (Iframe < 0 || Iframe < Ibegin) Iframe = Iend;
				break;

			case MOVIE_INTERP:
				Iframe = Ishown-1;
				/* Check if we've used up all the real interpolations */
				if (Iframe < Ibegin) Iframe = Iend;
				break;
			}
		}

	/* Go to given time */
	else
		{
		/* Compute prog time */
		pframe = Iframe;
		Iframe = Ibegin;
		mfirst = TimeList[Ifirst].mplus;
		mplus  = mfirst + calc_prog_time_minutes(TimeList[Ifirst].jtime, vtime,
																		&valid);
		if (!valid) return FALSE;

		/* Find matching frame */
		switch (MovieMode)
			{
			case MOVIE_DEPICT:
				mpold = TimeList[pframe].mplus;
				Iframe = pframe;
				if (mplus > mpold)
					{
					for (itime=pframe+1; itime<=Iend; itime++)
						{
						if (!TimeList[itime].depict) continue;
						if (TimeList[itime].mplus >= mplus)
							{
							Iframe = itime;
							Iframe = MAX(Iframe, Ibegin);
							Iframe = MIN(Iframe, Iend);
							break;
							}
						}
					}
				else if (mplus < mpold)
					{
					for (itime=pframe-1; itime>=Ibegin; itime--)
						{
						if (!TimeList[itime].depict) continue;
						if (TimeList[itime].mplus <= mplus)
							{
							Iframe = itime;
							Iframe = MAX(Iframe, Ibegin);
							Iframe = MIN(Iframe, Iend);
							break;
							}
						}
					}
				break;

			case MOVIE_INTERP:
				Iframe = (mplus - mfirst) / DTween;
				Iframe = MAX(Iframe, Ibegin);
				Iframe = MIN(Iframe, Iend);
				break;

			default:
				return FALSE;
			}
		}

	show_movie_frame((LOGICAL *)0);
	Ishown = Iframe;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	animation_mode

	(
	STRING	mode,
	STRING	svt,
	STRING	evt
	)

	{
	int				ivt, idfld, exist, mplus;
	DFLIST			*dfld;
	LOGICAL			dolink, interp, notween, nosnap, valid;

	MovieMsg  = "movie-select";
	MovieMode = MOVIE_NONE;
	MovieOK   = FALSE;
	Ifirst    = first_depict_time();
	Ilast     = last_depict_time();

	if (ConserveMem) release_interp();
	clear_equation_database();

	/* Animation of regular depictions */
	if (same(mode,"DEPICT"))
		{
		MovieMsg  = "movie-depict";
		MovieMode = MOVIE_DEPICT;
		Ibegin    = Ifirst;
		Iend      = Ilast;
		ivt = which_depict_time(svt);
		if (ivt >= 0) Ibegin = ivt;
		ivt = which_depict_time(evt);
		if (ivt >= 0) Iend   = ivt;
		if (Iframe < Ibegin)    Iframe = Ibegin;
		else if (Iframe > Iend) Iframe = Iend;
		MovieOK   = TRUE;
#		ifdef DEBUG
		pr_diag("Editor", "[animation_mode] %s\n", MovieMsg);
		pr_diag("Editor", "[animation_mode]  Frames: %d to %d (%d to %d)\n",
			Ibegin, Iend, Ifirst, Ilast);
#		endif /* DEBUG */
		}

	/* Animation of interpolated depictions */
	else if (same(mode,"INTERP"))
		{
		/* Make sure at least some fields are interpolated */
		dolink  = FALSE;
		interp  = FALSE;
		notween = FALSE;
		nosnap  = FALSE;
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			dfld = DfldList + idfld;
			if (!dfld->there)  continue;
			if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;
			dolink = TRUE;
			if (!dfld->interp) continue;
			if (!dfld->intlab) continue;
			interp = TRUE;
			if (tdep_special(dfld->tdep))
				{
				if (dfld->snaps)  continue;
				nosnap = TRUE;
				}
			else
				{
				if (dfld->tweens) continue;
				notween = TRUE;
				}
			}

		if (!dolink)
			{
			MovieMsg = "movie-interp-nolink";
			return FALSE;
			}

		if (!interp)
			{
			MovieMsg = "movie-interp-nointerp";
			return FALSE;
			}

		if (notween || nosnap)
			{
			/* Make sure existing interpolated files are consistent with */
			/* current depiction sequence */
			exist = verify_interp(TRUE, FALSE);
			if (!exist)
				{
				MovieMsg = "movie-interp-nofile";
				return FALSE;
				}

			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;
				if (!dfld->there)  continue;
				if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;
				dolink = TRUE;
				if (!dfld->interp) continue;
				if (!dfld->intlab) continue;
				set_dfield_visibility(dfld);
				}
			}

		MovieMsg  = "movie-interp";
		MovieMode = MOVIE_INTERP;
		Ibegin    = 0;
		Iend      = NumTween-1;
		if ( !blank(svt) )
			{
			mplus = calc_prog_time_minutes(TimeList[Ifirst].jtime, svt, &valid);
			if (valid) Ibegin = NINT(mplus / DTween);
			}
		if ( !blank(evt) )
			{
			mplus = calc_prog_time_minutes(TimeList[Ifirst].jtime, evt, &valid);
			if (valid) Iend   = NINT(mplus / DTween);
			}
		if (Iframe < Ibegin)    Iframe = Ibegin;
		else if (Iframe > Iend) Iframe = Iend;
		MovieOK   = TRUE;
#		ifdef DEBUG
		pr_diag("Editor", "[animation_mode] %s\n", MovieMsg);
		pr_diag("Editor", "[animation_mode]  Frames: %d to %d (%d to %d)\n",
			Ibegin, Iend, 0, NumTween-1);
#		endif /* DEBUG */
		}

	/* Unknown mode */
	else
		{
		MovieMsg = "movie-select";
#		ifdef DEBUG
		pr_diag("Editor", "[animation_mode] %s\n", MovieMsg);
#		endif /* DEBUG */
		return FALSE;
		}

	if (!MovieGoing) show_movie_frame((LOGICAL *)0);
	Ishown = Iframe;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	animation_delay

	(
	int		delay
	)

	{
	delay = MAX(delay, 10);
	MovieWait = delay;
#	ifdef DEBUG
	pr_diag("Editor", "[animation_delay] %d\n", delay);
#	endif /* DEBUG */
	return TRUE;
	}

/**********************************************************************/

LOGICAL	animation_dfield_state

	(
	STRING	elem,
	STRING	levl,
	STRING	state
	)

	{
	LOGICAL	showmovie;
	DFLIST	*dfld;

	if (blank(elem))  return FALSE;
	if (blank(levl))  return FALSE;
	if (blank(state)) return FALSE;

	/* Find the given field */
	dfld = find_dfield(elem, levl);
	if (IsNull(dfld)) return FALSE;

	/* Do we turn it on or off ? */
	if (same(state, "ON"))       showmovie = TRUE;
	else if (same(state, "OFF")) showmovie = FALSE;
	else                         return FALSE;

	/* Turn on/off the field if not already done */
	if ( showmovie &&  dfld->showmov) return TRUE;
	if (!showmovie && !dfld->showmov) return TRUE;
	dfld->showmov = showmovie;

	/* Retrieve or release interpolations if necessary */
	if (ConserveMem)
		{
		if (showmovie)
			{
			report_mem("Animation", "Memory Before Show", FALSE);
			report_mem("Animation", "Memory After Show", FALSE);
			}
		else
			{
			report_mem("Animation", "Memory Before Hide", FALSE);
			release_dfield_interp(dfld);
			clear_equation_database();
			report_mem("Animation", "Memory After Hide", FALSE);
			}
		}

	/* Set the visibility if different from the normal viewing state */
	/* Highlight each depiction appropriately */
	/* if (!showmovie) return TRUE; */
	set_dfield_visibility(dfld);
	if (!MovieGoing) show_movie_frame((LOGICAL *)0);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ n e x t _ f r a m e                                    *
*     s h o w _ m o v i e _ f r a m e                                  *
*     e r a s e _ m o v i e _ f r a m e                                *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
static	void	show_next_frame

	(
	XtPointer		data,
	XtIntervalId	*interval
	)

	{
#ifdef NOT_NEEDED
	DFLIST			*dfld;
	int				idfld, mplus, itime;
	FIELD			fld;
	SURFACE			sfc;
	int				pyear, pjday, phour, pmin, psec, pmonth, pmday;
	STRING			tstamp;
#endif

	unsigned long	delay;
	LOGICAL			contoured;

	/* We cannot get into this function without a valid sequence to display */
	/* unless the animation has just been stopped */

	/* Show the current frame */
	TimerHandle = 0;
	show_movie_frame(&contoured);
	Ishown = Iframe;
	if (!MovieGoing) return;

	/* Advance to next frame */
	switch (MovieMode)
		{
		case MOVIE_DEPICT:
			/* Find next depiction time */
			Iframe = next_depict_time(Ishown);
			if (Iframe < 0 || Iframe > Iend) Iframe = Ibegin;
			break;

		case MOVIE_INTERP:
			Iframe = Ishown+1;
			/* Check if we've used up all the real interpolations */
			if (Iframe > Iend) Iframe = Ibegin;
			break;
		}

	/* Calculate correct pause between frames */
	if (contoured)             delay = 10;
	else if (Iframe == Ibegin) delay = 2*MovieWait + 750;
	else                       delay = MovieWait;

	/* Schedule next frame */
	TimerHandle = XtAppAddTimeOut(X_appcon, delay, show_next_frame,
					(XtPointer) NULL);
	}
	
/**********************************************************************/

static	void	show_movie_frame

	(
	LOGICAL *contoured
	)

	{
	DFLIST		*dfld;
	int			idfld, mplus, itime;
	FIELD		fld;
	SET			lbls;
	SURFACE		sfc;
	int			pyear, pjday, phour, pminute, pmonth, pmday;
	STRING		tstamp, elem, levl, ftype;
	POINTER		fdata;

	/* Empty the previous frame */
	erase_movie_frame();

	/* Compute current prog time */
	switch (MovieMode)
		{
		case MOVIE_DEPICT:
			mplus = TimeList[Iframe].mplus;
			break;

		case MOVIE_INTERP:
			mplus = TimeList[Ifirst].mplus + Iframe*DTween;
			break;

		default:
			return;
		}

	/* Build the current frame */
	if (contoured) *contoured = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		elem = dfld->element;
		levl = dfld->level;
		fld  = NullFld;
		lbls = NullSet;
		switch (MovieMode)
			{
			case MOVIE_DEPICT:
				if (!dfld->there)         continue;
				if (!dfld->showmov)       continue;
				if (IsNull(dfld->fields)) continue;
				if (tdep_special(dfld->tdep))
					{
					for (itime=Iframe; itime>=0; itime--)
						{
						if (NotNull(dfld->fields[itime]))
							{
							fld  = dfld->fields[itime];
							lbls = dfld->flabs[itime];
							break;
							}
						}
					}
				else
					{
					fld  = dfld->fields[Iframe];
					lbls = dfld->flabs[Iframe];
					}
				break;

			case MOVIE_INTERP:
				if (!dfld->there)   continue;
				if (!dfld->showmov) continue;
				if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;
				if (!dfld->interp) continue;
				if (!dfld->intlab) continue;

				if (tdep_special(dfld->tdep))
					{
					for (itime=0; itime<NumTime-1; itime++)
						{
						acquire_dfield_snaps(dfld, itime);
						if (IsNull(dfld->snaps)) break;
						if (NotNull(dfld->snaps[itime]))
							{
							fld  = dfld->snaps[itime];
							lbls = dfld->slabs[itime];
							}
						if (itime >= NumTime-1)              break;
						if (TimeList[itime+1].mplus > mplus) break;
						}
					}
				else
					{
					acquire_dfield_tweens(dfld, Iframe);
					if (IsNull(dfld->tweens)) continue;
					if (NotNull(dfld->tweens[Iframe]))
						{
						fld  = dfld->tweens[Iframe];
						lbls = dfld->tlabs[Iframe];
						}
					clear_equation_database();
					}
				break;
			}

		if (IsNull(fld)) continue;

		/* Contour the current field if necessary */
		if (fld->ftype == FtypeSfc)
			{
			sfc = fld->data.sfc;
			if	( NotNull(sfc)
				  && ( IsNull(sfc->patches)
					   || IsNull(sfc->patches[0][0])
					   || !sfc->patches[0][0]->defined
					   || IsNull(sfc->patches[0][0]->contours)
					 )
				)
				{
				if (minutes_in_depictions())
					put_message("movie-contour-mins",
								fld->level, fld->element,
								hour_minute_string(0, mplus));
				else
					put_message("movie-contour",
								fld->level, fld->element, mplus/60);
				contour_surface(sfc);
				band_contour_surface(sfc);
				put_message(MovieMsg);
				if (contoured) *contoured = TRUE;
				}
			}

		/* Build frame to view */
		recall_fld_data(fld, &ftype, &fdata);
		fld = create_field(fld->entity, elem, levl);
		define_fld_data(fld, ftype, fdata);
		add_field_to_metafile(BlankMeta, fld);
		if (NotNull(lbls))
			{
			prepare_set(lbls);
			fld = create_field("d", elem, levl);
			define_fld_data(fld, "set", (POINTER)lbls);
			add_field_to_metafile(BlankMeta, fld);
			}
		}

	/* Determine the time of the current frame */
	tstamp = NULL;
	switch (MovieMode)
		{
		case MOVIE_DEPICT:
			tstamp = TimeList[Iframe].jtime;

			if (minutes_in_depictions())
				put_message("movie-depict-tstamp-mins",
							TimeList[Iframe].mtime,
							hour_minute_string(0, mplus));
			else
				put_message("movie-depict-tstamp",
							TimeList[Iframe].mtime, mplus/60);
			break;

		case MOVIE_INTERP:
			report_mem("Animation", "Memory After Disp", FALSE);
			pyear   = Syear;
			pjday   = Sjday;
			phour   = Shour;
			pminute = Sminute + mplus;
			tnorm(&pyear, &pjday, &phour, &pminute, NullInt);
			tstamp = build_tstamp(pyear, pjday, phour, pminute, FALSE,
						minutes_in_depictions());
			mdate(&pyear, &pjday, &pmonth, &pmday);

			if (minutes_in_depictions())
				put_message("movie-interp-tstamp-mins",
							pyear, pmonth, pmday, phour, pminute,
							hour_minute_string(0, mplus));
			else
				put_message("movie-interp-tstamp",
							pyear, pmonth, pmday, phour, mplus/60);
			break;
		}
	showing_chart(tstamp);

	/* Display the current frame */
	present_node(DnMap);
	}

/**********************************************************************/

static	void	erase_movie_frame(void)
	{
	int		ifld;
	FIELD	fld;

    for (ifld=0; ifld<BlankMeta->numfld; ifld++)
        {
        fld = BlankMeta->fields[ifld];
        switch (fld->ftype)
            {
            case FtypeSfc:  fld->data.sfc  = NullSfc;
                            break;
            case FtypeSet:  fld->data.set  = NullSet;
                            break;
            case FtypePlot: fld->data.plot = NullPlot;
                            break;
            }
        }
	empty_metafile(BlankMeta);
	}


/***********************************************************************
*                                                                      *
*     r e p o r t _ m e m                                              *
*                                                                      *
***********************************************************************/

static	STRING	Omem = 0;
static	STRING	Pmem = 0;
static	STRING	Amem = 0;
static	STRING	Xmem = 0;

#ifdef MACHINE_HP
	static	int		MemSize = UL_GETMAXBRK;
#endif
#ifdef MACHINE_PCLINUX
	static	int		MemSize = __UL_GETMAXBRK;
#endif
#ifdef MACHINE_SUN
	static	int		MemSize = UL_GMEMLIM;
#endif

static	void	report_mem

	(
	STRING	module,
	STRING	msg,
	LOGICAL	start
	)

	{
	if (start)
		{
		Omem = (STRING) sbrk(0);
		Xmem = (STRING) ulimit(MemSize);
		pr_diag(module, "%s: %d Max: %d Free: %d\n", msg, Omem, Xmem, Xmem-Omem);
		Pmem = Omem;
		}

	else
		{
		Amem = (STRING) sbrk(0);
		if (Amem > Pmem) pr_diag(module, "%s: %d Used: %d Net: %d Free: %d\n",
									msg, Amem, Amem-Pmem, Amem-Omem, Xmem-Amem);
		Pmem = Amem;
		}
	}
