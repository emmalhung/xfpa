/***********************************************************************
*                                                                      *
*      m m m . c                                                       *
*                                                                      *
*      Routines to handle Memory Management Mode.                      *
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

#include "mmm.h"
#include "parse.h"
#include "message.h"

#include <fpa_getmem.h>

#include <stdlib.h>

/***********************************************************************
*                                                                      *
*      s e t _ M M M                                                   *
*      g e t _ M M M                                                   *
*      c h e c k _ M M M                                               *
*                                                                      *
***********************************************************************/

static	MMM		CurrentMMM = MMM_AllocateWhenNeeded;
static	LOGICAL	MMMchecked = FALSE;
static	LOGICAL	MMMenvset  = FALSE;

/**********************************************************************/

void	set_MMM(MMM mmm)
	{
	STRING	mode;

	if (MMMenvset) return;

	/* Override mode setting with environment setting if present */
	/* or "advanced_features" block setting if present */
	if (!MMMchecked)
		{
		mode = getenv("FPA_MMM");
		if (blank(mode)) mode = get_feature_mode("MMM");
		if (blank(mode))
			{
			MMMenvset = FALSE;
			}
		else if (same_ic(mode, "prealloc"))
			{
			mmm = MMM_Preallocate;
			MMMenvset = TRUE;
			}
		else if (same_ic(mode, "alloc"))
			{
			mmm = MMM_AllocateWhenNeeded;
			MMMenvset = TRUE;
			}
		else if (same_ic(mode, "dealloc"))
			{
			mmm = MMM_AllocateAndFree;
			MMMenvset = TRUE;
			}
		else
			{
			pr_warning("MMM", "Unknown Memory Management Mode \'%s\'.\n", mode);
			MMMenvset = FALSE;
			}
		}

	/* Set the mode */
	switch (mmm)
		{
		case MMM_Preallocate:
		case MMM_AllocateWhenNeeded:
		case MMM_AllocateAndFree:
				if (MMMchecked && CurrentMMM==mmm) return;
				CurrentMMM = mmm;
				break;

		default:
				pr_warning("MMM", "Unknown Memory Management Mode.\n");
				if (MMMchecked)
					{
					pr_warning("MMM", "No change to Memory Management.\n");
					return;
					}
		}
	MMMchecked = TRUE;

	switch (mmm)
		{
		case MMM_Preallocate:
				pr_status("MMM", "Large objects will be pre-allocated.\n");
				CurrentMMM = mmm;
				return;

		case MMM_AllocateWhenNeeded:
				pr_status("MMM",
					"Large objects will be dynamically allocated.\n");
				CurrentMMM = mmm;
				return;

		case MMM_AllocateAndFree:
				pr_status("MMM",
					"Large objects will be dynamically allocated and freed.\n");
				CurrentMMM = mmm;
				return;
		}

	}

/**********************************************************************/

MMM		get_MMM(void)
	{
	check_MMM();
	return CurrentMMM;
	}

/**********************************************************************/

void	check_MMM(void)
	{
	if (MMMchecked) return;
	set_MMM(CurrentMMM);
	}

/***********************************************************************
*                                                                      *
*     M M M _ b e g i n _ c o u n t                                    *
*     M M M _ r e p o r t _ c o u n t                                  *
*                                                                      *
***********************************************************************/

void	MMM_begin_count(void)

	{
	memstart();
	}

/**********************************************************************/

void	MMM_report_count(STRING	msg)

	{
	size_t	m, r;
	memstop();
	m = memgetm();
	r = memgetr();

	pr_diag("MMM", "%s alloc: %d realloc: %d\n", msg, m, r);
	}
