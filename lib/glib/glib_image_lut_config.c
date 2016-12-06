/******************************************************************************/
/**
 *	@file glib_image_lut_config.c
 *
 *   Functions for creating and manipulating colour look up tables (LUT).
 */
/******************************************************************************/
/*
 *
 *   This is complex as it handles look-up-tables associated with data images
 *   as well as byte images. Data images consist of arrays of numbers, usually
 *   floats, which thus need to associate a range of values with a given
 *   colour.
 *
 *   Note that the returned index is an integer value. Values in the range of
 *   -1 to -SAT_LUT_OFFSET are reserved for radar look-up tables associated
 *   with radar data "images". Index values less than SAT_LUT_OFFSET are for
 *   satellite or data tables.
 *
 *   Radar data tables require special processing as the information required
 *   to generate the table will not generally be available when the table is
 *   first read.
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
/******************************************************************************/
#include <limits.h>
#include <tools/tools.h>
#include <fpa_math.h>
#include "glib_private.h"

#define SAT_LUT_OFFSET	10000

/* General lookup table array. The upper_bound and lower_bound values are used
 * to hold information to enable the creation of a legend only and have no
 * other use.
 */
static int    ngenlut = 0;
static glLUT *genlut  = (glLUT *)0;

/* Lookup table for satellite and gridded data. The bound values are used to
 * create the image as well as for use in creating a legend.
 */
static int    ngrid_data = 0;
static glLUT *grid_data  = (glLUT *)0; 

/* See glImageReadLUT for a description of the reason for the large amount
 * of information required for radar data look-up tables.
 */
typedef struct {
	STRING    table_id;		/* data table the radar lut is for */
	STRING    item_id;		/* data item the lut is for */
	glLUT     dlut;			/* data-rgb assignment colour lookup table */
	int       ndata;	    /* number of radar pixel data elements using this lut */
	IMRADDATA **data;	    /* radar data pointer array */
	int       *lut;			/* the lut assigned to this path/data combination */
} RLUT;

static int  nradar_data  = 0;
static RLUT *radar_data = (RLUT*)0;



/*======================= Private functions ==========================*/



/* return the next token, defined as anything surrounded by white space.
 */
static STRING next_token( STRING line )
{
	line += strcspn(line, WHITESPACE);
	line += strspn(line, WHITESPACE);
	return line;
}



/* Sorting the data tables will allow for binary searches of the data to
 * speed things up.
 */
static void sort_data_lut_table( glLUTCOLOR *ls, int nls )
{
	int        i;
	LOGICAL    do_sort = TRUE;
	glLUTCOLOR dls;

	while(do_sort)
	{
		do_sort = FALSE;
		for(i=1; i<nls; i++)
		{
			if(ls[i].lower_bound >= ls[i-1].lower_bound) continue;
			do_sort = TRUE;
			(void) memcpy((void*)&dls,     (void*)&ls[i-1], sizeof(glLUTCOLOR));
			(void) memcpy((void*)&ls[i-1], (void*)&ls[i],   sizeof(glLUTCOLOR));
			(void) memcpy((void*)&ls[i],   (void*)&dls,     sizeof(glLUTCOLOR));
		}
	}
}



/*============================= Library Private Functions ===================*/




/* This requires some explanation. Often in palatted images like GIF the colour map
 * changes from image to image depending on the encoding. The resulting raster colours
 * are the same from image to image, but the location of the colours in the colour
 * map changes. This function first tries to find an exact match to the colour map.
 * If not found it tries to find a table containing all of the colours independent of
 * order. if found then a remapping array is created and returned. If still not found
 * it makes a new lut. 
 *
 * Note that input colour arrays of 256 elements fully filled out is assumed and
 *      this function creates arrays of 256 elements.
 *
 * Parms:
 *   Input:	 cmap  - The input colormap.
 *
 *   Return: remap - The remapping array.
 *           lutp  - pointer to the returned lut
 *
 * Return:
 *   The required lut.
 *        
 */
