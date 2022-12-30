/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * file_inspector.c  This file is part of LibWoofer
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
/*< none >*/

// Module includes
#include <woofer/file_inspector.h>

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module provides some functions to inspect files and folders selected by
 * the user to search and extract files of specific type.  The content of every
 * folder is inspected to get its content, which in turn may contain folder to
 * be inspected as well.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static gint wf_file_inspector_compare_alphabetic_cb(gconstpointer a, gconstpointer b);

static const gchar * wf_file_inspector_mime_get(GFileInfo *file_info);

static gboolean wf_file_inspector_mime_is_audio(const gchar *mime_type);
static gboolean wf_file_inspector_mime_is_media(const gchar *mime_type);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */
/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static gint
wf_file_inspector_compare_alphabetic_cb(gconstpointer a, gconstpointer b)
{
	GFile *file_a, *file_b;
	gchar *string_a, *string_b;
	gint result;

	// Silently discard const tag
	file_a = (GFile *) a;
	file_b = (GFile *) b;

	// Get filepaths
	string_a = g_file_get_uri(file_a);
	string_b = g_file_get_uri(file_b);

	// Compare
	result = g_strcmp0(string_a, string_b);

	g_free(string_a);
	g_free(string_b);

	return result;
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

static const gchar *
wf_file_inspector_mime_get(GFileInfo *file_info)
{
	g_return_val_if_fail(G_IS_FILE_INFO(file_info), NULL);

	return g_file_info_get_content_type(file_info);
}

WfFileInspectorType
wf_file_inspector_get_file_type(GFile *file, const gchar **mime_rv)
{
	const gchar *attributes = G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE;
	const gchar *mime_info;
	WfFileInspectorType type_info;
	GFileInfo *info;
	GFileType type;
	GError *err = NULL;

	g_return_val_if_fail(G_IS_FILE(file), WF_FILE_INSPECTOR_TYPE_ERROR);

	info = g_file_query_info(file, attributes, G_FILE_QUERY_INFO_NONE, NULL /* GCancellable */, &err);

	if (err != NULL)
	{
		g_warning("Failed to get file info: %s", err->message);
		g_error_free(err);
		err = NULL;

		if (info != NULL)
		{
			g_object_unref(info);
		}

		return WF_FILE_INSPECTOR_TYPE_ERROR;
	}
	else
	{
		type = g_file_info_get_file_type(info);
	}

	if (type == G_FILE_TYPE_REGULAR)
	{
		mime_info = wf_file_inspector_mime_get(info);

		if (mime_info == NULL)
		{
			type_info = WF_FILE_INSPECTOR_TYPE_MIME_UNKNOWN;
		}
		else if (wf_file_inspector_mime_is_audio(mime_info))
		{
			type_info = WF_FILE_INSPECTOR_TYPE_MIME_AUDIO;
		}
		else if (wf_file_inspector_mime_is_media(mime_info))
		{
			type_info = WF_FILE_INSPECTOR_TYPE_MIME_MEDIA;
		}
		else
		{
			type_info = WF_FILE_INSPECTOR_TYPE_MIME_IRRELEVANT;
		}

		// Set mime description for other uses for the caller
		if (mime_rv != NULL)
		{
			*mime_rv = mime_info;
		}
	}
	else if (type == G_FILE_TYPE_DIRECTORY)
	{
		type_info = WF_FILE_INSPECTOR_TYPE_DIRECTORY;
	}
	else
	{
		type_info = WF_FILE_INSPECTOR_TYPE_UNKNOWN;
	}

	g_object_unref(info);

	return type_info;
}

GSList *
wf_file_inspector_get_directory_files(GFile *file)
{
	const gchar *name;
	gchar *dir_path, *file_path;
	GDir *dir;
	GFile *child;
	GSList *list = NULL;
	GError *err = NULL;

	g_return_val_if_fail(G_IS_FILE(file), NULL);

	dir_path = g_file_get_path(file);
	dir = g_dir_open(dir_path, 0, &err);

	if (err != NULL)
	{
		g_warning("Could not open directory %s: %s", dir_path, err->message);
		g_error_free(err);

		return NULL;
	}

	do
	{
		name = g_dir_read_name(dir);

		if (name != NULL)
		{
			file_path = g_build_filename(dir_path, name, NULL /* terminator */);

			child = g_file_new_for_path(file_path);
			list = g_slist_append(list, child);

			g_free(file_path);
		}
	} while (name != NULL);

	list = g_slist_sort(list, wf_file_inspector_compare_alphabetic_cb);

	g_dir_close(dir);
	g_free(dir_path);

	return list;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static gboolean
wf_file_inspector_mime_is_audio(const gchar *mime_type)
{
	g_return_val_if_fail(mime_type != NULL, FALSE);

	return g_str_has_prefix(mime_type, "audio/");
}

static gboolean
wf_file_inspector_mime_is_media(const gchar *mime_type)
{
	gboolean is_audio;
	gboolean is_video;

	g_return_val_if_fail(mime_type != NULL, FALSE);

	is_audio = g_str_has_prefix(mime_type, "audio/");
	is_video = g_str_has_prefix(mime_type, "video/");

	return (is_audio || is_video);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */
/* DESTRUCTORS END */

/* END OF FILE */
