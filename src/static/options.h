/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * static/options.h  This file is part of LibWoofer
 * Copyright (C) 2021, 2022  Quico Augustijn
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

#ifndef __WF_STATIC_OPTIONS__
#define __WF_STATIC_OPTIONS__

/* INCLUDES BEGIN */

#include <gio/gio.h>

/* INCLUDES END */

/* MODULE TYPES BEGIN */

typedef struct _WfApplicationEntries WfApplicationEntries;

struct _WfApplicationEntries
{
	/* Hidden options */
	gboolean shortlist;
	gchar *name;
	gchar *icon;
	gchar *desktop_entry;

	/* Startup options */
	gchar *config;
	gchar *library;
	gboolean background;

	/* Runtime options */
	gboolean play_pause;
	gboolean play;
	gboolean pause;
	gboolean previous;
	gboolean next;
	gboolean stop;

	/* Miscellaneous options */
	gboolean verbose;
	gboolean version;
};

/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

const WfApplicationEntries * wf_option_entries_get_entries(void);

const GOptionEntry * wf_option_entries_get_main(void);
const GOptionEntry * wf_option_entries_get_app(void);

/* FUNCTION PROTOTYPES END */

#endif /* __WF_STATIC_OPTIONS__ */

/* END OF FILE */
