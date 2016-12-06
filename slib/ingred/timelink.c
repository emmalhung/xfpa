/***********************************************************************
*                                                                      *
*     t i m e l i n k . c                                              *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all edit and display functions for the time linker.      *
*                                                                      *
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

#include "ingred_private.h"

#undef DEBUG_INTERP
#undef DEBUG_INTERP_POINTS
#undef DEBUG_PANELS

/* >>> not sure what is happening with these! <<< */
#define ltime LHtime
#define rtime RHtime

#define LinkHours   "hours"
#define LinkMinutes "minutes"

/***********************************************************************
*                                                                      *
*     s h o w _ t i m e l i n k                                        *
*     h i d e _ t i m e l i n k                                        *
*     p r e s e n t _ t i m e l i n k                                  *
*                                                                      *
***********************************************************************/

LOGICAL	show_timelink(void)

	{

	/* Set display state on */
	define_dn_vis(DnLink, TRUE);
	LinkShown = TRUE;

	(void) extract_special_tags();
	(void) extract_links();
	(void) extract_unlinked();
	return present_all();
	}

/**********************************************************************/

LOGICAL	hide_timelink(void)

	{
	if (!LinkShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnLink, FALSE);
	LinkShown = FALSE;

	(void) release_unlinked();
	(void) release_links();
	(void) release_special_tags();
	(void) remove_extrap();
	(void) release_ambiguous_nodes();
	(void) empty_temp();
	return present_all();
	}

/**********************************************************************/

LOGICAL	present_timelink

	(
	LOGICAL	all
	)

	{
	if (!LinkShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnLink);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ l i n k _ r e s o l u t i o n                      *
*                                                                      *
***********************************************************************/

LOGICAL	define_link_resolution(void)

	{
	float	s;

	/* Define distance in terms of km/hr wrt average size of map */
	/* Note that internal time units are in minutes!             */
	s = (MapProj->definition.xlen + MapProj->definition.ylen) / 2.0;
	s *= MapProj->definition.units / 1000.0;
	s /= 60.0;

	LinkRes = s/750;

	/* Set the parameters to use for QuasiLinear_Tween() interpolation */
	set_quasilinear_mode(QL_Fixed, LinkRes, LinkRes*2.5);
	/* set_quasilinear_mode(QL_Proportional, 0.01, 0.02) */

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e a d _ l i n k s                                              *
*     s a v e _ l i n k s                                              *
*                                                                      *
***********************************************************************/

LOGICAL	read_links(void)

	{
	FILE	*fp;
	STRING	file;
	STRING	cmd;
	int		rev1, rev2;
	char	s1[80], s2[80], s3[80];
	LOGICAL	lhours, consistent, fieldOK, sectionOK, status;
	int		nvt, nmaster, ndfld, ivt, last, itime, lnum;
	int		mplus, smplus, emplus, cmplus, mfirst, att, typ, mem;
	LNODE	node, *nodes;
	float	x, y;
	POINT	pos;
	DFLIST	*dfld;

	if (LinkRead) return FALSE;
	(void) clear_links(FALSE);

	/* Make sure the link file is present */
	file = depiction_link_file();
	(void) pr_status("Timelink", "Reading Links file: \"%s\"\n", file);
	if (!find_file(file))
		{
		LinkRead = TRUE;
		if (NumTime <= 0) return TRUE;
		(void) pr_error("Timelink", "Cannot access link file %s\n", file);
		return FALSE;
		}

	/* Open the link file */
	fp = fopen(file, "r");
	if (!fp)
		{
		LinkRead = TRUE;
		if (NumTime <= 0) return TRUE;
		(void) pr_error("Timelink", "Cannot open link file %s\n", file);
		return FALSE;
		}

	/* Read the link file to get information from the revision line */
	rev1 = 1;
	rev2 = 0;
	lhours = TRUE;
	while (getvalidline(fp, Msg, sizeof(Msg), "*#"))
		{
		cmd = string_arg(Msg);
		if (blank(cmd)) continue;

		/* Read the revision information */
		else if (same(cmd, "rev"))
			{

			/* Parse the revision information */
			(void) pr_status("Timelink", "Links file revision: \"%s\"\n", Msg);
			if (sscanf(Msg, "%d.%d-%s", &rev1, &rev2, s1) != 3)
				{
				(void) pr_error("Timelink",
					"Error in Links file revision format: \"%s\"\n", Msg);
				break;
				}

			/* Set features according to the revision number */
			if      (same(s1, LinkHours))   lhours = TRUE;
			else if (same(s1, LinkMinutes)) lhours = FALSE;
			else
				{
				(void) pr_error("Timelink",
					"Error in Links file revision type: \"%s\"\n", s1);
				(void) pr_error("Timelink",
					"  Should be one of: \"%s\"  \"%s\"\n",
					LinkHours, LinkMinutes);
				break;
				}
			}
		}

	/* Rewind the link file */
	(void) rewind(fp);

	/* Re-read the link file to get the valid time information */
	nvt    = first_depict_time();
	mfirst = (nvt >= 0)? TimeList[nvt].mplus: 0;
	consistent = TRUE;
	sectionOK  = TRUE;
	while (getvalidline(fp, Msg, sizeof(Msg), "*#"))
		{

		/* Skip comment lines */
		cmd = string_arg(Msg);
		if (blank(cmd)) continue;

		/* Problem encountered in link file */
		if (!sectionOK)
			{
			consistent = FALSE;
			break;
			}

		/* Input a valid time */
		else if (same(cmd, "valid"))
			{
			sectionOK = FALSE;
			if (nvt < 0)        break;
			if (nvt >= NumTime) break;
			strcpy_arg(s1, Msg, &status);
			if (!matching_tstamps(s1, TimeList[nvt].jtime)) break;
			nvt = next_depict_time(nvt);
			if (nvt < 0) nvt = NumTime;
			sectionOK = TRUE;
			}
		}

	/* Error in valid times */
	if (!consistent || !sectionOK || nvt != NumTime)
		{
		LinkRead = TRUE;
		(void) fclose(fp);
		(void) pr_error("Timelink",
				"Valid time mismatch in link file %s\n", file);
		return FALSE;
		}

	/* Rewind the link file */
	(void) rewind(fp);

	/* Re-read the link file to get the link information */
	nmaster = 0;
	ndfld   = 0;
	dfld    = NULL;
	nodes   = NullLnodeList;
	consistent = TRUE;
	fieldOK    = TRUE;
	sectionOK  = TRUE;
	while (getvalidline(fp, Msg, sizeof(Msg), "*#"))
		{

		/* Skip comment lines */
		cmd = string_arg(Msg);
		if (blank(cmd)) continue;

		/* Problem encountered in link file */
		if (!sectionOK)
			{
			consistent = FALSE;
			break;
			}

		/* Input a master link */
		else if (same(cmd, "master"))
			{
			fieldOK = FALSE;
			if (nmaster >= NumMaster)
				{
				dfld = NULL;
				break;
				}
			strcpy_arg(s1, Msg, &status);	if (!status) break;
			dfld = find_master_dfield(s1);
			if (!dfld) break;
			dfld->there = TRUE;
			nmaster++;
			fieldOK = TRUE;
			}

		/* Input a field */
		else if (same(cmd, "field"))
			{
			fieldOK = FALSE;
			if (ndfld >= NumDfld)
				{
				dfld = NULL;
				break;
				}
			strcpy_arg(s1, Msg, &status);	if (!status) break;
			strcpy_arg(s2, Msg, &status);	if (!status) break;
			strcpy_arg(s3, Msg, &status);	if (!status) break;

			/* Field not found ... so skip any link information */
			dfld = find_dfield(s1, s2);
			if (!dfld) continue;
			if (!same(s3, dfld->entity)) continue;
			ndfld++;
			fieldOK = TRUE;
			}

		/* Link to another field */
		else if (same(cmd, "link_to"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)         break;
			if (!dfld->dolink) break;
			strcpy_arg(s1, Msg, &status);	if (!status) break;
			strcpy_arg(s2, Msg, &status);	if (!status) (void) strcpy(s2, "");
			(void) borrow_links(dfld, s1, s2, FALSE);
			sectionOK = TRUE;
			}

		/* Input a link chain */
		else if (same(cmd, "link"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)         break;
			if (!dfld->dolink) break;
			mplus  = int_arg(Msg, &status);	if (!status) break;
			smplus = (lhours) ? mplus*60 : mplus;
			mplus  = int_arg(Msg, &status);	if (!status) break;
			emplus = (lhours) ? mplus*60 : mplus;
			lnum   = NumTime;
			nodes  = INITMEM(LNODE, lnum);
			for (ivt=0; ivt<NumTime; ivt++)
				{
				nodes[ivt] = create_lnode(TimeList[ivt].mplus);
				if (!TimeList[ivt].depict) continue;

				strcpy_arg(s2, Msg, &status);	if (!status) break;
				if (same(s2, "-"))                           continue;
				else if (same(s2, "()"))                     continue;
				else if (same(s2, "{}"))                     continue;
				else if (sscanf(s2, "(%f,%f)", &x, &y) == 2)
					{
					set_point(pos, x, y);
					define_lnode_type(nodes[ivt], TRUE, FALSE, LchainNode);
					define_lnode_node(nodes[ivt], pos);
					}
				else if (sscanf(s2, "{%f,%f}", &x, &y) == 2)
					{
					set_point(pos, x, y);
					define_lnode_type(nodes[ivt], TRUE, TRUE, LchainNode);
					define_lnode_node(nodes[ivt], pos);
					}
				else
					{
					(void) pr_error("Timelink",
						"Error in \"link\" format \"%s\"\n", s2);
					break;
					}
				}
			if (ivt < NumTime)
				{
				for (ivt=0; ivt<NumTime; ivt++)
					{
					nodes[ivt] = destroy_lnode(nodes[ivt]);
					}
				FREEMEM(nodes);
				lnum = 0;
				(void) pr_error("Timelink",
						"Error in \"link\" times\n");
				break;
				}
			last = dfld->nchain++;
			dfld->chains = GETMEM(dfld->chains, LCHAIN, dfld->nchain);
			dfld->chains[last] = create_lchain();
			define_lchain_reference_time(dfld->chains[last], Stime);
			define_lchain_interp_delta(dfld->chains[last], DTween);
			dfld->chains[last]->lnum     = lnum;
			dfld->chains[last]->nodes    = nodes;
			dfld->chains[last]->splus    = smplus + mfirst;
			dfld->chains[last]->eplus    = emplus + mfirst;
			dfld->chains[last]->dointerp = TRUE;
			define_lchain_default_attribs(dfld->chains[last]);
			nodes = NullLnodeList;
			sectionOK = TRUE;
			}

		/* Input node attach information */
		else if (same(cmd, "attach"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)         break;
			if (!dfld->dolink) break;
			if (dfld->nchain < 1) break;
			last = dfld->nchain-1;
			for (ivt=0; ivt<NumTime; ivt++)
				{
				if (!TimeList[ivt].depict) continue;

				strcpy_arg(s2, Msg, &status);	if (!status) break;
				if (same(s2, "-"))                           continue;
				else if (same(s2, "()"))                     continue;
				else if (same(s2, "{}"))                     continue;
				else if (sscanf(s2, "(%d,%d,%d)", &att, &typ, &mem) == 3)
					{
					define_lnode_attach(dfld->chains[last]->nodes[ivt],
																att, typ, mem);
					continue;
					}
				else
					{
					(void) pr_error("Timelink",
						"Error in \"link\" format \"%s\"\n", s2);
					}
				}
			sectionOK = TRUE;
			}

		/* Input a control node */
		else if (same(cmd, "control"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)         break;
			if (!dfld->dolink) break;
			if (dfld->nchain < 1) break;
			mplus  = int_arg(Msg, &status);	if (!status) break;
			cmplus = (lhours) ? mplus*60 : mplus;

			strcpy_arg(s1, Msg, &status);	if (!status) break;
			if (sscanf(s1, "(%f,%f)", &x, &y) == 2)
				{
				set_point(pos, x, y);
				node = create_lnode(cmplus + mfirst);
				define_lnode_type(node, TRUE, FALSE, LchainControl);
				define_lnode_node(node, pos);
				}
			else if (sscanf(s1, "{%f,%f}", &x, &y) == 2)
				{
				set_point(pos, x, y);
				node = create_lnode(cmplus + mfirst);
				define_lnode_type(node, TRUE, TRUE, LchainControl);
				define_lnode_node(node, pos);
				}
			else break;

			last = dfld->nchain-1;
			(void) add_lchain_lnode(dfld->chains[last], node);
			sectionOK = TRUE;
			}

		/* Has field been sufficiently linked ? */
		/* Will be tested later */
		else if (same(cmd, "linked"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)                                      break;
			if (!dfld->dolink && !tdep_special(dfld->tdep)) break;
			dfld->linked = TRUE;
			sectionOK = TRUE;
			}

		/* Has field been interpolated ? */
		/* Will be tested later */
		else if (same(cmd, "interp"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)                                      break;
			if (!dfld->dolink && !tdep_special(dfld->tdep)) break;
			dfld->interp = TRUE;
			sectionOK = TRUE;
			}

		/* Have field and labels been interpolated ? */
		/* Will be tested later */
		else if (same(cmd, "interp_full"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)                                      break;
			if (!dfld->dolink && !tdep_special(dfld->tdep)) break;
			dfld->interp = TRUE;
			dfld->intlab = TRUE;
			sectionOK = TRUE;
			}

		/* Has field been saved ? */
		/* Will be tested later */
		else if (same(cmd, "saved"))
			{
			if (!fieldOK) continue;
			sectionOK = FALSE;
			if (!dfld)                                      break;
			if (!dfld->dolink && !tdep_special(dfld->tdep)) break;
			dfld->saved = TRUE;
			sectionOK = TRUE;
			}
		}

	/* Close the link file */
	(void) fclose(fp);

	/* Check all link chains */
	(void) verify_links(TRUE);

	if (!consistent || !sectionOK)
		{
		(void) clear_links(FALSE);
		(void) save_links();
		}
	(void) revise_links();

	LinkRead = TRUE;
	if (!consistent || !sectionOK) return FALSE;
	else                           return TRUE;
	}

/**********************************************************************/

LOGICAL	save_links(void)

	{
	FILE		*fp, *fopen();
	STRING		file, fname;
	int			ivt, ictl, nvt, idfld, ichain, smplus, emplus, mfirst, inode;
	DFLIST		*dfld;
	LCHAIN		chain;
	LNODE		node;
	char		mident[FILE_IDENT_LEN];
	METAFILE	tmeta;
	SET			tset;

	if (!LinkRead) return FALSE;
	if (ViewOnly)  return TRUE;

#	ifdef DEBUG
	(void) printf("         Saving links\n");
#	endif /* DEBUG */

	/* Make sure the file is there (create if necessary) */
	file = depiction_link_file();
	if (!create_file(file, NULL))
		{
		(void) pr_error("Timelink", "Cannot access link file %s\n", file);
		return FALSE;
		}

	/* Open the link file */
	fp = fopen(file, "w");
	if (!fp)
		{
		(void) pr_error("Timelink", "Cannot open link file %s\n", file);
		return FALSE;
		}

	/* Write a header */
	(void) fprintf(fp, "* Link File\n");
	(void) fprintf(fp, "\n");

	/* Write out revision information */
	if (minutes_in_depictions())
		(void) fprintf(fp, "rev 2.1-minutes\n");
	else
		(void) fprintf(fp, "rev 2.1-hours\n");
	(void) fprintf(fp, "\n");

	/* Finish here if no charts */
	if (NumTime <= 0)
		{
		(void) fprintf(fp, "* No Charts\n");
		(void) fclose(fp);
		return TRUE;
		}

	/* Write out the list of valid depiction times in the sequence */
	for (ivt=0; ivt<NumTime; ivt++)
		{
		if (!TimeList[ivt].depict) continue;
		if (minutes_in_depictions())
			(void) fprintf(fp, "valid %s\n", TimeList[ivt].jtime);
		else
			(void) fprintf(fp, "valid %s\n",
					tstamp_to_hours(TimeList[ivt].jtime, TRUE, NullInt));
		}
	nvt    = first_depict_time();
	mfirst = (nvt >= 0)? TimeList[nvt].mplus: 0;

	/* Write out master link information */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;

		if (!dfld->there) continue;
		(void) fprintf(fp, "\n");
		if (same(dfld->group, "GLOBAL"))
				(void) fprintf(fp, "master GLOBAL\n");
		else	(void) fprintf(fp, "master \"%s\"\n", dfld->group);

		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			smplus = chain->splus - mfirst;
			emplus = chain->eplus - mfirst;
			if (minutes_in_depictions())
				(void) fprintf(fp, "  link %d %d", smplus, emplus);
			else
				(void) fprintf(fp, "  link %d %d", smplus/60, emplus/60);
			for (ivt=0; ivt<NumTime; ivt++)
				{
				if (!TimeList[ivt].depict) continue;
				/* >>>>> replace this ...
				node = chain->nodes[ivt];
				... with this <<<<< */
				inode = which_lchain_node(chain, LchainNode, TimeList[ivt].mplus);
				if (inode < 0)
					{
					(void) fprintf(fp, " -");
					}
				else
					{
					node = chain->nodes[inode];
					if (!node->there)
						{
						(void) fprintf(fp, " -");
						}
					else if (!node->guess)
						{
						(void) fprintf(fp, " (%d,%d)", NINT(node->node[X]),
												NINT(node->node[Y]));
						}
					else
						{
						(void) fprintf(fp, " {%d,%d}", NINT(node->node[X]),
												NINT(node->node[Y]));
						}
					}
				}
			(void) fprintf(fp, "\n");
			}
		if (dfld->linked) (void) fprintf(fp, "  linked\n");
		}

	/* Write out field information and link chains for each field */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;

		if (!dfld->there)                               continue;
		(void) fprintf(fp, "\n");
		(void) fprintf(fp, "field \"%s\" \"%s\" \"%s\"\n",
				dfld->element, dfld->level, dfld->entity);

		if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;

		if (dfld->linkto)
			{
			if (same(dfld->linkto->element, "MASTER_LINK"))
				{
				(void) fprintf(fp, "  link_to MASTER\n");
				}
			else
				{
				(void) fprintf(fp, "  link_to \"%s\" \"%s\"\n",
						dfld->linkto->element, dfld->linkto->level);
				}
			}

		else
			{
			for (ichain=0; ichain<dfld->nchain; ichain++)
				{
				chain = dfld->chains[ichain];
				smplus = chain->splus - mfirst;
				emplus = chain->eplus - mfirst;
				if (minutes_in_depictions())
					(void) fprintf(fp, "  link %d %d", smplus, emplus);
				else
					(void) fprintf(fp, "  link %d %d",
							smplus/60, emplus/60);
				for (ivt=0; ivt<NumTime; ivt++)
					{
					if (!TimeList[ivt].depict) continue;
					/* >>>>> replace this ...
					node = chain->nodes[ivt];
					... with this <<<<< */
					inode = which_lchain_node(chain, LchainNode,
															TimeList[ivt].mplus);
					if (inode < 0)
						{
						(void) fprintf(fp, " -");
						}
					else
						{
						node = chain->nodes[inode];
						if (!dfld->frames[ivt].meta) node->there = FALSE;
						if (!node->there)
							{
							(void) fprintf(fp, " -");
							}
						else if (node->guess)
							{
							(void) fprintf(fp, " {%d,%d}",
										NINT(node->node[X]), NINT(node->node[Y]));
							}
						else
							{
							(void) fprintf(fp, " (%d,%d)",
										NINT(node->node[X]), NINT(node->node[Y]));
							}
						}
					}
				(void) fprintf(fp, "\n");

				/* Write out node attach information for object type fields */
				if (dfld->editor == FpaC_DISCRETE ||
		    			dfld->editor == FpaC_WIND ||
		    			dfld->editor == FpaC_LINE)
					{
					(void) fprintf(fp, "    attach");
					for (ivt=0; ivt<NumTime; ivt++)
						{
						if (!TimeList[ivt].depict) continue;
						/* >>>>> replace this ...
						node = chain->nodes[ivt];
						... with this <<<<< */
						inode = which_lchain_node(chain, LchainNode,
															TimeList[ivt].mplus);
						if (inode < 0)
							{
							(void) fprintf(fp, " -");
							}
						else
							{
							node = chain->nodes[inode];
							if (!node->there || node->guess)
								{
								(void) fprintf(fp, " -");
								}
							else
								{
								(void) fprintf(fp, " (%d,%d,%d)",
										node->attach, node->mtype, node->imem);
								}
							}
						}
					(void) fprintf(fp, "\n");
					}

				/* Write out any control nodes */
				for (inode=0; inode<chain->lnum; inode++)
					{
					node = chain->nodes[inode];
					if (!node->there)                 continue;
					if (node->ltype != LchainControl) continue;
					smplus = node->mplus - mfirst;
					if (minutes_in_depictions())
						(void) fprintf(fp, "    control %d", smplus);
					else
						(void) fprintf(fp, "    control %d", smplus/60);
					if (node->guess)
						{
						(void) fprintf(fp, " {%d,%d}",
									NINT(node->node[X]), NINT(node->node[Y]));
						}
					else
						{
						(void) fprintf(fp, " (%d,%d)",
									NINT(node->node[X]), NINT(node->node[Y]));
						}
					(void) fprintf(fp, "\n");
					}
				}
			}
		if (dfld->linked) (void) fprintf(fp, "  linked\n");
		if (dfld->interp)
			{
			if (dfld->intlab) (void) fprintf(fp, "  interp_full\n");
			else              (void) fprintf(fp, "  interp\n");
			}
		if (dfld->saved)  (void) fprintf(fp, "  saved\n");
		}

	/* Close the links file */
	(void) fclose(fp);

	/* Output named link files for master link information */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;

		if (!dfld->there) continue;

		/* Construct a filename for the master link group */
		(void) strcpy(mident, "master");
		(void) strcat(mident, FpaFidentDelimiter);
		(void) strcat(mident, dfld->group);
		(void) strcat(mident, FpaFidentDelimiter);
		(void) strcat(mident, FpaFile_Links);
		file = named_depiction_file(mident);
		if (blank(file)) continue;

		/* Create a metafile to hold link chains */
		tmeta = create_metafile();
		define_mf_projection(tmeta, MapProj);

		/* Add the link chains to the metafile */
		tset = create_set("lchain");
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			add_item_to_set(tset, copy_item("lchain", (ITEM) chain));
			}
		add_set_to_metafile(tmeta, "l", "master", dfld->group, tset);

		/* Output the metafile */
		write_metafile(file, tmeta, MaxDigits);
		tmeta = destroy_metafile(tmeta);
		}

	/* Output named link file for each field */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;

		if (!dfld->there)  continue;
		if (!dfld->dolink) continue;

		/* Daily and static fields will not have link chains */
		if (tdep_special(dfld->tdep)) continue;

		/* Construct a filename for each field */
		fname = construct_link_identifier(dfld->element, dfld->level);
		file = named_depiction_file(fname);
		if (blank(file)) continue;

		/* Create a metafile to hold link chains */
		tmeta = create_metafile();
		define_mf_projection(tmeta, MapProj);

		/* Add the link chains to the metafile */
		tset = create_set("lchain");
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			add_item_to_set(tset, copy_item("lchain", (ITEM) chain));
			}
		add_set_to_metafile(tmeta, "l", dfld->element, dfld->level, tset);

		/* Output the metafile */
		write_metafile(file, tmeta, MaxDigits);
		tmeta = destroy_metafile(tmeta);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c l e a r _ l i n k s                                            *
