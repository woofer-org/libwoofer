/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * mpris.c  This file is part of LibWoofer
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
 * - For all return value pointers, the suffix '_rv' is used to indicate the
 *   value of the pointer can be changed by the respective function.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/mpris.h>

// Dependency includes
#include <woofer/utils.h>
#include <woofer/memory.h>

// Resource includes
#include <woofer/static/mediaplayer2.h>

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module adds functionality to the main application that primarily
 * reports information to the desktop environment by following the
 * org.mpris.MediaPlayer2 D-Bus interface specifications (v2.2) from
 * FreeDesktop.  It uses GDBus from GLib/GIO to report the information.
 *
 * The way this module is build, is that the application includes mpris.h and
 * reports information to it by using a collection of get/set methods.  After
 * activating and while running the main loop, GDBus will ask for information by
 * calling the get_property function.  This function then compares a given
 * property name and packs the right value into one of the parameters received
 * from GDBus.  The specified "methods" that desktop environments may call, are
 * processed and reported back to the application via a provided callback
 * function.
 * For modules that utilize this module, it may look like they report
 * information and connect to signals as a client, while in fact the information
 * is reported to D-Bus as a server when D-Bus clients requests information.
 *
 * The implemented interface specification can be found at
 * <https://specifications.freedesktop.org/mpris-spec/latest/index.html>.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

#define MPRIS_BUS_NAME "org.mpris.MediaPlayer2." WF_TAG
#define MPRIS_INTERFACE_ROOT "org.mpris.MediaPlayer2"
#define MPRIS_INTERFACE_PLAYER "org.mpris.MediaPlayer2.Player"
#define MPRIS_INTERFACE_TRACKLIST "org.mpris.MediaPlayer2.TrackList"
#define MPRIS_INTERFACE_PLAYLISTS "org.mpris.MediaPlayer2.Playlists"
#define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_OBJECT_TRACK_ID "/org/mpris/MediaPlayer2/Track"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

/*
 * Structures containing function pointers to call when clients (desktop
 * environment) calls any methods.
 */

struct MediaRootMethods
{
	WfFuncRootRaise raise_func;
	WfFuncRootQuit quit_func;
};

struct MediaPlayerMethods
{
	WfFuncPlayerNext next_func;
	WfFuncPlayerPrevious previous_func;
	WfFuncPlayerPause pause_func;
	WfFuncPlayerPlayPause play_pause_func;
	WfFuncPlayerPlayPause play_func;
	WfFuncPlayerStop stop_func;
	WfFuncPlayerSeek seek_func;
	WfFuncPlayerSetPosition set_position_func;
	WfFuncPlayerOpenUri open_uri_func;
};

/*
 * Every property has a small comment indicating if a property (for
 * clients/desktop environment) is read-only or read-writable.  A read-only
 * property is set by this application and then read from clients via D-Bus.
 * Read-writable indicates that a client may override the value.  The new value
 * can then be processed by other parts of this application.
 */

// Structure to store properties for the MediaPlayer2 Interface
struct MediaRootDetails
{
	struct MediaRootMethods callbacks;

	gboolean can_raise; // Read-only for D-Bus
	gboolean can_quit; // Writable by D-Bus
	gboolean can_set_fullscreen; // Read-only for D-Bus
	gboolean has_track_list; // Read-only for D-Bus
	gboolean fullscreen; // Read-only for D-Bus
	gchar *identity; // Read-only for D-Bus
	gchar *desktop_entry; // Read-only for D-Bus
	gchar **supported_uri_schemes; // Read-only for D-Bus
	gchar **supported_mime_types; // Read-only for D-Bus
};

// Structure to store properties for the MediaPlayer2.Player Interface
struct MediaWfPlayerDetails
{
	struct MediaPlayerMethods callbacks;

	struct MediaMetadataDetails *metadata;

	WfPlaybackStatus playback_status; // Read-only for D-Bus
	gchar *loop_status; // Writable by D-Bus
	gdouble rate; // Writable by D-Bus
	gboolean shuffle; // Writable by D-Bus
	gdouble volume; // Writable by D-Bus
	gint64 position; // Read-only for D-Bus
	gdouble minimum_rate; // Read-only for D-Bus
	gdouble maximum_rate; // Read-only for D-Bus
	gboolean can_go_next; // Read-only for D-Bus
	gboolean can_go_previous; // Read-only for D-Bus
	gboolean can_play; // Read-only for D-Bus
	gboolean can_pause; // Read-only for D-Bus
	gboolean can_seek; // Read-only for D-Bus
	gboolean can_control; // Read-only for D-Bus
};

// Structure to store metadata for a property in MediaPlayer2.Player
struct MediaMetadataDetails
{
	/*
	 * All properties are read-only for D-Bus; following the specifications at
	 * <https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata/>
	 */

	// MPRIS-specific
	guint track_id; // Unique D-Bus track id
	gint64 length; // Microseconds
	gchar *art_url; // URI to art; starting with "file://"

	// Xesam properties
	gchar *url; // URI to the media file; starting with "file://"
	gchar *title; // Title of the track
	gchar *album; // Album name
	gchar **artists; // Array of artists of the track
	gchar **album_artists; // Array of artist names of the album
	gchar **composers; // Array of composers
	gchar **lyricists; // Array of lyricists
	gchar **genres; // Array of genres
	gint disc_number; // Disc number from album
	gint track_number; // Album track number
	gint audio_bpm; // Beats per minute
	gdouble user_rating; // Rating set by the user in range [0.0, 1.0]
	gdouble auto_rating; // Score in range [0.0, 1.0]
	gint use_count; // Play count
	GDateTime *first_used; // Timestamp of first play
	GDateTime *last_used; // Timestamp of last play
	GDateTime *content_created; // Timestamp of content creation in ISO 8601
	gchar *as_text; // Lyrics of the track
	gchar **comments; // Freeform comments
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_mpris_bus_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data);
static void wf_mpris_bus_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data);
static void wf_mpris_bus_name_lost_cb(GDBusConnection *connection, const gchar *name, gpointer user_data);


