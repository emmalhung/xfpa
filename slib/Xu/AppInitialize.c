/**************************************************************************/
/**
 * \file AppInitialize.c
 *
 *	\brief Xu application initialization functions
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
 *
 */
/**************************************************************************/

#include <stdarg.h>
#include <X11/IntrinsicP.h>
#include <X11/Xproto.h>
#include "XuP.h"
#include <Xm/Protocols.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>


/*
 * Old error handler function pointer
 */
static int (*old_error_handler) (Display*,XErrorEvent*) = NULL;


/*
 * Resources for the library routines. This ties in with StandardDialog.c
 */
static String fallback_resources[] = {
	"*.action_area.XmPushButton.marginHeight: 4",
	NULL
};


/*
 * Add the given font path(s) to the X server font path. The path can consist
 * of multiple comma separated paths and can contain an environment reference
 * (ie. $MYFONTS/fonts). If the path passed in is an absolute path (/path) then
 * the path is added as given. If relative, then it is prepended with the home
 * directory Fxu.home_dir. The expanded path is saved in case there are any font
 * related error messages.
 */
static  String path = (String)0;

static void add_font_path( String inpath )
{
	int     i, n, ndirs;
	String  *dirs, p, buf;
	Boolean ok;
	int     npaths  = 0;
	String  *paths  = NULL;
	Boolean *insert = NULL;

	if(blank(inpath)) return;

	XtFree((void*)path);
	path = NULL;
	if(!(p = env_sub(inpath))) return;
	path = XtNewString(p);
	buf  = XtNewString(p);
	p    = strtok(buf, ",");

	while(p != NULL)
	{
		no_white(p);
		paths = XTREALLOC(paths,npaths+1,String);

		/* A font server has the form of tcp/<host>:7000
		*/
		if(strncasecmp(p,"tcp/",4) == 0)
		{
			paths[npaths] = XtNewString(p);
		}
		else
		{
			/* If not absolute path name prepend the home directory path */
			if(*p != '/' && safe_strlen(Fxu.home_dir) > 0)
			{
				paths[npaths] = XTCALLOC(safe_strlen(p)+safe_strlen(Fxu.home_dir)+3, char);
				(void) safe_strcpy(paths[npaths], Fxu.home_dir);
				if(Fxu.home_dir[safe_strlen(Fxu.home_dir)-1] != '/')
					(void) strcat(paths[npaths], "/");
				(void) safe_strcat(paths[npaths], p);
			}
			else
			{
				paths[npaths] = XTCALLOC((int) safe_strlen(p)+2, char);
				(void) safe_strcpy(paths[npaths], p);
			}

			/* there must be a '/' on the end */
			p = paths[npaths] + safe_strlen(paths[npaths]) - 1;
			if(*p != '/') (void) strcat(paths[npaths], "/");
		}
		npaths++;
		p = strtok(NULL,",");
	}
	XtFree(buf);

	/* Add path only if we do not have it already
	*/
	dirs   = XGetFontPath(DefaultAppDisplayInfo->display, &ndirs);
	insert = XTCALLOC(npaths,Boolean);

	for(ok = False, n = 0; n < npaths; n++)
	{
		insert[n] = True;
		for( i = 0; i < ndirs; i++ )
		{
			if(same(dirs[i],paths[n]))
			{
				insert[n] = False;
				break;
			}
		}
		if(insert[n]) ok = True;
	}

	/* Execute only if there is at least one new path to add.
	 */
	if(ok)
	{
		/* The font paths are searched in the order they are found. Thus
		 * we want our added path to be first.
		 */
		String *new = XTCALLOC(ndirs+npaths, String);
		for(n = 0, i = 0; i < npaths; i++)
		{
			if(insert[i]) new[n++] = paths[i];
		}
		for(i = 0; i < ndirs; i++, n++)
		{
			new[n] = dirs[i];
		}
		XSetFontPath(DefaultAppDisplayInfo->display, new, n);
		XtFree((void*)new);
	}

	XFreeFontPath(dirs);
	XTFREELIST(paths, npaths);
	XtFree((void*) insert);
	return;
}



