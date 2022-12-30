/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * song_metadata.c  This file is part of LibWoofer
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

/*
 * Notes:
 * - For all callback functions, the suffix '_cb' is used as a quick indicator
 *   for the reader.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/song_metadata.h>

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This module splits the metadata fetched from a GstElement (uridecodebin) into
 * a custom private data structure.  This structure can be initialized with
 * wf_song_metadata_get_for_file() and a given filename.  The actual process of
 * fetching the metadata is done when calling wf_song_metadata_parse().  The
 * structure gets filled with standard metadata tags and can then be received
 * by using the respective song_metadata_get calls.
 *
 *
 * GStreamer tags available (85 in total)
 * GST_TAG_TITLE
 * GST_TAG_TITLE_SORTNAME
 * GST_TAG_ARTIST
 * GST_TAG_ARTIST_SORTNAME
 * GST_TAG_ALBUM
 * GST_TAG_ALBUM_SORTNAME
 * GST_TAG_ALBUM_ARTIST
 * GST_TAG_ALBUM_ARTIST_SORTNAME
 * GST_TAG_DATE
 * GST_TAG_DATE_TIME
 * GST_TAG_GENRE
 * GST_TAG_COMMENT
 * GST_TAG_EXTENDED_COMMENT
 * GST_TAG_TRACK_NUMBER
 * GST_TAG_TRACK_COUNT
 * GST_TAG_ALBUM_VOLUME_NUMBER
 * GST_TAG_ALBUM_VOLUME_COUNT
 * GST_TAG_LOCATION
 * GST_TAG_HOMEPAGE
 * GST_TAG_DESCRIPTION
 * GST_TAG_VERSION
 * GST_TAG_ISRC
 * GST_TAG_ORGANIZATION
 * GST_TAG_COPYRIGHT
 * GST_TAG_COPYRIGHT_URI
 * GST_TAG_ENCODED_BY
 * GST_TAG_COMPOSER
 * GST_TAG_CONDUCTOR
 * GST_TAG_CONTACT
 * GST_TAG_LICENSE
 * GST_TAG_LICENSE_URI
 * GST_TAG_PERFORMER
 * GST_TAG_DURATION
 * GST_TAG_CODEC
 * GST_TAG_VIDEO_CODEC
 * GST_TAG_AUDIO_CODEC
 * GST_TAG_SUBTITLE_CODEC
 * GST_TAG_CONTAINER_FORMAT
 * GST_TAG_BITRATE
 * GST_TAG_NOMINAL_BITRATE
 * GST_TAG_MINIMUM_BITRATE
 * GST_TAG_MAXIMUM_BITRATE
 * GST_TAG_SERIAL
 * GST_TAG_ENCODER
 * GST_TAG_ENCODER_VERSION
 * GST_TAG_TRACK_GAIN
 * GST_TAG_TRACK_PEAK
 * GST_TAG_ALBUM_GAIN
 * GST_TAG_ALBUM_PEAK
 * GST_TAG_REFERENCE_LEVEL
 * GST_TAG_LANGUAGE_CODE
 * GST_TAG_LANGUAGE_NAME
 * GST_TAG_IMAGE
 * GST_TAG_PREVIEW_IMAGE
 * GST_TAG_ATTACHMENT
 * GST_TAG_BEATS_PER_MINUTE
 * GST_TAG_KEYWORDS
 * GST_TAG_GEO_LOCATION_NAME
 * GST_TAG_GEO_LOCATION_LATITUDE
 * GST_TAG_GEO_LOCATION_LONGITUDE
 * GST_TAG_GEO_LOCATION_ELEVATION
 * GST_TAG_GEO_LOCATION_CITY
 * GST_TAG_GEO_LOCATION_COUNTRY
 * GST_TAG_GEO_LOCATION_SUBLOCATION
 * GST_TAG_GEO_LOCATION_HORIZONTAL_ERROR
 * GST_TAG_GEO_LOCATION_MOVEMENT_DIRECTION
 * GST_TAG_GEO_LOCATION_MOVEMENT_SPEED
 * GST_TAG_GEO_LOCATION_CAPTURE_DIRECTION
 * GST_TAG_SHOW_NAME
 * GST_TAG_SHOW_SORTNAME
 * GST_TAG_SHOW_EPISODE_NUMBER
 * GST_TAG_SHOW_SEASON_NUMBER
 * GST_TAG_LYRICS
 * GST_TAG_COMPOSER_SORTNAME
 * GST_TAG_GROUPING
 * GST_TAG_USER_RATING
 * GST_TAG_DEVICE_MANUFACTURER
 * GST_TAG_DEVICE_MODEL
 * GST_TAG_APPLICATION_NAME
 * GST_TAG_APPLICATION_DATA
 * GST_TAG_IMAGE_ORIENTATION
 * GST_TAG_PUBLISHER
 * GST_TAG_INTERPRETED_BY
 * GST_TAG_MIDI_BASE_NOTE
 * GST_TAG_PRIVATE_DATA
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Time to wait for messages on the bus containing the metadata tags (seconds)
#define TAG_BUS_TIMEOUT 3

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

struct _WfSongMetadata
{
	GstTagList *list;
	gchar *uri;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static gboolean wf_song_metadata_init(WfSongMetadata *metadata, const gchar *uri);

static gchar * wf_song_metadata_get_string(const WfSongMetadata *metadata, const gchar *tag);
static guint wf_song_metadata_get_uint(const WfSongMetadata *metadata, const gchar *tag);
static guint64 wf_song_metadata_get_uint64(const WfSongMetadata *metadata, const gchar *tag);

static void wf_song_metadata_pad_added_cb(GstElement *decoder, GstPad *pad, GstElement *fakesink);

static const gchar * wf_song_metadata_get_type_name(const GType type);
static const GValue * wf_song_metadata_get_tag(GstTagList *list, const gchar *tag, const guint index, const GType type);
static const GValue * wf_song_metadata_get_first_tag(GstTagList *list, const gchar *tag, const GType type);

static void wf_song_metadata_finalize(WfSongMetadata *metadata);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

WfSongMetadata *
wf_song_metadata_get_for_uri(const gchar *uri)
{
	WfSongMetadata *metadata;

	g_return_val_if_fail(uri != NULL, NULL);

	metadata = g_slice_alloc0(sizeof(WfSongMetadata));

	if (metadata == NULL)
	{
		g_warning("Failed to allocate storage for metadata parsing!");

		return NULL;
	}
	else if (!wf_song_metadata_init(metadata, uri))
	{
		g_slice_free1(sizeof(WfSongMetadata), metadata);

		return NULL;
	}

	return metadata;
}

static gboolean
wf_song_metadata_init(WfSongMetadata *metadata, const gchar *uri)
{
	g_return_val_if_fail(metadata != NULL, FALSE);
	g_return_val_if_fail(uri != NULL, FALSE);

	if (gst_uri_is_valid(uri))
	{
		metadata->uri = g_strdup(uri);

		return TRUE;
	}
	else
	{
		g_warning("Failed to build filepath for metadata parsing!");

		return FALSE;
	}
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

static gchar *
wf_song_metadata_get_string(const WfSongMetadata *metadata, const gchar *tag)
{
	const GType type = G_TYPE_STRING;
	const GValue *v;
	GstTagList *list;
	gint num, x;
	gchar *str = NULL, *tmp;

	g_return_val_if_fail(metadata != NULL, NULL);
	g_return_val_if_fail(tag != NULL, NULL);

	list = metadata->list;

	// Get amount of tags
	num = gst_tag_list_get_tag_size(list, tag);

	if (num == 0)
	{
		return NULL;
	}

	// Get the first tag
	v = wf_song_metadata_get_first_tag(list, tag, type);

	if (v != NULL)
	{
		// Copy content
		str = g_strdup(g_value_get_string(v));
	}

	if (num > 1)
	{
		// Get all other tags
		for (x = 1; x < num; x++)
		{
			v = wf_song_metadata_get_tag(list, tag, x, type);

			if (v != NULL)
			{
				// Copy content
				tmp = str;
				str = g_strdup_printf("%s, %s", tmp, g_value_get_string(v));
				g_free(tmp);
			}
		}
	}

	return str;
}

static guint
wf_song_metadata_get_uint(const WfSongMetadata *metadata, const gchar *tag)
{
	const GType type = G_TYPE_UINT;
	const GValue *v;
	GstTagList *list;

	g_return_val_if_fail(metadata != NULL, 0);
	g_return_val_if_fail(tag != NULL, 0);

	list = metadata->list;

	v = wf_song_metadata_get_first_tag(list, tag, type);

	if (v == NULL)
	{
		return 0;
	}
	else
	{
		return g_value_get_uint(v);
	}
}

static guint64
wf_song_metadata_get_uint64(const WfSongMetadata *metadata, const gchar *tag)
{
	const GType type = G_TYPE_UINT64;
	const GValue *v;
	GstTagList *list;

	g_return_val_if_fail(metadata != NULL, 0);
	g_return_val_if_fail(tag != NULL, 0);

	list = metadata->list;

	v = wf_song_metadata_get_first_tag(list, tag, type);

	if (v == NULL)
	{
		return 0;
	}
	else
	{
		return g_value_get_uint64(v);
	}
}

/*
 * GetMetadata function implementations (not all tags GStreamer provides are available here)
 */

