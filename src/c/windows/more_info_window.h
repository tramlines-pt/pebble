#pragma once

#include <pebble.h>

void more_info_window_set_info(Tuple *info_tuple);
void more_info_window_set_stops_more_info(Tuple *stops_more_info_tuple);
void more_info_window_reset_if_existing();
void more_info_window_push();