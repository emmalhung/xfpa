/***********************************************************************
*                                                                      *
*     i n t e r p _ a r e a . c                                        *
*                                                                      *
*     Routines to time-interpolate area fields.                        *
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
#undef DEBUG_INTERP_AREAS
#undef DEBUG_INTERP_AREA_POINTS
#undef DEBUG_INTERP_DIV_POINTS
#undef DEBUG_INTERP_HOLE_POINTS
#undef DEBUG_INTERP_SPATIAL_POINTS
#undef DEBUG_INTERP_LABELS
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

/* Structures for keyframe info for area links */
typedef struct
		{
		int		iarea;		/* Which area? */
		AMEMBER	mtype;		/* Type of link                           */
							/*  (AreaNone/AreaBound/AreaDiv/AreaHole) */
		LOGICAL	link;		/* Is this a link? */
		int		imem;		/* Which dividing line or hole? */
		int		ndiv;		/* Number of dividing lines for area */
		int		*sids;		/* Subarea ids for each dividing line */
		int		nhole;		/* Number of holes for area */
		double	cfact;		/* Scale factor */
		double	mfact;		/* Scale factor for merging areas */
		double	sfact;		/* Scale factor for splitting areas */
		POINT	lpos;		/* Position of first link node */
		AREA	area;		/* Area being linked */
		LINE	line;		/* Boundary, dividing line or hole being linked */
		int		nseg;		/* Number of segments for linking */
		int		*dseg;		/* Segement associated with each link */
		float	*dspan;		/* Distance along span for each segment */
		POINT	*dspt;		/* Location of link node for each segment */
		LOGICAL	cw;			/* Is the area drawn clockwise? */
		LOGICAL	flip;		/* Has the dividing line been flipped? */
		LOGICAL	hcw;		/* Is the hole drawn clockwise? */
		STRING	lsub, lval, llab;	/* Labels to left of dividing line */
		STRING	rsub, rval, rlab;	/* Labels to right of dividing line */
		CAL		lcal, rcal;	/* Attributes to left/right of dividing line*/
		} ALKEY;

/* Structures for area links */
typedef struct
		{
		AMEMBER	ltype;			/* Type of link                           */
								/*  (AreaNone/AreaBound/AreaDiv/AreaHole) */
		int		skey, splus;	/* Start keyframe and time (in minutes) */
		int		ekey, eplus;	/* End keyframe and time (in minutes) */
		LOGICAL	mflag;			/* Does the chain link areas that merge? */
		LOGICAL	sflag;			/* Does the chain link an area that splits? */
		int		icom;			/* Identifier for this link */
		int		ncom;			/* Number of associated links */
		int		*common;		/* Identifiers for associated links */ 
		int		ocom;			/* Original identifier for this link */
		ALKEY	*key;			/* Key frame information */
		LCTRL	*ctrl;			/* Structure for control nodes */
		int		*iaout;			/* Original area for merge/split at each time */
		} ALINK;
static  int     nALink  = 0;
static  ALINK   *ALinks = NULL;

/* Structures for areas and dividing lines and holes */
typedef struct DIVINFO_struct
		{
		int		ndiv;		/* Number of dividing lines */
		ALKEY	**divkey;	/* Keyframe link info for each dividing line */
		LINE	*divlines;	/* Dividing lines */
		} *DIVINFO;
typedef struct HOLEINFO_struct
		{
		int		nhole;		/* Number of holes */
		ALKEY	**holekey;	/* Keyframe link info for each hole */
		LINE	*holes;		/* Holes */
		} *HOLEINFO;
typedef struct AREAINFO_struct
		{
		int			narea;		/* Number of areas */
		int			*iaout;		/* Original area (in order processed) */
		int			*ialink;	/* Initial link chain for each area */
		ALKEY		**areakey;	/* Keyframe link info for each area */
		ALKEY		**chckkey;	/* Keyframe link info for checking area link */
		DIVINFO 	*divinfo;	/* Dividing line information for each area */
		HOLEINFO	*holeinfo;	/* Hole information for each area */
		} *AREAINFO;

static  int     nlines;
static  LINE    *lines;


static	void	spatial(ALINK *, int, int, int, int, double **, double **);
static  int     free_area_links(void);
static  int     make_area_links(DFLIST *);
static  void    debug_area_links(void);
static  void    add_area_to_areainfo(AREAINFO, int, int, ALKEY *);
static  void    add_divline_to_divinfo(AREAINFO, int, ALKEY *, ALKEY *,
						ALKEY *, LINE);
static  void    add_hole_to_holeinfo(AREAINFO, int, ALKEY *, ALKEY *,
						ALKEY *, LINE);

/***********************************************************************
*                                                                      *
*    i n t e r p _ a r e a                                             *
*                                                                      *
*    Perform the time interpolation of area features from the working  *
*    depiction sequence, onto the given interval.                      *
*                                                                      *
*    The set of keyframe images is provided in dfld->fields.           *
*    The set of generated images is deposited in dfld->tweens.         *
*                                                                      *
***********************************************************************/

LOGICAL	interp_area

	(
	DFLIST	*dfld,
	LOGICAL	show,
	LOGICAL	showtween
	)

	{
	LOGICAL		cw, hcw;
	int			ivt, ialink, iblink, iarea, iaout, ilab, jlink, isub;
	int			jarea, jdiv, numdiv, idiv, xdiv, *sids, *xrem, *order;
	int			jh, ih;
	int			iorder, ixorder, norder, *aids, *axids, *aouts;
	LOGICAL		*asave, mflag, sflag;
	int			ikey, jkey, pkey, nkey;
	int			itween, jtween, ptween, ntween, nxtween;
	int			ip, np, npmin=0, npmax=0, isp;
	int			skey, stween, smplus, sxplus, sxtween;
	int			ekey, etween, emplus, explus, extween;
	int			ntot, smin, emax;
	TSTAMP		vts, vte;
	double		sfact, efact;
	int			nspts, ncont, icont, inum, icnum, nsub;
	int			mfirst, mplus;
	SET			keyset, keysetx, genset, tmpset;
	FIELD		*genlist, fld;
	SET			*genlabs;
	AREAINFO	*genareas, genarea;
	DIVINFO		divinfo;
	HOLEINFO	holeinfo;
	AREA		area, warea;
	SUBAREA		sa, lsa, rsa, dsub;
	SPOT		spot, wspot;
	LINE		line, wline, divl, hole;
	DIVSTAT		dstat;
	LOGICAL		resetnext, closed, toshow, issaved;
	int			dformat;
	STRING		ent, elem, lev;
	float		len, minlen=0.0, res;
	float		x, y, dss, dse;
	int			nseg, iseg, jseg, ips, ipe, ipseg;
	ALINK		*alink, *blink;
	ALKEY		*aikey, *ajkey, *bikey, *bjkey, *areakey, *divkey;
	LCHAIN		chain;
	LCTRL		*ctrl;

#	ifdef DEBUG_INTERP_DIV_POINTS
	int			ipt;
	char		xbuf[10], ybuf[240];
#	endif /* DEBUG_INTERP_DIV_POINTS */

#	ifdef DEBUG_INTERP_HOLE_POINTS
	int			jpt;
#	endif /* DEBUG_INTERP_HOLE_POINTS */

	SPOT		*slist = NullSpotList;
	int			nsl    = 0;
	int			isl;

	int			*npseg;
	double		*dxseg, *dyseg, *ddseg;
	double		*keytimes, *tweentimes;
	double		*kbufx, *tbufx, **keyx, **tweenx, kbarx, tbarx, xoff;
	double		*kbufy, *tbufy, **keyy, **tweeny, kbary, tbary, yoff;
	double		*keytimes2;
	double		*kbufx2, **keyx2;
	double		*kbufy2, **keyy2;
	double		*subpts, *dspts;
	double		*dxsubs, *dysubs, *dxpts, *dypts;

	/* See if we can or even need to interpolate */
	if (!dfld)          return FALSE;
	if (!dfld->dolink)  return FALSE;
	switch (dfld->editor)
		{
		case FpaC_DISCRETE: break;
		case FpaC_WIND:     break;
		default:        return FALSE;
		}
	if (NumTime < 2)    return FALSE;
	if (NumTween < 2)   return FALSE;
	if (!dfld->linked)  return FALSE;
	if (dfld->interp && dfld->intlab) return TRUE;
	if (!dfld->there)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_area() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_area() - F %s %s\n", dfld->element, dfld->level);
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
	ent  = "b";
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
		if (!fld)                        continue;
		if (fld->ftype != FtypeSet)      continue;
		keyset = fld->data.set;
		if (!same(keyset->type, "area")) continue;

		/* Construct an empty copy to preserve background and presentation */
		genset = copy_set(keyset);
		empty_set(genset);
		genlist[itween] = create_field(ent, elem, lev);
		define_fld_data(genlist[itween], "set", (POINTER) genset);
		}

	/* Translate links into area links */
	if (make_area_links(dfld) <= 0)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_area() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_area() - F %s %s\n", dfld->element, dfld->level);
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

	/* Allocate space for area ordering and dividing line information */
	genareas = INITMEM(AREAINFO, NumTween);
	for (itween=0; itween<NumTween; itween++)
		{
		genareas[itween] = INITMEM(struct AREAINFO_struct, 1);
		genareas[itween]->narea    = 0;
		genareas[itween]->iaout    = NullInt;
		genareas[itween]->ialink   = NullInt;
		genareas[itween]->areakey  = (ALKEY **) 0;
		genareas[itween]->chckkey  = (ALKEY **) 0;
		genareas[itween]->divinfo  = (DIVINFO *) 0;
		genareas[itween]->holeinfo = (HOLEINFO *) 0;
		}

#	ifdef DEBUG_INTERP_AREAS
	(void) debug_area_links();
#	endif /* DEBUG_INTERP_AREAS */

	empty_blank();

	/* Process the time sequence for each independent area boundary first */
	/* Note that links on the same boundary have already been connected! */
	jlink  = 0;
	closed = TRUE;
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only process area boundaries */
		if (alink->ltype != AreaBound) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "\n");
			pr_status("Interp",
				"Area Bound Link Chain %d processed with Chain %d\n",
				ialink, alink->icom);
#			endif /* DEBUG_INTERP */

			continue;
			}

		interp_progress(dfld, jlink, nALink+NumTween, -1, -1);
		jlink++;
		skey  = alink->skey;
		ekey  = alink->ekey;
		mflag = alink->mflag;
		sflag = alink->sflag;

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "\n");
		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, KeyPlus[skey]));
			(void) strcpy(vte, hour_minute_string(0, KeyPlus[ekey]));
			pr_status("Interp",
				"Processing Area Bound Link Chain %d T%s (%d) to T%s (%d)\n",
				ialink, vts, skey, vte, ekey);
			}
		else
			{
			pr_status("Interp",
				"Processing Area Bound Link Chain %d T%+d (%d) to T%+d (%d)\n",
				ialink, KeyPlus[skey]/60, skey, KeyPlus[ekey]/60, ekey);
			}
#		endif /* DEBUG_INTERP */

		/* If no areas on link chain try next chain */
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
		sfact   = ((double) (alink->splus - mfirst)) / ((double) DTween);
		efact   = ((double) (alink->eplus - mfirst)) / ((double) DTween);
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
			put_message("area-interp-mins",
						dfld->level, dfld->element, ialink, vts, vte);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%s to T%s\n", vts, vte);
#			endif /* DEBUG_INTERP */
			}
		else
			{
			put_message("area-interp",
						dfld->level, dfld->element, ialink,
						sxplus/60, explus/60);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%+d to T%+d\n", sxplus/60, explus/60);
#			endif /* DEBUG_INTERP */
			}

		/* Check for intermediate control nodes */
		ctrl  = alink->ctrl;
		ncont = ctrl->ncont;
		if (ctrl->lnum != NumTween)
			{
			pr_error("Interp.Areas",
				"Error with intermediate control nodes!\n");
			pr_error("Interp.Areas", "  NumTween: %d  lnum: %d\n",
				NumTween, ctrl->lnum);
			pr_error("Interp.Areas", "Contact system adminstrator!\n");
			ncont = 0;
			}

		/* If there is only one active frame on the link chain, */
		/* and if there are no intermediate control nodes, then */
		/* duplicate the original area across the offset range  */
		if (nkey <= 1 && ncont <= 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Only one frame to interpolate\n");
#			endif /* DEBUG_INTERP */

			/* Extract the area */
			aikey = alink->key + skey;
			area  = aikey->area;
			line  = aikey->line;

			/* See if there are any labels on this area */
			if (NotNull(KeyLabs) && NotNull(KeyLabs[skey]))
				{
				for (ilab=0; ilab<KeyLabs[skey]->num; ilab++)
					{
					spot = (SPOT) KeyLabs[skey]->list[ilab];

					/* Does it belong to this area? */
					(void) eval_areaset(KeyList[skey]->data.set, spot->anchor,
								PickFirst, &sa, NullCalPtr);
					isub = which_area_subarea(area, sa);
					if (isub >= 0)
						{
						/* Add to list of labels to replicate */
						nsl++;
						slist = GETMEM(slist, SPOT, nsl);
						slist[nsl-1] = spot;
						}
					}
				}

			/* Duplicate the area in the output data */
			for (itween=sxtween; itween<=extween; itween++)
				{
				mplus = tweentimes[itween];

				/* Use previous key frame for early starts */
				if (itween < stween)
					{
					ikey = 0;
					while (ikey < NumKey-1)
						{
						if (mplus < KeyPlus[ikey+1]) break;
						ikey++;
						}
					ajkey = alink->key + ikey;

#					ifdef DEBUG_INTERP
					pr_diag("Interp.Areas",
						"  Area key for early start: %X (%X)\n",
						ajkey, aikey);
#					endif /* DEBUG_INTERP */
					}
				else
					{
					ajkey = aikey;
					}

#				ifdef DEBUG_INTERP
				if (minutes_in_depictions())
					pr_diag("Interp",
						"      Duplicating area in T%s (%d)\n",
						hour_minute_string(0, mplus), itween);
				else
					pr_diag("Interp",
						"      Duplicating area in T%+.2d (%d)\n",
						mplus/60, itween);
#				endif /* DEBUG_INTERP */

				/* Make a copy of the single frame boundary */
				genset = genlist[itween]->data.set;
				warea  = copy_area(area, FALSE);
				empty_area(warea);
				wline  = copy_line(line);
				define_area_boundary(warea, wline);
				build_area_subareas(warea);

				/* Add it to the affected set */
				add_item_to_set(genset, (ITEM) warea);

				/* Remember which interpolated boundary was built */
				alink->iaout[itween] = genset->num - 1;

				/* Save the area info for ordering and dividing lines */
				add_area_to_areainfo(genareas[itween],
										alink->iaout[itween], ialink, ajkey);

				/* Replicate labels in this area */
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

			FREEMEM(slist);
			nsl = 0;

			/* Display the area just once and move on to next chain */
			if (show)
				{
				warea = copy_area(area, FALSE);
				dformat = field_display_format(CurrElement, CurrLevel);
				if (dformat = DisplayFormatComplex) prep_area_complex(warea);
				else                                prep_area(warea);
				add_item_to_metafile(BlankMeta, "area", "b", "", "",
									 (ITEM) warea);
#				ifdef EFF
					gxSetupTransform(DnMap);
					glClipMode(glCLIP_ON);
					display_area(warea);
					glClipMode(glCLIP_OFF);
					sync_display();
#				else
					present_blank(TRUE);
#				endif /* EFF */
				}

			/* End of section for single active frame on a link chain */
			/* Move on to next link chain                             */
			continue;
			}

		/* If there is more than one active frame on the chain, */
		/*  or if there are intermeditate control nodes,        */
		/*  then true interpolation is required!                */

		/* Arrange spatial interpolation on each segment for multiple links */
		nseg = alink->key[skey].nseg;
		nseg = MAX(nseg, 1);
		npseg = INITMEM(int,    nseg);
		dxseg = INITMEM(double, nseg);
		dyseg = INITMEM(double, nseg);
		ddseg = INITMEM(double, nseg);

		/* Find max and min number of points over the active part */
		nspts = 0;
		for (iseg=0; iseg<nseg; iseg++)
			{
			for (ikey=skey; ikey<=ekey; ikey++)
				{
				aikey = alink->key + ikey;
				line  = aikey->line;
				jseg  = (iseg < nseg-1)? iseg + 1: 0;
				dss   = aikey->dspan[iseg];
				dse   = aikey->dspan[jseg];
				if (aikey->cw)
					{
					ips = dss + 1.0;
					ipe = dse;
					np  = ipe - ips + 2;
					if (dss >= dse) np += line->numpts;
					}
				else
					{
					ips = dss;
					ipe = dse + 1.0;
					np  = ips - ipe + 2;
					if (dse >= dss) np += line->numpts;
					}
				if ((ikey == skey) || (np < npmin)) npmin = np;
				if ((ikey == skey) || (np > npmax)) npmax = np;
				}

			/* Determine number of points to do spatial spline */
			npseg[iseg] = NINT(W1*npmax + W2*npmin);
			npseg[iseg] = MAX(npseg[iseg], 10);
			nspts += npseg[iseg];
			}

#		ifdef DEBUG_INTERP_AREA_POINTS
		pr_diag("Interp.Points", "   Spatial spline points: %d\n", nspts);
#		endif /* DEBUG_INTERP_AREA_POINTS */

		/* Find average distance between points for re-splining */
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			line  = aikey->line;
			line_properties(line, NullLogical, NullLogical, NullFloat, &len);
			len /= (line->numpts - 1);
			if ((ikey == skey) || (len < minlen)) minlen = len;
			}

		/* Determine resolution for graphics pipe */
		res = minlen * 0.75;
		if      (res <  1.0) res = 1.0;
		else if (res < 10.0) res = NINT(res);
		else                 res = NINT(res/5) * 5.0;

#		ifdef DEBUG_INTERP_AREA_POINTS
		pr_diag("Interp.Points",
			"   Graphics pipe filter resolution: %.3f\n", res);
#		endif /* DEBUG_INTERP_AREA_POINTS */

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

		/* Do spatial spline on each segment of each boundary in the */
		/* time sequence */
		for (ipseg=0, iseg=0; iseg<nseg; iseg++)
			{
			spatial(alink, iseg, npseg[iseg], skey, ekey,
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
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				/* <<< Ross comment ... Need to account for rotation? >>> */
				QuasiLinear_Tween(nkey, keytimes+skey, keyx[isp]+skey, keyy[isp]+skey, ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Re-interpolate for merge/split areas using interpolated nodes */
		/*  ... which should already include intermediate control nodes! */
		if (mflag || sflag)
			{

#			ifdef DEBUG_INTERP
			if (mflag)
				pr_diag("Interp", "   Processing for merging areas!\n");
			if (sflag)
				pr_diag("Interp", "   Processing for splitting areas!\n");
			pr_diag("Interp",
				"     Common links: %d  Segments: %d\n", alink->ncom, nseg);
#			endif /* DEBUG_INTERP */

			/* Allocate space for adjustments */
			subpts = INITMEM(double, nspts);
			dxsubs = INITMEM(double, nspts);
			dysubs = INITMEM(double, nspts);
			dspts  = INITMEM(double, nspts);
			dxpts  = INITMEM(double, nspts);
			dypts  = INITMEM(double, nspts);

			/* Determine interpolated points at each time */
			for (ikey=skey, itween=smin; itween<=emax; itween++)
				{

				/* Set the keyframe for this time */
				mplus = tweentimes[itween];
				if (ikey < ekey && mplus >= KeyPlus[ikey+1]) ikey++;
				aikey = alink->key + ikey;

#				ifdef DEBUG_INTERP
				if (minutes_in_depictions())
					pr_diag("Interp",
						"    Adjustments for time T%s (%d)  Keyframe: %d\n",
						hour_minute_string(0, mplus), itween, ikey);
				else
					pr_diag("Interp",
						"    Adjustments for time T%+.2d (%d)  Keyframe: %d\n",
						mplus/60, itween, ikey);
#				endif /* DEBUG_INTERP */

				/* Set modifications for each segment at this time */
				for (ipseg=0, iseg=0; iseg<nseg; iseg++)
					{
					dxseg[iseg] = dyseg[iseg] = ddseg[iseg] = 0.0;
					jseg = aikey->dseg[iseg];
					if (jseg == 0)
						{
						chain = dfld->chains[alink->ocom];

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"     Segment: %d  jseg: %d  Initial link chain: %d\n",
							iseg, jseg, alink->ocom);
#						endif /* DEBUG_INTERP */
						}
					else
						{
						blink = ALinks + alink->common[jseg-1];
						chain = dfld->chains[blink->ocom];

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"     Segment: %d  jseg: %d  Connected link chain: %d\n",
							iseg, jseg, blink->ocom);
#						endif /* DEBUG_INTERP */
						}

					/* Find matching interp node for this time */
					for (inum=0; inum<chain->inum; inum++)
						{
						if (chain->interps[inum]->mplus == mplus) break;
						}

					/* Determine adjustments to interpolated points */
					/*  if matching interp node for this time found */
					if (inum < chain->inum)
						{
						dxseg[iseg] = chain->interps[inum]->node[X]
										- tweenx[ipseg][itween];
						dyseg[iseg] = chain->interps[inum]->node[Y]
										- tweeny[ipseg][itween];
						ddseg[iseg] = sqrt(dxseg[iseg]*dxseg[iseg]
											+ dyseg[iseg]*dyseg[iseg]);
						}

					/* Go on to next segment */
					ipseg += npseg[iseg];
					}

#				ifdef DEBUG_INTERP
				pr_diag("Interp",
					"   Interpolated time (cnode): %d  Segments: %d\n",
					mplus, nseg);
				for (ipseg=0, iseg=0; iseg<nseg; iseg++)
					{
					pr_diag("Interp",
						"    Point: %3d  Segment offset: %.2f %.2f\n",
						ipseg, dxseg[iseg], dyseg[iseg]);
					ipseg += npseg[iseg];
					}
#				endif /* DEBUG_INTERP */

				/* With only one segment, apply the adjustments directly */
				/*  to the interpolated points                           */
				if ( nseg <= 1 )
					{
					for (isp=0; isp<nspts; isp++)
						{
						tweenx[isp][itween] += dxseg[0];
						tweeny[isp][itween] += dyseg[0];
						}
					}

				/* For more than one segment, extract a subset of   */
				/*  adjustments to apply to the interpolated points */
				else
					{

					/* Extract subset of adjustments based on link nodes */
					for (nsub=0, ipseg=0, iseg=0; iseg<nseg; iseg++)
						{

						subpts[nsub] = (double) ipseg;
						dxsubs[nsub] = dxseg[iseg];
						dysubs[nsub] = dyseg[iseg];
						nsub++;

						ipseg += npseg[iseg];
						}

					/* Add adjustment for last point in last segment  */
					/*  ... the same as first point in first segment! */
					subpts[nsub] = (double) ipseg - 1.0;
					dxsubs[nsub] = dxsubs[0];
					dysubs[nsub] = dysubs[0];
					nsub++;

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"    Adjustments for %.2f to %.2f points\n",
						subpts[0], subpts[nsub-1]);
					for (isp=0; isp<nsub; isp++)
						{
						pr_diag("Interp",
							"     At point: %.2f  Adjustments: %.2f %.2f\n",
							subpts[isp], dxsubs[isp], dysubs[isp]);
						}
#					endif /* DEBUG_INTERP */

					/* Determine adjustments for all points  */
					/*  using piecewise linear interpolation */
					for (isp=0; isp<nspts; isp++) dspts[isp] = (double) isp;
					PieceWise_2D(nsub, subpts, dxsubs, dysubs,
							nspts, dspts, dxpts, dypts);

					/* Apply the adjustments to the interpolated points */
					for (isp=0; isp<nspts; isp++)
						{
						tweenx[isp][itween] += dxpts[isp];
						tweeny[isp][itween] += dypts[isp];
						}
					}
				}
			}

		/* Re-interpolate for intermediate control nodes */
		else if (ncont > 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Processing for %d control nodes!\n", ncont);
#			endif /* DEBUG_INTERP */

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
			aikey = alink->key + skey;
			for (icont=0, ikey=skey, inum=0; inum<ctrl->lnum; inum++)
				{
				mplus = ctrl->mplus[inum];

				/* Use existing points for existing areas */
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

					/* Begin with interpolated points for this time */
					keytimes2[icont] = mplus;
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] = tweenx[isp][inum];
						keyy2[isp][icont] = tweeny[isp][inum];
						}

