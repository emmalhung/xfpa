/***********************************************************************
*                                                                      *
*     s a m p l e r _ o l d . c                                        *
*                                                                      *
*     Routines to access the Depiction Sampler Program.  These         *
*     make extensive use of the Inter-Process Communications library   *
*     "ipc.c".                                                         *
*                                                                      *
*     These functions provide the old-style interface to the new       *
*     sampler.  This feature is provided for compatibility only, and   *
*     will not be maintained for long.  It is recommended that the     *
*     new access funtions in sampler_access.c be used.                 *
*                                                                      *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
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

#include "sampler.h"

#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <string.h>
#include <stdio.h>

static	int		status;				/* Status indicator */
static	int		nchart = 0;			/* Number of charts in current sequence */
static	int		*tlist = NullInt;	/* Current list of prog times */

/***********************************************************************
*                                                                      *
*     s a m p l e r _ c o n n e c t                                    *
*                                                                      *
*     Establish communications with the Depiction Sampler Program.     *
*                                                                      *
***********************************************************************/

int		sampler_connect

	(
	STRING	setup_file
	)

	{
	return fpa_sampler_connect(setup_file);
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ d i s c o n n e c t                              *
*                                                                      *
*     Terminate communications with the Depiction Sampler Program.     *
*                                                                      *
***********************************************************************/

int	sampler_disconnect(void)

	{
	return fpa_sampler_disconnect();
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ s e t u p                                        *
*                                                                      *
*     Instruct the Depiction Sampler Program to access the specified   *
*     setup file.                                                      *
*                                                                      *
*     Success or failure is returned.                                  *
*                                                                      *
***********************************************************************/

int		sampler_setup

	(
	STRING	setup_file
	)

	{
	return fpa_sampler_connect(setup_file);
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ s e q u e n c e                                  *
*                                                                      *
*     Instruct the Depiction Sampler Program to input a new depiction  *
*     sequence.                                                        *
*                                                                      *
*     The number of charts in the sequence is returned.                *
*                                                                      *
***********************************************************************/

int		sampler_sequence

	(
	STRING	dir,
	int		year,
	int		jday,
	int		hour,
	int		maxprog
	)

	{
	int		iprog, sprog, nprog, *progs, ichart;

	/* Get available prog-times */
	status = fpa_sampler_source(dir, NullString);
	if (status < 0) return status;
	status = fpa_sampler_rtime(year, jday, hour);
	if (status < 0) return status;
	status = fpa_sampler_avail_tplus(&progs);
	if (status < 0) return status;
	nprog = status;

	/* Determine which prog-times are in the requested range */
	sprog  = -1;
	nchart = 0;
	for (iprog=0; iprog<nprog; iprog++)
		{
		if (progs[iprog] < 0)       continue;
		if (progs[iprog] > maxprog) break;;

		if (sprog < 0)
			{
			sprog = iprog;
			if (progs[iprog] > 0 && iprog > 0) sprog--;
			}
		nchart++;
		}

	/* Save the actual prog times */
	tlist = GETMEM(tlist,int,nchart);
	for (ichart=0; ichart<nchart; ichart++)
		{
		tlist[ichart] = progs[ichart+sprog];
		}

	/* Now request the prog-times */
	status = fpa_sampler_tplus(nchart, tlist);
	if (status < 0) return status;

	return nchart;
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ p r o g _ t i m e                                *
*                                                                      *
*     Return the prog time of the given chart.                         *
*                                                                      *
***********************************************************************/

int	sampler_prog_time

	(
	int	ichart
	)

	{
	/* Make sure there is a current depiction */
	if (nchart <= 0) return FAILURE;
	if (!tlist)      return FAILURE;

	/* Make sure requested chart is valid */
	if (ichart < 0)       return FAILURE;
	if (ichart >= nchart) return FAILURE;

	return tlist[ichart];
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ e v a l u a t e                                  *
*                                                                      *
*     Instruct the Depiction Sampler Program to evaluate the given     *
*     element at the given location in each frame of the current       *
*     depiction sequence.                                              *
*                                                                      *
*     The number of charts in the sequence is returned.                *
*                                                                      *
***********************************************************************/

int		sampler_evaluate

	(
	STRING	elem,
	STRING	level,
	int		lat,
	int		lon
	)

	{
	char	abuf[80], bbuf[80];
	STRING	alist[1], blist[1];
	float	degrees();

	/* Set up field and point */
	(void) safe_strcpy(abuf, elem);
	(void) safe_strcpy(bbuf, level);
	alist[0] = abuf;
	blist[0] = bbuf;
	status = fpa_sampler_field(1, alist, blist);
	if (status < 0) return status;
	(void) sprintf(abuf, "%f", degrees(lat));
	(void) sprintf(bbuf, "%f", -degrees(lon));
	alist[0] = abuf;
	blist[0] = bbuf;
	status = fpa_sampler_point(1, alist, blist);
	if (status < 0) return status;

	/* Do the evaluation */
	return fpa_sampler_evaluate();
	}

/***********************************************************************
*                                                                      *
*     s a m p l e r _ g e t _ v a l u e                                *
*                                                                      *
*     Return the sampled value from the given chart.                   *
*                                                                      *
***********************************************************************/

STRING	sampler_get_value

	(
	int		ichart
	)

	{
	return fpa_sampler_get_value(ichart, 0, 0);
	}
