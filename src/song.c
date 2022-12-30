/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song.c  This file is part of LibWoofer
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
 * This module implements a #GObject type.  API documentation comments start
 * with an extra '*' at the beginning of the comment block.  See the
 * GTK-Doc Manual for details.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/song.h>
#include <woofer/song_private.h>

// Dependency includes
#include <woofer/song_metadata.h>
#include <woofer/utils.h>
#include <woofer/characters.h>
#include <woofer/memory.h>
#include <woofer/tweaks.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/**
 * SECTION:wfsong
 * @title: Woofer Song
 * @short_description: The application's song object derived from #GObject
 *
 * This modules provides and handles everything regarding songs.  A single
 * object represents one song with its own URI, metadata, statistics and more.
 * Song objects are, together with main components such as player, application,
 * library and settings, crucial modules that form the fundamentals of the
 * application.  Song objects are the objects that are used to represent the
 * content of the song library, used to play audio and they can exist from a few
 * to potentially thousands of instances.
 *
 * The song library, living as a separate module, is sometimes referred to as
 * 'song list' or simply 'list' in this module, as the songs are linked together
 * in a two-way linked list structure.
 **/

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Parent #GType of #WfSong
#define WF_SONG_TYPE_PARENT G_TYPE_OBJECT

// Associated names for #WfSong and #WfSongStatus
#define WF_SONG_TYPE_NAME "WfSong"
#define WF_SONG_STATUS_TYPE_NAME "WfSongStatus"

// #GType flags for #WfSong and #WfSongStatus
#define WF_SONG_TYPE_FLAGS ((GTypeFlags) 0)

// Values to set as default for a new song
#define WF_SONG_INITIAL_RATING 0
#define WF_SONG_INITIAL_SCORE 50
#define WF_SONG_INITIAL_PLAYCOUNT 0
#define WF_SONG_INITIAL_SKIPCOUNT 0
#define WF_SONG_INITIAL_LASTPLAYED 0

// Flags to use when querying file information
#define WF_SONG_FILE_INFO G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," \
                          G_FILE_ATTRIBUTE_TIME_MODIFIED

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef enum _WfProp WfProp;

struct _WfSongPrivate
{
	WfSong *prev; // Previous item in library
	WfSong *next; // Next item in library

	WfSongStatus status; // Current item's status
	gint64 fs_modified; // Timestamp of last modification in filesystem
	gboolean in_list; // %TRUE if currently in the library
	gboolean queued; // %TRUE if the song is in the queue
	gboolean stop_after_playing; // %TRUE if the playback should stop after this song

	GFile *file; // #GFile container (used for file operations)
	gchar *uri; // Full URI (location)
	gchar *name; // Only filename including extension
	gchar *display_name; // Filename to be shown in interface
	gchar *tag; // ID (text representation of the hash)
	guint32 song_hash; // ID (number representation of the hash)
	guint32 artist_hash; // Hash of the artist string
	guint32 album_artist_hash; // Hash of the album artist string
	gint64 updated; // Timestamp of the last metadata update

	gchar *title; // Title read from metadata
	gchar *artist; // Artist read from metadata
	gchar *album_artist; // Album artist read from metadata
	gchar *album; // Album read from metadata
	gint number; // Track number read from metadata
	gint duration; // Duration read from metadata (secconds)

	gint rating; // User-set rating
	gdouble score; // Software determined rating
	gint playcount; // Amount of play times
	gint skipcount; // Amount of skips
	gint64 lastplayed; // Timestamp of the last playtime
};

// Property enum that can be converted to a #guint for easy property lookup
enum _WfProp
{
	WF_PROP_NONE,

	WF_PROP_STATUS,
	WF_PROP_FILE,
	WF_PROP_URI,
	WF_PROP_NAME,
	WF_PROP_DISPLAY_NAME,
	WF_PROP_MODIFIED,
	WF_PROP_IN_LIST,
	WF_PROP_QUEUED,
	WF_PROP_STOP,

	WF_PROP_TITLE,
	WF_PROP_ARTIST,
	WF_PROP_ALBUM_ARTIST,
	WF_PROP_ALBUM,
	WF_PROP_NUMBER,
	WF_PROP_DURATION,

	WF_PROP_RATING,
	WF_PROP_SCORE,
	WF_PROP_PLAY_COUNT,
	WF_PROP_SKIP_COUNT,
	WF_PROP_LAST_PLAYED,