static void
mpris_method_called_root_cb(GDBusConnection *connection,
                            const gchar *sender,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *method_name,
                            GVariant *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data);

static void
mpris_method_called_player_cb(GDBusConnection *connection,
                              const gchar *sender,
                              const gchar *object_path,
                              const gchar *interface_name,
                              const gchar *method_name,
                              GVariant *parameters,
                              GDBusMethodInvocation *invocation,
                              gpointer user_data);

static GVariant *
mpris_property_get_requested_root_cb(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GError **error,
                                     gpointer user_data);

static GVariant *
mpris_property_get_requested_player_cb(GDBusConnection *connection,
                                       const gchar *sender,
                                       const gchar *object_path,
                                       const gchar *interface_name,
                                       const gchar *property_name,
                                       GError **error,
                                       gpointer user_data);

static void wf_mpris_remote_emit_properties_changed(GVariant *parameters);

static void wf_mpris_root_method_raise(GVariant *parameters);
static void wf_mpris_root_method_quit(GVariant *parameters);

static void wf_mpris_player_method_next(GVariant *parameters);
static void wf_mpris_player_method_previous(GVariant *parameters);
static void wf_mpris_player_method_pause(GVariant *parameters);
static void wf_mpris_player_method_playpause(GVariant *parameters);
static void wf_mpris_player_method_stop(GVariant *parameters);
static void wf_mpris_player_method_play(GVariant *parameters);
static void wf_mpris_player_method_seek(GVariant *parameters);
static void wf_mpris_player_method_setposition(GVariant *parameters);
static void wf_mpris_player_method_openuri(GVariant *parameters);

static void wf_mpris_emit_root_raise(void);
static void wf_mpris_emit_root_quit(void);
static void wf_mpris_emit_player_next(void);
static void wf_mpris_emit_player_previous(void);
static void wf_mpris_emit_player_pause(void);
static void wf_mpris_emit_player_play_pause(void);
static void wf_mpris_emit_player_stop(void);
static void wf_mpris_emit_player_play(void);
static void wf_mpris_emit_player_seek(gint64 offset);
static void wf_mpris_emit_player_set_position(const gchar *track_id, gint64 position);
static void wf_mpris_emit_player_open_uri(const gchar *uri);

static GVariant * wf_mpris_new_variant_str(const gchar *str);
static GVariant * wf_mpris_new_variant_strv(gchar * const *strv);
static GVariant * wf_mpris_new_variant_date_time(GDateTime *dt);
static GVariant * wf_mpris_new_variant_track_path(guint track_id);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static GDBusConnection *SessionConnection = NULL;
static guint BusNameIdentifier = 0;
static guint InterfaceRootId = 0;
static guint InterfacePlayerId = 0;

static struct MediaMetadataDetails MediaMetadataInfo = { 0 };
static struct MediaRootDetails MediaRootData = { 0 };
static struct MediaWfPlayerDetails MediaPlayerData = { .metadata = &MediaMetadataInfo };

static const GDBusInterfaceVTable InterfaceRootMethodVTable =
{
	mpris_method_called_root_cb,
	mpris_property_get_requested_root_cb,
	NULL
};

