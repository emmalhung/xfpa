/***********************************************************************
*                                                                      *
*     g r a _ i o . c                                                  *
*                                                                      *
*     Routines to handle FpaGPgen input and output files               *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
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

#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <string.h>
#include <stdio.h>

/* Define "white space" */
#define WHITE	" \t\n\r\f"

/* Common directories */
static	char	HomeDir[GPGLong]        = FpaCblank;
static	char	BasePdfDir[GPGLong]     = FpaCblank;
static	char	BaseOutDir[GPGLong]     = FpaCblank;

/* Directories and filenames for fpdf files */
static	char	PdfDir[GPGLong]         = FpaCblank;
static	char	DefaultPdfDir[GPGLong]  = FpaCblank;
static	char	PdfFile[GPGLong]        = FpaCblank;
static	char	DefaultPdfFile[GPGLong] = FpaCblank;

/* Characters for reading fpdf files */
static	LOGICAL	IsLineComplete = TRUE;
static	char	CommentChar    = '!';
static	char	OpenBrace      = '{';
static	char	CloseBrace     = '}';
static	char	EqualSign      = '=';
static	char	SemiColon      = ';';
static	char	OpenAngle      = '<';
static	char	CloseAngle     = '>';

/* Directories and filenames for output files */
static	char	OutDir[GPGLong]         = FpaCblank;
static	char	DefaultOutDir[GPGLong]  = FpaCblank;
static	char	OutFile[GPGLong]        = FpaCblank;
static	char	DefaultOutFile[GPGLong] = FpaCblank;

/* Information for writing output files */
static	LOGICAL	NotInitialized          = TRUE;
static	LOGICAL	FirstTextWrite          = TRUE;
static	LOGICAL	FirstFontWrite          = TRUE;
static	char	HeaderBuf[GPGLong]      = FpaCblank;
static	char	*OutBuf                 = NullChar;
static	FILE	*FP_Out                 = NullPtr(FILE *);
static	char	OutputFileName[GPGLong] = FpaCblank;

/* Error reporting counters and buffers */
static	int				NumErrorBuf = 0;
static	int				MaxErrorBuf = 0;
static	DirectiveBuffer	*ErrorBufList = NullPtr(DirectiveBuffer *);


/* Structure for holding keywords for @loop_... directives */
typedef struct
	{
	int		num_vals;
	STRING	*values;
	} LOOP_KEYVALS;
typedef struct
	{
	STRING			group_name;
	int				num_keys;
	STRING			*keywords;
	LOOP_KEYVALS	*keyvals;
	LOOP_KEYVALS	*defvals;
	} LOOP_GROUP;
typedef struct
	{
	int			num_iterations;
	int			current_iteration;
	int			num_groups;
	LOOP_GROUP	*loop_groups;
	SET			fld_set;
	STRING		fld_attrib;
	STRING		fld_vtime;
	FILE		*file_pointer;
	long int	file_position;
	} LOOP_STRUCT;

/* Storage for @loop_... parameters */
static	int			NumLoops = 0;
static	LOOP_STRUCT	*Loops   = NullPtr(LOOP_STRUCT *);


/* Array for holding keywords for all directives */
typedef char ListDef[GPGMedium];


/* Structure for holding keywords for @groups directives */
typedef struct
	{
	char		group_name[GPGShort];

	ListDef		*list;
	int			num;
	int			maxnum;
	} KEY_GROUP;

/* Storage for @groups parameters */
static	int			NumGroups = 0;
static	KEY_GROUP	*Groups   = NullPtr(KEY_GROUP *);


/* Internal static functions */
static	void	write_psmet_stroke(PRES);
static	void	write_psmet_fill(PRES);
static	void	write_psmet_interior_fill(PRES);
static	void	write_psmet_font(PRES, float);
static	void	write_svgmet_stroke(PRES);
static	void	write_svgmet_fill(PRES);
static	void	write_svgmet_interior_fill(PRES);
static	void	write_svgmet_font(PRES, float, STRING);
static	void	check_svgmet_text(STRING);
static	void	write_cormet_pres_outline(PRES);
static	void	write_cormet_pres_fill(PRES);
static	void	write_cormet_pres_interior_fill(PRES);
static	void	write_cormet_pres_font(PRES);
static	void	transform_cormet_ellipse(float, float, float, float, char *);
static	void	transform_cormet_rectangle(float, float, float, float, char *);
static	void	transform_cormet_bitmap(FILE *, float, float, float, char *);
static	STRING	fgets_no_lead(FILE *, STRING, size_t);
static	int		dissect_body(STRING, ListDef **list, int *);
static	void	replace_specials(STRING);
static	void	getback_specials(STRING);
static	LOGICAL	set_version(char list[][GPGMedium], int);
static	LOGICAL	set_file_name(char list[][GPGMedium], int);
static	STRING	expand_dir_codewords(STRING);
static	STRING	expand_name_codewords(STRING);
static	STRING	expand_file_codewords(STRING);
static	LOGICAL	run_external_process(char list[][GPGMedium], int);
static	STRING	expand_process_codewords(STRING);
static	LOGICAL	insert_into_svg(STRING);
static	STRING	expand_insert_codewords(STRING);
static	LOGICAL	include_pdf_file(char list[][GPGMedium], int);
static	LOGICAL	set_loop_parameters(char list[][GPGMedium], int, FILE *);
static	LOGICAL	reset_loop(LOGICAL *, FILE **, long int *);
static	LOGICAL	set_loop_location_lookup(char list[][GPGMedium], int);
static	LOGICAL	add_group(char list[][GPGMedium], int);
static	LOGICAL	set_group_parameters(LOOP_GROUP, int);
static	LOGICAL	set_field_parameters(SET, STRING, STRING, int);

/***********************************************************************
*                                                                      *
*    i n i t i a l i z e _ p s m e t _  d i s p l a y                  *
*                                                                      *
*    i n i t i a l i z e _ s v g m e t _  d i s p l a y                *
*                                                                      *
*    i n i t i a l i z e _ c o r m e t _  d i s p l a y                *
*                                                                      *
*    i n i t i a l i z e _ t e x m e t _  d i s p l a y                *
*                                                                      *
***********************************************************************/

void		initialize_psmet_display

	(
	)

	{
	float		width, height, axis;

	/* Set default display units as inches */
	(void) define_graphics_units(DisplayUnitsInches, 100.0);
	(void) fprintf(stdout, " Default display units ... %s times %f\n",
			DisplayUnits.type, DisplayUnits.conversion);

	/* Initialize page size as 11.0 by 8.5 inches */
	width  = 11.0 * DisplayUnits.conversion;
	height =  8.5 * DisplayUnits.conversion;
	(void) initialize_graphics_size(width, height);

	/* Initialize map as 8 inches along longest axis ... centered on page */
	axis = 8.0 * DisplayUnits.conversion;
	(void) define_graphics_placement(0.0, axis, ScaleLongest, 0.0, 0.0);

	/* Initialize anchor location to centre of current map */
	(void) define_graphics_anchor(AnchorMap, 0.0, 0.0, 0.0, 0.0, FpaCblank);

	/* Set default presentation parameters */
	(void) default_graphics_presentation();
	}

void		initialize_svgmet_display

	(
	)

	{
	float		width, height, axis;

	/* Set default display units as inches */
	(void) define_graphics_units(DisplayUnitsInches, 100.0);
	(void) fprintf(stdout, " Default display units ... %s times %f\n",
			DisplayUnits.type, DisplayUnits.conversion);

	/* Initialize page size as 11.0 by 8.5 inches */
	width  = 11.0 * DisplayUnits.conversion;
	height =  8.5 * DisplayUnits.conversion;
	(void) initialize_graphics_size(width, height);

	/* Initialize map as 8 inches along longest axis ... centered on page */
	axis = 8.0 * DisplayUnits.conversion;
	(void) define_graphics_placement(0.0, axis, ScaleLongest, 0.0, 0.0);

	/* Initialize anchor location to centre of current map */
	(void) define_graphics_anchor(AnchorMap, 0.0, 0.0, 0.0, 0.0, FpaCblank);

	/* Set default presentation parameters */
	(void) default_graphics_presentation();
	}

void		initialize_cormet_display

	(
	)

	{
	float		width, height, axis;

	/* Set default display units as inches */
	(void) define_graphics_units(DisplayUnitsInches, 100.0);
	(void) fprintf(stdout, " Default display units ... %s times %f\n",
			DisplayUnits.type, DisplayUnits.conversion);

	/* Initialize page size as 11.0 by 8.5 inches */
	width  = 11.0 * DisplayUnits.conversion;
	height =  8.5 * DisplayUnits.conversion;
	(void) initialize_graphics_size(width, height);

	/* Initialize map as 8 inches along longest axis ... centered on page */
	axis = 8.0 * DisplayUnits.conversion;
	(void) define_graphics_placement(0.0, axis, ScaleLongest, 0.0, 0.0);

	/* Initialize anchor location to centre of current map */
	(void) define_graphics_anchor(AnchorMap, 0.0, 0.0, 0.0, 0.0, FpaCblank);

	/* Set default presentation parameters */
	(void) default_graphics_presentation();
	}

void		initialize_texmet_display

	(
	)

	{

	/* Initialize anchor location to first column and row of file */
	(void) define_graphics_anchor(AnchorAbsolute, 0.0, 0.0, 0.0, 0.0,
																	FpaCblank);

	/* Set default presentation parameters */
	(void) default_graphics_presentation();
	}

/***********************************************************************
*                                                                      *
*    i n i t i a l i z e _ g r a p h i c s _ d i r e c t o r i e s     *
*                                                                      *
***********************************************************************/

LOGICAL		initialize_graphics_directories

	(
	STRING		gra_dir,
	STRING		pdf_name
	)

	{
	STRING		dir;

	/* Check for the "home" directory in the setup file */
	dir = get_directory("home");
	if ( blank(dir) )
		{
		(void) pr_error("FpaGPgen", " No \"home\" directory in setup file!\n");
		return FALSE;
		}

	/* Set the home directory */
	(void) strcpy(HomeDir, dir);

	/* Check for the base fpdf file directory in the setup file */
	dir = get_directory(Program.pdf_tag);
	if ( blank(dir) )
		{
		(void) pr_error("FpaGPgen", " No \"%s\" directory in setup file!\n",
				Program.pdf_tag);
		return FALSE;
		}

	/* Set the base fpdf directory */
	(void) strcpy(BasePdfDir, dir);

	/* Build the fpdf directories ...                     */
	/*  using the base fpdf directory from the setup file */
	/*  and the application directory from the run string */
	(void) strcpy(PdfDir, dir);
	(void) strcat(PdfDir, "/");
	(void) strcat(PdfDir, gra_dir);

	/* Set the default fpdf directory and filename */
	(void) strcpy(DefaultPdfDir,  PdfDir);
	(void) strcpy(DefaultPdfFile, pdf_name);

	/* Check for the base output file directory in the setup file */
	dir = get_directory(Program.out_tag);
	if ( blank(dir) )
		{
		(void) pr_error("FpaGPgen", " No \"%s\" directory in setup file!\n",
				Program.out_tag);
		return FALSE;
		}

	/* Set the base output directory */
	(void) strcpy(BaseOutDir, dir);

	/* Build the output directories ...                     */
	/*  using the base output directory from the setup file */
	/*  and the application directory from the run string   */
	(void) strcpy(OutDir, dir);
	(void) strcat(OutDir, "/");
	(void) strcat(OutDir, gra_dir);

	/* Set the default output directory and filename */
	(void) strcpy(DefaultOutDir,  OutDir);
	(void) strcpy(DefaultOutFile, pdf_name);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    f i n d _ p d f _ f i l e                                         *
*    f i n d _ l o o k u p _ f i l e                                   *
*    f i n d _ s y m b o l _ f i l e                                   *
*    f i n d _ t e x t _ f i l e                                       *
*    f i n d _ d a t a _ f i l e                                       *
*                                                                      *
***********************************************************************/

STRING		find_pdf_file

	(
	STRING		name			/* fpdf file name */
	)

	{
	STRING			exname, fullname;

	static	char	filename[GPGLong] = FpaCblank;

	/* Expand the fpdf file name if macros are used */
	exname = env_sub(name);
	if ( blank(exname) ) return FpaCblank;

	/* Build the full pathname using the current fpdf directory */
	fullname = pathname(PdfDir, exname);
	if ( blank(fullname) ) return FpaCblank;

	/* Return the full pathname if "fpdf" extension already there */
	(void) strcpy(filename, fullname);
	if ( strstr(filename, ".fpdf") )
		{
		if ( find_file(filename) )
			return filename;
		else
			return FpaCblank;
		}

	/* Error message if "pdf" extension already there */
	else if ( strstr(filename, ".pdf") )
		{
		if ( find_file(filename) )
			{
			/* >>> error message for obsolete pdf extension          <<< */
			/* >>> this is an OldVersion and ObsoleteVersion feature <<< */
			(void) fprintf(stderr,"\n******** W A R N I N G ********\n");
			(void) fprintf(stderr, ">>> Warning - Obsolete \".pdf\" file");
			(void) fprintf(stderr, " extension for file\n");
			(void) fprintf(stderr, ">>>   \"%s\"\n", filename);
			(void) fprintf(stderr, ">>> Please rename file using \".fpdf\"");
			(void) fprintf(stderr, " file extension!");
			(void) fprintf(stderr,"\n**** E N D * W A R N I N G ****\n");
			return filename;
			}
		else
			return FpaCblank;
		}

	/* Return the full pathname after adding the "fpdf" or "pdf" extension */
	else
		{

		/* Add the "fpdf" extension and check for the filename */
		(void) strcpy(filename, fullname);
		(void) strcat(filename, ".fpdf");
		if ( find_file(filename) ) return filename;

		/* Add the "pdf" extension and check for the filename */
		(void) strcpy(filename, fullname);
		(void) strcat(filename, ".pdf");
		if ( find_file(filename) )
			{
			/* >>> error message for obsolete pdf extension          <<< */
			/* >>> this is an OldVersion and ObsoleteVersion feature <<< */
			(void) fprintf(stderr,"\n******** W A R N I N G ********\n");
			(void) fprintf(stderr, ">>> Warning - Obsolete \".pdf\" file");
			(void) fprintf(stderr, " extension for file\n");
			(void) fprintf(stderr, ">>>   \"%s\"\n", filename);
			(void) fprintf(stderr, ">>> Please rename file using \".fpdf\"");
			(void) fprintf(stderr, " file extension!");
			(void) fprintf(stderr,"\n**** E N D * W A R N I N G ****\n");
			return filename;
			}
		}

	/* File not found */
	return FpaCblank;
	}

STRING		find_lookup_file

	(
	STRING		name,			/* lookup file name */
	STRING		ext				/* lookup file extension */
	)

	{
	STRING			exname, fullname;

	static	char	filename[GPGLong] = FpaCblank;

	/* Expand the lookup file name if macros are used */
	exname = env_sub(name);
	if ( blank(exname) ) return FpaCblank;

	/* Build the full pathname using the current fpdf directory */
	fullname = pathname(PdfDir, exname);
	if ( blank(fullname) ) return FpaCblank;

	/* Add the lookup file extension (if not already there!) */
	(void) strcpy(filename, fullname);
	if ( !strstr(filename, ext) ) (void) strcat(filename, ext);

	/* Return the filename */
	return filename;
	}

STRING		find_symbol_file

	(
	STRING		name
	)

	{
	STRING			exname, symname;

	static	char	filename[GPGLong]    = FpaCblank;

	/* Expand the symbol file name if macros are used */
	exname = env_sub(name);
	if ( blank(exname) ) return FpaCblank;

	/* Add the appropriate extension (if not already there!)     */
	/*  and search for the file in the "..._symbols" directories */
	(void) strcpy(filename, exname);
	switch ( Program.macro )
		{
		case GPG_PSMet:
			if ( !strstr(filename, ".ps") )  (void) strcat(filename, ".ps");
			symname = get_file("psmet_symbols", filename);
			return ( !blank(symname) ) ? symname: FpaCblank;
		case GPG_SVGMet:
			if ( !strstr(filename, ".svg") )  (void) strcat(filename, ".svg");
			symname = get_file("svgmet_symbols", filename);
			return ( !blank(symname) ) ? symname: FpaCblank;
		case GPG_CorMet:
			if ( !strstr(filename, ".cmf") ) (void) strcat(filename, ".cmf");
			symname = get_file("cormet_symbols", filename);
			return ( !blank(symname) ) ? symname: FpaCblank;
		}

	/* Error return for unknown program types */
	return FpaCblank;
	}

STRING		find_text_file

	(
	STRING		name
	)

	{
	STRING			rxname, exname, fullname;

	static	char	filename[GPGLong] = FpaCblank;

	/* Replace magic directory keywords */
	rxname = expand_file_codewords(name);

	/* Expand the text file name if macros are used */
	exname = env_sub(rxname);
	if ( blank(exname) ) return FpaCblank;

	/* Build the full pathname using the current fpdf directory */
	fullname = pathname(PdfDir, exname);
	if ( blank(fullname) ) return FpaCblank;
	(void) strcpy(filename, fullname);

	/* Return the filename */
	return filename;
	}

STRING		find_data_file

	(
	STRING		name
	)

	{
	STRING			rxname, exname, fullname;

	static	char	filename[GPGLong]    = FpaCblank;

	/* Replace magic directory keywords */
	rxname = expand_file_codewords(name);

	/* Expand the data file name if macros are used */
	exname = env_sub(rxname);
	if ( blank(exname) ) return FpaCblank;

	/* Build the full pathname using the current fpdf directory */
	fullname = pathname(PdfDir, exname);
	if ( blank(fullname) ) return FpaCblank;
	(void) strcpy(filename, fullname);

	/* Return the full pathname */
	return filename;
	}

/***********************************************************************
*                                                                      *
*    o p e n _ g r a p h i c s _ f i l e                               *
*                                                                      *
***********************************************************************/

LOGICAL		open_graphics_file

	(
	STRING		out_dir,
	STRING		out_file
	)

	{
	LOGICAL		created;
	STRING		exname;

	static	char	filename[GPGLong] = FpaCblank;
	static	char	basedir[GPGLong]  = FpaCblank;

	/* Close the current output file (if one is already open!) */
	if ( NotNull(FP_Out) )
		{
		(void) close_graphics_file();
		(void) strcpy(OutputFileName, FpaCblank);
		}

	/* Use the default output directory if no directory given */
	if ( blank(out_dir) )
		{
		(void) strcpy(OutDir, DefaultOutDir);
		}

	/* Otherwise, expand the output directory (if macros are used) */
	else
		{
		exname = env_sub(out_dir);
		if ( blank(exname) ) return FALSE;;
		(void) strcpy(OutDir, exname);
		}

	/* Use the default output filename if no filename given */
	if ( blank(out_file) )
		{
		(void) strcpy(OutFile, DefaultOutFile);
		}

	/* Otherwise, expand the output filename (if macros are used) */
	else
		{
		exname = env_sub(out_file);
		if ( blank(exname) ) return FALSE;;
		(void) strcpy(OutFile, exname);
		}

	/* Build the full pathname using the output directory and filename */
	exname = pathname(OutDir, OutFile);
	if ( blank(exname) ) return FALSE;;

	/* Add the appropriate extension (if not already there!) */
	(void) strcpy(filename, exname);
	switch ( Program.macro )
		{
		case GPG_PSMet:
			if ( !strstr(filename, ".ps") )  (void) strcat(filename, ".ps");
			break;
		case GPG_SVGMet:
			if ( !strstr(filename, ".svg") )  (void) strcat(filename, ".svg");
			break;
		case GPG_CorMet:
			if ( !strstr(filename, ".cmf") ) (void) strcat(filename, ".cmf");
			break;
		case GPG_TexMet:
			if ( !strstr(filename, ".txt") ) (void) strcat(filename, ".txt");
			break;
		}

	/* Now try to create the output directory */
	(void) strcpy(basedir, dir_name(filename));
	if ( !create_directory(basedir, S_IRWXU|S_IRWXG|S_IRWXO, &created) )
		{
		(void) fprintf(stderr, " Cannot create directory ... \"%s\"\n",
				basedir);
		return FALSE;
		}
	else if ( created )
		{
		(void) fprintf(stdout, " Creating directory ... \"%s\"\n", basedir);
		}

	/* Now try to open the output file */
	if ( IsNull( FP_Out = fopen(filename, "w") ) )
		{
		(void) strcpy(OutputFileName, FpaCblank);
		(void) fprintf(stderr, " Cannot open file ... \"%s\"\n", filename);
		return FALSE;
		}

	(void) strcpy(OutputFileName, filename);
	(void) fprintf(stdout, " Outputting to file ... %s\n", filename);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    c l o s e _ p s m e t _ f i l e                                   *
*    i n i t i a l i z e _ p s m e t _ s i z e                         *
*                                                                      *
*    c l o s e _ s v g m e t _ f i l e                                 *
*    i n i t i a l i z e _ s v g m e t _ s i z e                       *
*                                                                      *
*    c l o s e _ c o r m e t _ f i l e                                 *
*    i n i t i a l i z e _ c o r m e t _ s i z e                       *
*                                                                      *
*    c l o s e _ t e x t _ f i l e                                     *
*    i n i t i a l i z e _ t e x m e t _ s i z e                       *
*                                                                      *
***********************************************************************/

void		close_psmet_file

	(
	)

	{

	/* Return immediately if no output file open to close */
	if ( IsNull(FP_Out) ) return;

	/* End the grouping in the current output file */
	(void) write_graphics_group(GPGend, NullPointer, 0);
	(void) fprintf(FP_Out, "%%### End of grouping for fpdf directives\n");

	/* Display symbols from  @legend  directive */
	(void) display_legend();

	/* Display the output file graphics */
	(void) fprintf(FP_Out, "%%### Display graphics\n");
	(void) fprintf(FP_Out, "showpage\n");

	/* Close the current output file */
	(void) fclose(FP_Out);
	FP_Out = NullPtr(FILE *);

	/* Reset default presentation parameters */
	/* >>> should this be here at all?!? <<< */
	(void) default_graphics_presentation();

	/* Reset the file initialization checks */
	NotInitialized = TRUE;
	FirstTextWrite = TRUE;
	FirstFontWrite = TRUE;
	}

LOGICAL		initialize_psmet_size

	(
	float		width,
	float		height
	)

	{
	char		err_buf[GPGLong];

	/* Error report if error in width or height */
	if ( width <= 0.0 || height <= 0.0 )
		{
		(void) sprintf(err_buf, "Error in width or height ... %f %f",
				width, height);
		(void) error_report(err_buf);
		}

	/* Set page width and height (global variables) */
	PageWidth  = width;
	PageHeight = height;

	/* Initialize postscript file header to size of page */
	(void) sprintf(HeaderBuf,
			"%%!PS-Adobe-2.0-for-FPA-V5 PSMet_size[0.000 %.3f %.3f 0.000]",
			PageHeight, PageWidth);

	/* Return TRUE if all went well */
	return TRUE;
	}

void		close_svgmet_file

	(
	)

	{

	/* Return immediately if no output file open to close */
	if ( IsNull(FP_Out) ) return;

	/* End the grouping in the current output file */
	(void) write_graphics_group(GPGend, NullPointer, 0);
	(void) fprintf(FP_Out, "<!-- End of grouping for fpdf directives -->\n");

	/* Display symbols from  @legend  directive */
	(void) display_legend();

	/* Display the output file graphics */
	(void) fprintf(FP_Out, "<!-- Display graphics -->\n");
	(void) fprintf(FP_Out, "</svg>\n");

	/* Close the current output file */
	(void) fclose(FP_Out);
	FP_Out = NullPtr(FILE *);

	/* Reset default presentation parameters */
	/* >>> should this be here at all?!? <<< */
	(void) default_graphics_presentation();

	/* Reset the file initialization checks */
	NotInitialized = TRUE;
	FirstTextWrite = TRUE;
	FirstFontWrite = TRUE;
	}

LOGICAL		initialize_svgmet_size

	(
	float		width,
	float		height
	)

	{
	char		err_buf[GPGLong];

	/* Error report if error in width or height */
	if ( width <= 0.0 || height <= 0.0 )
		{
		(void) sprintf(err_buf, "Error in width or height ... %f %f",
				width, height);
		(void) error_report(err_buf);
		}

	/* Set page width and height (global variables) */
	PageWidth  = width;
	PageHeight = height;

	/* Initialize SVG file header to size of page */
	/* >>> EMMA TESTING NEW HEADER FORMAT
	(void) sprintf(HeaderBuf,
			"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><!-- SVGMet_size[0 %.3f %.3f 0] --> <svg version=\"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%.3f\" height=\"%.3f\" >",
			PageHeight, PageWidth, PageWidth, PageHeight);
	*/
	(void) sprintf(HeaderBuf, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?> <!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"> <svg version=\"1.1\" baseProfile=\"full\" width=\"100%%\" height=\"100%%\" viewBox=\"0 0 %.3f %.3f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" >" , PageWidth, PageHeight);

	/* Return TRUE if all went well */
	return TRUE;
	}

void		close_cormet_file

	(
	)

	{

	/* Return immediately if no output file open to close */
	if ( IsNull(FP_Out) ) return;

	/* End the grouping in the current output file */
	(void) write_graphics_group(GPGend, NullPointer, 0);
	(void) fprintf(FP_Out, "%%### End of grouping for fpdf directives\n");

	/* Display symbols from  @legend  directive */
	(void) display_legend();

	/* Close the current output file */
	(void) fclose(FP_Out);
	FP_Out = NullPtr(FILE *);

	/* Reset default presentation parameters */
	/* >>> should this be here at all?!? <<< */
	(void) default_graphics_presentation();

	/* Reset the file initialization check */
	NotInitialized = TRUE;
	}

LOGICAL		initialize_cormet_size

	(
	float		width,
	float		height
	)

	{
	char		err_buf[GPGLong];

	static	int		CorelVersionNumber = 101;

	/* Error report if error in width or height */
	if ( width <= 0.0 || height <= 0.0 )
		{
		(void) sprintf(err_buf, "Error in width or height ... %f %f",
				width, height);
		(void) error_report(err_buf);
		}

	/* Set page width and height (global variables) */
	PageWidth  = width;
	PageHeight = height;

	/* Initialize CMF file header from size of page */
	(void) sprintf(HeaderBuf, "@CorelMF %d %.0f %.0f %.0f %.0f",
			CorelVersionNumber, -(PageWidth/2.0), (PageHeight/2.0),
			(PageWidth/2.0), -(PageHeight/2.0));

	/* Return TRUE if all went well */
	return TRUE;
	}

void		close_texmet_file

	(
	)

	{
	int			ii, jj;

	/* Return immediately if no output file open to close */
	if ( IsNull(FP_Out) ) return;

	/* Write out the current output buffer row by row */
	for ( ii=0; ii<Tny; ii++ )
		{

		/* Write each row column by column */
		for ( jj=0; jj<Tnx; jj++ )
			{
			(void) fprintf(FP_Out, "%c", OutBuf[ii*Tnx + jj]);
			}

		/* Write end of line after each row */
		(void) fprintf(FP_Out, "\n");
		}

	/* Re-initialize the current output buffer */
	(void) initialize_graphics_size((float) Tnx, (float) Tny);

	/* Close the current output file */
	(void) fclose(FP_Out);
	FP_Out = NullPtr(FILE *);

	/* Reset default presentation parameters */
	/* >>> should this be here at all?!? <<< */
	(void) default_graphics_presentation();

	/* Reset the file initialization check */
	NotInitialized = TRUE;
	}

LOGICAL		initialize_texmet_size

	(
	float		columns,
	float		rows
	)

	{
	char		err_buf[GPGLong];

	/* Free space in output buffer (if required) */
	if ( NotNull(OutBuf) ) FREEMEM(OutBuf);

	/* Error report if error in number of columns or rows */
	if ( columns <= 0.0 || rows <= 0.0 )
		{
		(void) sprintf(err_buf, "Error in number of columns or rows ... %f %f",
				columns, rows);
		(void) error_report(err_buf);
		}

	/* Set column and row dimensions for new output buffer (global variables) */
	Tnx = NINT(columns);
	Tny = NINT(rows);

	/* Initialize the contents of the new output buffer */
	OutBuf = INITMEM(char, Tnx*Tny);
	(void) memset(OutBuf, ' ', (size_t) Tnx*Tny);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    w r i t e _ p s m e t _ c o m m e n t                             *
*    w r i t e _ p s m e t _ g r o u p                                 *
*    w r i t e _ p s m e t _ b i t m a p                               *
*    w r i t e _ p s m e t _ b o x                                     *
*    w r i t e _ p s m e t _ e l l i p s e                             *
*    w r i t e _ p s m e t _ u n d e r l i n e                         *
*    w r i t e _ p s m e t _ t e x t                                   *
*    w r i t e _ p s m e t _ l i n e s                                 *
*    w r i t e _ p s m e t _ o u t l i n e s                           *
*    w r i t e _ p s m e t _ b o u n d a r i e s                       *
*    w r i t e _ p s m e t _ f e a t u r e s                           *
*    w r i t e _ p s m e t _ s y m b o l                               *
*    w r i t e _ p s m e t _ o u t l i n e _ m a s k                   *
*    w r i t e _ p s m e t _ b o u n d a r y _ m a s k                 *
*                                                                      *
*    w r i t e _ s v g m e t _ c o m m e n t                           *
*    w r i t e _ s v g m e t _ g r o u p                               *
*    w r i t e _ s v g m e t _ b i t m a p                             *
*    w r i t e _ s v g m e t _ b o x                                   *
*    w r i t e _ s v g m e t _ e l l i p s e                           *
*    w r i t e _ s v g m e t _ u n d e r l i n e                       *
*    w r i t e _ s v g m e t _ t e x t                                 *
*    w r i t e _ s v g m e t _ l i n e s                               *
*    w r i t e _ s v g m e t _ o u t l i n e s                         *
*    w r i t e _ s v g m e t _ b o u n d a r i e s                     *
*    w r i t e _ s v g m e t _ f e a t u r e s                         *
*    w r i t e _ s v g m e t _ s y m b o l                             *
*    w r i t e _ s v g m e t _ o u t l i n e _ m a s k                 *
*    w r i t e _ s v g m e t _ b o u n d a r y _ m a s k               *
*                                                                      *
*    w r i t e _ c o r m e t _ c o m m e n t                           *
*    w r i t e _ c o r m e t _ g r o u p                               *
*    w r i t e _ c o r m e t _ b i t m a p                             *
*    w r i t e _ c o r m e t _ b o x                                   *
*    w r i t e _ c o r m e t _ e l l i p s e                           *
*    w r i t e _ c o r m e t _ u n d e r l i n e                       *
*    w r i t e _ c o r m e t _ t e x t                                 *
*    w r i t e _ c o r m e t _ l i n e s                               *
*    w r i t e _ c o r m e t _ o u t l i n e s                         *
*    w r i t e _ c o r m e t _ b o u n d a r i e s                     *
*    w r i t e _ c o r m e t _ f e a t u r e s                         *
*    w r i t e _ c o r m e t _ s y m b o l                             *
*    w r i t e _ c o r m e t _ o u t l i n e _ m a s k                 *
*    w r i t e _ c o r m e t _ b o u n d a r y _ m a s k               *
*    w r i t e _ c o r m e t _ d i r e c t                             *
*                                                                      *
*    w r i t e _ t e x m e t _ c o m m e n t                           *
*    w r i t e _ t e x m e t _ g r o u p                               *
*    w r i t e _ t e x m e t _ t e x t                                 *
*                                                                      *
***********************************************************************/

/* Active symbol file name */
static	char	last_sym_file[GPGLong] = "";

/* These are used by SVGMet */
static	LOGICAL	svgmet_do_clipPath = FALSE;	/* Should we clip symbols? */
static	int		svgmet_clipNum     = 0;		/* Unique ident for clipPaths */

void		write_psmet_comment

	(
	STRING		buf
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized )
		{

		/* Reset the initialization check */
		NotInitialized = FALSE;

		/* Open the default output file (if an output file is not yet open!) */
		if ( IsNull(FP_Out) )
			{
			if ( !open_graphics_file(FpaCblank, FpaCblank) )
				{
				(void) error_report("Error opening default output file");
				}
			}

		/* Write the header information */
		(void) fprintf(FP_Out, "%s\n", HeaderBuf);

		/* Set landscape mode (if required) */
		if ( PageWidth > PageHeight )
			{
			(void) fprintf(FP_Out, "%%### Set landscape mode\n");
			(void) fprintf(FP_Out, "90.0 rotate\n");
			(void) fprintf(FP_Out, " 0.000 %.3f translate\n", -PageHeight);
			}

		/* Begin grouping of output objects */
		(void) fprintf(FP_Out, "%%### Start of grouping for fpdf directives\n");
		(void) write_psmet_group(GPGstart, NullPointer, 0);
		}

	/* Write the output buffer */
	if ( !blank(buf) ) (void) fprintf(FP_Out, "%%%s\n", buf);
	}

void		write_psmet_group

	(
	STRING		buf,
	char		unused_list[][GPGMedium],
	int			unused_num
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Output the start or end group buffer */
	if ( same(buf, GPGstart) )
		(void) fprintf(FP_Out, "%% start group\n");
	else if ( same(buf, GPGend) )
		(void) fprintf(FP_Out, "%% end group\n");
	else
		return;
	}

void		write_psmet_bitmap

	(
	FILE		*unused_symbol_file
	)

	{
	/* >>> comment out for now!
	int		c_bit;
	<<< */

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* <<<<< Return for now >>>>> */
	return;
	}

void		write_psmet_image
	(
	 Image img
	)
	{
		/* display imagery in product */
	unsigned char	*raster;
	int				width, height, x_orig, y_orig, xi;
	glCOLOR			*trans = glImageTransparentPixel();
	char			err_buf[GPGLong];

	if ( !glImageCreateRaster( img, glImagePixelMajor, &raster,
						&x_orig, &y_orig, &width, &height) )
		{
		return;
		}

	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "/DeviceRGB setcolorspace\n");

	/* Image is positioned wrt Lower Left corner of map area */
	/* Because the viewbox is the same size as the map x_orig & y_orig
	 * always seem to be 0.0, so for now I'll ignore them. This
	 * makes more sense in write_svgmet_image
	 */
	(void) fprintf(FP_Out, "%.3f %.3f translate\n", ULpoint[X], LRpoint[Y]);
	(void) fprintf(FP_Out, "%d %d scale\n", width, height);
	(void) fprintf(FP_Out, "<<\n");

	/* Image Type 4 indicates Colour Key Masking */
	(void) fprintf(FP_Out, "	/ImageType 4\n");

	/* Indicate colour or range of colours to mask out as transparent */
	(void) fprintf(FP_Out, "	/MaskColor [%d %d %d]\n",
				   trans->red, trans->green, trans->blue);
	(void) fprintf(FP_Out, "	/Width %d\n", width);
	(void) fprintf(FP_Out, "	/Height %d\n", height);
	(void) fprintf(FP_Out, "	/BitsPerComponent 8\n");

	/* Decode array corresponds to /DeviceRGB colour space */
	(void) fprintf(FP_Out, "	/Decode [0 1 0 1 0 1]\n");

	/* Image origin is centre of raster */
	(void) fprintf(FP_Out, "	/ImageMatrix [%d 0 0 -%d 0 %d]\n",
				   width, height, height);
	(void) fprintf(FP_Out, "	/DataSource currentfile /ASCIIHexDecode filter\n");
	(void) fprintf(FP_Out, ">>\n");
	(void) fprintf(FP_Out, "image\n\n");

	/* Print 2 digit hex code for each colour at each pixel in image */
	for(xi = 0; xi < width*height*3; xi++)
	{
		(void) fprintf(FP_Out, "%.2X", raster[xi]);
		if (!( (xi + 1) % 36)) (void) fprintf(FP_Out, "\n");
	}
	(void) fprintf(FP_Out, "\n>\n");
	(void) fprintf(FP_Out, "grestore\n");
	(void) fprintf(FP_Out, "\n");

	/* Free the memeory for the raster */
	FREEMEM(raster);

	return;
	}

void		write_psmet_box

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* First output the definition of the box */
	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "  newpath\n");
	(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xpos, ypos);
	(void) fprintf(FP_Out, "  %.3f rotate\n", rotation);
	(void) fprintf(FP_Out, "  %.3f %.3f rmoveto\n", -width/2.0, -height/2.0);
	(void) fprintf(FP_Out, "  %.3f %.3f rlineto\n", 0.0, height);
	(void) fprintf(FP_Out, "  %.3f %.3f rlineto\n", width, 0.0);
	(void) fprintf(FP_Out, "  %.3f %.3f rlineto\n", 0.0, -height);
	(void) fprintf(FP_Out, "  closepath\n");

	/* Then display the interior of the box */
	if (do_interior_fill ) (void) write_psmet_interior_fill(CurPres);

	/* Then display the outline of the box */
	if ( do_outline )      (void) write_psmet_stroke(CurPres);

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "grestore\n");
	}

