/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * file_inspector.h  This file is part of LibWoofer
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

#ifndef __WF_FILE_INSPECTOR__
#define __WF_FILE_INSPECTOR__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfFileInspectorType WfFileInspectorType;

enum _WfFileInspectorType
{
	WF_FILE_INSPECTOR_TYPE_UNKNOWN,
	WF_FILE_INSPECTOR_TYPE_ERROR,
	WF_FILE_INSPECTOR_TYPE_DIRECTORY,
	WF_FILE_INSPECTOR_TYPE_MIME_UNKNOWN,
	WF_FILE_INSPECTOR_TYPE_MIME_AUDIO,
	WF_FILE_INSPECTOR_TYPE_MIME_MEDIA,
	WF_FILE_INSPECTOR_TYPE_MIME_IRRELEVANT
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

WfFileInspectorType wf_file_inspector_get_file_type(GFile *file, const gchar **mime_rv);

GSList * wf_file_inspector_get_directory_files(GFile *file);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_FILE_INSPECTOR__ */

/* END OF FILE */