*     c l e a r _ d f i e l d _ l i n k s                              *
*     i n t e r p _ l i n k s                                          *
*     i n t e r p _ d f i e l d _ l i n k s                            *
*     i n t e r p _ l i n k _ n o d e                                  *
*     c o p y _ d f i e l d _ l i n k s                                *
*     r e v i s e _ l i n k s                                          *
*     r e v i s e _ d e p e n d e n t _ l i n k s                      *
*                                                                      *
***********************************************************************/

LOGICAL	clear_links

	(
	LOGICAL	report
	)

	{
	int		idfld;
	DFLIST	*dfld;

#	ifdef DEBUG
	if (LinkRead) (void) printf("         Clearing links\n");
	else          (void) printf("         Initialising links\n");
#	endif /* DEBUG */

	/* Empty the links in each master link */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		(void) clear_dfield_links(dfld, report);
		}

	/* Empty the links in each field */
	for (idfld=0; idfld<NumDfld; idfld++)
	    {
	    dfld = DfldList + idfld;
		(void) clear_dfield_links(dfld, report);
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	clear_dfield_links

	(
	DFLIST	*dfld,
	LOGICAL	report
	)

	{
	int		ichain;

	if (!dfld)             return FALSE;
	if (dfld->nchain <= 0) return TRUE;

	for (ichain=0; ichain<dfld->nchain; ichain++)
		{
		dfld->chains[ichain] = destroy_lchain(dfld->chains[ichain]);
		}
	FREEMEM(dfld->chains);
	dfld->nchain = 0;

	(void) verify_dfield_links(dfld, report);
	return TRUE;
	}

/**********************************************************************/

LOGICAL	interp_links

	(
	int		ivt,
	LOGICAL	report
	)

	{
	int		idfld;
	DFLIST	*dfld;

	/* Interpolate links in each master link */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		(void) interp_dfield_links(dfld, ivt, report);
		}

	/* Interpolate links in each field */
	for (idfld=0; idfld<NumDfld; idfld++)
	    {
	    dfld = DfldList + idfld;
		(void) interp_dfield_links(dfld, ivt, report);
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	interp_dfield_links

	(
	DFLIST	*dfld,
	int		ivt,
	LOGICAL	report
	)

	{
	int		mplus, ichain;
	LCHAIN	chain;
	LNODE	inode, lnode, rnode;
	int		ii, il, in, lvt, rvt;
	POINT	pos;
	FIELD	fld;
	SET		set;
	LOGICAL	changed, master;

	if (!dfld)             return FALSE;
	if (dfld->nchain <= 0) return TRUE;
	if (ivt < 0)           return FALSE;
	if (ivt >= NumTime)    return FALSE;
	if (dfld->linkto)      return TRUE;

	mplus = TimeList[ivt].mplus;

	master = same(dfld->element, "MASTER_LINK");

	/* Make sure there is a field */
	if (!master)
		{
		if (!dfld->frames)             return TRUE;
		if (!dfld->frames[ivt].meta)   return TRUE;
		if (dfld->editor == FpaC_DISCRETE  ||
		    dfld->editor == FpaC_WIND      ||
		    dfld->editor == FpaC_LINE      ||
		    dfld->editor == FpaC_SCATTERED ||
		    dfld->editor == FpaC_LCHAIN)
			{
			fld = dfld->fields[ivt];
			if (!fld)                   return FALSE;
			if (fld->ftype != FtypeSet) return FALSE;
			set = fld->data.set;
			if (!set)                   return FALSE;
			/*
			if (set->num <= 0)          return FALSE;
			*/
			/* >>> might need to split <<< */
			}
		}

	changed = FALSE;
	for (ichain=0; ichain<dfld->nchain; ichain++)
		{
		chain = dfld->chains[ichain];
		/* >>>>> replace this ...
		inode = chain->nodes[ivt];
		... with this <<<<< */

		/* Skip this chain if beyond limits of link chain */
		if (mplus < chain->splus || mplus > chain->eplus)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Timelink",
				"[interp_dfield_links] - Field: %s %s\n",
				dfld->element, dfld->level);
			pr_diag("Timelink",
				"[interp_dfield_links] -  Time: %d outside lchain times: %d to %d\n",
				mplus, chain->splus, chain->eplus);
#			endif /* DEBUG_INTERP */

			continue;
			}

		/* Look for an existing link chain node */
		il = which_lchain_node(chain, LchainNode, mplus);
		if (il >= 0 && chain->nodes[il]->there)
			{
			pr_diag("Timelink",
				"[interp_dfield_links] - Field: %s %s\n",
				dfld->element, dfld->level);
			pr_diag("Timelink",
				"[interp_dfield_links] -  Should this one (%d) become a guess?\n",
				mplus);
			continue;
			}

		/* Found an empty node that will be defined later */
		else if (il >= 0)
			{
			inode = chain->nodes[il];
			}

		/* Check for a matching (active) control node to promote */
		else
			{
			il = which_lchain_node(chain, LchainControl, mplus);
			if (il >= 0 && chain->nodes[il]->there)
				{

#				ifdef DEBUG_INTERP
				pr_diag("Timelink",
					"[interp_dfield_links] - Field: %s %s\n",
					dfld->element, dfld->level);
				pr_diag("Timelink",
					"[interp_dfield_links] -  Promoting control node: %d at time: %d\n",
					il, mplus);
#				endif /* DEBUG_INTERP */

				inode = chain->nodes[il];
				define_lnode_type(inode, TRUE, TRUE, LchainNode);
				define_lnode_attach(inode, -1, 0, -1);
				if (!inside_map_def(&MapProj->definition, inode->node))
					{
					(void) clip_to_map_def(&MapProj->definition, inode->node, pos);
					copy_point(inode->node, pos);
					}
				if (mplus < chain->splus) chain->splus = mplus;
				if (mplus > chain->eplus) chain->eplus = mplus;
				define_lchain_default_attribs(chain);
				changed = TRUE;
				continue;
				}

			/* Found an empty node that will be defined later */
			else if (il >= 0)
				{
				inode = chain->nodes[il];
				}

			/* Create a new link node at this time (if required) */
			else
				{
				inode = create_lnode(mplus);
				(void) add_lchain_lnode(chain, inode);

#				ifdef DEBUG_INTERP
				pr_diag("Timelink",
					"[interp_dfield_links] - Field: %s %s\n",
					dfld->element, dfld->level);
				pr_diag("Timelink",
					"[interp_dfield_links] -  Creating new link node for time: %d\n",
					mplus);
#				endif /* DEBUG_INTERP */
				}
			}

		/* Check for a matching interp node to promote */
		in = which_lchain_node(chain, LchainInterp, mplus);
		if (in >= 0 && chain->interps[in]->there)
			{

#			ifdef DEBUG_INTERP
			pr_diag("Timelink",
				"[interp_dfield_links] - Field: %s %s\n",
				dfld->element, dfld->level);
			pr_diag("Timelink",
				"[interp_dfield_links] -  Promoting interp node: %d at time: %d\n",
				in, mplus);
#			endif /* DEBUG_INTERP */

			define_lnode_type(inode, TRUE, TRUE, LchainNode);
			define_lnode_node(inode, chain->interps[in]->node);
			define_lnode_attach(inode, -1, 0, -1);
			if (!inside_map_def(&MapProj->definition, inode->node))
				{
				(void) clip_to_map_def(&MapProj->definition, inode->node, pos);
				copy_point(inode->node, pos);
				}
			chain->interps[in]->depict = TRUE;
			if (mplus < chain->splus) chain->splus = mplus;
			if (mplus > chain->eplus) chain->eplus = mplus;
			define_lchain_default_attribs(chain);
			changed = TRUE;
			continue;
			}

		/* Otherwise ... try to interpolate from existing nodes  */
		/* First make sure there is a node both before and after */
		if (ivt <= 0) continue;
		for (lvt=ivt-1; lvt>=0; lvt--)
			{
			/* >>>>> replace this ...
			lnode = chain->nodes[lvt];
			... with this <<<<< */
			il = which_lchain_node(chain, LchainNode, TimeList[lvt].mplus);
			if (il < 0)
				{
				il = which_lchain_node(chain, LchainControl, TimeList[lvt].mplus);
				if (il < 0) continue;
				}
			lnode = chain->nodes[il];
			if (lnode->there) break;
			}
		if (lvt < 0) continue;
		if (ivt >= NumTime-1) continue;
		for (rvt=ivt+1; rvt<NumTime; rvt++)
			{
			/* >>>>> replace this ...
			rnode = chain->nodes[rvt];
			... with this <<<<< */
			il = which_lchain_node(chain, LchainNode, TimeList[rvt].mplus);
			if (il < 0)
				{
				il = which_lchain_node(chain, LchainControl, TimeList[rvt].mplus);
				if (il < 0) continue;
				}
			rnode = chain->nodes[il];
			if (rnode->there) break;
			}
		if (rvt > NumTime-1) continue;

		/* Remove existing interpolated nodes */
		if (chain->inum > 0)
			{
			for (ii=0; ii<chain->inum; ii++)
				chain->interps[ii] = destroy_linterp(chain->interps[ii]);
			FREEMEM(chain->interps);
			chain->inum = 0;
			}

		/* Interpolate a new node */
		(void) interp_link_node(chain, lvt, ivt, rvt, master);
		changed = TRUE;
		}

	if (changed)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported", "[interp_dfield_links] - T %s %s\n",
				dfld->element, dfld->level);
		else
			pr_info("Editor.Reported", "[interp_dfield_links] - F %s %s\n",
				dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		(void) verify_dfield_links(dfld, report);
		(void) revise_dependent_links(dfld);
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	interp_link_node

	(
	LCHAIN	chain,
	int		lvt,
	int		ivt,
	int		rvt,
	LOGICAL	master
	)

	{
	int		il;
	LNODE	inode, lnode, rnode;
	float	implus, lmplus, rmplus, lwt, rwt, xpos, ypos;
	LOGICAL	guess;
	POINT	pos;

	if (!chain) return FALSE;

	/* >>>>> replace this ...
	lnode = chain->nodes[lvt];
	inode = chain->nodes[ivt];
	rnode = chain->nodes[rvt];
	... with this <<<<< */
	il = which_lchain_node(chain, LchainNode, TimeList[lvt].mplus);
	if (il < 0)
		{
		il = which_lchain_node(chain, LchainControl, TimeList[lvt].mplus);
		if (il < 0) return FALSE;
		}
	lnode = chain->nodes[il];
	il = which_lchain_node(chain, LchainNode, TimeList[ivt].mplus);
	if (il < 0) return FALSE;
	inode = chain->nodes[il];
	il = which_lchain_node(chain, LchainNode, TimeList[rvt].mplus);
	if (il < 0)
		{
		il = which_lchain_node(chain, LchainControl, TimeList[rvt].mplus);
		if (il < 0) return FALSE;
		}
	rnode = chain->nodes[il];

	/* Interpolate a node */
	implus = TimeList[ivt].mplus;
	lmplus = TimeList[lvt].mplus;
	rmplus = TimeList[rvt].mplus;
	lwt    = (rmplus-implus) / (rmplus-lmplus);
	rwt    = (implus-lmplus) / (rmplus-lmplus);
	xpos   = lwt*lnode->node[X] + rwt*rnode->node[X];
	ypos   = lwt*lnode->node[Y] + rwt*rnode->node[Y];
	set_point(pos, xpos, ypos);
	guess  = (master)? FALSE: TRUE;

	/* Set the node parameters */
	define_lnode_type(inode, TRUE, guess, LchainNode);
	define_lnode_node(inode, pos);
	define_lnode_attach(inode, -1, 0, -1);
	return TRUE;
	}

/**********************************************************************/

LOGICAL	copy_dfield_links

	(
	DFLIST	*dfld1,
	DFLIST	*dfld2
	)

	{
	int		nchain, ichain, ii, ivt, il, ictl, lvt, rvt;
	LCHAIN	chain1, chain2;
	LOGICAL	do_interp, master;

	if (!dfld1) return FALSE;
	(void) clear_dfield_links(dfld1, FALSE);
	if (!dfld2) return FALSE;

	master = same(dfld1->element, "MASTER_LINK");

	/* Make a copy of each link chain */
	nchain = dfld1->nchain = dfld2->nchain;
	dfld1->chains = GETMEM(dfld1->chains, LCHAIN, nchain);
	for (ichain=0; ichain<nchain; ichain++)
		{
		dfld1->chains[ichain] = copy_lchain(dfld2->chains[ichain]);
		}

	/* Reinterpolate the link chains to match the field */
	for (ichain=0; ichain<nchain; ichain++)
		{
		chain1 = dfld1->chains[ichain];
		chain2 = dfld2->chains[ichain];

		/* Remove the copied interpolations */
		if (chain1->inum > 0)
			{
			for (ii=0; ii<chain1->inum; ii++)
				chain1->interps[ii] = destroy_linterp(chain1->interps[ii]);
			FREEMEM(chain1->interps);
			chain1->inum = 0;
			}

		/* Now interpolate any gaps in this link chain */
		/* We can never use these or move them */
		/* It just prevents interpolating the field with missing links */

		/* Find first field with a link node */
		for (lvt=0; lvt<NumTime; lvt++)
			{
			if (dfld1->frames && dfld1->frames[lvt].meta)
				{
				/* >>>>> replace this ...
				if (chain1->nodes[lvt]->there) break;
				... with this <<<<< */
				il = which_lchain_node(chain1, LchainNode, TimeList[lvt].mplus);
				if (il < 0) continue;
				if (chain1->nodes[il]->there) break;
				}
			else
				{
				/* >>>>> replace this ...
				chain1->nodes[lvt]->there = FALSE;
				... with this <<<<< */
				il = which_lchain_node(chain1, LchainNode, TimeList[lvt].mplus);
				if (il < 0) continue;
				chain1->nodes[il]->there = FALSE;
				}
			}

		while (lvt < NumTime)
			{
			/* Find next field with a link node */
			/* Were there any inbetween without? */
			do_interp = FALSE;
			for (rvt=lvt+1; rvt<NumTime; rvt++)
				{
				if (dfld1->frames && dfld1->frames[rvt].meta)
					{
					/* >>>>> replace this ...
					if (chain1->nodes[rvt]->there) break;
					... with this <<<<< */
					il = which_lchain_node(chain1, LchainNode,
														TimeList[rvt].mplus);
					if (il < 0) continue;
					if (chain1->nodes[il]->there) break;

					if (!do_interp)
						{
						ivt = rvt;
						do_interp = TRUE;
						}
					}
				else
					{
					/* >>>>> replace this ...
					chain1->nodes[rvt]->there = FALSE;
					... with this <<<<< */
					il = which_lchain_node(chain1, LchainNode,
														TimeList[rvt].mplus);
					if (il < 0) continue;
					chain1->nodes[il]->there = FALSE;
					}
				}
			if (rvt >= NumTime) break;

			/* Do any interpolating if required */
			while (do_interp)
				{
				(void) interp_link_node(chain1, lvt, ivt, rvt, master);

				/* Find next field without a link between lvt and rvt */
				do_interp = FALSE;
				for (ivt++; ivt<rvt; ivt++)
					{
					if (dfld1->frames && dfld1->frames[ivt].meta)
						{
						/* >>>>> replace this ...
						if (chain1->nodes[ivt]->there) continue;
						... with this <<<<< */
						il = which_lchain_node(chain1, LchainNode,
														TimeList[ivt].mplus);
						if (il < 0) continue;
						if (chain1->nodes[il]->there) continue;
						do_interp = TRUE;
						break;
						}
					}
				}

			/* Advance to nextsection */
			lvt = rvt;
			}
		}

	(void) verify_dfield_links(dfld1, TRUE);
	return TRUE;
	}

/**********************************************************************/

LOGICAL	revise_links(void)

	{
	int		idfld;
	DFLIST	*dfld;

	/* Update the links in fields linked to a master link */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		(void) revise_dependent_links(dfld);
		}

	/* Update the links in fields linked to another field */
	for (idfld=0; idfld<NumDfld; idfld++)
	    {
	    dfld = DfldList + idfld;
		(void) revise_dependent_links(dfld);
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	revise_dependent_links

	(
	DFLIST	*dfld
	)

	{
	DFLIST	*df, *linkto;
	int		idfld;

	if (!dfld) return FALSE;

	for (idfld=0; idfld<NumDfld; idfld++)
		{
		df = DfldList + idfld;
		if (df == dfld) continue;
		linkto = df->linkto;
		if (!linkto)    continue;

		/* If linked to MASTER_LINK propagate from group master */
		/*  or global master as appropriate                     */
		if (same(linkto->element, "MASTER_LINK"))
			{
			if (linkto == dfld)
				{
				if (linkto->nchain <= 0)
						(void) copy_dfield_links(df, MasterLinks);
				else    (void) copy_dfield_links(df, linkto);
				}
			else if (dfld == MasterLinks)
				{
				if (linkto->nchain <= 0)
						(void) copy_dfield_links(df, MasterLinks);
				}
			}

		/* Otherwise just propagate from linked field */
		else if (linkto == dfld) (void) copy_dfield_links(df, linkto);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     e x t r a c t _ l i n k s                                        *
*     r e l e a s e _ l i n k s                                        *
*     b u i l d _ l i n k _ c h a i n                                  *
*     p r e p a r e _ l i n k _ c h a i n                              *
*     r e m o v e _ l i n k _ c h a i n                                *
*     r e m o v e _ l i n k _ n o d e                                  *
*     s h u f f l e _ l i n k _ c h a i n                              *
*     l i n k _ c h a i n s _ p o s t e d                              *
*     p o s t _ l i n k _ c h a i n s                                  *
*     r e p l a c e _ p o s t e d _ c h a i n s                        *
*     r e l e a s e _ p o s t e d _ c h a i n s                        *
*                                                                      *
***********************************************************************/

LOGICAL	extract_links(void)

	{
	int		ichain;
	LCHAIN	chain;

	/* Make sure active field is valid */
	if (!LinkShown)          return FALSE;
	if (!ActiveDfld)         return FALSE;
	if (!ActiveDfld->dolink) return FALSE;

	(void) release_links();
	for (ichain=0; ichain<ActiveDfld->nchain; ichain++)
		{

#		ifdef DEBUG_INTERP_POINTS
		pr_diag("Interp.Points",
			"Interpolated link chain: %d  (%s %s)\n",
			ichain, ActiveDfld->element, ActiveDfld->level);
#		endif /* DEBUG_INTERP_POINTS */

		chain = ActiveDfld->chains[ichain];
		(void) build_link_chain(ActiveDfld, chain, FALSE);
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL		release_links(void)

	{
	/* Make sure active field is valid */
	if (!ActiveDfld)         return TRUE;
	if (!ActiveDfld->dolink) return TRUE;

	/* Empty the display node */
	empty_metafile(LinkMeta);
	return TRUE;
	}

/**********************************************************************/

LOGICAL		build_link_chain

	(
	DFLIST	*dfld,
	LCHAIN	chain,
	LOGICAL	temp
	)

	{
	CURVE		box, track;
	AREA		gbox, abox;
	LINE		line;
	LABEL		xlab, slab, elab, klab;
	int			mplus, inode, its, ite, iprev, inext, il, ibgn, iend;
	LOGICAL		valid, master;
	float		size, asize, msize, csize;
	float		wdth, cwdth, swdth, hght, chght, shght;
	float		xdist, xtime, speed;
	double		xspeed;
	float		xl, xr, yt, yb, xc, yc, xxc, yyc;
	float		xxl, xxr, yyb, yyt;
	float		xsl, xsr, xsx, ysb, yst;
	POINT		pos;
	char		cbuf[80];
	COLOUR		colour, tcolr, gcolr;
	FSTYLE		fill;
	HILITE		hilite;
	METAFILE	meta;
	LNODE		node;

	static	const	float	afact  = 1.0;
	static	const	float	mfact  = 0.7;
	static	const	float	cfact  = 0.7;

	static	const	float	wfactm = 2.0;
	static	const	float	wfacth = 1.6;
	static	const	float	xfactm = 2.5;
	static	const	float	xfacth = 2.0;
	static	const	float	swfact = 2.5;
	static	const	float	hfact  = 1.0;
	static	const	float	zfact  = 0.8;
	static	const	float	shfact = 0.8;

	if (!dfld)  return FALSE;
	if (!chain) return FALSE;

	master = same(dfld->element, "MASTER_LINK");

	/* Build interpolated chain */
	if (!temp) (void) interpolate_lchain(chain);

	/* Decide where links are going */
	meta   = (temp)? TempMeta: LinkMeta;
	colour = (temp)? SafeColour: LinkColour;
	tcolr  = (temp)? SafeColour: LinkTextColour;
	gcolr  = (temp)? SafeColour: LinkGuessColour;
	fill   = (temp)? SafeFstyle: find_fstyle("solid_fill", &valid);
	hilite = (HILITE)((temp)? 2: 0);

	/* Compute appropriate size for the link markers and labels */
	gxSetupTransform(DnLink);
	size  = LmarkSize * gxGetMfact() / 1000;
	asize = afact * size;
	msize = mfact * size;
	csize = cfact * LmarkSize;

#	ifdef DEBUG_INTERP_POINTS
	(void) pr_diag("Interp.Points", "Size for markers: %.2f\n", msize);
#	endif /* DEBUG_INTERP_POINTS */

	if (minutes_in_depictions())
		{
		wdth  = wfactm * size;
		cwdth = xfactm * size;
		}
	else
		{
		wdth  = wfacth * size;
		cwdth = xfacth * size;
		}
	swdth = swfact * size;
	hght  = hfact  * size;
	chght = zfact  * size;
	shght = shfact * size;

	/* Construct the interpolated link path */
	if (NotNull(chain->interps) && chain->inum > 0)
		{

		/* Display the track as a thin line */
		track = create_curve("", "", "");
		define_lspec(&track->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		/* >>>>> replace this ...
		for (ictl=0; ictl<chain->inum; ictl++)
			{
			if (!chain->interps[ictl]->there) continue;
			copy_point(pos, chain->interps[ictl]->node);
			add_point_to_curve(track, pos);
			}
		... with this <<<<< */
		add_line_to_curve(track, chain->track);
		add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) track);

		/* Display the interpolated positions as small boxes or tiny boxes */
		for (inode=0; inode<chain->inum; inode++)
			{
			copy_point(pos, chain->interps[inode]->node);

			if (chain->interps[inode]->there)
				{

				/* Build a small box */
				xl = pos[X] - msize/8;	xr = xl + msize/4;
				yb = pos[Y] - msize/8;	yt = yb + msize/4;
				box = create_curve("", "", "");
				define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
							 hilite);
				add_point_to_curve(box, make_point(xl, yt));
				add_point_to_curve(box, make_point(xr, yt));
				add_point_to_curve(box, make_point(xr, yb));
				add_point_to_curve(box, make_point(xl, yb));
				add_point_to_curve(box, make_point(xl, yt));
				add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) box);
				}

			else
				{

				/* Build a tiny box */
				xl = pos[X] - msize/24;	xr = xl + msize/12;
				yb = pos[Y] - msize/24;	yt = yb + msize/12;
				box = create_curve("", "", "");
				define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
							 hilite);
				add_point_to_curve(box, make_point(xl, yt));
				add_point_to_curve(box, make_point(xr, yt));
				add_point_to_curve(box, make_point(xr, yb));
				add_point_to_curve(box, make_point(xl, yb));
				add_point_to_curve(box, make_point(xl, yt));
				add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) box);
				}
			}
		}

	/* Construct the floating node markers */
	if (NotNull(chain->nodes) && chain->lnum > 0)
		{

		/* Check for floating nodes */
		for (inode=0; inode<chain->lnum; inode++)
			{
			node = chain->nodes[inode];
			if (!node->there)                  continue;
			if (node->ltype != LchainFloating) continue;

			/* Check the floating node location */
			copy_point(pos, node->node);
			if (!inside_dn_window(DnMap, pos)) continue;

			/* Put a small diamond around the floating node */
			xc = pos[X];	xl = xc - msize/2.0;	xr = xc + msize/2.0;
			yc = pos[Y];	yb = yc - msize/2.0;	yt = yc + msize/2.0;
			box = create_curve("", "", "");
			define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
			add_point_to_curve(box, make_point(xl, yc));
			add_point_to_curve(box, make_point(xc, yt));
			add_point_to_curve(box, make_point(xr, yc));
			add_point_to_curve(box, make_point(xc, yb));
			add_point_to_curve(box, make_point(xl, yc));

			/* Display the diamond */
			add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) box);
			}
		}

	/* Construct the control node markers */
	if (NotNull(chain->nodes) && chain->lnum > 0)
		{

		/* Check for control nodes */
		for (inode=0; inode<chain->lnum; inode++)
			{
			node = chain->nodes[inode];
			if (!node->there)                  continue;
			if (node->ltype != LchainControl)  continue;

			/* Check the control node location */
			copy_point(pos, node->node);
			if (!inside_dn_window(DnMap, pos)) continue;

			/* Put a diamond around the control node */
			xc = pos[X];	xl = xc - msize/1.5;	xr = xc + msize/1.5;
			yc = pos[Y];	yb = yc - msize/1.5;	yt = yc + msize/1.5;
			box = create_curve("", "", "");
			define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
			add_point_to_curve(box, make_point(xl, yc));
			add_point_to_curve(box, make_point(xc, yt));
			add_point_to_curve(box, make_point(xr, yc));
			add_point_to_curve(box, make_point(xc, yb));
			add_point_to_curve(box, make_point(xl, yc));

			/* Display guess nodes with a filled diamond */
			if (node->guess)
				{
				gbox = create_area("", "", "");
				define_lspec(&gbox->lspec, colour, 0, NULL, FALSE, 0.0, 0.0,
							 (HILITE) 0);
				define_fspec(&gbox->fspec, gcolr, fill, NULL, FALSE, FALSE,
							 0.0, 0.0, (HILITE) 0);
				line = copy_line(box->line);
				define_area_boundary(gbox, line);

				/* Add the guess box */
				add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) gbox);
				}

			/* Display the diamond */
			add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) box);

			/* Display the time of control node (if requested) */
			if (DisplayCtrl)
				{
				xxc = xc;		xl  = xxc - cwdth/2;	xr  = xxc + cwdth/2;
								xxl = xxc - msize/2;	xxr = xxc + msize/2;
				yyc = yb - chght/2;
								yb  = yyc - chght/2;	yt  = yyc + chght/2;
								yyb = yb  - msize/2;	yyt = yt  + msize/2;
				mplus = node->mplus;
				if (minutes_in_depictions())
					(void) sprintf(cbuf, "C%s", hour_minute_string(0, mplus));
				else
					(void) sprintf(cbuf, "C%.2d", mplus/60);
				xlab = create_label("", "", cbuf, make_point(xxc, yyc), 0.0);
				define_tspec(&xlab->tspec, tcolr, SafeFont, FALSE, csize, 0.0,
							 Hc, Vc, hilite);
				abox = create_area("", "", "");
				define_lspec(&abox->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
							(HILITE) 1);
				define_fspec(&abox->fspec, colour, fill, NULL, FALSE, FALSE,
							 0.0, 0.0, (HILITE) 1);
				line = create_line();
				add_point_to_line(line, make_point(xl,  yb));
				add_point_to_line(line, make_point(xxl, yb));
				add_point_to_line(line, make_point(xxc, yyb));
				add_point_to_line(line, make_point(xxr, yb));
				add_point_to_line(line, make_point(xr,  yb));
				add_point_to_line(line, make_point(xr,  yt));
				add_point_to_line(line, make_point(xxr, yt));
				add_point_to_line(line, make_point(xxc, yyt));
				add_point_to_line(line, make_point(xxl, yt));
				add_point_to_line(line, make_point(xl,  yt));
				add_point_to_line(line, make_point(xl,  yb));
				define_area_boundary(abox, line);

				/* Display the control node time */
				add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) abox);
				add_item_to_metafile(meta, "label", "d", "", "", (ITEM) xlab);
				}
			}
		}

	/* Determine range of link nodes */
	its = chain->lnum - 1;
	ite = -1;
	if (NotNull(chain->nodes) && chain->lnum > 0)
		{
		for (inode=0; inode<chain->lnum; inode++)
			{
			node = chain->nodes[inode];
			if (!node->there)              continue;
			if (node->ltype != LchainNode) continue;

			if (inode < its) its = inode;
			ite = inode;
			}
		}

	/* Construct the link node markers */
	if (NotNull(chain->nodes) && chain->lnum > 0)
		{

		/* Check for link nodes */
		for (inode=0; inode<chain->lnum; inode++)
			{
			node = chain->nodes[inode];
			if (!node->there)                  continue;
			if (node->ltype != LchainNode)     continue;

			/* Check the link node location */
			copy_point(pos, node->node);
			if (!inside_dn_window(DnMap, pos)) continue;

			/* Put a box around the link node */
			xc = pos[X];	xl = xc - msize/2;	xr = xc + msize/2;
			yc = pos[Y];	yb = yc - msize/2;	yt = yc + msize/2;
			box = create_curve("", "", "");
			define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
			add_point_to_curve(box, make_point(xl, yb));
			add_point_to_curve(box, make_point(xl, yt));
			add_point_to_curve(box, make_point(xr, yt));
			add_point_to_curve(box, make_point(xr, yb));
			add_point_to_curve(box, make_point(xl, yb));

			/* Display guess nodes with a filled box */
			if (node->guess)
				{
				gbox = create_area("", "", "");
				define_lspec(&gbox->lspec, colour, 0, NULL, FALSE, 0.0, 0.0,
							 (HILITE) 0);
				define_fspec(&gbox->fspec, gcolr, fill, NULL, FALSE, FALSE,
							 0.0, 0.0, (HILITE) 0);
				line = copy_line(box->line);
				define_area_boundary(gbox, line);

				/* Add the guess box */
				add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) gbox);
				}

			/* Display the box */
			add_item_to_metafile(meta, "curve", "c", "", "", (ITEM) box);

			/* Put a larger white box around link nodes in active frame only */
			/* Place extrapolated times inside this box if applicable */
			mplus = node->mplus;
			if (!temp && (mplus == TimeList[EditTime].mplus))
				{
				xl = xc - asize/2;		xr = xc + asize/2;
				yb = yc - asize/2;		yt = yc + asize/2;

				/* Add early/late start/end times (if required) */
				slab  = NULL;
				elab  = NULL;
				if (DisplayEarly)
					{
					if (master)
						{
						iprev = prev_depict_time(EditTime);
						inext = next_depict_time(EditTime);
						}
					else
						{
						iprev = prev_active_time(EditTime);
						inext = next_active_time(EditTime);
						}

					/* Add early start time (back extrap) */
					if ((inode == its) && (iprev >= 0)
						&& (chain->splus > TimeList[iprev].mplus)
						&& (chain->splus < mplus))
						{
						xxc = xl - wdth/2;
						xl -= wdth;
						if (minutes_in_depictions())
							(void) sprintf(cbuf, "%s",
									hour_minute_string(0, chain->splus));
						else
							(void) sprintf(cbuf, "%.2d", chain->splus/60);

#						ifdef DEBUG
						(void) pr_diag("Timelink", "Early start time: %s\n", cbuf);
#						endif /* DEBUG */

						slab = create_label("", "", cbuf,
											make_point(xxc, yc), 0.0);
						define_tspec(&slab->tspec, tcolr, SafeFont, FALSE,
									 csize, 0.0, Hc, Vc, hilite);
						}

					/* Add late start time (truncation) */
					else if ((inode == its) && (inext > 0)
						&& (chain->splus > mplus)
						&& (chain->splus < TimeList[inext].mplus))
						{
						xxc = xl - wdth/2;
						xl -= wdth;
						if (minutes_in_depictions())
							(void) sprintf(cbuf, "%s",
									hour_minute_string(0, chain->splus));
						else
							(void) sprintf(cbuf, "%.2d", chain->splus/60);

#						ifdef DEBUG
						(void) pr_diag("Timelink", "Late start time: %s\n", cbuf);
#						endif /* DEBUG */

						slab = create_label("", "", cbuf,
											make_point(xxc, yc), 0.0);
						define_tspec(&slab->tspec, tcolr, SafeFont, FALSE,
									 csize, 0.0, Hc, Vc, hilite);
						}

					/* Add early end time (truncation) */
					if ((inode == ite) && (iprev >= 0)
						&& (chain->eplus > TimeList[iprev].mplus)
						&& (chain->eplus < mplus))
						{
						xxc = xr + wdth/2;
						xr += wdth;
						if (minutes_in_depictions())
							(void) sprintf(cbuf, "%s",
									hour_minute_string(0, chain->eplus));
						else
							(void) sprintf(cbuf, "%.2d", chain->eplus/60);

#						ifdef DEBUG
						(void) pr_diag("Timelink", "Early end time: %s\n", cbuf);
#						endif /* DEBUG */

						elab = create_label("", "", cbuf,
											make_point(xxc, yc), 0.0);
						define_tspec(&elab->tspec, tcolr, SafeFont, FALSE,
									 csize, 0.0, Hc, Vc, hilite);
						}

					/* Add late end time (fwd extrap) */
					else if ((inode == ite) && (inext > 0)
						&& (chain->eplus > mplus)
						&& (chain->eplus < TimeList[inext].mplus))
						{
						xxc = xr + wdth/2;
						xr += wdth;
						if (minutes_in_depictions())
							(void) sprintf(cbuf, "%s",
									hour_minute_string(0, chain->eplus));
						else
							(void) sprintf(cbuf, "%.2d", chain->eplus/60);

#						ifdef DEBUG
						(void) pr_diag("Timelink", "Late end time: %s\n", cbuf);
#						endif /* DEBUG */

						elab = create_label("", "", cbuf,
											make_point(xxc, yc), 0.0);
						define_tspec(&elab->tspec, tcolr, SafeFont, FALSE,
									 csize, 0.0, Hc, Vc, hilite);
						}
					}

				/* Display the link box and start/end times */
				abox = create_area("", "", "");
				define_lspec(&abox->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
							(HILITE) 1);
				define_fspec(&abox->fspec, colour, fill, NULL, FALSE, FALSE,
							 0.0, 0.0, (HILITE) 1);
				line = copy_line(box->line);
				add_point_to_line(line, make_point(xl, yb));
				add_point_to_line(line, make_point(xr, yb));
				add_point_to_line(line, make_point(xr, yt));
				add_point_to_line(line, make_point(xl, yt));
				add_point_to_line(line, make_point(xl, yb));
				define_area_boundary(abox, line);

				/* Add the active box and labels */
				add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) abox);
				add_item_to_metafile(meta, "label", "d", "", "", (ITEM) slab);
				add_item_to_metafile(meta, "label", "d", "", "", (ITEM) elab);

				/* Label the node (if required) */
				if (DisplayTime)
					{
					if (minutes_in_depictions())
						(void) sprintf(cbuf, "%s", hour_minute_string(0, mplus));
					else
						(void) sprintf(cbuf, "%.2d", mplus/60);
					xxc = xc;			xxl = xxc - wdth/2;		xxr = xxc + wdth/2;
					yyc = yb - hght/2;	yyb = yyc - hght/2;		yyt = yyc + hght/2;
					xlab = create_label("", "", cbuf, make_point(xxc, yyc), 0.0);
					define_tspec(&xlab->tspec, tcolr, SafeFont, FALSE, csize, 0.0,
								 Hc, Vc, hilite);

					abox = create_area("", "", "");
					define_lspec(&abox->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
								(HILITE) 1);
					define_fspec(&abox->fspec, colour, fill, NULL, FALSE, FALSE,
								 0.0, 0.0, (HILITE) 1);
					line = create_line();
					add_point_to_line(line, make_point(xxl, yyb));
					add_point_to_line(line, make_point(xxr, yyb));
					add_point_to_line(line, make_point(xxr, yyt));
					add_point_to_line(line, make_point(xxl, yyt));
					add_point_to_line(line, make_point(xxl, yyb));
					define_area_boundary(abox, line);

					/* Add the time box */
					add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) abox);
					add_item_to_metafile(meta, "label", "d", "", "", (ITEM) xlab);
					}

				/* Display speed of motion from interpolated nodes (if required) */
				if (DisplaySpeed)
					{

					/* Determine speed of motion from interpolated nodes */
					il = which_lchain_node(chain, LchainInterp, mplus);
					if (il < 0)
						{
						ibgn = iend = -1;
						}
					else if (il < chain->inum -1)
						{
						ibgn = il;
						iend = ibgn + 1;
						}
					else
						{
						iend = il;
						ibgn = (iend > 0)? iend - 1: iend;
						}
					if (ibgn < 0 || ibgn == iend)
						{
						speed = 0.0;
						}
					else
						{
						xdist = great_circle_distance(MapProj,
									chain->interps[ibgn]->node,
									chain->interps[iend]->node);
						xtime = (float) chain->interps[iend]->mplus
									- (float) chain->interps[ibgn]->mplus;
						speed = (xtime > 0.0)? (xdist / (xtime * 60.0)): 0.0;
						}

					/* Convert speed of motion to acceptable units */
					(void) convert_value("m/s", (double) speed, SpdUnits, &xspeed);

					/* Add a label for the speed of motion */
					xxc = xc;	xsl = xxc - swdth/2;	xsr = xxc + swdth/2;
								xsx = xsr + swdth/6;
					yyc = yt + shght/2;
								ysb = yyc - shght/2;	yst = yyc + shght/2;
					(void) sprintf(cbuf, "%d%s", NINT(xspeed), SpdLabel);
					klab = create_label("", "", cbuf, make_point(xxc, yyc), 0.0);
					define_tspec(&klab->tspec, tcolr, SafeFont, FALSE, csize, 0.0,
								 Hc, Vc, hilite);

#					ifdef DEBUG
					(void) pr_diag("Timelink",
						"Speed of motion at: %d (%d - %d)\n", il, ibgn, iend);
					if (ibgn != iend)
						(void) pr_diag("Timelink",
							"Speed of motion: %.1f (map units) over: %.1f (min)\n",
							xdist, xtime);
					(void) pr_diag("Timelink",
						"Speed of motion: %.1f (m/s) or %.1f (%s)\n",
						speed, xspeed, SpdUnits);
#					endif /* DEBUG */

					abox = create_area("", "", "");
					define_lspec(&abox->lspec, colour, 0, NULL, FALSE, 1.5, 0.0,
								(HILITE) 1);
					define_fspec(&abox->fspec, colour, fill, NULL, FALSE, FALSE,
								 0.0, 0.0, (HILITE) 1);
					line = create_line();
					add_point_to_line(line, make_point(xsl, ysb));
					add_point_to_line(line, make_point(xsr, ysb));
					add_point_to_line(line, make_point(xsx, yyc));
					add_point_to_line(line, make_point(xsr, yst));
					add_point_to_line(line, make_point(xsl, yst));
					add_point_to_line(line, make_point(xsl, ysb));
					define_area_boundary(abox, line);

					/* Add the active box and labels */
					add_item_to_metafile(meta, "area",  "c", "", "", (ITEM) abox);
					add_item_to_metafile(meta, "label", "d", "", "", (ITEM) klab);
					}
				}
			}
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL		prepare_link_chain

	(
	DFLIST	*dfld,
	LCHAIN	chain
	)

	{
	int			itime, ifirst, ilast, mfirst, mlast, inode;
	LOGICAL		ctrlok, first;
	LNODE		lnode;

	if (!dfld)         return FALSE;
	if (!dfld->fields) return FALSE;
	if (!dfld->frames) return FALSE;
	if (!chain)        return FALSE;

	/* Daily and static fields will not have link chains */
	if (tdep_special(dfld->tdep)) return FALSE;

	/* Determine start and end frames for this field */
	first  = TRUE;
	ifirst = -1;
	ilast  = -1;
	for (itime=0; itime<NumTime; itime++)
		{
		if (!dfld->fields[itime])      continue;
		if (!dfld->frames[itime].meta) continue;

		if (first)
			{
			ifirst = itime;
			first  = FALSE;
			}
		ilast = itime;
		}

	/* Return if no valid frames */
	if (ifirst < 0 || ilast < 0) return FALSE;

	/* Set start and end times for this field */
	mfirst = TimeList[ifirst].mplus;
	mlast  = TimeList[ilast].mplus;

	/* Set whether control nodes are allowed based on field type */
	switch (dfld->editor)
		{
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:			ctrlok = TRUE;
								break;

		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:		ctrlok = FALSE;
								break;

		default:				ctrlok = FALSE;
								break;
		}

	/* Adjust the link chain reference time to match */
	/*  the current T0 time (if required)            */
	if (!same(chain->xtime, Stime))
		{

#		ifdef DEBUG
		pr_diag("Editor",
			"[prepare_link_chain] Reset reference time from: %s to: %s\n",
			chain->xtime, Stime);
#		endif /* DEBUG */

		define_lchain_reference_time(chain, Stime);
		}

	/* Return if link chain is beyond time range */
	if (chain->eplus < mfirst || chain->splus > mlast) return FALSE;

	/* Reset start and end time (if required) */
	if (chain->splus < mfirst) define_lchain_start_time(chain, mfirst);
	if (chain->eplus > mlast)  define_lchain_end_time(chain, mlast);

	/* Remove all control and floating nodes (if required) */
	if (!ctrlok && chain->lnum > 0)
		{
		for (inode=chain->lnum-1; inode>=0; inode--)
			{
			lnode = chain->nodes[inode];
			if (lnode->ltype == LchainControl)
				(void) remove_lchain_lnode(chain, inode);
			else if (lnode->ltype == LchainFloating)
				(void) remove_lchain_lnode(chain, inode);
			}
		}

	/* Return if no remaining nodes */
	if (chain->lnum <= 0) return FALSE;

	/* Ensure "normal" nodes at all depiction times */
	for (itime=0; itime<NumTime; itime++)
		{
		if (!dfld->fields[itime])      continue;
		if (!dfld->frames[itime].meta) continue;

		/* Promote node at this depiction time to a "normal" node */
		promote_lchain_node(chain, LchainNode, TimeList[itime].mplus);

		/* Now turn it into a "guess" node */
		inode = which_lchain_node(chain, LchainNode, TimeList[itime].mplus);
		if (inode >= 0) chain->nodes[inode]->guess = TRUE;
		}

	/* Remove "normal" nodes at all non-depiction times */
	for (itime=0; itime<NumTime; itime++)
		{
		if (dfld->fields[itime] && dfld->frames[itime].meta) continue;

		/* Remove any "normal" node at this time */
		inode = which_lchain_node(chain, LchainNode, TimeList[itime].mplus);
		if (inode >= 0) (void) remove_lchain_lnode(chain, inode);
		}

	/* Change "control" nodes to "guess-control" nodes */
	/*  at all non-depiction times (if required)       */
	if (ctrlok)
		{

		/* Check for "control" nodes at all times */
		for (inode=0; inode<chain->lnum; inode++)
			{
			lnode = chain->nodes[inode];
			if (!lnode->there || lnode->ltype != LchainControl) continue;

			/* Change a "control" node at this time into a "guess-control" node */
			define_lnode_type(lnode, TRUE, TRUE, LchainControl);
			}
		}

	/* Re-interpolate the modified link chain */
	chain->dointerp = TRUE;
	(void) interpolate_lchain(chain);

	/* Reset presentation for the modified link chain */
	invoke_link_chain_presentation(chain);

	return TRUE;
	}

