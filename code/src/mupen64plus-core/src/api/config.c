/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - api/config.c                                       *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       * 
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
                       
/* This file contains the Core config functions which will be exported
 * outside of the core library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <retro_miscellaneous.h>

#define M64P_CORE_PROTOTYPES 1
#include "m64p_types.h"
#include "m64p_config.h"
#include "config.h"
#include "callbacks.h"

#include "main/util.h"

/* Cxd4 RSP */
#include "../../../mupen64plus-rsp-cxd4/config.h"

/* TODO/FIXME - put this in some header */

#define GFX_GLIDE64     0
#define GFX_RICE        1 
#define GFX_GLN64       2
#define GFX_ANGRYLION   3
#define GFX_PARALLEL    4

/* local types */
#define MUPEN64PLUS_CFG_NAME "mupen64plus.cfg"

#define SECTION_MAGIC 0xDBDC0580

typedef struct _config_var {
  char                 *name;
  m64p_type             type;
  union {
    int integer;
    float number;
    char *string;
  } val;
  char                 *comment;
  struct _config_var   *next;
  } config_var;

typedef struct _config_section {
  int                     magic;
  char                   *name;
  struct _config_var     *first_var;
  struct _config_section *next;
  } config_section;

typedef config_section *config_list;

/* local variables */
static int         l_ConfigInit = 0;
static int         l_SaveConfigOnExit = 0;
static char       *l_DataDirOverride = NULL;
static char       *l_ConfigDirOverride = NULL;
static config_list l_ConfigListActive = NULL;
static config_list l_ConfigListSaved = NULL;

/* --------------- */
/* local functions */
/* --------------- */

static int is_numeric(const char *string)
{
    char chTemp[16];
    float fTemp;
    int rval = sscanf(string, "%f%8s", &fTemp, chTemp);

    /* I want to find exactly one matched input item: a number with no garbage on the end */
    /* I use sscanf() instead of a custom loop because this routine must handle locales in which the decimal separator is not '.' */
    return (rval == 1);
}

/* This function returns a pointer to the pointer of the requested section
 * (i.e. a pointer the next field of the previous element, or to the first node).
 *
 * If there's no section named 'ParamName', returns the pointer to the next
 * field of the last element in the list (such that derefencing it is NULL).
 *
 * Useful for operations that need to modify the links, e.g. deleting a section.
 */
static config_section **find_section_link(config_list *list, const char *ParamName)
{
    config_section **curr_sec_link;
    for (curr_sec_link = list; *curr_sec_link != NULL; curr_sec_link = &(*curr_sec_link)->next)
    {
        if (strcasecmp(ParamName, (*curr_sec_link)->name) == 0)
            break;
    }

    return curr_sec_link;
}

/* This function is similar to the previous function, but instead it returns a
 * pointer to the pointer to the next section whose name is alphabetically
 * greater than or equal to 'ParamName'.
 *
 * Useful for inserting a section in its alphabetical position.
 */
static config_section **find_alpha_section_link(config_list *list, const char *ParamName)
{
    config_section **curr_sec_link;
    for (curr_sec_link = list; *curr_sec_link != NULL; curr_sec_link = &(*curr_sec_link)->next)
    {
        if (strcasecmp((*curr_sec_link)->name, ParamName) >= 0)
            break;
    }

    return curr_sec_link;
}

static config_section *find_section(config_list list, const char *ParamName)
{
    return *find_section_link(&list, ParamName);
}

static config_var *config_var_create(const char *ParamName, const char *ParamHelp)
{
    config_var *var = (config_var *) malloc(sizeof(config_var));

    if (var == NULL || ParamName == NULL)
        return NULL;

    memset(var, 0, sizeof(config_var));

    var->name = strdup(ParamName);
    if (var->name == NULL)
    {
        free(var);
        return NULL;
    }

    var->type = M64TYPE_INT;
    var->val.integer = 0;

    if (ParamHelp != NULL)
    {
        var->comment = strdup(ParamHelp);
        if (var->comment == NULL)
        {
            free(var->name);
            free(var);
            return NULL;
        }
    }
    else
        var->comment = NULL;

    var->next = NULL;
    return var;
}

static config_var *find_section_var(config_section *section, const char *ParamName)
{
    /* walk through the linked list of variables in the section */
    config_var *curr_var;
    for (curr_var = section->first_var; curr_var != NULL; curr_var = curr_var->next)
    {
        if (strcasecmp(ParamName, curr_var->name) == 0)
            return curr_var;
    }

    /* couldn't find this configuration parameter */
    return NULL;
}

