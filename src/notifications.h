/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * notifications.h  This file is part of LibWoofer
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

#ifndef __WF_NOTIFICATION__
#define __WF_NOTIFICATION__

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

void wf_notifications_init(GApplication *application);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_notifications_send_default(const gchar *title, const gchar *body);
void wf_notifications_send_playing(const gchar *title, const gchar *body);
void wf_notifications_send_player_message(const gchar *title, const gchar *body);
void wf_notifications_send(const gchar *id, const gchar *title, const gchar *body, GNotificationPriority priority);

void wf_notifications_withdraw_default(void);
void wf_notifications_withdraw_playing(void);
void wf_notifications_withdraw(const gchar *id);

void wf_notifications_default_player_handler(WfSong *song, gint64 duration);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */

gchar * wf_notifications_get_default_player_message(WfSong *song, gint64 duration);

/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_notifications_finalize(void);

/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_NOTIFICATION__ */

/* END OF FILE */