static const GDBusInterfaceVTable InterfacePlayerMethodVTable =
{
	mpris_method_called_player_cb,
	mpris_property_get_requested_player_cb,
	NULL
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

gboolean
wf_mpris_activate(void)
{
	GError *error = NULL;

	if (SessionConnection == NULL)
	{
		SessionConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL /* GCancellable */, &error);
		if (error != NULL)
		{
			g_warning("Failed to connect to D-Bus Session bus: %s", error->message);
			g_error_free(error);

			return FALSE;
		}
	}

	// Try to acquire the bus name
	BusNameIdentifier = g_bus_own_name(G_BUS_TYPE_SESSION,
	                                   MPRIS_BUS_NAME,
	                                   G_BUS_NAME_OWNER_FLAGS_NONE,
	                                   wf_mpris_bus_acquired_cb,
	                                   wf_mpris_bus_name_acquired_cb,
	                                   wf_mpris_bus_name_lost_cb,
	                                   NULL /* user_data */,
	                                   NULL /* GDestroyNotify */);

	/*
	 * Interface objects will be registered once the bus has been acquired (in
	 * callback bus_acquired).
	 */

	return TRUE;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

// Root interface getters/setters

void
wf_mpris_set_root_can_raise(gboolean can_raise)
{
	MediaRootData.can_raise = can_raise;
}

GVariant *
wf_mpris_get_root_can_raise(void)
{
	return g_variant_new_boolean(MediaRootData.can_raise);
}

void
wf_mpris_set_root_can_quit(gboolean can_quit)
{
	MediaRootData.can_quit = can_quit;
}

GVariant *
wf_mpris_get_root_can_quit(void)
{
	return g_variant_new_boolean(MediaRootData.can_quit);
}

void
wf_mpris_set_root_can_set_fullscreen(gboolean can_set_fullscreen)
{
	MediaRootData.can_set_fullscreen = can_set_fullscreen;
}

GVariant *
wf_mpris_get_root_can_set_fullscreen(void)
{
	return g_variant_new_boolean(MediaRootData.can_set_fullscreen);
}

void
wf_mpris_set_root_has_track_list(gboolean has_track_list)
{
	MediaRootData.has_track_list = has_track_list;
}

GVariant *
wf_mpris_get_root_has_track_list(void)
{
	return g_variant_new_boolean(MediaRootData.has_track_list);
}

void
wf_mpris_set_root_fullscreen(gboolean fullscreen)
{
	MediaRootData.fullscreen = fullscreen;
}

GVariant *
wf_mpris_get_root_fullscreen(void)
{
	return g_variant_new_boolean(MediaRootData.fullscreen);
}

void
wf_mpris_set_root_identity(const gchar *identity)
{
	g_free(MediaRootData.identity);

	MediaRootData.identity = g_strdup(identity);
}

GVariant *
wf_mpris_get_root_identity(void)
{
	return wf_mpris_new_variant_str(MediaRootData.identity);
}

void
wf_mpris_set_root_desktop_entry(const gchar *desktop_entry)
{
	g_free(MediaRootData.desktop_entry);

	MediaRootData.desktop_entry = g_strdup(desktop_entry);
}

GVariant *
wf_mpris_get_root_desktop_entry(void)
{
	return wf_mpris_new_variant_str(MediaRootData.desktop_entry);
}

void
wf_mpris_set_root_supported_uri_schemes(const gchar * const *uri_schemes)
{
	g_strfreev(MediaRootData.supported_uri_schemes);

	MediaRootData.supported_uri_schemes = g_strdupv((gchar **) uri_schemes);
}

GVariant *
wf_mpris_get_root_supported_uri_schemes(void)
{
	return wf_mpris_new_variant_strv(MediaRootData.supported_uri_schemes);
}

void
wf_mpris_set_root_supported_mime_types(const gchar * const *mime_types)
{
	g_strfreev(MediaRootData.supported_mime_types);

	MediaRootData.supported_mime_types = g_strdupv((gchar **) mime_types);
}

GVariant *
wf_mpris_get_root_supported_mime_types(void)
{
	return wf_mpris_new_variant_strv(MediaRootData.supported_mime_types);
}

// Player interface getters/setters

void
wf_mpris_set_player_playback_status(WfPlaybackStatus status)
{
	g_return_if_fail(status > WF_MPRIS_NOT_PLAYING && status < WF_MPRIS_DEFINED);

	MediaPlayerData.playback_status = status;
}

GVariant *
wf_mpris_get_player_playback_status(void)
{
	switch (MediaPlayerData.playback_status)
	{
		case WF_MPRIS_PLAYING:
			return g_variant_new_string("Playing");
		case WF_MPRIS_PAUSED:
			return g_variant_new_string("Paused");
		case WF_MPRIS_STOPPED:
			return g_variant_new_string("Stopped");
		default:
			g_warn_if_reached();
	}

	return g_variant_new_string("Stopped");
}

void
wf_mpris_set_player_rate(gdouble rate)
{
	g_return_if_fail(rate > 0.0 && rate < 10.0);

	MediaPlayerData.rate = rate;
}

GVariant *
wf_mpris_get_player_rate(void)
{
	return g_variant_new_double(MediaPlayerData.rate);
}

GVariant *
wf_mpris_get_player_metadata(void)
{
	/*
	 * According to the FreeDesktop MPRIS (Specifications), Metadata is of
	 * type Metadata_map, described as "a{sv}" (array of string/value pairs).
	 */

	static GVariantBuilder builder;
	struct MediaMetadataDetails *info = MediaPlayerData.metadata;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	// Always required
	g_variant_builder_add(&builder, "{sv}", "mpris:trackid", wf_mpris_new_variant_track_path(info->track_id));

	if (MediaPlayerData.playback_status != WF_MPRIS_STOPPED)
	{
		// Only relevant while active
		g_variant_builder_add(&builder, "{sv}", "mpris:length", g_variant_new_int64(info->length));
		g_variant_builder_add(&builder, "{sv}", "mpris:artUrl", wf_mpris_new_variant_str(info->art_url));
		g_variant_builder_add(&builder, "{sv}", "xesam:album", wf_mpris_new_variant_str(info->album));
		g_variant_builder_add(&builder, "{sv}", "xesam:albumArtist", wf_mpris_new_variant_strv(info->album_artists));
		g_variant_builder_add(&builder, "{sv}", "xesam:artist", wf_mpris_new_variant_strv(info->artists));
		g_variant_builder_add(&builder, "{sv}", "xesam:asText", wf_mpris_new_variant_str(info->as_text));
		g_variant_builder_add(&builder, "{sv}", "xesam:audioBPM", g_variant_new_int32(info->audio_bpm));
		g_variant_builder_add(&builder, "{sv}", "xesam:autoRating", g_variant_new_double(info->auto_rating));
		g_variant_builder_add(&builder, "{sv}", "xesam:comment", wf_mpris_new_variant_strv(info->comments));
		g_variant_builder_add(&builder, "{sv}", "xesam:composer", wf_mpris_new_variant_strv(info->composers));
		g_variant_builder_add(&builder, "{sv}", "xesam:contentCreated", wf_mpris_new_variant_date_time(info->content_created));
		g_variant_builder_add(&builder, "{sv}", "xesam:discNumber", g_variant_new_int32(info->disc_number));
		g_variant_builder_add(&builder, "{sv}", "xesam:firstUsed", wf_mpris_new_variant_date_time(info->first_used));
		g_variant_builder_add(&builder, "{sv}", "xesam:genre", wf_mpris_new_variant_strv(info->genres));
		g_variant_builder_add(&builder, "{sv}", "xesam:lastUsed", wf_mpris_new_variant_date_time(info->last_used));
		g_variant_builder_add(&builder, "{sv}", "xesam:lyricist", wf_mpris_new_variant_strv(info->lyricists));
		g_variant_builder_add(&builder, "{sv}", "xesam:title", wf_mpris_new_variant_str(info->title));
		g_variant_builder_add(&builder, "{sv}", "xesam:trackNumber", g_variant_new_int32(info->track_number));
		g_variant_builder_add(&builder, "{sv}", "xesam:url", wf_mpris_new_variant_str(info->url));
		g_variant_builder_add(&builder, "{sv}", "xesam:useCount", g_variant_new_int32(info->use_count));
		g_variant_builder_add(&builder, "{sv}", "xesam:userRating", g_variant_new_double(info->user_rating));
	}

	return g_variant_builder_end(&builder);
}

void
wf_mpris_set_player_volume(gdouble volume)
{
	g_return_if_fail(volume >= 0.0 && volume <= 1.0);

	MediaPlayerData.volume = volume;
}

GVariant *
wf_mpris_get_player_volume(void)
{
	return g_variant_new_double(MediaPlayerData.volume);
}

void
wf_mpris_set_player_position(gint64 position)
{
	g_return_if_fail(position >= 0);

	MediaPlayerData.position = position;
}

GVariant *
wf_mpris_get_player_position(void)
{
	return g_variant_new_int64(MediaPlayerData.position);
}

void
wf_mpris_set_player_minimum_rate(gdouble minimum_rate)
{
	g_return_if_fail(minimum_rate >= 0.0 && minimum_rate <= 1.0);

	MediaPlayerData.minimum_rate = minimum_rate;
}

GVariant *
wf_mpris_get_player_minimum_rate(void)
{
	return g_variant_new_double(MediaPlayerData.minimum_rate);
}

void
wf_mpris_set_player_maximum_rate(gdouble maximum_rate)
{
	g_return_if_fail(maximum_rate >= 0.0 && maximum_rate <= 1.0);

	MediaPlayerData.maximum_rate = maximum_rate;
}

GVariant *
wf_mpris_get_player_maximum_rate(void)
{
	return g_variant_new_double(MediaPlayerData.maximum_rate);
}

void
wf_mpris_set_player_can_go_next(gboolean can_go_next)
{
	MediaPlayerData.can_go_next = can_go_next;
}

GVariant *
wf_mpris_get_player_can_go_next(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_go_next);
}

