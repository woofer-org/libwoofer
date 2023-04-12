/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * library.c  This file is part of LibWoofer
 * Copyright (C) 2021-2023  Quico Augustijn
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
#include <woofer/library.h>
#include <woofer/library_private.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/song_private.h>
#include <woofer/file_inspector.h>
#include <woofer/utils.h>
#include <woofer/utils_private.h>
#include <woofer/memory.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/**
 * SECTION:library
 * @title: Woofer Library
 * @short_description: The application's song library
 *
 * The song library is essentially a list of all known songs, with references to
 * each individual object.  On application startup, the library file is read and
 * for every item found, a #WfSong object is created and added to the list,
 * regardless of whether it exists or not.  When any significant changes are made
 * to one or more songs, the file content is reconstructed and written back to
 * the disk.  When the user explicitly adds new items from the filesystem, new
 * objects are created and added to the end of the list.  This list is used by
 * the intelligence module whenever requested by the internals of the
 * application.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

/*
 * Version the software currently uses.
 *
 * This number represents the date when the way that the library file is handled
 * changes significantly.  Because of this, application software that uses an
 * older version of the file, may become incompatible.  Ever since this was
 * implemented, the versions are checked at runtime and it may issue a warning
 * and ignore the content of the settings file when it may be incompatible.
 *
 * Note: Please keep in mind that incompatibility warnings should only ever be
 * the case when the file was written with a newer version and then opened with
 * an older version.
 */
#define FILE_VERSION 20221201

// If %TRUE, do not inspect dot files on add directory
#define SKIP_DOT_FILES TRUE

// Define static names used in the key_file.
#define GROUP_PROPERTIES "Properties"
#define NAME_VERSION "FileVersion"
#define NAME_LOCATION "Location"
#define NAME_URI "URI"
#define NAME_UPDATED "LastMetadataUpdate"
#define NAME_TRACK_NUMBER "TrackNumber"
#define NAME_TITLE "Title"
#define NAME_ARTIST "Artist"
#define NAME_ALBUM_ARTIST "AlbumArtist"
#define NAME_ALBUM "Album"
#define NAME_DURATION "Duration"
#define NAME_RATING "Rating"
#define NAME_SCORE "Score"
#define NAME_PLAYCOUNT "PlayCount"
#define NAME_SKIPCOUNT "SkipCount"
#define NAME_LASTPLAYED "LastPlayed"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _WfLibraryEvents WfLibraryEvents;
typedef struct _WfLibraryDetails WfLibraryDetails;

struct _WfLibraryEvents
{
	WfFuncStatsUpdated stats_updated;
};

struct _WfLibraryDetails
{
	WfLibraryEvents events;

	gboolean active;
	gchar *default_path;
	gchar *file_path;

	GKeyFile *key_file;

	gboolean write_queued;

	gboolean track_number_column_empty;
	gboolean title_column_empty;
	gboolean artist_column_empty;
	gboolean album_column_empty;
	gboolean duration_column_empty;

