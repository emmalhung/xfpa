/***********************************************************************/
/** @file	gribmeta.c
 *
 * Routines to convert decoded GRIB data to FPA Metafile objects.
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    g r i b m e t a . c                                               *
*                                                                      *
*    Routines to convert decoded GRIB data to FPA Objects              *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

#define GRIBMETA_MAIN	/* To initialize defined constants and     */
						/*  internal structures in gribmeta.h file */

/* We need FPA definitions */
#include <fpa.h>

/* We need definitions for GRIB data structures */
#include "rgrib.h"

#include "gribmeta.h"

#include <math.h>
#include <stdio.h>

#ifdef DEBUG_GRIBMETA
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_GRIBMETA */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Accepted Grid Templates */
#define GT_LATLON				0
#define GT_ROTATED_LATLON		1
#define GT_STRETCHED_LATLON		2
#define GT_MERCATOR				10
#define GT_PSTEREO				20
#define GT_LAMBERT				30
#define GT_GAUSS				40
#define GT_ROTATED_GAUSS		41
#define GT_STRETCHED_GAUSS		42
/* Interface functions                  */
/*  ... these are defined in gribmeta.h */

/* Internal static functions */
static	SURFACE	grib_data_to_surface(DECODEDFIELD *, FLD_DESCRIPT *);
static	SURFACE	grib_data_to_surface_by_comp(DECODEDFIELD *, FLD_DESCRIPT *,
												COMPONENT, COMPONENT);
static	LOGICAL fill_in_missing_data (DECODEDFIELD *, float **, LOGICAL **);
static  LOGICAL	reorder_data (DECODEDFIELD *, float **);
static  LOGICAL	irregular2regular (DECODEDFIELD *, float *);
static  void	wrap_data (DECODEDFIELD *, float **);

/***********************************************************************
*                                                                      *
*    g r i b f i e l d _ t o _ m e t a f i l e                         *
*    g r i b f i e l d _ t o _ s u r f a c e                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Convert the given DECODEDFIELD to a METAFILE Object.
 *
 * Build a SURFACE into a METAFILE Object which bears the matching timestamp 
 * and projection information.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data.
 * @param[in]	*fdesc		pointer to output field descriptor.
 * @param[in]	units		GRIB field units label
 * @return	METAFILE object containing the GRIB data.
 ***********************************************************************/
METAFILE		gribfield_to_metafile

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units		/* GRIB field units label */
	)

	{
	METAFILE	meta;
	SURFACE		sfc;

	/* Return Null METAFILE if no GRIB data */
	if ( !gribfld ) return NullMeta;

	/* Return Null METAFILE if no GRIB map projection */
	if ( IsNull(gribfld->mproj) ) return NullMeta;

	/* Create SURFACE Object from GRIB Data */
	sfc = gribfield_to_surface(gribfld, fdesc, units);
	if ( !sfc ) return NullMeta;

	/* Create METAFILE Object to hold SURFACE Object */
	meta = create_metafile();
	(void) define_mf_tstamp(meta, fdesc->rtime, fdesc->vtime);
	(void) define_mf_projection(meta, &fdesc->mproj);

	/* Add the GRIB map projection as source projection for GRIB stitching */
	(void) add_mf_source_proj(meta, gribfld->mproj);

	/* Add SURFACE Object to METAFILE Object */
	(void) add_sfc_to_metafile(meta, "a",
			fdesc->edef->name, fdesc->ldef->name, sfc);

	/* Return METAFILE Object */
	return meta;
	}

/***********************************************************************/
/** Convert the given DECODEDFIELD to a SURFACE Object.
 *
 * Optionally build the SURFACE into a METAFILE Object which bears
 * the matching timestamp and projection information.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data.
 * @param[in]	*fdesc		pointer to output field descriptor.
 * @param[in]	units		GRIB field units label
 * @return	SURFACE object containing the GRIB data.
 ***********************************************************************/