static void append_var_to_section(config_section *section, config_var *var)
{
    config_var *last_var;

    if (section == NULL || var == NULL || section->magic != SECTION_MAGIC)
        return;

    if (section->first_var == NULL)
    {
        section->first_var = var;
        return;
    }

    last_var = section->first_var;
    while (last_var->next != NULL)
        last_var = last_var->next;

    last_var->next = var;
}

static void delete_var(config_var *var)
{
    if (var->type == M64TYPE_STRING)
        free(var->val.string);
    free(var->name);
    free(var->comment);
    free(var);
}

static void delete_section(config_section *pSection)
{
    config_var *curr_var;

    if (pSection == NULL)
        return;

    curr_var = pSection->first_var;
    while (curr_var != NULL)
    {
        config_var *next_var = curr_var->next;
        delete_var(curr_var);
        curr_var = next_var;
    }

    free(pSection->name);
    free(pSection);
}

static void delete_list(config_list *pConfigList)
{
    config_section *curr_section = *pConfigList;
    while (curr_section != NULL)
    {
        config_section *next_section = curr_section->next;
        /* delete the section itself */
        delete_section(curr_section);

        curr_section = next_section;
    }

    *pConfigList = NULL;
}

static config_section *config_section_create(const char *ParamName)
{
    config_section *sec;

    if (ParamName == NULL)
        return NULL;

    sec = (config_section *) malloc(sizeof(config_section));
    if (sec == NULL)
        return NULL;

    sec->magic = SECTION_MAGIC;
    sec->name = strdup(ParamName);
    if (sec->name == NULL)
    {
        free(sec);
        return NULL;
    }
    sec->first_var = NULL;
    sec->next = NULL;
    return sec;
}

static config_section * section_deepcopy(config_section *orig_section)
{
    config_section *new_section;
    config_var *orig_var, *last_new_var;

    /* Input validation */
    if (orig_section == NULL)
        return NULL;

    /* create and copy section struct */
    new_section = config_section_create(orig_section->name);
    if (new_section == NULL)
        return NULL;

    /* create and copy all section variables */
    orig_var = orig_section->first_var;
    last_new_var = NULL;
    while (orig_var != NULL)
    {
        config_var *new_var = config_var_create(orig_var->name, orig_var->comment);
        if (new_var == NULL)
        {
            delete_section(new_section);
            return NULL;
        }

        new_var->type = orig_var->type;
        switch (orig_var->type)
        {
            case M64TYPE_INT:
            case M64TYPE_BOOL:
                new_var->val.integer = orig_var->val.integer;
                break;
                
            case M64TYPE_FLOAT:
                new_var->val.number = orig_var->val.number;
                break;

            case M64TYPE_STRING:
                if (orig_var->val.string != NULL)
                {
                    new_var->val.string = strdup(orig_var->val.string);
                    if (new_var->val.string == NULL)
                    {
                        delete_section(new_section);
                        return NULL;
                    }
                }
                else
                    new_var->val.string = NULL;

                break;
        }

        /* add the new variable to the new section */
        if (last_new_var == NULL)
            new_section->first_var = new_var;
        else
            last_new_var->next = new_var;
        last_new_var = new_var;
        /* advance variable pointer in original section variable list */
        orig_var = orig_var->next;
    }

    return new_section;
}

static void copy_configlist_active_to_saved(void)
{
    config_section *curr_section = l_ConfigListActive;
    config_section *last_section = NULL;

    /* delete any pre-existing Saved config list */
    delete_list(&l_ConfigListSaved);

    /* duplicate all of the config sections in the Active list, adding them to the Saved list */
    while (curr_section != NULL)
    {
        config_section *new_section = section_deepcopy(curr_section);
        if (new_section == NULL) break;
        if (last_section == NULL)
            l_ConfigListSaved = new_section;
        else
            last_section->next = new_section;
        last_section = new_section;
        curr_section = curr_section->next;
    }
}

/* ----------------------------------------------------------- */
/* these functions are only to be used within the Core library */
/* ----------------------------------------------------------- */

m64p_error ConfigInit(const char *ConfigDirOverride, const char *DataDirOverride)
{
    if (l_ConfigInit)
        return M64ERR_ALREADY_INIT;
    l_ConfigInit = 1;

    return M64ERR_SUCCESS;
}

