/***********************************************************************
*                                                                      *
*     g e t m a p . c                                                  *
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

#include <fpa.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


/* Radius of earth in km */
static	const	double	Re = 6367.6500;

/* Common input buffer */
typedef	char	SBUF[64];
static	SBUF	answer;
static	int		pchoice=0, mchoice=0;
static	LOGICAL	ok=FALSE;

/* Projection parameters to be input */
typedef	enum	{
				Squit,
				Pname, Pargs,
				Mspec, Mopts, Msize, Mgfit, Mgout, Mdone
				} MODE;
static	SBUF	pname="", p1="", p2="", p3="", p4="", p5="";
static	SBUF	olat="", olon="", rlon="";
static	float	xmin, ymin, xmax, ymax, glen, xomax, yomax;
static	LOGICAL	km=FALSE, rel=FALSE, unfit=FALSE, fit=FALSE;
static	SBUF	pinfo="", minfo="", ginfo="";
static	char	fname[256];

static	MODE	get_squit(void);
static	MODE	get_pname(void);
static	MODE	get_pargs(void);
static	MODE	get_mspec(void);
static	MODE	get_mopts(void);
static	MODE	get_msize(void);
static	MODE	get_mgfit(void);
static	MODE	do_output(void);
static	void	tidy_lat(STRING);
static	void	tidy_lon(STRING);

#ifdef TRANS
static	void	do_trans(void);
#endif

/******************************************************************************/

int		main
	(
	int		argc,
	STRING	argv[]
	)

	{
	MODE	mode = Pname;

	/* Proprietary license call! */ app_license("generic");
	set_term_trap(SIG_DFL);

	if (argc >= 2 && !blank(argv[1])) strcpy(fname, argv[1]);

	/* Keep trying until we get all the way through */
	while (TRUE)
		{
		switch (mode)
			{
			case Squit:	mode = get_squit();	continue;
			case Pname:	mode = get_pname();	continue;
			case Pargs:	mode = get_pargs();	continue;
			case Mspec:	mode = get_mspec();	continue;
			case Mopts:	mode = get_mopts();	continue;
			case Msize:	mode = get_msize();	continue;
			case Mgfit:	mode = get_mgfit();	continue;
			case Mgout:	mode = do_output();	continue;
			case Mdone:	break;
			}
		break;
		}

#ifdef TRANS
	do_trans();
#endif
	return 0;
	}

/******************************************************************************/

static	MODE	get_squit(void)
	{
	printf("\nReally quit? (y/n) ");
	getword(stdin, answer, sizeof(answer));
	if (blank(answer)) return Squit;

	if (same_start_ic(answer, "y")) exit(1);
	return Pname;
	}

/******************************************************************************/

static	MODE	get_pname(void)
	{
	/* List the selections */
	printf("\n");
	printf("Available projections:\n");
	printf("  1 - polar stereographic.\n");
	printf("  2 - oblique stereographic.\n");
	printf("  3 - lambert conformal.\n");
	printf("  4 - mercator equatorial.\n");
	printf("  5 - plate caree.\n");
	printf("  6 - latitude-longitude.\n");

	/* Keep trying until we get a valid answer */
	do	{
		printf("Enter choice: ");
		getword(stdin, answer, sizeof(answer));
		if (blank(answer)) return Squit;
		pchoice = int_arg(answer, &ok);
		if (!ok) continue;

		switch (pchoice)
			{
			case 1:		strcpy(pname, "polar_stereographic");	break;
			case 2:		strcpy(pname, "oblique_stereographic");	break;
			case 3:		strcpy(pname, "lambert_conformal");		break;
			case 4:		strcpy(pname, "mercator_equatorial");	break;
			case 5:		strcpy(pname, "plate_caree");			break;
			case 6:		strcpy(pname, "latitude_longitude");	break;
			default:	ok = FALSE;
			}
		} while (!ok);

	return Pargs;
	}

/******************************************************************************/