	WF_PROP_COUNT
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static WfSong * wf_song_new(void);

static void wf_song_class_init(gpointer g_class, gpointer class_data);
static void wf_song_instance_init(GTypeInstance *instance, gpointer g_class);

static void wf_song_set_prev(WfSong *song, WfSong *prev);
static void wf_song_set_next(WfSong *song, WfSong *next);
static void wf_song_add_priv_struct(WfSong *song);

static void wf_song_set_tag_take_str(WfSong *song, gchar *tag);
static void wf_song_set_file(WfSong *song, GFile *file);
static void wf_song_set_uri(WfSong *song, const gchar *uri);
static void wf_song_set_uri_internal(WfSong *song, const gchar *uri);
static void wf_song_set_display_name(WfSong *song, const gchar *name);
static void wf_song_set_modified(WfSong *song, gint64 timestamp);

static void wf_song_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void wf_song_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void wf_song_clear_location(WfSong *song);

static WfSong * wf_song_prepend_song(WfSong *song);
static WfSong * wf_song_append_song(WfSong *song);

static void wf_song_set_new_metadata(WfSong *song, WfSongMetadata *metadata);
static void wf_song_update_fs_info(WfSong *song);

static WfSong * wf_song_ref_sink(WfSong *song);

static gboolean wf_song_is_valid_status(WfSongStatus state);

static gchar * wf_song_new_tag(guint32 hash);

static gchar * wf_song_last_played_to_string(gint64 last_played);
static gchar * wf_song_last_played_to_played_on_string(gint64 last_played, gboolean include_time);

static void wf_song_finalize(gpointer object);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

// List items
static WfSong *FirstSong;
static WfSong *LastSong;
static gint SongCount = 0;

// Array of pointers to property specifications
static GParamSpec *WfProperties[WF_PROP_COUNT];

// Registered #GType of #WfSong and #WfSongStatus
static GType WfSongType = 0;
static GType WfSongStatusType = 0;

/*
 * #GType info and callbacks for handling #WfSong instances (note that members
 * that are not given a value are set to 0 or %NULL.
 * The size values is what the type system uses to allocate the object's memory.
 */
static const GTypeInfo WfSongTypeInfo =
{
	// Size of the class structure
	.class_size = sizeof(WfSongClass),

	// Size of the instance structure
	.instance_size = sizeof(WfSong),

	// Class initialization function
	.class_init = wf_song_class_init,

	// Instance initialization function
	.instance_init = wf_song_instance_init,

	// Instance finalization function
	.base_finalize = wf_song_finalize,
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

/*
 * wf_song_new:
 *
 * Creates a new #WfSong object.  Once returned, the object is fully initialized
 * by GObject and all default values are set.
 *
 * The returned value is guaranteed to be non-%NULL, as GObject will terminate
 * the application when allocation fails.
 *
 * Returns: (transfer full): a new #WfSong
 *
 * Since: 0.1
 */
static WfSong *
wf_song_new(void)
{
	return (WfSong *) g_object_new(WF_TYPE_SONG, NULL /* terminator */);
}

/*
 * wf_song_class_init:
 *
 * Gets invoked when the class should initialize.  This should only be done by
 * GObject when it thinks it is time to do so.
 *
 * More information about the GObject structure can be found in the GObject
 * documentation or reference manual.
 *
 * Since: 0.1
 */
static void
wf_song_class_init(gpointer g_class, gpointer class_data)
{
	WfSongClass *wf_class = g_class;
	GObjectClass *object_class = G_OBJECT_CLASS(wf_class);
	GParamSpec *prop;
	WfProp prop_id;

	g_return_if_fail(WF_IS_SONG_CLASS(wf_class));
	g_return_if_fail(G_IS_OBJECT_CLASS(object_class));

	// Override object class method pointers
	object_class->get_property = wf_song_get_property;
	object_class->set_property = wf_song_set_property;

	// Create property specifications and install them

	/**
	 * WfSong:status:
	 *
	 * The current status of the song.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_STATUS;
	prop = g_param_spec_enum("status",
	                         "Status",
	                         "The current status of the song",
	                         WF_TYPE_SONG_STATUS,
	                         WF_SONG_STATUS_UNKNOWN,
	                         G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:file:
	 *
	 * The #GFile for the song.  When %NULL, it will be created.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_FILE;
	prop = g_param_spec_object("file",
	                           "File",
	                           "The #GFile object for the song",
	                           G_TYPE_FILE,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:uri:
	 *
	 * The URI for the song.  Unless any bugs are present or errors/warnings
	 * occur, it is should always be non-%NULL.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_URI;
	prop = g_param_spec_string("uri",
	                           "URI",
	                           "The URI for the song",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:name:
	 *
	 * The basename of the song's URI.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_NAME;
	prop = g_param_spec_string("name",
	                           "Basename",
	                           "The basename of the song",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:display-name:
	 *
	 * The display name of the song's file.  This name should be favored
	 * over the basename to show to the user.  It is read from metadata, but
	 * may not be accurate.  Use #WfSong:name in case this is %NULL.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_DISPLAY_NAME;
	prop = g_param_spec_string("display-name",
	                           "Display name",
	                           "The display name of the song's file",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:modified:
	 *
	 * Timestamp of the last modification to the song's file.
	 *
	 * It is possible that the modification timestamp is unknown.  In that
	 * case this property has a value of 0.
	 *
	 * The timestamp is in Unix time and is year-2038-safe.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_MODIFIED;
	prop = g_param_spec_int64("modified",
	                          "Modified",
	                          "Timestamp of the last modification to the song's file",
	                          0,
	                          G_MAXINT64,
	                          0,
	                          G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:in-list:
	 *
	 * In list flag indicating that this song is currently present in the
	 * song library.  In case this is %FALSE, there may still be a reference
	 * to the song somewhere and it will stay alive in memory until the last
	 * reference is dropped.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_IN_LIST;
	prop = g_param_spec_boolean("in-list",
	                            "In list",
	                            "%TRUE if the song is present in the library",
	                            FALSE,
	                            G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:queued:
	 *
	 * Queue flag indicating that this song is currently queued.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_QUEUED;
	prop = g_param_spec_boolean("queued",
	                            "Queue flag",
	                            "%TRUE if the song is queued",
	                            FALSE,
	                            G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:stop:
	 *
	 * Stop flag indicating that playback should stop after this song.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_STOP;
	prop = g_param_spec_boolean("stop",
	                            "Stop flag",
	                            "%TRUE if playback should stop after this song",
	                            FALSE,
	                            G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:title:
	 *
	 * Tag read from metadata containing the song's title.
	 *
	 * The title should represent the name of the song.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_TITLE;
	prop = g_param_spec_string("title",
	                           "Title",
	                           "Song's metadata title",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:artist:
	 *
	 * Tag read from metadata containing the song's artist.
	 *
	 * The artist should represent the artist of the song and may include
	 * names of featuring artists or collaborations.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_ARTIST;
	prop = g_param_spec_string("artist",
	                           "Artist",
	                           "Song's metadata artist",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:album-artist:
	 *
	 * Tag read from metadata containing the song's album artist.
	 *
	 * The album artist should represent the main artist of the album the
	 * song is part of.  It should be the same for all songs in the same
	 * album and should not include names of featuring artists or
	 * collaborations.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_ALBUM_ARTIST;
	prop = g_param_spec_string("album-artist",
	                           "Album artist",
	                           "Song's metadata album artist",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:album:
	 *
	 * Tag read from metadata containing the song's album.
	 *
	 * The album should represent the name of the album and should not
	 * contain any artist names or volume number.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_ALBUM;
	prop = g_param_spec_string("album",
	                           "Album",
	                           "Song's metadata album",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:number:
	 *
	 * Tag read from metadata containing the song's track number.
	 *
	 * The track number should represent the song's position in the album,
	 * starting at 1.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_NUMBER;
	prop = g_param_spec_string("number",
	                           "Track number",
	                           "Song's metadata track number",
	                           NULL,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:duration:
	 *
	 * Tag read from metadata containing the song's duration in seconds.
	 *
	 * Please note that the value may be inaccurate or 0, as it is not
	 * determined by the playback or by any audio stream length.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_DURATION;
	prop = g_param_spec_uint64("duration",
	                           "Duration",
	                           "Song's metadata duration",
	                           0,
	                           G_MAXUINT64,
	                           0,
	                           G_PARAM_READABLE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:rating:
	 *
	 * The song's rating.  The rating is set by the user and may be used by
	 * the software for auto song choosing.  The value is a regular integer
	 * but how it should be represented is up to the interface designer.
	 * For example, some stars that can be set by one or more half and full
	 * stars; a slider that the user can drag or a simple integer input
	 * field.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_RATING;
	prop = g_param_spec_int("rating",
	                        "User rating",
	                        "The song's user rating",
	                        0,
	                        100,
	                        0,
	                        G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:score:
	 *
	 * The song's score.  The score is changed by the software and may be
	 * used for auto song choosing.  User overriding should only be possible
	 * for advanced usage.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_SCORE;
	prop = g_param_spec_double("score",
	                           "Score",
	                           "The song's score",
	                           0.0,
	                           100.0,
	                           50.0,
	                           G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:play-count:
	 *
	 * The song's play count.  It simply represents the amount of times
	 * the song has been played.  User overriding should only be possible
	 * for advanced usage.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_PLAY_COUNT;
	prop = g_param_spec_int("play-count",
	                        "Play count",
	                        "The song's play count",
	                        0,
	                        G_MAXINT,
	                        0,
	                        G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:skip-count:
	 *
	 * The song's skip count.  It simply represents the amount of times
	 * the song has been skipped by the user.  User overriding should only
	 * be possible for advanced usage.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_SKIP_COUNT;
	prop = g_param_spec_int("skip-count",
	                        "Skip count",
	                        "The song's skip count",
	                        0,
	                        G_MAXINT,
	                        0,
	                        G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);

	/**
	 * WfSong:last-played:
	 *
	 * The song's last played timestamp.  User overriding should only be
	 * possible for advanced usage.
	 *
	 * The timestamp is in Unix time and is year-2038-safe.
	 *
	 * Since: 0.1
	 **/
	prop_id = WF_PROP_LAST_PLAYED;
	prop = g_param_spec_int64("last-played",
	                          "Last played",
	                          "The song's last played timestamp",
	                          0,
	                          G_MAXINT64,
	                          0,
	                          G_PARAM_READWRITE);
	WfProperties[prop_id] = prop;
	g_object_class_install_property(object_class, prop_id, prop);
}

/*
 * wf_song_instance_init:
 *
 * Gets invoked when the object's instance should initialize.  This should only
 * be done by GObject when it thinks it is time to do so.
 *
 * More information about the GObject structure can be found in the GObject
 * documentation or reference manual.
 *
 * Since: 0.1
 */
static void
wf_song_instance_init(GTypeInstance *instance, gpointer g_class)
{
	WfSong *song = WF_SONG(instance);

	g_return_if_fail(WF_IS_SONG(song));

	wf_song_add_priv_struct(song);
	wf_song_reset_stats(song);
	song->priv->status = WF_SONG_STATUS_UNKNOWN;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

// Set song @prev as the previous song relative to @song in the list
static void
wf_song_set_prev(WfSong *song, WfSong *prev)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->prev = prev;
}

// Set song @next as the next song relative to @song in the list
static void
wf_song_set_next(WfSong *song, WfSong *next)
{
	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->next = next;
}

// Allocate the private structure of a song object instance
static void
wf_song_add_priv_struct(WfSong *song)
{
	/*
	 * Because you cannot add any private structures to a #GObject without
	 * the use of the bloated macro's the library provides, allocate our
	 * private structure manually and add it to the object here.
	 */

	if (song != NULL && song->priv == NULL)
	{
		song->priv = g_slice_alloc0(sizeof(WfSongPrivate));
	}
}

/**
 * wf_song_get_count:
 *
 * Gets the amount of songs present in the song library.
 *
 * Returns: the amount of songs present in the library
 *
 * Since: 0.1
 **/
gint
wf_song_get_count(void)
{
	return SongCount;
}

/**
 * wf_song_get_first:
 *
 * Gets the first song present in the song library.
 *
 * Returns: (transfer none) (nullable): the first #WfSong
 *
 * Since: 0.1
 **/
WfSong *
wf_song_get_first(void)
{
	return FirstSong;
}

/**
 * wf_song_get_last:
 *
 * Gets the last song present in the song library.
 *
 * Returns: (transfer none) (nullable): the last #WfSong
 *
 * Since: 0.1
 **/
WfSong *
wf_song_get_last(void)
{
	return LastSong;
}

/**
 * wf_song_get_next:
 * @song: reference song
 *
 * Gets the next song in the song library, relative to @song.
 *
 * Returns: (transfer none) (nullable): the next #WfSong
 *
 * Since: 0.1
 **/
WfSong *
wf_song_get_next(const WfSong *song)
{
	return (song == NULL || song->priv == NULL) ? NULL : song->priv->next;
}

/**
 * wf_song_get_prev:
 * @song: reference song
 *
 * Gets the previous song in the song library, relative to @song.
 *
 * Returns: (transfer none) (nullable): the previous #WfSong
 *
 * Since: 0.1
 **/
WfSong *
wf_song_get_prev(const WfSong *song)
{
	return (song == NULL || song->priv == NULL) ? NULL : song->priv->next;
}

/**
 * wf_song_get_by_hash:
 * @hash: the hash to find its corresponding song for
 *
 * Get a song present in the song library by looking it up and matching by its
 * hash.  If no song with @hash is present, %NULL is returned.
 *
 * Returns: (transfer none) (nullable): the song matched by @hash
 *
 * Since: 0.1
 **/
WfSong *
wf_song_get_by_hash(guint32 hash)
{
	WfSong *song;

	for (song = FirstSong; song != NULL; song = song->priv->next)
	{
		if (song->priv->song_hash == hash)
		{
			return song;
		}
	}

	return NULL;
}

/**
 * wf_song_move_before:
 * @other_song: reference song for the new position
 *
 * Move @song before @other_song in the song library.
 *
 * Since: 0.1
 **/
void
wf_song_move_before(WfSong *song, WfSong *other_song)
{
	WfSong *next;
	WfSong *prev;

	g_return_if_fail(song != NULL);
	g_return_if_fail(other_song != NULL);

	// Get other songs
	prev = song->priv->prev;
	next = song->priv->next;

	// Link bordering songs together
	if (prev != NULL)
	{
		prev->priv->next = next;
	}
	if (next != NULL)
	{
		next->priv->prev = prev;
	}

	// Set song pointers
	song->priv->prev = other_song->priv->prev;
	song->priv->next = other_song;

	// Set right pointer in second reference song
	prev = other_song->priv->prev;
	if (prev != NULL)
	{
		prev->priv->next = song;
	}

	// Set reference song pointers
	other_song->priv->prev = song;

	// Set first & last list item if it changed
	if (FirstSong == other_song)
	{
		FirstSong = song;
	}
	if (LastSong == song)
	{
		LastSong = other_song;
	}
}

/**
 * wf_song_move_after:
 * @other_song: reference song for the new position
 *
 * Move @song after @other_song in the song library.
 *
 * Since: 0.1
 **/
void
wf_song_move_after(WfSong *song, WfSong *other_song)
{
	WfSong *next;
	WfSong *prev;

	g_return_if_fail(song != NULL);
	g_return_if_fail(other_song != NULL);

	// Get other songs
	prev = song->priv->prev;
	next = song->priv->next;

	// Link bordering songs together
	if (prev != NULL)
	{
		prev->priv->next = next;
	}
	if (next != NULL)
	{
		next->priv->prev = prev;
	}

	// Set song pointers
	song->priv->prev = other_song;
	song->priv->next = other_song->priv->next;

	// Set right pointer in second reference song
	next = other_song->priv->next;
	if (next != NULL)
	{
		next->priv->prev = song;
	}

	// Set reference song pointers
	other_song->priv->next = song;

	// Set first & last list item if it changed
	if (FirstSong == song)
	{
		FirstSong = other_song;
	}
	if (LastSong == other_song)
	{
		LastSong = song;
	}
}

/**
 * wf_song_get_hash:
 *
 * Gets the generated hash (or id) of a given song.
 *
 * Returns: the hash of a given song
 *
 * Since: 0.1
 **/
guint32
wf_song_get_hash(WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	if (song->priv->song_hash == 0)
	{
		song->priv->song_hash = wf_chars_get_hash(song->priv->uri);
	}

	return song->priv->song_hash;
}

/**
 * wf_song_get_tag:
 *
 * Gets the current tag of a given song.  The tag is usually a hexadecimal
 * representation of the hash, but it may be anything.  It is used e.g. to
 * uniquely identify groups in the library file.
 *
 * Returns: (transfer none): the tag of a given song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_tag(WfSong *song)
{
	guint32 hash;

	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	if (song->priv->song_hash == 0)
	{
		song->priv->song_hash = wf_chars_get_hash(song->priv->uri);
	}
	
	if (song->priv->tag == NULL)
	{
		hash = wf_song_get_hash(song);

		song->priv->tag = wf_song_new_tag(hash);
	}

	return song->priv->tag;
}

/*
 * wf_song_set_tag:
 * @tag: (transfer none): the new tag to set
 *
 * Sets the current tag of a given song.  The tag is usually hexadecimal
 * representation of the hash, but it may be anything.  It is used e.g. to
 * uniquely identify groups in the library file.
 *
 * Since: 0.1
 */
void
wf_song_set_tag(WfSong *song, const gchar *tag)
{
	gchar *str;

	g_return_if_fail(WF_IS_SONG(song));

	str = g_strdup(tag);

	wf_song_set_tag_take_str(song, str);
}

/*
 * wf_song_set_tag_take_str:
 * @tag: (transfer full): the new tag to set
 *
 * Sets the current tag of a given song, but unlike wf_song_set_tag() it does
 * not copy the string but take ownership.  This should only be done if the
 * string is already copied.
 *
 * Since: 0.1
 */
static void
wf_song_set_tag_take_str(WfSong *song, gchar *tag)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->tag);

