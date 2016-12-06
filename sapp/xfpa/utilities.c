/****************************************************************************
*
*  File:     utilities.c
*
*  Purpose:  Contains general purpose utility functions that don't fall
*            naturally under any other file.
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
****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include "global.h"
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Column.h>
#include <Xbae/Matrix.h>
#include <ingred.h>
#include "resourceDefines.h"
#include "fcstText.h"
#include "fpapm.h"

#define UINT unsigned int

/* Font information arrays. These are the fonts used by Ingred to
 * put information on the map area.
 */
static int    nfont_list        = 0;
static String *font_list        = NULL;
static String *font_fd          = NULL;
static int    nfont_sizes       = 0;
static String *font_sizes       = NULL;
static String *font_size_labels = NULL;




/*========================= MESSAGE HANDLING FUNCTIONS ====================*/

/*=========================================================================*/
/*
*	LogMsg() - Prints a status message to the log file. All messages are
*	automatically terminated by an " at <date-time-stamp>".
*/
/*=========================================================================*/
void LogMsg(String key )
{
	pr_status("LogMsg","%s at %s\n",
		XuFindKeyLine(key, "status", "??"),
		sysClockFmt());
}

/*=========================================================================*/
/*
*	Warning() - Prints a warning message to the log file.
*/
/*=========================================================================*/
void Warning(String module, String key, ...)
{
	char mbuf[128], *ptr, nbuf[1024];
	va_list	args;

	(void)strcpy(mbuf,GV_app_name);
	(void)strcat(mbuf, module);
	ptr = XuFindKeyLine(key, "warn", "??");

	va_start(args, key);
	(void) vsnprintf(nbuf, sizeof(nbuf), ptr, args);
	va_end(args);

	(void)strcat(nbuf, "\n");
	pr_warning(mbuf, nbuf, NULL);
}


/*======================== SETUP FILE INFO FUNCTION =======================*/


static int    nsetup       = 0;
static SETUP  *setup       = (SETUP*)NULL;
static SETUP  *empty_setup = (SETUP*)NULL;
static String setup_file   = (String)NULL;


/*=========================================================================*/
/*
*	GetSetupFile() - Return the path name of the setup file and read the
*	                 interface part into memory.
*/
/*=========================================================================*/
String GetSetupFile(int argc, String *argv)
{
    int     i, numsetup;
	String  sfile, *setuplist;
	String  ptr, line, item;
	SETUP   *set;
	PARM    *entry;

	if (setup_file) return setup_file;

	sfile = NULL;

	/* First look for the command line override
	*/
	for( i = 1; i < argc; i++ )
	{
		if(!same_ic(argv[i],"-setup") && !same_ic(argv[i],"-s")) continue;
		if(i+1 >= argc) break;;
		sfile = argv[i+1];
		break;
	}

	/* Now find the setup file in one of the standard places
	*/
	numsetup = setup_files(sfile, &setuplist);
	if (numsetup <= 0)
	{
		pr_error(GV_app_name,"Setup file \"%s\" not found!\n", sfile);
		return NULL;
	}

	/* Now invoke the setup file
	*/
	if(!define_setup(numsetup,setuplist))
	{
		pr_error(GV_app_name,"Setup file \"%s\" could not be read!\n", sfile);
		return NULL;
	}

	setup_file = XtNewString(setuplist[0]);

	/* Create a structure to return if there is no information
	*  found corresponding to the given key.
	*/
	empty_setup = OneMem(SETUP);

	/* read the setup file and store the contents in memory
	 */
	if(!find_setup_block(INTERFACE,True))
	{
		pr_error(GV_app_name,"No \"interface\" block in the setup file.\n");
		exit(1);
	}
	while((line = setup_block_line())) 
	{
		/* Does this line contain a key? */
		if( line[0] == '[' &&  (ptr = strchr(line,']')) != NULL )
		{
			setup = MoreMem(setup, SETUP, nsetup+1);
			set = setup + nsetup;
			nsetup++;
			*ptr = '\0';
			ptr = string_arg(line+1);
			set->key_id = (ptr)? XtNewString(ptr):NULL;
			set->nentry = 0;
			set->entry = NULL;
		}
		else
		{
			set->entry = MoreMem(set->entry, PARM, set->nentry+1);
			entry = set->entry + set->nentry;
			set->nentry++;
			entry->nparms = 0;
			entry->parm = NULL;
			while((item = string_arg(line)))
			{
				entry->parm = MoreStringArray(entry->parm, entry->nparms+1);
				entry->parm[entry->nparms] = XtNewString(item);
				entry->nparms++;
			}
		}
	}

	return setup_file;
}


/*=========================================================================*/
/*
*	GetSetup() - Returns a pointer to a structure containing information on
*                the given key from the interface setup block in the setup
*   file. Each key is defined as being on a separate line and encased in
*	square brackets.
*/
/*=========================================================================*/
SETUP *GetSetup(String key )
{
	int i;

	for( i = 0; i < nsetup; i++ )
	{
		if(same(key,setup[i].key_id)) return (setup+i);
	}
	return empty_setup;
}


/*=========================================================================*/
/*
*	SetupParm() - Gets a parameter from a setup array given the row and
*	column numbers.  If either the row or column value is out of range
*	NULL is returned.
*/
/*=========================================================================*/
String SetupParm(SETUP *set , int nrow , int ncol )
{
	if (!set) return NULL;

	if (nrow < 0 || nrow >= set->nentry             ) return NULL;
	if (ncol < 0 || ncol >= set->entry[nrow].nparms ) return NULL;

	return set->entry[nrow].parm[ncol];
}


/*=========================================================================*/
/*
*	GetSetupParms() - For those keys in the setup file which have only one
*	                  parameter line this function returns the number of
*	                  parameters of the line.
*/
/*=========================================================================*/
PARM *GetSetupParms(String key )
{
	SETUP *set = GetSetup(key);
	if (!set) return NULL;
	if (set->nentry <= 0 ) return NULL;
	if (set->entry[0].nparms <= 0 ) return NULL;
	return set->entry;
}


/*=========================================================================*/
/*
*	GetSetupKeyParms() - For Setup File blocks which contain lines where
*                        the first entry on one of the lines is a line key
*   as given by parm_key.  The line parameters are returned with parms[0]
*   containing the parm_key.
*/
/*=========================================================================*/
PARM *GetSetupKeyParms(String block_key , String parm_key )
{
	int i;
	SETUP *set;

	set = GetSetup(block_key);
	if (!set) return NULL;
	for(i = 0; i < set->nentry; i++)
	{
		if(same_ic(SetupParm(set, i, 0), parm_key)) return (set->entry + i);
	}
	return NULL;
}


/* Returns true if there is at least one entry in the setup file for
 * the given key.
 */
Boolean HaveSetupEntry(String key)
{
	SETUP *set = GetSetup(key);
	return ( NotNull(set) && set->nentry > 0 );

}


/*================ FIELD STRUCTURE RELATED FUNCTIONS ======================*/



/*  Return a string containing the character specific to a given field type.
 *  This is useful in the state store functions for things that are field
 *  type specific.
 */
String FieldTypeID(FIELD_INFO *field)
{
	if (!field) field = GV_active_field;

	switch(field->info->element->fld_type)
	{
		case FpaC_CONTINUOUS: return ContinuousFieldTypeID;
		case FpaC_VECTOR:     return VectorFieldTypeID;
		case FpaC_DISCRETE:   return DiscreteFieldTypeID;
		case FpaC_WIND:       return WindFieldTypeID;
		case FpaC_LINE:       return LineFieldTypeID;
		case FpaC_SCATTERED:  return ScatteredFieldTypeID;
		case FpaC_LCHAIN:     return LinkChainFieldTypeID;
	}
	return " ";
}


