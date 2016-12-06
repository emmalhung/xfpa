/*********************************************************************/
/** @file png_stream.c
 *
 * 
 * Contains code to allow us to read and write a png stream from memory 
 * instead of a file.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
#include "png_stream.h"
#include <fpa_getmem.h>
#ifdef MACHINE_PCLINUX
#include <png.h>
#endif

/***********************************************************************
*                                                                      *
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


/** Structure to hold the png stream in memory */
typedef struct
	{
	UNCHAR *stream_ptr;	/**< PNG stream */
	int		stream_len;	/**< number of bytes in stream */
	} PNG_STREAM;

#ifdef MACHINE_PCLINUX
void user_read_data(png_structp, png_bytep, png_uint_32);
void user_write_data(png_structp, png_bytep, png_uint_32);
void user_flush_data(png_structp);
#endif

/*********************************************************************/
/** Custom read function used so that libpng will read from memory 
 * 	instead of a file.
 *
 * 	@param[in]	png_ptr	
 * 	@param[in]	data
 * 	@param[in]	length
 *********************************************************************/
#ifdef MACHINE_PCLINUX
void	user_read_data
	(
	 png_structp png_ptr, 
	 png_bytep data, 
	 png_uint_32 length 
	)
	{
	char 		*ptr;
	int			offset;
	PNG_STREAM	*mem;

	mem = ( PNG_STREAM *)png_get_io_ptr(png_ptr);
	ptr = ( void * )mem->stream_ptr;
	offset = mem->stream_len;
	memcpy(data, ptr+offset, length);
	mem->stream_len += length;
	}
#endif
/*********************************************************************/
/** Custom write function used so that libpng will write to memory 
 * 	instead of a file.
 *
 * 	@param[in]	png_ptr	
 * 	@param[in]	data
 * 	@param[in]	length
 *********************************************************************/
#ifdef MACHINE_PCLINUX
void	user_write_data
	(
	 png_structp png_ptr, 
	 png_bytep data, 
	 png_uint_32 length 
	)
	{
	UNCHAR		*ptr;
	int			offset;
	PNG_STREAM	*mem;

	mem = ( PNG_STREAM *)png_get_io_ptr(png_ptr);
	ptr = mem->stream_ptr;
	offset = mem->stream_len;
	memcpy(ptr+offset, data, length);
	mem->stream_len += length;
	}
#endif
/*********************************************************************/
/** Dummy Custom flush function
 *
 * 	@param[in]	png_ptr	
 *********************************************************************/
#ifdef MACHINE_PCLINUX
void	user_flush_data
	(
	 png_structp png_ptr 
	)
	{
	/* Do nothing */
	return;
	}
#endif

/*********************************************************************/
/** Read from PNG stream
 *
 * 	@param[in]	pngbuf	input buffer
 * 	@param[in]	width	width of buffer
 * 	@param[in]	height	height of buffer
 * 	@param[in]	nbits	number of bits per pixel
 * 	@param[out]	*cout	output data
 *********************************************************************/
