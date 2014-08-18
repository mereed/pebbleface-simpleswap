/*
Copyright (C) 2014 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pebble.h"

int STYLE_KEY = 1;

static AppSync sync;
static uint8_t sync_buffer[64];

static int fonttwo;

enum {
  FONTTWO_KEY = 0x0
};

GColor background_color = GColorWhite;
GColor foreground_color = GColorBlack;
GCompOp compositing_mode = GCompOpAssign;

Window *window;
TextLayer *layer_date_text;
TextLayer *layer_wday_text;
TextLayer *layer_time_text;

BitmapLayer *layer_conn_img;

GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;
int cur_day = -1;
int charge_percent = 0;

static GFont *time1_font;
static GFont *day1_font;
static GFont *date1_font;

static GFont *time2_font;
static GFont *day2_font;
static GFont *date2_font;

TextLayer *battery_text_layer;


void font2(bool fonttwo) {

	if (!fonttwo) {
		
        time1_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FREESANS_52));
		day1_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FREESANS_24));
		date1_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FREESANS_20));
		
		text_layer_set_font(layer_time_text, time1_font);
		text_layer_set_font(layer_wday_text, day1_font);
        text_layer_set_font(layer_date_text, date1_font);
		text_layer_set_font(battery_text_layer, date1_font);

	}
	else if (fonttwo) {
		
		time2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CRYSRG_54));
		day2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CRYSRG_28));
		date2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CRYSRG_22));
		
		    text_layer_set_font(layer_time_text, time2_font);
		    text_layer_set_font(layer_wday_text, day2_font);
            text_layer_set_font(layer_date_text, date2_font);
		    text_layer_set_font(battery_text_layer, date2_font);

	}
  
  text_layer_get_layer(layer_time_text);
  text_layer_get_layer(layer_wday_text);
  text_layer_get_layer(layer_date_text);
  text_layer_get_layer(battery_text_layer);
  layer_mark_dirty(text_layer_get_layer(layer_time_text));
  layer_mark_dirty(text_layer_get_layer(layer_wday_text));
  layer_mark_dirty(text_layer_get_layer(layer_date_text));
  layer_mark_dirty(text_layer_get_layer(battery_text_layer));
  
 
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case FONTTWO_KEY:
      fonttwo = new_tuple->value->uint8;
	  font2(fonttwo);

      break;
  }
  
}
		
void update_battery_state(BatteryChargeState charge_state) {
    static char battery_text[] = "xxxxxxx x100%";

    if (charge_state.is_charging) {

        snprintf(battery_text, sizeof(battery_text), "Charge +%d%%", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "Batt. %d%%", charge_state.charge_percent);
    }
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(battery_text_layer, battery_text);
}

void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
        vibes_long_pulse();
    }
}

void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
    }
}

void update_time(struct tm *tick_time) {
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char date_text[] = "xxxxxxxxx 00xx";
    static char wday_text[] = "xxxxxxxxx";
    
    char *time_format;

    // Only update the date when it's changed.
    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;

			switch(tick_time->tm_mday)
  {
    case 1 :
    case 21 :
    case 31 :
      strftime(date_text, sizeof(date_text), "%B %est", tick_time);
      break;
    case 2 :
    case 22 :
      strftime(date_text, sizeof(date_text), "%B %end", tick_time);
      break;
    case 3 :
    case 23 :
      strftime(date_text, sizeof(date_text), "%B %erd", tick_time);
      break;
    default :
      strftime(date_text, sizeof(date_text), "%B %eth", tick_time);
      break;
  }
	
	  text_layer_set_text(layer_date_text, date_text);
		
        strftime(wday_text, sizeof(wday_text), "%A", tick_time);
        text_layer_set_text(layer_wday_text, wday_text);
    }

    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M";
    }

    strftime(time_text, sizeof(time_text), time_format, tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }

    text_layer_set_text(layer_time_text, time_text);
}

void set_style(void) {
    bool inverse = persist_read_bool(STYLE_KEY);
    
    background_color  = inverse ? GColorBlack : GColorWhite;
    foreground_color  = inverse ? GColorWhite : GColorBlack;
    compositing_mode  = inverse ? GCompOpAssignInverted : GCompOpAssign;
    
    window_set_background_color(window, background_color);
    text_layer_set_text_color(layer_time_text, background_color);
    text_layer_set_text_color(layer_wday_text, foreground_color);
    text_layer_set_text_color(layer_date_text, foreground_color);
    text_layer_set_text_color(battery_text_layer, foreground_color);
    bitmap_layer_set_compositing_mode(layer_conn_img, compositing_mode);
}

void force_update(void) {
    handle_bluetooth(bluetooth_connection_service_peek());
    time_t now = time(NULL);
    update_time(localtime(&now));
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

void handle_tap(AccelAxisType axis, int32_t direction) {
    persist_write_bool(STYLE_KEY, !persist_read_bool(STYLE_KEY));
    set_style();
    force_update();
    vibes_long_pulse();
    accel_tap_service_unsubscribe();
}

void handle_tap_timeout(void* data) {
    accel_tap_service_unsubscribe();
}

void handle_init(void) {
	
  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);  

    window = window_create();
    window_stack_push(window, true /* Animated */);

    // resources
    img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
 
    // layers
    layer_time_text = text_layer_create(GRect(0, -5, 144, 168));
    layer_wday_text = text_layer_create(GRect(0, 64, 144, 168));
    layer_date_text = text_layer_create(GRect(0, 94, 144, 168));
    layer_conn_img  = bitmap_layer_create(GRect(0, 0, 144, 168));
    battery_text_layer = text_layer_create(GRect(0, 144, 144, 30));

    text_layer_set_background_color(layer_wday_text, GColorClear);
    text_layer_set_background_color(layer_date_text, GColorClear);
    text_layer_set_background_color(layer_time_text, GColorClear);
    text_layer_set_background_color(battery_text_layer, GColorClear);

	text_layer_set_text_alignment(layer_wday_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(layer_time_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);

    // composing layers
    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img));
    layer_add_child(window_layer, text_layer_get_layer(layer_wday_text));
    layer_add_child(window_layer, text_layer_get_layer(layer_date_text));
    layer_add_child(window_layer, text_layer_get_layer(layer_time_text));
    layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));
		
    // style
    set_style();

	Tuplet initial_values[] = {
    TupletInteger(FONTTWO_KEY, 0)
  };
  
      app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
	
    // handlers
    battery_state_service_subscribe(update_battery_state);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    app_focus_service_subscribe(&handle_appfocus);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    accel_tap_service_subscribe(handle_tap);
    app_timer_register(10000, handle_tap_timeout, NULL);

	// Update the battery on launch
    update_battery_state(battery_state_service_peek());
	
    // draw first frame
    force_update();
}


void handle_deinit(void) {
	
    app_sync_deinit(&sync);

    tick_timer_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    battery_state_service_unsubscribe();
    app_focus_service_unsubscribe();
    accel_tap_service_unsubscribe();
	
  text_layer_destroy( layer_time_text );
  text_layer_destroy( layer_wday_text );
  text_layer_destroy( layer_date_text );
  text_layer_destroy( battery_text_layer );
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
  bitmap_layer_destroy(layer_conn_img);
  gbitmap_destroy(img_bt_connect);
  gbitmap_destroy(img_bt_disconnect);

	fonts_unload_custom_font(time1_font);
    fonts_unload_custom_font(date1_font);
    fonts_unload_custom_font(day1_font);

	fonts_unload_custom_font(time2_font);
    fonts_unload_custom_font(date2_font);
    fonts_unload_custom_font(day2_font);
	
    window_destroy(window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
