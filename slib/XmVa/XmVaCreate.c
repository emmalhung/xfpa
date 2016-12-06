#include <stdio.h>
#include <stdarg.h>
#include <Xm/Xm.h>
#include "XmVaCreate.h"
static Widget crFcn(Widget (*_fcn)(),Widget _parent,String _name, va_list _ap)
{
String argname;
ArgList args;
Widget w;
size_t act = 20;
int ac = 0;
args = (ArgList) XtCalloc(act,sizeof(Arg));
while((argname = va_arg(_ap,String)) != (char *)0)
{
if(ac>=act) args = (ArgList) XtRealloc((void*) args, (act+=10)*sizeof(Arg));
XtSetArg(args[ac], argname, va_arg(_ap,XtArgVal));
ac++;
}
w = _fcn(_parent, _name, args, ac);
XtFree((void*)args);
return w;
}
Widget XmVaCreateScrolledList(Widget _parent, String _name, ...)
{
Widget w;
va_list ap;
va_start(ap,_name);
w = crFcn(XmCreateScrolledList, _parent, _name, ap);
va_end(ap);
return(w);
}
Widget XmVaCreateManagedScrolledList(Widget _parent, String _name, ...)
{
Widget w;
va_list ap;
va_start(ap,_name);
w = crFcn(XmCreateScrolledList, _parent, _name, ap);
va_end(ap);
XtManageChild(w);
return(w);
}
Widget XmVaCreateScrolledText(Widget _parent, String _name, ...)
{
Widget w;
va_list ap;
va_start(ap,_name);
w = crFcn(XmCreateScrolledText, _parent, _name, ap);
va_end(ap);
return(w);
}
Widget XmVaCreateManagedScrolledText(Widget _parent, String _name, ...)
{
Widget w;
va_list ap;
va_start(ap,_name);
w = crFcn(XmCreateScrolledText, _parent, _name, ap);
va_end(ap);
XtManageChild(w);
return(w);
}