/*=========================================================================*/
/*
*	InFieldList() - Returns true if the element and level of the given 
*	field is found in the parallel string lists of element and level.
*/
/*=========================================================================*/
Boolean InFieldList(FIELD_INFO *field , int  nlist , String *elem , String *level , int *pos )
{
	int i;

	if(IsNull(field)) return False;

	for( i = 0; i < nlist; i++ )
	{
		if(!same(field->info->element->name, elem[i]) ) continue;
		if(!same(field->info->level->name,   level[i])) continue;
		if (pos) *pos = i;
		return True;
	}
	if(pos) *pos = 0;
	return False;
}


/*=========================================================================*/
/*
*	HaveField() - The same as InFieldList() but using config field structs
*                 as the input list.
*/
/*=========================================================================*/
Boolean HaveField(FIELD_INFO *field, int  nlist, FpaConfigFieldStruct **flist, int *pos)
{
	int i;

	if(NotNull(field))
	{
		for( i = 0; i < nlist; i++)
		{
			if(!same(field->info->element->name, flist[i]->element->name)) continue;
			if(!same(field->info->level->name,   flist[i]->level->name  )) continue;
			if (pos) *pos = i;
			return True;
		}
	}
	if (pos) *pos = 0;
	return False;
}


/*=========================================================================*/
/*
*	FieldInDepictSequence() - Is the given field found in the depiction
*                             sequence.
*/
/*=========================================================================*/
Boolean FieldInDepictSequence(FpaConfigFieldStruct *field )
{
	int i;

	for( i = 0; i < GV_nfield; i++)
	{
		if(!same(field->element->name, GV_field[i]->info->element->name)) continue;
		if(!same(field->level->name,   GV_field[i]->info->level->name  )) continue;
		return True;
	}
	return False;
}


/*=========================================================================*/
/*
*	FindField() - Returns a pointer to the field defined by the given
*                 element and level.  If not found NULL is returned.
*/
/*=========================================================================*/
FIELD_INFO *FindField(String elem , String level )
{
	int i;

	for( i = 0; i < GV_nfield; i++ )
	{
		if(!same_ic(GV_field[i]->info->element->name, elem) ) continue;
		if(!same_ic(GV_field[i]->info->level->name,   level)) continue;
		return GV_field[i];
	}
	return (FIELD_INFO *)NULL;
}


/*=========================================================================*/
/*
*	FindFieldGroup() - Returns a pointer to the field group defined by the
*                      given group name.  If not found NULL is returned.
*/
/*=========================================================================*/
GROUP *FindFieldGroup(String name )
{
	int i;

	for( i = 0; i < GV_ngroups; i++ )
	{
		if(same_ic(GV_groups[i]->name, name)) return GV_groups[i];
	}
	return (GROUP *)NULL;
}


/*=========================================================================*/
/*
*	SetFieldExistance() - Sets the existance flag of the field structure.
*/
/*=========================================================================*/
void SetFieldExistance(FIELD_INFO *fptr )
{
	int nlist;
	char mbuf[300];

	(void)snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s",
		fptr->info->element->name, fptr->info->level->name);

	fptr->exists = False;
	if(GEStatus(mbuf, &nlist, NULL, NULL, NULL) == GE_VALID)
		fptr->exists = (nlist > 0);
}


/*==========================================================================*/
/*
*	InterpFieldsAvailable() - Given a list of fields, this will	determine
*	                          if all of the fields are interpolated and
*                             popup a dialog if they are not.
*/
/*==========================================================================*/
Boolean InterpFieldsAvailable(
    String origin,  /* where the call comes from "Text" or "Model"               */
    String id,      /* identifier to be inserted into the text                   */
    Widget w,       /* widget popup dialog is to be positioned relative to       */
    int  nlist,     /* number of items in the field list                         */
    String *list    /* items in pairs ie list[0] = element list[1] = height etc. */
)
{
	int     i, j, len, nfields, total_chars;
	char    nbuf[25];
	String  mbuf, na, *elem, *level;
	Boolean warn, found, return_value;
	FpaConfigFieldStruct *fs;

	total_chars = 250;
	mbuf = NewMem(char, total_chars);
	warn = False;

	(void)GEStatus("FIELDS INTERPOLATED", &nfields, &elem, &level, NULL);
	(void)snprintf(mbuf, 250, "%s:\n\n", id);
	for( i = 0; i < nlist; i+=2 )
	{
		/* Check for an odd number of items in the list.  It must be even
		*  with element and level pairs.
		*/
		if( i+2 > nlist ) break;
		found = False;
		for( j = 0; j < nfields; j++)
		{
			found = same(list[i],elem[j]) && same(list[i+1],level[j]);
			if(found) break;
		}
		if (found) continue;
		(void)strcat(mbuf,"    ");
		warn = True;
		fs = identify_field(list[i], list[i+1]);
		if(fs->label)
		{
			len = safe_strlen(mbuf) + safe_strlen(fs->label) + 2;
			if( len >= total_chars )
			{
				total_chars += MAX(250, len);
				mbuf = MoreMem(mbuf, char, total_chars);
			}
			(void)strcat(mbuf, fs->label);
		}
		else
		{
			na = XuGetLabel("unknownElem");
			len = safe_strlen(mbuf) + safe_strlen(na) + safe_strlen(list[i]) + safe_strlen(list[i+1]) + 5;
			if( len >= total_chars )
			{
				total_chars += MAX(250, len);
				mbuf = MoreMem(mbuf, char, total_chars);
			}
			(void)strcat(mbuf, na);
			(void)strcat(mbuf, ": ");
			(void)strcat(mbuf, list[i]);
			(void)strcat(mbuf, " ");
			(void)strcat(mbuf, list[i+1]);
		}
		(void)strcat(mbuf,"\n");
	}
	return_value = True;
	if(warn)
	{
		(void)strcpy(nbuf,"FieldMissing_");
		(void)strcat(nbuf,origin);
		return_value = (XuAskUser(w, nbuf, mbuf, NULL) == XuYES);
	}
	FreeItem(mbuf);
	return return_value;
}


/*============================================================================*/
/*
*	SetFieldBackground() - Set the background value of the given field. This
*                          function call consists of a pair of functions.
*   SetFieldBackground puts do_set_field_bkgnd into a work proc and waits. This
*   ensures that all current processing is completed before the background is
*   set and avoids a potential fatal embrace problem when Ingred asks for a
*   value and it is set by an immediate callback to Ingred.
*/
/*============================================================================*/
/*ARGSUSED*/
static Boolean do_set_field_bkgnd(XtPointer client_data)
{
	char mbuf[300];
	FIELD_INFO *field = (FIELD_INFO *)client_data;

	(void)snprintf(mbuf, sizeof(mbuf), "BACKGROUND %s %s",
		field->info->element->name, field->info->level->name);
	(void) IngredEditCommand(mbuf, field->bkgnd_cal, NullCal);
	return True;
}

void SetFieldBackground(FIELD_INFO *field )
{
	if(IsNull(field) || IsNull(field->bkgnd_cal)) return;
	(void)XtAppAddWorkProc(GV_app_context, (XtWorkProc)do_set_field_bkgnd, (XtPointer)field);
	XuDelay(GW_mainWindow, 100);
}



/*============================================================================*/
/*
 *   DisplayAttributes() - Display the attributes of areas or lines as a popup.
 */
