/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * intelligence.c  This file is part of LibWoofer
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
#include <math.h>
#include <glib.h>

// Global includes
#include <woofer/constants.h>

// Module includes
#include <woofer/intelligence.h>
#include <woofer/intelligence_private.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/statistics.h>
#include <woofer/utils.h>
#include <woofer/dlist.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/**
 * SECTION:intelligence
 * @title: Woofer Intelligence
 * @short_description: The application's song choosing algorithm
 *
 * Name it intelligence, name it an algorithm, whatever you name it;
 * the 'smart' part of the player is referred to as 'intelligence'
 * and that's written in this file.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

/*
 * Multipliers to use on top of user defined multipliers.
 * Using a default value of 1000 keeps precision without using doubles as it can
 * be hard to detect zero values in a double.
 */
#define MULTIPLIER_RATING 1000
#define MULTIPLIER_SCORE 1000
#define MULTIPLIER_PLAYCOUNT 1000
#define MULTIPLIER_SKIPCOUNT 1000
#define MULTIPLIER_LASTPLAYED 1000

// Defines the minimum and maximum that may be added to the entries of the song
#define MIN_SONG_ENTRIES 0
#define MAX_SONG_ENTRIES 100

// Define a separate log domain so logging can easily be disabled
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN WF_TAG "-intelligence"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _WfIntelligenceContainer WfIntelligenceContainer;

// Container used for functions part of the song picker.
struct _WfIntelligenceContainer
{
	gint total_entries;

	gint default_rating;

	gboolean favor_low_ratings;
	gint rating_factor;

	gboolean favor_low_scores;
	gint score_factor;

	gboolean favor_low_playcount;
	gint playcount_factor;

	gboolean favor_low_skipcount;
	gint skipcount_factor;