	song->priv->tag = tag;
}

/*
 * wf_song_get_metadata_updated:
 *
 * Gets the timestamp of the last metadata update.  Metadata is only read when
 * the song's file is modified after the last metadata update.
 *
 * The timestamp is in Unix time and is year-2038-safe.
 *
 * Returns: timestamp of the last metadata update
 *
 * Since: 0.1
 */
gint64
wf_song_get_metadata_updated(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->updated;
}

/*
 * wf_song_set_metadata_updated:
 * @timestamp: Unix time representing when the metadata was updated
 *
 * Sets the timestamp of the last metadata update.  See
 * wf_song_get_metadata_updated().
 *
 * Since: 0.1
 */
void
wf_song_set_metadata_updated(WfSong *song, gint64 timestamp)
{
	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(timestamp >= 0);

	song->priv->updated = timestamp;
}

/*
 * wf_song_set_metadata_updated_now:
 *
 * Sets the timestamp of the last metadata update to the current wall-clock
 * time.  See wf_song_set_metadata_updated().
 *
 * Since: 0.1
 */
void
wf_song_set_metadata_updated_now(WfSong *song)
{
	gint64 time;

	g_return_if_fail(WF_IS_SONG(song));

	// Get timestamp (in seconds)
	time = wf_utils_time_now();

	if (time <= 1)
	{
		g_warning("Getting timestamp for time 'now' resulted in %ld", (long int) time);
	}
	else
	{
		song->priv->updated = time;
	}
}

/**
 * wf_song_get_file:
 *
 * Gets the associated #GFile for a given song.  See the #WfSong:file property.
 *
 * Returns: (transfer none): the #GFile of a song
 *
 * Since: 0.1
 **/
GFile *
wf_song_get_file(WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	// Create and sink #GFile if not present
	if (song->priv->file == NULL)
	{
		song->priv->file = g_file_new_for_uri(song->priv->uri);
	}

	return song->priv->file;
}

/*
 * wf_song_set_file:
 * @file: (transfer full): the #GFile to use
 *
 * Sets the associated #GFile of a given song.  It will take ownership of the
 * #GObject and set other song properties.
 *
 * Since: 0.1
 */