/*============================================================================*/
void DisplayAttributesPopup(Widget panel, Boolean show, CAL cal)
{
	int              i, n, nids, height;
	char             mbuf[256];
	String           value, *ids;
	Widget           disp;
	XtWidgetGeometry size;
	FpaConfigElementAttribStruct *ap;

	/* The shell position is static so that we can popup the shell fairly close to
	*  its final position so as to minimize the movement flashing.
	*/
	static Widget   dpy = NullWidget;
	static Position x   = 0;
	static Position y   = 0;

	if( x == 0 & y == 0 )
	{
		XtTranslateCoords( panel, 0, 0, &x,  &y  );
		x -= 400;
	}

	if(show)
	{
		if(IsNull(cal)) return;

		dpy = XuVaCreatePopupShell("attribDisplayPopup",
			overrideShellWidgetClass, GW_mainManager,
			XmNx, x, XmNy, y,
			NULL);

		disp = XmVaCreateColumn(dpy, "attribDisplay",
			XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,	
			XmNlabelSpacing, 9,
			XmNmarginWidth,  9,
			XmNmarginHeight, 9,
			XmNborderWidth,  2,
			NULL);

		ap = GV_active_field->info->element->elem_detail->attributes;

		CAL_get_attribute_names(cal, &ids, &nids);
		for(i = 0; i < nids; i++)
		{
			value = CAL_get_attribute(cal, ids[i]);
			if(CAL_is_value(value))
			{
				for(n = 0; n < ap->nattribs; n++)
				{
					XmString xmlabel;
					if(!same(ids[i], ap->attrib_names[n])) continue;
					strcpy(mbuf, ap->attrib_labels[n]);
					strcat(mbuf, ":");
					xmlabel = XmStringCreateLocalized(mbuf);
					(void)XmVaCreateManagedLabel(disp, DateString(value,DEFAULT_FORMAT),
						XmNentryLabelString, xmlabel,
						NULL);
					break;
				}
			}
		}
		XtManageChild(disp);
		FreeItem(ids);

		/* Determine display position. The actual positioning must be done after
		*  the shell is popped as we can not determine the size before that. We do
		*  not want to go off the bottom of the screen, thus the DisplayHeight call.
		*/
		XtTranslateCoords( panel, 0, 0, &x,  &y  );
		height = DisplayHeight(XtDisplay(dpy), XScreenNumberOfScreen(XtScreen(dpy)));
		size.request_mode = CWWidth | CWHeight;

		XtPopup(dpy, XtGrabNone);
		(void) XtQueryGeometry(dpy, NULL, &size);
		x -= size.width + 10,
		y = MIN(y, height - size.height);
		XtVaSetValues(dpy, XmNx, x, XmNy, y, NULL);
	}
	else if(NotNull(dpy))
	{
		XtPopdown(dpy);
		XtDestroyWidget(dpy);
		dpy = NullWidget;
	}
}


/*========================= PIXMAP FUNCTIONS ===============================*/


/* Get the pixmaps required for the three states of a toggle button:  label, selected
*  and insensitive. The file name must be of the form key.id.<type> where type is one
*  of lab, sel or ins corresponding to the three pixmap types above.
*/
void ToggleButtonPixmaps(Widget w, String key, String id, Pixmap *label, Pixmap *sel, Pixmap *ins)
{
	char   mbuf[256];
	Pixmap pix;

	if (label)
	{
		(void)snprintf(mbuf, sizeof(mbuf), "%s.%s.lab", key, id);
		lower_case(mbuf);
		*label = XuGetPixmap(w, mbuf);
	}
	if (sel)
	{
		(void)snprintf(mbuf, sizeof(mbuf), "%s.%s.sel", key, id);
		lower_case(mbuf);
		pix = XuGetPixmap(w, mbuf);
		*sel = (pix == XmUNSPECIFIED_PIXMAP)? *label : pix;
	}
	if (ins)
	{
		(void)snprintf(mbuf, sizeof(mbuf), "%s.%s.ins", key, id);
		lower_case(mbuf);
		pix = XuGetPixmap(w, mbuf);
		*ins = (pix == XmUNSPECIFIED_PIXMAP)? *label : pix;
	}
}


/* Get the pixmaps required for the two states of a push button.
*/
void PushButtonPixmaps(Widget w, String key , String id , Pixmap *label , Pixmap *insensitive )
{
	ToggleButtonPixmaps(w, key, id, label, NULL, insensitive);
}



/* Pixmaps to indicate the taper of the "stomp" tool used by continuous and vector
*  fields.
*/
void MakeTaperPixmaps(Widget w, int height, int width, String value, Pixmap *l, Pixmap *s, Pixmap *i)
{
	int       k, n, depth, ww, hh, ws, hs;
	float     taper;
	Display   *dpy;
	Pixmap    px[3];
	GC        gc;
	XGCValues values;
	XPoint    *xp;
	Pixel     fg[3], bg[3];

	/* Load up the pixmap colours.*/
	fg[0] = XuLoadColor(w, XtNforeground);
	fg[1] = XuLoadColor(w, "black");
	fg[2] = XuLoadColor(w, "Gray78");
	bg[0] = XuLoadColor(w, XtNbackground);
	bg[1] = XuLoadColor(w, "green");
	bg[2] = bg[0];

	(void) sscanf(value, "%d", &n);
	taper = (float)n/100;

	XtVaGetValues(w, XmNdepth, &depth, NULL);
	dpy = XtDisplay(w);

	/* The area into which we want to draw is smaller than the pixmap
	*  size as requested. We use 1/6 of the distance as the border.
	*/
	ws = width/6;
	ww = width - 2*ws;
	hs = height - height/6;
	hh = height - height/3;

	/* Put the sequence of points into a point structure array since it
	*  is the same for all three pixmaps.
	*/
	xp = NewMem(XPoint, ww+2);
	xp[0].x = (short) ws;
	xp[0].y = (short) hs;
	xp[ww+1].x = (short) (width - ws - 1);
	xp[ww+1].y = (short) hs;

	for(n = 0; n < ww; n++)
	{
		float x, y;
		x = (float)n/(float)(ww-1);
		y = 1 - 4*(1-taper)*(x-0.5)*(x-0.5);
		xp[n+1].x = (short) (ws + n);
		xp[n+1].y = (short) (hs - hh*y);
	}

	values.line_width = 2;

	/* Now create the three pixmaps for the label, selected state and the
	*  insensitive state.
	*/
	for(k = 0; k < 3; k++)
	{
		px[k] = XCreatePixmap(dpy, XtWindowOfObject(w), (UINT) width, (UINT) height, (UINT) depth);
		values.foreground = bg[k];
		gc = XCreateGC(dpy, px[k], GCLineWidth|GCForeground, &values);
		XFillRectangle(dpy, px[k], gc, 0, 0, (UINT) width, (UINT) height);
		values.foreground = fg[k];
		XChangeGC(dpy, gc, GCForeground, &values);
		XDrawLines(dpy, px[k], gc, xp, ww+2, CoordModeOrigin);
		XFreeGC(dpy, gc);
	}
	if (l) *l = px[0];
	if (s) *s = px[1];
	if (i) *i = px[2];

	FreeItem(xp);
}


/*========================== MAP FUNCTIONS = ===============================*/


Boolean IsPointInsideMap(float lat, float lon)
{
	MAP_PROJ *mp;
	POINT    point;

	mp = get_target_map();
	if(!ll_to_pos(mp, lat, lon, point))           return False;
	if(!inside_map_def(&(mp->definition), point)) return False;
	return True;
}



/*================= KEY STRING REPLACEMENT FUNCTIONS =======================*/