m64p_error ConfigShutdown(void)
{
    /* reset the initialized flag */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    l_ConfigInit = 0;

    /* free any malloc'd local variables */
    if (l_DataDirOverride != NULL)
    {
        free(l_DataDirOverride);
        l_DataDirOverride = NULL;
    }
    if (l_ConfigDirOverride != NULL)
    {
        free(l_ConfigDirOverride);
        l_ConfigDirOverride = NULL;
    }

    /* free all of the memory in the 2 lists */
    delete_list(&l_ConfigListActive);
    delete_list(&l_ConfigListSaved);

    return M64ERR_SUCCESS;
}

/* ------------------------------------------------ */
/* Selector functions, exported outside of the Core */
/* ------------------------------------------------ */

EXPORT m64p_error CALL ConfigListSections(void *context, void (*SectionListCallback)(void * context, const char * SectionName))
{
    config_section *curr_section;

    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (SectionListCallback == NULL)
        return M64ERR_INPUT_ASSERT;

    /* just walk through the section list, making a callback for each section name */
    curr_section = l_ConfigListActive;
    while (curr_section != NULL)
    {
        (*SectionListCallback)(context, curr_section->name);
        curr_section = curr_section->next;
    }

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigOpenSection(const char *SectionName, m64p_handle *ConfigSectionHandle)
{
    config_section **curr_section;
    config_section *new_section;

    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (SectionName == NULL || ConfigSectionHandle == NULL)
        return M64ERR_INPUT_ASSERT;

    /* walk through the section list, looking for a case-insensitive name match */
    curr_section = find_alpha_section_link(&l_ConfigListActive, SectionName);
    if (*curr_section != NULL && strcasecmp(SectionName, (*curr_section)->name) == 0)
    {
        *ConfigSectionHandle = *curr_section;
        return M64ERR_SUCCESS;
    }

    /* didn't find the section, so create new one */
    new_section = config_section_create(SectionName);
    if (new_section == NULL)
        return M64ERR_NO_MEMORY;

    /* add section to list in alphabetical order */
    new_section->next = *curr_section;
    *curr_section = new_section;

    *ConfigSectionHandle = new_section;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigListParameters(m64p_handle ConfigSectionHandle, void *context, void (*ParameterListCallback)(void * context, const char *ParamName, m64p_type ParamType))
{
    config_section *section;
    config_var *curr_var;

    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParameterListCallback == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* walk through this section's parameter list, making a callback for each parameter */
    curr_var = section->first_var;
    while (curr_var != NULL)
    {
        (*ParameterListCallback)(context, curr_var->name, curr_var->type);
        curr_var = curr_var->next;
    }

  return M64ERR_SUCCESS;
}

EXPORT int CALL ConfigHasUnsavedChanges(const char *SectionName)
{
    config_section *input_section, *curr_section;
    config_var *active_var, *saved_var;

    /* check input conditions */
    if (!l_ConfigInit)
    {
        DebugMessage(M64MSG_ERROR, "ConfigHasUnsavedChanges(): Core config not initialized!");
        return 0;
    }

    /* if SectionName is NULL or blank, then check all sections */
    if (SectionName == NULL || strlen(SectionName) < 1)
    {
        int iNumActiveSections = 0, iNumSavedSections = 0;
        /* first, search through all sections in Active list.  Recursively call ourself and return 1 if changed */
        curr_section = l_ConfigListActive;
        while (curr_section != NULL)
        {
            if (ConfigHasUnsavedChanges(curr_section->name))
                return 1;
            curr_section = curr_section->next;
            iNumActiveSections++;
        }
        /* Next, count the number of Saved sections and see if the count matches */
        curr_section = l_ConfigListSaved;
        while (curr_section != NULL)
        {
            curr_section = curr_section->next;
            iNumSavedSections++;
        }
        if (iNumActiveSections == iNumSavedSections)
            return 0;  /* no changes */
        else
            return 1;
    }

    /* walk through the Active section list, looking for a case-insensitive name match with input string */
    input_section = find_section(l_ConfigListActive, SectionName);
    if (input_section == NULL)
    {
        DebugMessage(M64MSG_ERROR, "ConfigHasUnsavedChanges(): section name '%s' not found!", SectionName);
        return 0;
    }

    /* walk through the Saved section list, looking for a case-insensitive name match */
    curr_section = find_section(l_ConfigListSaved, SectionName);
    if (curr_section == NULL)
    {
        /* if this section isn't present in saved list, then it has been newly created */
        return 1;
    }

    /* compare all of the variables in the two sections. They are expected to be in the same order */
    active_var = input_section->first_var;
    saved_var = curr_section->first_var;
    while (active_var != NULL && saved_var != NULL)
    {
        if (strcmp(active_var->name, saved_var->name) != 0)
            return 1;
        if (active_var->type != saved_var->type)
            return 1;
        switch(active_var->type)
        {
            case M64TYPE_INT:
                if (active_var->val.integer != saved_var->val.integer)
                    return 1;
                break;
            case M64TYPE_FLOAT:
                if (active_var->val.number != saved_var->val.number)
                    return 1;
                break;
            case M64TYPE_BOOL:
                if ((active_var->val.integer != 0) != (saved_var->val.integer != 0))
                    return 1;
                break;
            case M64TYPE_STRING:
                if (active_var->val.string == NULL)
                {
                    DebugMessage(M64MSG_ERROR, "ConfigHasUnsavedChanges(): Variable '%s' NULL Active string pointer!", active_var->name);
                    return 1;
                }
                if (saved_var->val.string == NULL)
                {
                    DebugMessage(M64MSG_ERROR, "ConfigHasUnsavedChanges(): Variable '%s' NULL Saved string pointer!", active_var->name);
                    return 1;
                }
                if (strcmp(active_var->val.string, saved_var->val.string) != 0)
                    return 1;
                break;
            default:
                DebugMessage(M64MSG_ERROR, "ConfigHasUnsavedChanges(): Invalid variable '%s' type %i!", active_var->name, active_var->type);
                return 1;
        }
        if (active_var->comment != NULL && saved_var->comment != NULL && strcmp(active_var->comment, saved_var->comment) != 0)
            return 1;
        active_var = active_var->next;
        saved_var = saved_var->next;
    }

    /* any extra new variables on the end, or deleted variables? */
    if (active_var != NULL || saved_var != NULL)
        return 1;

    /* exactly the same */
    return 0;
}

/* ------------------------------------------------------- */
/* Modifier functions, exported outside of the Core        */
/* ------------------------------------------------------- */

EXPORT m64p_error CALL ConfigDeleteSection(const char *SectionName)
{
    config_section **curr_section_link;
    config_section *next_section;

    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (l_ConfigListActive == NULL)
        return M64ERR_INPUT_NOT_FOUND;

    /* find the named section and pull it out of the list */
    curr_section_link = find_section_link(&l_ConfigListActive, SectionName);
    if (*curr_section_link == NULL)
        return M64ERR_INPUT_NOT_FOUND;

    next_section = (*curr_section_link)->next;

    /* delete the named section */
    delete_section(*curr_section_link);

    /* fix the pointer to point to the next section after the deleted one */
    *curr_section_link = next_section;

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSaveFile(void)
{
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;

    /* copy the active config list to the saved config list */
    copy_configlist_active_to_saved();

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSaveSection(const char *SectionName)
{
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigRevertChanges(const char *SectionName)
{
    config_section **active_section_link, *active_section, *saved_section, *new_section;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (SectionName == NULL)
        return M64ERR_INPUT_ASSERT;

    /* walk through the Active section list, looking for a case-insensitive name match with input string */
    active_section_link = find_section_link(&l_ConfigListActive, SectionName);
    active_section = *active_section_link;
    if (active_section == NULL)
        return M64ERR_INPUT_NOT_FOUND;

    /* walk through the Saved section list, looking for a case-insensitive name match */
    saved_section = find_section(l_ConfigListSaved, SectionName);
    if (saved_section == NULL)
    {
        /* if this section isn't present in saved list, then it has been newly created */
        return M64ERR_INPUT_NOT_FOUND;
    }

    /* copy the section as it is on the disk */
    new_section = section_deepcopy(saved_section);
    if (new_section == NULL)
        return M64ERR_NO_MEMORY;

    /* replace active_section with saved_section in the linked list */
    *active_section_link = new_section;
    new_section->next = active_section->next;

    /* release memory associated with active_section */
    delete_section(active_section);

    return M64ERR_SUCCESS;
}


/* ------------------------------------------------------- */
/* Generic Get/Set functions, exported outside of the Core */
/* ------------------------------------------------------- */

EXPORT m64p_error CALL ConfigSetParameter(m64p_handle ConfigSectionHandle, const char *ParamName, m64p_type ParamType, const void *ParamValue)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL || ParamValue == NULL || (int) ParamType < 1 || (int) ParamType > 4)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter doesn't already exist, then create it and add it to the section */
    var = find_section_var(section, ParamName);
    if (var == NULL)
    {
        var = config_var_create(ParamName, NULL);
        if (var == NULL)
            return M64ERR_NO_MEMORY;
        append_var_to_section(section, var);
    }

    /* cleanup old values */
    switch (var->type)
    {
        case M64TYPE_STRING:
            free(var->val.string);
	    break;
        default:
            break;
    }

    /* set this parameter's value */
    var->type = ParamType;
    switch(ParamType)
    {
        case M64TYPE_INT:
            var->val.integer = *((int *) ParamValue);
            break;
        case M64TYPE_FLOAT:
            var->val.number = *((float *) ParamValue);
            break;
        case M64TYPE_BOOL:
            var->val.integer = (*((int *) ParamValue) != 0);
            break;
        case M64TYPE_STRING:
            var->val.string = strdup((char *)ParamValue);
            if (var->val.string == NULL)
                return M64ERR_NO_MEMORY;
            break;
        default:
            /* this is logically impossible because of the ParamType check at the top of this function */
            break;
    }

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigGetParameter(m64p_handle ConfigSectionHandle, const char *ParamName, m64p_type ParamType, void *ParamValue, int MaxSize)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL || ParamValue == NULL || (int) ParamType < 1 || (int) ParamType > 4)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter doesn't already exist, return an error */
    var = find_section_var(section, ParamName);
    if (var == NULL)
        return M64ERR_INPUT_NOT_FOUND;

    /* call the specific Get function to translate the parameter to the desired type */
    switch(ParamType)
    {
        case M64TYPE_INT:
            if (MaxSize < sizeof(int)) return M64ERR_INPUT_INVALID;
            if (var->type != M64TYPE_INT && var->type != M64TYPE_FLOAT) return M64ERR_WRONG_TYPE;
            *((int *) ParamValue) = ConfigGetParamInt(ConfigSectionHandle, ParamName);
            break;
        case M64TYPE_FLOAT:
            if (MaxSize < sizeof(float)) return M64ERR_INPUT_INVALID;
            if (var->type != M64TYPE_INT && var->type != M64TYPE_FLOAT) return M64ERR_WRONG_TYPE;
            *((float *) ParamValue) = ConfigGetParamFloat(ConfigSectionHandle, ParamName);
            break;
        case M64TYPE_BOOL:
            if (MaxSize < sizeof(int)) return M64ERR_INPUT_INVALID;
            if (var->type != M64TYPE_BOOL && var->type != M64TYPE_INT) return M64ERR_WRONG_TYPE;
            *((int *) ParamValue) = ConfigGetParamBool(ConfigSectionHandle, ParamName);
            break;
        case M64TYPE_STRING:
        {
            const char *string;
            if (MaxSize < 1) return M64ERR_INPUT_INVALID;
            if (var->type != M64TYPE_STRING && var->type != M64TYPE_BOOL) return M64ERR_WRONG_TYPE;
            string = ConfigGetParamString(ConfigSectionHandle, ParamName);
            strncpy((char *) ParamValue, string, MaxSize);
            *((char *) ParamValue + MaxSize - 1) = 0;
            break;
        }
        default:
            /* this is logically impossible because of the ParamType check at the top of this function */
            break;
    }

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigGetParameterType(m64p_handle ConfigSectionHandle, const char *ParamName, m64p_type *ParamType)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL || ParamType == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter doesn't already exist, return an error */
    var = find_section_var(section, ParamName);
    if (var == NULL)
        return M64ERR_INPUT_NOT_FOUND;

    *ParamType = var->type;
    return M64ERR_SUCCESS;
}


EXPORT const char * CALL ConfigGetParameterHelp(m64p_handle ConfigSectionHandle, const char *ParamName)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit || ConfigSectionHandle == NULL || ParamName == NULL)
        return NULL;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return NULL;

    /* if this parameter doesn't exist, return an error */
    var = find_section_var(section, ParamName);
    if (var == NULL)
        return NULL;

    return var->comment;
}

