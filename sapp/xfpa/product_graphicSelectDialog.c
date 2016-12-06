/*========================================================================*/
/*
*  File:    product_graphicSelectDialog.c
*
*  Purpose: Provides an independent interface for specifying graphic
*           forecast products.  Products are identified by keyword. The
*           keywords are:
*
*			"graphics" - Ingred is called to produce a raster image which
*                        is output to a file.
*
*           "metafiles" - The actual depiction metafiles are send as a
*                       product in themselves.
*
*           "xxxx" - where xxxx is the name of a graphics processor such
*                    as "psmet", "cormet", "texmet" or some similar.
*                    There is no way to know in advance what this type is so
*                    it is assumed to use the same format.
*
* Notes: 1. This dialog also has a "Preview" function which outputs the
*           psmet and file products to a window so that the forecaster
*           can preview the output. See graphicPreview.c
*        2. The print function is activated by a PRINT= command in the
*           pdf file of the metsis and file products.
*        3. If no product is specified in the setup file then the associated
*           label is taken to be just non-functioning label in the list used
*           to group the graphic products for visual purposes for the user.
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
#include <sys/stat.h>
#include <fcntl.h>
#include "global.h"
#include <Xm/Form.h>
#include <Xm/List.h>
#include <ingred.h>
#include "resourceDefines.h"
#include "help.h"
#include "fpapm.h"
#include "productStatus.h"
#include "graphic.h"

#define ALL  "all"

static void exit_cb (Widget, XtPointer, XtPointer);
static void filter_cb (Widget, XtPointer, XtPointer);
static void generate_cb (Widget, XtPointer, XtPointer);
static void generate_next_product (void);
static void preview_cb (Widget, XtPointer, XtPointer);
static void print_cb (Widget, XtPointer, XtPointer);
static void product_cb (Widget, XtPointer, XtPointer);
static void launch_editor_cb (Widget, XtPointer, XtPointer);

static Widget dialog = NullWidget;
static Widget productList;
static Widget genBtn;
static Widget printBtn;
static Widget previewBtn;
static Widget editBtn;
static Pixel  generate_color = 0;	/* Colour to use for generate button and list colour when generating */
static int    np = 0;				/* The number of graphic products found */
static int    *xref = NULL;			/* Cross reference between the list position and the associated product array position */
static int    nprod_running = 0;	/* How many products are being generated at the moment */
static int    nfilters = 1;			/* Number of members of the product filter list */
static String filters[10] = {ALL};
static String show_key = ALL;
static GraphicProdStruct *prod = NULL; /* The array of graphic products (including labels) */


void InitGraphicProductsDialog(void)
{
	int i;
	String ptr, src;
	Boolean ok;
	SETUP  *setup;
	INFOFILE fh;
	GraphicProdStruct *p;

	static String module = "GraphicProducts";

	generate_color = XuLoadColorResource(GW_mainWindow, RNgraphicProductsGenerateColor, "Green");

	setup = GetSetup(PROD_GRAPHIC);
	prod  = NewMem(GraphicProdStruct, setup->nentry);
	xref  = NewMem(int, setup->nentry);

	/* In the following if the graphics program is not specified it is taken to mean
	 * that the entry is a label in the list and not a graphics program to run. For
	 * backwards compatability the same assumption is made if the pdf is "none".
	 */
	for ( i = 0; i < setup->nentry; i++)
	{
		ok = True;
		p = prod + np;
		p->label   = SetupParm(setup,i,0);
		p->program = SetupParm(setup,i,1);
		p->pdf     = SetupParm(setup,i,2);

		if(blank(p->program))
		{
			p->is_list_label = True;
		}
		else if(same_ic(p->program, "graphics"))
		{
			p->type = GRAPHICS;
			p->viewable = True;
			if(!(p->is_list_label = same_ic(p->pdf,"none")))
			{
				if((fh = info_file_open(get_file(p->program,p->pdf))) != 0)
				{
					p->printable = !blank(info_file_get_data(fh,"PRINT"));
					info_file_close(fh);
				}
				else
				{
					ok = False;
					pr_warning(module, "Unable to open Graphics pdf \"%s\"\n", p->pdf);
				}
			}
		}
		else if(same_ic(p->program, "metafiles"))
		{
			p->type = METAFILES;
			if(!(p->is_list_label = same_ic(p->pdf,"none")))
			{
				ok = ((fh = info_file_open(get_file(p->program,p->pdf))) != 0);
				if (ok)
				{
					/* Make sure the sources are available.
					*/
					ok = False;
					ptr = info_file_get_data(fh,"SOURCE");
					while((src = string_arg(ptr)))
					{
						if(!(ok = !blank(source_directory_by_name(src,NULL,NULL)))) break;
					}
					if(!ok) pr_warning(module, "MetFile pdf \"%s\" : Unrecognized source identifier \"%s\"\n", p->pdf, src);
					info_file_close(fh);
				}
				else
				{
					pr_warning(module, "Unable to open MetFile pdf \"%s\"\n", p->pdf);
				}
			}
		}
		else
		{
			char mbuf[256];
			p->type = FPAMET;
			(void) snprintf(mbuf, sizeof(mbuf), "*graphicProducts*.%s.labelString", p->program);
			if((ok = !blank(XuGetStringResource(mbuf,""))))
			{
				p->dir = SetupParm(setup,i,2);
				p->pdf = SetupParm(setup,i,3);
				p->is_list_label = (same_ic(p->dir,"none") || same_ic(p->pdf,"none"));

				(void) snprintf(mbuf,sizeof(mbuf), ".%sEditor", p->program);
				p->editable = !blank(XuGetStringResource(mbuf,NullString));

				(void) snprintf(mbuf,sizeof(mbuf), ".%sPreview", p->program);
				p->viewable = !blank(XuGetStringResource(mbuf,NullString));
			}
			else
			{
				Warning(module, "GraphicProdType", p->program);
			}
		}

		if (ok)
		{
			xref[np++] = np;
			if(!p->is_list_label)
				p->pid = ProductStatusAddInfo(PS_GRAPHICS, p->label, NULL);
			
			if(!InList(p->program,nfilters,filters,NULL) && nfilters < XtNumber(filters))
			{
				filters[nfilters] = p->program;
				nfilters++;
			}
		}
	}
}