	gboolean track_number_column_full;
	gboolean title_column_full;
	gboolean artist_column_full;
	gboolean album_column_full;
	gboolean duration_column_full;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_library_emit_stats_updated(WfLibraryEvents *events);

static GKeyFile * wf_library_new_key_file(void);
static gboolean wf_library_add_song_from_key_group(GKeyFile *key_file, const gchar *group);
static gint wf_library_update_metadata_internal(gboolean force);
static void wf_library_parse_key_file(GKeyFile *key_file, gint *added_rv);
static GKeyFile * wf_library_parse_list(void);
static gboolean wf_library_file_open(GKeyFile *key_file, const gchar *filename, GError **error);

static gint wf_library_add_uris_internal(GSList *files, gint *amount_rv, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);
static gint wf_library_add_files_internal(GSList *files, gint *amount_rv, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata);

static void wf_library_update_key_file_item(GKeyFile *key_file, WfSong *song);
static gboolean wf_library_check_file_compatible(GKeyFile *key_file, const gchar *file_path);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static WfLibraryDetails LibraryData = { 0 };

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_library_init(void)
{
	if (LibraryData.active)
	{
		g_warning("Module library is already initialized. This should not happen twice.");

		return;
	}

	LibraryData.default_path = wf_utils_get_config_filepath(WF_LIBRARY_FILENAME, WF_TAG);

	LibraryData.active = TRUE;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

void
wf_library_connect_event_stats_updated(WfFuncStatsUpdated cb_func)
{
	LibraryData.events.stats_updated = cb_func;
}

/**
 * wf_library_set_file:
 * @file_path: (transfer none) (nullable): path to the library file to use
 *
 * Sets the filepath of the library file to use.  This file may or may not
 * actually exist; it will be (over)written whenever library changed are to be
 * saved.
 *
 * Since: 0.2
 **/
void
wf_library_set_file(const gchar *file_path)
{
	g_free(LibraryData.file_path);

	// Copy and store new file path
	LibraryData.file_path = g_strdup(file_path);
}

/**
 * wf_library_get_file:
 *
 * Gets the filepath of the library file in use.
 *
 * Returns: (transfer none): path to the current library file
 *
 * Since: 0.2
 **/
const gchar *
wf_library_get_file(void)
{
	const gchar *file_path = LibraryData.file_path;
	const gchar *default_path = LibraryData.default_path;

	return (file_path != NULL) ? file_path : default_path;
}

gboolean
wf_library_track_number_column_is_empty(void)
{
	return LibraryData.track_number_column_empty;
}

gboolean
wf_library_title_column_is_empty(void)
{
	return LibraryData.title_column_empty;
}

gboolean
wf_library_artist_column_is_empty(void)
{
	return LibraryData.artist_column_empty;
}

gboolean
wf_library_album_column_is_empty(void)
{
	return LibraryData.album_column_empty;
}

gboolean
wf_library_duration_column_is_empty(void)
{
	return LibraryData.duration_column_empty;
}

gboolean
wf_library_track_number_column_is_full(void)
{
	return LibraryData.track_number_column_full;
}

gboolean
wf_library_title_column_is_full(void)
{
	return LibraryData.title_column_full;
}

gboolean
wf_library_artist_column_is_full(void)
{
	return LibraryData.artist_column_full;
}

gboolean
wf_library_album_column_is_full(void)
{
	return LibraryData.album_column_full;
}

GList *
wf_library_get(void)
{
	WfSong *song;
	GList *list = NULL;

	// Create a new #GList that can be freely used by the caller
	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		list = g_list_append(list, song);
	}

	return list;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

static void
wf_library_emit_stats_updated(WfLibraryEvents *events)
{
	g_return_if_fail(events != NULL);

	if (events->stats_updated != NULL)
	{
		events->stats_updated();
	}
}

static GKeyFile *
wf_library_new_key_file(void)
{
	// Free the old key_file, because we create a new one and overwrite the pointer anyway.
	if (LibraryData.key_file != NULL)
	{
		g_key_file_free(LibraryData.key_file);
	}

	LibraryData.key_file = g_key_file_new();

	return LibraryData.key_file;
}

static gboolean
wf_library_add_song_from_key_group(GKeyFile *key_file, const gchar *group)
{
	WfSong *song = NULL;
	GError *err = NULL;
	gboolean uri_set;
	gchar *uri;
	gchar *key;
	gchar **keyArray;
	gsize keyLength;
	guint x;

	gint intValue;
	gdouble doubleValue;
	gint64 int64Value;
	gchar *gcharValue;

	g_return_val_if_fail(group != NULL, FALSE);

	keyArray = g_key_file_get_keys(key_file, group, &keyLength, &err);

	if (err != NULL)
	{
		g_warning("Error occurred while getting keys from file: %s", err->message);
		g_error_free(err);

		return FALSE;
	}

	// Go through all keys of this particular song
	for (x = 0; x < keyLength; x++)
	{
		key = keyArray[x];
		uri_set = FALSE;

		if (key == NULL)
		{
			// End of key array
			break;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(GROUP_PROPERTIES, -1)) == 0)
		{
			// Not a song
			return FALSE;
		}

		// Vars starting with a capital letter are defined statically
		if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_URI, -1)) == 0)
		{
			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("No file location for item %s due to: %s. This item will not be added.", group, err->message);
				g_clear_error(&err);
				break; // Stop handling keys
			}
			else if (gcharValue == NULL)
			{
				g_warning("No uri for item %s. This item will not be added.", group);
				break;
			}
			else
			{
				song = wf_song_append_by_uri(gcharValue);

				uri_set = TRUE;
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_LOCATION, -1)) == 0)
		{
			// URI takes priority: If already set, skip this one
			if (uri_set)
			{
				continue;
			}

			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else if (gcharValue == NULL)
			{
				g_message("No value to for field %s", key);
			}
			else
			{
				uri = g_filename_to_uri(gcharValue, NULL /* hostname */, NULL /* GError */);

				if (uri == NULL)
				{
					g_warning("Failed to get URI for location %s", gcharValue);
				}
				else
				{
					song = wf_song_append_by_uri(uri);

					// Filename is converted to URI; save when done
					LibraryData.write_queued = TRUE;
				}
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_UPDATED, -1)) == 0)
		{
			int64Value = g_key_file_get_int64(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else if (int64Value < 0)
			{
				g_debug("Invalid %s: %ld", key, (long int) int64Value);
			}
			else if (int64Value != 0)
			{
				wf_song_set_metadata_updated(song, int64Value);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_TRACK_NUMBER, -1)) == 0)
		{
			intValue = g_key_file_get_integer(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else
			{
				wf_song_set_track_number(song, intValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_TITLE, -1)) == 0)
		{
			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else
			{
				wf_song_set_title(song, gcharValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_ARTIST, -1)) == 0)
		{
			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else
			{
				wf_song_set_artist(song, gcharValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_ALBUM_ARTIST, -1)) == 0)
		{
			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else
			{
				wf_song_set_album_artist(song, gcharValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_ALBUM, -1)) == 0)
		{
			gcharValue = g_key_file_get_string(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else
			{
				wf_song_set_album(song, gcharValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_DURATION, -1)) == 0)
		{
			intValue = g_key_file_get_integer(key_file, group, key, &err);

			if (err != NULL)
			{
				g_message("Failed to read field %s (%s): %s", key, group, err->message);
				g_clear_error(&err);
			}
			else if (intValue < 0)
			{
				g_debug("Invalid %s: %d", key, intValue);
			}
			else
			{
				wf_song_set_duration_seconds(song, intValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_RATING, -1)) == 0)
		{
			intValue = g_key_file_get_integer(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("Error occurred while getting rating (int) for %s: %s", group, err->message);
				g_clear_error(&err);
			}
			else if (intValue < 0)
			{
				g_debug("Invalid %s: %d", key, intValue);
			}
			else
			{
				wf_song_set_rating(song, intValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_SCORE, -1)) == 0)
		{
			doubleValue = g_key_file_get_double(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("Error occurred while getting score (double) for %s: %s", group, err->message);
				g_clear_error(&err);
			}
			else if (doubleValue < 0)
			{
				g_debug("Invalid %s: %f", key, doubleValue);
			}
			else
			{
				wf_song_set_score(song, doubleValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_PLAYCOUNT, -1)) == 0)
		{
			intValue = g_key_file_get_integer(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("Error occurred while getting play count (int) for %s: %s", group, err->message);
				g_clear_error(&err);
			}
			else if (intValue < 0)
			{
				g_debug("Invalid %s: %d", key, intValue);
			}
			else
			{
				wf_song_set_play_count(song, intValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_SKIPCOUNT, -1)) == 0)
		{
			intValue = g_key_file_get_integer(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("Error occurred while getting skip count (int) for %s: %s", group, err->message);
				g_clear_error(&err);
			}
			else if (intValue < 0)
			{
				g_debug("Invalid %s: %d", key, intValue);
			}
			else
			{
				wf_song_set_skip_count(song, intValue);
			}

			continue;
		}
		else if (g_strcmp0(g_utf8_strdown(key, -1), g_utf8_strdown(NAME_LASTPLAYED, -1)) == 0)
		{
			int64Value = g_key_file_get_int64(key_file, group, key, &err);

			if (err != NULL)
			{
				g_warning("Error occurred while getting last played (int64) for %s: %s", group, err->message);
				g_clear_error(&err);
			}
			else if (int64Value < 0)
			{
				g_debug("Invalid %s: %ld", key, (long int) int64Value);
			}
			else
			{
				wf_song_set_last_played(song, int64Value);
			}

			continue;
		}
	}

	if (song != NULL)
	{
		// Set the tag found in the file
		wf_song_set_tag(song, group);
	}

	g_strfreev(keyArray);

	return TRUE;
}

