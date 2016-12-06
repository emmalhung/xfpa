
/* Things that can be evaluated from a VECFLD */
typedef enum
	{
	VFEvalU,		/* U(x,y) (x-component) */
	VFEvalV,		/* V(x,y) (y-component) */
	VFEvalMag,		/* Magnitude sqrt(U*U + V*V) */
	VFEvalDir,		/* Direction atan2(V,U) (math degrees) */
	VFEvalDiv,		/* Divergence (DEL . F) */
	VFEvalCurl,		/* Curl (DEL X F) */
	} VFEVAL;
typedef enum
	{
	VFCalcFunc,		/* the function itself */
	VFCalcDerivX,	/* 1st deriv wrt x (df/dx) */
	VFCalcDerivY,	/* 1st deriv wrt y (df/dy) */
	VFCalcDerivXX,	/* 2nd deriv wrt xy (d2f/dx2) */
	VFCalcDerivXY,	/* 2nd deriv wrt xy (d2f/dxdy) */
	VFCalcDerivYX,	/* 2nd deriv wrt yx (d2f/dydx) (same as xy) */
	VFCalcDerivYY,	/* 2nd deriv wrt yy (d2f/dy2) */
	VFCalcCurv,		/* curvature */
	} VFCALC;