/**********************************************************************/

LOGICAL		remove_link_chain

	(
	DFLIST	*dfld,
	int		ichain
	)

	{
	int		ic;
	LCHAIN	chain;

	if (IsNull(dfld))          return FALSE;
	if (ichain < 0)            return FALSE;
	if (ichain > dfld->nchain) return FALSE;

	if (dfld->nchain <= 0)     return TRUE;

	/* Free space used by this link chain */
	dfld->chains[ichain] = destroy_lchain(dfld->chains[ichain]);

	/* Remove the link chain from the list */
	dfld->nchain--;
	for (ic=ichain; ic<dfld->nchain; ic++)
		{
		dfld->chains[ic] = dfld->chains[ic+1];
		}
	dfld->chains[ic] = NullLchain;

	return TRUE;
	}

/**********************************************************************/

LOGICAL		remove_link_node

	(
	DFLIST	*dfld,
	int		ichain,
	int		inode
	)

	{
	int		il, ils, ile, itime, splus, eplus;
	LCHAIN	chain;
	LNODE	node;

	if (IsNull(dfld))          return FALSE;
	if (ichain < 0)            return FALSE;
	if (ichain > dfld->nchain) return FALSE;
	if (dfld->nchain <= 0)     return TRUE;
	chain = dfld->chains[ichain];
	if (IsNull(chain))         return FALSE;
	if (inode < 0)             return FALSE;
	if (inode >= chain->lnum)  return FALSE;

	/* Determine node range for "normal" link nodes */
	ils = chain->lnum;
	ile = -1;
	for (il=0; il<chain->lnum; il++)
		{
		node = chain->nodes[il];
		if (!node->there)              continue;
		if (node->ltype != LchainNode) continue;

		if (il < ils) ils = il;
		ile = il;
		}

	/* Remove the link chain if no "normal" nodes left in link chain */
	if (ile < 0)
		{
		(void) remove_link_chain(dfld, ichain);
		return TRUE;
		}

	/* Remove the link chain if only one "normal" node left in link chain */
	if (inode == ils && inode == ile)
		{
		(void) remove_link_chain(dfld, ichain);
		return TRUE;
		}

	/* Remove the start node */
	else if (inode == ils)
		{

		/* Determine the start time for this field (may have changed) */
		for (itime=0; itime<NumTime; itime++)
			{
			if (!dfld->fields)             continue;
			if (!dfld->fields[itime])      continue;
			if (!dfld->frames)             continue;
			if (!dfld->frames[itime].meta) continue;

			splus = TimeList[itime].mplus;
			break;
			}

		/* Remove any "control" or "floating" nodes before the start time */
		for (il=0; il<chain->lnum; il++)
			{
			node = chain->nodes[il];
			if (!node->there)                         continue;
			if (node->ltype != LchainControl
					&& node->ltype != LchainFloating) continue;
			if (node->mplus < splus)
				(void) remove_lchain_lnode(chain, il);
			}

		/* Remove the old start node */
		empty_lnode(chain->nodes[inode]);

		/* Set the new start node */
		for (ils=inode+1; ils<=ile; ils++)
			{
			node = chain->nodes[ils];
			if (!node->there)              continue;
			if (node->ltype != LchainNode) continue;
			break;
			}

		/* Adjust start time based on new start node */
		/*  and remaining control nodes              */
		chain->splus = chain->nodes[ils]->mplus;
		for (il=0; il<chain->lnum; il++)
			{
			node = chain->nodes[il];
			if (!node->there) continue;
			if (chain->nodes[il]->mplus < chain->splus)
				chain->splus = chain->nodes[il]->mplus;
			}

		/* Chain is ready for re-interpolation */
		chain->dointerp = TRUE;
		return TRUE;
		}

	/* Remove the end node */
	else if (inode == ile)
		{

		/* Determine the end time for this field (may have changed) */
		for (itime=NumTime-1; itime>=0; itime--)
			{
			if (!dfld->fields)             continue;
			if (!dfld->fields[itime])      continue;
			if (!dfld->frames)             continue;
			if (!dfld->frames[itime].meta) continue;

			eplus = TimeList[itime].mplus;
			break;
			}

		/* Remove any "control" or "floating" nodes after the end time */
		for (il=0; il<chain->lnum; il++)
			{
			node = chain->nodes[il];
			if (!node->there)                         continue;
			if (node->ltype != LchainControl
					&& node->ltype != LchainFloating) continue;
			if (node->mplus > eplus)
				(void) remove_lchain_lnode(chain, il);
			}

		/* Remove the old end node */
		empty_lnode(chain->nodes[inode]);

		/* Set the new end node */
		for (ile=inode-1; ile>=ils; ile--)
			{
			node = chain->nodes[ile];
			if (!node->there)              continue;
			if (node->ltype != LchainNode) continue;
			break;
			}

		/* Adjust end time based on new end node */
		/*  and remaining control nodes          */
		chain->eplus = chain->nodes[ile]->mplus;
		define_lchain_default_attribs(chain);
		for (il=0; il<chain->lnum; il++)
			{
			node = chain->nodes[il];
			if (!node->there) continue;
			if (chain->nodes[il]->mplus > chain->eplus)
				chain->eplus = chain->nodes[il]->mplus;
			}

		/* Chain is ready for re-interpolation */
		chain->dointerp = TRUE;
		return TRUE;
		}

	/* Only start or end node can be removed! */
	return FALSE;
	}

