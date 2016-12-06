/***********************************************************************/
/** @file message.c
 *
 *     Routines to manage status and diagnostic messages.
 *
 *     The following message types are supported:
 *     @li @b error		-	an error condition the application cannot continue to
 *						    live with,
 *     @li @b warning	-	a condition the application can live with, but is
 *     						probably not desired,
 *     @li @b status	-	an important condition the user should know about,
 *     @li @b informative -	routine, narrative messages,
 *     @li @b diagnostic -	internal, design specific messages.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#include "message.h"
#include "parse.h"

#include <fpa_types.h>
#include <fpa_getmem.h>
#include <fpa_macros.h>
#include <stdio.h>
#include <stdarg.h>

/**********************************************************************/

typedef	struct pr_desc
		{
		STRING	module;
		int		olevel;
		int		mstyle;
		} PR_DESC;

static	PR_DESC	Defaults   = { NULL, 3, 2 };
static	int		NumModules = 0;
static	PR_DESC	*Modules   = NULL;

static	PR_DESC	*pr_get_module(STRING);
static	PR_DESC	*pr_add_module(STRING);
static	LOGICAL	pr_go(int, STRING);
static	void	pr_do_module(STRING);

/***********************************************************************
*                                                                      *
*     Obsolescent Functions:                                           *
*                                                                      *
*     p r _ m o d e                                                    *
*     p r _ o u t p u t                                                *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
void	pr_mode(int olevel, int	elevel, int	mstyle)
	{
	(void) printf("Obsolete function called:\n");
	(void) printf("   --> pr_mode(out_level, err_level, mod_style)\n");
	(void) printf("Replace with:\n");
	(void) printf("   --> pr_control(module, out_level, mod_style)\n");
	(void) printf("       module=NULL applies to default module\n");
	pr_control(NULL, olevel, mstyle);
	}

/**********************************************************************/

LOGICAL pr_output(int olevel)
	{
	(void) printf("Obsolete function called:\n");
	(void) printf("   --> pr_output(out_level)\n");
	(void) printf("Replace with:\n");
	(void) printf("   --> pr_level(module, out_level)\n");
	(void) printf("       module=NULL applies to default module\n");
	return pr_level(NULL, olevel);
	}

/***********************************************************************/
/** Set the module-specific printing states for subsequent calls to
 * 	the print functions.
 *
 *     The output print level controls whether or not certain types
 *     of message are printed to stdout.  The following table indicates
 *     which message types are printed under each print level:
 *
 * @verbatim
                            0  1  2  3  4  5+

             error       -     X  X  X  X  X
             warning     -        X  X  X  X
             status      -           X  X  X
             informative -              X  X
             diagnostic  -                 X
   @endverbatim
 *
 *     Message types which are allowed according to the output print
 *     level, will appear on stdout.  All other message types will not
 *     be printed.
 *
 *     Note, the error print level in pr_mode() is ignored.
 *
 *     The default output print level is 3.
 *
 *     Module style controls how module names appear in the message.
 *     The following module styles are supported:
 *
 *     @li @b 0  =  no module name appears,
 *     @li @b 1  =  the module name appears as "module:",
 *     @li @b 2+ =  the module name appears as "[module]".
 *
 *     The default module style is 2.
 *
 * 		@param[in]	module	module name
 * 		@param[in]	olevel	output print level
 * 		@param[in]	mstyle	module style
 ***********************************************************************/

void	pr_control
	(
	STRING	module,
	int		olevel,
	int		mstyle
	)

	{
	PR_DESC	*info;

	info = pr_add_module(module);

	info->olevel = olevel;
	info->mstyle = mstyle;
	}

/***********************************************************************/
/** Determine what levels should get printed
 *
 * 	@param[in]	module	module to check
 * 	@param[in]	olevel	output print level
 * 	@return TRUE if the print level is <= to the print level specified
 * 	in the setup file.
 ***********************************************************************/

