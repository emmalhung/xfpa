/**********************************************************************/
/** @file read_config.c
 *
 * Routines for reading new Version 4.0 configuration files
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e a d _ c o n f i g . c                                          *
*                                                                      *
*   Routines for reading new Version 4.0 configuration files           *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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
************************************************************************/

#define READ_CONFIG_INIT	/* To initialize declarations in read_config.h */

#include "revision.h"
#include "read_setup.h"
#include "config_structs.h"
#include "read_config.h"

#include <tools/tools.h>
#include <fpa_macros.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <string.h>
#include <stdio.h>

/* Interface functions                     */
/*  ... these are defined in read_config.h */

/* Internal static functions (Configuration File Push and Pop) */
static LOGICAL	push_config_file_name(STRING);
static LOGICAL	push_config_file_pointer(FILE **, STRING);
static LOGICAL	pop_config_file_name(void);
static LOGICAL	pop_config_file_pointer(FILE **);
static LOGICAL	current_config_file_name(STRING *);

/* Local variables for reading configuration files */
typedef	char	SCLine[FPAC_MAX_LENGTH];
static			SCLine	CfgLine = "";		/* general line reading buffer */
static			SCLine	CfgFNam = "";		/* general file name buffer */
static	const	STRING	Comment	= "#";		/* comment line character(s) */
static	const	int		Nccl    = sizeof(SCLine) - 1;
											/* size of general buffers - 1 */

/***********************************************************************
*                                                                      *
*   f i r s t _ c o n f i g _ f i l e _ o p e n                        *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function finds and opens a configuration file identified by
 * name in the user defined setup block.
 *
 * @param[in]	name		configuration file name in setup block
 * @param[out]	**fpcfg		configuration file pointer
 * @return If successful, the file is left open with the file pointer
 * pointing to the next line of the file, and TRUE is returned.
 * If the file is not found, or if the file cannot be opened, the
 * file is not opened, and FALSE is returned.
 **********************************************************************/
LOGICAL				first_config_file_open

(
 STRING		name,
 FILE		**fpcfg
)

{
  STRING		cfgfile;

  /* Get the user defined configuration file name from the setup block */
  cfgfile = config_file_name(name);

  if ( blank(cfgfile) )
  {
	(void) pr_error("Environ",
					"\"%s\" config file not defined in setup\n", name);
	return FALSE;
  }

  /* Open the configuration file */
  if ( !config_file_open(cfgfile, fpcfg) )
  {
	(void) pr_error("Environ",
					"\"%s\" config file unknown or not found\n", cfgfile);
	return FALSE;
  }

  /* Return TRUE if configuration file found and opened */
  return TRUE;
}

/***********************************************************************
*                                                                      *
*   c o n f i g _ f i l e _ o p e n                                    *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function opens a named configuration file and searches for
 * a "revision" line.
 *
 *	@param[in]	cfgname	configuration file name
 *	@param[out]	**fpcfg	configuration file pointer
 *  @return If the "revision" line is found, and the revision matches
 *  		the current software revision, the file is left open with
 *  		the file pointer pointing to the beginning of the next line,
 *  		and TRUE is returned. If the file is not found, or if the
 *  		file cannot be opened, or if the revision does not match
 *  		the current software revision, the file is not opened, and
 *  		FALSE is returned.
 **********************************************************************/
