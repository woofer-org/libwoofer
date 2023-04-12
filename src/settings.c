/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * settings.c  This file is part of LibWoofer
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
 * - For all callback functions, the suffix '_cb' is used as a quick indicator
 *   for the reader.
 * - For all return value pointers, the suffix '_rv' is used to indicate the
 *   value of the pointer can be changed by the respective function.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/settings.h>
#include <woofer/settings_private.h>

// Dependency includes
#include <woofer/intelligence.h>
#include <woofer/utils_private.h>
#include <woofer/memory.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/**
 * SECTION:settings
 * @title: Woofer Settings
 * @short_description: The application's settings manager
 *
 * The settings module is responsible for tracking all known settings and their
 * values.  The known settings are split in two parts: static and dynamic
 * settings.  Static settings are built-in and always exist.  Their value can be
 * get or set by using the settings enum.  Dynamic settings are registered
 * on-the-fly by front-ends.  All known settings (this may include settings from
 * different front-ends) are written into one settings file and are preserved
 * when reading and writing, even if the front-end changes.
 *
 * Front-end implementations are expected to register the settings they want to
 * use on application startup.  A default value should always be provided and is
 * used when the setting's value has not been set or read from the settings
 * file.  Only set values of dynamic settings when they have been changed (e.g.
 * preference update by the user), to prevent unnecessary writes to the disk.
 *
 * Note: after the file is read, settings are *extracted* into the setting value
 * structures, ready to be used; before the file is written, the values are
 * *updated* into the file reading/writing mechanism.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

/*
 * File version the software currently uses.
 *
 * This number represents the date when the way that settings are handled
 * changes significantly.  Because of this, application software that uses an
 * older version of the file, may become incompatible.  Ever since this was
 * implemented, the versions are checked at runtime and it may issue a warning
 * and ignore the content of the settings file when it is incompatible.
 *
 * Note: Please keep in mind that incompatibility warnings should only ever be
 * the case when the file was written with a newer version and then opened with
 * an older version.
 */
#define FILE_VERSION 20221201
#define FILE_MIN_VERSION 20221201

// Default out of range message
#define OUT_OF_RANGE_MESSAGE(name) (g_warning("Unable to set <%s>: value out of range", name))

// Define static names used in the key_file.
#define GROUP_PROPERTIES "Properties"
#define GROUP_GENERAL "General"
#define GROUP_FILTER "FilterOptions"
#define GROUP_MODIFIERS "ProbabilityModifiers"
#define GROUP_INTERFACE "Interface"
#define NAME_VERSION "FileVersion"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef enum _WfSettingValueType WfSettingValueType;
typedef union _WfSettingValue WfSettingValue;
typedef struct _WfSettingItem WfSettingItem;
typedef struct _WfStaticSetting WfStaticSetting;
typedef struct _WfDynamicSetting WfDynamicSetting;

typedef struct _WfSettingsDetails WfSettingsDetails;

enum _WfSettingValueType
{
	SETTING_VALUE_NONE,
	SETTING_VALUE_BOOL,
	SETTING_VALUE_ENUM,
	SETTING_VALUE_INT,
	SETTING_VALUE_INT64,
	SETTING_VALUE_UINT64,
	SETTING_VALUE_DOUBLE,
	SETTING_VALUE_STR,
	SETTING_VALUE_DEFINED // Validation checker
};

union _WfSettingValue
{
	gboolean v_bool;
	gint v_enum;
	gint v_int;
	gint64 v_int64;
	guint64 v_uint64;
	gdouble v_double;
	gchar *v_str;
};

struct _WfStaticSetting
{
	const gchar *name;
	const WfSettingType setting;
	const WfSettingValueType type;
	const WfSettingValue std;
	const WfSettingValue min;
	const WfSettingValue max;
	WfSettingValue value;
};

struct _WfDynamicSetting
{
	guint32 id;
	gchar *name;
	gchar *group;
	WfSettingValueType type;
	WfSettingValue std;
	WfSettingValue value;
};

struct _WfSettingsDetails
{
	gboolean active;

	// Custom structures used by the song choosing algorithm
	WfSongFilter filter;
	WfSongEntries entries;

	// Contains all static settings
	WfStaticSetting *general_settings;
	WfStaticSetting *filter_settings;
	WfStaticSetting *entry_settings;

	// Contains all dynamically registered settings
	GSList *registered_settings;

	// Settings properties
	gchar *default_path;
	gchar *file_path;
	GKeyFile *key_file;
	gboolean write_queued;