static	MODE	get_pargs(void)
	{
	sprintf(pinfo, "projection %s", pname);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("\n");

	strcpy(p1, "");
	strcpy(p2, "");
	strcpy(p3, "");
	strcpy(p4, "");
	strcpy(p5, "");

	/* Input parameters specific to each projection */
	switch (pchoice)
		{
		case 1:
			ptype1:
			do	{
				printf("Enter N for north or S for south: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) return Pname;
				if (same_ic(answer, "n")) strcpy(p1, "north");
				if (same_ic(answer, "s")) strcpy(p1, "south");
				ok = NotNull(p1);
				} while (!ok);

			do	{
				printf("Enter latitude at which projection is true: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) goto ptype1;
				(void) read_lat(answer, &ok);
				if (ok) strcpy_arg(p2, answer, &ok);
				if (ok) tidy_lat(p2);
				} while (!ok);

			break;

		case 2:
			ptype2:
			do	{
				printf("Enter latitude of tangent point: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) return Pname;
				(void) read_lat(answer, &ok);
				if (ok) strcpy_arg(p1, answer, &ok);
				if (ok) tidy_lat(p1);
				} while (!ok);

			do	{
				printf("Enter longitude of tangent point: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) goto ptype2;
				(void) read_lon(answer, &ok);
				if (ok) strcpy_arg(p2, answer, &ok);
				if (ok) tidy_lon(p2);
				} while (!ok);

			strcpy(p3, "?");

			break;

		case 3:
			ptype3:
			do	{
				printf("Enter upper latitude: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) return Pname;
				(void) read_lat(answer, &ok);
				if (ok) strcpy_arg(p1, answer, &ok);
				if (ok) tidy_lat(p1);
				} while (!ok);

			do	{
				printf("Enter lower latitude: ");
				getword(stdin, answer, sizeof(answer));
				if (blank(answer)) goto ptype3;
				(void) read_lat(answer, &ok);
				if (ok) strcpy_arg(p2, answer, &ok);
				if (ok) tidy_lat(p2);
				} while (!ok);

			break;

		case 4:
		case 5:
		case 6:		break;
		
		default:	return Pname;
		}

	return Mspec;
	}

/******************************************************************************/

static	MODE	get_mspec(void)
	{
	sprintf(pinfo, "projection %s %s %s %s %s %s", pname, p1, p2, p3, p4, p5);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("\n");

	/* Input origin latitude */
	get_olat:
	strcpy(olat, "");
	do	{
		printf("Enter origin latitude: ");
		getword(stdin, answer, sizeof(answer));
		if (blank(answer)) return (pchoice<=3)? Pargs: Pname;
		(void) read_lat(answer, &ok);
		if (ok) strcpy_arg(olat, answer, &ok);
		if (ok) tidy_lat(olat);
		} while (!ok);

	/* Input origin longitude */
	get_olon:
	strcpy(olon, "");
	do	{
		printf("Enter origin longitude: ");
		getword(stdin, answer, sizeof(answer));
		if (blank(answer)) goto get_olat;
		(void) read_lon(answer, &ok);
		if (ok) strcpy_arg(olon, answer, &ok);
		if (ok) tidy_lon(olon);
		} while (!ok);

	/* Input reference longitude */
	strcpy(rlon, "");
	do	{
		printf("Enter reference (vertical) longitude: ");
		getword(stdin, answer, sizeof(answer));
		if (blank(answer)) goto get_olon;
		(void) read_lon(answer, &ok);
		if (ok) strcpy_arg(rlon, answer, &ok);
		if (ok) tidy_lon(rlon);
		} while (!ok);

	return Mopts;
	}

/******************************************************************************/

static	MODE	get_mopts(void)
	{
	sprintf(minfo, "mapdef %s %s %s ? ? ? ? ?", olat, olon, rlon);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("> %s\n", minfo);
	printf("\n");

	printf("Map size options:\n");
	printf("  1 - Specify size in km from bottom left.\n");
	printf("  2 - Specify size in km from origin.\n");
	printf("  3 - Specify size in grids from bottom left.\n");
	printf("  4 - Specify size in grids from origin.\n");
	do	{
		printf("Enter choice: ");
		getword(stdin, answer, sizeof(answer));
		if (blank(answer)) return Mspec;
		mchoice = int_arg(answer, &ok);
		if (!ok) continue;

		switch (mchoice)
			{
			case 1:		km = TRUE;	rel = FALSE;	break;
			case 2:		km = TRUE;	rel = TRUE;		break;
			case 3:		km = FALSE;	rel = FALSE;	break;
			case 4:		km = FALSE;	rel = TRUE;		break;
			default:	ok = FALSE;
			}
		} while (!ok);

	return Msize;
	}

/******************************************************************************/

static	MODE	get_msize(void)
	{
	STRING	dunits, punits;
	float	xgrid, ygrid, xgmax, ygmax, secang;
	int		nx, ny;

	/* Re-calculate secant angle for normalized oblique stereographic */
	if (pchoice == 2)
		{
		sprintf(p3, "?");
		}
	sprintf(pinfo, "projection %s %s %s %s %s %s", pname, p1, p2, p3, p4, p5);
	sprintf(minfo, "mapdef %s %s %s ? ? ? ? ?", olat, olon, rlon);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("> %s\n", minfo);
	printf("\n");

	/* Get grid length if necessary */
	get_msize:
	glen = 1.0;
	if (!km)
		{
		do	{
			printf("Enter spline resolution (grid length) (km): ");
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) return Mopts;
			glen = float_arg(answer, &ok);
			} while (!ok);
		}

	/* Specify map relative to reference point */
	dunits = (km)? "km": "grid lengths";
	punits = (km)? "km": "grid pos, start=0";
	if (rel)
		{
		get_left:
		do	{
			printf("Enter distance left of origin (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer))
				{
				if (km) return Mopts;
				else    goto get_msize;
				}
			xmin = -glen*float_arg(answer, &ok);
			} while (!ok);
		get_right:
		do	{
			printf("Enter distance right of origin (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_left;
			xmax = glen*float_arg(answer, &ok);
			} while (!ok);
		get_bottom:
		do	{
			printf("Enter distance below origin (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_right;
			ymin = -glen*float_arg(answer, &ok);
			} while (!ok);
		get_top:
		do	{
			printf("Enter distance above origin (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_bottom;
			ymax = glen*float_arg(answer, &ok);
			} while (!ok);
		}

	/* Specify map relative to bottom left */
	else
		{
		get_xorg:
		do	{
			printf("Enter horiz location of origin (%s): ", punits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer))
				{
				if (km) return Mopts;
				else    goto get_msize;
				}
			xmin = -glen*float_arg(answer, &ok);
			} while (!ok);
		get_yorg:
		do	{
			printf("Enter vert location of origin (%s): ", punits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_xorg;
			ymin = -glen*float_arg(answer, &ok);
			} while (!ok);
		get_width:
		do	{
			printf("Enter map width (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_yorg;
			xmax = xmin + glen*float_arg(answer, &ok);
			} while (!ok);
		get_height:
		do	{
			printf("Enter map height (%s): ", dunits);
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_width;
			ymax = ymin + glen*float_arg(answer, &ok);
			} while (!ok);
		}

	/* Get grid length if not yet given */
	if (km)
		{
		do	{
			printf("Enter spline resolution (grid length) (km): ");
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) goto get_msize;
			glen = float_arg(answer, &ok);
			} while (!ok);
		}

	/* Determine whether the map dimensions match the grid spacing */
	xgrid = (xmax-xmin) / glen;
	ygrid = (ymax-ymin) / glen;
	nx    = NINT(xgrid);
	ny    = NINT(ygrid);
	xgmax = xmin + nx*glen;
	ygmax = ymin + ny*glen;
	unfit = (LOGICAL) (xgmax != xmax || ygmax != ymax);
	xomax = xmax;
	yomax = ymax;

	/* Calculate angle of secant for normalized oblique stereographic */
	if (pchoice == 2)
		{
		secang = xmax / Re / RAD;
		sprintf(p3, "%g", secang);
		}

	return (unfit)? Mgfit: Mgout;
	}

/******************************************************************************/

static	MODE	get_mgfit(void)
	{
	float	xgrid, ygrid, xgmax, ygmax, secang;
	int		nx, ny;

	if (fit)
		{
		xmax = xomax;
		ymax = yomax;

		/* Re-calculate secant angle for normalized oblique stereographic */
		if (pchoice == 2)
			{
			secang = xmax / Re / RAD;
			sprintf(p3, "%g", secang);
			}
		}
	sprintf(pinfo, "projection %s %s %s %s %s %s", pname, p1, p2, p3, p4, p5);
	sprintf(minfo, "mapdef %s %s %s %g %g %g %g 1000", olat, olon, rlon,
			xmin, ymin, xmax, ymax);
	sprintf(ginfo, "resolution %g 1000", glen);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("> %s\n", minfo);
	printf("> %s\n", ginfo);
	printf("\n");

	/* Compute size to nearest integral grid lengths */
	xgrid = (xmax-xmin) / glen;
	ygrid = (ymax-ymin) / glen;
	nx    = NINT(xgrid);
	ny    = NINT(ygrid);
	xgmax = xmin + nx*glen;
	ygmax = ymin + ny*glen;

	if (xgmax != xmax || ygmax != ymax)
		{
		do	{
			printf("Force map size to fit grid length (y/n)? ");
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) return Msize;
			switch (answer[0])
				{
				case 'y':
				case 'Y':	fit = TRUE;	ok = TRUE;	break;

				case 'n':
				case 'N':	fit = FALSE;	ok = TRUE;	break;

				default:	ok = FALSE;
				}
			} while (!ok);

		/* Force map dimensions to align with grid spacing */
		if (fit)
			{
			xmax = xgmax;
			ymax = ygmax;

			/* Re-calculate secant angle for normalized oblique stereographic */
			if (pchoice == 2)
				{
				secang = xmax / Re / RAD;
				sprintf(p3, "%g", secang);
				}
			}
		}

	return Mgout;
	}

/******************************************************************************/

static	MODE	do_output(void)
	{
	FILE	*file;

	sprintf(pinfo, "projection %s %s %s %s %s %s", pname, p1, p2, p3, p4, p5);
	sprintf(minfo, "mapdef %s %s %s %g %g %g %g 1000", olat, olon, rlon,
			xmin, ymin, xmax, ymax);
	sprintf(ginfo, "resolution %g 1000", glen);

	printf("\n");
	printf("> %s\n", pinfo);
	printf("> %s\n", minfo);
	printf("> %s\n", ginfo);
	printf("\n");

	/* Output the target map projection information */
	if (blank(fname))
		{
		printf("Map Definition File: ");
		getword(stdin, fname, sizeof(fname));
		if (blank(fname)) return (unfit)? Mgfit: Msize;
		}
	else
		{
		do	{
			printf("OK to proceed (y/n)? ");
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) return (unfit)? Mgfit: Msize;
			switch (answer[0])
				{
				case 'y':
				case 'Y':	ok = TRUE;	break;

				case 'n':
				case 'N':	return (unfit)? Mgfit: Msize;

				default:	ok = FALSE;
				}
			} while (!ok);
		}

	if (same(fname, "-"))
		{
		printf("Output to standard out\n");
		printf("\n");
		file = stdout;
		}
	else
		{
		file = fopen(fname, "w");
		if (!file)
			{
			printf("Cannot open file: %s\n", fname);
			printf("Output to standard out\n");
			printf("\n");
			file = stdout;
			}
		}

	fprintf(file, "%s\n", pinfo);
	fprintf(file, "%s\n", minfo);
	fprintf(file, "%s\n", ginfo);

	if (file != stdout) fclose(file);

	return Mdone;
	}

/******************************************************************************/

static	void	tidy_lat(lat)
STRING	lat;
	{
	int		n;
	char	c;
	LOGICAL	plus;

	if (blank(lat)) return;

	c = lat[0];
	switch (c)
		{
		case '-':	plus = FALSE;	strcpy(lat, lat+1);	break;

		case '+':	plus = TRUE;	strcpy(lat, lat+1);	break;

		default:	plus = TRUE;
		}

	n = strlen(lat) - 1;
	c = lat[n];
	switch (c)
		{
		case 'n':
		case 'N':	lat[n] = (plus)? 'N': 'S';	break;

		case 's':
		case 'S':	lat[n] = (plus)? 'S': 'N';	break;

		default:	strcat(lat, (plus)? "N": "S");
		}
	}

/******************************************************************************/

static	void	tidy_lon(lon)
STRING	lon;
	{
	int		n;
	char	c;
	LOGICAL	plus;

	if (blank(lon)) return;

	c = lon[0];
	switch (c)
		{
		case '-':	plus = FALSE;	strcpy(lon, lon+1);	break;

		case '+':	plus = TRUE;	strcpy(lon, lon+1);	break;

		default:	plus = TRUE;
		}

	n = strlen(lon) - 1;
	c = lon[n];
	switch (c)
		{
		case 'e':
		case 'E':	lon[n] = (plus)? 'E': 'W';	break;

		case 'w':
		case 'W':	lon[n] = (plus)? 'W': 'E';	break;

		default:	strcat(lon, (plus)? "E": "W");
		}
	}

/******************************************************************************/

#ifdef TRANS
static	void	do_trans(void)
	{
	float	lat, lon;
	POINT	p;
	PROJ_DEF	proj;
	MAP_DEF		map;
	MAP_PROJ	mp;

	ok = define_projection_by_name(&proj, pname, p1, p2, p3, p4, p5);
	if (!ok) return;
	strcpy(answer, olat);
	map.olat = read_lat(answer, &ok);
	strcpy(answer, olon);
	map.olon = read_lon(answer, &ok);
	strcpy(answer, rlon);
	map.lref = read_lon(answer, &ok);
	map.xorg = - xmin;
	map.yorg = - ymin;
	map.xlen = xmax - xmin;
	map.ylen = ymax - ymin;
	map.units = 1000;
	define_map_projection(&mp, &proj, &map, NullGridDef);

	while (TRUE)
		{
		printf("Translate:\n");
		printf(" L - lat-lon to x-y\n");
		printf(" X - x-y to lat-lon\n");
		printf(" [RETURN] to exit\n");
		do	{
			printf("Enter choice: ");
			getword(stdin, answer, sizeof(answer));
			if (blank(answer)) return;
			switch (answer[0])
				{
				case 'l':
				case 'L':
					printf(" Enter Lat: ");
					getword(stdin, answer, sizeof(answer));
					if (blank(answer)) break;
					lat = float_arg(answer, &ok);
					if (!ok) break;

					printf(" Enter Lon: ");
					getword(stdin, answer, sizeof(answer));
					if (blank(answer)) break;
					lon = float_arg(answer, &ok);
					if (!ok) break;

					ll_to_pos(&mp, lat, lon, p);
					printf("  %g %g -> (%g, %g)\n", lat, lon, p[X], p[Y]);
					printf("    GRID: (%g, %g)\n", p[X]/glen, p[Y]/glen);

					break;


				case 'x':
				case 'X':
					printf(" Enter X: ");
					getword(stdin, answer, sizeof(answer));
					if (blank(answer)) break;
					p[X] = float_arg(answer, &ok);
					if (!ok) break;

					printf(" Enter Y: ");
					getword(stdin, answer, sizeof(answer));
					if (blank(answer)) break;
					p[Y] = float_arg(answer, &ok);
					if (!ok) break;

					pos_to_ll(&mp, p, &lat, &lon);
					printf("  (%g, %g) -> %g %g\n", p[X], p[Y], lat, lon);

					break;

				default:	ok = FALSE;
				}

			} while (!ok);
		}
	}
#endif
