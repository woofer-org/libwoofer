/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * app.c  This file is part of LibWoofer
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

/*
 * This module implements a #GObject type derived from #GApplication.
 * API documentation comments start with an extra '*' at the beginning of the
 * comment block.  See the GTK-Doc Manual for details.
 *
 * Notes:
 * - For all callback functions, the suffix '_cb' is used as a quick indicator
 *   for the reader.
 * - For all return value pointers, the suffix '_rv' is used to indicate the
 *   value of the pointer can be changed by the respective function.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gst/gst.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/app.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/remote.h>
#include <woofer/notifications.h>
#include <woofer/player.h>
#include <woofer/settings.h>
#include <woofer/settings_private.h>
#include <woofer/library.h>
#include <woofer/library_private.h>
#include <woofer/song_manager.h>
#include <woofer/mpris.h>

// Resource includes
#include <woofer/static/options.h>

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/**
 * SECTION:wfapp
 * @title: Main Woofer Application
 * @short_description: The main Woofer application class derived from
 * 	#GApplication
 *
 * This module provides the Woofer Application #GObject to front-ends.  Creating
 * an instance of this object and running it will initialize and start all
 * internal modules.  Front-ends only have to create such instance, set the
 * right properties, connect to signals and call the run method.  Interface
 * initialization is then done in the callbacks connected to the signals.
 **/

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Array size macro
#define SIZEOF_ARRAY(a) ((a == NULL) ? 0 : sizeof(a)/sizeof(a[0]))

// Return codes for exiting the application or for returning handle functions
#define RETURN_CONTINUE -1
#define RETURN_SUCCESS 0
#define RETURN_ERROR 1

// Parent #GType of #WfApp
#define TYPE_PARENT G_TYPE_APPLICATION

// Associated name for #WfApp
#define TYPE_NAME "WfApp"

// #GType flags for #WfApp
#define TYPE_FLAGS ((GTypeFlags) 0)

// Application flags set in #GApplication
#define APP_FLAGS (G_APPLICATION_HANDLES_OPEN | G_APPLICATION_CAN_OVERRIDE_APP_ID)

// Help overview description
#define HELP_DESCRIPTION "Any leftover arguments are treated as input files " \
                         "and automatically added to the library if of any " \
                         "audio type."

// Actions names
#define ACTION_PLAY_PAUSE "play-pause"
#define ACTION_PLAY "play"
#define ACTION_PAUSE "pause"
#define ACTION_STOP "stop"
#define ACTION_PREVIOUS "previous"
#define ACTION_NEXT "next"
#define ACTION_RAISE "raise"
#define ACTION_QUIT "quit"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef enum _WfProp WfProp;
typedef enum _WfSignal WfSignal;

// Property enum that can be converted to a #guint for easy property lookup
enum _WfProp
{
	WF_PROP_NONE,
	WF_PROP_APP_TIME,
	WF_PROP_START_BACKGROUND,
	WF_PROP_DISPLAY_NAME,
	WF_PROP_ICON_NAME,
	WF_PROP_DESKTOP_ENTRY_NAME,
	WF_PROP_VOLUME,
	WF_PROP_VOLUME_PERCENTAGE,
	WF_PROP_INCOGNITO,
	WF_PROP_COUNT
};

// Signal enum that can be converted to a #guint for easy signal lookup
enum _WfSignal
{
	WF_SIGNAL_NONE,
	WF_SIGNAL_MESSAGE,
	WF_SIGNAL_SONGS_CHANGED,
	WF_SIGNAL_STATE_CHANGE,
	WF_SIGNAL_POSITION_UPDATED,
	WF_SIGNAL_NOTIFICATION,
	WF_SIGNAL_PLAYER_NOTIFICATION,
	WF_SIGNAL_COUNT
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_app_class_init(gpointer g_class, gpointer class_data);
static void wf_app_instance_init(GTypeInstance *instance, gpointer g_class);

static void wf_event_raise_cb(void);
static void wf_event_quit_cb(void);
static void wf_event_report_msg_cb(const gchar *message);
static void wf_event_songs_updated_cb(WfSong *song_previous, WfSong *song_current, WfSong *song_next);
static void wf_event_state_changed_cb(WfPlayerStatus state, gdouble duration);
static void wf_event_position_updated_cb(gdouble position, gdouble duration);
static void wf_event_player_notification_cb(WfSong *song, gint64 duration);

static void wf_app_action_play_pause_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_play_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_pause_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_stop_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_previous_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_next_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_raise_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void wf_app_action_quit_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data);

static void wf_app_startup_cb(GApplication *app, gpointer user_data);
static void wf_app_activate_cb(GApplication *app, gpointer user_data);
static gint wf_app_handle_local_options_cb(GApplication *app, GVariantDict *options, gpointer user_data);
static void wf_app_handle_open_command_cb(GApplication *app, gpointer files, gint n_files, gchar *hint, gpointer user_data);
static void wf_app_shutdown_cb(GApplication *app, gpointer user_data);

static void wf_app_constructed(GObject *object);
static void wf_app_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void wf_app_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static gint wf_app_handle_playback_options(void);

static gboolean wf_app_is_remote(void);

static GOptionGroup * wf_app_get_option_group(void);

static void wf_app_print_version_message(void);
static void wf_app_print_all_options(void);

static WfAppStatus wf_app_get_status(WfPlayerStatus state);

static void wf_app_dispose(GObject *object);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

// A pointer to the application object; Only one #WfApp instance may exist
static WfApp *WfAppInstance;

// A pointer to the object class
static WfAppClass *WfAppClassInstance;

// A pointer to the parent object
static GApplication *GAppInstance;

// A pointer to the parent object class
static GObjectClass *WfAppParentClass;

// Array of pointers to property specifications
static GParamSpec *WfProperties[WF_PROP_COUNT];

// Array of registered signal id's
static guint WfSignals[WF_SIGNAL_COUNT];

// Registered #GType of #WfApp
static GType WfAppType = 0;

// Timestamp of application start (microseconds)
gint64 WfTime;

// %TRUE if the application has started and is activated
gboolean WfActive;

// %TRUE if the application should be in its destruction phase
gboolean WfDestruct;

// Name of the desktop entry file
gchar *WfDesktopEntry;