static gint
wf_library_update_metadata_internal(gboolean force)
{
	WfSong *song;
	gint n = 0;

	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		if (wf_song_update_metadata(song, force))
		{
			n++;
		}
	}

	if (n > 0)
	{
		// Write library to disk which include the newly fetched metadata
		LibraryData.write_queued = TRUE;
	}

	return n;
}

void
wf_library_move_before(WfSong *song, WfSong *other_song)
{
	wf_song_move_before(song, other_song);

	wf_library_queue_write();
}

void
wf_library_move_after(WfSong *song, WfSong *other_song)
{
	wf_song_move_after(song, other_song);

	wf_library_queue_write();
}

// Get GList from GKeyFile
static void
wf_library_parse_key_file(GKeyFile *key_file, gint *added_rv)
{
	gchar **groupArray, *group;
	gsize groupLength = 0;
	gint amount = 0;
	guint x;

	g_return_if_fail(key_file != NULL);

	groupArray = g_key_file_get_groups(key_file, &groupLength);

	if (groupLength > 0)
	{
		// Go through all songs (one group == one song)
		for (x = 0; x < groupLength; x++)
		{
			group = groupArray[x];

			if (group == NULL)
			{
				// End of string array
				break;
			}
			else if (wf_library_add_song_from_key_group(key_file, group))
			{
				amount++;
			}
		}

		if (amount > 0)
		{
			g_info("Found %d songs in song library file", amount);
		}
	}

	g_strfreev(groupArray);

	// Update the provided value with the amount of added items
	if (added_rv != NULL)
	{
		*added_rv = amount;
	}
}

