/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * statistics.c  This file is part of LibWoofer
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
#include <woofer/statistics.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/song_manager.h>
#include <woofer/settings.h>
#include <woofer/utils.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module contains subroutines for modifying the statistics of any given
 * song.
 *
 * Since this module only contains utilities for other modules, all of these
 * "utilities" are part of the normal module functions and constructors,
 * destructors, etc. are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Range values
#define STAT_RATING_MIN 0
#define STAT_RATING_MAX 100
#define STAT_SCORE_MIN 0.0
#define STAT_SCORE_MAX 100.0
#define STAT_PLAYCOUNT_MIN 0
#define STAT_PLAYCOUNT_MAX 0 // Infinity
#define STAT_SKIPCOUNT_MIN 0
#define STAT_SKIPCOUNT_MAX 0 // Infinity
#define STAT_LASTPLAYED_MIN 0
#define STAT_LASTPLAYED_MAX 0 // Infinity

// Range validator macros
#define STAT_RATING_IS_VALID(rating) (rating >= STAT_RATING_MIN && rating <= STAT_RATING_MAX)
#define STAT_SCORE_IS_VALID(score) (score >= STAT_SCORE_MIN && score <= STAT_SCORE_MAX)
#define STAT_PLAYCOUNT_IS_VALID(playcount) (playcount >= STAT_PLAYCOUNT_MIN)
#define STAT_SKIPCOUNT_IS_VALID(skipcount) (skipcount >= STAT_SKIPCOUNT_MIN)
#define STAT_LASTPLAYED_IS_VALID(lastplayed) (lastplayed >= STAT_LASTPLAYED_MIN)

/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

/*
 * wf_stats_update_rating:
 * @song: song to alter
 * @rating: the new rating (or 0)
 * @increase: the amount to increase (or negative to decrease)
 *
 * Update a song's rating by providing a new value or a value to add to the
 * current rating.  If @rating is -1, the rating is reset to 0.  If @rating is
 * 0, @increase is used (if valid).
 */
void
wf_stats_update_rating(WfSong *song, gint rating, gint increase)
{
	const gchar *name;
	gint rating_current, rating_value = 0;

	g_return_if_fail(WF_IS_SONG(song));

	name = wf_song_get_name_not_empty(song);
	rating_current = wf_song_get_rating(song);

	if (rating == -1)
	{
		rating_value = 0;
		g_debug("Rating of %s has been reset to %d.", name, rating_value);
	}
	else if (rating != 0 && rating >= STAT_RATING_MIN && rating <= STAT_RATING_MAX)
	{
		rating_value = rating;
		g_debug("Rating of %s is now set to %d", name, rating_value);
	}
	else if (increase >= (-1 * STAT_RATING_MAX) && increase <= STAT_RATING_MAX)
	{
		rating_value += rating_current + increase;

		if (rating_value <= STAT_RATING_MIN)
		{
			g_warning("Increasing rating of %s resulted in an invalid value %d; value is unchanged", name, rating_value);

			return;
		}
		else if (rating_value > STAT_RATING_MAX)
		{
			g_warning("Increasing rating of %s resulted in an invalid value %d; value is unchanged", name, rating_value);

			return;
		}
		else
		{
			g_debug("Rating of %s is increased by %d to %d", name, increase, rating_value);
		}
	}
	else
	{
		// Invalid parameter
		g_warning("No valid parameters in attempt to update rating of %s. Rating is (still) %d", name, rating);

		return;
	}

	// Save the new rating
	wf_song_set_rating(song, rating_value);
}

/*
 * wf_stats_update_score:
 * @song: song to alter
 * @score: the new score (or 0.0)
 * @increase: the amount to increase (or negative to decrease)
 *
 * Update a song's score by providing a new value or a value to add to the
 * current score.  If @score is -1, the score is reset to 0.0.  If @score is
 * 0.0, @increase is used (if valid).
 */
