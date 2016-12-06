/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2004 by the LessTif Developers.
 *
 *                        All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 *
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Converters.c,v 1.20 2005/04/03 16:41:40 tobiasoed Exp $
 */

/*
 * Convert a comma separated list of strings to a NULL terminated array
 * of substrings.
 * A comma (,) terminates a string
 * Backslash is an escape character allowing leading and terminating
 * white space to be protected.  A backslash-comma (\,) does not
 * terminate a string and is copied as a comma (,).
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #include <X11/Xos.h> */
#include <X11/StringDefs.h>

#include <Xm/XmP.h>
#include <Xm/AtomMgr.h>

#include <Xbae/MatrixP.h>
#include <Xbae/Converters.h>

#include "XbaeDebug.h"

char xbaeBadString = 0;
static Boolean XbaeStringsAreEqual(String, String, int);

/* ARGSUSED */
Boolean
XbaeCvtStringToStringArray(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						   XrmValuePtr to, XtPointer * data)
{
	static String *array;
	String start = from->addr;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeCvtStringToStringArray\n"));

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToStringArray",
						"wrongParameters", "XbaeMatrix",
						"String to StringArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(String *))
	{
		to->size = sizeof(String *);
		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;
	else
	{
		char *ch, *next, *a;
		int i, sub, len, count;

		/*
		 * Count the substrings
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
		{

			/*
			 * We hit a backslash
			 */
			if(*ch == '\\')
			{
				ch++;
			}
			/*
			 * We hit an unescaped comma
			 */
			else if(*ch == ',')
				count++;
		}

		/*
		 * Malloc the array, make it one bigger for a terminating with a pointer to xbaeBadString
		 */
		array = (String *) XtMalloc((count + 1) * sizeof(String));
		array[count] = &xbaeBadString;

		for(sub = 0; sub < count; sub++)
		{

			/*
			 * Skip leading white space
			 */
			while(isspace((unsigned char) *start))
				start++;

			/*
			 * Count the number of chars in this substring.
			 * backslash-comma counts as one and does not terminate.
			 * backslash-backslash-comma counts as two and does not terminate.
			 */
			for(ch = start, len = 0; *ch != '\0' && *ch != ','; ch++)
			{
				/*
				 * We hit a backslash
				 */
				if(*ch == '\\')
				{
					ch++;

					if(*ch == '\0')
					{
						break;
					}

				}

				len++;
			}

			/*
			 * Save the beginning of the next substring
			 */
			next = ch + 1;

			/*
			 * Back up over unprotected trailing white space if we moved at all
			 */
			if(ch != start)
				while(*(ch - 2) != '\\' && isspace((unsigned char) *(--ch)))
					len--;

			/*
			 * Malloc a String of the correct size
			 */
			array[sub] = (String) XtMalloc(len + 1);

			/*
			 * Copy the substring into our new string.
			 */
			for(i = 0, ch = start, a = array[sub]; i < len; i++, ch++)
			{

				/*
				 * We hit a backslash
				 */
				if(*ch == '\\')
				{
					ch++;
				}

				*(a++) = *ch;
			}
			*a = '\0';

			/*
			 * Point to the beginning of the next string.
			 */
			start = next;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(String **) to->addr = array;
	to->size = sizeof(String *);

	return True;
}

/*
 * Free the string array allocated by the String to StringArray converter
 */

/* ARGSUSED */
void
XbaeStringArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						  XrmValuePtr args, Cardinal * num_args)
{
	String *array = *(String **) to->addr;
	String *entry;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeStringArrayDestructor\n"));

	if(array == NULL)
		return;

	for(entry = array; *entry != &xbaeBadString; entry++)
		XtFree((char *) *entry);

	XtFree((char *) array);
}

/* ARGSUSED */
Boolean
XbaeCvtStringToCellTable(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						 XrmValuePtr to, XtPointer * data)
{
	static String **array;
	String start = from->addr;
	char *ch, c;
	int k, count;
	XrmValue lfrom, lto;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeCvtStringToCellTable\n"));

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToCellTable",
						"wrongParameters", "XbaeMatrix",
						"String to CellTable conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(String **))
	{
		to->size = sizeof(String *);
		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;
	else
	{
		/*
		 * Count the nl separated rows
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
		{
			if((*ch == '\\' && *(ch + 1) == 'n') || *ch == '\n')
				count++;

		}

		/*
		 * Malloc the array
		 */
		array = (String **) XtMalloc((count + 1) * sizeof(String *));
		array[count] = (String *) NULL;

		for(k = 0; k < count; k++)
		{
			for(ch = start; *ch != '\0'; ch++)
			{
				if((*ch == '\\' && *(ch + 1) == 'n') || *ch == '\n')
					break;
			}
			c = *ch;
			*ch = '\0';

			lfrom.addr = start;
			lfrom.size = strlen(start) + 1;
			lto.addr = (char *) (&array[k]);
			lto.size = sizeof(String *);

			if(!XbaeCvtStringToStringArray(dpy, args, num_args, &lfrom, &lto, data))
			{
				*ch = c;
				XtDisplayStringConversionWarning(dpy, from->addr, XmRCellTable);
				array[k] = (String *) 0;
				lto.addr = (char *) &array;
				XbaeStringCellDestructor(0, &lto, 0, 0, 0);
				return False;
			}
			*ch = c;
			if(c == '\\')
				start = ch + 2;
			else if(c == '\n')
				start = ch + 1;
			else
				start = ch;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(String ***) to->addr = array;
	to->size = sizeof(String **);

	return True;
}

/*
 * Free the string array allocated by the String to StringCellTable converter
 */

/* ARGSUSED */
void
XbaeStringCellDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						 XrmValuePtr args, Cardinal * num_args)
{
	String **array = *(String ***) to->addr;
	String **entry, *row;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeStringCellDestructor\n"));

	if(array == NULL)
		return;

	for(entry = array; *entry != NULL; entry++)
	{
		for(row = *entry; *row != &xbaeBadString; row++)
			XtFree((char *) *row);
		XtFree((char *) *entry);
	}

	XtFree((char *) array);
}

/*
 * Convert a comma separated list of short ints to array of widths.
 * The array is terminated with BAD_SIZE.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToWidthArray(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						  XrmValuePtr to, XtPointer * data)
{
	static short *array;
	String start = from->addr;
	char *ch;
	int i, count;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeCvtStringToWidthArray\n"));

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToWidthArray",
						"wrongParameters", "XbaeMatrix",
						"String to WidthArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(short *))
	{
		to->size = sizeof(short *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated shorts
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (short *) XtMalloc((count + 1) * sizeof(short));

		array[count] = BAD_SIZE;

		for(i = 0; i < count; i++)
		{

			array[i] = (short) atoi(start);

			/*
			 * Find the comma at the end of this short
			 */
			for(; *start != '\0' && *start != ','; start++)
				;
			start++;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(short **) to->addr = array;
	to->size = sizeof(short *);

	return True;
}

/*
 * Free the width array allocated by the String to WidthArray converter
 */

/* ARGSUSED */
void
XbaeWidthArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						 XrmValuePtr args, Cardinal * num_args)
{
	short *array = *(short **) to->addr;

	DEBUGOUT(_XbaeDebug0(__FILE__, NULL, "XbaeWidthArrayDestructor\n"));

	XtFree((char *) array);
}

/*
 * Convert a comma separated list of ints to array of max lengths.
 * The array is terminated with BAD_MAXLENGTH.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToMaxLengthArray(Display * dpy, XrmValuePtr args, Cardinal * num_args,
							  XrmValuePtr from, XrmValuePtr to, XtPointer * data)
{
	static int *array;
	String start = from->addr;
	char *ch;
	int i, count;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToMaxLengthArray",
						"wrongParameters", "XbaeMatrix",
						"String to MaxLengthArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(int *))
	{
		to->size = sizeof(int *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated ints
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (int *) XtMalloc((count + 1) * sizeof(int));

		array[count] = BAD_MAXLENGTH;

		for(i = 0; i < count; i++)
		{

			array[i] = (int) atoi(start);

			/*
			 * Find the comma at the end of this int
			 */
			for(; *start != '\0' && *start != ','; start++)
				;
			start++;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(int **) to->addr = array;
	to->size = sizeof(int *);

	return True;
}