void		write_psmet_ellipse

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		sangle,
	float		eangle,
	LOGICAL		closed,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	float		xscale, yscale, xadj, yadj;
	char		err_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for ellipse with zero width or height */
	if ( width <= 0.0 || height <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with ellipse width ... %.2f  or height ... %.2f",
				width, height);
		(void) warn_report(err_buf);
		return;
		}

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Set scaling factors and adjusted offsets */
	xscale = 1.0;
	yscale = height/width;
	xadj   = xpos / xscale;
	yadj   = ypos / yscale;

	/* First save the current transformation matrix */
	/* This will isolate scaling the ellipse path from displaying the path */
	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "  /origmatrix matrix def\n");
	(void) fprintf(FP_Out, "  origmatrix currentmatrix pop\n");

	/* Then output the inital ellipse position and scaling */
	(void) fprintf(FP_Out, "    newpath\n");
	(void) fprintf(FP_Out, "    %.3f %.3f translate\n", xpos, ypos);
	(void) fprintf(FP_Out, "    %.3f rotate\n", rotation);
	(void) fprintf(FP_Out, "    %.3f %.3f scale\n", xscale, yscale);

	/* Draw a full ellipse */
	if ( sangle == eangle )
		{
		(void) fprintf(FP_Out, "    0.000 0.000 %.3f 0.000 360.000 arc\n",
				width/2.0);
		(void) fprintf(FP_Out, "    closepath\n");
		}

	/* Draw a partial ellipse */
	else if (closed)
		{
		(void) fprintf(FP_Out, "    0.000 0.000 moveto\n");
		(void) fprintf(FP_Out, "    0.000 0.000 lineto\n");
		(void) fprintf(FP_Out, "    0.000 0.000 %.3f %.3f %.3f arc\n",
				width/2.0, sangle, eangle);
		(void) fprintf(FP_Out, "    0.000 0.000 lineto\n");
		(void) fprintf(FP_Out, "    closepath\n");
		}

	/* Draw an elliptical arc */
	else
		{
		(void) fprintf(FP_Out, "    0.000 0.000 %.3f %.3f %.3f arc\n",
				width/2.0, sangle, eangle);
		}

	/* Reset the transformation matrix */
	/* This will ensure the outline is displayed with a constant width */
	(void) fprintf(FP_Out, "  origmatrix setmatrix\n");

	/* Then display the interior of the ellipse */
	if (closed && do_interior_fill )
		(void) write_psmet_interior_fill(CurPres);

	/* Then display the outline of the ellipse */
	if ( do_outline )
		(void) write_psmet_stroke(CurPres);

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "grestore\n");
	}

void		write_psmet_underline

	(
	float		xpos,
	float		ypos,
	float		width,
	float		rotation
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* First output the definition of the underline */
	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "  newpath\n");
	(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xpos, ypos);
	(void) fprintf(FP_Out, "  %.3f rotate\n", rotation);
	(void) fprintf(FP_Out, "  %.3f %.3f rmoveto\n", -width/2.0, 0.0);
	(void) fprintf(FP_Out, "  %.3f %.3f rlineto\n", width, 0.0);

	/* Then display the underline */
	(void) write_psmet_stroke(CurPres);

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "grestore\n");
	}

void		write_psmet_text

	(
	STRING		text,
	float		xpos,
	float		ypos,
	float		txt_size,
	STRING		justified,
	float		rotation,
	LOGICAL		do_outline
	)

	{
	size_t		nlen;
	char		err_buf[GPGLong], tbuf[GPGLong];

	/* Warning message for text size too small */
	if ( txt_size <= 0.0 )
		{
		(void) sprintf(err_buf, "Text size too small ... %.2f", txt_size);
		(void) warn_report(err_buf);
		return;
		}

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for no text */
	(void) strcpy(tbuf, text);
	nlen = strlen(tbuf);
	if ( nlen <= 0 ) return;

	/* Strip newline character off the end of text */
	if ( tbuf[nlen-1] == '\n' )
		{
		tbuf[nlen-1] = '\0';
		nlen--;
		}
	if ( nlen <= 0 ) return;

	/* Write some definitions for justified text */
	if ( FirstTextWrite )
		{

		/* Reset the initialization check */
		FirstTextWrite = FALSE;

		/* Write definitions for justified text */
		(void) fprintf(FP_Out, "%%### Definitions for justified text\n");
		(void) fprintf(FP_Out, "/left_just {\n");
		(void) fprintf(FP_Out, "  pop\n");
		(void) fprintf(FP_Out, "  0 0 rmoveto\n");
		(void) fprintf(FP_Out, "  } def\n");
		(void) fprintf(FP_Out, "/centre_just {\n");
		(void) fprintf(FP_Out, "  dup stringwidth pop 2 div\n");
		(void) fprintf(FP_Out, "  neg 0 rmoveto\n");
		(void) fprintf(FP_Out, "  } def\n");
		(void) fprintf(FP_Out, "/right_just {\n");
		(void) fprintf(FP_Out, "  dup stringwidth pop\n");
		(void) fprintf(FP_Out, "  neg 0 rmoveto\n");
		(void) fprintf(FP_Out, "  } def\n");
		(void) fprintf(FP_Out, "%%### End definitions\n");
		}

	/* First set the font parameters */
	(void) write_psmet_font(CurPres, txt_size);

	/* Then position the text location (justified) */
	/* Note that default is left justified!        */
	/* Note that miter limit is reset for text!    */
	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "  2.5 setmiterlimit\n");
	(void) fprintf(FP_Out, "  newpath\n");
	(void) fprintf(FP_Out, "  %.3f %.3f moveto\n", xpos, ypos);
	(void) fprintf(FP_Out, "  %.3f rotate\n", rotation);
	if ( same(justified, JustifyLeft) )
		(void) fprintf(FP_Out, "  (%s) left_just\n", tbuf);
	else if ( same(justified, JustifyCentre) )
		(void) fprintf(FP_Out, "  (%s) centre_just\n", tbuf);
	else if ( same(justified, JustifyRight) )
		(void) fprintf(FP_Out, "  (%s) right_just\n", tbuf);
	else
		(void) fprintf(FP_Out, "  (%s) left_just\n", tbuf);

	/* Then output the text definition */
	(void) fprintf(FP_Out, "  (%s) true charpath\n", tbuf);

	/* Display the text outline first (if requested) */
	if ( do_outline && CurPres.outline_first )
		{
		(void) write_psmet_stroke(CurPres);
		}

	/* Then display the filled text */
	(void) write_psmet_fill(CurPres);

	/* Display the text outline last (if requested) */
	if ( do_outline && !CurPres.outline_first )
		{
		(void) write_psmet_stroke(CurPres);
		}

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "grestore\n");
	}

void		write_psmet_lines

	(
	int			numlines,
	LINE		*lines
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for no lines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Write out each line */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty lines or lines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in line */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "newpath\n");
		(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

		/* Draw line to each subsequent point in line        */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in line                   */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in line    */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			}

		/* Then display the line */
		(void) write_psmet_stroke(CurPres);
		}
	}

void		write_psmet_outlines

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "newpath\n");
		(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

		/* Draw line to each subsequent point in outline     */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in outline                */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in outline */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			}

		/* Close the outline */
		(void) fprintf(FP_Out, "closepath\n");

		/* Display the interior of the outline */
		if ( do_interior_fill ) (void) write_psmet_interior_fill(CurPres);

		/* Then display the outline */
		if ( do_outline ) (void) write_psmet_stroke(CurPres);
		}
	}

void		write_psmet_boundaries

	(
	int			numbounds,
	BOUND		*bounds,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			ibnd, ipts, ihole;
	float		xo, yo, xx, yy, xp, yp;
	BOUND		bound;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for no boundaries */
	if ( numbounds <= 0 ) return;
	if ( IsNull(bounds) ) return;

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Write out each boundary (including holes) */
	for ( ibnd=0; ibnd<numbounds; ibnd++ )
		{

		/* Skip empty boundaries or boundaries with too few points */
		bound = bounds[ibnd];
		if ( IsNull(bound) ) continue;
		if ( IsNull(bound->boundary) ) continue;
		line = bound->boundary;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in boundary */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "newpath\n");
		(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

		/* Draw line to each subsequent point in boundary    */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in boundary               */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in boundary */
		/*  ... as long as it is a new point!  */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			}

		/* Close the boundary */
		(void) fprintf(FP_Out, "closepath\n");

		/* Now add any holes within the boundary */
		for ( ihole=0; ihole<bound->numhole; ihole++ )
			{

			/* Skip empty holes or holes with too few points */
			if ( IsNull(bound->holes[ihole]) ) continue;
			line = bound->holes[ihole];
			(void) condense_line(line);
			if ( line->numpts < 2 ) continue;

			/* Position to first point in hole */
			xo = line->points[0][X];
			yo = line->points[0][Y];
			(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

			/* Draw line to each subsequent point in hole        */
			/*  ... as long as they are spaced far enough apart! */
			if ( PolyFilter > 0.0 )
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = line->points[ipts][Y];
					xp = line->points[ipts+1][X];
					yp = line->points[ipts+1][Y];
					if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
							|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
						{
						(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to each subsequent point in hole                   */
			/*  ... as long as it is a new point but regardless of spacing! */
			else
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = line->points[ipts][Y];
					if ( xx != xo || yy != yo )
						{
						(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to last point in hole    */
			/*  ... as long as it is a new point! */
			xx = line->points[line->numpts-1][X];
			yy = line->points[line->numpts-1][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
				}

			/* Close the hole */
			(void) fprintf(FP_Out, "closepath\n");
			}

		/* Display the interior of the boundary (excluding holes) */
		if ( do_interior_fill ) (void) write_psmet_interior_fill(CurPres);

		/* Then display the boundary (including holes) */
		if ( do_outline ) (void) write_psmet_stroke(CurPres);
		}
	}

void		write_psmet_features

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Return immediately if outline and fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_fill || same(CurPres.fill, ColourNone) ) ) return;

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "newpath\n");
		(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

		/* Draw line to each subsequent point in outline */
		/*  ... as long as it is a new point!            */
		for ( ipts=1; ipts<line->numpts; ipts++ )
			{
			xx = line->points[ipts][X];
			yy = line->points[ipts][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
				xo = xx;
				yo = yy;
				}
			}

		/* Close the outline */
		(void) fprintf(FP_Out, "closepath\n");

		/* Display the interior of the outline */
		if ( do_fill ) (void) write_psmet_fill(CurPres);

		/* Then display the outline */
		if ( do_outline ) (void) write_psmet_stroke(CurPres);
		}
	}

void		write_psmet_symbol

	(
	STRING		sym_file,
	float		xpos,
	float		ypos,
	float		sym_scale,
	float		rotation
	)

	{
	char		iline[GPGLong];
	float		scale, lwidth;

	FILE		*sym_fp;
	char		out_buf[GPGLong], err_buf[GPGLong];

	/* Return immediately if no symbol file passed */
	if ( blank(sym_file) ) return;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	(void) sprintf(out_buf, "### Begin definition of ... %s ###", sym_file);
	(void) write_psmet_comment(out_buf);

	if ( Verbose && !same(sym_file, last_sym_file) )
		{
		(void) fprintf(stdout, "   Accessing Symbol from ... %s\n", sym_file);
		(void) strcpy(last_sym_file, sym_file);
		}

	/* Initialize symbol file location */
	(void) fprintf(FP_Out, "gsave\n");
	(void) fprintf(FP_Out, "  %.3f %.3f translate\n", xpos, ypos);

	/* Set symbol rotation and scale */
	(void) fprintf(FP_Out, "  %.3f rotate\n", rotation);
	(void) fprintf(FP_Out, "  %.3f %.3f scale\n", scale, scale);

	/* Set symbol colour from first line of symbol file */
	/* Note that line width is scaled to original size! */
	(void) fgets_no_lead(sym_fp, iline, (size_t) GPGLong);
	if ( NotNull(strstr(iline, "PSMet_both")) )
		{
		if ( !same(CurPres.outline, CurPres.fill) )
			{
			(void) sprintf(err_buf,
					"Outline \"%s\" and Fill \"%s\" mismatch for symbol ... %s",
					CurPres.outline, CurPres.fill, sym_file);
			(void) warn_report(err_buf);
			}
		else if ( same(CurPres.outline, ColourNone) )
			{
			(void) sprintf(err_buf,
					"Outline and Fill set to \"none\" for symbol ... %s",
					sym_file);
			(void) warn_report(err_buf);
			}
		else
			{
			(void) sscanf(CurPres.line_width, "%f", &lwidth);
			lwidth = lwidth / scale;
			(void) fprintf(FP_Out, "  %s setrgbcolor\n",    CurPres.outline);
			(void) fprintf(FP_Out, "  %.3f setlinewidth\n", lwidth);
			(void) fprintf(FP_Out, "  %s setdash\n",        CurPres.line_style);
			}
		}
	else if ( NotNull(strstr(iline, "PSMet_outline")) )
		{
		if ( same(CurPres.outline, ColourNone) )
			{
			(void) sprintf(err_buf, "Outline set to \"none\" for symbol ... %s",
					sym_file);
			(void) warn_report(err_buf);
			}
		else
			{
			(void) sscanf(CurPres.line_width, "%f", &lwidth);
			lwidth = lwidth / scale;
			(void) fprintf(FP_Out, "  %s setrgbcolor\n",    CurPres.outline);
			(void) fprintf(FP_Out, "  %.3f setlinewidth\n", lwidth);
			(void) fprintf(FP_Out, "  %s setdash\n",        CurPres.line_style);
			}
		}
	else if ( NotNull(strstr(iline, "PSMet_fill")) )
		{
		if ( same(CurPres.fill, ColourNone) )
			{
			(void) sprintf(err_buf, "Fill set to \"none\" for symbol ... %s",
					sym_file);
			(void) warn_report(err_buf);
			}
		else
			{
			(void) fprintf(FP_Out, "  %s setrgbcolor\n", CurPres.fill);
			}
		}

	/* Add all remaining lines in symbol file ... except "showpage" */
	while ( NotNull(fgets_no_lead(sym_fp, iline, (size_t) GPGLong)) )
		{
		if ( NotNull(strstr(iline, "showpage")) ) continue;
		(void) fprintf(FP_Out, "  %s\n", iline);
		}

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "grestore\n");

	(void) sprintf(out_buf, "### End definition of ... %s ###", sym_file);
	(void) write_psmet_comment(out_buf);

	/* Close the symbol file */
	(void) fclose(sym_fp);
	}

void		write_psmet_outline_mask

	(
	LINE		line,
	LOGICAL		do_mask
	)

	{
	int			ipts;
	float		xo, yo, xx, yy;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately if mask is turned off */
	if ( !do_mask )
		{
		(void) fprintf(FP_Out, "initclip\n");
		return;
		}

	/* Return immediately for empty outlines or outlines with too few points */
	if ( IsNull(line) ) return;
	(void) condense_line(line);
	if ( line->numpts < 2 ) return;

	/* Position to first point in outline */
	xo = line->points[0][X];
	yo = line->points[0][Y];
	(void) fprintf(FP_Out, "newpath\n");
	(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

	/* Draw line to each subsequent point in outline */
	/*  ... as long as it is a new point!            */
	for ( ipts=1; ipts<line->numpts-1; ipts++ )
		{
		xx = line->points[ipts][X];
		yy = line->points[ipts][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			xo = xx;
			yo = yy;
			}
		}

	/* Draw line to last point in outline */
	/*  ... as long as it is a new point! */
	xx = line->points[line->numpts-1][X];
	yy = line->points[line->numpts-1][Y];
	if ( xx != xo || yy != yo )
		{
		(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
		}

	/* Close the outline */
	(void) fprintf(FP_Out, "closepath\n");

	/* Then set the clipping mask */
	(void) fprintf(FP_Out, "clip\n");
	}

void		write_psmet_boundary_mask

	(
	BOUND		bound,
	LOGICAL		do_mask
	)

	{
	int			ipts, ihole;
	float		xo, yo, xx, yy;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_psmet_comment(FpaCblank);

	/* Return immediately if mask is turned off */
	if ( !do_mask )
		{
		(void) fprintf(FP_Out, "initclip\n");
		return;
		}

	/* Return immediately for empty boundaries */
	/*  or boundaries with too few points      */
	if ( IsNull(bound) ) return;
	if ( IsNull(bound->boundary) ) return;
	line = bound->boundary;
	(void) condense_line(line);
	if ( line->numpts < 2 ) return;

	/* Position to first point in boundary */
	xo = line->points[0][X];
	yo = line->points[0][Y];
	(void) fprintf(FP_Out, "newpath\n");
	(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

	/* Draw line to each subsequent point in boundary */
	/*  ... as long as it is a new point!             */
	for ( ipts=1; ipts<line->numpts-1; ipts++ )
		{
		xx = line->points[ipts][X];
		yy = line->points[ipts][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			xo = xx;
			yo = yy;
			}
		}

	/* Draw line to last point in boundary */
	/*  ... as long as it is a new point!  */
	xx = line->points[line->numpts-1][X];
	yy = line->points[line->numpts-1][Y];
	if ( xx != xo || yy != yo )
		{
		(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
		}

	/* Close the boundary */
	(void) fprintf(FP_Out, "closepath\n");

	/* Now add any holes within the boundary */
	for ( ihole=0; ihole<bound->numhole; ihole++ )
		{

		/* Skip empty holes or holes with too few points */
		if ( IsNull(bound->holes[ihole]) ) continue;
		line = bound->holes[ihole];
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in hole */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "%.3f %.3f moveto\n", xo, yo);

		/* Draw line to each subsequent point in hole */
		/*  ... as long as it is a new point!         */
		for ( ipts=1; ipts<line->numpts-1; ipts++ )
			{
			xx = line->points[ipts][X];
			yy = line->points[ipts][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
				xo = xx;
				yo = yy;
				}
			}

		/* Draw line to last point in hole    */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "%.3f %.3f lineto\n", xx, yy);
			}

		/* Close the hole */
		(void) fprintf(FP_Out, "closepath\n");
		}

	/* Then set the clipping mask */
	(void) fprintf(FP_Out, "clip\n");
	}

void		write_svgmet_comment

	(
	STRING		buf
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized )
		{

		/* Reset the initialization check */
		NotInitialized = FALSE;

		/* Open the default output file (if an output file is not yet open!) */
		if ( IsNull(FP_Out) )
			{
			if ( !open_graphics_file(FpaCblank, FpaCblank) )
				{
				(void) error_report("Error opening default output file");
				}
			}

		/* Write the header information */
		(void) fprintf(FP_Out, "%s\n", HeaderBuf);

		/* Begin grouping of output objects */
		(void) fprintf(FP_Out,
				"<!-- Start of grouping for fpdf directives -->\n");
		(void) write_svgmet_group(GPGstart, NullPointer, 0);
		}

	/* Write the output buffer */
	if ( !blank(buf) ) (void) fprintf(FP_Out, "<!-- %s -->\n", buf);
	}

void		write_svgmet_group

	(
	STRING		buf,
	char		list[][GPGMedium],
	int			num
	)

	{
	int		ii;
	STRING	key, action;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Output the start group buffer for a group with parameters */
	if ( same(buf, GPGstart) && num > 0 )
		{
		(void) fprintf(FP_Out, "<g");
		for ( ii=0; ii<num; ii++ )
			{
			(void) parse_program_instruction(list[ii], &key, &action);
			if ( same(key, KeyNone) )
				(void) fprintf(FP_Out, " \"%s\"", action);
			else
				(void) fprintf(FP_Out, " %s=\"%s\"", key, action);
			}
		(void) fprintf(FP_Out, "> <!-- start group-->\n");
		}
	/* Output the start group buffer for a group with no parameters */
	else if ( same(buf, GPGstart) )
		(void) fprintf(FP_Out, "<g> <!-- start group-->\n");
	/* Output the end group buffer */
	else if ( same(buf, GPGend) )
		(void) fprintf(FP_Out, "</g> <!-- end group -->\n");
	else
		return;
	}

void		write_svgmet_bitmap

	(
	FILE		*unused_symbol_file
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* <<<<< svg can include JPEG and PNG type bitmap images >>>>> */
	/* <<<<< Return for now >>>>> */
	/*
	 * (void) fprintf(FP_Out, "<image xlink:href=\"%s\" ", symbol_file_name);
	 * (void) fprintf(FP_Out, "x=\"%.3f\" y=\"%.3f\" ", xpos, ypos);
	 * (void) fprintf(FP_Out, "width=\"%.3f\" height=\"%.3f\"/> ", width, height);
	 */

	return;
	}

