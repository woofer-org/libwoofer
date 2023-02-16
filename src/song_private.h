/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song_private.h  This file is part of LibWoofer
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

#ifndef __WF_SONG_PRIVATE__
#define __WF_SONG_PRIVATE__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

void wf_song_set_tag(WfSong *song, const gchar *tag);

gint64 wf_song_get_metadata_updated(const WfSong *song);
void wf_song_set_metadata_updated(WfSong *song, gint64 timestamp);
void wf_song_set_metadata_updated_now(WfSong *song);

void wf_song_refresh_locations(WfSong *song);

void wf_song_set_title(WfSong *song, const gchar *title);
void wf_song_set_artist(WfSong *song, const gchar *artist);
void wf_song_set_album_artist(WfSong *song, const gchar *artist);
void wf_song_set_album(WfSong *song, const gchar *album);
void wf_song_set_track_number(WfSong *song, gint number);
void wf_song_set_duration_seconds(WfSong *song, gint seconds);
void wf_song_set_duration_nanoseconds(WfSong *song, gint64 nanoseconds);

void wf_song_set_queued(WfSong *song, gboolean value);
void wf_song_set_stop_flag(WfSong *song, gboolean value);
void wf_song_set_status(WfSong *song, WfSongStatus state);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean wf_song_is_unique(WfSong *song);
gboolean wf_song_is_unique_uri(const gchar *uri);

void wf_song_reset_stats(WfSong *song);

void wf_song_remove_all(void);

void wf_song_set_fs_info(WfSong *song, GFileInfo *info);
gboolean wf_song_update_metadata(WfSong *song, gboolean force);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_SONG_PRIVATE__ */

/* END OF FILE */