void
wf_mpris_set_player_can_go_previous(gboolean can_go_previous)
{
	MediaPlayerData.can_go_previous = can_go_previous;
}

GVariant *
wf_mpris_get_player_can_go_previous(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_go_previous);
}

void
wf_mpris_set_player_can_play(gboolean can_play)
{
	MediaPlayerData.can_play = can_play;
}

GVariant *
wf_mpris_get_player_can_play(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_play);
}

void
wf_mpris_set_player_can_pause(gboolean can_pause)
{
	MediaPlayerData.can_pause = can_pause;
}

GVariant *
wf_mpris_get_player_can_pause(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_pause);
}

void
wf_mpris_set_player_can_seek(gboolean can_seek)
{
	MediaPlayerData.can_seek = can_seek;
}

GVariant *
wf_mpris_get_player_can_seek(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_seek);
}

void
wf_mpris_set_player_can_control(gboolean can_control)
{
	MediaPlayerData.can_control = can_control;
}

GVariant *
wf_mpris_get_player_can_control(void)
{
	return g_variant_new_boolean(MediaPlayerData.can_control);
}

// Metadata setters

void
wf_mpris_set_info_track_id(guint track_id)
{
	MediaMetadataInfo.track_id = track_id;
}

void
wf_mpris_set_info_url(const gchar *url)
{
	g_free(MediaMetadataInfo.url);

	MediaMetadataInfo.url = g_strdup(url);
}

void
wf_mpris_set_info_title(const gchar *title)
{
	g_free(MediaMetadataInfo.title);

	MediaMetadataInfo.title = g_strdup(title);
}

void
wf_mpris_set_info_album(const gchar *album)
{
	g_free(MediaMetadataInfo.album);

	MediaMetadataInfo.album = g_strdup(album);
}

void
wf_mpris_set_info_artists(const gchar * const *artists)
{
	g_strfreev(MediaMetadataInfo.artists);

	MediaMetadataInfo.artists = g_strdupv((gchar **) artists);
}

void
wf_mpris_set_info_album_artists(const gchar * const *album_artists)
{
	g_strfreev(MediaMetadataInfo.album_artists);

	MediaMetadataInfo.album_artists = g_strdupv((gchar **) album_artists);
}

void
wf_mpris_set_info_composers(const gchar * const *composers)
{
	g_strfreev(MediaMetadataInfo.composers);

	MediaMetadataInfo.composers = g_strdupv((gchar **) composers);
}

void
wf_mpris_set_info_lyricists(const gchar * const *lyricists)
{
	g_strfreev(MediaMetadataInfo.lyricists);

	MediaMetadataInfo.lyricists = g_strdupv((gchar **) lyricists);
}

void
wf_mpris_set_info_genres(const gchar * const *genres)
{
	g_strfreev(MediaMetadataInfo.genres);

	MediaMetadataInfo.genres = g_strdupv((gchar **) genres);
}

void
wf_mpris_set_info_dics_number(gint disc_number) // >= 0
{
	g_return_if_fail(disc_number >= 0);

	MediaMetadataInfo.disc_number = disc_number;
}

