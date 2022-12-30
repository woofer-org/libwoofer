/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * static/options.c  This file is part of LibWoofer
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

#include <glib.h>

#include <woofer/static/options.h>

#include <woofer/settings.h>
#include <woofer/library.h>
#include <woofer/constants.h>

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * All application related command-line options are defined here as an array of
 * GOptionEntry items.  The structure is filled according to the GIO
 * documentation as follows:
 * - Long name (used with double dashes)
 * - Short name (used with a single dash)
 * - One or more option flags
 * - Type of argument
 * - Return location or callback to use (depending on the argument type)
 * - Entry description shown in the help overview
 * - Argument description shown in the help overview (depending on the
 * 	argument type)
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Playback options
#define OPTION_PLAY_PAUSE "play-pause"
#define OPTION_PLAY "play"
#define OPTION_PAUSE "pause"
#define OPTION_STOP "stop"
#define OPTION_PREVIOUS "previous"
#define OPTION_NEXT "next"

/* DEFINES END */

/* APPLICATION OPTIONS BEGIN */

// Structure for option return values
static WfApplicationEntries AppEntries;

/*
 * These options, even if supplied, might not have any effect if using
 * for example GTK, as it may also register an option with any of these
 * names and use it to override any other properties set by the
 * application or interface.  But in case it does not, register it here
 * so it can be used to manipulate information shown by the
 * desktop environment.
 */
static const GOptionEntry MainOptions[] =
{
	{
		"name", '\0', G_OPTION_FLAG_HIDDEN,
		G_OPTION_ARG_STRING, &AppEntries.name,
		"Use this string as application name",
		"name"
	},
	{
		"icon", '\0', G_OPTION_FLAG_HIDDEN,
		G_OPTION_ARG_STRING, &AppEntries.icon,
		"Use this string as the icon name of the graphical interface",
		"icon"
	},
	{
		"desktop_entry", '\0', G_OPTION_FLAG_HIDDEN,
		G_OPTION_ARG_STRING, &AppEntries.desktop_entry,
		"Use this string as the desktop entry filename",
		"name"
	},

	// Terminator
	{ NULL }
};

/*
 * These are the normal, visible options that are shown in the help overview.
 */
static const GOptionEntry AppOptions[] =
{
	// Hidden options
	{
		"shortlist", '\0', G_OPTION_FLAG_HIDDEN,
		G_OPTION_ARG_NONE, &AppEntries.shortlist,
		"Print all available options and exit",
		NULL
	},

	// Startup application options (primary instance)
	{
		"config", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING, &AppEntries.config,
		"Provide a location for the configuration file to use "
		"('~/.config/" WF_TAG "/" WF_SETTINGS_FILENAME "' by default)",
		"filepath"
	},
	{
		"library", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING, &AppEntries.library,
		"Provide a location for the library file to use "
		"('~/.config/" WF_TAG "/" WF_LIBRARY_FILENAME "' by default)",
		"filepath"
	},
	{
		"background", 'b', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.background,
		"Start the application in the background (do not show main window on startup)",
		NULL
	},

	// Runtime application options (after startup or remote activation)
	{
		"play-pause", 'p', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.play_pause,
		"Play or pause playback the main instance. "
		"If not running, start playback after startup",
		NULL
	},
	{
		"play", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.play,
		"Start playback in the main instance",
		NULL
	},
	{
		"pause", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.pause,
		"Pause playback in the main instance",
		NULL
	},
	{
		"stop", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.stop,
		"Stop playback in the main instance",
		NULL
	},
	{
		"previous", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.previous,
		"Play previous song in the main instance",
		NULL
	},
	{
		"next", 'n', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.next,
		"Play next song in the main instance",
		NULL
	},

	// Miscellaneous options
	{
		"version", 'V', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &AppEntries.version,
		"Show the application version and exit",
		NULL
	},

	// Terminator
	{ NULL }
};

/* APPLICATION OPTIONS END */

/* MODULE FUNCTIONS BEGIN */

const WfApplicationEntries *
wf_option_entries_get_entries(void)
{
	return &AppEntries;
}

const GOptionEntry *
wf_option_entries_get_main(void)
{
	return MainOptions;
}

const GOptionEntry *
wf_option_entries_get_app(void)
{
	return AppOptions;
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