	gboolean favor_low_lastplayed;
	gint lastplayed_factor;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static GList * wf_intelligence_remove_invalid_songs(GList *library);
static GList * wf_intelligence_filter_by_stats(GList *library, WfSongFilter *filter);
static GList * wf_intelligence_remove_songs_with_artists(GList *library, GList *artists, gint amount);
static GList * wf_intelligence_remove_recents(GList *library, GList *list_prev, GList *list_next, gint amount);
static gboolean wf_intelligence_determine_modifiers(WfIntelligenceContainer *container, WfSongEntries *preferences);
static WfDList * wf_intelligence_calculate_song_entries(WfIntelligenceContainer *container, GList *songs);
static WfSong * wf_intelligence_pick_winner(WfIntelligenceContainer *container, WfDList *list);

static guint wf_intelligence_random(gint lower, gint upper);
static gint wf_intelligence_get_percentage_of_list(GList *list, gdouble percentage);

static gboolean wf_intelligence_use_rating_filter(gboolean use, gint rating_min, gint rating_max);
static gboolean wf_intelligence_use_score_filter(gboolean use, gdouble score_min, gdouble score_max);
static gboolean wf_intelligence_use_playcount_filter(gboolean use, gint playcount_th);
static gboolean wf_intelligence_use_skipcount_filter(gboolean use, gint skipcount_th);
static gboolean wf_intelligence_use_lastplayed_filter(gboolean use, gint64 lastplayed_th);

static gint wf_intelligence_sort_compare(gconstpointer a, gconstpointer b);

static gint wf_intelligence_calculate_entries_with_fraction(const gint x, const gint a, const gint r, const gboolean invert);
static gint wf_intelligence_calculate_entries_with_sqrt(const gint x, const gint a, const gint r, const gboolean invert);

static gint wf_intelligence_get_entries_count(gint count);
static gint wf_intelligence_get_entries_count_inverted(gint count);
static gint wf_intelligence_get_entries_time_since(gint64 time_since);
static gint wf_intelligence_get_entries_time_since_inverted(gint64 time_since);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */
/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

/*
 * wf_intelligence_filter:
 * @available_songs: (element-type WfSong): full list with songs to filter
 * @previous_songs: (element-type WfSong): list of previously played songs
 * @play_next: (element-type WfSong): list of songs selected to play next
 * @recent_artists: (element-type guint32): list of recent artists
 * @filer: filter parameters to use
 *
 * Use the filtering part of the algorithm which involves filtering the list of
 * songs using a provided filter.
 *
 * Returns: a (potentially filtered) @available_songs
 */
GList *
wf_intelligence_filter(GList *available_songs,
                       GList *previous_songs,
                       GList *play_next,
                       GList *recent_artists,
                       WfSongFilter *filter)
{
	GList *filtered_songs;
	gint remove_recent;

	// Precondition checks
	if (available_songs == NULL)
	{
		g_warning("No songs to filter (empty list)");

		return NULL;
	}
	else if (filter == NULL)
	{
		g_warning("Nothing to filter (empty filter structure)");

		return available_songs;
	}

	filtered_songs = available_songs;

	/*
	 * Filtering process starts here.
	 *
	 * The order of these operations is interesting but very important.
	 * Say there are 10 songs in the list, amount to remove is at 3,
	 * percentage to remove is at 50 and 1 song matches the filters.  If the
	 * filters and amount to remove are handled first, the percentage
	 * operator only removes 3 items (10 - 1 - 3 = 6; 50% of 6 is 3),
	 * although 50% of the library is equal to 5 items.
	 * As you see, the order of these operations can make quite a
	 * difference and is thus very important.
	 *
	 * Please note that the filter to remove recent songs can try to remove
	 * an item that has already been removed by artist matching.  This means
	 * that it may seem like the filter removes some items that in fact do
	 * not exist anymore in that list, but the total items to remove is
	 * still counted.  In conclusion, the amount to "remove by recently
	 * played" is inclusive ones removed by artist matching.
	 */

	// First make sure any songs that aren't directly playable are filtered out
	filtered_songs = wf_intelligence_remove_invalid_songs(filtered_songs);

	// Artist filtering
	filtered_songs = wf_intelligence_remove_songs_with_artists(filtered_songs, recent_artists, filter->recent_artists);

	// Filter by song statistics
	filtered_songs = wf_intelligence_filter_by_stats(filtered_songs, filter);

	// Filter by recently played
	remove_recent = wf_intelligence_get_percentage_of_list(filtered_songs, filter->remove_recents_percentage);
	remove_recent += filter->remove_recents_amount;
	filtered_songs = wf_intelligence_remove_recents(filtered_songs, previous_songs, play_next, remove_recent);

	if (g_list_length(filtered_songs) == 0)
	{
		g_info("All songs are filtered out");

		return NULL;
	}

	return filtered_songs;
}

static GList *
wf_intelligence_remove_invalid_songs(GList *library)
{
	WfSong *song;
	WfSongStatus status;
	GList *list;
	GList *next;

	if (library == NULL)
	{
		return NULL;
	}

	// Set list node pointer to use
	list = library;

	// Go through the whole library
	while (list != NULL)
	{
		next = list->next;
		song = list->data;

		if (song == NULL)
		{
			continue;
		}

		status = wf_song_get_status(song);

		// Check status (switch can be easily expanded in the future)
		if (status != WF_SONG_AVAILABLE)
		{
			g_debug("Filtered out %s because it is not available", wf_song_get_name_not_empty(song));
			library = g_list_remove(library, song);
		}

		list = next;
	}

	return library;
}

static GList *
wf_intelligence_filter_by_stats(GList *library, WfSongFilter *filter)
{
	const gchar *name;
	WfSong *song;
	GList *item;
	GList *next;
	gboolean filtered;
	gint64 time;
	gint64 time_since_last_played;

	gboolean rating_on;
	gboolean score_on;
	gboolean playcount_on;
	gboolean skipcount_on;
	gboolean lastplayed_on;
	gboolean playcount_invert;
	gboolean skipcount_invert;
	gboolean lastplayed_invert;
	gboolean rating_inc_zero;
	gint rating_v;
	gint rating_min;
	gint rating_max;
	gdouble score_v;
	gdouble score_min;
	gdouble score_max;
	gint playcount_v;
	gint playcount_th;
	gint skipcount_v;
	gint skipcount_th;
	gint64 lastplayed_v;
	gint64 lastplayed_th;

	g_return_val_if_fail(filter != NULL, library);

	if (library == NULL)
	{
		// Nothing to filter
		return NULL;
	}

	// Get stat ranges
	rating_min = filter->rating_min;
	rating_max = filter->rating_max;
	rating_inc_zero = filter->rating_inc_zero;
	score_min = filter->score_min;
	score_max = filter->score_max;
	playcount_th = filter->playcount_th;
	playcount_invert = filter->playcount_invert;
	skipcount_th = filter->skipcount_th;
	skipcount_th = filter->skipcount_th;
	skipcount_invert = filter->skipcount_invert;
	lastplayed_th = filter->lastplayed_th;
	lastplayed_invert = filter->lastplayed_invert;

	// Determine what stats to use
	rating_on = wf_intelligence_use_rating_filter(filter->use_rating, rating_min, rating_max);
	score_on = wf_intelligence_use_score_filter(filter->use_score, score_min, score_max);
	playcount_on = wf_intelligence_use_playcount_filter(filter->use_playcount, playcount_th);
	skipcount_on = wf_intelligence_use_skipcount_filter(filter->use_skipcount, skipcount_th);
	lastplayed_on = wf_intelligence_use_lastplayed_filter(filter->use_lastplayed, lastplayed_th);

	item = library;

	// Set the time here, so all songs have the same probability to be filtered by last_played
	time = wf_utils_time_now();

	while (item != NULL)
	{
		next = item->next;
		song = item->data;
		filtered = FALSE;

		if (song == NULL)
		{
			item = next;
			continue;
		}

		// Set name pointer so it can be easily used in debug messages
		name = wf_song_get_name_not_empty(song);

		/*
		 * Filtering @item starts here
		 *
		 * First get the value and check if it is valid
		 * Then, take parameters as "invert" or "include zero" into account
		 * After that, determine if the value is within the specified
		 * range value.
		 * If that is not the case, remove the list node of that song from
		 * the filtered list.
		 * If filtered, do not check the other values but carry on
		 * the next song (to run more efficiently and prevent any more
		 * removements).  Only remove the node from the list and free
		 * it at the end, just in case it gets accessed anywhere.
		 */

		// Check rating
		if (rating_on && !filtered)
		{
			rating_v = wf_song_get_rating(song);

			if (!wf_stats_rating_is_valid(rating_v) ||
			    (!(rating_inc_zero && rating_v == 0) &&
			     (rating_v < rating_min || rating_v > rating_max)))
			{
				// Remove list node respective library node
				library = g_list_remove_link(library, item);
				g_debug("Song %s filtered out by rating %d", name, rating_v);
				filtered = TRUE;
			}
		}

		// Check score
		if (score_on && !filtered)
		{
			score_v = wf_song_get_score(song);

			if (!wf_stats_score_is_valid(score_v) ||
			    (score_v < score_min || score_v > score_max))
			{
				// Remove list node respective library node
				library = g_list_remove_link(library, item);
				g_debug("Song %s filtered out by score %f", name, (float) score_v);
				filtered = TRUE;
			}
		}

		// Check play count
		if (playcount_on && !filtered)
		{
			playcount_v = wf_song_get_play_count(song);

			if (!wf_stats_playcount_is_valid(playcount_v) ||
			    (playcount_invert && playcount_v > playcount_th) ||
			    (!playcount_invert && playcount_v < playcount_th))
			{
				// Remove list node respective library node
				library = g_list_remove_link(library, item);
				g_debug("Song %s filtered out by play count %d", name, playcount_v);
				filtered = TRUE;
			}
		}

		// Check skip count
		if (skipcount_on && !filtered)
		{
			skipcount_v = wf_song_get_skip_count(song);

			if (!wf_stats_skipcount_is_valid(skipcount_v) ||
			    (skipcount_invert && skipcount_v > skipcount_th) ||
			    (!skipcount_invert && skipcount_v < skipcount_th))
			{
				// Remove list node respective library node
				library = g_list_remove_link(library, item);
				g_debug("Song %s filtered out by skip count %d", name, skipcount_v);
				filtered = TRUE;
			}
		}

		// Check last played
		if (lastplayed_on && !filtered)
		{
			lastplayed_v = wf_song_get_last_played(song);
			time_since_last_played = wf_utils_time_compare(lastplayed_v, time);

			if (!wf_stats_lastplayed_is_valid(lastplayed_v) ||
			    (lastplayed_invert && time_since_last_played > lastplayed_th) ||
			    (!lastplayed_invert && time_since_last_played < lastplayed_th))
			{
				// Remove list node respective library node
				library = g_list_remove_link(library, item);
				g_debug("Song %s filtered out by last played %ld", name, (long int) lastplayed_v);
				filtered = TRUE;
			}
		}

		if (filtered)
		{
			// Only free node
			g_list_free(item);
		}

		// Set next item
		item = next;
	}

	return library;
}

static GList *
wf_intelligence_remove_songs_with_artists(GList *library, GList *artists, gint amount)
{
	WfSong *song;
	GList *list;
	GList *next;
	guint32 hash;
	guint32 artist;
	gint x;

	if (library == NULL || artists == NULL)
	{
		g_info("No songs to remove that match any recent artist");

		return library;
	}

	// Set list node pointer to use
	list = library;

	// Go through the whole library, only once
	while (list != NULL)
	{
		next = list->next;
		song = list->data;

		if (song == NULL)
		{
			continue;
		}

		artist = wf_song_get_artist_hash(song);

		if (artist != 0)
		{
			x = 0;

			// Get matching artist
			for (list = artists; list != NULL; list = list->next)
			{
				if (x >= amount)
				{
					// Already checked enough artists
					break;
				}

				x++;

				// Get the hash from the list node
				hash = GPOINTER_TO_UINT(list->data);

				if (artist == hash)
				{
					g_debug("Filtered out %s by artist %s", wf_song_get_name_not_empty(song), wf_song_get_artist(song));
					library = g_list_remove(library, song);

					break;
				}
			}
		}

		list = next;
	}

	return library;
}

static GList *
wf_intelligence_remove_recents(GList *library, GList *list_prev, GList *list_next, gint amount)
{
	WfSong *song;
	GList *list, *new_list;
	gint x = 0;

	if (library == NULL || amount <= 0)
	{
		g_info("No recent items to remove");

		return library;
	}
	else
	{
		g_info("Removing %d recently played songs", amount);
	}

	// Check if already chosen songs need to be removed
	if (list_next != NULL && x < amount)
	{
		for (list = list_next; list != NULL && x < amount; list = list->next)
		{
			song = list->data;

			if (song == NULL)
			{
				x--;
				continue;
			}
			else
			{
				g_debug("Filtered out previously selected %s", wf_song_get_name_not_empty(song));
			}

			// If non-existing, library is unchanged anyway
			library = g_list_remove(library, song);
			x++;
		}
	}

	// Check for songs that have been added to list_prev
	if (list_prev != NULL && x < amount)
	{
		for (list = list_prev; list != NULL && x < amount; list = list->next)
		{
			song = list->data;

			if (song == NULL)
			{
				x--;
				continue;
			}
			else
			{
				g_debug("Filtered out recently played %s", wf_song_get_name_not_empty(song));
			}

			// If non-existing, library is unchanged anyway
			library = g_list_remove(library, song);
			x++;
		}
	}

	// Remove based on last_played if not enough have been removed
	if (x < amount)
	{
		// First, copy the list to sort, so the original remains its order
		new_list = g_list_copy(library);

		// Sort the list so most recently played songs are at the top
		new_list = g_list_sort(new_list, wf_intelligence_sort_compare);

		// Loop @amount times and remove the respective item
		for (list = new_list; list != NULL && x < amount; x++)
		{
			song = list->data;
			list = list->next;

			// If invalid or never played, skip to the next one, but keep track of how many have already been removed
			if (song == NULL || wf_song_get_last_played(song) <= 0)
			{
				x--;
				continue;
			}

			g_debug("Filtered out %s by last_played %ld", wf_song_get_name_not_empty(song), (long int) wf_song_get_last_played(song));

			// Remove song from intelligence library
			library = g_list_remove(library, song);
		}

		g_list_free(new_list);
	}

	if (x == 0)
	{
		g_info("Did not remove any recently played songs");
	}
	else if (x < amount)
	{
		g_info("Only removed %d of the recently played songs", x);
	}

	return library;
}

/*
 * wf_intelligence_get_song:
 * @filtered_songs: (element-type WfSong): list of filtered songs to use
 * @entries: probability parameters to use
 *
 * Use the probability part of the algorithm which involves determining song
 * chances and choosing a song.
 *
 * Returns: a chosen song
 */
WfSong *
wf_intelligence_get_song(GList *filtered_songs, WfSongEntries *preferences)
{
	WfSong *winner = NULL;
	WfIntelligenceContainer container;
	WfDList *list;

	// If any songs are present, determine what modifiers to use
	if (filtered_songs == NULL ||
	    !wf_intelligence_determine_modifiers(&container, preferences))
	{
		return NULL;
	}

	// Now calculate the amount of entries for each individual song
	list = wf_intelligence_calculate_song_entries(&container, filtered_songs);

	// At last, pick a winner
	winner = wf_intelligence_pick_winner(&container, list);

	wf_d_list_free(list);

	return winner;
}

static gboolean
wf_intelligence_determine_modifiers(WfIntelligenceContainer *container, WfSongEntries *preferences)
{
	g_return_val_if_fail(container != NULL, FALSE);

	// First set all values to 0 (no entry modifiers)
	container->total_entries = 0;
	container->default_rating = 0;
	container->favor_low_ratings = FALSE;
	container->rating_factor = 0;
	container->favor_low_scores = FALSE;
	container->score_factor = 0;
	container->favor_low_playcount = FALSE;
	container->playcount_factor = 0;
	container->favor_low_skipcount = FALSE;
	container->skipcount_factor = 0;
	container->favor_low_lastplayed = FALSE;
	container->lastplayed_factor = 0;

	if (preferences == NULL)
	{
		return FALSE;
	}

	// Get rating preferences
	if (preferences->use_rating && preferences->rating_multiplier > 0.0)
	{
		container->rating_factor = preferences->rating_multiplier * MULTIPLIER_RATING;
		container->favor_low_ratings = preferences->invert_rating;
		container->default_rating = preferences->use_default_rating;

		g_info("Probability: use rating (invert: %d)", container->favor_low_ratings);
	}

	// Get score preferences
	if (preferences->use_score && preferences->score_multiplier > 0.0)
	{
		container->score_factor = preferences->score_multiplier * MULTIPLIER_SCORE;
		container->favor_low_scores = preferences->invert_score;

		g_info("Probability: use score (invert: %d)", container->favor_low_scores);
	}

	// Get play count preferences
	if (preferences->use_playcount && preferences->playcount_multiplier > 0)
	{
		container->playcount_factor = preferences->playcount_multiplier * MULTIPLIER_PLAYCOUNT;
		container->favor_low_playcount = preferences->invert_playcount;

		g_info("Probability: use play count (invert: %d)", container->favor_low_playcount);
	}

	// Get skip count preferences
	if (preferences->use_skipcount && preferences->skipcount_multiplier > 0)
	{
		container->skipcount_factor = preferences->skipcount_multiplier * MULTIPLIER_SKIPCOUNT;
		container->favor_low_skipcount = preferences->invert_skipcount;

		g_info("Probability: use skip count (invert: %d)", container->favor_low_skipcount);
	}

	// Get last played preferences
	if (preferences->use_lastplayed && preferences->lastplayed_multiplier > 0)
	{
		container->lastplayed_factor = preferences->lastplayed_multiplier * MULTIPLIER_LASTPLAYED;
		container->favor_low_lastplayed = preferences->invert_lastplayed;

		g_info("Probability: use last played (invert: %d)", container->favor_low_lastplayed);
	}

	return TRUE;
}

static WfDList *
wf_intelligence_calculate_song_entries(WfIntelligenceContainer *container, GList *songs)
{
	const gchar *name;
	const gint64 one_year = (365 * 24 * 60 * 60);
	const gint64 current_time = wf_utils_time_now();
	WfSong *song;
	GList *list;
	WfDList *dlist = NULL;
	gint x, entries, full_sum = 0;

	gint rating;
	gdouble score;
	gint playcount;
	gint skipcount;
	gint64 lastplayed;
	gint64 time_since_last_played;
	gboolean use_rating = FALSE;
	gboolean use_score = FALSE;
	gboolean use_playcount = FALSE;
	gboolean use_skipcount = FALSE;
	gboolean use_lastplayed = FALSE;

	g_return_val_if_fail(container != NULL, NULL);
	g_return_val_if_fail(songs != NULL, NULL);

	// Determine what to use
	use_rating = (container->rating_factor != 0);
	use_score = (container->score_factor != 0);
	use_playcount = (container->playcount_factor != 0);
	use_skipcount = (container->skipcount_factor != 0);
	use_lastplayed = (container->lastplayed_factor != 0);

	for (list = songs; list != NULL; list = list->next)
	{
		song = list->data;

		if (song == NULL)
		{
			continue;
		}

		name = wf_song_get_name_not_empty(song);
		entries = 0;

		if (use_rating)
		{
			rating = wf_song_get_rating(song);

			if (wf_stats_rating_is_valid(rating))
			{
				if (container->favor_low_ratings)
				{
					rating = wf_stats_rating_invert(rating);
				}
				else if (rating == 0)
				{
					rating = container->default_rating;
				}

				entries += rating * container->rating_factor;
			}
		}

		if (use_score)
		{
			score = wf_song_get_score(song);

			if (wf_stats_score_is_valid(score))
			{
				if (container->favor_low_scores)
				{
					score = wf_stats_score_invert(score);
				}

				entries += score * container->score_factor;
			}
		}

		if (use_playcount)
		{
			playcount = wf_song_get_play_count(song);

			if (wf_stats_playcount_is_valid(playcount))
			{
				if (container->favor_low_playcount)
				{
					x = wf_intelligence_get_entries_count_inverted(playcount);
				}
				else
				{
					x = wf_intelligence_get_entries_count(playcount);
				}

				entries += x * container->playcount_factor;
			}
		}

		if (use_skipcount)
		{
			skipcount = wf_song_get_skip_count(song);

			if (wf_stats_skipcount_is_valid(skipcount))
			{
				if (container->favor_low_skipcount)
				{
					x = wf_intelligence_get_entries_count_inverted(skipcount);
				}
				else
				{
					x = wf_intelligence_get_entries_count(skipcount);
				}

				entries += x * container->skipcount_factor;
			}
		}

		if (use_lastplayed)
		{
			lastplayed = wf_song_get_last_played(song);

			if (wf_stats_lastplayed_is_valid(lastplayed))
			{
				// Issue a warning is the current time could not be fetched
				g_warn_if_fail(current_time > 0);

				lastplayed = wf_song_get_last_played(song);
				time_since_last_played = wf_utils_time_compare(lastplayed, current_time);

				if (time_since_last_played > one_year)
				{
					// Just use maximum
					x = 100;
				}
				else
				{
					if (container->favor_low_lastplayed)
					{
						x = wf_intelligence_get_entries_time_since_inverted(time_since_last_played);
					}
					else
					{
						x = wf_intelligence_get_entries_time_since(time_since_last_played);
					}
				}

				entries += x * container->lastplayed_factor;
			}
		}

		// If many entries got subtracted, disqualify the song
		if (entries < 0)
		{
			g_debug("%s disqualified", name);
			continue;
		}

		// If this song got no entries from modifiers, give it at least one
		if (entries == 0)
		{
			entries = 1;
		}

		dlist = wf_d_list_add(dlist, song, GINT_TO_POINTER(entries));

		full_sum += entries;

		g_debug("Song <%s> has %d %s", name, entries, wf_utils_string_to_single_multiple(entries, "entry", "entries"));
	}

	if (full_sum <= 0)
	{
		g_message("No qualified songs");
		wf_d_list_free(dlist);

		return NULL;
	}
	else
	{
		container->total_entries = full_sum;

		return dlist;
	}
}

static WfSong *
wf_intelligence_pick_winner(WfIntelligenceContainer *container, WfDList *list)
{
	WfSong *song, *winner = NULL;
	WfDList *item;
	gint entries, total;
	guint rand;
	guint64 sum = 0;

	g_return_val_if_fail(container != NULL, NULL);

	if (list == NULL)
	{
		// No songs available
		return NULL;
	}

	total = container->total_entries;

	g_return_val_if_fail(total > 0, NULL);

	// Pick a winner
	rand = wf_intelligence_random(1, total);

	for (item = list; item != NULL; item = item->next)
	{
		entries = GPOINTER_TO_INT(item->value);

		if (entries <= 0)
		{
			g_debug("Invalid entry count %d/%d", entries, total);
			continue;
		}
		else
		{
			sum += entries;
		}

		if (sum >= rand)
		{
			// Now find the item
			song = item->key;

			if (song == NULL)
			{
				// Try next item
				g_debug("Invalid song while choosing one");
				continue;
			}
			else
			{
				// Found the winner
				winner = song;
				break;
			}
		}
	}

	if (winner == NULL)
	{
		g_warning("Failed to draw a winner (entry %d/%d)", rand, total);

		return NULL;
	}
	else
	{
		g_info("Winner (entry %d/%d): %s", rand, total, wf_song_get_name_not_empty(winner));

		return winner;
	}
}

/*
 * wf_intelligence_choose_new_song:
 * @library: (element-type WfSong): pointer to the full list with songs to filter
 * @previous_songs: (element-type WfSong): list of previously played songs
 * @play_next: (element-type WfSong): list of songs selected to play next
 * @recent_artists: (element-type guint32): list of recent artists
 * @filer: filter parameters to use
 * @entries: probability parameters to use
 *
 * Walk through the complete algorithm to choose a new song.
 *
 * Please note that @library is a pointer to the pointer of the first list
 * node.  Because the list may be filtered and as such the first item may also
 * be removed, changing the pointer to the first list node.  When the caller
 * tries to free the list, this would, without any ownership transfers, cause a
 * free on an already freed node.  By providing the pointer to the pointer of
 * the first node, the pointer of the first node can be changed and safely
 * freed later.
 *
 * Returns: a chosen song
 */
WfSong *
wf_intelligence_choose_new_song(GList **library,
                                GList *previous_songs,
                                GList *play_next,
                                GList *recent_artists,
                                WfSongFilter *filter,
                                WfSongEntries *entries)
{
	WfSong *new_song = NULL;

	g_return_val_if_fail(library != NULL, NULL);

	if (*library == NULL)
	{
		// No song to choose
		return NULL;
	}

	if (filter != NULL)
	{
		*library = wf_intelligence_filter(*library, previous_songs, play_next, recent_artists, filter);
	}

	if (entries != NULL)
	{
		new_song = wf_intelligence_get_song(*library, entries);
	}

	return new_song;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static guint
wf_intelligence_random(gint lower, gint upper)
{
	/*
	 * Formula:
	 * f(x) = (random_value % (entries - lowest_value + 1)) + lowest_value
	 *
	 * First, determine range:
	 * 	Subtract the lowest possible value (usually 0) from the total
	 * entries and add 1 (to include lower value in the range)
	 * Then, get the random value in the specified range:
	 * 	Divide range_value by the calculated range and take the remainder
	 * (remainder from division is calculated using the % operator).
	 * At last, calibrate the value so it fits within the given range:
	 * 	Add the lowest value (from the input) to the calculated remainder.
	 */

	GRand *rand = g_rand_new();
	guint32 value = g_rand_int(rand);

	guint random = (value % (upper - lower + 1)) + lower;

	g_rand_free(rand);

	return random;
}

static gint
wf_intelligence_get_percentage_of_list(GList *list, gdouble percentage)
{
	const gdouble range_min = 0.0, range_max = 100.0;
	gint total;

	if (list == NULL || percentage <= range_min)
	{
		return 0;
	}

	total = g_list_length(list);

	if (percentage >= range_max)
	{
		return total;
	}

	return (((gdouble) total * percentage) / 100.0);
}

static gboolean
wf_intelligence_use_rating_filter(gboolean use, gint rating_min, gint rating_max)
{
	if (!use || rating_min <= 0 || rating_max <= 0)
	{
		return FALSE;
	}
	else
	{
		return (wf_stats_rating_is_valid(rating_min) && wf_stats_rating_is_valid(rating_max));
	}
}

static gboolean
wf_intelligence_use_score_filter(gboolean use, gdouble score_min, gdouble score_max)
{
	if (!use || score_min <= 0.0 || score_max <= 0.0)
	{
		return FALSE;
	}
	else
	{
		return (wf_stats_score_is_valid(score_min) && wf_stats_score_is_valid(score_max));
	}
}

static gboolean
wf_intelligence_use_playcount_filter(gboolean use, gint playcount_th)
{
	if (!use || playcount_th <= 0)
	{
		return FALSE;
	}
	else
	{
		return (wf_stats_playcount_is_valid(playcount_th));
	}
}

static gboolean
wf_intelligence_use_skipcount_filter(gboolean use, gint skipcount_th)
{
	if (!use || skipcount_th <= 0)
	{
		return FALSE;
	}
	else
	{
		return (wf_stats_skipcount_is_valid(skipcount_th));
	}
}

static gboolean
wf_intelligence_use_lastplayed_filter(gboolean use, gint64 lastplayed_th)
{
	if (!use || (lastplayed_th <= 0))
	{
		return FALSE;
	}
	else
	{
		return (wf_stats_lastplayed_is_valid(lastplayed_th));
	}
}

static gint
wf_intelligence_sort_compare(gconstpointer a, gconstpointer b)
{
	const WfSong *song_a = a, *song_b = b;
	gint64 last_played_a, last_played_b;
	gboolean valid_a, valid_b;

	// Validation check (if FALSE, pointer *could* be NULL)
	valid_a = wf_song_is_valid(song_a);
	valid_b = wf_song_is_valid(song_b);

	// Check invalids
	if (!valid_a && !valid_b)
	{
		// Equal
		return 0;
	}
	else if (!valid_a && valid_b)
	{
		// @song_a comes after @song_b
		return 1;
	}
	else if (valid_a && !valid_b)
	{
		// @song_a comes before @song_b
		return -1;
	}
	else
	{
		last_played_a = wf_song_get_last_played(song_a);
		last_played_b = wf_song_get_last_played(song_b);

		if (last_played_a > last_played_b)
		{
			// @song_a comes before @song_b
			return -1;
		}
		else if (last_played_a < last_played_b)
		{
			// @song_a comes after @song_b
			return 1;
		}
		else
		{
			// Equal
			return 0;
		}
	}
}

static gint
wf_intelligence_calculate_entries_with_fraction(const gint x, const gint a, const gint r, const gboolean invert)
{
	/*
	 * Domain (x): [0; inf)
	 * Range (y): [0; r)
	 *
	 * The graph crosses the origin (at (0; 0))
	 * @a must be greater than zero and determines for what x the y is r/2
	 * @r must be greater than zero and determines the output range
	 *
	 * If !invert:
	 * 	Formula: f(x) = (-a * r) / (x + a) + r (see documentation)
	 * If invert:
	 * 	Formula: f(x) = ( a * r) / (x + a)     (see documentation)
	 */

	gdouble numerator, denominator;
	gint result;

	// Preconditions
	g_return_val_if_fail(a > 0, 0);
	g_return_val_if_fail(r > 0, 0);
	g_return_val_if_fail(x >= 0, 0);

	// Calculate
	if (invert)
	{
		numerator = (gdouble) (a * r);
		denominator = (gdouble) (x + a);
		result = (gint) (numerator / denominator);
	}
	else
	{
		numerator = (gdouble) (-1 * a * r);
		denominator = (gdouble) (x + a);
		result = (gint) (numerator / denominator) + r;
	}

	// Check if valid and return
	g_return_val_if_fail(result >= 0 && result <= r, result);

	return result;
}

static gint
wf_intelligence_calculate_entries_with_sqrt(const gint x, const gint a, const gint r, const gboolean invert)
{
	/*
	 * Domain (x): [0; inf)
	 * Range (y): [0; r)
	 *
	 * The graph crosses the origin (at (0; 0))
	 * @a must be greater than zero and determines x = @a^2 for which y is r
	 * @r must be greater than zero and determines the output range
	 *
	 * If !invert:
	 * 	Formula: f(x) = (( r * sqrt(x)) / a)     (see documentation)
	 * If invert:
	 * 	Formula: f(x) = ((-r * sqrt(x)) / a) + r (see documentation)
	 */

	gdouble numerator, denominator;
	gint result;

	// Preconditions
	g_return_val_if_fail(a > 0, 0);
	g_return_val_if_fail(r > 0, 0);
	g_return_val_if_fail(x >= 0, 0);

	// Calculate
	if (invert)
	{
		numerator = (gdouble) (-1 * r * sqrt(x));
		denominator = (gdouble) a;
		result = (gint) (numerator / denominator) + r;
	}
	else
	{
		numerator = (gdouble) (r * sqrt(x));
		denominator = (gdouble) a;
		result = (gint) (numerator / denominator);
	}

	// Check if valid and return
	g_return_val_if_fail(result >= 0 && result <= r, result);

	return result;
}

static gint
wf_intelligence_get_entries_count(gint count)
{
	const gint x = count; // Input value
	const gint r = MAX_SONG_ENTRIES; // Constant: range
	const gint a = 100; // Constant: shape
	const gboolean invert = FALSE;

	return wf_intelligence_calculate_entries_with_fraction(x, a, r, invert);
}

static gint
wf_intelligence_get_entries_count_inverted(gint count)
{
	const gint x = count; // Input value
	const gint r = MAX_SONG_ENTRIES; // Constant: range
	const gint a = 100; // Constant: shape
	const gboolean invert = TRUE;

	return wf_intelligence_calculate_entries_with_fraction(x, a, r, invert);
}

static gint
wf_intelligence_get_entries_time_since(gint64 time_since)
{
	// Note that time_since is capped to one year in seconds
	const gint64 one_year = (365 * 24 * 60 * 60);
	const gint x = (gint) ((time_since > one_year) ? one_year : time_since); // Input value
	const gint r = MAX_SONG_ENTRIES; // Constant: range
	const gint a = 5616; // Constant: shape
	const gboolean invert = FALSE;

	return wf_intelligence_calculate_entries_with_sqrt(x, a, r, invert);
}

static gint
wf_intelligence_get_entries_time_since_inverted(gint64 time_since)
{
	// Note that time_since is capped to one year in seconds
	const gint64 one_year = (365 * 24 * 60 * 60);
	const gint x = (gint) ((time_since > one_year) ? one_year : time_since); // Input value
	const gint r = MAX_SONG_ENTRIES; // Constant: range
	const gint a = 5616; // Constant: shape
	const gboolean invert = TRUE;

	return wf_intelligence_calculate_entries_with_sqrt(x, a, r, invert);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */
/* DESTRUCTORS END */

/* END OF FILE */
