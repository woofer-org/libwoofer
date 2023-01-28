/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * library.h  This file is part of LibWoofer
 * Copyright (C) 2021-2023  Quico Augustijn
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

#ifndef __WF_LIBRARY__
#define __WF_LIBRARY__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */

#define WF_LIBRARY_FILENAME "library.conf"

#define WF_LIBRARY_CHECK_DEFAULT WF_LIBRARY_CHECK_AUDIO

/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfLibraryFileChecks WfLibraryFileChecks;

typedef void (*WfFuncItemAdded) (WfSong *song, gint item, gint total);
typedef void (*WfFuncStatsUpdated) (void);

enum _WfLibraryFileChecks
{
	WF_LIBRARY_CHECK_NONE = 1,
	WF_LIBRARY_CHECK_AUDIO = 2,
	WF_LIBRARY_CHECK_MEDIA = 3
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

void wf_library_connect_event_stats_updated(WfFuncStatsUpdated cb_func);

void wf_library_set_file(const gchar *file_path);

gboolean wf_library_track_number_column_is_empty(void);
gboolean wf_library_title_column_is_empty(void);
gboolean wf_library_artist_column_is_empty(void);
gboolean wf_library_album_column_is_empty(void);
gboolean wf_library_duration_column_is_empty(void);

gboolean wf_library_track_number_column_is_full(void);
gboolean wf_library_title_column_is_full(void);
gboolean wf_library_artist_column_is_full(void);
gboolean wf_library_album_column_is_full(void);

GList * wf_library_get(void);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_library_move_before(WfSong *song, WfSong *other_song);
void wf_library_move_after(WfSong *song, WfSong *other_song);

gboolean wf_library_write(gboolean force);

gint wf_library_update_metadata(void);

gint wf_library_add_by_file(GFile *file, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);
gint wf_library_add_by_uri(const gchar *uri, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);
gint wf_library_add_strv(gchar *files[], WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);
gint wf_library_add_uris(GSList *files, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);
gint wf_library_add_files(GSList *files, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);

void wf_library_update_column_info(void);

void wf_library_remove_song(WfSong *song);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_LIBRARY__ */

/* END OF FILE */