void
wf_stats_update_score(WfSong *song, gdouble score, gdouble increase)
{
	const gchar *name;
	gdouble score_current, score_value = 0;

	g_return_if_fail(WF_IS_SONG(song));

	name = wf_song_get_name_not_empty(song);
	score_current = wf_song_get_score(song);

	if (score == -1)
	{
		score_value = 0;
		g_debug("Score of %s has been reset to %f", name, (float) score_value);
	}
	else if (score != 0.0 && score >= STAT_SCORE_MIN && score <= STAT_SCORE_MAX)
	{
		score_value = score;
		g_debug("Score of %s is now set to %f", name, (float) score_value);
	}
	else if (increase >= (-1 * STAT_SCORE_MAX) && increase <= STAT_SCORE_MAX)
	{
		score_value += score_current + increase;

		if (score_value < STAT_SCORE_MIN)
		{
			g_warning("Increasing score of %s resulted in an invalid value %f; value is unchanged", name, (float) score_value);

			return;
		}
		else if (score_value > STAT_SCORE_MAX)
		{
			g_warning("Increasing score of %s resulted in an invalid value %f; value is unchanged", name, (float) score_value);

			return;
		}
		else
		{
			g_debug("Score of %s is increased by %f to %f", name, (float) increase, (float) score_value);
		}
	}
	else
	{
		// Invalid parameter
		g_warning("No valid parameters in attempt to update score of %s. Score is (still) %f", name, (float) score);

		return;
	}

	// Save the new score
	wf_song_set_score(song, score_value);
}

/*
 * wf_stats_update_playcount:
 * @song: song to alter
 * @playcount: the new play count (or 0)
 * @increase: the amount to increase (or negative to decrease)
 *
 * Update a song's play count by providing a new value or a value to add to the
 * current count.  If @playcount is -1, the count is reset to 0.  If @increase
 * is not 0, use that; otherwise, use @playcount.
 */
void
wf_stats_update_playcount(WfSong *song, gint playcount, gint increase)
{
	const gchar *name;
	gint playcount_current, playcount_value = 0;

	g_return_if_fail(WF_IS_SONG(song));

	name = wf_song_get_name_not_empty(song);
	playcount_current = wf_song_get_play_count(song);

	if (playcount == -1)
	{
		playcount_value = 0;
		g_debug("Play count of %s has been reset to %d", name, playcount_value);
	}
	else if (increase == 0)
	{
		playcount_value = playcount;
		g_debug("Play count of %s is now set to %d", name, playcount_value);
	}
	else
	{
		playcount_value += playcount_current + increase;

		if (playcount_value < STAT_PLAYCOUNT_MIN)
		{
			g_warning("Increasing play count of %s resulted in an invalid value %d; value is unchanged", name, playcount_value);

			return;
		}
		else
		{
			g_debug("Play count of %s is increased by %d to %d", name, increase, playcount_value);
		}
	}

	// Save the new play count
	wf_song_set_play_count(song, playcount_value);
}

/*
 * wf_stats_update_skipcount:
 * @song: song to alter
 * @skipcount: the new skip count (or 0)
 * @increase: the amount to increase (or negative to decrease)
 *
 * Update a song's skip count by providing a new value or a value to add to the
 * current count.  If @skipcount is -1, the count is reset to 0.  If @increase
 * is not 0, use that; otherwise, use @skipcount.
 */
void
wf_stats_update_skipcount(WfSong *song, gint skipcount, gint increase)
{
	const gchar *name;
	gint skipcount_current, skipcount_value = 0;

	g_return_if_fail(WF_IS_SONG(song));

	name = wf_song_get_name_not_empty(song);
	skipcount_current = wf_song_get_skip_count(song);

	if (skipcount == -1)
	{
		skipcount_value = 0;
		g_debug("Skip count of %s has been reset to %d", name, skipcount_value);
	}
	else if (increase == 0)
	{
		skipcount_value = skipcount;
		g_debug("Skip count of %s is now set to %d", name, skipcount_value);
	}
	else
	{
		skipcount_value += skipcount_current + increase;

		if (skipcount_value < STAT_SKIPCOUNT_MIN)
		{
			g_warning("Increasing skip count of %s resulted in an invalid value %d; value is unchanged", name, skipcount_value);

			return;
		}
		else
		{
			g_debug("Skip count of %s is increased by %d to %d", name, increase, skipcount_value);
		}
	}

	// Save the new skip count
	wf_song_set_skip_count(song, skipcount_value);
}

