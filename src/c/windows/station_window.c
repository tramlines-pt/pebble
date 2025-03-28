#include "station_window.h"
#include "loading_window.h"
#include <pebble.h>

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static int s_num_stations = 0;

static char **s_station_lines = NULL;
static char **s_station_destinations = NULL;
static char **s_station_times = NULL;
static char **s_station_platforms = NULL;

void free_station_memory() {
  if (s_station_lines) {
    for (int i = 0; i < s_num_stations; i++) {
      free(s_station_lines[i]);
      free(s_station_destinations[i]);
      free(s_station_times[i]);
      free(s_station_platforms[i]);
    }
    free(s_station_lines);
    free(s_station_destinations);
    free(s_station_times);
    free(s_station_platforms);
    s_station_lines = NULL;
    s_station_destinations = NULL;
    s_station_times = NULL;
    s_station_platforms = NULL;
  }
}

void station_window_set_station(Tuple *station_tuple) {
  // Free previous allocations
  free_station_memory();

  // Parse the station information
  if (station_tuple) {
    const char *data = station_tuple->value->cstring;
    const char *ptr = data;
    s_num_stations = 0;

    // Count the number of stations
    while (*ptr != '\0') {
      if (*ptr == '[') {
        s_num_stations++;
      }
      ptr++;
    }

    // Allocate memory for the stations
    s_station_lines = malloc(s_num_stations * sizeof(char *));
    s_station_destinations = malloc(s_num_stations * sizeof(char *));
    s_station_times = malloc(s_num_stations * sizeof(char *));
    s_station_platforms = malloc(s_num_stations * sizeof(char *));

    // Parse the stations
    ptr = data;
    s_num_stations = 0;
    while (*ptr != '\0') {
      // Skip to the start of the Line
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the Line
      const char *start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      size_t len = ptr - start;
      s_station_lines[s_num_stations] = malloc(len + 1);
      strncpy(s_station_lines[s_num_stations], start, len);
      s_station_lines[s_num_stations][len] = '\0';
      ptr++;

      // Skip to the start of the Destination Name
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the Destination Name
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      s_station_destinations[s_num_stations] = malloc(len + 1);
      strncpy(s_station_destinations[s_num_stations], start, len);
      s_station_destinations[s_num_stations][len] = '\0';
      ptr++;

      // Skip to the start of the Time
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the Time
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      s_station_times[s_num_stations] = malloc(len + 1);
      strncpy(s_station_times[s_num_stations], start, len);
      s_station_times[s_num_stations][len] = '\0';
      ptr++;

      // Skip to the start of the Platform Number
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the Platform Number
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      s_station_platforms[s_num_stations] = malloc(len + 1);
      strncpy(s_station_platforms[s_num_stations], start, len);
      s_station_platforms[s_num_stations][len] = '\0';
      ptr++;

      // Skip to the next station
      while (*ptr != '[' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;

      s_num_stations++;
    }

  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "station_tuple is NULL");
  }
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_num_stations;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  GRect bounds = layer_get_bounds(cell_layer);
  GRect title_bounds = GRect(5, 2, bounds.size.w - 10, bounds.size.h / 2);
  GRect subtitle_bounds = GRect(5, bounds.size.h / 2, bounds.size.w - 10, bounds.size.h / 2);

  bool is_selected = menu_cell_layer_is_highlighted(cell_layer);

  graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
  graphics_context_set_fill_color(ctx, is_selected ? PBL_IF_BW_ELSE(GColorBlack, GColorDarkGreen) : GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Create the title string
  static char title[64];
  snprintf(title, sizeof(title), "%s", s_station_destinations[cell_index->row]);

  // Create the subtitle string
  static char subtitle[64];
  if (strlen(s_station_platforms[cell_index->row]) > 0) {
    snprintf(subtitle, sizeof(subtitle), "%s - %s - %s", s_station_times[cell_index->row], s_station_lines[cell_index->row], s_station_platforms[cell_index->row]);
  } else {
    snprintf(subtitle, sizeof(subtitle), "%s - %s", s_station_times[cell_index->row], s_station_lines[cell_index->row]);
  }
  #if PBL_DISPLAY_HEIGHT == 228
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 
                      title_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                      subtitle_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
  #else
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
                      title_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                      subtitle_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
  #endif
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int index = cell_index->row + 1;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected station Index: %d", cell_index->row);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, MESSAGE_KEY_GET_MORE_INFO, &index, sizeof(int), true);
  app_message_outbox_send();
  //push the loading window
  loading_window_push();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  #if PBL_RECT
  // Create the status bar
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // Adjust bounds for the menu layer to account for the status bar
  GRect menu_bounds = GRect(bounds.origin.x, bounds.origin.y + STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT);
  #else
  GRect menu_bounds = GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h);
  #endif
  s_menu_layer = menu_layer_create(menu_bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_highlight_colors(s_menu_layer, PBL_IF_BW_ELSE(GColorBlack, GColorDarkGreen), GColorWhite);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  // Free allocated memory
  free_station_memory();

  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  window_destroy(s_window);
  s_window = NULL;
}

void station_window_reset_if_existing() {
  if (s_window) {
    window_stack_remove(s_window, false);
    window_destroy(s_window);
    s_window = NULL;
  }
}

void station_window_push() {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  // Remove the current window and push the new one
  Window *current_window = window_stack_get_top_window();
  if (current_window) {
    window_stack_remove(current_window, false);
  }
  window_stack_push(s_window, true);
}