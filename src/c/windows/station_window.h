#pragma once

#include <pebble.h>

void station_window_set_station(Tuple *station_tuple);
void station_window_reset_if_existing();
void station_window_push();