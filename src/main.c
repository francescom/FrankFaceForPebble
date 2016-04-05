#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static GFont s_time_font, s_date_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static int s_battery_level;
static Layer *s_battery_layer;

static int wasConnected=0; // 0 - dunno, 1 - yes, -1 - ko


static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void bt_handler(bool connected) {
    static const uint32_t segmentsConn[] = { 200, 100, 100, 100, 100, 100, 100, 100, 100 };
    // Vibe pattern: ON for 1000ms, OFF for 500ms, 5 times:
    static const uint32_t segmentsDisc[] = { 1000, 500, 1000, 500, 1000, 500, 1000, 500, 1000 };

  
  // Buzz on current connection state to warn for phone left home, but..
  // no buzz first time (wasConnected==0 means "didn't know yet") if wasConnected==0 don't buzz
  // no buzz if status was the same (update twice with the same status doesn't buzz)
  // 
  // (you don't want to wake up at 3:00AM because your watch has to tell you
  // that Bluetooth is STILL disabled)
  
  if (connected) {
    VibePattern pat = {
      .durations = segmentsConn,
      .num_segments = ARRAY_LENGTH(segmentsConn),
    };
    if(wasConnected==-1) vibes_enqueue_custom_pattern(pat);
    wasConnected=1;
  } else {
    VibePattern pat = {
      .durations = segmentsDisc,
      .num_segments = ARRAY_LENGTH(segmentsDisc),
    };
    if(wasConnected==1) vibes_enqueue_custom_pattern(pat);
    wasConnected=-1;
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      text_layer_set_text(s_date_layer, "X axis positive.");
    } else {
      text_layer_set_text(s_date_layer, "X axis negative.");
    }
    break;
  case ACCEL_AXIS_Y:
    if (direction > 0) {
      text_layer_set_text(s_date_layer, "Y axis positive.");
    } else {
      text_layer_set_text(s_date_layer, "Y axis negative.");
    }
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      text_layer_set_text(s_date_layer, "Z axis positive.");
    } else {
      text_layer_set_text(s_date_layer, "Z axis negative.");
    }
    break;
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  
  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
  
}
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 144.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(0, 0, 144, bounds.size.h), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}



static void main_window_load(Window *window) {
  

  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PANGOO_LOGO);
  
  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(
    GRect(0, 1, bounds.size.w, 70)
  );
  
  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  
  // Create GFonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_56));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));


  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 168-56-1, bounds.size.w, 56));
  
  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");

  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 100, 144, 30));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, s_date_font);

  
  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(0, 80, 144, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // get BT current
  bt_handler(connection_service_peek_pebble_app_connection());


}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);

  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);

  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  layer_destroy(s_battery_layer);

}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  
  // gt BT events
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler
  });
  
  accel_tap_service_subscribe(tap_handler);

}

static void deinit() {
  // Destroy Window
  connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}