// Type info and callbacks for handling #WfApp instances
static const GTypeInfo WfAppTypeInfo =
{
	// Size of the class structure
	.class_size = sizeof(WfAppClass),

	// Size of the instance structure
	.instance_size = sizeof(WfApp),

	// Instance initialization function
	.class_init = wf_app_class_init,

	// Class initialization function
	.instance_init = wf_app_instance_init,
};

// Action info for cross-instance event activation
static const GActionEntry WfAppActions[] =
{
	{ ACTION_PLAY_PAUSE, wf_app_action_play_pause_cb, NULL, NULL, NULL },
	{ ACTION_PLAY, wf_app_action_play_cb, NULL, NULL, NULL },
	{ ACTION_PAUSE, wf_app_action_pause_cb, NULL, NULL, NULL },
	{ ACTION_STOP, wf_app_action_stop_cb, NULL, NULL, NULL },
	{ ACTION_PREVIOUS, wf_app_action_previous_cb, NULL, NULL, NULL },
	{ ACTION_NEXT, wf_app_action_next_cb, NULL, NULL, NULL },
	{ ACTION_RAISE, wf_app_action_raise_cb, NULL, NULL, NULL },
	{ ACTION_QUIT, wf_app_action_quit_cb, NULL, NULL, NULL },
	{ NULL } /* terminator */
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

/**
 * wf_app_new:
 *
 * Creates a new #WfApp object.
 *
 * Returns: (transfer full): a new #WfApp instance.  Unref it with
 * g_object_unref() at the end of the application life cycle.
 *
 * Since: 0.2
 **/
WfApp *
wf_app_new(void)
{
	return (WfApp *) g_object_new(WF_TYPE_APP, NULL /* terminator */);
}

// This gets invoked when the class should initialize
static void
wf_app_class_init(gpointer g_class, gpointer class_data)
{
	WfAppClass *wf_class = g_class;
	GObjectClass *object_class = G_OBJECT_CLASS(wf_class);
	GParamSpec *prop;
	guint signal;
	WfProp prop_id;
	WfSignal signal_id;

	g_return_if_fail(WF_IS_APP_CLASS(wf_class));
	g_return_if_fail(G_IS_OBJECT_CLASS(object_class));

	// Override object class method pointers
	object_class->constructed = wf_app_constructed;
	object_class->get_property = wf_app_get_property;
	object_class->set_property = wf_app_set_property;
	object_class->dispose = wf_app_dispose;

	// Provide a callback function to internal application events
	wf_player_connect_event_report_msg(wf_event_report_msg_cb);
	wf_player_connect_event_position_updated(wf_event_position_updated_cb);
	wf_player_connect_event_state_changed(wf_event_state_changed_cb);
	wf_player_connect_event_notification(wf_event_player_notification_cb);
	wf_song_manager_connect_event_songs_changed(wf_event_songs_updated_cb);
	wf_mpris_connect_root_raise(wf_event_raise_cb);
	wf_mpris_connect_root_quit(wf_event_quit_cb);

	// Create property specifications and install them

	/**
	 * WfApp:app-time:
	 *
	 * The time in seconds since the application started.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_APP_TIME;
	prop = g_param_spec_double("app-time",
	                           "Application time",
	                           "Time in seconds since application start",
	                           0.0,
	                           G_MAXDOUBLE,
	                           0.0,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:start-background:
	 *
	 * %TRUE if the application front-end should start in the background.
	 *
	 * If it should start in background, the application starts normally and
	 * the interface is supposed to construct, realize and then be hidden so
	 * it can be instantly shown to the user when requested.  This can be
	 * useful if the application is autostarted by the window manager so it
	 * can idle around and always be ready to play whenever desired.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_START_BACKGROUND;
	prop = g_param_spec_boolean("start-background",
	                            "Should start in background",
	                            "If the application should start in the background (no visible window)",
	                            FALSE,
	                            G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:display-name:
	 *
	 * This holds the name to be displayed in the main application window.
	 * The application processes the hidden `--name` option on startup.  If
	 * it is not present, the default name is used, so this property is
	 * guaranteed to be non-%NULL and can be used by the front-end to set
	 * the window title.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_DISPLAY_NAME;
	prop = g_param_spec_string("display-name",
	                           "Display name",
	                           "The display name that should be used to report to the window manager",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:icon-name:
	 *
	 * This holds the themed icon name to be used for the application
	 * window.  The application processes the hidden `--icon` option on
	 * startup.  If it is not present, the default icon name is used, so
	 * this property is guaranteed to be non-%NULL.  However, the front-end
	 * interface should still provide a fallback icon image that can be used
	 * if the icon name is not present in any active icon theme.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_ICON_NAME;
	prop = g_param_spec_string("icon-name",
	                           "Icon name",
	                           "The themed icon name used in the window manager",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:desktop-entry-name:
	 *
	 * The front-end name corresponding with the desktop entry file.  This
	 * can be used by the desktop environment to link a launcher identified
	 * by the desktop entry file with the media player that is provided via
	 * D-Bus using the Media Player Remote Interfacing Specification
	 * (MPRIS).  Command-line interfaces or front-ends that do not ship a
	 * desktop entry file can leave this empty.
	 *
	 * Set this property during construction, initialization or startup and
	 * do not include the '.desktop' extension, only the basename of the
	 * entry file.
	 **/
	prop_id = WF_PROP_DESKTOP_ENTRY_NAME;
	prop = g_param_spec_string("desktop-entry-name",
	                           "Desktop Entry name",
	                           "The basename of the desktop entry file",
	                           NULL,
	                           G_PARAM_READWRITE);

	/**
	 * WfApp:volume:
	 *
	 * The volume currently in use by the application.  This is available
	 * during standby (the value is stored in memory by the application, but
	 * is not used in any playback pipeline) as well as when in use (known
	 * by the application and made sure that it is used in the running
	 * playback pipeline).
	 *
	 * If your interface is built using GObject, you can create a #GBinding
	 * with g_object_bind_property() between this property and the value
	 * property of for example a volume slider.  The object system will sync
	 * the values of the properties automatically so less code is necessary
	 * on the front-end side.  You can create the binding in both ways by
	 * using the %G_BINDING_BIDIRECTIONAL flag which will update the source
	 * property as well as the target depending on which value changed.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_VOLUME;
	prop = g_param_spec_double("volume",
	                           "Volume",
	                           "Volume in use for the playback",
	                           0.0,
	                           1.0,
	                           1.0,
	                           G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:volume-percentage:
	 *
	 * The volume currently in use by the application, represented as a
	 * percentage (0%-100%).  This can be useful if you want to provide a
	 * widget for volume control using percentages, but without converting
	 * to a 0.0 to 1.0 range, as expected by the #WfApp:volume property (see
	 * this property for more information about volume control).
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_VOLUME_PERCENTAGE;
	prop = g_param_spec_double("volume-percentage",
	                           "Volume (Percentage)",
	                           "Volume in use for the playback, but represented as a percentage (0-100)",
	                           0.0,
	                           100.0,
	                           100.0,
	                           G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfApp:incognito:
	 *
	 * Whether incognito mode is currently active.  Incognito mode prevents
	 * statistics from being updated when the playback changes songs.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_INCOGNITO;
	prop = g_param_spec_boolean("incognito",
	                            "Incognito mode",
	                            "Statistics are not updated when incognito mode is active",
	                            FALSE,
	                            G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	// Create/register signals

	/**
	 * WfApp::message:
	 * @app: the #WfApp
	 * @message: the message from the internal application
	 *
	 * The ::message signal is emitted when any new informational message is
	 * present.  This message should normally be posted on the status bar
	 * usually present in the bottom-left corner of the window.  If the
	 * front-end is not graphical, this signal can be left unconnected or
	 * the messages can be ignored.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_MESSAGE;
	signal = g_signal_new("message",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_NONE, // Return type
	                      1, // Number of parameters
	                      G_TYPE_STRING, // Type of the first parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;

	/**
	 * WfApp::songs-changed:
	 * @app: the #WfApp
	 * @song_previous: the song previously played
	 * @song_current: the song that is currently playing
	 * @song_next: the song that is up next
	 *
	 * The ::songs-changed signal is emitted when the playback songs change.
	 * These include @song_previous, @song_current and @song_next and can be
	 * set in the interface to inform the user about previous, current and
	 * next songs.
	 *
	 * Status changes of any songs can also be updated in the interface
	 * when this signal is emitted.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_SONGS_CHANGED;
	signal = g_signal_new("songs-changed",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_NONE, // Return type
	                      3, // Number of parameters
	                      G_TYPE_POINTER, // Type of the first parameter
	                      G_TYPE_POINTER, // Type of the second parameter
	                      G_TYPE_POINTER, // Type of the third parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;

	/**
	 * WfApp::state-change:
	 * @app: the #WfApp
	 * @state: the new #WfAppStatus state
	 * @duration: known duration of the current playback stream (in seconds)
	 *
	 * The ::state-change signal is emitted when the playback changes state.
	 * This can be used in the interface to inform the user about the
	 * current status of the application and update any icons related to the
	 * status.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_STATE_CHANGE;
	signal = g_signal_new("state-change",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_NONE, // Return type
	                      2, // Number of parameters
	                      G_TYPE_INT, // Type of the first parameter
	                      G_TYPE_DOUBLE, // Type of the second parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;

	/**
	 * WfApp::position-updated:
	 * @app: the #WfApp
	 * @position: the current position of the playback (in seconds)
	 * @duration: the current known duration of the playback (in seconds)
	 *
	 * The ::position-updated signal is emitted when the playback changes
	 * state.  This can be used in the interface to inform the user about
	 * the current status of the application and update any icons related
	 * to the status.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_POSITION_UPDATED;
	signal = g_signal_new("position-updated",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_NONE, // Return type
	                      2, // Number of parameters
	                      G_TYPE_DOUBLE, // Type of the first parameter
	                      G_TYPE_DOUBLE, // Type of the second parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;

	/**
	 * WfApp::notification:
	 * @app: the #WfApp
	 * @title: the title
	 * @message: the message body
	 *
	 * The ::notification signal is emitted when a notification should be
	 * send to user (if supported), usually warnings or errors that have
	 * occurred.  These messages are always printed to the console, but they
	 * should also be shown in the interface if the interface is graphical
	 * (see the ::message signal).
	 *
	 * The default handler wf_app_default_notification_handler() can be
	 * connected to this signal to let the back-end handle notification
	 * with the use of #GNotification.  If you want to handle notifications
	 * yourself, create your own handler and connect it to this signal.
	 *
	 * Returns: %FALSE to propagate the event to the default handler to
	 * send a notification, %TRUE to stop processing the event.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_NOTIFICATION;
	signal = g_signal_new("notification",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_BOOLEAN, // Return type
	                      2, // Number of parameters
	                      G_TYPE_STRING, // Type of the first parameter
	                      G_TYPE_STRING, // Type of the second parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;

	/**
	 * WfApp::player-notification:
	 * @app: the #WfApp
	 * @song: the current song
	 * @duration: the current known duration of the playback (in seconds)
	 *
	 * The ::player-notification signal is emitted when a notification about
	 * the playback should be send to user (if supported).  The default
	 * handler wf_app_default_player_notification_handler() can be connected
	 * to this signal to let the back-end handle notification with the use
	 * of #GNotification.  If you want to handle notifications yourself,
	 * create your own handler and connect it to this signal.
	 *
	 * Returns: %FALSE to propagate the event to the default handler to
	 * send a notification, %TRUE to stop processing the event.
	 *
	 * Since: 0.1
	 **/
	signal_id = WF_SIGNAL_PLAYER_NOTIFICATION;
	signal = g_signal_new("player-notification",
	                      WfAppType,
	                      G_SIGNAL_RUN_LAST,
	                      0, // Class offset
	                      NULL, // Accumulator
	                      NULL, // Accumulator data
	                      NULL, // C marshaller
	                      G_TYPE_BOOLEAN, // Return type
	                      2, // Number of parameters
	                      G_TYPE_POINTER, // Type of the first parameter
	                      G_TYPE_INT64, // Type of the second parameter
	                      NULL); // Terminator
	WfSignals[signal_id] = signal;
}

// This gets invoked when the object's instance should initialize
static void
wf_app_instance_init(GTypeInstance *instance, gpointer g_class)
{
	WfApp *app = WF_APP(instance);
	GApplication *gapp = G_APPLICATION(instance);

	g_return_if_fail(WF_IS_APP(app));
	g_return_if_fail(G_IS_APPLICATION(gapp));

	// Set initial time
	WfTime = g_get_monotonic_time();

	// Set global program name
	g_set_application_name(WF_NAME);

	// Connect application signals
	g_signal_connect(gapp, "startup",
	                 G_CALLBACK(wf_app_startup_cb), NULL /* user_data */);
	g_signal_connect(gapp, "handle-local-options",
	                 G_CALLBACK(wf_app_handle_local_options_cb), NULL /* user_data */);
	g_signal_connect(gapp, "open",
	                 G_CALLBACK(wf_app_handle_open_command_cb), NULL /* user_data */);
	g_signal_connect_after(gapp, "activate",
	                       G_CALLBACK(wf_app_activate_cb), NULL /* user_data */);
	g_signal_connect_after(gapp, "shutdown",
	                       G_CALLBACK(wf_app_shutdown_cb), NULL /* user_data */);

	// Add cross instance actions
	g_action_map_add_action_entries(G_ACTION_MAP(gapp), WfAppActions, -1 /* length */, NULL /* user_data */);

	// Set option context string
	g_application_set_option_context_parameter_string(gapp, "[AUDIO FILES\342\200\246]");
	g_application_set_option_context_description(gapp, HELP_DESCRIPTION);

	// Add option context groups
	g_application_add_option_group(gapp, wf_app_get_option_group());
	g_application_add_option_group(gapp, gst_init_get_option_group());

	// Add main option context
	g_application_add_main_option_entries(gapp, wf_option_entries_get_main());

	// Initialize remote D-Bus interface
	wf_remote_init(NULL /* GDBusConnection */);
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

/**
 * wf_app_get_app_time:
 *
 * Gets the application time.  See the #WfApp:app-time property.
 *
 * Returns: the application running time in seconds
 *
 * Since: 0.1
 **/
gdouble
wf_app_get_app_time(void)
{
	/*
	 * Get monotonic time because it represent a somewhat 'machine time',
	 * this is usually suspended when the cpu is in sleep.  The thing here
	 * is to record process time, so wall clock accuracy is not the main
	 * point.
	 */

	gint64 startup = WfTime;
	gint64 now = g_get_monotonic_time();
	gint time_since = (now - startup); // Microseconds

	return ((gdouble) time_since) / 1000 / 1000; // Convert to seconds as a double
}

/**
 * wf_app_get_background_flag:
 *
 * Gets the value of the "should start in background" flag.  See property
 * #WfApp:start-background for details.
 *
 * Returns: %TRUE if the application should start in the background
 *
 * Since: 0.1
 **/
gboolean
wf_app_get_background_flag(void)
{
	const WfApplicationEntries *entries = wf_option_entries_get_entries();

	g_return_val_if_fail(entries != NULL, FALSE);

	return entries->background;
}

/**
 * wf_app_get_display_name:
 *
 * Gets the display name to use.  See the #WfApp:display-name property.
 *
 * Returns: (transfer none): a string containing the display name
 *
 * Since: 0.1
 **/
const gchar *
wf_app_get_display_name(void)
{
	const WfApplicationEntries *entries = wf_option_entries_get_entries();

	g_return_val_if_fail(entries != NULL, FALSE);

	return (entries->name != NULL) ? entries->name : WF_DISPLAY_NAME;
}

/**
 * wf_app_get_icon_name:
 *
 * Gets the icon name to use.  See the #WfApp:icon-name property.
 *
 * Returns: (transfer none): a string containing the icon name
 *
 * Since: 0.1
 **/
const gchar *
wf_app_get_icon_name(void)
{
	const WfApplicationEntries *entries = wf_option_entries_get_entries();

	g_return_val_if_fail(entries != NULL, FALSE);

	return (entries->icon != NULL) ? entries->icon : WF_ICON_NAME;
}

/**
 * wf_app_set_desktop_entry:
 * @name: (transfer none): the new desktop entry filename to set
 *
 * Sets the desktop entry filename to use.  See the #WfApp:desktop-entry-name
 * property.
 *
 * Since: 0.1
 **/
void
wf_app_set_desktop_entry(const gchar *filename)
{
	g_free(WfDesktopEntry);

	WfDesktopEntry = g_strdup(filename);
}

/**
 * wf_app_get_desktop_entry:
 *
 * Gets the desktop entry name to use.  See the #WfApp:desktop-entry-name
 * property.
 *
 * Returns: (transfer none): a string containing the desktop entry filename
 *
 * Since: 0.1
 **/
const gchar *
wf_app_get_desktop_entry(void)
{
	return WfDesktopEntry;
}

/**
 * wf_app_get_volume:
 *
 * Gets the volume in use.  See the #WfApp:volume property.
 *
 * Returns: the current volume
 *
 * Since: 0.1
 **/
gdouble
wf_app_get_volume(void)
{
	return wf_player_get_volume();
}

/**
 * wf_app_set_volume:
 * @volume: the new value for the volume to use
 *
 * Sets the volume to use and apply.  See the #WfApp:volume property.
 *
 * Since: 0.1
 **/
void
wf_app_set_volume(gdouble volume)
{
	wf_player_set_volume(volume);
}

/**
 * wf_app_get_volume_percentage:
 *
 * Gets the volume to use, given as a percentage.  See the
 * #WfApp:volume-percentage property.
 *
 * Returns: the current volume
 *
 * Since: 0.1
 **/
gdouble
wf_app_get_volume_percentage(void)
{
	return wf_player_get_volume_percentage();
}

/**
 * wf_app_set_volume_percentage:
 * @percentage: the new percentage for the volume to use
 *
 * Sets the volume to use, provided as a percentage.  See the
 * #WfApp:volume-percentage property.
 *
 * Since: 0.1
 **/
void
wf_app_set_volume_percentage(gdouble percentage)
{
	wf_player_set_volume_percentage(percentage);
}

/**
 * wf_app_get_incognito:
 *
 * Returns whether incognito mode is active.  See the #WfApp:incognito property.
 *
 * Returns: %TRUE if incognito is active
 *
 * Since: 0.1
 **/
gboolean
wf_app_get_incognito(void)
{
	return wf_song_manager_get_incognito();
}

/**
 * wf_app_set_incognito:
 * @incognito: the value to turn incognito on or off
 *
 * Sets incognito mode on or off.  See the #WfApp:incognito property.
 *
 * Since: 0.1
 **/
void
wf_app_set_incognito(gboolean incognito)
{
	wf_song_manager_set_incognito(incognito);
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static void
wf_event_raise_cb(void)
{
	wf_app_raise();
}

static void
wf_event_quit_cb(void)
{
	wf_app_quit();
}

static void
wf_event_report_msg_cb(const gchar *message)
{
	guint signal = WfSignals[WF_SIGNAL_MESSAGE];

	g_signal_emit(WfAppInstance, signal, 0 /* detail */, message, NULL /* return value */);
}

static void
wf_event_songs_updated_cb(WfSong *song_previous, WfSong *song_current, WfSong *song_next)
{
	guint signal = WfSignals[WF_SIGNAL_SONGS_CHANGED];

	g_signal_emit(WfAppInstance, signal, 0 /* detail */, song_previous, song_current, song_next, NULL /* return value */);
}

static void
wf_event_state_changed_cb(WfPlayerStatus state, gdouble duration)
{
	guint signal = WfSignals[WF_SIGNAL_STATE_CHANGE];

	WfAppStatus status = wf_app_get_status(state);

	g_signal_emit(WfAppInstance, signal, 0 /* detail */, status, duration, NULL /* return value */);
}

static void
wf_event_position_updated_cb(gdouble position, gdouble duration)
{
	guint signal = WfSignals[WF_SIGNAL_POSITION_UPDATED];

	g_signal_emit(WfAppInstance, signal, 0 /* detail */, position, duration, NULL /* return value */);
}

static void
wf_event_player_notification_cb(WfSong *song, gint64 duration)
{
	guint signal = WfSignals[WF_SIGNAL_PLAYER_NOTIFICATION];
	gboolean stop_rv;

	g_signal_emit(WfAppInstance, signal, 0 /* detail */, song, duration, &stop_rv);

	if (!stop_rv)
	{
		wf_app_default_player_notification_handler(WfAppInstance, song, duration, NULL /* user_data */);
	}
}

static void
wf_app_action_play_pause_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action play-pause activated");

	wf_player_play_pause();
}

static void
wf_app_action_play_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action play activated");

	wf_player_play();
}

static void
wf_app_action_pause_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action pause activated");

	wf_player_pause();
}