/*=========================================================================*/
/*
 * ReplaceKeyword() - Replace a keyword string found in the input string
 *                    with a replacement string. If no replacement is done
 * NULL is returned, otherwise memory is allocated and a string with the
 * replacement is returned. It is the responsibility of the calling function
 * to free this memory.
 *
 * There are two cases depending on wether the cal parameter is NULL or not.
 *
 * 1. If cal is NULL then the string defined by the key parameter is
 *    replaced by the parameter string replace.
 *
 * 2. If cal is not null, the replacement value will be the attribute as
 *    found from the key or the replace string if not found. In the input
 *    string the keyword must have the form "keyattribute@", where '@' is a
 *    special character that terminates the keyword. An example of this
 *    could be <ATTRIB:cloud>, where <ATTRIB: is the key, cloud is the
 *    attribute and '>' is the terminating character. The key parameter has
 *    a special form in that it includes the key and terminating character
 *    but not the attribute. In our example the key parameter would need to
 *    be "<ATTRIB:> to match the <ATTRIB:cloud> keyword in the input string.
 */
/*=========================================================================*/
String ReplaceKeyword(String instr, String key, CAL cal, String replace)
{
	int keylen = 0;
	String brb = NULL;
	String str = NULL;

	if (!instr) return NULL;
	if (!key)   return NULL;
	if (strlen(key) > strlen(instr)) return NULL;

	if(cal)
	{
		int    len, alen;
		char   end, mbuf[256];
		String bre, value;

		if (strlen(key) < 256)
		{
			/* Make the key without the ending token and save the end token */
			(void) strcpy(mbuf, key);
			len = strlen(mbuf) - 1;
			end = mbuf[len];
			mbuf[len] = '\0';
			/*
			 * Find the entire key, including the attribute part.
			 */
			if((brb = strstr(instr,mbuf)) && (bre = strchr(brb+1,end)))
			{
				/* Strip out the attribute part and find its value */
				alen = bre - brb - len;
				if(alen < 256)
				{
					keylen = bre-brb+1;
					(void) strncpy(mbuf, brb+len, (size_t)(alen));
					mbuf[alen] = '\0';
					no_white(mbuf);
					value = CAL_get_attribute(cal, mbuf);
					if(CAL_is_value(value)) replace = value;
				}
			}
		}
	}
	else if((brb = strstr(instr,key)))
	{
		keylen = strlen(key);
	}
	/*
	 * Do the keyword replacement
	 */
	if(keylen > 0)
	{
		str = NewMem(char, strlen(instr) - keylen + safe_strlen(replace) + 1);
		(void) strncpy(str, instr, (size_t)(brb-instr));
		(void) safe_strcat(str, replace);
		(void) safe_strcat(str, brb + keylen);
	}

	return str;
}


/*=========================================================================*/
/*
 *   NewStringReplaceKeyword() - Replace the keyword with the replace
 *                               string and always return an allocated
 *  string if a replacement was done or not. See replace_keyword for
 *  details.
 */
/*=========================================================================*/
String NewStringReplaceKeyword(String instr, String key, CAL cal, String replace)
{
	String rtn = ReplaceKeyword(instr, key, cal, replace);
	if (!rtn) rtn = XtNewString(instr);
	return rtn;
}


/*=========================================================================*/
/*
*    TimeKeyReplace() - Replaces time keys in the input string with the
*                       times taken from the input time string.
*
*    Note: This function allocates memory and it is the responsibility of
*          the calling routine to free this memory.
*/
/*=========================================================================*/
String TimeKeyReplace(String instr, String time)
{
	int     n;
    int     vals[7];
	String  ptr, end, tm, pos;

    static String keys[14] = {	"<year>","<month>","<julian>","<day>" ,"<hour>","<minute>","<delta>",
								"<YEAR>","<MONTH>","<JULIAN>","<DAY>" ,"<HOUR>","<MINUTE>","<DELTA>" };
    static String fmts[14] = {	"%.4d"  ,"%.2d"   ,"%.3d"    ,"%.2d"  ,"%.2d"  ,"%.2d"    ,"%+.2d"  ,
								"%.4d"  ,"%.2d"   ,"%.3d"    ,"%.2d"  ,"%.2d"  ,"%.2d"    ,"%+.2d"   };

	if (!instr) return NULL;

	tm = interpret_timestring(time, GV_T0_depict, 0);
	(void)parse_tstamp(tm, &vals[0], &vals[2], &vals[4], &vals[5], NULL, NULL);
	mdate(&vals[0], &vals[2], &vals[1], &vals[3]);
	vals[6] = HourDif(GV_T0_depict, time);
	/*
	 * This replacement strategy only works because the time string will
	 * always be smaller than the associated keyword.
	 */
	ptr = XtNewString(instr);
	end = ptr + strlen(ptr) + 1;
	for(n = 0; n < 14; n++)
	{
		/* If a key is found once scan again for another occurance */
		do {
			if((pos = strstr(ptr,keys[n])))
			{
				(void) snprintf(pos, end-pos, fmts[n], vals[n]);
				(void) strcat(ptr, pos+strlen(keys[n]));
			}
		} while(pos);
	}
	return ptr;
}



/*========================== MISC FUNCTIONS ================================*/


/*=========================================================================*/
/*
*   GraphicFileTypeByExtent() - Obtain the encoding type from the file
*                               extension. If not recognzed set to X Window
*                               Dump by default.
*/
/*=========================================================================*/
String GraphicFileTypeByExtent( String fname )
{
	int    n;
	String ptr;

	static String ext[] = { "xwd","gif","xgl","jpg", "jpeg", "png" };
	static String fmt[] = { "xwd","gif","xgl","jpeg","jpeg", "png" };

	if(!fname) return fmt[0];

	ptr = strrchr(fname, '.');
	if(!ptr) return fmt[0];

	ptr++;
	for( n = 0; n < XtNumber(ext); n++ )
	{
		if(same_ic(ptr,ext[n])) return fmt[n];
	}
	return fmt[0];
}


/*=========================================================================*/
/* 
 * AllocatedPrint() - This function operates like the AllocPrint function
 *                    below but takes a va_list as an argument. Thus its
 *                    main use is to be embedded in functions that have
 *                    a variable argument list and need printing.
 */
/*=========================================================================*/
String AllocatedPrint(String fmt, va_list ap)
{
	int     n, size = 256; /* arbitrary start size */
	String  p, np;

	if(!(p = NewMem(char,size)))
		return NULL;

	/* 10M bytes is arbitrary but if our output buffer is larger than this
	 * in this application something has gone wrong.
	 */
	while (size < 10000000)
	{
		n = vsnprintf (p, (size_t) size, fmt, ap);
		if (n > -1 && n < size) break;
		if (n > -1)			/* glibc 2.1 */
			size = n+1;		/* precisely what is needed */
		else				/* glibc 2.0 and HPUX 11 */
			size *= 2;		/* twice the old size */

		if(!(np = MoreMem(p,char,size)))
		{
			FreeItem(p);
			break;
		}
		p = np;
	}
	return p;
}


/*=========================================================================*/
/*
 * AllocPrint()  - Returns an array with the information printed into it.
 *                 Use just like the printf function with the format string
 *                 followed by the variables to print. Note that the return
 *                 is an allocated array which must be freed after use.
 */
/*=========================================================================*/
String AllocPrint(String fmt, ...)
{
	int size;
	String  p, s;
	va_list ap;

	va_start(ap, fmt);
	p = AllocatedPrint(fmt, ap);
	va_end(ap);

	/* Copy the return into an array of exactly the right size */
	size = safe_strlen(p);
	s = NewMem(char,size+1);
	if(size > 0) (void) strcpy(s,p);
	FreeItem(p);
	return s;
}


