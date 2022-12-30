/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * settings.h  This file is part of LibWoofer
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

#ifndef __WF_SETTINGS__
#define __WF_SETTINGS__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/intelligence.h>

/* INCLUDES END */

/* DEFINES BEGIN */

#define WF_SETTINGS_FILENAME "settings.conf"

/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _WfSettingType WfSettingType;

enum _WfSettingType
{
	WF_SETTING_NONE,

	WF_SETTING_VOLUME,
	WF_SETTING_UPDATE_INTERVAL,
	WF_SETTING_PREFER_PLAY_FROM_RAM,
	WF_SETTING_MIN_PLAYED_FRACTION,
	WF_SETTING_FULL_PLAYED_FRACTION,

	WF_SETTING_FILTER_RECENT_ARTISTS,
	WF_SETTING_FILTER_RECENT_AMOUNT,
	WF_SETTING_FILTER_RECENT_PERCENTAGE,
	WF_SETTING_FILTER_RATING,
	WF_SETTING_FILTER_SCORE,
	WF_SETTING_FILTER_PLAYCOUNT,
	WF_SETTING_FILTER_SKIPCOUNT,
	WF_SETTING_FILTER_LASTPLAYED,
	WF_SETTING_FILTER_RATING_INC_ZERO,
	WF_SETTING_FILTER_PLAYCOUNT_INV,
	WF_SETTING_FILTER_SKIPCOUNT_INV,
	WF_SETTING_FILTER_LASTPLAYED_INV,
	WF_SETTING_FILTER_RATING_MIN,
	WF_SETTING_FILTER_RATING_MAX,
	WF_SETTING_FILTER_SCORE_MIN,
	WF_SETTING_FILTER_SCORE_MAX,
	WF_SETTING_FILTER_PLAYCOUNT_TH,
	WF_SETTING_FILTER_SKIPCOUNT_TH,
	WF_SETTING_FILTER_LASTPLAYED_TH,

	WF_SETTING_MOD_RATING,
	WF_SETTING_MOD_SCORE,
	WF_SETTING_MOD_PLAYCOUNT,
	WF_SETTING_MOD_SKIPCOUNT,
	WF_SETTING_MOD_LASTPLAYED,
	WF_SETTING_MOD_RATING_INV,
	WF_SETTING_MOD_SCORE_INV,
	WF_SETTING_MOD_PLAYCOUNT_INV,
	WF_SETTING_MOD_SKIPCOUNT_INV,
	WF_SETTING_MOD_LASTPLAYED_INV,
	WF_SETTING_MOD_DEFAULT_RATING,
	WF_SETTING_MOD_RATING_MULTI,
	WF_SETTING_MOD_SCORE_MULTI,
	WF_SETTING_MOD_PLAYCOUNT_MULTI,
	WF_SETTING_MOD_SKIPCOUNT_MULTI,
	WF_SETTING_MOD_LASTPLAYED_MULTI,

	WF_SETTING_DEFINED /* Validation checker */
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

gboolean wf_settings_static_get_bool(WfSettingType type);
void wf_settings_static_set_bool(WfSettingType type, gboolean v_bool);
gint wf_settings_static_get_enum(WfSettingType type);
void wf_settings_static_set_enum(WfSettingType type, gint v_enum);
gint wf_settings_static_get_int(WfSettingType type);
void wf_settings_static_set_int(WfSettingType type, gint v_int);
gint64 wf_settings_static_get_int64(WfSettingType type);
void wf_settings_static_set_int64(WfSettingType type, gint64 v_int64);
guint64 wf_settings_static_get_uint64(WfSettingType type);
void wf_settings_static_set_uint64(WfSettingType type, gint64 v_uint64);
gdouble wf_settings_static_get_double(WfSettingType type);
void wf_settings_static_set_double(WfSettingType type, gdouble v_double);
const gchar * wf_settings_static_get_str(WfSettingType type);
void wf_settings_static_set_str(WfSettingType type, const gchar *v_str);

WfSongFilter * wf_settings_get_filter(void);
WfSongEntries * wf_settings_get_song_entry_modifiers(void);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

guint wf_settings_dynamic_register_bool(const gchar *name, const gchar *group, gboolean value);
guint wf_settings_dynamic_register_int(const gchar *name, const gchar *group, gint value);
guint wf_settings_dynamic_register_int64(const gchar *name, const gchar *group, gint64 value);
guint wf_settings_dynamic_register_uint64(const gchar *name, const gchar *group, guint64 value);
guint wf_settings_dynamic_register_double(const gchar *name, const gchar *group, gdouble value);
guint wf_settings_dynamic_register_str(const gchar *name, const gchar *group, const gchar *value);

gboolean wf_settings_dynamic_get_bool_by_id(guint32 id);
gint wf_settings_dynamic_get_int_by_id(guint32 id);
gint64 wf_settings_dynamic_get_int64_by_id(guint32 id);
guint64 wf_settings_dynamic_get_uint64_by_id(guint32 id);
gdouble wf_settings_dynamic_get_double_by_id(guint32 id);
const gchar * wf_settings_dynamic_get_str_by_id(guint32 id);

void wf_settings_dynamic_set_bool_by_id(guint32 id, gboolean v_bool);
void wf_settings_dynamic_set_int_by_id(guint32 id, gint v_int);
void wf_settings_dynamic_set_int64_by_id(guint32 id, gint64 v_int64);
void wf_settings_dynamic_set_uint64_by_id(guint32 id, guint64 v_uint64);
void wf_settings_dynamic_set_double_by_id(guint32 id, gdouble v_double);
void wf_settings_dynamic_set_str_by_id(guint32 id, const gchar *v_str);

gboolean wf_settings_read_file(void);
gboolean wf_settings_write(void);
void wf_settings_queue_write(void);
void wf_settings_write_if_queued(void);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */

guint32 wf_settings_get_id_from_name(const gchar *name);

/* UTILITY PROTOTYPES END */

/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_SETTINGS__ */

/* END OF FILE */
