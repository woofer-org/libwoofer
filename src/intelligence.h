/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * intelligence.h  This file is part of LibWoofer
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

#ifndef __WF_INTELLIGENCE__
#define __WF_INTELLIGENCE__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

G_BEGIN_DECLS

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef struct _WfSongFilter WfSongFilter;
typedef struct _WfSongEntries WfSongEntries;

/**
 * WfSongFilter:
 * @recent_artists: how many recent artists to use to filter out songs
 * @remove_recents_amount: how many recent songs to filter out
 * @remove_recents_percentage: how many recent songs to filter out defined as a
 * 	percentage of the total amount of songs
 * @use_rating: whether to use the rating filter
 * @use_score: whether to use the rating filter
 * @use_playcount: whether to use the rating filter
 * @use_skipcount: whether to use the rating filter
 * @use_lastplayed: whether to use the rating filter
 * @rating_inc_zero: whether to keep of songs with no rating
 * @playcount_invert: whether to invert the play count threshold
 * @skipcount_invert: whether to invert the skip count threshold
 * @lastplayed_invert: whether to invert the last played threshold
 * @rating_min: minimum rating value to use
 * @rating_max: maximum rating value to use
 * @score_min: minimum score value to use
 * @score_max: minimum score value to use
 * @playcount_th: threshold play count to use
 * @skipcount_th: threshold skip count to use
 * @lastplayed_th: threshold value in seconds since last played to use
 *
 * The #WfFilter contains parameters that define how the algorithm should filter
 * the list of songs.
 **/
struct _WfSongFilter
{
	gint recent_artists;

	gint remove_recents_amount;
	gdouble remove_recents_percentage;

	gboolean use_rating;
	gboolean use_score;
	gboolean use_playcount;
	gboolean use_skipcount;
	gboolean use_lastplayed;

	gboolean rating_inc_zero;

	gboolean playcount_invert;
	gboolean skipcount_invert;
	gboolean lastplayed_invert;

	gint rating_min;
	gint rating_max;
	gint score_min;
	gint score_max;
	gint playcount_th;
	gint skipcount_th;
	gint64 lastplayed_th;
};

/**
 * WfSongEntries:
 * @recent_artists: how many recent artists to use to filter out songs
 * @use_rating: whether to use ratings to determine a songs chance
 * @use_score: whether to use score values to determine a songs chance
 * @use_playcount: whether to use play counts to determine a songs chance
 * @use_skipcount: whether to use skip counts to determine a songs chance
 * @use_lastplayed: whether to use last played values to determine a songs
 * 	chance
 * @invert_rating: whether to invert the increase in chance that is calculated
 * 	with ratings
 * @invert_score: whether to invert the increase in chance that is calculated
 * 	with score values
 * @invert_playcount: whether to invert the increase in chance that is
 * 	calculated with play counts
 * @invert_skipcount: whether to invert the increase in chance that is
 * 	calculated with skip counts
 * @invert_lastplayed: whether to invert the increase in chance that is
 * 	calculated with last played values
 * @use_default_rating: the default rating to use when a song has no rating
 * @rating_multiplier: multiplier to use when calculating the chance using
 * 	ratings
 * @score_multiplier: multiplier to use when calculating the chance using score
 * 	values
 * @playcount_multiplier: multiplier to use when calculating the chance using
 * 	play counts
 * @skipcount_multiplier: multiplier to use when calculating the chance using
 * 	skip counts
 * @lastplayed_multiplier: multiplier to use when calculating the chance using
 * 	last played values
 *
 * The #WfProbabilities contains parameters that define how the algorithm should
 * determine probabilities of the qualified songs.
 **/
struct _WfSongEntries
{
	gboolean use_rating;
	gboolean use_score;
	gboolean use_playcount;
	gboolean use_skipcount;
	gboolean use_lastplayed;

	gboolean invert_rating;
	gboolean invert_score;
	gboolean invert_playcount;
	gboolean invert_skipcount;
	gboolean invert_lastplayed;

	gint use_default_rating;

	gdouble rating_multiplier;
	gdouble score_multiplier;
	gdouble playcount_multiplier;
	gdouble skipcount_multiplier;
	gdouble lastplayed_multiplier;
};

/* MODULE TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

GList *
wf_intelligence_filter(GList *available_songs,
                       GList *previous_songs,
                       GList *play_next,
                       GList *recent_artists,
                       WfSongFilter *filter);

/* FUNCTION PROTOTYPES END */

G_END_DECLS

#endif /* __WF_INTELLIGENCE__ */

/* END OF FILE */
