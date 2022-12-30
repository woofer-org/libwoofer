/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song.h  This file is part of LibWoofer
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

#ifndef __WF_SONG__
#define __WF_SONG__

/* INCLUDES BEGIN */

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

/* INCLUDES END */

/* DEFINES BEGIN */

#define WF_TYPE_SONG (wf_song_get_type())
#define WF_SONG(song) (G_TYPE_CHECK_INSTANCE_CAST(song, WF_TYPE_SONG, WfSong))
#define WF_SONG_GET_CLASS(song) (G_TYPE_INSTANCE_GET_CLASS(song, WF_TYPE_SONG, WfSongClass))
#define WF_SONG_CLASS(song_class) (G_TYPE_CHECK_CLASS_CAST(song_class, WF_TYPE_SONG, WfSongClass))
#define WF_IS_SONG(song) (G_TYPE_CHECK_INSTANCE_TYPE(song, WF_TYPE_SONG))
#define WF_IS_SONG_CLASS(song_class) (G_TYPE_CHECK_CLASS_TYPE(song_class, WF_TYPE_SONG))

#define WF_TYPE_SONG_STATUS (wf_song_get_enum_status_type())

/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfSongStatus WfSongStatus;

typedef struct _WfSong WfSong;
typedef struct _WfSongClass WfSongClass;
typedef struct _WfSongPrivate WfSongPrivate;

struct _WfSong
{
	/*< private >*/
	GObject parent_instance;

	WfSongPrivate *priv;
};

struct _WfSongClass
{
	/*< private >*/
	GObjectClass parent_class;
};

/**
 * WfSongStatus:
 * @WF_SONG_STATUS_UNKNOWN: Unknown song status
 * @WF_SONG_AVAILABLE: Song is available to be played
 * @WF_SONG_PLAYING: Song is currently being played
 * @WF_SONG_NOT_FOUND: Song is in the library, but not found or
 * 	readable on disk
 *
 * Represents the current status of a particular song.
 **/
enum _WfSongStatus
{
	WF_SONG_STATUS_UNKNOWN,
	WF_SONG_AVAILABLE,
	WF_SONG_PLAYING,
	WF_SONG_NOT_FOUND,
	WF_SONG_DEFINED /* Validation checker */
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

gint wf_song_get_count(void);

WfSong * wf_song_get_first(void);
WfSong * wf_song_get_last(void);
WfSong * wf_song_get_next(const WfSong *song);
WfSong * wf_song_get_prev(const WfSong *song);
WfSong * wf_song_get_by_hash(guint32 hash);

void wf_song_move_before(WfSong *song, WfSong *other_song);
void wf_song_move_after(WfSong *song, WfSong *other_song);

guint32 wf_song_get_hash(WfSong *song);
const gchar * wf_song_get_tag(WfSong *song);

GFile * wf_song_get_file(WfSong *song);
const gchar * wf_song_get_uri(const WfSong *song);
const gchar * wf_song_get_name(const WfSong *song);
const gchar * wf_song_get_name_not_empty(const WfSong *song);
const gchar * wf_song_get_display_name(const WfSong *song);
gint64 wf_song_get_modified(const WfSong *song);

const gchar * wf_song_get_title(const WfSong *song);
const gchar * wf_song_get_artist(const WfSong *song);
const gchar * wf_song_get_album_artist(const WfSong *song);
guint32 wf_song_get_artist_hash(const WfSong *song);
const gchar * wf_song_get_album(const WfSong *song);
gint wf_song_get_track_number(const WfSong *song);
gint wf_song_get_duration(const WfSong *song);
gchar * wf_song_get_duration_string(const WfSong *song);

gboolean wf_song_is_rating_unset(const WfSong *song);
void wf_song_set_rating(WfSong *song, gint rating);
gint wf_song_get_rating(const WfSong *song);
void wf_song_set_score(WfSong *song, gdouble score);
gdouble wf_song_get_score(const WfSong *song);
void wf_song_set_play_count(WfSong *song, gint playcount);
gint wf_song_get_play_count(const WfSong *song);
void wf_song_set_skip_count(WfSong *song, gint skipcount);
gint wf_song_get_skip_count(const WfSong *song);
void wf_song_set_last_played(WfSong *song, gint64 lastplayed);
gint64 wf_song_get_last_played(const WfSong *song);
gchar * wf_song_get_played_on_as_string(const WfSong *song);
gchar * wf_song_get_last_played_as_string(const WfSong *song);

gboolean wf_song_is_in_list(const WfSong *song);
gboolean wf_song_get_queued(const WfSong *song);
gboolean wf_song_get_stop_flag(const WfSong *song);
WfSongStatus wf_song_get_status(const WfSong *song);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean wf_song_is_valid(const WfSong *song);

GType wf_song_get_type(void);
GType wf_song_get_enum_status_type(void);

WfSong * wf_song_prepend_by_uri(const gchar *uri);
WfSong * wf_song_prepend_by_file(GFile *file);
WfSong * wf_song_append_by_uri(const gchar *uri);
WfSong * wf_song_append_by_file(GFile *file);
void wf_song_remove(WfSong *song);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_SONG__ */

/* END OF FILE */