SURFACE			gribfield_to_surface

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units		/* GRIB field units label */
	)

	{
	FpaConfigUnitStruct		*udef;
	SURFACE					sfc;

	static USPEC			uspec = {NullString, 1.0, 0.0};

	/* Return Null SURFACE if no GRIB data */
	if ( !gribfld ) return NullSfc;

	/* Determine units information for GRIB field units label */
	udef = identify_unit(units);
	if ( !udef ) return NullSfc;

	/* Determine SURFACE Object from GRIB data */
	sfc = grib_data_to_surface(gribfld, fdesc);
	if ( !sfc ) return NullSfc;

	/* Set units specs from GRIB field units information */
	(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
	(void) define_surface_units(sfc, &uspec);

	/* Return SURFACE Object */
	return sfc;
	}

/***********************************************************************
*                                                                      *
*    g r i b f i e l d _ t o _ m e t a f i l e _ b y _ c o m p         *
*    g r i b f i e l d _ t o _ s u r f a c e _ b y _ c o m p           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Build a SURFACE component  into a METAFILE Object which bears
 * the matching timestamp and projection information.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data.
 * @param[in]	*fdesc		pointer to output field descriptor.
 * @param[in]	units		GRIB field units label
 * @param[in]	compin		input component identifier
 * @param[in]	compout		output component identifier
 * @return Metafile object with given data, projection and timestamp.
 ***********************************************************************/
METAFILE		gribfield_to_metafile_by_comp

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units,		/* GRIB field units label */
	COMPONENT		compin,		/* input component identifier */
	COMPONENT		compout		/* output component identifier */
	)

	{
	int						isrc;
	const COMP_INFO			*cinfo;
	SURFACE					sfc;
	METAFILE				meta;

	/* Return Null METAFILE if no GRIB data */
	if ( !gribfld ) return NullMeta;

	/* Return Null METAFILE if no x/y component info for output field */
	if ( !xy_component_field(fdesc->edef->name) ) return NullMeta;

	/* Return Null METAFILE if no GRIB map projection */
	if ( IsNull(gribfld->mproj) ) return NullMeta;

	/* Create SURFACE Object from GRIB Data */
	sfc = gribfield_to_surface_by_comp(gribfld, fdesc, units, compin, compout);
	if ( !sfc ) return NullMeta;

	/* Create METAFILE Object to hold SURFACE Object */
	meta = create_metafile();
	(void) define_mf_tstamp(meta, fdesc->rtime, fdesc->vtime);
	(void) define_mf_projection(meta, &fdesc->mproj);

	/* Add the GRIB map projection as source projection for GRIB stitching */
	(void) add_mf_source_proj(meta, gribfld->mproj);

	/* Add the component to the source projection */
	isrc  = meta->nsrc - 1;
	cinfo = fdesc->edef->elem_detail->components->cinfo;
	(void) define_mf_source_comp(meta, isrc, cinfo);
	if ( !add_component(&meta->scomp[isrc], compin) )
		{
		sfc  = destroy_surface(sfc);
		meta = destroy_metafile(meta);
		return NullMeta;
		}

	/* Add SURFACE Object to METAFILE Object */
	(void) add_sfc_to_metafile(meta, "a",
			fdesc->edef->name, fdesc->ldef->name, sfc);

	/* Return METAFILE Object */
	return meta;
	}

/***********************************************************************/
/** Convert the given DECODEDFIELD component to a SURFACE Object.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data.
 * @param[in]	*fdesc		pointer to output field descriptor.
 * @param[in]	units		GRIB field units label
 * @param[in]	compin		input component identifier
 * @param[in]	compout		output component identifier
 * @return Surface object with given data.
 ***********************************************************************/