static void
wf_app_action_stop_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action stop activated");

	wf_player_stop();
}

static void
wf_app_action_previous_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action previous activated");

	wf_player_backward(FALSE);
}

static void
wf_app_action_next_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action next activated");

	wf_player_forward(FALSE);
}

static void
wf_app_action_raise_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action raise activated");

	wf_app_raise();
}

static void
wf_app_action_quit_cb(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_info("Action quit activated");

	wf_app_quit();
}

// General application startup and internal module initialization
static void
wf_app_startup_cb(GApplication *app, gpointer user_data)
{
	g_info("Application startup (application time %fsec)", wf_app_get_app_time());

	gst_init(NULL /* argc */, NULL /* argv */);

	wf_mpris_set_root_desktop_entry(wf_app_get_desktop_entry());
	wf_mpris_set_root_identity(WF_DISPLAY_NAME);
	wf_mpris_set_root_can_raise(TRUE);
	wf_mpris_set_root_can_quit(TRUE);
	wf_mpris_set_root_can_set_fullscreen(TRUE);

	wf_notifications_init(GAppInstance);

	wf_settings_init();
	wf_settings_read_file();

	wf_library_init();
	wf_library_read();

	wf_player_init();

	g_info("Application startup completed (application time %fsec).", wf_app_get_app_time());
}