/* ------------------------------------------------------- */
/* Special Get/Set functions, exported outside of the Core */
/* ------------------------------------------------------- */

EXPORT m64p_error CALL ConfigSetDefaultInt(m64p_handle ConfigSectionHandle, const char *ParamName, int ParamValue, const char *ParamHelp)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter already exists, then just return successfully */
    var = find_section_var(section, ParamName);
    if (var != NULL)
        return M64ERR_SUCCESS;

    /* otherwise create a new config_var object and add it to this section */
    var = config_var_create(ParamName, ParamHelp);
    if (var == NULL)
        return M64ERR_NO_MEMORY;
    var->type = M64TYPE_INT;
    var->val.integer = ParamValue;
    append_var_to_section(section, var);

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultFloat(m64p_handle ConfigSectionHandle, const char *ParamName, float ParamValue, const char *ParamHelp)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter already exists, then just return successfully */
    var = find_section_var(section, ParamName);
    if (var != NULL)
        return M64ERR_SUCCESS;

    /* otherwise create a new config_var object and add it to this section */
    var = config_var_create(ParamName, ParamHelp);
    if (var == NULL)
        return M64ERR_NO_MEMORY;
    var->type = M64TYPE_FLOAT;
    var->val.number = ParamValue;
    append_var_to_section(section, var);

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultBool(m64p_handle ConfigSectionHandle, const char *ParamName, int ParamValue, const char *ParamHelp)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter already exists, then just return successfully */
    var = find_section_var(section, ParamName);
    if (var != NULL)
        return M64ERR_SUCCESS;

    /* otherwise create a new config_var object and add it to this section */
    var = config_var_create(ParamName, ParamHelp);
    if (var == NULL)
        return M64ERR_NO_MEMORY;
    var->type = M64TYPE_BOOL;
    var->val.integer = ParamValue ? 1 : 0;
    append_var_to_section(section, var);

    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultString(m64p_handle ConfigSectionHandle, const char *ParamName, const char * ParamValue, const char *ParamHelp)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit)
        return M64ERR_NOT_INIT;
    if (ConfigSectionHandle == NULL || ParamName == NULL || ParamValue == NULL)
        return M64ERR_INPUT_ASSERT;

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
        return M64ERR_INPUT_INVALID;

    /* if this parameter already exists, then just return successfully */
    var = find_section_var(section, ParamName);
    if (var != NULL)
        return M64ERR_SUCCESS;

    /* otherwise create a new config_var object and add it to this section */
    var = config_var_create(ParamName, ParamHelp);
    if (var == NULL)
        return M64ERR_NO_MEMORY;
    var->type = M64TYPE_STRING;
    var->val.string = strdup(ParamValue);
    if (var->val.string == NULL)
    {
        delete_var(var);
        return M64ERR_NO_MEMORY;
    }
    append_var_to_section(section, var);

    return M64ERR_SUCCESS;
}

