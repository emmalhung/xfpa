/**********************************************************************/
/*
*   File:     glib_gif.c
*
*   Purpose:  Functions to read and write GIF files.
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
/**********************************************************************/
#include <sys/stat.h>
#include <tools/tools.h>
#include "glib_private.h"


/*************************  Read GIF Format Files ************************/


#define NEXTBYTE      (*dataptr++)
#define SKIPBYTES(n)  dataptr+=(n)
#define EXTENSION     0x21
#define IMAGESEP      0x2c
#define TRAILER       0x3b
#define INTERLACEMASK 0x40
#define COLORMAPMASK  0x80

  
static STRING Module = "GIFDecode";

static int	BitOffset = 0,		/* Bit Offset of next code */
			XC = 0, YC = 0,		/* Output X and Y coords of current pixel */
			Pass = 0,			/* Used by output routine if interlaced pic */
			OutCount = 0,		/* Decompressor output 'stack count' */
			Width, Height,		/* image dimensions */
			BitsPerPixel,		/* Bits per pixel, read from GIF header */
			ColorMapSize,		/* number of colors */
			CodeSize,			/* Code size, read from GIF header */
			Code,				/* Value returned by ReadCode */
			InitCodeSize,		/* Starting code size, used during Clear */
			MaxCode,			/* limiting value for current code size */
			ClearCode,			/* GIF clear code */
			EOFCode,			/* GIF end-of-information code */
			CurCode, OldCode, InCode,	/* Decompressor variables */
			FirstFree,			/* First free code, generated per GIF spec */
			FreeCode,			/* Decompressor,next free slot in hash table */
			FinChar,			/* Decompressor variable */
			BitMask,			/* AND mask for data size */
			ReadMask,			/* Code AND mask for current code size */
			Misc;			   /* miscellaneous bits (interlace, local cmap)*/


static LOGICAL Interlace, HasColormap;

static UNCHAR *RawGIF;			/* The heap array to hold it, raw */
static UNCHAR *Raster;			/* The raster data stream, unblocked */
static UNCHAR *pic8;

/* The hash table used by the decompressor */
static int *Prefix = NULL;
static int *Suffix = NULL;

/* An output array used by the decompressor */
static int *OutCode = NULL;

static int   gif89 = 0;
static char *id87  = "GIF87a";
static char *id89  = "GIF89a";

static LOGICAL   readImage   (UNCHAR*, UNCHAR*, UNCHAR*);
static int       readCode    (void);
static void      doInterlace (int);
static LOGICAL   gifError    (FILE *, char *);
static void      gifWarning  (char *);

static int     filesize;
static char    *bname;
static UNCHAR  *dataptr;
static STRING  comment;




LOGICAL _xgl_isGIF(STRING fname)
{
	char    buf[7];
	LOGICAL rtn = TRUE;
	FILE    *fp = fopen(fname, BINARY_READ);

	if(!fp) return FALSE;
	buf[6] = '\0';
	if( fread(buf,6,1,fp) != 1 ) rtn = FALSE;
	(void) fclose(fp);
	if( !same_ic(buf,"GIF87a") && !same_ic(buf,"GIF89a") ) rtn = FALSE;
	return rtn;
}