guint
wf_song_metadata_get_track_number(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_uint(metadata, GST_TAG_TRACK_NUMBER);
}

guint
wf_song_metadata_get_track_count(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_uint(metadata, GST_TAG_TRACK_COUNT);
}

//guint wf_song_metadata_get_volume_number(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_volume_count(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_serial_number(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_bitrate(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_bitrate_nominal(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_bitrate_minimum(const WfSongMetadata *metadata) {}

//guint wf_song_metadata_get_bitrate_maximum(const WfSongMetadata *metadata) {}

//gdouble wf_song_metadata_get_beats_per_minute(const WfSongMetadata *metadata) {}

//gdouble wf_song_metadata_get_track_gain(const WfSongMetadata *metadata) {}

//gdouble wf_song_metadata_get_track_peak(const WfSongMetadata *metadata) {}

//gdouble wf_song_metadata_get_album_gain(const WfSongMetadata *metadata) {}

//gdouble wf_song_metadata_get_album_peak() {}

guint64
wf_song_metadata_get_duration(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_uint64(metadata, GST_TAG_DURATION);
}

gchar *
wf_song_metadata_get_title(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_string(metadata, GST_TAG_TITLE);
}

//gchar * wf_song_metadata_get_title_sortname() {}

gchar *
wf_song_metadata_get_artist(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_string(metadata, GST_TAG_ARTIST);
}

