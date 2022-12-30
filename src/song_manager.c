/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song_manager.c  This file is part of LibWoofer
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
#include <glib.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/song_manager.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/song_private.h>
#include <woofer/library.h>
#include <woofer/library_private.h>
#include <woofer/intelligence.h>
#include <woofer/intelligence_private.h>
#include <woofer/settings.h>
#include <woofer/statistics.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module manages lists with songs that have been played, are waiting to be
 * played or are queued by the user.  The list containing songs that will be
 * played is also filled when necessary by this module when already playing or
 * idling, so the software can always quickly respond with something new to
 * play, without doing a lot of calculations or anything that may lead to
 * latency.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Define the maximum amount of items to keep (0 means no limit)
#define PLAYED_ITEMS_LIMIT 100
#define PLAYED_ARTISTS_LIMIT 50

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _WfSongManagerEvents WfSongManagerEvents;
typedef struct _WfSongManagerDetails WfSongManagerDetails;

struct _WfSongManagerEvents
{
	WfFuncSongsChanged songs_changed;
};

struct _WfSongManagerDetails
{
	WfSongManagerEvents events;

	gboolean active;

	WfSong *current;

	GList *list_previous;
	GList *list_next;
	GList *list_queue;
	GList *artists;

	gboolean incognito;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_song_manager_emit_songs_changed(WfSongManagerEvents *events, WfSong *song_previous, WfSong *song_current, WfSong *song_next);

static WfSong * wf_song_manager_choose_new_song(void);
static void wf_song_manager_add_prev_song(WfSong *song);
static void wf_song_manager_rm_prev_song(WfSong *song);
static void wf_song_manager_add_next_song(WfSong *song);
static GList * wf_song_manager_add_recent_artist(GList *list, guint32 artist);

static WfSong * wf_song_manager_get_song(GList *node);
static GList * wf_song_manager_trim_list_length(GList *list, guint limit, GDestroyNotify free_func);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static WfSongManagerDetails SongManagerData = { 0 };

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_song_manager_init(void)
{
	if (SongManagerData.active)
	{
		g_warning("Module song manager is already initialized. This should not happen.");

		return;
	}

	SongManagerData.active = TRUE;
};

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

gboolean
wf_song_manager_get_incognito(void)
{
	return SongManagerData.incognito;
}

void
wf_song_manager_set_incognito(gboolean enable)
{
	SongManagerData.incognito = enable;
}

WfSong *
wf_song_manager_get_queue_song(void)
{
	GList *node;
	
	node = g_list_first(SongManagerData.list_queue);

	return wf_song_manager_get_song(node);
}

WfSong *
wf_song_manager_get_next_song(void)
{
	WfSong *song;
	GList *node;

	node = g_list_first(SongManagerData.list_next);
	song = wf_song_manager_get_song(node);

	// Remove if song is not in the library
	if (song != NULL && !wf_song_is_in_list(song))
	{
		wf_song_manager_rm_next_song(song);

		// Caution: Recursive
		song = wf_song_manager_get_next_song();
	}
	else if (song == NULL)
	{
		// Get a new song
		song = wf_song_manager_choose_new_song();
		wf_song_manager_add_next_song(song);
	}

	return song;
}

WfSong *
wf_song_manager_get_current_song(void)
{
	return SongManagerData.current;
}

WfSong *
wf_song_manager_get_prev_song(void)
{
	GList *node = g_list_first(SongManagerData.list_previous);

	return wf_song_manager_get_song(node);
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

void
wf_song_manager_connect_event_songs_changed(WfFuncSongsChanged cb_func)
{
	SongManagerData.events.songs_changed = cb_func;
}

static void
wf_song_manager_emit_songs_changed(WfSongManagerEvents *events, WfSong *song_previous, WfSong *song_current, WfSong *song_next)
{
	g_return_if_fail(events != NULL);

	if (events->songs_changed != NULL)
	{
		events->songs_changed(song_previous, song_current, song_next);
	}
}

static WfSong *
wf_song_manager_choose_new_song(void)
{
	WfSong *song;
	WfSong *current = SongManagerData.current;
	WfSongFilter *filter = wf_settings_get_filter();
	WfSongEntries *modifiers = wf_settings_get_song_entry_modifiers();
	GList *list = wf_library_get(); // Will duplicate
	GList *prev = SongManagerData.list_previous;
	GList *next = SongManagerData.list_next;
	GList *artists = SongManagerData.artists;
	guint32 artist = 0;

	if (list == NULL)
	{
		// Library is empty
		return NULL;
	}

	if (current != NULL)
	{
		artist = wf_song_get_artist_hash(current);

		// Remove current song from the list so it can never be chosen
		list = g_list_remove_all(list, current);
	}

	if (list == NULL)
	{
		// Current song is the only song present
		return NULL;
	}

	// Copy artist list and add the current one to it
	artists = g_list_copy(artists);
	artists = wf_song_manager_add_recent_artist(artists, artist);

	// Get a new song
	song = wf_intelligence_choose_new_song(&list, prev, next, artists, filter, modifiers);

	// Free the lists
	g_list_free(list);
	g_list_free(artists);

	return song;
}

void
wf_song_manager_add_queue_song(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Add a reference in case the song gets removed from the library
	g_object_ref(song);

	// Set status
	wf_song_set_queued(song, TRUE);

	// Now add to the end of the queue
	SongManagerData.list_queue = g_list_append(SongManagerData.list_queue, song);
}

void
wf_song_manager_rm_queue_song(WfSong *song)
{
	GList *node;

	if (song == NULL)
	{
		return;
	}

	// Remove the previously added reference
	g_object_unref(song);

	// Remove the first item in the queue
	SongManagerData.list_queue = g_list_remove(SongManagerData.list_queue, song);

	// Reset status if this song is not in the queue anymore
	node = g_list_find(SongManagerData.list_queue, song);

	if (node == NULL)
	{
		wf_song_set_queued(song, FALSE);
	}
}

static void
wf_song_manager_add_prev_song(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Add a reference in case the song gets removed from the library
	g_object_ref(song);

	// Add song to play history
	SongManagerData.list_previous = g_list_prepend(SongManagerData.list_previous, song);
}

static void
wf_song_manager_rm_prev_song(WfSong *song)
{
	// Remove the previously added reference
	g_object_unref(song);

	// Remove song
	SongManagerData.list_previous = g_list_remove(SongManagerData.list_previous, song);
}

static void
wf_song_manager_add_next_song(WfSong *song)
{
	if (song != NULL)
	{
		// Add a reference in case the song gets removed from the library
		g_object_ref(song);

		// Add song
		SongManagerData.list_next = g_list_append(SongManagerData.list_next, song);
	}
}

void
wf_song_manager_rm_next_song(WfSong *song)
{
	// Remove the previously added reference
	g_object_unref(song);

	// Remove song
	SongManagerData.list_next = g_list_remove(SongManagerData.list_next, song);
}

void
wf_song_manager_clear_next(void)
{
	// Free the list and drop references to the songs
	g_list_free_full(SongManagerData.list_next, g_object_unref);

	SongManagerData.list_next = NULL;
}

void
wf_song_manager_refresh_next(void)
{
	gboolean active;

	// Remove old items
	wf_song_manager_clear_next();

	// Refill next songs
	wf_song_manager_sync();

	// Report the update
	active = (SongManagerData.current != NULL);
	wf_song_manager_songs_updated(active);
}

static GList *
wf_song_manager_add_recent_artist(GList *list, guint32 artist)
{
	if (artist != 0)
	{
		// Add artist
		return g_list_prepend(list, GUINT_TO_POINTER(artist));
	}

	return NULL;
}

void
wf_song_manager_settings_updated(void)
{
	wf_song_manager_refresh_next();
}

void
wf_song_manager_songs_updated(gboolean playback_active)
{
	WfSong *prev = NULL;
	WfSong *queue = NULL;
	WfSong *next = NULL;
	WfSong *current = SongManagerData.current;

	prev = wf_song_manager_get_prev_song();
	queue = wf_song_manager_get_queue_song();

	// Only mention next song if playing
	if (playback_active)
	{
		next = wf_song_manager_get_next_song();
	}

	// Queue takes priority: if set, use that
	if (queue != NULL)
	{
		next = queue;
	}

	// Do not mention next if stop flag of the current song is set
	if (current != NULL && wf_song_get_stop_flag(current))
	{
		next = NULL;
	}

	wf_song_manager_emit_songs_changed(&SongManagerData.events, prev, current, next);
}

void
wf_song_manager_song_is_playing(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Add a reference in case the song gets removed from the library
	g_object_ref(song);

	// Set song as current
	SongManagerData.current = song;
}

void
wf_song_manager_add_played_song(WfSong *song, gdouble played_fraction, gboolean skip_score_update)
{
	guint32 artist;

	g_return_if_fail(WF_IS_SONG(song));
	g_return_if_fail(played_fraction >= 0.0 && played_fraction <= 1.0);

	if (song == NULL)
	{
		// No songs played; nothing to do
		return;
	}

	// Add song to play history
	wf_song_manager_add_prev_song(song);

	// Add artist (if known) to the artist history list
	artist = wf_song_get_artist_hash(song);
	SongManagerData.artists = wf_song_manager_add_recent_artist(SongManagerData.artists, artist);

	// Update statistics
	if (!SongManagerData.incognito)
	{
		if (!skip_score_update)
		{
			wf_stats_modify_and_update_score(song, played_fraction);
		}

		wf_stats_modify_and_update_playcount(song, played_fraction, FALSE /* decrease */);
		wf_stats_modify_and_update_skipcount(song, played_fraction, FALSE /* decrease */);
		wf_stats_modify_and_update_lastplayed(song, played_fraction, 0 /* timestamp */);

		// Write the updated file to disk when idle
		wf_library_queue_write();
	}

	// Notify that songs have updated stats (this includes updated timestamps)
	wf_library_updated_stats();

	// Reset the currently playing as this one has *been* played
	SongManagerData.current = NULL;
}

WfSong *
wf_song_manager_played_song_revert(void)
{
	/*
	 * Revert the last changes of the last played song.  In detail: get the
	 * last played song to play now and add the current playing song to the
	 * list next.
	 */
	WfSong *song;
	WfSong *current = SongManagerData.current;

	song = wf_song_manager_get_prev_song();
	wf_song_manager_rm_prev_song(song);

	wf_song_manager_add_next_song(current);

	return song;
}

/*
 * Do any operations that might block the main loop due to calculations or other
 * intensive actions.  This is expected to be run after any required quick
 * responses elsewhere in the application are finished (such as starting
 * playback)
 */
void
wf_song_manager_sync(void)
{
	WfSong *song;

	// Get a new song from the algorithm while we have the time
	if (SongManagerData.list_next == NULL)
	{
		song = wf_song_manager_choose_new_song();
		wf_song_manager_add_next_song(song);
	}

	// Write the library file if modified
	wf_library_write(FALSE);

	// Trim lists length
	SongManagerData.list_previous = wf_song_manager_trim_list_length(SongManagerData.list_previous, PLAYED_ITEMS_LIMIT, g_object_unref);
	SongManagerData.artists = wf_song_manager_trim_list_length(SongManagerData.artists, PLAYED_ARTISTS_LIMIT, NULL /* GDestroyNotify */);
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static WfSong *
wf_song_manager_get_song(GList *node)
{
	WfSong *song;

	if (node != NULL)
	{
		song = node->data;

		if (WF_IS_SONG(song))
		{
			return song;
		}
	}

	return NULL;
}

static GList *
wf_song_manager_trim_list_length(GList *list, guint limit, GDestroyNotify free_func)
{
	GList *last_node;
	GList *node;
	GList *next;
	gpointer data;

	// If nothing to do, return
	if (list == NULL || limit == 0)
	{
		return list;
	}

	// Get last node to keep
	last_node = g_list_nth(list, limit - 1);

	if (last_node == NULL)
	{
		return list;
	}
	else if (list == last_node)
	{
		// Only item in the list: do not trim
		return list;
	}

	// End list right there
	node = last_node->next;
	last_node->next = NULL;

	// Free all remaining elements
	while (node != NULL)
	{
		next = node->next;
		data = node->data;

		if (free_func != NULL)
		{
			free_func(data);
		}

		g_list_free_1(node);

		node = next;
	}

	return list;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_song_manager_finalize(void)
{
	g_list_free_full(SongManagerData.list_previous, g_object_unref);
	g_list_free_full(SongManagerData.list_next, g_object_unref);
	g_list_free_full(SongManagerData.list_queue, g_object_unref);
	g_list_free(SongManagerData.artists);

	SongManagerData = (WfSongManagerDetails) { 0 };
}

/* DESTRUCTORS END */

/* END OF FILE */
