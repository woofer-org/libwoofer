/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * tweaks.c  This file is part of LibWoofer
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
#include <gio/gio.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/tweaks.h>

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This source file provides workaround or tweaks for some deprecated functions
 * or differences between systems, so the whole application can be successfully
 * build on any system currently supported.
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

// Compatibility workaround for g_file_info_get_modification_date_time
GDateTime *
wf_tweaks_file_info_get_modification_date_time(GFileInfo *info)
{
#if GLIB_MINOR_VERSION < 62 // Latest version for which to use the workaround
	// Values needed
	GTimeVal timeval;
	GDateTime *dt = NULL, *dt2 = NULL;

	// Preconditions
	g_return_val_if_fail(G_IS_FILE_INFO(info), NULL);

	// Old deprecated function
	g_file_info_get_modification_time(info, &timeval);

	// Corresponds to the used _g_file_attribute_value_get_uint64
	if (timeval.tv_sec == 0)
	{
		return NULL;
	}

	// Convert
	dt = g_date_time_new_from_unix_utc(timeval.tv_sec);

	// Corresponds to the used _g_file_attribute_value_get_uint32
	if (timeval.tv_usec == 0)
	{
		return g_steal_pointer(&dt);
	}

	dt2 = g_date_time_add(dt, timeval.tv_usec);
	g_date_time_unref(dt);

	return g_steal_pointer(&dt2);
#else // For versions that support the new call
	return g_file_info_get_modification_date_time(info);
#endif // GLIB_MINOR_VERSION < 62
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
