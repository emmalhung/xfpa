/*******************************************************************************/
/*
 * File:	glib_urp.c
 *
 * Purpose:	Contains code to parse the URP data files. All urp file format
 *          specific code is here for easier maintenance.
 *
 * Note:  1.The older format URP files preceeded the keywords with a '#'
 *          character. The code in this file is written to work with both the
 *          old and new format keywords.
 *        2.If the file does not contain a data to value cross reference
 *          table then a one-to-one table is used and named "EngineeringUnits".
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
/*******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fpa_types.h>
#include <fpa_math.h>
#include <tools/tools.h>
#include "glib_private.h"

#define ENGINEERING_UNITS "EngineeringUnits"


/* Radar data table storage. 
 */
static int       ndata_tables  = 0;
static IMRADDATA **data_tables = (IMRADDATA**)0;


/* Messages */
static STRING nomem  = "Memory allocation failure.";
static STRING nodata = "Did not find \"Data\" key.";
static STRING noread = "Read failure: binary data size mismatch.";
static STRING noend  = "TableEnd key not found.";


/*-------- Local to this file functions ------------*/

static STRING mygetline( FILE *fp, STRING line, int ncl, LOGICAL *pound_start)
{
	if (pound_start) *pound_start = FALSE;
	if(!getfileline(fp, line, (size_t)ncl)) return NULL;
	no_white(line);
	if (*line == '#')
	{
		*line = ' ';
		no_white(line);
		if (pound_start) *pound_start = TRUE;
	}
	return line;
}


static enum IMAGE_ENCODING urp_file_type(STRING fname)
{
	char   buf[51];
	STRING key, subkey;
	enum   IMAGE_ENCODING encode = ImageEncodingNone;

	FILE *fp = fopen(fname, BINARY_READ);
	if (!fp) return encode;

	buf[50] = '\0';
	while(mygetline(fp, buf, 50, NULL) != NULL)
	{
		key = string_arg(buf);

		if(same_ic(key, "Numeric"))
		{
			subkey = string_arg(buf);
			if(same_ic(subkey, "Definition"))
			{
				encode = ImageEncodingGriddedURP;
				break;
			}
		}
		else if(same_ic(key, "FieldType"))
		{
			subkey = string_arg(buf);
			if(same_ic(subkey, "Range,Theta"))
			{
				encode = ImageEncodingPolarURP;
				break;
			}
		}
		else if(same_ic(key, "Data"))
		{
			break;
		}
	}
	(void) fclose(fp);
	return encode;
}


static void read_data_table_labels(STRING ptr, IMRADDATA *data)
{
	int    n;
	STRING p;

	while((p = string_arg(ptr)))
	{
		if(same_ic(p,"N")) continue;
		data->item_id = GETMEM(data->item_id, STRING, data->nitems+1);
		data->item_id[data->nitems] = safe_strdup(p);
		data->nitems++;
	}

	if(data->nitems > 0)
	{
		data->item_values = MEM(float, data->nitems * 256); 
		for(n=0; n < data->nitems*256; n++)
		{
			data->item_values[n] = glDATA_MISSING;
		}
	}
}


/* Read the data table that follows a TableStart_[....] line
 */
static LOGICAL read_data_table(FILE *fp, IMRADDATA *data, glCOLOR *cmap)
{
	int     n, pixel, dbz_ndx = 0;
	char    buf[500];
	STRING  ptr, s, p;
	LOGICAL ok;

	/* Initialize the colour table */
	for( n = 0; n < 256; n++)
	{
		cmap[n].red   = (UNCHAR)n;
		cmap[n].green = (UNCHAR)n;
		cmap[n].blue  = (UNCHAR)n;
	}

	/* The dbz column is used as a reference to set the default
	 * transparency in the colour map.
	 */
	for( n = 0; n < data->nitems; n++ )
	{
		if(!same_ic(data->item_id[n], "DBZ")) continue;
		dbz_ndx = n;
		break;
	}

	while((ptr = mygetline(fp, buf, 500, NULL)))
	{
		if(same_start_ic(ptr, "TableEnd")) break;
		s = ptr;
		while((p = strchr(s,';')))
		{
			*p = '\0';
			pixel = int_arg(s, &ok);
			if(ok && pixel >= 0 && pixel < 256)
			{
				for( n = 0; n < data->nitems; n++)
				{
					float mag = float_arg(s, &ok);
					if (ok) data->item_values[pixel*data->nitems+n] = mag;
				}
				/* Set default transparancey as determined by the first entry
				 * (hopefully DBZ). The <= is used here as in some files (like doppler)
				 * missing is indicated by -999.9999 and missing is -999.
				 */
				if(data->item_values[pixel*data->nitems+dbz_ndx] <= glDATA_MISSING)
				{
					data->item_values[pixel*data->nitems+dbz_ndx] = glDATA_MISSING;
					cmap[pixel].red   = T_RED;
					cmap[pixel].green = T_GREEN;
					cmap[pixel].blue  = T_BLUE;
				}
			}
			s = p+1;
		}
	}
	/* If ptr is NULL then we reached the end of the file without finding the
	 * termination line and the file is probably corrupt.
	 */
	return (ptr != NULL);
}