#					ifdef DEBUG_INTERP_AREA_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in area boundary: %d\n",
						mplus, nspts);
					pr_diag("Interp.Points",
						"    Indexes ... alink->splus/sxtween: %d %d  alink->eplus/extween: %d %d  inum: %d\n",
						alink->splus, sxtween, alink->eplus, extween, inum);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Initial boundary: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Initial boundary: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_AREA_POINTS */

					/* Set modifications for each segment */
					for (ipseg=0, iseg=0; iseg<nseg; iseg++)
						{
						dxseg[iseg] = dyseg[iseg] = ddseg[iseg] = 0.0;
						for (icnum=0; icnum<ctrl->lcnum[inum]; icnum++)
							{
							/* Match the link chain with the segment          */
							/* Note that ilink identifies link chains in      */
							/*  the order they were created - 0 1 2 ... etc   */
							/* Note that dseg identifies the original segment */
							/*  associated with each link - the first link    */
							/*  chain is associated with the first and last   */
							/*  point in segment 0.  The second link chain    */
							/*  will split this segment, and thus become the  */
							/*  first point in segment 1 and the last point   */
							/*  in segment 0.  Subsequent link chains can     */
							/*  split these segments, but each is given an    */
							/*  original segment identifier of 2 3 ... etc    */
							if (ctrl->ilink[inum][icnum] == aikey->dseg[iseg])
								{

#								ifdef DEBUG_INTERP
								pr_diag("Interp",
									"  Matching control node: %d of (0-%d)\n",
									icnum, ctrl->lcnum[inum]-1);
								pr_diag("Interp",
									"    Segment: %d  Match: %d\n",
									iseg, aikey->dseg[iseg]);
#								endif /* DEBUG_INTERP */

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

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"   Interpolated time (cnode): %d  Segments: %d\n",
						mplus, nseg);
					for (ipseg=0, iseg=0; iseg<nseg; iseg++)
						{
						pr_diag("Interp",
							"    Point: %3d  Segment offset: %.2f %.2f\n",
							ipseg, dxseg[iseg], dyseg[iseg]);
						ipseg += npseg[iseg];
						}
#					endif /* DEBUG_INTERP */

					/* Determine adjustments for all interpolated points */

					/* With only one segment, apply the adjustments directly */
					/*  to the interpolated points                           */
					if ( nseg <= 1 )
						{
						for (isp=0; isp<nspts; isp++)
							{
							keyx2[isp][icont] += dxseg[0];
							keyy2[isp][icont] += dyseg[0];
							}
						}

					/* For more than one segment, extract a subset of   */
					/*  adjustments to apply to the interpolated points */
					else
						{

#						ifdef DEBUG_INTERP
						for (iseg=0; iseg<nseg; iseg++)
							{
							jseg  = (iseg < nseg-1)? iseg + 1: 0;
							dss   = aikey->dspan[iseg];
							dse   = aikey->dspan[jseg];
							if (aikey->cw)
								pr_diag("Interp",
									"    Segment positions (cw): %d %.2f %.2f\n",
									iseg, dss, dse);
							else
								pr_diag("Interp",
									"    Segment positions (ccw): %d %.2f %.2f\n",
									iseg, dss, dse);
							}
#						endif /* DEBUG_INTERP */

						/* Extract subset of adjustments based on link nodes */
						for (nsub=0, ipseg=0, iseg=0; iseg<nseg; iseg++)
							{

							subpts[nsub] = (double) ipseg;
							dxsubs[nsub] = dxseg[iseg];
							dysubs[nsub] = dyseg[iseg];
							nsub++;

							ipseg += npseg[iseg];
							}

						/* Add adjustment for last point in last segment  */
						/*  ... the same as first point in first segment! */
						subpts[nsub] = (double) ipseg - 1.0;
						dxsubs[nsub] = dxsubs[1];
						dysubs[nsub] = dysubs[1];
						nsub++;

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"    Adjustments for %.2f to %.2f points\n",
							subpts[0], subpts[nsub-1]);
						for (isp=0; isp<nsub; isp++)
							{
							pr_diag("Interp",
								"     At point: %.2f  Adjustments: %.2f %.2f\n",
								subpts[isp], dxsubs[isp], dysubs[isp]);
							}
#						endif /* DEBUG_INTERP */

						/* Determine adjustments for all points  */
						/*  using piecewise linear interpolation */
						for (isp=0; isp<nspts; isp++) dspts[isp] = (double) isp;
						PieceWise_2D(nsub, subpts, dxsubs, dysubs,
								nspts, dspts, dxpts, dypts);

#						ifdef DEBUG_INTERP_AREA_POINTS
						pr_diag("Interp.Points",
							"   Interpolated time (cnode): %d  Points in area boundary: %d\n",
							mplus, nspts);
						for (isp=0; isp<nspts; isp++)
							{
							pr_diag("Interp.Points",
								"    Adjustments to boundary: %.3d %.3f %.3f\n",
								isp, dxpts[isp], dypts[isp]);
							}
#						endif /* DEBUG_INTERP_AREA_POINTS */

						/* Apply the adjustments to the interpolated points */
						for (isp=0; isp<nspts; isp++)
							{
							keyx2[isp][icont] += dxpts[isp];
							keyy2[isp][icont] += dypts[isp];
							}
						}

#					ifdef DEBUG_INTERP_AREA_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in area boundary: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Final boundary: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Final boundary: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_AREA_POINTS */

					icont++;
					}
				}

			if (icont != nkey+ncont)
				{
				pr_error("Interp.Areas",
					"Too few link points: %d  Needed: %d\n",
					icont, nkey+ncont);
				pr_error("Interp.Areas", "Contact system adminstrator!\n");
				}

			/* Re-perform temporal spline on each splined point across */
			/* active key frame sequence, resulting in interpolated    */
			/* positions for each inbetween frame                      */

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(icont, keytimes2, keyx2[isp], keyy2[isp], ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Generate new area for each inbetween frame */
		/* using the new spatial spline coefficients  */
		toshow = FALSE;
		ikey   = 0;
		jkey   = skey;
		pkey   = -1;
		ptween = -1;
		for (itween=sxtween; itween<=extween; itween++)
			{
			mplus = tweentimes[itween];

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				pr_diag("Interp",
					"      Generating area in T%s (%d)\n",
					hour_minute_string(0, mplus), itween);
			else
				pr_diag("Interp",
					"      Generating area in T%+.2d (%d)\n",
					mplus/60, itween);
#			endif /* DEBUG_INTERP */

#			ifdef DEBUG_INTERP_AREA_POINTS
			pr_diag("Interp.Points",
				"   Points in area boundary: %d\n", nspts);
			for (isp=0; isp<nspts; isp++)
				{
				pr_diag("Interp.Points",
					"    Area boundary: %.3d %.3f %.3f\n",
					isp, tweenx[isp][itween], tweeny[isp][itween]);
				}
#			endif /* DEBUG_INTERP_AREA_POINTS */

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

			/* Re-spline splined line to desired resolution */
			/* Possibly area may be totally outside         */
			for (isp=0; isp<nspts; isp++)
				{
				x = tweenx[isp][itween];
				y = tweeny[isp][itween];
				put_pipe(x, y);
				}
			flush_pipe();
			nlines = recall_save(&lines);
			if (nlines <= 0)
				{
				pr_warning("Interp.Areas", " No area after resplining!\n");
				continue;
				}
			if (nlines > 1)
				{
				pr_warning("Interp.Areas",
					" More than one area (%d) after resplining!\n", nlines);
				}

			while (ikey < NumKey-1)
				{
				if (mplus < KeyPlus[ikey+1]) break;
				ikey++;
				}
			aikey = alink->key + ikey;
			jkey  = MAX(ikey, skey);
			jkey  = MIN(jkey, ekey);
			ajkey = alink->key + jkey;

			line  = lines[0];

			line_properties(line, NullLogical, &cw, NullFloat, NullFloat);
			if (ajkey->cw && !cw || !ajkey->cw && cw) reverse_line(line);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "      Area ikey/jkey: %d %d\n", ikey, jkey);
			if (ajkey->cw && !cw)
				pr_diag("Interp",
					"      Reverse interpolated boundary - Counterclockwise -> Clockwise\n");
			else if (!ajkey->cw && cw)
				pr_diag("Interp",
					"      Reverse interpolated boundary - Clockwise -> Counterclockwise\n");
#			endif /* DEBUG_INTERP */

#			ifdef DEBUG_INTERP_AREA_POINTS
			pr_diag("Interp.Points", "      Area points: %d\n", line->numpts);
#			endif /* DEBUG_INTERP_AREA_POINTS */

			if (showtween || (show && KeyPlus[ikey]==mplus))
				{
				toshow = TRUE;
				area   = create_area("", "", "");
				copy_lspec(&area->lspec, &ajkey->area->lspec);
				copy_fspec(&area->fspec, &ajkey->area->fspec);
				wline  = copy_line(line);
				define_area_boundary(area, wline);
				dformat = field_display_format(CurrElement, CurrLevel);
				if (dformat = DisplayFormatComplex) prep_area_complex(area);
				else                                prep_area(area);
				add_item_to_metafile(BlankMeta, "area", "b", "", "",
									 (ITEM) area);
#				ifdef EFF
					gxSetupTransform(DnMap);
					glClipMode(glCLIP_ON);
					display_area(area);
					glClipMode(glCLIP_OFF);
					sync_display();
#				endif /* EFF */
				}

			/* Make a copy of the interpolated boundary */
			area = copy_area(ajkey->area, FALSE);
			empty_area(area);
			wline = copy_line(line);
			define_area_boundary(area, wline);
			build_area_subareas(area);

			/* Add it to the affected set */
			add_item_to_set(genset, (ITEM) area);

			/* Remember which interpolated boundary was built */
			alink->iaout[itween] = genset->num - 1;

			/* Save the area info for ordering and dividing lines */
			add_area_to_areainfo(genareas[itween],
									alink->iaout[itween], ialink, aikey);

			/* See if original area has labels in it */
			if (jkey != pkey)
				{
				FREEMEM(slist);
				nsl  = 0;
				pkey = jkey;

				if (IsNull(KeyLabs) || IsNull(KeyLabs[jkey])) continue;

				if (NotNull(KeyLabs) && NotNull(KeyLabs[jkey]))
					{
					for (ilab=0; ilab<KeyLabs[jkey]->num; ilab++)
						{
						spot = (SPOT) KeyLabs[jkey]->list[ilab];

						/* Does it belong to this area? */
						(void) eval_areaset(KeyList[jkey]->data.set,
								spot->anchor, PickSmallest, &sa, NullCalPtr);
						isub = which_area_subarea(ajkey->area, sa);
						if (isub >= 0)
							{
							/* Add to list of labels to replicate */
							nsl++;
							slist = GETMEM(slist, SPOT, nsl);
							slist[nsl-1] = spot;
							}
						}
					}
				}

			/* Replicate labels in this area */
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
				/* Use starting key frame for early starts!           */
				jtween = (KeyPlus[ikey] - mfirst) / DTween;
				if (jtween < stween) jtween = stween;

				/* No offset if labels are at a key frame */
				if (jtween == itween)
					{
					ptween = itween;
					kbarx  = tbarx;
					kbary  = tbary;
					xoff   = 0.0;
					yoff   = 0.0;

#					ifdef DEBUG_INTERP_LABELS
					pr_diag("Interp.Labels",
						"    No label offset for: %d (from %d) of %d-%d\n",
						itween, jtween, sxtween, extween);
#					endif /* DEBUG_INTERP_LABELS */

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

#					ifdef DEBUG_INTERP_LABELS
					pr_diag("Interp.Labels",
						"    Determine label offset for: %d (from %d) of %d-%d\n",
						itween, jtween, sxtween, extween);
					pr_diag("Interp.Labels",
						"     x/y offset: %f/%f  x/y: %f/%f\n",
						kbarx, kbary, tbarx, tbary);
#					endif /* DEBUG_INTERP_LABELS */

					}

				/* Set offset if labels are not at a key frame */
				/* Key frame centroids have been set in cases above! */
				else
					{
					xoff   = tbarx - kbarx;
					yoff   = tbary - kbary;

#					ifdef DEBUG_INTERP_LABELS
					pr_diag("Interp.Labels",
						"    Set label offset for: %d (from %d) of %d-%d\n",
						itween, jtween, sxtween, extween);
					pr_diag("Interp.Labels",
						"     x/y offset: %f/%f  x/y: %f/%f\n",
						kbarx, kbary, tbarx, tbary);
#					endif /* DEBUG_INTERP_LABELS */

					}

				if (IsNull(genlabs[itween]))
					{
					genlabs[itween] = create_set("spot");
					setup_set_presentation(genlabs[itween], elem, lev, "FPA");
					}
				for (isl=0; isl<nsl; isl++)
					{
					wspot = copy_spot(slist[isl]);
					if (jtween != itween)
						offset_item("spot", (ITEM) wspot, xoff, yoff);
					add_item_to_set(genlabs[itween], (ITEM) wspot);
					}
				}
			}

		FREEMEM(slist);
		nsl = 0;

#		ifndef EFF
			if (toshow) present_blank(TRUE);
#		endif /* EFF */

		/* Free space for temporal spline parameters */
		FREEMEM(npseg);
		FREEMEM(dxseg);
		FREEMEM(dyseg);
		FREEMEM(ddseg);
		FREEMEM(tweenx);
		FREEMEM(tweeny);
		FREEMEM(keyx);
		FREEMEM(keyy);
		FREEMEM(tbufx);
		FREEMEM(tbufy);
		FREEMEM(kbufx);
		FREEMEM(kbufy);
		if (mflag || sflag)
			{
			FREEMEM(subpts);
			FREEMEM(dxsubs);
			FREEMEM(dysubs);
			FREEMEM(dspts);
			FREEMEM(dxpts);
			FREEMEM(dypts);
			}
		else if (ncont > 0)
			{
			FREEMEM(keytimes2);
			FREEMEM(keyx2);
			FREEMEM(keyy2);
			FREEMEM(kbufx2);
			FREEMEM(kbufy2);
			FREEMEM(subpts);
			FREEMEM(dxsubs);
			FREEMEM(dysubs);
			FREEMEM(dspts);
			FREEMEM(dxpts);
			FREEMEM(dypts);
			}

		/* End of boundary loop (ialink) */
		}

	/* Process the time sequence for all independent dividing lines now */
	/* Note that links on the same dividing line have already been connected! */
	closed = FALSE;
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only process dividing lines */
		if (alink->ltype != AreaDiv) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "\n");
			pr_status("Interp",
				"Area Div Link Chain %d processed with Chain %d\n",
				ialink, alink->icom);
#			endif /* DEBUG_INTERP */

			continue;
			}

		interp_progress(dfld, jlink, nALink+NumTween, -1, -1);
		jlink++;
		skey = alink->skey;
		ekey = alink->ekey;

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "\n");
		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, KeyPlus[skey]));
			(void) strcpy(vte, hour_minute_string(0, KeyPlus[ekey]));
			pr_status("Interp",
				"Processing Area Div Link Chain %d T%s (%d) to T%s (%d)\n",
				ialink, vts, skey, vte, ekey);
			}
		else
			{
			pr_status("Interp",
				"Processing Area Div Link Chain %d T%+d (%d) to T%+d (%d)\n",
				ialink, KeyPlus[skey]/60, skey, KeyPlus[ekey]/60, ekey);
			}
#		endif /* DEBUG_INTERP */

		/* If no dividing lines on link chain try next chain */
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
		sfact   = ((double) (alink->splus - mfirst)) / ((double) DTween);
		efact   = ((double) (alink->eplus - mfirst)) / ((double) DTween);
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
			put_message("area-interp-mins",
						dfld->level, dfld->element, ialink, vts, vte);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%s to T%s\n", vts, vte);
#			endif /* DEBUG_INTERP */
			}
		else
			{
			put_message("area-interp",
						dfld->level, dfld->element, ialink,
						sxplus/60, explus/60);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%+d to T%+d\n", sxplus/60, explus/60);
#			endif /* DEBUG_INTERP */
			}

		/* Check for intermediate control nodes */
		ctrl  = alink->ctrl;
		ncont = ctrl->ncont;
		if (ctrl->lnum != NumTween)
			{
			pr_error("Interp.Areas",
				"Error with intermediate control nodes!\n");
			pr_error("Interp.Areas", "  NumTween: %d  lnum: %d\n",
				NumTween, ctrl->lnum);
			pr_error("Interp.Areas", "Contact system adminstrator!\n");
			ncont = 0;
			}

		/* If there is only one active frame on the link chain,          */
		/*  and if there are no intermediate control nodes,              */
		/*  duplicate the original dividing line across the offset range */
		if (nkey <= 1 && ncont <= 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Only one frame to interpolate\n");
#			endif /* DEBUG_INTERP */

			/* Extract the dividing line */
			ajkey = alink->key + skey;
			line  = ajkey->line;
			jarea = ajkey->iarea;

			/* Duplicate the dividing line in the output data */
			ikey = 0;
			for (itween=sxtween; itween<=extween; itween++)
				{
				mplus = tweentimes[itween];

#				ifdef DEBUG_INTERP
				if (minutes_in_depictions())
					pr_diag("Interp",
						"      Duplicating dividing line in T%s (%d)\n",
						hour_minute_string(0, mplus), itween);
				else
					pr_diag("Interp",
						"      Duplicating dividing line in T%+.2d (%d)\n",
						mplus/60, itween);
#				endif /* DEBUG_INTERP */

				while (ikey < NumKey-1)
					{
					if (mplus < KeyPlus[ikey+1]) break;
					ikey++;
					}
				aikey = alink->key + ikey;

				if (ikey != skey || aikey != ajkey)
					{
					iarea = aikey->iarea;
					pr_diag("Interp.Areas",
						"  Key change!! ikey/skey: %d %d  Areas: %d %d\n",
						ikey, skey, iarea, jarea);
					}

				/* Find the area(s) to be divided */
				/* Note that we must search through the list of all areas */
				/*  to check those which may have been merged or split    */
				for (iblink=0; iblink<nALink; iblink++)
					{
					blink = ALinks + iblink;
					bikey = blink->key + ikey;
					bjkey = blink->key + skey;

					/* Only search boundary links */
					if (blink->ltype != AreaBound) continue;

					/* Check for matching area */
					if (bjkey->iarea != jarea) continue;

					/* Check location in interpolated set */
					iaout = blink->iaout[itween];
					if (iaout < 0) continue;

					if (ikey != skey || bikey != bjkey)
						{
						pr_diag("Interp.Areas",
							"  Key change!! ikey/skey: %d %d  Match areas: %d %d\n",
							ikey, skey, bikey->iarea, bjkey->iarea);
						}

					/* Save the dividing line information */
					wline = copy_line(line);
					add_divline_to_divinfo(genareas[itween], iaout,
										   bjkey, bikey, ajkey, wline);
					}

#				ifdef DEBUG_INTERP_DIV_POINTS
				pr_diag("Interp.Points",
					"      Dividing line points: %d\n", line->numpts);
#				endif /* DEBUG_INTERP_DIV_POINTS */
				}

			/* End of section for single active frame on a link chain */
			/* Move on to next link chain                             */
			continue;
			}

		/* If there is more than one dividing line on the chain, */
		/*  or if there are intermediate control nodes,          */
		/*  then true interpolation is required!                 */

		/* Arrange spatial interpolation on each segment for multiple links */
		nseg = alink->key[skey].nseg;
		nseg = MAX(nseg, 1);
		npseg = INITMEM(int,    nseg);
		dxseg = INITMEM(double, nseg);
		dyseg = INITMEM(double, nseg);
		ddseg = INITMEM(double, nseg);

		/* Find max and min number of points over the active part */
		nspts = 0;
		for (iseg=0; iseg<nseg; iseg++)
			{
#			ifdef DEBUG_INTERP_DIV_POINTS
			pr_diag("Interp.Points", "   Segment: %d of %d\n", iseg, nseg);
#			endif /* DEBUG_INTERP_DIV_POINTS */

			for (ikey=skey; ikey<=ekey; ikey++)
				{
				aikey = alink->key + ikey;
				line  = aikey->line;
				np    = line->numpts;

#				ifdef DEBUG_INTERP_DIV_POINTS
				pr_diag("Interp.Points", "     Frame: %d (%d-%d)  numpts: %d\n",
					ikey, skey, ekey, np);
#				endif /* DEBUG_INTERP_DIV_POINTS */

				if (np <= 0) break;
				if (np == 1)
					{
					add_point_to_line(line, line->points[0]);
					np++;
					}
				if ((ikey == skey) || (np < npmin)) npmin = np;
				if ((ikey == skey) || (np > npmax)) npmax = np;
				}

			/* Determine number of points to do spatial spline */
			npseg[iseg] = NINT(W1*npmax + W2*npmin);
			npseg[iseg] = MAX(npseg[iseg], 10);
			nspts += npseg[iseg];
			}

#		ifdef DEBUG_INTERP_DIV_POINTS
		pr_diag("Interp.Points", "   Spatial spline points: %d\n", nspts);
#		endif /* DEBUG_INTERP_DIV_POINTS */

		/* Find average distance between points for re-splining */
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			line  = aikey->line;
			line_properties(line, NullLogical, NullLogical, NullFloat, &len);
			len /= (line->numpts - 1);
			if ((ikey == skey) || (len < minlen)) minlen = len;
			}

		/* Determine resolution for graphics pipe */
		res = minlen * 0.75;
		if      (res <  1.0) res = 1.0;
		else if (res < 10.0) res = NINT(res);
		else                 res = NINT(res/5) * 5.0;

#		ifdef DEBUG_INTERP_DIV_POINTS
		pr_diag("Interp.Points",
			"   Graphics pipe filter resolution: %.3f\n", res);
#		endif /* DEBUG_INTERP_DIV_POINTS */

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

		/* Do spatial spline on each segment of each dividing line in the */
		/* time sequence */
		for (ipseg=0, iseg=0; iseg<nseg; iseg++)
			{
			spatial(alink, iseg, npseg[iseg], skey, ekey,
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
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(nkey, keytimes+skey, keyx[isp]+skey, keyy[isp]+skey, ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Re-interpolate for intermediate control nodes */
		if (ncont > 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Processing for %d control nodes!\n", ncont);
#			endif /* DEBUG_INTERP */

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
			aikey = alink->key + skey;
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

					/* Begin with interpolated points for this time */
					keytimes2[icont] = mplus;
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] = tweenx[isp][inum];
						keyy2[isp][icont] = tweeny[isp][inum];
						}

#					ifdef DEBUG_INTERP_DIV_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in dividing line: %d\n",
						mplus, nspts);
					pr_diag("Interp.Points",
						"    Indexes ... alink->splus/sxtween: %d %d  alink->eplus/extween: %d %d  inum: %d\n",
						alink->splus, sxtween, alink->eplus, extween, inum);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Initial dividing line: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Initial dividing line: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_DIV_POINTS */

					/* Set modifications for each segment */
					dxseg[0] = dyseg[0] = ddseg[0] = 0.0;
					for (ipseg=npseg[0], iseg=1; iseg<nseg; iseg++)
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
							/*  segment 1, and the start of the dividing line */
							/*  associated with the first point in segment 0. */
							/*  Subsequent link chains can split these        */
							/*  segments, but each is given an original       */
							/*  segment identifier of 2 3 ... etc             */
							/* Note that the start of the dividing line is    */
							/*  not used, so that links are matched with the  */
							/*  segment that is one greater than it           */
							if (ctrl->ilink[inum][icnum] == aikey->dseg[iseg]-1)
								{

#								ifdef DEBUG_INTERP
								pr_diag("Interp",
									"  Matching control node: %d of (0-%d)\n",
									icnum, ctrl->lcnum[inum]-1);
								pr_diag("Interp",
									"    Segment: %d  Match: %d\n",
									iseg, aikey->dseg[iseg]);
#								endif /* DEBUG_INTERP */

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

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"   Interpolated time (cnode): %d  Segments: %d\n",
						mplus, nseg);
					for (ipseg=npseg[0], iseg=1; iseg<nseg; iseg++)
						{
						pr_diag("Interp",
							"    Point: %3d  Segment offset: %.2f %.2f\n",
							ipseg, dxseg[iseg], dyseg[iseg]);
						ipseg += npseg[iseg];
						}
#					endif /* DEBUG_INTERP */

					/* Determine adjustments for all interpolated points */
					/* First extract a subset of the adjustments         */

					/* Extract adjustment for first point of each segment */
					/*  ... starting with the second segment!             */
					for (nsub=0, ipseg=npseg[0], iseg=1; iseg<nseg; iseg++)
						{

						subpts[nsub] = (double) ipseg;
						dxsubs[nsub] = dxseg[iseg];
						dysubs[nsub] = dyseg[iseg];
						nsub++;

						ipseg += npseg[iseg];
						}

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"    Adjustments for %.2f to %.2f points\n",
						subpts[0], subpts[nsub-1]);
					for (isp=0; isp<nsub; isp++)
						{
						pr_diag("Interp",
							"     At point: %.2f  Adjustments: %.2f %.2f\n",
							subpts[isp], dxsubs[isp], dysubs[isp]);
						}