// Get GKeyFile from GList
static GKeyFile *
wf_library_parse_list(void)
{
	WfSong *song;
	GKeyFile *key_file;

	g_debug("Generating library file...");
	key_file = wf_library_new_key_file();

	g_key_file_set_comment(key_file, NULL /* group */, NULL /* key */,
	                       "Note that any comment written to this file will "
	                       "not be preserved when the software rewrites this "
	                       "file.", NULL /* GError */);

	// Set file version
	g_key_file_set_integer(key_file, GROUP_PROPERTIES, NAME_VERSION, FILE_VERSION);

	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		wf_library_update_key_file_item(key_file, song);
	}

	return key_file;
}

static gboolean
wf_library_file_open(GKeyFile *key_file, const gchar *filename, GError **error)
{
	GError **file_error, *g_error = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(key_file != NULL, FALSE);

	// If given, use the input GError; otherwise use the local pointer to printed the messages at the very least
	if (error == NULL)
	{
		file_error = &g_error;
	}
	else
	{
		file_error = error;
	}

	if (!g_key_file_load_from_file(key_file, filename, G_KEY_FILE_KEEP_COMMENTS, file_error))
	{
		if (*file_error == NULL)
		{
			g_warning("Could not open file %s due to a unknown error", filename);
		}
		else
		{
			g_info("Could not open library file %s: %s", filename, (*file_error)->message);
		}

		result = FALSE;
	}
	else if (*file_error != NULL)
	{
		g_info("Successfully opened library file, but received an error: %s", (*file_error)->message);

		result = TRUE;
	}
	else
	{
		result = TRUE;
	}

	// Only free if local GError was used
	if (error == NULL && g_error != NULL)
	{
		g_error_free(g_error);
	}

	return result;
}

gboolean
wf_library_read(void)
{
	const gchar *file = wf_library_get_file();
	gboolean added = 0;
	GKeyFile *key_file;

	g_return_val_if_fail(file != NULL, FALSE);

	key_file = wf_library_new_key_file();

	if (!wf_library_file_open(key_file, file, NULL /* GError */) ||
	    !wf_library_check_file_compatible(key_file, file))
	{
		return FALSE;
	}

	// Clear first before overwriting
	wf_song_remove_all();

	// Now collect all songs from the file
	wf_library_parse_key_file(key_file, &added);

	// Check for any needed metadata updates
	wf_library_update_metadata_internal(FALSE /* force */);

	// Write changes (if any) to disk
	wf_library_write(FALSE);

	return (added > 0);
}

