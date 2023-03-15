/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * app.h  This file is part of LibWoofer
 * Copyright (C) 2022, 2023  Quico Augustijn
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

#ifndef __WF_APP__
#define __WF_APP__

/* INCLUDES BEGIN */

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <woofer/song.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */

#define WF_TYPE_APP (wf_app_get_type())
#define WF_APP(app) (G_TYPE_CHECK_INSTANCE_CAST(app, WF_TYPE_APP, WfApp))
#define WF_APP_GET_CLASS(app) (G_TYPE_INSTANCE_GET_CLASS(app, WF_TYPE_APP, WfAppClass))
#define WF_APP_CLASS(app_class) (G_TYPE_CHECK_CLASS_CAST(app_class, WF_TYPE_APP, WfAppClass))
#define WF_IS_APP(app) (G_TYPE_CHECK_INSTANCE_TYPE(app, WF_TYPE_APP))
#define WF_IS_APP_CLASS(app_class) (G_TYPE_CHECK_CLASS_TYPE(app_class, WF_TYPE_APP))

/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfAppStatus WfAppStatus;

typedef struct _WfApp WfApp;
typedef struct _WfAppClass WfAppClass;

/* Signal callbacks */
typedef void (*WfAppMessage) (WfApp *app, const gchar *message);
typedef void (*WfAppSongsChanged) (WfApp *app, WfSong *song_previous, WfSong *song_current, WfSong *song_next);
typedef void (*WfAppStateChange) (WfApp *app, gint state, gdouble duration);
typedef void (*WfAppPositionUpdated) (WfApp *app, gdouble position, gdouble duration);

typedef void (*WfAppNotification) (WfApp *app, const gchar *title, const gchar *message);
typedef void (*WfAppPlayerNotification) (WfApp *app, WfSong *song, gint64 duration);

enum _WfAppStatus
{
	WF_APP_UNKNOWN_STATUS,
	WF_APP_INIT,
	WF_APP_READY,
	WF_APP_PLAYING,
	WF_APP_PAUSED,
	WF_APP_STOPPED
};

struct _WfApp
{
	/*< private >*/
	GApplication parent_instance;
};

struct _WfAppClass
{
	/*< private >*/
	GApplicationClass parent_instance;
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

WfApp * wf_app_new(void);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

gdouble wf_app_get_app_time(void);
gboolean wf_app_get_background_flag(void);
const gchar * wf_app_get_display_name(void);
const gchar * wf_app_get_icon_name(void);
void wf_app_set_desktop_entry(const gchar *filename);
const gchar * wf_app_get_desktop_entry(void);
gdouble wf_app_get_volume(void);
void wf_app_set_volume(gdouble volume);
gdouble wf_app_get_volume_percentage(void);
void wf_app_set_volume_percentage(gdouble percentage);
gboolean wf_app_get_incognito(void);
void wf_app_set_incognito(gboolean incognito);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

GType wf_app_get_type(void);

void wf_app_settings_updated(void);
void wf_app_redraw_next_song(void);

void wf_app_default_notification_handler(WfApp *app, const gchar *title, const gchar *message, gpointer user_data);
gboolean wf_app_default_player_notification_handler(WfApp *app, WfSong *song, gint64 duration, gpointer user_data);
gchar * wf_app_default_player_notification_message(WfSong *song, gint64 duration);

void wf_app_open(WfSong *song);
void wf_app_play_pause(void);
void wf_app_play(void);
void wf_app_pause(void);
void wf_app_stop(void);
void wf_app_previous(void);
void wf_app_next(void);

gint wf_app_run(int argc, char **argv);
void wf_app_raise(void);
void wf_app_quit(void);

void wf_app_toggle_queue(WfSong *song);
void wf_app_toggle_stop(WfSong *song);
void wf_app_set_playback_position(gint64 position);
void wf_app_set_playback_percentage(gdouble position);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

G_END_DECLS

#endif /* __WF_APP__ */

/* END OF FILE */