/*
 * Handle X errors. Some errors we recognize and thus output our own error message.
 * The rest get passed on to the default handler.
 */
static int new_error_handler( Display *dpy, XErrorEvent *myerr )
{
	switch(myerr->request_code)
	{
		case X_SetFontPath:
			(void) fprintf(stderr,"X Error: Invalid font path \"%s\" for Display \"%s\"\n",
						path, XDisplayString(dpy));
			break;

		default:
			old_error_handler(dpy, myerr);
			break;
	}
	return (1);
}



/* ------------------- Profile Selection Dialog ---------------------------*/

/* The following two functions combine to form a specialized dialog for
 * selecting a profile to apply to an application.
 */
typedef struct { Widget dialog; String none; } TEMPLIST;

/*ARGSUSED*/
static void do_selection_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	TEMPLIST *tl = (TEMPLIST *)client_data;
	if (!tl) return;
	/* 
	 * It is important to NULL the dialog variable here as this is how the
	 * sync procedure below knows how to allow flow through. The delay
	 * is required to let the destroy complete so that the geometry of this
	 * dialog is saved in the root profile and not in the one that is about
	 * to be applied to the application.
	 */
	XuDestroyDialog(tl->dialog);
	tl->dialog = NullWidget;
	XuDelay(Fxu.top_level, 250);
	if(strcasecmp(XtName(w),tl->none))
		XuSetProfile(XtName(w));
}


/* 
 * This dialog that allows the user to select which profile to apply must appear
 * before any action is done on the main application. It is forced to appear on
 * the screen on which it is launched
 */