gboolean
wf_library_write(gboolean force)
{
	const gchar *file = wf_library_get_file();
	GKeyFile *key_file;
	GError *err = NULL;
	gboolean res = FALSE;

	// Only write if queued or forced
	if (!LibraryData.write_queued && !force)
	{
		return TRUE;
	}

	key_file = wf_library_parse_list();

	if (key_file == NULL)
	{
		g_info("Library file is empty; not writing to disk");
		LibraryData.write_queued = FALSE;

		return FALSE;
	}

	if (wf_utils_save_file_to_disk(key_file, file, &err))
	{
		g_info("Successfully written library file to disk");
		LibraryData.write_queued = FALSE;

		res = TRUE;
	}
	else
	{
		g_warning("Failed to write library file to disk: %s", err->message);
	}

	g_clear_error(&err);

	return res;
}

void
wf_library_updated_stats(void)
{
	wf_library_emit_stats_updated(&LibraryData.events);
}

void
wf_library_queue_write(void)
{
	LibraryData.write_queued = TRUE;
}

gint
wf_library_update_metadata(void)
{
	gint result;

	// Request a forced metadata update
	result = wf_library_update_metadata_internal(TRUE /* force */);

	// Save any changes
	wf_library_write(FALSE);

	// Return the amount of updated songs
	return result;
}

gint
wf_library_add_by_file(GFile *file, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	const gchar *mime = NULL;
	WfSong *song = NULL;
	WfLibraryFileChecks file_check = ((checks <= 0) ? WF_LIBRARY_CHECK_DEFAULT : checks);
	WfFileInspectorType type;
	GSList *dirs;
	gchar *uri;
	gint amount = 0;
	gboolean do_add = FALSE;

	g_return_val_if_fail(G_IS_FILE(file), 0);

	uri = g_file_get_uri(file);

	if (!wf_song_is_unique_uri(uri))
	{
		g_info("File %s already exists in library", uri);
	}
	else if (SKIP_DOT_FILES && wf_utils_file_is_dotfile(file)) // Check if dot file
	{
		g_info("File %s is a dotfile, not adding", uri);
	}
	else if (file_check != WF_LIBRARY_CHECK_NONE)
	{
		// Get type
		type = wf_file_inspector_get_file_type(file, &mime);

		switch (type)
		{
			case WF_FILE_INSPECTOR_TYPE_UNKNOWN:
				g_message("Invalid type for file %s", uri);
				break;
			case WF_FILE_INSPECTOR_TYPE_ERROR:
				// Do nothing, message has already been printed
				break;
			case WF_FILE_INSPECTOR_TYPE_DIRECTORY:
				// Inspect its files and add them
				dirs = wf_file_inspector_get_directory_files(file);

				// Since we now have the GFiles, we can use the function for adding a dir list
				wf_library_add_files_internal(dirs, &amount, func, file_check, skip_metadata);
				break;
			case WF_FILE_INSPECTOR_TYPE_MIME_UNKNOWN:
				g_message("Could not get mime type of %s. This file will not be added to the library", uri);
				break;
			case WF_FILE_INSPECTOR_TYPE_MIME_AUDIO:
				do_add = (file_check == WF_LIBRARY_CHECK_AUDIO || file_check == WF_LIBRARY_CHECK_MEDIA);
				break;
			case WF_FILE_INSPECTOR_TYPE_MIME_MEDIA:
				do_add = (file_check == WF_LIBRARY_CHECK_MEDIA);
				break;
			case WF_FILE_INSPECTOR_TYPE_MIME_IRRELEVANT:
				// File does not have any audio mimetype
				g_info("Found file %s with non-audio mime type <%s>. "
					   "This file will not be added to the library", uri, mime);
				break;
		}
	}
	else
	{
		do_add = TRUE;
	}

	if (do_add)
	{
		g_info("Found song %s", uri);

		song = wf_song_append_by_file(file);
		wf_song_set_status(song, WF_SONG_AVAILABLE);

		if (!skip_metadata)
		{
			wf_song_update_metadata(song, FALSE /* force */);
		}

		LibraryData.write_queued = TRUE;
		amount = 1;

		if (func != NULL)
		{
			// Update caller (report 0 as the total items are unknown)
			func(song, 0 /* item */, 0 /* total */);
		}
	}

	g_free(uri);
	g_object_unref(file);

	return amount;
}