/* Create and show the graphic product selection dialog.
*/
void ACTIVATE_graphicProductsDialog(Widget refw )
{
	int i;
	Widget display;
	XmRenderTable rtable;
	XmRendition rendition;
	Arg args[2];

    static XuDialogActionsStruct action_items_1[] = {
		{ "previewBtn",  preview_cb,  NULL},
		{ "printBtn",    print_cb,    NULL},
		{ "generateBtn", generate_cb, NULL}
	};
    static XuDialogActionsStruct action_items_2[] = {
		{ "graphicEditorBtn",launch_editor_cb,    NULL                  },
		{ "closeBtn",        XuDestroyDialogCB, NULL                  },
		{ "helpBtn",         HelpCB,            HELP_GRAPHIC_PRODUCTS }
	};

	if(dialog) XuShowDialog(dialog);
	if(dialog) return;

	dialog = XuCreateToplevelFormDialog(refw, "graphicProducts",
		XuNdestroyCallback, exit_cb,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		XuNactionAreaRow1Items, action_items_1,
		XuNnumActionAreaRow1Items, XtNumber(action_items_1),
		XuNactionAreaRow2Items, action_items_2,
		XuNnumActionAreaRow2Items, XtNumber(action_items_2),
		NULL);

	previewBtn = XuGetActionAreaBtn(dialog, action_items_1[0]);
	printBtn   = XuGetActionAreaBtn(dialog, action_items_1[1]);
	genBtn     = XuGetActionAreaBtn(dialog, action_items_1[2]);
	editBtn    = XuGetActionAreaBtn(dialog, action_items_2[0]);

	display = XuVaMenuBuildOption(dialog, "display", NULL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < nfilters; i++)
	{
		(void) XuMenuAddButton(display, filters[i], 0, NoId, filter_cb, (XtPointer)filters[i]);
	}

	XuMenuSelectItemByName(display, show_key);

	productList = XmVaCreateManagedScrolledList(dialog, "productList",
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginHeight, 6,
		XmNlistMarginWidth, 6,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, display,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(productList, XmNextendedSelectionCallback, product_cb, NULL);

	filter_cb(NullWidget, (XtPointer)show_key, (XtPointer)NULL);
	XuShowDialog(dialog);

	if( np > 0 ) XmListSelectPos(productList, 1, True);
}


/* Check to see if the times in the product pdf file exist in the depictions
*  so that output can be generated. If some are missing ask the user if they
*  want to proceed anyway.
*/
static Boolean have_valid_times(Widget w, GraphicProdStruct *p)
{
	int      n, len;
	char     mbuf[500];
	String   ptr, toff, tm;
	Boolean  some_ok;
	Source   src;
	INFOFILE fh;

	fh = info_file_open(get_file(p->program, p->pdf));

	ptr = string_arg(info_file_get_data(fh, "SOURCE"));
	src = FindSourceByName(blank(ptr) ? DEPICT : ptr, NULL);
	if(IsNull(src)) src = FindSourceByName(DEPICT, NULL);

	toff = info_file_get_data(fh, "TIME");
	if(blank(toff)) toff = info_file_get_data(fh, "OFFSET");

	some_ok = False;
	strcpy(mbuf, "");
	n = 1;
	len = 500;
	while((ptr = string_arg(toff)))
	{
		if(GetDepictionTimeFromOffset(src, ptr, EXACT, &tm))
		{
			some_ok = True;
		}
		else
		{
			len -= safe_strlen(mbuf);
			strncat(mbuf, DateString(tm,HOURS), len);
			strncat(mbuf, (n%2 == 0) ? "\n":"   ", len);
			n++;
		}
	}
	info_file_close(fh);

	if(!some_ok)
	{
		XuShowMessage(w, "NoDepictTime", NULL);
		return False;
	}
	else if(!blank(mbuf))
	{
		return (XuAskUser(w, "MissingDepict", mbuf, NULL) == XuYES);
	}
	return True;
}


/* Get the list of times for which a graphic can be generated.
*/
static String get_valid_time_list(GraphicProdStruct *p)
{
	int     n, nl, len;
	char    mbuf[2000];
	String  ptr, toff, tm, *l;
	Source  src;
	INFOFILE fh;

	fh = info_file_open(get_file(p->program, p->pdf));

	ptr = string_arg(info_file_get_data(fh, "SOURCE"));
	src = FindSourceByName(blank(ptr) ? DEPICT : ptr, NULL);
	if(IsNull(src)) src = FindSourceByName(DEPICT, NULL);

	toff = info_file_get_data(fh, "TIME");
	if(blank(toff)) toff = info_file_get_data(fh, "OFFSET");

	ptr = string_arg(toff);
	if(IsNull(ptr)) return NULL;

	len = 0;
	strcpy(mbuf, "");

	if(same_ic(ptr, "all"))
	{
		nl = FilteredValidTimeList(src->fd, FpaC_NORMAL, &l);
		for(n = 0; n < nl; n++)
		{
			len += safe_strlen(l[n]) + 1;
			if(len > 2000) break;
			strcat(mbuf, l[n]);
			strcat(mbuf, " ");
		}
		nl = FilteredValidTimeListFree(&l, nl);
	}
	else
	{
		do {
			if(GetDepictionTimeFromOffset(src, ptr, EXACT, &tm))
			{
				len += safe_strlen(tm) + 1;
				if(len > 2000) break;
				strcat(mbuf, tm);
				strcat(mbuf, " ");
			}
		} while(NotNull(ptr = string_arg(toff)));
	}
	info_file_close(fh);
	return XtNewString(mbuf);
}


/*ARGSUSED*/
static void filter_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int i, n = 0;
	XmString label;

	show_key = (String)client_data;
	XuListEmpty(productList);
	for(i = 0; i < np; i++)
	{
		if(!same(show_key,ALL) && !same(show_key,prod[i].program)) continue;
		label = XmStringCreateLocalized(prod[i].label);
		XmListAddItem(productList,label,0);
		XmStringFree(label);
		xref[n++] = i;
	}
	if(n > 0) XmListSelectPos(productList, 1, True);
}


