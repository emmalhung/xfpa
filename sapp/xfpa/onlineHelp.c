/*========================================================================*/
/*
*  File:    onlineHelp.c
*
*  Purpose: Provides the online html help functionality by calling and
*           controlling fpahelp as the help engine.
*
*     Note: The browser to use for online help is set through the use of
*           the *.onlineHelp.program resource.
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
/*========================================================================*/

#include <stdio.h>
#include "global.h"
#include "resourceDefines.h"
#include "help.h"

#define BUFLEN	300

static String module = "OnlineHelp";


void Help(XtPointer data)
{
	HelpCB(GW_mainWindow, data, NULL);
}


/*ARGSUSED*/
void HelpCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	char    href[BUFLEN+1];
	char    toc[BUFLEN+1];
	char    ndx[BUFLEN+1];
	FILE    *fp;
	String  buf;
	String  p;
	String  pgm;
	String  dpyname;
	String  ptr;
	String  fname;
	String  xref  = (String)client_data;

	if(blank(xref)) xref = "toc";

	/* This is to make sure the strncpy will always result in a terminating null */
	(void) memset(href, 0, BUFLEN+1);
	(void) memset(toc,  0, BUFLEN+1);
	(void) memset(ndx,  0, BUFLEN+1);

	/* Read the help cross reference file. This file must be named in CROSS_REF_FILE
	 * and be situated with the help information. The first entry on a line
	 * is the help key associated with the help buttons and defined in the
	 * file help.h and passed into this function and the second is the
	 * hyperlink reference into the help system.
	 */
	(void) strcpy(toc, "FPA_HelpTOC.html");
	(void) strcpy(ndx, "FPA_HelpIX.html");
	(void) strcpy(href, ndx);

	fname = get_file(HELP_SOURCE,CROSS_REF_FILE);
	if(NotNull(fname) && NotNull(fp = fopen(fname,"r")))
	{
		while((buf = ReadLine(fp)))
		{
			p = string_arg(buf);
			if(same_ic(p,xref))
			{
				ptr = string_arg(buf);
				if (!blank(ptr)) (void) strncpy(href, ptr, BUFLEN);
			}
			else if(same_ic(p,"toc"))
			{
				ptr = string_arg(buf);
				if (!blank(ptr)) (void) strncpy(toc, ptr, BUFLEN);
			}
			else if(same_ic(p,"index"))
			{
				ptr = string_arg(buf);
				if (!blank(ptr)) (void) strncpy(ndx, ptr, BUFLEN);
			}
		}
		(void) fclose(fp);
	}
	else
	{
		pr_error(module,"Unable to open help hyperlink cross reference file \"%s\".\n",
					CROSS_REF_FILE);
		return;
	}

	/* Find the browser to use */
	pgm = XuGetStringResource(RNonlineHelpProgram, "fpahelp");

	/* The fpahelp program has specific needs and uses the XuRunProgram facility. */
	if(same_ic(pgm,"fpahelp"))
	{
		static int fid = 0;

		(void) strcpy(href, get_path(HELP_SOURCE, (blank(href)) ? "index.html":href));

		if(!XuSendToProgram(fid, href))
		{
			int     ac = 0;
			char    mbuf[BUFLEN];
			String  args[12];

			/* If we get to here we need to launch the help program. The
			 * display we want it on should be found in the resource
			 */
			(void) snprintf(mbuf, BUFLEN, "*onlineHelp.%s", XuNdialogDisplay);
			if((dpyname = XuGetStringResource(mbuf,NULL)) && XuIsValidDisplayString(dpyname))
			{
				args[ac++] = "-display";
				args[ac++] = dpyname;
			}
			if (!blank(toc))
			{
				args[ac++] = "-toc";
				args[ac++] = toc;
			}
			if (!blank(ndx))
			{
				args[ac++] = "-index";
				args[ac++] = ndx;
			}
			args[ac++] = href;
			args[ac++] = NULL;
			
			fid = XuRunReceiveProgram(pgm, args);
		}
	}
	else
	{
		char   tab[10];
		char   dpy[BUFLEN];
		String dir;

		/* Get the complete file path */
		dir = get_directory(HELP_SOURCE);
		if(!dir)
		{
			pr_error(module,"Help directory \"%s\" not accessable.\n", dir);
			return;
		}

		/* Is a display location specified in the resources. */
		(void) snprintf(dpy, BUFLEN, "*%s.%s",RNonlineHelp,XuNdialogDisplay); 
		if((dpyname = XuGetStringResource(dpy,NULL)) && XuIsValidDisplayString(dpyname))
			(void) snprintf(dpy, BUFLEN, " -display %s", dpyname);
		else
			(void) strcpy(dpy,"");

		/* Add in the new tab request if it is in the resources. */
		if(XuGetBooleanResource(RNonlineHelpNewTab,False))
			(void) strcpy(tab, ",new-tab");
		else
			(void) strcpy(tab,"");

		/* Create the string to be processed by the system call. The mozilla series of browsers use the ping() operator
		 * to find out if a version of the program is running already. If so then send the href request to the existing
		 * program via the remote option. Otherwise start a new program.
		 */
		ptr = AllocPrint("r=`%s -remote 'ping()' 2>&1`\nif [ '$r' ]\nthen\n%s%s 'file:%s/%s' &\nelse\n%s -remote 'openFile(file:%s/%s%s)' &\nfi",
				pgm, pgm, dpy, dir, href, pgm, dir, href, tab);
		(void) system(ptr);
		FreeItem(ptr);
	}
}