#					endif /* DEBUG_INTERP */

					/* Determine adjustments for all points  */
					/*  using piecewise linear interpolation */
					for (isp=0; isp<nspts; isp++) dspts[isp] = (double) isp;
					PieceWise_2D(nsub, subpts, dxsubs, dysubs,
							nspts, dspts, dxpts, dypts);

#					ifdef DEBUG_INTERP_DIV_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in dividing line: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						pr_diag("Interp.Points",
							"    Adjustments to dividing line: %.3d %.3f %.3f\n",
							isp, dxpts[isp], dypts[isp]);
						}
#					endif /* DEBUG_INTERP_DIV_POINTS */

					/* Apply the adjustments to the interpolated points */
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] += dxpts[isp];
						keyy2[isp][icont] += dypts[isp];
						}

#					ifdef DEBUG_INTERP_DIV_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in dividing line: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Final dividing line: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Final dividing line: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_DIV_POINTS */

					icont++;
					}
				}

			if (icont != nkey+ncont)
				{
				pr_error("Interp.Areas", "Too few link points: %d %d\n",
					icont, nkey+ncont);
				pr_error("Interp.Areas", "Contact system adminstrator!\n");
				}

			/* Re-perform temporal spline on each splined point across */
			/* active key frame sequence, resulting in interpolated    */
			/* positions for each inbetween frame                      */

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(icont, keytimes2, keyx2[isp], keyy2[isp], ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Generate new dividing line for each inbetween frame */
		/* using the new spatial spline coefficients           */
		ikey = 0;
		for (itween=sxtween; itween<=extween; itween++)
			{
			mplus = tweentimes[itween];

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				pr_diag("Interp",
					"      Generating dividing line in T%s (%d)\n",
					hour_minute_string(0, mplus), itween);
			else
				pr_diag("Interp",
					"      Generating dividing line in T%+.2d (%d)\n",
					mplus/60, itween);
#			endif /* DEBUG_INTERP */

#			ifdef DEBUG_INTERP_DIV_POINTS
			pr_diag("Interp.Points",
				"   Points in dividing line: %d\n", nspts);
			for (isp=0; isp<nspts; isp++)
				{
				pr_diag("Interp.Points",
					"    Dividing line: %.3d %.3f %.3f\n",
					isp, tweenx[isp][itween], tweeny[isp][itween]);
				}
#			endif /* DEBUG_INTERP_DIV_POINTS */

			/* Set up spline operation */
			reset_pipe();
			enable_filter(res, 0.0);
			enable_spline(res, closed, 0.0, 0.0, 0.0);
			enable_save();

			/* Re-spline splined line to desired resolution  */
			/* Possibly dividing line may be totally outside */
			for (isp=0; isp<nspts; isp++)
				{
				x = tweenx[isp][itween];
				y = tweeny[isp][itween];
				put_pipe(x, y);
				}
			flush_pipe();
			nlines = recall_save(&lines);
			if (nlines <= 0)
				{
				pr_warning("Interp.Areas",
					" No dividing line after resplining!\n");
				continue;
				}
			if (nlines > 1)
				{
				pr_warning("Interp.Areas",
					" More than one dividing line (%d) after resplining!\n",
					nlines);
				}

			while (ikey < NumKey-1)
				{
				if (mplus < KeyPlus[ikey+1]) break;
				ikey++;
				}
			aikey = alink->key + ikey;
			jkey  = MAX(ikey, skey);
			jkey  = MIN(jkey, ekey);
			ajkey = alink->key + jkey;

			line  = lines[0];
			if (ajkey->flip) reverse_line(line);
			jarea = ajkey->iarea;

			if (ikey != jkey || aikey != ajkey)
				{
				iarea = aikey->iarea;
				pr_diag("Interp.Areas",
					"  Key change!! ikey/jkey: %d %d (%d-%d)  Areas: %d %d\n",
					ikey, jkey, skey, ekey, iarea, jarea);
				}

			/* Find the area(s) to be divided */
			/* Note that we must search through the list of all areas */
			/*  to check those which may have been merged or split    */
			for (iblink=0; iblink<nALink; iblink++)
				{
				blink = ALinks + iblink;
				bikey = blink->key + ikey;
				bjkey = blink->key + jkey;

				/* Only search boundary links */
				if (blink->ltype != AreaBound) continue;

				/* Check for matching area */
				if (bjkey->iarea != jarea) continue;

				/* Check location in interpolated set */
				iaout = blink->iaout[itween];
				if (iaout < 0) continue;

				if (ikey != jkey || bikey != bjkey)
					{
					pr_diag("Interp.Areas",
						"  Key change!! ikey/jkey: %d %d (%d-%d)  Match areas: %d %d\n",
						ikey, jkey, skey, ekey, bikey->iarea, bjkey->iarea);
					}

				/* Save the dividing line information */
				wline = copy_line(line);
				add_divline_to_divinfo(genareas[itween], iaout,
									   bjkey, bikey, ajkey, wline);
				}

#			ifdef DEBUG_INTERP_DIV_POINTS
			pr_diag("Interp.Points",
				"   Points after re-spline: %d\n", line->numpts);
			for (ipt=0; ipt<line->numpts; ipt++)
				pr_diag("Interp.Points",
					"    Dividing line: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);
#			endif /* DEBUG_INTERP_DIV_POINTS */
			}

		/* Free space for temporal spline parameters */
		FREEMEM(npseg);
		FREEMEM(dxseg);
		FREEMEM(dyseg);
		FREEMEM(ddseg);
		FREEMEM(tweenx);
		FREEMEM(tweeny);
		FREEMEM(keyx);
		FREEMEM(keyy);
		FREEMEM(tbufx);
		FREEMEM(tbufy);
		FREEMEM(kbufx);
		FREEMEM(kbufy);
		if (ncont > 0)
			{
			FREEMEM(keytimes2);
			FREEMEM(keyx2);
			FREEMEM(keyy2);
			FREEMEM(kbufx2);
			FREEMEM(kbufy2);
			FREEMEM(subpts);
			FREEMEM(dxsubs);
			FREEMEM(dysubs);
			FREEMEM(dspts);
			FREEMEM(dxpts);
			FREEMEM(dypts);
			}

		/* End of dividing line loop (ialink) */
		}

	/* Process the time sequence for all holes now */
	/* Note that links on the same hole have already been connected! */
	closed = TRUE;
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only process holes */
		if (alink->ltype != AreaHole) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "\n");
			pr_status("Interp",
				"Area Hole Link Chain %d processed with Chain %d\n",
				ialink, alink->icom);
#			endif /* DEBUG_INTERP */

			continue;
			}

		interp_progress(dfld, jlink, nALink+NumTween, -1, -1);
		jlink++;
		skey = alink->skey;
		ekey = alink->ekey;

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "\n");
		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, KeyPlus[skey]));
			(void) strcpy(vte, hour_minute_string(0, KeyPlus[ekey]));
			pr_status("Interp",
				"Processing Area Hole Link Chain %d T%s (%d) to T%s (%d)\n",
				ialink, vts, skey, vte, ekey);
			}
		else
			{
			pr_status("Interp",
				"Processing Area Hole Link Chain %d T%+d (%d) to T%+d (%d)\n",
				ialink, KeyPlus[skey]/60, skey, KeyPlus[ekey]/60, ekey);
			}
#		endif /* DEBUG_INTERP */

		/* If no holes on link chain try next chain */
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
		sfact   = ((double) (alink->splus - mfirst)) / ((double) DTween);
		efact   = ((double) (alink->eplus - mfirst)) / ((double) DTween);
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
			put_message("area-interp-mins",
						dfld->level, dfld->element, ialink, vts, vte);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%s to T%s\n", vts, vte);
#			endif /* DEBUG_INTERP */
			}
		else
			{
			put_message("area-interp",
						dfld->level, dfld->element, ialink,
						sxplus/60, explus/60);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Range T%+d to T%+d\n", sxplus/60, explus/60);
#			endif /* DEBUG_INTERP */
			}

		/* Check for intermediate control nodes */
		ctrl  = alink->ctrl;
		ncont = ctrl->ncont;
		if (ctrl->lnum != NumTween)
			{
			pr_error("Interp.Areas",
				"Error with intermediate control nodes!\n");
			pr_error("Interp.Areas", "  NumTween: %d  lnum: %d\n",
				NumTween, ctrl->lnum);
			pr_error("Interp.Areas", "Contact system adminstrator!\n");
			ncont = 0;
			}

		/* If there is only one active frame on the link chain, */
		/*  and if there are no intermediate control nodes,     */
		/*  duplicate the original hole across the offset range */
		if (nkey <= 1 && ncont <= 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Only one frame to interpolate\n");
#			endif /* DEBUG_INTERP */

			/* Extract the hole */
			ajkey = alink->key + skey;
			line  = ajkey->line;
			jarea = ajkey->iarea;

			/* Duplicate the hole in the output data */
			ikey = 0;
			for (itween=sxtween; itween<=extween; itween++)
				{
				mplus = tweentimes[itween];

#				ifdef DEBUG_INTERP
				if (minutes_in_depictions())
					pr_diag("Interp",
						"      Duplicating hole in T%s (%d)\n",
						hour_minute_string(0, mplus), itween);
				else
					pr_diag("Interp",
						"      Duplicating hole in T%+.2d (%d)\n",
						mplus/60, itween);
#				endif /* DEBUG_INTERP */

				while (ikey < NumKey-1)
					{
					if (mplus < KeyPlus[ikey+1]) break;
					ikey++;
					}
				aikey = alink->key + ikey;

				if (ikey != skey || aikey != ajkey)
					{
					iarea = aikey->iarea;
					pr_diag("Interp.Areas",
						"  Key change!! ikey/skey: %d %d  Areas: %d %d\n",
						ikey, skey, iarea, jarea);
					}

				/* Find the area(s) containing the hole */
				/* Note that we must search through the list of all areas */
				/*  to check those which may have been merged or split    */
				for (iblink=0; iblink<nALink; iblink++)
					{
					blink = ALinks + iblink;
					bikey = blink->key + ikey;
					bjkey = blink->key + skey;

					/* Only search boundary links */
					if (blink->ltype != AreaBound) continue;

					/* Check for matching area */
					if (bjkey->iarea != jarea) continue;

					/* Check location in interpolated set */
					iaout = blink->iaout[itween];
					if (iaout < 0) continue;

					if (ikey != skey || bikey != bjkey)
						{
						pr_diag("Interp.Areas",
							"  Key change!! ikey/skey: %d %d  Match areas: %d %d\n",
							ikey, skey, bikey->iarea, bjkey->iarea);
						}

					/* Save the hole information */
					wline = copy_line(line);
					add_hole_to_holeinfo(genareas[itween], iaout,
										   bjkey, bikey, ajkey, wline);
					}

#				ifdef DEBUG_INTERP_HOLE_POINTS
				pr_diag("Interp.Points",
					"      Hole points: %d\n", line->numpts);
#				endif /* DEBUG_INTERP_HOLE_POINTS */
				}

			/* End of section for single active frame on a link chain */
			/* Move on to next link chain                             */
			continue;
			}

		/* If there is more than one hole on the chain, */
		/*  or if there are intermediate control nodes, */
		/*  then true interpolation is required!        */

		/* Arrange spatial interpolation on each segment for multiple links */
		nseg = alink->key[skey].nseg;
		nseg = MAX(nseg, 1);
		npseg = INITMEM(int,    nseg);
		dxseg = INITMEM(double, nseg);
		dyseg = INITMEM(double, nseg);
		ddseg = INITMEM(double, nseg);

		/* Find max and min number of points over the active part */
		nspts = 0;
		for (iseg=0; iseg<nseg; iseg++)
			{
#			ifdef DEBUG_INTERP_HOLE_POINTS
			pr_diag("Interp.Points", "   Segment: %d of %d\n", iseg, nseg);
#			endif /* DEBUG_INTERP_HOLE_POINTS */

			for (ikey=skey; ikey<=ekey; ikey++)
				{
				aikey = alink->key + ikey;
				line  = aikey->line;
				jseg  = (iseg < nseg-1)? iseg + 1: 0;
				dss   = aikey->dspan[iseg];
				dse   = aikey->dspan[jseg];
				if (aikey->hcw)
					{
					ips = dss + 1.0;
					ipe = dse;
					np  = ipe - ips + 2;
					if (dss >= dse) np += line->numpts;
					}
				else
					{
					ips = dss;
					ipe = dse + 1.0;
					np  = ips - ipe + 2;
					if (dse >= dss) np += line->numpts;
					}

#				ifdef DEBUG_INTERP_HOLE_POINTS
				pr_diag("Interp.Points", "     Frame: %d (%d-%d)  numpts: %d\n",
					ikey, skey, ekey, np);
#				endif /* DEBUG_INTERP_HOLE_POINTS */

				if ((ikey == skey) || (np < npmin)) npmin = np;
				if ((ikey == skey) || (np > npmax)) npmax = np;
				}

			/* Determine number of points to do spatial spline */
			npseg[iseg] = NINT(W1*npmax + W2*npmin);
			npseg[iseg] = MAX(npseg[iseg], 10);
			nspts += npseg[iseg];
			}

#		ifdef DEBUG_INTERP_HOLE_POINTS
		pr_diag("Interp.Points", "   Spatial spline points: %d\n", nspts);
#		endif /* DEBUG_INTERP_HOLE_POINTS */

		/* Find average distance between points for re-splining */
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			line  = aikey->line;
			line_properties(line, NullLogical, NullLogical, NullFloat, &len);
			len /= (line->numpts - 1);
			if ((ikey == skey) || (len < minlen)) minlen = len;
			}

		/* Determine resolution for graphics pipe */
		res = minlen * 0.75;
		if      (res <  1.0) res = 1.0;
		else if (res < 10.0) res = NINT(res);
		else                 res = NINT(res/5) * 5.0;

#		ifdef DEBUG_INTERP_HOLE_POINTS
		pr_diag("Interp.Points",
			"   Graphics pipe filter resolution: %.3f\n", res);
