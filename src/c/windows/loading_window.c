#include "loading_window.h"
#include "../windows/no_internet_window.h"
#include <pebble.h>

static Window *s_window;
static Layer *s_loading_layer;
static TextLayer *s_text_layer;
static StatusBarLayer *s_status_bar;
static AppTimer *s_animation_timer;
static AppTimer *s_timeout_timer;

// Animation parameters
static int s_current_frame = 0;
#define NUM_DOTS 8
#define ANIMATION_DURATION 500 // milliseconds per frame
#define MAX_DOT_RADIUS 6
#define MIN_DOT_RADIUS 2
#define TIMEOUT_DURATION 20000 // 20 seconds

// Function to draw the loading spinner
static void loading_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Calculate center point and radius of the spinner circle
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  int circle_radius = bounds.size.w < bounds.size.h ? bounds.size.w / 4 : bounds.size.h / 4;
  
  // Calculate angle step between dots
  int32_t angle_step = TRIG_MAX_ANGLE / NUM_DOTS;
  
  // Draw each dot
  for (int i = 0; i < NUM_DOTS; i++) {
    // Calculate angle for this dot
    int32_t angle = i * angle_step;
    
    // Calculate position for dot
    GPoint dot_pos = {
      .x = center.x + sin_lookup(angle) * circle_radius / TRIG_MAX_RATIO,
      .y = center.y - cos_lookup(angle) * circle_radius / TRIG_MAX_RATIO
    };
    
    // Calculate the distance from current frame
    int distance = (i - s_current_frame + NUM_DOTS) % NUM_DOTS;
    
    // Set the dot size based on distance
    int dot_radius = (distance == 0) ? MAX_DOT_RADIUS : MIN_DOT_RADIUS;
    
    // Set fill color based on color display
    #ifdef PBL_COLOR
    if (dot_radius == MAX_DOT_RADIUS) {
      graphics_context_set_fill_color(ctx, GColorDarkGreen);
    } else {
      graphics_context_set_fill_color(ctx, GColorIslamicGreen);
    }
    #else
    graphics_context_set_fill_color(ctx, GColorBlack);
    #endif
    
    // Draw the dot
    graphics_fill_circle(ctx, dot_pos, dot_radius);
  }
}

// Animation timer callback
static void animation_timer_callback(void *context) {
  // Update frame
  s_current_frame = (s_current_frame + 1) % NUM_DOTS;
  
  // Mark layer for redraw
  if (s_loading_layer) {
    layer_mark_dirty(s_loading_layer);
  }
  
  // Schedule next frame
  s_animation_timer = app_timer_register(ANIMATION_DURATION, animation_timer_callback, NULL);
}

// Timeout timer callback
static void timeout_timer_callback(void *context) {
  // Show no connection screen
  no_internet_window_push();
  
  // Destroy the loading window
  window_stack_remove(s_window, true);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set window background color explicitly to black
  window_set_background_color(window, GColorWhite);
  
  #if PBL_RECT
  // Create the status bar
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
  #endif
  
  // Create loading layer for animation - make it larger and centered
  int loading_height = bounds.size.h - STATUS_BAR_LAYER_HEIGHT - 30;
  s_loading_layer = layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, loading_height));
  layer_set_update_proc(s_loading_layer, loading_layer_update_proc);
  layer_add_child(window_layer, s_loading_layer);
  
  s_animation_timer = app_timer_register(ANIMATION_DURATION, animation_timer_callback, NULL);
  s_timeout_timer = app_timer_register(TIMEOUT_DURATION, timeout_timer_callback, NULL);
}

static void window_unload(Window *window) {
  // Cancel animation timer
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
  }
  
  // Cancel timeout timer
  if (s_timeout_timer) {
    app_timer_cancel(s_timeout_timer);
    s_timeout_timer = NULL;
  }
  
  // Destroy layers
  if (s_loading_layer) {
    layer_destroy(s_loading_layer);
  }
  
  if (s_text_layer) {
    text_layer_destroy(s_text_layer);
  }
  
  if (s_status_bar) {
    status_bar_layer_destroy(s_status_bar);
  }
  
  window_destroy(s_window);
  s_window = NULL;
}

void loading_window_push() {
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_window, true);
}