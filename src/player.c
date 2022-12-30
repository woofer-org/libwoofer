/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * player.c  This file is part of LibWoofer
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
#include <gio/gio.h>
#include <gst/gst.h>

// Global includes
/*< none >*/

// Module includes
#include <woofer/player.h>

// Dependency includes
#include <woofer/song.h>
#include <woofer/song_private.h>
#include <woofer/song_manager.h>
#include <woofer/settings.h>
#include <woofer/mpris.h>
#include <woofer/utils.h>
#include <woofer/memory.h>

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This is one of the core modules and handles the playback: processing
 * events, creating the necessary GStreamer objects, managing playback states
 * and messages and generally making sure that a given input source is
 * converted to audio.
 *
 * The methods of this module can be divided into roughly three categories:
 * - General player calls (often public)
 * - Player signals (that handle callbacks to front-end)
 * - Pipeline functions (that handle the interaction with GStreamer)
 * - Message functions (that processes messages received from GStreamer)
 * - Remote functions (for the interaction with the MPRIS implementation)
 *
 * GStreamer uses some terms that are important to understand the process:
 * GStreamer elements are parts of a more complete pipeline (although a
 * 	pipeline is also an element).
 * GStreamer pipelines control the flow of information from a source (input)
 * 	to a sink (output).
 * GStreamer pads link the elements together so the information can flow from
 * 	one element to another.  These pads can be static, dynamic or on-request.
 * 	Static pads (or "always" pads) always exist (as long as the element
 * 	exists), while dynamic pads (or "sometimes" pads) are created on the go.
 * 	On-request pads only exist if anything requests them.
 * GStreamer buffers hold a chunk of data that flows through elements.
 * GStreamer decoders convert raw data e.g. from a file to more useful (audio)
 * data that other elements can use.
 *
 * The pipeline constructor creates a pipeline that will handle the process
 * from URI to sound.  The elements that can be used in the pipeline are:
 * - Any source element that handles and reads an URI.
 * - decodebin that handles the raw data decoding process and manages
 *   demuxers.
 * - playsink that processes the decoded data to the sound services.
 *
 * The call gst_element_make_from_uri() takes an URI, constructs a proper
 * source element and returns it.  The element returned can be of a variety of
 * source element types.  The most obvious and probably most used one is filesrc
 * that simply reads data from a file in the local filesystem.  The source
 * element is created and added to the pipeline on the go, as any URI change
 * can change the protocol needed to read the file.
 *
 * Location specific notes:
 * [1] The way volumes are handled is a bit different than just setting
 *     the double value and away you go.  The volume property of GStreamer
 *     elements are linear, but sliders in GUI interfaces should have a
 *     cubic scale (or third root).  Because of this, we need to convert.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Define a separate log domain for GStreamer junk
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN WF_TAG "-player"

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _WfPlayerEvents WfPlayerEvents;
typedef struct _WfPlayerDetails WfPlayerDetails;

struct _WfPlayerEvents
{
	WfFuncReportMsg report_msg;
	WfFuncStateChanged state_changed;
	WfFuncPositionUpdated position_updated;
	WfFuncNotification notification;
};

struct _WfPlayerDetails
{
	WfPlayerEvents events;

	WfPlayerStatus status; // Current state

	GstElementFactory *giostreamfactory; // giostreamsrc element factory
	GstElement *pipeline; // Currently used pipeline
	GstElement *source; // Currently used data source (input)
	GstElement *decoder; // Currently used data decoder (convert)
	GstElement *sink; // Currently used data sink (output)
	GstBus *bus; // Currently used bus
	GstQuery *query_duration; // Query for duration
	GstQuery *query_position; // Query for position

	const gchar *play_msg; // Custom play message
	WfSong *song; // Song currently playing
	gint64 duration; // Duration in nanoseconds of the current pipeline
	gdouble volume; // Currently used volume

	// Volume updated signal handler
	gpointer volume_instance;
	gulong volume_handler;