/* Find the size and number of bands required for a GIF file and return any comment.
*  Note that this procedure will return information only for the first image found
*  in the file and will ignore multiple image data.
*/
LOGICAL _xgl_queryGIF(STRING fname, int *w, int *h, int *bands, glCOLOR *cmap, int *t, char **comm)
{
	register UNCHAR ch;
	register int    i, block;
	int		        band, gotimage, sbsize, transparent = -1;
	UNCHAR          r[256], g[256], b[256];
	FILE            *fp;

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	/* Initialize parameters
	*/
	if (w)    *w = 0;
	if (h)    *h = 0;
	if (comm) *comm   = NULL;
	band = 1;

	/* Initialize
	*/
	for(i = 0; i < 256; i++) r[i] = g[i] = b[i] = 0;
	gotimage  = 0;
	gif89     = 0;
	comment   = (char *) NULL;

	/* find the size of the file
	*/
	(void) fseek(fp, 0L, SEEK_END);
	filesize = (int) ftell(fp);
	(void) fseek(fp, 0L, SEEK_SET);
	
	/* the +256 allows reading truncated GIF files without fear of segmentation violation
	*/
	dataptr = RawGIF = INITMEM(UNCHAR, filesize+256);
	if (!dataptr) return( gifError(fp, "not enough memory to read gif file") );
	
	if (fread(dataptr, (size_t) filesize, (size_t) 1, fp) != 1) 
	  return( gifError(fp, "GIF data read failed") );

	if      (same_start((char *) dataptr, id87)) gif89 = 0;
	else if (same_start((char *) dataptr, id89)) gif89 = 1;
	else    return( gifError(fp, "not a GIF file"));

	SKIPBYTES(10);
	
	ch = NEXTBYTE;
	HasColormap  = (ch & COLORMAPMASK) ? TRUE : FALSE;
	BitsPerPixel = (ch & 7) + 1;
	ColorMapSize = 1 << BitsPerPixel;
	
	SKIPBYTES(2);
	
	/* Read in global colormap.
	*/
	if (HasColormap)
	{
		for (i=0; i<ColorMapSize; i++)
		{
			r[i] = NEXTBYTE;
			g[i] = NEXTBYTE;
			b[i] = NEXTBYTE;
		}
	}

	while (1)
	{
		block = NEXTBYTE;

		if (block == EXTENSION)
		{
			ch = NEXTBYTE;
			if(ch == 0xF9)	/* Graphic Control Extension */
			{
				if( (sbsize = NEXTBYTE) == 4 )
				{
					int n, j;
					n = NEXTBYTE; NEXTBYTE; NEXTBYTE; j = NEXTBYTE;
					if ((n & 0x1) != 0) transparent = j;
				}
				else
				{
					SKIPBYTES(sbsize);
				}
				/* ignore remaining data sub-blocks */
				while((sbsize = NEXTBYTE) > 0) SKIPBYTES(sbsize);
			}
			else if (ch == 0xFE)		/* Comment Extension */
			{		
				int	 j, cmtlen;
				UNCHAR *ptr1, *cmt, *cmt1, *sp;

				cmtlen = 0;
				ptr1   = dataptr;					/* remember start of comments */

				/* figure out length of comment */
				while((sbsize = NEXTBYTE) > 0)
				{
					cmtlen += sbsize;
					SKIPBYTES(sbsize);
				}

				if (cmtlen>0)	/* build into one un-blocked comment */
				{
					cmt = INITMEM(UNCHAR, cmtlen + 1);
					if (!cmt) gifWarning("couldn't allocate memory for comments\n");
					else
					{
		  				sp = cmt;
		  				while((sbsize = (*ptr1++)) > 0)
		  				{
							for (j=0; j<sbsize; j++, sp++, ptr1++) *sp = *ptr1;
		 				}
						*sp = 0;

		  				if (comment)	/* have to strcat onto old comments */
		  				{
							cmt1 = INITMEM(UNCHAR, safe_strlen(comment) + cmtlen + 2);
							if (!cmt1)
							{
								gifWarning("couldn't allocate memory for comments\n");
								free((void*)cmt);
							}
							else
							{
								(void) safe_strcpy((char *) cmt1, (char *) comment);
								(void) safe_strcat((char *) cmt1, (char *) "\n");
								(void) safe_strcat((char *) cmt1, (char *) cmt);
								free((void*)comment);
								free((void*)cmt);
								comment = (char *) cmt1;
							}
		  				}
		  				else
		  				{
		  					comment = (char *) cmt;
		  				}
					}
				}
			}
			else	/* Ignore all other extensions */
			{ 
				while((sbsize = NEXTBYTE) > 0) SKIPBYTES(sbsize);
			}
		}
		else if (block == IMAGESEP)
		{
			if (gotimage)	/* ignore remaining images */
			{
				int misc;
				SKIPBYTES(8);										/* Image header */
				misc = NEXTBYTE;									/* misc. bits */
				if (misc & COLORMAPMASK) SKIPBYTES(3*(1 << ((misc&7)+1)));	/* Local colormap */
			}
			else
			{
				gotimage = 1;
				SKIPBYTES(4);	/* Offsets - Ignore */
				ch = NEXTBYTE;
				Width = ch + 0x100 * NEXTBYTE;
				ch = NEXTBYTE;
				Height = ch + 0x100 * NEXTBYTE;
				Misc = NEXTBYTE;
				if (Misc & COLORMAPMASK)
				{
					for (i=0; i< 1 << ((Misc&7)+1); i++)
					{
						r[i] = NEXTBYTE;
						g[i] = NEXTBYTE;
						b[i] = NEXTBYTE;
					}
				}
			}
			/* Skip over image data
			*/
			NEXTBYTE;	/* minimum code size */
			while((sbsize = NEXTBYTE) > 0) SKIPBYTES(sbsize);
	  	}
		else if (block == TRAILER)	/* stop reading blocks */
		{
			break;
	  	}
		else	/* unknown block type - output error only if file truncated and no image found */
		{
			if ((dataptr - RawGIF) < filesize)
			{
				char str[128];
				(void) snprintf(str, sizeof(str), "Unknown block type (0x%02x) at offset 0x%lx",
					(unsigned int) block, (unsigned long)((dataptr - RawGIF) - 1));
				if (!gotimage) return gifError(fp, str);
			}
			break;
		}
	}
	FREEMEM(RawGIF);

	if (!gotimage) return( gifError(fp, "no image data found in GIF file") );

	if (w)    *w    = Width;
	if (h)    *h    = Height;
	if (comm) *comm = comment;
	else      FREEMEM(comment);

	if (t) *t = transparent;

	if(cmap)
	{
		for(i = 0; i < 256; i++)
		{
			cmap[i].red   = r[i];
			cmap[i].green = g[i];
			cmap[i].blue  = b[i];
			if(r[i] != g[i] || r[i] != b[i]) band = 3;
		}
	}
	if (bands) *bands = band;

	(void) fclose(fp);

	return TRUE;
}


