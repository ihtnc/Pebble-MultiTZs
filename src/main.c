/** \file
 * Three time zone clock;
 *
 * When the time is in AM, the text and background are white.
 * When the time is in PM, they are black.
 *
 * Rather than use text layers, it draws the entire frame once per minute.
 *
 * Based on hudson's 3 TimeZone watchface: https://bitbucket.org/hudson/pebble/src/tip/timezones/src/tz.c?at=default
 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define SCREEN_HEIGHT 168
#define SCREEN_WIDTH 144
#define container_of(ptr, type, member) \
	({ \
		char *__mptr = (char *)(uintptr_t) (ptr); \
		(type *)(__mptr - offsetof(type,member) ); \
	 })

#define MY_UUID { 0x0A, 0x47, 0x27, 0xEA, 0xFF, 0x19, 0x4B, 0x81, 0xBA, 0xD8, 0x4A, 0x67, 0x00, 0x54, 0x47, 0x26 }
PBL_APP_INFO(MY_UUID,
             "TimeZones", "ihopethisnamecounts",
             1, 1, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

typedef struct
{
	const char * name;
	int offset;
	Layer layer;
} timezone_t;


// Local timezone GMT offset
static const int gmt_offset = 8 * 60;

#define NUM_TIMEZONES 3
#define LAYER_HEIGHT (SCREEN_HEIGHT / NUM_TIMEZONES)

static timezone_t timezones[NUM_TIMEZONES] =
{
	{ .name = "US Central", .offset = -6 * 60 },
	{ .name = "US Eastern", .offset = -5 * 60 },
	{ .name = "India", .offset = +(5 * 60 + 30) },
};


static Window window;
static PblTm now;
static GFont font_thin;
static GFont font_thick;

static void timezone_layer_update(Layer *const me, GContext *ctx)
{
	const timezone_t *const tz = container_of(me, timezone_t, layer);

	const int orig_hour = now.tm_hour;
	const int orig_min = now.tm_min;
	
	now.tm_min += (tz->offset - gmt_offset) % 60;
	
	if (now.tm_min > 60)
	{
		now.tm_hour++;
		now.tm_min -= 60;
	}
	else if (now.tm_min < 0)
	{
		now.tm_hour--;
		now.tm_min += 60;
	}

	now.tm_hour += (tz->offset - gmt_offset) / 60;
	if (now.tm_hour > 24) now.tm_hour -= 24;
	if (now.tm_hour < 0) now.tm_hour += 24;

	char buf[32];
	if(clock_is_24h_style()) 
	{ 
		string_format_time(buf, sizeof(buf), "%H:%M", &now);
	}
	else 
	{
		string_format_time(buf, sizeof(buf), "%I:%M", &now);
	}
	
	const int is_pm = (now.tm_hour > 12);
	now.tm_hour = orig_hour;
	now.tm_min = orig_min;
	
	const int w = me->bounds.size.w;
	const int h = me->bounds.size.h;
	
	// it is night there, draw in black video
	graphics_context_set_fill_color(ctx, is_pm ? GColorBlack : GColorWhite);
	graphics_context_set_text_color(ctx, is_pm ? GColorWhite : GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, w, h), 0, 0);
	
	graphics_text_draw(ctx, tz->name, font_thin, GRect(0, 0, w, h/3), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	graphics_text_draw(ctx, buf, font_thick, GRect(0, h/3, w, 2*h/3), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void handle_tick(AppContextRef ctx, PebbleTickEvent *const event)
{
	(void) ctx;
	
	now = *event->tick_time;
	for (int i = 0 ; i < NUM_TIMEZONES ; i++) layer_mark_dirty(&timezones[i].layer);
}

static void handle_init(AppContextRef ctx)
{
	(void) ctx;
	get_time(&now);

	window_init(&window, "Main");
	window_stack_push(&window, true);
	window_set_background_color(&window, GColorBlack);

	resource_init_current_app(&APP_RESOURCES);
	
    font_thin = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSDIGI_18));
    font_thick = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSDIGIT_34));

    for (int i = 0 ; i < NUM_TIMEZONES ; i++)
	{
		timezone_t * const tz = &timezones[i];
		layer_init(&tz->layer, GRect(0, i * LAYER_HEIGHT, SCREEN_WIDTH, LAYER_HEIGHT));

		tz->layer.update_proc = timezone_layer_update;
		layer_add_child(&window.layer, &tz->layer);
		layer_mark_dirty(&tz->layer);
	}
}

static void handle_deinit(AppContextRef ctx)
{
	(void) ctx;
	
	fonts_unload_custom_font(font_thin);
	fonts_unload_custom_font(font_thick);
}

void pbl_main(void * const params)
{
	PebbleAppHandlers handlers = 
	{
		.init_handler   = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info      = 
		{
			.tick_handler = &handle_tick,
			.tick_units = MINUTE_UNIT,
		},
	};
	
	app_event_loop(params, &handlers);
}