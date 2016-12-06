/**********************************************************************/
/** @file read_setup.c
 *
 *  Contians those functions which are used to read setup
 *  files which use the format:
 *
 * @code
 *  block_1_key
 *  {
 *    setup_key value value ...
 *    setup_key value value ...
 *  }
 *  block_2_key
 *    ...
 * @endcode
 *
 *  All blank lines and any lines beginning with "*" or "#" are
 *  ignored
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*  r e a d _ s e t u p . c                                             *
*                                                                      *
*  purpose: Contains those functions which are used to read setup      *
*           files which use the format:                                *
*                                                                      *
*                   block_1_key                                        *
*                   {                                                  *
*                       setup_key value value ...                      *
*                       setup_key value value ...                      *
*                   }                                                  *
*                   block_2_key                                        *
*                   ...                                                *
*                                                                      *
*           All blank lines and any lines beginning with "*" or "#"    *
*           are ignored.                                               *
*                                                                      *
*  External functions contained in this file:                          *
*                                                                      *
*                                                                      *
*                                                                      *
*                                                                      *
*                                                                      *
*                                                                      *
*    setup_files(local,setup_list) - Construct the list of setup files *
*                                    that are to be consulted by the   *
*                                    application.  If a local setup    *
*                                    file name is given, its full      *
*                                    pathname is constructed by        *
*                                    searching the standard list of    *
*                                    setup directories.                *
*                                                                      *
*    base_directory() - Return the standard directory which is used    *
*                       as the root or base of the directory tree.     *
*                                                                      *
*    home_directory() - Return the full name of the directory which    *
*                       is being used as the "home" directory - the    *
*                       directory which all filenames will be taken    *
*                       relative to.  This will either be the "home"   *
*                       directory, if defined in the "directories"     *
*                       block, or the base directory.                  *
*                                                                      *
*    work_directory() - Return the full name of the directory from     *
*                       which the application will be running.  This   *
*                       will be either the "home" directory, if        *
*                       defined in the "directories" block, or the     *
*                       current directory.                             *
*                                                                      *
*    define_setup(num,setup_list) - Identify the list of setup files,  *
*                                   read the "directories" block, and  *
*                                   chdir() to "home" directory, if    *
*                                   defined.                           *
*                                                                      *
*    current_setup_list() - Return the full path of the current setup  *
*                           file list.                                 *
*                                                                      *
*    find_setup_block(block_key, required) - Open the setup file, scan *
*                           for the given setup block and leave the    *
*                           file pointer at the beginning of the first *
*                           line of the block.                         *
*                                                                      *
*    setup_block_line() - Read the next line in the current setup      *
*                         block and leave the file pointer at the      *
*                         beginning of the following line.  A back-    *
*                         slash "\" at the end of a line causes the    *
*                         next line to be automatically appended.      *
*                         Several lines may be appended in this way    *
*                         within reason.                               *
*                                                                      *
*    app_service() - Returns the service associated with the given     *
*                    application.                                      *
*                                                                      *
*    config_file_name(key) - Return the full name of the config file   *
*                            identified by the given keyword in the    *
*                            "config_files" setup block.               *
*                                                                      *
*    open_config_file(key) - Open the config file identified by the    *
*                            given keyword for reading, and leave      *
*                            the file pointer at the beginning of      *
*                            the first line following the "revision"   *
*                            line.                                     *
*                                                                      *
*    close_config_file() - Close the currently open config file.       *
*                                                                      *
*    config_file_line() - Read the next line in the current config     *
*                         file and leave the file pointer at the       *
*                         beginning of the following line.  A back-    *
*                         slash "\" at the end of a line causes the    *
*                         next line to be automatically appended.      *
*                         Several lines may be appended in this way    *
*                         within reason.                               *
*                                                                      *
*    get_directory(key) - Return the full name of the directory        *
*                         identified by the given keyword in the       *
*                         "directories" block.                         *
*                                                                      *
*    get_path(key,file) - Return the full name of the given file in    *
*                         the directory identified by the given        *
*                         keyword in the "directories" setup block.    *
*                                                                      *
*    get_file(key,file) - Return the full name of the given file in    *
*                         the directory identified by the given        *
*                         keyword in the "directories" setup block but *
*                         only if the file exists. If it does not try  *
*                         the default directory entry in read_setup.h  *
*                         If this is not found either return NULL.     *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 2000 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#define READ_SETUP_INIT

#include "read_setup.h"
#include "read_setup_P.h"
#include "revision.h"
#include "adv_feature.h"

#include <tools/tools.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

typedef	char	DirPath[256];
typedef char	SCLine[4096];

/***********************************************************************
*                                                                      *
*    f p a _ c o n n e c t                                             *
*                                                                      *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/**	Request permission to run FPA library in a particular mode
 * 	
 * 	@param	setup	FPA setup file
 * 	@param	mode	FPA run mode
***********************************************************************/
LOGICAL	fpa_connect

	(
	STRING	setup,
	ACCESS	mode
	)

	{
	int		nsetup;
	STRING	*slist;

	/* Get permission to use the FPA library in the requested mode */
	if (!fpalib_license(mode))
		{
		(void) fprintf(stderr,
				"[fpa_connect] Problem with access mode\n");
		(void) fprintf(stderr,
				"              Use: FpaAccessFull or FpaAccessRead\n");
		return FALSE;
		}

	/* Access the given setup file */
	nsetup = setup_files(setup, &slist);
	if (!define_setup(nsetup, slist))
		{
		(void) fprintf(stderr,
				"[fpa_connect] Problem with setup file \"%s\"\n", setup);
		return FALSE;
		}

	/* Success! */
	return TRUE;
	}

