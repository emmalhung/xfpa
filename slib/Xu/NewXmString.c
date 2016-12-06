/*================================================================*/
/*
*   File: NewXmString
*
*  Purpose: 1. To convert values of type "String" to type "XmString"
*		       and allow multiple lines in the resultant XmString by
*              the use of a newline character "\n".
*           2. Allow the input String to contain directives to
*              provide multiple fonts in the resulting XmString.
*           3. To do conversions of special symbols from ansi to iso.
*              At the moment these conversions are:
*
*              - change the ansi character 263, displayed as a
*                degree symbol, to the iso character 260 which is
*                the iso degree symbol character.
*
*  Functions: XuNewXmString(string)
*			  XuNewXmStringFmt(format, string)
*             XuConvertAnsiToIsoChar (Boolean)
*
*  Directives: In order for the code to recognize that the string
*              contains font directives the fonts must be in the
*              format "@tag@" where tag is the rendition tag. If
*              the "@" char is required as part of the text it can
*              be escaped by preceeding it with a "\" (ie. "\@").
*              Return to the previous font with a @P@.
*
*              Examples:
*
*              @tag1@...@tag2@...@tag3@...@P@ the final @P@ will
*              return to the tag2 font.
*
*              @tag1@...@tag2@...@P@...@tag3@...@P@ both @P@ will
*              return the font to a tag1 type.
*
*  Usage: 
*
*  1. To use on any string just call the function.
*
*  2. Have render table resource or string contain the Tags for the
*     different fonts you want.
*
*  3. Use the tags in any resource of type XmString, start with "@"
*     and end with "@" to set the current Tag. "@@" returns to the
*     default render tag.
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
/*================================================================*/
#include <stdarg.h>
#include <string.h>
#include <Xm/Label.h>
#include "XuP.h"

XmString XuNewXmStringFmt(String _fmt,...)
{
	char mbuf[2048];

	va_list args;
	va_start(args, _fmt);
	(void) vsnprintf(mbuf, 2048, _fmt, args);
	va_end(args);
	return XuNewXmString(mbuf);
}


XmString XuNewXmString(String instring)
{
	int      len, i, j;
	char     sub, *substr, tagstr[256], nowtag[256], prvtag[256];
	XmString xm_str, xm_substr;
	Boolean	 in_tag;

	len     = (int) safe_strlen(instring);
	substr  = XTCALLOC(len+1, char);
	in_tag  = False;
	xm_str  = (XmString)NULL;

	(void) strcpy(nowtag, "");
	(void) strcpy(tagstr, "");
	(void) strcpy(prvtag, "");

	for(i = 0,j = 0; i < len; i++)
	{
		if(instring[i] == '\\' && instring[i+1] == '@')
		{
			substr[j++] = '@';
			i++;
		}
		else if(instring[i] != '@')
		{
			if(in_tag)
			{
				nowtag[j] = instring[i];
			}
			else
			{
				 substr[j] = instring[i];
			}
			j++;
		}
		else if(in_tag)
		{
			in_tag = False;
			nowtag[j] = '\0';
			if(j == 0)
			{
				(void) safe_strcpy(tagstr, XmSTRING_DEFAULT_CHARSET);
			}
			else if(strcasecmp(nowtag,"P") == 0)
			{
				(void) safe_strcpy(tagstr, prvtag);
			}
			else
			{
				(void) safe_strcpy(prvtag, tagstr);
				(void) safe_strcpy(tagstr, nowtag);
			}
			j = 0;
		}
		else
		{
			in_tag = True;
			if(j > 0)
			{
				substr[j] = '\0';
				xm_substr = XmStringGenerate(substr, NULL, XmCHARSET_TEXT, blank(tagstr)?NULL:tagstr);
				xm_str = (xm_str)? XmStringConcatAndFree(xm_str,xm_substr) : xm_substr;
				j = 0;
			}
		}
	}

	if(in_tag)
	{
		(void) fprintf(stderr,"XuNewXmString: Unclosed Rendition Tag: '@' missing > \"%s\"\n", instring);
	}
	else
	{
		substr[j] = '\0';
		xm_substr = XmStringGenerate(substr, NULL, XmCHARSET_TEXT, blank(tagstr)?NULL:tagstr);
		xm_str = (xm_str)? XmStringConcatAndFree(xm_str,xm_substr) : xm_substr;
	}
	XtFree(substr);

	return xm_str;
}


/*  Creates an XmString from a single byte character string with the rendition
 *  associated with the given tag. For complex fonts such as the XFT ones
 *  characters such as the degree sign require special processing in a multibyte
 *  font which explains the complexity. The function also validates the given
 *  rendition tag.
 */
XmString _xu_xmstring_create(Widget w, String str, String tag)
{
	int           n;
	XmRenderTable rendertable = NULL;
	XmString      xmstr = NULL;
	Boolean       multibyte = False;
	String        buf = NULL;
	String        tags[3] = {NULL,_MOTIF_DEFAULT_LOCALE,XmFONTLIST_DEFAULT_TAG};

	String degmb = "°";
	String crmb  = "©";

	if (!w) return NULL;
	if (!str) str = " ";

	XtVaGetValues(w, XmNrenderTable, &rendertable, NULL);

	if(!rendertable)
	{
		xmstr = XmStringGenerate((XtPointer) str, NULL, XmCHARSET_TEXT, NULL);
	}
	else
	{
		tags[0] = tag;
		for(n = blank(tag)?1:0; n < 3; n++)
		{
			XmRendition rendition = XmRenderTableGetRendition(rendertable, tags[n]);
			if(rendition)
			{
				Arg args[2];
				XmFontType type;
				tag = tags[n];
				XtSetArg(args[0], XmNfontType, &type);
				XmRenditionRetrieve(rendition, args, 1);
				multibyte = (type == XmFONT_IS_XFT);
				XmRenditionFree(rendition);
				break;
			}
		}
		if(!multibyte)
		{
			xmstr = XmStringGenerate((XtPointer) str, NULL, XmCHARSET_TEXT, tag);
		}
		else
		{
			String   p;
			Cardinal len = 1;
			Boolean  nosym = True;

			for(p = str; *p != '\0'; p++)
			{
				switch(*p)
				{
					case '\260':
						len += (Cardinal) strlen(degmb);
						nosym = False;
						break;
					case '\251':
						len += (Cardinal) strlen(crmb );
						nosym = False;
						break;
					default:
						len++;
				}
			}
			if(nosym)
			{
				xmstr = XmStringGenerate((XtPointer) str, NULL, XmCHARSET_TEXT, tag);
			}
			else
			{
				String symbol, buf, ptr;
				buf = XtCalloc(len, sizeof(char));
				for(p = ptr = str; *p != '\0'; p++)
				{
					switch(*p)
					{
						case '\260': symbol = degmb; break;
						case '\251': symbol = crmb;  break;
						default:     symbol = NULL;
					}
					if(symbol)
					{
						strncat(buf, ptr, p-ptr);
						strcat(buf, symbol);
						ptr = p+1;
					}
				}
				if(!blank(ptr)) strcat(buf, ptr);
				xmstr = XmStringGenerate((XtPointer) buf, NULL, XmMULTIBYTE_TEXT, tag);
				XtFree(buf);
			}
		}
	}
	return xmstr;
}
