#include "app_message.h"
#include "../windows/no_internet_window.h"
#include "../windows/station_list_window.h"
#include "../windows/station_window.h"
#include "../windows/loading_window.h"
#include "../windows/more_info_window.h"

void inbox_received_callback(DictionaryIterator *iter, void *context) {
    Tuple *no_internet_tuple = dict_find(iter, MESSAGE_KEY_NO_INTERNET);
    if (no_internet_tuple) {
        no_internet_window_push();
    }

    Tuple *stations_tuple = dict_find(iter, MESSAGE_KEY_STATIONS_ARRAY);
    if (stations_tuple) {
        station_list_window_set_stations(stations_tuple);
        station_list_window_push();
    }

    Tuple *station_tuple = dict_find(iter, MESSAGE_KEY_STATION_ARRAY);
    if (station_tuple) {
        station_window_reset_if_existing();
        station_window_set_station(station_tuple);
        station_window_push();
    }
    Tuple *more_info_tuple = dict_find(iter, MESSAGE_KEY_MORE_INFO);
    if (more_info_tuple) {;
        more_info_window_reset_if_existing();
        more_info_window_set_info(more_info_tuple);
        more_info_window_push();
    }
    Tuple *stops_more_info_tuple = dict_find(iter, MESSAGE_KEY_STOPS_MORE_INFO);
    if (stops_more_info_tuple) {
        more_info_window_set_stops_more_info(stops_more_info_tuple);
    }
    Tuple *more_info_timeout_tuple = dict_find(iter, MESSAGE_KEY_MORE_INFO_TIMEOUT);
    if (more_info_timeout_tuple) {
        //if we hit a more info timeout, we need to go back to the station window
        //because the train uuid is no longer valid
        //but we actually want to refresh the data first. MORE_INFO_TIMEOUT contains the station ID
        int station_id = more_info_timeout_tuple->value->int32;
        DictionaryIterator *iter2;
        app_message_outbox_begin(&iter2);
        dict_write_int(iter2, MESSAGE_KEY_GET_STATION, &station_id, sizeof(int), true);
        app_message_outbox_send();
    }
    Tuple *station_from_stop_tuple = dict_find(iter, MESSAGE_KEY_STATION_FROM_STOP);
    if (station_from_stop_tuple) {
        //if we hit a station from stop, we want to go back to the station window
        //but we also want to delete the more info window
        station_window_reset_if_existing();
        station_window_set_station(station_from_stop_tuple);
        station_window_push();        
    }
}
  
void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

void outbox_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}