// Application activation
static void
wf_app_activate_cb(GApplication *app, gpointer user_data)
{
	g_info("Application activated (application time %fsec)", wf_app_get_app_time());

	// Only execute this block once (just after startup)
	if (!WfActive)
	{
		WfActive = TRUE;

		/*
		 * Hold a use reference to the application; this will make sure
		 * the application stays alive as long as the reference exists.
		 * In case the application interface doesn't put any hold on the
		 * application during activation, it still stays alive.
		 */
		g_application_hold(app);

		// Now that this is the primary instance, re-check the playback options
		wf_app_handle_playback_options();
	}
}

// Process command-line parameters after GApplication construction
static gint
wf_app_handle_local_options_cb(GApplication *app, GVariantDict *options, gpointer user_data)
{
	const WfApplicationEntries *entries = wf_option_entries_get_entries();
	gint result = RETURN_CONTINUE;

	g_debug("Parsing command-line options...");

	g_return_val_if_fail(G_IS_APPLICATION(app), RETURN_ERROR);
	g_return_val_if_fail(entries != NULL, RETURN_ERROR);

	if (entries->shortlist)
	{
		wf_app_print_all_options();

		return RETURN_SUCCESS;
	}

	if (entries->version)
	{
		wf_app_print_version_message();

		return RETURN_SUCCESS;
	}

	if (entries->config != NULL)
	{
		g_info("Found command-line option config (specified path <%s>) ", entries->config);

		wf_settings_set_file(entries->config);

		g_free(entries->config);
	}

	if (entries->library != NULL)
	{
		g_info("Found command-line option library (specified path <%s>) ", entries->library);

		wf_library_set_file(entries->library);

		g_free(entries->library);
	}

	if (entries->background)
	{
		/*
		 * Print a simple message indicating that the option has been
		 * found.  The interface should check the value and decide to
		 * start in background or not.
		 */
		g_info("Found command-line option background");
	}

	/*
	 * Process the playback options if this instance is remote (if not,
	 * processing these options should be done later)
	 */
	if (wf_app_is_remote())
	{
		return wf_app_handle_playback_options();
	}
	else
	{
		return result;
	}
}