/*ARGSUSED*/
static void product_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;
	GraphicPreviewPopdown();
	if(rtn->selected_item_count > 0)
	{
		int i, ndx, first = -1;
		
		/* If any of the selected positions are a label deselect them */
		for(i = 0; i < rtn->selected_item_count; i++ )
		{
			ndx = xref[rtn->selected_item_positions[i]-1];
			if(prod[ndx].is_list_label)
			{
				XmListDeselectPos(productList, rtn->selected_item_positions[i]);
			}
			else if(first < 0)
			{
				first = ndx;
			}
		}

		if(first >= 0)
		{
			XtSetSensitive(editBtn,    prod[first].editable);
			XtSetSensitive(previewBtn, prod[first].viewable);
			XtSetSensitive(printBtn,   prod[first].printable);

			if(prod[first].type == METAFILES)
				XuWidgetLabel(genBtn, XuGetStringResource(RNsendBtnLabel,"Send"));
			else
				XuWidgetLabel(genBtn, XuGetStringResource(RNgenerateBtnLabel,"Generate"));
		}
	}
}


/*ARGSUSED*/
static void preview_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int nsl, *sl;
	Boolean ok;
	GraphicProdStruct *p;

	if(!XmListGetSelectedPos(productList, &sl, &nsl)) return;
	p = prod + xref[sl[0] - 1];
	FreeItem(sl);
	ok = True;
	if( p->type == GRAPHICS ) ok = have_valid_times(w, p);
	if( ok )
	{
		XuSetDialogCursor(dialog, XuBUSY_CURSOR, ON);
		GraphicPreviewShow(p);
		XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, ON);
	}
}


