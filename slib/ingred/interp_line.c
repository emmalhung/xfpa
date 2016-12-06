/***********************************************************************
*                                                                      *
*     i n t e r p _ l i n e . c                                        *
*                                                                      *
*     Routines to time-interpolate line fields.                        *
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

#undef DEBUG_INTERP
#undef DEBUG_INTERP_LINES
#undef DEBUG_INTERP_POINTS
#undef DEBUG_INTERP_LINKS
#undef EFF

/* Weights to select number of fit points throughout segment interpolations */
#define W1 0.75
#define W2 0.25

/* Keyframe time mask buffer */
static  int     NumKey   = 0;
static  int     *KeyTime = NULL;
static  int     *KeyPlus = NULL;
static  FIELD   *KeyList = NULL;
static  SET     *KeyLabs = NULL;

/* Structures for keyframe info for curve links */
typedef struct
		{
		int		icurv;
		LOGICAL	link;
		POINT	lpos;
		CURVE	curv;
		LINE	line;
		int		nseg;
		int		*dseg;
		float	*dspan;
		POINT	*dspt;
		LOGICAL	flip;
		HAND	sense;
		} CLKEY;

/* Structures for curve links */
typedef struct
		{
		int		skey, splus;
		int		ekey, eplus;
		int		icom;
		int		ncom;
		int		*common;
		CLKEY	*key;
		LCTRL	*ctrl;
		} CLINK;
static  int     nCLink  = 0;
static  CLINK   *CLinks = NULL;

static  int     nlines;
static  LINE    *lines;

static	void	spatial(CLINK *, int, int, int, int, double **, double **);
static  int     free_curve_links(void);
static  int     make_curve_links(DFLIST *);
static  void    debug_curve_links(void);

/***********************************************************************
*                                                                      *
*    i n t e r p _ l i n e                                             *
*                                                                      *
*    Perform the time interpolation of line features from the working  *
*    depiction sequence, onto the given interval.                      *
*                                                                      *
*    The set of keyframe images is provided in dfld->fields.           *
*    The set of generated images is deposited in dfld->tweens.         *
*                                                                      *
***********************************************************************/

LOGICAL	interp_line

	(
	DFLIST	*dfld,
	LOGICAL	show,
	LOGICAL	showtween
	)

	{
	int		ivt, iclink, jlink, imem;
	int		ikey, jkey, pkey, nkey;
	int		itween, jtween, ptween, ntween, nxtween;
	int     ip, np, npmin=0, npmax=0, isp, iky;
	int     skey, stween, smplus, sxplus, sxtween;
	int     ekey, etween, emplus, explus, extween;
	int		ntot, smin, emax;
	TSTAMP	vts, vte;
	double  sfact, efact;
	int		nspts, ncont, icont, inum, icnum, nsub;
	int		mfirst, mplus;
	SET		keyset, genset;
	FIELD	*genlist, fld;
	SET		*genlabs;
	CURVE	curv, wcurv, c2;
	SPOT	spot, wspot;
	LINE    line;
	STRING	ent, elem, lev;
	float	len, minlen=0.0, res, minres;
	float	x, y, dss, dse;
	int		nseg, iseg, ips, ipe, ipseg;
	LOGICAL	closed;
	CLINK	*clink;
	CLKEY	*cikey, *cjkey;
	LCTRL	*ctrl;

	SPOT	*slist = NullSpotList;
	int		nsl    = 0;
	int		isl;

	int		*npseg;
	double	*dxseg, *dyseg, *ddseg;
	double  *keytimes, *tweentimes;
	double  *kbufx, *tbufx, **keyx, **tweenx, kbarx, tbarx, xoff;
	double  *kbufy, *tbufy, **keyy, **tweeny, kbary, tbary, yoff;
	double  *keytimes2;
	double  *kbufx2, **keyx2;
	double  *kbufy2, **keyy2;
	double	*subpts, *dspts;
	double	*dxsubs, *dysubs, *dxpts, *dypts;

	/* See if we can or even need to interpolate */
	if (!dfld)         return FALSE;
	if (!dfld->dolink) return FALSE;
	switch (dfld->editor)
		{
		case FpaC_LINE:     break;
		default:       return FALSE;
		}
	if (NumTime < 2)   return FALSE;
	if (NumTween < 2)  return FALSE;
	if (!dfld->linked) return FALSE;
	if (dfld->interp && dfld->intlab) return TRUE;
	if (!dfld->there)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_line() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_line() - F %s %s\n", dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->interp   = TRUE;
		dfld->intlab   = TRUE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		(void) save_links();
		return TRUE;
		}

	showtween = (LOGICAL) (show && showtween);
	busy_cursor(TRUE);

	/* Build keyframe list and time mask buffer */
	NumKey  = 0;
	KeyTime = GETMEM(KeyTime, int, NumTime);
	KeyPlus = GETMEM(KeyPlus, int, NumTime);
	KeyList = GETMEM(KeyList, FIELD, NumTime);
	KeyLabs = GETMEM(KeyLabs, SET, NumTime);
	for (ivt=0; ivt<NumTime; ivt++)
		{
		if (!dfld->fields[ivt])      continue;
		if (!dfld->frames[ivt].meta) continue;
		ikey = NumKey++;
		KeyTime[ikey] = ivt;
		KeyPlus[ikey] = TimeList[ivt].mplus;
		KeyList[ikey] = dfld->fields[ivt];
		KeyLabs[ikey] = dfld->flabs[ivt];
		}

	/* Build the tween frame buffer */
	prepare_dfield_tweens(dfld);
	genlist = dfld->tweens;
	genlabs = dfld->tlabs;

	/* Build time buffers */
	keytimes   = INITMEM(double, NumKey);
	tweentimes = INITMEM(double, NumTween);
	for (ikey=0; ikey<NumKey; ikey++)
		{
		keytimes[ikey] = KeyPlus[ikey];
		}
	mfirst = TimeList[first_depict_time()].mplus;
	for (itween=0; itween<NumTween; itween++)
		{
		tweentimes[itween] = mfirst + itween*DTween;
		}

	/* Set start and end times from active frames */
	smplus = KeyPlus[0];
	emplus = KeyPlus[NumKey-1];

	/* Allocate output fields and replicate background and presentation */
	ent  = "c";
	elem = dfld->element;
	lev  = dfld->level;
	for (ikey=0, itween=0; itween<NumTween; itween++)
		{
		/* Get rid of old interpolations */
		genlist[itween] = destroy_field(genlist[itween]);
		genlabs[itween] = destroy_set(genlabs[itween]);

		/* Find keyframe which covers this inbetween frame */
		mplus = tweentimes[itween];
		if (mplus < smplus) continue;
		if (mplus > emplus) continue;
		while (ikey < NumKey-1)
			{
			if (mplus < KeyPlus[ikey+1]) break;
			ikey++;
			}

		/* Extract the set for that time */
		fld = KeyList[ikey];
		if (!fld)                         continue;
		if (fld->ftype != FtypeSet)       continue;
		keyset = fld->data.set;
		if (!same(keyset->type, "curve")) continue;

		/* Construct an empty copy to preserve background and presentation */
		genset = copy_set(keyset);
		empty_set(genset);
		genlist[itween] = create_field(ent, elem, lev);
		define_fld_data(genlist[itween], "set", (POINTER) genset);
		}

	/* Translate links into curve links */
	if (make_curve_links(dfld) <= 0)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_line() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_line() - F %s %s\n", dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		/* Free time lists */
		FREEMEM(keytimes);
		FREEMEM(tweentimes);
		FREEMEM(KeyTime);
		FREEMEM(KeyPlus);
		FREEMEM(KeyList);
		FREEMEM(KeyLabs);
		NumKey = 0;

		dfld->interp   = TRUE;
		dfld->intlab   = TRUE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		(void) save_links();
		busy_cursor(FALSE);
		return TRUE;
		}

#	ifdef DEBUG_INTERP_LINES
	(void) debug_curve_links();
#	endif /* DEBUG_INTERP_LINES */

	empty_blank();

	/* Process the time sequence for each independent curve link */
	/* Note that curve links on the same line have already been connected! */
	closed = FALSE;
	for (iclink=0, jlink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;

		/* Multiple links are associated with the first link found! */
		if (clink->icom != iclink) continue;

		interp_progress(dfld, jlink, nCLink, -1, -1);
		jlink++;
		skey = clink->skey;
		ekey = clink->ekey;

#		ifdef DEBUG_INTERP
		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, KeyPlus[skey]));
			(void) strcpy(vte, hour_minute_string(0, KeyPlus[ekey]));
			pr_status("Interp",
				"Processing Curve Link: %d T%s(%d) to T%s(%d)\n",
				iclink, vts, skey, vte, ekey);
			}
		else
			pr_status("Interp",
				"Processing Curve Link: %d T%+d(%d) to T%+d(%d)\n",
				iclink, KeyPlus[skey]/60, skey, KeyPlus[ekey]/60, ekey);
#		endif /* DEBUG_INTERP */

		/* If no curves on link chain try next chain */
		if (ekey < skey)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Nothing to interpolate\n");
#			endif /* DEBUG_INTERP */

			continue;
			}

		/* Now we require interpolation */
		/* Adjust start and end times to legal interpolated times */
		nkey    = ekey - skey + 1;

		/* First calculate without start/end offsets */
		sfact   = ((double) (KeyPlus[skey] - mfirst)) / ((double) DTween);
		efact   = ((double) (KeyPlus[ekey] - mfirst)) / ((double) DTween);
		smplus  = mfirst + DTween * (int) ceil(sfact);
		emplus  = mfirst + DTween * (int) floor(efact);
		stween  = (smplus - mfirst)/DTween;
		etween  = (emplus - mfirst)/DTween;
		ntween  = etween - stween + 1;

		/* Now take early/late start/end into account */
		sfact   = ((double) (clink->splus - mfirst)) / ((double) DTween);
		efact   = ((double) (clink->eplus - mfirst)) / ((double) DTween);
		sxplus  = mfirst + DTween * (int) ceil(sfact);
		explus  = mfirst + DTween * (int) floor(efact);
		sxtween = (sxplus - mfirst)/DTween;
		extween = (explus - mfirst)/DTween;
		nxtween = extween - sxtween + 1;

		/* Determine full range of key frames and early/late start/end times */
		smin = MIN(stween, sxtween);
		emax = MAX(etween, extween);
		ntot = emax - smin + 1;

		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, sxplus));
			(void) strcpy(vte, hour_minute_string(0, explus));
			put_message("line-interp-mins",
						dfld->level, dfld->element, iclink, vts, vte);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%s to T%s\n", vts, vte);