void		write_svgmet_image
	(
	 Image img
	)
	{
		/* display imagery in product */
		STRING		base64PNG, base64PNG_line[GPGMedium];
		int			width, height, x_orig, y_orig, ii, PNGlen;
		float		ypos;
		char		err_buf[GPGLong];

		/* Output image encoded in base64 to the svg file */
		if (NotNull(base64PNG = glImageToBase64PNG(img)))
		{
			glImageGetGeometry(img, &x_orig, &y_orig, &width, &height);

			/* Due to difference in origin for svg and gpgen standard must
			 * manually scale Y offset */
			ypos = PageHeight - ULpoint[Y];

			(void) fprintf(FP_Out, "<image x=\"%.3f\" y=\"%.3f\" ",
						   ULpoint[X], ypos);
			(void) fprintf(FP_Out, "width=\"%.3f\" height=\"%.3f\" ",
						   (float)width, (float)height);
			(void) fprintf(FP_Out, "xlink:href=\"data:;base64,\n");
			PNGlen = strlen(base64PNG);
			for(ii = 0; ii < PNGlen; ii++)
			{
				(void) fprintf(FP_Out, "%c", base64PNG[ii]);
				/* wrap the image every 80 columns to keep the file neat */
				if (!( (ii + 1) % 80)) (void) fprintf(FP_Out, "\n");
			}
			(void) fprintf(FP_Out, "\"/>\n");

		}
		else
		{
			(void) sprintf(err_buf, "Error occured while trying to convert to base64 file \n");
			(void) error_report(err_buf);
		}
	return;
	}

void		write_svgmet_box

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{

	/* Due to difference in origin for svg and gpgen standard must
	 * manually scale Y offset */
	ypos = PageHeight - ypos;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* First output the definition of the box             */
	/* SVG origin is upper left corner rather than centre */
	(void) fprintf(FP_Out,
			"<rect x=\"%.3f\" y=\"%.3f\" width=\"%.3f\" height=\"%.3f\" ",
			-width/2.0, -height/2.0, width, height);
	(void) fprintf(FP_Out,
			"transform=\"translate(%.3f %.3f) rotate(%.3f)\" ",
			xpos, ypos, -rotation);

	/* Then display the interior of the box */
	if (do_interior_fill )
		(void) write_svgmet_interior_fill(CurPres);
	else
		(void) fprintf(FP_Out, "fill=\"none\" ");

	/* Then display the outline of the box */
	if ( do_outline )
		(void) write_svgmet_stroke(CurPres);

	/* Close the tag */
	(void) fprintf(FP_Out, "/>\n");
	}

void		write_svgmet_ellipse

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		sangle,
	float		eangle,
	LOGICAL		closed,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			large_arc, sweep;
	float		extent, sx, sy, ex, ey, rx, ry;
	float		c_r, s_r, s_nr, s_s, c_s, s_e, c_e;
	char		err_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for ellipse with zero width or height */
	if ( width <= 0.0 || height <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with ellipse width ... %.2f  or height ... %.2f",
				width, height);
		(void) warn_report(err_buf);
		return;
		}

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Since svg does rotation clockwise and gpgen does it counter clockwise
	 * we need to negate the angles */
	sangle = -sangle;
	eangle = -eangle;
	rotation = -rotation;

	/* Due to difference in origin for svg and gpgen standard must
	 * manually scale Y offset */
	ypos = PageHeight - ypos;

	/* Convert from elliptic arc based around a central point to an elliptic arc
	 * parameterized for svg
	 * */
	rx = width/2;
	ry = height/2;

	/* Compute large_arc and sweep flags */
	extent    = eangle - sangle;
	large_arc = (fabs((double) extent) > 180)? 1: 0;
	sweep     = (extent > 0)? 1: 0;

	/* Convert to Radians */
	s_r  = sin(RAD *  rotation);
	c_r  = cos(RAD *  rotation);
	s_nr = sin(RAD * -rotation);
	s_s  = sin(RAD *  sangle);
	c_s  = cos(RAD *  sangle);
	s_e  = sin(RAD *  eangle);
	c_e  = cos(RAD *  eangle);

	/* Calculate start and end points of arc */
	sx = xpos + c_r * rx * c_s + s_nr *ry * s_s;
	sy = ypos + s_r * rx * c_s + c_r *ry * s_s;
	ex = xpos + c_r * rx * c_e + s_nr *ry * s_e;
	ey = ypos + s_r * rx * c_e + c_r *ry * s_e;

	/* First output the definition of the ellipse */

	/* Draw a full ellipse */
	if ( sangle == eangle )
		{
		(void) fprintf(FP_Out,
				"<ellipse cx=\"0\" cy=\"0\" rx=\"%.3f\" ry=\"%.3f\" ",
				rx, ry);
		(void) fprintf(FP_Out,
				"transform=\"translate(%.3f %.3f) rotate(%.3f)\" ",
				xpos, ypos, rotation);
		}

	/* Draw a partial ellipse */
	else if (closed)
		{
		(void) fprintf(FP_Out, "<path d=\"M %.3f %.3f ", xpos, ypos);
		(void) fprintf(FP_Out, "L %.3f %.3f  ", sx, sy);
		(void) fprintf(FP_Out, "A %.3f %.3f %.3f %d %d %.3f %.3f ",
				rx, ry, -rotation, large_arc, sweep, ex, ey);
		(void) fprintf(FP_Out, "L %.3f %.3f\" ", xpos, ypos);
		}

	/* Draw an elliptical arc */
	else
		{
		(void) fprintf(FP_Out, "<path d=\"M %.3f %.3f ", sx, sy);
		(void) fprintf(FP_Out, "A %.3f %.3f %.3f %d %d %.3f %.3f ",
				rx, ry, -rotation, large_arc, sweep, ex, ey);
		}

	/* Then display the interior of the ellipse */
	if (closed && do_interior_fill )
		(void) write_svgmet_interior_fill(CurPres);
	else
		(void) fprintf(FP_Out, "fill=\"none\" ");

	/* Then display the outline of the ellipse */
	if ( do_outline )
		(void) write_svgmet_stroke(CurPres);

	/* Close the tag */
	(void) fprintf(FP_Out, "/>\n");
	}

void		write_svgmet_underline

	(
	float		xpos,
	float		ypos,
	float		width,
	float		rotation
	)

	{

	/* Due to difference in origin for svg and gpgen standard must
	 * manually scale Y offset */
	ypos = PageHeight - ypos;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* First output the definition of the underline */
	(void) fprintf(FP_Out, "<path d=\"M %.3f 0 h %.3f\" ",
				-width/2.0, width);
	(void) fprintf(FP_Out, "transform=\"translate(%.3f %.3f) rotate(%.3f)\" ",
				xpos, ypos, -rotation);

	/* Then display the underline */
	(void) write_svgmet_stroke(CurPres);

	/* Close the tag */
	(void) fprintf(FP_Out, "/>\n");
	}

void		write_svgmet_text

	(
	STRING		text,
	float		xpos,
	float		ypos,
	float		txt_size,
	STRING		justified,
	float		rotation,
	LOGICAL		do_outline
	)

	{
	size_t		nlen;
	char		err_buf[GPGLong], tbuf[GPGLong];

	/* Warning message for text size too small */
	if ( txt_size <= 0.0 )
		{
		(void) sprintf(err_buf, "Text size too small ... %.2f", txt_size);
		(void) warn_report(err_buf);
		return;
		}

	/* Due to difference in origin for svg and gpgen standard must
	 * manually scale Y offset */
	ypos = PageHeight - ypos;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for no text */
	(void) strcpy(tbuf, text);
	nlen = strlen(tbuf);
	if ( nlen <= 0 ) return;

	/* Strip newline character off the end of text */
	if ( tbuf[nlen-1] == '\n' )
		{
		tbuf[nlen-1] = '\0';
		nlen--;
		}
	if ( nlen <= 0 ) return;

	/* Replace reserved characters with appropriate escapes */
	(void) check_svgmet_text(tbuf);

	/* Display the text outline first (if requested) */
	if ( do_outline && CurPres.outline_first)
		{
		/* Set style properties for the group */
		(void) fprintf(FP_Out, "<g ");
		(void) fprintf(FP_Out,
					"transform=\"translate(%.3f %.3f) rotate(%.3f)\" ",
					xpos, ypos, -rotation);

		(void) write_svgmet_font(CurPres, txt_size, justified);
		(void) write_svgmet_stroke(CurPres);
		(void) write_svgmet_fill(CurPres);

		(void) fprintf(FP_Out, ">\n");

		/* Write text with stroke and no fill */
		(void) fprintf(FP_Out, "<text x=\"0\" y=\"0\" fill=\"none\">\n");
		(void) fprintf(FP_Out, "%s\n", tbuf);

		/* Then output the text */
		(void) fprintf(FP_Out, "</text>\n");

		/* Write text with fill and no stroke */
		(void) fprintf(FP_Out, "<text x=\"0\" y=\"0\" stroke=\"none\">\n");
		(void) fprintf(FP_Out, "%s\n", tbuf);

		/* Then output the text */
		(void) fprintf(FP_Out, "</text>\n");
		(void) fprintf(FP_Out, "</g><!-- End Text Group -->\n");
		}

	/* Stroke done after fill by default in SVG */
	else
		{
		/* Set style properties for text */
		(void) fprintf(FP_Out, "<text x=\"0\" y=\"0\" ");
		(void) fprintf(FP_Out,
					"transform=\"translate(%.3f %.3f) rotate(%.3f)\" ",
					xpos, ypos, -rotation);

		(void) write_svgmet_font(CurPres, txt_size, justified);
		if (do_outline) (void) write_svgmet_stroke(CurPres);
		(void) write_svgmet_fill(CurPres);

		(void) fprintf(FP_Out, ">\n");
		(void) fprintf(FP_Out, "%s\n", tbuf);

		/* Then output the text */
		(void) fprintf(FP_Out, "</text>\n");
		}
	}

void		write_svgmet_lines

	(
	int			numlines,
	LINE		*lines
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for no lines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Write out each line */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty lines or lines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in line */
		xo = line->points[0][X];
		yo = PageHeight-line->points[0][Y];
		(void) fprintf(FP_Out, "<path d=\"\n M %.3f %.3f ", xo, yo);

		/* Draw line to each subsequent point in line        */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = PageHeight-line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in line                   */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in line    */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = PageHeight-line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
			}

		/* SVG will fill a path by default; Even if the path is not closed */
		(void) fprintf(FP_Out, "\"\n fill=\"none\" ");

		/* Then display the line */
		(void) write_svgmet_stroke(CurPres);

		/* Close the tag */
		(void) fprintf(FP_Out, "/>\n");
		}
	}

void		write_svgmet_outlines

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = PageHeight-line->points[0][Y];
		(void) fprintf(FP_Out, "<path d=\"\n M %.3f %.3f ", xo, yo);

		/* Draw line to each subsequent point in outline     */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = PageHeight-line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in outline                */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in outline */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = PageHeight-line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
			}

		/* Close the outline */
		(void) fprintf(FP_Out, "\"\n ");

		/* Display the interior of the outline */
		/* Note that SVG fills by default!     */
		if ( do_interior_fill )
			(void) write_svgmet_interior_fill(CurPres);
		else
			(void) fprintf(FP_Out, "fill=\"none\" ");

		/* Then display the outline */
		if ( do_outline ) (void) write_svgmet_stroke(CurPres);

		/* Close the tag */
		(void) fprintf(FP_Out, "/>\n");
		}
	}

void		write_svgmet_boundaries

	(
	int			numbounds,
	BOUND		*bounds,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			ibnd, ipts, ihole;
	float		xo, yo, xx, yy, xp, yp;
	BOUND		bound;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for no boundaries */
	if ( numbounds <= 0 ) return;
	if ( IsNull(bounds) ) return;

	/* Return immediately if outline and interior_fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_interior_fill || same(CurPres.interior_fill, ColourNone) ) )
					return;

	/* Write out each boundary (including holes) */
	for ( ibnd=0; ibnd<numbounds; ibnd++ )
		{

		/* Skip empty boundaries or boundaries with too few points */
		bound = bounds[ibnd];
		if ( IsNull(bound) ) continue;
		if ( IsNull(bound->boundary) ) continue;
		line = bound->boundary;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in boundary */
		xo = line->points[0][X];
		yo = PageHeight-line->points[0][Y];
		/* Path in svg will permit closing one outline and continuing to another */
		(void) fprintf(FP_Out, "<path d=\"\n  M %.3f %.3f ", xo, yo);

		/* Draw line to each subsequent point in boundary    */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = PageHeight-line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in boundary               */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = PageHeight-line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in boundary */
		/*  ... as long as it is a new point!  */
		xx = line->points[line->numpts-1][X];
		yy = PageHeight-line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
			}

		/* Close the boundary */
		(void) fprintf(FP_Out, "Z ");

		/* Now add any holes within the boundary */
		for ( ihole=0; ihole<bound->numhole; ihole++ )
			{

			/* Skip empty holes or holes with too few points */
			if ( IsNull(bound->holes[ihole]) ) continue;
			line = bound->holes[ihole];
			(void) condense_line(line);
			if ( line->numpts < 2 ) continue;

			/* Position to first point in hole */
			xo = line->points[0][X];
			yo = PageHeight-line->points[0][Y];
			(void) fprintf(FP_Out, "\n  M %.3f %.3f ", xo, yo);

			/* Draw line to each subsequent point in hole        */
			/*  ... as long as they are spaced far enough apart! */
			if ( PolyFilter > 0.0 )
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = PageHeight-line->points[ipts][Y];
					xp = line->points[ipts+1][X];
					yp = PageHeight-line->points[ipts+1][Y];
					if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
							|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
						{
						(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to each subsequent point in hole                   */
			/*  ... as long as it is a new point but regardless of spacing! */
			else
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = PageHeight-line->points[ipts][Y];
					if ( xx != xo || yy != yo )
						{
						(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to last point in hole    */
			/*  ... as long as it is a new point! */
			xx = line->points[line->numpts-1][X];
			yy = PageHeight-line->points[line->numpts-1][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "\n  L %.3f %.3f ", xx, yy);
				}

			/* Close the hole */
			(void) fprintf(FP_Out, "Z ");
			}
		(void) fprintf(FP_Out, "\" fill-rule=\"evenodd\" ");

		/* Display the interior of the boundary (excluding holes) */
		if ( do_interior_fill )
			(void) write_svgmet_interior_fill(CurPres);
		else
			(void) fprintf(FP_Out, "fill=\"none\" ");

		/* Then display the boundary (including holes) */
		if ( do_outline ) (void) write_svgmet_stroke(CurPres);
		(void) fprintf(FP_Out, "/>\n");
		}
	}

void		write_svgmet_features

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Return immediately if outline and fill are both turned off */
	if ( ( !do_outline || same(CurPres.outline, ColourNone) )
			&& ( !do_fill || same(CurPres.fill, ColourNone) ) ) return;

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = PageHeight-line->points[0][Y];
		(void) fprintf(FP_Out, "<path d=\"\n M %.3f %.3f ", xo, yo);

		/* Draw line to each subsequent point in outline */
		/*  ... as long as it is a new point!            */
		for ( ipts=1; ipts<line->numpts; ipts++ )
			{
			xx = line->points[ipts][X];
			yy = PageHeight-line->points[ipts][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "\n L %.3f %.3f ", xx, yy);
				xo = xx;
				yo = yy;
				}
			}

		/* Close the outline */
		(void) fprintf(FP_Out, "\"\n ");

		/* Display the interior of the outline */
		if ( do_fill ) (void) write_svgmet_fill(CurPres);
		else (void) fprintf(FP_Out, "fill=\"none\" ");
		/* Then display the outline */
		if ( do_outline ) (void) write_svgmet_stroke(CurPres);
		/* Close the tag*/
		(void) fprintf(FP_Out, "/>\n");
		}
	}

void		write_svgmet_symbol

	(
	STRING		sym_file,
	float		xpos,
	float		ypos,
	float		sym_scale,
	float		rotation
	)

	{
	char		iline[GPGLong];
	float		scale, lwidth;

	FILE		*sym_fp;
	char		out_buf[GPGLong], err_buf[GPGLong];


	/* Return immediately if no symbol file passed */
	if ( blank(sym_file) ) return;

	/* Due to difference in origin for svg and gpgen standard must
	 * manually scale Y offset */
	ypos = PageHeight - ypos;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	(void) sprintf(out_buf, "### Begin definition of ... %s ###", sym_file);
	(void) write_svgmet_comment(out_buf);

	if ( Verbose && !same(sym_file, last_sym_file) )
		{
		(void) fprintf(stdout, "   Accessing Symbol from ... %s\n", sym_file);
		(void) strcpy(last_sym_file, sym_file);
		}

	/* If this symbol uses clipping path then include its details */
	/*>>>>> Emma Sept 5 2006 <<<<<*/
	/* removing style attribute because generally it's bad form */
	if (svgmet_do_clipPath)
		(void) fprintf(FP_Out,
			"<g clip-path=\"url(#clip%d)\"><!-- Start Clip Area -->\n",
			svgmet_clipNum);

	/*>>>>> Emma Sept 5 2006 <<<<<*/
	/* Initialize symbol file location */
	(void) fprintf(FP_Out, "<g transform=\"translate(%.3f %.3f) ", xpos, ypos);

	/* Set symbol rotation and scale */
	(void) fprintf(FP_Out, "rotate(%.3f) ", -rotation);
	(void) fprintf(FP_Out, "scale(%.3f)\" ", scale);

	/* Set symbol colour from first line of symbol file */
	/* Note that line width is scaled to original size! */
	(void) fgets_no_lead(sym_fp, iline, (size_t) GPGLong);
	if ( same(CurPres.outline, ColourNone) && same(CurPres.fill, ColourNone) )
		{
		(void) sprintf(err_buf,
				"Outline and Fill set to \"none\" for symbol ... %s", sym_file);
		(void) warn_report(err_buf);
		}
	else
		{


		/* Scale the line width so that line width does not change */
		/*  when scaling the entire symbol                         */

		/* <<<<< This appears to be an unresolved issue in SVG!    >>>>> */
		/* <<<<< IE and Mozilla behave like PostScript with regard >>>>> */
		/* <<<<<  to scaling stroke-width.                         >>>>> */
		/* <<<<< However, Adobe Illustrator and Corel Draw do not  >>>>> */
		/* <<<<<  scale stroke-width.                              >>>>> */
		/* <<<<< For now we will follow the example in PSMet and   >>>>> */
		/* <<<<<  make appropriate adjustments at the fpdf level.  >>>>> */

		(void) sscanf(CurPres.line_width, "%f", &lwidth);
		lwidth = lwidth / scale;

		/* Add default stroke rules if applicable */
		if (!same(CurPres.outline, ColourNone))
			{
			(void) fprintf(FP_Out, "stroke=\"%s\" ",           CurPres.outline);
			(void) fprintf(FP_Out, "stroke-width=\"%.3f\" ",   lwidth);
			(void) fprintf(FP_Out, "stroke-dasharray=\"%s\" ", CurPres.line_style);
			}

		/* Add default fill rules or "none" if applicable */
		if (!same(CurPres.fill, ColourNone))
			(void) fprintf(FP_Out, "fill=\"%s\">\n", CurPres.fill);
		else
			(void) fprintf(FP_Out, "fill=\"none\">\n");
		}

	/* Add all remaining lines in symbol file ... except "</svg>" */
	while ( NotNull(fgets_no_lead(sym_fp, iline, (size_t) GPGLong)) )
		{
		if ( NotNull(strstr(iline, "</svg>")) ) continue;
		(void) fprintf(FP_Out, "  %s\n", iline);
		}

	/* Then reset the graphics state */
	(void) fprintf(FP_Out, "</g>\n");
	(void) sprintf(out_buf, "### End definition of ... %s ###", sym_file);
	(void) write_svgmet_comment(out_buf);

	/* If this symbol uses a clipping path then close the group */
	if (svgmet_do_clipPath)
		{
		(void) fprintf(FP_Out, "</g>\n");
		(void) sprintf(out_buf, "### End clip area for ... %s ###", sym_file);
		(void) write_svgmet_comment(out_buf);
		}

	/* Close the symbol file */
	(void) fclose(sym_fp);
	}

void		write_svgmet_outline_mask

	(
	LINE		line,
	LOGICAL		do_mask
	)

	{
	int			ipts;
	float		xo, yo, xx, yy;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Return immediately if mask is turned off */
	if ( !do_mask )
		{
		svgmet_do_clipPath = FALSE;
		return;
		}

	/* Set the do_clipPath flag and increment the clipPath counter */
	svgmet_do_clipPath = TRUE;
	svgmet_clipNum ++;

	/* Return immediately for empty outlines or outlines with too few points */
	if ( IsNull(line) ) return;
	(void) condense_line(line);
	if ( line->numpts < 2 ) return;

	/* Position to first point in outline */
	xo = line->points[0][X];
	yo = PageHeight-line->points[0][Y];

	/* Start definition of clipPath give it a unique name */
	(void) fprintf(FP_Out, "<defs>\n <clipPath id=\"clip%d\">\n  ",
			svgmet_clipNum);
	(void) fprintf(FP_Out, "<path d=\"\n  M%.3f %.3f", xo, yo);

	/* Draw line to each subsequent point in outline */
	/*  ... as long as it is a new point!            */
	for ( ipts=1; ipts<line->numpts-1; ipts++ )
		{
		xx = line->points[ipts][X];
		yy = PageHeight-line->points[ipts][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n  L%.3f %.3f", xx, yy);
			xo = xx;
			yo = yy;
			}
		}

	/* Draw line to last point in outline */
	/*  ... as long as it is a new point! */
	xx = line->points[line->numpts-1][X];
	yy = PageHeight-line->points[line->numpts-1][Y];
	if ( xx != xo || yy != yo )
		{
		(void) fprintf(FP_Out, "\n  L%.3f %.3f", xx, yy);
		}

	/* Close the outline */

	/* Then set the clipping mask */
	(void) fprintf(FP_Out, "\"/>\n </clipPath>\n</defs>\n");
	}

void		write_svgmet_boundary_mask

	(
	BOUND		bound,
	LOGICAL		do_mask
	)

	{
	int			ipts, ihole;
	float		xo, yo, xx, yy;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Turn off do_clipPath flag and return immediately if mask is turned off */
	if ( !do_mask )
		{
		svgmet_do_clipPath = FALSE;
		return;
		}

	/* Set the do_clipPath flag and increment the clipPath counter */
	svgmet_do_clipPath = TRUE;
	svgmet_clipNum ++;

	/* Return immediately for empty boundaries */
	/*  or boundaries with too few points      */
	if ( IsNull(bound) ) return;
	if ( IsNull(bound->boundary) ) return;
	line = bound->boundary;
	(void) condense_line(line);
	if ( line->numpts < 2 ) return;

	/* Position to first point in boundary */
	xo = line->points[0][X];
	yo = PageHeight-line->points[0][Y];
	/* Define clipPath and give it a unique name. */
	(void) fprintf(FP_Out, "<defs>\n <clipPath id=\"clip%d\">\n  ",
			svgmet_clipNum);
	(void) fprintf(FP_Out, "<path d=\"\n   M %.3f %.3f", xo, yo);

	/* Draw line to each subsequent point in boundary */
	/*  ... as long as it is a new point!             */
	for ( ipts=1; ipts<line->numpts-1; ipts++ )
		{
		xx = line->points[ipts][X];
		yy = PageHeight-line->points[ipts][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n   L %.3f %.3f", xx, yy);
			xo = xx;
			yo = yy;
			}
		}

	/* Draw line to last point in boundary */
	/*  ... as long as it is a new point!  */
	xx = line->points[line->numpts-1][X];
	yy = PageHeight-line->points[line->numpts-1][Y];
	if ( xx != xo || yy != yo )
		{
		(void) fprintf(FP_Out, "\n   L %.3f %.3f", xx, yy);
		}

	/* Close the boundary */
	(void) fprintf(FP_Out, "\nZ");

	/* Now add any holes within the boundary */
	for ( ihole=0; ihole<bound->numhole; ihole++ )
		{

		/* Skip empty holes or holes with too few points */
		if ( IsNull(bound->holes[ihole]) ) continue;
		line = bound->holes[ihole];
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in hole */
		xo = line->points[0][X];
		yo = PageHeight-line->points[0][Y];
		(void) fprintf(FP_Out, "\n   M %.3f %.3f", xo, yo);

		/* Draw line to each subsequent point in hole */
		/*  ... as long as it is a new point!         */
		for ( ipts=1; ipts<line->numpts-1; ipts++ )
			{
			xx = line->points[ipts][X];
			yy = PageHeight-line->points[ipts][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "\n   L %.3f %.3f", xx, yy);
				xo = xx;
				yo = yy;
				}
			}

		/* Draw line to last point in hole    */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = PageHeight-line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "\n   L %.3f %.3f", xx, yy);
			}

		/* Close the hole */
		}

	/* Then set the clipping mask */
	(void) fprintf(FP_Out,
			"\" clip-rule=\"evenodd\"/>\n  </clipPath>\n</defs>\n");
	}

void		write_cormet_comment

	(
	STRING		buf
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized )
		{

		/* Reset the initialization check */
		NotInitialized = FALSE;

		/* Open the default output file (if an output file is not yet open!) */
		if ( IsNull(FP_Out) )
			{
			if ( !open_graphics_file(FpaCblank, FpaCblank) )
				{
				(void) error_report("Error opening default output file");
				}
			}

		/* Write the header information */
		(void) fprintf(FP_Out, "%s\n", HeaderBuf);

		/* Write the default output units (1000ths of an inch) */
		(void) fprintf(FP_Out, "%%### Define output units for cmf file\n");
		(void) fprintf(FP_Out, "@mp 1000\n");

		/* Begin grouping of output objects */
		(void) fprintf(FP_Out, "%%### Start of grouping for fpdf directives\n");
		(void) write_cormet_group(GPGstart, NullPointer, 0);
		}

	/* Write the output buffer */
	if ( !blank(buf) ) (void) fprintf(FP_Out, "%%%s\n", buf);
	}

void		write_cormet_group

	(
	STRING		buf,
	char		unused_list[][GPGMedium],
	int			unused_num
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Output the start or end group buffer */
	if ( same(buf, GPGstart) )
		(void) fprintf(FP_Out, "@u %%start group\n");
	else if ( same(buf, GPGend) )
		(void) fprintf(FP_Out, "@U %%end group\n");
	else
		return;
	}

void		write_cormet_bitmap

	(
	FILE		*symbol_file
	)

	{
	int		c_bit;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Write out the bitmap character by character */
	while ( TRUE )
		{
		c_bit = getc(symbol_file);
		if ( c_bit == '}' || c_bit == EOF )
			{
			(void) fprintf(FP_Out, "\n");
			break;
			}
		else
			{
			(void) putc(c_bit, FP_Out);
			}
		}
	}

void		write_cormet_box

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	PRES		temp_pres;
	char		out_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline and interior_fill (if necessary) */
	if ( !do_outline )       (void) strcpy(temp_pres.outline,       "@xO");
	if ( !do_interior_fill ) (void) strcpy(temp_pres.interior_fill, "@xF");

	/* Return immediately if outline and interior_fill are both turned off */
	if ( same(temp_pres.outline, "@xO")
			&& same(temp_pres.interior_fill, "@xF") ) return;

	/* Output current graphics presentation for box */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_interior_fill(temp_pres);

	/* Draw the box */
	(void) sprintf(out_buf, "@r %.0f %.0f %.0f %.0f 0 %.0f",
			xpos, ypos, width, height, (rotation * 10.0));
	(void) fprintf(FP_Out, "%s\n", out_buf);
	}

void		write_cormet_ellipse

	(
	float		xpos,
	float		ypos,
	float		width,
	float		height,
	float		sangle,
	float		eangle,
	LOGICAL		closed,
	float		rotation,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	PRES		temp_pres;
	char		out_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline and interior_fill (if necessary) */
	if ( !do_outline )       (void) strcpy(temp_pres.outline,       "@xO");
	if ( !do_interior_fill ) (void) strcpy(temp_pres.interior_fill, "@xF");

	/* Return immediately if outline and interior_fill are both turned off */
	if ( same(temp_pres.outline, "@xO")
			&& same(temp_pres.interior_fill, "@xF") ) return;

	/* Output current graphics presentation for ellipse */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_interior_fill(temp_pres);

	/* Draw a full ellipse */
	if ( sangle == eangle )
		{
		(void) sprintf(out_buf, "@e %.0f %.0f %.0f %.0f 0 0 0 %.0f",
				xpos, ypos, width, height, (rotation * 10.0));
		(void) fprintf(FP_Out, "%s\n", out_buf);
		}

	/* Draw a partial ellipse */
	else if (closed)
		{
		(void) sprintf(out_buf, "@e %.0f %.0f %.0f %.0f %.0f %.0f 1 %.0f",
				xpos, ypos, width, height, (sangle * 10.0), (eangle * 10.0),
				(rotation * 10.0));
		(void) fprintf(FP_Out, "%s\n", out_buf);
		}

	/* Draw an elliptical arc */
	else
		{
		(void) sprintf(out_buf, "@e %.0f %.0f %.0f %.0f %.0f %.0f 0 %.0f",
				xpos, ypos, width, height, (sangle * 10.0), (eangle * 10.0),
				(rotation * 10.0));
		(void) fprintf(FP_Out, "%s\n", out_buf);
		}
	}

void		write_cormet_underline

	(
	float		xpos,
	float		ypos,
	float		width,
	float		rotation
	)

	{
	char		out_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Output current graphics presentation for underline */
	(void) write_cormet_pres_outline(CurPres);

	/* Draw the underline */
	(void) sprintf(out_buf, "@r %.0f %.0f %.0f 0 0 %.0f",
			xpos, ypos, width, (rotation * 10.0));
	(void) fprintf(FP_Out, "%s\n", out_buf);
	}

