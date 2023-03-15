/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * mpris.h  This file is part of LibWoofer
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

#ifndef __WF_MPRIS__
#define __WF_MPRIS__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfPlaybackStatus WfPlaybackStatus;
typedef enum _WfInterfaceProperty WfInterfaceProperty;

enum _WfPlaybackStatus
{
	WF_MPRIS_NOT_PLAYING,
	WF_MPRIS_PLAYING,
	WF_MPRIS_PAUSED,
	WF_MPRIS_STOPPED,
	WF_MPRIS_DEFINED
};

/*
 * Enumeration used to indicate which property has been changed by a client;
 * these can only be properties that are read/writable for clients, as
 * specified by the MediaPlayer2 interface specifications.
 */
enum _WfInterfaceProperty
{
	WF_MPRIS_PROP_NONE,
	WF_MPRIS_PROP_ROOT_FULLSCREEN,
	WF_MPRIS_PROP_PLAYER_LOOP_STATUS,
	WF_MPRIS_PROP_PLAYER_RATE,
	WF_MPRIS_PROP_PLAYER_SHUFFLE,
	WF_MPRIS_PROP_PLAYER_VOLUME,
	WF_MPRIS_PROP_DEFINED /* Validation checker */
};

/*
 * Callbacks to functions that will be called when the user interacts with any
 * desktop environment widget.
 */
typedef void (*WfFuncRootRaise) (void);
typedef void (*WfFuncRootQuit) (void);
typedef void (*WfFuncPlayerNext) (void);
typedef void (*WfFuncPlayerPrevious) (void);
typedef void (*WfFuncPlayerPause) (void);
typedef void (*WfFuncPlayerPlayPause) (void);
typedef void (*WfFuncPlayerStop) (void);
typedef void (*WfFuncPlayerPlay) (void);
typedef void (*WfFuncPlayerSeek) (gint64 offset);
typedef void (*WfFuncPlayerSetPosition) (const gchar *track_id, gint64 position);
typedef void (*WfFuncPlayerOpenUri) (const gchar *uri);

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

gboolean wf_mpris_activate(void);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

/* Root interface getters/setters */
void wf_mpris_set_root_can_raise(gboolean can_raise);
GVariant * wf_mpris_get_root_can_raise(void);
void wf_mpris_set_root_can_quit(gboolean can_quit);
GVariant * wf_mpris_get_root_can_quit(void);
void wf_mpris_set_root_can_set_fullscreen(gboolean can_set_fullscreen);
GVariant * wf_mpris_get_root_can_set_fullscreen(void);
void wf_mpris_set_root_has_track_list(gboolean has_track_list);
GVariant * wf_mpris_get_root_has_track_list(void);
void wf_mpris_set_root_fullscreen(gboolean fullscreen);
GVariant * wf_mpris_get_root_fullscreen(void);
void wf_mpris_set_root_identity(const gchar *identity);
GVariant * wf_mpris_get_root_identity(void);
void wf_mpris_set_root_desktop_entry(const gchar *desktop_entry);
GVariant * wf_mpris_get_root_desktop_entry(void);
void wf_mpris_set_root_supported_uri_schemes(const gchar * const *uri_schemes);
GVariant * wf_mpris_get_root_supported_uri_schemes(void);
void wf_mpris_set_root_supported_mime_types(const gchar * const *mime_types);
GVariant * wf_mpris_get_root_supported_mime_types(void);

