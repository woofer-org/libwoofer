/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * static/gdbus.c  This file is part of LibWoofer
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

/* INCLUDES BEGIN */

#include <glib.h>
#include <gio/gio.h>

#include <woofer/static/gdbus.h>

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * Auto generated structures defining the GDBus Remote Interface.  Do not modify
 * by hand.
 *
 * https://github.com/quicoa/gdbus_introspection_to_code
 */

/* DESCRIPTION END */

/* GDBUS INTERFACE BEGIN */

// Methods for org_woofer_app

// Method Quit
static GDBusMethodInfo wf_org_woofer_app_method_quit =
{
	-1,
	"Quit",
	NULL,
	NULL,
	NULL
};

// Method Raise
static GDBusMethodInfo wf_org_woofer_app_method_raise =
{
	-1,
	"Raise",
	NULL,
	NULL,
	NULL
};

// Arguments refreshmetadata for out

// Argument Amount
static GDBusArgInfo wf_refreshmetadata_arg_amount_out =
{
	-1,
	"Amount",
	"i",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_refreshmetadata_arg_out_pointers[] =
{
	&wf_refreshmetadata_arg_amount_out,
	NULL
};

// Method RefreshMetadata
static GDBusMethodInfo wf_org_woofer_app_method_refreshmetadata =
{
	-1,
	"RefreshMetadata",
	NULL,
	wf_refreshmetadata_arg_out_pointers,
	NULL
};

// Arguments addsong for in

// Argument URI
static GDBusArgInfo wf_addsong_arg_uri_in =
{
	-1,
	"URI",
	"s",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_addsong_arg_in_pointers[] =
{
	&wf_addsong_arg_uri_in,
	NULL
};

// Arguments addsong for out

// Argument Added
static GDBusArgInfo wf_addsong_arg_added_out =
{
	-1,
	"Added",
	"i",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_addsong_arg_out_pointers[] =
{
	&wf_addsong_arg_added_out,
	NULL
};

// Method AddSong
static GDBusMethodInfo wf_org_woofer_app_method_addsong =
{
	-1,
	"AddSong",
	wf_addsong_arg_in_pointers,
	wf_addsong_arg_out_pointers,
	NULL
};

// Array with method pointers
static GDBusMethodInfo * wf_org_woofer_app_method_pointers[] =
{
	&wf_org_woofer_app_method_quit,
	&wf_org_woofer_app_method_raise,
	&wf_org_woofer_app_method_refreshmetadata,
	&wf_org_woofer_app_method_addsong,
	NULL
};

// Signals for org_woofer_app

// Interface org.woofer.app
static GDBusInterfaceInfo wf_org_woofer_app_interface =
{
	-1,
	"org.woofer.app",
	wf_org_woofer_app_method_pointers,
	NULL,
	NULL,
	NULL
};

GDBusInterfaceInfo *
wf_org_woofer_app_get_interface_info(void)
{
	return &wf_org_woofer_app_interface;
}

/* GDBUS INTERFACE END */

/* GDBUS INTERFACE BEGIN */

// Methods for org_woofer_player

// Arguments setplaying for in

// Argument Song
static GDBusArgInfo wf_setplaying_arg_song_in =
{
	-1,
	"Song",
	"u",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_setplaying_arg_in_pointers[] =
{
	&wf_setplaying_arg_song_in,
	NULL
};

// Method SetPlaying
static GDBusMethodInfo wf_org_woofer_player_method_setplaying =
{
	-1,
	"SetPlaying",
	wf_setplaying_arg_in_pointers,
	NULL,
	NULL
};

// Arguments setqueue for in

// Argument Song
static GDBusArgInfo wf_setqueue_arg_song_in =
{
	-1,
	"Song",
	"u",
	NULL
};

// Argument Queue
static GDBusArgInfo wf_setqueue_arg_queue_in =
{
	-1,
	"Queue",
	"b",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_setqueue_arg_in_pointers[] =
{
	&wf_setqueue_arg_song_in,
	&wf_setqueue_arg_queue_in,
	NULL
};

// Method SetQueue
static GDBusMethodInfo wf_org_woofer_player_method_setqueue =
{
	-1,
	"SetQueue",
	wf_setqueue_arg_in_pointers,
	NULL,
	NULL
};

// Arguments stopaftersong for in

// Argument Song
static GDBusArgInfo wf_stopaftersong_arg_song_in =
{
	-1,
	"Song",
	"u",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_stopaftersong_arg_in_pointers[] =
{
	&wf_stopaftersong_arg_song_in,
	NULL
};

// Method StopAfterSong
static GDBusMethodInfo wf_org_woofer_player_method_stopaftersong =
{
	-1,
	"StopAfterSong",
	wf_stopaftersong_arg_in_pointers,
	NULL,
	NULL
};

// Arguments seek for in

// Argument Percentage
static GDBusArgInfo wf_seek_arg_percentage_in =
{
	-1,
	"Percentage",
	"d",
	NULL
};

// Array with argument pointers
static GDBusArgInfo * wf_seek_arg_in_pointers[] =
{
	&wf_seek_arg_percentage_in,
	NULL
};

// Method Seek
static GDBusMethodInfo wf_org_woofer_player_method_seek =
{
	-1,
	"Seek",
	wf_seek_arg_in_pointers,
	NULL,
	NULL
};

// Method Play
static GDBusMethodInfo wf_org_woofer_player_method_play =
{
	-1,
	"Play",
	NULL,
	NULL,
	NULL
};

// Method Pause
static GDBusMethodInfo wf_org_woofer_player_method_pause =
{
	-1,
	"Pause",
	NULL,
	NULL,
	NULL
};

// Method PlayPause
static GDBusMethodInfo wf_org_woofer_player_method_playpause =
{
	-1,
	"PlayPause",
	NULL,
	NULL,
	NULL
};

// Method Backward
static GDBusMethodInfo wf_org_woofer_player_method_backward =
{
	-1,
	"Backward",
	NULL,
	NULL,
	NULL
};

// Method Forward
static GDBusMethodInfo wf_org_woofer_player_method_forward =
{
	-1,
	"Forward",
	NULL,
	NULL,
	NULL
};

// Method Stop
static GDBusMethodInfo wf_org_woofer_player_method_stop =
{
	-1,
	"Stop",
	NULL,
	NULL,
	NULL
};

// Array with method pointers
static GDBusMethodInfo * wf_org_woofer_player_method_pointers[] =
{
	&wf_org_woofer_player_method_setplaying,
	&wf_org_woofer_player_method_setqueue,
	&wf_org_woofer_player_method_stopaftersong,
	&wf_org_woofer_player_method_seek,
	&wf_org_woofer_player_method_play,
	&wf_org_woofer_player_method_pause,
	&wf_org_woofer_player_method_playpause,
	&wf_org_woofer_player_method_backward,
	&wf_org_woofer_player_method_forward,
	&wf_org_woofer_player_method_stop,
	NULL
};

// Signals for org_woofer_player

// Properties for org_woofer_player

// Property SongPrevious
static GDBusPropertyInfo wf_org_woofer_player_property_songprevious =
{
	-1,
	"SongPrevious",
	"u",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
	NULL
};

// Property SongPlaying
static GDBusPropertyInfo wf_org_woofer_player_property_songplaying =
{
	-1,
	"SongPlaying",
	"u",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
	NULL
};

// Property SongNext
static GDBusPropertyInfo wf_org_woofer_player_property_songnext =
{
	-1,
	"SongNext",
	"u",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE,
	NULL
};

// Property Incognito
static GDBusPropertyInfo wf_org_woofer_player_property_incognito =
{
	-1,
	"Incognito",
	"b",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
	NULL
};

// Property Volume
static GDBusPropertyInfo wf_org_woofer_player_property_volume =
{
	-1,
	"Volume",
	"d",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
	NULL
};

// Property Position
static GDBusPropertyInfo wf_org_woofer_player_property_position =
{
	-1,
	"Position",
	"d",
	G_DBUS_PROPERTY_INFO_FLAGS_READABLE | G_DBUS_PROPERTY_INFO_FLAGS_WRITABLE,
	NULL
};

// Array with property pointers
static GDBusPropertyInfo * wf_org_woofer_player_property_pointers[] =
{
	&wf_org_woofer_player_property_songprevious,
	&wf_org_woofer_player_property_songplaying,
	&wf_org_woofer_player_property_songnext,
	&wf_org_woofer_player_property_incognito,
	&wf_org_woofer_player_property_volume,
	&wf_org_woofer_player_property_position,
	NULL
};

// Interface org.woofer.player
static GDBusInterfaceInfo wf_org_woofer_player_interface =
{
	-1,
	"org.woofer.player",
	wf_org_woofer_player_method_pointers,
	NULL,
	wf_org_woofer_player_property_pointers,
	NULL
};

GDBusInterfaceInfo *
wf_org_woofer_player_get_interface_info(void)
{
	return &wf_org_woofer_player_interface;
}

/* GDBUS INTERFACE END */

/* END OF FILE */