/* The engineering units table is just a one to one mapping of the units
 * and is used when a data mapping table is not found in the meta file.
 * The meaning of the units will be up to the user to intrepret. In this
 * function we do not create the table but just assign the table_id. The
 * actual table is created in _xgl_create_radar_data_table().
 */
static void assign_engineering_units_table(IMRADDATA *table)
{
	/* If there was a TableLabels_ line then the table_id variable will
	 * not be null. This normally should not happen, but you never know.
	 */
	if(table->table_id)
	{
		FREEMEM(table->table_id);
		FREELIST(table->item_id, table->nitems);
		table->nitems = 0;
		FREEMEM(table->item_values);
	}

	table->table_id = safe_strdup(ENGINEERING_UNITS);
}


static void destroy_data_table( IMRADDATA **table )
{
	FREEMEM((*table)->table_id);
	FREELIST((*table)->item_id, (*table)->nitems);
	FREEMEM((*table)->item_values);
	FREEMEM(*table);
}


/*------------ Public within library functions ---------------*/


/* Since the data contained in radar data files tends to be the same within
 * a group of images, we create a table and point to this single entitiy to
 * save memory.
 */
IMRADDATA *_xgl_create_radar_data_table( IMRADDATA *table )
{
	int i, n;

	/* Search for an existing table
	 */
	for(i = 0; i < ndata_tables; i++)
	{
		if(!same(table->table_id, data_tables[i]->table_id)) continue;

		if( table->encode != data_tables[i]->encode ) continue;
		if( table->range  != data_tables[i]->range  ) continue;
		if( table->theta  != data_tables[i]->theta  ) continue;
		if( table->rscale != data_tables[i]->rscale ) continue;
		if( table->tscale != data_tables[i]->tscale ) continue;
		if( table->nitems != data_tables[i]->nitems ) continue;

		/* If engineering table we do not check contents */
		if(!same(table->table_id, ENGINEERING_UNITS))
		{
			for(n = 0; n < table->nitems; n++)
			{
				if(!same(table->item_id[n],data_tables[i]->item_id[n])) break; 
			}
			if(n < table->nitems) continue;
			for(n = 0; n < table->nitems*256; n++)
			{
				if(table->item_values[n] != data_tables[i]->item_values[n]) break;
			}
			if(n < table->nitems*256) continue;
		}

		/* At this point we have a match in the stored tables so destroy
		 * the given table and return a pointer to the stored one.
		 */
		destroy_data_table(&table);

		return (data_tables[i]);
	}

	/* No existing table so add new one. If an engineering table create
	 * the table assignments now.
	 */
	if(same(table->table_id, ENGINEERING_UNITS))
	{
		table->nitems      = 1;
		table->item_id     = ONEMEM(STRING);
		table->item_id[0]  = safe_strdup("EU");
		table->item_values = MEM(float, 256); 

		for( n = 0; n < 256; n++)
			table->item_values[n] = (float)n;
	}

	data_tables = GETMEM(data_tables, IMRADDATA*, ndata_tables+1);
	data_tables[ndata_tables] = table;
	ndata_tables++;
	return table;
}



LOGICAL _xgl_isGridURP(STRING fname)
{
	return(urp_file_type(fname) == ImageEncodingGriddedURP);
}