void
wf_mpris_set_info_track_number(gint track_number) // >= 0
{
	g_return_if_fail(track_number >= 0);

	MediaMetadataInfo.track_number = track_number;
}

void
wf_mpris_set_info_beats_per_minute(gint bpm) // >= 0
{
	g_return_if_fail(bpm >= 0);

	MediaMetadataInfo.audio_bpm = bpm;
}

void
wf_mpris_set_info_duration(gint64 duration) // Nanoseconds
{
	g_return_if_fail(duration >= 0);

	MediaMetadataInfo.length = duration;
}

void
wf_mpris_set_info_rating(gint rating) // [0, 100]
{
	g_return_if_fail(rating >= 0 && rating <= 100);

	MediaMetadataInfo.user_rating = (((gdouble) rating) / 100);
}

void
wf_mpris_set_info_score(gdouble score) // [0.0, 100.0]
{
	g_return_if_fail(score >= 0.0 && score <= 100.0);

	MediaMetadataInfo.auto_rating = (score / 10);
}

void
wf_mpris_set_info_play_count(gint play_count) // >= 0
{
	g_return_if_fail(play_count >= 0);

	MediaMetadataInfo.use_count = play_count;
}

void
wf_mpris_set_info_first_played(GDateTime *first_used) // Adds reference
{
	g_date_time_unref(MediaMetadataInfo.first_used);

	if (first_used == NULL)
	{
		MediaMetadataInfo.first_used = NULL;
	}
	else
	{
		MediaMetadataInfo.first_used = g_date_time_ref(first_used);
	}
}

void
wf_mpris_set_info_first_played_sec(gint64 first_used) // >= 0
{
	GDateTime *dt = NULL;

	if (first_used >= 0)
	{
		dt = g_date_time_new_from_unix_local(first_used);
	}

	wf_mpris_set_info_first_played(dt);
}

void
wf_mpris_set_info_last_played(GDateTime *last_used) // Adds reference
{
	wf_memory_clear_date_time(&MediaMetadataInfo.last_used);

	if (last_used == NULL)
	{
		MediaMetadataInfo.last_used = NULL;
	}
	else
	{
		MediaMetadataInfo.last_used = g_date_time_ref(last_used);
	}
}

void
wf_mpris_set_info_last_played_sec(gint64 last_used) // >= 0
{
	GDateTime *dt = NULL;

	if (last_used >= 0)
	{
		dt = g_date_time_new_from_unix_local(last_used);
	}

	wf_mpris_set_info_last_played(dt);
}

void
wf_mpris_set_info_content_created(GDateTime *content_created) // Adds reference
{
	g_date_time_unref(MediaMetadataInfo.content_created);

	if (content_created == NULL)
	{
		MediaMetadataInfo.content_created = NULL;
	}
	else
	{
		MediaMetadataInfo.content_created = g_date_time_ref(content_created);
	}
}

void
wf_mpris_set_info_content_created_sec(gint64 content_created) // >= 0
{
	GDateTime *dt = NULL;

	if (content_created >= 0)
	{
		dt = g_date_time_new_from_unix_local(content_created);
	}

	wf_mpris_set_info_content_created(dt);
}

void
wf_mpris_set_info_art_url(const gchar *art_url)
{
	g_free(MediaMetadataInfo.art_url);

	MediaMetadataInfo.art_url = g_strdup(art_url);
}

void
wf_mpris_set_info_lyrics(const gchar *lyrics)
{
	g_free(MediaMetadataInfo.as_text);

	MediaMetadataInfo.as_text = g_strdup(lyrics);
}

void
wf_mpris_set_info_comments(const gchar * const *comments)
{
	g_strfreev(MediaMetadataInfo.comments);

	MediaMetadataInfo.comments = g_strdupv((gchar **) comments);
}

void
wf_mpris_connect_root_raise(WfFuncRootRaise cb_func)
{
	MediaRootData.callbacks.raise_func = cb_func;
}

void
wf_mpris_connect_root_quit(WfFuncRootRaise cb_func)
{
	MediaRootData.callbacks.quit_func = cb_func;
}

void
wf_mpris_connect_player_next(WfFuncPlayerNext cb_func)
{
	MediaPlayerData.callbacks.next_func = cb_func;
}

void
wf_mpris_connect_player_previous(WfFuncPlayerPrevious cb_func)
{
	MediaPlayerData.callbacks.previous_func = cb_func;
}

void
wf_mpris_connect_player_pause(WfFuncPlayerPause cb_func)
{
	MediaPlayerData.callbacks.pause_func = cb_func;
}

void
wf_mpris_connect_player_play_pause(WfFuncPlayerPlayPause cb_func)
{
	MediaPlayerData.callbacks.play_pause_func = cb_func;
}

void
wf_mpris_connect_player_stop(WfFuncPlayerStop cb_func)
{
	MediaPlayerData.callbacks.stop_func = cb_func;
}

void
wf_mpris_connect_player_play(WfFuncPlayerPlay cb_func)
{
	MediaPlayerData.callbacks.play_func = cb_func;
}

void
wf_mpris_connect_player_seek(WfFuncPlayerSeek cb_func)
{
	MediaPlayerData.callbacks.seek_func = cb_func;
}

void
wf_mpris_connect_player_set_position(WfFuncPlayerSetPosition cb_func)
{
	MediaPlayerData.callbacks.set_position_func = cb_func;
}