/*=========================================================================*/
/*
 * Manage() - Manage or unmanage a widget.
 */
/*=========================================================================*/
void Manage(Widget w, Boolean manage)
{
	if (w)
	{
		if (manage)
		{
			if(!XtIsManaged(w)) XtManageChild(w);
		}
		else
		{
			if(XtIsManaged(w)) XtUnmanageChild(w);
		}
	}
}


/*=========================================================================*/
/*
 * Map() - Map or Unmap a widget. If the widget is not managed then manage
 *         it as the assumption is that we want it seen. The reverse is
 *         not done as if we are mapping and unmapping the widget the
 *         presumption is it is meant to stay managed.
 */
/*=========================================================================*/
void Map(Widget w, Boolean map)
{
	if (w)
	{
		if (map)
		{
			if(!XtIsManaged(w)) XtManageChild(w);
			XtMapWidget(w);
		}
		else
		{
			XtUnmapWidget(w);
		}
	}
}



/*======================= STRING MANIPULATION =============================*/


static char comment_chars[8] = {'#','*',0,0,0,0,0,0};


/*=========================================================================*/
/*
 * SetReadLineComment() - Define the set of characters that if found at the
 *                        beginning of a line define that line as a comment
 * so that the ReadLine() function ignores the line. Note that this setting
 * is only good for the next invocation of ReadLine() as ReadLine() resets
 * the comment characters to the default after reading the EOF of the file.
 * The default character set is "#*".
 */
/*=========================================================================*/
void SetReadLineComment(String chars)
{
	(void) strncpy(comment_chars, chars, 7);
}


/*=========================================================================*/
/*
*	ReadLine() - Reads a line and checks for a continuation character at
*                the end "\" and concats the line. Returns all of the
*                information read or NULL on a read failure (normally the
*                end of file).
*/
/*=========================================================================*/
String ReadLine(FILE *fp)
{
	int     num, lps, pos;
	String  lpr, buf;
	const String ws = " \t";	/* white space (space or tab) */

	static int    linelen = 0;
	static String linebuf = NULL;

	if (!fp) return NULL;

	if (!linebuf) linebuf = NewMem(char,(linelen = 256));

	buf = NULL;
	lps = linelen;
	lpr = linebuf;
	pos = 0;

	while(getvalidline(fp, lpr, lps, comment_chars))
	{
		/* The assumption here is that if the line read fills the entire
		 * buffer, then the line is most likely longer than the buffer
		 * length and we need to increase the buffer size.
		 */
		if(strlen(lpr) >= lps-1)
		{
			linelen += 256;
			lps = linelen - pos;
			buf = MoreMem(linebuf,char,linelen);
			lpr = linebuf + pos;
			ungetfileline(fp);
		}
		else
		{
			/* Strip leading and trailing space. The following ensures that if the
			 * start of a continuation line is white space that the space will be
			 * preserved if the previous line did not end in space.
			 */
			if(lpr != linebuf && NotNull(strchr(ws,*lpr)) && IsNull(strchr(ws,*(lpr-1))))
				no_white(lpr+1);
			else
				no_white(lpr);

			num = strlen(lpr)-1;
			if(lpr[num] != '\\')
			{
				buf = linebuf;
				break;
			}

			pos += num;
			lpr = linebuf + pos;
			lps = linelen - pos;
		}
	}
	/*
	 * A null buf normally means the end of file has been reached, so free
	 * the buffer used to read the lines.
	 */
	if(!buf)
	{
		FreeItem(linebuf);
		linelen = 0;
		SetReadLineComment("#*");
	}
	return buf;
}


/*=========================================================================*/
/*
*	InList() - returns true if the character string key is found in the
*	list of strings list. The position of key in list is also returned.
*/
/*=========================================================================*/
Boolean InList(String key, int nlist, String *list, int *posn)
{
	int index;

	if(NotNull(key))
	{
		for( index = 0; index < nlist; index++ )
		{
			if( same( key, list[index] ) )
			{
				if (posn) *posn = index;
				return True;
			}
		}
	}
	if (posn) *posn = 0;
	return False;
}


/*=========================================================================*/
/*
*	LineBreak() - Breaks up a string into lines with the maximum line size
*	              and number of lines sepcified. If it is not possible to
*	break at a space and keep to the maximum characters per line then the
*	max line length is exceeded as it is expected that the receiving text
*	output widget will scroll.
*
*	NOTE: This function allocates memory for the return string.  It is the
*         responsibility of the calling routine to free the space.
*
*/
/*=========================================================================*/
String LineBreak(String string, int max_chars_per_line, int maxlines)
{
	int    len, nlines;
	String buf, bptr, endb;
	String sptr, endp, ends, bgns;

	
	if (!string) return XtNewString(" ");

	len  = strlen(string);
	ends = string + len;
	buf  = NewMem(char, len+maxlines+1);
	endb = buf + len + maxlines;

	/* now break the string into lines */
	nlines = 0;
	bgns   = string;
	bptr   = buf;

	while(nlines < maxlines && bgns < ends && bptr < endb )
	{
		/* default pointer position */
		sptr = ends;

		/* do we need to  break what is left of the string */
		if( ends-bgns > max_chars_per_line )
		{
			/* go to end of max line position */
			endp = bgns + max_chars_per_line;
			sptr = endp - 1;

			/* look for place to break into a line */
			while( sptr > bgns && *sptr != ' ' && *sptr != '\n' ) sptr--;

			/* if not found then look for the first one following */
			if(sptr <= bgns)
			{
				sptr = endp;
				while(sptr < ends && *sptr != ' ' && *sptr != '\n' ) sptr++;
			}
		}

		/* copy into our output buffer */
		while(bgns < sptr && bptr < endb) *bptr++ = *bgns++;
		*bptr++ = '\n';
		bgns = sptr+1;
		nlines++;
	}

	/* overwrite the last \n */
	if(bptr > buf) *(bptr-1) = '\0';
	return buf;
}


/*========================== DATA FUNCTIONS ================================*/


/*==========================================================================*/
/*
*	EntryExists() - Checks for the existance of setup file data
*	associated with the given pulldown menu.  If there is no data and the
*   remove_button parameter is True the button is unmanaged. If the
*   parameter is False the button is set insensitive.
*/
/*==========================================================================*/
Boolean EntryExists(int menu_item_id , String setup_keyword , Boolean remove_button )
{
	SETUP *set;
	Widget btn;

	set = GetSetup(setup_keyword);
	if( set->nentry > 0 ) return True;

	btn = XuMenuFindButton(GW_menuBar, menu_item_id);
	if (btn)
	{
		XtSetSensitive(btn, False);
		if(remove_button) XtUnmanageChild(btn);
	}
	return False;
}


/* 
 *	 Gets the list of avaiable fonts and font sizes from the resource file.
 *	 These are the fonts which the user can select and send to Ingred. The
 *	 resource entries are:
 *
 *		.fontsAvailable:       <comma separated list of id's)
 *      .fontsDescription:     <comma separated list of font descriptors>
 *		.fontSizesAvailable:   <comma separated list of sizes>
 *		.fontSizeLabels:       <comma separated list of size labels>
 *
 *	The fontsDescription is optional if none of the fontsAvailable is not
 *	defined internally in Ingred. If used, for every fontsAvailable entry
 *	there should be a corresponding entry in fontsDescription. If a font is
 *	defined internally in Ingred, then the entry may be blank or a dash.
 *
 *	For example:
 *
 *		.fontsAvailable: Helvetica,HelveticaBold,tahoma,TimesRoman
 *		.fontsDescription: -,-,-microsoft-tahoma-medium-r-normal--0-0-0-0-p-0,-
 *		.fontSizesAvailable: 1,2,3,4,5
 *		.fontSizeLabels: " 2 "," 4 "," 6 "," 8 "," 10 "
 *
 *	Note that the fontsDescription is not loaded into ingred by this function
 *	but by InitFonts.
 */
