/**************************************************************************/
/*
 *	File: calc_weights.c
 *
 *	Purpose: Calculates the rank weight of storms found in SCIT files.
 */
/**************************************************************************/
#include <math.h>
#include "rankweight.h"
#include "calc_weights.h"

extern STRING  stat_dir;
extern STRING  rankweight_id;
extern STRING  rank_id;
extern int     nrankweight_elem;
extern RW_ELEM *rankweight_elem;
extern int     nrankweight_fcst;
extern RW_FCST *rankweight_fcst;

typedef struct {
	int    num;
	double weight;
} STORM_STRUCT;


/* This is the function where the weight contribution of the elements is
 * calculated. All of the equation constants are stored in rankweight_elem.
 * See the rankweight.h header file for details.
 */
static LOGICAL calc_weight(xmlNodePtr node, double *rank)
{
	int n;
	LOGICAL rtnval = FALSE;

	*rank = 0;
	for(n = 0; n < nrankweight_elem; n++)
	{
		if(xmlStrcmp(node->name,rankweight_elem[n].id) == 0)
		{
			double val = 0, sval = 0, rval = 0;
			STRING strval = xmlNodeGetContent(node);
			rtnval = TRUE;

			if(xmlStrcasecmp(strval, DATA_NA))
			{
				char buf[32];
				LOGICAL ok;
				strncpy(buf, strval, 31);
				buf[31] = '\0';
				val = double_arg(strval, &ok);
				if(!ok) printlog("ERROR: Value parse failure for element %s - value = %s", node->name, buf);
			}
			xmlFree(strval);
			sval = val * rankweight_elem[n].m;

			if(rankweight_elem[n].a != 0.)
			{
				rval += rankweight_elem[n].a * pow(sval,4);
			}
			if(rankweight_elem[n].b != 0.)
			{
				rval += rankweight_elem[n].b * pow(sval,3);
			}
			if(rankweight_elem[n].c != 0.)
			{
				rval += rankweight_elem[n].c * pow(sval,2);
			}
			if(rankweight_elem[n].d != 0.)
			{
				rval += rankweight_elem[n].d * sval;
			}
			*rank = rval * rankweight_elem[n].f;
			printdebug("%s : val = %.1f  scaled val = %g  rank = %.1f", node->name, val, sval, *rank);			
			break;
		}
	}
	return rtnval;
}


/* Sort function for storm weight */
static int weightcmp(const void *a, const void *b)
{
	double diff = ((STORM_STRUCT *)b)->weight - ((STORM_STRUCT *)a)->weight;
	if(diff < 0) return -1;
	if(diff > 0) return 1;
	return 0;
}


/* Set the rank of the storms from 1 to n, with 1 being the most important
 */
static void calc_rankings(xmlNodePtr root, STORM_STRUCT *storm, int nstorm)
{
	int n, storm_rank = 999;
	xmlNodePtr top, cur, data, rank_node;

	/* If rank_id is NULL then there is no rank element for
	 * this file and the rankings do not have to be done.
	 */
	if(!rank_id) return;

	/* Sort the storms by weight with the greatest weight at index 0
	 * and the least at index nstorm - 1.
	 */
	qsort((void *)storm, nstorm, sizeof(STORM_STRUCT), weightcmp);

	/* Put the storm weights into the file. No assumption is made as to
	 * if the storm number comes before the rank or not.
	 */
	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;

		rank_node = NULL;
		storm_rank = 0;
		for(cur = top->children; cur; cur = cur->next)
		{
			if (cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(cur->name,STORM_ID) == 0)
			{
				int snum;
				STRING val = xmlNodeGetContent(cur);
				snum = atoi(val);
				xmlFree(val);
				for(n = 0; n < nstorm; n++)
					if(snum == storm[n].num) storm_rank = n+1;
			}
			else if(xmlStrcmp(cur->name,rank_id) == 0) 
			{
				rank_node = cur;
			}
		}
		if(rank_node != NULL)
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%d", storm_rank);
			xmlNodeSetContent(rank_node, buf);
		}
	}

}


/* Calculate the storm rank weight for the storms in the
 * given xml root for the file. Return true if there are
 * any changes in the value.
 */
LOGICAL calc_data_rankweights(xmlNodePtr root)
{
	LOGICAL file_change = FALSE;
	xmlNodePtr top, cur, data, rank_node;
	STRING old_rank = NULL;
	int nrank_weight = 0;
	double rank_weight = 0;
	int   nstorm = 0;
	STORM_STRUCT *storm = NULL;

	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;

		old_rank = NULL;
		rank_node = NULL;
		nrank_weight = 0;
		rank_weight = 0;
		storm = GETMEM(storm, STORM_STRUCT, nstorm+1);
		storm[nstorm].num = 0;
		storm[nstorm].weight = 0;

		for(cur = top->children; cur; cur = cur->next)
		{
			double rank;
			if (cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(cur->name,STORM_ID) == 0)
			{
				STRING val = xmlNodeGetContent(cur);
				printdebug("Storm number = %s", val);
				storm[nstorm].num = atoi(val);
				xmlFree(val);
			}
			else if(xmlStrcmp(cur->name,rankweight_id) == 0) 
			{
				old_rank = xmlNodeGetContent(cur);
				rank_node = cur;
			}
			else if(calc_weight(cur, &rank))
			{
				rank_weight += rank;
				nrank_weight++;
			}
		}
		if(rank_node != NULL && nrank_weight > 0)
		{
			int n, ndecimals = 1;
			float portion;
			char new_rank[32];
		
			/* Find the precision of the output for the rank weight element */
			for(n = 0; n < nrankweight_fcst; n++)
			{
				if(rankweight_fcst[n].minutes == 0)
				{
					ndecimals = rankweight_fcst[n].ndecimals;
					break;
				}
			}

			snprintf(new_rank, sizeof(new_rank), "%.*f", ndecimals, rank_weight);
			printdebug("------ old = %s  new = %s ------", old_rank, new_rank);				
			if(!same(new_rank, old_rank))
			{
				file_change = TRUE;
				xmlNodeSetContent(rank_node, new_rank);
			}
			storm[nstorm].weight = rank_weight;
		}
		nstorm++;
		xmlFree(old_rank);
	}

	/* If the file has changed chances are the storm rankings will change as well. */
	if(file_change)
		calc_rankings(root, storm, nstorm);

	FREEMEM(storm);

	return file_change;
}