#			endif /* DEBUG_INTERP */
			}
		else
			{
			put_message("line-interp",
						dfld->level, dfld->element, iclink,
						sxplus/60, explus/60);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%+d to T%+d\n", sxplus/60, explus/60);
#			endif /* DEBUG_INTERP */
			}

		/* Check for intermediate control nodes */
		ctrl  = clink->ctrl;
		ncont = ctrl->ncont;
		if (ctrl->lnum != NumTween)
			{
			pr_error("Interp.Curves",
				"Error with intermediate control nodes!\n");
			pr_error("Interp.Curves", "  NumTween: %d  lnum: %d\n",
					NumTween, ctrl->lnum);
			pr_error("Interp.Curves", "Contact system adminstrator!\n");
			ncont = 0;
			}

		/* If there is only one active frame on the link chain, */
		/* and if there are no intermediate control nodes, then */
		/* duplicate the original curve across the offset range */
		if (nkey <= 1 && ncont <= 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Only one frame to interpolate\n");
#			endif /* DEBUG_INTERP */

			/* Extract the curve */
			cikey = clink->key + skey;
			curv  = cikey->curv;

			/* See if there are any labels on this line */
			if (NotNull(KeyLabs) && NotNull(KeyLabs[skey]))
				{
				for (imem=0; imem<KeyLabs[skey]->num; imem++)
					{
					spot = (SPOT) KeyLabs[skey]->list[imem];

					/* Does it belong to this curve? */
					c2 = closest_curve(KeyList[skey]->data.set, spot->anchor,
								NullFloat, NullPoint, NullInt);
					if (c2 == curv)
						{
						/* Add to list of labels to replicate */
						nsl++;
						slist = GETMEM(slist, SPOT, nsl);
						slist[nsl-1] = spot;
						}
					}
				}

			/* Duplicate the curve in the output data */
			for (itween=sxtween; itween<=extween; itween++)
				{
				/* Add it to the affected set */
				genset = genlist[itween]->data.set;
				wcurv  = copy_curve(curv);
				add_item_to_set(genset, (ITEM) wcurv);

				/* Replicate labels on this curve */
				if (nsl > 0)
					{
					if (IsNull(genlabs[itween]))
						{
						genlabs[itween] = create_set("spot");
						setup_set_presentation(genlabs[itween],
								elem, lev, "FPA");
						}
					for (isl=0; isl<nsl; isl++)
						{
						wspot = copy_spot(slist[isl]);
						add_item_to_set(genlabs[itween], (ITEM) wspot);
						}
					}
				}

			/* Display the curve just once and move on to next chain */
			if (show)
				{
				wcurv = copy_curve(curv);
				add_item_to_metafile(BlankMeta, "curve", "c", "", "",
									 (ITEM) wcurv);
				present_blank(TRUE);
				}
			continue;
			}

		/* If there is more than one curve on the chain, */
		/* or if there are intermeditate control nodes,  */
		/* then true interpolation is required!          */

		/* Arrange spatial interpolation on each segment for multiple links */
		nseg = clink->key[skey].nseg;
		/* >>> can nseg ever be less than 2? <<< */
		if (nseg < 2)
			{
			pr_error("Interp.Curves", "Two few line segments: %d\n", nseg);
			continue;
			}
		npseg = INITMEM(int,    nseg);
		dxseg = INITMEM(double, nseg);
		dyseg = INITMEM(double, nseg);
		ddseg = INITMEM(double, nseg);

		/* Find max and min number of points over the active part */
		nspts = 0;
		for (iseg=0; iseg<nseg; iseg++)
			{
			npmin = 0;
			npmax = 0;
			for (ikey=skey; ikey<=ekey; ikey++)
				{
				cikey = clink->key + ikey;
				line  = cikey->line;
				dss   = cikey->dspan[iseg];
				/* >>>>> march17
				dse   = (iseg < nseg-1)? cikey->dspan[iseg+1]: line->numpts - 1;
				<<<<< */
				if (iseg < nseg-1)
					dse = cikey->dspan[iseg+1];
				else
					dse = (cikey->flip)? 0: line->numpts-1;
				if (cikey->flip)
					{
					ips = dss;
					/* >>>>> replaced this ...
					ipe = dse + 1.0;
					<<<<< */
					ipe = (iseg < nseg-1)? dse + 1.0: dse;
					if (ipe >= line->numpts) ipe = line->numpts-1;
					/* >>>>> ... by above <<<<< */
					np  = ips - ipe + 2;
					/* >>>>> testing
					<<<<< */
					if (iseg < nseg-1) np++;
					/* >>>>> testing <<<<< */
					}
				else
					{
					/* >>>>> replaced this ...
					ips = dss + 1.0;
					<<<<< */
					ips = (iseg > 0)? dss + 1.0: dss;
					if (ips >= line->numpts) ips = line->numpts-1;
					/* >>>>> ... by above <<<<< */
					ipe = dse;
					np  = ipe - ips + 2;
					/* >>>>> testing
					<<<<< */
					if (iseg < nseg-1) np++;
					/* >>>>> testing <<<<< */
					}

#				ifdef DEBUG_INTERP
				if (cikey->flip)
					pr_diag("Interp",
						"   Segment/Key: %d %d  Flip ipe/ips: %d %d  Spline points: %d\n",
							iseg, ikey, ipe, ips, np);
				else
					pr_diag("Interp",
						"   Segment/Key: %d %d  NoFlip - ips/ipe: %d %d  Spline points: %d\n",
							iseg, ikey, ips, ipe, np);
#				endif /* DEBUG_INTERP */

				np = MAX(np, 2);	/* Pretend there are at least 2 points */
				if ((ikey == skey) || (np < npmin)) npmin = np;
				if ((ikey == skey) || (np > npmax)) npmax = np;
				}

			/* Determine number of points to do spatial spline */
			npseg[iseg] = NINT(W1*npmax + W2*npmin);
			/* >>>>> try this Mar 17 2003
			npseg[iseg] = MAX(npseg[iseg], 10);
			<<<<< */
			npseg[iseg] = MAX(npseg[iseg], 3);
			nspts += npseg[iseg];

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Segment: %d  Spatial spline points (min/max): %d (%d/%d)\n",
				iseg, npseg[iseg], npmin, npmax);
#			endif /* DEBUG_INTERP */
			}

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "   Total spatial spline points: %d\n", nspts);
#		endif /* DEBUG_INTERP */

		/* Find average distance between points for re-splining */
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			cikey = clink->key + ikey;
			line  = cikey->line;
			line_properties(line, NullLogical, NullLogical, NullFloat, &len);
			len /= (line->numpts - 1);
			if ((ikey == skey) || (len < minlen)) minlen = len;
			}

		/* Determine resolution for graphics pipe */
		res = minlen * 0.75;
		if      (res <  1.0) res = 1.0;
		else if (res < 10.0) res = NINT(res);
		else                 res = NINT(res/5) * 5.0;
		minres = res / 500.0;

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "   Graphics pipe filter resolution: %.3f\n", res);
#		endif /* DEBUG_INTERP */

		/* Allocate space for spatial and temporal spline points */
		kbufx  = INITMEM(double, nspts*NumKey);
		kbufy  = INITMEM(double, nspts*NumKey);
		tbufx  = INITMEM(double, nspts*NumTween);
		tbufy  = INITMEM(double, nspts*NumTween);
		keyx   = INITMEM(double *, nspts);
		keyy   = INITMEM(double *, nspts);
		tweenx = INITMEM(double *, nspts);
		tweeny = INITMEM(double *, nspts);
		for (ip=0; ip<nspts; ip++)
			{
			keyx[ip]   = kbufx + ip*NumKey;
			keyy[ip]   = kbufy + ip*NumKey;
			tweenx[ip] = tbufx + ip*NumTween;
			tweeny[ip] = tbufy + ip*NumTween;
			}

		/* Initialize all buffers */
		for (ikey=0; ikey<NumKey; ikey++)
			for (ip=0; ip<nspts; ip++)
				keyx[ip][ikey] = keyy[ip][ikey] = 0.0;
		for (itween=0; itween<NumTween; itween++)
			for (ip=0; ip<nspts; ip++)
				tweenx[ip][itween] = tweeny[ip][itween] = 0.0;

		/* Do spatial spline on segments of each curve in the time sequence */
		ipseg = 0;
		for (iseg=0; iseg<nseg; iseg++)
			{
			spatial(clink, iseg, npseg[iseg], skey, ekey,
						keyx+ipseg, keyy+ipseg);
			ipseg += npseg[iseg];
			}

		/* Replicate single frames to each inbetween frame */
		if (nkey <= 1)
			{
			for (itween=smin; itween<=emax; itween++)
				for (isp=0; isp<nspts; isp++)
					{
					tweenx[isp][itween] = keyx[isp][skey];
					tweeny[isp][itween] = keyy[isp][skey];
					}
			}

		/* Perform temporal spline on each splined point across */
		/* active key frame sequence, resulting in interpolated */
		/* positions for each inbetween frame                   */
		else
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Temporal spline key:%d:%d (%d) --> tween:%d:%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			/* First spline the interpolated points */
			/* Note that using Tween() can create "fishhook" effects */
			/*  when some links (but not all) are at end of a line   */
			for (isp=0; isp<nspts; isp++)
				{
				/* original ...
				Tween(nkey,keytimes+skey,keyx[isp]+skey,keyy[isp]+skey,ntween,
					  tweentimes+stween,tweenx[isp]+stween,tweeny[isp]+stween);
				... original */
				QuasiLinear_Tween(nkey, keytimes+skey, keyx[isp]+skey, keyy[isp]+skey, ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);

#				ifdef DEBUG_INTERP
				pr_diag("Interp", "     For point: %d  Keys: %d\n", isp, nkey);
				for (iky=0; iky<nkey; iky++)
					pr_diag("Interp", "       Key: %.2d  %.0f  %.2f %.2f\n",
							iky, keytimes[skey+iky],
							keyx[isp][skey+iky], keyy[isp][skey+iky]);
#				endif /* DEBUG_INTERP */
				}
			}

		/* Re-interpolate for intermediate control nodes */
		if (ncont > 0)
			{

			/* Re-allocate space for spatial spline points */
			keytimes2 = INITMEM(double, (nkey+ncont));
			kbufx2    = INITMEM(double, nspts*(nkey+ncont));
			kbufy2    = INITMEM(double, nspts*(nkey+ncont));
			keyx2     = INITMEM(double *, nspts);
			keyy2     = INITMEM(double *, nspts);
			for (ip=0; ip<nspts; ip++)
				{
				keyx2[ip] = kbufx2 + ip*(nkey+ncont);
				keyy2[ip] = kbufy2 + ip*(nkey+ncont);
				}

			/* Allocate space for adjustments */
			subpts = INITMEM(double, nspts);
			dxsubs = INITMEM(double, nspts);
			dysubs = INITMEM(double, nspts);
			dspts  = INITMEM(double, nspts);
			dxpts  = INITMEM(double, nspts);
			dypts  = INITMEM(double, nspts);

			/* Save points for all times with normal OR control links */
			cikey = clink->key + skey;
			for (icont=0, ikey=skey, inum=0; inum<ctrl->lnum; inum++)
				{
				mplus = ctrl->mplus[inum];

				/* Use existing points for existing lines */
				if (ikey <= ekey && mplus == KeyPlus[ikey])
					{
					keytimes2[icont] = keytimes[ikey];
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] = keyx[isp][ikey];
						keyy2[isp][icont] = keyy[isp][ikey];
						}
					icont++;
					ikey++;
					}

				/* Otherwise ... modify interpolated points for control nodes */
				else if (ctrl->cthere[inum])
					{
					keytimes2[icont] = mplus;

					/* Begin with interpolated points for this time */
					for (itween=0; itween<NumTween; itween++)
						{
						if (tweentimes[itween] == mplus) break;
						}
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] = tweenx[isp][itween];
						keyy2[isp][icont] = tweeny[isp][itween];
						}