static void read_resource_font_info(void)
{
	int    n;
	char   mbuf[251], nbuf[251];
	String buf, ptr;
	static String module = "ReadFontInfo";

	/* If true the initialization has been done already */
	if(nfont_list != 0) return;

	buf = XtNewString(XuGetStringResource(RNfontsAvailable, "sans,sansBold"));
	ptr = strtok(buf,",");
	n = 0;
	while(ptr)
	{
		n++;
		no_white(ptr);
		if(blank(ptr))
		{
			pr_error(module,"Empty font identifier entry at position number %d in resource file line \"%s\"\n",
					n, RNfontsAvailable);
		}
		else
		{
			if(strpbrk(ptr," \n\t"))
				pr_error(module, "Font identifier \"%s\" specified in resource file line \"%s\" contains white space.\n",
						ptr, RNfontsAvailable);
			font_list = MoreStringArray(font_list, nfont_list+1);
			font_list[nfont_list] = ptr;
			nfont_list++;
		}
		ptr = strtok(NULL,",");
	}
	if(nfont_list < 1)
	{
		pr_error(module,"No map fonts specified in the resource file. See resource line \"%s\".\n", RNfontsAvailable);
	}

	/* These will be the actual font descriptors that correspond to the font id's
	 * defined above. For backwards compatability this line is optional.
	 */
	font_fd = NewStringArray(nfont_list);
	ptr = XuGetStringResource(RNfontsDescription, NULL);
	if(!blank(ptr))
	{
		buf = XtNewString(ptr);
		ptr = strtok(buf, ",");
		for(n = 0; n < nfont_list && ptr != NULL; n++)
		{
			no_white(ptr);
			if(blank(ptr))
			{
				font_fd[n] = "-";
				pr_warning(module,"Empty font descriptor in resource file line \"%s\" corresponding to font identifier \"%s\"\n",
						RNfontsDescription, font_list[n]);
			}
			else
			{
				font_fd[n] = ptr;
			}
			ptr = strtok(NULL,",");
		}
	}

	/* Just in case there were not font specifiers above */
	if(nfont_sizes != 0) return;

	/* Process the font size information
	 */
	ZeroBuffer(mbuf);
	(void)strncpy(mbuf, XuGetStringResource(RNfontSizeLabels, "4,6,8,10,12,14"), 250);
	while((ptr = strchr(mbuf,','))) *ptr = ' ';
	
	ZeroBuffer(nbuf);
	(void)strncpy(nbuf, XuGetStringResource(RNfontSizesAvailable, "1.75,2,2.25,2.5,2.75,3.0"), 250);
	while((ptr = strchr(nbuf,','))) *ptr = ' ';
	n = 0;
	while((ptr = string_arg(mbuf)))
	{
		font_size_labels = MoreStringArray(font_size_labels, nfont_sizes+1);
		font_size_labels[nfont_sizes] = XtNewString(ptr);

		font_sizes = MoreStringArray(font_sizes, nfont_sizes+1);
		if(blank(ptr = string_arg(nbuf)))
		{
			font_sizes[nfont_sizes] = "5";
		}
		else
		{
			font_sizes[nfont_sizes] = XtNewString(ptr);
			n++;
		}
		nfont_sizes++;
	}

	if(nfont_sizes < 1)
	{
		pr_error(module,"No map font sizes specified in the resource file. See resource key \"%s\".\n", RNfontSizeLabels);
	}
	else if(nfont_sizes != n)
	{
		pr_error(module,
			"Number of font size labels and font sizes given in resource file do not agree. See resource keys \"%s\" and \"%s\".\n",
			RNfontSizeLabels, RNfontSizesAvailable);
	}
}


/*============================================================================*/
/* 
 *   InitFonts() - Font initialization
 */
/*============================================================================*/
void InitFonts(void)
{
	int i;
	read_resource_font_info();
	for(i = 0; i < nfont_list; i++)
	{
		if(blank(font_fd[i])   ) continue;
		if(same(font_fd[i],"-")) continue;
		if(IngredVaCommand(GE_ACTION, "FONT ADD %s %s", font_list[i], font_fd[i]) == GE_INVALID)
		{
			pr_error("InitFonts","Unrecognized font descriptor \"%s\". Check resource file line \"%s\".\n",
					font_fd[i], RNfontsDescription);
		}
	}
}


/*============================================================================*/
/*
*	GetMapFontInfo() - Returns font information.
*/
/*============================================================================*/
void GetMapFontInfo(String **fonts, int *nfonts, String **sizes, String **size_labels, int *nsizes )
{
	read_resource_font_info();

	if (fonts)       *fonts       = font_list;
	if (nfonts)      *nfonts      = nfont_list;
	if (sizes)       *sizes       = font_sizes;
	if (size_labels) *size_labels = font_size_labels;
	if (nsizes)      *nsizes      = nfont_sizes;
}