LOGICAL _xgl_readGIF( ImagePtr im, UNCHAR **image )
{
	register UNCHAR ch;
	register int    i, block, sbsize;
	int		        gotimage;
	UNCHAR          r[256], g[256], b[256];
	FILE            *fp;

	Module = "GIFDecode";

	fp = _xgl_open_image_file(im);
	if (!fp) return FALSE;

	/* initialize variables */
	BitOffset = XC = YC = Pass = OutCount = gotimage = 0;
	RawGIF = Raster = pic8 = NULL;
	gif89 = 0;

	Prefix  = INITMEM(int, 4096);
	Suffix  = INITMEM(int, 4096);
	OutCode = INITMEM(int, 4097);

	/* find the size of the file */
	(void) fseek(fp, 0L, SEEK_END);
	filesize = (int) ftell(fp);
	(void) fseek(fp, 0L, SEEK_SET);
	
	/* the +256's are so we can read truncated GIF files without fear of 
	   segmentation violation */
	if (!(dataptr = RawGIF = INITMEM(UNCHAR, filesize+256)))
	  return( gifError(fp, "not enough memory to read gif file") );
	
	if (!(Raster = INITMEM(UNCHAR, filesize+256))) 
	  return( gifError(fp, "not enough memory to read gif file") );
	
	if (fread(dataptr, (size_t) filesize, (size_t) 1, fp) != 1) 
	  return( gifError(fp, "GIF data read failed") );


	if      (same_start((char *) dataptr, id87)) gif89 = 0;
	else if (same_start((char *) dataptr, id89)) gif89 = 1;
	else    return( gifError(fp, "not a GIF file"));
	
	SKIPBYTES(10);
	
	ch = NEXTBYTE;
	HasColormap  = (ch & COLORMAPMASK) ? TRUE : FALSE;
	BitsPerPixel = (ch & 7) + 1;
	ColorMapSize = 1 << BitsPerPixel;
	BitMask      = ColorMapSize - 1;
	
	SKIPBYTES(2);
	
	if (HasColormap)
	{
		for (i=0; i<ColorMapSize; i++)
			SKIPBYTES(3);
	}
	
	while (1)
	{
		block = NEXTBYTE;

		if (block == EXTENSION)
		{
			(void) NEXTBYTE;
			while((sbsize = NEXTBYTE) > 0) SKIPBYTES(sbsize);
		}
		else if (block == IMAGESEP)
		{
			if (gotimage)
			{   /* just skip over remaining images */
				int misc;

				/* skip image header */
				SKIPBYTES(8);
				misc = NEXTBYTE;
				if (misc & COLORMAPMASK) SKIPBYTES(3*(1 <<((misc&7)+1)));
				NEXTBYTE;			 /* minimum code size */

				/* skip image data sub-blocks */
				while((sbsize = NEXTBYTE) > 0)
				{
					SKIPBYTES(sbsize);
					if ((dataptr - RawGIF) > filesize) break;		  /* EOF */
				}
			}
			else
			{
				if (readImage(r, g, b)) gotimage = 1;
			}
	  	}
		else if (block == TRAILER)
		{		  /* stop reading blocks */
			break;
	  	}
		else
		{		  /* unknown block type */
			char str[128];

			/* don't mention bad block if file was trunc'd, as it's all bogus */
			if ((dataptr - RawGIF) < filesize)
			{
				(void) snprintf(str, sizeof(str), "Unknown block type (0x%02x) at offset 0x%lx",
					(unsigned int) block, (unsigned long)((dataptr - RawGIF) - 1));

				if (!gotimage) return gifError(fp, str);
				else gifWarning(str);
			}
			break;
		}
	}

	FREEMEM(RawGIF);
	FREEMEM(Raster);

	if (!gotimage) return( gifError(fp, "no image data found in GIF file") );

	FREEMEM(Prefix);
	FREEMEM(Suffix);
	FREEMEM(OutCode);

	(void) fclose(fp);

	*image = pic8;

	return TRUE;
}