static void show_select_profile_dialog(String id)
{
	int           n, nlist;
	Cardinal      ac;
	String        *list;
	XmString      xms;
	XmRenderTable table;
	Arg           arg[7];
	Widget        sw, label, slist, btn;
	TEMPLIST      tl;

	nlist = XuGetProfiles(&list);

	if(nlist < 1) return;
	/*
	 * Try and find an entry for "none" in the resource file or database
	 */
	tl.none = XuGetStringResource(RES(id,"*.defaultProfile.labelString"), NULL);
	if (!tl.none) tl.none = XuGetStringResource("*.defaultProfile.labelString", NULL);
	if (!tl.none) tl.none = XuGetStringResource(RES(id,"*.default.labelString"), NULL);
	if (!tl.none) tl.none = XuGetStringResource(RES(id,".default.labelString"), NULL);
	if (!tl.none) tl.none = XuGetStringResource("*.default.labelString", NULL);
	if (!tl.none) tl.none = XuGetLabel("default");
	if (!tl.none) tl.none = "Default";

	tl.dialog = XuCreateToplevelFormDialog(Fxu.top_level, id,
		XuNretainGeometry, XuRETAIN_ALL,
		XuNdestroyCallback, do_selection_cb,
		XmNkeyboardFocusPolicy, XmPOINTER,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	label = XtVaCreateManagedWidget("label", xmLabelWidgetClass, tl.dialog,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	/*
	 * Ensure that the list is wide enough for the label
	 */
	XtVaGetValues(label, XmNlabelString, &xms, XmNrenderTable, &table, NULL);
	XtVaSetValues(XuGetShell(tl.dialog), XmNminWidth, XmStringWidth(table, xms)+18, NULL);

	ac = 0;
	XtSetArg(arg[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
	XtSetArg(arg[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(arg[ac], XmNtopWidget, label); ac++;
	XtSetArg(arg[ac], XmNtopOffset, 0); ac++;
	XtSetArg(arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	sw = XmCreateScrolledWindow(tl.dialog, "sw", arg, ac);

	ac = 0;
	XtSetArg(arg[ac], XmNmarginHeight, 5); ac++;
	XtSetArg(arg[ac], XmNmarginWidth, 5); ac++;
    slist = XmCreateRowColumn(sw, "list", arg, ac);

	ac = 0;
	XtSetArg(arg[ac], XmNhighlightOnEnter, True); ac++;
	XtSetArg(arg[ac], XmNhighlightThickness, 2); ac++;
	XtSetArg(arg[ac], XmNmarginWidth, 5); ac++;

	btn = XmCreatePushButton(slist, tl.none, arg, ac);
	XtAddCallback(btn, XmNactivateCallback, do_selection_cb, (XtPointer)&tl);
	XtManageChild(btn);

	for( n = 0; n < nlist; n++ )
	{
		btn = XmCreatePushButton(slist, list[n], arg, ac);
		XtAddCallback(btn, XmNactivateCallback, do_selection_cb, (XtPointer)&tl);
		XtManageChild(btn);
	}
	XtManageChild(slist);
	XtManageChild(sw);

	XuShowDialog(tl.dialog);
	/*
	 * Sync procedure to keep the control at this point until the
	 * profile selection has been done.
	 */
	while( tl.dialog != NULL || XtAppPending(Fxu.app_context))
	{
		XtAppProcessEvent(Fxu.app_context, XtIMAll);
	}
	XTFREELIST(list, nlist);
}

/* ----------------- End Profile Selection Dialog -------------------------*/


/* 
 * Callback to destroy the application. Note that only if the application uses a
 * XtDestroyWidget() on the application shell and does NOT do an exit() will this
 * function be called from the destroy callback. Normally it will be called from
 * the XuDestroyApplication() function.
 */
/*ARGSUSED*/
static void destroy_cb( Widget w, XtPointer client_data, XtPointer unused )
{
	int i;
	XuDSP d = (XuDSP) client_data;

	if (!d) return;
	if (!d->dialog) return;

	/* Save the position of all dialogs */
	XuSavePositionOfDialogs();

	/* Save the application location */
	if(d->pin != XuPIN_ALWAYS)
	{
		char      buf[100];
		Dimension w, h;

		XtVaGetValues(d->dialog, XmNwidth, &w, XmNheight, &h, NULL);
		if((abs(d->width-(int)w) > MIN_SIZE_CHANGE) || (abs(d->height-(int)h) > MIN_SIZE_CHANGE))
		{
			d->width  = (int)w;
			d->height = (int)h;
		}
		if(d->pin != XuPIN_INITIAL && d->pin != XuPIN_ALWAYS)
		{
			Position  x, y;
			XtTranslateCoords(d->dialog, 0, 0, &x, &y);
			d->x = (int)x;
			d->y = (int)y;
		}
		(void) snprintf(buf, 100, "%d %d %d %d %d %d", d->x, d->y, d->width, d->height, d->offset.x, d->offset.y);
		_xu_state_data_save(d->name, d->dd->id, NULL, buf);
	}

	/* Close any open dialogs */
	for(i = 0; i < Fxu.ndl; i++)
	{
		if(!Fxu.dl[i]->dialog) continue;
		if(Fxu.dl[i]->dialog == d->dialog) continue;
		XuDestroyDialog(Fxu.dl[i]->dialog);
	}

	_xu_free_colours(NULL);
	(void)XSetErrorHandler(NULL);
	if (Fxu.mdbfp) (void) fclose(Fxu.mdbfp);
	Fxu.mdbfp = (FILE *)0;

	XuFreePixmap(d->dialog, d->icon_pixmap);
	XuFreePixmap(d->dialog, d->icon_mask);

	XtDestroyWidget(d->dialog);
	d->dialog = NullWidget;
	exit(0);
}



/*========================= PUBLIC FUNCTIONS ==========================*/


/**
 *	\brief Replaces XtVaAppInitialize when using the Xu library.
 *
 *     - installs special colour and string handling converters (See XuNewXmString).
 *     - puts some pre-determined variables into the environment if
 *       they are not already defined.
 *     - finds the location and size of the mainline and restores it
 *       to these values. This is stored in the standard state store
 *       location.
 *     - ties in with the functions found in Display.c to allow dialogs
 *       to be placed on a different screen from the main program.
 *     - Puts up a profile selection dialog if there are profiles stored
 *       in the state store file and applies the profile to the
 *       application start.
 *
 *	\param[out] app_context       Application context returned.
 *  \param[in] app_class          Application class.
 *  \param[in] options            As per XtVaAppInitialize()
 *  \param[in] num_options        Number of above options
 *  \param[in,out] argc               argc from main 
 *  \param[in,out] argv               argv from main
 *  \param[in] specification_list as per XtVaAppInitialize()
 *  \param[in] ...                resource pairs terminated by a NULL
 *
 *   \return The application shell widget
 *
 *	<b>New Resources:</b>
 *	<TABLE border="1">
 *	<TR><TH>Resource<TH>Description<TH>Default
 *	<TR><TD>XuNactionAreaButtonMarginHeight<TD>The margin height of the buttons in the
 *	                                           action area of dialogs.<TD>3
 *	<TR><TD>XuNactonAreaButtonTightness<TD>The closeness of the buttons in the action area
 *	                                       of dialogs.<TD>20
 *	<TR><TD>XuNactionAreaMarginHeight<TD>The margin between the buttons and the sides of the
 *	                                     action area of dialogs.<TD>5
 *	<TR><TD>XuNallowProfileSelection<TD>If true then a dialog asking the user to select
 *                                   a profile will apear before program realizaton.
 *                                   <TD>False
 *	<TR><TD>XuNdefaultActionItemId<TD>A list of comma separated dialog action area button id's that
 *	                                  is to be the default set of buttons whose XmNshowAsDefault
 *	                                  resource will be set in the dialog action area if one is not
 *	                                  explicitly set in the dialog resources. See XuCreateDialog
 *	                                  for more details on the action area.
 *	<TR><TD>XuNmwmDeleteOverride<TD>The function to call when the application shell delete button
 *	                                (normally the x at the top of the application) is pushed.
 *	<TR><TD>XuNmwmDeleteData<TD>The data to pass to the function assigned through XuNmwmDeleteOverride
 *	                          resource setting above.
 *	<TR><TD>XuNfontPath<TD>A list of comma separated directories where fonts for the
 *		                 application will be found.<TD>None
 *	<TR><TD>XuNiconPixmapFile<TD>A file containing a pixmap in XPM format that is to be used
 *	                     for the application icon.<TD>None
 *	<TR><TD>XuNiconMaskFile<TD>A file containing a bi-level pixmap in XPM format that is to be
 *	                     used as a mask for the icon pixmap of a non-rectangular icon is required.<TD>None
 *	<TR><TD>XuNselectProfileDialogName<TD>The name of the profile request dialog as it will be
 *		                             referenced in the resource database.<TD>"selectProfile"
 *	<TR><TD>XuNstateFile<TD>The file to use as the base file for the dialog information
 *		                  and profile system. All profiles are saved in the same
 *		                  directory as this file.<TD>"~/.xurescache"
 *	<TR><TD>XuNstateFileEditable<TD>Can the state file be edited (modified)?<TD>True
 *	</TABLE>
 *
 *	\note
 *	Unlike widgets, values specified in the resource database
 *	will override any values hard coded into the application.
 *
 *	\par
 *	If XuNallowProfileSelection is true, and if there are existing profiles found listed
 *  in the state file as set by XuNstateFile, then a dialog will be displayed asking the
 *  user to select the profile to apply to the application. This will happen before the
 *  application itself starts.
 *
 *	\attention This function looks for a home environment variable. It is
 *          expected that the name will be the application name, the name
 *          appended with HOME or _HOME, or if the name starts with an 'X'
 *          the name without the 'X' and with or without the appended bit
 *          as given above. Upper case is assumed.
 *
 *	\attention This function also looks for a command line argument of
 *			"-profile <profile name>" where profile name is the name of the
 *			profile as entered through the profile dialog. If found the profile is
 *			selected and the dialog is not popped up. If the name contains spaces
 *			then the name must be enclosed in quotes.
 */

Widget XuVaAppInitialize(	XtAppContext *app_context, String app_class,
							XrmOptionDescRec *options, Cardinal num_options,
							int *argc, String *argv,
							String *specification_list, ...)
{
	int           i, j, len, n, ac;
	String        cmd, *list, s, p;
	struct stat   sb;
	char          buf[300];
	ArgList       args, al;
	ResInfoStruct r;
	XuDSP         d;
	Boolean       profile_select   = False;
	String        select_dialog_id = "startupSelectProfile";
	String        font_path_list   = NULL;
	String        state_file       = NULL;
	Boolean       state_file_edit  = True;
	va_list       va_args;

	static String suffix[] = { "HOME","_HOME","" };

	/*
	 * Determine the application name.
	 */
	s = strrchr(argv[0], '/');
	if (s) s++;
	else   s = argv[0];
	Fxu.app_name  = XtNewString(s);
	Fxu.app_class = XtNewString(app_class);
	/* 
	 * Get the home directory as defined in the environment. The home
	 * environment key is often different from the program application
	 * name. We look for <app_name>HOME, <app_name>_HOME and <app_name>
	 * and if the name starts with an 'X' try it without that.
	 */
	(void) safe_strcpy(buf, Fxu.app_name);
	len = (int) safe_strlen(buf);
	for(i = 0; i < len; i++)
	{
		buf[i] = (char) toupper((int)Fxu.app_name[i]);
	}

	j = 0;
	do
	{
		for( i = 0; i < (int) XtNumber(suffix); i++ )
		{
			(void) safe_strcpy(buf+len, suffix[i]);
			if((p = getenv(buf+j)))
			{
				Fxu.home_dir = XtNewString(env_sub(p));
				break;
			}
		}
		j++;
	} while(IsNull(Fxu.home_dir) && *buf == 'X' && j < 2 );

	if(!Fxu.home_dir)
	{
		(void) fprintf(stderr, "%s: No home directory found in the environment.\n", Fxu.app_name);
	}
	else if(stat(Fxu.home_dir, &sb) != 0 || !S_ISDIR(sb.st_mode))
	{
		(void) fprintf(stderr, "%s: Invalid environment home directory \"%s=%s\".\n",
				Fxu.app_name, buf, Fxu.home_dir);
	}
	/*
	 * If not defined in the environment we set some pre-assumed environment
	 * variables. It is assumed that there will be a file in the users home
	 * directory called .<app_name> that contains resource overrides. The
	 * first letter is must be upper case unless the name starts with an x
	 * where the first two letters are must  be upper case. The program xfpa
	 * would assume .XFpa and the program aurora would assume .Aurora.
	 */
	if( IsNull(getenv(XAPPLRESDIR)) )
	{
		String d = buf+(int) safe_strlen(XAPPLRESDIR)+1;
		(void) snprintf(buf, sizeof(buf), "%s=%s/app-defaults", XAPPLRESDIR, Fxu.home_dir);
		if(stat(d, &sb) == 0 && S_ISDIR(sb.st_mode)) (void) putenv(XtNewString(buf));
	}
	if( IsNull(getenv(XENVIRONMENT)) && NotNull(p = getenv("HOME")))
	{
		String s = XtNewString(Fxu.app_name);
		s[0] = (char)toupper((int)s[0]);
		if(s[0] == 'X') s[1] = (char)toupper((int)s[1]);
		(void) snprintf(buf, sizeof(buf), "%s/.%s", p, s);
		if(access(buf,R_OK) == 0)
		{
			(void) snprintf(buf, sizeof(buf), "%s=%s/.%s", XENVIRONMENT, p, s);
			(void) putenv(XtNewString(buf));
		}
		XtFree(s);
	}
	/*
	 * Merge our local fallback resources with those passed in.
	 */
	list = fallback_resources;
	if(specification_list)
	{
		n = 0;
		while(specification_list[n]) n++;
		list = XTCALLOC(n+XtNumber(fallback_resources)+1, String);
		for(i = 0; i < n; i++)
		{
			list[i] = specification_list[i];
		}
		for(i = 0; i < (int) XtNumber(fallback_resources); i++)
		{
			list[n] = fallback_resources[i];
			n++;
		}
		list[n] = NULL;
	}
	/*
	 * Now begin our initialization procedure.
	 */
	XtToolkitInitialize();
	*app_context = Fxu.app_context = XtCreateApplicationContext();
	XtAppSetFallbackResources(Fxu.app_context, list);
	(void) _xu_open_display(NULL, options, num_options, argv, argc);
	/*
	 * Replace the default error handler with our own. This allows
	 * us to print out messages specific to our situation and thus
	 * make the errors more meaningful.
	 */
	old_error_handler = XSetErrorHandler(new_error_handler);
	/*
	 * Resources specific to XuVaAppInitialize and/or library wide resources.
	 * Note that these are done differently from the normal for widget resources.
	 * Any values found in the resource database will take precedence over any hard
	 * coded ones. This allows for the default values to be coded in the application
	 * and then overides put in the resource files.
	 */
	va_start(va_args, specification_list); 
	while((cmd = va_arg(va_args, String)))
	{
		if(same(cmd, XuNallowProfileSelection))
			profile_select = (Boolean) va_arg(va_args, int);
		else if(same(cmd, XuNselectProfileDialogName))
			select_dialog_id = va_arg(va_args, String);
		else if(same(cmd, XuNfontPath))
			font_path_list = va_arg(va_args, String);
		else if(same(cmd, XuNstateFile))
			state_file = va_arg(va_args, String);
		else if(same(cmd, XuNstateFileEditable))
			state_file_edit = (Boolean) va_arg(va_args, int);
		else if(same(cmd, XuNdefaultActionItemId))
			Fxu.default_action_id = va_arg(va_args, String);
		else if(same(cmd, XuNactionAreaButtonMarginHeight))
			Fxu.button_margins = va_arg(va_args, int);
		else if(same(cmd, XuNactionAreaButtonTightness))
			Fxu.tightness = va_arg(va_args, int);
		else if(same(cmd, XuNactionAreaMarginHeight))
			Fxu.margins = va_arg(va_args, int);
		else if(same(cmd, XuNiconPixmapFile) && (s = va_arg(va_args,String)))
			Fxu.top_icon_file = XtNewString(s);
		else if(same(cmd, XuNiconMaskFile) && (s = va_arg(va_args,String)))
			Fxu.top_mask_file = XtNewString(s);
	}
	va_end(va_args);
	/*
	 * Global library settings
	 */
	profile_select   = XuGetBooleanResource(XuNallowProfileSelection, profile_select);
	select_dialog_id = XuGetStringResource(XuNselectProfileDialogName, select_dialog_id);
	font_path_list   = XuGetStringResource(XuNfontPath, font_path_list);
	state_file       = XuGetStringResource(XuNstateFile, state_file);
	state_file_edit  = XuGetBooleanResource(XuNstateFileEditable, state_file_edit);
	/*
	 * Global variable settings 
	 */
	Fxu.default_action_id = XuGetStringResource(XuNdefaultActionItemId, Fxu.default_action_id);
	Fxu.button_margins    = XuGetIntResource(XuNactionAreaButtonMarginHeight, Fxu.button_margins);
	Fxu.tightness         = XuGetIntResource(XuNactionAreaButtonTightness, Fxu.tightness);
	Fxu.margins           = XuGetIntResource(XuNactionAreaMarginHeight, Fxu.margins);
	/*
	 * If there is an icon specified in the resource file the mask must also be specified
	 * in the resource file. Any mask from the argument list must not be used.
	 */
	if((s = XuGetStringResource(XuNiconPixmapFile, NULL)))
	{
		Fxu.top_icon_file = s;
		Fxu.top_mask_file = XuGetStringResource(XuNiconMaskFile, NULL);
	}
	/*
	 * Set the default state file
	 */
	Fxu.andx = Fxu.gndx = Fxu.bndx = XuSetStateFile(state_file, state_file_edit);
	/*
	 * and font path
	 */
	add_font_path(font_path_list);
	/*
	 * Scan our argument list for the general resouces.
	 */
	va_start(va_args, specification_list); 
	_xu_scan_resource_strings(Fxu.app_name, NULL, True, &r, &al, &ac, va_args);
	va_end(va_args);
	/*
	 * These two might be picked up from the argument list and they are processed
	 * separately in the resource code above.
	 */
	XtFree(r.icon_file);
	XtFree(r.mask_file);
	/* 
	 * Install ourselves as one of the dialogs. This way we can make use of the
	 * standard functions for handling resizing and location.
	 */
	r.retain  = XuRETAIN_ALL;
	r.dpyinfo = DefaultAppDisplayInfo;

	d = _xu_create_data_structure(TOP_LEVEL_ID, NULL, &r);

	d->type         = TOPLEVEL;
	d->resize       = True;
	d->default_pos  = False;
	d->relative_pos = False;
	/*
	 * If no size specification found look for a geometry specification
	 * in the resource file.
	 */
	if((d->width < 1 || d->height < 1) && ((p = XuGetStringResource(RES(NULL, XmNgeometry), NULL)) != NULL))
	{
		_xu_parse_geometry_string(p, &d->x, &d->y, &d->width, &d->height);
		_xu_set_geometry(d, d->x, d->y);
	}
	/*
	 * Find the minimum dialog size constraints and then tack the 
	 * added resources on to the end of the input list.
	 */
	_xu_get_minimum_size(Fxu.app_name, d, &r);
	args = XTCALLOC(ac+12, Arg);
	(void) memcpy((void*)args, (void*)al, ac * sizeof(Arg));


	XtSetArg(args[ac], XmNvisual,   DefaultAppDisplayInfo->visual); ac++;
	XtSetArg(args[ac], XmNdepth,    DefaultAppDisplayInfo->depth ); ac++;
	XtSetArg(args[ac], XmNcolormap, DefaultAppDisplayInfo->cmap  ); ac++;

	XtSetArg(args[ac], XmNminWidth,   r.min_width  ); ac++;
	XtSetArg(args[ac], XmNminHeight,  r.min_height ); ac++;
	XtSetArg(args[ac], XmNwidthInc,   r.width_inc  ); ac++;
	XtSetArg(args[ac], XmNheightInc,  r.height_inc ); ac++;
	XtSetArg(args[ac], XmNbaseWidth,  r.base_width ); ac++;
	XtSetArg(args[ac], XmNbaseHeight, r.base_height); ac++;
	XtSetArg(args[ac], XmNgeometry,   d->geometry  ); ac++;
	/*
	 * If a delete function is provided then remove the automatic destroy
	 * when the delete window protocol is invoked.
	 */
	if (r.delete_fcn)
	{
		XtSetArg(args[ac], XmNdeleteResponse, XmDO_NOTHING  ); ac++;
	}
	/*
	 * Check for an application parameter specification for geometry.
	 * If there is one we do not want to use the stored geometry.
	 */
	for( n = 0; n < *argc; n++ )
	{
		if(strcmp(argv[n], "-geometry")) continue;
		ac--;
		break;
	}
	/* 
	 * Finally create the top level application shell
	 */
	Fxu.top_level = XtAppCreateShell(Fxu.app_name, Fxu.app_class, sessionShellWidgetClass,
						DefaultAppDisplayInfo->display, args, (Cardinal) ac);

	XtAddCallback(Fxu.top_level, XmNdestroyCallback, destroy_cb, (XtPointer) d);
	/*
	 * If there is an icon pixmap file specified it must be processed now after the
	 * widget has been created as XuCreatePixmap needs a widget reference. The mask
	 * file is only processed if there is a valid pixmap.
	 */
	if (Fxu.top_icon_file)
	{
		Pixmap pix;
		Arg arg[2];

		ac = 0;
		pix = XuGetPixmap(Fxu.top_level, Fxu.top_icon_file);
		if(pix != XmUNSPECIFIED_PIXMAP)
		{
			d->icon_pixmap = pix;
			XtSetArg(arg[ac], XmNiconPixmap, pix); ac++;

			if(Fxu.top_mask_file)
			{
				pix = XuGetPixmap(Fxu.top_level, Fxu.top_mask_file);
				if(pix != XmUNSPECIFIED_PIXMAP)
				{
					d->icon_mask = pix;
					XtSetArg(arg[ac], XmNiconMask, pix); ac++;
				}
				else
				{
					Fxu.top_mask_file = NULL;
				}
			}
		}
		else
		{
			Fxu.top_icon_file = NULL;
			Fxu.top_mask_file = NULL;
		}
		XtSetValues(Fxu.top_level, arg, ac);
	}
	/*
	 * If there is a delete callback defined then override the delete window protocol
	 * with a call to this function.
	 */
	if (r.delete_fcn)
	{
		Atom delete_atom = XInternAtom(XtDisplay(Fxu.top_level), WM_DELETE_WINDOW, True);
		XmAddWMProtocolCallback( Fxu.top_level, delete_atom, r.delete_fcn, r.delete_data );
	}
	/*
	 * Set the dialog structure elements to our shell
	 */
	d->dialog = Fxu.top_level;
	d->stopw  = Fxu.top_level;

	XtFree((void*)al);
	XtFree((void*)args);
	/*
	 * Select and apply the profile setting to the top level shell. Look for a profile
	 * in the command line (-profile <key>) "none" or "default" is a valid entry,
	 */
	if (profile_select)
	{
		p = NULL;
		for( i = 0; i < *argc-1; i++ )
		{
			if(!same(argv[i],"-profile")) continue;
			p = argv[i+1];
			if(!same(p,"none") && !same(p,"default"))
			{
				n = XuGetProfiles(&list);
				for( j = 0; j < n; j++ )
				{
					if(!same(p, list[j])) continue;
					XuSetProfile(p);
					break;
				}
				if( j >= n ) p = NULL;
			}
			break;
		}
		if (!p) show_select_profile_dialog(select_dialog_id);
		if(XuIsProfiledDialog(d->name))
		{
			if(XuVaStateDataGet(d->name, d->dd->id, NULL, "%d %d %d %d %d %d",
					&d->x, &d->y, &d->width, &d->height, &d->offset.x, &d->offset.y) == 6)
			{
				_xu_set_geometry(d, d->x, d->y);
				XtVaSetValues(d->dialog, XmNgeometry, d->geometry, NULL);
			}
			if(XuVaStateDataGet(PROFILED_DIALOGS, PINSTATE, NULL, "%d", &n) == 1)
				d->pin = (XuPIN_STYLE) n;
		}
	}
	/*
	 * For the toplevel we want a long delay time before checking the frame size
	 * as it was found from experience that the program needed time to settle 
	 * down before the information was stable. This might no longer apply as it
	 * was quite a while ago that this was tested but it does no harm.
	 */
	_xu_check_frame_size(d, 10000);

	return d->dialog;
}


/**
 * \brief Realize an application.
 *
 * \param[in] wid The widget as returned by XuVaAppInitialize
 *
 * \note
 * This function exists so that the application realization can be done differently
 *  in future without requiring any modification of application code.
 *
 */
void XuRealizeApplication( Widget wid )
{
	XtRealizeWidget(wid);
}


/**
 * \brief Called to terminate an application using the Xu library.
 *
 * \param[in] wid The widget as returned by XuVaAppInitialize
 *
 * \note
 * This function ensures that the position and size of the application
 * main and of all open dialogs are saved to the state file.
 */
void XuDestroyApplication( Widget wid )
{
	XuDSP d = _xu_dialog_data(wid);
	destroy_cb(wid, (XtPointer) d, NULL);
}
