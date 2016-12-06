/*********************************************************************/
/** @file getmem_ext.c
 *
 * Natural extensions to the UNIX malloc library and malloc.h
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     g e t m e m _ e x t . c                                          *
*                                                                      *
*     Natural extensions to the UNIX malloc library and malloc.h       *
*                                                                      *
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

#include "getmem_ext.h"

/***********************************************************************
*                                                                      *
*     m e m r e s e t  - Reset alloc counters                          *
*     m e m s t a r t  - Enable alloc counters                         *
*     m e m s t o p    - Disable alloc counters                        *
*     m e m a d d m    - Add to malloc counter                         *
*     m e m a d d r    - Add to realloc counter                        *
*     m e m g e t m    - Return malloc counter                         *
*     m e m g e t r    - Return realloc counter                        *
*                                                                      *
***********************************************************************/

static	size_t	Mcount = 0;
static	size_t	Rcount = 0;
static	int		Enable = 0;

/*********************************************************************/
/** Reset alloc counters.
 *********************************************************************/
void	memreset(void)
	{
	Mcount = 0;
	Rcount = 0;
	}

/*********************************************************************/
/** Enable alloc counters.
 *********************************************************************/
void	memstart(void)
	{
	memreset();
	Enable = 1;
	}

/*********************************************************************/
/** Disable alloc counters.
 *********************************************************************/
void	memstop(void)
	{
	Enable = 0;
	}

/*********************************************************************/
/** Add to malloc counter.
 *
 * @return The amount that was added to the memory count.
 *********************************************************************/
size_t	memaddm(size_t size)
	{
	if (Enable) Mcount += size;
	return size;
	}

/*********************************************************************/
/** Add to realloc counter.
 *
 * @return The amount that was added to the memory count.
 *********************************************************************/
size_t	memaddr(size_t size)
	{
	if (Enable) Rcount += size;
	return size;
	}

/*********************************************************************/
/** Get malloc counter.
 *
 * @return Current malloc count.
 *********************************************************************/
size_t	memgetm(void)
	{
	return Mcount;
	}

/*********************************************************************/
/** Get realloc counter.
 *
 * @return Current realloc count.
 *********************************************************************/
size_t	memgetr(void)
	{
	return Rcount;
	}