#		endif /* DEBUG_INTERP_HOLE_POINTS */

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

		/* Do spatial spline on each segment of each hole in the */
		/* time sequence */
		for (ipseg=0, iseg=0; iseg<nseg; iseg++)
			{
			spatial(alink, iseg, npseg[iseg], skey, ekey,
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
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(nkey, keytimes+skey, keyx[isp]+skey, keyy[isp]+skey, ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Re-interpolate for intermediate control nodes */
		if (ncont > 0)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "   Processing for %d control nodes!\n", ncont);
#			endif /* DEBUG_INTERP */

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
			aikey = alink->key + skey;
			for (icont=0, ikey=skey, inum=0; inum<ctrl->lnum; inum++)
				{
				mplus = ctrl->mplus[inum];

				/* Use existing points for existing holes */
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

					/* Begin with interpolated points for this time */
					keytimes2[icont] = mplus;
					for (isp=0; isp<nspts; isp++)
						{
						keyx2[isp][icont] = tweenx[isp][inum];
						keyy2[isp][icont] = tweeny[isp][inum];
						}

#					ifdef DEBUG_INTERP_HOLE_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in hole: %d\n",
						mplus, nspts);
					pr_diag("Interp.Points",
						"    Indexes ... alink->splus/sxtween: %d %d  alink->eplus/extween: %d %d  inum: %d\n",
						alink->splus, sxtween, alink->eplus, extween, inum);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Initial hole: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Initial hole: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_HOLE_POINTS */

					/* Set modifications for each segment */
					for (ipseg=0, iseg=0; iseg<nseg; iseg++)
						{
						dxseg[iseg] = dyseg[iseg] = ddseg[iseg] = 0.0;
						for (icnum=0; icnum<ctrl->lcnum[inum]; icnum++)
							{
							/* Match the link chain with the segment          */
							/* Note that ilink identifies link chains in      */
							/*  the order they were created - 0 1 2 ... etc   */
							/* Note that dseg identifies the original segment */
							/*  associated with each link - the first link    */
							/*  chain is associated with the first and last   */
							/*  point in segment 0.  The second link chain    */
							/*  will split this segment, and thus become the  */
							/*  first point in segment 1 and the last point   */
							/*  in segment 0.  Subsequent link chains can     */
							/*  split these segments, but each is given an    */
							/*  original segment identifier of 2 3 ... etc    */
							if (ctrl->ilink[inum][icnum] == aikey->dseg[iseg])
								{

#								ifdef DEBUG_INTERP
								pr_diag("Interp",
									"  Matching control node: %d of (0-%d)\n",
									icnum, ctrl->lcnum[inum]-1);
								pr_diag("Interp",
									"    Segment: %d  Match: %d\n",
									iseg, aikey->dseg[iseg]);
#								endif /* DEBUG_INTERP */

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

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"   Interpolated time (cnode): %d  Segments: %d\n",
						mplus, nseg);
					for (ipseg=npseg[0], iseg=1; iseg<nseg; iseg++)
						{
						pr_diag("Interp",
							"    Point: %3d  Segment offset: %.2f %.2f\n",
							ipseg, dxseg[iseg], dyseg[iseg]);
						ipseg += npseg[iseg];
						}
#					endif /* DEBUG_INTERP */

					/* Determine adjustments for all interpolated points */

					/* With only one segment, apply the adjustments directly */
					/*  to the interpolated points                           */
					if ( nseg <= 1 )
						{
						for (isp=0; isp<nspts; isp++)
							{
							keyx2[isp][icont] += dxseg[0];
							keyy2[isp][icont] += dyseg[0];
							}
						}

					/* For more than one segment, extract a subset of   */
					/*  adjustments to apply to the interpolated points */
					else
						{

#						ifdef DEBUG_INTERP
						for (iseg=0; iseg<nseg; iseg++)
							{
							jseg  = (iseg < nseg-1)? iseg + 1: 0;
							dss   = aikey->dspan[iseg];
							dse   = aikey->dspan[jseg];
							if (aikey->hcw)
								pr_diag("Interp",
									"    Segment positions (cw): %d %.2f %.2f\n",
									iseg, dss, dse);
							else
								pr_diag("Interp",
									"    Segment positions (ccw): %d %.2f %.2f\n",
									iseg, dss, dse);
							}
#						endif /* DEBUG_INTERP */

						/* Extract subset of adjustments based on link nodes */
						for (nsub=0, ipseg=0, iseg=0; iseg<nseg; iseg++)
							{

							subpts[nsub] = (double) ipseg;
							dxsubs[nsub] = dxseg[iseg];
							dysubs[nsub] = dyseg[iseg];
							nsub++;

							ipseg += npseg[iseg];
							}

						/* Add adjustment for last point in last segment  */
						/*  ... the same as first point in first segment! */
						subpts[nsub] = (double) ipseg - 1.0;
						dxsubs[nsub] = dxsubs[1];
						dysubs[nsub] = dysubs[1];
						nsub++;

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"    Adjustments for %.2f to %.2f points\n",
							subpts[0], subpts[nsub-1]);
						for (isp=0; isp<nsub; isp++)
							{
							pr_diag("Interp",
								"     At point: %.2f  Adjustments: %.2f %.2f\n",
								subpts[isp], dxsubs[isp], dysubs[isp]);
							}
#						endif /* DEBUG_INTERP */

						/* Determine adjustments for all points  */
						/*  using piecewise linear interpolation */
						for (isp=0; isp<nspts; isp++) dspts[isp] = (double) isp;
						PieceWise_2D(nsub, subpts, dxsubs, dysubs,
								nspts, dspts, dxpts, dypts);

#						ifdef DEBUG_INTERP_HOLE_POINTS
						pr_diag("Interp.Points",
							"   Interpolated time (cnode): %d  Points in hole: %d\n",
							mplus, nspts);
						for (isp=0; isp<nspts; isp++)
							{
							pr_diag("Interp.Points",
								"    Adjustments to hole: %.3d %.3f %.3f\n",
								isp, dxpts[isp], dypts[isp]);
							}
#						endif /* DEBUG_INTERP_HOLE_POINTS */

						/* Apply the adjustments to the interpolated points */
						for (isp=0; isp<nspts; isp++)
							{
							keyx2[isp][icont] += dxpts[isp];
							keyy2[isp][icont] += dypts[isp];
							}
						}

#					ifdef DEBUG_INTERP_HOLE_POINTS
					pr_diag("Interp.Points",
						"   Interpolated time (cnode): %d  Points in hole: %d\n",
						mplus, nspts);
					for (isp=0; isp<nspts; isp++)
						{
						if (isp==0)
							pr_diag("Interp.Points",
								"    Final hole: %.3d %.3f %.3f\n",
								isp, keyx2[isp][icont], keyy2[isp][icont]);
						else
							pr_diag("Interp.Points",
								"    Final hole: %.3d %.3f %.3f (%.3f @ %.1f)\n",
								isp, keyx2[isp][icont], keyy2[isp][icont],
								hypot(keyx2[isp][icont]-keyx2[isp-1][icont],
										keyy2[isp][icont]-keyy2[isp-1][icont]),
								fpa_atan2deg(keyy2[isp][icont]-keyy2[isp-1][icont],
										keyx2[isp][icont]-keyx2[isp-1][icont]));
						}
#					endif /* DEBUG_INTERP_HOLE_POINTS */

					icont++;
					}
				}

			if (icont != nkey+ncont)
				{
				pr_error("Interp.Areas", "Too few link points: %d %d\n",
					icont, nkey+ncont);
				pr_error("Interp.Areas", "Contact system adminstrator!\n");
				}

			/* Re-perform temporal spline on each splined point across */
			/* active key frame sequence, resulting in interpolated    */
			/* positions for each inbetween frame                      */

#			ifdef DEBUG_INTERP
			pr_diag("Interp",
				"   Temporal spline key: %d->%d (%d) --> tween: %d->%d (%d)\n",
				skey, ekey, nkey, stween, etween, ntween);
#			endif /* DEBUG_INTERP */

			for (isp=0; isp<nspts; isp++)
				{
				QuasiLinear_Tween(icont, keytimes2, keyx2[isp], keyy2[isp], ntot,
						tweentimes+smin, tweenx[isp]+smin, tweeny[isp]+smin);
				}
			}

		/* Generate new hole for each inbetween frame */
		/* using the new spatial spline coefficients  */
		ikey = 0;
		for (itween=sxtween; itween<=extween; itween++)
			{
			mplus = tweentimes[itween];

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				pr_diag("Interp",
					"      Generating hole in T%s (%d)\n",
					hour_minute_string(0, mplus), itween);
			else
				pr_diag("Interp",
					"      Generating hole in T%+.2d (%d)\n",
					mplus/60, itween);
#			endif /* DEBUG_INTERP */

#			ifdef DEBUG_INTERP_HOLE_POINTS
			pr_diag("Interp.Points",
				"   Points in hole: %d\n", nspts);
			for (isp=0; isp<nspts; isp++)
				{
				pr_diag("Interp.Points",
					"    Hole: %.3d %.3f %.3f\n",
					isp, tweenx[isp][itween], tweeny[isp][itween]);
				}
#			endif /* DEBUG_INTERP_HOLE_POINTS */

			/* Set up spline operation */
			reset_pipe();
			enable_filter(res, 0.0);
			enable_spline(res, closed, 0.0, 0.0, 0.0);
			enable_save();

			/* Re-spline splined line to desired resolution */
			/* Possibly hole may be totally outside         */
			for (isp=0; isp<nspts; isp++)
				{
				x = tweenx[isp][itween];
				y = tweeny[isp][itween];
				put_pipe(x, y);
				}
			flush_pipe();
			nlines = recall_save(&lines);
			if (nlines <= 0)
				{
				pr_warning("Interp.Areas",
					" No hole after resplining!\n");
				continue;
				}
			if (nlines > 1)
				{
				pr_warning("Interp.Areas",
					" More than one hole (%d) after resplining!\n",
					nlines);
				}

			while (ikey < NumKey-1)
				{
				if (mplus < KeyPlus[ikey+1]) break;
				ikey++;
				}
			aikey = alink->key + ikey;
			jkey  = MAX(ikey, skey);
			jkey  = MIN(jkey, ekey);
			ajkey = alink->key + jkey;

			line  = lines[0];

			line_properties(line, NullLogical, &hcw, NullFloat, NullFloat);
			if (ajkey->hcw && !hcw || !ajkey->hcw && hcw) reverse_line(line);

#			ifdef DEBUG_INTERP
			pr_diag("Interp", "      Hole ikey/jkey: %d %d\n", ikey, jkey);
			if (ajkey->hcw && !hcw)
				pr_diag("Interp",
					"      Reverse interpolated hole - Counterclockwise -> Clockwise\n");
			else if (!ajkey->hcw && hcw)
				pr_diag("Interp",
					"      Reverse interpolated hole - Clockwise -> Counterclockwise\n");
#			endif /* DEBUG_INTERP */

			jarea = ajkey->iarea;

			if (ikey != jkey || aikey != ajkey)
				{
				iarea = aikey->iarea;
				pr_diag("Interp.Areas",
					"  Key change!! ikey/jkey: %d %d (%d-%d)  Areas: %d %d\n",
					ikey, jkey, skey, ekey, iarea, jarea);
				}

			/* Find the area(s) containing the hole */
			/* Note that we must search through the list of all areas */
			/*  to check those which may have been merged or split    */
			for (iblink=0; iblink<nALink; iblink++)
				{
				blink = ALinks + iblink;
				bikey = blink->key + ikey;
				bjkey = blink->key + jkey;

				/* Only search boundary links */
				if (blink->ltype != AreaBound) continue;

				/* Check for matching area */
				if (bjkey->iarea != jarea) continue;

				/* Check location in interpolated set */
				iaout = blink->iaout[itween];
				if (iaout < 0) continue;

				if (ikey != jkey || bikey != bjkey)
					{
					pr_diag("Interp.Areas",
						"  Key change!! ikey/jkey: %d %d (%d-%d)  Match areas: %d %d\n",
						ikey, jkey, skey, ekey, bikey->iarea, bjkey->iarea);
					}

				/* Save the hole information */
				wline = copy_line(line);
				add_hole_to_holeinfo(genareas[itween], iaout,
									   bjkey, bikey, ajkey, wline);
				}

#			ifdef DEBUG_INTERP_HOLE_POINTS
			pr_diag("Interp.Points",
				"   Points after re-spline: %d\n", line->numpts);
			for (jpt=0; jpt<line->numpts; jpt++)
				pr_diag("Interp.Points",
					"    Hole: %.3d  %.2f %.2f\n",
					jpt, line->points[jpt][X], line->points[jpt][Y]);
#			endif /* DEBUG_INTERP_HOLE_POINTS */
			}

		/* Free space for temporal spline parameters */
		FREEMEM(npseg);
		FREEMEM(dxseg);
		FREEMEM(dyseg);
		FREEMEM(ddseg);
		FREEMEM(tweenx);
		FREEMEM(tweeny);
		FREEMEM(keyx);
		FREEMEM(keyy);
		FREEMEM(tbufx);
		FREEMEM(tbufy);
		FREEMEM(kbufx);
		FREEMEM(kbufy);
		if (ncont > 0)
			{
			FREEMEM(keytimes2);
			FREEMEM(keyx2);
			FREEMEM(keyy2);
			FREEMEM(kbufx2);
			FREEMEM(kbufy2);
			FREEMEM(subpts);
			FREEMEM(dxsubs);
			FREEMEM(dysubs);
			FREEMEM(dspts);
			FREEMEM(dxpts);
			FREEMEM(dypts);
			}

		/* End of hole loop (ialink) */
		}

	/* Now add holes and divide all areas which have holes and/or divides */
	ikey = 0;
	for (itween=0; itween<NumTween; itween++)
		{

		interp_progress(dfld, jlink, nALink+NumTween, -1, -1);
		jlink++;

		/* Set frame time */
		mplus  = tweentimes[itween];
		while (ikey < NumKey-1)
			{
			if (mplus < KeyPlus[ikey+1]) break;
			ikey++;
			}

		/* Check for interpolated areas */
		genarea = genareas[itween];
		if (genarea->narea <= 0) continue;

		/* Get affected set */
		if (!genlist[itween])
			{
			(void) printf("!!!HELP!!!\n");
			continue;
			}
		genset = genlist[itween]->data.set;

		/* Loop through interpolated areas */
		for (jarea=0; jarea<genarea->narea; jarea++)
			{

			/* Which interpolated area is affected */
			iaout   = genarea->iaout[jarea];
			areakey = genarea->areakey[jarea];
			if (iaout < 0)            continue;
			if (iaout >= genset->num) continue;
			area = (AREA) genset->list[iaout];
			if (IsNull(area)) continue;

			/* Check for holes */
			holeinfo = genarea->holeinfo[jarea];
			if (holeinfo->nhole > 0)
				{

				if (minutes_in_depictions())
					{
					(void) strcpy(vts, hour_minute_string(0, mplus));
					put_message("area-interp-hole-mins",
								dfld->level, dfld->element, vts);
					}
				else
					{
					put_message("area-interp-hole",
							dfld->level, dfld->element, mplus/60);
					}

				if (holeinfo->nhole > areakey->nhole)
					{
					pr_error("Interp.Areas",
						"Too many holes (%d > %d) for area %d - %X\n",
						holeinfo->nhole, areakey->nhole, areakey->iarea,
						areakey);

					if (minutes_in_depictions())
						{
						pr_error("Interp.Areas",
							"  for field %s %s at T%s (%d)\n",
							dfld->level, dfld->element, vts, itween);
						put_message("area-interp-warn2-mins",
									dfld->level, dfld->element, vts);
						(void) sleep(2);
						}
					else
						{
						pr_error("Interp.Areas",
							"  for field %s %s at T%+.2d (%d)\n",
							dfld->level, dfld->element, mplus/60, itween);
						put_message("area-interp-warn2",
									dfld->level, dfld->element, mplus/60);
						(void) sleep(2);
						}
					}

#				ifdef DEBUG_INTERP
				pr_diag("Interp", "\n");
				if (minutes_in_depictions())
					pr_diag("Interp",
						"      Adding holes to area %d (%d) in T%s (%d)\n",
						areakey->iarea, iaout, vts, itween);
				else
					pr_diag("Interp",
						"      Adding holes to area %d (%d) in T%+.2d (%d)\n",
						areakey->iarea, iaout, mplus/60, itween);
#				endif /* DEBUG_INTERP */

				/* Area has holes ... so replicate order */
				order = INITMEM(int, areakey->nhole);
				for (jh=0; jh<areakey->nhole; jh++) order[jh] = -1;

				/* Now match the holes */
				for (jh=0; jh<areakey->nhole; jh++)
					{
					for (ih=0; ih<holeinfo->nhole; ih++)
						if (holeinfo->holekey[ih]->imem == jh) break;
					if (ih >= holeinfo->nhole)
						{

#						ifdef DEBUG_INTERP
						pr_diag("Interp", "        Missing hole!\n");
#						endif /* DEBUG_INTERP */

						continue;
						}
					order[jh] = ih;
					}

				/* Now loop through holes */
				for (jh=0; jh<areakey->nhole; jh++)
					{

					/* Skip missing holes */
					if (order[jh] < 0) continue;

					/* Ensure that hole is inside area */
					hole = holeinfo->holes[order[jh]];
					if (!hole_inside_area(area, hole))
						{

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"        Skipping hole: %d\n", order[jh]);
#						endif /* DEBUG_INTERP */

						continue;
						}

#					ifdef DEBUG_INTERP
					pr_diag("Interp", "        Adding hole: %d\n", order[jh]);
#					endif /* DEBUG_INTERP */

					/* Add the hole to the area */
					add_area_hole(area, copy_line(hole));
					}

				/* Free space for hole arrays */
				FREEMEM(order);
				}

			/* Check for dividing lines */
			divinfo = genarea->divinfo[jarea];
			if (divinfo->ndiv > 0)
				{

				if (minutes_in_depictions())
					{
					(void) strcpy(vts, hour_minute_string(0, mplus));
					put_message("area-interp-div-mins",
								dfld->level, dfld->element, vts);
					}
				else
					{
					put_message("area-interp-div",
								dfld->level, dfld->element, mplus/60);
					}

				if (divinfo->ndiv > areakey->ndiv)
					{
					pr_error("Interp.Areas",
						"Too many dividing lines (%d > %d) for area %d - %X\n",
						divinfo->ndiv, areakey->ndiv, areakey->iarea, areakey);

					if (minutes_in_depictions())
						{
						pr_error("Interp.Areas",
							"  for field %s %s at T%s (%d)\n",
							dfld->level, dfld->element, vts, itween);
						put_message("area-interp-warn-mins",
									dfld->level, dfld->element, vts);
						(void) sleep(2);
						}
					else
						{
						pr_error("Interp.Areas",
							"  for field %s %s at T%+.2d (%d)\n",
							dfld->level, dfld->element, mplus/60, itween);
						put_message("area-interp-warn",
									dfld->level, dfld->element, mplus/60);
						(void) sleep(2);
						}
					}
				numdiv = areakey->ndiv;

#				ifdef DEBUG_INTERP
				pr_diag("Interp", "\n");
				if (minutes_in_depictions())
					pr_diag("Interp",
						"      Dividing area %d (%d) in T%s (%d)\n",
						areakey->iarea, iaout, vts, itween);
				else
					pr_diag("Interp",
						"      Dividing area %d (%d) in T%+.2d (%d)\n",
						areakey->iarea, iaout, mplus/60, itween);
#				endif /* DEBUG_INTERP */

				/* Area has dividing lines ... so replicate subids */
				sids  = INITMEM(int, areakey->ndiv);
				xrem  = INITMEM(int, areakey->ndiv);
				order = INITMEM(int, areakey->ndiv);
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{
					sids[jdiv]  = areakey->sids[jdiv];
					xrem[jdiv]  = 0;
					order[jdiv] = -1;
					}

				/* Now match the dividing lines to the subids */
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{
					for (idiv=0; idiv<numdiv; idiv++)
						if (divinfo->divkey[idiv]->imem == jdiv) break;
					if (idiv >= numdiv)
						{
						sids[jdiv] = -1;
						for (xdiv=jdiv; xdiv<areakey->ndiv; xdiv++)
							if (sids[xdiv] > jdiv) xrem[jdiv]++;
						continue;
						}
					order[jdiv] = idiv;
					}

#				ifdef DEBUG_INTERP_DIV_POINTS
				(void) strcpy(ybuf, "");
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{
					(void) sprintf(xbuf, " %d", sids[jdiv]);
					(void) strcat(ybuf, xbuf);
					}
				pr_diag("Interp", "          Area subids:  %d - %s\n",
						areakey->ndiv, ybuf);
				(void) strcpy(ybuf, "");
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{
					(void) sprintf(xbuf, " %d", xrem[jdiv]);
					(void) strcat(ybuf, xbuf);
					}
				pr_diag("Interp", "          Subid adjust: %d - %s\n",
						areakey->ndiv, ybuf);
#				endif /* DEBUG_INTERP_DIV_POINTS */

				/* Adjust subids to match missing dividing lines */
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{
					if (sids[jdiv] < 0) continue;
					sids[jdiv] -= xrem[jdiv];
					}

				/* Now loop through dividing lines */
				for (jdiv=0; jdiv<areakey->ndiv; jdiv++)
					{

					/* Set the subarea to divide */
					isub = sids[jdiv];
					if (isub < 0) continue;
					if (isub > area->numdiv+1)
						{
						pr_error("Interp.Areas",
							"Subarea mismatch!! isub: %d  max: %d\n",
							isub, area->numdiv+1);
						continue;
						}
					dsub   = area->subareas[isub];
					divkey = divinfo->divkey[order[jdiv]];

#					ifdef DEBUG_INTERP
					pr_diag("Interp", "        Dividing subarea: %d\n", isub);
#					endif /* DEBUG_INTERP */

					/* Prepare the dividing line for this area */
					line = divinfo->divlines[order[jdiv]];
					divl = prepare_area_divline(area, dsub, line, &dstat);

					/* This dividing line is outside its subarea */
					if (IsNull(divl))
						{

						/* Need to reset the subarea attributes */
						switch(dstat)
							{
							case DivAreaLeft:
								define_subarea_value(dsub, divkey->lsub,
										divkey->lval, divkey->llab);
								define_subarea_attribs(dsub, divkey->lcal);
								resetnext = !areakey->cw;
								break;
							case DivAreaRight:
								define_subarea_value(dsub, divkey->rsub,
										divkey->rval, divkey->rlab);
								define_subarea_attribs(dsub, divkey->rcal);
								resetnext = areakey->cw;
								break;
							default:
								continue;
							}

						/* Also need to shuffle dividing lines and subareas  */
						/*  to avoid additional divides that no longer exist */
						(void) reset_area_subids(areakey->ndiv, sids, jdiv,
								area->numdiv+1, resetnext);
						continue;
						}

					/* Divide the area */
					if (!divide_area(area, dsub, divl, &lsa, &rsa, &dstat))
						{

						/* Need to reset the subarea attributes */
						switch(dstat)
							{
							case DivAreaLeft:
								define_subarea_value(dsub, divkey->lsub,
										divkey->lval, divkey->llab);
								define_subarea_attribs(dsub, divkey->lcal);
								resetnext = !areakey->cw;
								break;
							case DivAreaRight:
								define_subarea_value(dsub, divkey->rsub,
										divkey->rval, divkey->rlab);
								define_subarea_attribs(dsub, divkey->rcal);
								resetnext = areakey->cw;
								break;
							default:
								destroy_line(divl);
								continue;
							}

						/* Also need to shuffle dividing lines and subareas  */
						/*  to avoid additional divides that no longer exist */
						(void) reset_area_subids(areakey->ndiv, sids, jdiv,
								area->numdiv+1, resetnext);
						destroy_line(divl);
						continue;
						}

					/* Set the subarea values on both sides of the divide */
					define_subarea_value(lsa, divkey->lsub, divkey->lval,
										 divkey->llab);
					define_subarea_attribs(lsa, divkey->lcal);
					define_subarea_value(rsa, divkey->rsub, divkey->rval,
										 divkey->rlab);
					define_subarea_attribs(rsa, divkey->rcal);
					}

				/* Reset default area attributes from first subarea */
				if (areakey->ndiv > 0)
					{
#					ifdef DEBUG_INTERP
					pr_diag("Interp", "      Reset area attribs\n");
#					endif /* DEBUG_INTERP */
					define_area_attribs(area, area->subareas[0]->attrib);
					}

				/* Free space for dividing line arrays */
				FREEMEM(sids);
				FREEMEM(xrem);
				FREEMEM(order);
				}

			/* Do not need to re-display if no holes or dividing lines */
			if (holeinfo->nhole <= 0 && divinfo->ndiv <= 0) continue;

			/* Invoke the display specifications (saved in genset) */
			invoke_item_catspec("area", area, genset->ncspec, genset->cspecs);

			/* Re-display the interpolated area */
			if (showtween || (show && KeyPlus[ikey]==mplus))
				{
				warea = copy_area(area, FALSE);
				dformat = field_display_format(CurrElement, CurrLevel);
				if (dformat = DisplayFormatComplex) prep_area_complex(warea);
				else                                prep_area(warea);
				add_item_to_metafile(BlankMeta, "area", "b", "", "",
										(ITEM) warea);
#				ifdef EFF
					gxSetupTransform(DnMap);
					glClipMode(glCLIP_ON);
					display_area(warea);
					glClipMode(glCLIP_OFF);
					sync_display();
#				else
					present_blank(TRUE);
#				endif /* EFF */
				}
			}
		}

	/* Re-order interpolated areas to match original order */
	ikey = 0;
	for (itween=0; itween<NumTween; itween++)
		{
		if (IsNull(genlist[itween])) continue;
		genset = genlist[itween]->data.set;
		if (IsNull(genset))          continue;
		if (genset->num <= 1)        continue;

		/* Find keyframe which covers this inbetween frame */
		mplus = tweentimes[itween];
		while (ikey < NumKey-1)
			{
			if (mplus < KeyPlus[ikey+1]) break;
			ikey++;
			}
		keyset = (NotNull(KeyList[ikey]))? KeyList[ikey]->data.set: NullSet;

		/* We may need information from the next frame for early starts! */
		jkey    = (ikey < NumKey-1)? ikey+1: -1;
		keysetx = NullSet;
		if (jkey >= 0 && NotNull(KeyList[jkey]))
			{
			keysetx = KeyList[jkey]->data.set;
			}

		if (minutes_in_depictions())
			{
			(void) strcpy(vts, hour_minute_string(0, mplus));
			put_message("area-interp-order-mins",
						dfld->level, dfld->element, vts);
			}
		else
			{
			put_message("area-interp-order",
					dfld->level, dfld->element, mplus/60);
			}

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "\n");
		if (minutes_in_depictions())
			pr_diag("Interp", "      Reordering areas in T%s (%d)\n",
				hour_minute_string(0, mplus), itween);
		else
			pr_diag("Interp", "      Reordering areas in T%+.2d (%d)\n",
				mplus/60, itween);
#		endif /* DEBUG_INTERP */

		/* Get the area info */
		genarea = genareas[itween];
		if (genarea->narea != genset->num)
			{
			pr_error("Interp.Areas",
				"Area mismatch!! genset: %d  genarea: %d\n",
				genset->num, genarea->narea);
			}

#		ifdef DEBUG_INTERP
		for (jarea=0; jarea<genarea->narea; jarea++)
			pr_diag("Interp",
				"        Interp area: (%d)  Link: %d\n",
				genarea->iaout[jarea], genarea->ialink[jarea]);
#		endif /* DEBUG_INTERP */

		/* Initialize array to hold ordering information */
		norder = 0;
		aids   = INITMEM(int,     genarea->narea);
		axids  = INITMEM(int,     genarea->narea);
		aouts  = INITMEM(int,     genarea->narea);
		asave  = INITMEM(LOGICAL, genarea->narea);
		for (jarea=0; jarea<genarea->narea; jarea++)
			{
			aids[jarea]  = axids[jarea] = aouts[jarea] = -1;
			asave[jarea] = FALSE;
			}

		/* First order areas based on this keyframe */
		if (NotNull(keyset) && keyset->num > 0)
			{

			/* Examine all generated areas in this generated frame */
			for (jarea=0; jarea<genarea->narea; jarea++)
				{

				/* Only process boundaries */
				ialink = genarea->ialink[jarea];
				alink  = ALinks + ialink;
				if (alink->ltype != AreaBound) continue;
				if (alink->icom  != ialink)    continue;

				/* Compare areas with the keyframe */
				for (iarea=0; iarea<keyset->num; iarea++)
					{

					/* Check for matching area */
					aikey = alink->key + ikey;
					if (aikey->iarea != iarea) continue;

					/* Check location in interpolated set */
					iaout = alink->iaout[itween];
					if (iaout < 0)             continue;

					/* Set the next link too */
					if (jkey >= 0) ajkey = alink->key + jkey;

					/* Check whether this area has already been saved */
					issaved = FALSE;
					for (iorder=0; iorder<norder; iorder++)
						{
						if (aids[iorder] == iarea)
							{
							issaved = TRUE;
							break;
							}
						}

					/* Find where the area order information belongs */
					for (iorder=0; iorder<norder; iorder++)
						{
						if (aids[iorder] > iarea) break;
						}
					for (ixorder=norder; ixorder>iorder; ixorder--)
						{
						aids[ixorder]  = aids[ixorder-1];
						axids[ixorder] = axids[ixorder-1];
						aouts[ixorder] = aouts[ixorder-1];
						asave[ixorder] = asave[ixorder-1];
						}

					/* Save only one copy of an area for each keyframe */
					/* Any matching areas will be from merge/split areas */
					if (issaved && mplus == KeyPlus[ikey])
						{
						aids[iorder]  = iarea;
						if (jkey >= 0) axids[iorder] = ajkey->iarea;
						aouts[iorder] = iaout;
						asave[iorder] = FALSE;
						norder++;

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"         Skipping - duplicate merge/split: %d (%d)\n",
							iarea, iaout);
#						endif /* DEBUG_INTERP */

						continue;
						}

					/* Save the area order information */
					aids[iorder]  = iarea;
					if (jkey >= 0) axids[iorder] = ajkey->iarea;
					aouts[iorder] = iaout;
					asave[iorder] = TRUE;
					norder++;

#					ifdef DEBUG_INTERP
					if (jkey >= 0)
						{
						pr_diag("Interp",
							"        Ordering area: %d (%d)  Link: %d  Next: %d\n",
							iarea, iaout, ialink, ajkey->iarea);
						}
					else
						{
						pr_diag("Interp",
							"        Ordering area: %d (%d)  Link: %d\n",
							iarea, iaout, ialink);
						}
#					endif /* DEBUG_INTERP */
					}
				}
			}

		/* Next order areas from early starts in next keyframe */
		if (NotNull(keysetx) && keysetx->num > 0)
			{

			/* Examine all generated areas in this generated frame */
			for (jarea=0; jarea<genarea->narea; jarea++)
				{

				/* Only process boundaries */
				ialink = genarea->ialink[jarea];
				alink  = ALinks + ialink;
				if (alink->ltype != AreaBound) continue;
				if (alink->icom  != ialink)    continue;

				/* Compare areas with the keyframe */
				for (iarea=0; iarea<keysetx->num; iarea++)
					{

					/* Check for matching area */
					ajkey = alink->key + jkey;
					if (ajkey->iarea != iarea) continue;

					/* Check location in interpolated set */
					iaout = alink->iaout[itween];
					if (iaout < 0)             continue;

					/* Check whether this area has already been saved */
					for (iorder=0; iorder<norder; iorder++)
						{
						if (aouts[iorder] == iaout) break;
						}
					if (iorder < norder)
						{

#						ifdef DEBUG_INTERP
						pr_diag("Interp",
							"         Skipping next - already ordered: %d (%d)  Link: %d\n",
							iarea, iaout, ialink);
#						endif /* DEBUG_INTERP */

						continue;
						}

					/* Find where the area order information belongs */
					for (iorder=0; iorder<norder; iorder++)
						{
						if (axids[iorder] > iarea) break;
						}
					for (ixorder=norder; ixorder>iorder; ixorder--)
						{
						aids[ixorder]  = aids[ixorder-1];
						axids[ixorder] = axids[ixorder-1];
						aouts[ixorder] = aouts[ixorder-1];
						asave[ixorder] = asave[ixorder-1];
						}

					/* Save the area order information */
					aids[iorder]  = -1;
					axids[iorder] = iarea;
					aouts[iorder] = iaout;
					asave[iorder] = TRUE;
					norder++;

#					ifdef DEBUG_INTERP
					pr_diag("Interp",
						"        Ordering next area: %d (%d)  Link: %d\n",
						iarea, iaout, ialink);
#					endif /* DEBUG_INTERP */
					}
				}
			}

#		ifdef DEBUG_INTERP
		pr_diag("Interp", "      Final reordering\n");
		for (iorder=0; iorder<norder; iorder++)
			{
			if (asave[iorder])
				pr_diag("Interp",
					"        Area order: %d - %d  (%d)\n",
					aids[iorder], axids[iorder], aouts[iorder]);
			else
				pr_diag("Interp",
					"        Area order: %d - %d  (%d)  Skip!\n",
					aids[iorder], axids[iorder], aouts[iorder]);
			}
		pr_diag("Interp", "      Saving areas\n");
