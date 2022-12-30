/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * utils.c  This file is part of LibWoofer
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
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gio/gio.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/utils.h>
#include <woofer/utils_private.h>

// Dependency includes
#include <woofer/song.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * SECTION:utils
 * @title: Woofer utilities
 * @short_description: General purpose Woofer utilities
 *
 * This module contains some general purpose utilities used throughout the
 * software.
 *
 * Since this module only contains utilities for other modules, all of these
 * "utilities" are part of the normal module functions and constructors,
 * destructors, etc. are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

gboolean
wf_utils_str_is_equal(const gchar *str1, const gchar *str2)
{
	return (str1 != NULL && str2 != NULL && g_strcmp0(str1,str2) == 0);
}

gchar *
wf_utils_str_to_lower(gchar *str)
{
	gchar *s;

	if (str == NULL)
	{
		return str;
	}

	for (s = str; *s != '\0'; s++)
	{
		*s = g_ascii_tolower(*s);
	}

	return str;
}

const gchar *
wf_utils_string_to_single_multiple(gint amount, const gchar *single, const gchar *multiple)
{
	return (amount == 1 || amount == -1) ? single : multiple;
}

gdouble
wf_utils_third_power(gdouble x)
{
	gdouble y = 1.0;

	for (gint i = 3; i > 0; i--)
	{
		y *= x;
	}

	return y;
}

gdouble
wf_utils_third_root(gdouble x)
{
	return cbrt(x);
}

gint
wf_utils_floor(gdouble value)
{
	return (gint) floor(value);
}

gint
wf_utils_round(gdouble value)
{
	return (gint) floor(value + 0.5);
}

gdouble
wf_utils_round_double(gdouble value, gint decimals)
{
	gdouble res = 0;
	gdouble multiplier = 1;

	// Calculate a multiplier that is equal to 10^(@decimals)
	for (gint x = decimals; x > 0; x--)
	{
		multiplier *= 10;
	}

	// Round with a given precision
	res = floor(value * multiplier + 0.5);

	// Convert back to the right value
	return res / multiplier;
}

gint64
wf_utils_time_now(void)
{
	gint64 time;

	time = g_get_real_time();

	// Convert to whole seconds (rounds automatically)
	return (time / 1000 / 1000);
}

gint64
wf_utils_time_compare(gint64 time_first, gint64 time_last)
{
	return ABS(time_last - time_first);
}

gchar *
wf_utils_duration_to_string(gint64 duration)
{
	gint total_seconds, seconds, minutes, hours;
	gchar *str;

	// Convert nanoseconds to seconds
	total_seconds = duration / 1000 / 1000 / 1000;

	// Get full minutes
	minutes = floor(total_seconds / 60);

	// Get remaining seconds (without minutes)
	seconds = total_seconds - (minutes * 60);

	if (total_seconds > 60 * 60)
	{
		// Get full hours
		hours = floor(minutes / 60);

		// Get remaining seconds (without hours)
		minutes = seconds - (hours * 60 * 60);

		// %02d Adds leading zeros if number if below 10
		str = g_strdup_printf("%d:%02d:%02d", hours, minutes, seconds);
	}
	else
	{
		str = g_strdup_printf("%d:%02d", minutes, seconds);
	}

	return str;
}