void
wf_mpris_connect_player_open_uri(WfFuncPlayerOpenUri cb_func)
{
	MediaPlayerData.callbacks.open_uri_func = cb_func;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static void
wf_mpris_bus_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	GDBusInterfaceInfo *interface_root = NULL, *interface_player = NULL;
	GError *error = NULL;

	g_debug("D-Bus acquired bus <%s>", name);

	interface_root = mp2_org_mpris_mediaplayer2_get_interface_info();

	// Register root interface object
	InterfaceRootId = g_dbus_connection_register_object(SessionConnection,
	                                                    MPRIS_OBJECT_PATH,
	                                                    interface_root,
	                                                    &InterfaceRootMethodVTable,
	                                                    NULL /* user_data */,
	                                                    NULL /* GDestroyNotify */,
	                                                    &error);
	if (error != NULL)
	{
		g_warning("Failed to register D-Bus object (MediaPlayer2): %s", error->message);
		g_error_free(error);

		g_dbus_connection_unregister_object(SessionConnection, InterfaceRootId);
		InterfaceRootId = 0;

		return;
	}

	interface_player = mp2_org_mpris_mediaplayer2_player_get_interface_info();

	// Register player interface object
	InterfacePlayerId = g_dbus_connection_register_object(SessionConnection,
	                                                      MPRIS_OBJECT_PATH,
	                                                      interface_player,
	                                                      &InterfacePlayerMethodVTable,
	                                                      NULL /* user_data */,
	                                                      NULL /* GDestroyNotify */,
	                                                      &error);
	if (error != NULL)
	{
		g_warning("Failed to register D-Bus object (MediaPlayer2.Player): %s", error->message);
		g_error_free(error);

		g_dbus_connection_unregister_object(SessionConnection, InterfacePlayerId);
		InterfacePlayerId = 0;

		return;
	}

	g_info("Media Player Remote Interface objects registered");
}

static void
wf_mpris_bus_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_debug("D-Bus name <%s> acquired", name);
}

static void
wf_mpris_bus_name_lost_cb(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_debug("D-Bus name <%s> lost", name);
}

// GDBusInterfaceMethodCallFunc
static void
mpris_method_called_root_cb(GDBusConnection *connection,
                            const gchar *sender,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *method_name,
                            GVariant *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
	gchar *method;

	g_info("Remote Media Player Interface method %s called from %s", method_name, sender);

	/*
	 * From here on, finding out what method has been called is done via
	 * a bunch of string compares.  If a more efficient method is possible
	 * relatively easy, please replace the code below.
	 */

	method = g_strdup(method_name);
	wf_utils_str_to_lower(method);

	if (wf_utils_str_is_equal(method, "raise"))
	{
		wf_mpris_root_method_raise(parameters);
	}
	else if (wf_utils_str_is_equal(method, "quit"))
	{
		wf_mpris_root_method_quit(parameters);
	}
	else
	{
		// G_DBUS_ERROR_NOT_SUPPORTED
	}

	g_free(method);

	g_dbus_method_invocation_return_value(invocation, NULL /* parameters */);
}

// GDBusInterfaceMethodCallFunc
static void
mpris_method_called_player_cb(GDBusConnection *connection,
                            const gchar *sender,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *method_name,
                            GVariant *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer user_data)
{
	gchar *method;

	g_info("Remote Media Player Interface method %s called from %s", method_name, sender);

	method = g_strdup(method_name);
	wf_utils_str_to_lower(method);

	if (wf_utils_str_is_equal(method, "next"))
	{
		wf_mpris_player_method_next(parameters);
	}
	else if (wf_utils_str_is_equal(method, "previous"))
	{
		wf_mpris_player_method_previous(parameters);
	}
	else if (wf_utils_str_is_equal(method, "pause"))
	{
		wf_mpris_player_method_pause(parameters);
	}
	else if (wf_utils_str_is_equal(method, "playpause"))
	{
		wf_mpris_player_method_playpause(parameters);
	}
	else if (wf_utils_str_is_equal(method, "stop"))
	{
		wf_mpris_player_method_stop(parameters);
	}
	else if (wf_utils_str_is_equal(method, "play"))
	{
		wf_mpris_player_method_play(parameters);
	}
	else if (wf_utils_str_is_equal(method, "seek"))
	{
		wf_mpris_player_method_seek(parameters);
	}
	else if (wf_utils_str_is_equal(method, "setposition"))
	{
		wf_mpris_player_method_setposition(parameters);
	}
	else if (wf_utils_str_is_equal(method, "openuri"))
	{
		wf_mpris_player_method_openuri(parameters);
	}
	else
	{
		// G_DBUS_ERROR_NOT_SUPPORTED
	}

	g_free(method);

	g_dbus_method_invocation_return_value(invocation, NULL /* parameters */);
}

// GDBusInterfaceGetPropertyFunc
static GVariant *
mpris_property_get_requested_root_cb(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GError **error,
                                     gpointer user_data)
{
	gchar *property;
	GVariant *value = NULL;

	property = g_strdup(property_name);
	wf_utils_str_to_lower(property);

	if (wf_utils_str_is_equal(property, "canquit"))
	{
		value = wf_mpris_get_root_can_quit();
	}
	else if (wf_utils_str_is_equal(property, "fullscreen"))
	{
		value = wf_mpris_get_root_fullscreen();
	}
	else if (wf_utils_str_is_equal(property, "cansetfullscreen"))
	{
		value = wf_mpris_get_root_can_set_fullscreen();
	}
	else if (wf_utils_str_is_equal(property, "canraise"))
	{
		value = wf_mpris_get_root_can_raise();
	}
	else if (wf_utils_str_is_equal(property, "hastracklist"))
	{
		value = wf_mpris_get_root_has_track_list();
	}
	else if (wf_utils_str_is_equal(property, "identity"))
	{
		value = wf_mpris_get_root_identity();
	}
	else if (wf_utils_str_is_equal(property, "desktopentry"))
	{
		value = wf_mpris_get_root_desktop_entry();
	}
	else if (wf_utils_str_is_equal(property, "supportedurischemes"))
	{
		value = wf_mpris_get_root_supported_uri_schemes();
	}
	else if (wf_utils_str_is_equal(property, "supportedmimetypes"))
	{
		value = wf_mpris_get_root_supported_mime_types();
	}

	if (value == NULL)
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
		            "Property %s.%s not recognised", interface_name, property_name);
	}

	g_free(property);

	return value;
}

