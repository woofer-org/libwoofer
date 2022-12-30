/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * intelligence_private.h  This file is part of LibWoofer
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

#ifndef __WF_INTELLIGENCE_PRIVATE__
#define __WF_INTELLIGENCE_PRIVATE__

/* INCLUDES BEGIN */

#include <glib.h>

#include <woofer/song.h>
#include <woofer/intelligence.h>

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

WfSong * wf_intelligence_get_song(GList *filtered_songs, WfSongEntries *preferences);

WfSong *
wf_intelligence_choose_new_song(GList **library,
                                GList *previous_songs,
                                GList *play_next,
                                GList *recent_artists,
                                WfSongFilter *filter,
                                WfSongEntries *entries);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_INTELLIGENCE_PRIVATE__ */

/* END OF FILE */