static LOGICAL readImage(UNCHAR *r, UNCHAR *g, UNCHAR *b)
{
	register UNCHAR ch, ch1, *ptr1, *picptr;
	int		       i, npixels, maxpixels;

	npixels = maxpixels = 0;

	/* read in values from the image descriptor */
	
	SKIPBYTES(4);
	ch = NEXTBYTE;
	Width   = ch + 0x100 * NEXTBYTE;
	ch = NEXTBYTE;
	Height  = ch + 0x100 * NEXTBYTE;

	Misc = NEXTBYTE;
	Interlace = ((Misc & INTERLACEMASK) ? TRUE : FALSE);

	if (Misc & COLORMAPMASK)
	{
		for (i=0; i< 1 << ((Misc&7)+1); i++)
		{
			r[i] = NEXTBYTE;
			g[i] = NEXTBYTE;
			b[i] = NEXTBYTE;
		}
	}

	if (!HasColormap && !(Misc & COLORMAPMASK))
	{
		/* no global or local colormap */
		pr_warning(Module, "%s:  %s", bname, "No colormap in this GIF file.  Assuming Greyscale.\n");
	}
	  
	/* Start reading the raster data. First we get the intial code size
	 * and compute decompressor constant values, based on this code size.
	 */
	
	CodeSize = NEXTBYTE;

	ClearCode = (1 << CodeSize);
	EOFCode = ClearCode + 1;
	FreeCode = FirstFree = ClearCode + 2;
	
	/* The GIF spec has it that the code size is the code size used to
	 * compute the above values is the code size given in the file, but the
	 * code size used in compression/decompression is the code size given in
	 * the file plus one. (thus the ++).
	 */
	
	CodeSize++;
	InitCodeSize = CodeSize;
	MaxCode = (1 << CodeSize);
	ReadMask = MaxCode - 1;
	
	/* UNBLOCK:
	 * Read the raster data.  Here we just transpose it from the GIF array
	 * to the Raster array, turning it from a series of blocks into one long
	 * data stream, which makes life much easier for readCode().
	 */
	
	ptr1 = Raster;
	do
	{
	  ch = ch1 = NEXTBYTE;
	  while (ch--) { *ptr1 = NEXTBYTE; ptr1++; }
	  if ((dataptr - RawGIF) > filesize)
	  {
			pr_warning(Module,"%s:  %s", bname,"This GIF file seems to be truncated.  Winging it.\n");
			break;
	  }
	} while(ch1);

	/* Allocate the 'pic' */
	maxpixels = Width*Height;
	picptr = pic8 = INITMEM(UNCHAR, maxpixels);
	if (!pic8) return( gifError(NULL, "couldn't allocate memory for 'pic8'") );

	/* Decompress the file, continuing until you see the GIF EOF code.
	 * One obvious enhancement is to add checking for corrupt files here.
	 */
	
	Code = readCode();
	while (Code != EOFCode)
	{
	  /* Clear code sets everything back to its initial value, then reads the
	   * immediately subsequent code as uncompressed data.
	   */

		if (Code == ClearCode)
		{
			CodeSize = InitCodeSize;
			MaxCode = (1 << CodeSize);
			ReadMask = MaxCode - 1;
			FreeCode = FirstFree;
			Code = readCode();
			CurCode = OldCode = Code;
			FinChar = CurCode & BitMask;
			if (!Interlace) *picptr++ = (UNCHAR) FinChar;
			   else doInterlace(FinChar);
			npixels++;
		}
		else
		{
			/* If not a clear code, must be data: save same as CurCode and InCode */

			/* if we're at maxcode and didn't get a clear, stop loading */
			if (FreeCode>=4096) { /* printf("freecode blew up\n"); */
				  break; }

			CurCode = InCode = Code;
			
			/* If greater or equal to FreeCode, not in the hash table yet;
			 * repeat the last character decoded
			 */
			
			if (CurCode >= FreeCode)
			{
				CurCode = OldCode;
				if (OutCount > 4096) {	/* printf("outcount1 blew up\n"); */ break; }
				OutCode[OutCount++] = FinChar;
			}
			
			/* Unless this code is raw data, pursue the chain pointed to by CurCode
			 * through the hash table to its end; each code in the chain puts its
			 * associated output code on the output queue.
			 */
			
			while (CurCode > BitMask)
			{
				if (OutCount > 4096) break;	 /* corrupt file */
				OutCode[OutCount++] = Suffix[CurCode];
				CurCode = Prefix[CurCode];
			}
			
			if (OutCount > 4096) { /* printf("outcount blew up\n"); */ break; }
			
			/* The last code in the chain is treated as raw data. */
			
			FinChar = CurCode & BitMask;
			OutCode[OutCount++] = FinChar;
			
			/* Now we put the data out to the Output routine.
			 * It's been stacked LIFO, so deal with it that way...
			 */

			/* safety thing:  prevent exceeding range of 'pic8' */
			if (npixels + OutCount > maxpixels) OutCount = maxpixels-npixels;
	
			npixels += OutCount;
			if (!Interlace) for (i=OutCount-1; i>=0; i--) *picptr++ = (UNCHAR) OutCode[i];
			          else  for (i=OutCount-1; i>=0; i--) doInterlace(OutCode[i]);
			OutCount = 0;

			/* Build the hash table on-the-fly. No table is stored in the file. */
			
			Prefix[FreeCode] = OldCode;
			Suffix[FreeCode] = FinChar;
			OldCode = InCode;
			
			/* Point to the next slot in the table.  If we exceed the current
			 * MaxCode value, increment the code size unless it's already 12.  If it
			 * is, do nothing: the next code decompressed better be CLEAR
			 */
			
			FreeCode++;
			if (FreeCode >= MaxCode)
			{
				if (CodeSize < 12)
				{
					CodeSize++;
					MaxCode *= 2;
					ReadMask = (1 << CodeSize) - 1;
				}
			}
		}
		Code = readCode();
		if (npixels >= maxpixels) break;
	}
	
	if (npixels != maxpixels)
	{
	  pr_warning(Module,"%s:  %s", bname, "This GIF file seems to be truncated.  Winging it.\n");
	  if (!Interlace)  /* clear->EOBuffer */
			(void) memset((char *)pic8+npixels, 0, (size_t) (maxpixels-npixels));
	}

	/* comment gets handled in main LoadGIF() block-reader */

	return TRUE;
}