LOGICAL _xgl_queryGridURP(STRING fname, glCOLOR *cmap, IMDEF *info, IMRADDATA **table)
{
	int       n, w = 480, h = 480;
	char      buf[500];
	float     lat, lon, scale = 1.0;
	STRING    key, p, ptr;
	LOGICAL   ok, pound_start;
	LOGICAL   table_found = FALSE;
	IMRADDATA *data;
	FILE      *fp;

	/* Initialize colour table to greyscale */
	for( n = 0; n < 256; n++)
	{
		cmap[n].red   = (UNCHAR)n;
		cmap[n].green = (UNCHAR)n;
		cmap[n].blue  = (UNCHAR)n;
	}

	data = ONEMEM(IMRADDATA);
	data->rscale = 1.0;

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	while((ptr = mygetline(fp, buf, 500, &pound_start)))
	{
		key = string_arg(ptr);

		if(same_ic(key, "Data"))
		{
			break; /* the end of the header block */
		}
		else if(same_ic(key, "LatCentre"))
		{
			lat = read_lat(ptr, &ok);
			if (!ok) goto err2;
		}
		else if(same_ic(key, "LonCentre"))
		{
			lon = read_lon(ptr, &ok);
			if (!ok) goto err2;
		}
		else if(same_ic(key, "Scale"))
		{
			if (pound_start)
			{
				p = string_arg(ptr);
				(void)sscanf(p, "%f:%d:%d", &scale, &w, &h);	/* backwards compatability */
			}
			else
			{
				scale = float_arg(ptr, &ok);
				if (!ok) goto err2;
			}
		}
		else if(same_ic(key, "Width"))
		{
			w = int_arg(ptr, &ok);
			if (!ok) goto err2;
		}
		else if(same_ic(key, "Height"))
		{
			h = int_arg(ptr, &ok);
			if (!ok) goto err2;
		}
		else if(same_start_ic(key, "TableLabels_") && IsNull(data->table_id))
		{
			data->table_id = safe_strdup(key+12);
			read_data_table_labels(ptr, data);
		}
		else if(same_start_ic(key, "TableStart_") && same(data->table_id, key+11))
		{
			table_found = TRUE;
			if (read_data_table(fp, data, cmap)) break;
			goto err2;
		}
	}
	if (!ptr) goto err2;

	if (!table_found) assign_engineering_units_table(data);

	(void) fclose(fp);

	info->info.width  = w;
	info->info.height = h;
	data->rscale      = scale;

	if(same_map_def(&info->mproj.definition, &NoMapDef))
	{
		info->mproj.definition.olat  = lat;
		info->mproj.definition.olon  = lon;
		info->mproj.definition.lref  = lon;
		info->mproj.definition.xlen  = w / scale; /* Must be in Km */
		info->mproj.definition.ylen  = h / scale;
		info->mproj.definition.xorg  = info->mproj.definition.xlen / 2.;
		info->mproj.definition.yorg  = info->mproj.definition.ylen / 2.;
		info->mproj.definition.units = 1000;
	}

	*table = data;

	return TRUE;

err2:
	(void) fclose(fp);
	destroy_data_table(&data);
	pr_error(NULL,"Gridded data file parse error.\n");
	return FALSE;
}


/* Read a radar image in appropriate file format.
 */
LOGICAL _xgl_get_gridded_urp_raster(ImagePtr im, UNCHAR **rast)
{
	int    y;
	char   buf[20];
	STRING ptr, errstr;
	UNCHAR *raster = (UNCHAR*)0;
	FILE   *fp;

	fp = _xgl_open_image_file(im);
	if (!fp) return FALSE;

	while((ptr = mygetline(fp, buf, 20, NULL)) != NULL && !same_ic(ptr, "DATA-"));
	errstr = nodata;
	if (!ptr) goto err4;
	raster = MEM(UNCHAR, im->ow*im->oh);
	errstr = nomem;
	if (!raster) goto err4;
	errstr = noread;
	for( y = im->oh-1; y >= 0; y--)
	{
		if(fread(raster+y*im->ow, 1, (size_t)im->ow, fp) != (size_t) im->ow) goto err4;
	}

	*rast = raster;

	(void) fclose(fp);
	return TRUE;

err4:
	(void) fclose(fp);
	FREEMEM(raster);
	pr_error(NULL,"  %s\n", errstr);
	ptr = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
	pr_error(NULL,"Errors encountered while reading radar image \"%s\"\n", ptr);
	FREEMEM(ptr);
	return FALSE;
}



LOGICAL _xgl_isPolarURP(STRING fname)
{
	return(urp_file_type(fname) == ImageEncodingPolarURP);
}