gchar *
wf_utils_get_pretty_song_msg(WfSong *song, gint64 duration)
{
	const gchar *name = NULL;
	const gchar *title = NULL;
	const gchar *artist = NULL;
	const gchar *album = NULL;
	gchar *title_str = NULL;
	gchar *artist_str = NULL;
	gchar *album_str = NULL;
	gchar *duration_str = NULL;
	gchar *msg = NULL;

	// Check names
	if (song != NULL)
	{
		name = wf_song_get_name(song);
		title = wf_song_get_title(song);
	}

	if (title == NULL)
	{
		if (duration > 0)
		{
			// String with the filename and duration
			duration_str = wf_utils_duration_to_string(duration);
			msg = g_strdup_printf("%s (%s)", name, duration_str);
			g_free(duration_str);
		}
		else
		{
			// String with only the filename
			msg = g_strdup(name);
		}

		return msg;
	}

	// Create partial strings for every property that exists

	artist = wf_song_get_artist(song);
	album = wf_song_get_album(song);

	// Title string
	title_str = g_strdup((title != NULL) ? title : name);

	// Check for artist
	if (artist == NULL)
	{
		artist_str = g_strdup("");
	}
	else
	{
		artist_str = g_strdup_printf(" by %s", artist);
	}

	// Check for album
	if (album == NULL)
	{
		album_str = g_strdup("");
	}
	else
	{
		album_str = g_strdup_printf(" on %s", album);
	}

	// Check for duration
	if (duration <= 0)
	{
		duration_str = g_strdup("");
	}
	else
	{
		msg = wf_utils_duration_to_string(duration);
		duration_str = g_strdup_printf(" (%s)", msg);
		g_free(msg);
	}

	// Now merge all partial string
	msg = g_strjoin(NULL /* separator */, title_str, artist_str, album_str, duration_str, NULL /* terminator */);

	// Free temporary strings
	g_free(title_str);
	g_free(artist_str);
	g_free(album_str);
	g_free(duration_str);

	return msg;
}

/*
 * Read an array of strings (character arrays) and add every item to a newly
 * allocated #GSList.  Be careful not to free the entire string array, as the
 * string pointers themselves are used in the list; so only free the string
 * array when this new #GSList is no longer needed.
 */
GSList *
wf_utils_files_strv_to_slist(gchar **strv)
{
	GSList *list = NULL;
	gchar *item;
	gint i;

	for (i = 0; strv[i] != NULL; i++)
	{
		item = strv[i];

		list = g_slist_append(list, item);
	}

	return list;
}

/*
 * Build a filepath with a filename that is located in the users config
 * directory.  String must be freed with g_free().
 *
 * Returns: (transfer full): Filepath string
 */
gchar *
wf_utils_get_config_filepath(const gchar *filename, const gchar *app_name)
{
	const gchar *configDir;
	gchar *path;

	g_return_val_if_fail(app_name != NULL, NULL);

	// $HOME/.config
	configDir = g_get_user_config_dir();

	if (filename == NULL)
	{
		path = g_build_filename(configDir, app_name, NULL);
	}
	else
	{
		path = g_build_filename(configDir, app_name, filename, NULL);
	}

	return path;
}

gboolean
wf_utils_file_is_dotfile(GFile *file)
{
	gchar *basename;
	gboolean is_dotfile = FALSE;

	g_return_val_if_fail(G_IS_FILE(file), FALSE);

	basename = g_file_get_basename(file);

	if (basename == NULL)
	{
		return FALSE;
	}

	if (basename[0] == '.')
	{
		is_dotfile = TRUE;
	}

	g_free(basename);

	return is_dotfile;
}

// Wrapper for g_key_file_save_to_file that makes sure the file directory exists before writing
gboolean
wf_utils_save_file_to_disk(GKeyFile *key_file, const gchar *filename, GError **error)
{
	gchar *dir;
	GFile *file;
	GError *err = NULL;

	g_return_val_if_fail(filename != NULL, FALSE);

	dir = g_path_get_dirname(filename);
	file = g_file_new_for_path(dir);

	if (!g_file_make_directory_with_parents(file, NULL/* GCancellable */, &err))
	{
		if (g_io_error_from_errno(err->code) != G_IO_ERROR_EXISTS)
		{
			// Already exists; no need to panic
			g_error_free(err);
		}
		else
		{
			g_warning("Failed to create directory <%s>: %s (%d)", dir, err->message, err->code);

			g_error_free(err);

			return FALSE;
		}
	}

	// Try saving the file
	return g_key_file_save_to_file(key_file, filename, error);
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