	// Temporary properties
	gboolean is_active; // Interface is currently active
	gboolean is_visible; // Interface is currently visible
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_settings_static_get_by_type(WfSettingType setting, WfSettingValueType type, WfSettingValue *value);
static void wf_settings_static_get_internal(WfStaticSetting *setting, WfSettingValueType type, WfSettingValue *value);

static void wf_settings_static_set_by_type(WfSettingType setting, WfSettingValueType type, WfSettingValue *value);
static void wf_settings_static_set_internal(WfStaticSetting *setting, WfSettingValueType type, WfSettingValue *value);

static void wf_settings_static_set_defaults(WfStaticSetting *setting);

static WfStaticSetting * wf_settings_static_get_struct(WfSettingType type);

static guint wf_settings_dynamic_register(const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value);
static gboolean wf_settings_dynamic_get_value_by_id(guint32 id, WfSettingValue *value_rv);
static void wf_settings_dynamic_set_value_by_id(guint32 id, WfSettingValueType type, WfSettingValue *value);

static void wf_settings_extract_keyfile(GKeyFile *key_file);
static void wf_settings_extract_static(GKeyFile *key_file, WfStaticSetting *settings, const gchar *group);
static gboolean wf_settings_extract_item(GKeyFile *key_file, const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value);

static void wf_settings_update_keyfile(GKeyFile *key_file);
static void wf_settings_update_static(GKeyFile *key_file, WfStaticSetting *settings, const gchar *group);
static void wf_settings_update_dynamic(GKeyFile *key_file, GSList *settings);
static void wf_settings_update_item(GKeyFile *key_file, const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value);

static gboolean wf_settings_process_error(const gchar *group, const gchar *key, GError **error);

static gboolean wf_settings_check_if_exists(GKeyFile *key_file, const gchar *group, const gchar *key);

static gboolean wf_settings_keyfile_write(GKeyFile *key_file, const gchar *file_path);

static void wf_settings_value_destruct(WfSettingValueType type, WfSettingValue *value);

static void wf_settings_dynamic_free_cb(gpointer data);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static WfStaticSetting GeneralSettings[] =
{
	{
		// Volume in use or last time the application operated
		"UsedVolume",
		WF_SETTING_VOLUME,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 80.0 },
		{ .v_double = 0.0 },
		{ .v_double = 100.0 },
	},
	{
		// Location prefix to use for library songs
		"SongPrefix",
		WF_SETTING_SONG_PREFIX,
		SETTING_VALUE_STR,
	},
	{
		// Interval to use to update the interface
		"UpdateInterval",
		WF_SETTING_UPDATE_INTERVAL,
		SETTING_VALUE_INT,
		{ .v_int = 100 },
		{ .v_int = 0 },
		{ .v_int = 60000 },
	},
	{
		// Prefer to read a file before playing and then play it from memory
		"PreferPlayFromRam",
		WF_SETTING_PREFER_PLAY_FROM_RAM,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		// Only update play count and last played if player more than this fraction
		"MinimumPlayedFraction",
		WF_SETTING_MIN_PLAYED_FRACTION,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 0.2 },
		{ .v_double = 0.0 },
		{ .v_double = 1.0 },
	},
	{
		// A song is said to be fully played if played more than this fraction
		"FullPlayedFraction",
		WF_SETTING_FULL_PLAYED_FRACTION,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 0.8 },
		{ .v_double = 0.0 },
		{ .v_double = 1.0 },
	},

	// Terminator
	{ NULL }
};

static WfStaticSetting FilterSettings[] =
{
	// Descriptions of these items can be found in the intelligence header

	{
		"RemoveSameRecentArtist",
		WF_SETTING_FILTER_RECENT_ARTISTS,
		SETTING_VALUE_INT,
		{ .v_int = 0 },
		{ .v_int = 0 },
		{ .v_int = 25 },
	},
	{
		"AmountOfRecentsToRemove",
		WF_SETTING_FILTER_RECENT_AMOUNT,
		SETTING_VALUE_INT,
		{ .v_int = 0 },
		{ .v_int = 0 },
		{ .v_int = 100 },
	},
	{
		"PercentageOfRecentsToRemove",
		WF_SETTING_FILTER_RECENT_PERCENTAGE,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 50.0 },
		{ .v_double = 0 },
		{ .v_double = 100.0 },
	},
	{
		"EnableRating",
		WF_SETTING_FILTER_RATING,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"EnableScore",
		WF_SETTING_FILTER_SCORE,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"EnablePlayCount",
		WF_SETTING_FILTER_PLAYCOUNT,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"EnableSkipCount",
		WF_SETTING_FILTER_SKIPCOUNT,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"EnableLastPlayed",
		WF_SETTING_FILTER_LASTPLAYED,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"RatingIncludeZero",
		WF_SETTING_FILTER_RATING_INC_ZERO,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"PlayCountInvertThreshold",
		WF_SETTING_FILTER_PLAYCOUNT_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"SkipCountInvertThreshold",
		WF_SETTING_FILTER_SKIPCOUNT_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"LastPlayedInvertThreshold",
		WF_SETTING_FILTER_LASTPLAYED_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"RatingMin",
		WF_SETTING_FILTER_RATING_MIN,
		SETTING_VALUE_INT,
		{ .v_int = 50 },
		{ .v_int = 0 },
		{ .v_int = 100 },
	},
	{
		"RatingMax",
		WF_SETTING_FILTER_RATING_MAX,
		SETTING_VALUE_INT,
		{ .v_int = 100 },
		{ .v_int = 0 },
		{ .v_int = 100 },
	},
	{
		"ScoreMin",
		WF_SETTING_FILTER_SCORE_MIN,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 25.0 },
		{ .v_double = 0.0 },
		{ .v_double = 100.0 },
	},
	{
		"ScoreMax",
		WF_SETTING_FILTER_SCORE_MAX,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 100.0 },
		{ .v_double = 0.0 },
		{ .v_double = 100.0 },
	},
	{
		"PlayCountThreshold",
		WF_SETTING_FILTER_PLAYCOUNT_TH,
		SETTING_VALUE_INT,
		{ .v_int = 0 },
		{ .v_int = 0 },
		{ .v_int = G_MAXINT },
	},
	{
		"SkipCountThreshold",
		WF_SETTING_FILTER_SKIPCOUNT_TH,
		SETTING_VALUE_INT,
		{ .v_int = 0 },
		{ .v_int = 0 },
		{ .v_int = G_MAXINT },
	},
	{
		"LastPlayedThreshold",
		WF_SETTING_FILTER_LASTPLAYED_TH,
		SETTING_VALUE_INT64,
		{ .v_int64 = 0 },
		{ .v_int64 = 0 },
		{ .v_int64 = G_MAXINT64 },
	},

	// Terminator
	{ NULL }
};