#					ifdef DEBUG_INTERP_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in curve: %d  inum/itween: %d/%d\n",
						mplus, nspts, inum, itween);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Initial curve: %.3d %.2f %.2f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Initial curve: %.3d %.2f %.2f (%.2f %.2f %.2f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								keyx2[isp][icont]-keyx2[isp-1][icont],
								keyy2[isp][icont]-keyy2[isp-1][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_POINTS */

					/* Set modifications for each segment */
					ipseg = 0;
					for (iseg=0; iseg<nseg; iseg++)
						{
						dxseg[iseg] = dyseg[iseg] = ddseg[iseg] = 0.0;
						for (icnum=0; icnum<ctrl->lcnum[inum]; icnum++)
							{
							/* Match the link chain with the segment          */
							/* Note that ilink identifies link chains in      */
							/*  the order they were created - 0 1 2 ... etc   */
							/* Note that dseg identifies the original segment */
							/*  associated with each link - the first link    */
							/*  chain creates two segments, 0 and 1, with the */
							/*  link node associated with the first point in  */
							/*  segment 1, and the start of the line          */
							/*  associated with the first point in segment 0. */
							/*  Subsequent link chains can split these        */
							/*  segments, but each is given an original       */
							/*  segment identifier of 2 3 ... etc             */
							/* Note that the start of the line is not used,   */
							/*  so that links are matched with the segment    */
							/*  that is one greater than it                   */
							if (ctrl->ilink[inum][icnum] == cikey->dseg[iseg]-1)
								{
								dxseg[iseg] = ctrl->cnode[inum][icnum][X]
												- keyx2[ipseg][icont];
								dyseg[iseg] = ctrl->cnode[inum][icnum][Y]
												- keyy2[ipseg][icont];
								ddseg[iseg] = sqrt(dxseg[iseg]*dxseg[iseg]
												+ dyseg[iseg]*dyseg[iseg]);
								break;
								}
							}
						ipseg += npseg[iseg];
						}

#					ifdef DEBUG_INTERP_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Segments: %d\n",
						mplus, nseg);
					for (ipseg=npseg[0], iseg=1; iseg<nseg; iseg++)
						{
						pr_diag("Interp.Points",
							"    Point: %3d  Segment offset: %.2f %.2f\n",
							ipseg, dxseg[iseg], dyseg[iseg]);
						ipseg += npseg[iseg];
						}
#					endif /* DEBUG_INTERP_POINTS */

					/* Determine adjustments for all points                  */
					/*  using piecewise linear interpolation on each segment */
					for (isp=0; isp<nspts; isp++) dspts[isp] = (double) isp;
					for (ipseg=0, iseg=0; iseg<nseg; iseg++)
						{
						ips = (iseg > 0)? ipseg: 0;
						ipe = (iseg < nseg-1)? ipseg+npseg[iseg]-1: nspts-1;
						nsub = 2;
						subpts[0] = (double) ips;
						dxsubs[0] = (iseg > 0)? dxseg[iseg]: dxseg[iseg+1];
						dysubs[0] = (iseg > 0)? dyseg[iseg]: dyseg[iseg+1];
						subpts[1] = (double) ipe;
						dxsubs[1] = (iseg < nseg-1)? dxseg[iseg+1]: dxseg[iseg];
						dysubs[1] = (iseg < nseg-1)? dyseg[iseg+1]: dyseg[iseg];
						PieceWise_2D(nsub, subpts, dxsubs, dysubs,
								(ipe-ips+1), dspts+ips, dxpts+ips, dypts+ips);
						ipseg += npseg[iseg];

#						ifdef DEBUG_INTERP_POINTS
						pr_diag("Interp.Points",
							"    Segment: %d  Points: %.3d to %.3d  Offsets: %.2f %.2f to %.2f %.2f\n",
							iseg, ips, ipe, dxsubs[0], dysubs[0],
							dxsubs[1], dysubs[1]);
#						endif /* DEBUG_INTERP_POINTS */
						}

#					ifdef DEBUG_INTERP_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in curve: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						pr_diag("Interp.Points",
							"    Adjustments to curve: %.3d %.2f %.2f\n",
							isp, dxpts[isp], dypts[isp]);
						}
#					endif /* DEBUG_INTERP_POINTS */

					/* Apply the adjustments to the interpolated points */
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] += dxpts[isp];
						keyy2[isp][icont] += dypts[isp];
						}

#					ifdef DEBUG_INTERP_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in curve: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Final curve: %.3d %.2f %.2f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Final curve: %.3d %.2f %.2f (%.2f %.2f %.2f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								keyx2[isp][icont]-keyx2[isp-1][icont],
								keyy2[isp][icont]-keyy2[isp-1][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_POINTS */

					icont++;
					}
				}

			if (icont != nkey+ncont)
				{
				pr_error("Interp.Curves", "Too few link points: %d %d\n",
						icont, nkey+ncont);
				}

			/* Re-perform temporal spline on each splined point across */
			/* active key frame sequence, resulting in interpolated    */
			/* positions in each inbetween frame                       */

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Temporal spline key:%d:%d (%d) --> tween:%d:%d (%d)\n",
				skey, ekey, nkey, smin, emax, ntot);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(nkey+ncont, keytimes2, keyx2[isp], keyy2[isp], ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);

#				ifdef DEBUG_INTERP
				pr_diag("Interp",
					"     For point: %d  Keys: %d\n", isp, nkey+ncont);
				for (iky=0; iky<nkey+ncont; iky++)
					pr_diag("Interp", "       Key: %.2d  %.0f  %.2f %.2f\n",
							iky, keytimes2[iky], keyx2[isp][iky], keyy2[isp][iky]);
