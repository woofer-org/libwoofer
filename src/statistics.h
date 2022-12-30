/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * statistics.h  This file is part of LibWoofer
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

#ifndef __WF_STATISTICS__
#define __WF_STATISTICS__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void wf_stats_update_rating(WfSong *song, gint rating, gint increase);
void wf_stats_update_score(WfSong *song, gdouble score, gdouble increase);
void wf_stats_update_playcount(WfSong *song, gint playcount, gint increase);
void wf_stats_update_skipcount(WfSong *song, gint skipcount, gint increase);
void wf_stats_update_lastplayed(WfSong *song, gint64 lastplayed, gint increase);

void wf_stats_modify_and_update_score(WfSong *song, gdouble played_fraction);
void wf_stats_modify_and_update_playcount(WfSong *song, gdouble played_fraction, gboolean decrease);
void wf_stats_modify_and_update_skipcount(WfSong *song, gdouble played_fraction, gboolean decrease);
void wf_stats_modify_and_update_lastplayed(WfSong *song, gdouble played_fraction, gint64 time);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */

gboolean wf_stats_rating_is_valid(gint rating);
gboolean wf_stats_score_is_valid(gdouble score);
gboolean wf_stats_playcount_is_valid(gint playcount);
gboolean wf_stats_skipcount_is_valid(gint skipcount);
gboolean wf_stats_lastplayed_is_valid(gint64 lastplayed);

gint wf_stats_rating_invert(gint rating);
gdouble wf_stats_score_invert(gdouble score);

/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_STATISTICS__ */

/* END OF FILE */
