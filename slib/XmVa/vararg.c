/* This program creates a variable argument form of the standard XmCreate* functions.
 * The functions to be created must be listed in this program.
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

#include <stdio.h>
#include <Xm/Xm.h>

int main(int *argc, char *argv[])
{
	int  i;
	FILE *out_c,*out_h;

	/* Structure holds the name of the XmCreate* function without the Xm in front
	 * and the header for the function as found in the Xm header directory. If a
	 * given header is used for more than one creation function then all occurances
	 * beyond the first are set to NULL. As the functions are intrinsic to most of
	 * the Motif 2.3 widgets, only a limited subset is done in this case. inMofif23
	 * indicates which ones are native. In this case only the header is output.
	 */
	struct {
		char inMotif23;
		char *name;
		char *header;
	} fcns[] = {
        {False, "ScrolledList",      "List.h"       },
        {False, "ScrolledText",      "Text.h"       },
        {True,  "ArrowButton",       "ArrowB.h"     },
        {True,  "BulletinBoard",     "BulletinB.h"  },
        {True,  "ComboBox",          "ComboBox.h"   },
        {True,  "CascadeButton",     "CascadeB.h"   },
        {True,  "DrawingArea",       "DrawingA.h"   },
        {True,  "DrawnButton",       "DrawnB.h"     },
        {True,  "Form",              "Form.h"       },
        {True,  "Frame",             "Frame.h"      },
        {True,  "Label",             "Label.h"      },
        {True,  "List",               NULL,         },
        {True,  "PopupMenu",         "RowColumn.h"  },
        {True,  "PushButton",        "PushB.h"      },
        {True,  "RowColumn",         "RowColumn.h"  },
        {True,  "Scale",             "Scale.h"      },
        {True,  "ScrollBar",         "ScrollBar.h"  },
        {True,  "ScrolledWindow",    "ScrolledW.h"  },
        {True,  "Separator",         "Separator.h"  },
        {True,  "SpinBox",           "SpinB.h"      },
        {True,  "Text",               NULL,         },
        {True,  "TextField",         "TextF.h"      },
        {True,  "ToggleButton",      "ToggleB.h"    }
    };

    out_h = fopen("XmVaCreate.h","w");
    out_c = fopen("XmVaCreate.c","w");

	/* Build top of header file. */
	fprintf(out_h,"#ifndef XMVACREATE_H\n");
	fprintf(out_h,"#define XMVACREATE_H\n\n");

	/* Include the header file from our list of functions */
	for( i = 0; i < sizeof(fcns)/sizeof(fcns[0]); i++ )
	{
		if(fcns[i].inMotif23 && XmVersion >= 2003) continue;
		if(!fcns[i].header) continue;
		fprintf(out_h, "#include <Xm/%s>\n", fcns[i].header);
	}
	
	/* Create the general widget resource scanning and creation function. Note that no prototype is
	 * used in the function parameters as these can vary. For example the standard Xm functions use
	 * Cardinal for the number of arguments while some vendor functions use int.
	 */
	fprintf(out_c, "#include <stdio.h>\n");
	fprintf(out_c, "#include <stdarg.h>\n");
	fprintf(out_c, "#include <Xm/Xm.h>\n");
	fprintf(out_c, "#include \"XmVaCreate.h\"\n");
	fprintf(out_c, "static Widget crFcn(Widget (*_fcn)(),Widget _parent,String _name, va_list _ap)\n");
	fprintf(out_c, "{\n");
	fprintf(out_c, "String argname;\n");
	fprintf(out_c, "ArgList args;\n");
	fprintf(out_c, "Widget w;\n");
	fprintf(out_c, "size_t act = 20;\n");
	fprintf(out_c, "int ac = 0;\n");
	fprintf(out_c, "args = (ArgList) XtCalloc(act,sizeof(Arg));\n");
	fprintf(out_c, "while((argname = va_arg(_ap,String)) != (char *)0)\n");
	fprintf(out_c, "{\n");
	fprintf(out_c, "if(ac>=act) args = (ArgList) XtRealloc((void*) args, (act+=10)*sizeof(Arg));\n");
	fprintf(out_c, "XtSetArg(args[ac], argname, va_arg(_ap,XtArgVal));\n");
	fprintf(out_c, "ac++;\n");
	fprintf(out_c, "}\n");
	fprintf(out_c, "w = _fcn(_parent, _name, args, ac);\n");
	fprintf(out_c, "XtFree((void*)args);\n");
	fprintf(out_c, "return w;\n");
	fprintf(out_c, "}\n");

	/* Create all of the required variable argument creation forms but only if we are not
	 * using Motif2.3 or greater as most of these functions are defined in these versions
	 * already.
	 */
	(void) fprintf(stdout, "Using Motif Version %d.%d\n", XmVersion/1000, XmVersion%1000);
	for( i = 0; i < sizeof(fcns)/sizeof(fcns[0]); i++ )
	{
		if(fcns[i].inMotif23 && XmVersion >= 2003) continue;

		/* Output prototype of new variable arg motif function */
		fprintf(out_h,"extern Widget XmVaCreate%s(Widget parent, String name, ...);\n",fcns[i].name);
		fprintf(out_h,"extern Widget XmVaCreateManaged%s(Widget parent, String name, ...);\n",fcns[i].name);

		/* Output source code of new variable arg motif function */
		fprintf(out_c,"Widget XmVaCreate%s(Widget _parent, String _name, ...)\n",fcns[i].name);
		fprintf(out_c,"{\n");
		fprintf(out_c,"Widget w;\n");
		fprintf(out_c,"va_list ap;\n");
		fprintf(out_c,"va_start(ap,_name);\n");
		fprintf(out_c,"w = crFcn(XmCreate%s, _parent, _name, ap);\n", fcns[i].name);
		fprintf(out_c,"va_end(ap);\n");
		fprintf(out_c,"return(w);\n");
		fprintf(out_c,"}\n");

		fprintf(out_c,"Widget XmVaCreateManaged%s(Widget _parent, String _name, ...)\n",fcns[i].name);
		fprintf(out_c,"{\n");
		fprintf(out_c,"Widget w;\n");
		fprintf(out_c,"va_list ap;\n");
		fprintf(out_c,"va_start(ap,_name);\n");
		fprintf(out_c,"w = crFcn(XmCreate%s, _parent, _name, ap);\n", fcns[i].name);
		fprintf(out_c,"va_end(ap);\n");
		fprintf(out_c,"XtManageChild(w);\n");
		fprintf(out_c,"return(w);\n");
		fprintf(out_c,"}\n");
	}
	fprintf(out_h,"\n#endif\n");
	fclose(out_c);
	fclose(out_h);
}