#				endif /* DEBUG_INTERP */
				}
			}

		/* Generate new curve in each inbetween frame using the */
		/* new spatial spline coefficients                      */
		ikey   = 0;
		jkey   = skey;
		pkey   = -1;
		jtween = sxtween;
		ptween = -1;
		for (itween=sxtween; itween<=extween; itween++)
			{
			mplus = tweentimes[itween];

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				pr_diag("Interp", "      Generating curve in T%s (%d)\n",
					hour_minute_string(0, mplus), itween);
			else
				pr_diag("Interp", "      Generating curve in T%+.2d (%d)\n",
					mplus/60, itween);
#			endif /* DEBUG_INTERP */

			/* Get affected set */
			if (!genlist[itween])
				{
				(void) printf("!!!HELP!!!\n");
				continue;
				}
			genset = genlist[itween]->data.set;

			/* Set up spline operation */
			reset_pipe();
			enable_filter(res, 0.0);
			enable_spline(res, closed, 0.0, 0.0, 0.0);
			enable_save();

#			ifdef DEBUG_INTERP_POINTS
			pr_diag("Interp.Points", "  Points in curve: %d\n", nspts);
#			endif /* DEBUG_INTERP_POINTS */

			/* Re-spline splined line to desired resolution */
			/* Possibly line may be totally outside */
			for (isp=0; isp<nspts; isp++)
				{
				x = tweenx[isp][itween];
				y = tweeny[isp][itween];

				/* Condense line for duplicate points */
				if (isp > 0 &&
						fabs(x - tweenx[isp-1][itween]) < minres &&
						fabs(y - tweeny[isp-1][itween]) < minres) continue;

#				ifdef DEBUG_INTERP_POINTS
				if (isp==0)
					pr_diag("Interp.Points",
						"Interp curve: %.3d %.2f %.2f\n", isp, x, y);
				else
					pr_diag("Interp.Points",
						"Interp curve: %.3d %.2f %.2f (%.2f %.2f %.2f)\n",
						isp, x, y,
						x-tweenx[isp-1][itween], y-tweeny[isp-1][itween],
						hypot((double) x - tweenx[isp-1][itween],
								(double) y - tweeny[isp-1][itween]));
#				endif /* DEBUG_INTERP_POINTS */

				put_pipe(x, y);
				}
			flush_pipe();
			nlines = recall_save(&lines);
			if (nlines <= 0)
				{
				pr_warning("Interp.Curves", " No line after resplining!\n");
				continue;
				}
			if (nlines > 1)
				{
				pr_warning("Interp.Curves",
					" More than one line (%d) after resplining!\n", nlines);
				}

			while (ikey < NumKey-1)
				{
				if (mplus < KeyPlus[ikey+1]) break;
				ikey++;
				}
			cikey = clink->key + ikey;

			jkey  = MAX(ikey, skey);
			jkey  = MIN(jkey, ekey);
			cjkey = clink->key + jkey;

			line = copy_line(lines[0]);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", " Segment: %X Points: %d\n", line, line->numpts);
#			endif /* DEBUG_INTERP */

#			ifdef DEBUG_INTERP_POINTS
			for (isp=0; isp<line->numpts; isp++)
				{
				if (isp==0)
					pr_diag("Interp.Points",
						"Final curve: %.3d %.2f %.2f\n",
						isp, line->points[isp][X], line->points[isp][Y]);
				else
					pr_diag("Interp.Points",
						"Final curve: %.3d %.2f %.2f (%.2f %.2f %.2f)\n",
						isp, line->points[isp][X], line->points[isp][Y],
						line->points[isp][X]-line->points[isp-1][X],
						line->points[isp][Y]-line->points[isp-1][Y],
						hypot((double) (line->points[isp][X]-line->points[isp-1][X]),
								(double) (line->points[isp][Y]-line->points[isp-1][Y])));
				}
#			endif /* DEBUG_INTERP_POINTS */


			if (showtween || (show && KeyPlus[ikey]==mplus))
				{
				curv = create_curve("", "", "");
				copy_lspec(&curv->lspec, &cjkey->curv->lspec);
				add_line_to_curve(curv, line);
				add_item_to_metafile(BlankMeta, "curve", "c", "", "",
										(ITEM) curv);
				}

			curv = copy_curve(cjkey->curv);
			curv->sense = cjkey->sense;
			empty_curve(curv);
			add_line_to_curve(curv, line);
			add_item_to_set(genset, (ITEM) curv);
			line = destroy_line(line);

			/* See if original curve has labels on it */
			if (jkey != pkey)
				{
				FREEMEM(slist);
				nsl  = 0;
				pkey = jkey;

				if (IsNull(KeyLabs) || IsNull(KeyLabs[jkey])) continue;

				if (NotNull(KeyLabs) && NotNull(KeyLabs[jkey]))
					{
					for (imem=0; imem<KeyLabs[jkey]->num; imem++)
						{
						spot = (SPOT) KeyLabs[jkey]->list[imem];

						/* Does it belong to this curve? */
						c2 = closest_curve(KeyList[jkey]->data.set,
								spot->anchor, NullFloat, NullPoint, NullInt);
						if (c2 == cjkey->curv)
							{
							/* Add to list of labels to replicate */
							nsl++;
							slist = GETMEM(slist, SPOT, nsl);
							slist[nsl-1] = spot;
							}
						}
					}
				}

			/* Replicate labels on this curve */
			if (nsl > 0)
				{

				/* Calculate tween frame centroid */
				tbarx = 0;
				tbary = 0;
				for (isp=0; isp<nspts; isp++)
					{
					tbarx += tweenx[isp][itween];
					tbary += tweeny[isp][itween];
					}
				tbarx /= nspts;
				tbary /= nspts;

				/* Calculate key frame centroid to get average motion */
				/* Use the next key frame for early starts!           */
				jtween = (KeyPlus[ikey] - mfirst) / DTween;
				if (sxtween < stween)
					{
					jtween = (KeyPlus[ikey+1] - mfirst) / DTween;
					}

				/* No offset if labels are at a key frame */
				if (jtween == itween)
					{
					ptween = itween;
					kbarx  = tbarx;
					kbary  = tbary;
					xoff   = 0.0;
					yoff   = 0.0;

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"    No label offset for: %d (%d) of %d-%d\n",
						jtween, itween, sxtween, extween);
#					endif /* DEBUG_INTERP */

					}

				/* Determine offset if labels are not at a key frame */
				/* This will be early or late starts!                */
				else if (jtween != ptween)
					{
					ptween = jtween;
					kbarx  = 0;
					kbary  = 0;
					for (isp=0; isp<nspts; isp++)
						{
						kbarx += tweenx[isp][jtween];
						kbary += tweeny[isp][jtween];
						}
					kbarx /= nspts;
					kbary /= nspts;
					xoff   = tbarx - kbarx;
					yoff   = tbary - kbary;

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"    Determine label offset for: %d (%d) of %d-%d\n",
						jtween, itween, sxtween, extween);
					pr_diag("Interp",
						"     x/y offset: %f/%f  x/y: %f/%f\n",
						kbarx, kbary, tbarx, tbary);
#					endif /* DEBUG_INTERP */

					}

				/* Set offset if labels are not at a key frame */
				/* Key frame centroids have been set in cases above! */
				else
					{
					xoff   = tbarx - kbarx;
					yoff   = tbary - kbary;

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"    Set label offset for: %d (%d) of %d-%d\n",
						jtween, itween, sxtween, extween);
					pr_diag("Interp",
						"     x/y offset: %f/%f  x/y: %f/%f\n",
						kbarx, kbary, tbarx, tbary);
#					endif /* DEBUG_INTERP */

					}

				if (IsNull(genlabs[itween]))
					{
					genlabs[itween] = create_set("spot");
					setup_set_presentation(genlabs[itween], elem, lev, "FPA");
					}
				for (isl=0; isl<nsl; isl++)
					{
					wspot = copy_spot(slist[isl]);
					offset_item("spot", (ITEM) wspot, xoff, yoff);
					add_item_to_set(genlabs[itween], (ITEM) wspot);
					}
				}
			}

		FREEMEM(slist);
		nsl = 0;

		if (show) present_blank(TRUE);

		/* Free space for temporal spline parameters */
		FREEMEM(npseg);
		FREEMEM(dxseg);
		FREEMEM(dyseg);
		FREEMEM(ddseg);
		FREEMEM(kbufx);
		FREEMEM(kbufy);
		FREEMEM(tbufx);
		FREEMEM(tbufy);
		FREEMEM(keyx);
		FREEMEM(keyy);
		FREEMEM(tweenx);
		FREEMEM(tweeny);
		if (ncont > 0)
			{
			FREEMEM(keytimes2);
			FREEMEM(kbufx2);
			FREEMEM(kbufy2);
			FREEMEM(keyx2);
			FREEMEM(keyy2);
			FREEMEM(subpts);
			FREEMEM(dxsubs);
			FREEMEM(dysubs);
			FREEMEM(dspts);
			FREEMEM(dxpts);
			FREEMEM(dypts);
			}

		/* End of main loop (iclink) */
		}

	/* Free curve link buffers */
	(void) free_curve_links();

	/* Free time lists */
	FREEMEM(keytimes);
	FREEMEM(tweentimes);
	FREEMEM(KeyTime);
	FREEMEM(KeyPlus);
	FREEMEM(KeyList);
	FREEMEM(KeyLabs);
	NumKey = 0;

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In interp_line() - T %s %s\n", dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In interp_line() - F %s %s\n", dfld->element, dfld->level);
#	endif /* DEVELOPMENT */

	/* Restore graphics and set flags */
	dfld->interp   = TRUE;
	dfld->intlab   = TRUE;
	dfld->saved    = FALSE;
	dfld->reported = FALSE;
	link_status(dfld);
	(void) save_links();
	empty_blank();
	busy_cursor(FALSE);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s p a t i a l                                                     *
*    t e m p o r a l                                                   *
*                                                                      *
***********************************************************************/