/* Called when the editor launch finishes.
*/
/*ARGSUSED*/
static void editor_done(XtPointer client_data , int status_key , String status )
{
	Pixel  color;
	String editor = (String)client_data;
	
	if(NotNull(dialog))
	{
		XtVaGetValues(dialog, XmNbackground, &color, NULL);
		XtVaSetValues(editBtn, XmNbackground, color, NULL);
	}
	if(status_key == XuRUN_ERROR && NotNull(status))
	{
		XuShowMessage(NotNull(dialog)? editBtn:GW_mainWindow, "no-pgm", editor, status, NULL);
	}
	FreeItem(editor);
}


/*ARGSUSED*/
static void launch_editor_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int     ac, nsl, *sl;
	char    mbuf[500];
	String  sa, editor;
	Boolean need_files;
	Arg     al[4];
	GraphicProdStruct *p;

	if(!XmListGetSelectedPos(productList, &sl, &nsl)) return;
	p = prod + xref[sl[0] - 1];
	FreeItem(sl);

	/* Change the button colour to let the user know that something is
	*  actually happening.
	*/
	XtVaSetValues(w, XmNbackground, generate_color, NULL);
	XuUpdateDisplay(w);

	(void) snprintf(mbuf,sizeof(mbuf), ".%sEditor", p->program);
	strcpy(mbuf, XuGetStringResource(mbuf,""));
	editor = XtNewString((sa = string_arg(mbuf))? sa : "vi");
	need_files = same_ic(string_arg(mbuf),"<infile>");

	ac = 0;
	XtSetArg(al[0], PmNprogram,     PmGRAPHIC ); ac++;
	XtSetArg(al[1], PmNselect,      PmEDITOR  ); ac++;
	XtSetArg(al[2], PmNpostProcess, editor    ); ac++;
	if (need_files)
	{
/* XXXXXXXXXXXXXXXX Need procedure to find the files to edit XXXXXXXXXXXXXX */
		XtSetArg(al[3], PmNargs, 		""        ); ac++;
	}

	if(!RunProgramManager(editor_done, (XtPointer)editor, al, ac))
	{
		Pixel color;
		FreeItem(editor);
		XtVaGetValues(dialog, XmNbackground, &color, NULL);
		XtVaSetValues(w, XmNbackground, color, NULL);
	}
}


/* Reset the generation indicators/
*/
static void reset_indicators(GraphicProdStruct *p )
{
	p->running = False;
	FreeItem(p->generate_times);
	p->generate = False;
	FreeItem(p->print_times);
	p->print = False;

	ProductStatusUpdateInfo(p->pid, PS_ENDED, NULL);

	nprod_running--;
	if(nprod_running < 0) nprod_running = 0;

	if(!dialog) return;

	if(same(show_key,ALL) || same(show_key,p->program))
	{
		int i, n;
		XmString label;
		/*
		 * Find the position in the list where the product is to be found
		 */
		for(n = 1, i = 0; i < np; i++)
		{
			if(!same(show_key,ALL) && !same(show_key,prod[i].program)) continue;
			if(p == &prod[i]) break;
			n++;
		}
		/*
		 * For some reason using XmListReplacePos() here breaks the code while doing it
		 * when starting up the generate or print does not. Why? No idea. Thus the delete
		 * and add.
		 */
		label = XmStringCreateLocalized(p->label);
		XmListDeletePos(productList, n);
		XmListAddItemUnselected(productList, label, n);
		XuListMakePosVisible(productList, n);
		XmStringFree(label);
	}
	if( nprod_running < 1 )
	{
		Pixel color;
		XtVaGetValues(dialog,   XmNbackground, &color, NULL);
		XtVaSetValues(genBtn,   XmNbackground, color,  NULL);
		XtVaSetValues(printBtn, XmNbackground, color,  NULL);
	}
}


/* Called when the currently running product finishes.
*/
/*ARGSUSED*/
static void product_finished(XtPointer client_data , int status_key , String status )
{
	GraphicProdStruct *p = (GraphicProdStruct *)client_data;
	if(p->type == METAFILES && NotNull(p->dir))
	{
		Boolean ok;
		(void) remove_directory(p->dir, &ok);
		FreeItem(p->dir);
	}
	reset_indicators(p);
	generate_next_product();
}


static void copy_file(String fname, String dir)
{
	int c;
	char mbuf[1024];
	FILE *from_fp, *to_fp;

	if(blank(fname) || blank(dir)) return;

	strcpy(mbuf, dir);
	if(mbuf[safe_strlen(mbuf)-1] != '/') strcat(mbuf, "/");
	strcat(mbuf, base_name(fname,NULL));
	to_fp = fopen(mbuf, "w");
	from_fp = fopen(fname, "r");
	while((c = getc(from_fp)) != EOF) (void) putc(c, to_fp);
	fclose(to_fp);
	fclose(from_fp);
}


