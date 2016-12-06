/***********************************************************************
*                                                                      *
*      p a t t e r n _ l i n e . c                                     *
*                                                                      *
*      Routines to handle graphical output of patterned lines, etc.    *
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

#include "pattern.h"
#include "gx.h"

#include <string.h>
#include <stdio.h>
#undef index

#undef DEBUG_PATTERN
#undef DEBUG_PATTERN_POINTS
#undef DEBUG_PATTERN_READ

typedef	enum { BaseC, BaseB, BaseT } BASE;
typedef	struct
		{
		STRING	name;	/* pattern name */
		int		num;	/* number of pattern components */
		ITEM	*list;	/* list of pattern components */
		STRING	*type;	/* list of corresponding component types */
		float	repeat;
		BASE	base;
		} PATTERN;

static	PATTERN	*get_pattern(STRING, HAND, float *, float *);
static	PATTERN	*find_pattern(STRING);
static	PATTERN	*read_pattern(STRING);
static	LOGICAL	save_pattern_info(STRING, STRING, LOGICAL *);
static	LOGICAL	find_pattern_info(STRING, LOGICAL *);

static	LOGICAL	AnchorBaseline = TRUE;

/***********************************************************************
*                                                                      *
*      c e n t r e _ p a t t e r n                                     *
*      d r a w _ p a t t e r n                                         *
*                                                                      *
*      Display the given curve with the associated pattern.            *
*      centre_pattern centres the pattern on the given line.           *
*      draw_pattern anchors the pattern baseline on the given line.    *
*                                                                      *
***********************************************************************/

LOGICAL	centre_pattern

	(
	POINT	*points,	/* list of points in curve */
	int		numpts,		/* number of points in list */
	STRING	pattern,	/* pattern name */
	HAND	sense,		/* pattern handedness (flip sense) */
	float	width,		/* pattern width factor */
	float	length,		/* pattern repetition factor */
	HILITE	hilite		/* highlight level */
	)

	{
	LOGICAL	val;

	AnchorBaseline = FALSE;
	val = draw_pattern(points, numpts, pattern, sense, width, length, hilite);
	AnchorBaseline = TRUE;

	return val;
	}