static	void	spatial

	(
	CLINK   *clink,
	int     iseg,
	int     nspts,
	int     skey,
	int     ekey,
	double  **keyx,
	double  **keyy
	)

	{
	int     mplus, nresp;
	int     isp, ikey, lspts, np, nseg, ips, ipe;
	LINE    line, wline, spseg;
	LOGICAL closed, doseg;
	float   slen, length, spres, sprev, spmin, spmax, dss, dse;
	CLKEY	*cikey;

	/* Figure out what we're interpolating */
	doseg  = (LOGICAL) (iseg >= 0 && clink->key[skey].nseg > 0);
	closed = FALSE;

	/* Do spatial spline on each line in the time sequence */
	for (ikey=skey; ikey<=ekey; ikey++)
		{
		mplus = KeyPlus[ikey];
		cikey = clink->key + ikey;

		/* Special handling for segmented lines */
		if (doseg)
			{
			line  = cikey->line;
			np    = line->numpts;
			nseg  = cikey->nseg;
			wline = create_line();
			dss   = cikey->dspan[iseg];
			/* >>>>> march17
			dse   = (iseg < nseg-1)? cikey->dspan[iseg+1]: np-1;
			<<<<< */
			if (iseg < nseg-1)
				dse = cikey->dspan[iseg+1];
			else
				dse = (cikey->flip)? 0: np-1;
			if (cikey->flip)
				{
				ips = dss;
				/* >>>>> testing
				ipe = dse + 1.0;
				<<<<< */
				ipe = (iseg < nseg-1)? dse + 1.0: dse;
				add_point_to_line(wline, cikey->dspt[iseg]);
				append_line_pdir(wline, line, ipe, ips, FALSE);
				/* >>>>> testing
				<<<<< */
				if (iseg < nseg-1) add_point_to_line(wline, cikey->dspt[iseg+1]);
				/* >>>>> testing <<<<< */
				}
			else
				{
				/* >>>>> testing
				ips = dss + 1.0;
				<<<<< */
				ips = (iseg > 0)? dss + 1.0: dss;
				ipe = dse;
				add_point_to_line(wline, cikey->dspt[iseg]);
				append_line_pdir(wline, line, ips, ipe, TRUE);
				/* >>>>> testing
				<<<<< */
				if (iseg < nseg-1) add_point_to_line(wline, cikey->dspt[iseg+1]);
				/* >>>>> testing <<<<< */
				}
			}

		/* Handling for entire lines */
		else
			{
			line  = cikey->line;
			wline = copy_line(line);
			if (cikey->flip) reverse_line(wline);
			}

#		ifdef DEBUG_INTERP
		if (minutes_in_depictions())
			pr_diag("Interp", "      Processing keyframe T%s\n",
				hour_minute_string(0, mplus));
		else
			pr_diag("Interp", "      Processing keyframe T%+.2d\n", mplus/60);
#		endif /* DEBUG_INTERP */

		/* Replicate short segments */
		condense_line(wline);
		if (wline->numpts == 1)
			{

#			ifdef DEBUG_INTERP_POINTS
			pr_diag("Interp.Points",
				"   Working line: %d  length: 0.0  Position: %.2f %.2f\n",
				wline->numpts, wline->points[0][X], wline->points[0][Y]);
#			endif /* DEBUG_INTERP_POINTS */

			for (isp=0; isp<nspts; isp++)
				{
				keyx[isp][ikey] = wline->points[0][X];
				keyy[isp][ikey] = wline->points[0][Y];
				}
			wline = destroy_line(wline);
			continue;
			}

		/* Refit working line once to make sure it extrapolates */
		/* beyond edges */
		line_properties(wline, NULL, NULL, NULL, &length);
		/* >>>>> replace this ...
		spres = length / (wline->numpts - 1.1);
		<<<<< */
		spres = length / (wline->numpts - 1.005);
		/* >>>>> ... with above <<<<< */

#		ifdef DEBUG_INTERP_POINTS
		pr_diag("Interp.Points",
			"   Working line: %d  length: %.2f  spres: %.3f\n",
			wline->numpts, length, spres);
		pr_diag("Interp.Points", "   Resplined curve positions -\n");
		for (isp=0; isp<wline->numpts; isp++)
			pr_diag("Interp.Points", "    Position: %.3d  %.2f %.2f\n",
					isp, wline->points[isp][X], wline->points[isp][Y]);
#		endif /* DEBUG_INTERP_POINTS */

		reset_pipe();
		enable_filter(spres, 0.0);
		enable_spline(spres, closed, 0.0, 0.0, 0.0);
		enable_save();
		line_pipe(wline);
		nlines = recall_save(&lines);
		wline  = destroy_line(wline);
		wline  = copy_line(lines[0]);
		line_properties(wline, NULL, NULL, NULL, &slen);
		length = slen;
		sprev  = 0.0;
		spmin  = 0.0;
		spmax  = 0.0;

		/* Set up initial spline operation with resolution that */
		/* should generate the same number of points in each line */
		spseg = copy_line(wline);
		/* >>>>> replace this ...
		spres = length / (nspts - 1.1);
		<<<<< */
		spres = length / (nspts - 1.005);
		/* >>>>> ... with above <<<<< */
		nresp = 0;

#		ifdef DEBUG_INTERP_POINTS
		pr_diag("Interp.Points",
			"   Spline line:  %d  length: %.2f  spres: %.3f\n",
			nspts, length, spres);
#		endif /* DEBUG_INTERP_POINTS */

Respline:   /* Spline the line to the desired resolution */
		nresp++;
		reset_pipe();
		enable_filter(spres, 0.0);
		enable_spline(spres, closed, 0.0, 0.0, 0.0);
		enable_save();
		line_pipe(spseg);
		nlines = recall_save(&lines);
		spseg  = destroy_line(spseg);
		spseg  = copy_line(lines[0]);
		line_properties(spseg, NULL, NULL, NULL, &slen);
		lspts = spseg->numpts;

#		ifdef DEBUG_INTERP_POINTS
		pr_diag("Interp.Points",
			"   Resplined curve points: %d %d (ReSpline: %d)\n",
			lspts, nspts, nresp);
#		endif /* DEBUG_INTERP_POINTS */

		/* If we got too many points, increase resolution and respline */
		if (lspts > nspts)
			{
			/* Adjust the minimum resolution */
			if (spmin > 0) spmin = MAX(spmin, spres);
			else           spmin = spres;

			/* Compute a larger resolution */
			sprev  = spres;
			spres *= (float)(lspts-1) / (float)(nspts-1);

			/* Make sure we didn't jump too far the other way */
			if (spmax > 0)
				{
				if (spres >= spmax) spres = (sprev+spmax)/2;
				}

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"         Too many points %d (target %d)\n", lspts, nspts);
			pr_diag("Interp",
				"         Refit with resolution %f -> %f\n", sprev, spres);
#			endif /* DEBUG_INTERP */

			/* Start with a fresh copy of the working line */
			spseg = destroy_line(spseg);
			spseg = copy_line(wline);
			goto Respline;
			}

		/* If we got too few points, decrease resolution and respline */
		else if (lspts < nspts)
			{
			/* Adjust the maximum resolution */
			if (spmax > 0) spmax = MIN(spmax, spres);
			else           spmax = spres;

			/* Compute a smaller resolution */
			sprev  = spres;
			spres *= (float)(lspts-1) / (float)(nspts-1);

			/* Make sure we didn't jump too far the other way */
			if (spmin > 0)
				{
				if (spres <= spmin) spres = (sprev+spmin)/2;
				}

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"         Too few points %d (target %d)\n", lspts, nspts);
			pr_diag("Interp",
				"         Refit with resolution %f -> %f\n", sprev, spres);
#			endif /* DEBUG_INTERP */

			/* Start with a fresh copy of the working line */
			spseg = destroy_line(spseg);
			spseg = copy_line(wline);
			goto Respline;
			}

		/* If splined length differs significantly, replace working */
		/* line by splined line, adjust resolution and respline */
		else if (fabs(slen-length) >= spres)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"         Length bad %f (target %f)\n", slen, length);
			pr_diag("Interp", "         Refit new line\n");
#			endif /* DEBUG_INTERP */

			length = slen;
			lspts  = nspts;
			sprev  = 0.0;
			spmin  = 0.0;
			spmax  = 0.0;
			wline  = destroy_line(wline);
			wline  = copy_line(spseg);
			/* >>>>> replace this ...
			spres  = length / (nspts - 1.1);
			<<<<< */
			spres  = length / (nspts - 1.005);
			/* >>>>> ... with above <<<<< */
			goto Respline;
			}

		/* Copy splined line into working buffer */
		for (isp=0; isp<nspts; isp++)
			{
			keyx[isp][ikey] = spseg->points[isp][X];
			keyy[isp][ikey] = spseg->points[isp][Y];
			}

		/* Get rid of temporary segment till next frame */
		spseg = destroy_line(spseg);
		wline = destroy_line(wline);

#		ifdef DEBUG_INTERP_POINTS
		pr_diag("Interp.Points",
			"   Spatial curve points: %d\n", nspts);
		if (nspts > 0)
			{
			pr_diag("Interp.Points", "   Spatial curve positions -\n");
			for (isp=0; isp<nspts; isp++)
				if (isp==0)
					pr_diag("Interp.Points",
						"    Position: %.3d  %.2f %.2f\n",
						isp, keyx[isp][ikey], keyy[isp][ikey]);
				else
					pr_diag("Interp.Points",
						"    Position: %.3d  %.2f %.2f (%.2f %.2f %.2f)\n",
						isp, keyx[isp][ikey], keyy[isp][ikey],
						keyx[isp][ikey]-keyx[isp-1][ikey],
						keyy[isp][ikey]-keyy[isp-1][ikey],
						hypot(keyx[isp][ikey]-keyx[isp-1][ikey],
								keyy[isp][ikey]-keyy[isp-1][ikey]));
			}
#		endif /* DEBUG_INTERP_POINTS */

		/* End of spatial spline loop (ikey) */
		}
	}

/***********************************************************************
*                                                                      *
*    f r e e _ c u r v e _ l i n k s                                   *
*    m a k e _ c u r v e _ l i n k s                                   *
*                                                                      *
*    Construct curve link information.                                 *
*                                                                      *
***********************************************************************/

static	int	free_curve_links(void)

	{
	int		iclink, ikey;
	CLINK	*clink;
	CLKEY	*cikey;

	/* Empty curve link buffer */
	for (iclink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;
		if (clink)
			{
			if (NotNull(clink->key))
				{
				for (ikey=0; ikey<NumKey; ikey++)
					{
					cikey = clink->key + ikey;
					if (NotNull(cikey))
						{
						FREEMEM(cikey->dseg);
						FREEMEM(cikey->dspan);
						FREEMEM(cikey->dspt);
						}
					}
				}
			FREEMEM(clink->common);
			FREEMEM(clink->key);
			(void) free_control_list(clink->ctrl);
			}
		}
	FREEMEM(CLinks);
	nCLink = 0;

	return nCLink;
	}

/**********************************************************************/

