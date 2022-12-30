/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * notifications.c  This file is part of LibWoofer
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

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <gio/gio.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/notifications.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/utils.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module provides the application with built-in support for desktop
 * notifications using #GNotification.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

#define NOTIFICATION_ID_DEFAULT WF_NAME
#define NOTIFICATION_ID_PLAYER "player"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static GApplication *App;
static gboolean Active;

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_notifications_init(GApplication *application)
{
	g_return_if_fail(G_IS_APPLICATION(application));

	App = application;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

void
wf_notifications_send_default(const gchar *title, const gchar *body)
{
	wf_notifications_send(NOTIFICATION_ID_DEFAULT, title, body, G_NOTIFICATION_PRIORITY_NORMAL);
}

void
wf_notifications_send_playing(const gchar *title, const gchar *body)
{
	wf_notifications_send(NOTIFICATION_ID_PLAYER, title, body, G_NOTIFICATION_PRIORITY_LOW);
}

void
wf_notifications_send_player_message(const gchar *title, const gchar *body)
{
	wf_notifications_send(NOTIFICATION_ID_PLAYER, title, body, G_NOTIFICATION_PRIORITY_NORMAL);
}

void
wf_notifications_send(const gchar *id, const gchar *title, const gchar *body, GNotificationPriority priority)
{
	GNotification *g_noti;

	g_return_if_fail(id != NULL);

	if (title != NULL)
	{
		g_noti = g_notification_new(title);
	}
	else
	{
		g_noti = g_notification_new(WF_NAME);
	}

	if (body != NULL)
	{
		g_notification_set_body(g_noti, body);
	}

	// Player notifications don't need the users attention
	g_notification_set_priority(g_noti, priority);

	// Send the notification with id 'player'.  This is used to replace this notification with newer ones.
	g_application_send_notification(App, id, g_noti);

	Active = TRUE;
}

void
wf_notifications_withdraw_default(void)
{
	wf_notifications_withdraw(NOTIFICATION_ID_DEFAULT);
}

void
wf_notifications_withdraw_playing(void)
{
	wf_notifications_withdraw(NOTIFICATION_ID_PLAYER);
}

void
wf_notifications_withdraw(const gchar *id)
{
	g_return_if_fail(id != NULL);

	g_application_withdraw_notification(App, id);
}

void
wf_notifications_default_player_handler(WfSong *song, gint64 duration)
{
	gchar *info;

	if (song != NULL)
	{
		info = wf_notifications_get_default_player_message(song, duration);
		wf_notifications_send_playing("Now playing", info);
		g_free(info);
	}
	else
	{
		wf_notifications_withdraw_playing();
	}
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

gchar *
wf_notifications_get_default_player_message(WfSong *song, gint64 duration)
{
	const gchar *name = NULL;
	gchar *duration_str = NULL;
	gchar *msg = NULL;

	// Check names
	if (song != NULL)
	{
		name = wf_song_get_name(song);
	}

	if (song == NULL || name == NULL)
	{
		if (duration > 0)
		{
			// Nameless string (only show duration)
			duration_str = wf_utils_duration_to_string(duration);
			msg = g_strdup_printf("Duration: %s", duration_str);
			g_free(duration_str);
		}
		else
		{
			// No notification body
			msg = NULL;
		}

		return msg;
	}
	else
	{
		return wf_utils_get_pretty_song_msg(song, duration);
	}
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_notifications_finalize(void)
{
	if (Active)
	{
		wf_notifications_withdraw_default();
		wf_notifications_withdraw_playing();
	}

	App = NULL;
	Active = FALSE;
}

/* DESTRUCTORS END */

/* END OF FILE */