void		write_cormet_text

	(
	STRING		text,
	float		xpos,
	float		ypos,
	float		txt_size,
	STRING		justified,
	float		rotation,
	LOGICAL		do_outline
	)

	{
	size_t		nlen;
	PRES		temp_pres;
	char		err_buf[GPGLong], tbuf[GPGLong], out_buf[GPGLong];

	/* Warning message for text size too small */
	if ( txt_size <= 0.0 )
		{
		(void) sprintf(err_buf, "Text size too small ... %.2f", txt_size);
		(void) warn_report(err_buf);
		return;
		}

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately for no text */
	(void) strcpy(tbuf, text);
	nlen = strlen(tbuf);
	if ( nlen <= 0 ) return;

	/* Strip newline character off the end of text */
	if ( tbuf[nlen-1] == '\n' )
		{
		tbuf[nlen-1] = '\0';
		nlen--;
		}
	if ( nlen <= 0 ) return;

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline (if necessary) */
	if ( !do_outline ) (void) strcpy(temp_pres.outline, "@xO");

	/* Output current graphics presentation for text */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_fill(temp_pres);
	(void) write_cormet_pres_font(temp_pres);

	/* Set current graphics presentation for drawing outline first */
	if ( temp_pres.outline_first ) (void) fprintf(FP_Out, "@FO 0\n");

	/* Output text with current presentation paramters */
	if ( same(justified, JustifyLeft) )
		(void) sprintf(out_buf, "@t %.0f %.0f %.0f %.0f 1 %s %s %s \"%s\"",
				xpos, ypos, txt_size, (rotation * 10.0), temp_pres.char_space,
				temp_pres.word_space, temp_pres.line_space, tbuf);
	else if ( same(justified, JustifyCentre) )
		(void) sprintf(out_buf, "@t %.0f %.0f %.0f %.0f 2 %s %s %s \"%s\"",
				xpos, ypos, txt_size, (rotation * 10.0), temp_pres.char_space,
				temp_pres.word_space, temp_pres.line_space, tbuf);
	else if ( same(justified, JustifyRight) )
		(void) sprintf(out_buf, "@t %.0f %.0f %.0f %.0f 3 %s %s %s \"%s\"",
				xpos, ypos, txt_size, (rotation * 10.0), temp_pres.char_space,
				temp_pres.word_space, temp_pres.line_space, tbuf);
	else
		(void) sprintf(out_buf, "@t %.0f %.0f %.0f %.0f 0 %s %s %s \"%s\"",
				xpos, ypos, txt_size, (rotation * 10.0), temp_pres.char_space,
				temp_pres.word_space, temp_pres.line_space, tbuf);
	(void) fprintf(FP_Out, "%s\n", out_buf);

	/* Reset current graphics presentation for drawing outline last */
	if ( temp_pres.outline_first ) (void) fprintf(FP_Out, "@FO 1\n");
	}

void		write_cormet_lines

	(
	int			numlines,
	LINE		*lines
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately for no lines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Output current graphics presentation for line */
	(void) write_cormet_pres_outline(CurPres);

	/* Write out each line */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty lines or lines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in line */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "@m %.0f %.0f\n", xo, yo);

		/* Draw line to each subsequent point in line        */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in line                   */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in line    */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
			}

		/* End the line */
		(void) fprintf(FP_Out, "@p\n");
		}
	}

void		write_cormet_outlines

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy, xp, yp;
	PRES		temp_pres;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline and interior_fill (if necessary) */
	if ( !do_outline )       (void) strcpy(temp_pres.outline,       "@xO");
	if ( !do_interior_fill ) (void) strcpy(temp_pres.interior_fill, "@xF");

	/* Return immediately if outline and interior_fill are both turned off */
	if ( same(temp_pres.outline, "@xO")
			&& same(temp_pres.interior_fill, "@xF") ) return;

	/* Output current graphics presentation for outline */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_interior_fill(temp_pres);

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "@m %.0f %.0f\n", xo, yo);

		/* Draw line to each subsequent point in outline     */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in outline                */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in outline */
		/*  ... as long as it is a new point! */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
			}

		/* Close and end the outline */
		(void) fprintf(FP_Out, "@cl\n@p\n");
		}
	}

void		write_cormet_boundaries

	(
	int			numbounds,
	BOUND		*bounds,
	LOGICAL		do_outline,
	LOGICAL		do_interior_fill
	)

	{
	int			ibnd, ipts, ihole;
	float		xo, yo, xx, yy, xp, yp;
	PRES		temp_pres;
	BOUND		bound;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately for no boundaries */
	if ( numbounds <= 0 ) return;
	if ( IsNull(bounds) ) return;

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline and interior_fill (if necessary) */
	if ( !do_outline )       (void) strcpy(temp_pres.outline,       "@xO");
	if ( !do_interior_fill ) (void) strcpy(temp_pres.interior_fill, "@xF");

	/* Return immediately if outline and interior_fill are both turned off */
	if ( same(temp_pres.outline, "@xO")
			&& same(temp_pres.interior_fill, "@xF") ) return;

	/* Output current graphics presentation for outline */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_interior_fill(temp_pres);

	/* Write out each boundary (including holes) */
	for ( ibnd=0; ibnd<numbounds; ibnd++ )
		{

		/* Skip empty boundaries or boundaries with too few points */
		bound = bounds[ibnd];
		if ( IsNull(bound) ) continue;
		if ( IsNull(bound->boundary) ) continue;
		line = bound->boundary;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in boundary */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "@m %.0f %.0f\n", xo, yo);

		/* Draw line to each subsequent point in boundary    */
		/*  ... as long as they are spaced far enough apart! */
		if ( PolyFilter > 0.0 )
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				xp = line->points[ipts+1][X];
				yp = line->points[ipts+1][Y];
				if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
						|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to each subsequent point in boundary               */
		/*  ... as long as it is a new point but regardless of spacing! */
		else
			{
			for ( ipts=1; ipts<line->numpts-1; ipts++ )
				{
				xx = line->points[ipts][X];
				yy = line->points[ipts][Y];
				if ( xx != xo || yy != yo )
					{
					(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
					xo = xx;
					yo = yy;
					}
				}
			}

		/* Draw line to last point in boundary */
		/*  ... as long as it is a new point!  */
		xx = line->points[line->numpts-1][X];
		yy = line->points[line->numpts-1][Y];
		if ( xx != xo || yy != yo )
			{
			(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
			}

		/* Close the boundary */
		(void) fprintf(FP_Out, "@cl\n");

		/* Now add any holes within the boundary */
		for ( ihole=0; ihole<bound->numhole; ihole++ )
			{

			/* Skip empty holes or holes with too few points */
			if ( IsNull(bound->holes[ihole]) ) continue;
			line = bound->holes[ihole];
			(void) condense_line(line);
			if ( line->numpts < 2 ) continue;

			/* Position to first point in hole */
			xo = line->points[0][X];
			yo = line->points[0][Y];
			(void) fprintf(FP_Out, "@m %.0f %.0f\n", xo, yo);

			/* Draw line to each subsequent point in hole        */
			/*  ... as long as they are spaced far enough apart! */
			if ( PolyFilter > 0.0 )
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = line->points[ipts][Y];
					xp = line->points[ipts+1][X];
					yp = line->points[ipts+1][Y];
					if ( (float) hypot((double) (xo-xx), (double) (yo-yy)) > PolyFilter
							|| (float) hypot((double) (xp-xx), (double) (yp-yy)) > PolyFilter )
						{
						(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to each subsequent point in hole                   */
			/*  ... as long as it is a new point but regardless of spacing! */
			else
				{
				for ( ipts=1; ipts<line->numpts-1; ipts++ )
					{
					xx = line->points[ipts][X];
					yy = line->points[ipts][Y];
					if ( xx != xo || yy != yo )
						{
						(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
						xo = xx;
						yo = yy;
						}
					}
				}

			/* Draw line to last point in hole    */
			/*  ... as long as it is a new point! */
			xx = line->points[line->numpts-1][X];
			yy = line->points[line->numpts-1][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "@l %.0f %.0f\n", xx, yy);
				}

			/* Close the hole */
			(void) fprintf(FP_Out, "@cl\n");
			}

		/* End the boundary (including holes) */
		(void) fprintf(FP_Out, "@p\n");
		}
	}

void		write_cormet_features

	(
	int			numlines,
	LINE		*lines,
	LOGICAL		do_outline,
	LOGICAL		do_fill
	)

	{
	int			iline, ipts;
	float		xo, yo, xx, yy;
	PRES		temp_pres;
	LINE		line;

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately for no outlines */
	if ( numlines <= 0 ) return;
	if ( IsNull(lines) ) return;

	/* Copy the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset presentation for outline and fill (if necessary) */
	if ( !do_outline ) (void) strcpy(temp_pres.outline, "@xO");
	if ( !do_fill )    (void) strcpy(temp_pres.fill,    "@xF");

	/* Return immediately if outline and fill are both turned off */
	if ( same(temp_pres.outline, "@xO")
			&& same(temp_pres.fill, "@xF") ) return;

	/* Output current graphics presentation for outline */
	(void) write_cormet_pres_outline(temp_pres);
	(void) write_cormet_pres_fill(temp_pres);

	/* Write out each outline */
	for ( iline=0; iline<numlines; iline++ )
		{

		/* Skip empty outlines or outlines with too few points */
		line = lines[iline];
		if ( IsNull(line) ) continue;
		(void) condense_line(line);
		if ( line->numpts < 2 ) continue;

		/* Position to first point in outline */
		xo = line->points[0][X];
		yo = line->points[0][Y];
		(void) fprintf(FP_Out, "@m %.0f %.0f\n", xo, yo);

		/* Draw line to each subsequent point in outline */
		/*  ... as long as it is a new point!            */
		for ( ipts=1; ipts<line->numpts; ipts++ )
			{
			xx = line->points[ipts][X];
			yy = line->points[ipts][Y];
			if ( xx != xo || yy != yo )
				{
				(void) fprintf(FP_Out, "@L %.0f %.0f\n", xx, yy);
				xo = xx;
				yo = yy;
				}
			}

		/* Close and end the outline */
		(void) fprintf(FP_Out, "@cl\n@p\n");
		}
	}

void		write_cormet_symbol

	(
	STRING		sym_file,
	float		xpos,
	float		ypos,
	float		sym_scale,
	float		rotation
	)

	{
	int			xx, yy, tx, ty;
	char		*ptok;
	char		iline[GPGLong], line[GPGLong];
	float		scale, cos_theta, sin_theta;

	FILE		*sym_fp;
	char		out_buf[GPGLong], err_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately if no symbol file passed */
	if ( blank(sym_file) ) return;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	(void) sprintf(out_buf, "### Begin definition of ... %s ###", sym_file);
	(void) write_cormet_comment(out_buf);

	/* Output current presentation for symbol */
	(void) write_cormet_pres_outline(CurPres);
	(void) write_cormet_pres_fill(CurPres);

	if ( Verbose && !same(sym_file, last_sym_file) )
		{
		(void) fprintf(stdout, "   Accessing Symbol from ... %s\n", sym_file);
		(void) strcpy(last_sym_file, sym_file);
		}

	/* Set parameters from symbol rotation */
	cos_theta = (float) cos(RAD * (double) rotation);
	sin_theta = (float) sin(RAD * (double) rotation);

	/* Read and ignore first line (containing Corel Metafile info) */
	(void) fgets_no_lead(sym_fp, iline, (size_t) GPGLong);

	/* Read and convert all remaining lines */
	while ( NotNull(fgets_no_lead(sym_fp, iline, (size_t) GPGLong)) )
		{

		(void) strcpy(line, iline);
		ptok  = strtok(line, " ");

		/* If Transformation is required */
		if (  strstr("@clp @c @C @l @L @m", ptok))
			{
			(void) sprintf(out_buf, "%s", ptok);
			ptok = strtok('\0', " \n \r \012");
			while( NotNull(ptok) )
				{
				(void) sscanf(ptok, "%d", &xx);
				ptok = strtok('\0', " \n \r \012");
				(void) sscanf(ptok, "%d", &yy);
				ptok = strtok('\0', " \n \r \012");
				tx = (int) xpos
						+ (int) (scale * (cos_theta*xx - sin_theta*yy));
				ty = (int) ypos
						+ (int) (scale * (cos_theta*yy + sin_theta*xx));
				(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);
				}

			(void) fprintf(FP_Out, "%s\n", out_buf);
			}
		else if ( same(ptok, "@e") )
			{
			(void) transform_cormet_ellipse(xpos, ypos, scale, rotation, ptok);
			}

		else if ( same(ptok, "@r") )
			{
			(void) transform_cormet_rectangle(xpos, ypos, scale, rotation, ptok);
			}

		else if ( same(ptok, "@b1") || same(ptok, "@b8") || same(ptok, "@b24") )
			{
			(void) transform_cormet_bitmap(sym_fp, xpos, ypos, scale, ptok);
			}

		else
			{
			/* Strip off the CMF format carriage return (
			/* Note the C format carriage return (/r) is checked instead */
			if ( iline[strlen(iline)-2] == '\r' )
				iline[strlen(iline)-2] = '\0';
			if ( !same(iline, "{") && !same(iline, "}") )
				{
				(void) sprintf(out_buf, "%s", iline);
				(void) fprintf(FP_Out, "%s\n", out_buf);
				}
			}
		}

	(void) sprintf(out_buf, "### End definition of ... %s ###", sym_file);
	(void) write_cormet_comment(out_buf);

	/* Close the symbol file */
	(void) fclose(sym_fp);
	}

void		write_cormet_outline_mask

	(
	LINE		line,
	LOGICAL		do_mask
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately if mask is turned off */
	if ( !do_mask ) return;

	/* Return immediately for empty outlines or outlines with too few points */
	if ( IsNull(line) ) return;
	(void) condense_line(line);
	if ( line->numpts < 2 ) return;

	/* CorMet clipping does not function properly */
	return;
	}

void		write_cormet_boundary_mask

	(
	BOUND		bound,
	LOGICAL		do_mask
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_cormet_comment(FpaCblank);

	/* Return immediately if mask is turned off */
	if ( !do_mask ) return;

	/* Return immediately for empty boundaries */
	/*  or boundaries with too few points      */
	if ( IsNull(bound) ) return;
	if ( IsNull(bound->boundary) ) return;
	(void) condense_line(bound->boundary);
	if ( bound->boundary->numpts < 2 ) return;

	/* CorMet clipping does not function properly */
	return;
	}

/* >>> Have not gone through following code yet!!      <<< */
/* >>> Note that this is a CorMet specific directive!! <<< */
LOGICAL		write_cormet_direct

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	size_t		nc;
	float		xul, yul, width, height;
	float		xc, yc, xx0, yy0, xx1, yy1;
	LOGICAL		argok;
	STRING		expression;
	char		tbuf[GPGTiny], xbuf[GPGLong], out_buf[GPGLong];

	/* Process each CMF command on the list */
	for ( ii=0; ii<num; ii++ )
		{

		/* Check the first part of the CMF command */
		expression = list[ii];
		(void) sscanf(expression, "%s", tbuf);

		/* Reset default line width */
		if ( same(tbuf, "@wd") )
			{
			(void) strcpy(CurPres.line_width, expression);
			continue;
			}

		/* Reset default line style */
		else if ( same(tbuf, "@dt") )
			{
			(void) strcpy(CurPres.line_style, expression);
			continue;
			}

		/* Reset default outline colour */
		else if ( same(tbuf, "@xO") || same(tbuf, "@uO") )
			{
			(void) strcpy(CurPres.outline, expression);
			continue;
			}

		/* Reset default fill colour */
		else if ( same(tbuf, "@xF") || same(tbuf, "@uF") )
			{
			(void) strcpy(CurPres.fill, expression);
			continue;
			}

		/* Reset default font */
		else if ( same(tbuf, "@f") )
			{
			(void) strcpy(CurPres.font, expression);
			continue;
			}

		/* Display text */
		else if ( same(tbuf, "@t") || same(tbuf, "@m") || same(tbuf, "@l") )
			{

			/* Copy the part of the CMF command up to the positions */
			nc = strlen(tbuf);
			(void) strncpy(out_buf, expression, nc);

			/* Extract the positions from the CMF command */
			(void) strcpy(xbuf, expression+nc+1);
			xul = float_arg(xbuf, &argok);  if ( !argok ) continue;
			yul = float_arg(xbuf, &argok);  if ( !argok ) continue;

			/* Set the positions wrt the current anchor location */
			(void) anchored_location(ZeroPoint, xul, yul, &xc, &yc);

			/* Add the positions and append the rest of the CMF command */
			(void) sprintf(out_buf+nc, "%6.0f %6.0f ", xc, yc);
			(void) strcat(out_buf, xbuf);

			/* Write out the buffer */
			(void) fprintf(FP_Out, "%s\n", out_buf);
			continue;
			}

		/* Display boxed text */
		else if ( same(tbuf, "@tb") )
			{

			/* Copy the part of the CMF command up to the positions */
			nc = strlen(tbuf);
			(void) strncpy(out_buf, expression, nc);

			/* Extract the positions and sizes from the CMF command */
			(void) strcpy(xbuf, expression+nc+1);
			xul    = float_arg(xbuf, &argok);  if ( !argok ) continue;
			yul    = float_arg(xbuf, &argok);  if ( !argok ) continue;
			width  = float_arg(xbuf, &argok);  if ( !argok ) continue;
			height = float_arg(xbuf, &argok);  if ( !argok ) continue;

			/* Set the positions and sizes wrt map */
			if ( same_ic(Anchor, AnchorAbsolute) )
				{
				xx0 = (float) XYpoint[X] + xul;
				yy1 = (float) XYpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorCurrent) )
				{
				xx0 = (float) XYpoint[X] + xul;
				yy1 = (float) XYpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorMap) )
				{
				xx0 = xul;
				yy1 = yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorLowerLeft) )
				{
				xx0 = (float) ULpoint[X] + xul;
				yy1 = (float) LRpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorCentreLeft) )
				{
				xx0 = (float) ULpoint[X] + xul;
				yy1 = (float) (ULpoint[Y] + LRpoint[Y]) / 2.0 + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorUpperLeft) )
				{
				xx0 = (float) ULpoint[X] + xul;
				yy1 = (float) ULpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorLowerCentre) )
				{
				xx0 = (float) (ULpoint[X] + LRpoint[X]) / 2.0 + xul;
				yy1 = (float) LRpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorCentre) )
				{
				xx0 = (float) (ULpoint[X] + LRpoint[X]) / 2.0 + xul;
				yy1 = (float) (ULpoint[Y] + LRpoint[Y]) / 2.0 + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorUpperCentre) )
				{
				xx0 = (float) (ULpoint[X] + LRpoint[X]) / 2.0 + xul;
				yy1 = (float) ULpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorLowerRight) )
				{
				xx0 = (float) LRpoint[X] + xul;
				yy1 = (float) LRpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorCentreRight) )
				{
				xx0 = (float) LRpoint[X] + xul;
				yy1 = (float) (ULpoint[Y] + LRpoint[Y]) / 2.0 + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			else if ( same_ic(Anchor, AnchorUpperRight) )
				{
				xx0 = (float) LRpoint[X] + xul;
				yy1 = (float) ULpoint[Y] + yul;
				xx1 = xx0 + width;
				yy0 = yy1 - height;
				}

			/* Add the positions and append the rest of the CMF command */
			(void) sprintf(out_buf+nc,
					"%6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f ",
					xx0, yy0, xx1, yy0, xx0, yy1, xx1, yy1);
			(void) strcat(out_buf, xbuf);

			/* Write out the buffer */
			(void) fprintf(FP_Out, "%s\n", out_buf);
			continue;
			}

		/* Display rectangles and ellipses */
		else if ( same(tbuf, "@r") || same(tbuf, "@e") )
			{

			/* Copy the part of the CMF command up to the positions */
			nc = strlen(tbuf);
			(void) strncpy(out_buf, expression, nc);

			/* Extract the positions and sizes from the CMF command */
			(void) strcpy(xbuf, expression+nc+1);
			xul    = float_arg(xbuf, &argok);  if ( !argok ) continue;
			yul    = float_arg(xbuf, &argok);  if ( !argok ) continue;
			width  = float_arg(xbuf, &argok);  if ( !argok ) continue;
			height = float_arg(xbuf, &argok);  if ( !argok ) continue;

			/* Set the output positions */
			xc = (float) XYpoint[X] + xul + width/2.0;
			yc = (float) XYpoint[Y] + yul - height/2.0;

			/* Add the positions and append the rest of the CMF command */
			(void) sprintf(out_buf+nc,
					"%6.0f %6.0f %6.0f %6.0f ", xc, yc, width, height);
			(void) strcat(out_buf, xbuf);

			/* Write out the buffer */
			(void) fprintf(FP_Out, "%s\n", out_buf);
			continue;
			}

		/* Write out all other commands as is */
		else
			{

			/* Write out the buffer */
			(void) strcpy(out_buf, expression);
			(void) fprintf(FP_Out, "%s\n", out_buf);
			continue;
			}
		}

	/* Return TRUE when all members of list have been processed */
	return TRUE;
	}
/* >>> Have not gone through previous code yet!! <<< */

void		write_texmet_comment

	(
	STRING		unused_buf
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized )
		{

		/* Reset the initialization check */
		NotInitialized = FALSE;

		/* Open the default output file (if an output file is not yet open!) */
		if ( IsNull(FP_Out) )
			{
			if ( !open_graphics_file(FpaCblank, FpaCblank) )
				{
				(void) error_report("Error opening default output file");
				}
			}
		}

	/* TexMet does not require output file comments! */
	return;
	}

void		write_texmet_group

	(
	STRING		unused_buf,
	char		unused_list[][GPGMedium],
	int			unused_num
	)

	{

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_texmet_comment(FpaCblank);

	/* TexMet does not require output file grouping! */
	return;
	}

void		write_texmet_text

	(
	STRING		text,
	float		xpos,
	float		ypos,
	float		unused_size,
	STRING		justified,
	float		unused_rotation,
	LOGICAL		unused_do_outline
	)

	{
	int			nx, ny, nnx;
	size_t		nlen;
	char		tbuf[GPGLong], err_buf[GPGLong];

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_texmet_comment(FpaCblank);

	/* Error if output buffer has not yet been set */
	if ( Tnx <= 0 || Tny <= 0 )
		{
		(void) sprintf(err_buf,
				"Need @size directive to set number of columns or rows ... %d %d",
				Tnx, Tny);
		(void) error_report(err_buf);
		}

	/* Set and check current position */
	nx = NINT(xpos);
	ny = NINT(ypos);
	if ( nx < 1 )
		{
		(void) warn_report("Attempt to position left of first column!");
		nx = 1;
		}
	else if ( nx > Tnx )
		{
		(void) sprintf(err_buf, "Attempt to position right of column %d!", Tnx);
		(void) warn_report(err_buf);
		nx = Tnx;
		}
	if ( ny < 1 )
		{
		(void) warn_report("Attempt to position above first row!");
		ny = 1;
		}
	else if ( ny > Tny )
		{
		(void) sprintf(err_buf, "Attempt to position below row %d!", Tny);
		(void) warn_report(err_buf);
		ny = Tny;
		}

	/* Return immediately for no text */
	(void) strcpy(tbuf, text);
	nlen = strlen(tbuf);
	if ( nlen <= 0 ) return;

	/* Strip newline character off the end of text */
	if ( tbuf[nlen-1] == '\n' )
		{
		tbuf[nlen-1] = '\0';
		nlen--;
		}
	if ( nlen <= 0 ) return;

	/* Truncate text that is longer than allowed */
	if ( nlen > Tnx )
		{
		(void) sprintf(err_buf, "More than %d characters in string \"%s\"!",
				Tnx, tbuf);
		(void) warn_report(err_buf);
		nlen = Tnx;
		tbuf[nlen-1] = '\0';
		}

	/* Set output position of text ... left justified */
	if ( same(justified, JustifyLeft) )
		{
		nnx = nx;
		if ( (nnx+nlen-1) > Tnx )
			{
			(void) sprintf(err_buf, "Attempt to write right of column %d!",
					Tnx);
			(void) warn_report(err_buf);
			nnx = Tnx - nlen + 1;
			}
		}

	/* Set output position of text ... centre justified */
	else if ( same(justified, JustifyCentre) )
		{
		nnx = nx - (nlen/2);
		if ( nnx < 1 )
			{
			(void) sprintf(err_buf, "Attempt to write left of first column!");
			(void) warn_report(err_buf);
			nnx = 1;
			}
		else if ( (nnx+nlen-1) > Tnx )
			{
			(void) sprintf(err_buf, "Attempt to write right of column %d!",
					Tnx);
			(void) warn_report(err_buf);
			nnx = Tnx - nlen + 1;
			}
		}

	/* Set output position of text ... right justified */
	else if ( same(justified, JustifyRight) )
		{
		nnx = nx - nlen + 1;
		if ( nnx < 1 )
			{
			(void) sprintf(err_buf, "Attempt to write left of first column!");
			(void) warn_report(err_buf);
			nnx = 1;
			}
		}

	/* Set output position of text ... default is same as left justified */
	else
		{
		nnx = nx;
		if ( (nnx+nlen-1) > Tnx )
			{
			(void) sprintf(err_buf, "Attempt to write right of column %d!",
					Tnx);
			(void) warn_report(err_buf);
			nnx = Tnx - nlen + 1;
			}
		}

	/* Output text to output buffer */
	(void) strncpy(OutBuf + ((ny-1)*Tnx + (nnx-1)), tbuf, nlen);
	}

/***********************************************************************
*                                                                      *
*    p s m e t _ s y m b o l _ s i z e                                 *
*                                                                      *
*    s v g m e t _ s y m b o l _ s i z e                               *
*                                                                      *
*    c o r m e t _ s y m b o l _ s i z e                               *
*                                                                      *
***********************************************************************/

