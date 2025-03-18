#include "no_internet_window.h"
#include <pebble.h>

static Window *s_window;
static Layer *s_image_layer;
static TextLayer *s_message_layer;
static StatusBarLayer *s_status_bar;
static GDrawCommandImage *s_no_internet_image;

static void image_layer_update_proc(Layer *layer, GContext *ctx) {
  // Draw the image in the update proc with the provided context
  if (s_no_internet_image) {
    GRect bounds = layer_get_bounds(layer);
    GSize image_size = gdraw_command_image_get_bounds_size(s_no_internet_image);
    GPoint image_origin = GPoint(
      (bounds.size.w - image_size.w) / 2, 
      (bounds.size.h - image_size.h) / 2
    );
    
    gdraw_command_image_draw(ctx, s_no_internet_image, image_origin);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set window background color
  window_set_background_color(window, GColorWhite);
  
  // Create status bar
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // Load the image
  s_no_internet_image = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_WATCH_DISCONNECTED);
  
  // Create image layer
  s_image_layer = layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT - 40));
  layer_set_update_proc(s_image_layer, image_layer_update_proc);
  layer_add_child(window_layer, s_image_layer);

  // Create message layer
  s_message_layer = text_layer_create(GRect(0, bounds.size.h - 40, bounds.size.w, 40));
  text_layer_set_text(s_message_layer, "Keine Verbindung :(");
  #if PBL_PLATFORM_EMERY
  text_layer_set_font(s_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  #endif
  text_layer_set_text_alignment(s_message_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_message_layer, GColorWhite);
  text_layer_set_text_color(s_message_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(s_message_layer));
}

static void window_unload(Window *window) {
  gdraw_command_image_destroy(s_no_internet_image);
  layer_destroy(s_image_layer);
  text_layer_destroy(s_message_layer);
  status_bar_layer_destroy(s_status_bar);
  window_destroy(s_window);
  s_window = NULL;
}

void no_internet_window_push() {
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  //window_stack_push(s_window, true);
  //replace root window
  window_stack_pop_all(false);
  window_stack_push(s_window, true);
}