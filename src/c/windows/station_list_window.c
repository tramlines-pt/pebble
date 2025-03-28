#include "station_list_window.h"
#include "loading_window.h"
#include <pebble.h>

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static int s_num_stations = 0;
static char s_station_names[10][32];
static char s_station_distances[10][16];
static int s_station_ids[10];

void station_list_window_set_stations(Tuple *stations_tuple) {
  // Parse the stations array
  if (stations_tuple) {
    const char *data = stations_tuple->value->cstring;
    const char *ptr = data;
    s_num_stations = 0;

    while (*ptr != '\0' && s_num_stations < 10) {
      // Skip to the start of the station name
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the station name
      const char *start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      size_t len = ptr - start;
      strncpy(s_station_names[s_num_stations], start, len);
      s_station_names[s_num_stations][len] = '\0';
      ptr++;

      // Skip to the start of the distance
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the distance
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      strncpy(s_station_distances[s_num_stations], start, len);
      s_station_distances[s_num_stations][len] = '\0';
      strcat(s_station_distances[s_num_stations], " km");
      ptr++;

      // Skip to the start of the ID
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the ID
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      char id_str[16];
      strncpy(id_str, start, len);
      id_str[len] = '\0';
      s_station_ids[s_num_stations] = atoi(id_str);
      ptr++;

      // Skip to the next station
      while (*ptr != '[' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;

      s_num_stations++;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total stations: %d", s_num_stations);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "stations_tuple is NULL");
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
    GRect name_bounds = GRect(5, 2, bounds.size.w - 10, bounds.size.h / 2);
    GRect distance_bounds = GRect(5, bounds.size.h / 2, bounds.size.w - 10, bounds.size.h / 2);
  
    bool is_selected = menu_cell_layer_is_highlighted(cell_layer);
  
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    graphics_context_set_fill_color(ctx, is_selected ? PBL_IF_BW_ELSE(GColorBlack, GColorDarkGreen) : GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    #if PBL_DISPLAY_HEIGHT == 228
    graphics_draw_text(ctx, s_station_names[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 
                      name_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
    graphics_draw_text(ctx, s_station_distances[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                        distance_bounds, GTextOverflowModeTrailingEllipsis, 
                        PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
    #else
    graphics_draw_text(ctx, s_station_names[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
                      name_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
    graphics_draw_text(ctx, s_station_distances[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                        distance_bounds, GTextOverflowModeTrailingEllipsis, 
                        PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
    #endif
  }

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int station_id = s_station_ids[cell_index->row];
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected station ID: %d", station_id);
  //send the station ID to the phone
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, MESSAGE_KEY_GET_STATION, &station_id, sizeof(int), true);
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
  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  window_destroy(s_window);
  s_window = NULL;
}

void station_list_window_push() {
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