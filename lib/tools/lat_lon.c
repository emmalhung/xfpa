/*********************************************************************/
/** @file lat_lon.c
 *
 * Functions to convert between various forms of lat-lon degrees.
 *
 *   Accepted formats are:
 *
 * @li	- [+|-]DDD[N|E|W|S]              = whole degrees
 * @li 	- [+|-]DDD.ddd[N|E|W|S]          = decimal degrees
 * @li 	- [+|-]DDD:MM[:SS][N|E|W|S]      = degrees, minutes, seconds
 * @li 	- [+|-]DDD³MM['SS["]][N|E|W|S]   = degrees, minutes, seconds
 *
 *   The default hemisphere for latitude is north.
 *   The default hemisphere for longitude is east.
 *   A hemisphere is not permitted for generic angles.
 *
 *   The extended ASCII code for the degree symbol (³) is @verbatim \263 @endverbatim octal
 *   (for ANSI fonts) or @verbatim \260 @endverbatim (for ISO fonts) - both are supported.
 *
 *   Be very careful to escape the minute and second symbols (',")
 *   in quoted expressions.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     l a t _ l o n . c                                                *
*                                                                      *
*     Functions to convert between various forms of lat-lon degrees.   *
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

#include "lat_lon.h"
#include "parse.h"

#include <fpa_math.h>
#include <fpa_types.h>
#include <string.h>

#define DegANSI '\263'
#define DegISO  '\260'

/***********************************************************************
*                                                                      *
*   d e g r e e s   - Convert DDDMM to decimal degrees.                *
*   d d d m m       - Convert decimal degrees to DDDMM.                *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Convert DDDMM to decimal degrees. Where:
 * 	- DDD = Degrees
 * 	- MM  = Minutes
 * @param[in] iang Degree Minute formatted angle
 * @return The given coordinate in decimal degrees.
 *********************************************************************/
float	degrees(int	iang)
	{
	return (float)(iang/100) + (float)(iang%100)/60.0;
	}


/*********************************************************************/
/** Convert decimal degrees to DDDMM. Where:
 * 	- DDD = Degress
 * 	- MM  = Minutes
 *
 * @param[in] angle decimal degree formatted angle
 * @return The given coordinate in DDDMM format.
 *********************************************************************/
int	dddmm(float	angle)
	{
	return 100*(int)angle + NINT(60.0*fmod(angle,1.0));
	}

/***********************************************************************
*                                                                      *
*   n o r m _ l a t _ l o n   - normalize the given lat-lon into the   *
*                               conventional range:                    *
*                                                                      *
*                               -90 <= lat <= 90                       *
*                              -180 <= lon <= 180                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Normalize the given lat-lon into the conventional range.
 *
 * @f[ -90 \le lat \le 90 @f]
 * @f[ -180 \le lon \le 180 @f]
 *********************************************************************/
void	norm_lat_lon(float	*lat, float	*lon)
	{
	float	xlat, xlon;

	if (lat)
		{
		xlat = *lat;
		xlat = fmod(xlat, 360.0);
		if (xlat >  90) xlat =  180 - xlat;
		if (xlat < -90) xlat = -180 - xlat;
		*lat = xlat;
		}

	if (lon)
		{
		xlon = *lon;
		xlon = fmod(xlon, 360.0);
		if (xlon >  180) xlon -= 360;
		if (xlon <=-180) xlon += 360;
		*lon = xlon;
		}
	}

/***********************************************************************
*                                                                      *
*   r e a d _ l a t   - Read a latitude from a string.                 *
*   r e a d _ l o n   - Read a longitude from a string.                *
*   r e a d _ a n g   - Read a generic angle from a string.            *
*                                                                      *
***********************************************************************/

typedef	enum	{Ang, Lat, Lon} LatLon;
static	float	read_lat_lon(STRING, LatLon, LOGICAL *);

/*********************************************************************/
/** Read a latitude from a string.
 *
 * @param[in] string  input string
 * @param[out] *valid Set to FALSE if input string cannot be converted.
 * @return Latitude in decimal degrees.
 *********************************************************************/
float	read_lat(STRING	string, LOGICAL	*valid)
	{
	return read_lat_lon(string, Lat, valid);
	}

/*********************************************************************/
/** Read a longitude from a string.
 *
 * @param[in] string  input string
 * @param[out] *valid Set to FALSE if input string cannot be converted.
 * @return Longitude in decimal degrees.
 *********************************************************************/
float	read_lon(STRING	string, LOGICAL	*valid)
	{
	return read_lat_lon(string, Lon, valid);
	}

/*********************************************************************/
/** Read a generic angle from a string.
 *
 * @param[in] string  input string
 * @param[out] *valid Set to FALSE if input string cannot be converted.
 * @return converted angle
 *********************************************************************/
float	read_ang(STRING	string, LOGICAL	*valid)
	{
	return read_lat_lon(string, Ang, valid);
	}