static	int		make_curve_links

	(
	DFLIST	*dfld
	)

	{
	int		np, iclink, iblink, ilink, ikey, jkey, ivt, inode, icurv;
	FIELD	fld;
	SET		set;
	CLINK	*clink, *blink;
	CLKEY	*cikey, *cjkey, *bikey;
	LCHAIN	chain;
	CURVE	curve;
	LINE	line;
	LOGICAL	sym;
	LOGICAL	cfound;
	float	rel, dsl, dst, dspan, dsrel, length, slen;
	float	dss, dse, dbot, dtop;
	float	xa, ya, xb, yb, x, y;
	int		nseg, iseg, jseg, rseg, ispan, jspan;
	LOGICAL	extrev, first, orientation, atstart, rflip;
	HAND	csense;
	POINT	ipos, jpos;
	TSTAMP	vts, vte;

	/* Empty old curve link buffer */
	(void) free_curve_links();

	/* Allocate curve link buffer */
	nCLink = dfld->nchain;
	CLinks = INITMEM(CLINK, nCLink);

	/* Save curve link information in buffer */
	for (iclink=0; iclink<nCLink; iclink++)
		{

#		ifdef DEBUG_INTERP_LINKS
		pr_diag("make_curve_links()",
			"Interpolated link chain: %d  (%s %s)\n",
			iclink, dfld->element, dfld->level);
#		endif /* DEBUG_INTERP_LINKS */

		chain = dfld->chains[iclink];
		(void) interpolate_lchain(chain);

		clink = CLinks + iclink;
		clink->skey   = -1;
		clink->ekey   = -1;
		clink->splus  = chain->splus;
		clink->eplus  = chain->eplus;
		clink->icom   = iclink;
		clink->ncom   = 0;
		clink->common = NullInt;
		clink->key    = INITMEM(CLKEY, NumKey);
		clink->ctrl   = build_control_list(chain, 0);

		/* Build list of curves in the current link chain */
		for (ikey=0; ikey<NumKey; ikey++)
			{
			cikey = clink->key + ikey;
			ivt   = KeyTime[ikey];

			cikey->icurv = -1;
			cikey->link  = FALSE;
			cikey->curv  = NullCurve;
			cikey->line  = NullLine;
			cikey->nseg  = 0;
			cikey->dseg  = NullInt;
			cikey->dspan = NullFloat;
			cikey->dspt  = NullPointList;
			cikey->flip  = FALSE;
			cikey->sense = Right;
			set_point(cikey->lpos, -1.0, -1.0);

			/* >>>>> replace this ...
			if (!chain->nodes[ivt]->there) continue;
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, TimeList[ivt].mplus);
			if (inode < 0)                   continue;
			if (!chain->nodes[inode]->there) continue;

			/* Extract the set that corresponds to the current keyframe */
			fld = dfld->fields[ivt];
			if (!fld)                      continue;
			if (fld->ftype != FtypeSet)    continue;
			if (!fld->data.set)            continue;
			set = fld->data.set;
			if (!same(set->type, "curve")) continue;

			/* Extract the curve on the current link chain */
			icurv = chain->nodes[inode]->attach;
			if (icurv < 0) continue;
			curve = (CURVE) set->list[icurv];
			if (IsNull(curve))       continue;
			if (IsNull(curve->line)) continue;
			cikey->icurv = icurv;
			cikey->link  = TRUE;
			cikey->curv  = curve;
			cikey->line  = curve->line;
			copy_point(cikey->lpos, chain->nodes[inode]->node);

			/* Keep track of active part of link chain */
			if (clink->skey < 0) clink->skey = ikey;
			clink->ekey = ikey;
			}
		}

	/* Find out if any curves have multiple link chains */
	for (iclink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;

		/* Number the initial link chain (for control nodes) */
		ilink = 0;

		/* See if another chain has the same curve on it */
		for (iblink=iclink+1; iblink<nCLink; iblink++)
			{
			blink = CLinks + iblink;

			/* Already found? */
			if (blink->icom != iblink) continue;

			/* Check in each keyframe */
			/* Common if the line is shared in all co-existing keyframes */
			cfound = FALSE;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				cikey = clink->key + ikey;
				bikey = blink->key + ikey;
				if (cikey->icurv < 0) continue;
				if (bikey->icurv < 0) continue;

				if (cikey->icurv != bikey->icurv)
					{
					cfound = FALSE;
					break;
					}

				/* Disregard if they only co-exist at joined ends */
				if (ikey > 0 && ikey < NumKey-1)
					{
					if (    (cikey-1)->link && !(cikey+1)->link
						&& !(bikey-1)->link &&  (bikey+1)->link)
						{
						cfound = FALSE;
						break;
						}
					if (   !(cikey-1)->link &&  (cikey+1)->link
						&&  (bikey-1)->link && !(bikey+1)->link)
						{
						cfound = FALSE;
						break;
						}
					}

				cfound = TRUE;
				}

			/* Make note if common */
			if (cfound)
				{

				/* Identify this link as connected */
				blink->icom = clink->icom;

				/* Save the earliest start and latest end times */
				clink->skey = MIN(clink->skey, blink->skey);
				clink->ekey = MAX(clink->ekey, blink->ekey);
				clink->splus = MIN(clink->splus, blink->splus);
				clink->eplus = MAX(clink->eplus, blink->eplus);

				/* Save the control node information */
				/* Associate control nodes with the next link chain */
				ilink++;
				(void) add_nodes_to_control_list(clink->ctrl, ilink,
																blink->ctrl);

				/* Share line info over non-common frames to help */
				/* generate pseudo-links */
				for (ikey=0; ikey<NumKey; ikey++)
					{
					cikey = clink->key + ikey;
					bikey = blink->key + ikey;

					if (cikey->icurv < 0 && bikey->icurv >= 0)
						{
						cikey->link  = FALSE;
						cikey->icurv = bikey->icurv;
						cikey->curv  = bikey->curv;
						cikey->line  = bikey->line;
						}

					else if (bikey->icurv < 0 && cikey->icurv >= 0)
						{
						bikey->link  = FALSE;
						bikey->icurv = cikey->icurv;
						bikey->curv  = cikey->curv;
						bikey->line  = cikey->line;
						}
					}
				}
			}
		}

	/* Generate list of common-chain segments in each keyframe */
	/* Create pseudo-links where required */
	for (iclink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;

		/* Multiple links are associated with the first link found! */
		if (clink->icom  != iclink) continue;

		clink->ncom   = 0;
		clink->common = (int *)0;

		/* Determine orientation of line wrt first frame with an    */
		/*  asymmetric line style or pattern (if found)             */
		/* Otherwise, determine orientation of line wrt first frame */
		first   = TRUE;
		atstart = TRUE;
		csense  = Right;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			cikey = clink->key + ikey;

			if (cikey->link)
				{

				/* Determine location of link wrt end point of line */
				line = cikey->line;
				line_properties(line, NullLogical, NullLogical,
								NullFloat, &length);
				line_test_point(line, cikey->lpos, NullFloat,
								ipos, &ispan, NullChar, NullChar);
				dsl = point_dist(line->points[ispan], ipos);
				dst = point_dist(line->points[ispan],
								 line->points[ispan+1]);
				dspan = ispan + dsl/dst;
				slen  = line_slen(line, 0, dspan);

				/* See whether curve uses a symmetric line style or pattern */
				curve = cikey->curv;
				if (!get_pattern_info(curve->lspec.pattern, &sym)) sym = TRUE;

				/* Set the line orientation wrt first asymmetric line found */
				if (!sym)
					{
					jkey = ikey;
					if (curve->sense != Right) csense  = Left;
					if (slen > length/2.0)     atstart = FALSE;
					else                       atstart = TRUE;
					break;
					}

				/* Set the line orientation wrt end point of first line */
				if (first)
					{
					jkey = ikey;
					if (slen > length/2.0) atstart = FALSE;
					first = FALSE;
					}
				}
			}

#		ifdef DEBUG_INTERP_LINKS
		if (atstart)
			pr_diag("make_curve_links()",
				"Interpolated link chain: %d  csense: %c  atstart: T  frame: %d\n",
				iclink, csense, jkey);
		else
			pr_diag("make_curve_links()",
				"Interpolated link chain: %d  csense: %c  atstart: F  frame: %d\n",
				iclink, csense, jkey);