/**********************************************************************/

LOGICAL		shuffle_link_chain

	(
	DFLIST	*dfld,
	int		ifrom,
	int		jto
	)

	{
	int		ic;
	LCHAIN	chain;

	if (IsNull(dfld))         return FALSE;
	if (ifrom < 0)            return FALSE;
	if (ifrom > dfld->nchain) return FALSE;
	if (jto < 0)              return FALSE;
	if (jto > dfld->nchain)   return FALSE;

	if (dfld->nchain <= 0)    return TRUE;
	if (ifrom == jto)         return TRUE;

	/* Save the information from this link chain */
	chain = dfld->chains[ifrom];

	/* Move the link information to an earlier location */
	if (ifrom > jto)
		{
		for (ic=ifrom; ic>jto; ic--)
			{
			dfld->chains[ic] = dfld->chains[ic-1];
			}
		}

	/* Move the link information to a later location */
	else
		{
		for (ic=ifrom; ic<jto; ic++)
			{
			dfld->chains[ic] = dfld->chains[ic+1];
			}
		}

	/* Add back the saved information to the open location */
	dfld->chains[ic] = chain;

	return TRUE;
	}

/**********************************************************************/

static	enum { LinkNone, LinkMaster, LinkSpline, LinkArea, LinkLine }
		LinkType = LinkNone;

static	STRING	LinkElem  = FpaCnone;
static	STRING	LinkLevel = FpaCnone;
static	int		NumChains = 0;
static	LCHAIN	*Chains   = NULL;

/**********************************************************************/

LOGICAL	link_chains_posted

	(
	STRING	which,
	STRING	elem,
	STRING	level
	)

	{
	LOGICAL	status = FALSE;

	if (blank(which)                && (LinkType == LinkNone))
		status = TRUE;
	else if (same(which, "master")  && (LinkType == LinkMaster))
		status = TRUE;
	else if (same(which, "surface") && (LinkType == LinkSpline))
		status = (LOGICAL) (same(elem, LinkElem) && same(level, LinkLevel));
	else if (same(which, "areas")   && (LinkType == LinkArea))
		status = (LOGICAL) (same(elem, LinkElem) && same(level, LinkLevel));
	else if (same(which, "curves")  && (LinkType == LinkLine))
		status = (LOGICAL) (same(elem, LinkElem) && same(level, LinkLevel));

	if (!status) release_posted_chains();

	return status;
	}

/**********************************************************************/

void	post_link_chains

	(
	STRING	which,
	STRING	elem,
	STRING	level
	)

	{
	int		ii, ivt, ictl, intp;
	LCHAIN	chain;

	/* Make sure active field is valid */
	if (!ActiveDfld)         return;
	if (!ActiveDfld->dolink) return;

	release_posted_chains();

	if (same(which, "master"))       LinkType = LinkMaster;
	else if (same(which, "surface")) LinkType = LinkSpline;
	else if (same(which, "areas"))   LinkType = LinkArea;
	else if (same(which, "curves"))  LinkType = LinkLine;
	else return;

	/* Save the element and level */
	LinkElem  = elem;
	LinkLevel = level;

	/* Save all the link chains */
	NumChains = ActiveDfld->nchain;
	Chains    = GETMEM(Chains, LCHAIN, NumChains);
	for (ii=0; ii<NumChains; ii++)
		{
		Chains[ii] = copy_lchain(ActiveDfld->chains[ii]);
		}
	return;
	}

/**********************************************************************/

void	replace_posted_chains

	(
	STRING	which,
	STRING	elem,
	STRING	level
	)

	{
	int		ii, ivt, ictl;
	LCHAIN	chain;

	/* Make sure active field is valid */
	if (!ActiveDfld)         return;
	if (!ActiveDfld->dolink) return;

	if (blank(which))                                            return;
	else if (same(which, "master")  && (LinkType != LinkMaster)) return;
	else if (same(which, "surface") && (LinkType != LinkSpline)) return;
	else if (same(which, "areas")   && (LinkType != LinkArea))   return;
	else if (same(which, "curves")  && (LinkType != LinkLine))   return;
	if (!same(elem,  LinkElem))                                  return;
	if (!same(level, LinkLevel))                                 return;

	/* Remove current link chains */
	(void) clear_dfield_links(ActiveDfld, FALSE);

	/* Replace link chains */
	ActiveDfld->nchain = NumChains;
	ActiveDfld->chains = INITMEM(LCHAIN, ActiveDfld->nchain);
	for (ii=0; ii<ActiveDfld->nchain; ii++)
		{
		ActiveDfld->chains[ii] = copy_lchain(Chains[ii]);
		}

	/* Verify replacement */
	(void) verify_dfield_links(ActiveDfld, FALSE);
	}

/**********************************************************************/

void	release_posted_chains(void)

	{
	int		ii;

	for (ii=0; ii<NumChains; ii++)
		{
		Chains[ii] = destroy_lchain(Chains[ii]);
		}
	FREEMEM(Chains);
	NumChains = 0;

	LinkType  = LinkNone;
	LinkElem  = FpaCnone;
	LinkLevel = FpaCnone;
	}

/***********************************************************************
*                                                                      *
*     i n v o k e _ l i n k _ s e t _ p r e s e n t a t i o n          *
*     i n v o k e _ c h a i n _ s e t _ p r e s e n t a t i o n        *
*                                                                      *
***********************************************************************/

static	int		NumCspec = 0;
static	CATSPEC	*Cspecs  = (CATSPEC *) 0;
static	int		NumXspec = 0;
static	CATSPEC	*Xspecs  = (CATSPEC *) 0;

static	void	define_link_presentation(void);

/**********************************************************************/