void		psmet_symbol_size

	(
	STRING		sym_file,
	float		sym_scale,
	float		*width,
	float		*height,
	float		*xcenoff,
	float		*ycenoff
	)

	{
	float		scale, xmin, xmax, ymin, ymax;
	STRING		sline;
	char		line[GPGLong];
	FILE		*sym_fp;
	char		err_buf[GPGLong];

	/* Initialize the return parameters */
	if ( NotNull(width) )   *width   = 0.0;
	if ( NotNull(height) )  *height  = 0.0;
	if ( NotNull(xcenoff) ) *xcenoff = 0.0;
	if ( NotNull(ycenoff) ) *ycenoff = 0.0;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Read the first line of the symbol file */
	(void) fgets_no_lead(sym_fp, line, (size_t) GPGLong);

	/* Close the symbol file */
	(void) fclose(sym_fp);

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	/* Extract the PSMet dimensions (if available) */
	if ( IsNull( sline = strstr(line, "PSMet_size") ) )
		{
		(void) sprintf(err_buf,
				"No PSMet dimensions in symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}
	if ( sscanf(sline,
			"PSMet_size[%f %f %f %f]", &xmin, &ymax, &xmax, &ymin) != 4 )
		{
		(void) sprintf(err_buf,
				"Cannot read dimensions from symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Return the scaled symbol file dimensions */
	if ( NotNull(width) )   *width   = (xmax - xmin) * scale;
	if ( NotNull(height) )  *height  = (ymax - ymin) * scale;
	if ( NotNull(xcenoff) ) *xcenoff = (xmax + xmin) / 2.0 * scale;
	if ( NotNull(ycenoff) ) *ycenoff = (ymax + ymin) / 2.0 * scale;
	}

void		svgmet_symbol_size

	(
	STRING		sym_file,
	float		sym_scale,
	float		*width,
	float		*height,
	float		*xcenoff,
	float		*ycenoff
	)

	{
	float		scale, xmin, swidth, ymin, sheight;
	STRING		sline;
	char		line[GPGLong];
	FILE		*sym_fp;
	char		err_buf[GPGLong];

	/* Initialize the return parameters */
	if ( NotNull(width) )   *width   = 0.0;
	if ( NotNull(height) )  *height  = 0.0;
	if ( NotNull(xcenoff) ) *xcenoff = 0.0;
	if ( NotNull(ycenoff) ) *ycenoff = 0.0;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Read the first line of the symbol file */
	(void) fgets_no_lead(sym_fp, line, (size_t) GPGLong);

	/* Close the symbol file */
	(void) fclose(sym_fp);

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	/* Extract the SVGMet dimensions (if available) */
	if ( IsNull( sline = strstr(line, "viewBox") ) )
		{
		(void) sprintf(err_buf,
				"No viewBox dimensions in symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}
	if ( sscanf(sline,
			"viewBox=\"%f %f %f %f\"", &xmin, &ymin, &swidth, &sheight) != 4 )
		{
		(void) sprintf(err_buf,
				"Cannot read dimensions from symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Return the scaled symbol file dimensions */
	if ( NotNull(width) )   *width   = swidth * scale;
	if ( NotNull(height) )  *height  = sheight * scale;
	if ( NotNull(xcenoff) ) *xcenoff = (swidth + xmin) / 2.0 * scale;
	if ( NotNull(ycenoff) ) *ycenoff = (sheight + ymin) / 2.0 * scale;
	}

void		cormet_symbol_size

	(
	STRING		sym_file,
	float		sym_scale,
	float		*width,
	float		*height,
	float		*xcenoff,
	float		*ycenoff
	)

	{
	int			version_number;
	float		scale, xmin, xmax, ymin, ymax;
	char		line[GPGLong], version[GPGMedium];
	FILE		*sym_fp;
	char		err_buf[GPGLong];

	/* Initialize the return parameters */
	if ( NotNull(width) )   *width   = 0.0;
	if ( NotNull(height) )  *height  = 0.0;
	if ( NotNull(xcenoff) ) *xcenoff = 0.0;
	if ( NotNull(ycenoff) ) *ycenoff = 0.0;

	/* Open the symbol file */
	if ( IsNull( sym_fp = fopen(sym_file, "r") ) )
		{
		(void) sprintf(err_buf, "Cannot open symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Read the first line (containing Corel Metafile info) */
	(void) fgets_no_lead(sym_fp, line, (size_t) GPGLong);

	/* Close the symbol file */
	(void) fclose(sym_fp);

	/* Set the symbol scale */
	if ( sym_scale <= 0.0 )
		{
		(void) sprintf(err_buf,
				"Problem with symbol scale ... %.2f", sym_scale);
		(void) warn_report(err_buf);
		(void) fclose(sym_fp);
		return;
		}
	scale = sym_scale / 100.0;

	/* Extract the dimensions from the Corel Metafile info */
	if ( sscanf(line, "%s %d %f %f %f %f",
			version, &version_number, &xmin, &ymax, &xmax, &ymin) != 6 )
		{
		(void) sprintf(err_buf,
				"Cannot read dimensions from symbol file ... %s", sym_file);
		(void) warn_report(err_buf);
		return;
		}

	/* Return the scaled symbol file dimensions */
	if ( NotNull(width) )   *width   = (xmax - xmin) * scale;
	if ( NotNull(height) )  *height  = (ymax - ymin) * scale;
	if ( NotNull(xcenoff) ) *xcenoff = (xmax + xmin) / 2.0 * scale;
	if ( NotNull(ycenoff) ) *ycenoff = (ymax + ymin) / 2.0 * scale;
	}

/***********************************************************************
*                                                                      *
*    p r o c e s s _ p d f _ f i l e                                   *
*                                                                      *
***********************************************************************/

LOGICAL		process_pdf_file

	(
	STRING		pdf_file
	)

	{
	int			ii, num, maxnum, nloops;
	size_t		nlen;
	LOGICAL		isdirective, isopenbrace, isclosebrace, isendloop;
	char		*token;
	char		rbuf[GPGMedium], directive[GPGMedium];
	char		body[GPGHuge], err_buf[GPGLong];
	ListDef		*list;
	long int	pos_reset;
	FILE		*FP_pdf, *FP_reset;
	char		OriginalPdfDir[GPGLong];
	char		OriginalPdfFile[GPGLong];

	if ( IsNull( FP_pdf = fopen (pdf_file, "r") ) )
		{
		(void) fprintf(stderr, "*** Cannot open fpdf file ... %s\n", pdf_file);
		return FALSE;
		}
	(void) fprintf(stdout, "\n*** In fpdf file ......... %s\n", pdf_file);

	/* Save fpdf filename and directory */
	(void) safe_strcpy(OriginalPdfDir,  PdfDir);
	(void) safe_strcpy(PdfDir,          dir_name(pdf_file));
	(void) safe_strcpy(OriginalPdfFile, PdfFile);
	(void) safe_strcpy(PdfFile,         pdf_file);

	/* Set parameters to check for directive and { } characters */
	isdirective  = FALSE;
	isopenbrace  = FALSE;
	isclosebrace = FALSE;

	/* Set the directive, body, and Error Reporting counter */
	(void) strcpy(directive, FpaCblank);
	(void) strcpy(body,      FpaCblank);
	NumErrorBuf = 0;

	/* Initialize the keyword list for this fpdf file */
	maxnum = 0;
	list   = NullPtr(ListDef *);

	/* Set the looping count for this fpdf file */
	nloops = 0;

	/* Read each line in the fpdf file */
	while ( NotNull( fgets_no_lead(FP_pdf, rbuf, (size_t) GPGMedium) ) )
		{

		/* Skip blank lines */
		if ( blank(rbuf) ) continue;

		/* Skip comment lines */
		if ( rbuf[0] == CommentChar ) continue;

		/* Save each line in an Error Reporting buffer */
		NumErrorBuf++;
		if ( NumErrorBuf > MaxErrorBuf )
			{
			ErrorBufList = GETMEM(ErrorBufList, DirectiveBuffer, NumErrorBuf);
			MaxErrorBuf  = NumErrorBuf;
			}
		(void) strcpy(ErrorBufList[NumErrorBuf-1], rbuf);

		/* Replace special characters within lines              */
		/* This will only miss special characters that straddle */
		/*  a line that is longer than GPGMedium characters!    */
		(void) replace_specials(rbuf);

		/* Strip comments off the ends of lines */
		if ( NotNull( token = strchr(rbuf, CommentChar) ) )
			{
			*token = '\0';
			(void) no_white(rbuf);
			}

		/* Get the directive */
		if ( !isdirective )
			{

			/* The first character of a directive must be '@'       */
			/*  ... otherwise reset the Error Reporting counter and */
			/*       keep reading lines until a directive is found! */
			if ( rbuf[0] != '@' )
				{
				NumErrorBuf = 0;
				continue;
				}

			/* Save the directive */
			isdirective = TRUE;
			(void) strcpy(directive, rbuf);

			/* Check for the open brace ... and just save the directive! */
			if ( NotNull( token = strchr(rbuf, OpenBrace) ) )
				{

				/* Terminate the directive */
				directive[token-rbuf] = '\0';
				(void) no_white(directive);

				/* Save the remainder of the buffer */
				(void) memmove(rbuf, token, strlen(token)+1);
				(void) no_white(rbuf);
				}

			/* Read the next line if only the directive was found */
			else
				{
				continue;
				}
			}

		/* Check for the open brace */
		if ( !isopenbrace )
			{

			/* The first character must be an open brace */
			if ( rbuf[0] != OpenBrace )
				{
				(void) sprintf(err_buf,
						"Syntax error - missing or misplaced \"%c\"",
						OpenBrace);
				(void) error_report(err_buf);
				}

			/* Save the body */
			isopenbrace = TRUE;
			(void) strcpy(body, rbuf);
			}

		/* Add to the body if the open brace has already been found */
		else
			{

			/* Check size of body */
			if ( (strlen(body) + strlen(rbuf)) >= GPGHuge )
				{
				(void) error_report("Directive too long!");
				}

			/* Append to the body */
			(void) strcat(body, rbuf);

			/* Replace special characters within lines           */
			/* This will catch special characters that straddle  */
			/*  a line that is longer than GPGMedium characters! */
			(void) replace_specials(body);
			}

		/* Check for the close brace */
		(void) no_white(body);
		nlen = strlen(body);
		if ( NotNull( token = strchr(body, CloseBrace) ) )
			{

			/* Ensure that the close brace is at the end! */
			if ( token != (body + nlen -1) )
				{
				(void) sprintf(err_buf,
						"Syntax error - missing or misplaced \"%c\"",
						CloseBrace);
				(void) error_report(err_buf);
				}

			/* Ensure that the body contains only one open brace! */
			if ( NotNull( token = strchr(body+1, OpenBrace) ) )
				{
				(void) sprintf(err_buf, "Syntax error - more than one \"%c\"",
						OpenBrace);
				(void) error_report(err_buf);
				}

			/* Body is now complete */
			isclosebrace = TRUE;
			}

		/* Read the next line if the close brace has not been found */
		else
			{

			/* Error report if directive will be too long */
			if ( (nlen + 2) >= GPGHuge )
				{
				(void) error_report("Directive too long!");
				}

			/* Remove line continuation characters      */
			/*  ... but only if complete line was read! */
			if ( body[nlen-1] == '\\' && IsLineComplete )
				{
				body[nlen-1] = '\0';
				}

			/* Add a space after an open brace */
			else if ( body[nlen-1] == OpenBrace )
				{
				(void) strcat(body, " ");
				}

			/* Add a space if the line ends with ";" */
			else if ( body[nlen-1] == SemiColon )
				{
				(void) strcat(body, " ");
				}

			/* Append the line with "; " (if complete line was read) */
			/*  ... but not for the @gpgen_insert directive!         */
			else if ( IsLineComplete && !same(directive, "@gpgen_insert") )
				{
				(void) strcat(body, "; ");
				}

			/* Read the next line */
			continue;
			}

		/* Ready to process the complete directive */
		if ( isdirective && isopenbrace && isclosebrace)
			{

			/* @gpgen_insert directive is a special case  */
			/*  which must bypass routine dissect_body()! */
			if ( same(directive, "@gpgen_insert") )
				{
				num = 1;
				if ( num > maxnum )
					{
					maxnum += 10;
					list    = GETMEM(list, ListDef, maxnum);
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"Increasing number of keywords to: %d in ... %s\n",
								maxnum, PdfFile);
						}
					}
				(void) strncpy(list[0], body, GPGMedium-1);
				}

			/* Split the body of the directive into keywords and values */
			else
				num = dissect_body(body, &list, &maxnum);

			/* Now process directives */
			if ( Verbose )
				{
				(void) fprintf(stdout, "Processing directive ... %s",
						directive);
				if ( num <= 0 )
					{
					(void) fprintf(stdout, " with no keyword parameters\n");
					(void) fprintf(stdout, "  %s { }\n", directive);
					}
				else
					{
					(void) fprintf(stdout, " ... with %d keyword parameter(s)\n",
							num);
					(void) fprintf(stdout, "  %s\n    {\n", directive);
					for ( ii=0; ii<num; ii++ )
						{
						(void) fprintf(stdout, "    %s\n", list[ii]);
						}
					(void) fprintf(stdout, "    }\n");
					}
				}

			/* @version directive must be the first directive */
			if ( same(directive, "@version") )
				{
				if ( !set_version(list, num) )
					{
					(void) error_report("Error setting @version directive");
					}
				}

			/* All other directives require @version directive to be set */
			else if ( !blank(Version) )
				{

				/* @file_name directive opens output file */
				if ( same(directive, "@file_name") )
					{
					if ( !set_file_name(list, num) )
						{
						(void) error_report("Invalid file name specification");
						}
					}

				/* @file_close directive closes currently open output file */
				else if ( same(directive, "@file_close") )
					{
					if ( NotNull(FP_Out) ) (void) close_graphics_file();
					}

				/* @process directive runs external processes */
				else if ( same(directive, "@process") )
					{
					if ( !run_external_process(list, num) )
						{
						(void) error_report("Problem running external process");
						}
					}

				/* @gpgen_insert directive inserts "as-is" into output file */
				else if ( same(directive, "@gpgen_insert") )
					{

					/* Insert "as-is" ... based on application */
					switch ( Program.macro )
						{

						/* Insert "as-is" for SVGMet */
						case GPG_SVGMet:

							if ( !insert_into_svg(body) )
								{
								(void) error_report("Problem inserting into SVG output file");
								}
							break;

						/* Cannot insert "as-is" for other applications */
						default:
							(void) sprintf(err_buf,
									"Cannot insert \"as-is\" for %s!",
									Program.label);
							(void) error_report(err_buf);
						}
					}

				/* @include directive opens another fpdf file */
				else if ( same(directive, "@include") )
					{
					if ( !include_pdf_file(list, num) )
						{
						(void) error_report("Cannot process included file");
						}
					}

				/* @loop_begin directive begins a loop in present fpdf file */
				else if ( same(directive, "@loop_begin") )
					{
					/* Set parameters for new loop */
					if ( !set_loop_parameters(list, num, FP_pdf) )
						{
						(void) error_report("Error setting loop parameters");
						}

					/* Increment the loop count */
					nloops++;
					}

				/* @loop_end directive resets or ends the currently active */
				/*  loop in present fpdf file                              */
				else if ( same(directive, "@loop_end") )
					{
					/* Error if no currently active loops in this fpdf file */
					if ( nloops < 1 )
						{
						(void) error_report("Too many @loop_end directives!");
						}

					/* Reset parameters for currently active loop */
					if ( !reset_loop(&isendloop, &FP_reset, &pos_reset) )
						{
						(void) error_report("Error re-setting loop parameters");
						}

					/* Ensure reset is for the same fpdf file! */
					if ( FP_reset != FP_pdf )
						{
						(void) sprintf(err_buf, "Unmatched file pointers ... %lx (reset) %lx (original)\n",
								(unsigned long) FP_reset, (unsigned long) FP_pdf);
						(void) error_report(err_buf);
						}

					/* Decrement loop count for end of currently active loop */
					if ( isendloop )
						{
						nloops--;
						}

					/* Reset looping for next iteration */
					else
						{
						if ( fseek(FP_reset, pos_reset, SEEK_SET) != 0 )
							{
							(void) error_report("Error resetting loop!");
							}
						}
					}

				/* @loop_location_look_up directive builds a location look up */
				/*   table from current features in an loop                   */
				else if ( same(directive, "@loop_location_look_up") )
					{
					/* Build a look up table for current features in a loop */
					if ( !set_loop_location_lookup(list, num) )
						{
						(void) error_report("Error setting loop parameters");
						}
					}

				/* @group directives can be anywhere */
				else if ( same(directive, "@group") )
					{
					if ( !add_group(list, num) )
						{
						(void) error_report("Error in group specification");
						}
					}

				/* Process all other fpdf file directives */
				else
					{
					(void) process_graphics_directive(directive, list, num);
					}
				}

			/* Error report if @version directive not set correctly */
			else
				{
				(void) error_report("@version must be the first directive");
				}

			/* Reset parameters to check for directive and { } characters */
			isdirective  = FALSE;
			isopenbrace  = FALSE;
			isclosebrace = FALSE;

			/* Reset the directive, body, and Error Reporting counter */
			(void) strcpy(directive, FpaCblank);
			(void) strcpy(body,      FpaCblank);
			NumErrorBuf = 0;
			}
		}

	/* Ensure that all looping in the fpdf file has been completed */
	if ( nloops > 0 )
		{
		(void) fprintf(stderr, "*** Missing %d @loop_end directive(s)\n",
				nloops);
		(void) fprintf(stderr, "***  in fpdf file ... %s\n", PdfFile);
		return FALSE;
		}

	/* Free the keyword list for this fpdf file */
	FREEMEM(list);
	if ( Verbose )
		{
		(void) fprintf(stdout,
				"Freeing space for %d keywords in ... %s\n", maxnum, PdfFile);
		}

	/* Close the fpdf file */
	(void) fclose(FP_pdf);

	/* Reset fpdf filename and directory */
	(void) safe_strcpy(PdfDir,  OriginalPdfDir);
	(void) safe_strcpy(PdfFile, OriginalPdfFile);
	if ( !blank(PdfFile) )
		(void) fprintf(stdout, "\n*** In fpdf file ......... %s\n", PdfFile);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    p a r s e _ i n s t r u c t i o n                                 *
*    p a r s e _ p r o g r a m _ i n s t r u c t i o n                 *
*                                                                      *
***********************************************************************/

void		parse_instruction

	(
	STRING		buf,
	STRING		*key,
	STRING		*action
	)

	{
	size_t		nlen;
	char		*token;
	char		tbuf[GPGMedium];

	static	char	KeyBuf[GPGMedium]    = FpaCblank;
	static	char	ActionBuf[GPGMedium] = FpaCblank;

	/* Check for  "value"  lines */
	if ( IsNull( token = strchr(buf, EqualSign) ) )
		{

		/* Set the  "keyword"  and  "value"  parts */
		(void) strcpy(KeyBuf,    KeyNone);
		(void) strcpy(ActionBuf, buf);

		/* Get back any special characters preceded by an escape character */
		(void) getback_specials(ActionBuf);
		(void) no_white(ActionBuf);

		/* Strip quotes from  "value"  part (if required) */
		nlen = strlen(ActionBuf);
		if ( ( ActionBuf[0] == '"' && ActionBuf[nlen-1] == '"' )
				|| ( ActionBuf[0] == '\'' && ActionBuf[nlen-1] == '\'' ) )
			{
			(void) strcpy(tbuf, ActionBuf+1);
			tbuf[nlen-2] = '\0';
			(void) strcpy(ActionBuf, tbuf);
			}
		}

	/* Expand  "keyword = value"  lines */
	else
		{

		/* Extract the  "keyword"  part */
		(void) strcpy(KeyBuf, buf);
		KeyBuf[token-buf] = '\0';
		(void) no_white(KeyBuf);
		(void) lower_case(KeyBuf);

		/* Extract the  "value"  part */
		(void) strcpy(ActionBuf, token+1);

		/* Get back any special characters preceded by an escape character */
		(void) getback_specials(ActionBuf);
		(void) no_white(ActionBuf);

		/* Strip quotes from  "value"  part (if required) */
		nlen = strlen(ActionBuf);
		if ( ( ActionBuf[0] == '"' && ActionBuf[nlen-1] == '"' )
				|| ( ActionBuf[0] == '\'' && ActionBuf[nlen-1] == '\'' ) )
			{
			(void) strcpy(tbuf, ActionBuf+1);
			tbuf[nlen-2] = '\0';
			(void) strcpy(ActionBuf, tbuf);
			}

		/* Convert  "value"  portions of each  "keyword"  if required */
		(void) check_graphics_keyword(KeyBuf, ActionBuf);
		}

	/* Return the  "keyword"  and  "value"  parts */
	if ( NotNull(key) )    *key    = KeyBuf;
	if ( NotNull(action) ) *action = ActionBuf;
	}

void		parse_program_instruction

	(
	STRING		buf,
	STRING		*key,
	STRING		*action
	)

	{
	size_t		nlen;
	char		*token;
	char		tbuf[GPGMedium];

	static	char	KeyBuf[GPGMedium]    = FpaCblank;
	static	char	ActionBuf[GPGMedium] = FpaCblank;

	/* Check for  "value"  lines */
	if ( IsNull( token = strchr(buf, EqualSign) ) )
		{

		/* Set the  "keyword"  and  "value"  parts */
		(void) strcpy(KeyBuf,    KeyNone);
		(void) strcpy(ActionBuf, buf);

		/* Get back any special characters preceded by an escape character */
		(void) getback_specials(ActionBuf);
		(void) no_white(ActionBuf);

		/* Strip quotes from  "value"  part (if required) */
		nlen = strlen(ActionBuf);
		if ( ( ActionBuf[0] == '"' && ActionBuf[nlen-1] == '"' )
				|| ( ActionBuf[0] == '\'' && ActionBuf[nlen-1] == '\'' ) )
			{
			(void) strcpy(tbuf, ActionBuf+1);
			tbuf[nlen-2] = '\0';
			(void) strcpy(ActionBuf, tbuf);
			}
		}

	/* Expand  "keyword = value"  lines */
	else
		{

		/* Extract the  "keyword"  part */
		(void) strcpy(KeyBuf, buf);
		KeyBuf[token-buf] = '\0';
		(void) no_white(KeyBuf);
		(void) lower_case(KeyBuf);

		/* Extract the  "value"  part */
		(void) strcpy(ActionBuf, token+1);

		/* Get back any special characters preceded by an escape character */
		(void) getback_specials(ActionBuf);
		(void) no_white(ActionBuf);

		/* Strip quotes from  "value"  part (if required) */
		nlen = strlen(ActionBuf);
		if ( ( ActionBuf[0] == '"' && ActionBuf[nlen-1] == '"' )
				|| ( ActionBuf[0] == '\'' && ActionBuf[nlen-1] == '\'' ) )
			{
			(void) strcpy(tbuf, ActionBuf+1);
			tbuf[nlen-2] = '\0';
			(void) strcpy(ActionBuf, tbuf);
			}
		}

	/* Return the  "keyword"  and  "value"  parts */
	if ( NotNull(key) )    *key    = KeyBuf;
	if ( NotNull(action) ) *action = ActionBuf;
	}

/***********************************************************************
*                                                                      *
*    e r r o r _ r e p o r t                                           *
*    w a r n _ r e p o r t                                             *
*                                                                      *
***********************************************************************/

void		error_report

	(
	STRING		buf
	)

	{
	int			nn;

	(void) fprintf(stderr,"\n********** E R R O R **********\n");
	(void) fprintf(stderr,"Error in file ... %s\n", PdfFile);
	(void) fprintf(stderr,"  in block ...\n");
	for ( nn=0; nn<NumErrorBuf; nn++ )
		{
		(void) fprintf(stderr,"    %s\n", ErrorBufList[nn]);
		}
	(void) fprintf(stderr,"\n   >>>>> %s <<<<<\n", buf);
	(void) fprintf(stderr,"\n***** E N D *** E R R O R *****\n");
	(void) exit(0);
	}

void		warn_report

	(
	STRING		buf
	)

	{
	int			nn;

	(void) fprintf(stderr,"\n******** W A R N I N G ********\n");
	(void) fprintf(stderr,"Warning in file ... %s\n", PdfFile);
	(void) fprintf(stderr,"  in block ...\n");
	for ( nn=0; nn<NumErrorBuf; nn++ )
		{
		(void) fprintf(stderr,"    %s\n", ErrorBufList[nn]);
		}
	(void) fprintf(stderr,"\n   >>>>> %s <<<<<\n\n", buf);
	if ( NotNull(Fdesc.sdef) )
		{
		(void) fprintf(stderr,"  Source = %s --- Valid time = %s\n",
				Fdesc.sdef->name, Fdesc.vtime);
		}
	(void) fprintf(stderr,"\n**** E N D * W A R N I N G ****\n");
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES                                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this source file.                                                *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    w r i t e _ p s m e t _ s t r o k e                               *
*    w r i t e _ p s m e t _ f i l l                                   *
*    w r i t e _ p s m e t _ i n t e r i o r _ f i l l                 *
*    w r i t e _ p s m e t _ f o n t                                   *
*                                                                      *
***********************************************************************/

static	void		write_psmet_stroke

	(
	PRES		pres
	)

	{

	/* Display a stroked outline (if required) */
	if ( !same(pres.outline, ColourNone) )
		{
		(void) fprintf(FP_Out, "  gsave\n");
		(void) fprintf(FP_Out, "    %s setrgbcolor\n",  pres.outline);
		(void) fprintf(FP_Out, "    %s setlinewidth\n", pres.line_width);
		(void) fprintf(FP_Out, "    %s setdash\n",      pres.line_style);
		(void) fprintf(FP_Out, "    stroke\n");
		(void) fprintf(FP_Out, "  grestore\n");
		}
	}

static	void		write_psmet_fill

	(
	PRES		pres
	)

	{

	/* Display a filled outline (if required) */
	if ( !same(pres.fill, ColourNone) )
		{
		(void) fprintf(FP_Out, "  gsave\n");
		(void) fprintf(FP_Out, "    %s setrgbcolor\n", pres.fill);
		(void) fprintf(FP_Out, "    fill\n");
		(void) fprintf(FP_Out, "  grestore\n");
		}
	}

static	void		write_psmet_interior_fill

	(
	PRES		pres
	)

	{

	/* Display an interior filled outline (if required) */
	if ( !same(pres.interior_fill, ColourNone) )
		{
		(void) fprintf(FP_Out, "  gsave\n");
		(void) fprintf(FP_Out, "    %s setrgbcolor\n", pres.interior_fill);
		(void) fprintf(FP_Out, "    eofill\n");
		(void) fprintf(FP_Out, "  grestore\n");
		}
	}

static	void		write_psmet_font

	(
	PRES		pres,
	float		size
	)

	{

	/* Write a definition for ISO encoded font */
	if ( FirstFontWrite )
		{

		/* Reset the initialization check */
		FirstFontWrite = FALSE;

		/* Write definition for ISO encoded font */
		(void) fprintf(FP_Out, "%%### Definition for ISO encoded font\n");
		(void) fprintf(FP_Out, "/special_font {\n");
		(void) fprintf(FP_Out, "  dup findfont\n");
		(void) fprintf(FP_Out, "  dup length dict begin\n");
		(void) fprintf(FP_Out, "    {\n");
		(void) fprintf(FP_Out, "    1 index /FID ne\n");
		(void) fprintf(FP_Out, "      {def}\n");
		(void) fprintf(FP_Out, "      {pop pop}\n");
		(void) fprintf(FP_Out, "      ifelse\n");
		(void) fprintf(FP_Out, "    }forall\n");
		(void) fprintf(FP_Out, "    /Encoding ISOLatin1Encoding def\n");
		(void) fprintf(FP_Out, "    currentdict\n");
		(void) fprintf(FP_Out, "  end\n");
		(void) fprintf(FP_Out, "  /Special-ISOLatinFont exch definefont\n");
		(void) fprintf(FP_Out, "  } def\n");
		(void) fprintf(FP_Out, "%%### End definition\n");
		}

	/* Display the font and text size */
	(void) fprintf(FP_Out, "(%s) special_font\n",  pres.font);
	(void) fprintf(FP_Out, "%.3f scalefont\n", size);
	(void) fprintf(FP_Out, "setfont\n");
	}

/***********************************************************************
*                                                                      *
*    w r i t e _ s v g m e t _ s t r o k e                             *
*    w r i t e _ s v g m e t _ f i l l                                 *
*    w r i t e _ s v g m e t _ i n t e r i o r _ f i l l               *
*    w r i t e _ s v g m e t _ f o n t                                 *
*    c h e c k _ s v g m e t _ t e x t                                 *
*                                                                      *
***********************************************************************/

static  void		check_svgmet_text

	(
	 STRING buf
	)
	{
	char *c, temp[GPGLong];
	char special[] = "&<>\'\"";
	char *escape[] = { "&amp;", "&lt;", "&gt;", "&apos;", "&quot;" };
	int	 ii, nspecial = strlen(special);
	for ( ii=0; ii < nspecial; ii++ )
		{
		c = buf;
		while ( NotNull(c = strchr(c,special[ii])) )
			{
			*c++ = '\0';
			safe_strcpy(temp, buf);
			safe_strcat(temp, escape[ii]);
			safe_strcat(temp, c);
			safe_strcpy(buf, temp);
			c += strlen(escape[ii])-1;
			}
		}
	}

static	void		write_svgmet_stroke

	(
	PRES		pres
	)

	{

	/* Display a stroked outline (if required) */
	if ( !same(pres.outline, ColourNone) )
		{
		(void) fprintf(FP_Out,
				"stroke=\"%s\" stroke-width=\"%s\" stroke-dasharray=\"%s\" ",
				pres.outline, pres.line_width, pres.line_style);
		}
	}

static	void		write_svgmet_fill

	(
	PRES		pres
	)

	{

	/* Display a filled outline (if required) */
	if ( !same(pres.fill, ColourNone) )
		{
		(void) fprintf(FP_Out, "fill=\"%s\" ", pres.fill);
		}
	else
		(void) fprintf(FP_Out, "fill=\"none\" ");
	}

static	void		write_svgmet_interior_fill

	(
	PRES		pres
	)

	{

	/* Display an interior filled outline (if required) */
	if ( !same(pres.interior_fill, ColourNone) )
		(void) fprintf(FP_Out, "fill=\"%s\" ", pres.interior_fill);
	else
		(void) fprintf(FP_Out, "fill=\"none\" ");
	}

static	void		write_svgmet_font

	(
	PRES		pres,
	float		size,
	STRING		justified
	)

	{

	if ( same(justified, JustifyLeft) )
		{
		(void) fprintf(FP_Out, "text-anchor=\"start\" ");
		}
	else if ( same(justified, JustifyCentre) )
		{
		(void) fprintf(FP_Out, "text-anchor=\"middle\" ");
		}
	else if ( same(justified, JustifyRight) )
		{
		(void) fprintf(FP_Out, "text-anchor=\"end\" ");
		}

	/* Display the font and text size */
	(void) fprintf(FP_Out,
			"font-family=\"%s\" font-size=\"%.3f\" ", pres.font, size);
	}

/***********************************************************************
*                                                                      *
*    w r i t e _ c o r m e t _ p r e s _ o u t l i n e                 *
*    w r i t e _ c o r m e t _ p r e s _ f i l l                       *
*    w r i t e _ c o r m e t _ p r e s _ i n t e r i o r _ f i l l     *
*    w r i t e _ c o r m e t _ p r e s _ f o n t                       *
*    t r a n s f o r m _ c o r m e t _ e l l i p s e                   *
*    t r a n s f o r m _ c o r m e t _ r e c t a n g l e               *
*    t r a n s f o r m _ c o r m e t _ b i t m a p                     *
*                                                                      *
***********************************************************************/

static	void		write_cormet_pres_outline

	(
	PRES		pres
	)

	{

	/* Display the outline colour and line parameters */
	(void) fprintf(FP_Out, "%s\n", pres.outline);
	(void) fprintf(FP_Out, "%s\n", pres.line_style);
	(void) fprintf(FP_Out, "%s\n", pres.line_width);
	}

static	void		write_cormet_pres_fill

	(
	PRES		pres
	)

	{

	/* Display the fill colour */
	(void) fprintf(FP_Out, "%s\n", pres.fill);
	}

static	void		write_cormet_pres_interior_fill

	(
	PRES		pres
	)

	{

	/* Display the interior fill colour */
	(void) fprintf(FP_Out, "%s\n", pres.interior_fill);
	}

static	void		write_cormet_pres_font

	(
	PRES		pres
	)

	{

	/* Display the font parameters */
	(void) fprintf(FP_Out, "%s %s %s\n",
			pres.font, pres.font_weight, pres.italics);
	}

static	void		transform_cormet_ellipse

	(
	float		xoff,
	float		yoff,
	float		scale,
	float		rotation,
	char		*ptok
	)

	{
	int			xx, yy, token, tx, ty;
	float		cos_theta, sin_theta;
	char		out_buf[GPGLong];

	cos_theta = (float) cos(RAD * (double) rotation);
	sin_theta = (float) sin(RAD * (double) rotation);

	(void) sprintf(out_buf, "%s", ptok);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &xx);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &yy);

	tx = (int) xoff + (int) (scale * (cos_theta*xx - sin_theta*yy));
	ty = (int) yoff + (int) (scale * (cos_theta*yy + sin_theta*xx));
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	tx = (int) (scale * token);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	ty = (int) (scale * token);
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	tx = (int) token;
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	ty = (int) token;
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	tx = (int) token;
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	ty = (int) token + (int) rotation*10;
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	/* Write out the final pattern */
	(void) fprintf(FP_Out, "%s\n", out_buf);
	}

static	void		transform_cormet_rectangle

	(
	float		xoff,
	float		yoff,
	float		scale,
	float		rotation,
	char		*ptok
	)

	{
	int			xx, yy, token, tx, ty;
	float		cos_theta, sin_theta;
	char		out_buf[GPGLong];

	cos_theta = (float) cos(RAD * (double) rotation);
	sin_theta = (float) sin(RAD * (double) rotation);

	(void) sprintf(out_buf, "%s", ptok);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &xx);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &yy);
	tx = (int) xoff + (int) (scale * (cos_theta*xx - sin_theta*yy));
	ty = (int) yoff + (int) (scale * (cos_theta*yy + sin_theta*xx));
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	tx = (int) (scale * token);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	ty = (int) (scale * token);
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	tx = (int) token;
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	ty = (int) token + (int) rotation*10;
	(void) sprintf(out_buf, "%s %d %d", out_buf, tx, ty);

	/* Write out the final pattern */
	(void) fprintf(FP_Out, "%s\n", out_buf);
	}

static	void		transform_cormet_bitmap

	(
	FILE		*sym_fp,
	float		xoff,
	float		yoff,
	float		scale,
	char		*ptok
	)

	{
	int			token;
	int			xul, yul, xlr, ylr;
	int			xx1, yy1, xx2, yy2;
	char		out_buf[GPGLong];

	(void) sprintf(out_buf, "%s", ptok);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &xx1);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &yy1);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &xx2);
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &yy2);

	if ( xx1 < xx2 )
		{
		xul = 0;
		xlr = xx2-xx1;
		}
	else
		{
		xul = 0;
		xlr = xx1-xx2;
		}

	if ( yy1 < yy2 )
		{
		yul = 0;
		ylr = yy1-yy2;
		}
	else
		{
		yul = 0;
		ylr = yy2-yy1;
		}
	xx1 = (int) xoff + (int)(scale * xul);
	yy1 = (int) yoff + (int) (scale * yul);
	xx2 = (int) xoff + (int)(scale * xlr);
	yy2 = (int) yoff + (int)(scale * ylr);
	(void) sprintf(out_buf, "%s %d %d %d %d", out_buf, xx1, yy1, xx2, yy2);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	xx1 = (int)token;
	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	yy1 =   (int)token;
	(void) sprintf(out_buf, "%s %d %d", out_buf, xx1, yy1);

	ptok = strtok('\0', " \n \r \012");
	(void) sscanf(ptok, "%d", &token);
	xx1 = (int)token;
	(void) sprintf(out_buf, "%s %d", out_buf, xx1);
	(void) fprintf(FP_Out, "%s\n", out_buf);

	/* Write the bitmap */
	(void) write_cormet_bitmap(sym_fp);
	}

/***********************************************************************
*                                                                      *
*    f g e t s _ n o _ l e a d                                         *
*                                                                      *
***********************************************************************/

static	STRING		fgets_no_lead

	(
	FILE		*fp,
	STRING		line,
	size_t		ncl
	)

	{
	size_t		nlen, start;
	STRING		full;

	/* Set global parameter */
	IsLineComplete = TRUE;

	/* Return immediately if empty buffer */
	if ( IsNull(line) ) return NullString;

	/* Get the next line from the file */
	full = fgets(line, (int) ncl, fp);
	if ( IsNull(full) ) return NullString;

	/* Check for incomplete lines with no trailing line feed */
	nlen = strcspn(line, "\n\r\f");
	if (nlen >= strlen(full))
		{

		/* Set parameter for incomplete line */
		IsLineComplete = FALSE;

		/* Remove whitespace only from beginning of line */
		start = strspn(line, WHITE);
		(void) strcpy(line, line+start);
		}

	/* Process complete lines */
	else
		{

		/* Strip trailing line-feed */
		full[nlen] = '\0';
		line[nlen] = '\0';

		/* Remove white space from beginning and end of line */
		(void) no_white(line);
		}

	/* Return the full line (including whitespace) */
	return full;
	}

/***********************************************************************
*                                                                      *
*    d i s s e c t _ b o d y                                           *
*                                                                      *
***********************************************************************/

static	int			dissect_body

	(
	STRING		body,
	ListDef		**list,
	int			*maxnum
	)

	{
	size_t		nlen;
	int			num, maxn, ii, jj;
	ListDef		*dlist;
	char		*token;
	char		buf[GPGHuge], err_buf[GPGLong];

	/* Initialize buffers */
	num   = 0;
	maxn  = *maxnum;
	dlist = *list;
	(void) strcpy(buf, body);

	/* Remove start and end braces { } */
	token = strchr(buf, OpenBrace);
	if ( IsNull(token) ) return 0;
	(void) memmove(buf, token+1, strlen(token)+2);

	token = strchr(buf, CloseBrace);
	if ( IsNull(token) ) return 0;
	*token = '\0';

	/* Put individual keyword instructions into the list */
	while ( TRUE )
		{

		/* End when buffer is empty */
		(void) no_white(buf);
		if ( blank(buf) ) break;

		/* Skip blank keyword instructions */
		if ( buf[0] == SemiColon )
			{
			(void) memmove(buf, buf+1, strlen(buf)+2);
			continue;
			}

		/* Add another keyword instruction ... terminated by ';' */
		if ( NotNull( token = strchr(buf, SemiColon) ) )
			{
			num++;
			if ( num > maxn )
				{
				maxn += 10;
				dlist = GETMEM(dlist, ListDef, maxn);
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"Increasing number of keywords to: %d in ... %s\n",
							maxn, PdfFile);
					}
				}

			/* Check for keywords with too many characters */
			nlen = strlen(buf) - strlen(token);
			if ( nlen >= GPGMedium )
				{
				(void) sprintf(err_buf, "Keyword has more than %d characters!",
						(GPGMedium - 1));
				(void) error_report(err_buf);
				}

			/* Save the keyword instruction */
			(void) strncpy(dlist[num-1], buf, nlen);
			dlist[num-1][nlen] = '\0';
			(void) no_white(dlist[num-1]);
			(void) memmove(buf, token+1, strlen(token)+2);
			}

		/* Add another keyword instruction ... not terminated   */
		/* Note that this will be the last keyword instruction! */
		else
			{
			num++;
			if ( num > maxn )
				{
				maxn += 10;
				dlist = GETMEM(dlist, ListDef, maxn);
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"Increasing number of keywords to: %d in ... %s\n",
							maxn, PdfFile);
					}
				}

			/* Check for keywords with too many characters */
			nlen = strlen(buf);
			if ( nlen >= GPGMedium )
				{
				(void) sprintf(err_buf, "Keyword has more than %d characters!",
						(GPGMedium - 1));
				(void) error_report(err_buf);
				}

			/* Save the keyword instruction */
			(void) strcpy(dlist[num-1], buf);
			buf[0] = '\0';
			}

		/* Include keyword instructions set in @group directives */
		if ( dlist[num-1][0] == OpenAngle )
			{

			/* Overwrite the < character */
			(void) memmove(dlist[num-1], dlist[num-1]+1, strlen(dlist[num-1])+2);

			/* Look for the group name */
			for ( jj=0; jj<NumGroups; jj++ )
				{
				if ( same(dlist[num-1], Groups[jj].group_name) )
					{

					/* Overwrite the group name with the keywords */
					num--;
					for ( ii=0; ii<Groups[jj].num; ii++ )
						{
						num++;
						if ( num > maxn )
							{
							maxn += 10;
							dlist = GETMEM(dlist, ListDef, maxn);
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"Increasing number of keywords to: %d in ... %s\n",
										maxn, PdfFile);
								}
							}
						(void) strcpy(dlist[num-1], Groups[jj].list[ii]);
						}
					break;
					}
				}

			/* Error message if group name not found */
			/* No group name found ... so overwrite the group name */
			if (jj >= NumGroups)
				{
				(void) sprintf(err_buf, "Missing group name ... %s",
						dlist[num-1]);
				(void) error_report(err_buf);
				}
			}
		}

	/* Check for more than one '=' in each keyword instruction */
	for ( ii=0; ii<num; ii++ )
		{
		if ( NotNull( token = strchr(dlist[ii], EqualSign) )
				&& NotNull ( token = strchr(token+1, EqualSign) ) )
			{
			(void) sprintf(err_buf, "Multiple = in keyword line ... %s",
					dlist[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return the number of keyword instructions found */
	*maxnum = maxn;
	*list   = dlist;
	return num;
	}

/***********************************************************************
*                                                                      *
*    r e p l a c e _ s p e c i a l s                                   *
*    g e t b a c k _ s p e c i a l s                                   *
*                                                                      *
***********************************************************************/

static	void		replace_specials

	(
	STRING		buf
	)

	{
	size_t		nlen;
	int			ii;
	char		*token;

	/* Replace all special characters preceeded by '\\' */
	nlen = strlen(buf);
	ii = 0;
	while ( NotNull( token = strchr(buf+ii, '\\') ) )
		{

		/* Replace the next character if it is a special one */
		ii = token - buf + 1;
		if ( ii >= nlen ) break;
		switch ( buf[ii] )
			{
			case ';':				/* Replace ';' with '\001' */
				buf[ii] = '\001';
				break;
			case '!':				/* Replace '!' with '\002' */
				buf[ii] = '\002';
				break;
			case '=':				/* Replace '=' with '\007' */
				buf[ii] = '\007';
				break;
			case '{':				/* Replace '{' with '\033' */
				buf[ii] = '\033';
				break;
			case '}':				/* Replace '}' with '\035' */
				buf[ii] = '\035';
				break;
			case '\\':				/* Replace '\\' with '\034' */
				buf[ii] = '\034';
				break;
			default:
				break;
			}

		/* Reset counter to check the rest of the buffer */
		if ( ++ii >= nlen ) break;
		}

	/* Return the buffer with special characters replaced */
	return;
	}

static	void		getback_specials

	(
	STRING		buf
	)

	{
	size_t		nlen;
	int			ii, jj;
	LOGICAL		lastescape;
	char		tbuf[GPGHuge];

	/* Copy buffer character by character */
	nlen       = strlen(buf);
	lastescape = FALSE;
	for ( ii=0, jj=0; ii<nlen; ii++, jj++ )
		{

		/* Copy the current character */
		tbuf[jj] = buf[ii];

		/* Overwrite the previous character with a special character */
		/*  if the previous character was an escape character        */
		if ( lastescape )
			{

			/* Replace the previous character with a special character */
			switch ( buf[ii] )
				{
				case '\001':			/* Replace '\001' with ';' */
					tbuf[--jj] = ';';
					break;
				case '\002':			/* Replace '\002' with '!' */
					tbuf[--jj] = '!';
					break;
				case '\007':			/* Replace '\007' with '=' */
					tbuf[--jj] = '=';
					break;
				case '\033':			/* Replace '\033' with '{' */
					tbuf[--jj] = '{';
					break;
				case '\035':			/* Replace '\035' with '}' */
					tbuf[--jj] = '}';
					break;
				case '\034':			/* Replace '\034' with '\\' */
					tbuf[--jj] = '\\';
					break;

				/* Not a special character after all! */
				default:
					break;
				}

			/* Reset the escape character flag */
			lastescape = FALSE;
			}

		/* Set the escape character flag if the present character */
		/*  is an escape character                                */
		else if ( buf[ii] == '\\' )
			{
			lastescape = TRUE;
			}
		}

	/* Return the buffer with special characters returned */
	/*  ... and all escape characters removed             */
	tbuf[jj] = '\0';
	(void) strcpy(buf, tbuf);
	return;
	}

/***********************************************************************
*                                                                      *
*    s e t _ v e r s i o n                                             *
*                                                                      *
***********************************************************************/

/* Define structure for acceptable versions of applications */
typedef struct
	{
	STRING	name;
	LOGICAL	oldversion;
	LOGICAL	obsoleteversion;
	} GPG_VERSION_LIST;

/* Define acceptable versions of PSMet application */
static const GPG_VERSION_LIST PSMetVersions[] =
	{
		{ "psmet8.1",     FALSE, FALSE },
		{ "psmet8.0",     TRUE,  FALSE },
		{ "psmet7.0",     TRUE,  FALSE },
		{ "psmet6.0",     TRUE,  FALSE },
		{ "psmet5.0",     FALSE, TRUE  },
	};

/* Set number of acceptable versions of PSMet application */
static const int NumPSMetVersions =
	(int) (sizeof(PSMetVersions) / sizeof(GPG_VERSION_LIST));

/* Define acceptable versions of SVGMet application */
static const GPG_VERSION_LIST SVGMetVersions[] =
	{
		{ "svgmet8.1",     FALSE, FALSE },
		{ "svgmet8.0",     TRUE,  FALSE },
		{ "svgmet7.0",     TRUE,  FALSE },
		{ "svgmet6.0",     TRUE,  FALSE },
	};

/* Set number of acceptable versions of SVGMet application */
static const int NumSVGMetVersions =
	(int) (sizeof(SVGMetVersions) / sizeof(GPG_VERSION_LIST));

/* Define acceptable versions of CorMet application */
static const GPG_VERSION_LIST CorMetVersions[] =
	{
		{ "cormet8.1",     FALSE, FALSE },
		{ "cormet8.0",     TRUE,  FALSE },
		{ "cormet7.0",     TRUE,  FALSE },
		{ "cormet6.0",     TRUE,  FALSE },
		{ "cormet5.0",     FALSE, TRUE  },
		{ "cormet4.1gfa",  FALSE, TRUE  },
	};

/* Set number of acceptable versions of CorMet application */
static const int NumCorMetVersions =
	(int) (sizeof(CorMetVersions) / sizeof(GPG_VERSION_LIST));

/* Define acceptable versions of TexMet application */
static const GPG_VERSION_LIST TexMetVersions[] =
	{
		{ "texmet8.1",     FALSE, FALSE },
		{ "texmet8.0",     TRUE,  FALSE },
		{ "texmet7.0",     TRUE,  FALSE },
		{ "texmet6.0",     TRUE,  FALSE },
		{ "texmet5.0",     FALSE, TRUE  },
		{ "texmet4.1gfa",  FALSE, TRUE  },
	};

/* Set number of acceptable versions of TexMet application */
static const int NumTexMetVersions =
	(int) (sizeof(TexMetVersions) / sizeof(GPG_VERSION_LIST));

static	LOGICAL		set_version

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	const GPG_VERSION_LIST		*(GraphicsVersions);
	int							NumGraphicsVersions, nv;

	/* Only one value is allowed */
	if ( num != 1 ) return FALSE;

	/* Set version list to use ... based on application */
	switch ( Program.macro )
		{
		case GPG_PSMet:
			GraphicsVersions     = PSMetVersions;
			NumGraphicsVersions  = NumPSMetVersions;
			break;
		case GPG_SVGMet:
			GraphicsVersions     = SVGMetVersions;
			NumGraphicsVersions  = NumSVGMetVersions;
			break;
		case GPG_CorMet:
			GraphicsVersions     = CorMetVersions;
			NumGraphicsVersions  = NumCorMetVersions;
			break;
		case GPG_TexMet:
			GraphicsVersions     = TexMetVersions;
			NumGraphicsVersions  = NumTexMetVersions;
			break;
		}

	/* Set version from list of acceptable versions */
	if ( sscanf(list[0], "%s", Version) )
		{

		/* Match version against list of acceptable versions */
		for ( nv=0; nv<NumGraphicsVersions; nv++ )
			{
			if ( same(Version, GraphicsVersions[nv].name) )
				{
				(void) fprintf(stdout, "\nPDF File Version ... %s\n", Version);
				OldVersion      = GraphicsVersions[nv].oldversion;
				ObsoleteVersion = GraphicsVersions[nv].obsoleteversion;
				return TRUE;
				}
			}

		/* Error if version not found in list */
		if ( nv >= NumGraphicsVersions )
			{
			(void) fprintf(stderr,"\n*** Unacceptable version:  %s", Version);
			(void) fprintf(stderr,"\n*** Currently supported versions:");
			for ( nv=0; nv<NumGraphicsVersions; nv++ )
				{
				(void) fprintf(stderr,"  %s", GraphicsVersions[nv].name);
				}
			(void) fprintf(stderr,"\n");
			return FALSE;
			}
		}

	/* Return FALSE if problem reading version */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ f i l e _ n a m e                                         *
*    e x p a n d _ d i r _ c o d e w o r d s                           *
*    e x p a n d _ n a m e _ c o d e w o r d s                         *
*                                                                      *
***********************************************************************/

static	LOGICAL		set_file_name

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	STRING		out_dir, out_file;
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize current output directory and filename */
	out_dir  = FpaCblank;
	out_file = FpaCblank;

	/* Process each keyword */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		/* Process dir keyword */
		if ( same(key, "dir") )
			{
			out_dir = expand_dir_codewords(action);
			}

		/* Process name keyword */
		else if ( same(key, "name") )
			{
			out_file = expand_name_codewords(action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Now open the output file */
	if ( !open_graphics_file(out_dir, out_file) )
		{
		(void) sprintf(err_buf, "Error with directory ... %s  or file ... %s",
				out_dir, out_file);
		(void) error_report(err_buf);
		}

	return TRUE;
	}

static	STRING		expand_dir_codewords

	(
	STRING		dirbuf
	)

	{
	STRING		ps, pb, pe, pbn, xdir;
	char		tbuf[GPGLong], xbuf[GPGLong];
	char		err_buf[GPGLong];

	static	char	buf[GPGLong];

	/* Set pointer to input buffer and initialize output buffer */
	ps = dirbuf;
	(void) strcpy(buf, FpaCblank);

	/* Check input buffer for known codewords beginning with an angle bracket */
	while ( NotNull( pb = strchr(ps, OpenAngle) ) )
		{

		/* Ignore angle bracket if it is "escaped"                    */
		/* Note that input buffer should never begin with an "escape" */
		if ( pb != ps && *(pb-1) == '\\' )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Check for ending angle bracket ... and end if not found */
		if ( IsNull( pe = strchr(pb+1, CloseAngle) ) ) break;

		/* Check for another angle bracket before ending angle bracket */
		if ( NotNull( pbn = strchr(pb+1, OpenAngle) ) && pbn < pe )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Save the part of input buffer before angle brackets             */
		/*  ... and reset pointer for input buffer to after angle brackets */
		(void) strncat(buf, ps, (size_t) (pb-ps));
		ps = pe + 1;

		/* Extract the part of input buffer between angle brackets */
		/*  ... but also save original input buffer                */
		(void) strcpy(tbuf,  FpaCblank);
		(void) strncat(tbuf, pb+1, (size_t) (pe-pb-1));
		(void) strcpy(xbuf,  FpaCblank);
		(void) strncat(xbuf, pb,   (size_t) (pe-pb+1));

		/* Now check against known codewords */
		if ( same_ic(tbuf, "default") )
			(void) strcat(buf, DefaultOutDir);
		else if ( same_ic(tbuf, "psout") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "psmet") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "svgout") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "svgmet") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "corout") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "cormet") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "texout") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "texmet") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "home") )
			(void) strcat(buf, HomeDir);

		/* Check for codewords from other GPGen directories */
		else if ( same_ic(tbuf, "psout") )
			{
			xdir = get_directory("psout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "psmet") )
			{
			xdir = get_directory("psmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgout") )
			{
			xdir = get_directory("svgout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgmet") )
			{
			xdir = get_directory("svgmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "corout") )
			{
			xdir = get_directory("corout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... corout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "cormet") )
			{
			xdir = get_directory("cormet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... cormet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texout") )
			{
			xdir = get_directory("texout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texmet") )
			{
			xdir = get_directory("texmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texmet");
			(void) error_report(err_buf);
			}

		/* Check for possible spelling problems */
		else
			{

			/* None of the known codewords have embedded whitespace */
			if ( IsNull( strpbrk(tbuf, WHITE) ) )
				{
				(void) sprintf(err_buf, "Possible invalid codeword ... %s",
						xbuf);
				(void) warn_report(err_buf);
				}

			/* Save original string unchanged */
			(void) strcat(buf, xbuf);
			}

		/* Check remainder of input buffer for angle brackets */
		}

	/* Append any remaining portion of input buffer */
	(void) strcat(buf, ps);

	/* Return output buffer */
	return buf;
	}

static	STRING		expand_name_codewords

	(
	STRING		namebuf
	)

	{
	LOGICAL		rlocal, vlocal, status;
	int			ryear, rjday, rhour, rminute, rmon, rmday;
	int			vyear, vjday, vhour, vminute, vmon, vmday;
	int			cyear, cjday, chour, cminute, cmon, cmday;
	int			pmin, amin, citer;
	STRING		ps, pb, pe, pbn;
	LOOP_STRUCT	*Ploop = NullPtr(LOOP_STRUCT *);
	char		tbuf[GPGLong], xbuf[GPGLong], ibuf[GPGTiny];
	char		err_buf[GPGLong];

	static	char	buf[GPGLong];

	/* Set pointer to input buffer and initialize output buffer */
	ps = namebuf;
	(void) strcpy(buf, FpaCblank);

	/* Read the current T0 timestamp */
	if ( !parse_tstamp(T0stamp, &ryear, &rjday, &rhour, &rminute, &rlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading issue time ... %s", T0stamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&ryear, &rjday, &rmon, &rmday);

	/* Read the current TV timestamp */
	if ( !parse_tstamp(TVstamp, &vyear, &vjday, &vhour, &vminute, &vlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading valid time ... %s", TVstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&vyear, &vjday, &vmon, &vmday);

	/* Read the current TC timestamp */
	if ( !parse_tstamp(TCstamp, &cyear, &cjday, &chour, &cminute,
						NullLogicalPtr, NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading creation time ... %s", TCstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&cyear, &cjday, &cmon, &cmday);

	/* Determine current prog time */
	pmin = calc_prog_time_minutes(T0stamp, TVstamp, &status);
	if ( !status )
		{
		(void) sprintf(err_buf,
				"Error calculating prog time from TVstamp - T0stamp ... %s %s",
				TVstamp, T0stamp);
		(void) error_report(err_buf);
		}

	/* Identify the current iteration/attribute of the currently active loop */
	if ( NumLoops > 0 )
		{
		Ploop = &Loops[NumLoops-1];
		citer = Ploop->current_iteration;
		}
	else
		{
		citer = 0;
		}

	/* Check input buffer for known codewords beginning with an angle bracket */
	while ( NotNull( pb = strchr(ps, OpenAngle) ) )
		{

		/* Ignore angle bracket if it is "escaped"                    */
		/* Note that input buffer should never begin with an "escape" */
		if ( pb != ps && *(pb-1) == '\\' )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Check for ending angle bracket ... and end if not found */
		if ( IsNull( pe = strchr(pb+1, CloseAngle) ) ) break;

		/* Check for another angle bracket before ending angle bracket */
		if ( NotNull( pbn = strchr(pb+1, OpenAngle) ) && pbn < pe )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Save the part of input buffer before angle brackets             */
		/*  ... and reset pointer for input buffer to after angle brackets */
		(void) strncat(buf, ps, (size_t) (pb-ps));
		ps = pe + 1;

		/* Extract the part of input buffer between angle brackets */
		/*  ... but also save original input buffer                */
		(void) strcpy(tbuf,  FpaCblank);
		(void) strncat(tbuf, pb+1, (size_t) (pe-pb-1));
		(void) strcpy(xbuf,  FpaCblank);
		(void) strncat(xbuf, pb,   (size_t) (pe-pb+1));

		/* Now check against known codewords */
		if ( same_ic(tbuf, "pdf") )
			(void) strcat(buf, DefaultPdfFile);
		else if ( same_ic(tbuf, "year") )
			{
			(void) sprintf(ibuf, "%.4d", ryear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "month") )
			{
			(void) sprintf(ibuf, "%.2d", rmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "day") )
			{
			(void) sprintf(ibuf, "%.2d", rmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "julian") )
			{
			(void) sprintf(ibuf, "%.3d", rjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "hour") )
			{
			(void) sprintf(ibuf, "%.2d", rhour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "minute") )
			{
			(void) sprintf(ibuf, "%.2d", rminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "local") )
			{
			if ( rlocal ) (void) strcat(buf, "L");
			}
		else if ( same_ic(tbuf, "v_year") )
			{
			(void) sprintf(ibuf, "%.4d", vyear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_month") )
			{
			(void) sprintf(ibuf, "%.2d", vmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_day") )
			{
			(void) sprintf(ibuf, "%.2d", vmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_julian") )
			{
			(void) sprintf(ibuf, "%.3d", vjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_hour") )
			{
			(void) sprintf(ibuf, "%.2d", vhour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_minute") )
			{
			(void) sprintf(ibuf, "%.2d", vminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_local") )
			{
			if ( vlocal ) (void) strcat(buf, "L");
			}
		else if ( same_ic(tbuf, "c_year") )
			{
			(void) sprintf(ibuf, "%.4d", cyear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_month") )
			{
			(void) sprintf(ibuf, "%.2d", cmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_day") )
			{
			(void) sprintf(ibuf, "%.2d", cmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_julian") )
			{
			(void) sprintf(ibuf, "%.3d", cjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_hour") )
			{
			(void) sprintf(ibuf, "%.2d", chour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_minute") )
			{
			(void) sprintf(ibuf, "%.2d", cminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_hour") )
			{
			(void) sprintf(ibuf, "%.2d", NINT((float) pmin / 60.0));
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_minute") )
			{
			(void) sprintf(ibuf, "%d", pmin);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_hr_min") )
			{
			amin = abs(pmin);
			if ( pmin < 0 )
				(void) sprintf(ibuf, "-%.2d:%.2d", amin/60, amin%60);
			else
				(void) sprintf(ibuf,  "%.2d:%.2d", amin/60, amin%60);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "iteration") )
			{
			(void) sprintf(ibuf, "%d", citer);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "iteration_attribute") )
			{
			(void) strcat(buf, CurAttribute);
			}

		/* Check for possible spelling problems */
		else
			{

			/* None of the known codewords have embedded whitespace */
			if ( IsNull( strpbrk(tbuf, WHITE) ) )
				{
				(void) sprintf(err_buf, "Possible invalid codeword ... %s",
						xbuf);
				(void) warn_report(err_buf);
				}

			/* Save original string unchanged */
			(void) strcat(buf, xbuf);
			}

		/* Check remainder of input buffer for angle brackets */
		}

	/* Append any remaining portion of input buffer */
	(void) strcat(buf, ps);

	/* Return output buffer */
	return buf;
	}

/***********************************************************************
*                                                                      *
*    e x p a n d _ f i l e _ c o d e w o r d s                         *
*                                                                      *
***********************************************************************/

static	STRING		expand_file_codewords

	(
	STRING		filebuf
	)

	{
	STRING		ps, pb, pe, pbn, xdir;
	char		tbuf[GPGLong], xbuf[GPGLong];
	char		err_buf[GPGLong];

	static	char	buf[GPGLong];

	/* Set pointer to input buffer and initialize output buffer */
	ps = filebuf;
	(void) strcpy(buf, FpaCblank);

	/* Check input buffer for known codewords beginning with an angle bracket */
	while ( NotNull( pb = strchr(ps, OpenAngle) ) )
		{

		/* Ignore angle bracket if it is "escaped"                    */
		/* Note that input buffer should never begin with an "escape" */
		if ( pb != ps && *(pb-1) == '\\' )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Check for ending angle bracket ... and end if not found */
		if ( IsNull( pe = strchr(pb+1, CloseAngle) ) ) break;

		/* Check for another angle bracket before ending angle bracket */
		if ( NotNull( pbn = strchr(pb+1, OpenAngle) ) && pbn < pe )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Save the part of input buffer before angle brackets             */
		/*  ... and reset pointer for input buffer to after angle brackets */
		(void) strncat(buf, ps, (size_t) (pb-ps));
		ps = pe + 1;

		/* Extract the part of input buffer between angle brackets */
		/*  ... but also save original input buffer                */
		(void) strcpy(tbuf,  FpaCblank);
		(void) strncat(tbuf, pb+1, (size_t) (pe-pb-1));
		(void) strcpy(xbuf,  FpaCblank);
		(void) strncat(xbuf, pb,   (size_t) (pe-pb+1));

		/* Now check against known codewords */
		if ( same_ic(tbuf, "default") )
			(void) strcat(buf, DefaultOutDir);
		else if ( same_ic(tbuf, "psout") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "psmet") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "svgout") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "svgmet") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "corout") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "cormet") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "texout") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "texmet") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "home") )
			(void) strcat(buf, HomeDir);

		/* Check for codewords from other GPGen directories */
		else if ( same_ic(tbuf, "psout") )
			{
			xdir = get_directory("psout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "psmet") )
			{
			xdir = get_directory("psmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgout") )
			{
			xdir = get_directory("svgout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgmet") )
			{
			xdir = get_directory("svgmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "corout") )
			{
			xdir = get_directory("corout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... corout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "cormet") )
			{
			xdir = get_directory("cormet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... cormet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texout") )
			{
			xdir = get_directory("texout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texmet") )
			{
			xdir = get_directory("texmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texmet");
			(void) error_report(err_buf);
			}

		/* Check for possible spelling problems */
		else
			{

			/* None of the known codewords have embedded whitespace */
			if ( IsNull( strpbrk(tbuf, WHITE) ) )
				{
				(void) sprintf(err_buf, "Possible invalid codeword ... %s",
						xbuf);
				(void) warn_report(err_buf);
				}

			/* Save original string unchanged */
			(void) strcat(buf, xbuf);
			}

		/* Check remainder of input buffer for angle brackets */
		}

	/* Append any remaining portion of input buffer */
	(void) strcat(buf, ps);

	/* Return output buffer */
	return buf;
	}

/***********************************************************************
*                                                                      *
*    r u n _ e x t e r n a l _ p r o c e s s                           *
*    e x p a n d _ p r o c e s s _ c o d e w o r d s                   *
*                                                                      *
***********************************************************************/

static	LOGICAL		run_external_process

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii, status;
	STRING		key, action;
	char		buf[GPGHuge], err_buf[GPGLong];

	/* Flush the current output file if it is still open! */
	if ( NotNull(FP_Out) ) (void) fflush(FP_Out);

	/* Initialize current command line */
	(void) strcpy(buf, FpaCblank);

	/* Process each command */
	for ( ii=0; ii<num; ii++ )
		{

		(void) parse_instruction(list[ii], &key, &action);

		/* Append each command to the current command line */
		if ( ii > 0 ) (void) strcat(buf, "; ");

		/* Append to command line */
		(void) strcat(buf, expand_process_codewords(action));
		}

	/* Now execute the command line */
	(void) fprintf(stdout, " Running external process ... %s\n", buf);
	status = shrun(buf, TRUE);
	if ( status != 0 )
		{
		(void) sprintf(err_buf, "Error processing command line ... %s", buf);
		(void) error_report(err_buf);
		}

	return TRUE;
	}

static	STRING		expand_process_codewords

	(
	STRING		probuf
	)

	{
	LOGICAL		rlocal, vlocal, status;
	int			ryear, rjday, rhour, rminute, rmon, rmday;
	int			vyear, vjday, vhour, vminute, vmon, vmday;
	int			cyear, cjday, chour, cminute, cmon, cmday;
	int			pmin, amin, citer;
	STRING		ps, pb, pe, pbn, pext, xdir;
	LOOP_STRUCT	*Ploop = NullPtr(LOOP_STRUCT *);
	char		tbuf[GPGLong], xbuf[GPGLong], sbuf[GPGLong], ibuf[GPGTiny];
	char		err_buf[GPGLong];

	static	char	buf[GPGHuge];

	/* Set pointer to input buffer and initialize output buffer */
	ps = probuf;
	(void) strcpy(buf, FpaCblank);

	/* Read the current T0 timestamp */
	if ( !parse_tstamp(T0stamp, &ryear, &rjday, &rhour, &rminute, &rlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading issue time ... %s", T0stamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&ryear, &rjday, &rmon, &rmday);

	/* Read the current TV timestamp */
	if ( !parse_tstamp(TVstamp, &vyear, &vjday, &vhour, &vminute, &vlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading valid time ... %s", TVstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&vyear, &vjday, &vmon, &vmday);

	/* Read the current TC timestamp */
	if ( !parse_tstamp(TCstamp, &cyear, &cjday, &chour, &cminute,
						NullLogicalPtr, NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading creation time ... %s", TCstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&cyear, &cjday, &cmon, &cmday);

	/* Determine current prog time */
	pmin = calc_prog_time_minutes(T0stamp, TVstamp, &status);
	if ( !status )
		{
		(void) sprintf(err_buf,
				"Error calculating prog time from TVstamp - T0stamp ... %s %s",
				TVstamp, T0stamp);
		(void) error_report(err_buf);
		}

	/* Identify the current iteration/attribute of the currently active loop */
	if ( NumLoops > 0 )
		{
		Ploop = &Loops[NumLoops-1];
		citer = Ploop->current_iteration;
		}
	else
		{
		citer = 0;
		}

	/* Check input buffer for known codewords beginning with an angle bracket */
	while ( NotNull( pb = strchr(ps, OpenAngle) ) )
		{

		/* Ignore angle bracket if it is "escaped"                    */
		/* Note that input buffer should never begin with an "escape" */
		if ( pb != ps && *(pb-1) == '\\' )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Check for ending angle bracket ... and end if not found */
		if ( IsNull( pe = strchr(pb+1, CloseAngle) ) ) break;

		/* Check for another angle bracket before ending angle bracket */
		if ( NotNull( pbn = strchr(pb+1, OpenAngle) ) && pbn < pe )
			{
			(void) strncat(buf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Save the part of input buffer before angle brackets             */
		/*  ... and reset pointer for input buffer to after angle brackets */
		(void) strncat(buf, ps, (size_t) (pb-ps));
		ps = pe + 1;

		/* Extract the part of input buffer between angle brackets */
		/*  ... but also save original input buffer                */
		(void) strcpy(tbuf,  FpaCblank);
		(void) strncat(tbuf, pb+1, (size_t) (pe-pb-1));
		(void) strcpy(xbuf,  FpaCblank);
		(void) strncat(xbuf, pb,   (size_t) (pe-pb+1));

		/* Now check against known codewords */
		if ( same_ic(tbuf, "SETUP") )
			(void) strcat(buf, SetupFile);
		else if ( same_ic(tbuf, "RTIME") )
			(void) strcat(buf, T0stamp);
		else if ( same_ic(tbuf, "VTIME") )
			(void) strcat(buf, TVstamp);
		else if ( same_ic(tbuf, "file_name") )
			(void) strcat(buf, OutputFileName);
		else if ( same_ic(tbuf, "file_name_base") )
			{
			(void) strcpy(sbuf, OutputFileName);
			switch ( Program.macro )
				{
				case GPG_PSMet:
					pext = strrchr(sbuf, '.');
					if ( NotNull(pext) && same(pext, ".ps") )  *pext = '\0';
					break;
				case GPG_SVGMet:
					pext = strrchr(sbuf, '.');
					if ( NotNull(pext) && same(pext, ".svg") ) *pext = '\0';
					break;
				case GPG_CorMet:
					pext = strrchr(sbuf, '.');
					if ( NotNull(pext) && same(pext, ".cmf") ) *pext = '\0';
					break;
				case GPG_TexMet:
					pext = strrchr(sbuf, '.');
					if ( NotNull(pext) && same(pext, ".txt") ) *pext = '\0';
					break;
				}
			(void) strcat(buf, sbuf);
			}
		else if ( same_ic(tbuf, "default") )
			(void) strcat(buf, DefaultOutDir);
		else if ( same_ic(tbuf, "psout") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "psmet") && (Program.macro == GPG_PSMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "svgout") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "svgmet") && (Program.macro == GPG_SVGMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "corout") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "cormet") && (Program.macro == GPG_CorMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "texout") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BaseOutDir);
		else if ( same_ic(tbuf, "texmet") && (Program.macro == GPG_TexMet) )
			(void) strcat(buf, BasePdfDir);
		else if ( same_ic(tbuf, "home") )
			(void) strcat(buf, HomeDir);
		else if ( same_ic(tbuf, "pdf") )
			(void) strcat(buf, DefaultPdfFile);
		else if ( same_ic(tbuf, "year") )
			{
			(void) sprintf(ibuf, "%.4d", ryear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "month") )
			{
			(void) sprintf(ibuf, "%.2d", rmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "day") )
			{
			(void) sprintf(ibuf, "%.2d", rmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "julian") )
			{
			(void) sprintf(ibuf, "%.3d", rjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "hour") )
			{
			(void) sprintf(ibuf, "%.2d", rhour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "minute") )
			{
			(void) sprintf(ibuf, "%.2d", rminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "local") )
			{
			if ( rlocal ) (void) strcat(buf, "L");
			}
		else if ( same_ic(tbuf, "v_year") )
			{
			(void) sprintf(ibuf, "%.4d", vyear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_month") )
			{
			(void) sprintf(ibuf, "%.2d", vmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_day") )
			{
			(void) sprintf(ibuf, "%.2d", vmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_julian") )
			{
			(void) sprintf(ibuf, "%.3d", vjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_hour") )
			{
			(void) sprintf(ibuf, "%.2d", vhour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_minute") )
			{
			(void) sprintf(ibuf, "%.2d", vminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "v_local") )
			{
			if ( vlocal ) (void) strcat(buf, "L");
			}
		else if ( same_ic(tbuf, "c_year") )
			{
			(void) sprintf(ibuf, "%.4d", cyear);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_month") )
			{
			(void) sprintf(ibuf, "%.2d", cmon);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_day") )
			{
			(void) sprintf(ibuf, "%.2d", cmday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_julian") )
			{
			(void) sprintf(ibuf, "%.3d", cjday);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_hour") )
			{
			(void) sprintf(ibuf, "%.2d", chour);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "c_minute") )
			{
			(void) sprintf(ibuf, "%.2d", cminute);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_hour") )
			{
			(void) sprintf(ibuf, "%.2d", NINT((float) pmin / 60.0));
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_minute") )
			{
			(void) sprintf(ibuf, "%d", pmin);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "p_hr_min") )
			{
			amin = abs(pmin);
			if ( pmin < 0 )
				(void) sprintf(ibuf, "-%.2d:%.2d", amin/60, amin%60);
			else
				(void) sprintf(ibuf,  "%.2d:%.2d", amin/60, amin%60);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "iteration") )
			{
			(void) sprintf(ibuf, "%d", citer);
			(void) strcat(buf, ibuf);
			}
		else if ( same_ic(tbuf, "iteration_attribute") )
			{
			(void) strcat(buf, CurAttribute);
			}

		/* Check for codewords from other GPGen directories */
		else if ( same_ic(tbuf, "psout") )
			{
			xdir = get_directory("psout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "psmet") )
			{
			xdir = get_directory("psmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... psmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgout") )
			{
			xdir = get_directory("svgout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "svgmet") )
			{
			xdir = get_directory("svgmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... svgmet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "corout") )
			{
			xdir = get_directory("corout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... corout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "cormet") )
			{
			xdir = get_directory("cormet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... cormet");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texout") )
			{
			xdir = get_directory("texout");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texout");
			(void) error_report(err_buf);
			}
		else if ( same_ic(tbuf, "texmet") )
			{
			xdir = get_directory("texmet");
			if ( !blank(xdir) )
				{
				(void) strcat(buf, xdir);
				continue;
				}
			(void) sprintf(err_buf, "Invalid codeword for program ... texmet");
			(void) error_report(err_buf);
			}

		/* Check for possible spelling problems */
		else
			{

			/* None of the known codewords have embedded whitespace */
			if ( IsNull( strpbrk(tbuf, WHITE) ) )
				{
				(void) sprintf(err_buf, "Possible invalid codeword ... %s",
						xbuf);
				(void) warn_report(err_buf);
				}

			/* Save original string unchanged */
			(void) strcat(buf, xbuf);
			}

		/* Check remainder of input buffer for angle brackets */
		}

	/* Append any remaining portion of input buffer */
	(void) strcat(buf, ps);

	/* Return output buffer */
	return buf;
	}

/***********************************************************************
*                                                                      *
*    i n s e r t _ i n t o _ s v g                                     *
*    e x p a n d _ i n s e r t _ c o d e w o r d s                     *
*                                                                      *
***********************************************************************/

static	LOGICAL		insert_into_svg

	(
	STRING		body
	)

	{
	char		*token;
	char		buf[GPGHuge];
	STRING		outbuf;

	/* Flush the current output file if it is still open! */
	if ( NotNull(FP_Out) ) (void) fflush(FP_Out);

	/* Initialize buffer */
	(void) strcpy(buf, body);

	/* Remove start and end braces { } */
	token = strchr(buf, OpenBrace);
	if ( IsNull(token) ) return FALSE;
	(void) strcpy(buf, token+1);

	token = strchr(buf, CloseBrace);
	if ( IsNull(token) ) return FALSE;
	*token = '\0';

	/* Get back any special characters preceded by an escape character */
	(void) getback_specials(buf);

	/* Remove white space from beginning and end of buffer */
	(void) no_white(buf);

	/* Replace SVG insert codewords */
	outbuf = expand_insert_codewords(buf);

	/* Ensure that an output file is open and initialized */
	if ( NotInitialized ) (void) write_svgmet_comment(FpaCblank);

	/* Now insert the buffer into SVG output file */
	if ( !blank(outbuf) ) (void) fprintf(FP_Out, "%s\n", outbuf);

	return TRUE;
	}

static	STRING		expand_insert_codewords

	(
	STRING		inbuf
	)

	{
	LOGICAL		rlocal, vlocal, status;
	int			ryear, rjday, rhour, rminute, rmon, rmday;
	int			vyear, vjday, vhour, vminute, vmon, vmday;
	int			cyear, cjday, chour, cminute, cmon, cmday;
	int			pmin, amin, citer;
	STRING		ps, pb, pe, pbn;
	LOOP_STRUCT	*Ploop = NullPtr(LOOP_STRUCT *);
	char		tbuf[GPGHuge], xbuf[GPGHuge], ibuf[GPGTiny];
	char		err_buf[GPGLong];

	static	char	outbuf[GPGHuge];

	/* Set pointer to input buffer and initialize output buffer */
	ps = inbuf;
	(void) strcpy(outbuf, FpaCblank);

	/* Read the current T0 timestamp */
	if ( !parse_tstamp(T0stamp, &ryear, &rjday, &rhour, &rminute, &rlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading issue time ... %s", T0stamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&ryear, &rjday, &rmon, &rmday);

	/* Read the current TV timestamp */
	if ( !parse_tstamp(TVstamp, &vyear, &vjday, &vhour, &vminute, &vlocal,
						NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading valid time ... %s", TVstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&vyear, &vjday, &vmon, &vmday);

	/* Read the current TC timestamp */
	if ( !parse_tstamp(TCstamp, &cyear, &cjday, &chour, &cminute,
						NullLogicalPtr, NullLogicalPtr) )
		{
		(void) sprintf(err_buf, "Error reading creation time ... %s", TCstamp);
		(void) error_report(err_buf);
		}
	(void) mdate(&cyear, &cjday, &cmon, &cmday);

	/* Determine current prog time */
	pmin = calc_prog_time_minutes(T0stamp, TVstamp, &status);
	if ( !status )
		{
		(void) sprintf(err_buf,
				"Error calculating prog time from TVstamp - T0stamp ... %s %s",
				TVstamp, T0stamp);
		(void) error_report(err_buf);
		}

	/* Identify the current iteration/attribute of the currently active loop */
	if ( NumLoops > 0 )
		{
		Ploop = &Loops[NumLoops-1];
		citer = Ploop->current_iteration;
		}
	else
		{
		citer = 0;
		}

	/* Check input buffer for known codewords beginning with an angle bracket */
	while ( NotNull( pb = strchr(ps, OpenAngle) ) )
		{

		/* Ignore angle bracket if it is "escaped"                    */
		/* Note that input buffer should never begin with an "escape" */
		if ( pb != ps && *(pb-1) == '\\' )
			{
			(void) strncat(outbuf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Check for ending angle bracket ... and end if not found */
		if ( IsNull( pe = strchr(pb+1, CloseAngle) ) ) break;

		/* Check for another angle bracket before ending angle bracket */
		if ( NotNull( pbn = strchr(pb+1, OpenAngle) ) && pbn < pe )
			{
			(void) strncat(outbuf, ps, (size_t) (pb-ps+1));
			ps = pb + 1;
			continue;
			}

		/* Save the part of input buffer before angle brackets             */
		/*  ... and reset pointer for input buffer to after angle brackets */
		(void) strncat(outbuf, ps, (size_t) (pb-ps));
		ps = pe + 1;

		/* Extract the part of input buffer between angle brackets */
		/*  ... but also save original input buffer                */
		(void) strcpy(tbuf,  FpaCblank);
		(void) strncat(tbuf, pb+1, (size_t) (pe-pb-1));
		(void) strcpy(xbuf,  FpaCblank);
		(void) strncat(xbuf, pb,   (size_t) (pe-pb+1));

		/* Now check against known codewords */
		if ( same_ic(tbuf, "GPGEN_default") )
			(void) strcat(outbuf, DefaultOutDir);
		else if ( same_ic(tbuf, "GPGEN_svgout") )
			(void) strcat(outbuf, BaseOutDir);
		else if ( same_ic(tbuf, "GPGEN_home") )
			(void) strcat(outbuf, HomeDir);
		else if ( same_ic(tbuf, "GPGEN_year") )
			{
			(void) sprintf(ibuf, "%.4d", ryear);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_month") )
			{
			(void) sprintf(ibuf, "%.2d", rmon);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_day") )
			{
			(void) sprintf(ibuf, "%.2d", rmday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_julian") )
			{
			(void) sprintf(ibuf, "%.3d", rjday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_hour") )
			{
			(void) sprintf(ibuf, "%.2d", rhour);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_minute") )
			{
			(void) sprintf(ibuf, "%.2d", rminute);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_local") )
			{
			if ( rlocal ) (void) strcat(outbuf, "L");
			}
		else if ( same_ic(tbuf, "GPGEN_v_year") )
			{
			(void) sprintf(ibuf, "%.4d", vyear);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_month") )
			{
			(void) sprintf(ibuf, "%.2d", vmon);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_day") )
			{
			(void) sprintf(ibuf, "%.2d", vmday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_julian") )
			{
			(void) sprintf(ibuf, "%.3d", vjday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_hour") )
			{
			(void) sprintf(ibuf, "%.2d", vhour);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_minute") )
			{
			(void) sprintf(ibuf, "%.2d", vminute);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_v_local") )
			{
			if ( vlocal ) (void) strcat(outbuf, "L");
			}
		else if ( same_ic(tbuf, "GPGEN_c_year") )
			{
			(void) sprintf(ibuf, "%.4d", cyear);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_c_month") )
			{
			(void) sprintf(ibuf, "%.2d", cmon);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_c_day") )
			{
			(void) sprintf(ibuf, "%.2d", cmday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_c_julian") )
			{
			(void) sprintf(ibuf, "%.3d", cjday);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_c_hour") )
			{
			(void) sprintf(ibuf, "%.2d", chour);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_c_minute") )
			{
			(void) sprintf(ibuf, "%.2d", cminute);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_p_hour") )
			{
			(void) sprintf(ibuf, "%.2d", NINT((float) pmin / 60.0));
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_p_minute") )
			{
			(void) sprintf(ibuf, "%d", pmin);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_p_hr_min") )
			{
			amin = abs(pmin);
			if ( pmin < 0 )
				(void) sprintf(ibuf, "-%.2d:%.2d", amin/60, amin%60);
			else
				(void) sprintf(ibuf,  "%.2d:%.2d", amin/60, amin%60);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_iteration") )
			{
			(void) sprintf(ibuf, "%d", citer);
			(void) strcat(outbuf, ibuf);
			}
		else if ( same_ic(tbuf, "GPGEN_iteration_attribute") )
			{
			(void) strcat(outbuf, CurAttribute);
			}

		/* Check for possible spelling problems */
		else
			{

			/* None of the known codewords have embedded whitespace */
			if ( IsNull( strpbrk(tbuf, WHITE) ) )
				{
				(void) sprintf(err_buf, "Possible invalid codeword ... %s",
						xbuf);
				(void) warn_report(err_buf);
				}

			/* Save original string unchanged */
			(void) strcat(outbuf, xbuf);
			}

		/* Check remainder of input buffer for angle brackets */
		}

	/* Append any remaining portion of input buffer */
	(void) strcat(outbuf, ps);

	/* Return output buffer */
	return outbuf;
	}

/***********************************************************************
*                                                                      *
*    i n c l u d e _ p d f _ f i l e                                   *
*                                                                      *
***********************************************************************/

static	LOGICAL		include_pdf_file

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		fname;

	/* Process all "included" files */
	for ( ii=0; ii<num; ii++ )
		{

		/* Find the fpdf file */
		(void) no_white(list[ii]);
		fname = find_pdf_file(list[ii]);
		if ( blank(fname) )
			{
			(void) fprintf(stderr, "*** Cannot find fpdf file %s\n", list[ii]);
			return FALSE;
			}

		/* Process the fpdf file */
		if ( !process_pdf_file(fname) ) return FALSE;
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ l o o p _ p a r a m e t e r s                             *
*    r e s e t _ l o o p                                               *
*    b u i l d _ l o o p _ l o c a t i o n _ l o o k _ u p             *
*                                                                      *
***********************************************************************/

static	LOGICAL		set_loop_parameters

	(
	char		list[][GPGMedium],
	int			num,
	FILE		*fpdf
	)

	{
	int				ii, jj, kk, ll, fkind, isub, ihole;
	char			element[GPGMedium], level[GPGMedium];
	char			field_type[GPGMedium], area_type[GPGMedium];
	char			cat_cascade[GPGMedium], attribute[GPGMedium];
	int				niters, num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	STRING			key, action;
	char			err_buf[GPGLong];

	LOOP_STRUCT		*Ploop   = NullPtr(LOOP_STRUCT *);
	LOOP_GROUP		*Pgroup  = NullPtr(LOOP_GROUP *);
	LOOP_KEYVALS	*Pkeyval = NullPtr(LOOP_KEYVALS *);
	LOOP_KEYVALS	*Pdefval = NullPtr(LOOP_KEYVALS *);

	FLD_DESCRIPT	descript;
	SET				set, copyset;
	AREA			area, carea;
	CURVE			curve;
	SPOT			spot;
	LCHAIN			lchain;

	/* Initialize all parameters */
	(void) strcpy(element,        FpaCblank);
	(void) strcpy(level,          FpaCblank);
	(void) strcpy(field_type,     FpaCblank);
	(void) strcpy(area_type,      AreaTypeSubareas);
	(void) strcpy(cat_cascade,    CatCascadeAnd);
	(void) strcpy(attribute,      AttribAutolabel);
	num_catatt = 0;
	num_att    = 0;
	num_cat    = 0;

	/* Allocate space for another set of loop parameters */
	NumLoops++;
	Loops = GETMEM(Loops, LOOP_STRUCT, NumLoops);
	Ploop = &Loops[NumLoops-1];
	Ploop->num_iterations    = 0;
	Ploop->current_iteration = 0;
	Ploop->num_groups        = 0;
	Ploop->loop_groups       = NullPtr(LOOP_GROUP *);
	Ploop->fld_set           = NullSet;
	Ploop->fld_attrib        = NullString;
	Ploop->fld_vtime         = NullString;
	Ploop->file_pointer      = fpdf;
	Ploop->file_position     = ftell(fpdf);

	/* Process all keywords in list */
	for ( ii=0, jj=-1, kk=-1; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "number_of_iterations") )
			{
			(void) sscanf(action, "%d", &niters);
			if ( niters <= 0 )
				{
				(void) sprintf(err_buf,
						" Error in \"number_of_iterations\" ... %s\n", action);
				(void) error_report(err_buf);
				}
			Ploop->num_iterations = niters;
			}

		else if ( same(key, "group_name") )
			{
			/* Check for this group name in the list */
			for ( jj=0; jj<Ploop->num_groups; jj++ )
				{
				if ( same(Ploop->loop_groups[jj].group_name, action) ) break;
				}

			/* Add another group */
			if ( jj == Ploop->num_groups )
				{

				/* Add space for another group */
				Ploop->num_groups++;
				Ploop->loop_groups = GETMEM(Ploop->loop_groups, LOOP_GROUP,
															Ploop->num_groups);
				Pgroup = &Ploop->loop_groups[jj];
				Pgroup->group_name = strdup(action);
				Pgroup->num_keys   = 0;
				Pgroup->keywords   = NullStringList;
				Pgroup->keyvals    = NullPtr(LOOP_KEYVALS *);
				Pgroup->defvals    = NullPtr(LOOP_KEYVALS *);
				kk = -1;
				}
			}

		else if ( same(key, "keyword_name") )
			{
			if ( jj < 0 )
				{
				(void) error_report("Missing \"group_name\" keyword!");
				}

			/* Add space for another keyword */
			kk = Pgroup->num_keys++;
			Pgroup->keywords = GETMEM(Pgroup->keywords, STRING,
															Pgroup->num_keys);
			Pgroup->keyvals  = GETMEM(Pgroup->keyvals,  LOOP_KEYVALS,
															Pgroup->num_keys);
			Pgroup->keywords[kk] = strdup(action);
			Pkeyval = &Pgroup->keyvals[kk];
			Pkeyval->num_vals = 0;
			Pkeyval->values   = NullStringList;
			}

		else if ( same(key, "keyword_value") )
			{
			if ( jj < 0 )
				{
				(void) error_report("Missing \"group_name\" keyword!");
				}

			/* Add a single value to the default list */
			if ( kk < 0 )
				{

				/* Add space for the default value list (if required) */
				if ( IsNull(Pgroup->defvals) )
					{
					Pgroup->defvals = INITMEM(LOOP_KEYVALS, 1);
					Pdefval = Pgroup->defvals;
					Pdefval->num_vals = 0;
					Pdefval->values   = NullStringList;
					}
				
				/* Add a single value to the default list */
				ll = Pdefval->num_vals++;
				Pdefval->values = GETMEM(Pdefval->values, STRING,
															Pdefval->num_vals);
				Pdefval->values[ll] = strdup(action);
				}

			/* Add a single value to the keyword list */
			else
				{
				ll = Pkeyval->num_vals++;
				Pkeyval->values = GETMEM(Pkeyval->values, STRING,
															Pkeyval->num_vals);
				Pkeyval->values[ll] = strdup(action);
				}
			}

		else if ( same(key, "keyword_value_list") )
			{
			if ( jj < 0 )
				{
				(void) error_report("Missing \"group_name\" keyword!");
				}

			/* Add multiple values to the default list */
			if ( kk < 0 )
				{

				/* Add space for the default value list (if required) */
				if ( IsNull(Pgroup->defvals) )
					{
					Pgroup->defvals = INITMEM(LOOP_KEYVALS, 1);
					Pdefval = Pgroup->defvals;
					Pdefval->num_vals = 0;
					Pdefval->values   = NullStringList;
					}
				
				/* Add multiple values to the default list */
				while ( !blank(action) )
					{
					ll = Pdefval->num_vals++;
					Pdefval->values = GETMEM(Pdefval->values, STRING,
																Pdefval->num_vals);
					Pdefval->values[ll] = strdup(string_arg(action));
					}
				}

			/* Add multiple values to the keyword list */
			else
				{
				while ( !blank(action) )
					{
					ll = Pkeyval->num_vals++;
					Pkeyval->values = GETMEM(Pkeyval->values, STRING,
																Pkeyval->num_vals);
					Pkeyval->values[ll] = strdup(string_arg(action));
					}
				}
			}

		else if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "field_type") )
			{
			(void) strcpy(field_type, action);
			}

		else if ( same(key, "area_type") )
			{
			(void) strcpy(area_type, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Error return for incorrect "area_type" */
	if ( !same(area_type, AreaTypeSubareas)
			&& !same(area_type, AreaTypeBoundary) )
		{
		(void) sprintf(err_buf, "Recognized area types: %s %s",
				AreaTypeSubareas, AreaTypeBoundary);
		(void) error_report(err_buf);
		}

	/* Set number of features from "element" and "level" */
	if ( !blank(element) && !blank(level) )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
										   FpaF_SOURCE_NAME,   CurSource,
										   FpaF_RUN_TIME,      FpaCblank,
										   FpaF_ELEMENT_NAME,  element,
										   FpaF_LEVEL_NAME,    level,
										   FpaF_VALID_TIME,    TVstamp,
										   FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					element, level, CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Set the field type from the element and level */
		fkind = descript.edef->fld_type;

		/* Reset the field type (if requested) */
		if ( !blank(field_type) )
			{
			fkind = field_data_type(field_type);
			if ( fkind == FpaCnoMacro )
				{
				(void) sprintf(err_buf,
						" Unrecognized field type ... %s\n", field_type);
				(void) error_report(err_buf);
				}
			}

		/* Set the field type for retrieving the features */
		(void) set_fld_descript(&descript, FpaF_FIELD_MACRO, fkind,
											FpaF_END_OF_LIST);

		/* Retrieve the set containing the features */
		switch (fkind)
			{

			/* Extract features from discrete type fields */
			case FpaC_DISCRETE:
				set = retrieve_areaset(&descript);
				if ( IsNull(set) || set->num <= 0 )
					{
					(void) sprintf(err_buf,
							" No areas for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					}
				break;

			/* Extract features from line type fields */
			case FpaC_LINE:
				set = retrieve_curveset(&descript);
				if ( IsNull(set) || set->num <= 0 )
					{
					(void) sprintf(err_buf,
							" No lines for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					}
				break;

			/* Extract features from scattered type fields */
			/*  ... or labels from other type fields       */
			case FpaC_SCATTERED:
				set = retrieve_spotset(&descript);
				if ( IsNull(set) || set->num <= 0 )
					{
					(void) sprintf(err_buf,
							" No spots for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					}
				break;

			/* Extract features from link chain type fields */
			case FpaC_LCHAIN:
				set = retrieve_lchainset(&descript);
				if ( IsNull(set) || set->num <= 0 )
					{
					(void) sprintf(err_buf,
							" No link chains for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					}
				break;

			/* Cannot extract features from other types of fields */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
			default:
				(void) sprintf(err_buf,
						" Cannot iterate with features from %s %s\n",
						element, level);
				(void) error_report(err_buf);
			}

		/* Rebuild the AREA set using subareas (if requested) */
		if ( same(set->type, "area") && same(area_type, AreaTypeSubareas) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Rebuilding set from subareas of %d area features\n",
						set->num);
				}
			
			/* Make a copy of each area or each subarea */
			copyset = create_set("area");
			for ( ii=0; ii<set->num; ii++ )
				{
				area = (AREA) set->list[ii];
				if ( IsNull(area) ) continue;

				if ( Verbose )
					{
					if ( area->numdiv == 0 )
						{
						(void) fprintf(stdout,
							"  Area feature: %d  with attributes ...\n", ii);
						(void) debug_attrib_list("     ", area->attrib);
						}
					else
						{
						(void) fprintf(stdout,
							"  Area feature: %d  (%d subareas) with attributes ...\n",
								ii, area->numdiv+1);
						for ( isub=0; isub<=area->numdiv; isub++ )
							{
							(void) debug_attrib_list("     ",
									area->subareas[isub]->attrib);
							if ( isub < area->numdiv )
								(void) fprintf(stdout, "      and ...\n");
							}
						}
					}

				/* Make a copy of area if no subareas */
				if ( area->numdiv == 0 )
					{
					carea = copy_area(area, FALSE);
					if ( NotNull(carea) )
						{
						(void) add_item_to_set(copyset, (ITEM) carea);
						}
					}

				/* Otherwise make a copy of each subarea */
				else
					{
					for ( isub=0; isub<=area->numdiv; isub++ )
						{
						carea = area_from_subarea(area->subareas[isub]);
						if ( NotNull(carea) )
							{
							(void) add_item_to_set(copyset, (ITEM) carea);
							}
						}
					}
				}

			/* Save the new set */
			(void) destroy_set(set);
			set = copyset;

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Rebuilt set with %d area features\n", set->num);
				}
			}

		/* Rebuild the AREA set using boundaries (if requested) */
		else if ( same(set->type, "area") && same(area_type, AreaTypeBoundary) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Rebuilding set from boundaries of %d area features\n",
						set->num);
				}
			
			/* Make a copy of each area using just the boundary and holes */
			copyset = create_set("area");
			for ( ii=0; ii<set->num; ii++ )
				{
				area = (AREA) set->list[ii];
				if ( IsNull(area) ) continue;

				if ( Verbose )
					{
					(void) fprintf(stdout,
						"  Area feature: %d  with attributes ...\n", ii);
					(void) debug_attrib_list("     ", area->attrib);
					}

				/* Create a copy of area using just the boundary and holes */
				carea = copy_area(area, FALSE);
				if ( NotNull(carea) )
					{
					empty_area(carea);
					define_area_boundary(carea, copy_line(area->bound->boundary));
					for ( ihole=0; ihole<area->bound->numhole; ihole++ )
						add_area_hole(carea, copy_line(area->bound->holes[ihole]));
					(void) add_item_to_set(copyset, (ITEM) carea);
					}
				}

			/* Save the new set */
			(void) destroy_set(set);
			set = copyset;

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Rebuilt set with %d area features\n", set->num);
				}
			}

		if ( Verbose )
			{
			if (same(set->type, "area"))
				(void) fprintf(stdout,
						" Checking %d area features for ... %s %s from %s at %s\n",
						set->num, element, level, CurSource, TVstamp);
			else if (same(set->type, "curve"))
				(void) fprintf(stdout,
						" Checking %d line features for ... %s %s from %s at %s\n",
						set->num, element, level, CurSource, TVstamp);
			else if (same(set->type, "spot"))
				(void) fprintf(stdout,
						" Checking %d scattered features for ... %s %s from %s at %s\n",
						set->num, element, level, CurSource, TVstamp);
			else if (same(set->type, "lchain"))
				(void) fprintf(stdout,
						" Checking %d link chain features for ... %s %s from %s at %s\n",
						set->num, element, level, CurSource, TVstamp);
			}

		/* Remove features that do not match the category attributes */
		for ( ii=set->num-1; ii>=0; ii-- )
			{

			/* Check for matching features from discrete type fields */
			if ( same(set->type, "area") )
				{
				area = (AREA) set->list[ii];
				if ( !match_category_attributes(area->attrib,
										cat_cascade, cat_attrib, num_catatt) )
					{
					(void) remove_item_from_set(set, (ITEM) area);
					}
				}

			/* Check for matching features from line type fields */
			else if ( same(set->type, "curve") )
				{
				curve = (CURVE) set->list[ii];
				if ( !match_category_attributes(curve->attrib,
										cat_cascade, cat_attrib, num_catatt) )
					{
					(void) remove_item_from_set(set, (ITEM) curve);
					}
				}

			/* Check for matching features from scattered type fields */
			else if ( same(set->type, "spot") )
				{
				spot = (SPOT) set->list[ii];
				if ( !match_category_attributes(spot->attrib,
										cat_cascade, cat_attrib, num_catatt) )
					{
					(void) remove_item_from_set(set, (ITEM) spot);
					}
				}

			/* Check for matching features from link chain type fields */
			else if ( same(set->type, "lchain") )
				{
				lchain = (LCHAIN) set->list[ii];
				if ( !match_category_attributes(lchain->attrib,
										cat_cascade, cat_attrib, num_catatt) )
					{
					(void) remove_item_from_set(set, (ITEM) lchain);
					}
				}
			}

		/* Error return if no features match the category attributes */
		if ( set->num <= 0 )
			{
			switch (fkind)
				{

				/* No matching discrete type features */
				case FpaC_DISCRETE:
					(void) sprintf(err_buf,
							" No matching areas for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					break;

				/* No matching line type features */
				case FpaC_LINE:
					(void) sprintf(err_buf,
							" No matching lines for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					break;

				/* No matching scattered type features */
				case FpaC_SCATTERED:
					(void) sprintf(err_buf,
							" No matching spots for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					break;

				/* No matching link chain type features */
				case FpaC_LCHAIN:
					(void) sprintf(err_buf,
							" No matching link chains for ... %s %s from %s at %s\n",
							element, level, CurSource, TVstamp);
					(void) error_report(err_buf);
					break;
				}
			}

		if ( Verbose )
			{
			if (same(set->type, "area"))
				{
				(void) fprintf(stdout,
						" Iteration for %d area features\n", set->num);
				for ( ii=0; ii<set->num; ii++ )
					{
					area = (AREA) set->list[ii];
					if ( IsNull(area) ) continue;
	
					(void) fprintf(stdout,
						"  Area feature: %d  with attributes ...\n", ii);
					(void) debug_attrib_list("     ", area->attrib);
					}
				}
			else if (same(set->type, "curve"))
				(void) fprintf(stdout,
						" Iteration for %d line features\n", set->num);
			else if (same(set->type, "spot"))
				(void) fprintf(stdout,
						" Iteration for %d scattered features\n", set->num);
			else if (same(set->type, "lchain"))
				(void) fprintf(stdout,
						" Iteration for %d link chain features\n", set->num);
			}

		/* Set the field parameters for this loop */
		Ploop->num_iterations = set->num;
		Ploop->fld_set        = set;
		Ploop->fld_attrib     = safe_strdup(attribute);
		Ploop->fld_vtime      = safe_strdup(TVstamp);
		}

	/* Error return if missing "element" or "level" */
	else if ( !blank(element) && blank(level) )
		{
		(void) sprintf(err_buf,
				"No \"level\" keyword with \"element = %s\"", element);
		(void) error_report(err_buf);
		}
	else if ( blank(element) && !blank(level) )
		{
		(void) sprintf(err_buf,
				"No \"element\" keyword with \"level = %s\"", level);
		(void) error_report(err_buf);
		}

	/* Error return for no "number_of_iterations" keyword */
	else if ( Ploop->num_iterations <= 0 )
		(void) error_report("No \"number_of_iterations\" keyword");

	/* Error return for no "group_name" keywords    */
	/*   when "number_of_iterations" keyword is set */
	else if ( Ploop->num_groups <= 0 )
		(void) error_report("No \"group_name\" keywords");

	/* Check groups for keyword/value pairs or values */
	for ( jj=0; jj<Ploop->num_groups; jj++ )
		{
		Pgroup = &Ploop->loop_groups[jj];

		/* Error return for no "keyword_name" keywords for a "group_name" */
		/*  ... or no default values (if keyword not required)            */
		if ( Pgroup->num_keys <= 0 && IsNull(Pgroup->defvals) )
			{
			(void) sprintf(err_buf,
				"No \"keyword_name\" keywords (or values) for group_name: %s",
				Pgroup->group_name);
			(void) error_report(err_buf);
			}

		/* Error return for no "value" keywords for a "keyword_name" */
		for ( kk=0; kk<Pgroup->num_keys; kk++ )
			{
			Pkeyval = &Pgroup->keyvals[kk];
			if ( Pkeyval->num_vals <= 0 )
				{
				(void) sprintf(err_buf,
					"No \"value\" keywords for keyword_name: %s  of group_name: %s",
					Pgroup->keywords[kk], Pgroup->group_name);
				(void) error_report(err_buf);
				}
			}
		}

	/* Now set the group parameters for the first iteration */
	for ( jj=0; jj<Ploop->num_groups; jj++ )
		{
		if ( !set_group_parameters(Ploop->loop_groups[jj],
				Ploop->current_iteration) )
			{
			(void) error_report("Error setting group parameters");
			}
		}

	/* Now set the field parameters for the first iteration */
	if ( !set_field_parameters(Ploop->fld_set, Ploop->fld_attrib,
			Ploop->fld_vtime, Ploop->current_iteration) )
		{
		(void) error_report("Error setting field parameters");
		}

	/* Free category attribute buffers */
	(void) free_category_attribs();

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		reset_loop

	(
	LOGICAL		*endofiterations,
	FILE		**fpdf,
	long int	*posn
	)

	{
	int				ii, jj, kk;
	LOOP_STRUCT		*Ploop   = NullPtr(LOOP_STRUCT *);
	LOOP_GROUP		*Pgroup  = NullPtr(LOOP_GROUP *);
	LOOP_KEYVALS	*Pkeyval = NullPtr(LOOP_KEYVALS *);

	/* Identify the currently active loop */
	Ploop = &Loops[NumLoops-1];

	/* Increment the loop iteration */
	Ploop->current_iteration++;

	/* End of iterations for the currently active loop */
	if ( Ploop->current_iteration >= Ploop->num_iterations )
		{

		/* Set return parameters */
		if ( NotNull(endofiterations) ) *endofiterations = TRUE;
		if ( NotNull(fpdf) )            *fpdf            = Ploop->file_pointer;
		if ( NotNull(posn) )            *posn            = 0;

		/* Free space used by loop groups */
		if ( Ploop->num_groups > 0 )
			{
			for ( jj=0; jj<Ploop->num_groups; jj++ )
				{
				Pgroup = &Ploop->loop_groups[jj];
				if ( NotNull(Pgroup->defvals) )
					{
					FREELIST(Pgroup->defvals->values, Pgroup->defvals->num_vals);
					FREEMEM(Pgroup->defvals);
					}
				for ( kk=0; kk<Pgroup->num_keys; kk++ )
					{
					Pkeyval = &Pgroup->keyvals[kk];
					FREELIST(Pkeyval->values, Pkeyval->num_vals);
					}
				FREEMEM(Pgroup->keyvals);
				FREELIST(Pgroup->keywords, Pgroup->num_keys);
				FREEMEM(Pgroup->group_name);
				}
			FREEMEM(Ploop->loop_groups);
			}

		/* Reset feature parameters */
		(void) strcpy(CurType,      FpaCblank);
		(void) strcpy(CurAttribute, FpaCblank);
		(void) strcpy(CurVtime,     FpaCblank);
		CurArea   = NullArea;
		CurCurve  = NullCurve;
		CurSpot   = NullSpot;
		CurLchain = NullLchain;

		/* Free space used by field features */
		(void) destroy_set(Ploop->fld_set);
		FREEMEM(Ploop->fld_attrib);
		FREEMEM(Ploop->fld_vtime);

		/* Decrement number of loops */
		NumLoops--;

		/* Return now if this was the last loop */
		if ( NumLoops <= 0 ) return TRUE;

		/* Identify the previous active loop */
		Ploop = &Loops[NumLoops-1];

		/* Reset group parameters for current iteration of previous active loop */
		for ( jj=0; jj<Ploop->num_groups; jj++ )
			{
			if ( !set_group_parameters(Ploop->loop_groups[jj],
					Ploop->current_iteration) )
				{
				(void) error_report("Error resetting group parameters");
				}
			}

		/* Reset field parameters for current iteration of previous active loop */
		if ( !set_field_parameters(Ploop->fld_set, Ploop->fld_attrib,
				Ploop->fld_vtime, Ploop->current_iteration) )
			{
			(void) error_report("Error resetting field parameters");
			}

		/* Return TRUE if all went well */
		return TRUE;
		}

	/* Set the group parameters for the next iteration */
	for ( jj=0; jj<Ploop->num_groups; jj++ )
		{
		if ( !set_group_parameters(Ploop->loop_groups[jj],
				Ploop->current_iteration) )
			{
			(void) error_report("Error setting group parameters");
			}
		}

	/* Set the field parameters for the next iteration */
	if ( !set_field_parameters(Ploop->fld_set, Ploop->fld_attrib,
			Ploop->fld_vtime, Ploop->current_iteration) )
		{
		(void) error_report("Error setting field parameters");
		}

	/* Set return parameters */
	if ( NotNull(endofiterations) ) *endofiterations = FALSE;
	if ( NotNull(fpdf) )            *fpdf            = Ploop->file_pointer;
	if ( NotNull(posn) )            *posn            = Ploop->file_position;

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		set_loop_location_lookup

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			lookupok;
	int				ii, nn, tdiff;
	int				byear, bjday, bhour, bminute;
	int				eyear, ejday, ehour, eminute;
	STRING			next, vtime;
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			stime[GPGTiny], etime[GPGTiny];
	int				num_times;
	STRING			*times;
	int				num_labs;
	STRING			*labs;
	float			xinterval;
	double			dinterval;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(location_look_up, FpaCblank);
	(void) strcpy(location_units,   FpaCmksUnits);
	(void) strcpy(stime,            FpaCblank);
	(void) strcpy(etime,            FpaCblank);
	num_times = 0;
	num_labs  = 0;
	times     = NullStringPtr;
	labs      = NullStringPtr;
	xinterval = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "location_interval") )
			{
			(void) sscanf(action, "%f", &xinterval);
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "times") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_times++;
				times = GETMEM(times, STRING, num_times);
				times[num_times-1] = safe_strdup(next);
				}
			}

		else if ( same(key, "start_time") )
			{
			(void) strcpy(stime, action);
			}

		else if ( same(key, "end_time") )
			{
			(void) strcpy(etime, action);
			}

		else if ( same(key, "labels") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_labs++;
				labs = GETMEM(labs, STRING, num_labs);
				labs[num_labs-1] = safe_strdup(next);
				}
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(location_look_up) )
		(void) error_report("No location look up name");

	/* Check for negative "location_interval" */
	if ( xinterval < 0.0 )
		{
		(void) sprintf(err_buf,
				"Location interval: %.3f  must be positive!", xinterval);
		(void) error_report(err_buf);
		}

	/* Check "location_units" keyword and convert "location_interval" to km */
	if ( IsNull(identify_unit(location_units)) )
		{
		(void) sprintf(err_buf, "Unknown location units: %s", location_units);
		(void) error_report(err_buf);
		}
	if ( !convert_value(location_units, (double) xinterval,
							LocationUnitsKm, &dinterval) )
		{
		(void) sprintf(err_buf,
				"Incorrect location units: %s  (must convert to: %s)",
				location_units, LocationUnitsKm);
		(void) error_report(err_buf);
		}
	xinterval = (float) dinterval;

	/* Check for consistent "times" (must be increasing) */
	if ( num_times > 0)
		{

		/* Set first time (without worrying about longitude) */
		vtime = interpret_timestring(times[0], T0stamp, 0.0);
		if ( blank(vtime) )
			{
			(void) sprintf(err_buf,
					"Incorrect times parameter ... %s", times[0]);
			(void) error_report(err_buf);
			}
		(void) parse_tstamp(vtime, &byear, &bjday, &bhour, &bminute,
											NullLogicalPtr, NullLogicalPtr);

		/* Check subsequent times for consistency */
		for ( nn=1; nn<num_times; nn++ )
			{

			/* Set next time (without worrying about longitude) */
			vtime = interpret_timestring(times[nn], T0stamp, 0.0);
			if ( blank(vtime) )
				{
				(void) sprintf(err_buf,
						"Incorrect times parameter ... %s", times[nn]);
				(void) error_report(err_buf);
				}
			(void) parse_tstamp(vtime, &eyear, &ejday, &ehour, &eminute,
											NullLogicalPtr, NullLogicalPtr);

			/* Determine time difference */
			tdiff = mdif(byear, bjday, bhour, bminute,
							eyear, ejday, ehour, eminute);

			/* Error if times are not increasing */
			if ( tdiff <= 0 )
				{
				(void) sprintf(err_buf,
						"Inconsistent times (not increasing) for lookup table ... %s",
						location_look_up);
				(void) error_report(err_buf);
				}

			/* Reset times */
			byear   = eyear;
			bjday   = ejday;
			bhour   = ehour;
			bminute = eminute;
			}
		}

	/* Check for consistent "start_time" and "end_time" (must be increasing) */
	if ( !blank(stime) && !blank(etime) )
		{

		/* Set start time (without worrying about longitude) */
		vtime = interpret_timestring(stime, T0stamp, 0.0);
		if ( blank(vtime) )
			{
			(void) sprintf(err_buf, "Incorrect start time ... %s", stime);
			(void) error_report(err_buf);
			}
		(void) parse_tstamp(vtime, &byear, &bjday, &bhour, &bminute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set end time (without worrying about longitude) */
		vtime = interpret_timestring(etime, T0stamp, 0.0);
		if ( blank(vtime) )
			{
			(void) sprintf(err_buf, "Incorrect end time ... %s", etime);
			(void) error_report(err_buf);
			}
		(void) parse_tstamp(vtime, &eyear, &ejday, &ehour, &eminute,
											NullLogicalPtr, NullLogicalPtr);

		/* Determine time difference between start and end times */
		tdiff = mdif(byear, bjday, bhour, bminute, eyear, ejday, ehour, eminute);

		/* Error if times are not increasing */
		if ( tdiff <= 0 )
			{
			(void) sprintf(err_buf,
					"Inconsistent start time ... %s  and end time ... %s  (not increasing)",
					stime, etime);
			(void) error_report(err_buf);
			}
		}

	/* Add (or re-define) an internal location look up table */
	lookupok = add_loop_location_lookup(location_look_up, stime, etime,
					times, num_times, labs, num_labs, xinterval);

	/* Free space used by times and labels */
	for ( nn=0; nn<num_times; nn++ )
		{
		FREEMEM(times[nn]);
		}
	FREEMEM(times);
	for ( nn=0; nn<num_labs; nn++ )
		{
		FREEMEM(labs[nn]);
		}
	FREEMEM(labs);

	/* Return TRUE if all went well */
	return lookupok;
	}

/***********************************************************************
*                                                                      *
*    a d d _ g r o u p                                                 *
*    s e t _ g r o u p _ p a r a m e t e r s                           *
*                                                                      *
***********************************************************************/

static	LOGICAL		add_group

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii, iname, jj;
	STRING		key, action;
	char		group_name[GPGShort];

	/* Look for keyword for name of group */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "group_name") )
			{
			iname = ii;
			(void) strcpy(group_name, action);
			break;
			}
		}
	if ( ii == num ) (void) error_report("No group_name keyword for group");

	/* See if this name already exists */
	for ( jj=0; jj<NumGroups; jj++ )
		{
		if ( same(Groups[jj].group_name, group_name) ) break;
		}

	/* Add another group */
	if ( jj == NumGroups )
		{

		/* Get space for another group */
		NumGroups++;
		Groups = GETMEM(Groups, KEY_GROUP, NumGroups);
		(void) strcpy(Groups[jj].group_name, group_name);
		Groups[jj].num    = 0;
		Groups[jj].maxnum = 0;
		Groups[jj].list   = NullPtr(ListDef *);

		/* Save all keywords except for the group_name keyword */
		for ( ii=0; ii<num; ii++ )
			{
			if ( ii == iname ) continue;
			Groups[jj].num++;
			if (Groups[jj].num > Groups[jj].maxnum)
				{
				Groups[jj].maxnum += 10;
				Groups[jj].list    = GETMEM(Groups[jj].list, ListDef,
												Groups[jj].maxnum);
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"Increasing number of keywords to: %d for group %s\n",
							Groups[jj].maxnum, Groups[jj].group_name);
					}
				}
			(void) strcpy(Groups[jj].list[Groups[jj].num-1], list[ii]);
			}
		}

	/* Replace keywords for this group */
	else
		{

		/* Reset the number of keywords to 0 */
		Groups[jj].num = 0;

		/* Save all keywords except for the group_name keyword */
		for ( ii=0; ii<num; ii++ )
			{
			if ( ii == iname ) continue;
			Groups[jj].num++;
			if (Groups[jj].num > Groups[jj].maxnum)
				{
				Groups[jj].maxnum += 10;
				Groups[jj].list    = GETMEM(Groups[jj].list, ListDef,
												Groups[jj].maxnum);
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"Increasing number of keywords to: %d for group %s\n",
							Groups[jj].maxnum, Groups[jj].group_name);
					}
				}
			(void) strcpy(Groups[jj].list[Groups[jj].num-1], list[ii]);
			}
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		set_group_parameters

	(
	LOOP_GROUP	pgroup,
	int			iteration
	)

	{
	int			jj, kk, ll;
	char		keylist[GPGMedium];

	/* Check the group list to see if this name already exists */
	for ( jj=0; jj<NumGroups; jj++ )
		{
		if ( same(Groups[jj].group_name, pgroup.group_name) ) break;
		}

	/* Add another group */
	if ( jj == NumGroups )
		{

		/* Get space for another group */
		NumGroups++;
		Groups = GETMEM(Groups, KEY_GROUP, NumGroups);
		(void) strcpy(Groups[jj].group_name, pgroup.group_name);
		Groups[jj].num    = 0;
		Groups[jj].maxnum = 0;
		Groups[jj].list   = NullPtr(ListDef *);
		}

	/* Reset parameters for this group */
	else
		{

		/* Reset the number of keywords to 0 */
		Groups[jj].num = 0;
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, "   Setting loop parameters for group ... %s\n",
				pgroup.group_name);
		(void) fprintf(stdout, "    Loop iteration ... %d\n", iteration);
		}

	/* Build a string containing the current value (without a keyword) */
	if ( NotNull(pgroup.defvals) )
		{

		/* Use the appropriate value for this iteration */
		ll = iteration % pgroup.defvals->num_vals;
		(void) strcpy(keylist, pgroup.defvals->values[ll]);

		/* Save the value entry */
		Groups[jj].num++;
		if (Groups[jj].num > Groups[jj].maxnum)
			{
			Groups[jj].maxnum += 10;
			Groups[jj].list    = GETMEM(Groups[jj].list, ListDef,
											Groups[jj].maxnum);
			if ( Verbose )
				{
				(void) fprintf(stdout,
						"Increasing number of values to: %d for group %s\n",
						Groups[jj].maxnum, Groups[jj].group_name);
				}
			}
		(void) strcpy(Groups[jj].list[Groups[jj].num-1], keylist);

		if ( Verbose )
			{
			(void) fprintf(stdout, "     Value string ... %s\n", keylist);
			}
		}

	/* Build a string containing each keyword and the current value */
	else
		{
		for ( kk=0; kk<pgroup.num_keys; kk++ )
			{

			/* Initialize the keyword list entry with the keyword name */
			(void) strcpy(keylist, pgroup.keywords[kk]);
			(void) strcat(keylist, " = ");

			/* Append the appropriate value for this iteration */
			ll = iteration % pgroup.keyvals[kk].num_vals;
			(void) strcat(keylist, pgroup.keyvals[kk].values[ll]);

			/* Save the keyword list entry */
			Groups[jj].num++;
			if (Groups[jj].num > Groups[jj].maxnum)
				{
				Groups[jj].maxnum += 10;
				Groups[jj].list    = GETMEM(Groups[jj].list, ListDef,
												Groups[jj].maxnum);
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"Increasing number of keywords to: %d for group %s\n",
							Groups[jj].maxnum, Groups[jj].group_name);
					}
				}
			(void) strcpy(Groups[jj].list[Groups[jj].num-1], keylist);
			if ( Verbose )
				{
				(void) fprintf(stdout, "     Keyword string ... %s\n", keylist);
				}
			}
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ f i e l d _ p a r a m e t e r s                           *
*                                                                      *
***********************************************************************/

static	LOGICAL		set_field_parameters

	(
	SET			fld_set,
	STRING		fld_attrib,
	STRING		fld_vtime,
	int			iteration
	)

	{
	STRING		value;
	ATTRIB_LIST	attribs;

	/* Initialize feature parameters */
	(void) strcpy(CurType,      FpaCblank);
	(void) strcpy(CurAttribute, FpaCblank);
	(void) strcpy(CurVtime,     FpaCblank);
	CurArea   = NullArea;
	CurCurve  = NullCurve;
	CurSpot   = NullSpot;
	CurLchain = NullLchain;

	/* Return if no field features */
	if ( IsNull(fld_set) )  return TRUE;

	/* Error if no set features or mismatched feature count */
	if ( fld_set->num < 0 )         return FALSE;
	if ( iteration < 0 )            return FALSE;
	if ( iteration >= fld_set->num) return FALSE;

	if ( Verbose )
		(void) fprintf(stdout, "   Loop iteration ... %d\n", iteration);

	/* Set the feature type and valid time for current iteration */
	(void) safe_strcpy(CurType,  fld_set->type);
	(void) safe_strcpy(CurVtime, fld_vtime);

	/* Set the AREA object for current iteration */
	if ( same(CurType, "area") )
		{
		CurArea   = (AREA) fld_set->list[iteration];
		attribs   = CurArea->attrib;

		if ( Verbose ) (void) fprintf(stdout, "    Set AREA parameters\n");
		}

	/* Set the CURVE object for current iteration */
	else if ( same(CurType, "curve") )
		{
		CurCurve  = (CURVE) fld_set->list[iteration];
		attribs   = CurCurve->attrib;

		if ( Verbose ) (void) fprintf(stdout, "    Set CURVE parameters\n");
		}

	/* Set the SPOT object for current iteration */
	else if ( same(CurType, "spot") )
		{
		CurSpot   = (SPOT) fld_set->list[iteration];
		attribs   = CurSpot->attrib;

		if ( Verbose ) (void) fprintf(stdout, "    Set SPOT parameters\n");
		}

	/* Set the LCHAIN object for current iteration */
	else if ( same(CurType, "lchain") )
		{
		CurLchain = (LCHAIN) fld_set->list[iteration];
		attribs   = CurLchain->attrib;

		if ( Verbose ) (void) fprintf(stdout, "    Set LCHAIN parameters\n");
		}

	/* Set the attribute value for the current iteration */
	if ( !blank(fld_attrib) )
		{
		value = CAL_get_attribute(attribs, fld_attrib);
		(void) safe_strcpy(CurAttribute, value);

		if ( Verbose )
			(void) fprintf(stdout, "    Set attribute \"%s\" from \"%s\"\n",
															value, fld_attrib);
		}

	/* Return TRUE if all went well */
	return TRUE;
	}