ImageLUT _xgl_create_lut(glCOLOR *cmap, UNCHAR **remap, glLUT **lutp)
{
	int   i, j, n;

	if (remap) *remap = NULL;
	if (lutp)  *lutp  = NULL;

	/* The first time in we will create a greyscale lut for all those images
	 * which end up using this as a default.
	 */
	if(ngenlut == 0)
	{
		genlut = INITMEM(glLUT, 1);
		(void) memset((void*)genlut, 0, sizeof(glLUT));
		genlut[0].ncells = 256;
		genlut[0].cells  = INITMEM(glLUTCOLOR, 256);
		for( i = 0; i < 256; i++ )
		{
			genlut[0].cells[i].red   = (UNCHAR)i;
			genlut[0].cells[i].green = (UNCHAR)i;
			genlut[0].cells[i].blue  = (UNCHAR)i;
		}
		ngenlut = 1;
	}

	/* If the colormap array is null create an entry and return.
	 * It is assumed that the calling function will operate on this
	 * and fill in a valid colormap
	 */
	if(!cmap)
	{
		genlut = GETMEM(genlut, glLUT, ngenlut+1);
		(void) memset((void*)&genlut[ngenlut], 0, sizeof(glLUT));
		genlut[ngenlut].ncells = 256;
		genlut[ngenlut].cells  = INITMEM(glLUTCOLOR, 256);
		if (lutp) *lutp = genlut + ngenlut;
		ngenlut++;
		return ngenlut;
	}

	/*  Search for an existing lut array entry.
	 */
	for( n=0; n < ngenlut; n++)
	{
		if(genlut[n].ncells != 256) continue;

		for( i = 0; i < 256; i++ )
			if(!SAME_COLOR(cmap[i], genlut[n].cells[i])) break;
		if(i >= 256)
		{
			if (lutp) *lutp = genlut + n;
			return n+1;
		}
	}

    /* Not found so search for an existing entry that contains all of
     * the colours - but only if the remap variable is defined.
     */
    if(remap)
    {
        int trns, map[256];
        
        /* Find a colormap that contains all of our colours */
        for( n=0; n < ngenlut; n++)
        {
            if(genlut[n].ncells != 256) continue;

            /* Find a transparent entry in the colormap. As there are
             * normally many transparent entries in a colormap this is
             * used below to speed up the colour matching.
             */
            for(trns = 0; trns < 256; trns++)
                if(!OPAQUE_COLOR(genlut[n].cells[trns])) break;

            /* Scan through our input colormap */
            for( i = 0; i < 256; i++ )
            {
                map[i] = i;
                if(SAME_COLOR(cmap[i], genlut[n].cells[i])) continue;

                if(OPAQUE_COLOR(cmap[i])) 
                {
                    for( j = 0; j < 256; j++ )
                    {
                        if(SAME_COLOR(cmap[i], genlut[n].cells[j]))
                        {
                            map[i] = j;
                            break;
                        }
                    }
                    if(j >= 256) break;
                }
                else if(trns < 256)
                {
                    map[i] = trns;
                }
                else
                {
                    break;
                }
            }
            
            if(i >= 256)
            {
                UNCHAR *m = (UNCHAR *)malloc(256);
                for(i = 0; i < 256; i++) m[i] = (UNCHAR) map[i];
                *remap = m;
                if (lutp) *lutp = genlut + n;
                return n+1;
            }
        }
    }


    /* Nothing contains all of the colours so need a new entry */
    genlut = GETMEM(genlut, glLUT, ngenlut+1);
    (void) memset((void*)&genlut[ngenlut], 0, sizeof(glLUT));
    genlut[ngenlut].ncells = 256;
    genlut[ngenlut].cells  = INITMEM(glLUTCOLOR, 256);
    for( i = 0; i < 256; i++ )
    {
        genlut[ngenlut].cells[i].red   = cmap[i].red;
        genlut[ngenlut].cells[i].green = cmap[i].green;
        genlut[ngenlut].cells[i].blue  = cmap[i].blue;
    }
    if (lutp) *lutp = genlut + ngenlut;
    ngenlut++;
    return ngenlut;
}


/* Return a copy of the data colour look up table and set the color map type.
 * This is almost exactly like glImageGetDataLUT() below, but takes an image
 * pointer as an argument and sets the color map type in the image structure.
 * Note that the returned lut array must be freed by the calling routine.
 */
LOGICAL _xgl_get_image_data_lut( ImagePtr im, glLUTCOLOR **ls, size_t *nls )
{
    int        ndx = 0;
    size_t     nlp = 0;
    glLUTCOLOR *lp = NULL;

    *nls = 0;
    *ls  = NULL;

    if( im->lut >= 0 ) return FALSE;

    ndx = abs(im->lut);
    if( ndx <= nradar_data )
    {
        ndx--;
        lp  = radar_data[ndx].dlut.cells;
        nlp = (size_t) radar_data[ndx].dlut.ncells;
    }
    else
    {
        ndx -= SAT_LUT_OFFSET;
        if( ndx <= ngrid_data )
        {
            ndx--;
            lp  = grid_data[ndx].cells;
            nlp = (size_t) grid_data[ndx].ncells;
        }
    }

    if (!lp)  return FALSE;
    if (!nlp) return FALSE;

    *nls = nlp;
    *ls  = INITMEM(glLUTCOLOR, nlp);
    (void) memcpy((void *)(*ls), (void *)lp, nlp*sizeof(glLUTCOLOR));
    return TRUE;
}

/*   Get the lut for the image if it exists. If the lut reference is less than
 *   0 it is for radar. If the cross reference does not exist for this then
 *   we must generate it.
 */