/* Fetch the next code from the raster data stream.	The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.	We compute
 * the byte Offset into the raster array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it. 
 */

static int readCode(void)
{
	int RawCode, UNCHAROffset;
	
	UNCHAROffset = BitOffset / 8;
	RawCode = Raster[UNCHAROffset] + (Raster[UNCHAROffset + 1] << 8);
	if (CodeSize >= 8)
	  RawCode += ( ((int) Raster[UNCHAROffset + 2]) << 16);
	RawCode >>= (BitOffset % 8);
	BitOffset += CodeSize;

	return(RawCode & ReadMask);
}


static void doInterlace(int Index)
{
	static UNCHAR *ptr = NULL;
	static int   oldYC = -1;
	
	if (oldYC != YC) {  ptr = pic8 + YC * Width;  oldYC = YC; }
	
	if (YC<Height)
	  *ptr++ = (UNCHAR) Index;
	
	/* Update the X-coordinate, and if it overflows, update the Y-coordinate */
	
	if (++XC == Width)
	{  
	  /* deal with the interlace as described in the GIF
	   * spec.  Put the decoded scan line out to the screen if we haven't gone
	   * past the bottom of it
	   */
	  
	  XC = 0;
	  
	  switch (Pass) {
	  case 0:
			YC += 8;
			if (YC >= Height) { Pass++; YC = 4; }
			break;
			
	  case 1:
			YC += 8;
			if (YC >= Height) { Pass++; YC = 2; }
			break;
			
	  case 2:
			YC += 4;
			if (YC >= Height) { Pass++; YC = 1; }
			break;
			
	  case 3:
			YC += 2;  break;
			
	  default:
			break;
	  }
	}
}

			
static LOGICAL gifError(FILE *fp, char *st)
{
	pr_error("GifDecode Error","%s\n", st);

	FREEMEM(Prefix);
	FREEMEM(Suffix);
	FREEMEM(OutCode);
	FREEMEM(RawGIF);
	FREEMEM(Raster);
	FREEMEM(comment);
	FREEMEM(pic8);

	if (fp) (void) fclose(fp);

	return FALSE;
}