static WfStaticSetting EntrySettings[] =
{
	// Descriptions of these items can be found in the intelligence header

	{
		"RatingModifiesProbability",
		WF_SETTING_MOD_RATING,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"ScoreModifiesProbability",
		WF_SETTING_MOD_SCORE,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"PlayCountModifiesProbability",
		WF_SETTING_MOD_PLAYCOUNT,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"SkipCountModifiesProbability",
		WF_SETTING_MOD_SKIPCOUNT,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"LastPlayedModifiesProbability",
		WF_SETTING_MOD_LASTPLAYED,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"RatingInvertedProbability",
		WF_SETTING_MOD_RATING_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"ScoreInvertedProbability",
		WF_SETTING_MOD_SCORE_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"PlaycountInvertedProbability",
		WF_SETTING_MOD_PLAYCOUNT_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"SkipcountInvertedProbability",
		WF_SETTING_MOD_SKIPCOUNT_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = TRUE },
	},
	{
		"LastplayedInvertedProbability",
		WF_SETTING_MOD_LASTPLAYED_INV,
		SETTING_VALUE_BOOL,
		{ .v_bool = FALSE },
	},
	{
		"DefaultRating",
		WF_SETTING_MOD_DEFAULT_RATING,
		SETTING_VALUE_INT,
		{ .v_int = 0 },
		{ .v_int = 0 },
		{ .v_int = 100 },
	},
	{
		"RatingMultiplier",
		WF_SETTING_MOD_RATING_MULTI,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 1.0 },
		{ .v_double = 0.0 },
		{ .v_double = 10.0 },
	},
	{
		"ScoreMultiplier",
		WF_SETTING_MOD_SCORE_MULTI,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 1.0 },
		{ .v_double = 0.0 },
		{ .v_double = 10.0 },
	},
	{
		"PlayCountMultiplier",
		WF_SETTING_MOD_PLAYCOUNT_MULTI,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 1.0 },
		{ .v_double = 0.0 },
		{ .v_double = 10.0 },
	},
	{
		"SkipCountMultiplier",
		WF_SETTING_MOD_SKIPCOUNT_MULTI,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 1.0 },
		{ .v_double = 0.0 },
		{ .v_double = 10.0 },
	},
	{
		"LastPlayedMultiplier",
		WF_SETTING_MOD_LASTPLAYED_MULTI,
		SETTING_VALUE_DOUBLE,
		{ .v_double = 1.0 },
		{ .v_double = 0.0 },
		{ .v_double = 10.0 },
	},

	// Terminator
	{ NULL }
};

static WfSettingsDetails SettingsData =
{
	.general_settings = GeneralSettings,
	.filter_settings = FilterSettings,
	.entry_settings = EntrySettings,
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_settings_init(void)
{
	if (SettingsData.active)
	{
		g_warning("Module settings is already initialized. This should not happen.");

		return;
	}

	SettingsData.default_path = wf_utils_get_config_filepath(WF_SETTINGS_FILENAME, WF_TAG);

	SettingsData.general_settings = GeneralSettings;
	SettingsData.filter_settings = FilterSettings;
	SettingsData.entry_settings = EntrySettings;

	wf_settings_static_set_defaults(SettingsData.general_settings);
	wf_settings_static_set_defaults(SettingsData.filter_settings);
	wf_settings_static_set_defaults(SettingsData.entry_settings);

	SettingsData.active = TRUE;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

/**
 * wf_settings_set_file:
 * @file_path: (transfer none) (nullable): path to the settings file to use
 *
 * Sets the filepath of the settings file to use.  This file may or may not
 * actually exist; it will be (over)written whenever settings are changed and
 * applied.
 *
 * Since: 0.2
 **/
void
wf_settings_set_file(const gchar *file_path)
{
	g_free(SettingsData.file_path);

	// Copy and store new file path
	SettingsData.file_path = g_strdup(file_path);
}

/**
 * wf_settings_get_file:
 *
 * Gets the filepath of the settings file in use.
 *
 * Returns: (transfer none): path to the current settings file
 *
 * Since: 0.2
 **/
const gchar *
wf_settings_get_file(void)
{
	const gchar *file_path = SettingsData.file_path;
	const gchar *default_path = SettingsData.default_path;

	return (file_path != NULL) ? file_path : default_path;
}

static void
wf_settings_static_get_by_type(WfSettingType setting, WfSettingValueType type, WfSettingValue *value)
{
	WfStaticSetting *item = wf_settings_static_get_struct(setting);

	wf_settings_static_get_internal(item, type, value);
}

static void
wf_settings_static_get_internal(WfStaticSetting *setting, WfSettingValueType type, WfSettingValue *value)
{
	g_return_if_fail(setting != NULL);
	g_return_if_fail(value != NULL);

	// Set the default value;
	*value = setting->std;

	if (setting->type != type)
	{
		g_warning("Invalid setting type for %s", setting->name);

		return;
	}

	switch (type)
	{
		case SETTING_VALUE_NONE:
		case SETTING_VALUE_DEFINED:
			g_warn_if_reached();
			break;
		case SETTING_VALUE_BOOL:
		case SETTING_VALUE_ENUM:
		case SETTING_VALUE_INT:
		case SETTING_VALUE_INT64:
		case SETTING_VALUE_UINT64:
		case SETTING_VALUE_DOUBLE:
		case SETTING_VALUE_STR:
			*value = setting->value;
			break;
	}
}

static void
wf_settings_static_set_by_type(WfSettingType setting, WfSettingValueType type, WfSettingValue *value)
{
	WfStaticSetting *item = wf_settings_static_get_struct(setting);

	wf_settings_static_set_internal(item, type, value);
}

static void
wf_settings_static_set_internal(WfStaticSetting *setting, WfSettingValueType type, WfSettingValue *value)
{
	g_return_if_fail(setting != NULL);
	g_return_if_fail(value != NULL);

	if (setting->type != type)
	{
		g_warning("Invalid setting type for %s", setting->name);

		return;
	}

	switch (type)
	{
		case SETTING_VALUE_NONE:
		case SETTING_VALUE_DEFINED:
			g_warn_if_reached();
			break;
		case SETTING_VALUE_BOOL:
			setting->value.v_bool = (value->v_bool ? TRUE : FALSE);
			break;
		case SETTING_VALUE_ENUM:
			if (value->v_enum < setting->min.v_enum ||
			    value->v_enum > setting->max.v_enum)
			{
				OUT_OF_RANGE_MESSAGE(setting->name);
			}
			else
			{
				setting->value.v_enum = value->v_enum;
			}
			break;
		case SETTING_VALUE_INT:
			if (value->v_int < setting->min.v_int ||
			    value->v_int > setting->max.v_int)
			{
				OUT_OF_RANGE_MESSAGE(setting->name);
			}
			else
			{
				setting->value.v_int = value->v_int;
			}
			break;
		case SETTING_VALUE_INT64:
			if (value->v_int64 < setting->min.v_int64 ||
			    value->v_int64 > setting->max.v_int64)
			{
				OUT_OF_RANGE_MESSAGE(setting->name);
			}
			else
			{
				setting->value.v_int64 = value->v_int64;
			}
			break;
		case SETTING_VALUE_UINT64:
			if (value->v_uint64 < setting->min.v_uint64 ||
			    value->v_uint64 > setting->max.v_uint64)
			{
				OUT_OF_RANGE_MESSAGE(setting->name);
			}
			else
			{
				setting->value.v_uint64 = value->v_uint64;
			}
			break;
		case SETTING_VALUE_DOUBLE:
			if (value->v_double < setting->min.v_double ||
			    value->v_double > setting->max.v_double)
			{
				OUT_OF_RANGE_MESSAGE(setting->name);
			}
			else
			{
				setting->value.v_double = value->v_double;
			}
			break;
		case SETTING_VALUE_STR:
			g_free(setting->value.v_str);
			setting->value.v_str = value->v_str;
			break;
	}
}

gboolean
wf_settings_static_get_bool(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_BOOL, &value);
	return value.v_bool;
}

void
wf_settings_static_set_bool(WfSettingType type, gboolean v_bool)
{
	WfSettingValue value = { 0 };
	value.v_bool = v_bool;
	wf_settings_static_set_by_type(type, SETTING_VALUE_BOOL, &value);
}

gint
wf_settings_static_get_enum(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_ENUM, &value);
	return value.v_enum;
}

