/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * player.h  This file is part of LibWoofer
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

#ifndef __WF_PLAYER__
#define __WF_PLAYER__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfPlayerStatus WfPlayerStatus;

typedef void (*WfFuncReportMsg) (const gchar *msg);
typedef void (*WfFuncStateChanged) (WfPlayerStatus status, gdouble duration);
typedef void (*WfFuncPositionUpdated) (gdouble duration, gdouble position);
typedef void (*WfFuncNotification) (WfSong *song, gint64 duration);

enum _WfPlayerStatus
{
	WF_PLAYER_NO_STATUS,
	WF_PLAYER_INIT,
	WF_PLAYER_READY,
	WF_PLAYER_PLAYING,
	WF_PLAYER_PAUSED,
	WF_PLAYER_STOPPED
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

void wf_player_init(void);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

void wf_player_connect_event_report_msg(WfFuncReportMsg cb_func);
void wf_player_connect_event_state_changed(WfFuncStateChanged cb_func);
void wf_player_connect_event_position_updated(WfFuncPositionUpdated cb_func);
void wf_player_connect_event_notification(WfFuncNotification cb_func);

gdouble wf_player_get_volume(void);
gdouble wf_player_get_volume_percentage(void);
void wf_player_set_volume(gdouble volume);
void wf_player_set_volume_percentage(gdouble volume);

WfPlayerStatus wf_player_get_status(void);
WfSong * wf_player_get_current_song(void);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean wf_player_is_active(void);

void wf_player_toggle_queue(WfSong *song);
void wf_player_queue_add(WfSong *song);
void wf_player_queue_rm(WfSong *song);

void wf_player_stop_after_song(WfSong *song);

gdouble wf_player_get_position(void);

void wf_player_open(WfSong *song);
void wf_player_play(void);
void wf_player_pause(void);
void wf_player_play_pause(void);
void wf_player_stop(void);
void wf_player_forward(gboolean omit_score_update);
void wf_player_backward(gboolean omit_score_update);
void wf_player_seek_position(gint64 position);
void wf_player_seek_seconds(gdouble seconds);
void wf_player_seek_percentage(gdouble percentage);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_player_finalize(void);

/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_PLAYER__ */

/* END OF FILE */