/* Player interface getters/setters */
void wf_mpris_set_player_playback_status(WfPlaybackStatus status);
GVariant * wf_mpris_get_player_playback_status(void);
void wf_mpris_set_player_rate(gdouble rate);
GVariant * wf_mpris_get_player_rate(void);
GVariant * wf_mpris_get_player_metadata(void);
void wf_mpris_set_player_volume(gdouble volume);
GVariant * wf_mpris_get_player_volume(void);
void wf_mpris_set_player_position(gint64 position);
GVariant * wf_mpris_get_player_position(void);
void wf_mpris_set_player_minimum_rate(gdouble minimum_rate);
GVariant * wf_mpris_get_player_minimum_rate(void);
void wf_mpris_set_player_maximum_rate(gdouble maximum_rate);
GVariant * wf_mpris_get_player_maximum_rate(void);
void wf_mpris_set_player_can_go_next(gboolean can_go_next);
GVariant * wf_mpris_get_player_can_go_next(void);
void wf_mpris_set_player_can_go_previous(gboolean can_go_previous);
GVariant * wf_mpris_get_player_can_go_previous(void);
void wf_mpris_set_player_can_play(gboolean can_play);
GVariant * wf_mpris_get_player_can_play(void);
void wf_mpris_set_player_can_pause(gboolean can_pause);
GVariant * wf_mpris_get_player_can_pause(void);
void wf_mpris_set_player_can_seek(gboolean can_seek);
GVariant * wf_mpris_get_player_can_seek(void);
void wf_mpris_set_player_can_control(gboolean can_control);
GVariant * wf_mpris_get_player_can_control(void);

/* Metadata setters */
void wf_mpris_set_info_track_id(guint track_id);
void wf_mpris_set_info_url(const gchar *url);
void wf_mpris_set_info_title(const gchar *title);
void wf_mpris_set_info_album(const gchar *album);
void wf_mpris_set_info_artists(const gchar * const *artists);
void wf_mpris_set_info_album_artists(const gchar * const *album_artists);
void wf_mpris_set_info_composers(const gchar * const *composers);
void wf_mpris_set_info_lyricists(const gchar * const *lyricists);
void wf_mpris_set_info_genres(const gchar * const *genres);
void wf_mpris_set_info_dics_number(gint disc_number);
void wf_mpris_set_info_track_number(gint track_number);
void wf_mpris_set_info_beats_per_minute(gint bpm);
void wf_mpris_set_info_duration(gint64 duration);
void wf_mpris_set_info_rating(gint rating);
void wf_mpris_set_info_score(gdouble score);
void wf_mpris_set_info_play_count(gint play_count);
void wf_mpris_set_info_first_played(GDateTime *first_used);
void wf_mpris_set_info_first_played_sec(gint64 first_used);
void wf_mpris_set_info_last_played(GDateTime *last_used);
void wf_mpris_set_info_last_played_sec(gint64 last_used);
void wf_mpris_set_info_content_created(GDateTime *content_created);
void wf_mpris_set_info_content_created_sec(gint64 content_created);
void wf_mpris_set_info_art_url(const gchar *art_url);
void wf_mpris_set_info_lyrics(const gchar *lyrics);
void wf_mpris_set_info_comments(const gchar * const *comments);

/* Callback setters */
void wf_mpris_connect_root_raise(WfFuncRootRaise cb_func);
void wf_mpris_connect_root_quit(WfFuncRootRaise cb_func);
void wf_mpris_connect_player_next(WfFuncPlayerNext cb_func);
void wf_mpris_connect_player_previous(WfFuncPlayerPrevious cb_func);
void wf_mpris_connect_player_pause(WfFuncPlayerPause cb_func);
void wf_mpris_connect_player_play_pause(WfFuncPlayerPlayPause cb_func);
void wf_mpris_connect_player_stop(WfFuncPlayerStop cb_func);
void wf_mpris_connect_player_play(WfFuncPlayerPlay cb_func);
void wf_mpris_connect_player_seek(WfFuncPlayerSeek cb_func);
void wf_mpris_connect_player_set_position(WfFuncPlayerSetPosition cb_func);
void wf_mpris_connect_player_open_uri(WfFuncPlayerOpenUri cb_func);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_mpris_flush_changes(void);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_mpris_deactivate(void);

/* DESTRUCTOR PROTOTYPES END */

G_END_DECLS

#endif /* __WF_MPRIS__ */

/* END OF FILE */