void
wf_settings_static_set_enum(WfSettingType type, gint v_enum)
{
	WfSettingValue value = { 0 };
	value.v_enum = v_enum;
	wf_settings_static_set_by_type(type, SETTING_VALUE_ENUM, &value);
}

gint
wf_settings_static_get_int(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_INT, &value);
	return value.v_int;
}

void
wf_settings_static_set_int(WfSettingType type, gint v_int)
{
	WfSettingValue value = { 0 };
	value.v_int = v_int;
	wf_settings_static_set_by_type(type, SETTING_VALUE_INT, &value);
}

gint64
wf_settings_static_get_int64(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_INT64, &value);
	return value.v_int;
}

void
wf_settings_static_set_int64(WfSettingType type, gint64 v_int64)
{
	WfSettingValue value = { 0 };
	value.v_int64 = v_int64;
	wf_settings_static_set_by_type(type, SETTING_VALUE_INT64, &value);
}

guint64
wf_settings_static_get_uint64(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_INT64, &value);
	return value.v_int64;
}

void
wf_settings_static_set_uint64(WfSettingType type, gint64 v_uint64)
{
	WfSettingValue value = { 0 };
	value.v_uint64 = v_uint64;
	wf_settings_static_set_by_type(type, SETTING_VALUE_UINT64, &value);
}

gdouble
wf_settings_static_get_double(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_DOUBLE, &value);
	return value.v_double;
}

void
wf_settings_static_set_double(WfSettingType type, gdouble v_double)
{
	WfSettingValue value = { 0 };
	value.v_double = v_double;
	wf_settings_static_set_by_type(type, SETTING_VALUE_DOUBLE, &value);
}

const gchar *
wf_settings_static_get_str(WfSettingType type)
{
	WfSettingValue value = { 0 };
	wf_settings_static_get_by_type(type, SETTING_VALUE_STR, &value);
	return value.v_str;
}

void
wf_settings_static_set_str(WfSettingType type, const gchar *v_str)
{
	WfSettingValue value = { 0 };
	value.v_str = g_strdup(v_str);
	wf_settings_static_set_by_type(type, SETTING_VALUE_STR, &value);
}

WfSongFilter *
wf_settings_get_filter(void)
{
	WfSongFilter *filter = &SettingsData.filter;
	WfStaticSetting *sett;

	// Update all values into the respective structure
	for (sett = SettingsData.filter_settings; sett != NULL && sett->name != NULL; sett++)
	{
		switch (sett->setting)
		{
			case WF_SETTING_FILTER_RECENT_ARTISTS:
				filter->recent_artists = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_RECENT_AMOUNT:
				filter->remove_recents_amount = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_RECENT_PERCENTAGE:
				filter->remove_recents_percentage = sett->value.v_double;
				break;
			case WF_SETTING_FILTER_RATING:
				filter->use_rating = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_SCORE:
				filter-> use_score= sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_PLAYCOUNT:
				filter->use_playcount = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_SKIPCOUNT:
				filter->use_skipcount = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_LASTPLAYED:
				filter->use_lastplayed = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_RATING_INC_ZERO:
				filter->rating_inc_zero = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_PLAYCOUNT_INV:
				filter->playcount_invert = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_SKIPCOUNT_INV:
				filter->skipcount_invert = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_LASTPLAYED_INV:
				filter->lastplayed_invert = sett->value.v_bool;
				break;
			case WF_SETTING_FILTER_RATING_MIN:
				filter->rating_min = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_RATING_MAX:
				filter->rating_max = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_SCORE_MIN:
				filter->score_min = sett->value.v_double;
				break;
			case WF_SETTING_FILTER_SCORE_MAX:
				filter->score_max = sett->value.v_double;
				break;
			case WF_SETTING_FILTER_PLAYCOUNT_TH:
				filter->playcount_th = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_SKIPCOUNT_TH:
				filter->skipcount_th = sett->value.v_int;
				break;
			case WF_SETTING_FILTER_LASTPLAYED_TH:
				filter->lastplayed_th = sett->value.v_int64;
				break;
			default:
				g_warning("Unsupported filter setting %s", sett->name);
		}
	}

	return filter;
}