/**********************************************************************/
/**********************************************************************/
static	DirPath	BaseDir  = "";		/* standard base directory */
static	DirPath	HomeDir  = "";		/* home directory */
static	DirPath	WorkDir  = "";		/* working directory */
/**********************************************************************/

/***********************************************************************
*                                                                      *
*    s e t u p _ f i l e s                                             *
*    g e t _ s e t u p                                                 *
*    b a s e _ d i r e c t o r y                                       *
*    h o m e _ d i r e c t o r y                                       *
*    w o r k _ d i r e c t o r y                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function constructs the list of setup files to be consulted
 * by the application.
 *
 * @note This function does not depend on the contents of the setup
 * file, and thus may be called prior to define_setup().
 *
 *	@param[in]	lfile  setup file name
 *	@param[out]	**list a list of setup files
 *  @return The size of the list.
 **********************************************************************/
int		setup_files

	(
	STRING		lfile,
	STRING		**list
	)

	{
	STRING	file;

	static	STRING	*Slist  = NullStringList;
	static	int		Nlist   = 0;
	static	LOGICAL	Defined = FALSE;

	/* Only do this once */
	if (Defined)
		{
		if (list) *list = Slist;
		return Nlist;
		}
	if (list) *list = NullStringList;

	/* Get the setup file */
	file = get_setup(lfile);
	if (!blank(file))
		{
		Nlist++;
		Slist = GETMEM(Slist, STRING, Nlist);
		Slist[Nlist-1] = file;
		}

	/* Done */
	Defined = TRUE;
	if (list) *list = Slist;
	(void) fpalib_verify(FpaAccessRead);
	(void) userlib_verify();
	return Nlist;
	}

/**********************************************************************/

/**********************************************************************/
/** This function constructs the full pathname of the given
 * setup file.
 *
 * @note This function does not depend on the contents of the setup
 * file, and thus may be called prior to define_setup().
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	sfile 	setup file name
 * 	@return If the given file is already an absolute pathname, its
 * 			value is returned unaltered.  Otherwise, its full pathname
 * 			is produced by appending the given file name to the the
 * 			first directory in the standard list of setup directories
 * 			in which the given setup file is found. In either case, if
 * 			the file is not found, Null is returned.
 **********************************************************************/
STRING	get_setup

	(
	STRING		sfile
	)

	{
	STRING	env, dir, file;
	char	cwd[256];

	static	STRING	Sfile  = NullString;

	/* Only do this once */
	/* if (!blank(Sfile)) return Sfile; */

	/* If no file given use $FPA_SETUP_FILE or fpa.setup */
	if (blank(sfile)) sfile = getenv("FPA_SETUP_FILE");
	if (blank(sfile)) sfile = "fpa.setup";

	/* If file is an absolute path test it */
	file = sfile;
	if (abspath(file))
		{
		if (find_file(file)) Sfile = strdup(file);
		return Sfile;
		}

	/* Otherwise search the standard setup directories */

	/* Try ./sfile */
	dir  = getcwd(cwd,256);
	file = pathname(dir, sfile);
	if (find_file(file))
		{
		Sfile = strdup(file);
		return Sfile;
		}

	/* Try $HOME/sfile */
	/* Try $HOME/setup/sfile */
	env = getenv("HOME");
	if (!blank(env))
		{
		dir  = env;
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}

		dir  = pathname(env, "setup");
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}
		}

	/* Try $FPA_SETUP_DIR/sfile */
	env = getenv("FPA_SETUP_DIR");
	if (!blank(env))
		{
		dir  = env;
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}

		}

	/* Try $FPA/setup/sfile */
	/* Try $FPA/config/sfile */
	/* Try $FPA/config/setup/sfile */
	env = getenv("FPA");
	if (!blank(env))
		{
		dir  = pathname(env, "setup");
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}

		dir  = pathname(env, "config");
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}

		dir  = pathname(env, "config/setup");
		file = pathname(dir, sfile);
		if (find_file(file))
			{
			Sfile = strdup(file);
			return Sfile;
			}
		}

	return NullString;
	}

/**********************************************************************/