static void gifWarning(char *st)
{
	pr_error("GifDecode Warning", "%s\n", st);
}


/*********************** Write GIF Format File ***********************/


static void putword		(int, FILE *);
static void compress	(int, FILE *, int, int, UNCHAR*, UNCHAR *);
static void output		(int);
static void cl_block	(void);
static void cl_hash		(long int);
static void char_init	(void);
static void char_out	(int);
static void flush_char	(void);


void _xgl_writeGIF( char *fname, int width, int height, UNCHAR *image, UNCHAR *r, UNCHAR *g, UNCHAR *b, int nrgb, char *comm )
{
	int     RWidth, RHeight;
	int     LeftOfs, TopOfs;
	int     Background;
	int     i,j,nc;
	UNCHAR    r1[256], g1[256], b1[256];
	UNCHAR    pc2nc[256];
	FILE    *fp = NULL;

	Module = "WriteGIF";

	fp = fopen(fname, "w");
	if(!fp)
	{
		pr_error(Module, "Cannot open file \"%s\" for write.\n", fname);
		return;
	}

	Background = 0;

	for(i = 0; i < 256; i++)
	{
		pc2nc[i] = r1[i] = g1[i] = b1[i] = 0;
	}

	/* If we do not have a colormap assume that the image is in 8 bits per pixel format.
	*  Otherwise we find out how many colours we actually have.
	*/
	if(nrgb < 1)
	{
		BitsPerPixel = 8;
		ColorMapSize = 0;
		for(i = 0; i < 256; i++) pc2nc[i] = (UNCHAR) i;
	}
	else
	{
		/* compute number of unique colors
		*/
		for(nc = 0, i = 0; i < nrgb; i++)
		{
			/* see if color #i is already used
			*/
			for (j = 0; j < i; j++)
			{
				if (r[i] == r[j] && g[i] == g[j] && b[i] == b[j]) break;
			}

			if( j == i )
			{	/* wasn't found */
				pc2nc[i] = (UNCHAR) nc;
				r1[nc]   = r[i];
				g1[nc]   = g[i];
				b1[nc]   = b[i];
				nc++;
			}
			else
			{
				pc2nc[i] = pc2nc[j];
			}
		}
		/* figure out 'BitsPerPixel'
		*/
		for(i = 1; i < 8; i++) if ( (1<<i) >= nc) break;	
		BitsPerPixel = i;
		ColorMapSize = 1 << BitsPerPixel;
	}
		
	
	RWidth  = Width  = width;
	RHeight = Height = height;
	LeftOfs = TopOfs = 0;
	
	InitCodeSize = (BitsPerPixel <= 1) ? 2 : BitsPerPixel;

	if(comm && safe_strlen(comm) > 0)
		(void) fwrite("GIF89a", 1, 6, fp);    /* the GIF magic number */
	else
		(void) fwrite("GIF87a", 1, 6, fp);    /* the GIF magic number */

	putword(RWidth, fp);           /* screen descriptor */
	putword(RHeight, fp);

	i = (ColorMapSize) ? COLORMAPMASK : 0; /* Is there is a color map */
	i |= (8-1)<<4;                 /* OR in the color resolution (hardwired 8) */
	i |= (BitsPerPixel - 1);       /* OR in the # of bits per pixel */
	(void) fputc(i,fp);          

	(void) fputc(Background, fp);         /* background color */

	(void) fputc(0, fp);                  /* future expansion byte */

	for(i = 0; i < ColorMapSize; i++) /* write out Global colormap */
	{
		(void) fputc((int)r1[i], fp);
		(void) fputc((int)g1[i], fp);
		(void) fputc((int)b1[i], fp);
	}

	if (comm && safe_strlen(comm)>0)  /* write comment blocks */
	{
		char *s;
		int   blen;

		(void) fputc(0x21, fp);     /* EXTENSION block */
		(void) fputc(0xFE, fp);     /* comment extension */

		s = comm;
		while ((blen = (int) safe_strlen(s)) > 0)
		{
			if(blen > 255) blen = 255;
			(void) fputc(blen, fp);
			for(i = 0; i < blen; i++, s++) (void) fputc((int)*s, fp);
		}
		(void) fputc(0, fp);    /* zero-length data subblock to end extension */
	}


	(void) fputc( (int)',', fp );              /* image separator */

	/* Write the Image header */
	putword(LeftOfs, fp);
	putword(TopOfs,  fp);
	putword(Width,   fp);
	putword(Height,  fp);

	(void) fputc(0x00, fp);

	(void) fputc(InitCodeSize, fp);
	compress(InitCodeSize+1, fp, Height, Width, image, pc2nc);

	(void) fputc(0,fp);                      /* Write out a Zero-length packet (EOF) */
	(void) fputc((int)';',fp);                    /* Write GIF file terminator */

	(void) fclose(fp);
	return;
}