SURFACE			gribfield_to_surface_by_comp

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units,		/* GRIB field units label */
	COMPONENT		compin,		/* input component identifier */
	COMPONENT		compout		/* output component identifier */
	)

	{
	FpaConfigUnitStruct		*udef;
	SURFACE					sfc;

	static USPEC			uspec = {NullString, 1.0, 0.0};

	/* Return Null SURFACE if no GRIB data */
	if ( !gribfld ) return NullSfc;

	/* Return Null SURFACE if no x/y component info for output field */
	if ( !xy_component_field(fdesc->edef->name) ) return NullSfc;

	/* Return Null SURFACE if no units information for GRIB field units label */
	udef = identify_unit(units);
	if ( !udef ) return NullSfc;

	/* Return Null SURFACE if no GRIB map projection */
	if ( IsNull(gribfld->mproj) ) return NullSfc;

	/* Determine SURFACE Object from GRIB data */
	sfc = grib_data_to_surface_by_comp(gribfld, fdesc, compin, compout);
	if ( !sfc ) return NullSfc;

	/* Set units specs from GRIB field units information */
	(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
	(void) define_surface_units(sfc, &uspec);

	/* Return SURFACE Object */
	return sfc;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES                                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this source file.                                                *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    g r i b _ d a t a _ t o _ s u r f a c e                           *
*    g r i b _ d a t a _ t o _ s u r f a c e _ b y _ c o m p           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Convert the given GRIBFIELD Object to a SURFACE Object.
 *
 *  The given GRIBFIELD is remapped to the grid points of a grid
 *  defined by the output map projection.
 *
 *  Locations outside the boundary of the supplied GRIBFIELD are
 *  set to the value at the nearest boundary.
 *
 *  Note that all distances are given in km, all latitudes in
 *  degrees North, all longitudes in degrees East, and that the
 *  reference longitude is parallel to the y axis.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to output field descriptor
 * @return	Surface object containing given data.
 **********************************************************************/
static SURFACE	grib_data_to_surface

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc		/* pointer to output field descriptor */
	)

	{
	GRID_DEF	gdef, odef;
	GRID		gridd;
	SURFACE		sfc;
	float		*data   = gribfld->data;
	LOGICAL		*bitmap = gribfld->bmap;

	/* Return Null SURFACE if no GRIB data */
	if ( !gribfld ) return NullSfc;

	/* Return Null SURFACE if no GRIB map projection */
	if ( IsNull(gribfld->mproj) ) return NullSfc;

	/* Fill in missing data if required */
	if ( NotNull(bitmap) ) 
		{
		if ( !fill_in_missing_data(gribfld, &data, &bitmap) ) return NullSfc;
		gribfld->data = data;
		}

	/* Reorder data */
	if ( !reorder_data(gribfld, &data) ) return NullSfc;
	gribfld->data = data;

	/* If Gaussian projection convert it to a Lat-Lon projection */
	 if ( GT_GAUSS == gribfld->projection )
		{
		if ( !irregular2regular(gribfld, data)) return NullSfc;
		gribfld->projection = GT_LATLON;
		}
	 
	/* Wrap data if projection wraps around world */
	(void) wrap_data(gribfld, &data);
	gribfld->data = data;

	/* Move array of GRIB data into GRID Object */
	gdef = gribfld->mproj->grid;
	(void) init_grid(&gridd);
	(void) define_grid(&gridd, gdef.nx, gdef.ny, ZeroPoint, 0.0, gdef.gridlen,
			data, gdef.nx);

	/* Remap the GRIB data to the output map projection */
	(void) remap_grid(&gridd, gribfld->mproj, &fdesc->mproj);
	odef = fdesc->mproj.grid;

	/* Move grid of remapped GRIB data to SURFACE Object */
	sfc = create_surface();
	(void) grid_surface(sfc, odef.gridlen, odef.nx, odef.ny, gridd.gval);
	(void) free_grid(&gridd);

	/* Return pointer to SURFACE Object */
	return sfc;
	}

/**********************************************************************/
/** Convert the given GRIBFIELD Object to a SURFACE Object.
 *
 *  The given GRIBFIELD is remapped to the grid points of a grid
 *  defined by the output map projection.
 *
 *  Locations outside the boundary of the supplied GRIBFIELD are
 *  set to the value at the nearest boundary.
 *
 *  Note that all distances are given in km, all latitudes in
 *  degrees North, all longitudes in degrees East, and that the
 *  reference longitude is parallel to the y axis.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to output field descriptor
 * @return	Surface object containing given data.
 **********************************************************************/
static SURFACE	grib_data_to_surface_by_comp

	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	COMPONENT		compin,		/* input component identifier */
	COMPONENT		compout		/* output component identifier */
	)

	{
	int			compflag, iix, iiy;
	float		**coefs;
	GRID_DEF	gdef, odef;
	GRID		gridd;
	SURFACE		sfc;
	float		*data   = gribfld->data;
	LOGICAL		*bitmap = gribfld->bmap;

	/* Return Null SURFACE if no GRIB data */
	if ( !gribfld ) return NullSfc;

	/* Return Null SURFACE if no x/y component info for output field */
	if ( !xy_component_field(fdesc->edef->name) ) return NullSfc;

	/* Return Null SURFACE if no GRIB map projection */
	if ( IsNull(gribfld->mproj) ) return NullSfc;

	/* Fill in missing data if required */
	if ( NotNull(bitmap) ) 
		{
		if ( !fill_in_missing_data(gribfld, &data, &bitmap) ) return NullSfc;
		gribfld->data = data;
		}

	/* Reorder data */
	if ( !reorder_data(gribfld, &data) ) return NullSfc;
	gribfld->data = data;

	/* If Gaussian projection convert it to a Lat-Lon projection */
	 if ( GT_GAUSS == gribfld->projection )
		{
		if ( !irregular2regular(gribfld, data)) return NullSfc;
		gribfld->projection = GT_LATLON;
		}

	/* Wrap data if projection wraps around world */
	(void) wrap_data(gribfld, &data);
	gribfld->data = data;

	/* Move array of GRIB data into GRID Object */
	gdef = gribfld->mproj->grid;
	(void) init_grid(&gridd);
	(void) define_grid(&gridd, gdef.nx, gdef.ny, ZeroPoint, 0.0, gdef.gridlen,
			data, gdef.nx);

	/* Remap the GRIB data to the output map projection */
	(void) remap_grid(&gridd, gribfld->mproj, &fdesc->mproj);
	odef = fdesc->mproj.grid;

	/* Determine component coefficients */
	compflag = gribfld->component_flag;
	if ( compflag < 0 ) return NullSfc;
	if ( !grid_component_coefficients(compflag, gribfld->mproj, compin,
			&fdesc->mproj, compout, &coefs) ) return NullSfc;

	/* Adjust grid values by component coefficients (if required) */
	if ( coefs )
		{
		for ( iiy=0; iiy<odef.ny; iiy++ )
			for ( iix=0; iix<odef.nx; iix++ )
				{
				gridd.gval[iiy][iix] *= coefs[iiy][iix];
				}
		}

	/* Move grid of remapped GRIB data to SURFACE Object */
	sfc = create_surface();
	(void) grid_surface(sfc, odef.gridlen, odef.nx, odef.ny, gridd.gval);
	(void) free_grid(&gridd);

	/* Return pointer to SURFACE Object */
	return sfc;
	}

/***********************************************************************
*                                                                      *
*    f i l l _ b i t m a p                                             *
*    f i l l _ i n _ m i s s in g _ d a t a                            *
*    N o t T w i x t                                                   *
*                                                                      *
*    Given a full dataset with masked out bits. Fill in the masked out *
*    bits with a reasonable approximation                              *
***********************************************************************/
/**********************************************************************/
/** Similar to the twixt function. This function performs a constant
 *  or linear interpolation depending on the information available.
 *
 * The constant or linear interpolation is faster than a spline interpolation.
 *
 * @param[in]	numkeys		number of "filled in" values in the row.
 * @param[in]	*keylist	list of locations for the "filled in" 
 * 							values in the row.
 * @param[in]	*keyval		list of "filled in" values in the row.
 * @param[in]	numtweens	full length of row.
 * @param[in]	*tweenlist	not really sure what this is for.
 * @param[out] 	*tweenval adjusted to linearly interpolate missing values.
 **********************************************************************/