/**********************************************************************/
/** This function constructs the standard base directory.  This
 * directory is normally defined by the environment variable $FPA.
 * If the FPA variable is not defined, then the directory is
 * defined as $HOME.  If the HOME variable is also not defined,
 * then `pwd` is used.
 *
 * @note This function does not depend on the contents of the setup
 * file, and thus may be called prior to define_setup().
 *
 * @note This function does not check whether or not the computed
 * directory exists.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return the name of the standard base directory as a STRING
***********************************************************************/
STRING	base_directory

	(
	)

	{
	STRING	dir;

	/* Only need to compute this once */
	if (!blank(BaseDir)) return BaseDir;

	/* Use $FPA if defined */
	dir = getenv("FPA");
	if (!blank(dir))
		{
		(void) strcpy(BaseDir,dir);
		return BaseDir;
		}

	/* Use $HOME if defined */
	dir = getenv("HOME");
	if (!blank(dir))
		{
		(void) strcpy(BaseDir,dir);
		return BaseDir;
		}

	/* Otherwise use the current directory */
	dir = getcwd(BaseDir, sizeof(DirPath));
	return BaseDir;
	}

/**********************************************************************/

/**********************************************************************/
/**  This function constructs the full name of the directory which
 *   is being used as the "home" directory - the directory to which
 *   all non-absolute filenames will be computed relative to.  This
 *   will either be the "home" directory, if defined in the
 *   "directories" block, or the base directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return The full name of the directory which is being used as the
 * "home directory".
 **********************************************************************/
STRING	home_directory

	(
	)

	{
	STRING	dir;

	/* Only need to compute this once */
	if (!blank(HomeDir)) return HomeDir;

	/* Use the "home" directory if defined in the "directories" block */
	dir = get_directory("home");
	if (!blank(dir))
		{
		(void) strcpy(HomeDir,dir);
		return HomeDir;
		}

	/* Otherwise use the base directory */
	dir = base_directory();
	(void) strcpy(HomeDir,dir);
	return HomeDir;
	}

/**********************************************************************/

/**********************************************************************/
/**  This function constructs the full name of the directory from
 *   which the application will be running.  This will be either the
 *   "home" directory, if defined in the "directories" block, or the
 *   current directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return The full path name of the working directory of the
 * application.
 **********************************************************************/
