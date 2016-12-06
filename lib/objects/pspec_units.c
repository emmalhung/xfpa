/***********************************************************************
*                                                                      *
*      p s p e c _ u n i t s . c                                       *
*                                                                      *
*      Routines to handle units presentation specs.                    *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "pspec.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

/***********************************************************************
*                                                                      *
*      i n i t _ u s p e c                                             *
*      s k i p _ u s p e c                                             *
*      f r e e _ u s p e c                                             *
*      c o p y _ u s p e c                                             *
*      d e f i n e _ u s p e c                                         *
*      r e c a l l _ u s p e c                                         *
*                                                                      *
*      Routines to manage units presentation specs.                    *
*                                                                      *
***********************************************************************/
void	init_uspec

	(
	USPEC	*uspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!uspec) return;

	/* Use reasonable initial values */
	uspec->name      = SafeUname;
	uspec->factor    = SafeUfactor;
	uspec->offset    = SafeUoffset;
	}

void	skip_uspec

	(
	USPEC	*uspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!uspec) return;

	/* Reset all values to skip values */
	free_uspec(uspec);
	uspec->name      = SkipUname;
	uspec->factor    = SkipUfactor;
	uspec->offset    = SkipUoffset;
	}

void	free_uspec

	(
	USPEC	*uspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!uspec) return;

	/* Free any allocated members */
	FREEMEM(uspec->name);

	/* Give back an initialized one */
	init_uspec(uspec);
	}

void	copy_uspec

	(
	USPEC		*unew,
	const USPEC	*uspec
	)

	{
	/* Do nothing if target does not exist */
	if (!unew) return;

	/* If the source does not exist initialize the target */
	if (!uspec)
		{
		free_uspec(unew);
		return;
		}

	/* Duplicate all values */
	define_uspec(unew,uspec->name,uspec->factor,uspec->offset);
	}

void	define_uspec

	(
	USPEC	*uspec,
	STRING	name,
	double	factor,
	double	offset
	)

	{
	/* Do nothing if structure does not exist */
	if (!uspec) return;

	/* Use all the given values */
	if (name      != SkipUname)   uspec->name      = STRMEM(uspec->name,name);
	if (factor    != SkipUfactor) uspec->factor    = factor;
	if (offset    != SkipUoffset) uspec->offset    = offset;
	}

void	recall_uspec

	(
	USPEC	*uspec,
	STRING	*name,
	double	*factor,
	double	*offset
	)

	{
	/* Return only what was asked for */
	if (name)      *name      = (uspec) ? uspec->name      : NULL;
	if (factor)    *factor    = (uspec) ? uspec->factor    : SafeUfactor;
	if (offset)    *offset    = (uspec) ? uspec->offset    : SafeUoffset;
	}

double	convert_by_uspec

	(
	const USPEC	*to,
	const USPEC	*from,
	double		value
	)

	{
	/* Take drastic measures if the unit specs aren't given */
	if (!to)   return value;
	if (!from) return value;

	/* Don't convert if same units */
	if ( same(to->name, from->name) ) return value;

	value -= from->offset;
	value /= from->factor;
	value *= to->factor;
	value += to->offset;
	return value;
	}

#ifdef DEBUG_PSPEC
/***********************************************************************
*                                                                      *
*      d e b u g _ u s p e c                                           *
*                                                                      *
***********************************************************************/

void	debug_uspec(USPEC	*uspec ,
                    STRING	msg ,
                    int	indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (uspec)
		{
		(void) printf(" %s %g %g %g",uspec->name,uspec->factor,uspec->offset);
		}
	(void) printf("\n");
	}

#endif /* DEBUG_PSPEC */