static void
wf_song_set_file(WfSong *song, GFile *file)
{
	gchar *uri;

	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(G_IS_FILE(file));

	// First clear the old location
	wf_song_clear_location(song);

	// Take ownership and (possibly) increase reference count
	song->priv->file = (GFile *) g_object_ref_sink(file);

	// Take a copy of the URI
	uri = g_file_get_uri(file);

	// Set required information
	wf_song_set_uri_internal(song, uri);

	// Free the original copy
	g_free(uri);
}

/**
 * wf_song_get_uri:
 *
 * Gets the URI for a given song.  See the #WfSong:uri property.
 *
 * Returns: (transfer none): the URI of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_uri(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->uri;
}

/**
 * wf_song_set_uri:
 * @uri: (transfer none): the uri to use
 *
 * Sets the URI for a given song.  See the #WfSong:uri property.
 *
 * Since: 0.1
 **/
static void
wf_song_set_uri(WfSong *song, const gchar *uri)
{
	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(uri != NULL);

	// First clear the old location
	wf_song_clear_location(song);

	// Set properties
	wf_song_set_uri_internal(song, uri);
}

static void
wf_song_set_uri_internal(WfSong *song, const gchar *uri)
{
	g_return_if_fail(song != NULL);
	g_return_if_fail(uri != NULL);

	// Unescape special characters to UTF-8
	song->priv->uri = g_uri_unescape_string(uri, NULL /* illegal_characters */);

	// Allocate and set new values
	song->priv->name = g_path_get_basename(song->priv->uri);
	song->priv->song_hash = wf_chars_get_hash(song->priv->uri);
}

/**
 * wf_song_get_name:
 *
 * Gets the basename for a given song.  See the #WfSong:name property.
 *
 * Returns: (transfer none) (nullable): the basename of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_name(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->name;
}

/**
 * wf_song_get_name_not_empty:
 *
 * Gets the basename for a given song, one that is guaranteed to never be empty.
 * This is useful for printing song names to stdout without checking for %NULL.
 *
 * Returns: (transfer none): the basename of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_name_not_empty(const WfSong *song)
{
	const gchar *unknown_str = "(unknown)";
	const gchar *invalid_str = "(invalid)";

	g_return_val_if_fail(WF_IS_SONG(song), unknown_str);
	g_return_val_if_fail(wf_song_is_valid(song), invalid_str);

	return (song->priv->name != NULL) ? song->priv->name : unknown_str;
}

/*
 * wf_song_set_display_name:
 * @name: (transfer none): the new display name to use
 *
 * Sets the display name for a given song.  See the #WfSong:display-name
 * property.
 *
 * Since: 0.1
 */
static void
wf_song_set_display_name(WfSong *song, const gchar *name)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->display_name);

	song->priv->display_name = g_strdup(name);
}

/**
 * wf_song_get_display_name:
 *
 * Gets the display name for a given song.  See the #WfSong:display-name
 * property.
 *
 * Returns: (transfer none) (nullable): the basename of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_display_name(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->display_name;
}

/*
 * wf_song_set_modified:
 * @timestamp: the file modification timestamp of a song
 *
 * Sets the file modification timestamp for a given song.  See the
 * #WfSong:modified property.
 *
 * Since: 0.1
 */
static void
wf_song_set_modified(WfSong *song, gint64 timestamp)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->fs_modified = timestamp;
}

/**
 * wf_song_get_modified:
 *
 * Gets the file modification timestamp for a given song.  See the
 * #WfSong:modified property.
 *
 * Returns: the song's file modification timestamp
 *
 * Since: 0.1
 **/
gint64
wf_song_get_modified(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->fs_modified;
}

/*
 * wf_song_set_title:
 * @title: (transfer none): the title to set
 *
 * Sets the title for a given song.  See the #WfSong:title property.
 *
 * Since: 0.1
 */
void
wf_song_set_title(WfSong *song, const gchar *title)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->title);

	song->priv->title = g_strdup(title);
}

/**
 * wf_song_get_title:
 *
 * Gets the title for a given song.  See the #WfSong:title property.
 *
 * Returns: (transfer none) (nullable): the title of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_title(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->title;
}

/*
 * wf_song_set_artist:
 * @artist: (transfer none): the artist to set
 *
 * Sets the artist for a given song.  See the #WfSong:artist property.
 *
 * Since: 0.1
 */
void
wf_song_set_artist(WfSong *song, const gchar *artist)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->artist);

	song->priv->artist = g_strdup(artist);
	song->priv->artist_hash = wf_chars_get_hash_converted(artist);
}

/**
 * wf_song_get_artist:
 *
 * Gets the artist for a given song.  See the #WfSong:artist property.
 *
 * Returns: (transfer none) (nullable): the artist of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_artist(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->artist;
}

/*
 * wf_song_set_album_artist:
 * @artist: (transfer none): the album artist to set
 *
 * Sets the album artist for a given song.  See the #WfSong:album-artist
 * property.
 *
 * Since: 0.1
 */
void
wf_song_set_album_artist(WfSong *song, const gchar *artist)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->album_artist);

	song->priv->album_artist = g_strdup(artist);
	song->priv->album_artist_hash = wf_chars_get_hash_converted(artist);
}

/**
 * wf_song_get_album_artist:
 *
 * Gets the album artist for a given song.  See the #WfSong:album-artist
 * property.
 *
 * Returns: (transfer none) (nullable): the album artist of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_album_artist(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->album_artist;
}

/**
 * wf_song_get_artist_hash:
 *
 * Gets the generated artist hash.  Generally, the #WfSong:album-artist
 * property is used to generate the hash, but if it is not present,
 * #WfSong:artist is used.  These hashes are primarily used to find songs with
 * similar artists.
 *
 * Returns: the hash of the artist
 *
 * Since: 0.1
 **/
guint32
wf_song_get_artist_hash(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	if (song->priv->album_artist_hash != 0)
	{
		return song->priv->album_artist_hash;
	}
	else
	{
		return song->priv->artist_hash;
	}
}

/*
 * wf_song_set_album:
 * @album: (transfer none): the album to set
 *
 * Sets the album for a given song.  See the #WfSong:album property.
 *
 * Since: 0.1
 */
void
wf_song_set_album(WfSong *song, const gchar *album)
{
	g_return_if_fail(WF_IS_SONG(song));

	g_free(song->priv->album);

	song->priv->album = g_strdup(album);
}

/**
 * wf_song_get_album:
 *
 * Gets the album for a given song.  See the #WfSong:album property.
 *
 * Returns: (transfer none) (nullable): the album of a song
 *
 * Since: 0.1
 **/
const gchar *
wf_song_get_album(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	return song->priv->album;
}

/*
 * wf_song_set_track_number:
 * @number: the track number to set
 *
 * Sets the track number for a given song.  See the #WfSong:number property.
 *
 * Since: 0.1
 */
void
wf_song_set_track_number(WfSong *song, gint number)
{
	g_return_if_fail(WF_IS_SONG(song));

	if (number >= 0)
	{
		song->priv->number = number;
	}
}

/**
 * wf_song_get_track_number:
 *
 * Gets the track number for a given song.  See the #WfSong:number property.
 *
 * Returns: the track number of a song
 *
 * Since: 0.1
 **/
gint
wf_song_get_track_number(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->number;
}

/*
 * wf_song_set_duration_seconds:
 * @seconds: the duration in seconds
 *
 * Sets the duration for a given song by supplying a number of seconds.  See the
 * #WfSong:duration property.
 *
 * Since: 0.1
 */
void
wf_song_set_duration_seconds(WfSong *song, gint seconds)
{
	g_return_if_fail(WF_IS_SONG(song));

	if (seconds >= 0)
	{
		song->priv->duration = seconds;
	}
}

/*
 * wf_song_set_duration_nanoseconds:
 * @nanoseconds: the duration in nanoseconds
 *
 * Sets the duration for a given song by supplying a number of nanoseconds.
 * See the #WfSong:duration property.
 *
 * Since: 0.1
 */
void
wf_song_set_duration_nanoseconds(WfSong *song, gint64 nanoseconds)
{
	gint64 seconds;

	g_return_if_fail(WF_IS_SONG(song));

	seconds = nanoseconds / 1000 / 1000 / 1000;

	wf_song_set_duration_seconds(song, seconds); // Conversion to int
}

