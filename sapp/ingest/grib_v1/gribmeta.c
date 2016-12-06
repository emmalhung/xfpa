/***********************************************************************
*                                                                      *
*    g r i b m e t a . c                                               *
*                                                                      *
*    Routines to convert decoded GRIB data to FPA Objects              *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#include "rgrib_edition1.h"
#include "gribmeta.h"

#include <math.h>
#include <stdio.h>

#ifdef DEBUG_GRIBMETA
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_GRIBMETA */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                  */
/*  ... these are defined in gribmeta.h */

/* Internal static functions */
static	LOGICAL	grib_data_mapproj(GRIBFIELD *, MAP_PROJ *);
static	int		grib_data_component_flag(GRIBFIELD *);
static	SURFACE	grib_data_to_surface(GRIBFIELD *, FLD_DESCRIPT *, float **);

/* Maximum latitude and longitude in GRIB data */
static	const	long int	MaxLatitude  = 90000;
static	const	long int	MaxLongitude = 360000;

/* Conversion factors for GRIB data */
static	const	float	GribToDegrees  = 1000.0;
static	const	float	GribToMeters   = 1000.0;
static	const	float	GribToPolePos  = 1000.0;

/* Default units for FPA map projections */
static	const	float	MetersPerUnit  = 1000.0;


/***********************************************************************
*                                                                      *
*    g r i b f i e l d _ m a p p r o j                                 *
*                                                                      *
*    Obtain a map projection which describes the data coverage of      *
*    given gribfield.                                                  *
*                                                                      *
***********************************************************************/

LOGICAL			gribfield_mapproj

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	MAP_PROJ		*mproj		/* pointer to map projection */
	)

	{

	/* Return FALSE if no GRIB data */
	if ( !gribfld ) return FALSE;

	/* Determine map projection from GRIB data */
	return grib_data_mapproj(gribfld, mproj);
	}

/***********************************************************************
*                                                                      *
*    g r i b f i e l d _ t o _ m e t a f i l e                         *
*    g r i b f i e l d _ t o _ s u r f a c e                           *
*                                                                      *
*    Convert the given GRIBFIELD to a SURFACE Object.                  *
*                                                                      *
*    Optionally build the SURFACE into a METAFILE Object which bears   *
*    the matching timestamp and projection information.                *
*                                                                      *
***********************************************************************/

METAFILE		gribfield_to_metafile

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units		/* GRIB field units label */
	)

	{
	MAP_PROJ	mproj;
	METAFILE	meta;
	SURFACE		sfc;

	/* Return Null METAFILE if no GRIB data */
	if ( !gribfld ) return NullMeta;

	/* Return Null METAFILE if no GRIB map projection */
	if ( !grib_data_mapproj(gribfld, &mproj) ) return NullMeta;

	/* Create SURFACE Object from GRIB Data */
	sfc = gribfield_to_surface(gribfld, fdesc, units);
	if ( !sfc ) return NullMeta;

	/* Create METAFILE Object to hold SURFACE Object */
	meta = create_metafile();
	(void) define_mf_tstamp(meta, fdesc->rtime, fdesc->vtime);
	(void) define_mf_projection(meta, &fdesc->mproj);

	/* Add the GRIB map projection as source projection for GRIB stitching */
	(void) add_mf_source_proj(meta, &mproj);

	/* Add SURFACE Object to METAFILE Object */
	(void) add_sfc_to_metafile(meta, "a",
			fdesc->edef->name, fdesc->ldef->name, sfc);

	/* Return METAFILE Object */
	return meta;
	}


SURFACE			gribfield_to_surface

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
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
	sfc = grib_data_to_surface(gribfld, fdesc, NullPtr(float **));
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
*    Convert the given GRIBFIELD component to a SURFACE Object.        *
*                                                                      *
*    Optionally build the SURFACE into a METAFILE Object which bears   *
*    the matching timestamp and projection information.                *
*                                                                      *
***********************************************************************/

