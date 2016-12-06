/*********************************************************************/
/** @file solar.c
 *
 * Routines to compute solar attributes.
 *
 * Reference for solar parameters, equation of time, etc:
 * THE ASTRONOMICAL ALMANAC FOR THE YEAR 1993, page C24
 * (Equation of time accurate to 6 seconds between 1950 and 2050)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     s o l a r . c                                                    *
*                                                                      *
*     Routines to compute solar attributes.                            *
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

#include "solar.h"
#include "time.h"

#include <fpa_math.h>
#include <stdio.h>

/* Define various constants */
static	const	double	secperday     = 86400;
static	const	double	secperdeg     = 86400/360;
/* May come in handy later:
static	const	double	tropical_year = 365.2422;
*/

/* Local static functions */
static	double	julian(int, int, int);
static	double	ref_days(int, int, int);

/***********************************************************************
*                                                                      *
*     Static Functions:                                                *
*                                                                      *
*     j u l i a n   - Compute the astronomical Julian date for the     *
*                     given date-time.                                 *
*                                                                      *
*     r e f _ d a y s   - Compute number of days from the reference    *
*                         date-time to the given date-time.            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Compute the astronomical Julian date for the given date-time.
 *
 *	@param[in]	year	Year (full year in Gregorian calendar)
 *	@param[in]	jday	Day of year
 *	@param[in]	ztime	GMT time in seconds
 * 	@return Astronomical Julian date.
 *********************************************************************/
static	double	julian

	(
	int	year,
	int	jday,
	int	ztime
	)

	{
	/* Define the conventional epoch time */
	static	const	long	epoch_year  = -4713;	/* 4713 BC */
	static	const	long	epoch_jday  = 1;		/* Jan 1st */
	static	const	long	epoch_ztime = 86400/2;	/* mean noon */

	double	jd;

	jd  = (double) jdif(epoch_year, epoch_jday, year, jday);
	jd += (ztime-epoch_ztime) / secperday;
	return jd;
	}

/*********************************************************************/
/** Compute the number of days from the reference date-time to the
 * given date-time.
 *
 *	@param[in]	year	Year (full year in Gregorian calendar)
 *	@param[in]	jday	Day of year
 *	@param[in]	ztime	GMT time in seconds
 * 	@return Difference in Days.
 *********************************************************************/
static	double	ref_days

	(
	int	year,
	int	jday,
	int	ztime
	)

	{
	/* Define the reference time for equation of time, etc. */
	static	const	long	ref_year  = 2000;		/* 2000 AD */
	static	const	long	ref_jday  = 1;			/* Jan 1st */
	static	const	long	ref_ztime = 86400/2;	/* mean noon */
	static			double	ref_jd    = -1;

	/* Convert reference date-time to full Julian date */
	if (ref_jd < 0)
		{
		ref_jd = julian(ref_year, ref_jday, ref_ztime);
		}

	/* Return the difference */
	return (julian(year, jday, ztime) - ref_jd);
	}

/***********************************************************************
*                                                                      *
*     s e c o n d s _ f r o m _ g m t   - Compute the local mean time  *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Compute the local mean time difference from Greenwich Meridian
 * to the given longitude.
 *
 *	@param[in]	lon		longitude (real degrees, +=E)
 * 	@return The number of seconds from gmt.
 *********************************************************************/

int	seconds_from_gmt

	(
	float	lon
	)

	{
	return (int) (secperdeg * lon);
	}

/*********************************************************************/
/** Compute the local mean time difference from Greenwich Meridian
 * to the given longitude.
 *
 *	@param[in]	lon		longitude (real degrees, +=E)
 * 	@return The number of minutes from gmt.
 *********************************************************************/
int	minutes_from_gmt

	(
	float	lon
	)

	{
	return NINT( (secperdeg*lon)/60.0 );
	}

/*********************************************************************/
/** Compute the local mean time difference from Greenwich Meridian
 * to the given longitude.
 *
 *	@param[in]	lon		longitude (real degrees, +=E)
 * 	@return The number of hours from gmt.
 *********************************************************************/
int	hours_from_gmt

	(
	float	lon
	)

	{
	return NINT( (secperdeg*lon)/3600.0 );
	}

