/*========================================================================*/
/*
*	File:		ingredCommand.c
*
*	Purpose:	Contains functions responsible for commanding Ingred.
*               The return will be True if the return from the Ingred
*   function is GE_VALID. The function SendIngredCommands() sets the
*   toggle which allows commands to be sent to Ingred. This is useful
*   for turning off interface commands when they are not wanted (normally
*   during the initialization phase).
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
#include <stdarg.h>
#include "global.h"
#include <ingred.h>
#include "depiction.h"

static Boolean send_to_ingred = True;

void SendIngredCommands(Boolean state )
{
	send_to_ingred = state;
}


/* This is defined so that we can handle the CAL structures which are only
*  sent to ingred when edit commands are required. The second extended CAL
*  calx, is only used by the lchain field at the moment to set the node
*  cal values at the same time as the chain cal values.
*/
Boolean IngredEditCommand(String cmd, CAL cal, CAL calx)
{
	GEREPLY rtn;
	XuUpdateDisplay(GW_mainWindow);
	ZoomStateCheck(GE_DEPICTION, cmd);
	if(!send_to_ingred) return True;
	rtn = GEDepiction(cmd, cal, calx);
	return (rtn == GE_VALID);
}

void IngredVaEditCommand(CAL cal, CAL calx, String fmt, ...)
{
	String  p;
	va_list ap;

	va_start(ap, fmt);
	p = AllocatedPrint(fmt, ap);
	va_end(ap);
	if (p) (void) IngredEditCommand(p, cal, calx);
	FreeItem(p);
}


Boolean IngredCommand(GE_CMD type , String cmd )
{
	int     fcn;
	GEREPLY rtn;
	String  names[] = {
		"GEAction","GEAnimate","GEDepiction","GEEdit", "GEGuidance","GEImagery",
		"GESequence","GEScratchpad","GETimelink","GEZoom","Unknown" };

	/* Return true if an explicit lockout is in effect */
	if(!send_to_ingred) return True;

	XuUpdateDisplay(GW_mainWindow);
	ZoomStateCheck(type, cmd);

	switch(type)
	{
		case GE_ACTION:		fcn = 0;  rtn = GEAction(cmd);	break;
		case GE_ANIMATE:	fcn = 1;  rtn = GEAnimate(cmd);	break;
		case GE_DEPICTION:	fcn = 2;  rtn = GEDepiction(cmd,NullCal,NullCal); break;
		case GE_EDIT:		fcn = 3;  rtn = GEEdit(cmd); break;
		case GE_GUIDANCE:	fcn = 4;  rtn = GEGuidance(cmd); break;
		case GE_IMAGERY:    fcn = 5;  rtn = GEImagery(cmd); break;
		case GE_SEQUENCE:	fcn = 6;  rtn = GESequence(cmd); break;
		case GE_SCRATCHPAD:	fcn = 7;  rtn = GEScratchpad(cmd); break;
		case GE_TIMELINK:	fcn = 8;  rtn = GETimelink(cmd); break;
		case GE_ZOOM:		fcn = 9;  rtn = GEZoom(cmd); break;
		default:            fcn = 10; rtn = GE_INVALID; break;
	}

	pr_diag("SendToIngred", "%s(%s)\n", names[fcn], cmd);

	return (rtn == GE_VALID);
}


Boolean IngredVaCommand(GE_CMD type, String fmt, ...)
{
	String  p;
	Boolean rtn = False;
	va_list ap;

	va_start(ap, fmt);
	p = AllocatedPrint(fmt, ap);
	va_end(ap);
	if (p) rtn = IngredCommand(type, p);
	FreeItem(p);
	return rtn;
}