METAFILE		gribfield_to_metafile_by_comp

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units,		/* GRIB field units label */
	COMPONENT		compin,		/* input component identifier */
	COMPONENT		compout		/* output component identifier */
	)

	{
	int						isrc;
	const COMP_INFO			*cinfo;
	MAP_PROJ				mproj;
	SURFACE					sfc;
	METAFILE				meta;

	/* Return Null METAFILE if no GRIB data */
	if ( !gribfld ) return NullMeta;

	/* Return Null METAFILE if no x/y component info for output field */
	if ( !xy_component_field(fdesc->edef->name) ) return NullMeta;

	/* Return Null METAFILE if no GRIB map projection */
	if ( !grib_data_mapproj(gribfld, &mproj) ) return NullMeta;

	/* Create SURFACE Object from GRIB Data */
	sfc = gribfield_to_surface_by_comp(gribfld, fdesc, units, compin, compout);
	if ( !sfc ) return NullMeta;

	/* Create METAFILE Object to hold SURFACE Object */
	meta = create_metafile();
	(void) define_mf_tstamp(meta, fdesc->rtime, fdesc->vtime);
	(void) define_mf_projection(meta, &fdesc->mproj);

	/* Add the GRIB map projection as source projection for GRIB stitching */
	(void) add_mf_source_proj(meta, &mproj);

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


SURFACE			gribfield_to_surface_by_comp

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	STRING			units,		/* GRIB field units label */
	COMPONENT		compin,		/* input component identifier */
	COMPONENT		compout		/* output component identifier */
	)

	{
	FpaConfigUnitStruct		*udef;
	MAP_PROJ				mproj;
	int						compflag;
	float					**coefs;
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
	if ( !grib_data_mapproj(gribfld, &mproj) ) return NullSfc;

	/* Determine component coefficients */
	compflag = grib_data_component_flag(gribfld);
	if ( compflag < 0 ) return NullSfc;
	if ( !grid_component_coefficients(compflag, &mproj, compin,
			&fdesc->mproj, compout, &coefs) ) return NullSfc;

	/* Determine SURFACE Object from GRIB data */
	sfc = grib_data_to_surface(gribfld, fdesc, coefs);

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
*    g r i b _ d a t a _ m a p p r o j                                 *
*                                                                      *
*    Obtain a map projection which describes the data coverage of      *
*    given GRIBFIELD grid definition block.                            *
*                                                                      *
*    This routine is based on Section 2 and Table 1 in the 1991 GRIB   *
*    Edition 1 document WMO Code FM 92-VIII Ext. entitled "The WMO     *
*    Format For The Storage Of Weather Product Information And The     *
*    Exchange Of Weather Product Messages In Gridded Binary Form"      *
*    editted by John D. Stackpole of the U.S. Department of Commerce,  *
*    NMC.                                                              *
*                                                                      *
*    The  resltn  definitions are from Table 7.                        *
*    The  scan_mode  definitions are from Table 8.                     *
*    The  origin/delta  definitions are from Table C.                  *
*                                                                      *
*    Note Edition 1 polar stereographic projections are true at the    *
*    60 degree latitude closest to the pole of projection.             *
*                                                                      *
***********************************************************************/

static	LOGICAL	grib_data_mapproj

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	MAP_PROJ		*mproj		/* constructed map projection */
	)

	{
	E1_Grid_description_data	*gdd;
	PROJ_DEF					proj;
	MAP_DEF						map;
	GRID_DEF					grid;
	LOGICAL						left, bottom;
	float						splat, splon, rot;
	float						plat, tlat, slat;
	int							nn;
	STRING						pbuf, mbuf;

	/* Set origins and deltas based on type of GRIB grid */
	gdd = &gribfld->Gdd;
	switch (gdd->dat_rep)
		{

		case LATLON_GRID:			/* LATITUDE-LONGITUDE */

			define_projection(&proj, ProjectLatLon, 0., 0., 0., 0., 0.);

			/* Error message if no increments given for Lat/Long grid */
			if ( !(0x80 & gdd->defn.reg_ll.resltn) )
				{
				(void) pr_error("Grib-Meta", " No increments given for Lat/Long grid\n");
				(void) pr_error(" ", "  La1 = %d  Lo1 = %d  La2 = %d  Lo2 = %d\n",
						gdd->defn.reg_ll.La1, gdd->defn.reg_ll.Lo1,
						gdd->defn.reg_ll.La2, gdd->defn.reg_ll.Lo2);
				(void) pr_error(" ", "  resltn = %d  Ni = %d  Nj = %d  Di = %d  Dj = %d\n",
						gdd->defn.reg_ll.resltn,
						gdd->defn.reg_ll.Ni, gdd->defn.reg_ll.Nj,
						gdd->defn.reg_ll.Di, gdd->defn.reg_ll.Dj);
				return FALSE;
				}

			/* Determine the location of the first grid point */
			if ( gdd->defn.reg_ll.scan_mode.west == 0 )
				left = (LOGICAL) (gribfld->Dii > 0);
			else
				left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.reg_ll.scan_mode.north == 0 )
				bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive East and positive North */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToDegrees;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToDegrees;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = 1.0;

			/* Set map definition */
			map.olat  = (float) gdd->defn.reg_ll.La1 / GribToDegrees;
			map.olon  = (float) gdd->defn.reg_ll.Lo1 / GribToDegrees;
			map.lref  = 0.0;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (left)?   0.0: map.xlen;
			map.yorg  = (bottom)? 0.0: map.ylen;
			map.units = 1.0;

			break;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */

			/* Set projection to north or south polar stereographic */
			/* >>> assummed to be true at 60N or 60S! <<< */
			if (gdd->defn.ps.proj_centre.pole == 0)
				{
				plat = 90.0;
				tlat = 60.0;
				}
			else
				{
				plat = -90.0;
				tlat = -60.0;
				}
			define_projection(&proj, ProjectPolarSt, plat, tlat, 0., 0., 0.);

			/* Determine the location of the first grid point */
			if ( gdd->defn.ps.scan_mode.west == 0 )
				left = (LOGICAL) (gribfld->Dii > 0);
			else
				left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.ps.scan_mode.north == 0 )
				bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive right and positive up */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToMeters;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToMeters;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = MetersPerUnit;

			/* Set map definition */
			/* Set origins from latitude/longitude of first location */
			/*  ... if latitude/longitude is given!                  */
			if ( labs(gdd->defn.ps.La1) <= MaxLatitude
				&& labs(gdd->defn.ps.Lo1) <= MaxLongitude )
				{
				map.olat = (float) gdd->defn.ps.La1 / GribToDegrees;
				map.olon = (float) gdd->defn.ps.Lo1 / GribToDegrees;
				}

			/* Otherwise, set origins from location of pole */
			else
				{
				MAP_PROJ	mp;
				POINT		origin;

				origin[X] = -((float) gdd->defn.ps.pole_i) * grid.xgrid
							/ GribToPolePos;
				origin[Y] = -((float) gdd->defn.ps.pole_j) * grid.ygrid
							/ GribToPolePos;
				if (gdd->defn.ps.proj_centre.pole == 0)
					map.olat = 90.0;
				else
					map.olat = -90.0;
				map.olon  = 0.0;
				map.lref  = (float) gdd->defn.ps.LoV / GribToDegrees;
				map.xlen  = grid.xgrid * (grid.nx - 1);
				map.ylen  = grid.ygrid * (grid.ny - 1);
				map.xorg  = (left)?   0.0: map.xlen;
				map.yorg  = (bottom)? 0.0: map.ylen;
				map.units = MetersPerUnit;

				define_map_projection(&mp, &proj, &map, &grid);
				pos_to_ll(&mp, origin, &map.olat, &map.olon);
				}

			/* Set reference longitude in degrees East, and */
			/*  parallel to the y axis                      */
			map.lref  = (float) gdd->defn.ps.LoV / GribToDegrees;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (left)?   0.0: map.xlen;
			map.yorg  = (bottom)? 0.0: map.ylen;
			map.units = MetersPerUnit;

			break;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */

			/* Check for unsuppoorted modes */
			if (gdd->defn.lambert.proj_centre.bipolar != 0)
				{
				/* Bi-polar not supported */
				(void) pr_error("Grib-Meta",
					" Bi-polar form of Lambert conformal grid not supported\n");
				return FALSE;
				}

			/* Set projection to N or S Lambert conformal with given */
			/* tangent latitude */
			if (gdd->defn.lambert.proj_centre.pole == 0)
				{
				plat = 90.0;
				/* Check if cone bends north */
				}
			else
				{
				plat = -90.0;
				/* Check if cone bends south */
				}
			tlat = (float) gdd->defn.lambert.Latin1 / GribToDegrees;
			slat = (float) gdd->defn.lambert.Latin2 / GribToDegrees;
			define_projection(&proj, ProjectLambertConf,
						tlat, slat, 0., 0., 0.);

			/* Determine the location of the first grid point */
			if ( gdd->defn.lambert.scan_mode.west == 0 )
				left = (LOGICAL) (gribfld->Dii > 0);
			else
				left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.lambert.scan_mode.north == 0 )
				bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive right and positive up */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToMeters;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToMeters;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = MetersPerUnit;

			/* Set map definition */
			/* Set origins from latitude/longitude of first location */
			/*  ... if latitude/longitude is given!                  */
			if ( labs(gdd->defn.lambert.La1) <= MaxLatitude
				&& labs(gdd->defn.lambert.Lo1) <= MaxLongitude )
				{
				map.olat = (float) gdd->defn.lambert.La1 / GribToDegrees;
				map.olon = (float) gdd->defn.lambert.Lo1 / GribToDegrees;
				}

			/* Otherwise, set origins from location of pole */
			else
				{
				/* Not supported */
				(void) pr_error("Grib-Meta",
					" Setting origin from pole location for Lambert conformal grid not supported\n");
				return FALSE;
				}

			/* Set reference longitude in degrees East, and */
			/*  parallel to the y axis                      */
			map.lref  = (float) gdd->defn.lambert.LoV / GribToDegrees;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (left)?   0.0: map.xlen;
			map.yorg  = (bottom)? 0.0: map.ylen;
			map.units = MetersPerUnit;

			break;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE GRID */

			/* Set rotated latitude-longitude projection */
			splat = (float) gdd->defn.rotate_ll.LaP / GribToDegrees;
			splon = (float) gdd->defn.rotate_ll.LoP / GribToDegrees;
			rot   = (float) gdd->defn.rotate_ll.AngR / GribToDegrees;

			define_projection(&proj, ProjectLatLonAng, splat, splon, rot,
								0., 0.);

			/* Error message if no increments given for rotated Lat/Long grid */
			if ( !(0x80 & gdd->defn.rotate_ll.resltn) )
				{
				(void) pr_error("Grib-Meta", " No increments given for rotated Lat/Long grid\n");
				(void) pr_error(" ", "  La1 = %d  Lo1 = %d  La2 = %d  Lo2 = %d\n",
						gdd->defn.rotate_ll.La1, gdd->defn.rotate_ll.Lo1,
						gdd->defn.rotate_ll.La2, gdd->defn.rotate_ll.Lo2);
				(void) pr_error(" ", "  resltn = %d  Ni = %d  Nj = %d  Di = %d  Dj = %d\n",
						gdd->defn.rotate_ll.resltn,
						gdd->defn.rotate_ll.Ni, gdd->defn.rotate_ll.Nj,
						gdd->defn.rotate_ll.Di, gdd->defn.rotate_ll.Dj);
				return FALSE;
				}

			/* Determine the location of the first grid point */
			if ( gdd->defn.rotate_ll.scan_mode.west == 0 )
				left = (LOGICAL) (gribfld->Dii > 0);
			else
				left = (LOGICAL) (gribfld->Dii < 0);
			if ( gdd->defn.rotate_ll.scan_mode.north == 0 )
				bottom = (LOGICAL) (gribfld->Djj < 0);
			else
				bottom = (LOGICAL) (gribfld->Djj > 0);

			/* Set grid definition */
			/* Increments ... positive East and positive North */
			grid.xgrid   = (float) abs(gribfld->Dii) / GribToDegrees;
			grid.ygrid   = (float) abs(gribfld->Djj) / GribToDegrees;
			/* Grid dimensions ... set from extracted data    */
			/*  (may include pole data and an extra meridian) */
			grid.nx      = gribfld->Nii;
			grid.ny      = gribfld->Njj;
			grid.gridlen = 0.0;	/* Forces use of independent grid lengths */
			grid.units   = 1.0;

			/* Set map definition wrt rotated latitude-longitude grid */
			map.olat  = (float) gdd->defn.rotate_ll.La1 / GribToDegrees;
			map.olon  = (float) gdd->defn.rotate_ll.Lo1 / GribToDegrees;
			map.lref  = 0.0;
			map.xlen  = grid.xgrid * (grid.nx - 1);
			map.ylen  = grid.ygrid * (grid.ny - 1);
			map.xorg  = (left)?   0.0: map.xlen;
			map.yorg  = (bottom)? 0.0: map.ylen;
			map.units = 1.0;

			break;

		default:

			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if (gdd->dat_rep == GRIBGridLabels[nn].ident)
					{
					(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if (nn >= NumGRIBGridLabels)
				{
				(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return FALSE;
		}

	/* One last step ... */
	/* This map projection may be compared with another map projection read */
	/*  from an FPA metafile (such as combining u/v component fields).      */
	/* Therefore, we will "write" and "re-read" the projection information  */
	/*  exactly as is done in "write_metafile()" and "read_metafile()" to   */
	/*  ensure that the map projections will compare exactly!               */
	pbuf = format_metafile_projection(&proj);
	mbuf = format_metafile_mapdef(&map, MaxDigits);
	if ( !parse_metafile_projection(pbuf, &proj) ) return FALSE;
	if ( !parse_metafile_mapdef(mbuf, &map) ) return FALSE;

	/* Now define the map projection */
	define_map_projection(mproj, &proj, &map, &grid);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    g r i b _ d a t a _ c o m p o n e n t _ f l a g                   *
*                                                                      *
*    Return flag for determining components of vector fields from      *
*    GRIB data.                                                        *
*                                                                      *
*    The component flag definitions are from Table 7.                  *
*                                                                      *
*    GRIB data expressed in terms of the East and North coordinate     *
*    system returns a component flag of 0.                             *
*    GRIB data expressed in terms of the usual x/y coordinates of      *
*    the GRIB map projection returns a component flag of 1.            *
*                                                                      *
***********************************************************************/

static int		grib_data_component_flag

	(
	GRIBFIELD		*gribfld	/* decoded GRIB data */
	)

	{
	E1_Grid_description_data	*gdd;
	int							nn;

	/* Set component flag based on type of GRIB grid */
	gdd = &gribfld->Gdd;
	switch (gdd->dat_rep)
		{

		case LATLON_GRID:			/* LATITUDE-LONGITUDE */

			/* Components set in terms of East and North coordinates */
			/* (Note that x and y are also East and North!)          */
			return 0;

		case PSTEREO_GRID:			/* POLAR STEREOGRAPHIC */

			/* Components set in terms of x and y coordinates */
			if ( 0x8 & gdd->defn.ps.compnt ) return 1;

			/* Components set in terms of East and North coordinates */
			else                             return 0;

		case LAMBERTC_GRID:			/* LAMBERT CONFORMAL */

			/* Components set in terms of x and y coordinates */
			if ( 0x8 & gdd->defn.lambert.compnt ) return 1;

			/* Components set in terms of East and North coordinates */
			else                                  return 0;

		case ROTATED_LATLON_GRID:	/* ROTATED LATITUDE-LONGITUDE GRID */

			/* Components set in terms of rotated x and y coordinates */
			if ( 0x8 & gdd->defn.rotate_ll.resltn) return 1;

			/* Components set in terms of East and North coordinates */
			else                                   return 0;

		default:

			/* Error message for all other grid types */
			for (nn=0; nn<NumGRIBGridLabels; nn++)
				{
				if (gdd->dat_rep == GRIBGridLabels[nn].ident)
					{
					(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d  %s\n",
							gdd->dat_rep, GRIBGridLabels[nn].label);
					break;
					}
				}
			if (nn >= NumGRIBGridLabels)
				{
				(void) pr_error("Grib-Meta", " Unsupported GRIB grid type: %d\n",
						gdd->dat_rep);
				}
			return -1;
		}
	}

/***********************************************************************
*                                                                      *
*    g r i b _ d a t a _ t o _ s u r f a c e                           *
*                                                                      *
*    Convert the given GRIBFIELD Object to a SURFACE Object.           *
*                                                                      *
*    The given GRIBFIELD is remapped to the grid points of a grid      *
*    defined by the output map projection.                             *
*                                                                      *
*    Locations outside the boundary of the supplied GRIBFIELD are      *
*    set to the value at the nearest boundary.                         *
*                                                                      *
*    Note that all distances are given in km, all latitudes in         *
*    degrees North, all longitudes in degrees East, and that the       *
*    reference longitude is parallel to the y axis.                    *
*                                                                      *
***********************************************************************/

static SURFACE	grib_data_to_surface

	(
	GRIBFIELD		*gribfld,	/* decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to output field descriptor */
	float			**coefs		/* component coefficients    */
								/*  (Null if no components!) */
	)

	{
	int			iix, iiy;
	MAP_PROJ	mproj;
	GRID_DEF	gdef, odef;
	GRID		gridd;
	SURFACE		sfc;

	/* Return Null SURFACE if no GRIB data */
	if ( !gribfld ) return NullSfc;

	/* Return Null SURFACE if no GRIB map projection */
	if ( !grib_data_mapproj(gribfld, &mproj) ) return NullSfc;

	/* Move array of GRIB data into GRID Object */
	gdef = mproj.grid;
	(void) init_grid(&gridd);
	(void) define_grid(&gridd, gdef.nx, gdef.ny, ZeroPoint, 0.0, gdef.gridlen,
			gribfld->PData, gdef.nx);

	/* Remap the GRIB data to the output map projection */
	(void) remap_grid(&gridd, &mproj, &fdesc->mproj);

	/* Adjust grid values by component coefficients (if required) */
	odef = fdesc->mproj.grid;
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