gint
wf_library_add_by_uri(const gchar *uri, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	GFile *file;

	file = g_file_new_for_uri(uri);

	return wf_library_add_by_file(file, func, checks, skip_metadata);
}

// Check and add a list of files (could be both files and dirs) by taking a array of string
gint
wf_library_add_strv(gchar *files[], WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	const gchar *uri;
	gchar **strings;
	gint amount;
	gint total = 0;

	g_return_val_if_fail(files != NULL, 0);

	// Go over each string in the array
	for (strings = files; *strings != NULL; strings++)
	{
		uri = *strings;

		if (uri == NULL)
		{
			continue;
		}

		amount = wf_library_add_by_uri(uri, func, checks, skip_metadata);

		total += amount;
	}

	return total;
}

static gint
wf_library_add_uris_internal(GSList *files, gint *amount_rv, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	const gchar *uri;
	GSList *l;
	gint x;
	gint added = 0;

	g_return_val_if_fail(files != NULL, 0);

	for (l = files; l != NULL; l = l->next)
	{
		uri = l->data;

		if (uri == NULL)
		{
			continue;
		}

		x = wf_library_add_by_uri(uri, func, checks, skip_metadata);

		added += x;
	}

	*amount_rv += added;

	return added;
}

// Add a #GSList with URIs to check and add
gint
wf_library_add_uris(GSList *files, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	gint amount = 0;

	wf_library_add_uris_internal(files, &amount, func, checks, skip_metadata);

	wf_library_write(FALSE);

	return amount;
}

static gint
wf_library_add_files_internal(GSList *files, gint *amount_rv, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	GSList *l;
	GFile *f;
	gint x;
	gint added = 0;

	if (files == NULL)
	{
		// No files, nothing added
		return 0;
	}

	for (l = files; l != NULL; l = l->next)
	{
		f = l->data;

		if (!G_IS_FILE(f))
		{
			continue;
		}

		x = wf_library_add_by_file(f, func, checks, skip_metadata);

		added += x;
	}

	*amount_rv = added;

	return added;
}

// Add a #GSList with #GFile items to check and add
gint
wf_library_add_files(GSList *files, WfFuncItemAdded func, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	gint amount = 0;

	g_return_val_if_fail(files != NULL, 0);

	wf_library_add_files_internal(files, &amount, func, checks, skip_metadata);

	wf_library_write(FALSE);

	return amount;
}

void
wf_library_update_column_info(void)
{
	WfSong *song;
	gint amount = 0, has_track_number = 0, has_title = 0, has_artist = 0, has_album = 0, has_duration = 0;

	// Set to %TRUE and unset if a value is found that can be used in the respective column
	LibraryData.track_number_column_empty = TRUE;
	LibraryData.title_column_empty = TRUE;
	LibraryData.artist_column_empty = TRUE;
	LibraryData.album_column_empty = TRUE;
	LibraryData.duration_column_empty = TRUE;
	LibraryData.track_number_column_full = FALSE;
	LibraryData.title_column_full = FALSE;
	LibraryData.artist_column_full = FALSE;
	LibraryData.album_column_full = FALSE;
	LibraryData.duration_column_full = FALSE;

	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		if (wf_song_get_track_number(song) > 0)
		{
			has_track_number++;
			LibraryData.track_number_column_empty = FALSE;
		}

		if (wf_song_get_title(song) != NULL)
		{
			has_title++;
			LibraryData.title_column_empty = FALSE;
		}

		if (wf_song_get_artist(song) != NULL)
		{
			has_artist++;
			LibraryData.artist_column_empty = FALSE;
		}

		if (wf_song_get_album(song) != NULL)
		{
			has_album++;
			LibraryData.album_column_empty = FALSE;
		}

		if (wf_song_get_duration(song) > 1)
		{
			has_duration++;
			LibraryData.duration_column_empty = FALSE;
		}

		amount++;
	}

	if (has_track_number == amount)
	{
		LibraryData.track_number_column_full = TRUE;
	}

	if (has_title == amount)
	{
		LibraryData.title_column_full = TRUE;
	}

	if (has_artist == amount)
	{
		LibraryData.artist_column_full = TRUE;
	}

	if (has_album == amount)
	{
		LibraryData.album_column_full = TRUE;
	}

	if (has_duration == amount)
	{
		LibraryData.duration_column_full = TRUE;
	}
}

