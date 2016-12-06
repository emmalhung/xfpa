/*********************************************************************/
/** @file plot.h
 *
 * PLOT object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    p l o t . h                                                       *
*                                                                      *
*    PLOT object definition (include file)                             *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

/* See if already included */
#ifndef PLOT_DEFS
#define PLOT_DEFS

/* Include things that can belong to a plot */
#include "item.h"

#define	DELTA_PLOT 1

/* Define PSUB structure */
/** a sub-field or member of a plot */
typedef	struct
	{
	char	*name;			/**< sub-field identifier */
	char	*type;			/**< type of item in this sub-field */
	ITEM	proto;			/**< prototype item showing defaults and offset */
	STRING	*sval1, *sval2;	/**< string values */
	int		*ival1, *ival2;	/**< integer values !!! in for completeness !!! */
	float	*fval1, *fval2;	/**< float values */
	} PSUB;

/* Define PLOT object */
/** a set of similar groups of dis-similar items */
typedef struct PLOT_struct
	{
	PSUB	*subs;		/**< list of sub-fields */
	int		nsubs;		/**< number of sub-fields */
	POINT	*pts;		/**< plot points */
	int		numpts;		/**< number of points in plot */
	int		maxpts;		/**< allocated points in plot */
	PLTSPEC	*cspecs;	/**< list of plot subfield specs */
	short	ncspec;		/**< number of plot subfield specs */
	} *PLOT;

/* Convenient definitions */
#define NullPsub     NullPtr(PSUB)
#define NullPsubList NullPtr(PSUB *)
#define NullPlot     NullPtr(PLOT)
#define NullPlotPtr  NullPtr(PLOT *)

/* Declare all functions in plot.c */
PLOT	create_plot(void);
PLOT	destroy_plot(PLOT plot);
void	empty_plot(PLOT plot);
PLOT	copy_plot(const PLOT plot);
void	add_subfld_to_plot(PLOT plot, STRING subname, STRING type, ITEM item);
void	add_point_to_plot(PLOT plot, POINT pos);
void	remove_point_from_plot(PLOT plot, POINT pos);
void	define_subfld_value(PLOT plot, STRING subname, int member,
						STRING c1, STRING c2, int i1, int i2,
						float f1, float f2);
void	take_subfld_value(PLOT plot, STRING subname, int member, ITEM item);
void	define_plot_pltspecs(PLOT plot, int ncspec, PLTSPEC *cspecs);
void	add_pltspec_to_plot(PLOT plot, PLTSPEC *cspec);
void	recall_plot_pltspecs(PLOT plot, int *ncspec, PLTSPEC **cspecs);
PLTSPEC	*find_plot_pltspec(PLOT plot, STRING subname);
void	invoke_plot_pltspecs(PLOT plot);
void	highlight_plot(PLOT plot, HILITE hilite);
void	highlight_subfld(PLOT plot, STRING subname, HILITE hilite);
void	change_subfld_pspec(PLOT plot, STRING subname,
						PPARAM param, POINTER value);
void	recall_subfld_pspec(PLOT plot, STRING subname,
						PPARAM param, POINTER value);
int		which_plot_subfld(PLOT plot, STRING subname);
int		which_plot_point(PLOT plot, POINT pos);
int		closest_plot_point(PLOT plot, POINT ptest, float *dist, POINT point);
void	strip_plot(PLOT plot, const BOX *box);
LOGICAL	reproject_plot(PLOT plot,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);

/* Now it has been included */
#endif