/*============================================================================*/
/*
*  Given the field pointer from the configuration structures this will
*  generate the list of items which may be sampled from that field.
*  Note that this function assignes memory for the list and that it is
*  the responsibility of the calling procedure to free this memory.
*/
/*============================================================================*/
void MakeSampleItemList(FLD_DESCRIPT *fd , SampleListStruct **list , int *nlist )
{
	int                               i, j, nl;
	SampleListStruct                  *l;
	FpaConfigFieldStruct              *fld;
	FpaConfigContinuousSamplingStruct *csp;
	FpaConfigVectorSamplingStruct     *vsp;
	FpaConfigDiscreteSamplingStruct   *dsp;
	FpaConfigWindSamplingStruct       *wsp;
	FpaConfigLineSamplingStruct       *lsp;
	FpaConfigScatteredSamplingStruct  *ssp;
	FpaConfigLchainSamplingStruct     *lcp;
	FpaConfigElementAttribStruct      *ap; 	/* For attribute labelling */

	if (list) *list = NULL;
	if (nlist) *nlist = 0;

	fld = get_field_info(fd->edef->name,fd->ldef->name);
	if(IsNull(fld)) return;
	if(IsNull(fld->element->elem_detail)) return;
	/*
	 * Note that the sampling "type" is a union, so we only need
	 *  to check for "type.continuous"
	 */
	if(IsNull(fld->element->elem_detail->sampling->type.continuous)) return;

	switch(fld->element->fld_type)
	{
		case FpaC_CONTINUOUS:

			csp = fld->element->elem_detail->sampling->type.continuous;
			nl = csp->nsample + csp->nwindsamp + 1;
			l = NewMem(SampleListStruct, nl);
			for(i = 0; i < csp->nsample; i++)
			{
				l[i].type     = FpaCsampleControlValueType;
				l[i].name     = csp->samples[i]->name;
				l[i].label    = csp->samples[i]->label;
				l[i].sh_label = csp->samples[i]->sh_label;
			}
			for(i = 0; i < csp->nwindsamp; i++)
			{
				j = i + csp->nsample;
				l[j].type     = FpaCsampleControlWindType;
				l[j].name     = csp->windsamps[i]->name;
				l[j].label    = csp->windsamps[i]->label;
				l[j].sh_label = csp->windsamps[i]->sh_label;
			}

			j = csp->nsample + csp->nwindsamp;
			l[j].type     = FpaCsampleControlFieldLabels;
			l[j].name     = AttribFieldLabels;
			l[j].label    = XuAssignLabel(AttribFieldLabelsLabel);
			l[j].sh_label = l[j].label;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;

		case FpaC_VECTOR:

			vsp = fld->element->elem_detail->sampling->type.vector;
			nl = vsp->nsample + vsp->nwindsamp + 1;
			l = NewMem(SampleListStruct, nl);
			for(i = 0; i < vsp->nsample; i++)
			{
				l[i].type     = FpaCsampleControlValueType;
				l[i].name     = vsp->samples[i]->name;
				l[i].label    = vsp->samples[i]->label;
				l[i].sh_label = vsp->samples[i]->sh_label;
			}
			for(i = 0; i < vsp->nwindsamp; i++)
			{
				j = i + vsp->nsample;
				l[j].type     = FpaCsampleControlWindType;
				l[j].name     = vsp->windsamps[i]->name;
				l[j].label    = vsp->windsamps[i]->label;
				l[j].sh_label = vsp->windsamps[i]->sh_label;
			}

			j = vsp->nsample + vsp->nwindsamp;
			l[j].type     = FpaCsampleControlFieldLabels;
			l[j].name     = AttribFieldLabels;
			l[j].label    = XuAssignLabel(AttribFieldLabelsLabel);
			l[j].sh_label = l[j].label;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;

		case FpaC_DISCRETE:

			dsp = fld->element->elem_detail->sampling->type.discrete;
			nl = dsp->nsattribs + 2;
			l = NewMem(SampleListStruct, nl);

			l[0].type     = FpaCsampleControlAttribType;
			l[0].name     = AttribAll;
			l[0].label    = XuAssignLabel(AttribAllLabel);
			l[0].sh_label = l[0].label;

			for(i = 0; i < dsp->nsattribs; i++)
			{
				l[i+1].type     = FpaCsampleControlAttribType;
				l[i+1].name     = dsp->sattrib_names[i];
				l[i+1].label    = dsp->sattrib_names[i];
				l[i+1].sh_label = dsp->sattrib_names[i];
			}

			l[i+1].type     = FpaCsampleControlFieldLabels;
			l[i+1].name     = AttribFieldLabels;
			l[i+1].label    = XuAssignLabel(AttribFieldLabelsLabel);
			l[i+1].sh_label = l[i+1].label;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;

		case FpaC_WIND:

			wsp = fld->element->elem_detail->sampling->type.wind;
			l = NewMem(SampleListStruct, wsp->nwcref + 3);

			l[0].type     = FpaCsampleControlAttribType;
			l[0].name     = AttribAll;
			l[0].label    = XuAssignLabel(AttribAllLabel);
			l[0].sh_label = l[0].label;

			nl = 1;
			for(i = 0; i < wsp->nwcref; i++)
			{
				l[nl].type     = FpaCsampleControlWindCrossRef;
				l[nl].name     = wsp->wcrefs[i]->name;
				l[nl].label    = wsp->wcrefs[i]->label;
				l[nl].sh_label = wsp->wcrefs[i]->sh_label;
				nl++;
			}

			l[nl].type     = FpaCsampleControlWindType;
			l[nl].name     = wsp->windsample->name;
			l[nl].label    = wsp->windsample->label;
			l[nl].sh_label = wsp->windsample->sh_label;
			nl++;

			l[nl].type     = FpaCsampleControlFieldLabels;
			l[nl].name     = AttribFieldLabels;
			l[nl].label    = XuAssignLabel(AttribFieldLabelsLabel);
			l[nl].sh_label = l[nl].label;
			nl++;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;

		case FpaC_LINE:

			lsp = fld->element->elem_detail->sampling->type.line;
			nl = lsp->nsattribs + 2;
			l = NewMem(SampleListStruct, nl);

			l[0].type     = FpaCsampleControlAttribType;
			l[0].name     = AttribAll;
			l[0].label    = XuAssignLabel(AttribAllLabel);
			l[0].sh_label = l[0].label;

			for(i = 0; i < lsp->nsattribs; i++)
			{
				l[i+1].type     = FpaCsampleControlAttribType;
				l[i+1].name     = lsp->sattrib_names[i];
				l[i+1].label    = lsp->sattrib_names[i];
				l[i+1].sh_label = lsp->sattrib_names[i];
			}

			l[i+1].type     = FpaCsampleControlFieldLabels;
			l[i+1].name     = AttribFieldLabels;
			l[i+1].label    = XuAssignLabel(AttribFieldLabelsLabel);
			l[i+1].sh_label = l[i+1].label;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;

		case FpaC_SCATTERED:

			ssp = fld->element->elem_detail->sampling->type.scattered;
			nl = ssp->nsattribs + 1;
			l = NewMem(SampleListStruct, nl);

			l[0].type     = FpaCsampleControlAttribType;
			l[0].name     = AttribAll;
			l[0].label    = XuAssignLabel(AttribAllLabel);
			l[0].sh_label = l[0].label;

			for(i = 0; i < ssp->nsattribs; i++)
			{
				l[i+1].type     = FpaCsampleControlAttribType;
				l[i+1].name     = ssp->sattrib_names[i];
				l[i+1].label    = ssp->sattrib_names[i];
				l[i+1].sh_label = ssp->sattrib_names[i];
			}

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;
			
		case FpaC_LCHAIN:

			lcp = fld->element->elem_detail->sampling->type.lchain;
			nl = lcp->nsattribs + 2;
			l = NewMem(SampleListStruct, nl);

			l[0].type     = FpaCsampleControlAttribType;
			l[0].name     = AttribAll;
			l[0].label    = XuAssignLabel(AttribAllLabel);
			l[0].sh_label = l[0].label;

			for(i = 0; i < lcp->nsattribs; i++)
			{
				l[i+1].type     = FpaCsampleControlAttribType;
				l[i+1].name     = lcp->sattrib_names[i];
				l[i+1].label    = lcp->sattrib_names[i];
				l[i+1].sh_label = lcp->sattrib_names[i];
			}

			l[i+1].type     = FpaCsampleControlLinkNodes;
			l[i+1].name     = AttribLinkNodes;
			l[i+1].label    = XuAssignLabel(AttribLinkNodesLabel);
			l[i+1].sh_label = l[i+1].label;

			/* Attribute labelling */
			ap = fld->element->elem_detail->attributes;
			for(i = 0; i < nl; i++)
			{
				for(j = 0; j < ap->nattribs; j++)
				{
					if (same_ic(l[i].name, ap->attrib_names[j]))
					{
						l[i].label    = ap->attrib_labels[j];
						l[i].sh_label = ap->attrib_sh_labels[j];
						break;
					}
				}
			}
			break;


		default:
			return;
	}
	if (list) *list = l; else FreeItem(l);
	if (nlist) *nlist = nl;
}


