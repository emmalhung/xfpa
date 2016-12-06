/*========================================================================*/
/*
*	File:		pointFcstTableFcns.c
*
*	Purpose:	Common functions used by the point forecast routines.
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*/
/*========================================================================*/
#include "global.h"
#include "pointFcst.h"

extern String pf_info_file;
extern int pf_nclass;
extern PFCLASS *pf_class;

/*========================================================================*/
/*
*	pf_FreeData() - Free data in the data structure.  If it is changed later
*   this ensures a minimum of coding changes.
*/
/*========================================================================*/
void pf_FreeData(PFDATA *data )

{
	FreeItem(data->id);
	FreeItem(data->label);
	FreeItem(data);
}

/*========================================================================*/
/*
*	pf_FmtData() - Formats some of the data into strings.
*/
/*========================================================================*/
String pf_FmtData(PFDATA *data , String key )
{
	int i, deg, min, sec;
	static char mbuf[50];

	strcpy(mbuf, "");
	if(same(key,keylat))
	{
		if( data->latitude != (float)MISSING )
		{
			pf_DegreeComponents(data->latitude, &deg, &min, &sec);
			(void) snprintf(mbuf, sizeof(mbuf), "%d%c %d%c %d%c",
				deg, degree_symbols[0],
				min, degree_symbols[1],
				sec, degree_symbols[2]);
		}
	}
	else if(same(key,keyinfolat))
	{
		if(data->latitude != (float)MISSING)
		{
			pf_DegreeComponents(data->latitude, &deg, &min, &sec);
			(void) snprintf(mbuf, sizeof(mbuf), "%d%c%d%c%d%c%c",
				deg, degree_symbols[0],
				min, degree_symbols[1],
				sec, degree_symbols[2],
				(data->latitude < 0)? 'S':'N');
		}
	}
	else if(same(key,keylong))
	{
		if( data->longitude != (float)MISSING )
		{
			pf_DegreeComponents(data->longitude, &deg, &min, &sec);
			(void) snprintf(mbuf, sizeof(mbuf), "%d%c %d%c %d%c",
				deg, degree_symbols[0],
				min, degree_symbols[1],
				sec, degree_symbols[2]);
		}
	}
	else if(same(key,keyinfolong))
	{
		if(data->longitude != (float)MISSING)
		{
			pf_DegreeComponents(data->longitude, &deg, &min, &sec);
			(void) snprintf(mbuf, sizeof(mbuf), "%d%c%d%c%d%c%c",
				deg, degree_symbols[0],
				min, degree_symbols[1],
				sec, degree_symbols[2],
				(data->longitude < 0)? 'W':'E');
		}
	}
	else if(same(key,keyissue))
	{
		for( i = 0; i < data->nissuetimes; i++ )
		{
			if(i>0) strcat(mbuf,",");
			(void) snprintf(&mbuf[safe_strlen(mbuf)], (sizeof(mbuf) - safe_strlen(mbuf)), "%d", data->issuetimes[i]);
		}
	}
	return mbuf;
}

/*========================================================================*/
/*
*	pf_GetFmtData() - Reads formatted ascii strings and puts the data into
*	the data structure.
*/
/*========================================================================*/
void pf_GetFmtData(PFDATA *data ,
                    String sdata ,
                    String key )

{
	int i, n;
	float val, read_lat(), read_lon();
	char mbuf[128];
	String sd, ptr;
	Boolean ok;

	sd = XtNewString(sdata);

	if(same(key,keylat))
	{
		pf_ParseLatLongString(sd, mbuf);
		val = read_lat(mbuf, &ok);
		if(ok) data->latitude = val;
		else   data->latitude = (float)MISSING;
	}
	else if(same(key,keyinfolat))
	{
		pf_ParseLatLongString(sd, mbuf);
		val = read_lat(mbuf, &ok);
		if(ok) data->latitude = val;
		else   data->latitude = (float)MISSING;
	}
	/* In the following longitude conversions we look for one of "+-WE" in
	*  the string.
	*/
	else if(same(key,keylong))
	{
		pf_ParseLatLongString(sd, mbuf);
		val = read_lon(mbuf, &ok);
		if(ok) data->longitude = val;
		else   data->longitude = (float)MISSING;
	}
	else if(same(key,keyinfolong))
	{
		pf_ParseLatLongString(sd, mbuf);
		val = read_lon(mbuf, &ok);
		if(ok) data->longitude = val;
		else   data->longitude = (float)MISSING;
	}
	else if(same(key,keyissue))
	{
		data->nissuetimes = 0;
		while((ptr = strchr(sd,','))) *ptr = ' ';
		for( i = 0; i < 24; i++ )
		{
			n = int_arg(sd, &ok);
			if(!ok) break;
			data->issuetimes[data->nissuetimes] = n;
			data->nissuetimes++;
		}
	}
	else if(same(key,keylanguage))
	{
		data->language = GV_language[0].key;
		for( i = 0; i < GV_nlanguages; i++ )
		{
			if(!same(sd, GV_language[i].key)) continue;
			data->language = GV_language[i].key;
			break;
		}
	}
	else if(same(key,keytimezone))
	{
		data->timezone = GV_timezone[0].key;
		for( i = 0; i < GV_ntimezones; i++ )
		{
			if(!same(sd, GV_timezone[i].key)) continue;
			data->timezone = GV_timezone[i].key;
			break;
		}
	}
	FreeItem(sd);
}