LOGICAL	draw_pattern

	(
	POINT	*points,	/* list of points in curve */
	int		numpts,		/* number of points in list */
	STRING	pattern,	/* pattern name */
	HAND	sense,		/* pattern handedness (flip sense) */
	float	width,		/* pattern width factor */
	float	length,		/* pattern repetition factor */
	HILITE	hilite		/* highlight level */
	)

	{
	int		icomp, ispan, ip;
	PATTERN	*p;
	AREA	parea;
	CURVE	pcurve;
	LINE	pline,  tline  = NullLine;
	float	x, xb, xc, xd, xpat;
	float	y, yb, yc, yd, ypat;
	float	dxcurr, dxnext;
	float	dycurr, dynext;
	float	dscurr, dsnext;
	float	nxcurr, nxnext, nxlbar, nxrbar;
	float	nycurr, nynext, nylbar, nyrbar;
	float	wb, wc, ww;
	float	nscale, lpos, rpos, lpat, rpat, mfact;
	int		last, rclose, poly;

	int		iline, nline;
	LINE	*lines;

	MAP_PROJ	*mproj, *zproj;
	LINE		xline, zline;
	POINT		pos, zpos;
	LOGICAL		inside, zdisplay;

	/* Make sure we have something to draw */
	if (!points)     return TRUE;
	if (numpts <= 0) return TRUE;

	/* Make sure we have a pattern */
	if (blank(pattern)) return FALSE;

	/* If the curve only has one point the pattern cannot be used */
	if (numpts ==1) return TRUE;

	/* Make sure we can obtain the requested pattern */
	mfact  = gxGetMfact() / 1000;
	width  = fabs(width)  * mfact;
	length = fabs(length) * mfact;
	if (!(p = get_pattern(pattern, sense, &width, &length))) return FALSE;

#	ifdef DEBUG_PATTERN
	(void) fprintf(stderr,
		"[draw_pattern] Pattern: %s  mfact: %.2g  width/length: %.2g %.2g\n",
		pattern, mfact, width, length);
#	endif /* DEBUG_PATTERN */

	/* Check location of points if the display is zoomed */
	if ( ( NotNull(mproj = get_target_map())
				&& !equivalent_map_projection(mproj, &NoMapProj) )
			&& ( NotNull(zproj = gxGetZoomMproj())
					&& !equivalent_map_projection(zproj, &NoMapProj) )
			&& ( !equivalent_map_projection(mproj, zproj) ) )
		{

#		ifdef DEBUG_PATTERN
		(void) fprintf(stderr,
			"[draw_pattern] Checking pattern points in zoomed display\n");
#		endif /* DEBUG_PATTERN */

		/* Make drawn line from list of points */
		xline = create_line();
		for (ip=0; ip<numpts; ip++) add_point_to_line(xline, points[ip]);

		/* Make line from corners of zoomed projection */
		zline = create_line();
		zpos[X] = 0.0;						zpos[Y] = 0.0;
		(void) pos_to_pos(zproj, zpos, mproj, pos);
		add_point_to_line(zline, pos);
		zpos[X] = 0.0;						zpos[Y] = zproj->definition.ylen;
		(void) pos_to_pos(zproj, zpos, mproj, pos);
		add_point_to_line(zline, pos);
		zpos[X] = zproj->definition.xlen;	zpos[Y] = zproj->definition.ylen;
		(void) pos_to_pos(zproj, zpos, mproj, pos);
		add_point_to_line(zline, pos);
		zpos[X] = zproj->definition.xlen;	zpos[Y] = 0.0;
		(void) pos_to_pos(zproj, zpos, mproj, pos);
		add_point_to_line(zline, pos);
		close_line(zline);

		/* Quick check for drawn line inside zoomed projection */
		zdisplay = FALSE;
		if (!zdisplay)
			{
			line_test_point(zline, xline->points[0], NullFloat, NullPoint,
							NullInt, &inside, NullChar);
			if (inside) zdisplay = TRUE;
			}

		/* Quick check for zoomed projection inside drawn line (closed) */
		if (!zdisplay && line_closed(xline))
			{
			line_test_point(xline, zline->points[0], NullFloat, NullPoint,
							NullInt, &inside, NullChar);
			if (inside) zdisplay = TRUE;
			}

		/* Check for zoomed projection and drawn line crossing over */
		if (!zdisplay)
			{
			if (find_line_crossing(zline, xline, 0, zline->points[0],
									NullPoint, NullInt, NullInt, NullLogical))
				zdisplay = TRUE;
			}

		/* Return if line does not need to be displayed */
		xline = destroy_line(xline);
		zline = destroy_line(zline);
		if (!zdisplay) return TRUE;
		}

#	ifdef DEBUG_PATTERN
	(void) fprintf(stderr,
		"[draw_pattern]  Drawing pattern: %s\n", pattern);
#	endif /* DEBUG_PATTERN */

	/* Protect current graphics pipe */
	(void) push_save();

	/* Replicate each component of the pattern along the whole line */
	for (icomp=0; icomp<p->num; icomp++)
		{
		parea  = NullArea;
		pcurve = NullCurve;

		if (same(p->type[icomp], "area"))
			{
			parea = (AREA)  p->list[icomp];
			define_lspec_value(&parea->lspec, LINE_HILITE, (POINTER)&hilite);
			define_fspec_value(&parea->fspec, FILL_HILITE, (POINTER)&hilite);
			gxLineSpec(&parea->lspec);
			gxFillSpec(&parea->fspec, FALSE);

#			ifdef DEBUG_PATTERN
			(void) fprintf(stderr,
				"[draw_pattern]  Area component: %d  width/length: %.2g %.2g  colour: %d\n",
				icomp, parea->lspec.width, parea->lspec.length, parea->lspec.hilite);
#			endif /* DEBUG_PATTERN */

			pline = parea->bound->boundary;
			poly  = TRUE;
			}

		else if (same(p->type[icomp], "curve"))
			{
			pcurve = (CURVE) p->list[icomp];
			define_lspec_value(&pcurve->lspec, LINE_HILITE, (POINTER)&hilite);
			gxLineSpec(&pcurve->lspec);

#			ifdef DEBUG_PATTERN
			(void) fprintf(stderr,
				"[draw_pattern]  curve component: %d  width/length: %.2g %.2g  colour: %d\n",
				icomp, pcurve->lspec.width, pcurve->lspec.length,
				pcurve->lspec.hilite);
#			endif /* DEBUG_PATTERN */

			pline = pcurve->line;
			poly  = FALSE;
			}

		else continue;

		/* Match pattern component against each span of the given curve */
		/* Each span defines a trapeziod, having a height equal to twice the */
		/* signed amplitude, and mean length equal to the length of the span */
		/* The ends of the trapezoid are tilted according to the average of */
		/* the span perpendicular with that of either adjacent span */
		/* Hence we need to keep track of 3 spans (4 points) running: */
		/*    The 4 points are refered to as a, b, c and d in sequence */
		/*    Span prev (ab) is the span preceding the current span */
		/*    Span curr (bc) is the span that we are currently plotting */
		/*    Span next (cd) is the span following the current span */
		/*    frst and last tell whether prev or next span are absent */
		/*    dx.. and dy.. are the x and y lengths of the corresponding span */
		/*    nx.. and ny.. are the x and y normals of the corresponding span */
		/*    nx.bar and ny.bar are the average normals at the span ends */
		xb = points[0][X];
		yb = points[0][Y];
		for (ispan=0; ispan<numpts-1; ispan++)
			{
			xc     = points[ispan+1][X];
			yc     = points[ispan+1][Y];
			dxcurr = xc - xb;
			dycurr = yc - yb;
			dscurr = hypot(dxcurr, dycurr);
			if (dscurr > 0) break;
			}
		if (ispan >= numpts-1) continue;

		lpos   = 0;
		rpos   = lpos + dscurr;
		nscale = width / dscurr;
		nxcurr = -dycurr * nscale;
		nycurr = dxcurr * nscale;
		nxlbar = nxcurr;
		nylbar = nycurr;
		nxrbar = nxcurr;
		nyrbar = nycurr;
		for ( ; ispan<numpts-1; ispan++)
			{

#			ifdef DEBUG_PATTERN_POINTS
			(void) fprintf(stderr,
				"[draw_pattern] ispan: %d  nxcurr/nycurr: %.2f %.2f   nxlbar/nylbar: %.2f %.2f\n",
				ispan, nxcurr, nycurr, nxlbar, nylbar);
#			endif /* DEBUG_PATTERN_POINTS */

			/* Make sure we remain continuous with following span */
			last = (int) (ispan >= numpts-2);
			if (!last)
				{
				xd     = points[ispan+2][X];
				yd     = points[ispan+2][Y];
				dxnext = xd - xc;
				dynext = yd - yc;
				dsnext = hypot(dxnext, dynext);
				if (dsnext <= 0) continue;

				nscale = width / dsnext;
				nxnext = -dynext * nscale;
				nynext = dxnext * nscale;
				nxrbar = (nxcurr+nxnext)/2;
				nyrbar = (nycurr+nynext)/2;	
				}

#			ifdef DEBUG_PATTERN_POINTS
			(void) fprintf(stderr,
				"[draw_pattern]   nxnext/nynext: %.2f %.2f  nxrbar/nyrbar: %.2f %.2f\n",
				nxnext, nynext, nxrbar, nyrbar);
#			endif /* DEBUG_PATTERN_POINTS */

			/* Map the trapezoid to the corresponding portion of the pattern */
			lpat = lpos;
			while (lpat < rpos)
				{
				/* Break apart at multiples of the pattern length */
				if (rpos >= length)
					{
					rclose = TRUE;
					rpat   = length;
					}
				else
					{
					rclose = last;
					rpat   = rpos;
					}
				
				/* Set up clipping to the current portion of the pattern */
				/* Clip the current pattern component and display the pieces */
				reset_pipe();
				enable_clip(lpat, rpat, -width, width, poly, poly);
				enable_save();
				line_pipe(pline);
				nline = recall_save(&lines);
				for (iline=0; iline<nline; iline++)
					{
					tline = copy_line(lines[iline]);
					for (ip=0; ip<tline->numpts; ip++)
						{
						xpat = tline->points[ip][X];
						ypat = tline->points[ip][Y];
						wc = (xpat-lpos)/dscurr;
						wb = 1 - wc;
						ww = ypat/width;			

						x  = wb*(xb + ww*nxlbar) + wc*(xc + ww*nxrbar);
						y  = wb*(yb + ww*nylbar) + wc*(yc + ww*nyrbar);

#						ifdef DEBUG_PATTERN_POINTS
						(void) fprintf(stderr,
							"[draw_pattern]    ip: %d  xpat/ypat: %.2f %.2f  x/y: %.2f %.2f\n",
							ip, xpat, ypat, x, y);
#						endif /* DEBUG_PATTERN_POINTS */

						tline->points[ip][X] = x;
						tline->points[ip][Y] = y;
						}
					if (poly)
						{
						glFilledPolygon(tline->numpts, tline->points);
						glPolygon(tline->numpts, tline->points);
						}
					else
						{
						glPolyLine(tline->numpts, tline->points);
						}
					tline = destroy_line(tline);
					}

				if (rclose)
					{
					lpos -= length;
					rpos -= length;
					lpat -= length;
					rpat  = 0;
					}
				rclose = FALSE;
				lpat   = rpat;
				}

			/* Close off last polygon if filled */
			if (poly && last)
				{
				}

			/* Prepare next span */
			if (!last)
				{
				xb     = xc;		yb     = yc;
				xc     = xd;		yc     = yd;
				dxcurr = dxnext;	dycurr = dynext;
				nxcurr = nxnext;	nycurr = nynext;
				nxlbar = nxrbar;	nylbar = nyrbar;
				nxrbar = nxcurr;	nyrbar = nycurr;
				dscurr = dsnext;
				lpos   = rpos;
				rpos   = lpos + dscurr;
				}
			}
		}

	/* Restore original graphics pipe */
	(void) pop_save();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      g e t _ p a t t e r n                                           *
*      f i n d _ p a t t e r n                                         *
*      r e a d _ p a t t e r n                                         *
*                                                                      *
*      Obtain the pattern structure corresponding to the specified     *
*      pattern name.  New patterns are read in from the corresponding  *
*      pattern metafile, and the pattern components are saved in a     *
*      list of pattern structures.  Patterns already encountered are   *
*      retrieved from the pattern list.                                *
*                                                                      *
***********************************************************************/

static	PATTERN	*PatternList = NULL;
static	int		NumPatterns  = 0;

static	PATTERN	*get_pattern

	(
	STRING	pattern,	/* pattern name */
	HAND	sense,		/* pattern handedness (flip sense) */
	float	*width,		/* pattern width factor */
	float	*length		/* pattern repetition factor */
	)

	{
	static	PATTERN	P = { NULL, 0, NULL, NULL, 1, BaseC };

	PATTERN	*p;
	int		ic;
	ITEM	item;
	STRING	type;
	float	xscale, yscale, yoff;

	/* Initialize current pattern */
	if (P.num > 0)
		{
		for (ic=0; ic<P.num; ic++)
			{
			destroy_item(P.type[ic], P.list[ic]);
			FREEMEM(P.type[ic]);
			}
		P.name   = NULL;
		P.num    = 0;
		P.repeat = 1;
		P.base   = BaseC;
		}

	/* Find the pattern if already encountered */
	/* Otherwise try to read it in */
	p = find_pattern(pattern);
	if (!p) p = read_pattern(pattern);
	if (!p) return NULL;

	/* Copy the pattern */
	P.name   = p->name;
	P.num    = p->num;
	P.list   = GETMEM(P.list, ITEM, P.num);
	P.type   = GETMEM(P.type, STRING, P.num);
	P.repeat = p->repeat;
	P.base   = p->base;

	/* Scale the pattern to the specified width and length */
	xscale = *length * P.repeat;
	yscale = (sense == Left)? -*width: *width;
	yoff   = 0;
	*length *= P.repeat;
	if (!AnchorBaseline)
		{
		switch (P.base)
			{
			case BaseT:
				yoff =  0.5*SIGN(yscale);
				break;

			case BaseB:
				yoff = -0.5*SIGN(yscale);
				break;
			}
		}
	for (ic=0; ic<P.num; ic++)
		{
		type = p->type[ic];
		item = copy_item(type, p->list[ic]);
		if (yoff != 0.0) offset_item(type, item, 0.0, yoff);
		scale_item(type, item, xscale, yscale);

		P.list[ic] = item;
		P.type[ic] = strdup(type);
		}

	return &P;
	}

static	PATTERN	*find_pattern

	(
	STRING	pattern	/* pattern name */
	)

	{
	PATTERN	*p;
	int		ip;

	for (ip=0; ip<NumPatterns; ip++)
		{
		p = PatternList + ip;
		if (same(p->name, pattern)) return p;
		}

	return NULL;
	}

static	PATTERN	*read_pattern

	(
	STRING	pattern	/* pattern name */
	)

	{
	PATTERN		*p;
	STRING		pname;
	METAFILE	meta;
	FIELD		fld;
	int			ifld, ilist, ic;
	SET			set;
	STRING		type;
	int			ncspec;
	CATSPEC		*cspecs;
	ITEM		item;
	float		width, length, lscale, wscale, rpt;
	int			ninfo, ii;
	TABLE		*info;
	LOGICAL		ok;

	/* Try to read the metafile */
	pname = get_file("patterns", env_sub(pattern));
	meta  = read_metafile(pname, NullMapProj);
	if (!meta)
		{
		(void) fprintf(stderr,
			"[Display] ERROR! Cannot read \"patterns\" file:  %s\n",
			env_sub(pattern));
		return NULL;
		}
	(void) save_pattern_info(pattern, pname, 0);
	width  = meta->mproj.definition.ylen;	if (width  <= 0) width  = 100;
	length = meta->mproj.definition.xlen;	if (length <= 0) length = 100;
	wscale = 1/width;
	lscale = 1/length;

#	ifdef DEBUG_PATTERN_READ
	(void) fprintf(stderr,
		"[read_pattern] Reading pattern: %s  wscale/lscale: %g %g\n",
		pattern, wscale, lscale);
#	endif /* DEBUG_PATTERN_READ */

	/* Expand the pattern list */
	NumPatterns++;
	PatternList = GETMEM(PatternList, PATTERN, NumPatterns);
	p = PatternList + NumPatterns - 1;
	p->name   = strdup(pattern);
	p->num    = 0;
	p->list   = NULL;
	p->type   = NULL;
	p->repeat = 1;
	p->base   = BaseC;

	/* Extract the components */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* Use the item list from set fields */
		if (fld->ftype == FtypeSet)
			{
			set = fld->data.set;
			if (!set)          continue;
			if (set->num <= 0) continue;

			/* Expand the pattern item list */
			ic      = p->num;
			p->num += set->num;
			p->list = GETMEM(p->list, ITEM, p->num);
			p->type = GETMEM(p->type, STRING, p->num);

			/* Take the items from the set */
			type   = set->type;
			ncspec = set->ncspec;
			cspecs = set->cspecs;
			for (ilist=0; ilist<set->num; ilist++, ic++)
				{
				item = copy_item(type, set->list[ilist]);
				invoke_item_catspec(type, item, ncspec, cspecs);
				scale_item(type, item, lscale, wscale);

				p->list[ic] = item;
				p->type[ic] = strdup(type);
				}
			}

		}

	/* Clean up */
	meta = destroy_metafile(meta);

	/* Interpret additional pattern info from the metafile */
	if (read_meta_info(pname, "pattern", &ninfo, &info))
		{
		for (ii=0; ii<ninfo; ii++)
			{
			if (same(info[ii].index, "repeat"))
				{
				rpt = float_arg(info[ii].value, &ok);
				if (ok) p->repeat = rpt;
				}
			else if (same(info[ii].index, "baseline"))
				{
				if (same_ic(info[ii].value, "top"))
					{
					p->base = BaseT;
					}
				else if (same_ic(info[ii].value, "bottom"))
					{
					p->base = BaseB;
					}
				}
			FREEMEM(info[ii].index);
			FREEMEM(info[ii].value);
			}
		FREEMEM(info);
		}

	/* Return the pattern info */
	return p;
	}