/*=========================================================================*/
/*
*	RunProgramManager() - Run the fpapm srcipt. The input parameters are
*                         arbitrary in number and must be terminated with a NULL.
*
*   The calling syntax is:
*
*		RunProgramManager(rtn_fcn, rtn_data, args, nargs)
*
*	where: rtn_fcn  - the function to call when this incarnation of the
*                     program manager terminates. rtn_fcn must have a 
*                     prototype of
*                      (void) rtn_fcn()(XtPointer data, int key, String status)
*                     See XuRunSendingProgram function for details.
*
*          rtn_data - the data to pass back to rtn_fcn.
*
*          args     - Xt style Arg parameter containing key - action pairs
*                     of data.  See global.h for the macros.  The meaning
*                     is dependent on the fpapm script which is run by this
*                     function.
*
*          nargs    - number of args arguments.
*
*   Note that the FPA setup file is automatically included as one of the
*   parameters in the program run string.
*/
/*=========================================================================*/
Boolean RunProgramManager(void (*rtn_fcn)(), XtPointer rtn_data, Arg al[] , int ac)
{
	int     i, argno;
	size_t  len;
	String  *args;
	Boolean success;

	args = NewStringArray(ac+2);
	len = safe_strlen(PmNsetupFile)+safe_strlen(GetSetupFile(0,NULL))+4;
	args[0] = NewMem(char, len);
	(void)snprintf(args[0],len, "%s='%s'", PmNsetupFile, GetSetupFile(0,NULL));
	argno = 1;
	for( i = 0; i < ac; i++ )
	{
		len = safe_strlen((String)al[i].name)+safe_strlen((String)al[i].value)+4;
		args[argno] = NewMem(char, len);
		(void)snprintf(args[argno], len, "%s='%s'", (String)al[i].name, (String)al[i].value);
		argno++;
	}
	args[argno] = (String)NULL;
	success = XuRunSendingProgram(FPAPM, args, rtn_fcn, rtn_data);
	FreeList(args, argno);
	return success;
}


/*=========================================================================*/
/*
*   CheckPresetListFiles() - Check for the existance of the preset list
*                            type files and print out a message if they
*                            do not exist. This is done to avoid multiple
*                            warning messages from appearing in the log file.
*/
/*=========================================================================*/
void CheckPresetListFiles(void)
{
	int    n;
	String fname, dir;
	struct stat sb;

	static String module = "CheckPresetListFiles";
	static String fmt = "Predefined %s file \"%s\" in directory \"%s\" not found.\n";
	static String files[] = {
			AREA_SAMPLE_FILTERS_FILE,  "Area Sample Filters",
			LABEL_SAMPLE_FILTERS_FILE, "Label Sample Filters",
			FIELD_VIS_LIST_FILE,       "Field Visibility Settings",
			GUIDANCE_LIST_FILE,        "Guidance Lists",
			POINTS_FILE,               "Point Forecast Definition"
		};

	for( n = 0; n < 10; n+=2 )
	{
		fname = get_file(PRESET_LISTS, files[n]);
		if( fname == NULL || stat(fname,&sb) == -1 )
		{
			dir = get_directory(PRESET_LISTS);
			pr_warning(module, fmt, files[n+1], files[n], dir);
		}
	}
}


/*ARGSUSED*/
void ShowFieldStatusLegendCB( Widget w, XtPointer notused, XtPointer unused )
{
	int i;
	char nbuf[10];
	Pixel colours[7];
	Widget index1, index2;

	static Widget legendDialog;
	static XuDialogActionsStruct action_items[] = {
		{"closeBtn",  XuDestroyDialogCB, NULL }
	};

	if (legendDialog) return;

	legendDialog = XuCreateFormDialog(w, "statusLegend",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XmNnoResize, True,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &legendDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		NULL);

	XtVaGetValues(legendDialog, XmNbackground, &colours[0], NULL);
	colours[1] = XuLoadColorResource(legendDialog, RNnoLinkColorBg,"CadetBlue1");
	colours[2] = XuLoadColorResource(legendDialog, RNpartialLinkColorBg,"RoyalBlue");
	colours[3] = XuLoadColorResource(legendDialog, RNalmostColorBg,"GoldenRod");
	colours[4] = XuLoadColorResource(legendDialog, RNfieldInterpColorBg,"green");
	colours[5] = XuLoadColorResource(legendDialog, RNallSetColorBg,"ForestGreen");
	colours[6] = XuLoadColorResource(legendDialog, RNnotLinkableColorBg,"IndianRed");

	/* Create the index which shows what the coloured boxes mean when we
	*  present the status information. No field will be an empty box.
	*/
	index1 = XmVaCreateRowColumn(legendDialog, "index",
		XmNspacing, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		NULL);

	index2 = XmVaCreateRowColumn(legendDialog, "index",
		XmNspacing, 5,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, index1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, index1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL);

	(void)strcpy(nbuf, "index0");
	for(i = 0; i < XtNumber(colours); i++)
	{
		nbuf[5]++;
		(void) XmVaCreateManagedLabel(index2, nbuf, NULL);
		(void) XmVaCreateManagedLabel(index1, "  ",
			XmNborderWidth, 1,
			XmNbackground, colours[i],
			NULL);
	}
	XtManageChild(index1);
	XtManageChild(index2);
	XuShowDialog(legendDialog);
}


/*=========================== SEQUENCE UTILITY FUNCTIONS ============================*/


void GetSequenceBtnColor( String dt, Pixel *fg, Pixel *bg )
{
	int del, min;

	static Pixel   is_field_bg, is_field_fg;
	static Pixel   mn_field_fg, mn_field_bg;
	static Pixel   no_field_bg, no_field_fg;
	static Boolean first = True;

	if (first)
	{
		first = False;
		is_field_bg = XuLoadColorResource(GW_topLevel, RNsequenceIsFieldBg, "ForestGreen");
		is_field_fg = XuLoadColorResource(GW_topLevel, RNsequenceIsFieldFg, "White"      );
		mn_field_bg = XuLoadColorResource(GW_topLevel, RNsequenceMnFieldBg, "SpringGreen");
		mn_field_fg = XuLoadColorResource(GW_topLevel, RNsequenceMnFieldFg, "Black"      );
		no_field_bg = XuLoadColorResource(GW_topLevel, RNsequenceNoFieldBg, "Red"        );
		no_field_fg = XuLoadColorResource(GW_topLevel, RNsequenceNoFieldFg, "Black"      );
	}

	if( NotNull(dt) && valid_tstamp(dt) && valid_tstamp(GV_T0_depict) )
	{
		min = MinuteDif( GV_T0_depict, dt );
		del = min%60;
		*fg = del ? mn_field_fg : is_field_fg;
		*bg = del ? mn_field_bg : is_field_bg;
	}
	else
	{
		*fg = no_field_fg;
		*bg = no_field_bg;
	}
}

/*===================== Xbae Matrix functions ========================*/


/*ARGSUSED*/
static void enter_cell_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XbaeMatrixEnterCellCallbackStruct *rtn = (XbaeMatrixEnterCellCallbackStruct *)call_data;
	rtn->doit = False;
	rtn->map  = False;
}


/* Create an instance of the XbaeMatrix Widget. In all of the applications of this widget in the
 * program the cells are selected and never edited. To this end this function overrides the
 * button 1 down translation so that the cell is selected on a button 1 down and the enter cell
 * callback is used to disallow editing.
 */
Widget CreateXbaeMatrix(Widget parent, String id, ...)
{
	Cardinal argc = 0;
	Widget   w; 
	String   argname;
	Arg      args[30];
	va_list  ap;
	
	va_start(ap,id);
	while((argname = va_arg(ap,String)) != (String)NULL && argc < 30)
	{
		XtSetArg(args[argc], argname, va_arg(ap,XtArgVal)); argc++;
	}
	va_end(ap);
	XtSetArg(args[argc], XmNcolumnWidthInPixels, True); argc++;

	w = XtCreateManagedWidget(id, xbaeMatrixWidgetClass, parent, args, argc);
	XtOverrideTranslations(w, XtParseTranslationTable(":<Btn1Down>: SelectCell(cell) EditCell(Pointer)"));
	XtAddCallback(w, XmNenterCellCallback, enter_cell_cb, NULL); 
	return w;
}