/*
 * Free the max length array allocated by the String to
 * MaxLengthArray converter
 */

/* ARGSUSED */
void
XbaeMaxLengthArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
							 XrmValuePtr args, Cardinal * num_args)
{
	int *array = *(int **) to->addr;

	XtFree((char *) array);
}

/*
 * Compare two strings up to length chars, and return True if they are equal.
 * Handles Xm prefix too. The string test must be lower case.
 * Used by StringToAlignmentArray converter.
 */
static Boolean XbaeStringsAreEqual(String in, String test, int length)
{
	int i;

	if((in[0] == 'X' || in[0] == 'x') && (in[1] == 'M' || in[1] == 'm'))
		in += 2;

	for(i = 0; i < length; i++)
	{
		char c = *in;

		if(isupper(c))
			c = tolower(c);

		if(c != test[i])
			return False;

		in++;
	}

	/*
	 * String in may have trailing garbage, but as long as the first
	 * length chars matched, we return True
	 */
	return True;
}


/*
 * Convert a comma separated list of alignments to array of Booleans.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToBooleanArray(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
							XrmValuePtr to, XtPointer * data)
{
	static Boolean *array;
	String start = from->addr;
	char *ch;
	int i, count;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToBooleanArray",
						"wrongParameters", "XbaeMatrix",
						"String to BooleanArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(Boolean *))
	{
		to->size = sizeof(Boolean *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated alignments
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (Boolean *) XtMalloc((count + 1) * sizeof(Boolean));

		for(i = 0; i < count; i++)
		{
			/*
			 * Skip leading white space
			 */
			while(isspace(*start))
				start++;

			if(XbaeStringsAreEqual(start, "true", 4))
				array[i] = True;
			else if(XbaeStringsAreEqual(start, "1", 1))
				array[i] = True;
			else if(XbaeStringsAreEqual(start, "false", 5))
				array[i] = False;
			else if(XbaeStringsAreEqual(start, "0", 1))
				array[i] = False;
			else
			{
				XtDisplayStringConversionWarning(dpy, from->addr, XmRBooleanArray);
				XtFree((void *) array);
				return False;
			}

			/*
			 * Find the comma at the end of this short
			 */
			for(; *start != '\0' && *start != ','; start++)
				;
			start++;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(Boolean **) to->addr = array;
	to->size = sizeof(Boolean *);

	return True;
}