#		endif /* DEBUG_INTERP */

		/* Make an empty area set and steal areas from genset */
		tmpset = create_set("area");
		tmpset->max  = genset->max;
		tmpset->num  = genset->num;
		tmpset->list = genset->list;
		genset->max  = 0;
		genset->num  = 0;
		genset->list = NullItemList;

		/* Steal areas back into genset in desired order */
		for (iorder=0; iorder<norder; iorder++)
			{
			if (asave[iorder])
				{
				add_item_to_set(genset, tmpset->list[aouts[iorder]]);
				tmpset->list[aouts[iorder]] = NullItem;

#				ifdef DEBUG_INTERP
				pr_diag("Interp",
					"        Saving area: %d - %d  (%d)\n",
					aids[iorder], axids[iorder], aouts[iorder]);
#				endif /* DEBUG_INTERP */

				}
			else
				{

#				ifdef DEBUG_INTERP
				pr_diag("Interp",
					"         Skipping area - duplicate merge/split: %d - %d  (%d)\n",
					aids[iorder], axids[iorder], aouts[iorder]);
#				endif /* DEBUG_INTERP */

				}
			}

		/* Clean up */
		FREEMEM(aids);
		FREEMEM(axids);
		FREEMEM(aouts);
		FREEMEM(asave);
		destroy_set(tmpset);
		}

	/* Recompute labels to account for divides etc. */
	for (itween=0; itween<NumTween; itween++)
		{
		if (IsNull(genlist[itween])) continue;
		genset = genlist[itween]->data.set;
		if (IsNull(genset))          continue;

		if (NotNull(genlabs[itween]))
			{
			recompute_areaset_labs(genset, genlabs[itween], TRUE);
			}
		}

	/* Free area link buffers */
	(void) free_area_links();

	/* Free time lists */
	FREEMEM(keytimes);
	FREEMEM(tweentimes);
	FREEMEM(KeyTime);
	FREEMEM(KeyPlus);
	FREEMEM(KeyList);
	FREEMEM(KeyLabs);
	NumKey = 0;

	/* Free space for area, dividing line and hole information */
	for (itween=0; itween<NumTween; itween++)
		{
		if (genareas[itween]->narea > 0)
			{
			for (iarea=0; iarea<genareas[itween]->narea; iarea++)
				{
				holeinfo = genareas[itween]->holeinfo[iarea];
				if (holeinfo->nhole > 0)
					{
					for (ih=0; ih<holeinfo->nhole; ih++)
						{
						hole = holeinfo->holes[ih];
						hole = destroy_line(hole);
						}
					FREEMEM(holeinfo->holekey);
					FREEMEM(holeinfo->holes);
					}
				FREEMEM(holeinfo);
				divinfo = genareas[itween]->divinfo[iarea];
				if (divinfo->ndiv > 0)
					{
					for (idiv=0; idiv<divinfo->ndiv; idiv++)
						{
						divl = divinfo->divlines[idiv];
						divl = destroy_line(divl);
						}
					FREEMEM(divinfo->divkey);
					FREEMEM(divinfo->divlines);
					}
				FREEMEM(divinfo);
				}
			FREEMEM(genareas[itween]->iaout);
			FREEMEM(genareas[itween]->ialink);
			FREEMEM(genareas[itween]->areakey);
			FREEMEM(genareas[itween]->chckkey);
			FREEMEM(genareas[itween]->divinfo);
			FREEMEM(genareas[itween]->holeinfo);
			}
		FREEMEM(genareas[itween]);
		}
	FREEMEM(genareas);

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In interp_area() - T %s %s\n", dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In interp_area() - F %s %s\n", dfld->element, dfld->level);
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
	ALINK	*alink,
	int		iseg,
	int		nspts,
	int		skey,
	int		ekey,
	double	**keyx,
	double	**keyy
	)

	{
	int		mplus;
	int		isp, ikey, lspts, itop, ip, np, nseg, jseg, ips, ipe;
	ALKEY	*aikey;
	LINE	line, wline, spseg;
	LOGICAL	closed, cw, hcw, doseg;
	float	slen, length, angp=0, spres, sprev, spmin, spmax, dss, dse;

#	ifdef EFF
	static	CURVE	wcurv=NullCurve;
#	endif /* EFF */

	/* Figure out what we're interpolating */
	doseg = FALSE;
	switch (alink->ltype)
		{
		case AreaBound:	closed = TRUE;
						if (iseg >= 0 && alink->key[skey].nseg > 0)
							{
							doseg  = TRUE;
							closed = FALSE;
							}
						break;
		case AreaDiv:	closed = FALSE;
						if (iseg >= 0 && alink->key[skey].nseg > 0)
							{
							doseg  = TRUE;
							}
						break;
		case AreaHole:	closed = TRUE;
						if (iseg >= 0 && alink->key[skey].nseg > 0)
							{
							doseg  = TRUE;
							closed = FALSE;
							}
						break;
		default:		return;
		}

	/* Do spatial spline on each area, dividing line or hole in time sequence */
	for (ikey=skey; ikey<=ekey; ikey++)
		{
		mplus = KeyPlus[ikey];
		aikey = alink->key + ikey;

		/* Special handling for segmented boundaries */
		if (alink->ltype == AreaBound && doseg)
			{
			line  = aikey->line;
			np    = line->numpts;
			nseg  = aikey->nseg;
			wline = create_line();
			jseg  = (iseg < nseg-1)? iseg + 1: 0;
			dss   = aikey->dspan[iseg];
			dse   = aikey->dspan[jseg];
			if (aikey->cw)
				{
				ips = dss + 1.0;
				ipe = dse;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (dss < dse)
					{
					append_line_pdir(wline, line, ips, ipe, TRUE);
					}
				else
					{
					append_line_pdir(wline, line, ips, np-1, TRUE);
					append_line_pdir(wline, line, 0,   ipe,  TRUE);
					}
				}
			else
				{
				ips = dss;
				ipe = dse + 1.0;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (dse < dss)
					{
					append_line_pdir(wline, line, ipe, ips, FALSE);
					}
				else
					{
					append_line_pdir(wline, line, 0,   ips,  FALSE);
					append_line_pdir(wline, line, ipe, np-1, FALSE);
					}
				}
#				ifdef EFF
					if (!wcurv)
						{
						wcurv = create_curve("", "", "");
						define_lspec(&wcurv->lspec, 0, 0, NULL, FALSE,
									1.0, 2.0, (HILITE) 2);
						}
					wcurv->line = wline;
					gxSetupTransform(DnMap);
					glClipMode(glCLIP_ON);
					display_curve(wcurv);
					glClipMode(glCLIP_OFF);
					sync_display();
					wcurv->line = NullLine;
#				endif /* EFF */
			}

		/* Special handling for entire boundaries */
		else if (alink->ltype == AreaBound)
			{
			line  = aikey->line;
			wline = copy_line(line);
			line_properties(wline, NullLogical, &cw, NullFloat, NullFloat);

			/* Extract portion of boundary needed for merge/split */
			/* For now, centre it on the link point */
			if (aikey->mfact < 1.0 || aikey->sfact < 1.0)
				{
				int		nmp0, nmp, imp, imps, impe, jmp, kmp;
				float	fact, dmx, dmy;
				POINT	p;

				fact = MIN(aikey->mfact, aikey->sfact);
				nmp0  = line->numpts;
				nmp   = fact*(nmp0-1) + 5;
				if (nmp > 6 && nmp < nmp0)
					{
					empty_line(wline);
					imp  = line_closest_point(line, aikey->lpos, NULL, NULL);
					imps = imp - nmp/2;
					impe = imps + nmp - 1;

					jmp = imps;
					if (jmp < 0) jmp += nmp0;
					copy_point(p, line->points[jmp]);
					kmp = jmp;
					do	{
						kmp++;
						if (kmp >= nmp0) kmp -= nmp0;
						dmx = line->points[kmp][X] - p[X];
						dmy = line->points[kmp][Y] - p[Y];
						} while (dmx==0 && dmy==0);
					p[X] += (cw)?  dmy: -dmy;
					p[Y] += (cw)? -dmx:  dmx;
					add_point_to_line(wline, p);

					for (imp=imps; imp<=impe; imp++)
						{
						jmp = imp;
						if (jmp < 0)    jmp += nmp0;
						if (jmp >= nmp0) jmp -= nmp0;
						add_point_to_line(wline, line->points[jmp]);
						}

					jmp = impe;
					if (jmp > nmp0) jmp -= nmp0;
					copy_point(p, line->points[jmp]);
					kmp = jmp;
					do	{
						kmp--;
						if (kmp < 0) kmp += nmp0;
						dmx = line->points[kmp][X] - p[X];
						dmy = line->points[kmp][Y] - p[Y];
						} while (dmx==0 && dmy==0);
					p[X] += (cw)? -dmy:  dmy;
					p[Y] += (cw)?  dmx: -dmx;
					add_point_to_line(wline, p);

					add_point_to_line(wline, wline->points[0]);
					}
				}

			if (!cw) reverse_line(wline);
			}

		/* Special handling for segmented dividing lines */
		else if (alink->ltype == AreaDiv && doseg)
			{
			line  = aikey->line;
			np    = line->numpts;
			nseg  = aikey->nseg;
			wline = create_line();
			dss   = aikey->dspan[iseg];
			if (iseg < nseg-1)
				dse = aikey->dspan[iseg+1];
			else
				dse = (aikey->flip)? 0: np-1;
			if (aikey->flip)
				{
				ips = dss;
				ipe = dse + 1.0;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (ipe <= ips)
					append_line_pdir(wline, line, ipe, ips, FALSE);
				}
			else
				{
				ips = dss + 1.0;
				ipe = dse;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (ips <= ipe)
					append_line_pdir(wline, line, ips, ipe, TRUE);
				}
			}

		/* Special handling for entire dividing lines */
		/* >>>>> should never get to here!!!!! <<<<< */
		else if (alink->ltype == AreaDiv)
			{
			float	dx, dy, angc, dang;
			LOGICAL	flip;

			pr_error("Interp.Areas", "Dividing line without segments!!!!!\n");

			line  = aikey->line;
			wline = copy_line(line);

			/* Figure out sense of divline and reverse if needed */
			np   = wline->numpts;
			dx   = wline->points[np-1][X] - wline->points[0][X];
			dy   = wline->points[np-1][Y] - wline->points[0][Y];
			angc = atan2deg(dy, dx);
			flip = FALSE;
			if (ikey > skey)
				{
				dang = fabs(angp - angc);
				if (dang > 180) dang = 360 - dang;
				flip = (LOGICAL) (dang > 90);
				}
			if (flip) reverse_line(wline);
			aikey->flip = flip;
			angp = angc;
			}

		/* Special handling for segmented holes */
		if (alink->ltype == AreaHole && doseg)
			{
			line  = aikey->line;
			np    = line->numpts;
			nseg  = aikey->nseg;
			wline = create_line();
			jseg  = (iseg < nseg-1)? iseg + 1: 0;
			dss   = aikey->dspan[iseg];
			dse   = aikey->dspan[jseg];
			if (aikey->hcw)
				{
				ips = dss + 1.0;
				ipe = dse;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (dss < dse)
					{
					append_line_pdir(wline, line, ips, ipe, TRUE);
					}
				else
					{
					append_line_pdir(wline, line, ips, np-1, TRUE);
					append_line_pdir(wline, line, 0,   ipe,  TRUE);
					}
				}
			else
				{
				ips = dss;
				ipe = dse + 1.0;
				add_point_to_line(wline, aikey->dspt[iseg]);
				if (dse < dss)
					{
					append_line_pdir(wline, line, ipe, ips, FALSE);
					}
				else
					{
					append_line_pdir(wline, line, 0,   ips,  FALSE);
					append_line_pdir(wline, line, ipe, np-1, FALSE);
					}
				}
#				ifdef EFF
					if (!wcurv)
						{
						wcurv = create_curve("", "", "");
						define_lspec(&wcurv->lspec, 0, 0, NULL, FALSE,
									1.0, 2.0, (HILITE) 2);
						}
					wcurv->line = wline;
					gxSetupTransform(DnMap);
					glClipMode(glCLIP_ON);
					display_curve(wcurv);
					glClipMode(glCLIP_OFF);
					sync_display();
					wcurv->line = NullLine;
#				endif /* EFF */
			}

		/* Special handling for entire holes */
		else if (alink->ltype == AreaHole)
			{
			line  = aikey->line;
			wline = copy_line(line);
			line_properties(wline, NullLogical, &hcw, NullFloat, NullFloat);

			if (!hcw) reverse_line(wline);
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
		line_properties(wline, NullLogical, NullLogical, NullFloat, &length);
		spres = length / (wline->numpts - 1.1);

#		ifdef DEBUG_INTERP_SPATIAL_POINTS
		pr_diag("Interp.Points", "        Working line: %d %.2f %.3f\n",
			wline->numpts, length, spres);
#		endif /* DEBUG_INTERP_SPATIAL_POINTS */

		reset_pipe();
		enable_filter(spres, 0.0);
		enable_spline(spres, closed, 0.0, 0.0, 0.0);
		enable_save();
		line_pipe(wline);
		nlines = recall_save(&lines);
		wline  = destroy_line(wline);
		wline  = copy_line(lines[0]);
		line_properties(wline, NullLogical, NullLogical, NullFloat, &slen);
		length = slen;
		sprev  = 0.0;
		spmin  = 0.0;
		spmax  = 0.0;

		/* Set up initial spline operation with resolution that */
		/* should generate the same number of points in each line */
		spseg = copy_line(wline);
		spres = length / (nspts - 1.1);

#		ifdef DEBUG_INTERP_SPATIAL_POINTS
		pr_diag("Interp.Points", "        Spline line:  %d %.2f %.3f\n",
			nspts, length, spres);
#		endif /* DEBUG_INTERP_SPATIAL_POINTS */

Respline:	/* Spline the boundary line to the desired resolution */
		reset_pipe();
		enable_filter(spres, 0.0);
		enable_spline(spres, closed, 0.0, 0.0, 0.0);
		enable_save();
		line_pipe(spseg);
		nlines = recall_save(&lines);
		spseg  = destroy_line(spseg);
		spseg  = copy_line(lines[0]);
		line_properties(spseg, NullLogical, NullLogical, NullFloat, &slen);
		lspts = spseg->numpts;

#		ifdef DEBUG_INTERP_SPATIAL_POINTS
		pr_diag("Interp.Points",
			"   Resplined area points: %d %d\n", lspts, nspts);
		if (lspts > 0)
			{
			pr_diag("Interp.Points", "   Resplined area positions -\n");
			for (isp=0; isp<lspts; isp++)
				pr_diag("Interp.Points", "    Position: %.3d  %.2f %.2f\n",
						isp, spseg->points[isp][X], spseg->points[isp][Y]);
			}
#		endif /* DEBUG_INTERP_SPATIAL_POINTS */

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
			spres  = length / (nspts - 1.1);
			goto Respline;
			}

		/* If segment is closed find reference point closest to link node */
		if (!closed) itop = 0;
		else
			{
			itop = line_closest_point(spseg, aikey->lpos, NULL, NULL);
			}

		/* Copy splined line into working buffer */
		ip = itop;
		for (isp=0; isp<nspts; isp++)
			{
			keyx[isp][ikey] = spseg->points[ip][X];
			keyy[isp][ikey] = spseg->points[ip][Y];
			if (++ip >= nspts) ip = 0;
			}

		/* Get rid of temporary segment till next frame */
		spseg = destroy_line(spseg);
		wline = destroy_line(wline);

#		ifdef DEBUG_INTERP_SPATIAL_POINTS
		pr_diag("Interp.Points",
			"   Spatial area points: %d\n", nspts);
		if (nspts > 0)
			{
			pr_diag("Interp.Points", "   Spatial area positions -\n");
			for (isp=0; isp<nspts; isp++)
				pr_diag("Interp.Points", "    Position: %.3d  %.2f %.2f\n",
						isp, keyx[isp][ikey], keyy[isp][ikey]);
			}
#		endif /* DEBUG_INTERP_SPATIAL_POINTS */

		/* End of spatial spline loop (ikey) */
		}
	}

/***********************************************************************
*                                                                      *
*    f r e e _ a r e a _ l i n k s                                     *
*    m a k e _ a r e a _ l i n k s                                     *
*                                                                      *
*    Construct area link information                                   *
*                                                                      *
***********************************************************************/

static	int	free_area_links(void)

	{
	int		ialink, ikey;
	ALINK	*alink;
	ALKEY	*aikey;

	/* Empty area link buffer */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;
		if (NotNull(alink))
			{
			if (NotNull(alink->key))
				{
				for (ikey=0; ikey<NumKey; ikey++)
					{
					aikey = alink->key + ikey;
					if (NotNull(aikey))
						{
						aikey->sids = NullInt;
						FREEMEM(aikey->dseg);
						FREEMEM(aikey->dspan);
						FREEMEM(aikey->dspt);
						}
					}
				}
			FREEMEM(alink->common);
			FREEMEM(alink->key);
			(void) free_control_list(alink->ctrl);
			FREEMEM(alink->iaout);
			}
		}
	FREEMEM(ALinks);
	nALink = 0;

	return nALink;
	}

/**********************************************************************/

static	int		make_area_links

	(
	DFLIST	*dfld
	)

	{
	int		ialink, iblink, ilink, imlink, islink, ikey, jkey, ivt;
	int		inode, imem, iarea, itween;
	AMEMBER	mtype;
	float	msize, ssize, msum, ssum, mfact, sfact;
	LOGICAL	mlook, slook, mfound, sfound, cfound;
	FIELD	fld;
	SET		set;
	AREA	area;
	SUBAREA	lsa, rsa;
	ALINK	*alink, *blink;
	ALKEY	*aikey, *ajkey, *bikey, *bjkey;
	LCHAIN	chain;
	LOGICAL	lfrst;
	LINE	line;
	LOGICAL	cw, hcw;
	int		nseg, iseg, jseg, rseg, ispan, jspan, itop, jtop, np;
	LOGICAL	extrev, rcw, first, orientation, atstart, rflip;
	POINT	tpos, ipos, jpos;
	float	rel, dsl, dst, dspan, dsrel, length, slen, xa, xb, ya, yb, x, y;
	float	dtop, dbot, dss, dse;
	TSTAMP	vts, vte;

	/* Empty old area link buffer */
	(void) free_area_links();

	/* Allocate area link buffer */
	nALink = dfld->nchain;
	ALinks = INITMEM(ALINK, nALink);

	/* Save area link information in buffer */
	for (ialink=0; ialink<nALink; ialink++)
		{

#		ifdef DEBUG_INTERP_LINKS
		pr_diag("make_area_links()",
			"Interpolated link chain: %d  (%s %s)\n",
			ialink, dfld->element, dfld->level);
#		endif /* DEBUG_INTERP_LINKS */

		chain = dfld->chains[ialink];
		(void) interpolate_lchain(chain);

		alink = ALinks + ialink;
		alink->ltype  = AreaNone;
		alink->skey   = -1;
		alink->ekey   = -1;
		alink->splus  = chain->splus;
		alink->eplus  = chain->eplus;
		alink->mflag  = FALSE;
		alink->sflag  = FALSE;
		alink->icom   = ialink;
		alink->ncom   = 0;
		alink->common = NullInt;
		alink->ocom   = ialink;
		alink->key    = INITMEM(ALKEY, NumKey);
		alink->ctrl   = build_control_list(chain, 0);
		alink->iaout  = INITMEM(int, NumTween);
		for (itween=0; itween<NumTween; itween++)
			alink->iaout[itween] = -1;

		/* Build list of areas in the current link chain */
		lfrst = TRUE;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			ivt   = KeyTime[ikey];

			aikey->iarea = -1;
			aikey->mtype = AreaNone;
			aikey->link  = FALSE;
			aikey->imem  = -1;
			aikey->ndiv  = -1;
			aikey->sids  = NullInt;
			aikey->nhole = -1;
			aikey->cfact = 1.0;
			aikey->mfact = 1.0;
			aikey->sfact = 1.0;
			aikey->area  = NullArea;
			aikey->line  = NullLine;
			aikey->nseg  = 0;
			aikey->dseg  = NullInt;
			aikey->dspan = NullFloat;
			aikey->dspt  = NullPointList;
			aikey->cw    = TRUE;
			aikey->flip  = FALSE;
			aikey->hcw   = TRUE;
			aikey->lsub  = NULL;
			aikey->lval  = NULL;
			aikey->llab  = NULL;
			aikey->lcal  = NullCal;
			aikey->rsub  = NULL;
			aikey->rval  = NULL;
			aikey->rlab  = NULL;
			aikey->rcal  = NullCal;
			set_point(aikey->lpos, -1.0, -1.0);

			/* >>>>> replace this ...
			if (!chain->nodes[ivt]->there) continue;
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, TimeList[ivt].mplus);
			if (inode < 0)                   continue;
			if (!chain->nodes[inode]->there) continue;

			/* Extract the set that corresponds to the current keyframe */
			fld = dfld->fields[ivt];
			if (!fld)                     continue;
			if (fld->ftype != FtypeSet)   continue;
			if (!fld->data.set)           continue;
			set = fld->data.set;
			if (!same(set->type, "area")) continue;

			/* Extract the area on the current link chain */
			iarea = chain->nodes[inode]->attach;
			if (iarea < 0) continue;
			area  = (AREA) set->list[iarea];
			mtype = chain->nodes[inode]->mtype;
			imem  = chain->nodes[inode]->imem;
			build_area_subareas(area);
			area_properties(area, &cw, NullFloat, NullFloat);
			aikey->iarea = iarea;
			aikey->mtype = mtype;
			aikey->link  = TRUE;
			aikey->imem  = imem;
			aikey->area  = area;
			aikey->cw    = cw;
			copy_point(aikey->lpos, chain->nodes[inode]->node);
			switch (mtype)
				{
				case AreaBound:
					aikey->line  = area->bound->boundary;
					aikey->ndiv  = area->numdiv;
					aikey->sids  = area->subids;
					aikey->nhole = area->bound->numhole;
					break;

				case AreaDiv:
					if (imem < 0)             break;
					if (imem >= area->numdiv) break;
					adjacent_subareas(area, imem, &lsa, &rsa);
					aikey->line = area->divlines[imem];
					aikey->flip = FALSE;
					aikey->lsub = lsa->subelem;
					aikey->lval = lsa->value;
					aikey->llab = lsa->label;
					aikey->lcal = lsa->attrib;
					aikey->rsub = rsa->subelem;
					aikey->rval = rsa->value;
					aikey->rlab = rsa->label;
					aikey->rcal = rsa->attrib;
					break;

				case AreaHole:
					if (imem < 0)                     break;
					if (imem >= area->bound->numhole) break;
					line = area->bound->holes[imem];
					line_properties(line, NullLogical, &hcw,
									NullFloat, NullFloat);
					aikey->line = line;
					aikey->hcw  = hcw;
					break;
				}

			/* Keep track of active part of link chain */
			if (alink->skey < 0) alink->skey = ikey;
			alink->ekey = ikey;

			/* Keep track of type of feature linked */
			if (lfrst)
				{
				alink->ltype = aikey->mtype;
				lfrst = FALSE;
				}
			else if (aikey->mtype != alink->ltype)
				{
				alink->ltype = AreaNone;
				}
			}
		}

#	ifdef DEBUG_INTERP_LINKS
	pr_diag("Interp.Links", "Area link chains for: %s %s  Links: %d\n",
		dfld->element, dfld->level, nALink);
	if (nALink > 0)
		{
		for (ialink=0; ialink<nALink; ialink++)
			{
			alink = ALinks + ialink;
			switch (alink->ltype)
				{
				case AreaBound:
					pr_diag("Interp.Links",
						" Link chain: %d  Type: Boundary\n", ialink);
					break;
				case AreaDiv:
					pr_diag("Interp.Links",
						" Link chain: %d  Type: Divide\n", ialink);
					break;
				case AreaHole:
					pr_diag("Interp.Links",
						" Link chain: %d  Type: Hole\n", ialink);
					break;
				case AreaNone:
				default:
					pr_diag("Interp.Links",
						" Link chain: %d  Type: Unknown\n", ialink);
				}
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				switch (aikey->mtype)
					{
					case AreaBound:
						pr_diag("Interp.Links",
							"  Key: %d  Area: %d\n", ikey, aikey->iarea);
						break;
					case AreaDiv:
						pr_diag("Interp.Links",
							"  Key: %d  Area: %d  Divide: %d (of %d)\n",
							ikey, aikey->iarea, aikey->imem, aikey->ndiv);
						break;
					case AreaHole:
						pr_diag("Interp.Links",
							"  Key: %d  Area: %d  Hole: %d (of %d)\n",
							ikey, aikey->iarea, aikey->imem, aikey->nhole);
						break;
					case AreaNone:
					default:
						pr_diag("Interp.Links",
							"  Key: %d  Area: %d\n", ikey, aikey->iarea);
					}
				}
			}
		}