static void generate_fpa_metafile(GraphicProdStruct *p )
{
	int ac;
	Arg al[5];

	ac = 0;
	XtSetArg(al[ac], PmNprogram,   PmGRAPHIC   ); ac++;
	XtSetArg(al[ac], PmNselect,    p->program  ); ac++;
	XtSetArg(al[ac], PmNt0,        GV_T0_depict); ac++;
	XtSetArg(al[ac], PmNdirectory, p->dir      ); ac++;
	XtSetArg(al[ac], PmNpdf,       p->pdf      ); ac++;
	if(!RunProgramManager(product_finished, (XtPointer)p, al, ac))
	{
		reset_indicators(p);
	}
}


/* Create the list of metafile names to be copied (in fpapm) to
*  wherever the user has set.
*/
static void generate_metafiles(GraphicProdStruct *p )
{
	int          ac, i, n, nvalid_times, nfields;
	char         dir[500];
	String       ptr, src, slist, flist, list;
	String       post_process, *valid_times, *elm, *lev;
	Boolean      user_specified_target;
	Arg          al[6];
	INFOFILE     fh;
	FLD_DESCRIPT fd;

	XuSetBusyCursor(ON);

	fh = info_file_open(get_file(p->program, p->pdf));

	/* If the user has specified a directory we use this, otherwise we use
	*  a temporary directory and delete it after processing has finished.
	*  The user specified directory is not removed.
	*/
	ptr = info_file_get_data(fh,"TARGET_DIRECTORY");
	user_specified_target = !blank(ptr);
	if(user_specified_target)
	{
		FreeItem(p->dir);
		strcpy(dir, ptr);
	}
	else
	{
		p->dir = tempnam(GV_working_directory,NULL);
		strcpy(dir, p->dir);
	}
	if(!create_directory(dir, S_IRWXU|S_IRWXG|S_IRWXO, NULL))
	{
		pr_error(GV_app_name, "Unable to create the directory \"%s\".\n", dir);
		FreeItem(p->dir);
		XuSetBusyCursor(OFF);
		return;
	}

	/* If the link file is required and copy it to the target directory. */
	if(same_ic("true", info_file_get_data(fh,"SEND_LINK_FILE")))
	{
		copy_file(depiction_link_file(), dir);
	}

	/* Get the list of fields to be processed. */
	nfields = 0;
	elm = NewStringArray(GV_nfield);
	lev = NewStringArray(GV_nfield);
	flist = XtNewString(info_file_get_data(fh,"FIELDS"));
	if(same_ic(flist, "ALL"))
	{
		nfields = GV_nfield;
		for(i = 0; i < GV_nfield; i++)
		{
			elm[i] = GV_field[i]->info->element->name;
			lev[i] = GV_field[i]->info->level->name;
		}
	}
	else
	{
		elm[0] = strtok_arg(flist);
		while(nfields < GV_nfield && elm[nfields] != NULL && (lev[nfields] = strtok_arg(NULL)) != NULL)
		{
			nfields++;
			elm[nfields] = strtok_arg(NULL);
		}
	}

	/* Loop through the list of sources */
	slist = XtNewString(info_file_get_data(fh,"SOURCE"));
	while((src = string_arg(slist)) != NULL)
	{
		init_fld_descript(&fd);
		if(!set_fld_descript(&fd, FpaF_SOURCE_NAME, src, FpaF_END_OF_LIST)) continue;

		/* Get the list of required valid times */
		list = info_file_get_data(fh,"TIME");
		if(same_ic(list, "ALL"))
		{
			nvalid_times = FilteredValidTimeList(&fd, FpaC_TIMEDEP_ANY, &valid_times);
		}
		else
		{
			/* The list is given as +dt or -dt where dt is the delta from T0 */
			char   mbuf[32];
			String del;

			nvalid_times = 0;
			valid_times  = NULL;
			while((del = string_arg(list)))
			{
				mbuf[31] = '\0';
				strncpy(mbuf, del, 31);
				valid_times = MoreMem(valid_times, String, nvalid_times+1);
				valid_times[nvalid_times] = XtNewString(ParseTimeDeltaString(mbuf));
				nvalid_times++;
			}
		}

		/* Create the file names. find_meta_filename returns NULL if file does not exist */
		for(i = 0; i < nvalid_times; i++)
		{
			for(n = 0; n < nfields; n++)
			{
				if(set_fld_descript(&fd, FpaF_VALID_TIME, valid_times[i],
					FpaF_ELEMENT_NAME, elm[n], FpaF_LEVEL_NAME, lev[n],
					FpaF_END_OF_LIST))
				{
					copy_file(find_meta_filename(&fd), dir);
				}
			}
		}
		nvalid_times = FilteredValidTimeListFree(&valid_times, nvalid_times);
	}
	FreeItem(slist);
	FreeItem(flist);
	FreeItem(elm);
	FreeItem(lev);

	/* Set up for running fpapm for any post_processing */
	if(blank(post_process = info_file_get_data(fh,"POST_PROCESS")))
	{
		if(!user_specified_target)
		{
			pr_error(GV_app_name, "No POST_PROCESS line in metafile pdf \"%s\"", p->pdf);
			FreeItem(p->dir);
		}
		reset_indicators(p);
	}
	else
	{
		ptr = NewStringReplaceKeyword(post_process, METADIR, NULL, dir);
		ac = 0;
		XtSetArg(al[ac], PmNprogram,     PmGRAPHIC ); ac++;
		XtSetArg(al[ac], PmNselect,      p->program   ); ac++;
		XtSetArg(al[ac], PmNpostProcess, ptr       ); ac++;
		if( !RunProgramManager(product_finished, (XtPointer)p, al, ac) )
		{
			reset_indicators(p);
		}
		FreeItem(ptr);
	}
	info_file_close(fh);
	XuSetBusyCursor(OFF);
}