/**
 * wf_song_get_duration:
 *
 * Gets the duration for a given song.  See the #WfSong:duration property.
 *
 * Returns: the duration of a song in seconds
 *
 * Since: 0.1
 **/
gint
wf_song_get_duration(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->duration;
}

/**
 * wf_song_get_duration_string:
 *
 * Convenient method to get the duration for a given song as a string in the
 * format "%H:%M:%S" (with %H as the amount of hours, %M as the amount of
 * minutes and %S the amount of seconds).  This string can be used e.g. to
 * print the duration to stdout or show it in a graphical interface.  See the
 * #WfSong:duration property.
 *
 * Returns: (transfer full) (nullable): the duration of a song as a string.
 * Free it with g_free() when no longer needed.
 *
 * Since: 0.1
 **/
gchar *
wf_song_get_duration_string(const WfSong *song)
{
	const gint one_hour = 60 * 60, one_min = 60;
	gint dur, secs = 0, mins = 0, hours = 0;
	gchar *str;

	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	dur = song->priv->duration;

	if (dur <= 1)
	{
		return NULL;
	}
	else
	{
		if (dur < one_hour)
		{
			mins = dur / one_min; // Rounded
			secs = dur - (mins * one_min);

			str = g_strdup_printf("%d:%2d", mins, secs);
		}
		else
		{
			hours = dur / one_hour; // Rounded
			mins = (dur - (hours * one_hour)) / one_min; // Rounded
			secs = dur - (hours * one_hour) - (mins * one_min);

			str = g_strdup_printf("%d:%2d:%2d", hours, mins, secs);
		}
	}

	return str;
}

/**
 * wf_song_is_rating_unset:
 *
 * Convenient method to check whether the rating of a song is not set.
 *
 * Returns: %TRUE if the rating is unset, %FALSE if it is
 *
 * Since: 0.1
 **/
gboolean
wf_song_is_rating_unset(const WfSong *song)
{
	return (song == NULL || song->priv == NULL || song->priv->rating == 0);
}

/**
 * wf_song_set_rating:
 * @rating: the new rating
 *
 * Sets a new rating for a given song.  See the #WfSong:rating property.
 *
 * Since: 0.1
 **/
void
wf_song_set_rating(WfSong *song, gint rating)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->rating = rating;
}

/**
 * wf_song_get_rating:
 *
 * Gets the current rating of a given song.  See the #WfSong:rating property.
 *
 * Returns: the rating of a song
 *
 * Since: 0.1
 **/
gint
wf_song_get_rating(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->rating;
}

/**
 * wf_song_set_score:
 * @score: the new score
 *
 * Sets a new score for a given song.  See the #WfSong:score property.
 *
 * Since: 0.1
 **/
void
wf_song_set_score(WfSong *song, gdouble score)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->score = score;
}

/**
 * wf_song_get_score:
 *
 * Gets the current score of a given song.  See the #WfSong:score property.
 *
 * Returns: the score of a song
 *
 * Since: 0.1
 **/
gdouble
wf_song_get_score(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0.0);

	return song->priv->score;
}

/**
 * wf_song_set_play_count:
 * @playcount: the new play count
 *
 * Sets a new play count for a given song.  See the #WfSong:play-count property.
 *
 * Since: 0.1
 **/
void
wf_song_set_play_count(WfSong *song, gint playcount)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->playcount = playcount;
}

/**
 * wf_song_get_play_count:
 *
 * Gets the current play count of a given song.  See the #WfSong:play-count
 * property.
 *
 * Returns: the play count of a song
 *
 * Since: 0.1
 **/
gint
wf_song_get_play_count(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->playcount;
}

/**
 * wf_song_set_skip_count:
 * @skipcount: the new skip count
 *
 * Sets a new skip count for a given song.  See the #WfSong:skip-count property.
 *
 * Since: 0.1
 **/
void
wf_song_set_skip_count(WfSong *song, gint skipcount)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->skipcount = skipcount;
}

/**
 * wf_song_get_skip_count:
 *
 * Gets the current skip count of a given song.  See the #WfSong:skip-count
 * property.
 *
 * Returns: the skip count of a song
 *
 * Since: 0.1
 **/
gint
wf_song_get_skip_count(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->skipcount;
}

/**
 * wf_song_set_last_played:
 * @lastplayed: the new last played timestamp
 *
 * Sets a new last played timestamp for a given song.  See the
 * #WfSong:last-played property.
 *
 * Since: 0.1
 **/
void
wf_song_set_last_played(WfSong *song, gint64 lastplayed)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->lastplayed = lastplayed;
}

/**
 * wf_song_get_last_played:
 *
 * Gets the current last played timestamp of a given song.  See the
 * #WfSong:last-played property.
 *
 * Returns: the last played timestamp of a song
 *
 * Since: 0.1
 **/
gint64
wf_song_get_last_played(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), 0);

	return song->priv->lastplayed;
}

/**
 * wf_song_get_played_on_as_string:
 *
 * Convenient method to get the last played timestamp represented as the time
 * since the last play and stored as a string.  See the #WfSong:last-played
 * property.
 *
 * Returns: (transfer full): the time since last play as a string.  Free it
 * with g_free() when no longer needed.
 *
 * Since: 0.1
 **/
gchar *
wf_song_get_played_on_as_string(const WfSong *song)
{
	const gboolean include_time = TRUE;
	gint64 last_played;

	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	last_played = wf_song_get_last_played(song);

	return wf_song_last_played_to_played_on_string(last_played, include_time);
}

/**
 * wf_song_get_played_on_as_string:
 *
 * Convenient method to get the last played timestamp as a string.  See the
 * #WfSong:last-played property.
 *
 * Returns: (transfer full): the last played timestamp as a string.  Free it
 * with g_free() when no longer needed.
 *
 * Since: 0.1
 **/
gchar *
wf_song_get_last_played_as_string(const WfSong *song)
{
	gint64 last_played;

	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	last_played = wf_song_get_last_played(song);

	return wf_song_last_played_to_string(last_played);
}

/**
 * wf_song_is_in_list:
 *
 * Gets the value of the in list flag.  See the #WfSong:in-list property.
 *
 * Returns: %TRUE if the song is present in the library, %FALSE otherwise
 *
 * Since: 0.1
 **/
gboolean
wf_song_is_in_list(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), FALSE);

	return song->priv->in_list;
}

/*
 * wf_song_set_queued:
 * @value: %TRUE if the song is queued
 *
 * Sets the queue status of a given song.  See the #WfSong:queued property.
 *
 * Since: 0.1
 */
void
wf_song_set_queued(WfSong *song, gboolean value)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->queued = value;
}

/**
 * wf_song_get_queued:
 *
 * Gets the queue status of a given song.  See the #WfSong:queued property.
 *
 * Returns: %TRUE if the song is queued, %FALSE otherwise
 *
 * Since: 0.1
 **/
gboolean
wf_song_get_queued(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), FALSE);

	return song->priv->queued;
}

/*
 * wf_song_set_stop_flag:
 * @value: %TRUE if the playback should stop after this song
 *
 * Sets the status of the stop flag of a given song.  See the #WfSong:stop
 * property.
 *
 * Since: 0.1
 */
void
wf_song_set_stop_flag(WfSong *song, gboolean value)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->stop_after_playing = value;
}

/**
 * wf_song_get_queued:
 *
 * Gets the status of the stop flag of a given song.  See the #WfSong:stop
 * property.
 *
 * Returns: %TRUE if the playback should stop after this song, %FALSE
 * otherwise
 *
 * Since: 0.1
 **/
gboolean
wf_song_get_stop_flag(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), FALSE);

	return song->priv->stop_after_playing;
}

/*
 * wf_song_set_status:
 * @state: new status to use
 *
 * Sets the new status of a given song.  See the #WfSong:status property.
 *
 * Since: 0.1
 */
void
wf_song_set_status(WfSong *song, WfSongStatus state)
{
	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(wf_song_is_valid_status(state));

	song->priv->status = state;
}

/**
 * wf_song_get_status:
 *
 * Gets the current status of a given song.  See the #WfSong:status property.
 *
 * Returns: the current status of a song
 *
 * Since: 0.1
 **/
WfSongStatus
wf_song_get_status(const WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), WF_SONG_STATUS_UNKNOWN);

	return song->priv->status;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