LOGICAL _xgl_queryPolarURP(STRING fname, glCOLOR *cmap, IMDEF *info, IMRADDATA **table)
{
	int       n, range, theta;
	char      buf[500];
	STRING    key, ptr, errstr;
	float     lat, lon, rscale = 1.0, tscale = 1.0;
	LOGICAL   ok;
	LOGICAL   table_found = FALSE;
	IMRADDATA *data;
	FILE      *fp;

	/* Initialize colour table to greyscale */
	for( n = 0; n < 256; n++)
	{
		cmap[n].red   = (UNCHAR)n;
		cmap[n].green = (UNCHAR)n;
		cmap[n].blue  = (UNCHAR)n;
	}

	data = ONEMEM(IMRADDATA);
	data->rscale = 1.0;
	data->tscale = 1.0;

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	while((ptr = mygetline(fp, buf, 500, NULL)))
	{
		key = string_arg(ptr);

		if(same_ic(key, "Data"))
		{
			break; /* the end of the header block */
		}
		else if(same_ic(key, "Range"))
		{
			range = int_arg(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "Theta"))
		{
			theta = int_arg(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "BinResolution"))
		{
			rscale = float_arg(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "AzimuthalResolution"))
		{
			tscale = float_arg(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "LatCentre"))
		{
			lat = read_lat(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "LonCentre"))
		{
			lon = read_lon(ptr, &ok);
			if (!ok) goto err3;
		}
		else if(same_ic(key, "DataElementFormat"))
		{
			if(same_ic(string_arg(ptr),"ascii") && same_ic(string_arg(ptr),"float"))
				data->encode = AsciiFloat;
		}
		else if(same_start_ic(key, "TableLabels_") && IsNull(data->table_id))
		{
			data->table_id = safe_strdup(key+12);
			read_data_table_labels(ptr, data);
		}
		else if(same_start_ic(key, "TableStart_") && same(data->table_id, key+11))
		{
			table_found = TRUE;
			if (read_data_table(fp, data, cmap)) break;
			errstr = noend;
			goto err3;
		}
	}
	errstr = nodata;
	if (!ptr) goto err3;

	if (!table_found) assign_engineering_units_table(data);

	(void) fclose(fp);

	info->info.width   = range * rscale * 2;	/* Must be in Km */
	info->info.height  = range * rscale * 2;

	data->range  = range;
	data->theta  = theta;
	data->rscale = rscale;
	data->tscale = tscale;

	if(same_map_def(&info->mproj.definition, &NoMapDef))
	{
		info->mproj.definition.olat  = lat;
		info->mproj.definition.olon  = lon;
		info->mproj.definition.lref  = lon;
		info->mproj.definition.xorg  = range * rscale;
		info->mproj.definition.yorg  = range * rscale;
		info->mproj.definition.xlen  = (float) info->info.width;
		info->mproj.definition.ylen  = (float) info->info.height;
		info->mproj.definition.units = 1000;
	}

	*table = data;

	return TRUE;

err3:
	(void) fclose(fp);
	destroy_data_table(&data);
	pr_error(NULL,"Range-Theta data file parse error. %s\n", errstr);
	return FALSE;
}


LOGICAL _xgl_get_polar_urp_raster(ImagePtr im, UNCHAR **rast)
{
	int    size;
	char   buf[50];
	long   fpos;
	STRING s, p, ptr, errstr;
	UNCHAR *raster = (UNCHAR*)0;
	FILE   *fp;

	fp = _xgl_open_image_file(im);
	if (!fp) return FALSE;

	size = 0;
	while((ptr = mygetline(fp, buf, 50, NULL)))
	{
		s = strtok(ptr, " ");
		p = strtok(NULL, " ");
		if(same_ic(s, "SizeInBytes")) (void)sscanf(p, "%d", &size);
		if(size > 0 && same_start_ic(s, "Data")) break;
		fpos = ftell(fp);
	}
	errstr = nodata;
	if (!ptr) goto err5;
	raster = MEM(UNCHAR, size);
	errstr = nomem;
	if (!raster) goto err5;
	/*
	 * The data key is "Data " so offset is 5
	 */
	(void) fseek(fp, fpos+5, SEEK_SET);
	errstr = noread;
	if(fread(raster, 1, (size_t) size, fp) != (size_t) size) goto err5;

	/* If the elements are ascii float then the raster will need to be
	 * parsed into an array of float values and the raster will then
	 * be set to this array.
	 */
	if(im->info.radar->encode == AsciiFloat)
	{
		int n;
		float *array;
		size = im->info.radar->range * im->info.radar->theta;
		errstr = nomem;
		array = MEM(float, size);
		if(!array) goto err5;

		ptr = (STRING)raster;
		for( n = 0; n < size; n++ )
		{
			array[n] = (float)strtod(ptr, &s);
			if(s == ptr)
			{
				array[n] = 0.0;
				break;
			}
			p = strchr(ptr,',');
			if (!p) break;
			ptr = p + 1;
		}
		FREEMEM(raster);
		raster = (UNCHAR *)array;
	}

	*rast = raster;

	(void) fclose(fp);
	return TRUE;

err5:
	(void) fclose(fp);
	FREEMEM(raster);
	pr_error(NULL,"  %s\n", errstr);
	ptr = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
	pr_error(NULL,"Errors encountered while reading radar image \"%s\"\n", ptr);
	FREEMEM(ptr);
	return FALSE;
}