WfSongEntries *
wf_settings_get_song_entry_modifiers(void)
{
	WfSongEntries *entries = &SettingsData.entries;
	WfStaticSetting *sett;

	// Update all values into the respective structure
	for (sett = SettingsData.entry_settings; sett != NULL && sett->name != NULL; sett++)
	{
		switch (sett->setting)
		{
			case WF_SETTING_MOD_RATING:
				entries->use_rating = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_SCORE:
				entries->use_score = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_PLAYCOUNT:
				entries->use_playcount = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_SKIPCOUNT:
				entries->use_skipcount = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_LASTPLAYED:
				entries->use_lastplayed = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_RATING_INV:
				entries->invert_rating = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_SCORE_INV:
				entries->invert_score = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_PLAYCOUNT_INV:
				entries->invert_playcount = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_SKIPCOUNT_INV:
				entries->invert_skipcount = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_LASTPLAYED_INV:
				entries->invert_lastplayed = sett->value.v_bool;
				break;
			case WF_SETTING_MOD_DEFAULT_RATING:
				entries->use_default_rating = sett->value.v_int;
				break;
			case WF_SETTING_MOD_RATING_MULTI:
				entries->rating_multiplier = sett->value.v_double;
				break;
			case WF_SETTING_MOD_SCORE_MULTI:
				entries->score_multiplier = sett->value.v_double;
				break;
			case WF_SETTING_MOD_PLAYCOUNT_MULTI:
				entries->playcount_multiplier = sett->value.v_double;
				break;
			case WF_SETTING_MOD_SKIPCOUNT_MULTI:
				entries->skipcount_multiplier = sett->value.v_double;
				break;
			case WF_SETTING_MOD_LASTPLAYED_MULTI:
				entries->lastplayed_multiplier = sett->value.v_double;
				break;
			default:
				g_warning("Unsupported entry setting %s", sett->name);
		}
	}

	return entries;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

static void
wf_settings_static_set_defaults(WfStaticSetting *setting)
{
	WfStaticSetting *sett;

	for (sett = setting; sett != NULL && sett->name != NULL; sett++)
	{
		// Now copy union content
		switch (sett->type)
		{
			case SETTING_VALUE_NONE:
			case SETTING_VALUE_DEFINED:
				g_warn_if_reached();
				break;
			case SETTING_VALUE_BOOL:
			case SETTING_VALUE_ENUM:
			case SETTING_VALUE_INT:
			case SETTING_VALUE_INT64:
			case SETTING_VALUE_UINT64:
			case SETTING_VALUE_DOUBLE:
				sett->value = sett->std;
				break;
			case SETTING_VALUE_STR:
				g_free(sett->value.v_str);
				sett->value.v_str = g_strdup(sett->std.v_str);
				break;
		}
	}
}

static WfStaticSetting *
wf_settings_static_get_struct(WfSettingType type)
{
	WfStaticSetting *sett;

	g_return_val_if_fail(type > WF_SETTING_NONE && type < WF_SETTING_DEFINED, NULL);

	for (sett = SettingsData.general_settings; sett != NULL && sett->name != NULL; sett++)
	{
		if (sett->setting == type)
		{
			return sett;
		}
	}

	for (sett = SettingsData.filter_settings; sett != NULL && sett->name != NULL; sett++)
	{
		if (sett->setting == type)
		{
			return sett;
		}
	}

	for (sett = SettingsData.entry_settings; sett != NULL && sett->name != NULL; sett++)
	{
		if (sett->setting == type)
		{
			return sett;
		}
	}

	return NULL;
}

static guint
wf_settings_dynamic_register(const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value)
{
	WfDynamicSetting *setting;
	GKeyFile *key_file = SettingsData.key_file;
	GSList *list;
	guint32 id;
	gboolean res = FALSE;

	g_return_val_if_fail(name != NULL, 0);
	g_return_val_if_fail(type != SETTING_VALUE_NONE, 0);
	g_return_val_if_fail(value != NULL, 0);

	// Generate an id from the name
	id = wf_settings_get_id_from_name(name);

	// Check each registered setting
	for (list = SettingsData.registered_settings; list != NULL; list = list->next)
	{
		setting = list->data;

		// Check if the setting with this id already exists
		if (setting != NULL && setting->id == id)
		{
			g_info("Setting with name <%s> is already registered", name);

			if (setting->type == type)
			{
				// Set new default value
				if (type == SETTING_VALUE_STR)
				{
					g_free(setting->value.v_str);
					setting->value.v_str = value->v_str;
				}
				else
				{
					setting->value = *value;
				}
			}
			else
			{
				// Type is not right; do not set the new value
				g_warning("Setting with name <%s> does not have the right type", name);
			}

			// Skip registration and return known id
			return setting->id;
		}
	}

	// Allocate new structure
	setting = (WfDynamicSetting *) g_slice_alloc0(sizeof(WfDynamicSetting));

	// Add to the registration list
	SettingsData.registered_settings = g_slist_append(SettingsData.registered_settings, setting);

	// Set basic information
	setting->id = id;
	setting->name = g_strdup(name);
	setting->type = type;
	setting->group = g_strdup((group != NULL) ? group : GROUP_INTERFACE);

	// Set default value
	setting->std = *value;

	// Get the value from the settings file, if it exists
	if (key_file != NULL)
	{
		res = wf_settings_extract_item(key_file, setting->name, setting->group, setting->type, value);
	}

	// Set current value (only copy if it a string that has not been extracted from a keyfile
	if (type == SETTING_VALUE_STR && !res)
	{
		setting->value.v_str = g_strdup(value->v_str);
	}
	else
	{
		setting->value = *value;
	}

	return id;
}

guint
wf_settings_dynamic_register_bool(const gchar *name, const gchar *group, gboolean value)
{
	WfSettingValue v;
	v.v_bool = value;
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_BOOL, &v);
}

