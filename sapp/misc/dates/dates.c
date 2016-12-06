/***********************************************************************
*                                                                      *
*  Routine to convert dates for handling Julian Days                   *
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

void	jdate_test(void);
void	mdate_test(void);
void	cal_test(void);

int		main(void)
	{
	char    buf[25];

	while (1)
		{
		(void) printf("\n");
		(void) printf("Test modes:\n");
		(void) printf("   1 - Month/day -> Julian day\n");
		(void) printf("   2 - Julian day -> month/day\n");
		(void) printf("   3 - Calendar\n");
		(void) printf("Enter test mode: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf)) break;
		else if (same(buf, "1")) jdate_test();
		else if (same(buf, "2")) mdate_test();
		else if (same(buf, "3")) cal_test();
		}
	return 0;
	}

void	jdate_test(void)
	{
	char	buf[25];
	int		year, month, ndm, mday, jday;

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter year: ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;
		if (sscanf(buf, "%d", &year) < 1) continue;

		(void) printf("Enter month (1-12): ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &month) < 1) continue;
		if (month < 1)  continue;
		if (month > 12) continue;

		ndm = ndmonth(year, month);
		(void) printf("Enter day of month (1-%d): ", ndm);
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &mday) < 1) continue;
		if (mday < 1)   continue;
		if (mday > ndm) continue;

		jdate(&year, &month, &mday, &jday);
		(void) printf("Day of year: %d\n", jday);
		}
	}

void	mdate_test(void)
	{
	char	buf[25];
	int		year, month, ndy, mday, jday;

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter year: ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;
		if (sscanf(buf, "%d", &year) < 1) continue;

		ndy = ndyear(year);
		(void) printf("Enter day of year (1-%d): ", ndy);
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &jday) < 1) continue;
		if (jday < 1)   continue;
		if (jday > ndy) continue;

		mdate(&year, &jday, &month, &mday);
		(void) printf("Month: %d\n", month);
		(void) printf("Day of month: %d\n", mday);
		}
	}

void	cal_test(void)
	{
	char	buf[25];
	int		year, month, day, jday, wkd, ndm, jmax;

	static	STRING	mname[]
					= { "January",   "February", "March",    "April",
						"May",       "June",     "July",     "August",
						"September", "October",  "November", "December" };

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter year: ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;
		if (sscanf(buf, "%d", &year) < 1) continue;

		(void) printf("Enter month (1-12): ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &month) < 1) continue;
		if (month < 1)  continue;
		if (month > 12) continue;

		(void) printf("\n Calendar for %s %d\n", mname[month-1], year);
		(void) printf(" |-----------------------------------------------------------------------------------|\n");
		(void) printf(" |   Sunday  |   Monday  |  Tuesday  | Wednesday |  Thursday |   Friday  |  Saturday |\n");
		(void) printf(" |-----------------------------------------------------------------------------------|\n");
		day = 1;
		jdate(&year, &month, &day, &jday);
		wkd = wkday(year, jday);
		switch (wkd)
			{
			case 1:
				(void) printf(" | %3d (%.3d)", day, jday);
				break;
			case 2:
				(void) printf(" |           | %3d (%.3d)", day, jday);
				break;
			case 3:
				(void) printf(" |           |           | %3d (%.3d)", day, jday);
				break;
			case 4:
				(void) printf(" |           |           |           | %3d (%.3d)", day, jday);
				break;
			case 5:
				(void) printf(" |           |           |           |           | %3d (%.3d)", day, jday);
				break;
			case 6:
				(void) printf(" |           |           |           |           |           | %3d (%.3d)", day, jday);
				break;
			case 7:
				(void) printf(" |           |           |           |           |           |           | %3d (%.3d) |\n", day, jday);
				break;
			}
		ndm  = ndmonth(year, month);
		jmax = jday + ndm - 1;
		for (++jday; jday<=jmax; jday++)
			{
			mdate(&year, &jday, &month, &day);
			wkd = wkday(year, jday);
			switch (wkd)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
					(void) printf(" | %3d (%.3d)", day, jday);
					break;
				case 7:
					(void) printf(" | %3d (%.3d) |\n", day, jday);
					break;
				}
			}
		switch (wkd)
			{
			case 1:
				(void) printf(" |           |           |           |           |           |           |\n");
				break;
			case 2:
				(void) printf(" |           |           |           |           |           |\n");
				break;
			case 3:
				(void) printf(" |           |           |           |           |\n");
				break;
			case 4:
				(void) printf(" |           |           |           |\n");
				break;
			case 5:
				(void) printf(" |           |           |\n");
				break;
			case 6:
				(void) printf(" |           |\n");
				break;
			case 7:
				break;
			}
		(void) printf(" |-----------------------------------------------------------------------------------|\n");
		}
	}