//gchar * wf_song_metadata_get_artist_sortname() {}

gchar *
wf_song_metadata_get_album(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_string(metadata, GST_TAG_ALBUM);
}

//gchar * wf_song_metadata_get_album_sortname() {}

gchar *
wf_song_metadata_get_album_artist(const WfSongMetadata *metadata)
{
	return wf_song_metadata_get_string(metadata, GST_TAG_ALBUM_ARTIST);
}

//gchar * wf_song_metadata_get_album_artist_sortname(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_show_name(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_show_name_sortname(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_genre(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_lyrics(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_organization(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_performer(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_composer(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_composer_sortname(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_conductor(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_contact(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_publisher(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_origin_location(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_homepage(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_description(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_version(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_isrc(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_copyright(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_copyright_uri(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_license(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_license_uri(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_codec(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_codec_video(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_codec_audio(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_codec_subtitle(const WfSongMetadata *metadata) {}

//gchar * wf_song_metadata_get_container_format() {}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static void
wf_song_metadata_pad_added_cb(GstElement *decoder, GstPad *pad, GstElement *fakesink)
{
	GstPad *sinkpad = gst_element_get_static_pad(fakesink, "sink");

	// Check if pad is already linked
	if (!gst_pad_is_linked(sinkpad))
	{
		// Now link the pad
		if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK)
		{
			g_warning("Failed to link pads for metadata fetching!");
		}
	}

	gst_object_unref(sinkpad);
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