guint
wf_settings_dynamic_register_int(const gchar *name, const gchar *group, gint value)
{
	WfSettingValue v;
	v.v_int = value;
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_INT, &v);
}

guint
wf_settings_dynamic_register_int64(const gchar *name, const gchar *group, gint64 value)
{
	WfSettingValue v;
	v.v_int64 = value;
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_INT64, &v);
}

guint
wf_settings_dynamic_register_uint64(const gchar *name, const gchar *group, guint64 value)
{
	WfSettingValue v;
	v.v_uint64 = value;
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_UINT64, &v);
}

guint
wf_settings_dynamic_register_double(const gchar *name, const gchar *group, gdouble value)
{
	WfSettingValue v;
	v.v_double = value;
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_DOUBLE, &v);
}

guint
wf_settings_dynamic_register_str(const gchar *name, const gchar *group, const gchar *value)
{
	WfSettingValue v;
	v.v_str = g_strdup(value);
	return wf_settings_dynamic_register(name, group, SETTING_VALUE_STR, &v);
}

static gboolean
wf_settings_dynamic_get_value_by_id(guint32 id, WfSettingValue *value_rv)
{
	WfDynamicSetting *setting;
	GSList *list;

	g_return_val_if_fail(id != 0, FALSE);
	g_return_val_if_fail(value_rv != NULL, FALSE);

	for (list = SettingsData.registered_settings; list != NULL; list = list->next)
	{
		setting = list->data;

		if (setting != NULL && setting->id == id)
		{
			*value_rv = setting->value;

			return TRUE;
		}
	}

	g_warning("Setting with id <%d> not found", id);

	return FALSE;
}

gboolean
wf_settings_dynamic_get_bool_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_bool : 0;
}

gint
wf_settings_dynamic_get_int_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_int : 0;
}

gint64
wf_settings_dynamic_get_int64_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_int64 : 0;
}

guint64
wf_settings_dynamic_get_uint64_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_uint64 : 0;
}

gdouble
wf_settings_dynamic_get_double_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_double : 0.0;
}

const gchar *
wf_settings_dynamic_get_str_by_id(guint32 id)
{
	WfSettingValue value;
	gboolean res = wf_settings_dynamic_get_value_by_id(id, &value);
	return res ? value.v_str : NULL;
}

static void
wf_settings_dynamic_set_value_by_id(guint32 id, WfSettingValueType type, WfSettingValue *value)
{
	WfDynamicSetting *setting;
	GSList *list;

	g_return_if_fail(id != 0);
	g_return_if_fail(value != NULL);
	g_return_if_fail(type != SETTING_VALUE_NONE);

	for (list = SettingsData.registered_settings; list != NULL; list = list->next)
	{
		setting = list->data;

		if (setting != NULL && setting->id == id)
		{
			if (type == SETTING_VALUE_STR)
			{
				g_free(setting->value.v_str);
				setting->value.v_str = g_strdup(value->v_str);
			}
			else
			{
				setting->value = *value;
			}

			return;
		}
	}

	g_warning("Setting with id <%d> not found", id);
}

void
wf_settings_dynamic_set_bool_by_id(guint32 id, gboolean v_bool)
{
	WfSettingValue value;
	value.v_bool = v_bool;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_BOOL, &value);
}

void
wf_settings_dynamic_set_int_by_id(guint32 id, gint v_int)
{
	WfSettingValue value;
	value.v_int = v_int;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_INT, &value);
}

void
wf_settings_dynamic_set_int64_by_id(guint32 id, gint64 v_int64)
{
	WfSettingValue value;
	value.v_int64 = v_int64;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_INT64, &value);
}

void
wf_settings_dynamic_set_uint64_by_id(guint32 id, guint64 v_uint64)
{
	WfSettingValue value;
	value.v_uint64 = v_uint64;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_UINT64, &value);
}

void
wf_settings_dynamic_set_double_by_id(guint32 id, gdouble v_double)
{
	WfSettingValue value;
	value.v_double = v_double;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_DOUBLE, &value);
}

void
wf_settings_dynamic_set_str_by_id(guint32 id, const gchar *v_str)
{
	WfSettingValue value;
	value.v_str = (gchar *) v_str;
	wf_settings_dynamic_set_value_by_id(id, SETTING_VALUE_STR, &value);
}

static void
wf_settings_extract_keyfile(GKeyFile *key_file)
{
	gint version = 0;

	g_return_if_fail(key_file != NULL);

	// Incompatibility check
	version = g_key_file_get_integer(key_file, GROUP_PROPERTIES, NAME_VERSION, NULL /* GError */);

	if (version <= 0)
	{
		// No version available; continue
	}
	else if (version < FILE_VERSION)
	{
		if (version < FILE_MIN_VERSION)
		{
			// Older version; incompatible
			g_message("Settings file is written with an older version of the software "
			          "that is incompatible with this version");
			return;
		}
		else
		{
			// Older version (note that the version will be updated before writing)
			g_info("Settings file is written with an older version of the software");
		}
	}
	else if (version > FILE_VERSION)
	{
		// Newer version
		g_message("Settings file is written with a newer version of the software. "
		          "Refusing to parse settings to prevent any glitches or unexpected behavior.");

		return;
	}
	else // version == FILE_VERSION
	{
		// Compatible; nothing to report
	}

	// Now extract the values from the key file
	wf_settings_extract_static(key_file, SettingsData.general_settings, GROUP_GENERAL);
	wf_settings_extract_static(key_file, SettingsData.filter_settings, GROUP_FILTER);
	wf_settings_extract_static(key_file, SettingsData.entry_settings, GROUP_MODIFIERS);

	// In case values are altered (e.g. they were out of range), re-write the file
	wf_settings_write_if_queued();
}

