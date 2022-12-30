/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * dlist.c  This file is part of LibWoofer
 * Copyright (C) 2021  Quico Augustijn
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed "as is" in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
 * computer no longer boots, divides by 0 or explodes, you are the only
 * one responsible.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this library.  If not, see
 * <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/dlist.h>

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * WfDList is adapted from glist.c (glib library), but utilizes two data
 * pointers (a key and value pair) instead of one (dual-data list, aka dlist).
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static WfDList * wf_d_list_alloc0(void);

static WfDList * wf_d_list_remove_link(WfDList *list, WfDList *link);

static void wf_d_list_free1(WfDList *list_node);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

static WfDList *
wf_d_list_alloc0(void)
{
	return g_slice_new0(WfDList);
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

WfDList *
wf_d_list_add(WfDList *list, gpointer key, gpointer value)
{
	WfDList *new_list;
	WfDList *last;

	new_list = wf_d_list_alloc0();
	new_list->key = key;
	new_list->value = value;
	new_list->next = NULL;

	if (list != NULL)
	{
		last = wf_d_list_last(list);
		last->next = new_list;
		new_list->prev = last;

		return list;
	}
	else
	{
		new_list->prev = NULL;

		return new_list;
	}
}

/*
 * Modified g_list_remove(), but only the key or the value has to match.
 * Key or value can be %NULL and an element can still be removed.
 */
WfDList *
wf_d_list_remove_any(WfDList *list, gpointer key, gpointer value)
{
	WfDList *tmp;

	for (tmp = list; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->key == key || tmp->value == value)
		{
			list = wf_d_list_remove_link(list, tmp);
			wf_d_list_free1(tmp);

			break;
		}
	}

	return list;
}

/*
 * Modified g_list_remove_all(), but both the key and the value pointers need to
 * match the pointers in an list element in order for the it to be removed.
 */
WfDList *
wf_d_list_remove_all(WfDList *list, gpointer key, gpointer value)
{
	WfDList *next, *tmp;

	tmp = list;

	while (tmp != NULL)
	{
		if (tmp->key == key && tmp->value == value)
		{
			next = tmp->next;

			if (tmp->prev != NULL)
			{
				tmp->prev->next = next;
			}
			else
			{
				list = next;
			}

			if (next != NULL)
			{
				next->prev = tmp->prev;
			}

			wf_d_list_free1(tmp);
			tmp = next;
		}
		else
		{
			tmp = tmp->next;
		}
	}

	return list;
}

static WfDList *
wf_d_list_remove_link(WfDList *list, WfDList *link)
{
	if (link == NULL)
	{
		return list;
	}

	if (link->prev != NULL)
	{
		if (link->prev->next == link)
		{
			link->prev->next = link->next;
		}
		else
		{
			g_warning("Corrupted double-data list detected");
		}
	}

	if (link->next != NULL)
	{
		if (link->next->prev == link)
		{
			link->next->prev = link->prev;
		}
		else
		{
			g_warning("Corrupted double-data list detected");
		}
	}

	if (link == list)
	{
		list = list->next;
	}

	link->next = NULL;
	link->prev = NULL;

	return list;
}

WfDList *
wf_d_list_last(WfDList *list)
{
	if (list != NULL)
	{
		while (list->next != NULL)
		{
			list = list->next;
		}
	}

	return list;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */
/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_d_list_free(WfDList *list)
{
	g_slice_free_chain(WfDList, list, next);
}

static void
wf_d_list_free1(WfDList *list_node)
{
	g_slice_free(WfDList, list_node);
}

/* DESTRUCTORS END */

/* END OF FILE */