static void generate_graphics(GraphicProdStruct *p )
{
	int  ac, height, width;
	char mbuf[512], mode[30], olay[256], tm[20];
	String  dfile, ptr, ifile, ofile, post_process, fmt;
	Boolean pad, ok;
	Arg al[10];
	INFOFILE fh;

	ifile = (String)NULL;
	ofile = (String)NULL;
	post_process = (String)NULL;

	fh = info_file_open(get_file(p->program, p->pdf));

	fmt = GraphicFileTypeByExtent(info_file_get_data(fh,"OUTPUT"));

	ptr = info_file_get_data(fh, "HEIGHT");
	height = int_arg(ptr, &ok);
	if (!ok) height = 600;

	ptr = info_file_get_data(fh, "WIDTH");
	width = int_arg(ptr, &ok);
	if (!ok) width = 600;

	ptr = info_file_get_data(fh, "MODE");
	strcpy(mode, ptr);

	ptr = info_file_get_data(fh, "PAD");
	pad = same_start_ic(ptr, "T");

	ptr = info_file_get_data(fh, "OVERLAY");
	ptr = get_file(p->program, ptr);
	strcpy(olay, blank(ptr) ? "" : ptr);

	strcpy(tm, string_arg(p->generate_times));

	dfile = tempnam(GV_working_directory,NULL);
	(void) snprintf(mbuf,sizeof(mbuf), "RASTER_DUMP %s %s %s %d %d %s %s %s",
		fmt, mode, tm,
		width, height,
		dfile,
		pad ? "PAD" : "NOPAD",
		olay);

	if(IngredCommand(GE_ACTION, mbuf))
	{
		/* Set up the information required for the fpapm script.
		*/
		ac = 0;
		XtSetArg(al[ac], PmNprogram, PmGRAPHIC); ac++;
		XtSetArg(al[ac], PmNselect, p->program); ac++;
		XtSetArg(al[ac], PmNfileName, dfile); ac++;
		XtSetArg(al[ac], PmNt0, GV_T0_depict); ac++;
		XtSetArg(al[ac], PmNtime, tm); ac++;

		ptr = info_file_get_data(fh,"OUTPUT");
		if(!blank(ptr))
		{
			ofile = TimeKeyReplace(ptr, tm);
			XtSetArg(al[ac], PmNoutputFile, ofile); ac++;
		}

		ptr = info_file_get_data(fh,"INFO");
		if(!blank(ptr))
		{
			ifile = TimeKeyReplace(ptr, tm);
			XtSetArg(al[ac], PmNinfoFile, ifile); ac++;
		}

		ptr = info_file_get_data(fh,"POST_PROCESS");
		if(!blank(ptr))
		{
			post_process = TimeKeyReplace(ptr, tm);
			XtSetArg(al[ac], PmNpostProcess, post_process); ac++;
		}

		if( !RunProgramManager(product_finished, (XtPointer)p, al, ac) )
		{
			reset_indicators(p);
		}

		FreeItem(ifile);
		FreeItem(ofile);
		FreeItem(post_process);
	}
	info_file_close(fh);
	FreeItem(dfile);
}