/* writes a 16-bit integer in GIF order (LSB first)
*/
static void putword(int w , FILE *fp )
{
	(void) fputc(w       % 256, fp);
	(void) fputc((w/256) % 256, fp);
}


static UNLONG cur_accum = 0;
static int   cur_bits = 0;

#define min(a,b)        ((a>b) ? b : a)

#define XV_BITS	12    /* BITS was already defined on some systems */
#define MSDOS	1

#define HSIZE  5003            /* 80% occupancy */


static int n_bits;                    /* number of bits/code */
static int maxbits = XV_BITS;         /* user settable max # bits/code */
static int maxcode;                   /* maximum code, given n_bits */
static int maxmaxcode = 1 << XV_BITS; /* NEVER generate this */

#define MAXCODE(n_bits)     ( (1 << (n_bits)) - 1)

static  long   htab [HSIZE];
static  UNSHORT codetab [HSIZE];
#define HashTabOf(i)   htab[i]
#define CodeTabOf(i)   codetab[i]

static int hsize = HSIZE;            /* for dynamic table sizing */

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i) ((UNCHAR *)(htab))[i]
#define de_stack        ((UNCHAR *)&tab_suffixof(1<<XV_BITS))

static int free_ent = 0;                  /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static long int in_count = 1;            /* length of input */
static long int out_count = 0;           /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.
 */

static int g_init_bits;
static FILE *g_outfile;