// Open file received from command-line parameters
static void
wf_app_handle_open_command_cb(GApplication *app, gpointer files, gint n_files, gchar *hint, gpointer user_data)
{
	GFile **file_list = files;
	GFile *file;
	gint i;
	gint amount;
	gint added = 0;

	g_return_if_fail(app != NULL);
	g_return_if_fail(files != NULL);
	g_return_if_fail(n_files > 0);

	if (hint == NULL || hint[0] == '\0')
	{
		hint = "<empty>";
	}

	g_info("Opening files from command-line parameters with hint: %s", hint);

	for (i = 0; i < n_files; i++)
	{
		file = file_list[i];

		if (!G_IS_FILE(file))
		{
			g_warning("File is not a GFile");
			break;
		}

		/*
		 * Because all #GFiles will be unreffed by GLib after opening,
		 * make sure to add reference that the library may remove if it
		 * decides it doesn't want the file.  This to prevent GLib from
		 * unreffing all files and creating a segmentation fault as the
		 * objects were already freed.
		 */
		g_object_ref(file);

		amount = wf_library_add_by_file(file, NULL /* func */, 0 /* checks: default */, FALSE);
		added += amount;
	}

	wf_library_write(FALSE);

	// Files have been opened, now show the interface window
	g_application_activate(GAppInstance);
}