#	endif /* DEBUG_INTERP_LINKS */

	/* Find out if any area boundaries have multiple link chains */
	/* First check only for areas that merge or split! */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only check area boundaries */
		if (alink->ltype != AreaBound) continue;

		mfound = FALSE;
		sfound = FALSE;

		/* Check in each keyframe */
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			if (!aikey->link)              continue;

			/* Only check area boundaries */
			if (aikey->mtype != AreaBound) continue;

			/* See if there is a possibility for merge or split */
			mlook = (LOGICAL) (ikey > 0        && ikey < alink->ekey
								&& (aikey-1)->link && aikey->mfact == 1.0);
			slook = (LOGICAL) (ikey < NumKey-1 && ikey > alink->skey
								&& (aikey+1)->link && aikey->sfact == 1.0);

#			ifdef DEBUG_INTERP_LINKS
			if (mlook)
				pr_diag("Interp.Links",
					"Checking merge for Link: %d  Key: %d  Area: %d\n",
					ialink, ikey, aikey->iarea);
			else
				pr_diag("Interp.Links",
					"Skipping merge for Link: %d  Key: %d  Area: %d  Size: %.2f\n",
					ialink, ikey, aikey->iarea, aikey->mfact);
			if (slook)
				pr_diag("Interp.Links",
					"Checking split for Link: %d  Key: %d  Area: %d\n",
					ialink, ikey, aikey->iarea);
			else
				pr_diag("Interp.Links",
					"Skipping split for Link: %d  Key: %d  Area: %d  Size: %.2f\n",
					ialink, ikey, aikey->iarea, aikey->sfact);
#			endif /* DEBUG_INTERP_LINKS */

			if (!mlook && !slook) continue;

			mfound = FALSE;
			sfound = FALSE;

			/* See if another chain has the same area */
			iarea = aikey->iarea;
			msum  = 0.0;
			ssum  = 0.0;
			for (iblink=0; iblink<nALink; iblink++)
				{
				if (iblink == ialink) continue;
				blink = ALinks + iblink;
				bikey = blink->key + ikey;

				/* Only check area boundaries */
				if (bikey->mtype != AreaBound) continue;
				if (bikey->iarea != iarea)     continue;

				/* Area is on another chain */
				/* See if it merges or splits */
				if (mlook && (bikey-1)->link &&
					(bikey-1)->iarea != (aikey-1)->iarea &&
					(bikey-1)->line)
					{
					line_properties((bikey-1)->line, NullLogical, NullLogical,
									NullFloat, &msize);
					msum  += msize;
					mfound = TRUE;
					}
				if (slook && (bikey+1)->link &&
					(bikey+1)->iarea != (aikey+1)->iarea &&
					(bikey+1)->line)
					{
					line_properties((bikey+1)->line, NullLogical, NullLogical,
									NullFloat, &ssize);
					ssum  += ssize;
					sfound = TRUE;
					}
				}

			if (!mfound && !sfound) continue;

			/* Compute scale factor for merge and split */
			if (mfound)
				{
				line_properties((aikey-1)->line, NullLogical, NullLogical,
								NullFloat, &msize);
				msum += msize;
				mfact = msize/msum;
				}
			if (sfound)
				{
				line_properties((aikey+1)->line, NullLogical, NullLogical,
								NullFloat, &ssize);
				ssum += ssize;
				sfact = ssize/ssum;
				}

			/* Exit ikey loop so we can break the link chain into */
			/* parts before and after merge/split */
			break;
			}

		imlink = ialink;
		islink = ialink;

		/* Break link chain into parts before and after merge */
		if (mfound)
			{
			/* Add another chain for the second part */
			nALink++;
			ALinks = GETMEM(ALinks, ALINK, nALink);
			iblink = nALink - 1;
			blink  = ALinks + iblink;
			alink  = ALinks + imlink;

#			ifdef DEBUG_INTERP_LINKS
			pr_diag("Interp.Links",
				"  Link chain merge: %d and %d\n", imlink, iblink);
#			endif /* DEBUG_INTERP_LINKS */

			/* Set flag for areas that merge */
			alink->mflag  = TRUE;

			/* Copy portion of link chain after merge */
			blink->ltype  = AreaBound;
			blink->skey   = ikey;
			blink->ekey   = alink->ekey;
			blink->splus  = KeyPlus[ikey];
			blink->eplus  = alink->eplus;
			blink->mflag  = alink->mflag;
			blink->icom   = iblink;
			blink->ncom   = 0;
			blink->common = NullInt;
			blink->ocom   = alink->ocom;
			blink->key    = INITMEM(ALKEY, NumKey);
			blink->ctrl   = copy_control_list(alink->ctrl);
			blink->iaout  = INITMEM(int, NumTween);
			for (itween=0; itween<NumTween; itween++)
				blink->iaout[itween] = -1;
			for (jkey=0; jkey<ikey; jkey++)
				{
				bjkey = blink->key + jkey;

				bjkey->iarea = -1;
				bjkey->mtype = AreaNone;
				bjkey->link  = FALSE;
				bjkey->imem  = -1;
				bjkey->ndiv  = -1;
				bjkey->sids  = NullInt;
				bjkey->nhole = -1;
				bjkey->cfact = 1.0;
				bjkey->mfact = 1.0;
				bjkey->sfact = 1.0;
				bjkey->area  = NullArea;
				bjkey->line  = NullLine;
				bjkey->nseg  = 0;
				bjkey->dseg  = NullInt;
				bjkey->dspan = NullFloat;
				bjkey->dspt  = NullPointList;
				bjkey->cw    = TRUE;
				bjkey->flip  = FALSE;
				bjkey->hcw   = TRUE;
				bjkey->lsub  = NULL;
				bjkey->lval  = NULL;
				bjkey->llab  = NULL;
				bjkey->lcal  = NullCal;
				bjkey->rsub  = NULL;
				bjkey->rval  = NULL;
				bjkey->rlab  = NULL;
				bjkey->rcal  = NullCal;
				set_point(bjkey->lpos, -1.0, -1.0);
				}
			for (jkey=ikey; jkey<NumKey; jkey++)
				{
				ajkey = alink->key + jkey;
				bjkey = blink->key + jkey;

				bjkey->iarea = ajkey->iarea;
				bjkey->mtype = ajkey->mtype;
				bjkey->link  = ajkey->link;
				bjkey->imem  = ajkey->imem;
				bjkey->ndiv  = ajkey->ndiv;
				bjkey->sids  = ajkey->sids;
				bjkey->nhole = ajkey->nhole;
				bjkey->cfact = ajkey->cfact;
				bjkey->mfact = ajkey->mfact;
				bjkey->sfact = ajkey->sfact;
				bjkey->area  = ajkey->area;
				bjkey->line  = ajkey->line;
				bjkey->nseg  = 0;
				bjkey->dseg  = NullInt;
				bjkey->dspan = NullFloat;
				bjkey->dspt  = NullPointList;
				bjkey->cw    = ajkey->cw;
				bjkey->flip  = ajkey->flip;
				bjkey->hcw   = ajkey->hcw;
				bjkey->lsub  = ajkey->lsub;
				bjkey->lval  = ajkey->lval;
				bjkey->llab  = ajkey->llab;
				bjkey->lcal  = ajkey->lcal;
				bjkey->rsub  = ajkey->rsub;
				bjkey->rval  = ajkey->rval;
				bjkey->rlab  = ajkey->rlab;
				bjkey->rcal  = ajkey->rcal;
				copy_point(bjkey->lpos, ajkey->lpos);
				}

			/* Remove portion of current link chain after merge */
			alink->ekey  = ikey;
			alink->eplus = KeyPlus[ikey];
			alink->key[ikey].mfact = mfact;
			for (jkey=ikey+1; jkey<NumKey; jkey++)
				{
				ajkey = alink->key + jkey;

				ajkey->iarea = -1;
				ajkey->mtype = AreaNone;
				ajkey->link  = FALSE;
				ajkey->imem  = -1;
				ajkey->ndiv  = -1;
				ajkey->sids  = NullInt;
				ajkey->nhole = -1;
				ajkey->cfact = 1.0;
				ajkey->mfact = 1.0;
				ajkey->sfact = 1.0;
				ajkey->area  = NullArea;
				ajkey->line  = NullLine;
				aikey->nseg  = 0;
				ajkey->dseg  = NullInt;
				ajkey->dspan = NullFloat;
				ajkey->dspt  = NullPointList;
				ajkey->cw    = TRUE;
				ajkey->flip  = FALSE;
				ajkey->hcw   = TRUE;
				ajkey->lsub  = NULL;
				ajkey->lval  = NULL;
				ajkey->llab  = NULL;
				ajkey->lcal  = NullCal;
				ajkey->rsub  = NULL;
				ajkey->rval  = NULL;
				ajkey->rlab  = NULL;
				ajkey->rcal  = NullCal;
				set_point(ajkey->lpos, -1.0, -1.0);
				}
			}

		/* Break link chain into parts before and after split */
		else if (sfound)
			{
			/* Add another chain for the second part */
			nALink++;
			ALinks = GETMEM(ALinks, ALINK, nALink);
			iblink = nALink - 1;
			blink  = ALinks + iblink;
			alink  = ALinks + islink;

#			ifdef DEBUG_INTERP_LINKS
			pr_diag("Interp.Links",
				"  Link chain split: %d and %d\n", islink, iblink);
#			endif /* DEBUG_INTERP_LINKS */

			/* Set flag for an area that splits */
			alink->sflag  = TRUE;

			/* Copy portion of link chain after split */
			blink->ltype  = AreaBound;
			blink->skey   = ikey;
			blink->ekey   = alink->ekey;
			blink->splus  = KeyPlus[ikey];
			blink->eplus  = alink->eplus;
			blink->sflag  = alink->sflag;
			blink->icom   = iblink;
			blink->ncom   = 0;
			blink->common = NullInt;
			blink->ocom   = alink->ocom;
			blink->key    = INITMEM(ALKEY, NumKey);
			blink->ctrl   = copy_control_list(alink->ctrl);
			blink->iaout  = INITMEM(int, NumTween);
			for (itween=0; itween<NumTween; itween++)
				blink->iaout[itween] = -1;
			for (jkey=0; jkey<ikey; jkey++)
				{
				bjkey = blink->key + jkey;

				bjkey->iarea = -1;
				bjkey->mtype = AreaNone;
				bjkey->link  = FALSE;
				bjkey->imem  = -1;
				bjkey->ndiv  = -1;
				bjkey->sids  = NullInt;
				bjkey->nhole = -1;
				bjkey->cfact = 1.0;
				bjkey->mfact = 1.0;
				bjkey->sfact = 1.0;
				bjkey->area  = NullArea;
				bjkey->line  = NullLine;
				bjkey->nseg  = 0;
				bjkey->dseg  = NullInt;
				bjkey->dspan = NullFloat;
				bjkey->dspt  = NullPointList;
				bjkey->cw    = TRUE;
				bjkey->flip  = FALSE;
				bjkey->hcw   = TRUE;
				bjkey->lsub  = NULL;
				bjkey->lval  = NULL;
				bjkey->llab  = NULL;
				bjkey->lcal  = NullCal;
				bjkey->rsub  = NULL;
				bjkey->rval  = NULL;
				bjkey->rlab  = NULL;
				bjkey->rcal  = NullCal;
				set_point(bjkey->lpos, -1.0, -1.0);
				}
			for (jkey=ikey; jkey<NumKey; jkey++)
				{
				ajkey = alink->key + jkey;
				bjkey = blink->key + jkey;

				bjkey->iarea = ajkey->iarea;
				bjkey->mtype = ajkey->mtype;
				bjkey->link  = ajkey->link;
				bjkey->imem  = ajkey->imem;
				bjkey->ndiv  = ajkey->ndiv;
				bjkey->sids  = ajkey->sids;
				bjkey->nhole = ajkey->nhole;
				bjkey->cfact = ajkey->cfact;
				bjkey->mfact = ajkey->mfact;
				bjkey->sfact = ajkey->sfact;
				bjkey->area  = ajkey->area;
				bjkey->line  = ajkey->line;
				bjkey->nseg  = 0;
				bjkey->dseg  = NullInt;
				bjkey->dspan = NullFloat;
				bjkey->dspt  = NullPointList;
				bjkey->cw    = ajkey->cw;
				bjkey->flip  = ajkey->flip;
				bjkey->hcw   = ajkey->hcw;
				bjkey->lsub  = ajkey->lsub;
				bjkey->lval  = ajkey->lval;
				bjkey->llab  = ajkey->llab;
				bjkey->lcal  = ajkey->lcal;
				bjkey->rsub  = ajkey->rsub;
				bjkey->rval  = ajkey->rval;
				bjkey->rlab  = ajkey->rlab;
				bjkey->rcal  = ajkey->rcal;
				copy_point(bjkey->lpos, ajkey->lpos);
				}
			blink->key[ikey].sfact = sfact;

			/* Remove portion of current link chain after split */
			alink->ekey  = ikey;
			alink->eplus = KeyPlus[ikey];
			for (jkey=ikey+1; jkey<NumKey; jkey++)
				{
				ajkey = alink->key + jkey;

				ajkey->iarea = -1;
				ajkey->mtype = AreaNone;
				ajkey->link  = FALSE;
				ajkey->imem  = -1;
				ajkey->ndiv  = -1;
				ajkey->sids  = NullInt;
				ajkey->nhole = -1;
				ajkey->cfact = 1.0;
				ajkey->mfact = 1.0;
				ajkey->sfact = 1.0;
				ajkey->area  = NullArea;
				ajkey->line  = NullLine;
				ajkey->nseg  = 0;
				ajkey->dseg  = NullInt;
				ajkey->dspan = NullFloat;
				ajkey->dspt  = NullPointList;
				ajkey->cw    = TRUE;
				ajkey->flip  = FALSE;
				ajkey->hcw   = TRUE;
				ajkey->lsub  = NULL;
				ajkey->lval  = NULL;
				ajkey->llab  = NULL;
				ajkey->lcal  = NullCal;
				ajkey->rsub  = NULL;
				ajkey->rval  = NULL;
				ajkey->rlab  = NULL;
				ajkey->rcal  = NullCal;
				set_point(ajkey->lpos, -1.0, -1.0);
				}
			}

#		ifdef DEBUG_INTERP_LINKS
		pr_diag("Interp.Links",
			" Link chain: %d  Type: Boundary\n", ialink);
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			pr_diag("Interp.Links",
				"  Key: %d  Area: %d\n", ikey, aikey->iarea);
			}