#		endif /* DEBUG_INTERP_LINKS */

		/* Initialize segments in each keyframe */
		extrev = FALSE;
		rel    = -1;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			cikey = clink->key + ikey;

			/* Determine orientation of line wrt chosen frame */
			cikey->flip  = FALSE;
			cikey->sense = csense;
			if (cikey->link)
				{

				/* First look for more than one link chain on line */
				line = cikey->line;
				orientation = FALSE;
				for (iblink=0; iblink<nCLink; iblink++)
					{
					if (iblink == iclink)      continue;
					blink = CLinks + iblink;
					if (blink->icom != iclink) continue;

					bikey = blink->key + ikey;
					if (!bikey->link)                 continue;
					if (bikey->icurv != cikey->icurv) continue;

					/* Found a matching link chain */
					/* Determine orientation based on link nodes */
					line_test_point(line, cikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);
					line_test_point(line, bikey->lpos, NullFloat,
									jpos, &jspan, NullChar, NullChar);
					if (ispan == jspan)
						{
						dsl = point_dist(line->points[ispan], ipos);
						dst = point_dist(line->points[jspan], jpos);
						}

					/* Change flip based on location wrt start of line */
					if (atstart)
						{
						if (ispan == jspan && dsl > dst) cikey->flip = TRUE;
						else if (ispan > jspan)          cikey->flip = TRUE;
						}
					else
						{
						if (ispan == jspan && dsl < dst) cikey->flip = TRUE;
						else if (ispan < jspan)          cikey->flip = TRUE;
						}
					orientation = TRUE;
					break;
					}

				/* Did not find a matching link chain */
				/* Determine orientation based on symmetry or location on line */
				if (!orientation)
					{

					/* See whether curve uses a symmetric line style or pattern */
					curve = cikey->curv;
					if (!get_pattern_info(curve->lspec.pattern, &sym)) sym = TRUE;

					/* Determine location of link wrt end point */
					line_properties(line, NullLogical, NullLogical,
									NullFloat, &length);
					line_test_point(line, cikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);
					dsl = point_dist(line->points[ispan], ipos);
					dst = point_dist(line->points[ispan],
									 line->points[ispan+1]);
					dspan = ispan + dsl/dst;
					slen  = line_slen(line, 0, dspan);

					/* Reset flip for asymmetric lines based on curve sense */
					if (!sym)
						{
						if ((csense == Right && curve->sense != Right)
								|| (csense != Right && curve->sense == Right))
							{
							cikey->flip = TRUE;
							}
						}

					/* Reset flip based on location on line */
					else if ((atstart && slen > length/2.0)
							|| (!atstart && slen <= length/2.0))
						{
						cikey->flip = TRUE;
						}
					}
				}

			/* Define 2 segments for the whole line, the first time */
			cikey->nseg  = 2;
			cikey->dseg  = GETMEM(cikey->dseg,  int,   2);
			cikey->dspan = GETMEM(cikey->dspan, float, 2);
			cikey->dspt  = GETMEM(cikey->dspt,  POINT, 2);

			if (cikey->link)
				{
				/* Define first segment from start of line to first link node */
				/* and second segment from link node to end of line */
				line = cikey->line;
				np   = line->numpts;
				line_test_point(line, cikey->lpos, NullFloat,
						ipos, &ispan, NullChar, NullChar);
				dsl = point_dist(line->points[ispan], ipos);
				dst = point_dist(line->points[ispan], line->points[ispan+1]);
				dspan = ispan + dsl/dst;
				if (cikey->flip)
					{
					cikey->dseg[0]  = 0;
					cikey->dspan[0] = np-1;
					copy_point(cikey->dspt[0], line->points[np-1]);
					cikey->dseg[1]  = 1;
					cikey->dspan[1] = dspan;
					copy_point(cikey->dspt[1], cikey->lpos);
					rel = (np-1 - dspan) / (np-1);
					}
				else
					{
					cikey->dseg[0]  = 0;
					cikey->dspan[0] = 0;
					copy_point(cikey->dspt[0], line->points[0]);
					cikey->dseg[1]  = 1;
					cikey->dspan[1] = dspan;
					copy_point(cikey->dspt[1], cikey->lpos);
					rel = dspan / (np-1);
					}

				/* Extend pseudo-links back in time if required */
				/* (First active node is not at the first time) */
				if (!extrev)
					{
					for (jkey=0; jkey<ikey; jkey++)
						{
						cjkey = clink->key + jkey;
						if (cjkey->icurv < 0)    continue;
						if (IsNull(cjkey->line)) continue;
						line  = cjkey->line;
						np    = line->numpts;
						dspan = (np-1)*rel;
						jspan = dspan;
						dsrel = dspan - jspan;
						/*>>>*/
						xa = line->points[jspan][X];
						xb = line->points[jspan+1][X];
						ya = line->points[jspan][Y];
						yb = line->points[jspan+1][Y];
						x  = xa + dsrel*(xb-xa);
						y  = ya + dsrel*(yb-ya);
						if (cjkey->flip)
							{
							cjkey->dseg[0]  = 0;
							cjkey->dspan[0] = np-1;
							copy_point(cjkey->dspt[0], line->points[np-1]);
							cjkey->dseg[1]  = 1;
							cjkey->dspan[1] = dspan;
							set_point(cjkey->dspt[1], x, y);
							}
						else
							{
							cjkey->dseg[0]  = 0;
							cjkey->dspan[0] = 0;
							copy_point(cjkey->dspt[0], line->points[0]);
							cjkey->dseg[1]  = 1;
							cjkey->dspan[1] = dspan;
							set_point(cjkey->dspt[1], x, y);
							}
						}
					extrev = TRUE;
					}
				}

			/* Extend pseudo-links forward in time if required */
			/* (Current is not active, but follows an active one) */
			else if (rel >= 0)
				{
				if (cikey->icurv < 0)    continue;
				if (IsNull(cikey->line)) continue;
				line  = cikey->line;
				np    = line->numpts;
				dspan = (np-1)*rel;
				ispan = dspan;
				dsrel = dspan - ispan;
				xa = line->points[ispan][X];
				xb = line->points[ispan+1][X];
				ya = line->points[ispan][Y];
				yb = line->points[ispan+1][Y];
				x  = xa + dsrel*(xb-xa);
				y  = ya + dsrel*(yb-ya);
				if (cikey->flip)
					{
					cikey->dseg[0]  = 0;
					cikey->dspan[0] = np-1;
					copy_point(cikey->dspt[0], line->points[np-1]);
					cikey->dseg[1]  = 1;
					cikey->dspan[1] = dspan;
					set_point(cikey->dspt[1], x, y);
					}
				else
					{
					cikey->dseg[0]  = 0;
					cikey->dspan[0] = 0;
					copy_point(cikey->dspt[0], line->points[0]);
					cikey->dseg[1]  = 1;
					cikey->dspan[1] = dspan;
					set_point(cikey->dspt[1], x, y);
					}
				}
			}

		/* Find any common chains */
		for (iblink=0; iblink<nCLink; iblink++)
			{
			if (iblink == iclink)       continue;
			blink = CLinks + iblink;
			if (blink->icom  != iclink) continue;

			clink->ncom++;
			clink->common = GETMEM(clink->common, int, clink->ncom);
			clink->common[clink->ncom-1] = iblink;

			/* Divide segments in each keyframe with new nodes */
			extrev = FALSE;
			rel    = -1;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				cikey = clink->key + ikey;
				bikey = blink->key + ikey;

				if (bikey->link)
					{
					if (bikey->icurv != cikey->icurv) continue;

					/* Find which segment contains this node */
					/* so we can divide it */
					line = cikey->line;
					np   = line->numpts;
					line_test_point(line, bikey->lpos, NullFloat, ipos,
								&ispan, NullChar, NullChar);
					if (ispan >= np-1)
						{
						dspan = np-1;
						}
					else
						{
						dsl = point_dist(line->points[ispan], ipos);
						dst = point_dist(line->points[ispan],
									line->points[ispan+1]);
						dspan = ispan + dsl/dst;
						}
					nseg = cikey->nseg;
					for (iseg=0; iseg<nseg-1; iseg++)
						{
						dss = cikey->dspan[iseg];
						/* >>>>> march17
						dse = (iseg < nseg-1)? cikey->dspan[iseg+1]: np-1;
						<<<<< */
						if (iseg < nseg-1)
							dse = cikey->dspan[iseg+1];
						else
							dse = (cikey->flip)? 0: np-1;
						if (cikey->flip)
							{
							if (dse <= dspan && dspan < dss) break;
							}
						else
							{
							if (dss <= dspan && dspan < dse) break;
							}
						}

					/* Break the segment that contains it (dspan[iseg]) */
					cikey->nseg++;
					nseg = cikey->nseg;
					cikey->dseg  = GETMEM(cikey->dseg,  int,   nseg);
					cikey->dspan = GETMEM(cikey->dspan, float, nseg);
					cikey->dspt  = GETMEM(cikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>iseg; jseg--)
						{
						cikey->dseg[jseg+1]  = cikey->dseg[jseg];
						cikey->dspan[jseg+1] = cikey->dspan[jseg];
						copy_point(cikey->dspt[jseg+1], cikey->dspt[jseg]);
						}
					cikey->dseg[iseg+1]  = nseg-1;
					cikey->dspan[iseg+1] = dspan;
					copy_point(cikey->dspt[iseg+1], bikey->lpos);
					dss = cikey->dspan[iseg];
					/* >>>>> march17
					dse = (iseg < nseg-2)? cikey->dspan[iseg+2]: np-1;
					<<<<< */
					if (iseg < nseg-2)
						dse = cikey->dspan[iseg+2];
					else
						dse = (cikey->flip)? 0: np-1;
					if (cikey->flip)
						{
						dtop = dspan - dse;
						dbot = dss - dse;
						}
					else
						{
						dtop = dspan - dss;
						dbot = dse - dss;
						}
					if (dbot <= 0) rel = 0.0;
					else           rel = dtop / dbot;
					rseg  = iseg;
					rflip = cikey->flip;

					/* Extend pseudo-links back in time if required */
					if (!extrev)
						{
						for (jkey=0; jkey<ikey; jkey++)
							{
							cjkey = clink->key + jkey;
							if (cjkey->icurv < 0)    continue;
							if (IsNull(cjkey->line)) continue;
							line  = cjkey->line;
							np    = line->numpts;
							cjkey->nseg++;
							nseg = cjkey->nseg;
							cjkey->dseg  = GETMEM(cjkey->dseg,  int,   nseg);
							cjkey->dspan = GETMEM(cjkey->dspan, float, nseg);
							cjkey->dspt  = GETMEM(cjkey->dspt,  POINT, nseg);
							for (jseg=nseg-2; jseg>rseg; jseg--)
								{
								cjkey->dseg[jseg+1]  = cjkey->dseg[jseg];
								cjkey->dspan[jseg+1] = cjkey->dspan[jseg];
								copy_point(cjkey->dspt[jseg+1],
											cjkey->dspt[jseg]);
								}
							dss = cjkey->dspan[rseg];
							/* >>>>> march17
							dse = (rseg < nseg-2)? cjkey->dspan[rseg+2]: np-1;
							<<<<< */
							if (rseg < nseg-2)
								dse = cjkey->dspan[rseg+2];
							else
								dse = (cjkey->flip)? 0: np-1;
							if (cjkey->flip)
								{
								dbot  = dss - dse;
								if (rflip) dspan = dbot*rel + dse;
								else       dspan = dbot*(1-rel) + dse;
								}
							else
								{
								dbot  = dse - dss;
								if (rflip) dspan = dbot*(1-rel) + dss;
								else       dspan = dbot*rel + dss;
								}
							if (dspan > np-1) dspan = np-1;
							jspan = dspan;
							dsrel = dspan - jspan;
							xa = line->points[jspan][X];
							xb = line->points[jspan+1][X];
							ya = line->points[jspan][Y];
							yb = line->points[jspan+1][Y];
							x  = xa + dsrel*(xb-xa);
							y  = ya + dsrel*(yb-ya);
							cjkey->dseg[rseg+1]  = nseg-1;
							cjkey->dspan[rseg+1] = dspan;
							set_point(cjkey->dspt[rseg+1], x, y);
							}
						extrev = TRUE;
						}
					}

				/* Extend pseudo-links forward in time if required */
				/* come here from continues...  */
				else if (rel >= 0)
					{
					if (cikey->icurv < 0)    continue;
					if (IsNull(cikey->line)) continue;
					line  = cikey->line;
					np    = line->numpts;
					cikey->nseg++;
					nseg = cikey->nseg;
					cikey->dseg  = GETMEM(cikey->dseg,  int,   nseg);
					cikey->dspan = GETMEM(cikey->dspan, float, nseg);
					cikey->dspt  = GETMEM(cikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>rseg; jseg--)
						{
						cikey->dseg[jseg+1]  = cikey->dseg[jseg];
						cikey->dspan[jseg+1] = cikey->dspan[jseg];
						copy_point(cikey->dspt[jseg+1], cikey->dspt[jseg]);
						}
					dss = cikey->dspan[rseg];
					/* >>>>> march17
					dse = (rseg < nseg-2)? cikey->dspan[rseg+2]: np-1;
					<<<<< */
					if (rseg < nseg-2)
						dse = cikey->dspan[rseg+2];
					else
						dse = (cikey->flip)? 0: np-1;
					if (cikey->flip)
						{
						dbot  = dss - dse;
						if (rflip) dspan = dbot*rel + dse;
						else       dspan = dbot*(1-rel) + dse;
						}
					else
						{
						dbot  = dse - dss;
						if (rflip) dspan = dbot*(1-rel) + dss;
						else       dspan = dbot*rel + dss;
						}
					if (dspan > np-1) dspan = np-1;
					jspan = dspan;
					dsrel = dspan - jspan;
					xa = line->points[jspan][X];
					xb = line->points[jspan+1][X];
					ya = line->points[jspan][Y];
					yb = line->points[jspan+1][Y];
					x  = xa + dsrel*(xb-xa);
					y  = ya + dsrel*(yb-ya);
					cikey->dseg[rseg+1]  = nseg-1;
					cikey->dspan[rseg+1] = dspan;
					set_point(cikey->dspt[rseg+1], x, y);
					}
				}
			}
		}

	/* Check the curve links to ensure all segments are consistent */
	for (iclink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;

		/* Multiple links are associated with the first link found! */
		if (clink->icom  != iclink) continue;

		for (ikey=0; ikey<NumKey; ikey++)
			{
			cikey = clink->key + ikey;
			if (!cikey->link) continue;
			break;
			}
		if (ikey >= NumKey) continue;

		for (jkey=ikey+1; jkey<NumKey; jkey++)
			{
			cjkey = clink->key + jkey;
			if (!cjkey->link) continue;

			/* Check for consistent number of segments */
			if (cjkey->nseg != cikey->nseg)
				{
				if (minutes_in_depictions())
					{
					pr_warning("Interp.Curves",
						"Inconsistent number of curve links: %d:%d %d:%d\n",
						ikey, cikey->nseg, jkey, cjkey->nseg);
					(void) strcpy(vts, hour_minute_string(0, clink->splus));
					(void) strcpy(vte, hour_minute_string(0, clink->eplus));
					pr_warning("Interp.Curves",
						"  for link chain: %d  between: T%s and T%s\n",
						iclink, vts, vte);
					pr_warning("Interp.Curves",
						"May be problem with interpolation of %s %s\n",
						dfld->element, dfld->level);
					(void) strcpy(vts,
						hour_minute_string(0, KeyPlus[ikey]));
					(void) strcpy(vte,
						hour_minute_string(0, KeyPlus[jkey]));
					pr_warning("Interp.Curves",
						"  between: T%s and T%s\n", vts, vte);
					}
				else
					{
					pr_warning("Interp.Curves",
						"Inconsistent number of curve links: %d:%d %d:%d\n",
						ikey, cikey->nseg, jkey, cjkey->nseg);
					pr_warning("Interp.Curves",
						"  for link chain: %d  between: T%d and T%d\n",
						iclink, clink->splus/60, clink->eplus/60);
					pr_warning("Interp.Curves",
						"May be problem with interpolation of %s %s\n",
						dfld->element, dfld->level);
					pr_warning("Interp.Curves",
						"  between: T%d and T%d\n",
						KeyPlus[ikey]/60, KeyPlus[jkey]/60);
					}
				continue;
				}

			/* Check for consistent order of segments */
			for (iseg=0; iseg<cikey->nseg; iseg++)
				{
				if (cjkey->dseg[iseg] != cikey->dseg[iseg])
					{
					if (minutes_in_depictions())
						{
						pr_warning("Interp.Curves",
							"Inconsistent curve link order: %d:%d %d:%d\n",
							ikey, cikey->dseg[iseg], jkey, cjkey->dseg[iseg]);
						(void) strcpy(vts, hour_minute_string(0, clink->splus));
						(void) strcpy(vte, hour_minute_string(0, clink->eplus));
						pr_warning("Interp.Curves",
							"  for link chain: %d  between: T%s and T%s\n",
							iclink, vts, vte);
						pr_warning("Interp.Curves",
							"May be problem with interpolation of %s %s\n",
							dfld->element, dfld->level);
						(void) strcpy(vts,
							hour_minute_string(0, KeyPlus[ikey]));
						(void) strcpy(vte,
							hour_minute_string(0, KeyPlus[jkey]));
						pr_warning("Interp.Curves",
							"  between: T%s and T%s\n", vts, vte);
						}
					else
						{
						pr_warning("Interp.Curves",
							"Inconsistent curve link order: %d:%d %d:%d\n",
							ikey, cikey->dseg[iseg], jkey, cjkey->dseg[iseg]);
						pr_warning("Interp.Curves",
							"  for link chain: %d  between: T%d and T%d\n",
							iclink, clink->splus/60, clink->eplus/60);
						pr_warning("Interp.Curves",
							"May be problem with interpolation of %s %s\n",
							dfld->element, dfld->level);
						pr_warning("Interp.Curves",
							"  between: T%d and T%d\n",
							KeyPlus[ikey]/60, KeyPlus[jkey]/60);
						}
					}
				}
			}
		}

	return nCLink;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ c u r v e _ l i n k s                                 *