static void print(GraphicProdStruct *p )
{
	int i, ac;
	float val;
	char mbuf[512], olay[256];
	char xbuf[16], ybuf[16], dbuf[16];
	String ptr, dfile, line, printer, mode, tm;
	Boolean ok;
	Arg al[10];
	INFOFILE fh;

    String module = "GraphicProductSelect";

    float   xlen = 7.9;
    float   ylen = 10.0;
    float   xoff = 0;
    float   yoff = 0;
    int     dpi  = 150;
    int     nx, ny;


	fh = info_file_open(get_file(p->program, p->pdf));

	ptr = info_file_get_data(fh, "OVERLAY");
	ptr = get_file(p->program, ptr);
	strcpy(olay, blank(ptr) ? "" : ptr);

	line = info_file_get_data(fh,"PRINT");
	printer = strtok_arg(line);
	mode = strtok_arg(NULL);
	if(same(mode,"portrait"))
	{
		xlen = 7.9;
		ylen = 10.0;
	}
	else if(same(mode,"landscape"))
	{
		xlen = 10.0;
		ylen = 7.9;
	}
	else
	{
		Warning(module, "PrintOrientation", mode);
		return;
	}
	val = floattok_arg(NULL, &ok);
	if(ok)
	{
		xoff = val;
		xlen -= val;
	}
	val = floattok_arg(NULL, &ok);
	if(ok)
	{
		yoff = val;
		ylen -= val;
	}
	val = floattok_arg(NULL, &ok);
	if(ok) xlen = val;
	val = floattok_arg(NULL, &ok);
	if(ok) ylen = val;
	i = inttok_arg(NULL, &ok);
	if(ok) dpi = i;

	if (xlen<=0 || ylen<=0)
	{
		(void) snprintf(mbuf,sizeof(mbuf), "%g X %g",xlen,ylen);
		Warning(module, "PrintSize", mbuf);
		return;
	}
	nx = (int) ceil(xlen*dpi);
	ny = (int) ceil(ylen*dpi);

	tm = string_arg(p->print_times);

	dfile = tempnam(GV_working_directory,NULL);

	(void) snprintf(mbuf,sizeof(mbuf), "RASTER_DUMP xwd bw %s %d %d %s NOPAD %s",
		tm, nx, ny, dfile, olay);

	if(IngredCommand(GE_ACTION, mbuf))
	{
		ac = 0;
		XtSetArg(al[ac], PmNprogram, PmSCREEN_PRINT); ac++;
		XtSetArg(al[ac], PmNinfoFile, dfile); ac++;
		XtSetArg(al[ac], PmNpageMode, mode); ac++;
		XtSetArg(al[ac], PmNprinter, printer); ac++;
		if(xoff > 0)
		{
			(void) snprintf(xbuf, sizeof(xbuf), "%g", xoff);
			XtSetArg(al[ac], PmNxOffset, xbuf); ac++;
		}
		if(yoff > 0)
		{
			(void) snprintf(ybuf, sizeof(ybuf), "%g", yoff);
			XtSetArg(al[ac], PmNyOffset, ybuf); ac++;
		}
		(void) snprintf(dbuf, sizeof(dbuf), "%d", dpi);
		XtSetArg(al[ac], PmNdpi, dbuf); ac++;
		if( !RunProgramManager(product_finished, (XtPointer)p, al, ac) )
		{
			reset_indicators(p);
		}
	}
	info_file_close(fh);
	FreeItem(dfile);
}


/* Generate the next product which has the generate or print flags set.
*/
static void generate_next_product(void)
{
	int i;
	Pixel gen_color;
	GraphicProdStruct *p;

	/* Check to see if there are any more products flaged to be generated.
	*/
	for( i = 0; i < np; i++ )
	{
		p = prod + i;
		if(!p->generate && !p->print) continue;

		p->running = True;
		nprod_running++;
		gen_color = XuLoadColorResource(genBtn, RNgraphicProductsGenerateColor, "Green");

		if(p->generate)
		{
			if(NotNull(dialog))
			{
				XtVaSetValues(genBtn, XmNbackground, gen_color, NULL);
				XuUpdateDisplay(dialog);
			}
			switch(p->type)
			{
				case FPAMET:    generate_fpa_metafile(p);    break;
				case METAFILES: generate_metafiles(p); break;
				case GRAPHICS:  generate_graphics(p);  break;
			}
		}
		else if(p->print)
		{
			if(NotNull(dialog))
			{
				XtVaSetValues(printBtn, XmNbackground, gen_color, NULL);
				XmUpdateDisplay(dialog);
			}
			print(p);
		}
		break;
	}
}