static	void	define_link_presentation

	(
	)

	{
	CATSPEC	*catspec = NullPtr(CATSPEC *);

	/* Create default presentation for link chains only once */
	if (NumCspec <= 0)
		{

		/* Add presentation for link chain track */
		NumCspec++;
		Cspecs  = GETMEM(Cspecs, CATSPEC, NumCspec);
		catspec = Cspecs + (NumCspec - 1);
		init_catspec(catspec);
		define_catspec(catspec, "", "", "", "LSPEC", "", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->lspec.colour = 3;
		catspec->lspec.width  = 3;
		}

	/* Create default presentation for link nodes only once */
	if (NumXspec <= 0)
		{

		/* Add presentation for "interp" nodes */
		NumXspec++;
		Xspecs  = GETMEM(Xspecs, CATSPEC, NumXspec);
		catspec = Xspecs + (NumXspec - 1);
		init_catspec(catspec);
		define_catspec(catspec, "interp", "marker", "mark", "MSPEC",
														"", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->mspec.type   = 5;
		catspec->mspec.size   = 3;
		catspec->mspec.colour = 3;

		/* Add presentation for "control" and "control-guess"  nodes */
		NumXspec++;
		Xspecs  = GETMEM(Xspecs, CATSPEC, NumXspec);
		catspec = Xspecs + (NumXspec - 1);
		init_catspec(catspec);
		define_catspec(catspec, "control", "marker", "mark", "MSPEC",
														"", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->mspec.type   = 5;
		catspec->mspec.size   = 8;
		catspec->mspec.colour = 3;

		NumXspec++;
		Xspecs  = GETMEM(Xspecs, CATSPEC, NumXspec);
		catspec = Xspecs + (NumXspec - 1);
		init_catspec(catspec);
		define_catspec(catspec, "control-guess", "marker", "mark", "MSPEC",
														"", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->mspec.type   = 5;
		catspec->mspec.size   = 8;
		catspec->mspec.colour = 3;

		/* Add presentation for "normal" and "normal-guess" nodes */
		NumXspec++;
		Xspecs  = GETMEM(Xspecs, CATSPEC, NumXspec);
		catspec = Xspecs + (NumXspec - 1);

		init_catspec(catspec);
		define_catspec(catspec, "normal", "marker", "mark", "MSPEC",
														"", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->mspec.type   = 2;
		catspec->mspec.size   = 8;
		catspec->mspec.colour = 3;

		NumXspec++;
		Xspecs  = GETMEM(Xspecs, CATSPEC, NumXspec);
		catspec = Xspecs + (NumXspec - 1);

		init_catspec(catspec);
		define_catspec(catspec, "normal-guess", "marker", "mark", "MSPEC",
														"", "", ZeroPoint, 0.0);
		skip_lspec(&catspec->lspec);
		skip_fspec(&catspec->fspec);
		skip_tspec(&catspec->tspec);
		skip_mspec(&catspec->mspec);
		skip_bspec(&catspec->bspec);
		catspec->mspec.type   = 2;
		catspec->mspec.size   = 8;
		catspec->mspec.colour = 3;
		}
	}

/**********************************************************************/

void		invoke_link_set_presentation

	(
	SET 	set
	)

	{
	int		ispec;

	/* Initialize default link presentation */
	define_link_presentation();

	/* Return if no set */
	if (IsNull(set)) return;

	/* Copy the link chain presentation to the given set */
	for (ispec=0; ispec<NumCspec; ispec++)
		add_catspec_to_set(set, &(Cspecs[ispec]));

	/* Copy the link node presentation to the given set */
	for (ispec=0; ispec<NumXspec; ispec++)
		add_secondary_catspec_to_set(set, &(Xspecs[ispec]));

	/* Invoke the set presentation */
	invoke_set_catspecs(set);
	}

/**********************************************************************/

void		invoke_link_chain_presentation

	(
	LCHAIN 	lchain
	)

	{
	int		ispec;

	/* Initialize default link presentation */
	define_link_presentation();

	/* Return if no link chain */
	if (IsNull(lchain)) return;

	/* Invoke the default presentation for the link chain */
	invoke_item_catspec("lchain", (LCHAIN) lchain, NumCspec, Cspecs);

	/* Invoke the default presentation for the link nodes */
	invoke_item_catspec("nodes",  (LCHAIN) lchain, NumXspec, Xspecs);
	}

/***********************************************************************
*                                                                      *
*     e x t r a c t _ u n l i n k e d                                  *
*     r e l e a s e _ u n l i n k e d                                  *
*                                                                      *
***********************************************************************/

LOGICAL	extract_unlinked(void)

	{
	if (NewAreas)  return extract_unlinked_areas();
	if (NewCurves) return extract_unlinked_lines();
	return FALSE;
	}

/**********************************************************************/

LOGICAL	release_unlinked(void)

	{
	if (NewAreas)  return release_unlinked_areas();
	if (NewCurves) return release_unlinked_lines();
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ e x t r a p                                          *
*     r e m o v e _ e x t r a p                                        *
*     p i c k _ e x t r a p                                            *
*     p r e s e n t _ e x t r a p                                      *
*                                                                      *
***********************************************************************/

LOGICAL	build_extrap

	(
	LOGICAL	start,
	LOGICAL	forward,
	int		ivt,
	int		otmin,
	int		otmax,
	POINT	dpos
	)

	{
	LOGICAL	before, early, late;
	char	clab[80], cval[80];
	float	vx, vy, x, y, csize, xlen, ylen;
	int		iprev, inext;
	int		idef, ikey, nt, nx, ny, nadj, it, ix, iy;
	LOGICAL	valid, any;
	int		tkey, tdef, mplus, tmin, tmax;
	BOX		box;
	BUTTON	button;
	COLOUR	fcolour, tcolour;

	/* Size of boxes for times */
	static	float	dx = 0;
	static	float	dy = 0;

	/* Presentation info for early/late start/end time choosing */
	static	COLOUR	edgeclr = 0;	/* Colour for edge of time boxes */
	static	COLOUR	filldef = 0;	/* Box colour for previously chosen time */
	static	COLOUR	keyclr  = 0;	/* Box colour for keyframe time */
	static	COLOUR	fillclr = 0;	/* Box colour for early start or late end */
	static	COLOUR	backclr = 0;	/* Box colour for late start or early end */
	static	COLOUR	textdef = 0;	/* Text colour for previously chosen time */
	static	COLOUR	textclr = 0;	/* Text colour for all other time boxes */
	static	FSTYLE	filltyp = 0;	/* Style for interior of time boxes */
	static	LOGICAL	defined = FALSE;

	static	LOGICAL	EnableTrunc = FALSE;
	static	LOGICAL	TruncSet    = FALSE;

	if (ivt<0) return FALSE;

	if (!TruncSet)
		{
		STRING	tmode;

		TruncSet = TRUE;
		tmode    = get_feature_mode("Link.Truncation");
		if (blank(tmode))
			{
			EnableTrunc = TRUE;
			}
		else if (same_ic(tmode, "yes"))
			{
			EnableTrunc = TRUE;
			}
		else if (same_ic(tmode, "no"))
			{
			EnableTrunc = FALSE;
			}
		else
			{
			(void) pr_warning("Timelink",
				"Supported Link.Truncation Modes: yes no\n");
			EnableTrunc = TRUE;
			}
		}

	/* Display before or after given node? */
	before = (LOGICAL) ((start && forward) || (!start && !forward));
	iprev = prev_active_time(ivt);
	inext = next_active_time(ivt);
	early = (LOGICAL) (iprev >= 0);
	late  = (LOGICAL) (inext >= 0);
	if (!early && !late) return FALSE;

	/* Set display colours */
	if (!defined)
		{
		if (minutes_in_depictions())
			{
			dx = 50;
			dy = 25;
			}
		else
			{
			dx = 35;
			dy = 25;
			}
		edgeclr = find_colour("White", &valid);
		keyclr  = find_colour("Salmon", &valid);
		fillclr = find_colour("DarkSlateBlue", &valid);
		backclr = find_colour("SeaGreen", &valid);
		filldef = find_colour("Cyan", &valid);
		textclr = find_colour("White", &valid);
		textdef = find_colour("Navy", &valid);
		filltyp = find_fstyle("solid_fill", &valid);
		defined = TRUE;
		}

	/* Time parameters */
	tdef = (before)? otmin: otmax;
	tkey = TimeList[ivt].mplus;
	tmin = (early)? TimeList[iprev].mplus + DTween: tkey;
	tmax = (late)?  TimeList[inext].mplus - DTween: tkey;
	nt   = (tmax - tmin)/DTween + 1;
	idef = (tdef - tmin)/DTween;
	ikey = (tkey - tmin)/DTween;

	/* Size and position parameters */
	gxSetupTransform(DnMap);
	glMap2Vdc(dpos[X], dpos[Y], &vx, &vy);
	gxSetupTransform(DnExtrap);
	glVdc2Map(vx, vy, &x, &y);
	csize = 0.7*dy;
	nx   = MIN(nt, 6);
	ny   = (int) ceil((double) nt / (double) nx);
	nadj = NINT((float)nx * 1.3);
	while (ny>nadj)
		{
		nx  += 6;
		ny   = (int) ceil((double) nt / (double) nx);
		nadj = NINT((float)nx * 1.3);
		}
	xlen = nx*dx;
	ylen = ny*dy;
	if (before) x -= xlen;
	y += ylen/2;
	x  = MAX(x, DnExtrap->window.left);
	x  = MIN(x, DnExtrap->window.right - xlen);
	y  = MAX(y, DnExtrap->window.bottom + ylen);
	y  = MIN(y, DnExtrap->window.top);

#	ifdef DEBUG_PANELS
	pr_diag("Interp.Panels",
		"DnMap window dimensions: %.2f/%.2f to %.2f/%.2f\n",
		DnMap->window.left,  DnMap->window.bottom,
		DnMap->window.right, DnMap->window.top);
	pr_diag("Interp.Panels",
		"DnExtrap window dimensions: %.2f/%.2f to %.2f/%.2f\n",
		DnExtrap->window.left,  DnExtrap->window.bottom,
		DnExtrap->window.right, DnExtrap->window.top);
	pr_diag("Interp.Panels",
		"Extrap boxes: %.2f/%.2f to %.2f/%.2f\n",
		x, y, x+xlen, y+ylen);
#	endif /* DEBUG_PANELS */

	/* Build the menu */
	empty_metafile(ExtrapMeta);
	mplus = tmin;
	any   = FALSE;
	for (it=0, iy=0; iy<ny; iy++)
		{
		box.top    = y - iy*dy;
		box.bottom = box.top - dy;

		for (ix=0; ix<nx && it<nt; ix++, it++, mplus+=DTween)
			{
			box.left  = x + ix*dx;
			box.right = box.left + dx;

			/* Limit end to come at or after start */
			if (!start)
				{
				if (before)
					{
					if (mplus > otmax) continue;
					}
				else
					{
					if (mplus < otmin) continue;
					}
				}
			if (!EnableTrunc)
				{
				if (before)
					{
					if (it > ikey) continue;
					}
				else
					{
					if (it < ikey) continue;
					}
				}

			/* Colour boxes to indicate truncation, extension, etc. */
			if (it == idef)
				{
				fcolour = filldef;
				tcolour = textdef;
				}
			else if (it == ikey)
				{
				fcolour = keyclr;
				tcolour = textclr;
				}
			else if ((before && it<ikey) || (!before && it>ikey))
				{
				fcolour = fillclr;
				tcolour = textclr;
				}
			else
				{
				fcolour = backclr;
				tcolour = textclr;
				}

			/* Construct a button */
			if (it != ikey) any = TRUE;
			if (minutes_in_depictions())
				(void) sprintf(clab, "%s", hour_minute_string(0, mplus));
			else
				(void) sprintf(clab, "%.2d", mplus/60);
			(void) sprintf(cval, "%d", mplus);
			button = create_button("", cval, clab, &box);
			define_lspec(&button->lspec, edgeclr, 0, NULL, FALSE, 0.0, 0.0,
						(HILITE) 0);
			define_fspec(&button->fspec, fcolour, filltyp, NULL, FALSE, FALSE,
						 0.0, 0.0, (HILITE) 0);
			define_tspec(&button->tspec, tcolour, SafeFont, TRUE, csize, 0.0,
						 Hc, Vc, (HILITE) 0);
			add_item_to_metafile(ExtrapMeta, "button", "d", "", "",
								 (ITEM) button);
			}
		}
	if (!any) return FALSE;

	/* Now display it */
	if (!ExtrapShown)
		{
		/* Set display state on */
		define_dn_vis(DnExtrap, TRUE);
		ExtrapShown = TRUE;
		return present_extrap(TRUE);
		}
	else
		return present_all();
	}

/**********************************************************************/

LOGICAL	remove_extrap(void)

	{
	if (!ExtrapShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnExtrap, FALSE);
	ExtrapShown = FALSE;

	empty_metafile(ExtrapMeta);
	return present_all();
	}

/**********************************************************************/

LOGICAL	pick_extrap

	(
	POINT	pos,
	int		*mplus
	)

	{
	SET		set;
	BUTTON	button;
	int		i;
	float	vx, vy, x, y;

	if (!ExtrapShown) return FALSE;

	set = find_mf_set(ExtrapMeta, "button", "d", "", "");
	if (!set)          return FALSE;
	if (set->num <= 0) return FALSE;

	gxSetupTransform(DnMap);
	glMap2Vdc(pos[X], pos[Y], &vx, &vy);
	gxSetupTransform(DnExtrap);
	glVdc2Map(vx, vy, &x, &y);

	for (i=0; i<set->num; i++)
		{
		button = (BUTTON) set->list[i];
		if (inside_button_xy(button, x, y))
			{
			if (NotNull(mplus))
				if (sscanf(button->value, "%d", mplus) < 1) return FALSE;
			return TRUE;
			}
		}

	return FALSE;
	}

/**********************************************************************/

LOGICAL	present_extrap

	(
	LOGICAL	all
	)

	{
	if (!ExtrapShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnExtrap);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ a m b i g u o u s _ n o d e s                        *
*     r e l e a s e _ a m b i g u o u s _ n o d e s                    *
*     p i c k _ a m b i g u o u s _ n o d e                            *
*     p r e s e n t _ a m b i g u o u s _ n o d e s                    *
*                                                                      *
***********************************************************************/

LOGICAL	build_ambiguous_nodes

	(
	LCHAIN	lchain,
	LTYPE	ltype,
	int		inum,
	int		*inodes,
	int		mplus,
	POINT	dpos
	)

	{
	char	clab[80], cval[80];
	int		ii, *xmplus, in, ic;
	int		ntb, nxb, nyb, nsb, nadjb, nd, nte, nxe, nye, nadje;
	float	vx, vy, x, y, csize, xlen, ylen;
	int		it, ix, iy, ixs;
	LOGICAL	valid;
	BOX		box;
	BUTTON	button;
	COLOUR	fcolour, tcolour;

	/* Size of boxes for times */
	static	float	dx = 0;
	static	float	dy = 0;

	/* Presentation info for ambiguous node time choosing */
	static	COLOUR	edgeclr = 0;	/* Colour for edge of time boxes */
	static	COLOUR	keyclr  = 0;	/* Box colour for depiction time */
	static	COLOUR	filldef = 0;	/* Box colour for control times */
	static	COLOUR	fillclr = 0;	/* Box colour for times before depiction */
	static	COLOUR	backclr = 0;	/* Box colour for times after depiction */
	static	COLOUR	textclr = 0;	/* Text colour for time boxes */
	static	FSTYLE	filltyp = 0;	/* Style for interior of time boxes */
	static	LOGICAL	defined = FALSE;

	/* Set display colours (first time only) */
	if (!defined)
		{
		if (minutes_in_depictions())
			{
			dx = 50;
			dy = 25;
			}
		else
			{
			dx = 35;
			dy = 25;
			}
		edgeclr = find_colour("White", &valid);
		keyclr  = find_colour("Salmon", &valid);
		fillclr = find_colour("DarkSlateBlue", &valid);
		backclr = find_colour("SeaGreen", &valid);
		filldef = find_colour("Cyan", &valid);
		textclr = find_colour("White", &valid);
		filltyp = find_fstyle("solid_fill", &valid);
		defined = TRUE;
		}

	/* Return if no link chain or not enough ambiguous nodes */
	if (!lchain)   return FALSE;
	if (inum <= 1) return FALSE;

	/* Set the display times based on type of nodes */
	switch (ltype)
		{
		case LINK_REG:
			xmplus = INITMEM(int, inum);
			for (ii=0; ii<inum; ii++)
				xmplus[ii] = lchain->nodes[inodes[ii]]->mplus;
			break;
		case LINK_INT:
			xmplus = INITMEM(int, inum);
			for (ii=0; ii<inum; ii++)
				xmplus[ii] = lchain->interps[inodes[ii]]->mplus;
			break;
		default:
			return FALSE;
		}

	/* Set number of display boxes based on display times */
	ntb = nd = nte = 0;
	for (ii=0; ii<inum; ii++)
		{
		if      (xmplus[ii] < mplus) ntb++;
		else if (xmplus[ii] > mplus) nte++;
		else                         nd++;
		}

	/* Set rows and columns of display boxes */
	if (ntb > 0)
		{
		nxb   = MIN(ntb, 6);
		nyb   = (int) ceil((double) ntb / (double) nxb);
		nadjb = NINT((float)nxb * 1.3);
		while (nyb>nadjb)
			{
			nxb  += 6;
			nyb   = (int) ceil((double) ntb / (double) nxb);
			nadjb = NINT((float)nxb * 1.3);
			}
		if (nyb>1) nsb = nyb*nxb - ntb;
		else       nsb = 0;
		}
	else
		{
		nxb = nyb = nsb = 0;
		}
	if (nte > 0)
		{
		nxe   = MIN(nte, 6);
		nye   = (int) ceil((double) nte / (double) nxe);
		nadje = NINT((float)nxe * 1.3);
		while (nye>nadje)
			{
			nxe  += 6;
			nye   = (int) ceil((double) nte / (double) nxe);
			nadje = NINT((float)nxe * 1.3);
			}
		}
	else
		{
		nxe = nye = 0;
		}

	/* Size and position parameters */
	gxSetupTransform(DnMap);
	glMap2Vdc(dpos[X], dpos[Y], &vx, &vy);
	gxSetupTransform(DnLNode);
	glVdc2Map(vx, vy, &x, &y);
	csize = 0.7*dy;

	xlen = (nxb+nd+nxe)*dx;
	ylen = (nyb+nye)*dx;
	if (nyb>0 && nye>0) ylen -= dx;

	x -= 0.5*dx;
	if (nxb>0) x -= nxb*dx;
	y += 0.5*dy;
	if (nyb>0) y += (nyb-1)*dy;

	x  = MAX(x, DnLNode->window.left);
	x  = MIN(x, DnLNode->window.right - xlen);
	y  = MAX(y, DnLNode->window.bottom + ylen);
	y  = MIN(y, DnLNode->window.top);

#	ifdef DEBUG_PANELS
	pr_diag("Interp.Panels",
		"DnMap window dimensions: %.2f/%.2f to %.2f/%.2f\n",
		DnMap->window.left,  DnMap->window.bottom,
		DnMap->window.right, DnMap->window.top);
	pr_diag("Interp.Panels",
		"DnLNode window dimensions: %.2f/%.2f to %.2f/%.2f\n",
		DnLNode->window.left,  DnLNode->window.bottom,
		DnLNode->window.right, DnLNode->window.top);
	pr_diag("Interp.Panels",
		"Node boxes: %.2f/%.2f to %.2f/%.2f\n",
		x, y, x+xlen, y+ylen);
#	endif /* DEBUG_PANELS */

	/* Build the menu */
	empty_metafile(LNodeMeta);

	/* First display the times before the current depiction time */
	ii = 0;
	for (it=0, iy=0; iy<nyb; iy++)
		{
		box.top    = y - iy*dy;
		box.bottom = box.top - dy;
		if (iy==0) ixs = nsb;
		else       ixs = 0;

		for (ix=ixs; ix<nxb && it<ntb; ix++, it++)
			{
			box.left  = x + ix*dx;
			box.right = box.left + dx;

			/* Set box colour */
			fcolour = fillclr;
			tcolour = textclr;

			/* Adjust box colour for depiction/control times */
			in = which_lchain_node(lchain, LchainNode,    xmplus[ii]);
			ic = which_lchain_node(lchain, LchainControl, xmplus[ii]);
			if (in >= 0 || ic >=0)
				{
				fcolour = filldef;
				tcolour = textclr;
				}

			/* Construct a button */
			if (minutes_in_depictions())
				(void) sprintf(clab, "%s", hour_minute_string(0, xmplus[ii]));
			else
				(void) sprintf(clab, "%.2d", xmplus[ii]/60);
			(void) sprintf(cval, "%d", xmplus[ii]);
			ii++;
			button = create_button("", cval, clab, &box);
			define_lspec(&button->lspec, edgeclr, 0, NULL, FALSE, 0.0, 0.0,
						(HILITE) 0);
			define_fspec(&button->fspec, fcolour, filltyp, NULL, FALSE, FALSE,
						 0.0, 0.0, (HILITE) 0);
			define_tspec(&button->tspec, tcolour, SafeFont, TRUE, csize, 0.0,
						 Hc, Vc, (HILITE) 0);
			add_item_to_metafile(LNodeMeta, "button", "d", "", "",
								 (ITEM) button);
			}
		}

	/* Shift the display */
	x += nxb*dx;
	if (nyb>1) y -= (nyb-1)*dy;

	/* Next display the current depiction time */
	if (nd==1)
		{

		box.top    = y;
		box.bottom = box.top - dy;
		box.left   = x;
		box.right  = box.left + dx;

		/* Set box colour */
		fcolour = keyclr;
		tcolour = textclr;

		/* Construct a button */
		if (minutes_in_depictions())
			(void) sprintf(clab, "%s", hour_minute_string(0, mplus));
		else
			(void) sprintf(clab, "%.2d", mplus/60);
		(void) sprintf(cval, "%d", mplus);
		ii++;
		button = create_button("", cval, clab, &box);
		define_lspec(&button->lspec, edgeclr, 0, NULL, FALSE, 0.0, 0.0,
					(HILITE) 0);
		define_fspec(&button->fspec, fcolour, filltyp, NULL, FALSE, FALSE,
					 0.0, 0.0, (HILITE) 0);
		define_tspec(&button->tspec, tcolour, SafeFont, TRUE, csize, 0.0,
					 Hc, Vc, (HILITE) 0);
		add_item_to_metafile(LNodeMeta, "button", "d", "", "", (ITEM) button);
		}

	/* Shift the display */
	x += nd*dx;

	/* Last display the times after the current depiction time */
	for (it=0, iy=0; iy<nye; iy++)
		{
		box.top    = y - iy*dy;
		box.bottom = box.top - dy;

		for (ix=0; ix<nxe && it<nte; ix++, it++)
			{
			box.left  = x + ix*dx;
			box.right = box.left + dx;

			/* Set box colour */
			fcolour = backclr;
			tcolour = textclr;

			/* Adjust box colour for depiction/control times */
			in = which_lchain_node(lchain, LchainNode,    xmplus[ii]);
			ic = which_lchain_node(lchain, LchainControl, xmplus[ii]);
			if (in >= 0 || ic >=0)
				{
				fcolour = filldef;
				tcolour = textclr;
				}

			/* Construct a button */
			if (minutes_in_depictions())
				(void) sprintf(clab, "%s", hour_minute_string(0, xmplus[ii]));
			else
				(void) sprintf(clab, "%.2d", xmplus[ii]/60);
			(void) sprintf(cval, "%d", xmplus[ii]);
			ii++;
			button = create_button("", cval, clab, &box);
			define_lspec(&button->lspec, edgeclr, 0, NULL, FALSE, 0.0, 0.0,
						(HILITE) 0);
			define_fspec(&button->fspec, fcolour, filltyp, NULL, FALSE, FALSE,
						 0.0, 0.0, (HILITE) 0);
			define_tspec(&button->tspec, tcolour, SafeFont, TRUE, csize, 0.0,
						 Hc, Vc, (HILITE) 0);
			add_item_to_metafile(LNodeMeta, "button", "d", "", "",
								 (ITEM) button);
			}
		}

	/* Now display it */
	FREEMEM(xmplus);
	if (!LNodeShown)
		{
		/* Set display state on */
		define_dn_vis(DnLNode, TRUE);
		LNodeShown = TRUE;
		return present_ambiguous_nodes(TRUE);
		}
	else
		return present_all();
	}

/**********************************************************************/

LOGICAL	release_ambiguous_nodes(void)

	{
	if (!LNodeShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnLNode, FALSE);
	LNodeShown = FALSE;

	empty_metafile(LNodeMeta);
	return present_all();
	}

/**********************************************************************/

LOGICAL	pick_ambiguous_node

	(
	POINT	pos,
	int		*mplus
	)

	{
	SET		set;
	BUTTON	button;
	int		i;
	float	vx, vy, x, y;

	if (!LNodeShown) return FALSE;

	set = find_mf_set(LNodeMeta, "button", "d", "", "");
	if (!set)          return FALSE;
	if (set->num <= 0) return FALSE;

	gxSetupTransform(DnMap);
	glMap2Vdc(pos[X], pos[Y], &vx, &vy);
	gxSetupTransform(DnLNode);
	glVdc2Map(vx, vy, &x, &y);

	for (i=0; i<set->num; i++)
		{
		button = (BUTTON) set->list[i];
		if (inside_button_xy(button, x, y))
			{
			if (NotNull(mplus))
				if (sscanf(button->value, "%d", mplus) < 1) return FALSE;
			return TRUE;
			}
		}

	return FALSE;
	}

/**********************************************************************/

LOGICAL	present_ambiguous_nodes

	(
	LOGICAL	all
	)

	{
	if (!LNodeShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnLNode);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    l i n k _ f r a m e _ s t a t u s                                 *
*                                                                      *
*    Get the link status of an individual frame.                       *
*                                                                      *
***********************************************************************/

STRING	link_frame_status

	(
	DFLIST	*dfld,
	int		ivt
	)

	{
	if (!dfld)                          return "DELETED";
	if (!dfld->there)                   return "DELETED";
	if (ivt < 0)                        return "DELETED";
	if (ivt >= NumTime)                 return "DELETED";

	if (same(dfld->element, "MASTER_LINK"))
		return master_link_frame_status(dfld, ivt);

	if (!dfld->frames)                  return "DELETED";
	if (!dfld->frames[ivt].meta)        return "DELETED";
	if (!dfld->dolink && !tdep_special(dfld->tdep))
										return "NONE";
	if (dfld->interp && dfld->intlab)   return "INTERP";
	if (dfld->interp)                   return "FIELD";
	if (dfld->linked)                   return "LINKED";
	if (dfld->nchain <= 0)              return "NONE";

	switch (dfld->editor)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	return spline_link_frame_status(dfld, ivt);

		case FpaC_DISCRETE:
		case FpaC_WIND:			return area_link_frame_status(dfld, ivt);

		case FpaC_LINE:			return line_link_frame_status(dfld, ivt);

		case FpaC_SCATTERED:
		case FpaC_LCHAIN:		return "NONE";

		default:				return "NONE";
		}
	}

/***********************************************************************
*                                                                      *
*     v e r i f y _ l i n k s                                          *
*     v e r i f y _ d f i e l d_ l i n k s                             *
*     r e a d y _ l i n k s                                            *
*     r e a d y _ d f i e l d_ l i n k s                               *
*                                                                      *
***********************************************************************/

LOGICAL	verify_links

	(
	LOGICAL	report	/* report status if changed */
	)

	{
	int		idfld;
	DFLIST	*dfld;
	LOGICAL	any, status;

	/* Check field links */
	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;
		status = verify_dfield_links(dfld, report);
		if (status) any = TRUE;
		}

	/* Check master links */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld   = MasterLinks + idfld;
		status = verify_dfield_links(dfld, TRUE);
		if (status) any = TRUE;
		}

	return any;
	}

/**********************************************************************/

LOGICAL	verify_dfield_links

	(
	DFLIST	*dfld,
	LOGICAL	report	/* report status if changed */
	)

	{
	if (!dfld)         return FALSE;
	if (!dfld->dolink) return TRUE;

	switch (dfld->editor)
		{
		case FpaCnoMacro:		return verify_master_links(dfld, report);

		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	return verify_spline_links(dfld, report);

		case FpaC_DISCRETE:
		case FpaC_WIND:			return verify_area_links(dfld, report);

		case FpaC_LINE:			return verify_line_links(dfld, report);

		case FpaC_SCATTERED:
		case FpaC_LCHAIN:		return FALSE;
		

		default:				return FALSE;
		}
	}

/**********************************************************************/

LOGICAL	ready_links

	(
	LOGICAL	report	/* report status if changed */
	)

	{
	int		idfld;
	DFLIST	*dfld;
	LOGICAL	any, status;

	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;
		status = ready_dfield_links(dfld, report);
		if (status) any = TRUE;
		}

	return any;
	}

/**********************************************************************/

LOGICAL	ready_dfield_links

	(
	DFLIST	*dfld,
	LOGICAL	report	/* report status if changed */
	)

	{
	if (!dfld)         return FALSE;
	if (!dfld->dolink) return TRUE;

	switch (dfld->editor)
		{
		case FpaCnoMacro:		return ready_master_links(dfld, report);

		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	return ready_spline_links(dfld, report);

		case FpaC_DISCRETE:
		case FpaC_WIND:			return ready_area_links(dfld, report);

		case FpaC_LINE:			return ready_line_links(dfld, report);

		case FpaC_SCATTERED:
		case FpaC_LCHAIN:		return FALSE;

		default:				return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     v e r i f y _ i n t e r p                                        *
*     v e r i f y _ d f i e l d_ i n t e r p                           *
*                                                                      *
***********************************************************************/

LOGICAL	verify_interp

	(
	LOGICAL	report,	/* report status if changed */
	LOGICAL	purge	/* purge interpolated files if inconsistent */
	)

	{
	int		idfld;
	DFLIST	*dfld;
	LOGICAL	any, status;

	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;
		status = verify_dfield_interp(dfld, report, purge);
		if (status) any = TRUE;
		}

	return any;
	}

/**********************************************************************/

LOGICAL	verify_dfield_interp

	(
	DFLIST	*dfld,
	LOGICAL	report,	/* report status if changed */
	LOGICAL	purge	/* purge interpolated files if field not interpolated */
	)

	{
	STRING			elem, levl, fname, *tlist, vfirst, vtime, vtimef, vtimel;
	FLD_DESCRIPT	fd;
	int				nt, it, ivt, ichain;
	int				nvt, mfirst, fdif, ldif, mstart, mend;
	LOGICAL			exist, consistent, ok;

	if (!dfld)                                      return FALSE;
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return TRUE;

	elem = dfld->element;
	levl = dfld->level;

	/* Check valid times for existing interpolated files */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_SOURCE_NAME,	"interp",
					FpaF_ELEMENT_NAME,	elem,
					FpaF_LEVEL_NAME,	levl,
					FpaF_END_OF_LIST)) return FALSE;
	nt = source_valid_time_list(&fd, FpaC_TIMEDEP_ANY, &tlist);
	exist = (LOGICAL) (nt > 0);
	consistent = (LOGICAL) (exist && dfld->linked && dfld->interp &&
							dfld->saved);

	/* Set time of first depiction in sequence */
	nvt    = first_depict_time();
	mfirst = TimeList[nvt].mplus;
	vfirst = TimeList[nvt].jtime;

#	ifdef DEBUG_INTERP
		pr_info("Timelink",
			"[verify_dfield_interp] Field: %s %s\n", dfld->element, dfld->level);
	if (consistent)
		pr_info("Timelink",
			"[verify_dfield_interp]  Interp files: %d  Consistent!\n", nt);
	else
		{
		pr_info("Timelink",
			"[verify_dfield_interp]  Interp files: %d  Not consistent!\n", nt);
		if (!dfld->linked)
			pr_info("Timelink", "[verify_dfield_interp]   Not linked!\n");
		if (!dfld->interp)
			pr_info("Timelink", "[verify_dfield_interp]   Not interpolated!\n");
		if (!dfld->saved)
			pr_info("Timelink", "[verify_dfield_interp]   Not saved!\n");
		}
#	endif /* DEBUG_INTERP */

	/* Make sure the interpolated files match the depiction sequence */
	if (exist && consistent)
		{

		/* Check all interpolated files for daily/static fields */
		if (tdep_special(dfld->tdep))
			{
			for (it=0; it<nt; it++)
				{
				vtime = tlist[it];
				if (!blank(vtime))
					{

					/* Check for a matching depiction file */
					for (ivt=0; ivt<NumTime; ivt++)
						{
						if (dfld->frames && dfld->frames[ivt].meta)
							{
							/* Does it match? */
							if (same(vtime, TimeList[ivt].jtime)) break;
							}
						}
					if (ivt >= NumTime) consistent = FALSE;
					}
				else
					{
					consistent = FALSE;
					}
				}
			}

		/* Check first and last interpolated files for regular fields */
		else
			{
			vtimef = tlist[0];
			vtimel = tlist[nt-1];

#			ifdef DEBUG_INTERP
			pr_info("Timelink",
				"[verify_dfield_interp]  Checking times: %s and %s\n",
				vtimef, vtimel);
#			endif /* DEBUG_INTERP */

			if (!blank(vtimef) && !blank(vtimel))
				{

				/* First file must match with first depiction time */
				fdif = calc_prog_time_minutes(vfirst, vtimef, &ok);
				for (ivt=0; ivt<NumTime; ivt++)
					{
					if (dfld->frames && dfld->frames[ivt].meta) break;
					}
				if (ivt >= NumTime)                     consistent = FALSE;
				if (!same(vtimef, TimeList[ivt].jtime)) consistent = FALSE;

#				ifdef DEBUG_INTERP
				pr_info("Timelink",
					"[verify_dfield_interp]  Matching time: %s with %s (%d)\n",
					vtimef, TimeList[ivt].jtime, ivt);
#				endif /* DEBUG_INTERP */

				/* Last file must match with last depiction time */
				ldif = calc_prog_time_minutes(vfirst, vtimel, &ok);
				for (ivt=NumTime-1; ivt>=0; ivt--)
					{
					if (dfld->frames && dfld->frames[ivt].meta) break;
					}
				if (ivt < 0)                            consistent = FALSE;
				if (!same(vtimel, TimeList[ivt].jtime)) consistent = FALSE;

#				ifdef DEBUG_INTERP
				pr_info("Timelink",
					"[verify_dfield_interp]  Matching time: %s with %s (%d)\n",
					vtimel, TimeList[ivt].jtime, ivt);
#				endif /* DEBUG_INTERP */

				/* Check for consistent link chains */
				if (dfld->nchain >= 1)
					{

					/* Find earliest start time from link chains */
					mstart = TimeList[NumTime-1].mplus;
					for (ichain=0; ichain<dfld->nchain; ichain++)
						{
						if (dfld->chains[ichain]->splus < mstart)
							mstart = dfld->chains[ichain]->splus;
						}

					/* Find latest end time from link chains */
					mend = TimeList[0].mplus;
					for (ichain=0; ichain<dfld->nchain; ichain++)
						{
						if (dfld->chains[ichain]->eplus > mend)
							mend = dfld->chains[ichain]->eplus;
						}

					/* Compare with times from interpolations */
					switch (dfld->editor)
						{
						/* Spline type fields ...              */
						/*  start/end times must match exactly */
						case FpaC_VECTOR:
						case FpaC_CONTINUOUS:
							if ((mstart-mfirst) != fdif) consistent = FALSE;
							if ((mend-mfirst)   != ldif) consistent = FALSE;
							break;

						/* Object type fields ...                            */
						/*  start/end times must be between first/last times */
						case FpaC_DISCRETE:
						case FpaC_WIND:
						case FpaC_LINE:
							if ((mstart-mfirst) < fdif
								|| (mstart-mfirst) > ldif) consistent = FALSE;
							if ((mend-mfirst) < fdif
								|| (mend-mfirst) > ldif)   consistent = FALSE;
							break;

						case FpaC_SCATTERED:
						case FpaC_LCHAIN:
						default:
							consistent = FALSE;
							break;
						}
					}
				}
			else
				{
				consistent = FALSE;
				}
			}
		}

	/* Reset interp states if not consistent */
	if (!consistent)
		{
		if (tdep_special(dfld->tdep))
			{
			if (!dfld->snaps)
				{

#				ifdef DEVELOPMENT
				if (dfld->reported)
					pr_info("Editor.Reported",
						"In verify_dfield_interp() - T %s %s\n",
						dfld->element, dfld->level);
				else
					pr_info("Editor.Reported",
						"In verify_dfield_interp() - F %s %s\n",
						dfld->element, dfld->level);
#				endif /* DEVELOPMENT */

				dfld->interp   = FALSE;
				dfld->intlab   = FALSE;
				dfld->saved    = FALSE;
				dfld->reported = FALSE;
				}
			}
		else
			{
			if (!dfld->tweens)
				{

#				ifdef DEVELOPMENT
				if (dfld->reported)
					pr_info("Editor.Reported",
						"In verify_dfield_interp() - T %s %s\n",
						dfld->element, dfld->level);
				else
					pr_info("Editor.Reported",
						"In verify_dfield_interp() - F %s %s\n",
						dfld->element, dfld->level);
#				endif /* DEVELOPMENT */

				dfld->interp   = FALSE;
				dfld->intlab   = FALSE;
				dfld->saved    = FALSE;
				dfld->reported = FALSE;
				}
			}
		if (report) link_status(dfld);
		}

	/* Purge inconsistent files if requested */
	if (exist && purge && !dfld->interp)
		{
		(void) pr_diag("Timelink",
			"Purging %s %s interpolations\n", levl, elem);
		for (it=0; it<nt; it++)
			{
			(void) set_fld_descript(&fd,
							FpaF_SOURCE_NAME,	"interp",
							FpaF_VALID_TIME,	tlist[it],
							FpaF_ELEMENT_NAME,	elem,
							FpaF_LEVEL_NAME,	levl,
							FpaF_END_OF_LIST);

			/* Check for new format FPA Metafiles */
			fname = construct_meta_filename(&fd);
			(void) remove_file(fname, NULL);

			/* Check for old format FPA Metafiles */
			fname = build_meta_filename(&fd);
			(void) remove_file(fname, NULL);
			}
		exist = FALSE;
		}

	/* Free list of valid times and return consistency check */
	(void) source_valid_time_list_free(&tlist, nt);
	return consistent;
	}

/***********************************************************************
*                                                                      *
*     r e l e a s e _ i n t e r p                                      *
*     r e l e a s e _ d f i e l d _ i n t e r p                        *
*     r e l e a s e _ d f i e l d _ s n a p s                          *
*     r e l e a s e _ d f i e l d _ t w e e n s                        *
*     a c q u i r e _ i n t e r p                                      *
*     a c q u i r e _ d f i e l d _ i n t e r p                        *
*     a c q u i r e _ d f i e l d _ s n a p s                          *
*     a c q u i r e _ d f i e l d _ t w e e n s                        *
*     p r e p a r e _ i n t e r p                                      *
*     p r e p a r e _ d f i e l d _ i n t e r p                        *
*     p r e p a r e _ d f i e l d _ s n a p s                          *
*     p r e p a r e _ d f i e l d _ t w e e n s                        *
*                                                                      *
***********************************************************************/

LOGICAL	release_interp(void)

	{
	int		idfld;
	DFLIST	*dfld;
	LOGICAL	any, status;

	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;
		status = release_dfield_interp(dfld);
		if (status) any = TRUE;
		}

	return any;
	}

/**********************************************************************/

LOGICAL	release_dfield_interp

	(
	DFLIST	*dfld
	)

	{
	if (!dfld)                                      return FALSE;
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return TRUE;

	if (tdep_special(dfld->tdep))
			return release_dfield_snaps(dfld);
	else	return release_dfield_tweens(dfld);
	}

/**********************************************************************/

LOGICAL	release_dfield_snaps

	(
	DFLIST	*dfld
	)

	{
	int		ivt;

	if (!dfld)                                      return FALSE;
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return TRUE;
	if (!dfld->snaps)                               return TRUE;

	for (ivt=0; ivt<NumTime; ivt++)
		{
		destroy_field(dfld->snaps[ivt]);
		destroy_set(dfld->slabs[ivt]);
		}
	FREEMEM(dfld->snaps);
	FREEMEM(dfld->slabs);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	release_dfield_tweens

	(
	DFLIST	*dfld
	)

	{
	int		itween;

	if (!dfld)         return FALSE;
	if (!dfld->dolink) return TRUE;
	if (!dfld->tweens) return TRUE;

	for (itween=0; itween<NumTween; itween++)
		{
		destroy_field(dfld->tweens[itween]);
		destroy_set(dfld->tlabs[itween]);
		}
	FREEMEM(dfld->tweens);
	FREEMEM(dfld->tlabs);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	acquire_interp(void)

	{
	int		idfld, ivt, itween;
	DFLIST	*dfld;
	LOGICAL	any, status;

	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;

		/* Retrieve static or daily field snapshots */
		if (tdep_special(dfld->tdep))
			{
			for (ivt=0; ivt<NumTime; ivt++)
				{
				status = acquire_dfield_interp(dfld, ivt);
				if (status) any = TRUE;
				}
			}

		/* Retrieve regular field interpolations */
		else
			{
			for (itween=0; itween<NumTween; itween++)
				{
				status = acquire_dfield_interp(dfld, itween);
				if (status) any = TRUE;
				}
			}
		}

	return any;
	}

/**********************************************************************/

LOGICAL	acquire_dfield_interp

	(
	DFLIST	*dfld,
	int		it
	)

	{
	if (!dfld)                                      return FALSE;
	if (!dfld->there)                               return TRUE;
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return TRUE;
	if (!dfld->interp)                              return TRUE;

	if (tdep_special(dfld->tdep))
			return acquire_dfield_snaps(dfld, it);
	else	return acquire_dfield_tweens(dfld, it);
	}

/**********************************************************************/

LOGICAL	acquire_dfield_snaps

	(
	DFLIST	*dfld,
	int		it
	)

	{
	STRING			elem, levl, vtime, file;
	int				ivt;
	FLD_DESCRIPT    fd;
	METAFILE		meta;
	FIELD			fld;
	SET				labels;

	if (!dfld)                     return FALSE;
	if (!dfld->there)              return TRUE;
	if (!dfld->interp)             return TRUE;
	if (!tdep_special(dfld->tdep)) return TRUE;

	elem = dfld->element;
	levl = dfld->level;

	/* Retrieve static or daily field snapshots */
	(void) prepare_dfield_snaps(dfld);

	/* See if snapshot is already in */
	ivt = it;
	if (dfld->snaps[ivt]) return TRUE;
	vtime = TimeList[ivt].jtime;

	/* Read in existing interpolated file */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_SOURCE_NAME,		"interp",
					FpaF_VALID_TIME,		vtime,
					FpaF_ELEMENT_NAME,		elem,
					FpaF_LEVEL_NAME,		levl,
					FpaF_END_OF_LIST)) return FALSE;
	file = check_meta_filename(&fd);
	if (blank(file))
		{
		if (ivt>0 && ivt<NumTime-1) return FALSE;
		file = find_meta_filename(&fd);
		if (blank(file))                return FALSE;
		}
	meta = read_metafile(file,MapProj);
	if (!meta) return FALSE;

	(void) pr_diag("Timelink", "Acquiring %s %s snaps for %s\n",
		dfld->level, dfld->element, vtime);

	fld    = NullFld;
	labels = NullSet;
	switch (dfld->editor)
		{
		case FpaC_VECTOR:
			fld    = take_mf_field(meta, "surface", NULL, "v", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_CONTINUOUS:
			fld    = take_mf_field(meta, "surface", NULL, "a", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_DISCRETE:
		case FpaC_WIND:
			fld    = take_mf_field(meta, "set", "area","b", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_LINE:
			fld    = take_mf_field(meta, "set", "curve", "c", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_SCATTERED:
			fld    = take_mf_field(meta, "set", "spot", "d", elem, levl);
			labels = NullSet;
			break;

		case FpaC_LCHAIN:
			fld    = take_mf_field(meta, "set", "lchain", "l", elem, levl);
			labels = NullSet;
			break;
		}
	destroy_metafile(meta);

	setup_fld_presentation(fld, "FPA");
	setup_set_presentation(labels, elem, levl, "FPA");
	highlight_field(fld, 0);
	highlight_set(labels, 0);
	dfld->snaps[ivt] = fld;
	dfld->slabs[ivt] = labels;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	acquire_dfield_tweens

	(
	DFLIST	*dfld,
	int		it
	)

	{
	STRING			elem, levl, vtime, file;
	int				itween, ifirst;
	int				pyear, pjday, phour, pminute;
	FLD_DESCRIPT    fd;
	METAFILE		meta;
	FIELD			fld;
	SET				labels;

	if (!dfld)         return FALSE;
	if (!dfld->there)  return TRUE;
	if (!dfld->dolink) return TRUE;
	if (!dfld->interp) return TRUE;
	if (!tdep_normal(dfld->tdep)) return TRUE;

	elem = dfld->element;
	levl = dfld->level;

	/* Retrieve regular field interpolations */
	(void) prepare_dfield_tweens(dfld);

	/* See if interpolation is already in */
	itween = it;
	if (dfld->tweens[itween]) return TRUE;

	/* Build valid timestamp for constructing metafile names */
	ifirst  = first_depict_time();
	pyear   = Syear;
	pjday   = Sjday;
	phour   = Shour;
	pminute = Sminute + TimeList[ifirst].mplus + itween*DTween;
	tnorm(&pyear,&pjday,&phour,&pminute,NullInt);
	vtime = build_tstamp(pyear, pjday, phour, pminute, FALSE,
				minutes_in_depictions());

	/* Read in existing interpolated file */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_SOURCE_NAME,		"interp",
					FpaF_VALID_TIME,		vtime,
					FpaF_ELEMENT_NAME,		elem,
					FpaF_LEVEL_NAME,		levl,
					FpaF_END_OF_LIST)) return TRUE;
	file = check_meta_filename(&fd);
	if (blank(file)) return TRUE;
	meta = read_metafile(file,MapProj);
	if (!meta)       return TRUE;

	(void) pr_diag("Timelink", "Acquiring %s %s interp for %s\n",
		dfld->level, dfld->element, vtime);

	fld    = NullFld;
	labels = NullSet;
	switch (dfld->editor)
		{
		case FpaC_VECTOR:
			fld    = take_mf_field(meta, "surface", NULL, "v", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_CONTINUOUS:
			fld    = take_mf_field(meta, "surface", NULL, "a", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_DISCRETE:
		case FpaC_WIND:
			fld    = take_mf_field(meta, "set", "area","b", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_LINE:
			fld    = take_mf_field(meta, "set", "curve", "c", elem, levl);
			labels = take_mf_set(meta, "spot", "d", elem, levl);
			break;

		case FpaC_SCATTERED:
			fld    = take_mf_field(meta, "set", "spot", "d", elem, levl);
			labels = NullSet;
			break;
		}
	destroy_metafile(meta);

	setup_fld_presentation(fld, "FPA");
	setup_set_presentation(labels, elem, levl, "FPA");
	highlight_field(fld, 0);
	highlight_set(labels, 0);
	dfld->tweens[itween] = fld;
	dfld->tlabs[itween]  = labels;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	prepare_interp(void)

	{
	int		idfld;
	DFLIST	*dfld;
	LOGICAL	any, status;

	any = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld   = DfldList + idfld;
		status = prepare_dfield_interp(dfld);
		if (status) any = TRUE;
		}

	return any;
	}

/**********************************************************************/

LOGICAL	prepare_dfield_interp

	(
	DFLIST	*dfld
	)

	{
	if (!dfld)                                      return FALSE;
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return TRUE;

	if (tdep_special(dfld->tdep))
			return prepare_dfield_snaps(dfld);
	else	return prepare_dfield_tweens(dfld);
	}

/**********************************************************************/

LOGICAL	prepare_dfield_snaps

	(
	DFLIST	*dfld
	)

	{
	int		ivt;

	if (!dfld)                     return FALSE;
	if (!dfld->there)              return TRUE;
	if (!tdep_special(dfld->tdep)) return TRUE;
	if (dfld->snaps)               return TRUE;

	dfld->snaps = INITMEM(FIELD,NumTime);
	dfld->slabs = INITMEM(SET, NumTween);
	for (ivt=0; ivt<NumTime; ivt++)
		{
		dfld->snaps[ivt] = NullFld;
		dfld->slabs[ivt] = NullSet;
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	prepare_dfield_tweens

	(
	DFLIST	*dfld
	)

	{
	int		itween;

	if (!dfld)                    return FALSE;
	if (!dfld->there)             return TRUE;
	if (!dfld->dolink)            return TRUE;
	if (!tdep_normal(dfld->tdep)) return TRUE;
	if (dfld->tweens)             return TRUE;

	dfld->tweens = INITMEM(FIELD, NumTween);
	dfld->tlabs  = INITMEM(SET, NumTween);
	for (itween=0; itween<NumTween; itween++)
		{
		dfld->tweens[itween] = NullFld;
		dfld->tlabs[itween]  = NullSet;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     b o r r o w _ l i n k s                                          *
*                                                                      *
***********************************************************************/

LOGICAL	borrow_links

	(
	DFLIST	*dfld,
	STRING	elem,
	STRING	level,
	LOGICAL	copy
	)

	{
	int		idfld;
	DFLIST	*linkto, *df;

	if (!dfld) return FALSE;

	if (blank(elem) || same(elem, "SELF"))
		{
		/* Unlink - keep copied links */
		dfld->linkto = NULL;
		if (copy)
			{
			(void) ready_dfield_links(dfld, TRUE);
			dfld->reported = FALSE;
			link_status(dfld);
			(void) save_links();
			}
		return TRUE;
		}

	if (same(elem, "MASTER_LINK"))
		{
		/* Link to master */
		linkto = find_master_dfield(dfld->group);
		}

	else
		{
		/* Link to another field */
		linkto = find_dfield(elem, level);

		/* Can't link to one that's already linked to another */
		if (linkto && linkto->linkto) return FALSE;
		}

	/* Can't link one that already has others linked to it */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		df = DfldList + idfld;
		if (df->linkto == dfld) return FALSE;
		}

	/* Copy and recompute new links (Blow away existing links) */
	dfld->linkto = linkto;
	if (copy)
		{
		(void) release_unlinked();
		(void) release_links();
		(void) copy_dfield_links(dfld, linkto);
		(void) extract_links();
		(void) extract_unlinked();
		(void) present_all();
		}
	else
		(void) clear_dfield_links(dfld, FALSE);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     t i m e l i n k _ c h e c k                                      *
*     t i m e l i n k _ e d i t                                        *
*                                                                      *
***********************************************************************/

LOGICAL	timelink_check(void)

	{
	if (!LinkShown) return FALSE;

	if (EditTime < 0) return FALSE;

	/* Locate field info buffer */
	if (!ActiveDfld) return FALSE;
	if (!ActiveDfld->dolink)
	    {
		put_message("link-not-sup", CurrElement, CurrLevel);
	    return FALSE;
	    }
	if (tdep_special(ActiveDfld->tdep))
	    {
		put_message("link-not-req", CurrElement, CurrLevel);
	    return FALSE;
	    }

	return TRUE;
	}

/**********************************************************************/

LOGICAL	timelink_edit

	(
	STRING	mode
	)

	{
#	ifdef DEBUG
	(void) pr_diag("Timelink",
		"[timelink_edit] EditMode: %s  mode: %s\n", EditMode, mode);
#	endif /* DEBUG */

	/* Ignore accept and undo modes */
	/* >>>>> let through undo for now!!!
	if (same(mode, "confirm") || same(mode, "undo"))
		{
		return timelink_check();
		}
	<<<<<< */

	/* Ignore accept mode */
	if (same(mode, "confirm"))
		{
		return timelink_check();
		}

	/* >>>>> let through clear for now!!!
	if (same(mode, "clear"))
		{
		(void) clear_links(TRUE);
		link_status(ActiveDfld);
		(void) save_links();
		return timelink_check();
		}
	<<<<<< */

	if (!timelink_check()) return FALSE;

	/* Handle "forward" link */
	if (same(EditMode, "FORWARD"))
		{
		switch (CurrEditor)
			{
			case FpaCnoMacro:		(void) link_master(mode, TRUE);	break;

			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:	(void) link_spline(mode, TRUE);	break;

			case FpaC_DISCRETE:
			case FpaC_WIND:			(void) link_area(mode, TRUE);	break;

			case FpaC_LINE:			(void) link_line(mode, TRUE);	break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				put_message("link-not-sup", CurrElement, CurrLevel);
			}
		}

	/* Handle "backward" link */
	else if (same(EditMode, "BACKWARD"))
		{
		switch (CurrEditor)
			{
			case FpaCnoMacro:		(void) link_master(mode, FALSE);	break;

			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:	(void) link_spline(mode, FALSE);	break;

			case FpaC_DISCRETE:
			case FpaC_WIND:			(void) link_area(mode, FALSE);		break;

			case FpaC_LINE:			(void) link_line(mode, FALSE);		break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				put_message("link-not-sup", CurrElement, CurrLevel);
			}
		}

	/* Handle "move" link */
	else if (same(EditMode, "MOVE"))
		{
		switch (CurrEditor)
			{
			case FpaCnoMacro:		(void) mvlink_master(mode);	break;

			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:	(void) mvlink_spline(mode);	break;

			case FpaC_DISCRETE:
			case FpaC_WIND:			(void) mvlink_area(mode);	break;

			case FpaC_LINE:			(void) mvlink_line(mode);	break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				put_message("link-not-sup", CurrElement, CurrLevel);
			}
		}

	/* Handle "merge" link */
	else if (same(EditMode, "MERGE"))
		{
		switch (CurrEditor)
			{
			case FpaCnoMacro:		(void) mrglink_master(mode,
											EditVal[1], EditVal[2],
											EditVal[3], EditVal[4]);	break;

			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:	(void) mrglink_spline(mode,
											EditVal[1], EditVal[2],
											EditVal[3], EditVal[4]);	break;

			case FpaC_DISCRETE:
			case FpaC_WIND:			(void) mrglink_area(mode,
											EditVal[1], EditVal[2],
											EditVal[3], EditVal[4]);	break;

			case FpaC_LINE:			(void) mrglink_line(mode,
											EditVal[1], EditVal[2],
											EditVal[3], EditVal[4]);	break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				put_message("link-not-sup", CurrElement, CurrLevel);
			}
		}

	/* Handle "delink" link */
	else if (same(EditMode, "DELINK"))
		{
		switch (CurrEditor)
			{
			case FpaCnoMacro:	(void) delink_master(mode, EditVal[0]);	break;

			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:
								(void) delink_spline(mode, EditVal[0]);	break;

			case FpaC_DISCRETE:
			case FpaC_WIND:		(void) delink_area(mode, EditVal[0]);	break;

			case FpaC_LINE:		(void) delink_line(mode, EditVal[0]);	break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				put_message("link-not-sup", CurrElement, CurrLevel);
			}
		}

	/* Unknown function */
	else
		{
		put_message("link-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c l o s e s t _ l i n k _ c h a i n                              *
*     c l o s e s t _ l i n k _ e n d                                  *
*                                                                      *
***********************************************************************/

static	LOGICAL	duplicate_tracks(LINE, LINE, float);

/**********************************************************************/

static	LOGICAL	duplicate_tracks

	(
	LINE	track1,
	LINE	track2,
	float	dtol
	)

	{
	int		ii;

	if (!track1)            return FALSE;
	if (track1->numpts < 1) return FALSE;
	if (!track2)            return FALSE;
	if (track2->numpts < 1) return FALSE;

	if (track1->numpts != track2->numpts) return FALSE;

	/* Compare points in each track */
	for (ii=0; ii<track1->numpts; ii++)
		{
		if (point_dist(track1->points[ii], track2->points[ii]) > dtol)
			return FALSE;
		}

	/* Return TRUE if all track points are close together */
	return TRUE;
	}

/**********************************************************************/

int		closest_link_chain

	(
	int		pvt, /* preferred time for ambiguous nodes */
	POINT	pos,
	DFLIST	*dfld,
	float	dtol,
	int		*member,
	LTYPE	*ltype,
	LOGICAL	*ambig,
	LOGICAL	*dupl
	)

	{
	int		ichain, ictl, ivt, il, intp, bchain;
	int		pplus, mplus, mdiff, bplus, bdiff, bmem;
	LTYPE	btype;
	POINT	lpos;
	float	dx, dy, dist;
	LCHAIN	chain, xchain;

	/* >>>>> testing <<<<< */
	int	iix;
	/* >>>>> testing <<<<< */

	if (pvt < 0) pplus = 0;
	else         pplus = TimeList[pvt].mplus;

	/* Search for closest link node on any link chain, on any keyframe */
	bchain = -1;
	bplus  = pplus;
	bdiff  = 0;
	bmem   = -1;
	btype  = LINK_REG;
	if (ambig) *ambig = TRUE;
	if (dupl)  *dupl  = FALSE;

	/* First search control nodes (if present) */
	for (ichain=dfld->nchain-1; ichain>=0; ichain--)
		{
		chain = dfld->chains[ichain];

		/* Search control nodes - give preference to preferred time */
		if (NotNull(chain->nodes) && (chain->lnum > 0))
			{
			for (ictl=0; ictl<chain->lnum; ictl++)
				{
				if (!chain->nodes[ictl]->there)                  continue;
				if (chain->nodes[ictl]->ltype != LchainControl)  continue;
				copy_point(lpos, chain->nodes[ictl]->node);
				if (!inside_map_def(&MapProj->definition, lpos)) continue;

				dx   = pos[X] - lpos[X];
				dy   = pos[Y] - lpos[Y];
				dist = hypot(dx, dy);
				if (dist > dtol/2) continue;

				mplus = chain->nodes[ictl]->mplus;
				mdiff = abs(mplus - pplus);

				if (bchain < 0)
					{
					bchain = ichain;
					bplus  = mplus;
					bdiff  = mdiff;
					bmem   = ictl;
					btype  = LINK_CTL;
					if (ambig) *ambig = FALSE;
					if (dupl)  *dupl  = FALSE;
					}
				else if (ichain != bchain)
					{
					if (bplus == pplus)
						{
						if (mplus == pplus)
							{
							xchain = dfld->chains[bchain];
							if (ambig) *ambig = TRUE;
							if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
							/* >>>>> testing <<<<< */
							(void) printf("[closest_link_chain] Control chain: %d (%d)  to chain: %d (%d)\n",
									ichain, chain->track->numpts,
									bchain, xchain->track->numpts);
							if (chain->track->numpts == xchain->track->numpts)
								{
								for (iix=0; iix<chain->track->numpts; iix++)
									(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
										iix, chain->track->points[iix][X],
										chain->track->points[iix][Y],
										xchain->track->points[iix][X],
										xchain->track->points[iix][Y], dtol);
								}
							if (*dupl)
								(void) printf("[closest_link_chain] Duplicate!\n");
							/* >>>>> testing <<<<< */
							}
						}
					else if (mplus == pplus || mdiff < bdiff)
						{
						bchain = ichain;
						bplus  = mplus;
						bdiff  = mdiff;
						bmem   = ictl;
						btype  = LINK_CTL;
						if (ambig) *ambig = FALSE;
						if (dupl)  *dupl  = FALSE;
						}
					else
						{
						xchain = dfld->chains[bchain];
						if (ambig) *ambig = TRUE;
						if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
						/* >>>>> testing <<<<< */
						(void) printf("[closest_link_chain] Control chain: %d (%d)  to chain: %d (%d)\n",
								ichain, chain->track->numpts,
								bchain, xchain->track->numpts);
						if (chain->track->numpts == xchain->track->numpts)
							{
							for (iix=0; iix<chain->track->numpts; iix++)
								(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
									iix, chain->track->points[iix][X],
									chain->track->points[iix][Y],
									xchain->track->points[iix][X],
									xchain->track->points[iix][Y], dtol);
							}
						if (*dupl)
							(void) printf("[closest_link_chain] Duplicate!\n");
						/* >>>>> testing <<<<< */
						}
					}
				else if (ichain == bchain)
					{
					if (mplus == pplus || mdiff < bdiff)
						{
						bplus  = mplus;
						bdiff  = mdiff;
						bmem   = ictl;
						btype  = LINK_CTL;
						if (ambig) *ambig = FALSE;
						if (dupl)  *dupl  = FALSE;
						}
					}
				}
			}
		}

	/* Return if we found a matching control node */
	if (bchain >= 0)
		{
#		ifdef DEBUG
		(void) pr_diag("Timelink",
				"Matching control node: %d (at %d)\n", bmem, bdiff);
#		endif /* DEBUG */
		if (member) *member = bmem;
		if (ltype)  *ltype  = btype;
		return bchain;
		}

	/* Next search regular link nodes */
	for (ichain=dfld->nchain-1; ichain>=0; ichain--)
		{
		chain = dfld->chains[ichain];

		/* Search regular link nodes - give preference to preferred time */
		for (ivt=0; ivt<NumTime; ivt++)
			{
			if (!dfld->frames || !dfld->frames[ivt].meta)
				{
				if (!same(dfld->element, "MASTER_LINK")) continue;
				if (!TimeList[ivt].depict)               continue;
				}

			/* >>>>> replace this ...
			if (!chain->nodes[ivt]->there) continue;
			copy_point(lpos, chain->nodes[ivt]->node);
			... with this <<<<< */
			il = which_lchain_node(chain, LchainNode, TimeList[ivt].mplus);
			if (il < 0 || !chain->nodes[il]->there) continue;
			copy_point(lpos, chain->nodes[il]->node);

			if (!inside_map_def(&MapProj->definition, lpos)) continue;

			dx   = pos[X] - lpos[X];
			dy   = pos[Y] - lpos[Y];
			dist = hypot(dx, dy);
			if (dist > dtol) continue;

			mplus = chain->nodes[il]->mplus;
			mdiff = abs(mplus - pplus);

			if (bchain < 0)
				{
				bchain = ichain;
				bplus  = mplus;
				bdiff  = mdiff;
				bmem   = il;
				btype  = LINK_REG;
				if (ambig) *ambig = FALSE;
				if (dupl)  *dupl  = FALSE;
				}
			else if (ichain != bchain)
				{
				if (bplus == pplus)
					{
					if (mplus == pplus)
						{
						xchain = dfld->chains[bchain];
						if (ambig) *ambig = TRUE;
						if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
						/* >>>>> testing <<<<< */
						(void) printf("[closest_link_chain] Node chain: %d (%d)  to chain: %d (%d)\n",
								ichain, chain->track->numpts,
								bchain, xchain->track->numpts);
						if (chain->track->numpts == xchain->track->numpts)
							{
							for (iix=0; iix<chain->track->numpts; iix++)
								(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
									iix, chain->track->points[iix][X],
									chain->track->points[iix][Y],
									xchain->track->points[iix][X],
									xchain->track->points[iix][Y], dtol);
							}
						if (*dupl)
							(void) printf("[closest_link_chain] Duplicate!\n");
						/* >>>>> testing <<<<< */
						}
					}
				else if (mplus == pplus || mdiff < bdiff)
					{
					bchain = ichain;
					bplus  = mplus;
					bdiff  = mdiff;
					bmem   = il;
					btype  = LINK_REG;
					if (ambig) *ambig = FALSE;
					if (dupl)  *dupl  = FALSE;
					}
				else
					{
					xchain = dfld->chains[bchain];
					if (ambig) *ambig = TRUE;
					if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
					/* >>>>> testing <<<<< */
					(void) printf("[closest_link_chain] Node chain: %d (%d)  to chain: %d (%d)\n",
							ichain, chain->track->numpts,
							bchain, xchain->track->numpts);
					if (chain->track->numpts == xchain->track->numpts)
						{
						for (iix=0; iix<chain->track->numpts; iix++)
							(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
								iix, chain->track->points[iix][X],
								chain->track->points[iix][Y],
								xchain->track->points[iix][X],
								xchain->track->points[iix][Y], dtol);
						}
					if (*dupl)
						(void) printf("[closest_link_chain] Duplicate!\n");
					/* >>>>> testing <<<<< */
					}
				}
			else if (ichain == bchain)
				{
				if (mplus == pplus || mdiff < bdiff)
					{
					bplus  = mplus;
					bdiff  = mdiff;
					bmem   = il;
					btype  = LINK_REG;
					if (ambig) *ambig = FALSE;
					if (dupl)  *dupl  = FALSE;
					}
				}
			}
		}

	/* Return if we found a matching link node */
	if (bchain >= 0)
		{
#		ifdef DEBUG
		(void) pr_diag("Timelink",
				"Matching link node: %d (at %d)  from chain: %d\n",
				bmem, bdiff, bchain);
#		endif /* DEBUG */
		if (member) *member = bmem;
		if (ltype)  *ltype  = btype;
		return bchain;
		}

	/* Last search interp nodes (if present) */
	for (ichain=dfld->nchain-1; ichain>=0; ichain--)
		{
		chain = dfld->chains[ichain];

		/* Search interp nodes - give preference to preferred time */
		if (NotNull(chain->interps) && (chain->inum > 0))
			{
			for (intp=0; intp<chain->inum; intp++)
				{
				if (!chain->interps[intp]->there) continue;
				copy_point(lpos, chain->interps[intp]->node);
				if (!inside_map_def(&MapProj->definition, lpos)) continue;

				dx   = pos[X] - lpos[X];
				dy   = pos[Y] - lpos[Y];
				dist = hypot(dx, dy);
				if (dist > dtol/2) continue;

				mplus = chain->interps[intp]->mplus;
				mdiff = abs(mplus - pplus);

				if (bchain < 0)
					{
					bchain = ichain;
					bplus  = mplus;
					bdiff  = mdiff;
					bmem   = intp;
					btype  = LINK_INT;
					if (ambig) *ambig = FALSE;
					if (dupl)  *dupl  = FALSE;
					}
				else if (ichain != bchain)
					{
					if (bplus == pplus)
						{
						if (mplus == pplus)
							{
							xchain = dfld->chains[bchain];
							if (ambig) *ambig = TRUE;
							if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
							/* >>>>> testing <<<<< */
							(void) printf("[closest_link_chain] Interp chain: %d (%d)  to chain: %d (%d)\n",
									ichain, chain->track->numpts,
									bchain, xchain->track->numpts);
							if (chain->track->numpts == xchain->track->numpts)
								{
								for (iix=0; iix<chain->track->numpts; iix++)
									(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
										iix, chain->track->points[iix][X],
										chain->track->points[iix][Y],
										xchain->track->points[iix][X],
										xchain->track->points[iix][Y], dtol);
								}
							if (*dupl)
								(void) printf("[closest_link_chain] Duplicate!\n");
							/* >>>>> testing <<<<< */
							}
						}
					else if (mplus == pplus || mdiff < bdiff)
						{
						bchain = ichain;
						bplus  = mplus;
						bdiff  = mdiff;
						bmem   = intp;
						btype  = LINK_INT;
						if (ambig) *ambig = FALSE;
						if (dupl)  *dupl  = FALSE;
						}
					else
						{
						xchain = dfld->chains[bchain];
						if (ambig) *ambig = TRUE;
						if (dupl)  *dupl  = duplicate_tracks(chain->track,
															xchain->track, 1.0);
						/* >>>>> testing <<<<< */
						(void) printf("[closest_link_chain] Interp chain: %d (%d)  to chain: %d (%d)\n",
								ichain, chain->track->numpts,
								bchain, xchain->track->numpts);
						if (chain->track->numpts == xchain->track->numpts)
							{
							for (iix=0; iix<chain->track->numpts; iix++)
								(void) printf("[closest_link_chain]   Points: %d  at %.2f/%.2f  and %.2f/%.2f  dtol: %.2f\n",
									iix, chain->track->points[iix][X],
									chain->track->points[iix][Y],
									xchain->track->points[iix][X],
									xchain->track->points[iix][Y], dtol);
							}
						if (*dupl)
							(void) printf("[closest_link_chain] Duplicate!\n");
						/* >>>>> testing <<<<< */
						}
					}
				else if (ichain == bchain)
					{
					if (mplus == pplus || mdiff < bdiff)
						{
						bplus  = mplus;
						bdiff  = mdiff;
						bmem   = intp;
						btype  = LINK_INT;
						if (ambig) *ambig = FALSE;
						if (dupl)  *dupl  = FALSE;
						}
					}
				}
			}
		}

	/* Return what we found */
#	ifdef DEBUG
	if (bchain >= 0)
		(void) pr_diag("Timelink",
				"Matching interp node: %d (at %d)\n", bmem, bdiff);
	else
		(void) pr_diag("Timelink", "No matching node\n");
#	endif /* DEBUG */
	if (member) *member = bmem;
	if (ltype)  *ltype  = btype;
	return bchain;
	}

/**********************************************************************/

int		closest_link_end

	(
	LOGICAL	fwd,
	int		ivt,
	POINT	pos,
	DFLIST	*dfld,
	float	dtol,
	LOGICAL	*ambig
	)

	{
	int		ichain, bchain, jvt, jl;
	POINT	lpos;
	float	dx, dy, dist;
	LCHAIN	chain;

	if (ivt < first_depict_time()) return -1;
	if (ivt > last_depict_time())  return -1;
	/*
	if (fwd && ivt == first_depict_time()) return -1;
	if (!fwd && ivt == last_depict_time()) return -1;
	*/

	if (!dfld->frames || !dfld->frames[ivt].meta)
		{
		if (!same(dfld->element, "MASTER_LINK")) return -1;
		if (!TimeList[ivt].depict)               return -1;
		}

	/* Search for closest link node on the appropriate end of a link chain */
	bchain = -1;
	if (ambig) *ambig = TRUE;
	for (ichain=dfld->nchain-1; ichain>=0; ichain--)
		{
		chain = dfld->chains[ichain];

		/* Make sure the last/first node on this chain occurs at ivt */
		if (fwd)
			{
			for (jvt=NumTime-1; jvt>=ivt; jvt--)
				{
				/* >>>>> replace this ...
				if (!chain->nodes[ivt]->there) continue;
				copy_point(lpos, chain->nodes[jvt]->node);
				... with this <<<<< */
				jl = which_lchain_node(chain, LchainNode, TimeList[jvt].mplus);
				if (jl < 0 || !chain->nodes[jl]->there) continue;
				copy_point(lpos, chain->nodes[jl]->node);
				if (!inside_map_def(&MapProj->definition, lpos)) continue;
				break;
				}
			if (jvt != ivt) continue;
			}
		else
			{
			for (jvt=0; jvt<=ivt; jvt++)
				{
				/* >>>>> replace this ...
				if (!chain->nodes[ivt]->there) continue;
				copy_point(lpos, chain->nodes[jvt]->node);
				... with this <<<<< */
				jl = which_lchain_node(chain, LchainNode, TimeList[jvt].mplus);
				if (jl < 0 || !chain->nodes[jl]->there) continue;
				copy_point(lpos, chain->nodes[jl]->node);
				if (!inside_map_def(&MapProj->definition, lpos)) continue;
				break;
				}
			if (jvt != ivt) continue;
			}

		dx   = pos[X] - lpos[X];
		dy   = pos[Y] - lpos[Y];
		dist = hypot(dx, dy);
		if (dist > dtol) continue;
		if (bchain < 0)
			{
			bchain = ichain;
			if (ambig) *ambig = FALSE;
			}
		else
			{
			if (ichain != bchain && ambig) *ambig = TRUE;
			}
		}

	return bchain;
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ c o n t r o l _ l i s t                              *
*     c o p y _ c o n t r o l _ l i s t                                *
*     f r e e _ c o n t r o l _ l i s t                                *
*     a d d _ n o d e s _ t o _ c o n t r o l _ l i s t                *
*                                                                      *
***********************************************************************/

LCTRL	*build_control_list

	(
	LCHAIN	chain,
	int		ilink
	)

	{
	int			mfirst, mplus, itween, inum;
	LCTRL		*ctrl;

	/* Allocate the control node list */
	ctrl = INITMEM(LCTRL, 1);
	ctrl->lnum   = 0;
	ctrl->ncont  = 0;
	ctrl->cthere = NullLogical;
	ctrl->mplus  = NullInt;
	ctrl->lcnum  = NullInt;
	ctrl->cnode  = NullPtr(POINT **);
	ctrl->ilink  = NullPtr(int **);

	/* Return now if no chain or no intermediate link nodes possible */
	if (IsNull(chain)) return ctrl;
	if (NumTween <= 0) return ctrl;

	/* Add times for intermediate link nodes */
	ctrl->lnum   = NumTween;
	ctrl->cthere = GETMEM(ctrl->cthere, LOGICAL, ctrl->lnum);
	ctrl->mplus  = GETMEM(ctrl->mplus,  int,     ctrl->lnum);
	ctrl->lcnum  = GETMEM(ctrl->lcnum,  int,     ctrl->lnum);
	ctrl->cnode  = GETMEM(ctrl->cnode,  POINT *, ctrl->lnum);
	ctrl->ilink  = GETMEM(ctrl->ilink,  int *,   ctrl->lnum);
	mfirst = TimeList[first_depict_time()].mplus;
	for (inum=0, itween=0; itween<NumTween; itween++)
		{
		ctrl->cthere[itween] = FALSE;
		ctrl->mplus[itween]  = mfirst + itween*DTween;

		/* Initialized with the interpolated locations (if there) */
		while (inum < chain->inum 
				&& chain->interps[inum]->mplus <  ctrl->mplus[itween])
			{
			inum++;
			}
		if (inum < chain->inum
				&& chain->interps[inum]->mplus == ctrl->mplus[itween])
			{
			ctrl->lcnum[itween]  = 1;
			ctrl->cnode[itween]  = INITMEM(POINT, 1);
			ctrl->ilink[itween]  = INITMEM(int,   1);
			(void) copy_point(ctrl->cnode[itween][0],
												chain->interps[inum]->node);
			ctrl->ilink[itween][0] = ilink;
			inum++;
			}
		else
			{
			ctrl->lcnum[itween]  = 0;
			ctrl->cnode[itween]  = NullPointList;
			ctrl->ilink[itween]  = NullInt;
			}
		}

	/* Return now if no link nodes */
	if (chain->lnum <= 0) return ctrl;

	/* Add the control node information */
	for (inum=0; inum<chain->lnum; inum++)
		{
		if (!chain->nodes[inum]->there)                 continue;
		if (chain->nodes[inum]->ltype != LchainControl) continue;
		mplus = chain->nodes[inum]->mplus;

		/* Find matching time in list */
		for (itween=0; itween<NumTween; itween++)
			{
			if (mplus != ctrl->mplus[itween]) continue;

			/* Should never have a control node without an interpolation! */
			if (ctrl->lcnum[itween] != 1)
				{
				(void) pr_warning("Timelink",
					"Uninterpolated control node at time: %d\n", mplus);
				continue;
				}

			/* Replace control node location */
			ctrl->ncont++;
			ctrl->cthere[itween] = TRUE;
			(void) copy_point(ctrl->cnode[itween][0],
												chain->nodes[inum]->node);
			break;
			}
		if (itween >= ctrl->lnum)
			{
			(void) pr_warning("Timelink",
				"Unmatched control node at time: %d\n", mplus);
			}
		}

	/* Return the structure */
	return ctrl;
	}

/**********************************************************************/

LCTRL	*copy_control_list

	(
	LCTRL	*ctrl
	)

	{
	int			inum, lc;
	LCTRL		*cnew;

	/* Allocate the control node list */
	cnew = INITMEM(LCTRL, 1);
	cnew->lnum   = ctrl->lnum;
	cnew->ncont  = ctrl->ncont;
	cnew->cthere = NullLogical;
	cnew->mplus  = NullInt;
	cnew->lcnum  = NullInt;
	cnew->cnode  = NullPtr(POINT **);
	cnew->ilink  = NullPtr(int **);

	/* Return now if no intermediate link nodes possible */
	if (cnew->lnum <= 0) return cnew;

	/* Duplicate the control node structure at each time */
	cnew->cthere = GETMEM(cnew->cthere, LOGICAL, cnew->lnum);
	cnew->mplus  = GETMEM(cnew->mplus,  int,     cnew->lnum);
	cnew->lcnum  = GETMEM(cnew->lcnum,  int,     cnew->lnum);
	cnew->cnode  = GETMEM(cnew->cnode,  POINT *, cnew->lnum);
	cnew->ilink  = GETMEM(cnew->ilink,  int *,   cnew->lnum);
	for (inum=0; inum<cnew->lnum; inum++)
		{
		cnew->cthere[inum] = ctrl->cthere[inum];
		cnew->mplus[inum]  = ctrl->mplus[inum];
		cnew->lcnum[inum]  = ctrl->lcnum[inum];
		cnew->cnode[inum]  = NullPtr(POINT *);
		cnew->ilink[inum]  = NullInt;

		if (cnew->lcnum[inum] > 0)
			{
			cnew->cnode[inum] = INITMEM(POINT, cnew->lcnum[inum]);
			cnew->ilink[inum] = INITMEM(int,   cnew->lcnum[inum]);
			for (lc=0; lc<cnew->lcnum[inum]; lc++)
				{
				(void) copy_point(cnew->cnode[inum][lc], ctrl->cnode[inum][lc]);
				cnew->ilink[inum][lc] = ctrl->ilink[inum][lc];
				}
			}
		}

	/* Return the structure */
	return cnew;
	}

/**********************************************************************/

void	free_control_list

	(
	LCTRL	*ctrl
	)

	{
	if (IsNull(ctrl)) return;

	FREEMEM(ctrl->cthere);
	FREEMEM(ctrl->mplus);
	FREEMEM(ctrl->lcnum);
	FREELIST(ctrl->cnode, ctrl->lnum);
	FREELIST(ctrl->ilink, ctrl->lnum);
	FREEMEM(ctrl);
	return;
	}

/**********************************************************************/

void	add_nodes_to_control_list

	(
	LCTRL	*ctrl,
	int		ilink,
	LCTRL	*ctrlx
	)

	{
	int			inum, snum, icnum;

	if (IsNull(ctrl))  return;
	if (IsNull(ctrlx)) return;
	if (ctrl->lnum != ctrlx->lnum) return;

	/* Copy the control node information */
	for (inum=0; inum<ctrl->lnum; inum++)
		{
		if (ctrlx->lcnum[inum] <= 0) continue;

		/* Add control nodes to list */
		if (!ctrl->cthere[inum] && ctrlx->cthere[inum])
			{
			ctrl->ncont++;
			ctrl->cthere[inum] = TRUE;
			}
		snum = ctrl->lcnum[inum];
		ctrl->lcnum[inum] += ctrlx->lcnum[inum];
		ctrl->cnode[inum] = GETMEM(ctrl->cnode[inum], POINT,
													ctrl->lcnum[inum]);
		ctrl->ilink[inum] = GETMEM(ctrl->ilink[inum], int,
													ctrl->lcnum[inum]);
		for (icnum=0; icnum<ctrlx->lcnum[inum]; icnum++)
			{
			(void) copy_point(ctrl->cnode[inum][snum+icnum],
								ctrlx->cnode[inum][icnum]);
			ctrl->ilink[inum][snum+icnum] = ilink;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     p o s t _ i n t e r p _ c a n c e l                              *
*     i n t e r p o l a t e                                            *
*     i n t e r p o l a t e _ d f i e l d                              *
*     i n t e r p _ n e e d e d                                        *
*                                                                      *
***********************************************************************/

static	METAFILE	OutMeta      = NullMeta;
static	TSTAMP		*Vtween      = (TSTAMP *)0;
static	LOGICAL		InterpShow   = TRUE;
static	LOGICAL		InterpGoing  = FALSE;
static	LOGICAL		InterpCancel = FALSE;

void	post_interp_cancel

	(void)

	{
	if (InterpGoing) InterpCancel = TRUE;
	}

/**********************************************************************/

LOGICAL	interpolate

	(
	STRING	elem,
	STRING	level
	)

	{
	LOGICAL			changed;
	int				idfld, itween, ifld, nfld;
	int				pyear, pjday, phour, pminute;
	DFLIST			*dfld;
	STRING			vtime;

	if (ViewOnly) return FALSE;

	busy_cursor(TRUE);
	(void) show_blank();

	/* Build list of interp valid times */
	Vtween  = INITMEM(TSTAMP, NumTween);
	pyear   = Syear;
	pjday   = Sjday;
	phour   = Shour;
	pminute = Sminute + TimeList[first_depict_time()].mplus;
	for (itween=0; itween<NumTween; itween++)
	    {
	    tnorm(&pyear, &pjday, &phour, &pminute, NullInt);
		vtime = build_tstamp(pyear, pjday, phour, pminute, FALSE,
					minutes_in_depictions());
		(void) strcpy(Vtween[itween], vtime);

	    /* Advance to next valid time */
	    pminute += DTween;
		}

	/* Create a temporary metafile for outputing the interpolated fields */
	if (IsNull(OutMeta))
		{
		OutMeta = create_metafile();
		define_mf_projection(OutMeta, MapProj);
		}
	busy_cursor(FALSE);

	/* Interpolate one field at a time */
	InterpGoing  = TRUE;
	InterpCancel = FALSE;
	changed = FALSE;
	if (same(elem, "ALL") && same(level, "ALL"))
		{
		put_message("interp-all");
		nfld = 0;
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			dfld = DfldList + idfld;
			if (!interp_needed(dfld)) continue;
			nfld++;
			}
		ifld = 0;
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			dfld = DfldList + idfld;
			if (!interp_needed(dfld)) continue;
			interp_progress(dfld, 0, NumTween, ifld, nfld);
			changed |= interpolate_dfield(dfld);
			if (InterpCancel) break;
			interp_progress(dfld, NumTween, NumTween, ifld, nfld);
			ifld++;
			}
		}
	else
		{
		dfld = find_dfield(elem, level);
		if (NotNull(dfld) && interp_needed(dfld))
			{
			interp_progress(dfld, 0, NumTween, 0, 1);
			changed |= interpolate_dfield(dfld);
			interp_progress(dfld, NumTween, NumTween, 0, 1);
			}
		}
	InterpGoing  = FALSE;
	InterpCancel = FALSE;

	/* Destroy the temporary metafile */
	OutMeta = destroy_metafile(OutMeta);
	FREEMEM(Vtween);

	/* Restore the image */
	if (changed) put_message("interp-done");
	else         put_message("interp-none");
	(void) hide_blank();

	if (changed) (void) save_links();
	return TRUE;
	}

/**********************************************************************/

LOGICAL	interpolate_dfield

	(
	DFLIST	*dfld
	)

	{
	FLD_DESCRIPT	fd;
	STRING			elem, levl, file, vtime;
	LOGICAL			interp, changed=FALSE;
	int				ivt, itween;
	FIELD			fld;

	/* Check link and interp states and get rid of old interpolated files */
	if (!interp_needed(dfld)) return FALSE;

	/* OK we have to interpolate */
	elem = dfld->element;
	levl = dfld->level;
	active_field_info(elem, levl, "depict", NULL, NULL, NULL);

	/* Clear out old interpolated fields */
	(void) release_dfield_interp(dfld);

	/* Interpolate this field */
	put_message("interp-fld", levl, elem);
	if (tdep_special(dfld->tdep))
		interp = interp_static(dfld, InterpShow);
	else
		{
		switch (dfld->editor)
			{
			case FpaC_VECTOR:
				interp = interp_spline_2D(dfld, InterpShow, FALSE);
				break;

			case FpaC_CONTINUOUS:
				interp = interp_spline(dfld, InterpShow, FALSE);
				break;

			case FpaC_DISCRETE:
			case FpaC_WIND:
				interp = interp_area(dfld, InterpShow, TRUE);
				break;

			case FpaC_LINE:
				interp = interp_line(dfld, InterpShow, TRUE);
				break;

			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				interp = FALSE;
			}
		}
	if (!interp) return FALSE;
	changed = TRUE;

	/* Clear internal buffers in the "luke-warm" database */
	clear_equation_database();

	/* Now output the results */
	busy_cursor(TRUE);
	put_message("interp-out", levl, elem);
	if (tdep_special(dfld->tdep))
		{
		for (ivt=0; ivt<NumTime; ivt++)
			{
			/* Skip if no interpolated field for this time */
			if (!dfld->snaps) continue;
			fld = dfld->snaps[ivt];
			if (!fld) continue;

			/* Construct old and new metafile names for this field */
			/*  ... and remove any existing metafiles with these names! */
			vtime = TimeList[ivt].jtime;
			(void) init_fld_descript(&fd);
			if (!set_fld_descript(&fd,
							FpaF_SOURCE_NAME,		"interp",
							FpaF_VALID_TIME,		vtime,
							FpaF_ELEMENT_NAME,		elem,
							FpaF_LEVEL_NAME,		levl,
							FpaF_END_OF_LIST)) continue;
			file = build_meta_filename(&fd);
			(void) remove_file(file, NULL);
			file = construct_meta_filename(&fd);
			(void) remove_file(file, NULL);

			/* Add the interpolated field and labels to temporary metafile */
			add_field_to_metafile(OutMeta, copy_field(fld));
			add_set_to_metafile(OutMeta, "d", elem, levl,
											copy_set(dfld->slabs[ivt]));

			/* Output the temporary metafile using new format filename */
			write_metafile(file, OutMeta, MaxDigits);

			/* Empty the temporary metafile structure for re-use */
			empty_metafile(OutMeta);
			}
		}
	else
		{
		for (itween=0; itween<NumTween; itween++)
			{
			/* Skip if no interpolated field for this time */
			if (!dfld->tweens) continue;
			fld = dfld->tweens[itween];
			if (!fld) continue;

			/* Construct old and new metafile names for this field */
			/*  ... and remove any existing metafiles with these names! */
			vtime = Vtween[itween];
			(void) init_fld_descript(&fd);
			if (!set_fld_descript(&fd,
							FpaF_SOURCE_NAME,		"interp",
							FpaF_VALID_TIME,		vtime,
							FpaF_ELEMENT_NAME,		elem,
							FpaF_LEVEL_NAME,		levl,
							FpaF_END_OF_LIST)) continue;
			file = build_meta_filename(&fd);
			(void) remove_file(file, NULL);
			file = construct_meta_filename(&fd);
			(void) remove_file(file, NULL);

			/* Add the interpolated field and labels to temporary metafile */
			add_field_to_metafile(OutMeta, copy_field(fld));
			add_set_to_metafile(OutMeta, "d", elem, levl,
											copy_set(dfld->tlabs[itween]));

			/* Output the temporary metafile using new format filename */
			write_metafile(file, OutMeta, MaxDigits);

			/* Empty the temporary metafile structure for re-use */
			empty_metafile(OutMeta);
			}
		}

	/* Notify the interface of the results */
	dfld->saved = TRUE;
	(void) verify_dfield_interp(dfld, TRUE, TRUE);

	/* Clear out new interpolated fields if low memory */
	if (ConserveMem) (void) release_dfield_interp(dfld);
	busy_cursor(FALSE);

	return changed;
	}

/**********************************************************************/

LOGICAL	interp_needed

	(
	DFLIST	*dfld
	)

	{
	/* Check link and interp states and get rid of old interpolated files */
	if (IsNull(dfld))                               return FALSE;
	(void) verify_dfield_links(dfld, FALSE);
	(void) verify_dfield_interp(dfld, FALSE, TRUE);
	if (!dfld->dolink && !tdep_special(dfld->tdep)) return FALSE;
	if (!dfld->there)                               return FALSE;
	if (!dfld->linked)                              return FALSE;

	/* We can interpolate - but do we have to? */
	if (!dfld->saved)                               return TRUE;
	if (!dfld->interp)                              return TRUE;
	if (!dfld->intlab)                              return TRUE;

	/* We don't have to interpolate */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    i n t e r p _ s t a t i c                                         *
*                                                                      *
*    Perform the time interpolation of static fields from the working  *
*    depiction sequence, onto the given interval.                      *
*                                                                      *
*    The set of keyframe images is provided in dfld->fields.           *
*    The set of generated images is deposited in dfld->snaps.          *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
LOGICAL	interp_static

	(
	DFLIST	*dfld,
	LOGICAL	show
	)

	{
	int		ivt;

	/* See if we can or even need to interpolate */
	if (!dfld)                        return FALSE;
	if (!tdep_special(dfld->tdep))    return FALSE;
	if (NumTime < 1)                  return FALSE;
	if (!dfld->linked)                return FALSE;
	if (dfld->interp && dfld->intlab) return TRUE;
	if (!dfld->there)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_static() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_static() - F %s %s\n", dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->interp   = TRUE;
		dfld->intlab   = TRUE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		(void) save_links();
		return TRUE;
		}

	busy_cursor(TRUE);

	/* Allocate output fields and replicate background and presentation */
	(void) prepare_dfield_snaps(dfld);
	for (ivt=0; ivt<NumTime; ivt++)
	    {
		/* Get rid of old interpolated field */
		dfld->snaps[ivt]  = destroy_field(dfld->snaps[ivt]);
		dfld->slabs[ivt]  = destroy_set(dfld->slabs[ivt]);

		/* Extract the field for that time */
		dfld->snaps[ivt]  = copy_field(dfld->fields[ivt]);
		dfld->slabs[ivt]  = copy_set(dfld->flabs[ivt]);
	    }

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In interp_static() - T %s %s\n", dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In interp_static() - F %s %s\n", dfld->element, dfld->level);
#	endif /* DEVELOPMENT */


	/* Set flags */
	dfld->interp   = TRUE;
	dfld->intlab   = TRUE;
	dfld->saved    = FALSE;
	dfld->reported = FALSE;
	link_status(dfld);
	(void) save_links();

	busy_cursor(FALSE);
	return TRUE;
	}