#		endif /* DEBUG_INTERP_LINKS */
		}

	/* Find out if any area boundaries have multiple link chains */
	/* Note that areas that merge or split are done above! */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Number the initial link chain (for control nodes) */
		ilink = 0;

		/* Only check area boundaries */
		if (alink->ltype != AreaBound) continue;

		/* See if another chain has the same area on it */
		for (iblink=ialink+1; iblink<nALink; iblink++)
			{
			blink = ALinks + iblink;

			/* Only check area boundaries */
			if (blink->ltype != AreaBound) continue;

			/* Already found? */
			if (blink->icom != iblink)     continue;

			/* Check in each keyframe */
			/* Common if the area is shared in all co-existing keyframes */
			cfound = FALSE;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaBound) continue;
				if (bikey->mtype != AreaBound) continue;
				if (aikey->iarea < 0)          continue;
				if (bikey->iarea < 0)          continue;

				if (aikey->iarea != bikey->iarea)
					{
					cfound = FALSE;
					break;
					}

				/* Disregard if they only co-exist at joined ends */
				if (ikey > 0 && ikey < NumKey-1)
					{
					if (    (aikey-1)->link && !(aikey+1)->link
						&& !(bikey-1)->link &&  (bikey+1)->link)
						{
						cfound = FALSE;
						break;
						}
					if (   !(aikey-1)->link &&  (aikey+1)->link
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

#				ifdef DEBUG_INTERP_LINKS
				pr_diag("Interp.Links",
					"  Link chain: %d connected with %d\n", iblink, ialink);
#				endif /* DEBUG_INTERP_LINKS */

				/* Identify this link as connected */
				blink->icom = alink->icom;

				/* Save the earliest start and latest end times */
				alink->skey  = MIN(alink->skey, blink->skey);
				alink->ekey  = MAX(alink->ekey, blink->ekey);
				alink->splus = MIN(alink->splus, blink->splus);
				alink->eplus = MAX(alink->eplus, blink->eplus);

				/* Save the control node information */
				/* Associate control nodes with the next link chain */
				ilink++;
				(void) add_nodes_to_control_list(alink->ctrl, ilink,
																blink->ctrl);

				/* Share area info over non-common frames to help */
				/* generate pseudo-links */
				for (ikey=0; ikey<NumKey; ikey++)
					{
					aikey = alink->key + ikey;
					bikey = blink->key + ikey;

					if (aikey->iarea < 0 && bikey->iarea >= 0)
						{
						if (bikey->mtype != AreaBound) continue;
						aikey->link  = FALSE;
						aikey->mtype = bikey->mtype;
						aikey->iarea = bikey->iarea;
						aikey->imem  = bikey->imem;
						aikey->area  = bikey->area;
						aikey->line  = bikey->line;
						}

					else if (bikey->iarea < 0 && aikey->iarea >= 0)
						{
						if (aikey->mtype != AreaBound) continue;
						bikey->link  = FALSE;
						bikey->mtype = aikey->mtype;
						bikey->iarea = aikey->iarea;
						bikey->imem  = aikey->imem;
						bikey->area  = aikey->area;
						bikey->line  = aikey->line;
						}
					}
				}
			}
		}

	/* Generate list of common-chain segments in each keyframe */
	/* Create pseudo-links where required */
	tpos[X] = MapProj->definition.xorg + .5*MapProj->definition.xlen;
	tpos[Y] = MapProj->definition.yorg + 10*MapProj->definition.ylen;
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only check area boundaries */
		if (alink->ltype != AreaBound) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)    continue;

		alink->ncom   = 0;
		alink->common = (int *)0;

		/* Initialize segments in each keyframe */
		extrev = FALSE;
		rel    = -1;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			if (aikey->mtype != AreaBound) continue;

			/* Define a segment for the whole area, the first time */
			aikey->nseg  = 1;
			aikey->dseg  = GETMEM(aikey->dseg,  int,   1);
			aikey->dspan = GETMEM(aikey->dspan, float, 1);
			aikey->dspt  = GETMEM(aikey->dspt,  POINT, 1);

			/* This is an active node */
			if (aikey->link)
				{

				/* Define the segment relative to the first link node */
				/* and compute position of node relative to top of area */
				line = aikey->line;
				np   = line->numpts;
				line_test_point(line, aikey->lpos, NullFloat,
								ipos, &ispan, NullChar, NullChar);
				dsl = point_dist(line->points[ispan], ipos);
				dst = point_dist(line->points[ispan], line->points[ispan+1]);
				dspan = ispan + dsl/dst;
				aikey->dseg[0]  = 0;
				aikey->dspan[0] = dspan;
				copy_point(aikey->dspt[0], aikey->lpos);
				itop = line_closest_point(line, tpos, NullFloat, NullPoint);
				if (aikey->cw)
					{
					if (ispan >= itop) rel = (dspan - itop) / (np-1);
					else               rel = (dspan + np-1 - itop) / (np-1);
					}
				else
					{
					if (ispan <= itop) rel = (itop - dspan) / (np-1);
					else               rel = (itop + np-1 - dspan) / (np-1);
					}

				/* Extend pseudo-links back in time if required */
				/* (First active node is not at the first time) */
				/* (Do this once for all times before ikey time) */
				if (!extrev)
					{
					for (jkey=0; jkey<ikey; jkey++)
						{
						ajkey = alink->key + jkey;
						if (ajkey->iarea < 0)    continue;
						if (IsNull(ajkey->line)) continue;
						line  = ajkey->line;
						np    = line->numpts;
						jtop  = line_closest_point(line, tpos, NullFloat,
								NullPoint);
						/* Use value of rel (from above) to set location */
						if (ajkey->cw)
							{
							dspan = jtop + (np-1)*rel;
							if (dspan >= np-1) dspan -= np-1;
							}
						else
							{
							dspan = jtop - (np-1)*rel;
							if (dspan < 0) dspan += np-1;
							}
						jspan = dspan;
						dsrel = dspan - jspan;
						/* >>>>> not sure if there was a problem here? <<<<< */
						xa = line->points[jspan][X];
						xb = line->points[jspan+1][X];
						ya = line->points[jspan][Y];
						yb = line->points[jspan+1][Y];
						x  = xa + dsrel*(xb-xa);
						y  = ya + dsrel*(yb-ya);
						ajkey->dseg[0]  = 0;
						ajkey->dspan[0] = dspan;
						set_point(ajkey->dspt[0], x, y);
						}
					extrev = TRUE;
					}
				}

			/* Not an active node */
			/* Extend pseudo-links forward in time if required */
			/* (Current is not active, but follows an active one) */
			/* (Use rel from last active node to set locations) */
			else if (rel >= 0)
				{
				if (aikey->iarea < 0)    continue;
				if (IsNull(aikey->line)) continue;
				line  = aikey->line;
				np    = line->numpts;
				itop  = line_closest_point(line, tpos, NullFloat,
						NullPoint);
				if (aikey->cw)
					{
					dspan = itop + (np-1)*rel;
					if (dspan >= np-1) dspan -= np-1;
					}
				else
					{
					dspan = itop - (np-1)*rel;
					if (dspan < 0) dspan += np-1;
					}
				ispan = dspan;
				dsrel = dspan - ispan;
				xa = line->points[ispan][X];
				xb = line->points[ispan+1][X];
				ya = line->points[ispan][Y];
				yb = line->points[ispan+1][Y];
				x  = xa + dsrel*(xb-xa);
				y  = ya + dsrel*(yb-ya);
				aikey->dseg[0]  = 0;
				aikey->dspan[0] = dspan;
				set_point(aikey->dspt[0], x, y);
				}
			}

		/* Find any common chains */
		for (iblink=0; iblink<nALink; iblink++)
			{
			if (iblink == ialink)           continue;
			blink = ALinks + iblink;
			if (blink->ltype != AreaBound)  continue;
			if (blink->icom  != ialink)     continue;

			/* Save the common link */
			alink->ncom++;
			alink->common = GETMEM(alink->common, int, alink->ncom);
			alink->common[alink->ncom-1] = iblink;

			/* Divide segments in each keyframe with new nodes */
			extrev = FALSE;
			rel    = -1;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaBound)    continue;
				if (bikey->mtype != AreaBound)    continue;

				/* This is an active node */
				if (bikey->link)
					{
					if (bikey->iarea != aikey->iarea) continue;

					/* Find which segment contains this node */
					/* so we can divide it */
					line = aikey->line;
					np   = line->numpts;
					line_test_point(line, bikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);

					/* >>>>> print ispan and np??? <<<<< */

					dsl = point_dist(line->points[ispan], ipos);
					dst = point_dist(line->points[ispan],
								line->points[ispan+1]);
					dspan = ispan + dsl/dst;
					nseg = aikey->nseg;
					for (iseg=0; iseg<nseg-1; iseg++)
						{
						dss = aikey->dspan[iseg];
						dse = aikey->dspan[iseg+1];
						if (aikey->cw)
							{
							if (dss <= dse)
								{
								if (dss <= dspan && dspan < dse) break;
								}
							else
								{
								if (dss <= dspan || dspan < dse) break;
								}
							}
						else
							{
							if (dss >= dse)
								{
								if (dss >= dspan && dspan > dse) break;
								}
							else
								{
								if (dss >= dspan || dspan > dse) break;
								}
							}
						}

					/* Break the segment that contains it (dspan[iseg]) */
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>iseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					aikey->dseg[iseg+1]  = nseg-1;
					aikey->dspan[iseg+1] = dspan;
					copy_point(aikey->dspt[iseg+1], bikey->lpos);
					jseg = (iseg < nseg-2)? iseg + 2: 0;
					dss  = aikey->dspan[iseg];
					dse  = aikey->dspan[jseg];
					if (aikey->cw)
						{
						dtop = dspan - dss;	if (dtop <  0) dtop += np-1;
						dbot = dse - dss;	if (dbot <= 0) dbot += np-1;
						}
					else
						{
						dtop = dspan - dse;	if (dtop <  0) dtop += np-1;
						dbot = dss - dse;	if (dbot <= 0) dbot += np-1;
						}
					rel  = dtop / dbot;
					rseg = iseg;
					rcw  = aikey->cw;

					/* Extend pseudo-links back in time if required */
					/* (First active node is not at the first time) */
					/* (Do this once for all times before ikey time) */
					if (!extrev)
						{
						for (jkey=0; jkey<ikey; jkey++)
							{
							ajkey = alink->key + jkey;
							if (ajkey->iarea < 0)    continue;
							if (IsNull(ajkey->line)) continue;
							line  = ajkey->line;
							np    = line->numpts;
							ajkey->nseg++;
							nseg = ajkey->nseg;
							ajkey->dseg  = GETMEM(ajkey->dseg,  int,   nseg);
							ajkey->dspan = GETMEM(ajkey->dspan, float, nseg);
							ajkey->dspt  = GETMEM(ajkey->dspt,  POINT, nseg);
							for (jseg=nseg-2; jseg>rseg; jseg--)
								{
								ajkey->dseg[jseg+1]  = ajkey->dseg[jseg];
								ajkey->dspan[jseg+1] = ajkey->dspan[jseg];
								copy_point(ajkey->dspt[jseg+1],
											ajkey->dspt[jseg]);
								}
							jseg = (rseg < nseg-2)? rseg + 2: 0;
							dss  = ajkey->dspan[rseg];
							dse  = ajkey->dspan[jseg];
							if (ajkey->cw)
								{
								dbot  = dse - dss;	if (dbot <= 0) dbot += np-1;
								if (rcw) dspan = dbot*rel + dss;
								else     dspan = dbot*(1-rel) + dss;
								}
							else
								{
								dbot  = dss - dse;	if (dbot <= 0) dbot += np-1;
								if (rcw) dspan = dbot*(1-rel) + dse;
								else     dspan = dbot*rel + dse;
								}
							if (dspan >= np-1) dspan -= np-1;
							jspan = dspan;
							dsrel = dspan - jspan;
							xa = line->points[jspan][X];
							xb = line->points[jspan+1][X];
							ya = line->points[jspan][Y];
							yb = line->points[jspan+1][Y];
							x  = xa + dsrel*(xb-xa);
							y  = ya + dsrel*(yb-ya);
							ajkey->dseg[rseg+1]  = nseg-1;
							ajkey->dspan[rseg+1] = dspan;
							set_point(ajkey->dspt[rseg+1], x, y);
							}
						extrev = TRUE;
						}
					}

				/* Not an active node */
				/* Extend pseudo-links forward in time if required */
				/* (Current is not active, but follows an active one) */
				/* (Use rel from last active node to set locations) */
				else if (rel >= 0)
					{
					if (aikey->iarea < 0)    continue;
					if (IsNull(aikey->line)) continue;
					line  = aikey->line;
					np    = line->numpts;
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>rseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					jseg = (rseg < nseg-2)? rseg + 2: 0;
					dss  = aikey->dspan[rseg];
					dse  = aikey->dspan[jseg];
					if (aikey->cw)
						{
						dbot  = dse - dss;	if (dbot <= 0) dbot += np-1;
						if (rcw) dspan = dbot*rel + dss;
						else     dspan = dbot*(1-rel) + dss;
						}
					else
						{
						dbot  = dss - dse;	if (dbot <= 0) dbot += np-1;
						if (rcw) dspan = dbot*(1-rel) + dse;
						else     dspan = dbot*rel + dse;
						}
					if (dspan >= np-1) dspan -= np-1;
					jspan = dspan;
					dsrel = dspan - jspan;
					xa = line->points[jspan][X];
					xb = line->points[jspan+1][X];
					ya = line->points[jspan][Y];
					yb = line->points[jspan+1][Y];
					x  = xa + dsrel*(xb-xa);
					y  = ya + dsrel*(yb-ya);
					aikey->dseg[rseg+1]  = nseg-1;
					aikey->dspan[rseg+1] = dspan;
					set_point(aikey->dspt[rseg+1], x, y);
					}
				}
			}
		}

	/* Find out if any area dividing lines have multiple link chains */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Number the initial link chain (for control nodes) */
		ilink = 0;

		/* Only check area dividing lines */
		if (alink->ltype != AreaDiv) continue;

		/* See if another chain has the same dividing line on it */
		for (iblink=ialink+1; iblink<nALink; iblink++)
			{
			blink = ALinks + iblink;

			/* Only check area dividing lines */
			if (blink->ltype != AreaDiv) continue;

			/* Already found? */
			if (blink->icom  != iblink)  continue;

			/* Check in each keyframe */
			/* Common if the line is shared in all co-existing keyframes */
			cfound = FALSE;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaDiv)            continue;
				if (bikey->mtype != AreaDiv)            continue;
				if (aikey->iarea < 0)                   continue;
				if (bikey->iarea < 0)                   continue;
				if (aikey->imem < 0)                    continue;
				if (bikey->imem < 0)                    continue;
				if (aikey->imem >= aikey->area->numdiv) continue;
				if (bikey->imem >= bikey->area->numdiv) continue;

				/* Check for matching area and matching dividing line */
				if (aikey->iarea != bikey->iarea)
					{
					cfound = FALSE;
					break;
					}
				if (aikey->imem != bikey->imem)
					{
					cfound = FALSE;
					break;
					}

				/* Disregard if they only co-exist at joined ends */
				if (ikey > 0 && ikey < NumKey-1)
					{
					if (    (aikey-1)->link && !(aikey+1)->link
						&& !(bikey-1)->link &&  (bikey+1)->link)
						{
						cfound = FALSE;
						break;
						}
					if (   !(aikey-1)->link &&  (aikey+1)->link
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
				blink->icom = alink->icom;

				/* Save the earliest start and latest end times */
				alink->skey  = MIN(alink->skey, blink->skey);
				alink->ekey  = MAX(alink->ekey, blink->ekey);
				alink->splus = MIN(alink->splus, blink->splus);
				alink->eplus = MAX(alink->eplus, blink->eplus);

				/* Save the control node information */
				/* Associate control nodes with the next link chain */
				ilink++;
				(void) add_nodes_to_control_list(alink->ctrl, ilink,
																blink->ctrl);

				/* Share line info over non-common frames to help */
				/* generate pseudo-links */
				for (ikey=0; ikey<NumKey; ikey++)
					{
					aikey = alink->key + ikey;
					bikey = blink->key + ikey;

					if (aikey->iarea < 0 && bikey->iarea >= 0)
						{
						if (bikey->mtype != AreaDiv) continue;
						aikey->link  = FALSE;
						aikey->mtype = bikey->mtype;
						aikey->iarea = bikey->iarea;
						aikey->imem  = bikey->imem;
						aikey->area  = bikey->area;
						aikey->line  = bikey->line;
						}

					else if (bikey->iarea < 0 && aikey->iarea >= 0)
						{
						if (aikey->mtype != AreaDiv) continue;
						bikey->link  = FALSE;
						bikey->mtype = aikey->mtype;
						bikey->iarea = aikey->iarea;
						bikey->imem  = aikey->imem;
						bikey->area  = aikey->area;
						bikey->line  = aikey->line;
						}
					}
				}
			}
		}

	/* Generate list of common-chain segments in each keyframe */
	/* Create pseudo-links where required */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only check area dividing lines */
		if (alink->ltype != AreaDiv) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)  continue;

		alink->ncom   = 0;
		alink->common = (int *)0;

		/* Initialize segments in each keyframe */
		extrev  = FALSE;
		first   = TRUE;
		atstart = TRUE;
		rel     = -1;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			if (aikey->mtype != AreaDiv) continue;

			/* Determine orientation of dividing line wrt first frame */
			aikey->flip = FALSE;
			if (aikey->link)
				{

				/* First look for more than one link chain on dividing line */
				line = aikey->line;
				orientation = FALSE;
				for (iblink=0; iblink<nALink; iblink++)
					{
					if (iblink == ialink)        continue;
					blink = ALinks + iblink;
					if (blink->ltype != AreaDiv) continue;
					if (blink->icom  != ialink)  continue;

					bikey = blink->key + ikey;
					if (bikey->mtype != AreaDiv)      continue;
					if (!bikey->link)                 continue;
					if (bikey->iarea != aikey->iarea) continue;

					/* Found a matching link chain */
					/* Determine orientation based on link nodes */
					line_test_point(line, aikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);
					line_test_point(line, bikey->lpos, NullFloat,
									jpos, &jspan, NullChar, NullChar);
					if (ispan == jspan)
						{
						dsl = point_dist(line->points[ispan], ipos);
						dst = point_dist(line->points[jspan], jpos);
						}
					if (first)
						{
						if (ispan == jspan && dsl > dst) atstart = FALSE;
						else if (ispan > jspan)          atstart = FALSE;
						first       = FALSE;
						orientation = TRUE;
						break;
						}
					else
						{
						if (atstart)
							{
							if (ispan == jspan && dsl > dst) aikey->flip = TRUE;
							else if (ispan > jspan)          aikey->flip = TRUE;
							}
						else
							{
							if (ispan == jspan && dsl < dst) aikey->flip = TRUE;
							else if (ispan < jspan)          aikey->flip = TRUE;
							}
						orientation = TRUE;
						break;
						}
					}

				/* Did not find a matching link chain */
				/* Determine orientation based on location on dividing line */
				if (!orientation)
					{
					line_properties(line, NullLogical, NullLogical,
									NullFloat, &length);
					line_test_point(line, aikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);
					dsl = point_dist(line->points[ispan], ipos);
					dst = point_dist(line->points[ispan],
									 line->points[ispan+1]);
					dspan = ispan + dsl/dst;
					slen  = line_slen(line, 0, dspan);

					/* Determine the line orientation */
					if (first)
						{
						if (slen > length/2.0) atstart = FALSE;
						first = FALSE;
						}
					else
						{
						if ((atstart && slen > length/2.0)
								|| (!atstart && slen <= length/2.0))
						aikey->flip = TRUE;
						}
					}
				}

			/* Define 2 segments for the dividing line, the first time */
			aikey->nseg  = 2;
			aikey->dseg  = GETMEM(aikey->dseg,  int,   2);
			aikey->dspan = GETMEM(aikey->dspan, float, 2);
			aikey->dspt  = GETMEM(aikey->dspt,  POINT, 2);

			if (aikey->link)
				{

				/* Define first segment from start of dividing line to first */
				/* link node and second segment from link node to end of line */
				line = aikey->line;
				np   = line->numpts;
				line_test_point(line, aikey->lpos, NullFloat,
								ipos, &ispan, NullChar, NullChar);
				dsl = point_dist(line->points[ispan], ipos);
				dst = point_dist(line->points[ispan], line->points[ispan+1]);
				dspan = ispan + dsl/dst;

				/* Set the segment parameters */
				if (aikey->flip)
					{
					aikey->dseg[0]  = 0;
					aikey->dspan[0] = np-1;
					copy_point(aikey->dspt[0], line->points[np-1]);
					aikey->dseg[1]  = 1;
					aikey->dspan[1] = dspan;
					copy_point(aikey->dspt[1], aikey->lpos);
					rel = (np-1 - dspan) / (np-1);
					}
				else
					{
					aikey->dseg[0]  = 0;
					aikey->dspan[0] = 0;
					copy_point(aikey->dspt[0], line->points[0]);
					aikey->dseg[1]  = 1;
					aikey->dspan[1] = dspan;
					copy_point(aikey->dspt[1], aikey->lpos);
					rel = dspan / (np-1);
					}

				/* Extend pseudo-links back in time if required */
				/* (First active node is not at the first time) */
				if (!extrev)
					{
					for (jkey=0; jkey<ikey; jkey++)
						{
						ajkey = alink->key + jkey;
						if (ajkey->iarea < 0)    continue;
						if (IsNull(ajkey->line)) continue;
						line  = ajkey->line;
						np    = line->numpts;
						dspan = (np-1)*rel;
						jspan = dspan;
						dsrel = dspan - jspan;
						/* >>>>> not sure if there was a problem here? <<<<< */
						xa = line->points[jspan][X];
						xb = line->points[jspan+1][X];
						ya = line->points[jspan][Y];
						yb = line->points[jspan+1][Y];
						x  = xa + dsrel*(xb-xa);
						y  = ya + dsrel*(yb-ya);
						if (ajkey->flip)
							{
							ajkey->dseg[0]  = 0;
							ajkey->dspan[0] = np-1;
							copy_point(ajkey->dspt[0], line->points[np-1]);
							ajkey->dseg[1]  = 1;
							ajkey->dspan[1] = dspan;
							set_point(ajkey->dspt[1], x, y);
							}
						else
							{
							ajkey->dseg[0]  = 0;
							ajkey->dspan[0] = 0;
							copy_point(ajkey->dspt[0], line->points[0]);
							ajkey->dseg[1]  = 1;
							ajkey->dspan[1] = dspan;
							set_point(ajkey->dspt[1], x, y);
							}
						}
					extrev = TRUE;
					}
				}

			/* Extend pseudo-links forward in time if required */
			/* (Current is not active, but follows an active one) */
			else if (rel >= 0)
				{
				if (aikey->iarea < 0)    continue;
				if (IsNull(aikey->line)) continue;
				line  = aikey->line;
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
				if (ajkey->flip)
					{
					ajkey->dseg[0]  = 0;
					ajkey->dspan[0] = np-1;
					copy_point(ajkey->dspt[0], line->points[np-1]);
					ajkey->dseg[1]  = 1;
					ajkey->dspan[1] = dspan;
					set_point(ajkey->dspt[1], x, y);
					}
				else
					{
					ajkey->dseg[0]  = 0;
					ajkey->dspan[0] = 0;
					copy_point(ajkey->dspt[0], line->points[0]);
					ajkey->dseg[1]  = 1;
					ajkey->dspan[1] = dspan;
					set_point(ajkey->dspt[1], x, y);
					}
				}
			}

		/* Find any common chains */
		for (iblink=0; iblink<nALink; iblink++)
			{
			if (iblink == ialink)        continue;
			blink = ALinks + iblink;
			if (blink->ltype != AreaDiv) continue;
			if (blink->icom  != ialink)  continue;

			alink->ncom++;
			alink->common = GETMEM(alink->common, int, alink->ncom);
			alink->common[alink->ncom-1] = iblink;

			/* Divide segments in each keyframe with new nodes */
			extrev = FALSE;
			rel    = -1;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaDiv) continue;
				if (bikey->mtype != AreaDiv) continue;

				if (bikey->link)
					{
					if (bikey->iarea != aikey->iarea) continue;

					/* Find which segment contains this node */
					/* so we can divide it */
					line = aikey->line;
					np   = line->numpts;
					line_test_point(line, bikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);
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
					nseg = aikey->nseg;
					for (iseg=0; iseg<nseg-1; iseg++)
						{
						dss = aikey->dspan[iseg];
						if (iseg < nseg-1)
							dse = aikey->dspan[iseg+1];
						else
							dse = (aikey->flip)? 0: np-1;
						if (aikey->flip)
							{
							if (dse <= dspan && dspan < dss) break;
							}
						else
							{
							if (dss <= dspan && dspan < dse) break;
							}
						}

					/* Break the segment that contains it (dspan[iseg]) */
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>iseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					aikey->dseg[iseg+1]  = nseg-1;
					aikey->dspan[iseg+1] = dspan;
					copy_point(aikey->dspt[iseg+1], bikey->lpos);
					dss  = aikey->dspan[iseg];
					if (iseg < nseg-2)
						dse = aikey->dspan[iseg+2];
					else
						dse = (aikey->flip)? 0: np-1;
					if (aikey->flip)
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
					rflip = aikey->flip;

					/* Extend pseudo-links back in time if required */
					if (!extrev)
						{
						for (jkey=0; jkey<ikey; jkey++)
							{
							ajkey = alink->key + jkey;
							if (ajkey->iarea < 0)    continue;
							if (IsNull(ajkey->line)) continue;
							line  = ajkey->line;
							np    = line->numpts;
							ajkey->nseg++;
							nseg = ajkey->nseg;
							ajkey->dseg  = GETMEM(ajkey->dseg,  int,   nseg);
							ajkey->dspan = GETMEM(ajkey->dspan, float, nseg);
							ajkey->dspt  = GETMEM(ajkey->dspt,  POINT, nseg);
							for (jseg=nseg-2; jseg>rseg; jseg--)
								{
								ajkey->dseg[jseg+1]  = ajkey->dseg[jseg];
								ajkey->dspan[jseg+1] = ajkey->dspan[jseg];
								copy_point(ajkey->dspt[jseg+1],
											ajkey->dspt[jseg]);
								}
							dss  = ajkey->dspan[rseg];
							if (rseg < nseg-2)
								dse = ajkey->dspan[rseg+2];
							else
								dse = (ajkey->flip)? 0: np-1;
							if (ajkey->flip)
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
							ajkey->dseg[rseg+1]  = nseg-1;
							ajkey->dspan[rseg+1] = dspan;
							set_point(ajkey->dspt[rseg+1], x, y);
							}
						extrev = TRUE;
						}
					}

				/* Extend pseudo-links forward in time if required */
				/* come here from continues...  */
				else if (rel >= 0)
					{
					if (aikey->iarea < 0)    continue;
					if (IsNull(aikey->line)) continue;
					line  = aikey->line;
					np    = line->numpts;
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>rseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					dss  = aikey->dspan[rseg];
					if (rseg < nseg-2)
						dse = aikey->dspan[rseg+2];
					else
						dse = (aikey->flip)? 0: np-1;
					if (aikey->flip)
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
					aikey->dseg[rseg+1]  = nseg-1;
					aikey->dspan[rseg+1] = dspan;
					set_point(aikey->dspt[rseg+1], x, y);
					}
				}
			}
		}

	/* Find out if any area holes have multiple link chains */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Number the initial link chain (for control nodes) */
		ilink = 0;

		/* Only check area holes */
		if (alink->ltype != AreaHole) continue;

		/* See if another chain has the same hole on it */
		for (iblink=ialink+1; iblink<nALink; iblink++)
			{
			blink = ALinks + iblink;

			/* Only check area holes */
			if (blink->ltype != AreaHole) continue;

			/* Already found? */
			if (blink->icom  != iblink)  continue;

			/* Check in each keyframe */
			/* Common if the hole is shared in all co-existing keyframes */
			cfound = FALSE;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaHole)                   continue;
				if (bikey->mtype != AreaHole)                   continue;
				if (aikey->iarea < 0)                           continue;
				if (bikey->iarea < 0)                           continue;
				if (aikey->imem < 0)                            continue;
				if (bikey->imem < 0)                            continue;
				if (aikey->imem >= aikey->area->bound->numhole) continue;
				if (bikey->imem >= bikey->area->bound->numhole) continue;

				/* Check for matching area and matching hole */
				if (aikey->iarea != bikey->iarea)
					{
					cfound = FALSE;
					break;
					}
				if (aikey->imem != bikey->imem)
					{
					cfound = FALSE;
					break;
					}

				/* Disregard if they only co-exist at joined ends */
				if (ikey > 0 && ikey < NumKey-1)
					{
					if (    (aikey-1)->link && !(aikey+1)->link
						&& !(bikey-1)->link &&  (bikey+1)->link)
						{
						cfound = FALSE;
						break;
						}
					if (   !(aikey-1)->link &&  (aikey+1)->link
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
				blink->icom = alink->icom;

				/* Save the earliest start and latest end times */
				alink->skey  = MIN(alink->skey, blink->skey);
				alink->ekey  = MAX(alink->ekey, blink->ekey);
				alink->splus = MIN(alink->splus, blink->splus);
				alink->eplus = MAX(alink->eplus, blink->eplus);

				/* Save the control node information */
				/* Associate control nodes with the next link chain */
				ilink++;
				(void) add_nodes_to_control_list(alink->ctrl, ilink,
																blink->ctrl);

				/* Share hole info over non-common frames to help */
				/* generate pseudo-links */
				for (ikey=0; ikey<NumKey; ikey++)
					{
					aikey = alink->key + ikey;
					bikey = blink->key + ikey;

					if (aikey->iarea < 0 && bikey->iarea >= 0)
						{
						if (bikey->mtype != AreaHole) continue;
						aikey->link  = FALSE;
						aikey->mtype = bikey->mtype;
						aikey->iarea = bikey->iarea;
						aikey->imem  = bikey->imem;
						aikey->area  = bikey->area;
						aikey->line  = bikey->line;
						}

					else if (bikey->iarea < 0 && aikey->iarea >= 0)
						{
						if (aikey->mtype != AreaHole) continue;
						bikey->link  = FALSE;
						bikey->mtype = aikey->mtype;
						bikey->iarea = aikey->iarea;
						bikey->imem  = aikey->imem;
						bikey->area  = aikey->area;
						bikey->line  = aikey->line;
						}
					}
				}
			}
		}

	/* Generate list of common-chain segments in each keyframe */
	/* Create pseudo-links where required */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Only check area holes */
		if (alink->ltype != AreaHole) continue;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink)  continue;

		alink->ncom   = 0;
		alink->common = (int *)0;

		/* Initialize segments in each keyframe */
		extrev = FALSE;
		rel    = -1;
		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			if (aikey->mtype != AreaHole) continue;

			/* Define a segment for the whole hole, the first time */
			aikey->nseg  = 1;
			aikey->dseg  = GETMEM(aikey->dseg,  int,   1);
			aikey->dspan = GETMEM(aikey->dspan, float, 1);
			aikey->dspt  = GETMEM(aikey->dspt,  POINT, 1);

			if (aikey->link)
				{

				/* Define the segment relative to the first link node */
				/* and compute position of node relative to top of hole */
				line = aikey->line;
				np   = line->numpts;
				line_test_point(line, aikey->lpos, NullFloat,
								ipos, &ispan, NullChar, NullChar);
				dsl = point_dist(line->points[ispan], ipos);
				dst = point_dist(line->points[ispan], line->points[ispan+1]);
				dspan = ispan + dsl/dst;
				aikey->dseg[0]  = 0;
				aikey->dspan[0] = dspan;
				copy_point(aikey->dspt[0], aikey->lpos);
				itop = line_closest_point(line, tpos, NullFloat, NullPoint);
				if (aikey->hcw)
					{
					if (ispan >= itop) rel = (dspan - itop) / (np-1);
					else               rel = (dspan + np-1 - itop) / (np-1);
					}
				else
					{
					if (ispan <= itop) rel = (itop - dspan) / (np-1);
					else               rel = (itop + np-1 - dspan) / (np-1);
					}

				/* Extend pseudo-links back in time if required */
				/* (First active node is not at the first time) */
				if (!extrev)
					{
					for (jkey=0; jkey<ikey; jkey++)
						{
						ajkey = alink->key + jkey;
						if (ajkey->iarea < 0)    continue;
						if (IsNull(ajkey->line)) continue;
						line  = ajkey->line;
						np    = line->numpts;
						jtop  = line_closest_point(line, tpos, NullFloat,
								NullPoint);
						if (ajkey->hcw)
							{
							dspan = jtop + (np-1)*rel;
							if (dspan >= np-1) dspan -= np-1;
							}
						else
							{
							dspan = jtop - (np-1)*rel;
							if (dspan < 0) dspan += np-1;
							}
						jspan = dspan;
						dsrel = dspan - jspan;
						/* >>>>> not sure if there was a problem here? <<<<< */
						xa = line->points[jspan][X];
						xb = line->points[jspan+1][X];
						ya = line->points[jspan][Y];
						yb = line->points[jspan+1][Y];
						x  = xa + dsrel*(xb-xa);
						y  = ya + dsrel*(yb-ya);
						ajkey->dseg[0]  = 0;
						ajkey->dspan[0] = dspan;
						set_point(ajkey->dspt[0], x, y);
						}
					extrev = TRUE;
					}
				}

			/* Extend pseudo-links forward in time if required */
			/* (Current is not active, but follows an active one) */
			else if (rel >= 0)
				{
				if (aikey->iarea < 0)    continue;
				if (IsNull(aikey->line)) continue;
				line  = aikey->line;
				np    = line->numpts;
				itop  = line_closest_point(line, tpos, NullFloat,
						NullPoint);
				if (aikey->hcw)
					{
					dspan = itop + (np-1)*rel;
					if (dspan >= np-1) dspan -= np-1;
					}
				else
					{
					dspan = itop - (np-1)*rel;
					if (dspan < 0) dspan += np-1;
					}
				ispan = dspan;
				dsrel = dspan - ispan;
				xa = line->points[ispan][X];
				xb = line->points[ispan+1][X];
				ya = line->points[ispan][Y];
				yb = line->points[ispan+1][Y];
				x  = xa + dsrel*(xb-xa);
				y  = ya + dsrel*(yb-ya);
				aikey->dseg[0]  = 0;
				aikey->dspan[0] = dspan;
				set_point(aikey->dspt[0], x, y);
				}
			}

		/* Find any common chains */
		for (iblink=0; iblink<nALink; iblink++)
			{
			if (iblink == ialink)         continue;
			blink = ALinks + iblink;
			if (blink->ltype != AreaHole) continue;
			if (blink->icom  != ialink)   continue;

			alink->ncom++;
			alink->common = GETMEM(alink->common, int, alink->ncom);
			alink->common[alink->ncom-1] = iblink;

			/* Divide segments in each keyframe with new nodes */
			extrev = FALSE;
			rel    = -1;
			for (ikey=0; ikey<NumKey; ikey++)
				{
				aikey = alink->key + ikey;
				bikey = blink->key + ikey;
				if (aikey->mtype != AreaHole) continue;
				if (bikey->mtype != AreaHole) continue;

				if (bikey->link)
					{
					if (bikey->iarea != aikey->iarea) continue;

					/* Find which segment contains this node */
					/* so we can divide it */
					line = aikey->line;
					np   = line->numpts;
					line_test_point(line, bikey->lpos, NullFloat,
									ipos, &ispan, NullChar, NullChar);

					dsl = point_dist(line->points[ispan], ipos);
					dst = point_dist(line->points[ispan],
								line->points[ispan+1]);
					dspan = ispan + dsl/dst;
					nseg = aikey->nseg;
					for (iseg=0; iseg<nseg-1; iseg++)
						{
						dss = aikey->dspan[iseg];
						dse = aikey->dspan[iseg+1];
						if (aikey->hcw)
							{
							if (dss <= dse)
								{
								if (dss <= dspan && dspan < dse) break;
								}
							else
								{
								if (dss <= dspan || dspan < dse) break;
								}
							}
						else
							{
							if (dss >= dse)
								{
								if (dss >= dspan && dspan > dse) break;
								}
							else
								{
								if (dss >= dspan || dspan > dse) break;
								}
							}
						}

					/* Break the segment that contains it (dspan[iseg]) */
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>iseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					aikey->dseg[iseg+1]  = nseg-1;
					aikey->dspan[iseg+1] = dspan;
					copy_point(aikey->dspt[iseg+1], bikey->lpos);
					jseg = (iseg < nseg-2)? iseg + 2: 0;
					dss  = aikey->dspan[iseg];
					dse  = aikey->dspan[jseg];
					if (aikey->hcw)
						{
						dtop = dspan - dss;	if (dtop <  0) dtop += np-1;
						dbot = dse - dss;	if (dbot <= 0) dbot += np-1;
						}
					else
						{
						dtop = dspan - dse;	if (dtop <  0) dtop += np-1;
						dbot = dss - dse;	if (dbot <= 0) dbot += np-1;
						}
					rel  = dtop / dbot;
					rseg = iseg;
					rcw  = aikey->hcw;

					/* Extend pseudo-links back in time if required */
					if (!extrev)
						{
						for (jkey=0; jkey<ikey; jkey++)
							{
							ajkey = alink->key + jkey;
							if (ajkey->iarea < 0)    continue;
							if (IsNull(ajkey->line)) continue;
							line  = ajkey->line;
							np    = line->numpts;
							ajkey->nseg++;
							nseg = ajkey->nseg;
							ajkey->dseg  = GETMEM(ajkey->dseg,  int,   nseg);
							ajkey->dspan = GETMEM(ajkey->dspan, float, nseg);
							ajkey->dspt  = GETMEM(ajkey->dspt,  POINT, nseg);
							for (jseg=nseg-2; jseg>rseg; jseg--)
								{
								ajkey->dseg[jseg+1]  = ajkey->dseg[jseg];
								ajkey->dspan[jseg+1] = ajkey->dspan[jseg];
								copy_point(ajkey->dspt[jseg+1],
											ajkey->dspt[jseg]);
								}
							jseg = (rseg < nseg-2)? rseg + 2: 0;
							dss  = ajkey->dspan[rseg];
							dse  = ajkey->dspan[jseg];
							if (ajkey->hcw)
								{
								dbot  = dse - dss;	if (dbot <= 0) dbot += np-1;
								if (rcw) dspan = dbot*rel + dss;
								else     dspan = dbot*(1-rel) + dss;
								}
							else
								{
								dbot  = dss - dse;	if (dbot <= 0) dbot += np-1;
								if (rcw) dspan = dbot*(1-rel) + dse;
								else     dspan = dbot*rel + dse;
								}
							if (dspan >= np-1) dspan -= np-1;
							jspan = dspan;
							dsrel = dspan - jspan;
							xa = line->points[jspan][X];
							xb = line->points[jspan+1][X];
							ya = line->points[jspan][Y];
							yb = line->points[jspan+1][Y];
							x  = xa + dsrel*(xb-xa);
							y  = ya + dsrel*(yb-ya);
							ajkey->dseg[rseg+1]  = nseg-1;
							ajkey->dspan[rseg+1] = dspan;
							set_point(ajkey->dspt[rseg+1], x, y);
							}
						extrev = TRUE;
						}
					}

				/* Extend pseudo-links forward in time if required */
				/* come here from continues...  */
				else if (rel >= 0)
					{
					if (aikey->iarea < 0)    continue;
					if (IsNull(aikey->line)) continue;
					line  = aikey->line;
					np    = line->numpts;
					aikey->nseg++;
					nseg = aikey->nseg;
					aikey->dseg  = GETMEM(aikey->dseg,  int,   nseg);
					aikey->dspan = GETMEM(aikey->dspan, float, nseg);
					aikey->dspt  = GETMEM(aikey->dspt,  POINT, nseg);
					for (jseg=nseg-2; jseg>rseg; jseg--)
						{
						aikey->dseg[jseg+1]  = aikey->dseg[jseg];
						aikey->dspan[jseg+1] = aikey->dspan[jseg];
						copy_point(aikey->dspt[jseg+1], aikey->dspt[jseg]);
						}
					jseg = (rseg < nseg-2)? rseg + 2: 0;
					dss  = aikey->dspan[rseg];
					dse  = aikey->dspan[jseg];
					if (aikey->hcw)
						{
						dbot  = dse - dss;	if (dbot <= 0) dbot += np-1;
						if (rcw) dspan = dbot*rel + dss;
						else     dspan = dbot*(1-rel) + dss;
						}
					else
						{
						dbot  = dss - dse;	if (dbot <= 0) dbot += np-1;
						if (rcw) dspan = dbot*(1-rel) + dse;
						else     dspan = dbot*rel + dse;
						}
					if (dspan >= np-1) dspan -= np-1;
					jspan = dspan;
					dsrel = dspan - jspan;
					xa = line->points[jspan][X];
					xb = line->points[jspan+1][X];
					ya = line->points[jspan][Y];
					yb = line->points[jspan+1][Y];
					x  = xa + dsrel*(xb-xa);
					y  = ya + dsrel*(yb-ya);
					aikey->dseg[rseg+1]  = nseg-1;
					aikey->dspan[rseg+1] = dspan;
					set_point(aikey->dspt[rseg+1], x, y);
					}
				}
			}
		}

	/* Check the area links to ensure all segments are consistent */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;

		/* Multiple links are associated with the first link found! */
		if (alink->icom  != ialink) continue;

		for (ikey=0; ikey<NumKey; ikey++)
			{
			aikey = alink->key + ikey;
			if (!aikey->link) continue;
			break;
			}
		if (ikey >= NumKey) continue;

		for (jkey=ikey+1; jkey<NumKey; jkey++)
			{
			ajkey = alink->key + jkey;
			if (!ajkey->link) continue;

			/* Check for consistent number of segments */
			if (ajkey->nseg != aikey->nseg)
				{
				if (minutes_in_depictions())
					{
					pr_warning("Interp.Areas",
						"Inconsistent number of area links: %d:%d %d:%d\n",
						ikey, aikey->nseg, jkey, ajkey->nseg);
					(void) strcpy(vts, hour_minute_string(0, alink->splus));
					(void) strcpy(vte, hour_minute_string(0, alink->eplus));
					pr_warning("Interp.Areas",
						"  for link chain: %d  between: T%s and T%s\n",
						ialink, vts, vte);
					pr_warning("Interp.Areas",
						"May be problem with interpolation of %s %s\n",
						dfld->element, dfld->level);
					(void) strcpy(vts,
						hour_minute_string(0, KeyPlus[ikey]));
					(void) strcpy(vte,
						hour_minute_string(0, KeyPlus[jkey]));
					pr_warning("Interp.Areas",
						"  between: T%s and T%s\n", vts, vte);
					}
				else
					{
					pr_warning("Interp.Areas",
						"Inconsistent number of area links: %d:%d %d:%d\n",
						ikey, aikey->nseg, jkey, ajkey->nseg);
					pr_warning("Interp.Areas",
						"  for link chain: %d  between: T%d and T%d\n",
						ialink, alink->splus/60, alink->eplus/60);
					pr_warning("Interp.Areas",
						"May be problem with interpolation of %s %s\n",
						dfld->element, dfld->level);
					pr_warning("Interp.Areas",
						"  between: T%d and T%d\n",
						KeyPlus[ikey]/60, KeyPlus[jkey]/60);
					}
				continue;
				}

			/* Check for consistent order of segments */
			for (iseg=0; iseg<aikey->nseg; iseg++)
				{
				if (ajkey->dseg[iseg] != aikey->dseg[iseg])
					{
					if (minutes_in_depictions())
						{
						pr_warning("Interp.Areas",
							"Inconsistent area link order: %d:%d %d:%d\n",
							ikey, aikey->dseg[iseg], jkey, ajkey->dseg[iseg]);
						(void) strcpy(vts, hour_minute_string(0, alink->splus));
						(void) strcpy(vte, hour_minute_string(0, alink->eplus));
						pr_warning("Interp.Areas",
							"  for link chain: %d  between: T%s and T%s\n",
							ialink, vts, vte);
						pr_warning("Interp.Areas",
							"May be problem with interpolation of %s %s\n",
							dfld->element, dfld->level);
						(void) strcpy(vts,
							hour_minute_string(0, KeyPlus[ikey]));
						(void) strcpy(vte,
							hour_minute_string(0, KeyPlus[jkey]));
						pr_warning("Interp.Areas",
							"  between: T%s and T%s\n", vts, vte);
						}
					else
						{
						pr_warning("Interp.Areas",
							"Inconsistent area link order: %d:%d %d:%d\n",
							ikey, aikey->dseg[iseg], jkey, ajkey->dseg[iseg]);
						pr_warning("Interp.Areas",
							"  for link chain: %d  between: T%d and T%d\n",
							ialink, alink->splus/60, alink->eplus/60);
						pr_warning("Interp.Areas",
							"May be problem with interpolation of %s %s\n",
							dfld->element, dfld->level);
						pr_warning("Interp.Areas",
							"  between: T%d and T%d\n",
							KeyPlus[ikey]/60, KeyPlus[jkey]/60);
						}
					}
				}
			}
		}

	return nALink;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ a r e a _ l i n k s                                   *