/***********************************************************************
*                                                                      *
*     s o l a r _ p a r m s                                            *
*     e q n _ t i m e                                                  *
*     s o l a r _ d e c l                                              *
*     s o l a r _ d i s t                                              *
*                                                                      *
*     Reference for solar parameters, equation of time, etc:           *
*     THE ASTRONOMICAL ALMANAC FOR THE YEAR 1993, page C24             *
*     (Equation of time accurate to 6 seconds between 1950 and 2050)   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Compute various solar parameters for the given date_time.
 *
 *	@param[in]	year		 Year (full year in Gregorian calendar)
 *	@param[in]	jday		 Day of year
 *	@param[in]	ztime		 GMT time in seconds
 *	@param[out]	*sdecl		 Solar declination
 *	@param[out]	*eqtime		 Equation of time
 *	@param[out]	*sdist		 Earth-Sun distance
 *********************************************************************/
void	solar_parms

	(
	int		year,
	int		jday,
	int		ztime,
	double	*sdecl,
	double	*eqtime,
	double	*sdist
	)

	{
	double	n, L, g, lambda, eps, t, alpha;

	/* Make sure we want something returned */
	if (!sdecl && !eqtime && !sdist) return;

	/* Days from reference date */
	n = ref_days(year, jday, ztime);

	/* Mean anomaly (in radians) */
	g = RAD * (357.528 + 0.9856003*n);
	g = fmod(g, 2*PI);

	/* Parameters needed for solar declination and equation of time */
	if (sdecl || eqtime)
		{
		/* Mean longitude of Sun, corrected for aberration (in radians) */
		L = RAD * (280.460 + 0.9856474*n);
		L = fmod(L, 2*PI);

		/* Ecliptic longitude (in radians) */
		lambda = L + RAD * (1.915*sin(g) + 0.020*sin(2*g));

		/* Obliquity of ecliptic (in radians) */
		eps = RAD * (23.439 - 0.0000004*n);
		}

	/* We want solar declination */
	if (sdecl)
		{
		/* Declination (in radians) */
		*sdecl = asin(sin(eps)*sin(lambda));
		}

	/* We want equation of time */
	if (eqtime)
		{
		/* Right ascension (in radians) */
		t  = tan(eps/2);
		t *= t;
		alpha = lambda - t*sin(2*lambda) + (t*t/2)*sin(4*lambda);

		/* Equation of time (apparent time - mean time) (in radians) */
		*eqtime = L - alpha;
		}

	/* We want Earth-Sun distance */
	if (sdist)
		{
		/* Earth-Sun distance (in au, i.e. d/dm) */
		*sdist = 1.00014 - 0.01671*cos(g) - 0.00014*cos(2*g);
		}
	}

/*********************************************************************/
/** Compute the equation of time for the given date-time.
 * This gives the difference between the mean time
 * (assuming exactly 24 hours per day) and the apparent time which
 * accounts for the variation in the length of the day due to the
 * earth's elliptical orbit.
 *
 *	@param[in]	year		Year (full year in Gregorian calendar)
 *	@param[in]	jday		Day of year
 *	@param[in]	ztime		GMT time in seconds
 * 	@return Difference between mean and apparent time.
 *********************************************************************/
double	eqn_time

	(
	int	year,
	int	jday,
	int	ztime
	)

	{
	double	eqtime;

	/* Compute the required solar parameters */
	solar_parms(year, jday, ztime, NULL, &eqtime, NULL);
	return eqtime;
	}

/*********************************************************************/
/** Compute the solar declination for the given date-time.
 *
 *	@param[in]	year		Year (full year in Gregorian calendar)
 *	@param[in]	jday		Day of year
 *	@param[in]	ztime		GMT time in seconds
 *	@return Solar declination.
 *********************************************************************/
double	solar_decl

	(
	int	year,
	int	jday,
	int	ztime
	)

	{
	double	sdecl;

	/* Compute the required solar parameters */
	solar_parms(year, jday, ztime, &sdecl, NULL, NULL);
	return sdecl;
	}

/*********************************************************************/
/** Compute the Earth-Sun distance in au. This is the ratio d/dm.
 *
 *	@param[in]	year		Year (full year in Gregorian calendar)
 *	@param[in]	jday		Day of year
 *	@param[in]	ztime		GMT time in seconds
 * 	@return Earth to Sun distance in au.
 *********************************************************************/
double	solar_dist

	(
	int	year,
	int	jday,
	int	ztime
	)

	{
	double	sdist;

	/* Compute the required solar parameters */
	solar_parms(year, jday, ztime, NULL, NULL, &sdist);
	return sdist;
	}

