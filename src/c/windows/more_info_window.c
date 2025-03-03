#include "more_info_window.h"
#include "loading_window.h"
#include <pebble.h>

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static Layer *s_info_layer;
static char *s_line_name = NULL;
static char *s_destination = NULL;
static char *s_platform = NULL;
static char *s_time = NULL;
static char *s_delay = NULL;
static char *s_type = NULL;
static int *s_stops_ids = NULL;  // Changed from char**
static char **s_stops = NULL;
static int s_num_stops = 0;

static GDrawCommandImage *s_tram_icon = NULL;
static GDrawCommandImage *s_train_icon = NULL;

static void create_menu_layer();

void free_more_info_memory() {
  if (s_stops) {
    for (int i = 0; i < s_num_stops; i++) {
      free(s_stops[i]);
    }
    free(s_stops);
    s_stops = NULL;
  }
  if (s_stops_ids) {
    free(s_stops_ids);  // Simplified, no need to free each element
    s_stops_ids = NULL;
  }
}

void more_info_window_set_info(Tuple *info_tuple) {
  // Free previous allocations
  free_more_info_memory();

  // Parse the info array
  if (info_tuple) {
    const char *data = info_tuple->value->cstring;
    const char *ptr = data;

    // Extract the line name
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    const char *start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    size_t len = ptr - start;
    s_line_name = malloc(len + 1);
    strncpy(s_line_name, start, len);
    s_line_name[len] = '\0';
    ptr++;

    // Extract the destination
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    len = ptr - start;
    s_destination = malloc(len + 1);
    strncpy(s_destination, start, len);
    s_destination[len] = '\0';
    ptr++;

    // Extract the platform
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    len = ptr - start;
    s_platform = malloc(len + 1);
    strncpy(s_platform, start, len);
    s_platform[len] = '\0';
    ptr++;

    // Extract the time schedule
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    len = ptr - start;
    s_time = malloc(len + 1);
    strncpy(s_time, start, len);
    s_time[len] = '\0';
    ptr++;

    // Extract the time delayed
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    len = ptr - start;
    s_delay = malloc(len + 1);
    strncpy(s_delay, start, len);
    s_delay[len] = '\0';
    ptr++;

    // Extract the type
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    ptr++;
    start = ptr;
    while (*ptr != '"' && *ptr != '\0') ptr++;
    if (*ptr == '\0') return;
    len = ptr - start;
    s_type = malloc(len + 1);
    strncpy(s_type, start, len);
    s_type[len] = '\0';
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "info_tuple is NULL");
  }
}

void more_info_window_set_stops_more_info(Tuple *stops_more_info_tuple) {
  // Free previous allocations
  free_more_info_memory();

  // Parse the stops more info array
  if (stops_more_info_tuple) {
    const char *data = stops_more_info_tuple->value->cstring;
    const char *ptr = data;
    s_num_stops = 0;

    // Count the number of stops
    while (*ptr != '\0') {
      if (*ptr == '[') {
        s_num_stops++;
      }
      ptr++;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total stops counted: %d", s_num_stops);

    // Allocate memory for the stops
    s_stops = malloc(s_num_stops * sizeof(char *));
    s_stops_ids = malloc(s_num_stops * sizeof(int));  // Changed to int*

    // Parse the stops
    ptr = data;
    int current_stop = 0;
    
    while (*ptr != '\0' && current_stop < s_num_stops) {
      // Skip to the start of the stop ID
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the stop ID as integer
      const char *start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      
      // Convert string to integer directly
      char temp[20] = {0};  // Temporary buffer for the ID string
      size_t len = ptr - start;
      if (len < sizeof(temp)) {
        strncpy(temp, start, len);
        temp[len] = '\0';
        s_stops_ids[current_stop] = atoi(temp);  // Convert to integer
      }
      ptr++;

      // Skip to the start of the stop name
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      ptr++;

      // Extract the stop name
      start = ptr;
      while (*ptr != '"' && *ptr != '\0') ptr++;
      if (*ptr == '\0') break;
      len = ptr - start;
      s_stops[current_stop] = malloc(len + 1);
      strncpy(s_stops[current_stop], start, len);
      s_stops[current_stop][len] = '\0';
      ptr++;
      
      // Increment current stop count
      current_stop++;
      
      // Skip to the next stop or end
      while (*ptr != '[' && *ptr != '\0') ptr++;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stops actually processed: %d", current_stop);
    s_num_stops = current_stop;
    create_menu_layer();
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "stops_more_info_tuple is NULL");
  }
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_num_stops;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  GRect bounds = layer_get_bounds(cell_layer);
  
  // Calculate vertical center position
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  GSize text_size = graphics_text_layout_get_content_size(
    s_stops[cell_index->row], font, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft
  );
  
  // Calculate vertical offset
  int y_offset = (bounds.size.h - text_size.h - 6) / 2;
  
  // Create text bounds with calculated offset
  #if PBL_RECT
  GRect text_bounds = GRect(5, y_offset, bounds.size.w - 30, text_size.h);
  #else
  GRect text_bounds = GRect(5, y_offset, bounds.size.w - 10, text_size.h);
  #endif

  bool is_selected = menu_cell_layer_is_highlighted(cell_layer);

  graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
  graphics_context_set_fill_color(ctx, is_selected ? PBL_IF_BW_ELSE(GColorBlack, GColorDarkGreen) : GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_draw_text(ctx, s_stops[cell_index->row], font, 
                      text_bounds, GTextOverflowModeTrailingEllipsis, 
                      PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter), NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int station_id = s_stops_ids[cell_index->row];  // No need to dereference
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected station ID: %d", station_id);
  // Send the station ID to the station window
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, MESSAGE_KEY_GET_STATION_FROM_STOP, &station_id, sizeof(int), true);
  app_message_outbox_send();
  // Push the loading window
  loading_window_push();
}

static void create_menu_layer() {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  //menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
  menu_layer_set_highlight_colors(s_menu_layer, PBL_IF_BW_ELSE(GColorBlack, GColorDarkGreen), GColorWhite);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
}

static void menu_click_config_provider(void *context);
static void show_info();

static void activate_menu() {
  layer_set_hidden(s_info_layer, true);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), false);
  menu_layer_set_selected_index(s_menu_layer, (MenuIndex){.section = 0, .row = 0}, MenuRowAlignCenter, false);
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  scroll_layer_set_callbacks(menu_layer_get_scroll_layer(s_menu_layer), (ScrollLayerCallbacks){
                                                                            .click_config_provider = menu_click_config_provider,
                                                                        });
  menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
}