#include "libretro.h"
extern retro_environment_t environ_cb;

typedef struct { int value; const char* name; } value_pair;

static int choose_value(const char* value_name, const value_pair* values)
{
   int i;
   if (value_name)
   {
       printf("choose_value %s\n",value_name);

    //   struct retro_variable var = { value_name, 0 };
    //   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

    //   for (i = 0; var.value && values && values[i].name; i ++)
    //   {
    //      if (strcmp(values[i].name, var.value) == 0)
    //         return values[i].value;
    //   }
   }

   return -1;
}

EXPORT int CALL ConfigGetParamInt(m64p_handle ConfigSectionHandle, const char *ParamName)
{
   int i;
   config_section *section;
   config_var *var;
   static const struct
   {
      const char* ParamName;
      const char* RetroName;
      const value_pair Values[32];
   }   libretro_translate[] =
   {
      { "R4300Emulator", "parallel-n64-cpucore", { { 0, "pure_interpreter" }, { 1, "cached_interpreter" }, { 2, "dynamic_recompiler" }, { 3, "neb_dynamic_recompiler" }, { 0, 0 } } },
      { "ScreenWidth", "parallel-n64-screensize", { 
                                                { 320, "320x200" },
                                                { 320, "320x240" },
                                                { 400, "400x256" },
                                                { 512, "512x384" },
                                                { 640, "640x200" },
                                                { 640, "640x350" },
                                                { 640, "640x400" },
                                                { 640, "640x480" },
                                                { 800, "800x600" },
                                                { 960, "960x720" },
                                                { 856,  "856x480" },
                                                { 512,  "512x256" },
                                                { 1024, "1024x768" },
                                                { 1280, "1280x1024" },
                                                { 1600, "1600x1200" },
                                                { 400, "400x300" },
                                                { 1152, "1152x864" },
                                                { 1280, "1280x960" },
                                                { 1600, "1600x1024" },
                                                { 1920, "1920x1440" },
                                                { 2048, "2048x1536" },
                                                { 2048, "2048x2048" },
                                                { 0, 0 } 
                                             }
      },
      { "ScreenHeight", "parallel-n64-screensize", { 
                                                 { 200, "320x200" },
                                                 { 240, "320x240" },
                                                 { 256, "400x256" },
                                                 { 384, "512x384" },
                                                 { 200, "640x200" },
                                                 { 350, "640x350" },
                                                 { 400, "640x400" },
                                                 { 480, "640x480" },
                                                 { 600, "800x600" },
                                                 { 720, "960x720" },
                                                 { 480, "856x480" },
                                                 { 256, "512x256" },
                                                 { 768, "1024x768" },
                                                 { 1024, "1280x1024" },
                                                 { 1200, "1600x1200" },
                                                 { 300, "400x300" },
                                                 { 854, "1152x864" },
                                                 { 960, "1280x960" },
                                                 { 1024, "1600x1024" },
                                                 { 1440, "1920x1440" },
                                                 { 1536, "2048x1536" },
                                                 { 2048, "2048x2048" },
                                                 { 0, 0 } 
                                              } 
      },
      { "DisableExtraMem", "parallel-n64-disable_expmem", {
                                                        { 0, "enabled" },
                                                        { 1, "disabled" },
                                                     }
      },
      {
         "BootDevice", "parallel-n64-boot-device",
         {
            { 0, "Default" },
            { 1, "64DD IPL" },
         }
      }, 
      { 0, 0, { {0, 0} } }
   };

   if (!strcmp(ParamName, "AnisoFilter"))
#ifdef HAVE_OPENGLES
      return 0;
#else
   return 1;
#endif

   for (i = 0; libretro_translate[i].ParamName; i ++)
   {
      if (strcmp(ParamName, libretro_translate[i].ParamName) == 0)
      {
         int result = choose_value(libretro_translate[i].RetroName, libretro_translate[i].Values);
         if (result >= 0)
            return result;
         break;
      }
   }

   /* check input conditions */
   if (!l_ConfigInit || ConfigSectionHandle == NULL || ParamName == NULL)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamInt(): Input assertion!");
      return 0;
   }

   section = (config_section *) ConfigSectionHandle;
   if (section->magic != SECTION_MAGIC)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamInt(): ConfigSectionHandle invalid!");
      return 0;
   }

   /* if this parameter doesn't already exist, return an error */
   var = find_section_var(section, ParamName);
   if (var == NULL)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamInt(): Parameter '%s' not found!", ParamName);
      return 0;
   }

   /* translate the actual variable type to an int */
   switch(var->type)
   {
      case M64TYPE_INT:
         return var->val.integer;
      case M64TYPE_FLOAT:
         return (int) var->val.number;
      case M64TYPE_BOOL:
         return (var->val.integer != 0);
      case M64TYPE_STRING:
         return atoi(var->val.string);
      default:
         DebugMessage(M64MSG_ERROR, "ConfigGetParamInt(): invalid internal parameter type for '%s'", ParamName);
         return 0;
   }

   return 0;
}

