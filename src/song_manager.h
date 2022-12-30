/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song_manager.h  This file is part of LibWoofer
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

#ifndef __WF_SONG_MANAGER__
#define __WF_SONG_MANAGER__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/constants.h>
#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef void (*WfFuncSongsChanged) (WfSong *song_prev, WfSong *song_current, WfSong *song_next);

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

void wf_song_manager_init(void);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

gboolean wf_song_manager_get_incognito(void);
void wf_song_manager_set_incognito(gboolean enable);

WfSong * wf_song_manager_get_queue_song(void);
WfSong * wf_song_manager_get_next_song(void);
WfSong * wf_song_manager_get_current_song(void);
WfSong * wf_song_manager_get_prev_song(void);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_song_manager_connect_event_songs_changed(WfFuncSongsChanged cb_func);

void wf_song_manager_add_queue_song(WfSong *song);
void wf_song_manager_rm_queue_song(WfSong *song);

void wf_song_manager_rm_next_song(WfSong *song);
void wf_song_manager_clear_next(void);
void wf_song_manager_refresh_next(void);

void wf_song_manager_settings_updated(void);
void wf_song_manager_songs_updated(gboolean playback_active);

void wf_song_manager_song_is_playing(WfSong *song);
void wf_song_manager_add_played_song(WfSong *song, gdouble played_fraction, gboolean skip_score_update);

WfSong * wf_song_manager_played_song_revert(void);

void wf_song_manager_sync(void);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_song_manager_finalize(void);

/* DESTRUCTOR PROTOTYPES END */

#endif /* __SONG_MANAGER__ */

/* END OF FILE */