// GDBusInterfaceGetPropertyFunc
static GVariant *
mpris_property_get_requested_player_cb(GDBusConnection *connection,
                                       const gchar *sender,
                                       const gchar *object_path,
                                       const gchar *interface_name,
                                       const gchar *property_name,
                                       GError **error,
                                       gpointer user_data)
{
	gchar *property;
	GVariant *value = NULL;

	property = g_strdup(property_name);
	wf_utils_str_to_lower(property);

	if (wf_utils_str_is_equal(property, "playbackstatus"))
	{
		value = wf_mpris_get_player_playback_status();
	}
	else if (wf_utils_str_is_equal(property, "loopstatus"))
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
		            "Property %s.%s not supported", interface_name, property_name);
	}
	else if (wf_utils_str_is_equal(property, "rate"))
	{
		value = wf_mpris_get_player_rate();
	}
	else if (wf_utils_str_is_equal(property, "shuffle"))
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
		            "Property %s.%s not supported", interface_name, property_name);
	}
	else if (wf_utils_str_is_equal(property, "metadata"))
	{
		value = wf_mpris_get_player_metadata();
	}
	else if (wf_utils_str_is_equal(property, "volume"))
	{
		value = wf_mpris_get_player_volume();
	}
	else if (wf_utils_str_is_equal(property, "position"))
	{
		value = wf_mpris_get_player_position();
	}
	else if (wf_utils_str_is_equal(property, "minimumrate"))
	{
		value = wf_mpris_get_player_minimum_rate();
	}
	else if (wf_utils_str_is_equal(property, "maximumrate"))
	{
		value = wf_mpris_get_player_maximum_rate();
	}
	else if (wf_utils_str_is_equal(property, "cangonext"))
	{
		value = wf_mpris_get_player_can_go_next();
	}
	else if (wf_utils_str_is_equal(property, "cangoprevious"))
	{
		value = wf_mpris_get_player_can_go_previous();
	}
	else if (wf_utils_str_is_equal(property, "canplay"))
	{
		value = wf_mpris_get_player_can_play();
	}
	else if (wf_utils_str_is_equal(property, "canpause"))
	{
		value = wf_mpris_get_player_can_pause();
	}
	else if (wf_utils_str_is_equal(property, "canseek"))
	{
		value = wf_mpris_get_player_can_seek();
	}
	else if (wf_utils_str_is_equal(property, "cancontrol"))
	{
		value = wf_mpris_get_player_can_control();
	}
	else
	{
		g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
		            "Property %s.%s not recognised", interface_name, property_name);
	}

	g_free(property);

	return value;
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

void
wf_mpris_flush_changes(void)
{
	GVariant *values;
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	// Add relevant properties (properties that may change frequently in runtime)
	g_variant_builder_add(&builder, "{sv}", "Rate", wf_mpris_get_player_rate());
	g_variant_builder_add(&builder, "{sv}", "Volume", wf_mpris_get_player_volume());
	g_variant_builder_add(&builder, "{sv}", "Position", wf_mpris_get_player_position());
	g_variant_builder_add(&builder, "{sv}", "PlaybackStatus", wf_mpris_get_player_playback_status());
	g_variant_builder_add(&builder, "{sv}", "Metadata", wf_mpris_get_player_metadata());

	values = g_variant_new("(sa{sv}as)", MPRIS_INTERFACE_PLAYER, &builder, NULL /* terminator */);

	wf_mpris_remote_emit_properties_changed(values);
}

static void
wf_mpris_remote_emit_properties_changed(GVariant *parameters)
{
	gboolean res;
	GError *error = NULL;

	g_return_if_fail(G_IS_DBUS_CONNECTION(SessionConnection));

	res = g_dbus_connection_emit_signal(SessionConnection,
	                                    NULL /* destination bus */,
	                                    MPRIS_OBJECT_PATH,
	                                    "org.freedesktop.DBus.Properties",
	                                    "PropertiesChanged",
	                                    parameters,
	                                    &error);

	if (res)
	{
		g_info("MPRIS clients notified of changed properties");
	}
	else
	{
		g_warning("Failed to notify MPRIS clients of property changes (by emitting D-Bus signal PropertiesChanged): %s", error->message);
	}

	g_clear_error(&error);
}

static void
wf_mpris_root_method_raise(GVariant *parameters)
{
	wf_mpris_emit_root_raise();
}

static void
wf_mpris_root_method_quit(GVariant *parameters)
{
	wf_mpris_emit_root_quit();
}

static void
wf_mpris_player_method_next(GVariant *parameters)
{
	wf_mpris_emit_player_next();
}

static void
wf_mpris_player_method_previous(GVariant *parameters)
{
	wf_mpris_emit_player_previous();
}

static void
wf_mpris_player_method_pause(GVariant *parameters)
{
	wf_mpris_emit_player_pause();
}