gboolean
wf_song_metadata_parse(WfSongMetadata *metadata)
{
	const GstMessageType wait_types = GST_MESSAGE_ERROR | GST_MESSAGE_TAG;
	GstElement *pipeline, *decode, *sink;
	GstMessage *msg;
	GstTagList *tags = NULL;
	GError *err = NULL;
	gboolean result = TRUE;
	gchar *str = NULL;

	g_return_val_if_fail(metadata != NULL, FALSE);

	pipeline = gst_pipeline_new("pipeline");
	decode = gst_element_factory_make("uridecodebin", NULL /* name */);
	g_object_set(decode, "uri", metadata->uri, NULL /* terminator */);
	gst_bin_add(GST_BIN(pipeline), decode);

	sink = gst_element_factory_make("fakesink", NULL /* name */);
	gst_bin_add(GST_BIN(pipeline), sink);

	g_signal_connect(decode, "pad-added", G_CALLBACK(wf_song_metadata_pad_added_cb), sink);

	gst_element_set_state(pipeline, GST_STATE_PAUSED);

	if (TAG_BUS_TIMEOUT <= 0)
	{
		msg = gst_bus_pop_filtered(GST_ELEMENT_BUS(pipeline), wait_types);
	}
	else
	{
		msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(pipeline), TAG_BUS_TIMEOUT * GST_SECOND, wait_types);
	}

	if (msg == NULL)
	{
		g_warning("Could not get metadata tags from empty GstMessage");
		result = FALSE;
	}
	else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
	{
		gst_message_parse_error(msg, &err, NULL /* debug */);
		
		if (err != NULL)
		{
			str = err->message;
		}

		g_warning("Could not get metadata tags from GstMessage: %s", str);
		result = FALSE;

		g_clear_error(&err);
	}
	else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_TAG)
	{
		gst_message_parse_tag(msg, &tags);

		metadata->list = tags;
		result = TRUE;
	}
	else
	{
		g_warning("GStreamer message from element %s: %d", GST_OBJECT_NAME(msg->src), GST_MESSAGE_TYPE(msg));
		result = FALSE;
	}

	if (msg != NULL)
	{
		gst_message_unref(msg);
	}

	gst_element_set_state(pipeline, GST_STATE_NULL);

	if (pipeline != NULL)
	{
		gst_object_unref(pipeline);
	}

	return result;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static const gchar *
wf_song_metadata_get_type_name(const GType type)
{
	switch (type)
	{
		case G_TYPE_INVALID:
			return "invalid";
		case G_TYPE_NONE:
			return "none";
		case G_TYPE_INTERFACE:
			return "interface";
		case G_TYPE_CHAR:
			return "char";
		case G_TYPE_UCHAR:
			return "uchar";
		case G_TYPE_BOOLEAN:
			return "bool";
		case G_TYPE_INT:
			return "int";
		case G_TYPE_UINT:
			return "uint";
		case G_TYPE_LONG:
			return "long";
		case G_TYPE_ULONG:
			return "ulong";
		case G_TYPE_INT64:
			return "int64";
		case G_TYPE_UINT64:
			return "uint64";
		case G_TYPE_ENUM:
			return "enum";
		case G_TYPE_FLAGS:
			return "flags";
		case G_TYPE_FLOAT:
			return "float";
		case G_TYPE_DOUBLE:
			return "double";
		case G_TYPE_STRING:
			return "string";
		default:
			return "unused";
	}
}

static const GValue *
wf_song_metadata_get_tag(GstTagList *list, const gchar *tag, const guint index, const GType type)
{
	const gchar *str1, *str2;
	const GValue *val;
	GType t;

	g_return_val_if_fail(GST_IS_TAG_LIST(list), NULL);
	g_return_val_if_fail(tag != NULL, NULL);

	val = gst_tag_list_get_value_index(list, tag, index);

	if (val == NULL)
	{
		// This tag does not seem to exist
		return NULL;
	}
	else if (G_VALUE_HOLDS(val, type))
	{
		// Right value, return it
		return val;
	}
	else
	{
		// Wrong type

		t = G_VALUE_TYPE(val);
		str1 = wf_song_metadata_get_type_name(type);
		str2 = wf_song_metadata_get_type_name(t);

		g_warning("Metadata value from '%s' does not hold a value of type %s but rather %s", tag, str1, str2);

		return NULL;
	}
}

static const GValue *
wf_song_metadata_get_first_tag(GstTagList *list, const gchar *tag, const GType type)
{
	return wf_song_metadata_get_tag(list, tag, 0 /* index */, type);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_song_metadata_free(WfSongMetadata *metadata)
{
	wf_song_metadata_finalize(metadata);

	g_slice_free1(sizeof(WfSongMetadata), metadata);
}

static void
wf_song_metadata_finalize(WfSongMetadata *metadata)
{
	if (metadata != NULL)
	{
		if (metadata->list != NULL)
		{
			gst_tag_list_unref(metadata->list);
		}

		g_free(metadata->uri);

		metadata->list = NULL;
		metadata->uri = NULL;
	}
}

/* DESTRUCTORS END */

/* END OF FILE */