/*ARGSUSED*/
static void print_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int i, n, nsl, *sl;
	XmString label[1];
	GraphicProdStruct *p;

	/* If anything is running ignore the print request */
	if(nprod_running > 0) return;

	/* If there is a pending edit put up a requester asking to proceed or not.  */
	if(XtIsSensitive(GW_editorAcceptBtn))
	{
		if(XuAskUser(w,"pending_edit",NULL) == XuNO) return;
	}

	/* There must be something selected to proceed */
	if( !XmListGetSelectedPos(productList, &sl, &nsl) ) return;

	/* Clear all of the print flags */
	for(i = 0; i < np; i++)
		prod[i].print = False;

	for( i = 0; i < nsl; i++ )
	{
		n = xref[sl[i]-1];
		p = prod + n;
		if(!p->printable) continue;
		if(p->is_list_label) continue;

		p->print = have_valid_times(w, p);
		if(p->print)
		{
			if(!p->generate) ProductStatusUpdateInfo(p->pid, PS_RUNNING, NULL);
			FreeItem(p->print_times);
			p->print_times = get_valid_time_list(p);
			label[0] = XmStringCreate(p->label, ITALIC_FONT);
			XmListReplaceItemsPosUnselected(productList, label, 1, sl[i]);
			XmStringFree(label[0]);
		}
	}
	FreeItem(sl);
	generate_next_product();
}


/* We will only check the required depiction times and not the list of fields
*  to transmit as it is frequently possible that the forecaster will not have
*  the fields in the sequence for a good reason. The depiction times, however,
*  is another matter is there is a specific list of times given.
*/
static void check_metafiles(Widget w , GraphicProdStruct *p )
{
	int nvalid_times, nnone;
	String del, srcstr, src, none[25];
	String *valid_times;
	INFOFILE fh;
	FLD_DESCRIPT fd;

	fh = info_file_open(get_file(p->program, p->pdf));
	if(same_ic("ALL", info_file_get_data(fh,"TIME")))
	{
		info_file_close(fh);
		return;
	}

	srcstr = XtNewString(info_file_get_data(fh,"SOURCE"));

	while((src = string_arg(srcstr)) != NULL && p->generate)
	{
		init_fld_descript(&fd);
		(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, src, FpaF_END_OF_LIST);
		nvalid_times = FilteredValidTimeList(&fd, FpaC_TIMEDEP_ANY, &valid_times);
		nnone = 0;
		del = strtok_arg(info_file_get_data(fh,"TIME"));
		while(del)
		{
			if(!InTimeList(ParseTimeDeltaString(del), valid_times, nvalid_times, NULL) && nnone < 25)
			{
				none[nnone] = del;
				nnone++;
			}
			del = strtok_arg(NULL);
		}
		(void)FilteredValidTimeListFree(&valid_times, nvalid_times);
		if(nnone > 0)
		{
			int i;
			char mbuf[500];
			strcpy(mbuf, "");
			for(i = 0; i < nnone; i++)
			{
				strcat(mbuf, DateString(ParseTimeDeltaString(none[i]),HOURS));
				strcat(mbuf, ((i+1)%2 == 0) ? "\n":"   ");
			}
			p->generate = (XuAskUser(w,"MissingDepict",mbuf,NULL) == XuYES);
		}
	}
	FreeItem(srcstr);
	info_file_close(fh);
}


static void check_graphics(Widget w , GraphicProdStruct *p )
{
	if((p->generate = have_valid_times(w, p)))
	{
		FreeItem(p->generate_times);
		p->generate_times = get_valid_time_list(p);
	}
}


/* Called when the generate button is pushed.  More than one list entry can be
*  processed at a time, but are set up to run sequentially.
*/
/*ARGSUSED*/
static void generate_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int i, nsl, *sl;
	XmString label[1];
	GraphicProdStruct *p;

	/* If anything is running ignore the generate request */
	if(nprod_running > 0) return;

	/* If there is a pending edit put up a requester asking to proceed or not.  */
	if(XtIsSensitive(GW_editorAcceptBtn))
	{
		if(XuAskUser(w,"pending_edit",NULL) == XuNO) return;
	}

	/* There must be something selected */
	if( !XmListGetSelectedPos(productList, &sl, &nsl) ) return;

	/* Clear all of the generate flags */
	for(i = 0; i < np; i++)
		prod[i].generate = False;

	for( i = 0; i < nsl; i++ )
	{
		/* >>>>> Added this check - June 2011 <<<<< */
		if (sl[i] < 0 || sl[i] > np)
		{
			pr_diag("generate_cb", "Error in sl[0] - nsl: %d  - sl[0]/np: %d/%d\n",
					nsl, sl[0], np);
			continue;
		}
		/* >>>>> Added this check - June 2011 <<<<< */

		p = prod + xref[sl[i]-1];
		if(p->is_list_label) continue;
		p->generate = True;
		if(!p->print) ProductStatusUpdateInfo(p->pid, PS_RUNNING, NULL);

		switch(p->type)
		{
			case METAFILES: check_metafiles(w, p); break;
			case GRAPHICS:  check_graphics (w, p); break;
		}
	}
	FreeItem(sl);
	generate_next_product();
}


/*ARGSUSED*/
static void exit_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	GraphicPreviewPopdown();
	dialog = NULL;
}