void
wf_library_remove_song(WfSong *song)
{
	if (song == NULL)
	{
		return;
	}

	wf_song_remove(song);

	LibraryData.write_queued = TRUE;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static void
wf_library_update_key_file_item(GKeyFile *key_file, WfSong *song)
{
	const gchar *group, *uri, *title, *artist, *album_artist, *album;
	gdouble score;
	gint rating, play_count, skip_count, track_number, duration;
	gint64 last_played, updated;

	g_return_if_fail(WF_IS_SONG(song));

	group = wf_song_get_tag(song);
	uri = wf_song_get_uri(song);
	updated = wf_song_get_metadata_updated(song);
	track_number = wf_song_get_track_number(song);
	title = wf_song_get_title(song);
	artist = wf_song_get_artist(song);
	album_artist = wf_song_get_album_artist(song);
	album = wf_song_get_album(song);
	duration = wf_song_get_duration(song);
	rating = wf_song_get_rating(song);
	score = wf_song_get_score(song);
	play_count = wf_song_get_play_count(song);
	skip_count = wf_song_get_skip_count(song);
	last_played = wf_song_get_last_played(song);

	g_key_file_set_string(key_file, group, NAME_URI, uri);
	g_key_file_set_integer(key_file, group, NAME_RATING, rating);
	g_key_file_set_double(key_file, group, NAME_SCORE, score);
	g_key_file_set_integer(key_file, group, NAME_PLAYCOUNT, play_count);
	g_key_file_set_integer(key_file, group, NAME_SKIPCOUNT, skip_count);
	g_key_file_set_int64(key_file, group, NAME_LASTPLAYED, last_played);
	g_key_file_set_int64(key_file, group, NAME_UPDATED, updated);

	if (track_number > 0)
	{
		g_key_file_set_integer(key_file, group, NAME_TRACK_NUMBER, track_number);
	}

	if (title != NULL)
	{
		g_key_file_set_string(key_file, group, NAME_TITLE, title);
	}

	if (artist != NULL)
	{
		g_key_file_set_string(key_file, group, NAME_ARTIST, artist);
	}

	if (album_artist != NULL)
	{
		g_key_file_set_string(key_file, group, NAME_ALBUM_ARTIST, album_artist);
	}

	if (album != NULL)
	{
		g_key_file_set_string(key_file, group, NAME_ALBUM, album);
	}

	if (duration <= 1)
	{
		g_key_file_set_integer(key_file, group, NAME_DURATION, duration);
	}
}

static gboolean
wf_library_check_file_compatible(GKeyFile *key_file, const gchar *file_path)
{
	const gchar *group = GROUP_PROPERTIES, *key = NAME_VERSION;
	gint version = 0;

	g_return_val_if_fail(key_file != NULL, FALSE);

	// Incompatibility check
	version = g_key_file_get_integer(key_file, group, key, NULL /* GError */);

	if (version < FILE_VERSION)
	{
		// Older version

		if (file_path == NULL)
		{
			g_info("Library file is written with an older software version");
		}
		else
		{
			g_info("Library %s is written with an older software version", file_path);
		}

		// Backward compatible; may update
		return TRUE;
	}
	else if (version > FILE_VERSION)
	{
		// Newer version

		if (file_path == NULL)
		{
			g_message("Library file is written with a newer version of the software.");
		}
		else
		{
			g_message("Library %s is written with a newer version of the software.", file_path);
		}

		return FALSE;
	}
	else // version == FILE_VERSION
	{
		// Compatible, nothing to report
		return TRUE;
	}
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_library_finalize(void)
{
	// Write any made changes to disk
	wf_library_write(FALSE);

	// Free file data
	g_free(LibraryData.file_path);
	g_free(LibraryData.default_path);
	wf_memory_clear_key_file(&LibraryData.key_file);

	LibraryData = (WfLibraryDetails) { 0 };

	wf_song_remove_all();
}

/* DESTRUCTORS END */

/* END OF FILE */