// Time to shutdown and finalize all internal modules
static void
wf_app_shutdown_cb(GApplication *app, gpointer user_data)
{
	g_info("Shutting down...");

	wf_player_finalize();
	wf_library_finalize();
	wf_settings_finalize();
	wf_notifications_finalize();
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

/**
 * wf_app_get_type:
 *
 * Get the matching #GType for #WfApp.  On its first run it will register the
 * type with GLib's type system.
 *
 * Returns: the #GType for #WfApp
 *
 * Since: 0.1
 **/
GType
wf_app_get_type(void)
{
	const gchar *name;
	GType type;

	// Initialize the new #WfApp type if not registered
	if (g_once_init_enter(&WfAppType))
	{
		// This will remember the string pointer for easy comparison
		name = g_intern_static_string(TYPE_NAME);

		// Register the type (should only run once per application instance)
		type = g_type_register_static(TYPE_PARENT, name, &WfAppTypeInfo, TYPE_FLAGS);

		// Remember registered type
		g_once_init_leave(&WfAppType, type);
	}

	return WfAppType;
}

/**
 * wf_app_settings_updated:
 *
 * Notify internal application components that some settings may have updated.
 * Any following operations may also emit any signals, so there is no need to
 * update anything in the interface.
 *
 * Since: 0.1
 **/
void
wf_app_settings_updated(void)
{
	wf_song_manager_settings_updated();
}

/**
 * wf_app_redraw_next_song:
 *
 * Clear the current next song and get a new one to play.
 *
 * Since: 0.1
 **/
void
wf_app_redraw_next_song(void)
{
	wf_song_manager_refresh_next();
}

/**
 * wf_app_default_notification_handler:
 * @title: (transfer none) (optional): the title, as provided by the signal
 * @message: (transfer none) (optional): the message body, as provided by the
 * 	signal
 *
 * The default notification handler.  This handler can be provided as the
 * callback for the #WfApp::notification signal, if you want to let the
 * application back-end handle notifications.
 *
 * Note that this function is not connected to any signals by default.  If you
 * want to give your users notifications, you will have to connect this handler
 * to the #WfApp::notification signal.
 *
 * Since: 0.1
 **/
void
wf_app_default_notification_handler(WfApp *app, const gchar *title, const gchar *message, gpointer user_data)
{
	wf_notifications_send_default(title, message);
}

/**
 * wf_app_default_player_notification_handler:
 * @song: the current song
 * @duration: the current known duration of the playback (in seconds)
 *
 * The default notification handler for player messages.  This handler can be
 * provided as the callback for the #WfApp::player-notification signal, if you
 * want to let the application back-end handle player notifications.
 *
 * Note that this function is not connected to any signals by default.  If you
 * want to give your users notifications, you will have to connect this handler
 * to the #WfApp::player-notification signal.
 *
 * Returns: %TRUE
 *
 * Since: 0.1
 **/
gboolean
wf_app_default_player_notification_handler(WfApp *app, WfSong *song, gint64 duration, gpointer user_data)
{
	wf_notifications_default_player_handler(song, duration);

	return TRUE;
}

/**
 * wf_app_get_default_player_notification_message:
 * @song: the current song
 * @duration: the current known duration of the playback (in seconds)
 *
 * Gets the default message used by wf_app_player_notification_handler().  This
 * can be used to let the back-end create the message used for notifications by
 * default, but handle it with your own handler (and perhaps another
 * notification provider).
 *
 * Since: 0.1
 **/
gchar *
wf_app_default_player_notification_message(WfSong *song, gint64 duration)
{
	return wf_notifications_get_default_player_message(song, duration);
}

/**
 * wf_app_open:
 * @song: the song to open
 *
 * Open the provided song in the player.  When called, it will immediately
 * finish any song that is playing and set the new song active in the pipeline.
 * This results in the playback pipeline going to the playing state (if no
 * errors occur).
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_open(WfSong *song)
{
	wf_player_open(song);
}

/**
 * wf_app_play_pause:
 *
 * Change the playback state depending on the current state.  The playback will
 * be paused if it is playing; it will be set to playing if it is paused and
 * playback will start if it is currently not active (idle).
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_play_pause(void)
{
	wf_player_play_pause();
}

/**
 * wf_app_play:
 *
 * Change the playback state to playing.  If the playback is paused, it will
 * simply set it to playing.  If the playback is not active (idle), it will
 * start the playback.  If it is already playing, calling this does not change
 * the state.
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_play(void)
{
	wf_player_play();
}

/**
 * wf_app_pause:
 *
 * Change the playback state to paused if it is playing.  It will report a
 * message if nothing is playing.
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_pause(void)
{
	wf_player_pause();
}

/**
 * wf_app_stop:
 *
 * Immediately stop the playback, finish the current song and change the
 * playback state to ready.
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_stop(void)
{
	wf_player_stop();
}

/**
 * wf_app_previous:
 *
 * Change the playing song back to the one just previously played.
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_previous(void)
{
	wf_player_backward(FALSE);
}

/**
 * wf_app_next:
 *
 * Skip to the next song.  This will change the playing song to the next song in
 * the user queue (if any) or one chosen by the software.
 *
 * No further action is required on the front-end side, as any messages or state
 * changes will be emitted via the respective signals.
 *
 * Since: 0.1
 **/
void
wf_app_next(void)
{
	wf_player_forward(FALSE);
}

/**
 * wf_app_run:
 * @argc: the amount of arguments, as provided by main()
 * @argv: the array of arguments, as provided by main()
 *
 * Runs the application.  This means all internal modules will be initialized
 * and the main loop gets called.
 *
 * See g_application_run() for more information.
 *
 * Since: 0.1
 **/
gint
wf_app_run(int argc, char **argv)
{
	return g_application_run(GAppInstance, argc, argv);
}

/**
 * wf_app_raise:
 *
 * Causes the main graphical window to be shown on the desktop (if any) and draw
 * attention from the user.  This is intended to only be used when the user
 * explicitly requests it.
 *
 * Since: 0.1
 **/
void
wf_app_raise(void)
{
	g_application_activate(GAppInstance);
}

/**
 * wf_app_quit:
 *
 * Quit the application as soon as possible.  The actual application cleanup
 * and shutdown is done in the next main loop iteration (see
 * g_application_quit() for details) and that's when everything that needs to be
 * destructed or cleaned up can be done.
 *
 * Since: 0.1
 **/
void
wf_app_quit(void)
{
	/*
	 * This will quit the application immediately, regardless of any hold
	 * references.  This can safely be used, as the hold reference set
	 * during activation is just so the application doesn't quit when no
	 * interface want to activate (and construct).  The remote D-Bus
	 * interface should always be available anyway.  Just in case the hold
	 * does have any impact, release it here.
	 */
	if (!WfDestruct)
	{
		WfDestruct = TRUE;

		g_application_release(GAppInstance);
		g_application_quit(GAppInstance);
	}
}

/**
 * wf_app_toggle_queue:
 * @song: the song to toggle in the queue
 *
 * Toggle @song in the queue.  If it is already queued (present in the queue),
 * dequeue it (remove it from the queue).  If it is not queued, add it to the
 * queue.
 *
 * The song that was first added to the queue is immediately played when the
 * playback continue to the next song.  The queue always takes priority over any
 * song chosen by the software itself.
 *
 * This can be called multiple times in a row, for example when queuing a user
 * selection of songs.
 *
 * Because this can change a songs status, the interface can be updated when the
 * #WfApp::songs-changed signal gets emitted.
 *
 * Since: 0.1
 **/
void
wf_app_toggle_queue(WfSong *song)
{
	wf_player_toggle_queue(song);
}

/**
 * wf_app_toggle_stop:
 * @song: (nullable): a song to toggle its stop flag
 *
 * Toggle the stop flag of @song.  When the flag is set, it will stop playing
 * after this song.  When no song is provided, the currently playing song is
 * used.
 *
 * This can be called multiple times in a row, for example when toggling the
 * flag of a user selection of songs.
 *
 * Because this can change a songs status, the interface can be updated when the
 * #WfApp::songs-changed signal gets emitted.
 *
 * Since: 0.1
 **/
void
wf_app_toggle_stop(WfSong *song)
{
	wf_player_stop_after_song(song);
}

/**
 * wf_app_set_playback_position:
 * @position: the position (in microseconds) to jump to
 *
 * Jumps (or seeks) to @position in the playback stream.  This can be used, for
 * example, when the user drags a position slider in the interface, although no
 * throttling is done by the application so keep in mind that this could
 * temporarily degrade performance when a lot of seeks are done in a short
 * period of time.
 *
 * The new position will be reported by via signals, so no further action is
 * required on the front-end side.
 *
 * Since: 0.1
 **/
void
wf_app_set_playback_position(gint64 position)
{
	wf_player_seek_position(position);
}

/**
 * wf_app_set_playback_percentage:
 * @position: the position (given as a percentage) to jump to
 *
 * Jumps (or seeks) to @position in the playback stream, but by providing a
 * percentage of the total duration.  See wf_app_set_playback_position() for
 * details.
 *
 * Since: 0.1
 **/
void
wf_app_set_playback_percentage(gdouble position)
{
	wf_player_seek_percentage(position);
}

// This gets invoked when the object is initialized/constructed
static void
wf_app_constructed(GObject *object)
{
	WfApp *app = WF_APP(object);
	WfAppClass *app_class = WF_APP_GET_CLASS(app);
	GApplication *gapp = G_APPLICATION(app);
	GObjectClass *obj_class = g_type_class_peek_parent(app_class);

	// Check that all types are valid
	g_return_if_fail(WF_IS_APP(app));
	g_return_if_fail(WF_IS_APP_CLASS(app_class));
	g_return_if_fail(G_IS_APPLICATION(gapp));
	g_return_if_fail(G_IS_OBJECT_CLASS(obj_class));

	// Set #GApplication properties
	g_application_set_application_id(gapp, WF_ID);
	g_application_set_flags(gapp, APP_FLAGS);

	// Set global variables
	WfAppInstance = app;
	GAppInstance = gapp;

	// Set the pointer to the classes
	WfAppClassInstance = app_class;
	WfAppParentClass = obj_class;

	// Chain up parent class methods
	obj_class->constructed(object);
}

// This gets invoked when a caller wants to get a property (the value needs to be set in @value)
static void
wf_app_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	WfApp *app = WF_APP(object);
	WfProp property = property_id;

	g_return_if_fail(WF_IS_APP(app));

	switch (property)
	{
		case WF_PROP_APP_TIME:
			g_value_set_double(value, wf_app_get_app_time());
			break;
		case WF_PROP_START_BACKGROUND:
			g_value_set_boolean(value, wf_app_get_background_flag());
			break;
		case WF_PROP_DISPLAY_NAME:
			g_value_set_string(value, wf_app_get_display_name());
			break;
		case WF_PROP_ICON_NAME:
			g_value_set_string(value, wf_app_get_icon_name());
			break;
		case WF_PROP_DESKTOP_ENTRY_NAME:
			g_value_set_string(value, wf_app_get_desktop_entry());
			break;
		case WF_PROP_VOLUME:
			g_value_set_double(value, wf_app_get_volume());
			break;
		case WF_PROP_VOLUME_PERCENTAGE:
			g_value_set_double(value, wf_app_get_volume_percentage());
			break;
		case WF_PROP_INCOGNITO:
			g_value_set_boolean(value, wf_app_get_incognito());
			break;
		default:
			// Unknown property
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

// This gets invoked when a caller wants to set a property (the value in @value needs to be set by the caller)
static void
wf_app_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	WfApp *app = WF_APP(object);
	WfProp property = property_id;

	g_return_if_fail(WF_IS_APP(app));

	switch (property)
	{
		case WF_PROP_DESKTOP_ENTRY_NAME:
			wf_app_set_desktop_entry(g_value_get_string(value));
			break;
		case WF_PROP_VOLUME:
			wf_app_set_volume(g_value_get_double(value));
			break;
		case WF_PROP_VOLUME_PERCENTAGE:
			wf_app_set_volume_percentage(g_value_get_double(value));
			break;
		case WF_PROP_INCOGNITO:
			wf_app_set_incognito(g_value_get_boolean(value));
			break;
		case WF_PROP_START_BACKGROUND:
		case WF_PROP_DISPLAY_NAME:
		case WF_PROP_ICON_NAME:
		default:
			// Unknown property
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

// Handle playback options that may be provided to alter the playback
static gint
wf_app_handle_playback_options(void)
{
	/*
	 * Following here are the options to manipulate the playback.  If this
	 * is the primary instance, activating an action will simply execute the
	 * corresponding action callback.  If this is a secondary instance,
	 * activation of an action (after registration) will result in the
	 * corresponding action being activated in the primary instance.
	 */

	const gchar *options = "--play-pause, --play, --pause, --stop, --previous or --next";
	const WfApplicationEntries *entries = wf_option_entries_get_entries();

	gboolean play_pause;
	gboolean play;
	gboolean pause;
	gboolean stop;
	gboolean previous;
	gboolean next;

	g_return_val_if_fail(entries != NULL, RETURN_ERROR);

	play_pause = entries->play_pause;
	play = entries->play;
	pause = entries->pause;
	stop = entries->stop;
	previous = entries->previous;
	next = entries->next;

	// Check if any option is supplied
	if (!(play_pause || play || pause || stop || previous || next))
	{
		// No option supplied; All good
		return RETURN_CONTINUE;
	}

	// Only one of these options may be supplied
	if (!(play_pause ^ play ^ pause ^ stop ^ previous ^ next))
	{
		g_printerr("Only one of %s may be supplied at a time\n", options);

		return RETURN_ERROR;
	}

	if (play_pause)
	{
		g_info("Found command-line option play-pause");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_PLAY_PAUSE, NULL /* parameter */);
	}

	if (play)
	{
		g_info("Found command-line option play");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_PLAY, NULL /* parameter */);
	}

	if (pause)
	{
		g_info("Found command-line option pause");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_PAUSE, NULL /* parameter */);
	}

	if (stop)
	{
		g_info("Found command-line option stop");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_STOP, NULL /* parameter */);
	}

	if (previous)
	{
		g_info("Found command-line option previous");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_PREVIOUS, NULL /* parameter */);
	}

	if (next)
	{
		g_info("Found command-line option next");

		g_action_group_activate_action(G_ACTION_GROUP(WfAppInstance), ACTION_NEXT, NULL /* parameter */);
	}

	return RETURN_SUCCESS;
}