STRING	work_directory

	(
	)

	{
	STRING	dir;

	/* Only need to compute this once */
	if (!blank(WorkDir)) return WorkDir;

	/* Use the "home" directory if defined in the "directories" block */
	dir = get_directory("home");
	if (!blank(dir))
		{
		(void) strcpy(WorkDir,dir);
		return WorkDir;
		}

	/* Otherwise use the current directory */
	dir = getcwd(WorkDir, sizeof(DirPath));
	return WorkDir;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ s e t u p                                           *
*    c u r r e n t _ s e t u p _ l i s t                               *
*    f i n d _ s e t u p _ b l o c k                                   *
*    s e t u p _ b l o c k _ l i n e                                   *
*                                                                      *
***********************************************************************/
static	const	STRING	Comment    = "*#";	/* comment line character(s) */
static			int		NumSetup   = 0;		/* number of setup files */
static			STRING	*SetupFile = NullStringList;
											/* setup file names */
static			FILE	*FpSetup   = NullPtr(FILE *);
											/* current setup file pointer */
static			SCLine	SetupLine  = "";	/* general line reading buffer */
static	const	size_t	Ncsl       = sizeof(SCLine)-1;	/* size of above - 1 */
static			LOGICAL	BlockEmpty = TRUE;	/* Is the current block empty? */
static			STRING	BlockName  = NullString;
											/* Current block name */
static			int		BlockSetup = 0;		/* Current setup file number */

/**********************************************************************/
/** This function examines the setup file refered to by the given
 *  list of setup files.  If the files exist and appear to be bona
 *  fide setup files, the setup file names are then saved.  It then
 *  reads the setup block which defines the directory paths.
 *  A chdir() is then done to the directory defined by the "home"
 *  key, if found.  This means that all relative directory and file
 *  paths will be interpretted with respect to this home directory.
 *
 *	@param[in]	nsetup	number of setup files
 *	@param[out]	*setup	list of setup file names
 *  @return True if successful.
 **********************************************************************/
int		define_setup

	(
	int			nsetup,
	STRING		*setup
	)

	{
	int		i;
	STRING	hdir;
	STRING	rdir;
	STRING	arg;
	STRING	rev;

	/* Make sure the given setup file list is legal */
	if ( (nsetup <= 0) || !setup )
		{
		(void) pr_error("Environ", "No setup files specified.\n");
		return FALSE;
		}

	/* Don't allow a re-definition, but ignore the same list */
	if ( NumSetup > 0 )
		{
		if (nsetup == NumSetup)
			{
			for (i=0; i<NumSetup; i++)
				{
				if (!same(setup[i], SetupFile[i])) break;
				}
			if( i >= NumSetup ) return TRUE;
			}

		(void) pr_error("Environ", "Attempt to override setup list.\n");
		return FALSE;
		}

	/* Make sure the given setup files exist and are readable */
	for (i=0; i<nsetup; i++)
		{
		/* Make sure the file exists */
		if ( !find_file(setup[i]) )
			{
			(void) pr_error("Environ",
					"Setup file not found: \"%s\".\n", setup[i]);
			return FALSE;
			}

		/* Make sure the file is readable */
		FpSetup = fopen(setup[i],"r");
		if ( !FpSetup )
			{
			(void) pr_error("Environ",
					"Setup file not readable: \"%s\".\n", setup[i]);
			return FALSE;
			}

#		ifndef SKIP_REVISION
		/* Search for first uncommented line */
		if (!getvalidline(FpSetup,SetupLine,Ncsl,Comment))
			{
			(void) pr_error("Environ", "Setup file \"%s\" empty.\n", setup[i]);
			(void) fclose(FpSetup);
			FpSetup = NullPtr(FILE *);
			return FALSE;
			}

		/* Must be a revision line */
		arg = string_arg(SetupLine);
		if (same(arg, "revision"))
			{
			rev = string_arg(SetupLine);
			}
		else
			{
			rev = FpaOldestRev;
			(void) pr_warning("Environ",
					"Setup file \"%s\" missing revision - assuming %s.\n",
					setup[i], rev);
			}

		/* Make sure the revision matches the software revision */
		if (newer_revision(FpaRevision, rev))
			{
			(void) pr_error("Environ",
					"Unsupported revision %s in setup \"%s\".\n",
					rev, setup[i]);
			(void) fclose(FpSetup);
			FpSetup = NullPtr(FILE *);
			return FALSE;
			}
#		endif /* SKIP_REVISION */

		/* Looks OK, close it till we need it */
		(void) fclose(FpSetup);
		FpSetup = NullPtr(FILE *);
		}

	/* Store the setup file list */
	NumSetup  = nsetup;
	SetupFile = INITMEM(STRING, nsetup);
	(void) pr_status("Environ", "Revision: %s.\n", FpaRevision);
	for (i=0; i<nsetup; i++)
		{
		SetupFile[i] = strdup(setup[i]);
		(void) pr_status("Environ", "Setup%d: \"%s\".\n", i, setup[i]);
		}

	/* Set last character in line buffer to ensure we never miss end */
	SetupLine[Ncsl] = '\0';

	/* Make sure home and work directories are recomputed */
	(void) strcpy(HomeDir,"");
	(void) strcpy(WorkDir,"");

	/* Change to base directory of the FPA directory structure */
	hdir = home_directory();
	rdir = work_directory();
	if ( chdir(rdir) != 0 )
		{
		(void) pr_error("Environ",
				"Cannot access working directory: \"%s\".\n", rdir);
		return FALSE;
		}

	/* Force read of "advanced_features" block */
	(void) adv_feature(NULL);

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** This function returns the full path of the current setup files.
 *
 *	@param[out]	**list	list of full path names
 *  @return The size of the setup file list.
 **********************************************************************/
int		current_setup_list

	(
	STRING		**list
	)

	{
	if (list) *list = SetupFile;
	return NumSetup;
	}

/**********************************************************************/

/**********************************************************************/
/** This function scans the setup file for the setup block defined
 *  by the given block key.  If the block is found, the setup file
 *  is left open with the file pointer pointing to the beginning of
 *  the first line of the block, and TRUE is returned.  If not found,
 *  the setup file is closed and FALSE is returned.
 *
 *	@param[in]	block_key	keyword which identifies block in setup file
 *	@param[in]	required	complain if required and not found
 *  @return True if block is found, False otherwise.
 **********************************************************************/
int		find_setup_block

	(
	STRING		block_key,
	LOGICAL		required
	)

	{
	int		i;
	STRING	ptr;

	BlockEmpty = TRUE;
	BlockSetup = 0;
	FREEMEM(BlockName);

	/* Cannot work until setup list has been defined */
	if ( !SetupFile )
		{
		(void) pr_error("Environ", "Setup file not yet defined.\n");
		return FALSE;
		}

	/* If already open - close it */
	if ( FpSetup )
		{
		(void) fclose(FpSetup);
		FpSetup = NullPtr(FILE *);
		}

	/* Open the setup files in sequence until we find the requested block */
	for (i=0; i<NumSetup; i++)
		{
		LOGICAL	inside_block = FALSE;
		LOGICAL found_header = FALSE;

		FpSetup = fopen(SetupFile[i],"r");
		if ( !FpSetup )
			{
			(void) pr_error("Environ", "Cannot open setup%d.\n", i);
			continue;
			}

		/* Scan for beginning of requested block */
		while ( getvalidline(FpSetup,SetupLine,Ncsl,Comment) )
			{
			while ( !blank(ptr = string_arg(SetupLine)) )
				{
				if (found_header)
					{
					/* Ignore lines after block header until left bracket */
					if ( *ptr == '{' )
						{
						(void) pr_status("Environ",
								"Accessing \"%s\" setup block in setup%d.\n",
								block_key, i);
						BlockSetup = i;
						BlockName  = strdup(block_key);
						return TRUE;
						}
					}
				else if (inside_block)
					{
					/* Ignore lines inside wrong block until right bracket */
					if ( *ptr == '}' ) inside_block = FALSE;
					}
				else if ( same(block_key,ptr) )
					{
					/* We found the right block header */
					found_header = TRUE;
					}
				else if ( *ptr == '{' )
					{
					/* We are inside the wrong block */
					inside_block = TRUE;
					}
				}
			}
		}

	/* Didn't find the requested block */
	if (required)
		{
		(void) pr_error("Environ", "Cannot find \"%s\" setup block.\n",
						block_key);
		}
	else
		{
		(void) pr_diag("Environ", "Cannot find \"%s\" setup block.\n",
						block_key);
		}
	(void) fclose(FpSetup);
	FpSetup = NullPtr(FILE *);
	return FALSE;
	}

/**********************************************************************/

/**********************************************************************/
/** This function reads the next valid line of the current setup
 *  block in the setup file.  It is assumed that the function
 *  find_setup_block() has already been called.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return If the next valid line happens to be the terminating brace
 * "}" of the setup block, or the end-of-file is reached, the setup
 * file is closed and Null is returned. Otherwise the next valid line
 * of the current setup block in the setup file.
 **********************************************************************/
STRING	setup_block_line()
	{
	STRING	buffer;
	size_t	size, rem;

	/* Cannot work until setup list has been defined */
	if ( !SetupFile )
		{
		(void) pr_error("Environ", "Setup file not yet defined.\n");
		return NullString;
		}

	/* Assume no more lines in setup block if setup file not open */
	if ( !FpSetup ) return NullString;

	/* Read the next line */
	buffer = SetupLine;
	size   = 0;
	rem    = Ncsl - size;
	while( getvalidline(FpSetup,buffer,rem,Comment) )
		{
		if( SetupLine[0] == '}' ) break;
		size = strlen(SetupLine);
		if( size <= 0 ) break;
		BlockEmpty = FALSE;

		/* If line ends with a '\' replace it with a blank and append */
		/* the next line (if space allows) */
		if( SetupLine[size-1] == '\\' )
			{
			SetupLine[size-1] = ' ';
			buffer = SetupLine + size;
			rem    = Ncsl - size;
			if( rem > 0 ) continue;
			}

		/* Otherwise just return the line with language substitutions made */
		strip_language_tokens(SetupLine);
		return SetupLine;
		}

	/* End of block or end of file */
	(void) fclose(FpSetup);
	FpSetup = NullPtr(FILE *);

	/* Check if block was empty */
	if (BlockEmpty && BlockSetup<NumSetup-1)
		{
		(void) pr_warning("Environ",
				"Empty \"%s\" setup block in setup%d.\n",
				BlockName, BlockSetup);
		}
	BlockEmpty = TRUE;
	FREEMEM(BlockName);

	/* Return the line, if any, with language substitutions made */
	strip_language_tokens(SetupLine);
	return (size > 0)? SetupLine: NullString;
	}

/***********************************************************************
*                                                                      *
*    a p p _ s e r v i c e                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Lookup the service, as found in /etc/services, for the given
 *  named application, from the setup file.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return The service for the given named application, from the setup
 * file.
 *
 **********************************************************************/

STRING	app_service

	(
	STRING		name
	)

	{
	STRING line;
	static STRING service = NullString;

	if(service) FREEMEM(service);
	if( find_setup_block("services", FALSE) )
		{
		while( line = setup_block_line() )
			{
			if(!same(name,string_arg(line))) continue;
			service = strdup_arg(line);
			break;
			}
		}
	return service;
	}

/***********************************************************************
*                                                                      *
*    o p e n _ c o n f i g _ f i l e                                   *
*    c l o s e _ c o n f i g _ f i l e                                 *
*    c o n f i g _ f i l e _ l i n e                                   *
*                                                                      *
***********************************************************************/

/* Local variables for reading config files */
static			STRING	CfgFile = NullString;
											/* current config file name */
static			FILE	*FpCfg  = NullPtr(FILE *);
											/* config file pointer */
static			SCLine	CfgLine = "";		/* general line reading buffer */
static	const	int		Nccl    = sizeof(SCLine)-1;
											/* size of above - 1 */

/**********************************************************************/
/** This function opens the config file identified by the given
 *  keyword in the "config_files" block of the setup file.
 *
 *  If the keyword is found and the corresponding config file exists
 *  and is readable, the config file is openned.  Then its contents
 *  are searched for a "revision" line.
 *
 *	@param[in]	key			config file keyword
 *  @return If the revision matches the software revision
 *  		(found in config.h), then the file is left open with the
 *  		file pointer pointing to the beginning of the next line,
 *  		and TRUE is returned. If the file is not found, or cannot
 *  		be opened, or does not have the correct revision, nothing
 *  		is opened and FALSE is returned.
 **********************************************************************/
int		open_config_file

	(
	STRING		key
	)

	{
	STRING	line, arg, rev;

	/* If a config file is already open - close it */
	close_config_file();

	/* Construct the name of the requested config file */
	CfgFile = config_file_name(key);
	if ( !CfgFile )
		{
		(void) pr_error("Environ",
				"\"%s\" config file not defined in setup.\n", key);
		return FALSE;
		}

	/* Now try to open it */
	FpCfg = fopen(CfgFile,"r");
	if ( !FpCfg )
		{
		(void) pr_error("Environ",
				"Cannot open \"%s\" config file: \"%s\".\n", key, CfgFile);
		CfgFile = NullString;
		return FALSE;
		}

#	ifndef SKIP_REVISION
	/* Search for first uncommented line */
	line = config_file_line();
	if (blank(line))
		{
		(void) pr_error("Environ", "\"%s\" config file empty.\n", key);
		close_config_file();
		return FALSE;
		}

	/* Must be a revision line */
	arg = string_arg(line);
	if (same(arg, "revision"))
		{
		rev = string_arg(line);
		}
	else
		{
		/* If no revision line, assume default and re-open the file */
		rev = FpaOldestRev;
		(void) pr_warning("Environ",
				"\"%s\" config file missing revision - assuming %s.\n",
				key, rev);
		close_config_file();
		FpCfg = fopen(CfgFile,"r");
		}

	/* Make sure the revision matches the software revision */
	if (newer_revision(FpaRevision, rev))
		{
		(void) pr_error("Environ",
				"\"%s\" config file revision %s not supported.\n", key, rev);
		close_config_file();
		return FALSE;
		}
#	endif /* SKIP_REVISION */

	/* We made it - leave the file open */
	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** This function closes the currently open config file.
 **********************************************************************/
void	close_config_file

	(
	)

	{
	/* If a config file is already open - close it */
	if ( FpCfg )
		{
		(void) fclose(FpCfg);
		FpCfg   = NullPtr(FILE *);
		CfgFile = NullString;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** This function reads the next valid line of the current config
 *  file.  It is assumed that the function open_config_file() has
 *  already been called.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return If the end-of-file is reached, the config file is closed
 * and Null is returned. Otherwise the next valid line is returned.
 **********************************************************************/
STRING	config_file_line

	(
	)

	{
	STRING	buffer;
	size_t	size, rem;

	/* Assume no more lines in config file if not open */
	if ( !FpCfg || !CfgFile ) return NullString;

	/* Read the next line */
	buffer = CfgLine;
	size   = 0;
	rem    = Nccl - size;
	while( getvalidline(FpCfg,buffer,rem,Comment) )
		{
		if( CfgLine[0] == '}' ) break;
		size = strlen(CfgLine);
		if( size <= 0 ) break;
		if( CfgLine[size-1] != '\\' )
			{
			strip_language_tokens(CfgLine);
			return CfgLine;
			}

		/* Line ends with a '\' */
		/* Replace it with a blank and append the next line */
		CfgLine[size-1] = ' ';
		buffer = CfgLine + size;
		rem    = Nccl - size;
		if( rem <= 0 )
			{
			strip_language_tokens(CfgLine);
			return CfgLine;
			}
		}

	/* End of block or end of file */
	(void) fclose(FpCfg);
	FpCfg   = NullPtr(FILE *);
	CfgFile = NullString;
	strip_language_tokens(CfgLine);
	return (size > 0)? CfgLine: NullString;
	}

/***********************************************************************
*                                                                      *
*     c o n f i g _ f i l e _ n a m e                                  *
*     r e a d _ c f i l e _ s e t u p                                  *
*                                                                      *
***********************************************************************/

/* Local variables for saving the "config_files" setup block */
static	int		read_cfile_setup(void);
static	int		CfgSetup = FALSE;				/* has it been read yet ? */
static	TABLE	*Cfg     = NullPtr(TABLE *);	/* config file name table */
static	int		Ncfg     = 0;					/* number of config files */

/**********************************************************************/

/**********************************************************************/
/** Search for the full path name of a config file.
 *
 *	@param[in]	key			config file keyword to search for
 * 	@return the full name of the config file identified by the
 * 			given keyword in the "config_files" setup block.
 **********************************************************************/
STRING	config_file_name

	(
	STRING		key
	)

	{
	int i;

	if ( !read_cfile_setup() ) return NullString;

	for (i=0; i<Ncfg; i++)
		{
		if ( same(key,Cfg[i].index) ) return Cfg[i].value;
		}
	return NullString;
	}

/**********************************************************************/

void	report_config_files

	(
	FILE	*fp
	)

	{
	int		icfg;
	STRING	key, cfg;

	if ( !read_cfile_setup() ) return;

	(void) fprintf(fp, "FPA Config File Assignments:\n");

	for (icfg=0; icfg<Ncfg; icfg++)
		{
		key = Cfg[icfg].index;
		cfg = Cfg[icfg].value;

		(void) fprintf(fp, "%15s: %s\n", key, cfg);
		}
	}

/**********************************************************************/

static	int		read_cfile_setup

	(
	)

	{
	int     i, n, ndir, ncfg;
	STRING	line, cdir, arg, dir;

	if ( CfgSetup ) return TRUE;

	/* Find the "config" directory from the "directories" setup block */
	/* If not found, use the "home" directory */
	/* If still not found, use the current directory */
	cdir = get_directory("config");
	if ( !cdir ) cdir = get_directory("home");
	if ( !cdir ) cdir = ".";

	Ncfg = 0;
	if ( find_setup_block("config_files", TRUE) )
		{
		/* Store the full configuration file names */
		while( line = setup_block_line() )
			{
			Ncfg++;
			Cfg = GETMEM(Cfg,TABLE,Ncfg);
			Cfg[Ncfg-1].index = strdup_arg(line);
			Cfg[Ncfg-1].value = NullString;

			arg = strenv_arg(line);
			if ( blank(arg) ) continue;

			dir = pathname(cdir,arg);
			Cfg[Ncfg-1].value = INITSTR(dir);
			}
		}

	/* Merge in the default path keys and directory names */
	ncfg = Ncfg;
	ndir = (sizeof(default_config_dirs)/sizeof(default_config_dirs[0]));
	for(i = 0; i < ndir; i++)
	{
		for(n = 0; n < ncfg; n++)
		{
			if(same(Cfg[n].index, default_config_dirs[i].index)) break;
		}
		if(n >= ncfg)
		{
			Ncfg++;
			Cfg = GETMEM(Cfg,TABLE,Ncfg);
			Cfg[Ncfg-1].index = default_config_dirs[i].index;
			arg = env_sub(default_config_dirs[i].value);
			dir = pathname(cdir,arg);
			Cfg[Ncfg-1].value = INITSTR(dir);
		}
	}

	CfgSetup = TRUE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g e t _ d i r e c t o r y                                        *
*     g e t _ p a t h                                                  *
*     r e a d _ d i r e c t o r y _ s e t u p                          *
*                                                                      *
***********************************************************************/

static			int		read_directory_setup(void);
static			int		DirSetup = FALSE;		/* has it been read yet ? */
static			TABLE	*Dirs    = NullPtr(TABLE *);
												/* list of directories */
static			int		Ndirs    = 0;			/* number of directories */

/**********************************************************************/

/**********************************************************************/
/** Search for the full name of a directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	key			directory keyword to search for
 * 	@return The full path name of the directory identified by the given
 * 			keyword in the "directories" block.
 **********************************************************************/
STRING	get_directory

	(
	STRING		key
	)

	{
	int		i;

	if ( !read_directory_setup() ) return NullString;
	if ( blank(key) )              return NullString;

	/* Search for directory keywords from setup files */
	for (i=0; i<Ndirs; i++)
		{
		if ( same(key,Dirs[i].index) ) return Dirs[i].value;
		}

	/* Return Null if keyword not found */
	(void) fprintf(stderr, "\n[get_directory] Directory keyword \"%s\"", key);
	(void) fprintf(stderr, " not found in setup file!\n");
	return NullString;
	}

/**********************************************************************/

/**********************************************************************/
/** Assemble the full path name of a file given a directory keyword.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	key 		path key for get_directory
 *	@param[in]	fname		file name in directory
 * 	@return the full path name of the given file in the directory
 * 			identified by the given keyword in the "directories"
 * 			setup block.
 **********************************************************************/
STRING	get_path

	(
	STRING		key,
	STRING		fname
	)

	{
	STRING	dir;

	dir = get_directory(key);
	if (!dir) return NullString;

	return pathname(dir,fname);
	}

/**********************************************************************/

/**********************************************************************/
/** Search for the full path name of a file given a directory keyword.
 * And confirm that it exists.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	key 		path key for get_directory
 *	@param[in]	fname		file name in directory
 * 	@return The full name of the given file in the directory identified
 * 			by the given keyword in the "directories" setup block but
 * 			only if the file exists. If it does not try the default
 * 			directory entry in read_setup.h. If this is not found
 * 			either return NULL.
 **********************************************************************/
STRING	get_file

	(
	STRING		key,
	STRING		fname
	)

	{
	int     i, ndef;
	char    buf[2048];
	STRING	dir, fn, wdir;
	struct	stat stbuf;

	fn = get_path(key,fname);
	if(stat(fn,&stbuf) == 0) return fn;

	/* If we get here the file was not found so try default directory */
	ndef = sizeof(default_dirs) / sizeof(default_dirs[0]);
	for(i = 0; i < ndef; i++)
		{
		if(!same(key,default_dirs[i].index)) continue;

		/* Try the first default directory */
		env_compute(default_dirs[i].value1, buf, 2048);
		wdir = base_directory();
		dir = (wdir) ? pathname(wdir,buf) : buf;
		fn = pathname(dir,fname);
		if(stat(fn,&stbuf) == 0) return fn;

		/* If we get here the first default failed so try second */
		if(default_dirs[i].value2)
			{
			env_compute(default_dirs[i].value2, buf, 2048);
			wdir = base_directory();
			dir = (wdir) ? pathname(wdir,buf) : buf;
			fn = pathname(dir,fname);
			if(stat(fn,&stbuf) == 0) return fn;
			}
		break;
		}
	return NullString;
	}

/**********************************************************************/

void	report_directories

	(
	FILE	*fp
	)

	{
	int		idir, ndef, idef;
	STRING	wdir, key, dir, def1, def2;
	char	buf[2048];

	if ( !read_directory_setup() ) return;

	ndef = sizeof(default_dirs) / sizeof(default_dirs[0]);
	wdir = base_directory();

	(void) fprintf(fp, "FPA Directory Assignments:\n");

	for (idir=0; idir<Ndirs; idir++)
		{
		key = Dirs[idir].index;
		dir = Dirs[idir].value;

		(void) fprintf(fp, "%15s: %s\n", key, dir);

		for(idef = 0; idef<ndef; idef++)
			{
			if (!same(key, default_dirs[idef].index)) continue;

			def1 = default_dirs[idef].value1;
			def2 = default_dirs[idef].value2;

			if (!blank(def1))
				{
				env_compute(def1, buf, 2048);
				dir = (wdir) ? pathname(wdir,buf) : buf;
				if (!same(dir, Dirs[idir].value))
					(void) fprintf(fp, "                 %s\n", dir);
				}
			if (!blank(def2))
				{
				env_compute(def2, buf, 2048);
				dir = (wdir) ? pathname(wdir,buf) : buf;
				if (!same(dir, Dirs[idir].value))
					(void) fprintf(fp, "                 %s\n", dir);
				}
			}
		}
	}

/**********************************************************************/

static	int		read_directory_setup

	(
	)

	{
	int     i, n, ndir, ndef;
	char    buf[2048];
	STRING	line, key, arg, wdir;

	/* Only do this once */
	if ( DirSetup ) return TRUE;
	Ndirs = 0;

	/* Assume all relative directories are relative to $FPA and */
	/* make all paths absolute */
	wdir = base_directory();

	/* Read the "directories" block and look for a "home" key */
	if ( find_setup_block("directories", TRUE) )
		{
		while ( line = setup_block_line() )
			{
			/* Read the keyword and directory from the current line */
			key = strdup_arg(line);

			/* Check for "home" key */
			if(!same(key,"home"))
				{
				FREEMEM(key);
				continue;
				}

			arg = string_arg(line);
			env_compute(arg, buf, 2048);

			/* If we must use absolute paths, append the current entry */
			/* to the working directory */
			arg = (wdir) ? pathname(wdir,buf) : buf;

			/* Add the entry in the directory list */
			Ndirs = 1;
			Dirs  = INITMEM(TABLE,1);
			Dirs[0].index = key;
			Dirs[0].value = wdir = INITSTR(arg);
			break;
			}
		}

	/* Read the "directories" block again except for the "home" key */
	if ( find_setup_block("directories", TRUE) )
		{
		while ( line = setup_block_line() )
			{
			key = strdup_arg(line);

			/* Skip the "home" key */
			if(same(key,"home"))
				{
				FREEMEM(key);
				continue;
				}

			arg = string_arg(line);
			env_compute(arg, buf, 2048);
			arg = (wdir) ? pathname(wdir,buf) : buf;

			Ndirs++;
			Dirs = GETMEM(Dirs,TABLE,Ndirs);
			Dirs[Ndirs-1].index = key;
			Dirs[Ndirs-1].value = INITSTR(arg);
			}
		}

	/* Merge the default directories into the data */
	ndef = (sizeof(default_dirs)/sizeof(default_dirs[0]));
	ndir = Ndirs;
	for(i = 0; i < ndef; i++)
		{
		for(n = 0; n < ndir; n++)
			{
			if(same(Dirs[n].index, default_dirs[i].index)) break;
			}
		if(n >= ndir)
			{
			/* Default entry not found - add to directories struct */
			env_compute(default_dirs[i].value1, buf, 2048);
			arg = (wdir) ? pathname(wdir,buf) : buf;

			Ndirs++;
			Dirs = GETMEM(Dirs,TABLE,Ndirs);
			Dirs[Ndirs-1].index = default_dirs[i].index;
			Dirs[Ndirs-1].value = INITSTR(arg);
			}
		}

	DirSetup = (LOGICAL) (Ndirs > 0);
	return DirSetup;
	}