// Hides the menu and unbinds the click handler
static void deactivate_menu() {
  // First disable click config on the menu layer
  if (s_menu_layer) {
    // Use proper API to remove menu click config
    // Don't set to null directly - this resets click config properly
    window_set_click_config_provider(s_window, NULL);
    
    // Reset scroll layer callbacks safely
    ScrollLayer *scroll_layer = menu_layer_get_scroll_layer(s_menu_layer);
    if (scroll_layer) {
      scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks){
        .click_config_provider = NULL,
      });
    }
    
    // Finally hide the menu layer
    layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
  }
}

static void info_down_handler(ClickRecognizerRef recognizer, void *context) {
  activate_menu();
}

static void info_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_DOWN, info_down_handler);
}

static void menu_up_handler(ClickRecognizerRef recognizer, void *context) {
  // Safety check to make sure menu layer exists and is visible
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }

  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  if (index.row == 0) {
    // Don't call menu_layer_set_click_config_onto_window with NULL - this causes the fault
    // Instead, let deactivate_menu handle click config properly
    deactivate_menu();
    show_info();
    return;
  }

  // Set next selection with safety bounds check
  menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
}

static void menu_down_handler(ClickRecognizerRef recognizer, void *context) {
  // Safety check to make sure menu layer exists and is visible
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }

  // Safety check to prevent scrolling past the end
  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  if (index.row >= s_num_stops - 1) {
    return;
  }

  menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
}

static void menu_select_handler(ClickRecognizerRef recognizer, void *context) {
  // Safety check
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }

  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  // Validate index before using it
  if (index.row < s_num_stops) {
    menu_select_callback(s_menu_layer, &index, NULL);
  }
}

static AppTimer *s_scroll_timer = NULL;
static bool s_scrolling_up = false;

static void scroll_timer_callback(void *data) {
  // Safety check
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }
  
  // Continue scrolling in the appropriate direction
  if (s_scrolling_up) {
    MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
    if (index.row > 0) {
      menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
      // Schedule next scroll
      s_scroll_timer = app_timer_register(200, scroll_timer_callback, NULL);
    }
  } else {
    MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
    if (index.row < s_num_stops - 1) {
      menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
      // Schedule next scroll
      s_scroll_timer = app_timer_register(200, scroll_timer_callback, NULL);
    }
  }
}

static void menu_up_long_start(ClickRecognizerRef recognizer, void *context) {
  // Safety check
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }
  
  // First scroll once immediately
  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  if (index.row == 0) {
    deactivate_menu();
    show_info();
    return;
  }
  
  // Set scrolling direction and start continuous scrolling
  s_scrolling_up = true;
  menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
  // Start timer for continuous scrolling
  s_scroll_timer = app_timer_register(100, scroll_timer_callback, NULL);
}

