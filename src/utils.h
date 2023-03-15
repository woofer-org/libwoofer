/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * utils.h  This file is part of LibWoofer
 * Copyright (C) 2022  Quico Augustijn
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

#ifndef __WF_UTILS__
#define __WF_UTILS__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/song.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean wf_utils_str_is_equal(const gchar *str1, const gchar *str2);
gchar * wf_utils_str_to_lower(gchar *str);

const gchar * wf_utils_string_to_single_multiple(gint amount, const gchar *single, const gchar *multiple);

gdouble wf_utils_third_power(gdouble x);
gdouble wf_utils_third_root(gdouble x);
gint wf_utils_floor(gdouble value);
gint wf_utils_round(gdouble value);
gdouble wf_utils_round_double(gdouble value, gint decimals);

gint64 wf_utils_time_now(void);
gint64 wf_utils_time_compare(gint64 time_first, gint64 time_last);

gchar * wf_utils_duration_to_string(gint64 duration);

gchar * wf_utils_get_pretty_song_msg(WfSong *song, gint64 duration);

GSList * wf_utils_files_strv_to_slist(gchar **strv);

gboolean wf_utils_file_is_dotfile(GFile *file);

/* FUNCTION PROTOTYPES END */

G_END_DECLS

#endif /* __WF_UTILS__ */

/* END OF FILE */