void _xgl_get_image_cmap( ImagePtr im, glCOLOR *cmap )
{
    int    n, ndx;
    glLUT *lutptr;

    /* Set return to the default grayscale.
     */
    for(n = 0; n < 256; n++)
        cmap[n].red = cmap[n].green = cmap[n].blue = (UNCHAR)n;

    if(IsNull(im) || im->lut == glNoLUT) return;

    if(im->lut < 0)
    {
        if(im->group == RadarGroup && im->info.radar != 0)
        {
            if(im->info.radar->nlut_xref < nradar_data)
            {
                im->info.radar->lut_xref = GETMEM(im->info.radar->lut_xref, ImageLUT, nradar_data);
                for(n=im->info.radar->nlut_xref; n < nradar_data; n++)
                {
                    im->info.radar->lut_xref[n] = -1;
                }
                im->info.radar->nlut_xref = nradar_data;
            }
            ndx = abs(im->lut)-1;

            /* If the cross reference is < 0 this means that we have not assigned it yet */
            if(im->info.radar->lut_xref[ndx] < 0)
            {
                im->info.radar->lut_xref[ndx] = _xgl_create_radar_data_lut(im->info.radar, ndx);
            }
            if(im->info.radar->lut_xref[ndx] == glNoLUT ) return;
            if(im->info.radar->lut_xref[ndx] <  1       ) return;
            if(im->info.radar->lut_xref[ndx] >  ngenlut ) return;

            lutptr = genlut + im->info.radar->lut_xref[ndx] - 1;
            for(n = 0; n < lutptr->ncells; n++)
            {
                cmap[n].red   = lutptr->cells[n].red;
                cmap[n].green = lutptr->cells[n].green;
                cmap[n].blue  = lutptr->cells[n].blue;
            }
        }
        return;
    }
    else if ( im->lut > 0 && im->lut <= ngenlut )
    {
        lutptr = genlut + im->lut - 1;
        for(n = 0; n < lutptr->ncells; n++)
        {
            cmap[n].red   = lutptr->cells[n].red;
            cmap[n].green = lutptr->cells[n].green;
            cmap[n].blue  = lutptr->cells[n].blue;
        }
    }
    return;
}


/* Note that for any given radar lut table there could be more than one pixel data table.
 * This is because different radar data images could contain different assignments of
 * data values for the same data_label/element combination. Normally this should not happen
 * but one never knows as different radars could have different calibrations. We thus end
 * up with a matrix of radar lut table vs radar pixel data table.
 */
ImageLUT _xgl_create_radar_data_lut( IMRADDATA *imrd, int rdndx )
{
    int     i, n, ndx;
    glCOLOR cmap[256];
    float   val;
    RLUT    *rd;

    if(imrd->nitems < 1) return glNoLUT;
    if(rdndx >= nradar_data) return glNoLUT;

    rd = radar_data + rdndx;

    /* Have we got this element already?
     */
    for(n = 0; n < rd->ndata; n++)
    {
        if(rd->data[n] == imrd) return rd->lut[n];
    }

    /* We don't so add this data to our list
     */
    n = rd->ndata++;
    rd->data = GETMEM(rd->data, IMRADDATA*, rd->ndata);
    rd->lut  = GETMEM(rd->lut, int , rd->ndata);

    rd->data[n] = imrd;
    rd->lut[n]  = glNoLUT;

    /* Make sure we are dealing with the same data table
     */
    if(!same(rd->table_id, imrd->table_id))
    {
        rd->lut[rd->ndata-1] = glNoLUT;
        return glNoLUT;
    }

    /* Now ensure that we have the appropriate data item in our data table
     */
    for(ndx=0; ndx<imrd->nitems; ndx++)
    {
        if(same(rd->item_id, imrd->item_id[ndx])) break;
    }
    if( ndx >= imrd->nitems )
    {
        rd->lut[rd->ndata-1] = glNoLUT;
        return glNoLUT;
    }

    /* Now apply the radar data lut table to our radar data table to create the
     * look-up table appropriate to the data table. The entry is transparent by
     * default.
     */
    for(n = 0; n < 256; n++)
    {
        /* Set default transparent */
        cmap[n].red   = T_RED;
        cmap[n].green = T_GREEN;
        cmap[n].blue  = T_BLUE;

        val = imrd->item_values[n*imrd->nitems+ndx];
        if(val != glDATA_MISSING)
        {
            for(i=0; i<rd->dlut.ncells; i++)
            {
                if(val >= rd->dlut.cells[i].lower_bound && val <  rd->dlut.cells[i].upper_bound)
                {
                    cmap[n].red   = rd->dlut.cells[i].red;
                    cmap[n].green = rd->dlut.cells[i].green;
                    cmap[n].blue  = rd->dlut.cells[i].blue;
                    break;
                }
            }
        }
    }
    rd->lut[rd->ndata-1] = _xgl_create_lut(cmap, NULL, NULL);
    return rd->lut[rd->ndata-1];
}


/* Parse the given string looking for a colour reference. This
 * can be r g b (027 133 206) or an X colour name ("green").
 */
static LOGICAL get_rgb_color(STRING ptr, int *r, int *g, int *b)
{
	char    cname[100];
	char    line[100];
	FILE    *fp;
	STRING  rgbfile;

	/* This is the default location of the rbg.txt file.
	 * Different operating systems may have a different location
	 * which can be reset using the RGB_TXT_FILE environment variable.
	 */
	const STRING default_rgbfile = "/usr/share/X11/rgb.txt";

	/* First try for a three digit colour specification */
	if(sscanf(ptr, "%d%d%d", r, g, b) == 3 ) return TRUE;

	/* Try for a colour name from the rgb.txt file. */
	/* Use the RGB_TXT_FILE environment variable path (if available) */
	rgbfile = getenv("RGB_TXT_FILE");
	if(blank(rgbfile))
	{
		pr_error("[get_rgb_color]", "Environment variable RGB_TXT_FILE not set! Using default location.\n");
		/* Use the default location */
		rgbfile = default_rgbfile;
	}

	/* Open the rgb.txt file */
	fp = fopen(rgbfile,"r");
	if(!fp)
	{
		pr_error("[get_rgb_color]", "Unable to open %s. Use rgb values instead of colour names.\n", rgbfile);
		return FALSE;
	}

	/* Find the colour name */
	no_white(ptr);
	while(getvalidline(fp, line, 100, "!"))
	{
		if(sscanf(line, "%d%d%d%s", r,g,b,cname) != 4) continue;
		no_white(cname);
		if(!same_ic(cname,ptr)) continue;
		(void) fclose(fp);
		return TRUE;
	}
	(void) fclose(fp);
	pr_error("[get_rgb_color]", "Unable to find the colour name '%s' in %s.\n", ptr, rgbfile);
	return FALSE;
}