/*
 * Free the alignment array allocated by the String to BooleanArray converter
 */

/* ARGSUSED */
void
XbaeBooleanArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						   XrmValuePtr args, Cardinal * num_args)
{
	Boolean *array = *(Boolean **) to->addr;

	XtFree((char *) array);
}


/*
 * Convert a comma separated list of alignments to array of alignments
 * (unsigned chars).  The array is terminated by BAD_ALIGNMENT.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToAlignmentArray(Display * dpy, XrmValuePtr args, Cardinal * num_args,
							  XrmValuePtr from, XrmValuePtr to, XtPointer * data)
{
	static unsigned char *array;
	String start = from->addr;
	char *ch;
	int i, count;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToAlignmentArray",
						"wrongParameters", "XbaeMatrix",
						"String to AlignmentArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(unsigned char *))
	{
		to->size = sizeof(unsigned char *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated alignments
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (unsigned char *) XtMalloc((count + 1) * sizeof(unsigned char));

		array[count] = BAD_ALIGNMENT;

		/*
		 * Compare each substring to the alignment strings.
		 * If we find a bad one, display a warning and fail.
		 * We should be able to use XtCallConverter on _XmCvtStringToAlignment,
		 * but that function is static so we have to duplicate its
		 * functionality.
		 */
		for(i = 0; i < count; i++)
		{
			/*
			 * Skip leading white space
			 */
			while(isspace(*start))
				start++;

			if(XbaeStringsAreEqual(start, "alignment_beginning", 19))
				array[i] = XmALIGNMENT_BEGINNING;
			else if(XbaeStringsAreEqual(start, "alignment_center", 16))
				array[i] = XmALIGNMENT_CENTER;
			else if(XbaeStringsAreEqual(start, "alignment_end", 13))
				array[i] = XmALIGNMENT_END;
			else
			{
				XtDisplayStringConversionWarning(dpy, from->addr, XmRAlignmentArray);
				XtFree((void *) array);
				return False;
			}

			/*
			 * Find the comma at the end of this alignment
			 */
			/* EMPTY */
			for(; *start != '\0' && *start != ','; start++);
			start++;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(unsigned char **) to->addr = array;
	to->size = sizeof(unsigned char *);

	return True;
}


/*
 * Free the alignment array allocated by the String to AlignmentArray converter
 */

/* ARGSUSED */
void
XbaeAlignmentArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
							 XrmValuePtr args, Cardinal * num_args)
{
	unsigned char *array = *(unsigned char **) to->addr;

	XtFree((char *) array);
}

