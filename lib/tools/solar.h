/**********************************************************************/
/** @file solar.h
 *
 * 	Routines to compute solar attributes. (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    s o l a r . h                                                     *
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

int		seconds_from_gmt (float lon);
int		minutes_from_gmt (float lon);
int		hours_from_gmt (float lon);

void	solar_parms (int year, int jday, int ztime, double *sdecl,
				double *eqtime, double *sdist);
double	eqn_time (int year, int jday, int ztime);
double	solar_decl (int year, int jday, int ztime);
double	solar_dist (int year, int jday, int ztime);

void	sun_pos (int year, int jday, int ztime, float *lat, float *lon);
void	sun_angle (int year, int jday, int ztime, float blat, float blon,
				float *azimuth, float *zenith);
void	sun_pos_angle (float slat, float slon, float blat, float blon,
				float *azimuth, float *zenith);

double	solar_flux (int year, int jday, int ztime, float blat, float blon);
double	sun_pos_flux (float slat, float slon, float dfact, float blat,
				float blon);

void	sun_times (int year, int jday, float blat, float blon, int *zrise,
				int *zset);