LOGICAL				config_file_open

	(
	STRING		cfgname,
	FILE		**fpcfg
	)

	{
	STRING		line, rev;
	FILE		*fp = NullPtr(FILE *);

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Initialize the  configuration file pointer */
	*fpcfg = NullPtr(FILE *);

	/* Check if the configuration file has already been opened */
	/*  by trying to push the file name onto the stack         */
	if ( !push_config_file_name(cfgname) )
		{
		(void) pr_error("Environ",
				"\"%s\" config file is already open\n", cfgname);
		return FALSE;
		}

	/* Try to open the named configuration file */
	if ( IsNull( fp = fopen(cfgname, "r") ) )
		{
		(void) pr_error("Environ",
				"\"%s\" config file cannot be opened\n", cfgname);
		(void) pop_config_file_name();
		return FALSE;
		}

#	ifndef SKIP_REVISION
	/* Search for first uncommented line */
	line = read_config_file_line(&fp);
	if ( blank(line) )
		{
		(void) pr_error("Environ", "\"%s\" config file empty\n", cfgname);
		(void) config_file_close(&fp);
		return FALSE;
		}

	/* Must be a revision line of form "revision <revision_number>" */
	if ( same_ic(string_arg(line), FpaCrevisionLine) )
		{
		rev = string_arg(line);
		}
	else
		{
		(void) pr_warning("Environ",
				"\"%s\" config file missing revision - assuming %s\n",
				cfgname, FpaOldestRev);
		rev = FpaOldestRev;
		(void) rewind(fp);
		}

	/* Make sure the revision matches the software revision */
	if ( newer_revision(FpaRevision, rev) )
		{
		(void) pr_error("Environ",
				"\"%s\" config file revision %s not supported\n",
				cfgname, rev);
		(void) config_file_close(&fp);
		return FALSE;
		}
#	endif /* SKIP_REVISION */

	/* Set pointer to configuration file and return TRUE */
	*fpcfg = fp;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   c o n f i g _ f i l e _ c l o s e                                  *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function closes a currently open configuration file.
 *
 *	@param[in]	**fpcfg		configuration file pointer
 **********************************************************************/

void				config_file_close

	(
	FILE		**fpcfg
	)

	{
	/* If the configuration file is open, pop the configuration */
	/*  file name off the stack and close the file              */
	if ( NotNull(fpcfg) )
		{
		(void) pop_config_file_name();
		(void) fclose(*fpcfg);
		*fpcfg = NullPtr(FILE *);
		}
	}

/***********************************************************************
*                                                                      *
*   r e a d _ c o n f i g _ f i l e _ l i n e                          *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function reads the next valid line of the current
 * configuration file, identified by a file pointer.
 * (The configuration file is opened with a call to the function
 * If an "include" line is encountered, the function attempts to
 * read from the configuration file identified by the "include" line.
 * If an end-of-file is encountered, the function closes the
 * present configuration file and returns to reading the previous
 * configuration file.
 *
 * @note (The next to last return may contain some left over bits of a
 * line if there are problems reading the last line.)
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	**fpcfg		configuration file pointer
 * 	@return When there are no further previous configuration files,
 * 			the function returns a Null. Otherwise it contains the
 * 			next valid config file line.
 **********************************************************************/

STRING				read_config_file_line

	(
	FILE		**fpcfg
	)

	{
	STRING	buffer;
	int		size, rem;

	/* Return Null if no configuration file pointer */
	if ( IsNull(fpcfg) || IsNull(*fpcfg) ) return NullString;

	/* Reset the line reading buffer */
	size   = 0;
	buffer = CfgLine + size;
	rem    = Nccl - size;

	/* Continue reading until a valid line is encountered and returned */
	/*  ... including reading any "include" configuration files        */
	while ( TRUE )
		{

		/* Read the next line of the present configuration file */
		if ( getvalidline(*fpcfg, buffer, rem, Comment)
				&& (size = strlen(CfgLine)) > 0 )
			{

			/* Line ends with a '\' (line continuation character!)      */
			/*  ... so replace it with a blank and append the next line */
			if ( CfgLine[size-1] == '\\' )
				{

				/* Set the line reading buffer to append the next line */
				CfgLine[size-1] = ' ';
				buffer = CfgLine + size;
				rem    = Nccl - size;

				/* Read and append the next line if space left in buffer */
				if ( rem > 0 ) continue;

				/* Error message if no space left in buffer */
				else
					{
					(void) pr_error("Environ",
							"Line in config file too long: %s\n", CfgLine);
					}
				}

			/* Normal line (or line continuation that ran out of space!) */
			/*  ... strip off language tokens                            */
			(void) strip_language_tokens(CfgLine);

			/* Check if line is an "include" */
			if ( same_start_ic(CfgLine, FpaCincludeFile) )
				{

				/* Read the line (destructively) to get the configuration */
				/*  file name (the format should be "include <filename>") */
				/* Note that the name is stashed in the static variable   */
				/*  CfgFNam to prevent overwriting!                       */
				(void) string_arg(CfgLine);
				(void) strcpy(CfgFNam, strenv_arg(CfgLine));

				/* Push the present configuration file pointer onto the stack */
				/*  and try to open the "include" configuration file          */
				/* We do not care if this works, since the routine returns a  */
				/*  pointer to one configuration file or the other anyways!   */
				(void) push_config_file_pointer(fpcfg, CfgFNam);

				/* Reset the line reading buffer to read the next line */
				size    = 0;
				buffer  = CfgLine + size;
				rem     = Nccl - size;
				continue;
				}

			/* Return the line */
			return CfgLine;
			}

		/* Invalid line or last line read from this configuration file */
		else
			{

			/* Pop the present configuration file off the stack and return a */
			/*  pointer to the previous one ... if there is a previous one!  */
			if ( pop_config_file_pointer(fpcfg) )
				{

				/* Reset the line reading buffer to read the next line */
				/*  of the previous configuration file                 */
				size    = 0;
				buffer  = CfgLine + size;
				rem     = Nccl - size;
				continue;
				}

			/* There is no previous configuration file, so exit now */
			else
				{
				break;
				}
			}
		}

	/* End of block or end of file                               */
	/*  ... so return any left over bits that may have been read */
	(void) strip_language_tokens(CfgLine);
	return ( size > 0 )? CfgLine: NullString;
	}

/***********************************************************************
*                                                                      *
*   s k i p _ c o n f i g _ f i l e _ b l o c k                        *
*   s k i p _ t o _ e n d _ o f _ b l o c k                            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function reads consecutive lines of a configuration file,
 * keeping track of the open and close brackets to identify when
 * a complete block of data has been read.
 *
 *	@param[in]	**fpcfg		configuration file pointer
 * 	@return True if successful.
 **********************************************************************/
LOGICAL				skip_config_file_block

	(
	FILE		**fpcfg
	)

	{
	long int	position;
	int			numbrace;
	STRING		line, arg;

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Save the position in the configuration file to return to */
	/*  the same place if there is no block to skip!            */
	position = ftell(*fpcfg);

	/* Read the next line of the configuration file */
	line = read_config_file_line(fpcfg);

	/* Extract the first argument from the line */
	arg = string_arg(line);

	/* Reposition file to re-read current line if no block to skip! */
	if ( !same(arg, FpaCopenBrace) )
		{
		(void) fseek(*fpcfg, position, SEEK_SET);
		return TRUE;
		}

	/* Continue reading the configuration file line by line */
	/*  ... until end of block is encountered               */
	numbrace = 1;
	while ( NotNull( line = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		arg = string_arg(line);

		/* Increment counter for open brackets */
		if ( same(arg, FpaCopenBrace) ) numbrace++;

		/* Decrement counter for close brackets */
		else if ( same(arg, FpaCcloseBrace) ) numbrace--;

		/* Return when enclosing brackets match */
		if ( numbrace == 0 ) return TRUE;

		/* Error if unbalanced close brackets */
		if ( numbrace < 0 )
			{
			(void) pr_error("Environ",
					"#################################################\n");
			(void) pr_error("Environ",
					"### Unbalanced close brackets in config file! ###\n");
			(void) pr_error("Environ",
					"#################################################\n");
			return FALSE;
			}
		}

	/* Error message if unbalanced open brackets               */
	/*  ... end-of-file encountered before end of block found! */
	(void) pr_error("Environ",
			"################################################\n");
	(void) pr_error("Environ",
			"### Unbalanced open brackets in config file! ###\n");
	(void) pr_error("Environ",
			"################################################\n");
	return FALSE;
	}


/**********************************************************************/
/** This function reads consecutive lines of a configuration file,
 * keeping track of the open and close brackets to identify the end
 * of a partially read block of data.
 *
 *	@param[in]	**fpcfg		configuration file pointer
 * 	@return True if successful.
 **********************************************************************/
LOGICAL				skip_to_end_of_block

	(
	FILE		**fpcfg
	)

	{
	long int	position;
	int			numbrace;
	STRING		line, arg;

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Save the current position in the configuration file */
	position = ftell(*fpcfg);

	/* Read the configuration file line by line       */
	/*  ... until end of current block is encountered */
	numbrace = 1;
	while ( NotNull( line = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		arg = string_arg(line);

		/* Increment counter for open brackets */
		if ( same(arg, FpaCopenBrace) ) numbrace++;

		/* Decrement counter for close brackets */
		else if ( same(arg, FpaCcloseBrace) ) numbrace--;

		/* Reposition file to re-read last line at end of current block */
		/*  ... and note that last line will be close brackets!         */
		if ( numbrace == 0 )
			{
			(void) fseek(*fpcfg, position, SEEK_SET);
			return TRUE;
			}

		/* Error if unbalanced close brackets */
		if ( numbrace < 0 )
			{
			(void) pr_error("Environ",
					"#################################################\n");
			(void) pr_error("Environ",
					"### Unbalanced close brackets in config file! ###\n");
			(void) pr_error("Environ",
					"#################################################\n");
			return FALSE;
			}

		/* Save the position of next line in the configuration file */
		position = ftell(*fpcfg);
		}

	/* Error message if unbalanced open brackets               */
	/*  ... end-of-file encountered before end of block found! */
	(void) pr_error("Environ",
			"################################################\n");
	(void) pr_error("Environ",
			"### Unbalanced open brackets in config file! ###\n");
	(void) pr_error("Environ",
			"################################################\n");
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   c o n f i g _ f i l e _ l o c a t i o n                            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function returns the current configuration file name and
 * location in the file of the current file pointer.
 *
 *	@param[in]	*fpcfg		configuration file pointer
 *	@param[out]	*cfgname	pointer to configuration file name
 *	@param[out] *position	pointer to configuration file location
 *  @return True if successful.
 **********************************************************************/
LOGICAL				config_file_location

	(
	FILE		*fpcfg,
	STRING		*cfgname,
	long int	*position
	)

	{

	/* Initialize the  configuration file name and position */
	if ( NotNull(cfgname) )  *cfgname  = NullString;
	if ( NotNull(position) ) *position = 0;

	/* Return FALSE if no currently open configuration file */
	if ( !current_config_file_name(cfgname) ) return FALSE;

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Set the position in the configuration file */
	if ( NotNull(position) ) *position = ftell(fpcfg);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Configuration File Push and Pop)        *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   p u s h _ c o n f i g _ f i l e _ n a m e                          *
*   p u s h _ c o n f i g _ f i l e _ p o i n t e r                    *
*   p o p _ c o n f i g _ f i l e _ n a m e                            *
*   p o p _ c o n f i g _ f i l e _ p o i n t e r                      *
*   c u r r e n t _ c o n f i g _ f i l e _ n a m e                    *
*                                                                      *
*   These functions keep track of filenames and pointers of            *
*   embedded reading of configuration files.                           *
*                                                                      *
***********************************************************************/

/* Storage locations for pointers to known configuration files */
static	int		NumConfigKnown    = 0;
static	STRING	*KnownConfigNames = NullStringList;

/* Storage locations for pointers to current configuration files */
static	int		NumConfigFiles = 0;
static	int		MaxConfigFiles = 0;
static	STRING	*ConfigNames   = NullStringList;
static	FILE	**ConfigFiles  = NullPtr(FILE **);

/**********************************************************************/

static	LOGICAL		push_config_file_name

	(
	STRING		cfgname		/* configuration file name */
	)

	{
	int		nn;
	STRING	cfg;

	/* Return FALSE if no configuration file name passed */
	if ( blank(cfgname) ) return FALSE;

	/* Identify configuration file name from known list */
	for ( nn=0; nn<NumConfigKnown; nn++ )
		{
		if ( same(cfgname, KnownConfigNames[nn]) )
			{
			cfg = KnownConfigNames[nn];
			break;
			}
		}

	/* Add configuration file name to known list */
	if ( nn >= NumConfigKnown )
		{
		(void) pr_status("Environ", "Accessing config file \"%s\"\n", cfgname);
		NumConfigKnown++;
		KnownConfigNames = GETMEM(KnownConfigNames, STRING, NumConfigKnown);
		KnownConfigNames[NumConfigKnown-1] = strdup(cfgname);
		cfg = KnownConfigNames[NumConfigKnown-1];
		}

	/* Check that configuration file is not already on stack */
	for ( nn=0; nn<NumConfigFiles; nn++ )
		if ( same(cfg, ConfigNames[nn]) ) return FALSE;

	/* Add the present name to the stack */
	NumConfigFiles++;
	if ( NumConfigFiles > MaxConfigFiles )
		{
		MaxConfigFiles = NumConfigFiles;
		ConfigNames = GETMEM(ConfigNames, STRING, NumConfigFiles);
		ConfigFiles = GETMEM(ConfigFiles, FILE *, NumConfigFiles);
		}
	ConfigNames[NumConfigFiles-1] = cfg;
	ConfigFiles[NumConfigFiles-1] = NullPtr(FILE *);

	/* Return TRUE if all went well */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL		push_config_file_pointer

	(
	FILE		**fpcfg,	/* pointer to configuration file */
	STRING		cfgname		/* configuration file name */
	)

	{
	FILE		*fpnew = NullPtr(FILE *);

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Try to open named configuration file                                */
	/* Note that this will push the configuration file name onto the stack */
	if ( config_file_open(cfgname, &fpnew) )
		{

		/* Paranoid check to make sure we have enough files open! */
		if ( NumConfigFiles < 2 )
			{
			(void) pr_error("Environ",
					"Impossible error in push_config_file_pointer!\n");
			return FALSE;
			}

		/* Push the pointer to the present configuration file onto the stack */
		ConfigFiles[NumConfigFiles-2] = *fpcfg;

		/* Reset pointer to the new configuration file */
		*fpcfg = fpnew;
		}

	/* Keep present pointer and return FALSE      */
	/*  if configuration file could not be opened */
	return FALSE;
	}

/**********************************************************************/

static	LOGICAL		pop_config_file_name

	(
	)

	{

	/* Paranoid check to make sure we have enough files open! */
	if ( NumConfigFiles < 1 )
		{
		(void) pr_error("Environ",
				"Impossible error in pop_config_file_name!\n");
		return FALSE;
		}

	/* Take the present configuration file name and pointer off the stack */
	NumConfigFiles--;
	ConfigNames[NumConfigFiles] = NullString;
	ConfigFiles[NumConfigFiles] = NullPtr(FILE *);

	/* Return TRUE if all went well */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL		pop_config_file_pointer

	(
	FILE		**fpcfg		/* pointer to configuration file */
	)

	{

	/* Return FALSE if no configuration file pointer passed */
	if ( IsNull(fpcfg) ) return FALSE;

	/* Close present configuration file and reset configuration file pointer */
	(void) config_file_close(fpcfg);
	*fpcfg = NullPtr(FILE *);

	/* Return FALSE if no more configuration files on the stack */
	if ( NumConfigFiles <= 0 ) return FALSE;

	/* Take the previous configuration file pointer off the stack */
	/*  and return TRUE                                           */
	*fpcfg = ConfigFiles[NumConfigFiles-1];
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL		current_config_file_name

	(
	STRING		*cfgname	/* pointer to configuration file name */
	)

	{

	/* Return FALSE if no configuration file is currently open */
	if ( NumConfigFiles < 1 )
		{
		(void) pr_error("Environ",
				"Error in current_config_file_name ... no file open!\n");
		if ( NotNull(cfgname) ) *cfgname = NullString;
		return FALSE;
		}

	/* Set pointer to current configuration file and return TRUE */
	if ( NotNull(cfgname) ) *cfgname = ConfigNames[NumConfigFiles - 1];
	return TRUE;
	}