/*
 * Convert a comma separated list of shadow types to array of shadowTypes
 * (unsigned chars).  The array is terminated by BAD_SHADOW
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToShadowTypeArray(Display * dpy, XrmValuePtr args, Cardinal * num_args,
							   XrmValuePtr from, XrmValuePtr to, XtPointer * data)
{
	static unsigned char *array;
	String start = from->addr;
	char *ch;
	int i, count;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToShadowTypeArray",
						"wrongParameters", "XbaeMatrix",
						"String to ShadowTypeArray conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(unsigned char *))
	{
		to->size = sizeof(unsigned char *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated alignments
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (unsigned char *) XtMalloc((count + 1) * sizeof(unsigned char));

		array[count] = BAD_SHADOW;

		/*
		 * Compare each substring to the alignment strings.
		 * If we find a bad one, display a warning and fail.
		 * We should be able to use XtCallConverter on _XmCvtStringToAlignment,
		 * but that function is static so we have to duplicate its
		 * functionality.
		 */
		for(i = 0; i < count; i++)
		{
			/*
			 * Skip leading white space
			 */
			while(isspace(*start))
				start++;

			if(XbaeStringsAreEqual(start, "shadow_in", 9))
				array[i] = XmSHADOW_IN;
			else if(XbaeStringsAreEqual(start, "shadow_out", 10))
				array[i] = XmSHADOW_OUT;
			else
			{
				XtDisplayStringConversionWarning(dpy, from->addr, XmRShadowTypeArray);
				XtFree((void *) array);
				return False;
			}

			/*
			 * Find the comma at the end of this alignment
			 */
			/* EMPTY */
			for(; *start != '\0' && *start != ','; start++);
			start++;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(unsigned char **) to->addr = array;
	to->size = sizeof(unsigned char *);

	return True;
}


/*
 * Free the alignment array allocated by the String to ShadowTypeArray converter
 */

/* ARGSUSED */
void
XbaeShadowTypeArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
							  XrmValuePtr args, Cardinal * num_args)
{
	unsigned char *array = *(unsigned char **) to->addr;

	XtFree((char *) array);
}



/*
 * Convert a comma separated list of alignments to array of Booleans.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToPixelArray(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						  XrmValuePtr to, XtPointer * data)
{
	static Pixel *array, last_pixel = 0;
	String start = from->addr;
	char *ch;
	int i, count;
	XrmValue lfrom, lto;

	if(*num_args != 2)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToPixelArray",
						"wrongParameters", "XbaeMatrix",
						"String to PixelArray conversion needs screen and colormap arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(Pixel *))
	{
		to->size = sizeof(Pixel *);

		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma separated alignments
		 */
		for(ch = start, count = 1; *ch != '\0'; ch++)
			if(*ch == ',')
				count++;

		/*
		 * Malloc the array
		 */
		array = (Pixel *) XtMalloc((count + 1) * sizeof(Pixel));

		for(i = 0; i < count; i++)
		{
			/*
			 * Skip leading white space
			 */
			while(isspace(*start))
				start++;
			/*
			 * Find the comma at the end of this color
			 */
			for(ch = start; *ch != '\0' && *ch != ','; ch++)
			{
				if((*ch == '\\' && *(ch + 1) == 'n') || *ch == '\n')
					break;
			}
			*ch = '\0';

			lfrom.addr = start;
			lfrom.size = strlen(start) + 1;
			lto.addr = (char *) (&array[i]);
			lto.size = sizeof(Pixel);

			if(!XtCvtStringToPixel(dpy, args, num_args, &lfrom, &lto, data))
			{
				array[i] = last_pixel;
				XtDisplayStringConversionWarning(dpy, from->addr, XmRPixelArray);
			}
			last_pixel = array[i];
			start = ch + 1;
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(Pixel **) to->addr = array;
	to->size = sizeof(Pixel *);

	return True;
}


/*
 * Free the alignment array allocated by the String to BooleanArray converter
 */

/* ARGSUSED */
void
XbaePixelArrayDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						 XrmValuePtr args, Cardinal * num_args)
{
	Pixel *array = *(Pixel **) to->addr;
	XtFree((char *) array);
}


/*
 * Convert a comma separated list of pixels to array of pixels
 * .  The array is terminated by BAD_PIXEL.
 */