	guint update_event; // Event source ID for the front-end update interval
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void wf_player_set_volume_internal(gdouble volume);

static gboolean wf_player_message_arrived_cb(GstBus *bus, GstMessage *message, gpointer user_data);
static void wf_player_pipeline_pad_added_cb(GstElement *source_element, GstPad *pad, gpointer user_data);

static gboolean wf_player_update_event_run_cb(gpointer user_data);
static void wf_player_update_event_rm_cb(gpointer data);

static void wf_player_volume_updated_cb(GObject *object, GParamSpec *pspec, gpointer user_data);

static void wf_player_remote_init(void);

static gboolean wf_player_pipeline_construct(void);

static void wf_player_remote_next_cb(void);
static void wf_player_remote_previous_cb(void);
static void wf_player_remote_pause_cb(void);
static void wf_player_remote_play_pause_cb(void);
static void wf_player_remote_stop_cb(void);
static void wf_player_remote_play_cb(void);

static void wf_player_emit_report_msg(WfPlayerEvents *events, const gchar *message);
static void wf_player_emit_state_changed(WfPlayerEvents *events, WfPlayerStatus status, gdouble duration);
static void wf_player_emit_position_updated(WfPlayerEvents *events, gdouble duration, gdouble position);
static void wf_player_emit_notification(WfPlayerEvents *events, WfSong *song, gint64 duration);

static void wf_player_message_eos(GstMessage *msg);
static void wf_player_message_error(GstMessage *msg);
static void wf_player_message_warning(GstMessage *msg);
static void wf_player_message_info(GstMessage *msg);
static void wf_player_message_buffering(GstMessage *msg);
static void wf_player_message_state_changed(GstMessage *msg);
static void wf_player_message_async_done(GstMessage *msg);
static void wf_player_message_stream_start(GstMessage *msg);

static void wf_player_state_ready(void);
static void wf_player_state_paused(void);
static void wf_player_state_playing(void);

static void wf_player_remote_update(void);
static void wf_player_remote_reset(void);

static void wf_player_songs_updated(void);
static void wf_player_report_playing(void);

static void wf_player_pipeline_add_source(GstElement *element);
static GstElement * wf_player_pipeline_memory_source_get(void);
static gboolean wf_player_pipeline_memory_source_set_file(GstElement *source, GFile *file);

static void wf_player_pipeline_open(WfSong *song);
static void wf_player_pipeline_set_song(WfSong *song);
static void wf_player_pipeline_update_volume(void);
static void wf_player_pipeline_play(void);
static void wf_player_pipeline_pause(void);
static void wf_player_pipeline_ready(void);
static void wf_player_pipeline_stop(void);
static void wf_player_pipeline_seek(gdouble position);
static gboolean wf_player_pipeline_has_data(void);
static gint64 wf_player_pipeline_get_duration(void);
static gint64 wf_player_pipeline_get_position(void);

static void wf_player_update_event_update(void);

static gdouble wf_player_get_played_fraction(void);

static void wf_player_finish_song(void);
static void wf_player_finish_song_error(void);
static void wf_player_play_next_song(void);
static void wf_player_play_prev_song(void);
static void wf_player_seek(gint64 position);

static void wf_player_remote_finalize(void);
static void wf_player_pipeline_destruct(void);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static WfPlayerDetails PlayerData =
{
	.status = WF_PLAYER_NO_STATUS,
	.volume = 1.0,
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
wf_player_init(void)
{
	gdouble volume;

	// Modules initialization
	wf_song_manager_init();
	wf_player_remote_init();

	// Pipeline construction and playback preparations
	if (wf_player_pipeline_construct())
	{
		wf_player_pipeline_ready();
		wf_song_manager_sync();

		// Setting volume
		volume = wf_settings_static_get_double(WF_SETTING_VOLUME);
		volume = CLAMP(volume / 100.0, 0.0, 1.0);
		wf_player_set_volume_internal(volume);
	}

	// Now ready
	PlayerData.status = WF_PLAYER_INIT;
}

// Player remote handles information for MPRIS clients
static void
wf_player_remote_init(void)
{
	// Connect to MPRIS signals
	wf_mpris_connect_player_next(wf_player_remote_next_cb);
	wf_mpris_connect_player_previous(wf_player_remote_previous_cb);
	wf_mpris_connect_player_pause(wf_player_remote_pause_cb);
	wf_mpris_connect_player_play_pause(wf_player_remote_play_pause_cb);
	wf_mpris_connect_player_stop(wf_player_remote_stop_cb);
	wf_mpris_connect_player_play(wf_player_remote_play_cb);

	// Set MPRIS properties
	wf_mpris_set_player_playback_status(WF_MPRIS_STOPPED);
	wf_mpris_set_player_minimum_rate(1.0);
	wf_mpris_set_player_maximum_rate(1.0);
	wf_mpris_set_player_rate(1.0);
	wf_mpris_set_player_volume(1.0);
	wf_mpris_set_player_can_go_next(TRUE);
	wf_mpris_set_player_can_go_previous(TRUE);
	wf_mpris_set_player_can_play(TRUE);
	wf_mpris_set_player_can_pause(TRUE);
	wf_mpris_set_player_can_seek(FALSE);
	wf_mpris_set_player_can_control(TRUE);

	// Try to activate
	wf_mpris_activate();
}

static gboolean
wf_player_pipeline_construct(void)
{
	GstElement *pipeline;
	GstElement *decoder = NULL;
	GstElement *sink = NULL;
	GstBus *bus;

	if (PlayerData.pipeline != NULL)
	{
		// Pipeline already constructed; nothing to do
		return TRUE;
	}

	// Create elements
	PlayerData.pipeline = pipeline = gst_pipeline_new("pipeline");
	PlayerData.decoder = decoder = gst_element_factory_make("decodebin", "decoder");
	PlayerData.sink = sink = gst_element_factory_make("playsink", "sink");
	PlayerData.duration = GST_CLOCK_TIME_NONE;

	// Return if any elements are not present
	if (pipeline == NULL || decoder == NULL || sink == NULL)
	{
		g_warning("Could not create one or more GStreamer elements");
		wf_player_pipeline_destruct();

		return FALSE;
	}

	// Set sink (output) properties
	gst_util_set_object_arg(G_OBJECT(sink), "flags", "audio+soft-volume+buffering");

	// Connect to the "volume" property to give it back to the application
	PlayerData.volume_instance = sink;
	PlayerData.volume_handler = g_signal_connect(PlayerData.volume_instance, "notify::volume", G_CALLBACK(wf_player_volume_updated_cb), NULL /* user_data */);

	// Add elements to pipeline (this will transfer ownership of the elements)
	gst_bin_add(GST_BIN(pipeline), decoder);
	gst_bin_add(GST_BIN(pipeline), sink);

	// Linking dynamic pads when they become available by the element
	g_signal_connect(decoder, "pad-added", G_CALLBACK(wf_player_pipeline_pad_added_cb), sink);

	// Get pipeline bus
	PlayerData.bus = bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

	// Add bus watch
	gst_bus_add_watch(bus, wf_player_message_arrived_cb, NULL /* user_data */);

	// Allocate structures to query duration and position
	PlayerData.query_duration = gst_query_new_duration(GST_FORMAT_TIME);

	return TRUE;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

void
wf_player_connect_event_report_msg(WfFuncReportMsg cb_func)
{
	PlayerData.events.report_msg = cb_func;
}

void
wf_player_connect_event_state_changed(WfFuncStateChanged cb_func)
{
	PlayerData.events.state_changed = cb_func;
}

void
wf_player_connect_event_position_updated(WfFuncPositionUpdated cb_func)
{
	PlayerData.events.position_updated = cb_func;
}

void
wf_player_connect_event_notification(WfFuncNotification cb_func)
{
	PlayerData.events.notification = cb_func;
}

gdouble
wf_player_get_volume(void)
{
	gdouble volume;

	// Use third power (see note [1] at module description)
	volume = wf_utils_third_root(PlayerData.volume);

	return volume;
}

gdouble
wf_player_get_volume_percentage(void)
{
	gdouble volume = wf_player_get_volume();

	return (volume * 100.0);
}

void
wf_player_set_volume(gdouble volume)
{
	// Cap the volume to the range
	volume = CLAMP(volume, 0.0, 1.0);

	// Save the value
	wf_settings_static_set_double(WF_SETTING_VOLUME, volume * 100.0);
	wf_settings_queue_write();

	// Set the actual value
	wf_player_set_volume_internal(volume);
}

void
wf_player_set_volume_percentage(gdouble volume)
{
	// Cap the volume to the range
	volume = CLAMP(volume, 0.0, 100.0);

	// Save the value
	wf_settings_static_set_double(WF_SETTING_VOLUME, volume);
	wf_settings_queue_write();

	// Set the actual value
	wf_player_set_volume_internal(volume / 100.0);
}

static void
wf_player_set_volume_internal(gdouble volume)
{
	// Use third power (see note [1] at module description)
	volume = wf_utils_third_power(volume);

	// Set volume
	PlayerData.volume = volume;

	// Apply volume
	wf_player_pipeline_update_volume();
}

WfPlayerStatus
wf_player_get_status(void)
{
	return PlayerData.status;
}

WfSong *
wf_player_get_current_song(void)
{
	return PlayerData.song;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static gboolean
wf_player_message_arrived_cb(GstBus *bus, GstMessage *message, gpointer user_data)
{
	// Handle most of the message types to separate functions
	switch (GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_EOS:
			wf_player_message_eos(message);
			break;
		case GST_MESSAGE_ERROR:
			wf_player_message_error(message);
			break;
		case GST_MESSAGE_WARNING:
			wf_player_message_warning(message);
			break;
		case GST_MESSAGE_INFO:
			wf_player_message_info(message);
			break;
		case GST_MESSAGE_TAG:
			break;
		case GST_MESSAGE_BUFFERING:
			wf_player_message_buffering(message);
			break;
		case GST_MESSAGE_STATE_CHANGED:
			wf_player_message_state_changed(message);
			break;
		case GST_MESSAGE_STEP_DONE:
		case GST_MESSAGE_CLOCK_PROVIDE:
		case GST_MESSAGE_CLOCK_LOST:
		case GST_MESSAGE_NEW_CLOCK:
		case GST_MESSAGE_STRUCTURE_CHANGE:
		case GST_MESSAGE_STREAM_STATUS:
		case GST_MESSAGE_APPLICATION:
		case GST_MESSAGE_ELEMENT:
		case GST_MESSAGE_SEGMENT_START:
		case GST_MESSAGE_SEGMENT_DONE:
		case GST_MESSAGE_DURATION_CHANGED:
		case GST_MESSAGE_LATENCY:
			break;
		case GST_MESSAGE_ASYNC_DONE:
			wf_player_message_async_done(message);
			break;
		case GST_MESSAGE_REQUEST_STATE:
		case GST_MESSAGE_STEP_START:
		case GST_MESSAGE_QOS: // Quality of service
			break;
		case GST_MESSAGE_PROGRESS:
		case GST_MESSAGE_TOC: // Table of content
		case GST_MESSAGE_RESET_TIME:
			break;
		case GST_MESSAGE_STREAM_START:
			wf_player_message_stream_start(message);
			break;
		default:
			break; // Do nothing
	}

	return G_SOURCE_CONTINUE; // %TRUE
}

static void
wf_player_pipeline_pad_added_cb(GstElement *source_element, GstPad *pad, gpointer user_data)
{
	GstElement *sink_element = (GstElement *) user_data;

	g_return_if_fail(GST_IS_ELEMENT(source_element));
	g_return_if_fail(GST_IS_ELEMENT(sink_element));

	// Link pads
	gst_element_link(source_element, sink_element);
}

// Executed in a specified interval.  Report the duration and position of the player to the front-end
static gboolean
wf_player_update_event_run_cb(gpointer user_data)
{
	gint64 position;
	gint64 duration;
	gdouble d_pos;
	gdouble d_dur;

	if (wf_player_is_active())
	{
		// Get information
		position = wf_player_pipeline_get_position();
		duration = wf_player_pipeline_get_duration();

		// Convert to seconds
		d_pos = GST_TIME_AS_SECONDS((gdouble) position);
		d_dur = GST_TIME_AS_SECONDS((gdouble) duration);

		if (d_dur > 0.0 && d_pos >= 0.0)
		{
			wf_player_emit_position_updated(&PlayerData.events, d_pos, d_dur);
		}

		return G_SOURCE_CONTINUE; // %TRUE
	}
	else
	{
		PlayerData.update_event = 0;

		return G_SOURCE_REMOVE; // FALSE
	}
}

// Event source got removed
static void
wf_player_update_event_rm_cb(gpointer data)
{
	PlayerData.update_event = 0;
}

static void
wf_player_volume_updated_cb(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	gdouble volume = -1.0;

	g_return_if_fail(G_IS_OBJECT(object));

	g_object_get(object, "volume", &volume, NULL /* terminator */);

	// Cap to max 100%
	if (volume > 1.0)
	{
		volume = 1.0;
	}

	// Only update if positive
	if (volume >= 0.0)
	{
		PlayerData.volume = volume;

		// Use third root to save value (see note [1] at module description)
		volume = wf_utils_third_root(volume);
		wf_settings_static_set_double(WF_SETTING_VOLUME, volume * 100.0);
	}
}

static void
wf_player_remote_next_cb(void)
{
	g_info("Remote: Next");

	wf_player_forward(FALSE);
}

static void
wf_player_remote_previous_cb(void)
{
	g_info("Remote: Previous");

	wf_player_backward(FALSE);
}

static void
wf_player_remote_pause_cb(void)
{
	g_info("Remote: Pause");

	wf_player_pause();
}

static void
wf_player_remote_play_pause_cb(void)
{
	g_info("Remote: Play/Pause");

	wf_player_play_pause();
}

static void
wf_player_remote_stop_cb(void)
{
	g_info("Remote: Stop");

	wf_player_stop();
}

static void
wf_player_remote_play_cb(void)
{
	g_info("Remote: Play");

	wf_player_play();
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

static void
wf_player_emit_report_msg(WfPlayerEvents *events, const gchar *message)
{
	g_return_if_fail(events != NULL);

	if (events->report_msg != NULL)
	{
		events->report_msg(message);
	}
}

static void
wf_player_emit_state_changed(WfPlayerEvents *events, WfPlayerStatus status, gdouble duration)
{
	g_return_if_fail(events != NULL);

	if (events->state_changed != NULL)
	{
		events->state_changed(status, duration);
	}
}

static void
wf_player_emit_position_updated(WfPlayerEvents *events, gdouble duration, gdouble position)
{
	g_return_if_fail(events != NULL);

	if (events->position_updated != NULL)
	{
		events->position_updated(duration, position);
	}
}

static void
wf_player_emit_notification(WfPlayerEvents *events, WfSong *song, gint64 duration)
{
	g_return_if_fail(events != NULL);

	if (events->notification != NULL)
	{
		events->notification(song, duration);
	}
}

static void
wf_player_message_eos(GstMessage *msg)
{
	g_info("Reached end of stream");

	// Warn if the current song is not available
	g_warn_if_fail(PlayerData.song != NULL);

	// Stop if stop flag is set
	if (PlayerData.song != NULL && wf_song_get_stop_flag(PlayerData.song))
	{
		wf_player_stop();
	}
	else
	{
		PlayerData.play_msg = "Going forward";

		wf_player_play_next_song();
	}
}

static void
wf_player_message_error(GstMessage *msg)
{
	WfSong *song;
	const gchar *error_msg;
	gchar *info = NULL;
	GError *error = NULL;

	g_return_if_fail(GST_IS_MESSAGE(msg));

	song = PlayerData.song;

	// Update statistics, even though they might not be accurate
	wf_player_finish_song_error();
	wf_player_pipeline_stop();

	// Error handling
	gst_message_parse_error(msg, &error, &info);

	// Console printing
	error_msg = (error != NULL) ? error->message : NULL;
	g_warning("Playback error from %s: %s", GST_OBJECT_NAME(msg->src), error_msg);
	g_debug("Debug info: %s", info);

	// Song status update
	if (song != NULL)
	{
		if (error->domain == GST_RESOURCE_ERROR &&
		    (error->code == GST_RESOURCE_ERROR_NOT_FOUND ||
		     error->code == GST_RESOURCE_ERROR_OPEN_READ ||
		     error->code == GST_RESOURCE_ERROR_READ ||
		     error->code == GST_RESOURCE_ERROR_NOT_AUTHORIZED))
		{
			wf_song_set_status(song, WF_SONG_NOT_FOUND);
		}
		else if (wf_song_get_status(PlayerData.song) == WF_SONG_PLAYING)
		{
			wf_song_set_status(song, WF_SONG_AVAILABLE);
		}
	}

	if (error->domain == GST_CORE_ERROR &&
	    error->code == GST_CORE_ERROR_STATE_CHANGE)
	{
		/*
		 * Do not report this error message to the front-end, as it can
		 * be a pretty useless message like "some element failed to
		 * post a proper error message with the reason for the failure",
		 * while in fact it does have a proper message, that has already
		 * been posted on the bus.
		 */
	}
	else
	{
		// Report error to the front-end
		error_msg = (error_msg != NULL) ? error_msg : "A playback error occurred, see console for details";
		wf_player_emit_report_msg(&PlayerData.events, error_msg);
	}

	// Report song changes
	wf_player_songs_updated();

	g_clear_error(&error);
	g_free(info);

	wf_song_manager_sync();
}

static void
wf_player_message_warning(GstMessage *msg)
{
	const gchar *warn_msg;
	gchar *info = NULL;
	GError *error = NULL;

	g_return_if_fail(GST_IS_MESSAGE(msg));

	gst_message_parse_warning(msg, &error, &info);

	warn_msg = (error != NULL) ? error->message : NULL;
	g_message("Playback warning from %s: %s", GST_OBJECT_NAME(msg->src), warn_msg);
	g_debug("Debug info: %s", info);

	g_clear_error(&error);
	g_free(info);
}

static void
wf_player_message_info(GstMessage *msg)
{
	const gchar *info_msg;
	gchar *info = NULL;
	GError *error = NULL;

	g_return_if_fail(GST_IS_MESSAGE(msg));

	gst_message_parse_info(msg, &error, &info);

	info_msg = (error != NULL) ? error->message : NULL;
	g_message("Playback info from %s: %s", GST_OBJECT_NAME(msg->src), info_msg);
	g_debug("Debug info: %s", info);

	g_clear_error(&error);
	g_free(info);
}

static void
wf_player_message_buffering(GstMessage *msg)
{
	gint percentage = 0;

	g_return_if_fail(GST_IS_MESSAGE(msg));

	gst_message_parse_buffering(msg, &percentage);

	g_info("Buffering (%d%%)", percentage);
}

static void
wf_player_message_state_changed(GstMessage *msg)
{
	GstState oldstate = GST_STATE_NULL;
	GstState newstate = GST_STATE_NULL;

	gst_message_parse_state_changed(msg, &oldstate, &newstate, NULL /* pending */);

	if (oldstate == newstate)
	{
		// Ignore transitions to the same state
		return;
	}
	else if (GST_MESSAGE_SRC(msg) != GST_OBJECT(PlayerData.pipeline))
	{
		// Do not track state changes or elements other than the main pipeline
		return;
	}

	switch (newstate)
	{
		case GST_STATE_VOID_PENDING:
			break;
		case GST_STATE_NULL:
			break;
		case GST_STATE_READY:
			wf_player_state_ready();
			return;
		case GST_STATE_PAUSED:
			wf_player_state_paused();
			break;
		case GST_STATE_PLAYING:
			wf_player_state_playing();
			break;
		default:
			g_warning("Unknown state change");
			return;
	}
}

static void
wf_player_message_async_done(GstMessage *msg)
{
	g_info("Asynchronous state change done");
}

static void
wf_player_message_stream_start(GstMessage *msg)
{
	g_info("Player started playback");

	// Just started playing, so change state
	PlayerData.status = WF_PLAYER_PLAYING;

	// Update song manager
	wf_song_manager_song_is_playing(PlayerData.song);

	// Get duration
	PlayerData.duration = wf_player_pipeline_get_duration();

	// Update MPRIS
	wf_player_remote_update();
	wf_mpris_flush_changes();

	// Let the song manager do what it needs to do
	wf_song_manager_sync();

	// Send notification
	wf_player_emit_notification(&PlayerData.events, PlayerData.song, PlayerData.duration);

	// Set up an event interval to update the front-end
	wf_player_update_event_update();
}

static void
wf_player_state_ready(void)
{
	g_info("Player is now ready");

	if (PlayerData.status == WF_PLAYER_READY)
	{
		// If stop was demanded (state is ready), post a message
		wf_player_emit_report_msg(&PlayerData.events, "Stopped");
	}

	if (PlayerData.status == WF_PLAYER_READY ||
	    PlayerData.status == WF_PLAYER_STOPPED)
	{
		// Player just stopped: report song changes
		wf_player_songs_updated();

		// Update MPRIS
		wf_mpris_set_player_playback_status(WF_MPRIS_STOPPED);
		wf_player_remote_reset();
		wf_mpris_flush_changes();

		// Update notifications
		wf_player_emit_notification(&PlayerData.events, NULL /* song */, 0 /* duration */);

		// Process any pending tasks
		wf_song_manager_sync();
	}

	wf_player_emit_position_updated(&PlayerData.events, 0.0, 0.0);
}

static void
wf_player_state_paused(void)
{
	g_info("Player is now paused");

	if (PlayerData.status == WF_PLAYER_PAUSED)
	{
		wf_player_emit_report_msg(&PlayerData.events, "Paused");
		wf_player_songs_updated();

		// Update MPRIS
		wf_mpris_set_player_playback_status(WF_MPRIS_PAUSED);
		wf_mpris_flush_changes();
	}
}

static void
wf_player_state_playing(void)
{
	g_info("Player is now playing");

	if (PlayerData.status == WF_PLAYER_PLAYING)
	{
		// Update status
		wf_song_set_status(PlayerData.song, WF_SONG_PLAYING);

		// Report changes
		wf_player_report_playing();
		wf_player_songs_updated();

		// Update MPRIS
		wf_mpris_set_player_playback_status(WF_MPRIS_PLAYING);
		wf_mpris_flush_changes();
	}
}

static void
wf_player_remote_update(void)
{
	const gchar *title, *album, *artist;
	gint id, rating, play_count;
	gint64 last_played;
	gdouble score;
	WfSong *song = PlayerData.song;

	// Get info
	id = wf_song_get_hash(song);
	title = wf_song_get_title(song);
	artist = wf_song_get_artist(song);
	album = wf_song_get_album(song);
	rating = wf_song_get_rating(song);
	score = wf_song_get_score(song);
	play_count = wf_song_get_play_count(song);
	last_played = wf_song_get_last_played(song);

	const gchar *all_artists[] = { artist, NULL };

	// Set info
	wf_mpris_set_info_track_id(id);
	wf_mpris_set_info_title(title);
	wf_mpris_set_info_artists(all_artists);
	wf_mpris_set_info_album(album);
	wf_mpris_set_info_rating(rating);
	wf_mpris_set_info_score(score);
	wf_mpris_set_info_play_count(play_count);
	wf_mpris_set_info_last_played_sec(last_played);
}

static void
wf_player_remote_reset(void)
{
	wf_mpris_set_info_track_id(0);
	wf_mpris_set_info_title(NULL);
	wf_mpris_set_info_artists(NULL);
	wf_mpris_set_info_album(NULL);
	wf_mpris_set_info_rating(0);
	wf_mpris_set_info_score(0.0);
	wf_mpris_set_info_play_count(0);
	wf_mpris_set_info_last_played_sec(0);
}

// %TRUE is the playback is active (in playing or paused state)
gboolean
wf_player_is_active(void)
{
	return (PlayerData.status == WF_PLAYER_PLAYING || PlayerData.status == WF_PLAYER_PAUSED);
}

static void
wf_player_songs_updated(void)
{
	gdouble duration;
	gboolean active;

	duration = GST_TIME_AS_SECONDS((gdouble) PlayerData.duration);
	wf_player_emit_state_changed(&PlayerData.events, PlayerData.status, duration);

	active = wf_player_is_active();
	wf_song_manager_songs_updated(active);
}

// Report the custom "now playing" message
static void
wf_player_report_playing(void)
{
	const gchar *msg = (PlayerData.play_msg != NULL) ? PlayerData.play_msg : "Playing";

	PlayerData.play_msg = NULL;

	wf_player_emit_report_msg(&PlayerData.events, msg);
}

static void
wf_player_pipeline_add_source(GstElement *element)
{
	if (PlayerData.source != NULL)
	{
		// Unlink and remove the old source element (this will free it)
		gst_element_set_state(PlayerData.source, GST_STATE_NULL);
		gst_element_unlink(PlayerData.source, PlayerData.decoder);
		gst_bin_remove(GST_BIN(PlayerData.pipeline), PlayerData.source);
	}

	// Now add and link the new element
	gst_bin_add(GST_BIN(PlayerData.pipeline), element);
	gst_element_link(element, PlayerData.decoder);

	PlayerData.source = element;
}

static GstElement *
wf_player_pipeline_memory_source_get(void)
{
	GstElement *element = PlayerData.source;
	GstElementFactory *factory = PlayerData.giostreamfactory;
	GstElementFactory *element_factory;

	if (factory == NULL)
	{
		factory = PlayerData.giostreamfactory = gst_element_factory_find("giostreamsrc");
	}

	if (element != NULL)
	{
		element_factory = gst_element_get_factory(element);

		if (factory == element_factory) // Compare pointer
		{
			// Re-use element
			return element;
		}
	}

	// Otherwise create a new one
	return gst_element_factory_create(factory, NULL /* name */);
}

static gboolean
wf_player_pipeline_memory_source_set_file(GstElement *source, GFile *file)
{
	GInputStream *input_stream;
	gchar *content = NULL;
	gsize length = 0;
	GError *error = NULL;

	// Read file
	if (!g_file_load_contents(file, NULL /* GCancellable */, &content, &length, NULL /* etag_out */, &error))
	{
		g_warning("Failed to get content from file: %s", error->message);

		g_error_free(error);

		return FALSE;
	}
	else
	{
		// Create an input stream
		input_stream = g_memory_input_stream_new_from_data(content, length, g_free);

		// Set the input stream as the element source
		g_object_set(source, "stream", G_INPUT_STREAM(input_stream), NULL /* terminator */);

		g_object_unref(input_stream);

		return TRUE;
	}
}

static void
wf_player_pipeline_open(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Make sure the pipeline is constructed
	if (!wf_player_pipeline_construct())
	{
		return;
	}

	if (PlayerData.song != NULL)
	{
		// Do not stop in the future after this previous song, as a new one is forced to play
		wf_song_set_stop_flag(PlayerData.song, FALSE);
	}

	PlayerData.song = NULL;

	// If active, stop playback
	gst_element_set_state(PlayerData.pipeline, GST_STATE_READY);

	// Now set the new song
	wf_player_pipeline_set_song(song);

	// Force set volume
	wf_player_pipeline_update_volume();

	// Set information in data structure
	PlayerData.song = song;
	PlayerData.duration = GST_CLOCK_TIME_NONE;

	// Let it prepare the data in the pipeline
	gst_element_set_state(PlayerData.pipeline, GST_STATE_PAUSED);

	/*
	 * Please note that status and front-end updates are handled when
	 * specific messages are posted on the bus.
	 */
}

static void
wf_player_pipeline_set_song(WfSong *song)
{
	const gchar *uri;
	const gchar *msg;
	GFile *file;
	GstElement *source = NULL;
	GError *error = NULL;

	g_return_if_fail(WF_IS_SONG(song));

	file = wf_song_get_file(song);
	uri = wf_song_get_uri(song);

	if (wf_settings_static_get_bool(WF_SETTING_PREFER_PLAY_FROM_RAM)) // If %TRUE
	{
		// Create an element that reads from memory
		source = wf_player_pipeline_memory_source_get();

		// Read the full file content and set the source stream
		wf_player_pipeline_memory_source_set_file(source, file);
	}

	if (source == NULL)
	{
		// Create an element for this URI
		source = gst_element_make_from_uri(GST_URI_SRC, uri, NULL /* element name */, &error);
	}

	if (source == NULL || error != NULL)
	{
		msg = (error != NULL && error->message != NULL) ? error->message : "Could not create source element";
		g_warning("Failed to create source for %s: %s", uri, msg);
		g_clear_error(&error);

		return;
	}

	// And add the element to the pipeline
	wf_player_pipeline_add_source(source);
}

static void
wf_player_pipeline_update_volume(void)
{
	// Make sure the connected signals don't fire
	g_signal_handler_block(PlayerData.volume_instance, PlayerData.volume_handler);

	// Set volume
	wf_memory_g_object_set(G_OBJECT(PlayerData.volume_instance), "volume", PlayerData.volume, NULL /* terminator */);

	// Re-enable signals
	g_signal_handler_unblock(PlayerData.volume_instance, PlayerData.volume_handler);
}

static void
wf_player_pipeline_play(void)
{
	GstStateChangeReturn result;

	if (!wf_player_pipeline_has_data())
	{
		g_warning("No data loaded to play");

		return;
	}

	result = gst_element_set_state(PlayerData.pipeline, GST_STATE_PLAYING);

	switch (result)
	{
		case GST_STATE_CHANGE_SUCCESS:
			// All good
			break;
		case GST_STATE_CHANGE_ASYNC:
			// The "async done" message will be posted on the bus
			g_info("Pipeline changes state asynchronously");
			break;
		case GST_STATE_CHANGE_NO_PREROLL:
			g_info("Pipeline state change: no preroll");
			break;
		default:
			// An error message is probably posted on the bus
			g_info("Pipeline failed to change state to playing");

			return;
	}

	PlayerData.status = WF_PLAYER_PLAYING;
}

static void
wf_player_pipeline_pause(void)
{
	g_return_if_fail(PlayerData.pipeline != NULL);

	if (!wf_player_pipeline_has_data())
	{
		g_warning("No data loaded to play");

		return;
	}

	gst_element_set_state(PlayerData.pipeline, GST_STATE_PAUSED);

	PlayerData.status = WF_PLAYER_PAUSED;
}

static void
wf_player_pipeline_ready(void)
{
	g_return_if_fail(PlayerData.pipeline != NULL);

	gst_element_set_state(PlayerData.pipeline, GST_STATE_READY);

	if (PlayerData.song != NULL)
	{
		// Unset stop flag
		wf_song_set_stop_flag(PlayerData.song, FALSE);

		PlayerData.song = NULL;
	}

	PlayerData.status = WF_PLAYER_READY;
	PlayerData.duration = GST_CLOCK_TIME_NONE;
}

static void
wf_player_pipeline_stop(void)
{
	g_return_if_fail(PlayerData.pipeline != NULL);

	wf_player_pipeline_ready();

	PlayerData.status = WF_PLAYER_STOPPED;
}

static void
wf_player_pipeline_seek(gdouble position)
{
	g_return_if_fail(PlayerData.pipeline != NULL);
	g_return_if_fail(position >= 0.0);

	gst_element_seek_simple(PlayerData.pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, position);
}

static gboolean
wf_player_pipeline_has_data(void)
{
	return (PlayerData.pipeline != NULL && PlayerData.source != NULL);
}

static gint64
wf_player_pipeline_get_duration(void)
{
	gint64 duration = 0;
	gboolean res;

	g_return_val_if_fail(PlayerData.pipeline != NULL, 0);

	// Do nothing if duration is already set
	if (GST_CLOCK_TIME_IS_VALID(PlayerData.duration))
	{
		return PlayerData.duration;
	}

	// First try the pipeline duration
	res = gst_element_query(PlayerData.pipeline, PlayerData.query_duration);

	if (res)
	{
		gst_query_parse_duration(PlayerData.query_duration, NULL /* format */, &duration);

		if ((GstClockTime) duration != GST_CLOCK_TIME_NONE)
		{
			PlayerData.duration = duration;

			return duration;
		}
	}

	PlayerData.duration = GST_CLOCK_TIME_NONE;

	return 0;
}

static gint64
wf_player_pipeline_get_position(void)
{
	gint64 position = 0;
	gboolean res;

	g_return_val_if_fail(PlayerData.pipeline != NULL, position);

	res = gst_element_query_position(PlayerData.pipeline, GST_FORMAT_TIME, &position);

	return (res ? position : 0);
}

static void
wf_player_update_event_update(void)
{
	/*
	 * Add front-end updater if playing and if the interval is non-zero.
	 * Remove front-end updater if not playing or if the interval is zero.
	 */

	gboolean is_active;
	gint interval;

	is_active = wf_player_is_active();
	interval = wf_settings_static_get_int(WF_SETTING_UPDATE_INTERVAL);

	if (interval > 0 && PlayerData.update_event == 0 && is_active)
	{
		PlayerData.update_event = g_timeout_add_full(G_PRIORITY_DEFAULT, interval, wf_player_update_event_run_cb, NULL /* data */, wf_player_update_event_rm_cb);

		// Because g_timeout_add will execute the callback after the interval, execute it here once
		wf_player_update_event_run_cb(NULL /* data */);
	}
	else if (!is_active || (interval <= 0 && PlayerData.update_event != 0))
	{
		g_source_remove(PlayerData.update_event);

		PlayerData.update_event = 0;
	}
}

static gdouble
wf_player_get_played_fraction(void)
{
	gint64 duration;
	gint64 position;
	gdouble fraction = 1.0;

	duration = wf_player_pipeline_get_duration();
	position = wf_player_pipeline_get_position();

	if (duration <= 0 || position <= 0)
	{
		return fraction;
	}
	else if (position > duration)
	{
		/*
		 * For some reason, it can occur that the position exceeds the
		 * total stream duration and thus creating a fraction higher
		 * than 1.  To prevent that, just use 1 if it is higher.
		 */
		return 1.0;
	}

	fraction = (gdouble) position / (gdouble) duration;

	g_return_val_if_fail(fraction >= 0.0, 0.0); // If lower than 0, use 0
	g_return_val_if_fail(fraction <= 1.0, 1.0); // If higher than 1, use 1

	return fraction;
}

// Add @song to the queue if not already; de-queue otherwise
void
wf_player_toggle_queue(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	if (wf_song_get_queued(song)) // If flag is set
	{
		wf_player_queue_rm(song);
	}
	else
	{
		wf_player_queue_add(song);
	}
}

void
wf_player_queue_add(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Add the song to the queue list
	wf_song_manager_add_queue_song(song);

	// Report song changes
	wf_player_songs_updated();
}

void
wf_player_queue_rm(WfSong *song)
{
	g_return_if_fail(WF_IS_SONG(song));

	// Remove the song from the queue list
	wf_song_manager_rm_queue_song(song);

	// Report song changes
	wf_player_songs_updated();
}

void
wf_player_stop_after_song(WfSong *song)
{
	WfSong *target_song = (song != NULL) ? song : PlayerData.song;

	if (target_song != NULL)
	{
		if (wf_song_get_stop_flag(target_song)) // If flag is set
		{
			wf_song_set_stop_flag(target_song, FALSE);
		}
		else
		{
			wf_song_set_stop_flag(target_song, TRUE);
		}

		wf_player_songs_updated();
	}
	else
	{
		g_info("No song to set stop flag");
	}
}

gdouble
wf_player_get_position(void)
{
	gint64 pos;

	// Check if playing
	if (PlayerData.pipeline != NULL)
	{
		pos = wf_player_pipeline_get_position();

		if (pos > 0)
		{
			// Convert to seconds
			return GST_TIME_AS_SECONDS((gdouble) pos);
		}
	}

	return 0.0;
}

void
wf_player_open(WfSong *song)
{
	wf_player_finish_song();

	g_return_if_fail(wf_song_is_valid(song));

	wf_player_pipeline_open(song);
	wf_player_pipeline_play();
}

static void
wf_player_finish_song(void)
{
	gdouble fraction;

	if (PlayerData.song != NULL)
	{
		fraction = wf_player_get_played_fraction();
		wf_song_manager_add_played_song(PlayerData.song, fraction, FALSE);

		wf_song_set_status(PlayerData.song, WF_SONG_AVAILABLE);
	}

	PlayerData.duration = GST_CLOCK_TIME_NONE;
}

static void
wf_player_finish_song_error(void)
{
	gdouble fraction;

	if (PlayerData.song != NULL)
	{
		fraction = wf_player_get_played_fraction();

		// Prevent stats update if playback failed at the start or end
		if (fraction > 0.0 && fraction < 1.0)
		{
			wf_song_manager_add_played_song(PlayerData.song, fraction, FALSE);
		}

		wf_song_set_status(PlayerData.song, WF_SONG_AVAILABLE);
	}

	PlayerData.duration = GST_CLOCK_TIME_NONE;
}

static void
wf_player_play_next_song(void)
{
	WfSong *song = NULL;

	// First check the queue
	song = wf_song_manager_get_queue_song();
	wf_song_manager_rm_queue_song(song);

	// If nothing is queue, use something else
	if (song == NULL)
	{
		song = wf_song_manager_get_next_song();

		if (song != NULL)
		{
			wf_song_manager_rm_next_song(song);
		}
	}

	if (song == NULL)
	{
		wf_player_finish_song();
		wf_player_pipeline_stop();

		wf_player_emit_report_msg(&PlayerData.events, "No qualified songs to play");
	}
	else
	{
		wf_player_open(song);
	}
}

static void
wf_player_play_prev_song(void)
{
	WfSong *song;

	song = wf_song_manager_played_song_revert();

	if (song == NULL)
	{
		wf_player_emit_report_msg(&PlayerData.events, "No previous songs to play");
	}
	else
	{
		wf_player_open(song);
	}
}

void
wf_player_play(void)
{
	if (PlayerData.status == WF_PLAYER_PLAYING)
	{
		g_info("Player is already playing");

		wf_player_emit_report_msg(&PlayerData.events, "Already playing");
	}
	else if (PlayerData.status == WF_PLAYER_PAUSED)
	{
		wf_player_pipeline_play();
	}
	else
	{
		// Set custom play message for when the playback starts
		PlayerData.play_msg = "Now playing";

		wf_player_play_next_song();
	}
}

void
wf_player_pause(void)
{
	if (PlayerData.status == WF_PLAYER_PAUSED)
	{
		g_info("Player is already paused");

		wf_player_emit_report_msg(&PlayerData.events, "Already paused");
	}
	else if (PlayerData.status == WF_PLAYER_PLAYING)
	{
		wf_player_pipeline_pause();
	}
	else
	{
		wf_player_emit_report_msg(&PlayerData.events, "Not yet playing");
	}
}

void
wf_player_play_pause(void)
{
	if (PlayerData.status == WF_PLAYER_PLAYING)
	{
		wf_player_pause();
	}
	else
	{
		wf_player_play();
	}
}

void
wf_player_stop(void)
{
	wf_player_finish_song();

	if (PlayerData.pipeline != NULL)
	{
		wf_player_pipeline_ready();
	}
}

void
wf_player_forward(gboolean omit_score_update)
{
	PlayerData.play_msg = "Skipped forward";

	wf_player_play_next_song();
}

void
wf_player_backward(gboolean omit_score_update)
{
	PlayerData.play_msg = "Skipped backward";

	wf_player_play_prev_song();
}

static void
wf_player_seek(gint64 position)
{
	if (PlayerData.pipeline == NULL)
	{
		g_info("No playback active");
	}
	else
	{
		PlayerData.play_msg = "Seeked";

		wf_player_pipeline_seek(position);
	}
}

void
wf_player_seek_position(gint64 position)
{
	g_return_if_fail(position >= 0);

	wf_player_seek(position);
}

void
wf_player_seek_seconds(gdouble seconds)
{
	g_return_if_fail(seconds >= 0.0);

	wf_player_seek(seconds * GST_SECOND);
}

void
wf_player_seek_percentage(gdouble percentage)
{
	gint64 position;

	g_return_if_fail(percentage >= 0.0 || percentage <= 100.0);

	position = PlayerData.duration * (percentage / 100.0);

	wf_player_seek(position);
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */
/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
wf_player_finalize(void)
{
	// First force stop the playback
	wf_player_stop();

	// Stop remote media interface
	wf_player_remote_finalize();

	/*
	 * Notify song manager that it should save anything important and
	 * clear its memory.
	 */
	wf_song_manager_finalize();

	/*
	 * Only now destruct the pipeline, after everything else had the
	 * chance to get or save anything from the pipeline.
	 */
	wf_player_pipeline_destruct();
}

static void
wf_player_remote_finalize(void)
{
	wf_mpris_deactivate();
}

static void
wf_player_pipeline_destruct(void)
{
	gboolean pipeline = (PlayerData.pipeline != NULL);

	if (pipeline)
	{
		gst_element_set_state(PlayerData.pipeline, GST_STATE_NULL);
	}

	if (PlayerData.bus != NULL)
	{
		gst_bus_remove_watch(PlayerData.bus);
		gst_object_unref(PlayerData.bus);
	}

	if (PlayerData.query_duration != NULL)
	{
		gst_query_unref(PlayerData.query_duration);
	}

	if (pipeline)
	{
		gst_object_unref(PlayerData.pipeline);
	}

	PlayerData.pipeline = NULL;
	PlayerData.source = NULL;
	PlayerData.decoder = NULL;
	PlayerData.sink = NULL;
	PlayerData.bus = NULL;

	PlayerData.volume_instance = NULL;

	PlayerData.query_duration = NULL;
	PlayerData.query_position = NULL;
}

/* DESTRUCTORS END */

/* END OF FILE */
