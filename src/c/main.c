#include <pebble.h>

#include "modules/app_message.h"
#include "windows/loading_window.h"

static void init() {
  //no_internet_window_push();
  loading_window_push();
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(4096, 256); // Inbox could be large, but outbox is pretty much only requests
  
}

static void deinit() {
}

int main() {
  init();
  app_event_loop();
  deinit();
}