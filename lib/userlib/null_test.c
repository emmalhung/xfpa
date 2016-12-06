
/* FPA library definitions */
#include <fpa.h>

int		main

(
)

{
int				ii;
FLD_DESCRIPT	*descript = NullPtr(FLD_DESCRIPT *);

ii = sizeof(int);
fprintf(stdout, "Integer size: %d\n", ii);
fprintf(stdout, "Maximum integer: %d\n", MAXINT);

if ( IsNull(descript) ) fprintf(stdout, " Null descriptor ... OK\n");
if ( IsNull(descript) || IsNull(descript->sdef) )
	fprintf(stdout, " Null descriptor or pointer ... OK\n");
if ( IsNull(descript) && IsNull(descript->sdef) )
	fprintf(stdout, " Null descriptor and pointer ... OK\n");
else
	fprintf(stdout, " Null descriptor and pointer ... bad pointer\n");

return 0;
}
