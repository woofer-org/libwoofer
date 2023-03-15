/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * utils.h  This file is part of LibWoofer
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

#ifndef __WF_UTILS_PRIVATE__
#define __WF_UTILS_PRIVATE__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

#include <woofer/song.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfFileTypeInfo WfFileTypeInfo;

enum _WfFileTypeInfo
{
	WF_FILE_TYPE_INFO_UNKNOWN,
	WF_FILE_TYPE_INFO_ERROR,
	WF_FILE_TYPE_INFO_DIRECTORY,
	WF_FILE_TYPE_INFO_MIME_UNKNOWN,
	WF_FILE_TYPE_INFO_MIME_AUDIO,
	WF_FILE_TYPE_INFO_MIME_IRRELEVANT
};

/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gchar * wf_utils_get_config_filepath(const gchar *filename, const gchar *app_name);

gboolean wf_utils_save_file_to_disk(GKeyFile *key_file, const gchar *filename, GError **error);

/* FUNCTION PROTOTYPES END */

G_END_DECLS

#endif /* __WF_UTILS_PRIVATE__ */

/* END OF FILE */
