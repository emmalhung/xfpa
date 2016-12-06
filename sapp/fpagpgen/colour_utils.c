/***********************************************************************
*                                                                      *
*     c o l o u r _ u t i l s . c                                      *
*                                                                      *
*     Routines to handle colour conversions.                           *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#include "colour_utils.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <stdio.h>
#include <string.h>
#include <search.h>
#include <ctype.h>

typedef struct
	{
	int	red;
	int	green;
	int	blue;
	} RGB;

typedef struct
	{
	char	*string;
	RGB	rgb;
	} X11NAME;

/* Use the make_x11names_h.pl program to create a structure that holds the */
/*  RGB database, normally found in /usr/lib/X11/rgb.txt on HP-UX systems. */
/* This database is fairly static, so load it into memory at compile time. */
/* The structure is declared as "static X11NAME *x11_rgb_table".           */

#include "x11names.h"


/* Interface functions                      */
/*  ... these are defined in colour_utils.h */


/* Internal static functions */
static	void	load_x11name_hash_table(void);
static	STRING	no_blanks_lower(STRING);


/**********************************************************************
 ***                                                                ***
 *** c o n v e r t _ c m y k _ f o r _ p s m e t                    ***
 *** c o n v e r t _ r g b _ f o r _ p s m e t                      ***
 *** c o n v e r t _ x 1 1 _ f o r _ p s m e t                      ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		convert_cmyk_for_psmet

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	int		r, g, b;
	float	xr, xg, xb;
	char	tbuf[GPGMedium];

	/* Extract the CMYK parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d %d", &c, &m, &y, &k) ) return FALSE;

	/* Convert CMYK 0-100 to 0-255 */
	c = c * 255 / 100;
	m = m * 255 / 100;
	y = y * 255 / 100;
	k = k * 255 / 100;

	/* Convert CMYK to RGB */
	(void) cmyk_to_rgb( c , m , y , k , &r , &g , &b );

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Set RBG parameters for PSMet */
	xr = (float) r / 255.0;
	xg = (float) g / 255.0;
	xb = (float) b / 255.0;

	/* Reset the input buffer as RGB and return TRUE */
	(void) sprintf(tbuf, "%5.3f %5.3f %5.3f", xr, xg, xb);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_rgb_for_psmet

	(
	STRING	buf
	)

	{
	int		r, g, b;
	float	xr, xg, xb;
	char	tbuf[GPGMedium];

	/* Extract the RGB parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d", &r, &g, &b) ) return FALSE;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 100 || g > 100 || b > 100 ) return FALSE;

	/* Set RBG parameters for PSMet */
	xr = (float) r / 100.0;
	xg = (float) g / 100.0;
	xb = (float) b / 100.0;

	/* Reset the input buffer as RGB and return TRUE */
	(void) sprintf(tbuf, "%5.3f %5.3f %5.3f", xr, xg, xb);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_x11_for_psmet

	(
	STRING	buf
	)

	{
	int		r, g, b;
	float	xr, xg, xb;
	char	tbuf[GPGMedium];

	/* Convert x11 name to RBG */
	if ( !x11name_to_rgb( buf , &r , &g , &b ) ) return FALSE;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Set RBG parameters for PSMet */
	xr = (float) r / 255.0;
	xg = (float) g / 255.0;
	xb = (float) b / 255.0;

	/* Reset the input buffer as RGB and return TRUE */
	(void) sprintf(tbuf, "%5.3f %5.3f %5.3f", xr, xg, xb);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c o n v e r t _ c m y k _ f o r _ s v g m e t                  ***
 *** c o n v e r t _ r g b _ f o r _ s v g m e t                    ***
 *** c o n v e r t _ x 1 1 _ f o r _ s v g m e t                    ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		convert_cmyk_for_svgmet

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Extract the CMYK parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d %d", &c, &m, &y, &k) ) return FALSE;

	/* Convert CMYK 0-100 to 0-255 */
	c = c * 255 / 100;
	m = m * 255 / 100;
	y = y * 255 / 100;
	k = k * 255 / 100;

	/* Convert CMYK to RGB */
	(void) cmyk_to_rgb( c , m , y , k , &r , &g , &b );

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reset the input buffer to rgb(,,) style and return TRUE */
	(void) sprintf(tbuf, "rgb(%d,%d,%d)", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_rgb_for_svgmet

	(
	STRING	buf
	)

	{
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Extract the RGB parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d", &r, &g, &b) ) return FALSE;

	/* Convert RGB 0-100 to 0-255 */
	r = r * 255 / 100;
	g = g * 255 / 100;
	b = b * 255 / 100;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reset the input buffer to rgb(,,) style and return TRUE */
	(void) sprintf(tbuf, "rgb(%d,%d,%d)", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_x11_for_svgmet

	(
	STRING	buf
	)

	{
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Convert x11 name to RBG */
	if ( !x11name_to_rgb( buf , &r , &g , &b ) ) return FALSE;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reset the input buffer to rgb(,,) style and return TRUE */
	(void) sprintf(tbuf, "rgb(%d,%d,%d)", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c o n v e r t _ c m y k _ f o r _ r a d a r                    ***
 *** c o n v e r t _ r g b _ f o r _ r a d a r                      ***
 *** c o n v e r t _ x 1 1 _ f o r _ r a d a r                      ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		convert_cmyk_for_radar

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Extract the CMYK parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d %d", &c, &m, &y, &k) ) return FALSE;

	/* Convert CMYK 0-100 to 0-255 */
	c = c * 255 / 100;
	m = m * 255 / 100;
	y = y * 255 / 100;
	k = k * 255 / 100;

	/* Convert CMYK to RGB */
	(void) cmyk_to_rgb( c , m , y , k , &r , &g , &b );

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reformat for glib UNCHAR colour format */
	(void) sprintf(tbuf, "%d:%d:%d", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_rgb_for_radar

	(
	STRING	buf
	)

	{
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Extract the RGB parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d", &r, &g, &b) ) return FALSE;

	/* Convert RGB 0-100 to 0-255 */
	r = r * 255 / 100;
	g = g * 255 / 100;
	b = b * 255 / 100;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reformat for glib UNCHAR colour format */
	(void) sprintf(tbuf, "%d:%d:%d", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_x11_for_radar

	(
	STRING	buf
	)

	{
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Convert x11 name to RBG */
	if ( !x11name_to_rgb( buf , &r , &g , &b ) ) return FALSE;

	/* Check for parameters out of bounds */
	if ( r < 0 || g < 0 || b < 0 ) return FALSE;
	if ( r > 255 || g > 255 || b > 255 ) return FALSE;

	/* Reformat for glib UNCHAR colour format */
	(void) sprintf(tbuf, "%d:%d:%d", r, g, b);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c o n v e r t _ c m y k _ f o r _ c o r m e t                  ***
 *** c o n v e r t _ r g b _ f o r _ c o r m e t                    ***
 *** c o n v e r t _ x 1 1 _ f o r _ c o r m e t                    ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		convert_cmyk_for_cormet

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	char	tbuf[GPGMedium];

	/* Extract the CMYK parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d %d", &c, &m, &y, &k) ) return FALSE;

	/* Convert CMYK 0-100 to 0-255 */
	c = c * 255 / 100;
	m = m * 255 / 100;
	y = y * 255 / 100;
	k = k * 255 / 100;

	/* Check for parameters out of bounds */
	if ( c < 0 || m < 0 || y < 0 || k < 0 ) return FALSE;
	if ( c > 255 || m > 255 || y > 255 || k > 255 ) return FALSE;

	/* Reset the input buffer and return TRUE */
	(void) sprintf(tbuf, "%d %d %d %d", c, m, y, k);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_rgb_for_cormet

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	int		r, g, b;
	char	tbuf[GPGMedium];

	/* Extract the RGB parameters from the input buffer */
	if ( !sscanf(buf, "%d %d %d", &r, &g, &b) ) return FALSE;

	/* Convert RGB 0-100 to 0-255 */
	r = r * 255 / 100;
	g = g * 255 / 100;
	b = b * 255 / 100;

	/* Convert RGB to CMYK */
	(void) rgb_to_cmyk( r , g , b , &c , &m , &y , &k );

	/* Check for parameters out of bounds */
	if ( c < 0 || m < 0 || y < 0 || k < 0 ) return FALSE;
	if ( c > 255 || m > 255 || y > 255 || k > 255 ) return FALSE;

	/* Reset the input buffer as CMYK and return TRUE */
	(void) sprintf(tbuf, "%d %d %d %d", c, m, y, k);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

LOGICAL		convert_x11_for_cormet

	(
	STRING	buf
	)

	{
	int		c, m, y, k;
	char	tbuf[GPGMedium];

	/* Convert x11 name to CMYK */
	if ( !x11name_to_cmyk( buf , &c , &m , &y , &k ) ) return FALSE;

	/* Check for parameters out of bounds */
	if ( c < 0 || m < 0 || y < 0 || k < 0 ) return FALSE;
	if ( c > 255 || m > 255 || y > 255 || k > 255 ) return FALSE;

	/* Reset the input buffer as CMYK and return TRUE */
	(void) sprintf(tbuf, "%d %d %d %d", c, m, y, k);
	(void) strcpy(buf, tbuf);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** x 1 1 n a m e _ t o _ c m y k                                  ***
 ***                                                                ***
 *** first lookup the (Red,Green,Blue) components of a colour from  ***
 ***  a hash table constructed from /usr/lib/X11/rgb.txt.  Then     ***
 ***  convert to (Cyan,Magenta,Yellow,Black).                       ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		x11name_to_cmyk

	(
	STRING	colour,
	int		*c,
	int		*m,
	int		*y,
	int		*k
	)

	{
	int		r, g, b;

	if ( !x11name_to_rgb( colour, &r, &g, &b ) )
		return FALSE;
	else
		{
		rgb_to_cmyk( r, g, b, c, m, y, k );
		return TRUE;
		}
	}

/**********************************************************************
 ***                                                                ***
 *** x 1 1 n a m e _ t o _ r g b                                    ***
 ***                                                                ***
 *** lookup the (Red,Green,Blue) components of a colour from a hash ***
 ***  table constructed from /usr/lib/X11/rgb.txt.                  ***
 ***                                                                ***
 **********************************************************************/

LOGICAL		x11name_to_rgb

	(
	STRING	colour,
	int		*r,
	int		*g,
	int		*b
	)

	{
	ENTRY	item, *found_item, *hsearch();

	static	LOGICAL		x11name_hash_table_loaded = FALSE;

	/* No need to re-load each time. */
	if ( !x11name_hash_table_loaded )
		{
		(void) load_x11name_hash_table();
		x11name_hash_table_loaded = TRUE;
		}

	item.key = no_blanks_lower( colour );
	if ( (found_item = hsearch(item,FIND) ) != NULL )
		{
		*r = ((RGB *)found_item->data)->red;
		*g = ((RGB *)found_item->data)->green;
		*b = ((RGB *)found_item->data)->blue;
		return TRUE;
		}
	else
		return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** c m y k _ t o _ r g b                                          ***
 ***                                                                ***
 *** convert from cmyk (Cyan, Magenta, Yellow, Black)               ***
 ***  to rgb (Red, Green, Blue).  The formula is from               ***
 ***  David Bourgin's Colour Spaces FAQ (comp.graphics).            ***
 ***                                                                ***
 **********************************************************************/

void		cmyk_to_rgb

	(
	int		c,
	int		m,
	int		y,
	int		k,
	int		*r,
	int		*g,
	int		*b
	)

	{
	int		rr, gg, bb;
	float	white;

	if ( k == 255 )
		{
		*r = *g = *b = 0;
		}
	else
		{
		white = (255. - (float)k) / 255.;
		rr = (c * white) + k;
		gg = (m * white) + k;
		bb = (y * white) + k;
		*r = (int) ( 255. - MIN(255., rr));
		*g = (int) ( 255. - MIN(255., gg));
		*b = (int) ( 255. - MIN(255., bb));
		}
	}

/**********************************************************************
 ***                                                                ***
 *** r g b _ t o _ c m y k                                          ***
 ***                                                                ***
 *** convert from rgb (Red, Green, Blue)                            ***
 ***  to cmyk (Cyan, Magenta, Yellow, Black)  The formula is from   ***
 ***  David Bourgin's Colour Spaces FAQ (comp.graphics).            ***
 ***                                                                ***
 **********************************************************************/

void		rgb_to_cmyk

	(
	int		r,
	int		g,
	int		b,
	int		*c,
	int		*m,
	int		*y,
	int		*k
	)

	{
	int		cc, mm, yy;
	float	white;

	cc = 255 - r;
	mm = 255 - g;
	yy = 255 - b;
	*k = MIN(MIN(cc, mm), yy);   /* black */
	if ( *k == 255 )
		{
		*c = *m = *y = 0;
		}
	else
		{
		white = (255. - (float)*k) / 255.;
		*c = (int) ( (cc - *k) / white );
		*m = (int) ( (mm - *k) / white );
		*y = (int) ( (yy - *k) / white );
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES                                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this source file.                                                *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** l o a d _ x 1 1 n a m e _ h a s h _ t a b l e                  ***
 ***                                                                ***
 *** load the data stored in table (see x11names.h) into a hash     ***
 ***  table for quicker searching later.                            ***
 ***                                                                ***
 **********************************************************************/

static	void	load_x11name_hash_table

	(
	)

	{
	int		i;
	RGB		*info_ptr;
	ENTRY	item, *hsearch();

	(void) hcreate(X11NAME_NUM_ENTRIES);
	for (i=0; i < X11NAME_NUM_ENTRIES; i++)
		{
		item.key = x11_rgb_table[i].string;
		info_ptr = &x11_rgb_table[i].rgb;
		item.data = (char *)info_ptr;
		(void) hsearch( item , ENTER );
		}
	}

/**********************************************************************
 ***                                                                ***
 *** n o _ b l a n k s _ l o w e r                                  ***
 ***                                                                ***
 *** return pointer to a string with all whitespace removed and all ***
 ***  chars converted to lowercase.                                 ***
 ***                                                                ***
 **********************************************************************/

static	STRING	no_blanks_lower

	(
	STRING		str
	)

	{
	static	char	s[80];
	STRING	p;

	str[79] = '\0';     /* truncate really long names */
	(void) strcpy( s , str );
	p = s;

	while ( *p != '\0' )
		{
		if ( isupper(*p) )
			*p = tolower(*p);
		else if ( !islower(*p) && !isdigit(*p) )
			{
			(void) strcpy( p , p+1 );  /* overwrite char at location p */
			p--;
			}
		p++;
		}

	return s;
	}

/***********************************************************************
*                                                                      *
*      Test programs:                                                  *
*                                                                      *
***********************************************************************/

#ifdef COLOUR_STANDALONE

main()
	{
	char	sbuf[80];
	int		len;
	int		r, g, b, c, m, y, k;

	while (1)
		{
		printf( "Enter a colour: " );
		fgets( sbuf, 80 , stdin );
		len = strcspn( sbuf , "\n" );
		sbuf[len] = NULL;
		if ( strcmp(sbuf,"") == 0 )
			{
			(void) hdestroy();
			return;
			}

		if ( x11name_to_rgb( sbuf , &r , &g , &b ) )
			printf( "RGB for %s is %d %d %d\n", sbuf,r,g,b );
		else
			printf( "Can't find RGB for %s\n", sbuf );

		if ( x11name_to_cmyk( sbuf , &c , &m , &y , &k ) )
			printf( "CMYK for %s is %d %d %d %d\n", sbuf,c,m,y,k );
		else
			printf( "Can't find CMYK for %s\n", sbuf );
		}
	}

#endif /* COLOUR_STANDALONE */