/**
 * wf_song_is_valid:
 *
 * Checks if a given song is valid.  A song is valid if it exists and has an URI
 * set.  The URI does not have to be valid, nor does the song have to be present
 * in the song library.
 *
 * Returns: %TRUE if the song is valid, %FALSE if it is not
 *
 * Since: 0.1
 **/
gboolean
wf_song_is_valid(const WfSong *song)
{
	if (!WF_IS_SONG(song) ||
	    song->priv == NULL ||
	    song->priv->uri == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

/*
 * wf_song_is_unique:
 *
 * Checks if a given song is not already present in the song library.
 *
 * Returns: %TRUE if the song is unique, %FALSE if the song is already present
 * in the song library
 *
 * Since: 0.1
 */
gboolean
wf_song_is_unique(WfSong *song)
{
	WfSong *item;

	g_return_val_if_fail(WF_IS_SONG(song), FALSE);

	for (item = FirstSong; item != NULL; item = song->priv->next)
	{
		if (song == item) // Compare pointer
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*
 * wf_song_is_unique_uri:
 * @uri: (transfer none): the URI to check
 *
 * Checks if no song present in the song library already has the given URI.
 *
 * Returns: %TRUE if the URI is unique, %FALSE otherwise
 *
 * Since: 0.1
 */
gboolean
wf_song_is_unique_uri(const gchar *uri)
{
	WfSong *item;
	gchar *utf8;
	guint32 song_hash;
	guint32 hash;

	g_return_val_if_fail(uri != NULL, FALSE);

	utf8 = g_uri_unescape_string(uri, NULL /* illegal_characters */);
	hash = wf_chars_get_hash(utf8);
	g_free(utf8);

	for (item = FirstSong; item != NULL; item = item->priv->next)
	{
		song_hash = wf_song_get_hash(item);

		if (hash == song_hash)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*
 * wf_song_get_type:
 *
 * Get the matching #GType for #WfSong.  On first run it will register the type
 * with GLib's type system.
 *
 * Returns: the #GType for #WfSong
 *
 * Since: 0.1
 */
GType
wf_song_get_type(void)
{
	const gchar *name;
	GType type;

	// Initialize the new #WfSong type if not registered
	if (g_once_init_enter(&WfSongType))
	{
		// This will remember the string pointer for easy comparison
		name = g_intern_static_string(WF_SONG_TYPE_NAME);

		// Register the type (should only run once per application instance)
		type = g_type_register_static(WF_SONG_TYPE_PARENT, name, &WfSongTypeInfo, WF_SONG_TYPE_FLAGS);

		// Remember registered type
		g_once_init_leave(&WfSongType, type);
	}

	return WfSongType;
}

/*
 * wf_song_get_enum_status_type:
 *
 * Get the matching #GType for the #WfSongStatus enum.  On first run it will
 * register the type with GLib's type system.
 *
 * Returns: the #GType for #WfSongStatus
 *
 * Since: 0.1
 */
GType
wf_song_get_enum_status_type(void)
{
	const gchar *name;
	GType type;

	// Initialize the new #WfSong type if not registered
	if (g_once_init_enter(&WfSongStatusType))
	{
		static const GEnumValue enum_values[] =
		{
			{ WF_SONG_STATUS_UNKNOWN, "WF_SONG_STATUS_UNKNOWN", "status-unknown" },
			{ WF_SONG_AVAILABLE, "WF_SONG_AVAILABLE", "available" },
			{ WF_SONG_PLAYING, "WF_SONG_PLAYING", "playing" },
			{ WF_SONG_NOT_FOUND, "WF_SONG_NOT_FOUND", "not-found" },
			{ 0, NULL, NULL }
		};

		// This will remember the string pointer for easy comparison
		name = g_intern_static_string(WF_SONG_STATUS_TYPE_NAME);

		// Register the type (should only run once per application instance)
		type = g_enum_register_static(name, enum_values);

		// Remember registered type
		g_once_init_leave(&WfSongStatusType, type);
	}

	return WfSongStatusType;
}

// This gets invoked when a caller wants to get a property (the value needs to be set in @value)
static void
wf_song_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	WfSong *song = WF_SONG(object);
	WfProp property = property_id;

	g_return_if_fail(WF_IS_SONG(song));

	switch (property)
	{
		case WF_PROP_STATUS:
			g_value_set_enum(value, wf_song_get_status(song));
			break;
		case WF_PROP_FILE:
			g_value_set_object(value, wf_song_get_file(song));
			break;
		case WF_PROP_URI:
			g_value_set_string(value, wf_song_get_uri(song));
			break;
		case WF_PROP_NAME:
			g_value_set_string(value, wf_song_get_name(song));
			break;
		case WF_PROP_DISPLAY_NAME:
			g_value_set_string(value, wf_song_get_display_name(song));
			break;
		case WF_PROP_MODIFIED:
			g_value_set_boolean(value, wf_song_get_modified(song));
			break;
		case WF_PROP_QUEUED:
			g_value_set_boolean(value, wf_song_get_queued(song));
			break;
		case WF_PROP_STOP:
			g_value_set_boolean(value, wf_song_get_stop_flag(song));
			break;
		case WF_PROP_TITLE:
			g_value_set_string(value, wf_song_get_title(song));
			break;
		case WF_PROP_ARTIST:
			g_value_set_string(value, wf_song_get_artist(song));
			break;
		case WF_PROP_ALBUM_ARTIST:
			g_value_set_string(value, wf_song_get_album_artist(song));
			break;
		case WF_PROP_ALBUM:
			g_value_set_string(value, wf_song_get_album(song));
			break;
		case WF_PROP_NUMBER:
			g_value_set_int(value, wf_song_get_track_number(song));
			break;
		case WF_PROP_DURATION:
			g_value_set_int(value, wf_song_get_duration(song));
			break;
		case WF_PROP_RATING:
			g_value_set_int(value, wf_song_get_rating(song));
			break;
		case WF_PROP_SCORE:
			g_value_set_double(value, wf_song_get_score(song));
			break;
		case WF_PROP_PLAY_COUNT:
			g_value_set_int(value, wf_song_get_play_count(song));
			break;
		case WF_PROP_SKIP_COUNT:
			g_value_set_int(value, wf_song_get_skip_count(song));
			break;
		case WF_PROP_LAST_PLAYED:
			g_value_set_int64(value, wf_song_get_last_played(song));
			break;
		default:
			// Unknown property
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

// This gets invoked when a caller wants to set a property (the value in @value needs to be set by the caller)
static void
wf_song_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	WfSong *song = WF_SONG(object);
	WfProp property = property_id;

	g_return_if_fail(WF_IS_SONG(song));

	switch (property)
	{
		case WF_PROP_QUEUED:
			wf_song_set_queued(song, g_value_get_boolean(value));
			break;
		case WF_PROP_STOP:
			wf_song_set_stop_flag(song, g_value_get_boolean(value));
			break;
		case WF_PROP_RATING:
			wf_song_set_rating(song, g_value_get_int(value));
			break;
		case WF_PROP_SCORE:
			wf_song_set_score(song, g_value_get_double(value));
			break;
		case WF_PROP_PLAY_COUNT:
			wf_song_set_play_count(song, g_value_get_int(value));
			break;
		case WF_PROP_SKIP_COUNT:
			wf_song_set_skip_count(song, g_value_get_int(value));
			break;
		case WF_PROP_LAST_PLAYED:
			wf_song_set_last_played(song, g_value_get_int64(value));
			break;
		default:
			// Unknown property
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

/*
 * wf_song_reset_stats:
 *
 * Reset all statistics to their initial values.
 *
 * Since: 0.1
 */
void
wf_song_reset_stats(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	song->priv->rating = WF_SONG_INITIAL_RATING;
	song->priv->score = WF_SONG_INITIAL_SCORE;
	song->priv->playcount = WF_SONG_INITIAL_PLAYCOUNT;
	song->priv->skipcount = WF_SONG_INITIAL_SKIPCOUNT;
	song->priv->lastplayed = WF_SONG_INITIAL_LASTPLAYED;
	song->priv->stop_after_playing = FALSE;
}

/*
 * wf_song_clear_location:
 *
 * Reset and clear the properties that gives a song its unique location.
 *
 * Since: 0.1
 */
static void
wf_song_clear_location(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Free old values if set
	wf_memory_clear_object((GObject **) &song->priv->file);
	g_free(song->priv->uri);
	g_free(song->priv->name);
	g_free(song->priv->display_name);
	g_free(song->priv->tag);
}

/*
 * wf_song_prepend_song:
 * @song: (transfer full): the song to prepend
 *
 * Add a new song to the start of the library (prepend).
 *
 * Returns: (transfer none): @song with a sunken reference
 *
 * Since: 0.1
 */
static WfSong *
wf_song_prepend_song(WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	// Change prev/next pointers
	if (FirstSong != NULL)
	{
		song->priv->next = FirstSong;
		wf_song_set_prev(FirstSong, song);
	}

	// If no songs were present, one is now
	if (LastSong == NULL)
	{
		LastSong = song;
	}

	FirstSong = song;
	SongCount++;

	// Song is now present in the library
	wf_song_ref_sink(song);
	song->priv->in_list = TRUE;

	return song;
}

/*
 * wf_song_append_song:
 * @song: (transfer full): the song to append
 *
 * Add a new song to the end of the library (append).
 *
 * Returns: (transfer none): @song with a sunken reference
 *
 * Since: 0.1
 */
static WfSong *
wf_song_append_song(WfSong *song)
{
	g_return_val_if_fail(WF_IS_SONG(song), NULL);

	// Change prev/next pointers
	if (LastSong != NULL)
	{
		song->priv->prev = LastSong;
		wf_song_set_next(LastSong, song);
	}

	// If no songs were present, one is now
	if (FirstSong == NULL)
	{
		FirstSong = song;
	}

	LastSong = song;
	SongCount++;

	// Song is now present in the library
	wf_song_ref_sink(song);
	song->priv->in_list = TRUE;

	return song;
}

/**
 * wf_song_prepend_song:
 * @uri (transfer none): the URI to associate the new song with
 *
 * Validate and add a new song to the start of the library (prepend) by
 * providing its URI.  Note that the song will be added regardless of whether
 * the actual song exists and is playable.
 *
 * Returns: (transfer none): the newly added song
 *
 * Since: 0.1
 **/
WfSong *
wf_song_prepend_by_uri(const gchar *uri)
{
	WfSong *song;

	g_return_val_if_fail(uri != NULL, NULL);

	// Create initial object and add URI
	song = wf_song_new();
	wf_song_set_uri(song, uri);
	wf_song_prepend_song(song);

	return song;
}

/**
 * wf_song_prepend_song:
 * @file: (transfer full): the #GFile to associate the new song with
 *
 * Validate and add a new song to the start of the library (prepend) by
 * providing a #GFile.  @file will not be copied, a reference will be added to
 * it instead.
 *
 * Returns: (transfer none): the newly added song
 *
 * Since: 0.1
 **/
WfSong *
wf_song_prepend_by_file(GFile *file)
{
	WfSong *song;

	g_return_val_if_fail(G_IS_FILE(file), NULL);

	song = wf_song_new();
	wf_song_set_file(song, file);
	wf_song_prepend_song(song);

	return song;
}

/**
 * wf_song_append_song:
 * @uri (transfer none): the URI to associate the new song with
 *
 * Validate and add a new song to the end of the library (append) by providing
 * its URI.  Note that the song will be added regardless of whether the actual
 * song exists and is playable.
 *
 * Returns: (transfer none): the newly added song
 *
 * Since: 0.1
 **/
WfSong *
wf_song_append_by_uri(const gchar *uri)
{
	WfSong *song;

	g_return_val_if_fail(uri != NULL, NULL);

	song = wf_song_new();
	wf_song_set_uri(song, uri);
	wf_song_append_song(song);

	return song;
}

/**
 * wf_song_append_song:
 * @file: (transfer full): the #GFile to associate the new song with
 *
 * Validate and add a new song to the end of the library (append) by providing
 * a #GFile.  @file will not be copied, a reference will be added to it
 * instead.
 *
 * Returns: (transfer none): the newly added song
 *
 * Since: 0.1
 **/
WfSong *
wf_song_append_by_file(GFile *file)
{
	WfSong *song;

	g_return_val_if_fail(G_IS_FILE(file), NULL);

	song = wf_song_new();
	wf_song_set_file(song, file);
	wf_song_append_song(song);

	return song;
}

/**
 * wf_song_remove:
 *
 * Remove a song from the library.  This will also free the object if no other
 * references to it exist.
 *
 * Since: 0.1
 **/
void
wf_song_remove(WfSong *song)
{
	WfSong *prev;
	WfSong *next;

	if (song == NULL)
	{
		return;
	}

	prev = song->priv->prev;
	next = song->priv->next;

	if (prev != NULL)
	{
		prev->priv->next = next;
	}

	if (next != NULL)
	{
		next->priv->prev = prev;
	}

	if (FirstSong == song)
	{
		FirstSong = next;
	}

	if (LastSong == song)
	{
		LastSong = prev;
	}

	if (song->priv->in_list)
	{
		// Song is now out of the library
		song->priv->in_list = FALSE;
		song->priv->prev = NULL;
		song->priv->next = NULL;

		// Drop reference (may or may not free the object)
		g_object_unref(song);

		SongCount--;
	}
}

/*
 * wf_song_remove_all:
 *
 * Remove all known songs, essentially clearing the library.
 *
 * Since: 0.1
 */
void
wf_song_remove_all(void)
{
	WfSong *song = FirstSong;
	WfSong *next;

	while (song != NULL)
	{
		next = song->priv->next;

		song->priv->in_list = FALSE;
		song->priv->prev = NULL;
		song->priv->next = NULL;

		g_object_unref(song);

		song = next;
	}

	FirstSong = LastSong = NULL;
	SongCount = 0;
}

// Update a song's metadata by providing a set of new values
static void
wf_song_set_new_metadata(WfSong *song, WfSongMetadata *metadata)
{
	guint uintValue;
	guint64 uint64Value;
	gchar *str;

	g_return_if_fail(metadata != NULL);
	g_return_if_fail(song != NULL);
	g_return_if_fail(WF_IS_SONG(song));

	uintValue = wf_song_metadata_get_track_number(metadata);
	wf_song_set_track_number(song, uintValue);

	uint64Value = wf_song_metadata_get_duration(metadata);
	wf_song_set_duration_nanoseconds(song, uint64Value);

	str = wf_song_metadata_get_title(metadata);
	wf_song_set_title(song, str);

	str = wf_song_metadata_get_artist(metadata);
	wf_song_set_artist(song, str);

	str = wf_song_metadata_get_album_artist(metadata);
	wf_song_set_album_artist(song, str);

	str = wf_song_metadata_get_album(metadata);
	wf_song_set_album(song, str);

	wf_song_set_metadata_updated_now(song);
}

// Update a song's filesystem info by providing a #GFileInfo
void
wf_song_set_fs_info(WfSong *song, GFileInfo *info)
{
	const gchar *name;
	gint64 seconds;
	GDateTime *time;

	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(G_IS_FILE_INFO(info));

	// Get modification time
	time = wf_tweaks_file_info_get_modification_date_time(info); // Compatibility call
	seconds = g_date_time_to_unix(time);
	wf_song_set_modified(song, seconds);
	g_date_time_unref(time);

	// Get display filename
	name = g_file_info_get_display_name(info);
	wf_song_set_display_name(song, name);

	// Query successful, so song is available
	wf_song_set_status(song, WF_SONG_AVAILABLE);
}

// Update the filesystem info of a song's file
static void
wf_song_update_fs_info(WfSong *song)
{
	GFile *file;
	GFileInfo *info;
	GError *err = NULL;

	g_return_if_fail(WF_IS_SONG(song));

	file = wf_song_get_file(song);
	info = g_file_query_info(file, WF_SONG_FILE_INFO, G_FILE_QUERY_INFO_NONE, NULL /* GCancellable */, &err);

	if (err != NULL)
	{
		// Failed to get the file information
		g_message("%s", err->message);

		if (err->code == G_IO_ERROR_NOT_FOUND)
		{
			wf_song_set_status(song, WF_SONG_NOT_FOUND);
			wf_song_set_modified(song, -1);
		}

		g_error_free(err);
	}
	else
	{
		wf_song_set_fs_info(song, info);
	}

	wf_memory_clear_object((GObject **) &info);
}

// Returns %TRUE if an attempt has been made to get the metadata from the file
gboolean
wf_song_update_metadata(WfSong *song, gboolean force)
{
	const gchar *uri;
	WfSongMetadata *metadata;
	gboolean update = FALSE;
	gint64 modified = 0, last_updated = 0;

	g_return_val_if_fail(WF_IS_SONG(song), FALSE);

	uri = wf_song_get_uri(song);

	wf_song_update_fs_info(song);

	if (force)
	{
		// Update regardless
		update = TRUE;
	}
	else
	{
		last_updated = wf_song_get_metadata_updated(song);
		modified = wf_song_get_modified(song);

		/*
		 * Update if the file exists but the file information could not
		 * be fetched or if the file has been modified since last
		 * application metadata update.
		 */
		if (modified == -1)
		{
			update = FALSE;
		}
		else if (modified == 0 || last_updated <= modified)
		{
			update = TRUE;
		}
		else
		{
			update = FALSE;
		}
	}

	if (update)
	{
		metadata = wf_song_metadata_get_for_uri(uri);

		if (metadata == NULL)
		{
			return FALSE;
		}
		else if (!wf_song_metadata_parse(metadata))
		{
			wf_song_metadata_free(metadata);

			return FALSE;
		}
		else
		{
			wf_song_set_new_metadata(song, metadata);
			wf_song_metadata_free(metadata);

			return TRUE;
		}
	}
	else
	{
		// No need to update metadata, already up-to-date
		return FALSE;
	}

}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

// Remove floating reference, but do not add a reference
static WfSong *
wf_song_ref_sink(WfSong *song)
{
	/*
	 * After some experimenting, g_object_ref_sink seems to keep the
	 * reference count the same if it is floating, but adds one if it is
	 * sunken.  This means that any new GObject will start with a reference
	 * count of 2 when it is not floating.  To prevent the object from never
	 * getting freed, only sink the reference after creation, not adding a
	 * new reference.
	 */
	if (g_object_is_floating(song))
	{
		// Will sink reference but not add one
		return (WfSong *) g_object_ref_sink(song);
	}
	else
	{
		return song;
	}
}

// Check if a enum state value is valid
static gboolean
wf_song_is_valid_status(WfSongStatus state)
{
	return (state >= 0 && state < WF_SONG_DEFINED);
}

// Get a new tag for a given hash
static gchar *
wf_song_new_tag(guint32 hash)
{
	gchar *hex;

	g_return_val_if_fail(hash > 0, NULL);

	// Convert the sum to a hexadecimal character array
	hex = g_strdup_printf("song-%x", hash);

	return hex;
}

static gchar *
wf_song_last_played_to_string(gint64 last_played)
{
	const gint64 now = wf_utils_time_now();
	const gint minute = 60;
	const gint hour = minute * 60;
	const gint day = hour * 24;
	const gint week = day * 7;
	const gint month = day * 30;
	const gint year = day * 365;
	gchar *string;
	gint64 time_since;
	gint x;

	if (last_played < year)
	{
		/*
		 * The statement above means: if the song has been played in the
		 * first year before the UNIX Epoch, say it was never played.
		 */

		return g_strdup_printf("Never");
	}
	else
	{
		time_since = (now - last_played);
	}

	if (time_since < 5)
	{
		// Within 5 seconds, say it was played now

		string = g_strdup_printf("Just now");
	}
	else if (time_since < minute)
	{
		// Within a minute, set the amount of seconds

		if (time_since == 1)
		{
			string = g_strdup_printf("1 second ago");
		}
		else
		{
			string = g_strdup_printf("%ld seconds ago", (long int) time_since);
		}
	}
	else if (time_since < hour)
	{
		// Within one our, set the amount of minutes

		x = wf_utils_floor(time_since / minute);

		if (x == 1)
		{
			string = g_strdup_printf("1 minute ago");
		}
		else
		{
			string = g_strdup_printf("%d minutes ago", x);
		}
	}
	else if (time_since < day)
	{
		// Within one full day, set the amount of hours

		x = wf_utils_floor(time_since / hour);

		if (x == 1)
		{
			string = g_strdup_printf("1 hour ago");
		}
		else
		{
			string = g_strdup_printf("%d hours ago", x);
		}
	}
	else if (time_since < week)
	{
		// Within one full week, set the amount of days

		x = wf_utils_floor(time_since / day);

		if (x == 1)
		{
			string = g_strdup_printf("1 day ago");
		}
		else
		{
			string = g_strdup_printf("%d days ago", x);
		}
	}
	else if (time_since < month)
	{
		// Within one full month, set the amount of weeks

		x = wf_utils_floor(time_since / week);

		if (x == 1)
		{
			string = g_strdup_printf("1 week ago");
		}
		else
		{
			string = g_strdup_printf("%d weeks ago", x);
		}
	}
	else if (time_since < year)
	{
		// Within one full year, set the amount of months

		x = wf_utils_floor(time_since / month);

		if (x == 1)
		{
			string = g_strdup_printf("1 month ago");
		}
		else
		{
			string = g_strdup_printf("%d months ago", x);
		}
	}
	else
	{
		// It's been a long time, only mention years

		x = wf_utils_floor(time_since / year);

		if (x == 1)
		{
			string = g_strdup_printf("1 year ago");
		}
		else
		{
			string = g_strdup_printf("%d years ago", x);
		}
	}

	return string;
}

static gchar *
wf_song_last_played_to_played_on_string(gint64 last_played, gboolean include_time)
{
	const gint one_year = 60 * 60 * 24 * 365;
	const gint half_year = one_year / 2;
	GDateTime *date_time;
	gchar *date, *string, *month_of_year;
	gint64 time;

	if (last_played < one_year)
	{
		date = g_strdup("Never");
	}
	else
	{
		time = wf_utils_time_now();
		date_time = g_date_time_new_from_unix_local(last_played);

		// Get date_time vars
		gint year = g_date_time_get_year(date_time);
		gint month = g_date_time_get_month(date_time);
		gint day = g_date_time_get_day_of_month(date_time);
		gint hour = g_date_time_get_hour(date_time);
		gint minute = g_date_time_get_minute(date_time);

		// Convert @month to a string
		switch (month)
		{
			case 1:
				month_of_year = g_strdup("January");
				break;
			case 2:
				month_of_year = g_strdup("February");
				break;
			case 3:
				month_of_year = g_strdup("March");
				break;
			case 4:
				month_of_year = g_strdup("April");
				break;
			case 5:
				month_of_year = g_strdup("May");
				break;
			case 6:
				month_of_year = g_strdup("June");
				break;
			case 7:
				month_of_year = g_strdup("July");
				break;
			case 8:
				month_of_year = g_strdup("August");
				break;
			case 9:
				month_of_year = g_strdup("September");
				break;
			case 10:
				month_of_year = g_strdup("October");
				break;
			case 11:
				month_of_year = g_strdup("November");
				break;
			case 12:
				month_of_year = g_strdup("December");
				break;
			default:
				month_of_year = g_strdup_printf("%02d", month);
		}

		// Check if played within last half year
		if (time - last_played > half_year)
		{
			// Display timestamp including year
			string = g_strdup_printf("%02d %s %04d", day, month_of_year, year);
		}
		else
		{
			// Display timestamp without year
			string = g_strdup_printf("%02d %s", day, month_of_year);
		}

		if (include_time)
		{
			date = g_strdup_printf("%s, %02d:%02d", string, hour, minute);

			g_free(string);
		}
		else
		{
			date = string;
		}

		g_free(month_of_year);
	}

	return date;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

// This gets invoked when the object is disposed (before destruction)
static void
wf_song_finalize(gpointer object)
{
	WfSong *song = WF_SONG(object);
	GObjectClass *parent_class = G_OBJECT_GET_CLASS(object);

	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(G_IS_OBJECT_CLASS(parent_class));

	wf_song_clear_location(song);

	g_free(song->priv->title);
	g_free(song->priv->artist);
	g_free(song->priv->album_artist);
	g_free(song->priv->album);

	// Chain up parent class methods
	parent_class->finalize(object);
}

/* DESTRUCTORS END */

/* END OF FILE */