/* ARGSUSED */
Boolean
XbaeCvtStringToPixelTable(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						  XrmValuePtr to, XtPointer * data)
{
	static Pixel **array, *row;
	String start = from->addr;
	char *ch, c;
	int i, k, count_x, count_y;
	XrmValue lfrom, lto;
	Pixel last_pixel = 0;

	if(*num_args != 2)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToPixelTable",
						"wrongParameters", "XbaeMatrix",
						"String to PixelTable conversion needs screen and colormap arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(Pixel **))
	{
		to->size = sizeof(Pixel **);
		return False;
	}

	if(start == NULL || *start == '\0')
		array = NULL;

	else
	{

		/*
		 * Count the comma and nl separated alignments
		 */
		count_x = 1;
		for(ch = start, k = 1, count_y = 1; *ch != '\0'; ch++)
		{
			if(*ch == ',')
				k++;
			if((*ch == '\\' && *(ch + 1) == 'n') || *ch == '\n')
			{
				count_y++;
				if(k > count_x)
					count_x = k;
				k = 1;
			}
		}

		/*
		 * Malloc the arrays
		 */
		array = (Pixel **) XtMalloc((count_y + 1) * sizeof(Pixel *));
		array[count_y] = (Pixel *) 0;

		for(k = 0; k < count_y; k++)
		{
			row = (Pixel *) XtMalloc((count_x + 1) * sizeof(Pixel));
			row[count_x] = BAD_PIXEL;
			row[0] = last_pixel;
			array[k] = row;

			/*
			 * Convert each substring into Pixel
			 */
			for(i = 0; i < count_x; i++)
			{

				if(*start == '\0')
					break;
				/*
				 * Skip leading white space
				 */
				while(isspace(*start))
					start++;

				/*
				 * Find the comma at the end of this color
				 */
				/* EMPTY */
				for(ch = start; *ch != '\0' && *ch != ','; ch++)
				{
					if((*ch == '\\' && *(ch + 1) == 'n') || *ch == '\n')
						break;
				}
				c = *ch;
				*ch = '\0';

				lfrom.addr = start;
				lfrom.size = strlen(start) + 1;
				lto.addr = (char *) (&row[i]);
				lto.size = sizeof(Pixel);

				if(!XtCvtStringToPixel(dpy, args, num_args, &lfrom, &lto, data))
				{
					row[i] = last_pixel;
					XtDisplayStringConversionWarning(dpy, from->addr, XmRPixelTable);
				}
				last_pixel = row[i];
				*ch = c;
				if(c == '\0')
				{
					start = ch;
					break;
				}
				if(c == '\\')
				{
					ch++;
					start = ch + 1;
					break;
				}
				start = ch + 1;
			}
			for(; i < count_x - 1; i++)
			{
				/* fill rest of row with same value */
				row[i + 1] = row[i];
			}
		}
	}

	if(to->addr == NULL)
		to->addr = (XtPointer) & array;
	else
		*(Pixel ***) to->addr = array;
	to->size = sizeof(Pixel *);

	return True;
}

/*
 * Free the pixel array allocated by the String to PixelTable converter
 */

/* ARGSUSED */
void
XbaePixelTableDestructor(XtAppContext app, XrmValuePtr to, XtPointer converter_data,
						 XrmValuePtr args, Cardinal * num_args)
{
	Pixel **array = *(Pixel ***) to->addr;
	Pixel **col;

	if(array)
	{
		for(col = array; col; col++)
			XtFree((char *) *col);
		XtFree((char *) array);
	}
}