/*============================== Public Functions ============================*/



/******************************************************************************/
/**
     @brief   Read colourmap look-up table files and radar data files.

	 @param[in] path  The full path name of the file.

	 @return  A lut identifier.

	 @note  The contents of the colour look-up table file must be as follows:
  
     @verbatim
  
   COLOUR LOOKUP TABLE DESCRIPTIONS
  
      There are two types of colour lookup tables, one for mapping a greyscale
      ramp into colours and one for mapping data values as found in radar data
      URP files and binary encoded files as output by TerraScan.
  
   TABLE LABEL LINE
  
      All of the tables have a label display line. This is used to provide the
      labels to the user when the colour table is to be displayed in a legend
      so the user can relate the colours in an image to the values the colours
      are representing. This line consists of two elements:
  
      1. A label that describes the colour table that will be meaningful when
         displayed to a user.
      2. A units label that describes the units used in the table that will
         be seen by the user.
  
      An example line: "Cloud Top Temperature" "Deg C"
  
      Any label that contains embedded spaces must be enclosed in quotes.
      I normally find it best to use quotes for everything so that I don't
      forget.
  
   REGULAR LOOK UP TABLES
  
      For regular look up tables each line specifies the mapping of each of the
      possible 256 greyscale pixel values into colour space.
  
      The first line is the table label line (see above). Note that this and
      the values discussed in items 1 and 3 below are optional, but if left
      out it will not be possible to display the colourmap information to the
      user in a legend.
  
      The following lines contain the look-up table data. There are 3 types
      of data line entry:
  
      1. the keyword "ramp" followed by the number of pixels to apply the ramp
         to, followed by two sets of rgb index values that denote the beginning
         and end colours of the ramp, followed by the value corresponding to the
         first ramp entry then the value corresponding to the last ramp entry.
         If the table was for satellite cloud top temperatures the entry
  
            ramp 10 50 50 50 100 100 100 -30 -20
  
         would create a linear greyscale ramp for the next 10 pixels in the
         table starting with the rgb colour 50 50 50 and ending at the rgb colour
         100 100 100. The first ramp value corresponds to -30 degrees and the
         last ramp value corresponding to -20 degrees.
  
      2. the keyword "transparent" followed by the number of pixels to be set
         to be transparent. If there is no number following the keyword then
         the assumption of 1 pixel is made.
  
      3. one set of rgb pixel values applied to the next pixel in the sequence
         followed by the corresponding value. Thus a line containing
         126 25 220 -10 would apply the rgb colour 126 25 220 to the pixel and
         denote a value of -10 degrees if this was for cloud top temperature.
  
      Each of these lines is cumulative so that if a ramp of 10 pixels is
      followed by an rgb value the rgb value would apply to the 11th pixel.
  
   RADAR DATA TABLES
  
      The first non-comment line in the file must be "RadarDataLUT"
  
      This one is somewhat complicated due to the fact that any given radar data
      image can contain more than one data mapping to any given pixel in the image.
      An example radar data table as found in the URP data file might be:
  
      PrecipitationRate-Reflectivity
      N  DBZ      MM/HR  DBZ_SNOW   CM/HR
      0 -999.     -999.     -999.   -999.
      1 -31.5000 0.0004  -25.0000  0.0025
      2 -31.0000 0.0004  -24.5000  0.0026
      ...
      68  2.0000 0.0486    8.5000  0.0820
      69  2.5000 0.0523    9.0000  0.0864 
      70  3.0000 0.0562    9.5000  0.0910
      71  3.5000 0.0603    10.0000 0.0959 
      72  4.0000 0.0648    10.5000 0.1010
      73  4.5000 0.0697    11.0000 0.1064
      ...
  
      In this case the table label is "PrecipitationRate-Reflectivity", N is the
      pixel number and the line "DBZ MM/HR DBZ_SNOW CM/HR" indicates the 4 data
      items associated with this particular data image.
  
      In order to map values of any parameter to a colour we require a colour
      look up table. In order to match this table with the appropriate element
      in the URP data table we need a table which contains the table label
      it applies to and the specific item in the table it applies to. So for
      example we could have
  
      RadarDataLUT
      PrecipitationRate-Reflectivity MM/HR
        "PR" "Precipitation Rate" "MM/HR"
  
      0.05  1   240 240 240 
      1     2   255 255   0 
      2     4     0 200   0 
      4     8     0 240 255 
      ...
  
      Thus our colormap look up table is constructed as follows:
  
      The first line is our manditory identification for radar.
  
      The second line contains our data table label, which in this case is
      "PrecipitationRate-Reflectivity", and the data column which the colour mapping
      is for, in this case "MM/HR". Note that there are forms of data files that
      do not use the indirection of mapping a pixel to the table as above, but
      provide the values directly in the data array section of the file. An example
      of this is a rainfall accumulation product. In this case the line can be
      set to "None None".
  
      The third line is table label line (see above).
  
      This is followed by the data value to colour maping section where the
      numbers on each line correspond to the lower value of the range the
      upper value of the range and the colour to map any pixel value in the
      range to. We see that in the example above that pixel 69 is the first
      to fall within the range of the first entry in the look up table and would
      be mapped to the colour 240 240 240. Any values not mapped result in the
      corresponding pixel being mapped to transparent.
  
      Note that an entry of - for our lower or upper bounds will set the
      corresponding value to the smallest or largest possible value.
  
      The colour specification can also be an X colour name like "red", the key
      word "transparent" or the above rgb triple values.
  
      *** NOTE ***
  
      If the data file does not have a data mapping table as shown above the
      library will substitute a special one to one table as follows:
  
      EngineeringUnits EU
      N   EU
      0   0.
      1   1.
      2   2.
      ....
  
      A colour table can be constructed as described above by substituting
      all mention of PrecipitationRate-Reflectivity with EngineeringUnits
      and all MM/HR examples with EU.
  
  
   SATELLITE DATA TABLES
  
      The first non-comment line in the file must be "SatelliteDataLUT"
  
      The second line is the table label line (see above).
  
      The following lines are the data. Like the radar luts, these tables
      associate a range of values with colours. There are two forms of entry:
  
      1. These entries are a range of values associated with a colour.
      This consists of a lower limit value (>=) and an upper limit value (<)
      followed by the assigned colour. If the image contained temperature,
      then the line
  
            -40 -30 160 240 100
  
      would assign the rgb value 160,240,100 to any temperature that is
      greater-than-or-equal_to (>=) -40 and less-than (<) -30. The colour
      rgb triplet may be replaced by an X Color name.
  
      2. The keyword "ramp" followed by a lower limit, upper limit, the
      number of intervals between them, a starting rgb colour and an ending
      rgb colour. For example
  
          ramp -50 +50 20 255 255 255 0 0 0
  
      would produce 20 entries starting at -50 -55 255 255 255 and ending
      at 45 50 0 0 0.
  
      Any value not matched to a particular range will default to transparent.
	   
  
   DATA TABLES
  
      The first non-comment line in the file must be "DataLUT"
  
      The second line is the table label line (see above).
  
      The following lines are the data. Like the radar luts, these tables
      associate a range of values with colours. There are two forms of entry:
  
      1. These entries are a range of values associated with a colour.
      This consists of a lower limit value (>=) and an upper limit value (<)
      followed by the assigned colour. If the image contained temperature,
      then the line
  
            -40 -30 160 240 100
  
      would assign the rgb value 160,240,100 to any temperature that is
      greater-than-or-equal_to (>=) -40 and less-than (<) -30. The colour
      rgb triplet may be replaced by an X Color name.
  
      2. The keyword "ramp" followed by a lower limit, upper limit, the
      number of intervals between them, a starting rgb colour and an ending
      rgb colour. For example
  
          ramp -50 +50 20 255 255 255 0 0 0
  
      would produce 20 entries starting at -50 -55 255 255 255 and ending
      at 45 50 0 0 0.
  
      Any value not matched to a particular range will default to transparent.

  
   GRIDDED DATA TABLES
  
      The first non-comment line in the file must be "GriddedDataLUT".
  
      The second line is the table label line (see above).
  
      The following lines are the data. Like the radar luts, these tables
      associate a range of values with colours. There are two forms of entry:
  
      1. These entries are a range of values associated with a colour.
      This consists of a lower limit value (>=) and an upper limit value (<)
      followed by the assigned colour. If the image contained temperature,
      then the line
  
            -40 -30 160 240 100
  
      would assign the rgb value 160,240,100 to any temperature that is
      greater-than-or-equal_to (>=) -40 and less-than (<) -30. The colour
      rgb triplet may be replaced by an X Color name.
  
      2. The keyword "ramp" followed by a lower limit, upper limit, the
      number of intervals between them, a starting rgb colour and an ending
      rgb colour. For example
  
          ramp -50 +50 20 255 255 255 0 0 0
  
      would produce 20 entries starting at -50 -55 255 255 255 and ending
      at 45 50 0 0 0.
  
      Any value not matched to a particular range will default to transparent.

      @endverbatim
*/
/******************************************************************************/