/*========================================================================*/
/*
*    pf_WriteInfoData() - Write out the info file format data. Note that the
*                      internal forecast identifier used by fog is simply
*    the forecast label with all spaces replaced by an underscore. Because
*    of this the id is not carried along as a piece of data.
*/
/*========================================================================*/
void pf_WriteInfoData(void)

{
	int i, j;
	char keybuf[128], *ptr;
	INFOFILE fh;

	fh = info_file_create(pf_info_file);
	for( i = 0; i < pf_nclass; i++ )
	{
		for( j = 0; j < pf_class[i].ndata; j++ )
		{
			strcpy(keybuf, pf_class[i].data[j]->label);
			while((ptr = strchr(keybuf, ' '))) *ptr = '_';

			info_file_write_block_header(fh, pf_class[i].data[j]->id);
			info_file_write_line(fh, keylabel, pf_class[i].data[j]->label);
			info_file_write_line(fh, keyid, keybuf);
			info_file_write_line(fh, keyclass, pf_class[i].data[j]->class);
			info_file_write_line(fh, keylanguage, pf_class[i].data[j]->language);
			info_file_write_line(fh, keylat, pf_FmtData(pf_class[i].data[j], keyinfolat));
			info_file_write_line(fh, keylong, pf_FmtData(pf_class[i].data[j], keyinfolong));
			info_file_write_line(fh, keytimezone, pf_class[i].data[j]->timezone);
			info_file_write_line(fh, keyissue, pf_FmtData(pf_class[i].data[j], keyissue));
		}
	}
	info_file_close(fh);
}


void pf_CreateFileName(PFDATA *data ,
                        String fname )

{
	String ptr;
	(void) snprintf(fname, sizeof(fname), "%s.%c", data->label, data->language[0]);
	while((ptr = strchr(fname, ' '))) *ptr = '_';
}


void pf_DegreeComponents(float degrees , int *deg , int *min , int *sec )
{
	double d;

	d = fabs((double)degrees);
	*deg = (int)d;
	d = (d - (double)*deg) * 60.0;
	*min = (int)d;
	d = (d - (double)*min) * 60.0;
	*sec = NINT(d);
	if(*sec >= 60)
	{
		*min += 1;
		*sec -= 60;
	}
	if(*min >= 60)
	{
		*deg += 1;
		*min -= 60;
	}
}


/* Note: Latududes and longitudes can come in many forms. Especially if
*  people input them in a file. This code attempts to put the entry into
*  a single format ddd:mm:ss[NSEW] for processing.
*/
void pf_ParseLatLongString(String inbuf ,
                            String outbuf )

{
	String ptr, eptr, optr, nptr;

	strcpy(outbuf, inbuf);
	upper_case(outbuf);
	while((ptr = strpbrk(outbuf, degree_symbols))) *ptr = ' ';
	optr = outbuf;
	eptr = outbuf+safe_strlen(outbuf);
	for(ptr = outbuf; ptr < eptr; ptr++)
	{
		if(*ptr == ' ')
		{
			nptr = ptr+1;
			if(*nptr == ' ' || *nptr == '\0') continue;
			if(NotNull(strchr("NSEW", *nptr)))
			{
				*optr = *nptr;
				ptr++;
			}
			else
			{
				*optr = ':';
			}
		}
		else
		{
			*optr = *ptr;
		}
		optr++;
	}
	*optr = '\0';
}

void pf_MakeDataId(String mbuf ,
                    PFCLASS *class )

{
	int i;
	(void) snprintf(mbuf, sizeof(mbuf), "%s_%d", class->label, class->ndata);
	lower_case(mbuf);
	for(i = 0; i < safe_strlen(mbuf); i++) if(mbuf[i] == ' ') mbuf[i] = '_';
}
