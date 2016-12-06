/*
 * File: storm_environment.c
 *
 * Purpose: Looks for an "Environment" element for each storm. If one is found
 *          nothing is done. If one is not found then one is added with the value
 *          of "default". After being added the previous SCIT file is scanned and
 *          if the storm is found and has an environment element the value of the
 *          element in the current storm is set to the same value as the previous
 *          storm entry.
 */

#include "rankweight.h"
#include "storm_environment.h"

extern STRING  stat_dir;	/* Directory where STAT files are found */
extern STRING  file_mask;	/* STAT file name mask */


/* Return the doc pointer of the file previous to fname.
 * If none found return NULL.
 */
static xmlDocPtr get_previous_file_doc(STRING fname)
{
	int pos, nfilelist;
	STRING *filelist;
	xmlDocPtr doc = NULL;

	/* Get the list of files in time order */
	dirlist_reuse(FALSE);
	nfilelist = dirlist(stat_dir, file_mask, &filelist);
	dirlist_reuse(TRUE);
	time_sort_files(filelist, nfilelist);

	/* Search for the given fname in the list */
	for(pos = 0; pos < nfilelist; pos++)
		if(same(fname,filelist[pos])) break;

	/* Open the xml database for the previous file */
	if(pos > 0 && pos < nfilelist)
	{
		doc = xmlReadFile(pathname(stat_dir,filelist[pos-1]), NULL, XML_PARSE_NOBLANKS);
		if (!doc) printlog("ERROR: Unable to read xml file \'%s\'", filelist[pos-1]);
	}
	FREELIST(filelist, nfilelist);

	return doc;
}



/* Get the environment value. Look in the previous file and if there is an
 * environment defined for the storm copy it to the env parameter. If none
 * is found assign "default" as the value.
 */
static STRING get_environment(xmlNodePtr root, int storm_id)
{
	xmlNodePtr top, storm;
	static char env[128];

	strcpy(env, "default");
	if(!root) return env;

	for(top = root->children; top; top = top->next)
	{
		STRING val;
		LOGICAL storm_found = FALSE;

		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		/*
		 * Search for the given storm_id
		 */
		for(storm = top->children; storm && !storm_found; storm = storm->next)
		{
			if(storm->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(storm->name, STORM_ID)) continue;
			val = xmlNodeGetContent(storm);
			storm_found = (atoi(val) == storm_id);
			xmlFree(val);
		}
		/*
		 * If found look for the environment element and get its value
		 */
		if(storm_found)
		{
			for(storm = top->children; storm; storm = storm->next)
			{
				if(xmlStrcmp(storm->name,STORM_ENV_ID)) continue;
				val = xmlNodeGetContent(storm);
				if(val)
				{
					if(strlen(val) < sizeof(env))
						strcpy(env, val);
					xmlFree(val);
				}
				break;
			}
			break;
		}
	}
	return env;
}


/* Check the environment of the storms in the file. If one is found
 * leave everything as is. If not found add an environment element
 * to the storm and assign it a value.
 */
LOGICAL check_storm_environment(xmlNodePtr root, xmlNodePtr prev_root, STRING fname)
{
	xmlDocPtr prev_doc = NULL;
	xmlNodePtr top, storm;
	LOGICAL file_change = FALSE;

	/* If a previous database root node is not given try and get one
	 * from a previous file outselves.
	 */
	if(!prev_root)
	{
		prev_doc = get_previous_file_doc(fname);
		if(prev_doc)
			prev_root = xmlDocGetRootElement(prev_doc);
	}

	for(top = root->children; top; top = top->next)
	{
		LOGICAL environ_found = FALSE;
		int storm_id = -1;

		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		/*
		 * Does the storm have an environment element?
		 */
		for(storm = top->children; storm; storm = storm->next)
		{
			if(storm->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(storm->name, STORM_ID) == 0)
			{
				STRING val = xmlNodeGetContent(storm);
				storm_id = atoi(val);
				xmlFree(val);
			}
			else if(xmlStrcmp(storm->name,STORM_ENV_ID) == 0)
			{
				environ_found = TRUE;
			}
		}
		/*
		 * If no environment found add one to the storm
		 */
		if(storm_id != -1 && !environ_found)
		{
			xmlNodePtr node;
			STRING env = get_environment(prev_root, storm_id);
			node = xmlNewChild(top, NULL, STORM_ENV_ID, env);
			if (!node)
				printlog("ERROR: Unable to add \'%s\' element to storm \'%d\' in file \'%s\'",
					STORM_ENV_ID, storm_id, fname);
			file_change = TRUE;
		}
	}

	if (prev_doc) xmlFreeDoc(prev_doc);

	return file_change;
}