// Returns %TRUE if this instance is remote (primary instance already exists)
static gboolean
wf_app_is_remote(void)
{
	// Make sure to be registered before checking if remote
	return (g_application_register(GAppInstance, NULL /* GCancellable */, NULL /* GError */) &&
	        g_application_get_is_remote(GAppInstance));
}

// Get a #GOptionGroup with all application back-end options
static GOptionGroup *
wf_app_get_option_group(void)
{
	const GOptionEntry *entries;
	GOptionGroup *group;

	group = g_option_group_new(WF_TAG, WF_SUMMARY, "Show " WF_NAME " core options",
	                           NULL /* user_data */, NULL /* GDestroyNotify */);

	entries = wf_option_entries_get_app();
	g_option_group_add_entries(group, entries);

	return group;
}

// Print a nice text containing the application name, version and license
static void
wf_app_print_version_message(void)
{
	g_print("%s v%s\n%s\n%s\n\n%s\n",
	        WF_NAME,
	        WF_VERSION,
	        WF_COPYRIGHT,
	        WF_LICENSE,
	        WF_LICENSE_MESSAGE);
}

// Print all non-hidden available options
static void
wf_app_print_all_options(void)
{
	const GOptionEntry *entry;

	for (entry = wf_option_entries_get_app(); entry != NULL; entry++)
	{
		if (entry->long_name == NULL)
		{
			break;
		}
		else if (entry->flags != G_OPTION_FLAG_HIDDEN)
		{
			g_print("--%s ", entry->long_name);
		}
	}

	g_print("\n");
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

// Convert a #WfPlayerStatus value to the corresponding #WfAppStatus
static WfAppStatus
wf_app_get_status(WfPlayerStatus state)
{
	switch (state)
	{
		case WF_PLAYER_NO_STATUS:
			return WF_APP_UNKNOWN_STATUS;
		case WF_PLAYER_INIT:
			return WF_APP_INIT;
		case WF_PLAYER_READY:
			return WF_APP_READY;
		case WF_PLAYER_PLAYING:
			return WF_APP_PLAYING;
		case WF_PLAYER_PAUSED:
			return WF_APP_PAUSED;
		case WF_PLAYER_STOPPED:
			return WF_APP_STOPPED;
	}

	g_warn_if_reached();

	return 0;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

// This gets invoked when the object is disposed (before destruction)
static void
wf_app_dispose(GObject *object)
{
	GObjectClass *parent_class = WfAppParentClass;
	gdouble app_time;

	// Shut down remote interface
	wf_remote_finalize();

	// Get and reset app time
	app_time = wf_app_get_app_time();
	WfTime = 0;

	// Reset all reference pointers
	GAppInstance = NULL;
	WfAppInstance = NULL;
	WfAppClassInstance = NULL;
	WfAppParentClass = NULL;
	WfActive = FALSE;
	WfDestruct = FALSE;

	g_return_if_fail(G_IS_OBJECT_CLASS(parent_class));

	// Chain up parent class methods
	parent_class->dispose(object);

	g_info("Application time %fsec. The end.", app_time);
}

/* DESTRUCTORS END */

/* END OF FILE */