/***********************************************************************
*                                                                      *
*     s u n _ p o s                                                    *
*     s u n _ a n g l e                                                *
*     s u n _ p o s _ a n g l e                                        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Compute the location of the sun (lat-lon that the sun is directly
 * over) at the given date-time.
 *	@param[in]	year		 Year (full year in Gregorian calendar)
 *	@param[in]	jday		 Day of year
 *	@param[in]	ztime		 GMT time in seconds
 *	@param[out]	*lat		 latitude of sun (real degrees, +=N)
 *	@param[out]	*lon		 longitude of sun (real degrees, +=E)
 *********************************************************************/
void	sun_pos

	(
	int		year,
	int		jday,
	int		ztime,
	float	*lat,
	float	*lon
	)

	{
	double	sdecl, eqtime, meantime, apptime;

	solar_parms(year, jday, ztime, &sdecl, &eqtime, NULL);

	meantime = RAD * (ztime - secperday/2) / secperdeg;
	apptime  = meantime + eqtime;

	if (lat) *lat = (float) (sdecl / RAD);
	if (lon) *lon = (float) (-apptime / RAD);
	}

/*********************************************************************/
/** Compute the azimuth and zenith angles of the sun relative to a
 * given location on the earth, at the given date-time.
 *	@param[in]  year		 Year (full year in Gregorian calendar)
 *	@param[in]  jday		 Day of year
 *	@param[in]  ztime		 GMT time in seconds
 *	@param[in]  blat		 latitude (real degrees, +=N)
 *	@param[in]  blon		 longitude (real degrees, +=E)
 *	@param[out] *azimuth	 azimuth angle of sun (degrees wrt S along blon)
 *	@param[out] *zenith		 zenith angle of sun (degrees)
 *********************************************************************/
void	sun_angle

	(
	int		year,
	int		jday,
	int		ztime,
	float	blat,
	float	blon,
	float	*azimuth,
	float	*zenith
	)

	{
	float	slat, slon;

	sun_pos(year, jday, ztime, &slat, &slon);
	sun_pos_angle(slat, slon, blat, blon, azimuth, zenith);
	}

/*********************************************************************/
/** Compute the sun's azimuth and zenith angles for the given sun location.
 *	@param[in]	slat		 latitude of sun (real degrees, +=N)
 *	@param[in]	slon		 longitude of sun (real degrees, +=E)
 *	@param[in]	blat		 latitude (real degrees, +=N)
 *	@param[in]	blon		 longitude (real degrees, +=E)
 *	@param[out]	*azimuth	 azimuth angle of sun (degrees wrt S along blon)
 *	@param[out]	*zenith		 zenith angle of sun (degrees)
 *********************************************************************/
void	sun_pos_angle

	(
	float	slat,
	float	slon,
	float	blat,
	float	blon,
	float	*azimuth,
	float	*zenith
	)

	{
	double	apptime, cosapptime;
	double	decl, sindecl, cosdecl;
	double	lambda, sinlambda, coslambda;
	double	zeni, coszeni, sinzeni;
	double	azim, cosazim;

	lambda  = RAD * blat;
	decl    = RAD * slat;
	apptime = RAD * (blon - slon);

	/* Compute convenient sines and cosines */
	sindecl    = sin(decl);
	cosdecl    = cos(decl);
	sinlambda  = sin(lambda);
	coslambda  = cos(lambda);
	cosapptime = cos(apptime);

	/* Calculate zenith angle */
	coszeni = sindecl*sinlambda + cosdecl*coslambda*cosapptime;
	zeni    = acos(coszeni);
	sinzeni = sin(zeni);

	/* Calculate azimuth angle */
	/* Case where the sun is at the zenith */
	if (sinzeni == 0)
		{
		azim = 0;
		}
	/* Case where the observation point is at the north or south pole */
	else if (coslambda == 0)
		{
		if (lambda > 0) azim = slon - blon;
		else            azim = blon - slon;
		}
	/* Normal case */
		{
		cosazim = (coszeni*sinlambda - sindecl) / (sinzeni*coslambda);
		azim    = acos(cosazim);
		}

	if (zenith)  *zenith  = (float) (zeni / RAD);
	if (azimuth) *azimuth = (float) (azim / RAD);
	}

/***********************************************************************
*                                                                      *
*     s o l a r _ f l u x                                              *
*                                                                      *
*     s u n _ p o s _ f l u x                                          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Compute the solar flux in w/m/m at the given location on the
 * earth, at the given date-time.
 *
 *	@param[in]	year		Year (full year in Gregorian calendar)
 *	@param[in]	jday		Day of year
 *	@param[in]	ztime		GMT time in seconds
 *	@param[in]	blat		latitude (real degrees, +=N)
 *	@param[in]	blon		longitude (real degrees, +=E)
 * 	@return solar flux.
 *********************************************************************/