static	float	read_lat_lon(STRING	string, LatLon	which, LOGICAL	*valid)
	{
	LOGICAL	pos = TRUE;
	double	deg, fact;
	int		val;
	STRING	cp, ap;

	if (blank(string))
		{
		if (valid) *valid = FALSE;
		return 0.0;
		}

	/* Skip leading whitespace */
	cp = string + strspn(string, " \t\n\r\f");

	/* First character may be a sign */
	if (*cp == '+')
		{
		cp++;
		}
	else if (*cp == '-')
		{
		pos = !pos;
		cp++;
		}

	/* Now we must have a string of digits representing whole degrees */
	ap = cp + strspn(cp, "0123456789");
	if (ap == cp)
		{
		if (valid) *valid = FALSE;
		return 0.0;
		}
	val = 0;
	while (cp < ap)
		{
		val = val*10 + (*cp-'0');
		cp++;
		}
	deg = (double) val;

	/* Now we may have a . for decimal degrees */
	if (*cp == '.')
		{
		cp++;

		/* Now we must have a string of digits representing decimal degrees */
		ap = cp + strspn(cp, "0123456789");
		if (ap == cp)
			{
			if (valid) *valid = FALSE;
			return 0.0;
			}
		fact = 1.0;
		while (cp < ap)
			{
			fact /= 10.0;
			deg += (*cp-'0')*fact;
			cp++;
			}
		}

	/* Or we may have a : or ³ for degrees, minutes, seconds */
	else if (*cp == DegANSI || *cp == DegISO || *cp == ':')
		{
		cp++;

		/* Now we must have a string of digits representing minutes */
		ap = cp + strspn(cp, "0123456789");
		if (ap == cp)
			{
			if (valid) *valid = FALSE;
			return 0.0;
			}
		val = 0;
		while (cp < ap)
			{
			val = val*10 + (*cp-'0');
			cp++;
			}
		deg += ((double) val) / 60.0;

		/* Now we may have a . for decimal minutes */
		if (*cp == '.')
			{
			cp++;

			/* Now we must have a string of digits representing decimal minutes */
			ap = cp + strspn(cp, "0123456789");
			if (ap == cp)
				{
				if (valid) *valid = FALSE;
				return 0.0;
				}
			fact = 1.0;
			while (cp < ap)
				{
				fact /= 10.0;
				deg += ((*cp-'0')*fact / 60.0);
				cp++;
				}
			}

		/* Or we may have a : or ' for minutes, seconds */
		else if (*cp == '\'' || *cp == ':')
			{
			cp++;

			/* Now we must have a string of digits representing seconds */
			ap = cp + strspn(cp, "0123456789");
			if (ap == cp)
				{
				if (valid) *valid = FALSE;
				return 0.0;
				}
			val = 0;
			while (cp < ap)
				{
				val = val*10 + (*cp-'0');
				cp++;
				}
			deg += ((double) val) / 3600.0;
			}

		/* Now we may have a : or " for seconds */
		if (*cp == '"' || *cp == ':') cp++;
		}

	/* Now we may have a direction */
	switch (*cp)
		{
		case 'n':
		case 'N':	if (which == Lat) cp++;
					break;

		case 's':
		case 'S':	if (which == Lat) cp++;
					pos = !pos;
					break;

		case 'e':
		case 'E':	if (which == Lon) cp++;
					break;

		case 'w':
		case 'W':	if (which == Lon) cp++;
					pos = !pos;
					break;
		}

	/* Now there must be nothing left but whitespace */
	if (!blank(cp))
		{
		if (valid) *valid = FALSE;
		return 0.0;
		}

	/* What do you know? It worked. */
	if (valid) *valid = TRUE;
	return (float) (pos)? deg: -deg;
	}


#ifdef STANDALONE

/***********************************************************************
*                                                                      *
*     Stand-alone test program.                                        *
*                                                                      *
***********************************************************************/

static	STRING	lats[] =
	{
	"40", "40:15", "40:15:30", "-40:15", "garbage",
	"40.25", "40.25:30", "40'30", "40³15'00\"", "40³15'00",
	"40S", "40:15S", "40:15:30S", "-40:15S", "garbageS",
	"40.25S", "40.25:30S", "40'30S", "40³15'00\"S", "40³15'00S",
	"40N", "40:15N", "40:15:30N", "-40:15N", "garbageN",
	"40.25N", "40.25:30N", "40'30N", "40³15'00\"N", "40³15'00N",
	"60W", "60:15W", "60:15:30W", "-60:15W", "garbageW",
	"60E", "60:15E", "60:15:30E", "-60:15E", "garbageE",
	"12.19"
	};

static	int		nlat = (int) (sizeof(lats) / sizeof(STRING));

static	STRING	lons[] =
	{
	"60", "60:15", "60:15:30", "-60:15", "garbage",
	"60.25", "60.25:30", "60'30", "60³15'00\"", "60³15'00",
	"60W", "60:15W", "60:15:30W", "-60:15W", "garbageW",
	"60.25W", "60.25:30W", "60'30W", "60³15'00\"W", "60³15'00W",
	"60E", "60:15E", "60:15:30E", "-60:15E", "garbageE",
	"60.25E", "60.25:30E", "60'30E", "60³15'00\"E", "60³15'00E",
	"40S", "40:15S", "40:15:30S", "-40:15S", "garbageS",
	"40N", "40:15N", "40:15:30N", "-40:15N", "garbageN",
	"12.19"
	};

static	int		nlon = (int) (sizeof(lons) / sizeof(STRING));

void	main(void)
	{
	int		i;
	float	d;
	LOGICAL	valid;

	for (i=0; i<nlat; i++)
		{
		d = read_lat(lats[i], &valid);
		printf("\tLatitude:\t%s\t", lats[i]);
		if (valid) printf("%15.12f\n", d);
		else       printf("INVALID!\n");
		}

	for (i=0; i<nlon; i++)
		{
		d = read_lon(lons[i], &valid);
		printf("\tLongitude:\t%s\t", lons[i]);
		if (valid) printf("%15.12f\n", d);
		else       printf("INVALID!\n");
		}
	}
#endif /* STANDALONE */
