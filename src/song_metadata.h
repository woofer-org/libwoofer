/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song_metadata.h  This file is part of LibWoofer
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

#ifndef __WF_SONG_METADATA__
#define __WF_SONG_METADATA__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef struct _WfSongMetadata WfSongMetadata;

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

WfSongMetadata * wf_song_metadata_get_for_uri(const gchar *uri);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

guint wf_song_metadata_get_track_number(const WfSongMetadata *metadata);

guint wf_song_metadata_get_track_count(const WfSongMetadata *metadata);

guint wf_song_metadata_get_volume_number(const WfSongMetadata *metadata);

guint wf_song_metadata_get_volume_count(const WfSongMetadata *metadata);

guint wf_song_metadata_get_serial_number(const WfSongMetadata *metadata);

guint wf_song_metadata_get_bitrate(const WfSongMetadata *metadata);

guint wf_song_metadata_get_bitrate_nominal(const WfSongMetadata *metadata);

guint wf_song_metadata_get_bitrate_minimum(const WfSongMetadata *metadata);

guint wf_song_metadata_get_bitrate_maximum(const WfSongMetadata *metadata);

gdouble wf_song_metadata_get_beats_per_minute(const WfSongMetadata *metadata);

gdouble wf_song_metadata_get_track_gain(const WfSongMetadata *metadata);

gdouble wf_song_metadata_get_track_peak(const WfSongMetadata *metadata);

gdouble wf_song_metadata_get_album_gain(const WfSongMetadata *metadata);

gdouble wf_song_metadata_get_album_peak(const WfSongMetadata *metadata);

guint64 wf_song_metadata_get_duration(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_title(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_title_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_artist(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_artist_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_album(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_album_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_album_artist(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_album_artist_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_show_name(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_show_name_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_genre(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_lyrics(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_organization(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_performer(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_composer(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_composer_sortname(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_conductor(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_contact(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_publisher(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_origin_location(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_homepage(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_description(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_version(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_isrc(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_copyright(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_copyright_uri(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_license(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_license_uri(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_codec(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_codec_video(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_codec_audio(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_codec_subtitle(const WfSongMetadata *metadata);

gchar * wf_song_metadata_get_container_format(const WfSongMetadata *metadata);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean wf_song_metadata_parse(WfSongMetadata *metadata);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */

void wf_song_metadata_free(WfSongMetadata *metadata);

/* DESTRUCTOR PROTOTYPES END */

#endif /* __WF_SONG_METADATA__ */

/* END OF FILE */