double	solar_flux

	(
	int		year,
	int		jday,
	int		ztime,
	float	blat,
	float	blon
	)

	{
	double	sdist;
	float	slat, slon, dfact;

	/* Compute the location of the sun */
	sun_pos(year, jday, ztime, &slat, &slon);

	/* Compute the relative Earth-Sun distance */
	sdist = solar_dist(year, jday, ztime);
	dfact = 1.0 / sdist / sdist;

	/* Now compute the solar flux */
	return sun_pos_flux(slat, slon, dfact, blat, blon);
	}

/*********************************************************************/
/** Compute the solar flux at the given location on the earth,
 * for the given sun position and distance.
 *
 *	@param[in]	slat		latitude of sun (real degrees, +=N)
 *	@param[in]	slon		longitude of sun (real degrees, +=E)
 *	@param[in]	dfact		Earth-Sun distance factor (dm/d)^2
 *	@param[in]	blat		latitude (real degrees, +=N)
 *	@param[in]	blon		longitude (real degrees, +=E)
 * 	@return solar flux.
 *********************************************************************/
double	sun_pos_flux

	(
	float	slat,
	float	slon,
	float	dfact,
	float	blat,
	float	blon
	)

	{
	/* Define the solar output constant (w/m/m) */
	static	const	double	sconst = 1353.0;

	float	azimuth, zenith;

	/* Compute the zenith angle of the sun */
	sun_pos_angle(slat, slon, blat, blon, &azimuth, &zenith);

	/* Now compute the solar flux */
	return ( sconst * dfact * cos(RAD*zenith) );
	}

/***********************************************************************
*                                                                      *
*     s u n _ t i m e s
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Compute the sunrise and sunset times at a given location on the
 * earth, at the given date.
 *
 * Note: The sun times are returned in seconds relative to 0 GMT of
 *       the given day, but refer to the local mean day (within 12
 *       hours on either side of local mean noon), and thus can
 *       project into the previous or next GMT day.
 *
 * Note: At certain latitudes, there may be no sunrise or sunset on
 *       certain days.  This is denoted by setting both values to
 *       the appropriate extreme.
 *	@param[in]	year		 Year (full year in Gregorian calendar)
 *	@param[in]	jday		 Day of year
 *	@param[in]	blat		 latitude (real degrees, +=N)
 *	@param[in]	blon		 longitude (real degrees, +=E)
 *	@param[out]	*zrise		 sunrise time in seconds from 0 GMT
 *	@param[out]	*zset		 sunset time in seconds from 0 GMT
 *********************************************************************/
void	sun_times

	(
	int		year,
	int		jday,
	float	blat,
	float	blon,
	int		*zrise,
	int		*zset
	)

	{
	long	meannoon, rise, minrise, maxrise, set, minset, maxset;
	float	azimuth, zenith, midzeni, minzeni, maxzeni;

	/* Find local mean noon and the earliest sunrise and latest sunset */
	/* in seconds from 0 GMT */
	meannoon = secperday/2 - blon*secperdeg;
	minrise  = meannoon - secperday/2;
	maxrise  = meannoon;
	minset   = meannoon;
	maxset   = meannoon + secperday/2;
	if (zrise) *zrise = maxset;
	if (zset)  *zset  = minrise;

	/* Make sure the sun is up at local mean noon */
	sun_angle(year, jday, (int) meannoon, blat, blon, &azimuth, &midzeni);
	if (midzeni > 90) return;

	/* Search first for sunrise (secant search) */
	maxzeni = midzeni;
	sun_angle(year, jday, (int) minrise, blat, blon, &azimuth, &minzeni);
	while ((maxrise-minrise) > 1)
		{
		rise = (maxrise*(minzeni-90) - minrise*(maxzeni-90))
			 / (minzeni - maxzeni);
		if (rise == minrise)      rise++;
		else if (rise == maxrise) rise--;
		sun_angle(year, jday, (int) rise, blat, blon, &azimuth, &zenith);
		if (zenith > 90)      { minrise = rise;	minzeni = zenith; }
		else if (zenith < 90) { maxrise = rise;	maxzeni = zenith; }
		else                  break;
		}
	if (zrise) *zrise = rise;

	/* Now search for sunset (secant search) */
	minzeni = midzeni;
	sun_angle(year, jday, (int) maxset, blat, blon, &azimuth, &maxzeni);
	while ((maxset-minset) > 1)
		{
		set = (maxset*(minzeni-90) - minset*(maxzeni-90))
			/ (minzeni - maxzeni);
		if (set == minset)      set++;
		else if (set == maxset) set--;
		sun_angle(year, jday, (int) set, blat, blon, &azimuth, &zenith);
		if (zenith < 90)      { minset = set;	minzeni = zenith; }
		else if (zenith > 90) { maxset = set;	maxzeni = zenith; }
		else                  break;
		}
	if (zset) *zset = set;
	}


