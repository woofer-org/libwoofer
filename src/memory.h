/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * memory.h  This file is part of LibWoofer
 * Copyright (C) 2022  Quico Augustijn
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

#ifndef __WF_MEMORY__
#define __WF_MEMORY__

/* INCLUDES BEGIN */

#include <stdarg.h>

#include <glib.h>
#include <glib-object.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_memory_g_object_set(GObject *object, const gchar *property_name, ...);

void wf_memory_clear_str(gchar **string);

void wf_memory_clear_pointer(gpointer *mem);
void wf_memory_clear_object(GObject **obj);
void wf_memory_clear_date_time(GDateTime **dt);
void wf_memory_clear_variant(GVariant **value);
void wf_memory_clear_key_file(GKeyFile **key_file);

/* FUNCTION PROTOTYPES END */

#endif /* __WF_MEMORY__ */

/* END OF FILE */