/***********************************************************************
*                                                                      *
*      g e t _ p a t t e r n _ i n f o                                 *
*      s a v e _ p a t t e r n _ i n f o                               *
*      f i n d _ p a t t e r n _ i n f o                               *
*                                                                      *
*      Return info about the given pattern.                            *
*                                                                      *
***********************************************************************/

typedef struct psym_struct
	{
	STRING	name;
	LOGICAL	sym;
	} PINFO;

static	PINFO	*PatternInfo = (PINFO *)0;
static	int		NPatternInfo = 0;

/**********************************************************************/

LOGICAL	get_pattern_info

	(
	STRING	name,
	LOGICAL	*sym
	)

	{
	return save_pattern_info(name, NULL, sym);
	}

/**********************************************************************/

static	LOGICAL	save_pattern_info

	(
	STRING	name,
	STRING	file,
	LOGICAL	*sym
	)

	{
	int		ip, ilist, nlist;
	TABLE	*list;

	if (blank(name)) return FALSE;

	if (find_pattern_info(name, sym)) return TRUE;

	if (blank(file))
		{
		file = get_file("patterns", env_sub(name));
		if (blank(file)) return FALSE;
		}

	if (!read_meta_info(file, "pattern", &nlist, &list)) return FALSE;
	if (nlist <= 0) return FALSE;

	for (ilist=0; ilist<nlist; ilist++)
		{
		if (same_ic(list[ilist].index, "symmetric"))
			{
			ip = NPatternInfo++;
			PatternInfo = GETMEM(PatternInfo, PINFO, NPatternInfo);
			PatternInfo[ip].name = strdup(name);
			PatternInfo[ip].sym  = same_ic(list[ilist].value, "yes");
			if (sym) *sym = PatternInfo[ip].sym;
			break;
			}
		}

	for (ilist=0; ilist<nlist; ilist++)
		{
		FREEMEM(list[ilist].index);
		FREEMEM(list[ilist].value);
		}
	FREEMEM(list);
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	find_pattern_info

	(
	STRING	name,
	LOGICAL	*sym
	)

	{
	int		ip;

	for (ip=0; ip<NPatternInfo; ip++)
		{
		if (same(PatternInfo[ip].name, name))
			{
			if (sym) *sym = PatternInfo[ip].sym;
			return TRUE;
			}
		}

	return FALSE;
	}