LOGICAL pr_level
	(
	STRING module,
	int olevel
	)

	{
	PR_DESC	*info;

	info = pr_get_module(module);

	if (olevel <= info->olevel) return TRUE;
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     p r _ d i a g                                                    *
*     p r _ i n f o                                                    *
*     p r _ s t a t u s                                                *
*     p r _ w a r n i n g                                              *
*     p r _ e r r o r                                                  *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Optionally print a message, warning or error, in a uniform format.
*
*	These functions use the same variable argument list convention
*	as the printf() family of functions, where the number of
*  	arguments is determined by the format, as in:
*
*     @code   pr_diag(module, format, arg1, arg2, ... ) @endcode
*
*     For example:
*
* @code
*        strcpy(fname, "data.1");
*        fp = fopen(fname, "r");
*        if (!fp)
*           {
*           pr_diag("Data Access", "Cannot open file: %s\n", fname);
*           ...
*           }
* @endcode
*
*     If the file open above is not successful, the following message
*     would be printed:
*
*     @verbatim   [Data Access] Cannot open file: data.1 @endverbatim
*
* 	@param[in]	module	module sending the message.
* 	@param[in]	format	formated output string (as in fprintf).
* 	@param[in]	...		output variables.
***********************************************************************/

void	pr_diag(STRING module, STRING format, ...)
	{
	va_list	args;

	if (!pr_go(5, module)) return;

	pr_do_module(module);

	va_start(args, format);
	(void) vprintf(format, args);
	va_end(args);
	}

/**********************************************************************/

/***********************************************************************/
/**	Optionally print a message, warning or error, in a uniform format.
*
*	These functions use the same variable argument list convention
*	as the printf() family of functions, where the number of
*  	arguments is determined by the format, as in:
*
*     @code   pr_info(module, format, arg1, arg2, ... ) @endcode
*
*     For example:
*
* @code
*        strcpy(fname, "data.1");
*        fp = fopen(fname, "r");
*        if (!fp)
*           {
*           pr_info("Data Access", "Cannot open file: %s\n", fname);
*           ...
*           }
* @endcode
*
*     If the file open above is not successful, the following message
*     would be printed:
*
*     @verbatim   [Data Access] Cannot open file: data.1 @endverbatim
*
* 	@param[in]	module	module sending the message.
* 	@param[in]	format	formated output string (as in fprintf).
* 	@param[in]	...		output variables.
***********************************************************************/
void	pr_info(STRING module, STRING format, ...)
	{
	va_list	args;

	if (!pr_go(4, module)) return;

	pr_do_module(module);

	va_start(args, format);
	(void) vprintf(format, args);
	va_end(args);
	}

/**********************************************************************/

/***********************************************************************/
/**	Optionally print a message, warning or error, in a uniform format.
*
*	These functions use the same variable argument list convention
*	as the printf() family of functions, where the number of
*  	arguments is determined by the format, as in:
*
*     @code   pr_status(module, format, arg1, arg2, ... ) @endcode
*
*     For example:
*
* @code
*        strcpy(fname, "data.1");
*        fp = fopen(fname, "r");
*        if (!fp)
*           {
*           pr_status("Data Access", "Cannot open file: %s\n", fname);
*           ...
*           }
* @endcode
*
*     If the file open above is not successful, the following message
*     would be printed:
*
*     @verbatim   [Data Access] Cannot open file: data.1 @endverbatim
*
* 	@param[in]	module	module sending the message.
* 	@param[in]	format	formated output string (as in fprintf).
* 	@param[in]	...		output variables.
***********************************************************************/
void	pr_status(STRING module, STRING format, ...)
	{
	va_list	args;

	if (!pr_go(3, module)) return;

	pr_do_module(module);

	va_start(args, format);
	(void) vprintf(format, args);
	va_end(args);
	}

/**********************************************************************/

/***********************************************************************/
/**	Optionally print a message, warning or error, in a uniform format.
*
*	These functions use the same variable argument list convention
*	as the printf() family of functions, where the number of
*  	arguments is determined by the format, as in:
*
*     @code   pr_warning(module, format, arg1, arg2, ... ) @endcode
*
*     For example:
*
* @code
*        strcpy(fname, "data.1");
*        fp = fopen(fname, "r");
*        if (!fp)
*           {
*           pr_warning("Data Access", "Cannot open file: %s\n", fname);
*           ...
*           }
* @endcode
*
*     If the file open above is not successful, the following message
*     would be printed:
*
*     @verbatim   [Data Access] WARNING! Cannot open file: data.1 @endverbatim
*
* 	@param[in]	module	module sending the message.
* 	@param[in]	format	formated output string (as in fprintf).
* 	@param[in]	...		output variables.
***********************************************************************/
void	pr_warning(STRING module, STRING format, ...)
	{
	va_list	args;

	if (!pr_go(2, module)) return;

	pr_do_module(module);
	(void) printf("WARNING! ");

	va_start(args, format);
	(void) vprintf(format, args);
	va_end(args);
	}

/**********************************************************************/

/***********************************************************************/
/**	Optionally print a message, warning or error, in a uniform format.
*
*	These functions use the same variable argument list convention
*	as the printf() family of functions, where the number of
*  	arguments is determined by the format, as in:
*
*     @code   pr_error(module, format, arg1, arg2, ... ) @endcode
*
*     For example:
*
* @code
*        strcpy(fname, "data.1");
*        fp = fopen(fname, "r");
*        if (!fp)
*           {
*           pr_error("Data Access", "Cannot open file: %s\n", fname);
*           ...
*           }
* @endcode
*
*     If the file open above is not successful, the following message
*     would be printed:
*
*     @verbatim   [Data Access] ERROR! Cannot open file: data.1 @endverbatim
*
* 	@param[in]	module	module sending the message.
* 	@param[in]	format	formated output string (as in fprintf).
* 	@param[in]	...		output variables.
***********************************************************************/
void	pr_error(STRING module, STRING format, ...)
	{
	va_list	args;

	if (!pr_go(1, module)) return;

	pr_do_module(module);
	(void) printf("ERROR! ");

	va_start(args, format);
	(void) vprintf(format, args);
	va_end(args);
	}

/***********************************************************************
*                                                                      *
*     p r _ a d d _ m o d u l e                                        *
*     p r _ g e t _ m o d u l e                                        *
*                                                                      *
***********************************************************************/

static	PR_DESC	*pr_add_module(STRING module)
	{
	PR_DESC	*info;
	int		last;

	if (blank(module)) return &Defaults;

	info = pr_get_module(module);
	if (info != &Defaults) return info;

	last    = NumModules++;
	Modules = GETMEM(Modules, struct pr_desc, NumModules);
	info    = Modules + last;

	info->module = strdup(module);
	info->olevel = Defaults.olevel;
	info->mstyle = Defaults.mstyle;
	return info;
	}

/**********************************************************************/

static	PR_DESC	*pr_get_module(STRING module)
	{
	PR_DESC	*info;
	int		i;

	if (blank(module)) return &Defaults;

	for (i=0; i<NumModules; i++)
		{
		info = Modules + i;
		if (same(info->module, module)) return info;
		}

	return &Defaults;
	}

/***********************************************************************
*                                                                      *
*     p r _ s t r e a m                                                *
*     p r _ d o _ m o d u l e                                          *
*                                                                      *
***********************************************************************/

static	LOGICAL	pr_go(int olevel, STRING module)
	{
	PR_DESC	*info;

	info = pr_get_module(module);

	return (LOGICAL) (olevel <= info->olevel);
	}

/**********************************************************************/

static	void	pr_do_module(STRING module)
	{
	if (blank(module))    return;
	if (Defaults.mstyle <= 0) return;

	switch (Defaults.mstyle)
		{
		case 1:		(void) printf("%s: ", module);
					break;

		case 2:
		default:	(void) printf("[%s] ", module);
					break;
		}
	}

/***********************************************************************
*                                                                      *
*     s e t _ f e a t u r e _ m o d e                                  *
*     g e t _ f e a t u r e _ m o d e                                  *
*                                                                      *
***********************************************************************/

static	int		NFeatureList = 0;
static	TABLE	*FeatureList = (TABLE *)0;

/**********************************************************************/

void	set_feature_mode(STRING feature, STRING	mode)
	{
	int		i;

	if (blank(feature)) return;
	if (IsNull(mode))   mode = "";

	for (i=0; i<NFeatureList; i++)
		{
		if (same(feature, FeatureList[i].index))
			{
			/* Found it - change its value */
			FREEMEM(FeatureList[i].value);
			FeatureList[i].value = strdup(mode);
			return;
			}
		}

	/* Not found - add it */
	i = NFeatureList++;
	FeatureList = GETMEM(FeatureList, TABLE, NFeatureList);
	FeatureList[i].index = strdup(feature);
	FeatureList[i].value = strdup(mode);
	}

/**********************************************************************/

STRING	get_feature_mode(STRING feature)
	{
	int		i;

	if (blank(feature)) return NULL;

	for (i=0; i<NFeatureList; i++)
		{
		if (same(feature, FeatureList[i].index))
			{
			/* Found it - return its value */
			return FeatureList[i].value;
			}
		}

	/* Not found - return nothing */
	return NULL;
	}

#ifdef MACHINE_SUN

/***********************************************************************
*                                                                      *
*     v s c a n f                                                      *
*     v f s c a n f                                                    *
*     v s s c a n f                                                    *
*                                                                      *
***********************************************************************/

int		vscanf(const STRING format, ...)
	{
	va_list	args;
	POINTER	arg1, arg2, arg3, arg4, arg5;

	va_start(args, format);
	arg1 = va_arg(args, POINTER);
	arg2 = va_arg(args, POINTER);
	arg3 = va_arg(args, POINTER);
	arg4 = va_arg(args, POINTER);
	arg5 = va_arg(args, POINTER);
	va_end(args);

	return scanf(format, arg1, arg2, arg3, arg4, arg5);
	}

int		vfscanf(FILE *stream, const STRING format, ...)
	{
	va_list	args;
	POINTER	arg1, arg2, arg3, arg4, arg5;

	va_start(args, format);
	arg1 = va_arg(args, POINTER);
	arg2 = va_arg(args, POINTER);
	arg3 = va_arg(args, POINTER);
	arg4 = va_arg(args, POINTER);
	arg5 = va_arg(args, POINTER);
	va_end(args);

	return fscanf(stream, format, arg1, arg2, arg3, arg4, arg5);
	}

int		vsscanf(STRING input, const STRING format, ...)
	{
	va_list	args;
	POINTER	arg1, arg2, arg3, arg4, arg5;

	va_start(args, format);
	arg1 = va_arg(args, POINTER);
	arg2 = va_arg(args, POINTER);
	arg3 = va_arg(args, POINTER);
	arg4 = va_arg(args, POINTER);
	arg5 = va_arg(args, POINTER);
	va_end(args);

	return sscanf(input, format, arg1, arg2, arg3, arg4, arg5);
	}

#endif


#ifdef STANDALONE

/***********************************************************************
*                                                                      *
*     Test Programs:                                                   *
*                                                                      *
***********************************************************************/

void	main(void)
	{
	char    buf[25];
	int		olevel, mstyle;

	while (1)
		{
		(void) printf("\n");
		(void) printf("Output print level:\n");
		(void) printf("   0 - No output\n");
		(void) printf("   1 - Errors\n");
		(void) printf("   2 - Warnings\n");
		(void) printf("   3 - Status\n");
		(void) printf("   4 - Info\n");
		(void) printf("   5 - Diagnostic\n");
		(void) printf("Enter output print level: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf)) break;
		if (sscanf(buf, "%d", &olevel) != 1) continue;

		(void) printf("\n");
		(void) printf("Module style:\n");
		(void) printf("   0 - No module\n");
		(void) printf("   1 - \"Module:\"\n");
		(void) printf("   2 - \"[Module]\"\n");
		(void) printf("Enter module style: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &mstyle) != 1) continue;

		pr_control(NULL, olevel, mstyle);

		(void) printf("\n");
		(void) printf("=========================\n");

		pr_diag("module 1", "diag 1\n");
		pr_diag("module 1", "diag 2 \"%s\"\n", "arg1");
		pr_diag("module 1", "diag 3 \"%s\" %d\n", "arg1", olevel);

		pr_info("module 1", "info 1\n");
		pr_info("module 1", "info 2 \"%s\"\n", "arg1");
		pr_info("module 1", "info 3 \"%s\" %d\n", "arg1", olevel);

		pr_status("module 1", "status 1\n");
		pr_status("module 1", "status 2 \"%s\"\n", "arg1");
		pr_status("module 1", "status 3 \"%s\" %d\n", "arg1", olevel);

		pr_warning("module 1", "warning 1\n");
		pr_warning("module 1", "warning 2 \"%s\"\n", "arg1");
		pr_warning("module 1", "warning 3 \"%s\" %d\n", "arg1", olevel);

		pr_error("module 1", "error 1\n");
		pr_error("module 1", "error 2 \"%s\"\n", "arg1");
		pr_error("module 1", "error 3 \"%s\" %d\n", "arg1", olevel);

		(void) printf("=========================\n");
		}
	}

#endif /* STANDALONE */