EXPORT float CALL ConfigGetParamFloat(m64p_handle ConfigSectionHandle, const char *ParamName)
{
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit || ConfigSectionHandle == NULL || ParamName == NULL)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamFloat(): Input assertion!");
        return 0.0;
    }

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamFloat(): ConfigSectionHandle invalid!");
        return 0.0;
    }

    /* if this parameter doesn't already exist, return an error */
    var = find_section_var(section, ParamName);
    if (var == NULL)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamFloat(): Parameter '%s' not found!", ParamName);
        return 0.0;
    }

    /* translate the actual variable type to an int */
    switch(var->type)
    {
        case M64TYPE_INT:
            return (float) var->val.integer;
        case M64TYPE_FLOAT:
            return var->val.number;
        case M64TYPE_BOOL:
            return (var->val.integer != 0) ? 1.0f : 0.0f;
        case M64TYPE_STRING:
            return (float) atof(var->val.string);
        default:
            DebugMessage(M64MSG_ERROR, "ConfigGetParamFloat(): invalid internal parameter type for '%s'", ParamName);
            return 0.0;
    }

    return 0.0;
}


EXPORT int CALL ConfigGetParamBool(m64p_handle ConfigSectionHandle, const char *ParamName)
{
   int i;
   config_section *section;
   config_var *var;
   static const struct
   {
      const char* ParamName;
      const char* RetroName;
      const value_pair Values[32];
   }   libretro_translate[] =
   {
      { "64DD", "parallel-n64-64dd-hardware",
         {
            { 0, "disabled" }, { 1, "enabled" }
         }
      },
      { 0, 0, { {0, 0} } }
   };

   for (i = 0; libretro_translate[i].ParamName; i ++)
   {
      if (strcmp(ParamName, libretro_translate[i].ParamName) == 0)
      {
         int result = choose_value(libretro_translate[i].RetroName, libretro_translate[i].Values);
         if (result >= 0)
            return result;
         break;
      }
   }


   if (!strcmp(ParamName, "DisplayListToGraphicsPlugin"))
   {
      return CFG_HLE_GFX;
   }
   if (!strcmp(ParamName, "AudioListToAudioPlugin"))
   {
      return CFG_HLE_AUD;
   }
   if (!strcmp(ParamName, "WaitForCPUHost"))
   {
      return false;
   }
   if (!strcmp(ParamName, "SupportCPUSemaphoreLock"))
   {
      return false;
   }
#ifdef HAVE_THR_AL
   {
      extern unsigned angrylion_get_vi(void);
      if (!strcmp(ParamName, "VIOverlay"))
         return (int)angrylion_get_vi();
   }
#endif

   if (!strcmp(ParamName, "Fullscreen"))
      return true;
   if (!strcmp(ParamName, "VerticalSync"))
      return false;
   if (!strcmp(ParamName, "FBO"))
      return true;
   if (!strcmp(ParamName, "AnisotropicFiltering"))
#ifdef HAVE_OPENGLES
      return false;
#else
   return true;
#endif

   /* check input conditions */
   if (!l_ConfigInit || ConfigSectionHandle == NULL || ParamName == NULL)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamBool(): Input assertion!");
      return 0;
   }

   section = (config_section *) ConfigSectionHandle;
   if (section->magic != SECTION_MAGIC)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamBool(): ConfigSectionHandle invalid!");
      return 0;
   }

   /* if this parameter doesn't already exist, return an error */
   var = find_section_var(section, ParamName);
   if (var == NULL)
   {
      DebugMessage(M64MSG_ERROR, "ConfigGetParamBool(): Parameter '%s' not found!", ParamName);
      return 0;
   }

   /* translate the actual variable type to an int */
   switch(var->type)
   {
      case M64TYPE_INT:
         return (var->val.integer != 0);
      case M64TYPE_FLOAT:
         return (var->val.number != 0.0);
      case M64TYPE_BOOL:
         return var->val.integer;
      case M64TYPE_STRING:
         return (strcasecmp(var->val.string, "true") == 0);
      default:
         DebugMessage(M64MSG_ERROR, "ConfigGetParamBool(): invalid internal parameter type for '%s'", ParamName);
         return 0;
   }

   return 0;
}

