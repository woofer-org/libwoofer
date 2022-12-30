/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * memory.c  This file is part of LibWoofer
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

/* INCLUDES BEGIN */

// Library includes
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/memory.h>

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This modules provides wrappers for library calls to include %NULL checks and
 * variable reinitialization.  This to improve overall application stability and
 * reliability by preventing one of the common memory errors: Reusing memory
 * after it has been freed.
 *
 * Since this module only contains utilities for other modules, all of these
 * "utilities" are part of the normal module functions and constructors,
 * destructors, etc. are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

// Set one or more GObject properties if @object is not %NULL, return otherwise
void
wf_memory_g_object_set(GObject *object, const gchar *property_name, ...)
{
	va_list args;

	if (object != NULL && G_IS_OBJECT(object))
	{
		va_start(args, property_name);

		g_object_set_valist(object, property_name, args);

		va_end(args);
	}
}

void
wf_memory_clear_str(gchar **string)
{
	if (string == NULL || *string == NULL)
	{
		return;
	}
	else
	{
		g_free(*string);

		*string = NULL;
	}
}

void
wf_memory_clear_pointer(gpointer *mem)
{
	if (mem == NULL || *mem == NULL)
	{
		return;
	}
	else
	{
		g_free(*mem);

		*mem = NULL;
	}
}

void
wf_memory_clear_object(GObject **obj)
{
	if (obj == NULL || *obj == NULL)
	{
		return;
	}
	else
	{
		g_object_unref(*obj);

		*obj = NULL;
	}
}

void
wf_memory_clear_date_time(GDateTime **dt)
{
	if (dt == NULL || *dt == NULL)
	{
		return;
	}
	else
	{
		g_date_time_unref(*dt);
	}
}

void
wf_memory_clear_variant(GVariant **value)
{
	if (value == NULL || *value == NULL)
	{
		return;
	}
	else
	{
		g_variant_unref(*value);
	}
}

void
wf_memory_clear_key_file(GKeyFile **key_file)
{
	if (key_file == NULL || *key_file == NULL)
	{
		return;
	}
	else
	{
		g_key_file_free(*key_file);

		*key_file = NULL;
	}
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