/*
 * wf_stats_update_lastplayed:
 * @song: song to alter
 * @lastplayed: a new timestamp (or 0)
 * @increase: the amount to increase (or negative to decrease)
 *
 * Update a song's last played by providing a new value or a value to add to the
 * current timestamp.  If @lastplayed is -1, the timestamp is reset to 0.  If
 * @lastplayed is 0, @increase is used (if valid).
 */
void
wf_stats_update_lastplayed(WfSong *song, gint64 lastplayed, gint increase)
{
	const gchar *name;
	gint64 lastplayed_current, lastplayed_value = 0;

	g_return_if_fail(WF_IS_SONG(song));

	name = wf_song_get_name_not_empty(song);
	lastplayed_current = wf_song_get_last_played(song);

	if (lastplayed == -1)
	{
		lastplayed_value = 0;
		g_debug("Last played of %s has been reset to %ld", name, (long int) lastplayed_value);
	}
	else if (lastplayed >= 0)
	{
		lastplayed_value = lastplayed;
		g_debug("Last played of %s is now set to %ld", name, (long int) lastplayed_value);
	}
	else
	{
		lastplayed_value += lastplayed_current + increase;

		if (lastplayed_value <= STAT_LASTPLAYED_MIN)
		{
			g_warning("Increasing last played of %s resulted in an invalid value %ld; value is unchanged", name, (long int) lastplayed_value);

			return;
		}
		else
		{
			g_debug("Last played of %s is increased by %ld to %ld", name, (long int) increase, (long int) lastplayed_value);
		}
	}

	// Save the new last played
	wf_song_set_last_played(song, lastplayed_value);
}

// Make sure to run this function *prior* running the update_playcount respective function, because it relies on the non-updated playcount
void
wf_stats_modify_and_update_score(WfSong *song, gdouble played_fraction)
{
	// This method of updating is based on the Amarok Music player

	gdouble old_score;
	gdouble new_score;
	gdouble full_played_fraction = 1.0;
	gint playcount = 0;

	g_return_if_fail(WF_IS_SONG(song));

	if (wf_song_manager_get_incognito()) // If incognito is on
	{
		g_info("Incognito mode active; not updating score");

		return;
	}

	if (played_fraction < 0.0 || played_fraction > 1.0)
	{
		// Return if the fraction is invalid or unknown, to prevent modifications to the stats
		g_info("Invalid calculated played fraction");

		return;
	}
	else if (played_fraction >= 1.0)
	{
		// If played equal or more than full_played_fraction say it is fully played

		full_played_fraction = wf_settings_static_get_double(WF_SETTING_FULL_PLAYED_FRACTION);

		if (played_fraction >= full_played_fraction)
		{
			played_fraction = 1.0;
			g_debug("Over full played fraction setting; using a fraction of %f", (float) played_fraction);
		}
	}

	// Get stats
	playcount = wf_song_get_play_count(song);
	old_score = wf_song_get_score(song);

	if (old_score < STAT_SCORE_MIN || old_score > STAT_SCORE_MAX)
	{
		g_warning("Invalid score %f", (float) old_score);

		return;
	}

	if (playcount <= STAT_PLAYCOUNT_MIN)
	{
		// Take average of new and old (default score for new songs should be 50)
		new_score = (old_score + played_fraction * 100.0) / 2.0;
	}
	else
	{
		// Add a bit to the score, depending on the fraction played and the amount of time played
		new_score = ((old_score * (gdouble) playcount) + played_fraction * 100.0) / (playcount + 1);
	}

	wf_stats_update_score(song, CLAMP(new_score, 0.0, 100.0), 0);
}

void
wf_stats_modify_and_update_playcount(WfSong *song, gdouble played_fraction, gboolean decrease)
{
	gdouble min_played_fraction = 0;

	g_return_if_fail(WF_IS_SONG(song));

	if (wf_song_manager_get_incognito()) // If incognito is on
	{
		g_info("Incognito mode active; not updating play count");

		return;
	}

	min_played_fraction = wf_settings_static_get_double(WF_SETTING_MIN_PLAYED_FRACTION);

	if (played_fraction < min_played_fraction)
	{
		g_info("Below minimum played fraction; not updating play count");

		return;
	}

	if (decrease)
	{
		wf_stats_update_playcount(song, 0, -1);
	}
	else
	{
		wf_stats_update_playcount(song, 0, 1);
	}
}

