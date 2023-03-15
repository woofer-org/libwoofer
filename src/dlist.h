/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * dlist.h  This file is part of LibWoofer
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

#ifndef __WF_DLIST__
#define __WF_DLIST__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef struct _WfDList WfDList;

struct _WfDList
{
    gpointer key;
    gpointer value;
    WfDList *next;
    WfDList *prev;
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

WfDList * wf_d_list_add(WfDList *list, gpointer key, gpointer value);

WfDList * wf_d_list_remove_any(WfDList *list, gpointer key, gpointer value);
WfDList * wf_d_list_remove_all(WfDList *list, gpointer key, gpointer value);

WfDList * wf_d_list_last(WfDList *list);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_d_list_free(WfDList *list);

/* DESTRUCTOR PROTOTYPES END */

G_END_DECLS

#endif /* __WF_DLIST__ */

/* END OF FILE */