static void
wf_mpris_player_method_playpause(GVariant *parameters)
{
	wf_mpris_emit_player_play_pause();
}

static void
wf_mpris_player_method_stop(GVariant *parameters)
{
	wf_mpris_emit_player_stop();
}

static void
wf_mpris_player_method_play(GVariant *parameters)
{
	wf_mpris_emit_player_play();
}

static void
wf_mpris_player_method_seek(GVariant *parameters)
{
	gint64 position;

	g_return_if_fail(g_variant_is_of_type(parameters, (const GVariantType *) "(x)"));

	position = g_variant_get_int64(parameters);
	wf_mpris_emit_player_seek(position);
}

static void
wf_mpris_player_method_setposition(GVariant *parameters)
{
	const gchar *object_path;
	gint64 position;

	g_return_if_fail(g_variant_is_of_type(parameters, (const GVariantType *) "(ox)"));

	object_path = g_variant_get_string(parameters, NULL /* length_rv */);
	position = g_variant_get_int64(parameters);
	wf_mpris_emit_player_set_position(object_path, position);
}

static void
wf_mpris_player_method_openuri(GVariant *parameters)
{
	const gchar *uri;

	g_return_if_fail(g_variant_is_of_type(parameters, (const GVariantType *) "(x)"));

	uri = g_variant_get_string(parameters, NULL /* length_rv */);
	wf_mpris_emit_player_open_uri(uri);
}

static void
wf_mpris_emit_root_raise(void)
{
	if (MediaRootData.callbacks.raise_func != NULL)
	{
		MediaRootData.callbacks.raise_func();
	}
}

static void
wf_mpris_emit_root_quit(void)
{
	if (MediaRootData.callbacks.quit_func != NULL)
	{
		MediaRootData.callbacks.quit_func();
	}
}

static void
wf_mpris_emit_player_next(void)
{
	if (MediaPlayerData.callbacks.next_func != NULL)
	{
		MediaPlayerData.callbacks.next_func();
	}
}

static void
wf_mpris_emit_player_previous(void)
{
	if (MediaPlayerData.callbacks.previous_func != NULL)
	{
		MediaPlayerData.callbacks.previous_func();
	}
}

static void
wf_mpris_emit_player_pause(void)
{
	if (MediaPlayerData.callbacks.pause_func != NULL)
	{
		MediaPlayerData.callbacks.pause_func();
	}
}

static void
wf_mpris_emit_player_play_pause(void)
{
	if (MediaPlayerData.callbacks.play_pause_func != NULL)
	{
		MediaPlayerData.callbacks.play_pause_func();
	}
}

static void
wf_mpris_emit_player_stop(void)
{
	if (MediaPlayerData.callbacks.stop_func != NULL)
	{
		MediaPlayerData.callbacks.stop_func();
	}
}

static void
wf_mpris_emit_player_play(void)
{
	if (MediaPlayerData.callbacks.play_func != NULL)
	{
		MediaPlayerData.callbacks.play_func();
	}
}

static void
wf_mpris_emit_player_seek(gint64 offset)
{
	if (MediaPlayerData.callbacks.seek_func != NULL)
	{
		MediaPlayerData.callbacks.seek_func(offset);
	}
}

static void
wf_mpris_emit_player_set_position(const gchar *track_id, gint64 position)
{
	if (MediaPlayerData.callbacks.set_position_func != NULL)
	{
		MediaPlayerData.callbacks.set_position_func(track_id, position);
	}
}

static void
wf_mpris_emit_player_open_uri(const gchar *uri)
{
	if (MediaPlayerData.callbacks.set_position_func != NULL)
	{
		MediaPlayerData.callbacks.open_uri_func(uri);
	}
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

// Creates a GVariant with a string that can be NULL
static GVariant *
wf_mpris_new_variant_str(const gchar *str)
{
	if (str == NULL)
	{
		return g_variant_new_string("");
	}
	else
	{
		return g_variant_new_string(str);
	}
}

// Creates a GVariant with a string array that can be NULL
static GVariant *
wf_mpris_new_variant_strv(gchar * const *strv)
{
	gssize length = 0;

	if (strv != NULL)
	{
		length = -1; // NULL terminated
	}

	return g_variant_new_strv((const gchar * const *) strv, length);
}

// Creates a GVariant from a GDateTime in the ISO8601 format
static GVariant *
wf_mpris_new_variant_date_time(GDateTime *dt)
{
	// g_date_time_format with "$F" returns the ISO8601 format
	gchar *iso8601 = NULL;

	if (dt == NULL)
	{
		return g_variant_new_string("");
	}
	else
	{
		iso8601 = g_date_time_format(dt, "%F");

		return g_variant_new_take_string(iso8601); // Takes ownership
	}
}

// Creates a GVariant string with the right D-Bus Track object path
static GVariant *
wf_mpris_new_variant_track_path(guint track_id)
{
	return g_variant_new_printf("%s/%u", MPRIS_OBJECT_TRACK_ID, track_id);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_mpris_deactivate(void)
{
	g_dbus_connection_unregister_object(SessionConnection, InterfaceRootId);
	g_dbus_connection_unregister_object(SessionConnection, InterfacePlayerId);

	g_bus_unown_name(BusNameIdentifier);

	g_object_unref(SessionConnection);

	InterfaceRootId = 0;
	InterfacePlayerId = 0;
	BusNameIdentifier = 0;
	SessionConnection = NULL;

	g_info("Media Player Remote Interface objects unregistered");
}

/* DESTRUCTORS END */

/* END OF FILE */