#ifdef STANDALONE

/***********************************************************************
*                                                                      *
*     Stand-alone test program.                                        *
*                                                                      *
***********************************************************************/

void	main(void)
	{
	int		ref, year, jday, ztime, zt, min, zrise, zset;
	double	jd, sdecl, pdecl;
	float	blat, blon, azimuth, zenith;

	/* See if Julian date works */
	printf("\nTest of Julian date:\n");
	year  = 1931;
	jday  = 1;
	ztime = 12*3600;
	jd    = julian(year, jday, ztime);
	printf("Date: %d:%d %dsec -> J.D. %f\n", year, jday, ztime, jd);
	year  = 1993;
	jday  = 67;
	ztime = 0;
	jd    = julian(year, jday, ztime);
	printf("Date: %d:%d %dsec -> J.D. %f\n", year, jday, ztime, jd);
	year  = 2000;
	jday  = 1;
	ztime = 12*3600;
	jd    = julian(year, jday, ztime);
	printf("Date: %d:%d %dsec -> J.D. %f\n", year, jday, ztime, jd);

	/* Compute sun positions for given date at Toronto */
	printf("\nTest of sun positions:\n");
	blat  = 43.5;
	blon  = -79.5;
	year  = 1993;
	jday  = 67;
	ztime = 11*3600;
	printf("Searching for sunrise\n");
	for (min=0; min<=60; )
		{
		zt = ztime + min*60;
		sun_angle(year, jday, zt, blat, blon, &azimuth, &zenith);

		if (zenith < 91)
			{
			printf("Time: 11:%dZ Azimuth: %g Zenith: %g\n",
				   min, azimuth, zenith);
			if (zenith < 90) break;
			min++;
			}
		else
			min+=5;
		}
	ztime = 23*3600;
	printf("Searching for sunset\n");
	for (min=0; min<=60; )
		{
		zt = ztime + min*60;
		sun_angle(year, jday, zt, blat, blon, &azimuth, &zenith);

		if (zenith > 89)
			{
			printf("Time: 23:%dZ Azimuth: %g Zenith: %g\n",
				   min, azimuth, zenith);
			if (zenith > 90) break;
			min++;
			}
		else
			min+=5;
		}

	/* Compute sunrise and sunset directly */
	printf("\nTest of sun times:\n");
		/*
		blat = 43.5;
		blon = -79.5;
		year = 1993;
		*/
	blat = 60.25;
	blon = 25.05;
	year = 1998;
	for (jday=340; jday<366; jday++)
		{
		sun_times(year, jday, blat, blon, &zrise, &zset);
		printf("day: %d Sunrise: %d:%.2dZ Sunset: %d:%.2dZ\n", jday,
			   zrise/3600, (zrise%3600 + 30)/60,
			   zset/3600, (zset%3600 + 30)/60);
		}

	/* Compute sun declination around the vernal equinox */
	printf("\nTest of sun declination:\n");
	blat = 43.5;
	blon = -79.5;
	year = 1993;
	pdecl = 0;
	for (jday=78; jday<81; jday++)
		{
		sdecl = solar_decl(year, jday, 0);
		if (pdecl<0 && sdecl>0)
			{
			ztime = 12*3600 + 12*3600*fabs(pdecl)/(sdecl-pdecl);
			printf("day: %d Equinox: %d:%.2dZ\n", jday-1,
				   ztime/3600, (ztime%3600 + 30)/60);
			}
		printf("day: %d 00Z Declination: %g\n", jday, sdecl);
		pdecl = sdecl;
		sdecl = solar_decl(year, jday, 12*3600);
		if (pdecl<0 && sdecl>0)
			{
			ztime = 12*3600*fabs(pdecl)/(sdecl-pdecl);
			printf("day: %d Equinox: %d:%.2dZ\n", jday,
				   ztime/3600, (ztime%3600 + 30)/60);
			}
		printf("day: %d 12Z Declination: %g\n", jday, sdecl);
		pdecl = sdecl;
		}
	}

#endif /* STANDALONE */