static void menu_down_long_start(ClickRecognizerRef recognizer, void *context) {
  // Safety check
  if (!s_menu_layer || layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    return;
  }
  
  // First scroll once immediately
  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  if (index.row >= s_num_stops - 1) {
    return;
  }
  
  // Set scrolling direction and start continuous scrolling
  s_scrolling_up = false;
  menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
  // Start timer for continuous scrolling
  s_scroll_timer = app_timer_register(100, scroll_timer_callback, NULL);
}

static void menu_long_stop(ClickRecognizerRef recognizer, void *context) {
  // Cancel any ongoing scrolling timer
  if (s_scroll_timer) {
    app_timer_cancel(s_scroll_timer);
    s_scroll_timer = NULL;
  }
}

static void menu_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, menu_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, menu_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, menu_select_handler);

  window_long_click_subscribe(BUTTON_ID_UP, 100, menu_up_long_start, menu_long_stop);
  window_long_click_subscribe(BUTTON_ID_DOWN, 100, menu_down_long_start, menu_long_stop);
}

static void show_info() {
  // First make sure menu is properly deactivated
  if (!layer_get_hidden(menu_layer_get_layer(s_menu_layer))) {
    deactivate_menu();
  }
  
  // Then show info layer and set click provider
  layer_set_hidden(s_info_layer, false);
  window_set_click_config_provider(s_window, info_click_config_provider);
}