static	void		NotTwixt
	(
	 int	numkeys,
	 double	*keylist,
	 double *keyval,
	 int	numtweens,
	 double *tweenlist,
	 double *tweenval
	)
	{
	int ikey, skey, ekey, ii, gap;
	float sval, eval, frac;
	

	if (!keylist || !keyval || !tweenlist || !tweenval ) return;
	skey = 0;
	sval = keyval[0];

	for (ii=0; ii< numkeys; ii++)
		{
		ekey = keylist[ii];
		eval = keyval[ii];
		gap  = ekey - skey;

		for (ikey=skey; ikey<ekey; ikey++) 
			{
			frac = (ikey - skey)/(float)gap;
			tweenval[ikey] = ((sval)*(1-frac)+(eval)*(frac));
			}
		skey = ekey;
		sval = eval;
		}
		for (ikey=skey; ikey<numtweens; ikey++) tweenval[ikey] = (sval);

	}

/* Default number of grid spaces for bit map extrapolation */
static	const	int			BitMapEx = 1;

/**********************************************************************/
/** Fill in the bitmap to show which values have been filled in.
 *
 * @param[in]	npts	number of ON bits in row.
 * @param[in]	*bitloc	location of ON bits in row.
 * @param[in]	bitex	how far to expand bits out.
 * @param[in]	nout	total number of bits in row.
 * @param[out]	*bitsout array of bits to set.
 **********************************************************************/
static	void		fill_bitmap

	(
	int							npts,
	int							*bitloc,
	int							bitex,
	int							nout,
	LOGICAL						*bitsout
	)

	{
	int				nn, nnx, nbgn, ntrue;

	/* Initialize output bit map */
	for (nn=0; nn<nout; nn++) bitsout[nn] = TRUE;

	/* Return immediately if output bit map row or column is almost complete */
	if ( (nout - npts) <= (bitex * 2) ) return;

	/* Loop to reset output bit map for locations beyond extrapolation limit */
	for (nbgn=0, nn=0; nn<npts; nn++)
		{
		ntrue = bitloc[nn];
		if ( (ntrue - nbgn) > bitex )
			{
			for (nnx=nbgn; nnx<(ntrue-bitex); nnx++) bitsout[nnx] = FALSE;
			}
		nbgn = ntrue + bitex + 1;
		}
	if ( nbgn < nout )
		{
		for (nnx=nbgn; nnx<nout; nnx++) bitsout[nnx] = FALSE;
		}

	/* Return output bit map */
	return;
	}

/**********************************************************************/
/** Fill in missing values in an incomplete grid. 
 *
 * This function is required if you wish to create a surface from an
 * otherwise incomplete grid.
 *
 * 	@param[in]		*gribfld		DECODEDFIELD Object with decoded GRIB data
 *	@param[in,out]	**pgrib_data	filled in data grid
 *	@param[in,out]  **pbit_data		Bitmap
 *	@return True if successful.
 **********************************************************************/