static void compress(int  init_bits , FILE *outfile , int width, int height, UNCHAR *image, UNCHAR *pc2nc )
{
	int x, y, s, w;

	register long fcode;
	register int i = 0;
	register int c;
	register int ent;
	register int disp;
	register int hsize_reg;
	register int hshift;

	/*
	 * Set up the globals:  g_init_bits - initial number of bits
	 *                      g_outfile   - pointer to output file
	 */
	g_init_bits = init_bits;
	g_outfile   = outfile;

	/* initialize 'compress' globals */
	maxbits = XV_BITS;
	maxmaxcode = 1<<XV_BITS;
	(void) memset((void *)htab,    0, sizeof(htab));
	(void) memset((void *)codetab, 0, sizeof(codetab));
	hsize = HSIZE;
	free_ent = 0;
	clear_flg = 0;
	in_count = 1;
	out_count = 0;
	cur_accum = 0;
	cur_bits = 0;


	/*
	 * Set up the necessary values
	 */
	out_count = 0;
	clear_flg = 0;
	in_count = 1;
	maxcode = MAXCODE(n_bits = g_init_bits);

	ClearCode = (1 << (init_bits - 1));
	EOFCode = ClearCode + 1;
	free_ent = ClearCode + 2;

	char_init();
	ent = pc2nc[image[0]];

	hshift = 0;
	for ( fcode = (long) hsize;  fcode < 65536L; fcode *= 2L ) hshift++;
	hshift = 8 - hshift;                /* set hash code range bound */

	hsize_reg = hsize;
	cl_hash( (long int) hsize_reg);            /* clear hash table */

	output(ClearCode);
		
	s = 1;
	for(y = 0; y < height; y++)
	{
		w = y * width;
		for(x = s; x < width; x++)
		{
			c = pc2nc[image[w + x]];
			in_count++;

			fcode = (long) ( ( (long) c << maxbits) + ent);
			i = (((int) c << hshift) ^ ent);    /* xor hashing */

			if ( HashTabOf (i) == fcode )
			{
				ent = (int) CodeTabOf (i);
				continue;
			}
			else if ( (long)HashTabOf (i) < 0 )      /* empty slot */
			  goto nomatch;

			disp = hsize_reg - i;           /* secondary hash (after G. Knott) */
			if ( i == 0 ) disp = 1;

probe:
			if ( (i -= disp) < 0 )
			  i += hsize_reg;

			if ( HashTabOf (i) == fcode )
			{
			  ent = (int) CodeTabOf (i);
			  continue;
			}

			if ( (long)HashTabOf (i) >= 0 ) goto probe;

nomatch:
			output(ent);
			out_count++;
			ent = c;

			if ( free_ent < maxmaxcode )
			{
				CodeTabOf (i) = (UNSHORT) free_ent++; /* code -> hashtable */
				HashTabOf (i) = fcode;
			}
			else
				cl_block();
		}
		s = 0;
	}

	/* Put out the final code */
	output(ent);
	out_count++;
	output(EOFCode);
}


/*
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static UNLONG masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                         0x001F, 0x003F, 0x007F, 0x00FF,
                         0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                         0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

static void output(int code )
{
	cur_accum &= masks[cur_bits];

	if (cur_bits > 0)
		cur_accum |= ((long)code << cur_bits);
	else
		cur_accum = (UNLONG) code;
	
	cur_bits += n_bits;

	while( cur_bits >= 8 )
	{
		char_out( (int) (cur_accum & 0xff) );
		cur_accum >>= 8;
		cur_bits -= 8;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */

	if (free_ent > maxcode || clear_flg)
	{

		if( clear_flg )
		{
			maxcode = MAXCODE (n_bits = g_init_bits);
			clear_flg = 0;
		}
		else
		{
			n_bits++;
			maxcode = ( n_bits == maxbits ) ? maxmaxcode : MAXCODE(n_bits);
		}
	}
	
	if( code == EOFCode )
	{
		/* At EOF, write the rest of the buffer */
		while( cur_bits > 0 )
		{
			char_out( (int)(cur_accum & 0xff) );
			cur_accum >>= 8;
			cur_bits -= 8;
		}

		flush_char();
	
		(void) fflush( g_outfile );
	}
}


static void cl_block (void)       /* table clear for block compress */
{
	/* Clear out the hash table */
	cl_hash ( (long int) hsize );
	free_ent = ClearCode + 2;
	clear_flg = 1;
	output(ClearCode);
}


static void cl_hash(register long int hs )     /* reset code table */
{
	register long int *htab_p = htab+hsize;
	register long i;
	register long m1 = -1;

	i = hs - 16;
	do {                            /* might use Sys V memset(3) here */
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);

	for ( i += 16; i > 0; i-- )
		*--htab_p = m1;
}


/*
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Number of characters so far in this 'packet'
 */
static int a_count;

/*
 * Set up the 'byte output' routine
 */
static void char_init(void)
{
	a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[ 256 ];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void char_out(int c )
{
	accum[ a_count++ ] = c;
	if( a_count >= 254 ) flush_char();
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void flush_char(void)
{
	if( a_count > 0 )
	{
		(void) fputc( a_count, g_outfile );
		(void) fwrite( accum, 1, (size_t)a_count, g_outfile );
		a_count = 0;
	}
}	