ImageLUT glImageReadLUT( STRING path )
{
    int      n, rtn;
    long int posn;
    char     line[250], back[250];
    STRING   ptr;
    float    lim, lval, uval;
    FILE     *fd;

    static STRING Module = "glImageReadLUT";
    static STRING obs    = "File %s is obsolete look-up table. No LUT display label found.\n";
    static STRING generr = "Error in file %s line \"%s\"\n";
    static STRING create = "Creating %s colour look-up table from file \"%s\"\n";

    if (!path) return (ImageLUT)0;

    /* See if we already have this one */
    for(n=0; n<ngenlut; n++)
    {
        if(!same(genlut[n].path,path)) continue;
        pr_diag(Module, "Accessing colour look-up table \"%s\"\n", path);
        return n+1;
    }

    /* radar data lut? */
    for(n=0; n<nradar_data; n++)
    {
        if(!same(radar_data[n].dlut.path,path)) continue;
        pr_diag(Module, "Accessing radar data colour look-up table \"%s\"\n", path);
        return -(n+1);
    }

    /* gridded data lut? */
    for(n=0; n<ngrid_data; n++)
    {
        if(!same(grid_data[n].path,path)) continue;
        pr_diag(Module, "Accessing gridded data colour look-up table \"%s\"\n", path);
        return -(n+1+SAT_LUT_OFFSET);
    }

    /* See if file can be accessed */
    fd = fopen(path, "r");
    if (!fd)
    {
        pr_error(Module,"Unable to open Colour Table file: %s\n", path);
        return glNoLUT;
    }

    /* Read the first valid line of the file to see if this is a radar data
     * or satellite lut specification file or a regular lut specification.
     */
    (void)getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START);
    ptr = string_arg(line);

    if(same_ic(ptr, "RadarDataLUT"))
    {
        int  count, r, g, b;
        RLUT *rd;

        pr_diag(Module, create, "radar data", path);

        nradar_data++;
        rtn = -nradar_data;
        radar_data = GETMEM(radar_data, RLUT, nradar_data);
        rd = radar_data + nradar_data - 1;;
        (void) memset((void*)rd, 0, sizeof(RLUT));

        rd->dlut.path = safe_strdup(path);

        /* The next line has to contain the table label and item label */
        (void)getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START);
        rd->table_id = strdup_arg(line);
        rd->item_id  = strdup_arg(line);

        /* The next line should be the labels. Some old files may not have
         * this line and this explains the checking logic.
         */
        posn = ftell(fd);
        (void)getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START);
        ptr = string_arg(line);
        if(IsNull(ptr) || same(ptr,"-") || sscanf(ptr,"%f", &lim) == 1)
        {
            (void)fseek(fd, posn, SEEK_SET);
            pr_warning(Module, obs, path);
            rd->dlut.id    = rd->table_id;
            rd->dlut.label = rd->table_id;
            rd->dlut.item  = rd->item_id;
        }
        else
        {
            rd->dlut.id    = rd->table_id;
            rd->dlut.label = safe_strdup(ptr);
            rd->dlut.item  = strdup_arg(line);
        }

        /* Find out how many data lines we have
         */
        posn = ftell(fd);
        count = 0;
        while(getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START)) count++;
        (void)fseek(fd, posn, SEEK_SET);
        rd->dlut.cells = INITMEM(glLUTCOLOR, count);

        /* Now we parse the lines to get our lut information */
        while(getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START))
        {
            (void) safe_strcpy(back, line);
            if(same_start(line,"- "))
			{
                rd->dlut.cells[rd->dlut.ncells].lower_bound = FPA_FLT_MIN;
			}
            else if(sscanf(line, "%f", &rd->dlut.cells[rd->dlut.ncells].lower_bound) != 1)
			{
				pr_error(Module, generr, path, back);
				continue;
			}

            ptr = next_token(line);
            if(same_start(ptr,"- "))
			{
                rd->dlut.cells[rd->dlut.ncells].upper_bound = FPA_FLT_MAX;
			}
            else if(sscanf(ptr, "%f", &rd->dlut.cells[rd->dlut.ncells].upper_bound) != 1)
			{
				pr_error(Module, generr, path, back);
				continue;
			}

        	/* Error message if min/max are reversed! */
			if( rd->dlut.cells[rd->dlut.ncells].upper_bound <
            		rd->dlut.cells[rd->dlut.ncells].lower_bound )
			{
				lval = rd->dlut.cells[rd->dlut.ncells].lower_bound;
				uval = rd->dlut.cells[rd->dlut.ncells].upper_bound;
				rd->dlut.cells[rd->dlut.ncells].lower_bound = uval;
				rd->dlut.cells[rd->dlut.ncells].upper_bound = lval;
				pr_error(Module, generr, path, back);
			}

            ptr = next_token(ptr);
            if(!get_rgb_color(ptr, &r, &g, &b))
			{
				pr_error(Module, generr, path, back);
				continue;
			}

            rd->dlut.cells[rd->dlut.ncells].red   = (UNCHAR)r;
            rd->dlut.cells[rd->dlut.ncells].green = (UNCHAR)g;
            rd->dlut.cells[rd->dlut.ncells].blue  = (UNCHAR)b;
            rd->dlut.ncells++;
        }
        sort_data_lut_table(rd->dlut.cells, rd->dlut.ncells);
    }
    else if(same_ic(ptr, "SatelliteDataLUT") || same_ic(ptr,"DataLUT") || same_ic(ptr,"GriddedDataLUT"))
    {
        int    count, nr, r, g, b;
        float  lb, ub;
        glLUT *sld;

		if(same_ic(ptr,"SatelliteDataLUT"))
			ptr = "satellite data";
		else if(same_ic(ptr,"DataLUT"))
			ptr = "data";
		else
			ptr = "gridded data";

        pr_diag(Module, create, ptr, path);

        ngrid_data++;
        rtn = -(ngrid_data + SAT_LUT_OFFSET);
        grid_data = GETMEM(grid_data, glLUT, ngrid_data);
        sld = grid_data + ngrid_data - 1;;
        (void) memset((void*)sld, 0, sizeof(glLUT));

        sld->path = safe_strdup(path);

        /* The next line should be the labels. Some old files may not have
         * this line and this explains the checking logic.
         */
        posn = ftell(fd);
        (void)getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START);
        ptr = string_arg(line);
        if(IsNull(ptr) || same_ic(ptr,"ramp") || same(ptr,"-") || sscanf(ptr,"%f", &lim) == 1)
        {
            (void)fseek(fd, posn, SEEK_SET);
            pr_warning(Module, obs, path);
        }
        else
        {
            sld->label = safe_strdup(ptr);
            sld->item  = strdup_arg(line);
        }

        /* Find out how many data lines we have and alocate memory
         */
        posn = ftell(fd);
        count = 0;
        while(getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START))
        {
            if(same_start_ic(line,"ramp"))
            {
                ptr = next_token(line);
                if(sscanf(ptr, "%f%f%d", &lb, &ub, &nr) == 3) count += nr;
            }
            else
                count++;
        }
        (void)fseek(fd, posn, SEEK_SET);
        sld->cells = INITMEM(glLUTCOLOR, count);

        /* Now we parse the lines to get our lut information */
        while(getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START))
        {
            (void) safe_strcpy(back, line);
            if(same_start_ic(line,"ramp"))
            {
				char  cname1[50], cname2[50];
                int   ir, r0, g0, b0, rn, gn, bn;
                int   rval, gval, bval;
                float dr, dr1, lval, uval;

                ptr = next_token(line);
				/*
				 * Check first for the rgbrgb form of colour specification. If this fails check
				 * for colour specified as an X colour name.
				 */
                if (sscanf(ptr, "%f%f%d%d%d%d%d%d%d", &lb, &ub, &nr, &r0, &g0, &b0, &rn, &gn, &bn) != 9)
				{
					if(sscanf(ptr,"%f%f%d%s%s", &lb, &ub, &nr, cname1, cname2) != 5) goto sat_err;
					if(!get_rgb_color(cname1, &r0, &g0, &b0)) goto sat_err;
					if(!get_rgb_color(cname2, &rn, &gn, &bn)) goto sat_err;
				}

                /* Build a colour ramp */
                for (ir=0; ir<nr; ir++)
                {
                    dr   = (float)ir / (float)nr;
                    dr1  = (float)(ir+1)/(float)nr;
                    lval = lb + dr*(ub-lb);
                    uval = lb + dr1*(ub-lb);
                    rval = (int)((float)r0 + dr*(rn-r0));
                    gval = (int)((float)g0 + dr*(gn-g0));
                    bval = (int)((float)b0 + dr*(bn-b0));
                    sld->cells[sld->ncells].lower_bound = lval;
                    sld->cells[sld->ncells].upper_bound = uval;
                    sld->cells[sld->ncells].red         = (UNCHAR)rval;
                    sld->cells[sld->ncells].green       = (UNCHAR)gval;
                    sld->cells[sld->ncells].blue        = (UNCHAR)bval;
                    sld->ncells++;
                }
            }
            else
            {
                if(same_start(line,"- "))
                    sld->cells[sld->ncells].lower_bound = FPA_FLT_MIN;
                else
                    if(sscanf(line, "%f", &sld->cells[sld->ncells].lower_bound) != 1) goto sat_err;

                ptr = next_token(line);
                if(same_start(ptr,"- "))
                    sld->cells[sld->ncells].upper_bound = FPA_FLT_MAX;
                else
                    if(sscanf(ptr, "%f", &sld->cells[sld->ncells].upper_bound) != 1) goto sat_err;

                ptr = next_token(ptr);
                if(!get_rgb_color(ptr, &r, &g, &b)) goto sat_err;

                sld->cells[sld->ncells].red   = (UNCHAR)r;
                sld->cells[sld->ncells].green = (UNCHAR)g;
                sld->cells[sld->ncells].blue  = (UNCHAR)b;
                sld->ncells++;
            }

            continue;
sat_err:
            pr_error(Module, generr, path, back);
        }
        sort_data_lut_table(sld->cells, sld->ncells);
    }
    else
    {
        int    im, nr, ir;
        int    rval, gval, bval;
        int    r0,rn, g0,gn, b0,bn;
        int    nread;
        float  dr, v0, vn;
        glLUT *lut;

        pr_diag(Module, create, "general", path);

        rtn = _xgl_create_lut(NULL, NULL, &lut);
        lut->path = safe_strdup(path);

        /* The next line should be the labels. Some old files may not have
         * this line and this explains the checking logic.
         */
        posn = ftell(fd);
        (void)getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START);
        ptr = strtok_arg(line);
        if(IsNull(ptr) || same_ic(ptr,"ramp") || same_ic(ptr,"transparent") || sscanf(ptr,"%d", &n) == 1)
        {
            (void)fseek(fd, posn, SEEK_SET);
        }
        else
        {
            lut->id    = safe_strdup(ptr);
            lut->label = strdup_arg(line);
            lut->item  = strdup_arg(line);
        }

        /* Read the file. Note that if there is value information following the rgb
         * data that this is stored in the lower_bound variable.
         */
        im = 0;
        rewind(fd);
        while(getvalidline(fd, line, 250, CONFIG_COMMENT_LINE_START))
        {
            (void) safe_strcpy(back, line);

            if (same_start_ic(line, "ramp"))
            {
                ptr = next_token(line);
                nread = sscanf(ptr, "%d%d%d%d%d%d%d%f%f", &nr, &r0, &g0, &b0, &rn, &gn, &bn, &v0, &vn);
                if(nread != 7 && nread != 9)
                {
                    pr_error(Module, "Incorrect number of values\n");
                    goto else_err;
                }

                /* Check for too many colour entries */
                if (im+nr > 256)
                {
                    pr_error(Module, "Too many colour entries\n");
                    goto else_err;
                }

                /* Build a colour ramp */
                for (ir=0; ir<nr; ir++)
                {
                    dr   = (float)ir / (float)nr;
                    rval = (int)((float)r0 + dr*(rn-r0));
                    gval = (int)((float)g0 + dr*(gn-g0));
                    bval = (int)((float)b0 + dr*(bn-b0));
                    lut->cells[im].red   = (UNCHAR)rval;
                    lut->cells[im].green = (UNCHAR)gval;
                    lut->cells[im].blue  = (UNCHAR)bval;
                    if(nread == 9)
                    {
                        lut->cells[im].lower_bound = v0 + dr*(vn-v0);
                        lut->cells[im].upper_bound = lut->cells[im].lower_bound;
                    }
                    im++;
                }
            }
            else if (same_start_ic(line, "transparent"))
            {
                ptr = next_token(line);
                if (sscanf(ptr, "%d", &nr) != 1) nr = 1;

                /* Check for too many colour entries */
                if (im+nr > 256)
                {
                    pr_error(Module, "Too many colour entries\n");
                    goto else_err;
                }

                /* Set block of transparent */
                for (ir=0; ir<nr; ir++)
                {
                    lut->cells[im].red   = T_RED;
                    lut->cells[im].green = T_GREEN;
                    lut->cells[im].blue  = T_BLUE;
                    im++;
                }
            }
            else
            {
                nread = sscanf(line, "%d%d%d%f", &rval, &gval, &bval, &v0);
                if(nread != 3 && nread != 4)
                {
                    pr_error(Module, "Incorrect number of values\n");
                    goto else_err;
                }

                /* Check for too many colour entries */
                if (im+1 > 256)
                {
                    pr_error(Module, "Too many colour entries\n");
                    goto else_err;
                }

                /* Set colour values */
                lut->cells[im].red   = (UNCHAR)rval;
                lut->cells[im].green = (UNCHAR)gval;
                lut->cells[im].blue  = (UNCHAR)bval;
                if(nread == 4)
                {
                    lut->cells[im].lower_bound = v0;
                    lut->cells[im].upper_bound = v0;
                }
                im++;
            }
            continue;
else_err:
            pr_error(Module, generr, path, back);
        }
    }
    (void) fclose(fd);
    return rtn;
}