static	LOGICAL			fill_in_missing_data
	(
	DECODEDFIELD	*gribfld,		/* DECODEDFIELD Object with decoded GRIB data */
	float			**pgrib_data,	/* filled in data grid */
	LOGICAL     	**pbit_data		/* Bitmap */
	)
	{
	
	/* After some experimentation I have found that the combination of 5 Twixt interations
	 * and 100 NotTwixt iterations appears to be a happy compromise providing a reduction
	 * in noise but also a reasonable compute time. 
	 */
	const 	int		MaxNItr = 5, MaxMItr = 100;
	int				ii, ni, jj, nj, npts, nitr=0;
	LOGICAL			isweep ;
	unsigned int	num_vals;
	LOGICAL			complete;


	/* Input/Output data parameters */
	static LOGICAL	*bit_data         = NullPtr(LOGICAL *);
	static LOGICAL	*bit_data_by_row  = NullPtr(LOGICAL *);
	static LOGICAL	*bit_data_by_col  = NullPtr(LOGICAL *);
	static float	*grib_data        = NullFloat;
	static float	*grib_data_by_row = NullFloat;
	static float	*grib_data_by_col = NullFloat;
	static int		*bitqr = NullInt;
	static double	*posqr = NullDouble, *valqr  = NullDouble;
	static LOGICAL	*bflags = NullPtr(LOGICAL *);
	static double	*pstns = NullDouble, *values = NullDouble;
	LOGICAL			*bit_in, *bit_out, *bit_out_by_row, *bit_out_by_col;
	float			*val_in, *val_out, *val_out_by_row, *val_out_by_col;

	/* Do this only once for each GRIB field */
	if (gribfld->filled) return TRUE;
	gribfld->filled = TRUE;

	if ( IsNull(*pbit_data) )  return FALSE; /* Nothing to expand */
	if ( IsNull(*pgrib_data) ) return FALSE; /* Nothing to expand */

	/* Set sweep direction */
	isweep = gribfld->isweep;

	/* Set grid dimensions */
	ni = gribfld->mproj->grid.nx;
	nj = gribfld->mproj->grid.ny;

	/* Initialize pointer to input bit map flags and data values */
	bit_in = *pbit_data;
	val_in = *pgrib_data;

	/* Set final grid dimensions for expanded regular grid */
	num_vals = ni * nj;

	/* Initialize space to hold bit map flags and data values */
	/*  at all grid points                                    */
	bit_data  = GETMEM(bit_data, LOGICAL, num_vals);
	bit_out   = bit_data;
	grib_data = GETMEM(grib_data, float, num_vals);
	val_out   = grib_data;

	/* Initialize regular grid of bit map flags and data values */
	for(jj=0; jj<num_vals; jj++, bit_out++, bit_in++, val_out++, val_in++)
			{
			*bit_out = *bit_in;
			*val_out = *val_in;
			}

	/* Initialize space to hold bit map flags and data values  */
	/*  at all grid points for by row and by column processing */
	bit_data_by_row  = GETMEM(bit_data_by_row, LOGICAL, num_vals);
	bit_data_by_col  = GETMEM(bit_data_by_col, LOGICAL, num_vals);
	grib_data_by_row = GETMEM(grib_data_by_row, float, num_vals);
	grib_data_by_col = GETMEM(grib_data_by_col, float, num_vals);

	/* Initialize space to hold row or column data for processing */
	npts = MAX(ni, nj);
	bitqr  = GETMEM(bitqr,  int,     npts);
	posqr  = GETMEM(posqr,  double,  npts);
	valqr  = GETMEM(valqr,  double,  npts);

	bflags = GETMEM(bflags, LOGICAL, npts);
	pstns  = GETMEM(pstns,  double,  npts);
	values = GETMEM(values, double,  npts);
	for(ii=0; ii<npts; ii++)
		pstns[ii] = (double) ii;

	/* Loop through bit-map data by row and by column until all missing */
	/*  data locations have been filled in                              */
	do
		{

		/* Set flag for no missing data */
		complete = TRUE;

		/* Process grid of bit map flags and data values    */
		/*  for data with i'th direction incrementing first */
		if ( !isweep )
			{

			/* Initialize bit map and data values for output arrays */
			bit_in         = bit_data;
			bit_out_by_row = bit_data_by_row;
			bit_out_by_col = bit_data_by_col;
			val_in         = grib_data;
			val_out_by_row = grib_data_by_row;
			val_out_by_col = grib_data_by_col;
			for(jj=0; jj<nj; jj++)
				for(ii=0; ii<ni; ii++)
					{
					*bit_out_by_row++ = *bit_out_by_col++ = *bit_in++;
					*val_out_by_row++ = *val_out_by_col++ = *val_in++;
					}

			/* Fill in missing data row by row */
			for(jj=0; jj<nj; jj++)
				{
				for(npts=0, ii=0; ii<ni; ii++)
					{

					/* Set data for each row based on bit map */
					if ( bit_data[jj*ni + ii] )
						{
						bitqr[npts] = ii;
						posqr[npts] = pstns[ii];
						valqr[npts] = (double) grib_data[jj*ni + ii];
						npts++;
						}
					}

				/* Reset bit map and values for this row */
				if ( npts > 0 )
					{

					/* Determine values at all positions for this row */
					if (nitr < MaxNItr )
						(void)Twixt(npts, posqr, valqr, ni, pstns, values);
					else
						(void)NotTwixt(npts, posqr, valqr, ni, pstns, values);

					/* Determine adjusted bit map for this row */
					(void) fill_bitmap(npts, bitqr, BitMapEx, ni, bflags);

					/* Reset bit map and data values for this row */
					for(ii=0; ii<ni; ii++)
						{
						if (nitr < MaxMItr ) bit_data_by_row[jj*ni + ii]  = bflags[ii];
						else bit_data_by_row[jj*ni + ii]  = TRUE;
						grib_data_by_row[jj*ni + ii] = values[ii];
						}
					}
				}

			/* Fill in missing data column by column */
			for(ii=0; ii<ni; ii++)
				{
				for(npts=0, jj=0; jj<nj; jj++)
					{

					/* Set data for each column based on bit map */
					if ( bit_data[jj*ni + ii] )
						{
						bitqr[npts] = jj;
						posqr[npts] = pstns[jj];
						valqr[npts] = (double) grib_data[jj*ni + ii];
						npts++;
						}
					}

				/* Reset bit map and values for this column */
				if ( npts > 0 )
					{

					/* Determine values at all positions for this column */
					if (nitr < MaxNItr)
						(void)Twixt(npts, posqr, valqr, nj, pstns, values);
					else
						(void)NotTwixt(npts, posqr, valqr, nj, pstns, values);

					/* Determine adjusted bit map for this column */
					(void) fill_bitmap(npts, bitqr, BitMapEx, nj, bflags);

					/* Reset bit map and data values for this column */
					for(jj=0; jj<nj; jj++)
						{
						if (nitr < MaxMItr) bit_data_by_col[jj*ni + ii]  = bflags[jj];
						else bit_data_by_col[jj*ni + ii]  = TRUE;
						grib_data_by_col[jj*ni + ii] = values[jj];
						}
					}
				}

			/* Now use row value, columm value, or an average of the two */
			bit_out        = bit_data;
			bit_out_by_row = bit_data_by_row;
			bit_out_by_col = bit_data_by_col;
			val_out        = grib_data;
			val_out_by_row = grib_data_by_row;
			val_out_by_col = grib_data_by_col;
			for(jj=0; jj<num_vals; jj++)
				{
				if (!*bit_out)
				{
				if ( *bit_out_by_row && *bit_out_by_col )
					{
					*bit_out++ = TRUE;
					*val_out++ = (*val_out_by_row + *val_out_by_col) / 2.0;
					}
				else if ( *bit_out_by_row )
					{
					*bit_out++ = TRUE;
					*val_out++ = *val_out_by_row;
					}
				else if ( *bit_out_by_col )
					{
					*bit_out++ = TRUE;
					*val_out++ = *val_out_by_col;
					}
				else
					{
					*bit_out++ = FALSE;
					*val_out++ = 0.0;
					/* Reset flag for missing data */
					complete   = FALSE;
					}
				} else {
					bit_out++;
					val_out++;
				}
				bit_out_by_row++;
				bit_out_by_col++;
				val_out_by_row++;
				val_out_by_col++;
				}
			}

		/* Process grid of bit map flags and data values    */
		/*  for data with j'th direction incrementing first */
		else
			{

			/* Initialize bit map and data values for output arrays */
			bit_in         = bit_data;
			bit_out_by_row = bit_data_by_row;
			bit_out_by_col = bit_data_by_col;
			val_in         = grib_data;
			val_out_by_row = grib_data_by_row;
			val_out_by_col = grib_data_by_col;
			for(ii=0; ii<ni; ii++)
				for(jj=0; jj<nj; jj++)
					{
					*bit_out_by_row++ = *bit_out_by_col++ = *bit_in++;
					*val_out_by_row++ = *val_out_by_col++ = *val_in++;
					}

			/* Fill in missing data row by row */
			for(jj=0; jj<nj; jj++)
				{
				for(npts=0, ii=0; ii<ni; ii++)
					{

					/* Set data for each row based on bit map */
					if ( bit_data[ii*nj + jj] )
						{
						bitqr[npts] = ii;
						posqr[npts] = pstns[ii];
						valqr[npts] = (double) grib_data[ii*nj + jj];
						npts++;
						}
					}

				/* Reset bit map and values for this row */
				if ( npts > 0 )
					{

					/* Determine values at all positions for this row */
					if (nitr < MaxNItr)
						(void)Twixt(npts, posqr, valqr, ni, pstns, values);
					else 
						(void)NotTwixt(npts, posqr, valqr, ni, pstns, values);

					/* Determine adjusted bit map for this row */
					(void) fill_bitmap(npts, bitqr, BitMapEx, ni, bflags);

					/* Reset bit map and data values for this row */
					
					for(ii=0; ii<ni; ii++)
						{
						if (nitr < MaxMItr) bit_data_by_row[ii*nj + jj]  = bflags[ii];
						else bit_data_by_row[ii*nj + jj]  = TRUE;
						grib_data_by_row[ii*nj + jj] = values[ii];
						}
					}
				}

			/* Fill in missing data column by column */
			for(ii=0; ii<ni; ii++)
				{
				for(npts=0, jj=0; jj<nj; jj++)
					{

					/* Set data for each column based on bit map */
					if ( bit_data[ii*nj + jj] )
						{
						bitqr[npts] = jj;
						posqr[npts] = pstns[jj];
						valqr[npts] = (double) grib_data[ii*nj + jj];
						npts++;
						}
					}

				/* Reset bit map and values for this column */
				if ( npts > 0 )
					{

					/* Determine values at all positions for this column */
					if (nitr < MaxNItr)
						(void)Twixt(npts, posqr, valqr, nj, pstns, values);
					else
						(void)NotTwixt(npts, posqr, valqr, nj, pstns, values);

					/* Determine adjusted bit map for this column */
					(void) fill_bitmap(npts, bitqr, BitMapEx, nj, bflags);

					/* Reset bit map and data values for this column */
					for(jj=0; jj<nj; jj++)
						{
						if (nitr < MaxMItr) bit_data_by_col[ii*nj + jj]  = bflags[jj];
						else bit_data_by_col[ii*nj + jj]  = TRUE;
						grib_data_by_col[ii*nj + jj] = values[jj];
						}
					}
				}

			/* Now use row value, columm value, or an average of the two */
			bit_out        = bit_data;
			bit_out_by_row = bit_data_by_row;
			bit_out_by_col = bit_data_by_col;
			val_out        = grib_data;
			val_out_by_row = grib_data_by_row;
			val_out_by_col = grib_data_by_col;
			for(ii=0; ii<num_vals; ii++)
				{
				if (!*bit_out)
					{
					if ( *bit_out_by_row && *bit_out_by_col )
						{
						*bit_out++ = TRUE;
						*val_out++ = (*val_out_by_row + *val_out_by_col) / 2.0;
						}
					else if ( *bit_out_by_row )
						{
						*bit_out++ = TRUE;
						*val_out++ = *val_out_by_row;
						}
					else if ( *bit_out_by_col )
						{
						*bit_out++ = TRUE;
						*val_out++ = *val_out_by_col;
						}
					else
						{
						*bit_out++ = FALSE;
						*val_out++ = 0.0;
						/* Reset flag for missing data */
						complete   = FALSE;
						}
					} else {
						bit_out++;
						val_out++;
					}
					bit_out_by_row++;
					bit_out_by_col++;
					val_out_by_row++;
					val_out_by_col++;
				}
			}
		nitr++;
		} while ( !complete );

	/* Set pointer for returned GRIB data */
	*pgrib_data = grib_data;
	*pbit_data  = bit_data;
	dprintf(stderr, "  Completed expansion from bit mapped data\n");
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    r e o r d e r _ d a t a                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Re-order grib data if required.
 *
 *	Preferred FPA order is:
 *	-	Points of first row scan in +i (+x) direction
 *	-	Points of first column scan in -j (-y) direction
 *	-	Adjacent points in i (x) direction are consecutive
 *	-	Adjacent rows in scan in the same direction
 *
 * @param[in]		*gribfld		DECODEDFIELD object with decoded GRIB data
 * @param[in,out]	**pgrib_data 	data to reorder.
 * @return True if successful.
 **********************************************************************/
static LOGICAL	reorder_data
	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	float 			**pgrib_data
	)
	{
	static float   	*fld_out=NullFloat;
	float     		*value_in, *value_out;
	int     		jj, ii, nx, ny;
	LOGICAL 		left, bottom, isweep, rsweep;

	/* Do this only once for each GRIB field */
	if (gribfld->reordered) return TRUE;
	gribfld->reordered = TRUE;

	isweep = gribfld->isweep;
	rsweep = gribfld->rsweep;
	/* Interpret scan_mode */
	left   = gribfld->left;
	bottom = gribfld->bottom;

	ny        = gribfld->mproj->grid.ny;
	nx        = gribfld->mproj->grid.nx;
	fld_out   = GETMEM(fld_out, float, (nx * ny) );
	value_out = fld_out;

	/******************************************************/
	/* Re-order grib data if required                     */
	/* Preferred FPA order is:                            */
	/* Points of first row scan in +i (+x) direction      */
	/* Points of first column scan in -j (-y) direction   */
	/* Adjacent points in i (x) direction are consecutive */
	/* Adjacent rows in scan in the same direction        */
	/******************************************************/
	for(jj=0; jj<ny; jj++)
		{
		for(ii=0; ii<nx; ii++)
			{
			/* Data with i'th direction incrementing first */
			if (!isweep)
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered bottom to top */
					if ( bottom ) 
						{
						value_in = *pgrib_data + (jj*nx) + ii;
						}
					/* Data ordered top top bottom */
					else
						{
						value_in = *pgrib_data + ((ny-jj-1)*nx) + ii;
						}
					}
				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom ) 
						{
						value_in = *pgrib_data + (jj*nx) + (nx-ii-1);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + ((ny-jj-1)*nx) + (nx-ii-1);
						}
					}
				}
			/* Data with j'th direction incrementing first */
			else
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered bottom to top */
					if ( bottom ) 
						{
						value_in = *pgrib_data + jj + (ii*ny);
						}
					/* Data ordered top to bottom */
					else
						{
						value_in = *pgrib_data + (ny-jj-1) + (ii*ny);
						}
					}
				/* Data ordered right to left */
				else
					{
					/* Data ordered bottom to top */
					if ( bottom ) 
						{
						value_in = *pgrib_data + jj + ((nx-ii-1)*ny);
						}
					/* Data ordered top top bottom */
					else
						{
						value_in = *pgrib_data + (ny-jj-1) + ((nx-ii-1)*ny);
						}
					}
				}
			/* Set value in output data array */
			*value_out++ = *value_in;
			}
		/* If adjacent rows scan in opposite directions the reverse sense of "left" */
		if ( rsweep ) left = !left;
		}

	*pgrib_data = fld_out;

	dprintf(stderr, "  Completed reorder of data\n");
	/* Change flags to reflect new order */
	gribfld->west   = 0;
	gribfld->north  = 0;
	gribfld->isweep = 0;
	gribfld->rsweep = 0;

	return TRUE;
	}