void
wf_stats_modify_and_update_skipcount(WfSong *song, gdouble played_fraction, gboolean decrease)
{
	gdouble full_played_fraction;

	g_return_if_fail(WF_IS_SONG(song));

	if (wf_song_manager_get_incognito()) // If incognito is on
	{
		g_info("Incognito mode active; not updating skip count");

		return;
	}

	full_played_fraction = wf_settings_static_get_double(WF_SETTING_FULL_PLAYED_FRACTION);

	if (played_fraction > full_played_fraction)
	{
		/*
		 * If played more than full_played_fraction say it is fully
		 * played; the user might just want to get to the next song and
		 * skip the silence part at the end.
		 */

		g_info("Above full played fraction; not updating skip count");

		return;
	}

	if (decrease)
	{
		wf_stats_update_skipcount(song, 0, -1);
	}
	else
	{
		wf_stats_update_skipcount(song, 0, 1);
	}
}

void
wf_stats_modify_and_update_lastplayed(WfSong *song, gdouble played_fraction, gint64 time)
{
	gint64 time_now;
	gdouble min_played_fraction;

	g_return_if_fail(WF_IS_SONG(song));

	if (wf_song_manager_get_incognito()) // If incognito is on
	{
		g_info("Incognito mode active; not updating last played");

		return;
	}

	min_played_fraction = wf_settings_static_get_double(WF_SETTING_MIN_PLAYED_FRACTION);

	if (played_fraction < min_played_fraction)
	{
		g_info("Below minimum played fraction; not updating last played");

		return;
	}

	if (time == 0)
	{
		time_now = wf_utils_time_now();
		wf_stats_update_lastplayed(song, time_now, 0);
	}
	else
	{
		wf_stats_update_lastplayed(song, time, 0);
	}
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

gboolean
wf_stats_rating_is_valid(gint rating)
{
	if (STAT_RATING_IS_VALID(rating))
	{
		return TRUE;
	}
	else
	{
		g_debug("Rating %d is invalid", rating);

		return FALSE;
	}
}

gboolean
wf_stats_score_is_valid(gdouble score)
{
	if (STAT_SCORE_IS_VALID(score))
	{
		return TRUE;
	}
	else
	{
		g_debug("Score %lf is invalid", (float) score);

		return FALSE;
	}
}

gboolean
wf_stats_playcount_is_valid(gint playcount)
{
	if (STAT_PLAYCOUNT_IS_VALID(playcount))
	{
		return TRUE;
	}
	else
	{
		g_debug("Play count %d is invalid", playcount);

		return FALSE;
	}
}

gboolean
wf_stats_skipcount_is_valid(gint skipcount)
{
	if (STAT_SKIPCOUNT_IS_VALID(skipcount))
	{
		return TRUE;
	}
	else
	{
		g_debug("Skip count %d is invalid", skipcount);

		return FALSE;
	}
}

gboolean
wf_stats_lastplayed_is_valid(gint64 lastplayed)
{
	if (STAT_LASTPLAYED_IS_VALID(lastplayed))
	{
		return TRUE;
	}
	else
	{
		g_debug("Last played %ld is invalid", (long int) lastplayed);

		return FALSE;
	}
}

// Within range, make low ratings high and high ratings low
gint
wf_stats_rating_invert(gint rating)
{
	g_return_val_if_fail(STAT_RATING_IS_VALID(rating), 0);

	if (rating == 0)
	{
		// Unrated stays unrated
		return 0;
	}
	else
	{
		return ((STAT_RATING_MAX - rating) + STAT_RATING_MIN);
	}
}

// Within range, make low scores high and high score low
gdouble
wf_stats_score_invert(gdouble score)
{
	g_return_val_if_fail(STAT_SCORE_IS_VALID(score), 0.0);

	return ((STAT_SCORE_MAX - score) + STAT_SCORE_MIN);
}

/* MODULE UTILITIES END */

/* END OF FILE */