static void
wf_settings_extract_static(GKeyFile *key_file, WfStaticSetting *settings, const gchar *group)
{
	WfStaticSetting *sett;

	g_return_if_fail(key_file != NULL);
	g_return_if_fail(settings != NULL);
	g_return_if_fail(group != NULL);

	for (sett = settings; sett != NULL && sett->name != NULL; sett++)
	{
		// Get value
		wf_settings_extract_item(key_file, sett->name, group, sett->type, &sett->value);
	}
}

static gboolean
wf_settings_extract_item(GKeyFile *key_file, const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value)
{
	gboolean v_bool;
	gint v_enum;
	gint v_int;
	gint64 v_int64;
	guint64 v_uint64;
	gdouble v_double;
	gchar *str = NULL;
	GError *err = NULL;

	g_return_val_if_fail(key_file != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(type != SETTING_VALUE_NONE, FALSE);

	if (!wf_settings_check_if_exists(key_file, group, name))
	{
		return FALSE;
	}

	// Reset union data
	memset(value, 0, sizeof(*value));

	switch (type)
	{
		case SETTING_VALUE_BOOL:
			v_bool = g_key_file_get_boolean(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_bool = v_bool;
			}
			break;
		case SETTING_VALUE_ENUM:
			v_enum = g_key_file_get_integer(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_enum = v_enum;
			}
			break;
		case SETTING_VALUE_INT:
			v_int = g_key_file_get_integer(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_int = v_int;
			}
			break;
		case SETTING_VALUE_INT64:
			v_int64 = g_key_file_get_int64(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_int64 = v_int64;
			}
			break;
		case SETTING_VALUE_UINT64:
			v_uint64 = g_key_file_get_uint64(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_uint64 = v_uint64;
			}
			break;
		case SETTING_VALUE_DOUBLE:
			v_double = g_key_file_get_double(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_double = v_double;
			}
			break;
		case SETTING_VALUE_STR:
			str = g_key_file_get_string(key_file, group, name, &err);
			if (err == NULL)
			{
				value->v_str = str;
			}
			break;
		default:
			g_warning("Invalid type for key %s (%s)", name, group);
	}

	if (err == NULL)
	{
		return TRUE;
	}
	else
	{
		wf_settings_process_error(group, name, &err);

		return FALSE;
	}
}

static void
wf_settings_update_keyfile(GKeyFile *key_file)
{
	g_return_if_fail(key_file != NULL);

	// Update settings file version
	g_key_file_set_integer(key_file, GROUP_PROPERTIES, NAME_VERSION, FILE_VERSION);

	// Update all static settings groups
	wf_settings_update_static(key_file, SettingsData.general_settings, GROUP_GENERAL);
	wf_settings_update_static(key_file, SettingsData.filter_settings, GROUP_FILTER);
	wf_settings_update_static(key_file, SettingsData.entry_settings, GROUP_MODIFIERS);

	// Update all dynamic settings
	wf_settings_update_dynamic(key_file, SettingsData.registered_settings);
}

static void
wf_settings_update_static(GKeyFile *key_file, WfStaticSetting *settings, const gchar *group)
{
	WfStaticSetting *sett;
	WfSettingValue value;

	g_return_if_fail(key_file != NULL);
	g_return_if_fail(settings != NULL);
	g_return_if_fail(group != NULL);

	for (sett = settings; sett != NULL && sett->name != NULL; sett++)
	{
		// Reset union data
		memset(&value, 0, sizeof(value));

		// Get setting value
		wf_settings_static_get_internal(sett, sett->type, &value);

		// Update value
		wf_settings_update_item(key_file, sett->name, group, sett->type, &value);
	}
}

static void
wf_settings_update_dynamic(GKeyFile *key_file, GSList *settings)
{
	WfSettingValue value;
	WfDynamicSetting *sett;
	GSList *list;

	g_return_if_fail(key_file != NULL);

	if (settings == NULL)
	{
		return;
	}

	for (list = settings; list != NULL; list = list->next)
	{
		sett = list->data;

		// Reset union data
		memset(&value, 0, sizeof(value));

		// Update value
		wf_settings_update_item(key_file, sett->name, sett->group, sett->type, &sett->value);
	}
}

static void
wf_settings_update_item(GKeyFile *key_file, const gchar *name, const gchar *group, WfSettingValueType type, WfSettingValue *value)
{
	const gchar *str;

	g_return_if_fail(key_file != NULL);
	g_return_if_fail(name != NULL);
	g_return_if_fail(value != NULL);

	switch (type)
	{
		case SETTING_VALUE_BOOL:
			g_key_file_set_boolean(key_file, group, name, value->v_bool);
			break;
		case SETTING_VALUE_ENUM:
			g_key_file_set_integer(key_file, group, name, value->v_enum);
			break;
		case SETTING_VALUE_INT:
			g_key_file_set_integer(key_file, group, name, value->v_int);
			break;
		case SETTING_VALUE_INT64:
			g_key_file_set_int64(key_file, group, name, value->v_int64);
			break;
		case SETTING_VALUE_UINT64:
			g_key_file_set_uint64(key_file, group, name, value->v_uint64);
			break;
		case SETTING_VALUE_DOUBLE:
			g_key_file_set_double(key_file, group, name, value->v_double);
			break;
		case SETTING_VALUE_STR:
			str = (value->v_str != NULL) ? value->v_str : "";
			g_key_file_set_string(key_file, group, name, str);
			break;
		default:
			g_warning("Invalid type for key %s (%s)", name, group);
	}
}

gboolean
wf_settings_read_file(void)
{
	const gchar *file = wf_settings_get_file();
	GError *err = NULL;

	g_return_val_if_fail(file != NULL, FALSE);

	// Clear the old key file
	wf_memory_clear_key_file(&SettingsData.key_file);

	// Create a new key file
	SettingsData.key_file = g_key_file_new();

	// Attempt to read the settings file
	if (!g_key_file_load_from_file(SettingsData.key_file, file, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		if (err == NULL)
		{
			g_warning("Could not open file %s due to an unknown error", file);
		}
		else
		{
			g_info("Could not open file %s: %s", file, err->message);
			g_error_free(err);
		}

		return FALSE;
	}
	else if (err != NULL)
	{
		g_info("Successfully opened file, but received an error: %s", err->message);
		g_error_free(err);
	}

	// Now extract the known settings from the key file
	wf_settings_extract_keyfile(SettingsData.key_file);

	return TRUE;
}

gboolean
wf_settings_write(void)
{
	const gchar *file = wf_settings_get_file();

	if (SettingsData.key_file == NULL)
	{
		SettingsData.key_file = g_key_file_new();
	}

	wf_settings_update_keyfile(SettingsData.key_file);

	if (wf_settings_keyfile_write(SettingsData.key_file, file))
	{
		SettingsData.write_queued = FALSE;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void
wf_settings_queue_write(void)
{
	SettingsData.write_queued = TRUE;
}

void
wf_settings_write_if_queued(void)
{
	if (SettingsData.write_queued)
	{
		// This will reset "write_queued" to FALSE if write succeed
		wf_settings_write();
	}
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

guint32
wf_settings_get_id_from_name(const gchar *name)
{
	// Adapted from GLib-2.0:g_str_hash

	const gchar *p;
	gchar ch;
	guint32 x = 5381;

	for (p = name; *p != '\0'; p++)
	{
		ch = *p;

		x = (x << 5) + x + ch;
	}

	return x;
}

static gboolean
wf_settings_process_error(const gchar *group, const gchar *key, GError **error)
{
	g_return_val_if_fail(error != NULL, FALSE);

	if (*error == NULL)
	{
		return TRUE;
	}

	GError *g_err;
	g_err = *error;

	// Print an error message, with the information available
	if (group == NULL && key == NULL)
	{
		g_warning("An error occurred while parsing the settings file: %s", g_err->message);
	}
	else if (group == NULL && key != NULL)
	{
		g_warning("An error occurred while parsing key %s: %s", key, g_err->message);
	}
	else if (group != NULL && key == NULL)
	{
		g_warning("An error occurred while parsing a key from group %s: %s", group, g_err->message);
	}
	else
	{
		g_warning("An error occurred while parsing key %s from group %s: %s", key, group, g_err->message);
	}

	g_error_free(*error);
	*error = NULL; // Set it to NULL so it can be reused

	return FALSE;
}

static gboolean
wf_settings_check_if_exists(GKeyFile *key_file, const gchar *group, const gchar *key)
{
	GError *err = NULL;

	g_return_val_if_fail(key_file != NULL, FALSE);

	if (group == NULL || key == NULL)
	{
		g_info("No group or key to check for existence");

		return FALSE;
	}

	if (!g_key_file_has_key(key_file, group, key, &err))
	{
		if (err == NULL)
		{
			g_info("Key %s from group %s does not seem to exist in the settings file", key, group);
		}
		else
		{
			g_info("Key %s from group %s does not seem to exist in the settings file (%s)", key, group, err->message);

			g_error_free(err);
		}

		return FALSE;
	}
	else if (err != NULL)
	{
		g_warning("Key %s from group %s seem to exist, but an error has been reported: %s", key, group, err->message);

		g_error_free(err);

		return FALSE;
	}

	return TRUE;
}

static gboolean
wf_settings_keyfile_write(GKeyFile *key_file, const gchar *file_path)
{
	GError *err = NULL;
	gboolean res = FALSE;

	if (wf_utils_save_file_to_disk(key_file, file_path, &err))
	{
		g_info("Successfully written settings to disk");

		res = TRUE;
	}
	else
	{
		g_warning("Failed to write settings to disk: %s", err->message);
	}

	g_clear_error(&err);

	return res;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

static void
wf_settings_value_destruct(WfSettingValueType type, WfSettingValue *value)
{
	if (value != NULL && type == SETTING_VALUE_STR)
	{
		g_free(value->v_str);
	}
}

static void
wf_settings_dynamic_free_cb(gpointer data)
{
	WfDynamicSetting *setting = data;

	g_return_if_fail(setting != NULL);

	wf_settings_value_destruct(setting->type, &setting->std);
	wf_settings_value_destruct(setting->type, &setting->value);

	g_free(setting->name);
	g_free(setting->group);

	g_slice_free1(sizeof(WfDynamicSetting), setting);
}

void
wf_settings_finalize(void)
{
	// Write any made changes to disk
	wf_settings_write_if_queued();

	// Free file data
	g_free(SettingsData.file_path);
	g_free(SettingsData.default_path);
	wf_memory_clear_key_file(&SettingsData.key_file);

	// Free all dynamically registered settings
	g_slist_free_full(SettingsData.registered_settings, wf_settings_dynamic_free_cb);

	// Reset all data
	SettingsData = (WfSettingsDetails) { 0 };
}

/* DESTRUCTORS END */

/* END OF FILE */