*                                                                      *
***********************************************************************/

static	void	debug_curve_links

	(
	)

	{
	int		iclink, ic, skey, ekey, ikey, npts, ipt, nseg, iseg;
	CLINK	*clink, *blink;
	CLKEY	*cikey, *bikey;
	CURVE	curv;

	/* Display curve link information */
	pr_diag("Interp.Curves", "\n");
	pr_diag("Interp.Curves", "Number of curve links: %d\n", nCLink);
	for (iclink=0; iclink<nCLink; iclink++)
		{
		clink = CLinks + iclink;
		pr_diag("Interp.Curves", "\n");
		pr_diag("Interp.Curves", "Curve link: %d   Associated with link: %d\n",
				iclink, clink->icom);
		if (clink->icom != iclink) continue;
		if (clink->ncom > 0)
			{
			for (ic=0; ic<clink->ncom; ic++)
				pr_diag("Interp.Curves", " Also uses curve link: %d\n",
						clink->common[ic]);
			}
		pr_diag("Interp.Curves", " Start frame/time: %d %d   End frame/time: %d %d\n",
				clink->skey, clink->splus, clink->ekey, clink->eplus);
		skey = clink->skey;
		ekey = clink->ekey;
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			cikey = clink->key + ikey;
			if (cikey->flip)
				pr_diag("Interp.Curves",
						"  Curve link frame/curve: %d %d  Flip: T  Sense: %c\n",
						ikey, cikey->icurv, cikey->sense);
			else
				pr_diag("Interp.Curves",
						"  Curve link frame/curve: %d %d  Flip: F  Sense: %c\n",
						ikey, cikey->icurv, cikey->sense);
			if (cikey->link)
				pr_diag("Interp.Curves",
						"   Primary Link Position: %.2f %.2f  on curve: %d  T\n",
						cikey->lpos[X], cikey->lpos[Y], cikey->icurv);
			else
				pr_diag("Interp.Curves",
						"   Primary Link Position: %.2f %.2f  on curve: %d  F\n",
						cikey->lpos[X], cikey->lpos[Y], cikey->icurv);
			if (clink->ncom > 0)
				{
				for (ic=0; ic<clink->ncom; ic++)
					{
					blink = CLinks + clink->common[ic];
					bikey = blink->key + ikey;
					if (bikey->link)
						pr_diag("Interp.Curves",
								"   Secondary Link Position: %.2f %.2f  on curve: %d  T\n",
								bikey->lpos[X], bikey->lpos[Y], bikey->icurv);
					else
						pr_diag("Interp.Curves",
								"   Secondary Link Position: %.2f %.2f  on curve: %d  F\n",
								bikey->lpos[X], bikey->lpos[Y], bikey->icurv);
					}
				}

			curv = cikey->curv;
			npts = curv->line->numpts;
			pr_diag("Interp.Curves", "   Curve points: %d\n", npts);
			if (npts > 0)
				{
				pr_diag("Interp.Curves", "   Curve positions -\n");
				for (ipt=0; ipt<npts; ipt++)
					pr_diag("Interp.Curves", "    Position: %.3d  %.2f %.2f\n",
							ipt, curv->line->points[ipt][X],
							curv->line->points[ipt][Y]);
				}

			nseg = cikey->nseg;
			pr_diag("Interp.Curves", "   Curve segments: %d\n", nseg);
			for (iseg=0; iseg<nseg; iseg++)
				pr_diag("Interp.Curves", "    Segment: %d  Dseg: %d  Dspan: %.3f  Position: %.2f %.2f\n",
						iseg, cikey->dseg[iseg], cikey->dspan[iseg],
						cikey->dspt[iseg][X], cikey->dspt[iseg][Y]);
			}
		}
	}