/******************************************************************************/
/**
 *   @brief   Test if the given lut is for a data valued image.
 *
 *   @param[in]  lut   The lut to test.
 *
 *   @return   TRUE if a data lut, FALSE if not.
 */
/******************************************************************************/
LOGICAL glImageIsDataLUT( ImageLUT lut )
{
    return (lut < 0);
}


/******************************************************************************/
/**
     @brief  Return the lut information.
  
     @param[in]  lut   The lut the information is required for.
  
     @return   A pointer to a glLUT structure.
  
     @attention  The returned array must not be freed as it points to internal
                   static data.
  
       @note   The glLUT structure is defined as:
  
     @verbatim
      typedef struct {
          float   lower_bound;    lower bound of the range taken as >=
          float   upper_bound;    upper bound of the range taken as <
          UNCHAR  red;            red component of rgb assignment
          UNCHAR  green;          green component of rgb assignment
          UNCHAR  blue;           blue component of rgb assignment/
          UNCHAR  alpha;          alpha value for future consideration/
      } glLUTCOLOR;

      typedef struct {
          STRING     path;        path name of cmap file
          STRING     id;          identifier used to identify contents
          STRING     label;       data table label
          STRING     item;        data table item label
          int        ncells;      number of data-rgb specifications
          glLUTCOLOR *cells;      data-rgb lut specification
      } glLUT;
     @endverbatim        
*/
/******************************************************************************/
glLUT *glImageGetLUTInfo( ImageLUT lut )
{
	int ndx;

	if(lut == glNoLUT)
		return (glLUT *)0;

	if( lut > 0 )
		return (genlut + lut - 1);

	ndx = abs(lut);
	if( ndx <= nradar_data )
		return (&radar_data[ndx - 1].dlut);

	ndx -= SAT_LUT_OFFSET;
	if(  ndx > 0 && ndx <= ngrid_data )
		return (grid_data + ndx - 1);

	return (glLUT *)0;
}


/******************************************************************************/
/**
 *   @brief   Determine if the given lut is valid.
 *
 *   @param[in] lut   The lut to test.
 *
 *   @return   TRUE if the lut is valid, FALSE if not.
 */
/******************************************************************************/
LOGICAL glImageLUTisValid( ImageLUT lut )
{
	if(lut == glNoLUT) return TRUE;
	if(lut > 0)
	{
		if(lut <= ngenlut) return TRUE;
	}
	else if(lut < 0)
	{
		lut = abs(lut);
		if(lut <= nradar_data) return TRUE;
		lut -= SAT_LUT_OFFSET;
		if(lut > 0 && lut <= ngrid_data) return TRUE;
	}
	return FALSE;
}
