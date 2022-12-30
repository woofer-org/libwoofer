/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * remote.c  This file is part of LibWoofer
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

/*
 * Notes:
 * - For all callback functions, the suffix '_cb' is used as a quick indicator
 *   for the reader.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/remote.h>

// Dependency includes
#include <woofer/app.h>
#include <woofer/library.h>
#include <woofer/song_manager.h>
#include <woofer/player.h>
#include <woofer/song.h>
#include <woofer/utils.h>
#include <woofer/memory.h>

// Resource includes
#include <woofer/static/gdbus.h>

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This is the module for the D-Bus remote interface.  All application core
 * functionality should be accessible via this interface without the need of
 * any other cli or graphical interface.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */


static void
wf_remote_bus_get_finish_cb(GObject *source_object,
                            GAsyncResult *result,
                            gpointer user_data);

static void
wf_remote_bus_closed_cb(GDBusConnection *connection,
                        gboolean remote_peer_vanished,
                        GError *error,
                        gpointer user_data);

static void
remote_app_method_called_cb(GDBusConnection *connection,
                            const gchar *sender,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *method_name,
                            GVariant *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data);

static GVariant *
remote_app_property_get_cb(GDBusConnection *connection,
                           const gchar *sender,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *property_name,
                           GError **error,
                           gpointer user_data);

static gboolean
remote_app_property_set_cb(GDBusConnection *connection,
                           const gchar *sender,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *property_name,
                           GVariant *value,
                           GError **error,
                           gpointer user_data);

static void
remote_player_method_called_cb(GDBusConnection *connection,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data);

static GVariant *
remote_player_property_get_cb(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *property_name,
                              GError **error,
                              gpointer user_data);

static gboolean
remote_player_property_set_cb(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *property_name,
                              GVariant *value,
                              GError **error,
                              gpointer user_data);

static guint32 wf_remote_get_song_id(WfSong *song);

static gboolean wf_remote_parameters_get_bool(GVariant *parameters, gsize index);
static guint32 wf_remote_parameters_get_uint32(GVariant *parameters, gsize index);
static gdouble wf_remote_parameters_get_double(GVariant *parameters, gsize index);
static const gchar * wf_remote_parameters_get_str(GVariant *parameters, gsize index);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static GDBusConnection *DBusConnection;

static guint RemoteAppId = 0;
static guint RemotePlayerId = 0;

static const GDBusInterfaceVTable RemoteAppMethods =
{
	remote_app_method_called_cb,
	remote_app_property_get_cb,
	remote_app_property_set_cb
};