/* ARGSUSED */
Boolean
XbaeCvtStringToGridType(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						XrmValuePtr to, XtPointer * data)
{
	static unsigned char grid_type;
	String start = from->addr;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToGridType",
						"wrongParameters", "XbaeMatrix",
						"String to GridType conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(unsigned char))
	{
		to->size = sizeof(unsigned char);
		return False;
	}
	/*
	 * Skip leading white space
	 */
	while(isspace(*start))
		start++;

	if(XbaeStringsAreEqual(start, "grid_none", 9))
		grid_type = XmGRID_NONE;
	else if(XbaeStringsAreEqual(start, "grid_cell_line", 14))
		grid_type = XmGRID_CELL_LINE;
	else if(XbaeStringsAreEqual(start, "grid_cell_shadow", 16))
		grid_type = XmGRID_CELL_SHADOW;
	else if(XbaeStringsAreEqual(start, "grid_row_line", 13))
		grid_type = XmGRID_ROW_LINE;
	else if(XbaeStringsAreEqual(start, "grid_row_shadow", 15))
		grid_type = XmGRID_ROW_SHADOW;
	else if(XbaeStringsAreEqual(start, "grid_column_line", 16))
		grid_type = XmGRID_COLUMN_LINE;
	else if(XbaeStringsAreEqual(start, "grid_column_shadow", 15))
		grid_type = XmGRID_COLUMN_SHADOW;
	/* Deprecated types. To be removed in next version. */
	else if(XbaeStringsAreEqual(start, "grid_line", 9))
		grid_type = XmGRID_LINE;
	else if(XbaeStringsAreEqual(start, "grid_shadow_in", 14))
		grid_type = XmGRID_SHADOW_IN;
	else if(XbaeStringsAreEqual(start, "grid_shadow_out", 15))
		grid_type = XmGRID_SHADOW_OUT;
	else
	{
		XtDisplayStringConversionWarning(dpy, from->addr, XmRGridType);
		return False;
	}

	/* Deprecated types. To be removed in next version. */
	if(grid_type >= XmGRID_LINE)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToGridType",
						"deprecatedType", "XbaeMatrix",
						"Value for GridType is deprecated and will be removed in next release", NULL, NULL);

	/*
	 * Store our return value
	 */
	if(to->addr == NULL)
		to->addr = (XtPointer) & grid_type;
	else
		*(unsigned char *) to->addr = grid_type;
	to->size = sizeof(unsigned char);

	return True;
}

/* ARGSUSED */
Boolean
XbaeCvtStringToWrapType(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from,
						XrmValuePtr to, XtPointer * data)
{
	static unsigned char wrap_type;
	String start = from->addr;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "cvtStringToWrapType",
						"wrongParameters", "XbaeMatrix",
						"String to WrapType conversion needs no extra arguments", NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(unsigned char))
	{
		to->size = sizeof(unsigned char);
		return False;
	}
	/*
	 * Skip leading white space
	 */
	while(isspace(*start))
		start++;

	if(XbaeStringsAreEqual(start, "wrap_none", 9))
		wrap_type = XbaeWrapNone;
	else if(XbaeStringsAreEqual(start, "wrap_continuous", 15))
		wrap_type = XbaeWrapContinuous;
	else if(XbaeStringsAreEqual(start, "wrap_word", 9))
		wrap_type = XbaeWrapWord;
	else
	{
		XtDisplayStringConversionWarning(dpy, from->addr, XmRWrapType);
		return False;
	}

	/*
	 * Store our return value
	 */
	if(to->addr == NULL)
		to->addr = (XtPointer) & wrap_type;
	else
		*(unsigned char *) to->addr = wrap_type;
	to->size = sizeof(unsigned char);

	return True;
}

/* ARGSUSED */
Boolean
#ifdef __VMS
/* According to Barry Stone VMS only allows function names with a
   maximum length of 31 characters */
	XbaeCvtStringToMatrixScrollBarD
#else
	XbaeCvtStringToMatrixScrollBarDisplayPolicy
#endif
	(Display * dpy, XrmValuePtr args, Cardinal * num_args, XrmValuePtr from, XrmValuePtr to, XtPointer * data)
{
	static unsigned char display_policy;
	String start = from->addr;

	if(*num_args != 0)
		XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
						"cvtStringToMatrixScrollBarDisplayPolicy", "wrongParameters",
						"XbaeMatrix",
						"String to MatrixScrollBarDisplayPolicy conversion needs no extra arguments",
						NULL, NULL);

	/*
	 * User didn't provide enough space
	 */
	if(to->addr != NULL && to->size < sizeof(unsigned char))
	{
		to->size = sizeof(unsigned char);
		return False;
	}
	/*
	 * Skip leading white space
	 */
	while(isspace(*start))
		start++;

	if(XbaeStringsAreEqual(start, "display_none", 12))
		display_policy = XmDISPLAY_NONE;
	else if(XbaeStringsAreEqual(start, "display_as_needed", 17))
		display_policy = XmDISPLAY_AS_NEEDED;
	else if(XbaeStringsAreEqual(start, "display_static", 14))
		display_policy = XmDISPLAY_STATIC;
	else
	{
		XtDisplayStringConversionWarning(dpy, from->addr, XmRMatrixScrollBarDisplayPolicy);
		return False;
	}

	/*
	 * Store our return value
	 */
	if(to->addr == NULL)
		to->addr = (XtPointer) & display_policy;
	else
		*(unsigned char *) to->addr = display_policy;
	to->size = sizeof(unsigned char);

	return True;
}