*                                                                      *
***********************************************************************/

static	void	debug_area_links

	(
	)

	{
	int		ialink, ic, skey, ekey, ikey, npts, ipt, idiv, nseg, iseg;
	ALINK	*alink;
	ALKEY	*aikey;
	LINE	line;
	char	xbuf[10], ybuf[240];

	/* Display area link information */
	pr_diag("Interp.Areas", "\n");
	pr_diag("Interp.Areas", "Number of area links: %d\n", nALink);

	/* Display link information for area boundaries first */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;
		if (alink->ltype != AreaBound) continue;
		pr_diag("Interp.Areas", "\n");
		pr_diag("Interp.Areas",
			"Area boundary link: %d (%d)   Associated with link: %d\n",
			ialink, alink->ocom, alink->icom);
		if (alink->icom != ialink) continue;
		if (alink->ncom > 0)
			{
			for (ic=0; ic<alink->ncom; ic++)
				pr_diag("Interp.Areas", " Also uses boundary link: %d\n",
					alink->common[ic]);
			}
		pr_diag("Interp.Areas",
			" Start frame/time: %d %d   End frame/time: %d %d\n",
			alink->skey, alink->splus, alink->ekey, alink->eplus);
		skey = alink->skey;
		ekey = alink->ekey;
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			pr_diag("Interp.Areas", "  Area link frame/area: %d %d\n",
				ikey, aikey->iarea);
			pr_diag("Interp.Areas", "   Link Position: %.2f %.2f\n",
				aikey->lpos[X], aikey->lpos[Y]);
			(void) strcpy(ybuf, "");
			if (aikey->ndiv > 0)
				{
				for (idiv=0; idiv<aikey->ndiv; idiv++)
					{
					(void) sprintf(xbuf, " %d", aikey->sids[idiv]);
					(void) strcat(ybuf, xbuf);
					}
				}
			pr_diag("Interp.Areas", "   Area divide subids: %d - %s\n",
				aikey->ndiv, ybuf);
			pr_diag("Interp.Areas", "   Area holes: %d\n", aikey->nhole);

			line = aikey->line;
			npts = line->numpts;
			pr_diag("Interp.Areas", "   Area boundary points: %d\n", npts);
			if (npts > 0)
				{
				pr_diag("Interp.Areas", "   Boundary positions -\n");

				/* >>>>> this prints first two and last two points <<<<< */
				ipt = 0;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);
				if (npts > 2)
					{
					ipt = 1;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				pr_diag("Interp.Areas", "      to\n");
				if (npts > 3)
					{
					ipt = npts-2;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				ipt = npts-1;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);

				/* >>>>> this prints all of the points! <<<<< */
				/* >>>>>
				for (ipt=0; ipt<npts; ipt++)
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
				<<<<< */
				}

			nseg = aikey->nseg;
			pr_diag("Interp.Areas", "   Boundary segments: %d\n", nseg);
			for (iseg=0; iseg<nseg; iseg++)
				pr_diag("Interp.Areas",
					"    Segment: %d  Dseg: %d  Dspan: %.3f  Position: %.2f %.2f\n",
					iseg, aikey->dseg[iseg], aikey->dspan[iseg],
						aikey->dspt[iseg][X], aikey->dspt[iseg][Y]);

			pr_diag("Interp.Areas", "   Area attributes for frame %d (%d to %d)\n",
				ikey, skey, ekey);
			(void) debug_attrib_list(NULL, aikey->area->attrib);
			}
		}

	/* Display link information for area dividing lines next */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;
		if (alink->ltype != AreaDiv) continue;
		pr_diag("Interp.Areas", "\n");
		pr_diag("Interp.Areas",
			"Area dividing line link: %d (%d)   Associated with link: %d\n",
			ialink, alink->ocom, alink->icom);
		if (alink->icom != ialink) continue;
		if (alink->ncom > 0)
			{
			for (ic=0; ic<alink->ncom; ic++)
				pr_diag("Interp.Areas", " Also uses dividing line link: %d\n",
					alink->common[ic]);
			}
		pr_diag("Interp.Areas",
			" Start frame/time: %d %d   End frame/time: %d %d\n",
			alink->skey, alink->splus, alink->ekey, alink->eplus);
		skey = alink->skey;
		ekey = alink->ekey;
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			pr_diag("Interp.Areas", "  Area link frame/area/line: %d %d %d\n",
				ikey, aikey->iarea, aikey->imem);
			pr_diag("Interp.Areas", "   Link Position: %.2f %.2f\n",
				aikey->lpos[X], aikey->lpos[Y]);

			line = aikey->line;
			npts = line->numpts;
			pr_diag("Interp.Areas", "   Area dividing line points: %d\n", npts);
			if (aikey->flip)
				pr_diag("Interp.Areas", "   Dividing line Reversed!!\n");
			if (npts > 0)
				{
				pr_diag("Interp.Areas", "   Dividing line positions -\n");

				/* >>>>> this prints first two and last two points <<<<< */
				ipt = 0;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);
				if (npts > 2)
					{
					ipt = 1;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				pr_diag("Interp.Areas", "      to\n");
				if (npts > 3)
					{
					ipt = npts-2;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				ipt = npts-1;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);

				/* >>>>> this prints all of the points! <<<<< */
				/* >>>>>
				for (ipt=0; ipt<npts; ipt++)
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
				<<<<< */
				}

			nseg = aikey->nseg;
			pr_diag("Interp.Areas", "   Dividing line segments: %d\n", nseg);
			for (iseg=0; iseg<nseg; iseg++)
				pr_diag("Interp.Areas",
					"    Segment: %d  Dseg: %d  Dspan: %.3f  Position: %.2f %.2f\n",
					iseg, aikey->dseg[iseg], aikey->dspan[iseg],
					aikey->dspt[iseg][X], aikey->dspt[iseg][Y]);
			}
		}

	/* Display link information for area holes next */
	for (ialink=0; ialink<nALink; ialink++)
		{
		alink = ALinks + ialink;
		if (alink->ltype != AreaHole) continue;
		pr_diag("Interp.Areas", "\n");
		pr_diag("Interp.Areas",
			"Area hole link: %d (%d)   Associated with link: %d\n",
			ialink, alink->ocom, alink->icom);
		if (alink->icom != ialink) continue;
		if (alink->ncom > 0)
			{
			for (ic=0; ic<alink->ncom; ic++)
				pr_diag("Interp.Areas", " Also uses hole link: %d\n",
					alink->common[ic]);
			}
		pr_diag("Interp.Areas",
			" Start frame/time: %d %d   End frame/time: %d %d\n",
			alink->skey, alink->splus, alink->ekey, alink->eplus);
		skey = alink->skey;
		ekey = alink->ekey;
		for (ikey=skey; ikey<=ekey; ikey++)
			{
			aikey = alink->key + ikey;
			pr_diag("Interp.Areas", "  Area link frame/area/hole: %d %d %d\n",
				ikey, aikey->iarea, aikey->imem);
			pr_diag("Interp.Areas", "   Link Position: %.2f %.2f\n",
				aikey->lpos[X], aikey->lpos[Y]);

			line = aikey->line;
			npts = line->numpts;
			pr_diag("Interp.Areas", "   Area hole points: %d\n", npts);
			if (npts > 0)
				{
				pr_diag("Interp.Areas", "   Hole positions -\n");

				/* >>>>> this prints first two and last two points <<<<< */
				ipt = 0;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);
				if (npts > 2)
					{
					ipt = 1;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				pr_diag("Interp.Areas", "      to\n");
				if (npts > 3)
					{
					ipt = npts-2;
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
					}
				ipt = npts-1;
				pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
					ipt, line->points[ipt][X], line->points[ipt][Y]);

				/* >>>>> this prints all of the points! <<<<< */
				/* >>>>>
				for (ipt=0; ipt<npts; ipt++)
					pr_diag("Interp.Areas", "    Position: %.3d  %.2f %.2f\n",
						ipt, line->points[ipt][X], line->points[ipt][Y]);
				<<<<< */
				}

			nseg = aikey->nseg;
			pr_diag("Interp.Areas", "   Hole segments: %d\n", nseg);
			for (iseg=0; iseg<nseg; iseg++)
				pr_diag("Interp.Areas",
					"    Segment: %d  Dseg: %d  Dspan: %.3f  Position: %.2f %.2f\n",
					iseg, aikey->dseg[iseg], aikey->dspan[iseg],
					aikey->dspt[iseg][X], aikey->dspt[iseg][Y]);
			}
		}
	}

/***********************************************************************
*                                                                      *
*    a d d _ a r e a _ t o _ a r e a i n f o                           *
*    a d d _ d i v l i n e _ t o _ d i v i n f o                       *
*    a d d _ h o l e _ t o _ h o l e i n f o                           *
*                                                                      *
***********************************************************************/

static	void	add_area_to_areainfo

	(
	AREAINFO	genarea,
	int			iaout,
	int			ialink,
	ALKEY		*areakey
	)

	{
	int		iarea;

	/* Check for duplication of areas */
	for (iarea=0; iarea<genarea->narea; iarea++)
		{
		if (iaout == genarea->iaout[iarea] || ialink == genarea->ialink[iarea])
			pr_error("Interp.Areas",
				"Duplicate area: %d (Link: %d)!\n", iaout, ialink);
		}

	/* Add another area */
	iarea = genarea->narea++;
	genarea->iaout    = GETMEM(genarea->iaout,    int,      genarea->narea);
	genarea->ialink   = GETMEM(genarea->ialink,   int,      genarea->narea);
	genarea->areakey  = GETMEM(genarea->areakey,  ALKEY *,  genarea->narea);
	genarea->chckkey  = GETMEM(genarea->chckkey,  ALKEY *,  genarea->narea);
	genarea->divinfo  = GETMEM(genarea->divinfo,  DIVINFO,  genarea->narea);
	genarea->holeinfo = GETMEM(genarea->holeinfo, HOLEINFO, genarea->narea);
	genarea->iaout[iarea]    = iaout;
	genarea->ialink[iarea]   = ialink;
	genarea->areakey[iarea]  = areakey;
	genarea->chckkey[iarea]  = areakey;
	genarea->divinfo[iarea]  = INITMEM(struct DIVINFO_struct, 1);
	genarea->divinfo[iarea]->ndiv     = 0;
	genarea->divinfo[iarea]->divkey   = (ALKEY **) 0;
	genarea->divinfo[iarea]->divlines = (LINE *) 0;
	genarea->holeinfo[iarea] = INITMEM(struct HOLEINFO_struct, 1);
	genarea->holeinfo[iarea]->nhole   = 0;
	genarea->holeinfo[iarea]->holekey = (ALKEY **) 0;
	genarea->holeinfo[iarea]->holes   = (LINE *) 0;
	}

static	void	add_divline_to_divinfo

	(
	AREAINFO	genarea,
	int			iaout,
	ALKEY		*areakey,
	ALKEY		*chckkey,
	ALKEY		*divkey,
	LINE		divline
	)

	{
	int		iarea, idiv;
	DIVINFO	divinfo;

	/* Check for area to use */
	for (iarea=0; iarea<genarea->narea; iarea++)
		{
		if (iaout == genarea->iaout[iarea]) break;
		}
	if (iarea >= genarea->narea)
		{
		pr_error("Interp.Areas", "  Area not found: %d!\n", iaout);
		return;
		}
	if (chckkey != genarea->chckkey[iarea])
		{
		pr_error("Interp.Areas", "  Area keys do not match: %X %X!\n",
			chckkey, genarea->chckkey[iarea]);
		return;
		}
	if (areakey != genarea->areakey[iarea])
		{
		pr_diag("Interp.Areas", "  Area key change: %X to %X!\n",
			genarea->areakey[iarea], areakey);
		genarea->areakey[iarea] = areakey;
		}

	/* Add another dividing line */
	divinfo = genarea->divinfo[iarea];
	idiv    = divinfo->ndiv++;
	divinfo->divkey   = GETMEM(divinfo->divkey,   ALKEY *, divinfo->ndiv);
	divinfo->divlines = GETMEM(divinfo->divlines, LINE,    divinfo->ndiv);
	divinfo->divkey[idiv]   = divkey;
	divinfo->divlines[idiv] = divline;
	}

static	void	add_hole_to_holeinfo

	(
	AREAINFO	genarea,
	int			iaout,
	ALKEY		*areakey,
	ALKEY		*chckkey,
	ALKEY		*holekey,
	LINE		hole
	)

	{
	int			iarea, ih;
	HOLEINFO	holeinfo;

	/* Check for area to use */
	for (iarea=0; iarea<genarea->narea; iarea++)
		{
		if (iaout == genarea->iaout[iarea]) break;
		}
	if (iarea >= genarea->narea)
		{
		pr_error("Interp.Areas", "  Area not found: %d!\n", iaout);
		return;
		}
	if (chckkey != genarea->chckkey[iarea])
		{
		pr_error("Interp.Areas", "  Area keys do not match: %X %X!\n",
			chckkey, genarea->chckkey[iarea]);
		return;
		}
	if (areakey != genarea->areakey[iarea])
		{
		pr_diag("Interp.Areas", "  Area key change: %X to %X!\n",
			genarea->areakey[iarea], areakey);
		genarea->areakey[iarea] = areakey;
		}

	/* Add another hole */
	holeinfo = genarea->holeinfo[iarea];
	ih       = holeinfo->nhole++;
	holeinfo->holekey = GETMEM(holeinfo->holekey, ALKEY *, holeinfo->nhole);
	holeinfo->holes   = GETMEM(holeinfo->holes,   LINE,    holeinfo->nhole);
	holeinfo->holekey[ih] = holekey;
	holeinfo->holes[ih]   = hole;
	}