EXPORT const char * CALL ConfigGetParamString(m64p_handle ConfigSectionHandle, const char *ParamName)
{
    static char outstr[64];  /* warning: not thread safe */
    config_section *section;
    config_var *var;

    /* check input conditions */
    if (!l_ConfigInit || ConfigSectionHandle == NULL || ParamName == NULL)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamString(): Input assertion!");
        return "";
    }

    section = (config_section *) ConfigSectionHandle;
    if (section->magic != SECTION_MAGIC)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamString(): ConfigSectionHandle invalid!");
        return "";
    }

    /* if this parameter doesn't already exist, return an error */
    var = find_section_var(section, ParamName);
    if (var == NULL)
    {
        DebugMessage(M64MSG_ERROR, "ConfigGetParamString(): Parameter '%s' not found!", ParamName);
        return "";
    }

    /* translate the actual variable type to an int */
    switch(var->type)
    {
        case M64TYPE_INT:
            snprintf(outstr, 63, "%i", var->val.integer);
            outstr[63] = 0;
            return outstr;
        case M64TYPE_FLOAT:
            snprintf(outstr, 63, "%f", var->val.number);
            outstr[63] = 0;
            return outstr;
        case M64TYPE_BOOL:
            return (var->val.integer ? "True" : "False");
        case M64TYPE_STRING:
            return var->val.string;
        default:
            DebugMessage(M64MSG_ERROR, "ConfigGetParamString(): invalid internal parameter type for '%s'", ParamName);
            return "";
    }

  return "";
}

/* ------------------------------------------------------ */
/* OS Abstraction functions, exported outside of the Core */
/* ------------------------------------------------------ */
extern const char* retro_get_system_directory();

EXPORT const char * CALL ConfigGetSharedDataFilepath(const char *filename)
{
  static char configpath[PATH_MAX_LENGTH];
  if (filename == NULL) return NULL;

//   printf("neil why am I here2\n");
  //snprintf(configpath, PATH_MAX_LENGTH, "%s/%s", retro_get_system_directory(), filename);

  /* TODO: Return NULL if file does not exist */

  return configpath;
}

EXPORT const char * CALL ConfigGetUserConfigPath(void)
{
    // printf("neil why am I here2a\n");
    return NULL;
  //return retro_get_system_directory();
}

EXPORT const char * CALL ConfigGetUserDataPath(void)
{
    // printf("neil why am I here2b\n");
    return NULL;
  //return retro_get_system_directory();
}

EXPORT const char * CALL ConfigGetUserCachePath(void)
{
    // printf("neil why am I here2c\n");
    return NULL;
  //return retro_get_system_directory();
}