/**********************************************************************/
/** Take a grid with irregular spacing along the columns and make them regular
 *
 * @param[in]		*gribfld		DECODEDFIELD object with decoded GRIB data
 * @param[in,out]	**pgrib_data 	data interpolate
 * @return True if successful.
 **********************************************************************/
static LOGICAL	irregular2regular
	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	float 			*pgrib_data
	)
	{
	int         i,j;
	char        buf[255], file[255];
	FILE		*fp;
	int     	nx    = gribfld->mproj->grid.nx;
	int     	ny    = gribfld->mproj->grid.ny;
	double		olat  = gribfld->mproj_orig->definition.olat;
	double		ygrid = gribfld->mproj->grid.ygrid;
	double      *ir_list   = INITMEM(double, ny);
	double      *r_list    = INITMEM(double, ny);
	double      *ir_values = INITMEM(double, ny);
	double      *r_values  = INITMEM(double, ny);

	sprintf(file, "%d_%d.grid", (int)(olat*1000), ny); /* Assumes a pole to pole grid */
	(void) strcpy(buf, pathname(get_directory("ingest.gaussian_grid"), file));

	if ( (fp = fopen(buf, "r")) == NULL) 
		{
		/* PRINT ERROR MESSAGE */
		fprintf(stderr, "  Could not open file: %s\n", buf);
		return FALSE;
		}

	/* List arrays get populated only once */
	for (j = 0; j < ny; j++) 
		{ 
		fgets(buf, sizeof(buf), fp);
		ir_list[j] = atof(buf); 		/* list must be strictly increasing. */
		}
	fclose(fp);

	/* Create a regularly spaced grid */
	if (gribfld->north){
		for (j = 0; j < ny; j++) { r_list[j] =  (olat + j * ygrid); }
	}else{
		for (j = 0; j < ny; j++) { r_list[j] =  (olat - j * ygrid); }
	}

	/* Value lists get populated once per longitude */
	for (i=0; i<nx; i++)
		{
		for (j=0; j<ny; j++){ 
			if (gribfld->north) ir_values[j]      = (pgrib_data[i+j*nx]); 
			else				ir_values[ny-1-j] = (pgrib_data[i+j*nx]); 
			} /* Copy irregular grid to working space */
		(void)Twixt(ny,ir_list, ir_values, ny, r_list, r_values);
		for (j=0; j<ny; j++){ pgrib_data[i+j*nx] = (r_values[j]); }  /* Copy regular grid over input */
		}
	FREEMEM(ir_list);
	FREEMEM(r_list);
	FREEMEM(ir_values);
	FREEMEM(r_values);
	dprintf(stderr, "  Completed conversion to regular grid\n");
	return TRUE;
	}