static void info_layer_update_proc(Layer *layer, GContext *ctx) {
  #if PBL_ROUND
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Define some constants for layout
  const int x_offset = 5;
  int y_offset = 15; // Start a bit lower from the top
  const int line_height = 18;
  
  // Draw the line name at top (centered)
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_line_name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  y_offset += line_height + 5;
  
  // Draw the platform (centered)
  if (s_platform != NULL && strlen(s_platform) > 0) {
    char platform_text[strlen(s_platform) + 11];
    snprintf(platform_text, sizeof(platform_text), "Platform: %s", s_platform);
    graphics_draw_text(ctx, platform_text, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), 
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    y_offset += line_height + 5;
  }
  
  // Draw time and delay side by side
  int half_width = (bounds.size.w - (x_offset * 2)) / 2;
  
  // Draw the time (left side)
  graphics_draw_text(ctx, s_time, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(x_offset - 8, y_offset, half_width, line_height), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw the delay (right side)
  // if the delay starts with a minus sign, it is negative and the train is early
  if (s_delay[0] == '-' || strlen(s_delay) == 0) {
    graphics_context_set_text_color(ctx, GColorGreen);
    // If the delay is empty, display "+0"
    if (strlen(s_delay) == 0) {
      graphics_draw_text(ctx, "+0", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(bounds.size.w/2, y_offset, half_width, line_height), 
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else {
      graphics_draw_text(ctx, s_delay, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(bounds.size.w/2, y_offset, half_width, line_height), 
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
  } else {
    graphics_context_set_text_color(ctx, GColorRed);
    graphics_draw_text(ctx, s_delay, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(bounds.size.w/2, y_offset, half_width, line_height), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  graphics_context_set_text_color(ctx, GColorBlack);
  
  //y_offset += line_height;
  
  // Choose the correct image
  GDrawCommandImage *image = NULL;
  if (strcmp(s_type, "TRAM") == 0) {
    image = s_tram_icon;
  } else {
    image = s_train_icon;
  }

  // Draw the image in the center of screen
  if (image != NULL) {
    // Center the image (assuming 80x80 dimensions)
    gdraw_command_image_draw(ctx, image, GPoint(bounds.size.w/2 - 40, y_offset));
  }
  
  y_offset += 90; // Move past the image
  
  // Draw the destination at the bottom (centered)
  // Calculate the height needed for the destination text
  GSize destination_size = graphics_text_layout_get_content_size(
    s_destination, 
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
    GRect(x_offset, 0, bounds.size.w - (x_offset * 2), bounds.size.h), 
    GTextOverflowModeWordWrap,
    GTextAlignmentCenter
  );
  
  // Cap the height to 3 lines maximum
  int dest_height = destination_size.h > (line_height * 3) ? (line_height * 3) : destination_size.h;
  
  // Position destination text at the bottom
  graphics_draw_text(ctx, s_destination, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(x_offset, bounds.size.h - dest_height - 30, bounds.size.w - (x_offset * 2), dest_height), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  #else
  GRect bounds = layer_get_bounds(layer);

  // Draw white background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Define some constants for layout
  const int x_offset = 5;
  int y_offset = 5;
  const int line_height = 14;

  // Choose the correct image
  GDrawCommandImage *image = NULL;
  if (strcmp(s_type, "TRAM") == 0) {
    image = s_tram_icon;
  } else {
    image = s_train_icon;
  }

  // Draw the image, if it exists
  if (image != NULL) {
    gdraw_command_image_draw(ctx, image, GPoint(bounds.size.w - 75, y_offset));
  }

  // Draw the line name
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_line_name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(x_offset, y_offset, bounds.size.w - 50, line_height), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  y_offset += line_height + 5;

  // Draw the platform
  if (s_platform != NULL && strlen(s_platform) > 0) {
    // Check if platform text would be too long and potentially overlap with image
    char platform_text[strlen(s_platform) + 11];
    snprintf(platform_text, sizeof(platform_text), "Platform: %s", s_platform);

    GSize platform_size = graphics_text_layout_get_content_size(
      platform_text,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(0, 0, bounds.size.w - 75, line_height),  // Account for image width (75px)
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft
    );

    // If platform text would overlap with image, split into two lines
    if (platform_size.w > bounds.size.w - 85) {
      // First line: just "Platform:"
      graphics_draw_text(ctx, "Platform:", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                        GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), 
                        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
      y_offset += line_height;

      // Second line: just the platform number
      graphics_draw_text(ctx, s_platform, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                        GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), 
                        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
      y_offset += line_height;
    } else {
      // If it fits, display as one line
      graphics_draw_text(ctx, platform_text, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                        GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), 
                        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
      y_offset += line_height + 5;
    }
  } else {
    y_offset += line_height + 5;
  }

  // Draw the time
  graphics_draw_text(ctx, s_time, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM),
                     GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  y_offset += line_height + 10;

  // Draw the delay
  #if PBL_COLOR
  // if the delay starts with a minus sign, it is negative and the train is early
  if (s_delay[0] == '-') {
    graphics_context_set_text_color(ctx, GColorGreen);
  } else {
    graphics_context_set_text_color(ctx, GColorRed);
  }
  #endif
  #if PBL_PLATFORM_APLITE
  //For some reason the LECO font wont display the minus on Aplite
  //So we use the bold numbers font instead
  graphics_draw_text(ctx, s_delay, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  #else
  graphics_draw_text(ctx, s_delay, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM),
                     GRect(x_offset, y_offset, bounds.size.w - (x_offset * 2), line_height), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
  #endif
  #if PBL_COLOR
  graphics_context_set_text_color(ctx, GColorBlack);
  #endif
  
  // Draw the destination at the bottom center
  // Calculate the height needed for the destination text (allowing for up to 3 lines)
  GSize destination_size = graphics_text_layout_get_content_size(
    s_destination, 
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
    GRect(x_offset, 0, bounds.size.w - (x_offset * 2), bounds.size.h), 
    GTextOverflowModeWordWrap,
    GTextAlignmentCenter
  );
  
  // Cap the height to 3 lines maximum
  int dest_height = destination_size.h > (line_height * 3) ? (line_height * 3) : destination_size.h;
  
  // Position destination text with more space from bottom (20px instead of 10px)
  graphics_draw_text(ctx, s_destination, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(x_offset, bounds.size.h - dest_height - 20, bounds.size.w - (x_offset * 2), dest_height), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  #endif
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  #if PBL_ROUND
  s_info_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  #else
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  s_info_layer = layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT));
  #endif

  layer_add_child(window_layer, s_info_layer);

  s_tram_icon = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_TRAM);
  s_train_icon = gdraw_command_image_create_with_resource(RESOURCE_ID_IMAGE_TRAIN);

  // Set the update proc for the info layer
  layer_set_update_proc(s_info_layer, info_layer_update_proc);

  window_set_click_config_provider(window, info_click_config_provider);
}

static void window_unload(Window *window) {
  // Free allocated memory
  free_more_info_memory();
  free(s_line_name);
  free(s_destination);
  free(s_platform);
  free(s_time);
  free(s_delay);
  free(s_type);

  // Destroy the images
  if (s_tram_icon != NULL) {
    gdraw_command_image_destroy(s_tram_icon);
  }
  if (s_train_icon != NULL) {
    gdraw_command_image_destroy(s_train_icon);
  }

  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  layer_destroy(s_info_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void more_info_window_reset_if_existing() {
  if (s_window) {
    window_stack_remove(s_window, false);
    window_destroy(s_window);
    s_window = NULL;
  }
}

void more_info_window_push() {
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