static const GDBusInterfaceVTable RemotePlayerMethods =
{
	remote_player_method_called_cb,
	remote_player_property_get_cb,
	remote_player_property_set_cb
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_remote_init(GDBusConnection *connection)
{
	// Get a connection if we don't have one
	if (connection == NULL)
	{
		g_bus_get(G_BUS_TYPE_SESSION, NULL /* GCancellable */, wf_remote_bus_get_finish_cb, NULL /* user_data */);
	}
	else
	{
		// Setup now
		wf_remote_setup(connection);
	}
}

void
wf_remote_setup(GDBusConnection *connection)
{
	GDBusInterfaceInfo *iface_info;
	GError *error = NULL;
	guint id;

	g_return_if_fail(G_IS_DBUS_CONNECTION(connection));

	DBusConnection = connection;

	g_signal_connect(connection, "closed", G_CALLBACK(wf_remote_bus_closed_cb), NULL /* user_data */);

	// Registration of the application interface

	iface_info = wf_org_woofer_app_get_interface_info();

	id = g_dbus_connection_register_object(connection, WF_PATH, iface_info, &RemoteAppMethods, NULL /* user_data */, NULL /* GDestroyNotify */, &error);
	RemoteAppId = id;

	if (error != NULL)
	{
		g_warning("Could not register application D-Bus interface: %s", error->message);
		g_error_free(error);

		return;
	}

	// Registration of the player interface

	iface_info = wf_org_woofer_player_get_interface_info();

	id = g_dbus_connection_register_object(connection, WF_PATH, iface_info, &RemotePlayerMethods, NULL /* user_data */, NULL /* GDestroyNotify */, &error);
	RemotePlayerId = id;

	if (error != NULL)
	{
		g_warning("Could not register player D-Bus interface: %s", error->message);
		g_error_free(error);

		return;
	}
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static void
wf_remote_bus_get_finish_cb(GObject *source_object,
                            GAsyncResult *result,
                            gpointer user_data)
{
	GDBusConnection *connection;
	GError *error = NULL;

	connection = g_bus_get_finish(result, &error);

	if (error != NULL)
	{
		g_warning("Could not initialize D-Bus interface: %s", error->message);
		g_error_free(error);
	}
	else if (connection == NULL)
	{
		g_warning("Could not initialize D-Bus interface");
	}
	else
	{
		wf_remote_setup(connection);
	}
}

static void
wf_remote_bus_closed_cb(GDBusConnection *connection,
                        gboolean remote_peer_vanished,
                        GError *error,
                        gpointer user_data)
{
	wf_remote_finalize();
}

// GDBusInterfaceMethodCallFunc
static void
remote_app_method_called_cb(GDBusConnection *connection,
                            const gchar *sender,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *method_name,
                            GVariant *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
	const gchar *v_str;
	GError *error;
	gchar *method;
	gboolean v_bool;
	gint v_int;

	// Always check with lowercase names
	method = g_strdup(method_name);
	wf_utils_str_to_lower(method);

	if (wf_utils_str_is_equal(method, "quit"))
	{
		wf_app_quit();
	}
	else if (wf_utils_str_is_equal(method, "raise"))
	{
		wf_app_raise();
	}
	else if (wf_utils_str_is_equal(method, "refreshmetadata"))
	{
		v_int = wf_library_update_metadata();

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", v_int));

		return;
	}
	else if (wf_utils_str_is_equal(method, "addsong"))
	{
		v_str = wf_remote_parameters_get_str(parameters, 0);
		v_bool = wf_library_add_by_uri(v_str, NULL /* func_item_added */, 0 /* checks: default */, FALSE);

		g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", v_bool));

		return;
	}
	else
	{
		error = g_error_new(G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "Method <%s> not supported", method_name);
		g_dbus_method_invocation_take_error(invocation, error);

		return;
	}

	g_dbus_method_invocation_return_value(invocation, NULL /* parameters */);
}

// GDBusInterfaceGetPropertyFunc
static GVariant *
remote_app_property_get_cb(GDBusConnection *connection,
                           const gchar *sender,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *property_name,
                           GError **error,
                           gpointer user_data)
{
	// No properties to implement

	g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Property %s.%s not supported", interface_name, property_name);

	return NULL;
}

// GDBusInterfaceSetPropertyFunc
static gboolean
remote_app_property_set_cb(GDBusConnection *connection,
                           const gchar *sender,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *property_name,
                           GVariant *value,
                           GError **error,
                           gpointer user_data)
{
	 // No properties to implement

	g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Property %s.%s not supported", interface_name, property_name);

	return FALSE;
}

// GDBusInterfaceMethodCallFunc
static void
remote_player_method_called_cb(GDBusConnection *connection,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data)
{
	WfSong *song;
	GError *error;
	gchar *method;
	gboolean v_bool;
	guint32 v_uint32;
	gdouble v_double;

	// Always check with lowercase names
	method = g_strdup(method_name);
	wf_utils_str_to_lower(method);

	if (wf_utils_str_is_equal(method, "setplaying"))
	{
		v_uint32 = wf_remote_parameters_get_uint32(parameters, 0);
		song = wf_song_get_by_hash(v_uint32);

		if (song != NULL)
		{
			wf_player_open(song);
		}
	}
	else if (wf_utils_str_is_equal(method, "setqueue"))
	{
		v_uint32 = wf_remote_parameters_get_uint32(parameters, 0);
		v_bool = wf_remote_parameters_get_bool(parameters, 1);
		song = wf_song_get_by_hash(v_uint32);

		if (song != NULL)
		{
			v_bool ? wf_player_queue_add(song) : wf_player_queue_rm(song);
		}
	}
	else if (wf_utils_str_is_equal(method, "stopaftersong"))
	{
		v_uint32 = wf_remote_parameters_get_uint32(parameters, 0);
		song = wf_song_get_by_hash(v_uint32);

		wf_player_stop_after_song(song);
	}
	else if (wf_utils_str_is_equal(method, "seek"))
	{
		v_double = wf_remote_parameters_get_double(parameters, 0);
		wf_player_seek_percentage(v_double);
	}
	else if (wf_utils_str_is_equal(method, "play"))
	{
		wf_player_play();
	}
	else if (wf_utils_str_is_equal(method, "pause"))
	{
		wf_player_pause();
	}
	else if (wf_utils_str_is_equal(method, "playpause"))
	{
		wf_player_play_pause();
	}
	else if (wf_utils_str_is_equal(method, "backward"))
	{
		wf_player_backward(FALSE);
	}
	else if (wf_utils_str_is_equal(method, "forward"))
	{
		wf_player_forward(FALSE);
	}
	else if (wf_utils_str_is_equal(method, "stop"))
	{
		wf_player_stop();
	}
	else
	{
		error = g_error_new(G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "Method <%s> not supported", method_name);
		g_dbus_method_invocation_take_error(invocation, error);

		return;
	}

	g_dbus_method_invocation_return_value(invocation, NULL /* parameters */);
}

// GDBusInterfaceGetPropertyFunc
static GVariant *
remote_player_property_get_cb(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *property_name,
                              GError **error,
                              gpointer user_data)
{
	WfSong *song;
	gchar *property;
	gboolean v_bool;
	guint32 v_uint32;
	gdouble v_double;

	// Always check with lowercase names
	property = g_strdup(property_name);
	wf_utils_str_to_lower(property);

	if (wf_utils_str_is_equal(property, "songprevious"))
	{
		song = wf_song_manager_get_prev_song();
		v_uint32 = wf_remote_get_song_id(song);

		return g_variant_new_uint32(v_uint32);
	}
	else if (wf_utils_str_is_equal(property, "songplaying"))
	{
		song = wf_song_manager_get_current_song();
		v_uint32 = wf_remote_get_song_id(song);

		return g_variant_new_uint32(v_uint32);
	}
	else if (wf_utils_str_is_equal(property, "songnext"))
	{
		song = wf_song_manager_get_next_song();
		v_uint32 = wf_remote_get_song_id(song);

		return g_variant_new_uint32(v_uint32);
	}
	else if (wf_utils_str_is_equal(property, "incognito"))
	{
		v_bool = wf_song_manager_get_incognito();

		return g_variant_new_boolean(v_bool);
	}
	else if (wf_utils_str_is_equal(property, "volume"))
	{
		v_double = wf_player_get_volume_percentage();

		return g_variant_new_double(v_double);
	}
	else if (wf_utils_str_is_equal(property, "position"))
	{
		v_double = wf_player_get_position();

		return g_variant_new_double(v_double);
	}
	else
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Property %s.%s not supported", interface_name, property_name);

		return NULL;
	}
}

// GDBusInterfaceSetPropertyFunc
static gboolean
remote_player_property_set_cb(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *property_name,
                              GVariant *value,
                              GError **error,
                              gpointer user_data)
{
	gchar *property;
	gboolean v_bool;
	gdouble v_double;

	// Always check with lowercase names
	property = g_strdup(property_name);
	wf_utils_str_to_lower(property);

	if (wf_utils_str_is_equal(property, "incognito"))
	{
		v_bool = wf_remote_parameters_get_bool(value, 0);
		wf_song_manager_set_incognito(v_bool);

		return TRUE;
	}
	else if (wf_utils_str_is_equal(property, "volume"))
	{
		v_double = wf_remote_parameters_get_double(value, 0);
		wf_player_set_volume_percentage(v_double);

		return TRUE;
	}
	else if (wf_utils_str_is_equal(property, "position"))
	{
		v_double = wf_remote_parameters_get_double(value, 0);
		wf_player_seek_seconds(v_double);

		return TRUE;
	}
	else
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Property %s.%s not supported", interface_name, property_name);

		return FALSE;
	}
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */
/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static guint32
wf_remote_get_song_id(WfSong *song)
{
	return (song != NULL) ? wf_song_get_hash(song) : 0;
}

static gboolean
wf_remote_parameters_get_bool(GVariant *parameters, gsize index)
{
	gboolean b;
	GVariant *value;

	value = g_variant_get_child_value(parameters, index);
	b = g_variant_get_boolean(value);
	g_variant_unref(value);

	return b;
}

static guint32
wf_remote_parameters_get_uint32(GVariant *parameters, gsize index)
{
	guint32 u;
	GVariant *value;

	value = g_variant_get_child_value(parameters, index);
	u = g_variant_get_uint32(value);
	g_variant_unref(value);

	return u;
}

static gdouble
wf_remote_parameters_get_double(GVariant *parameters, gsize index)
{
	gdouble d;
	GVariant *value;

	value = g_variant_get_child_value(parameters, index);
	d = g_variant_get_double(value);
	g_variant_unref(value);

	return d;
}

static const gchar *
wf_remote_parameters_get_str(GVariant *parameters, gsize index)
{
	const gchar *s;
	GVariant *value;

	value = g_variant_get_child_value(parameters, index);
	s = g_variant_get_string(value, NULL /* length */);
	g_variant_unref(value);

	return s;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_remote_finalize(void)
{
	g_dbus_connection_unregister_object(DBusConnection, RemoteAppId);
	g_dbus_connection_unregister_object(DBusConnection, RemotePlayerId);

	wf_memory_clear_object((GObject **) &DBusConnection);

	DBusConnection = NULL;
}

/* DESTRUCTORS END */

/* END OF FILE */