/**********************************************************************/
/** Add another column or row if wrapped projection and wrap_i < 0.
 *
 *	Preferred FPA order is:
 *	-	Points of first row scan in +i (+x) direction
 *	-	Points of first column scan in -j (-y) direction
 *	-	Adjacent points in i (x) direction are consecutive
 *	-	Adjacent rows in scan in the same direction
 *
 * @param[in]		*gribfld		DECODEDFIELD object with decoded GRIB data
 * @param[in,out]	**pgrib_data 	data to wrap
 * @return True if successful.
 **********************************************************************/
static void	wrap_data
	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	float 			**pgrib_data
	)
	{
	static float   	*fld_out=NullFloat, *fld_in; 
	static MAP_PROJ new_mproj;
	LOGICAL			wrap, wrap_x;
	int				wrap_i;
	int     		jj, ii, nx, ny;
	MAP_DEF			mdef = gribfld->mproj_orig->definition;
	GRID_DEF        grid = gribfld->mproj_orig->grid;
	PROJ_DEF        proj = gribfld->mproj_orig->projection;

	/* Do this only once for each GRIB field */
	if (gribfld->wrapped) return;
	gribfld->wrapped = TRUE;

	/* Check if projection wraps around globe */
	wrap = wrap_map_projection(gribfld->mproj, &wrap_x, &wrap_i);
	if ( !wrap ) return; /* Projection does not wrap stop now */

	/* Do we need to add an extra column? */
	if ( wrap_x && wrap_i < 0 )
		{
		grid.nx++;	/* increase number of columns */

		/* increase length by 1 column width */
		if ( grid.gridlen == 0 )
			mdef.xlen += grid.xgrid * grid.units;	
		else
			mdef.xlen += grid.gridlen * grid.units;
		/* Reset origin if necessary */
		if ( mdef.xorg != 0 ) mdef.xorg = mdef.xlen;

		/* Recalculate map projection */
		define_map_projection(&new_mproj, &proj, &mdef, &grid);
		}
	/* Do we need to add an extra row? */
	else if ( !wrap_x && wrap_i < 0 )
		{
		grid.ny++;	/* increase number of rows */

		/* increase height by 1 row width */
		if ( grid.gridlen == 0 )
			mdef.ylen += grid.ygrid * grid.units;
		else
			mdef.ylen += grid.gridlen * grid.units;

		/* Reset origin if necessary */
		if ( mdef.yorg != 0 ) mdef.yorg = mdef.ylen;

		/* Recalculate map projection */
		define_map_projection(&new_mproj, &proj, &mdef, &grid);
		}
	else
		return;	/* Grid already overlaps, no new row/column required */
	
	ny        = grid.ny;
	nx        = grid.nx;
	fld_out   = GETMEM(fld_out, float, (nx * ny) );
	fld_in    = *pgrib_data;

	if (wrap_x)	/* Add a new column */
		{
		for (jj = 0; jj < ny; jj++)
			{
			for (ii = 0; ii < nx-1; ii++)
				{
				fld_out[jj*nx + ii] = fld_in[jj*(nx-1) + ii];
				}
			fld_out[jj*nx + (nx-1) ] = fld_in[jj*(nx-1)];
			}
		}
	else /* !wrap_x  add a new row */
		{
		for (jj = 0; jj < ny-1; jj++)
			{
			for (ii = 0; ii < nx; ii++)
				{
				fld_out[jj*nx + ii] = fld_in[jj*nx + ii];
				}
			}
		for (ii = 0; ii < nx; ii++) /* copy first row to last row */
			{
			fld_out[(ny-1)*nx + ii] = fld_in[ii];
			}
		}
	*pgrib_data    = fld_out;
	gribfld->mproj = &new_mproj;
	return;
	}