#ifdef MACHINE_PCLINUX
int	read_png
	(
	 UNCHAR *pngbuf, 
	 int *width,
	 int *height,
	 int nbits,
	 UNCHAR *cout
	)
	{
	int		interlace, colour, compress, filter, bit_depth;
	int		jj, kk, nn, bytes, clen;
	png_structp	png_ptr;
	png_infop	info_ptr, end_info;
	png_bytepp	row_pointers;
	PNG_STREAM	read_io_ptr;

	/* check if stream is valid PNG format */
	if ( png_sig_cmp(pngbuf, 0, 8) != 0 )
		{
		/* TODO: Error message goes here */
		return (-3);
		}
	/* create and initialize png_structs */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
	if ( !png_ptr )
		{
		/* TODO: Error message goes here */
		return (-1);
		}
	info_ptr = png_create_info_struct(png_ptr);
	if ( !info_ptr )
		{
		/* TODO: Error message goes here */
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (-2);
		}
	end_info = png_create_info_struct(png_ptr);
	if ( !end_info )
		{
		/* TODO: Error message goes here */
		png_destroy_read_struct(&png_ptr, (png_infopp)info_ptr, (png_infopp)NULL);
		return (-2);
		}

	/* Set Error callback */
	if (setjmp(png_jmpbuf(png_ptr)))
		{
		/* TODO: Error message goes here */
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return (-3);
		}

	/* Initialize info for reading PNG stream from memory */
	read_io_ptr.stream_ptr = (png_voidp)pngbuf;
	read_io_ptr.stream_len = 0;

	/* Set new custom read function */
	png_set_read_fn(png_ptr, (png_voidp)&read_io_ptr, (png_rw_ptr)user_read_data);
	
	/* Read and decode PNG stream */
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* Get pointer to each row of image data */
	row_pointers = png_get_rows(png_ptr, info_ptr);

	/* Get image info, such as size depth, colourtype, etc ... */
	(void)png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)width, (png_uint_32 *)height,
					   &bit_depth, &colour, &interlace, &compress, &filter);

	/* Check type of image */

	if ( colour == PNG_COLOR_TYPE_RGB ) 
		{
		bit_depth = 24;
		}
	else if ( colour == PNG_COLOR_TYPE_RGB_ALPHA ) 
		{
		bit_depth = 32;
		}

	/* Copy image data to output string */
	nn = 0;
	bytes = bit_depth/8;
	clen = (*width)*bytes;
	for (jj=0; jj<(*height); jj++)
		{
		for(kk=0; kk<clen; kk++)
			{
			cout[nn] = *(row_pointers[jj]+kk);
			nn++;
			}
		}
	/* Clean up */
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	return 0;
	}
#endif

/*********************************************************************/
/** Write to PNG stream
 *
 * 	@param[in]	data	data to write to the buffer	
 * 	@param[in]	width	width of buffer
 * 	@param[in]	height	height of buffer
 * 	@param[in]	nbits	number of bits per pixel
 * 	@param[out]	*pngbuf	output buffer
 *********************************************************************/
#ifdef MACHINE_PCLINUX
int	write_png
	(
	 UNCHAR *data,
	 int width,
	 int height,
	 int nbits,
	 UNCHAR *pngbuf
	)
	{
	int			colour_type;
	int			jj, bytes, pnglen, bit_depth;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep	**row_pointers;
	PNG_STREAM	write_io_ptr;

	/* create and initialize png_structs */
	png_ptr	= png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
	if ( !png_ptr ) 
		{
		/* TODO: Error message goes here */
		return -1;
		}
	
	info_ptr = png_create_info_struct(png_ptr);
	if ( !info_ptr ) 
		{
		/* TODO: Error message goes here */
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return -2;
		}

	/* Set Error callback */
	if (setjmp(png_jmpbuf(png_ptr)))
		{
		/* TODO: Error message goes here */
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return -2;
		}

	/* Initialize info for writing PNG stream to memory */
	write_io_ptr.stream_ptr = (png_voidp)pngbuf;
	write_io_ptr.stream_len = 0;

	/* Set new custom write functions */
	png_set_write_fn(png_ptr, (png_voidp)&write_io_ptr, (png_rw_ptr)user_write_data,
			(png_flush_ptr)user_flush_data);
	bit_depth = nbits;
	colour_type = PNG_COLOR_TYPE_GRAY;
	if (nbits == 24 ) 
		{
		bit_depth = 8;
		colour_type = PNG_COLOR_TYPE_RGB;
		}
	else if ( nbits == 32 ) 
		{
		bit_depth=8;
		colour_type = PNG_COLOR_TYPE_RGB_ALPHA;
		}
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, colour_type, 
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/* Put image data into PNG info structure */
	bytes = nbits/8;
	row_pointers = INITMEM(png_bytep*, height*sizeof(png_bytep*));
	for (jj = 0; jj < height; jj++) 
		row_pointers[jj] = (png_bytep *)(data+(jj*width*bytes));
	png_set_rows(png_ptr, info_ptr, (png_bytepp)row_pointers);

	/* Do the PNG encoding, and write out PNG stream */
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* Clean up */
	png_destroy_write_struct(&png_ptr, &info_ptr);
	FREEMEM(row_pointers);
	pnglen = write_io_ptr.stream_len;
	return pnglen;
	}
#endif
