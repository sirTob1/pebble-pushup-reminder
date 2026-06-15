#include <pebble.h>

// ============================================================================
// Constants & Persist Keys
// ============================================================================
#define PERSIST_KEY_DAILY_GOAL        1
#define PERSIST_KEY_REMINDER_INTERVAL 2
#define PERSIST_KEY_ACTIVE_START_HOUR 3
#define PERSIST_KEY_ACTIVE_END_HOUR   4
#define PERSIST_KEY_LANGUAGE          5

// Default values
#define DEFAULT_DAILY_GOAL        30
#define DEFAULT_REMINDER_INTERVAL 60
#define DEFAULT_ACTIVE_START_HOUR 8
#define DEFAULT_ACTIVE_END_HOUR   20

// ============================================================================
// Language Support
// ============================================================================
typedef enum {
  LANG_DE = 0,
  LANG_EN = 1
} AppLanguage;

static AppLanguage s_language = LANG_DE;

static const char* translate(const char* de, const char* en) {
  return (s_language == LANG_EN) ? en : de;
}

// ============================================================================
// Settings State
// ============================================================================
static uint16_t s_daily_goal        = DEFAULT_DAILY_GOAL;
static uint16_t s_reminder_interval = DEFAULT_REMINDER_INTERVAL;
static uint8_t  s_active_start_hour = DEFAULT_ACTIVE_START_HOUR;
static uint8_t  s_active_end_hour   = DEFAULT_ACTIVE_END_HOUR;

// ============================================================================
// UI Windows & Layers
// ============================================================================
static Window    *s_main_menu_window     = NULL;
static MenuLayer *s_main_menu_layer      = NULL;

static Window    *s_settings_menu_window = NULL;
static MenuLayer *s_settings_menu_layer  = NULL;

static Window *s_picker_window = NULL;
static Layer  *s_picker_layer  = NULL;

// ============================================================================
// Number Picker State
// ============================================================================
typedef enum {
  PICKER_DAILY_GOAL = 0,
  PICKER_REMINDER_INTERVAL,
  PICKER_START_HOUR,
  PICKER_END_HOUR
} PickerType;

static PickerType s_picker_type;
static int s_picker_value;
static int s_picker_min;
static int s_picker_max;
static int s_picker_step;

// ============================================================================
// Forward Declarations
// ============================================================================
static void settings_menu_window_load(Window *window);
static void settings_menu_window_unload(Window *window);
static void picker_window_load(Window *window);
static void picker_window_unload(Window *window);

// ============================================================================
// Persistent Storage: Load & Save
// ============================================================================
static void load_settings(void) {
  if (persist_exists(PERSIST_KEY_DAILY_GOAL)) {
    s_daily_goal = (uint16_t)persist_read_int(PERSIST_KEY_DAILY_GOAL);
  }
  if (persist_exists(PERSIST_KEY_REMINDER_INTERVAL)) {
    s_reminder_interval = (uint16_t)persist_read_int(PERSIST_KEY_REMINDER_INTERVAL);
  }
  if (persist_exists(PERSIST_KEY_ACTIVE_START_HOUR)) {
    s_active_start_hour = (uint8_t)persist_read_int(PERSIST_KEY_ACTIVE_START_HOUR);
  }
  if (persist_exists(PERSIST_KEY_ACTIVE_END_HOUR)) {
    s_active_end_hour = (uint8_t)persist_read_int(PERSIST_KEY_ACTIVE_END_HOUR);
  }
  if (persist_exists(PERSIST_KEY_LANGUAGE)) {
    s_language = (AppLanguage)persist_read_int(PERSIST_KEY_LANGUAGE);
  }
}

static void save_settings(void) {
  persist_write_int(PERSIST_KEY_DAILY_GOAL, s_daily_goal);
  persist_write_int(PERSIST_KEY_REMINDER_INTERVAL, s_reminder_interval);
  persist_write_int(PERSIST_KEY_ACTIVE_START_HOUR, s_active_start_hour);
  persist_write_int(PERSIST_KEY_ACTIVE_END_HOUR, s_active_end_hour);
  persist_write_int(PERSIST_KEY_LANGUAGE, s_language);
}

// ============================================================================
// Number Picker Window
// ============================================================================
static void picker_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Title
  const char *title = "";
  switch (s_picker_type) {
    case PICKER_DAILY_GOAL:
      title = translate("TAGESZIEL", "DAILY GOAL");
      break;
    case PICKER_REMINDER_INTERVAL:
      title = translate("INTERVALL (MIN)", "INTERVAL (MIN)");
      break;
    case PICKER_START_HOUR:
      title = translate("STARTZEIT", "START TIME");
      break;
    case PICKER_END_HOUR:
      title = translate("ENDZEIT", "END TIME");
      break;
  }

  // Header bar
  int header_h = 30;
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, header_h), 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, title,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(4, 4, bounds.size.w - 8, header_h - 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Value display
  static char val_buf[16];
  if (s_picker_type == PICKER_START_HOUR || s_picker_type == PICKER_END_HOUR) {
    snprintf(val_buf, sizeof(val_buf), "%02d:00", s_picker_value);
  } else if (s_picker_type == PICKER_REMINDER_INTERVAL) {
    snprintf(val_buf, sizeof(val_buf), "%d min", s_picker_value);
  } else {
    snprintf(val_buf, sizeof(val_buf), "%d", s_picker_value);
  }

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, val_buf,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS),
                     GRect(10, bounds.size.h / 2 - 30, bounds.size.w - 20, 50),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // UP/DOWN arrows
  int arrow_y_up = header_h + 8;
  int arrow_y_dn = bounds.size.h - 22;
  graphics_context_set_fill_color(ctx, GColorDarkGray);

  // Up arrow (triangle)
  GPoint up_pts[3] = {
    {bounds.size.w / 2, arrow_y_up},
    {bounds.size.w / 2 - 8, arrow_y_up + 10},
    {bounds.size.w / 2 + 8, arrow_y_up + 10}
  };
  GPathInfo up_info = { .num_points = 3, .points = up_pts };
  GPath *up_path = gpath_create(&up_info);
  gpath_draw_filled(ctx, up_path);
  gpath_destroy(up_path);

  // Down arrow (triangle)
  GPoint dn_pts[3] = {
    {bounds.size.w / 2, arrow_y_dn + 10},
    {bounds.size.w / 2 - 8, arrow_y_dn},
    {bounds.size.w / 2 + 8, arrow_y_dn}
  };
  GPathInfo dn_info = { .num_points = 3, .points = dn_pts };
  GPath *dn_path = gpath_create(&dn_info);
  gpath_draw_filled(ctx, dn_path);
  gpath_destroy(dn_path);

  // Hint footer
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, translate("SEL: Speichern | BACK: Abbruch",
                                     "SEL: Save | BACK: Cancel"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, bounds.size.h - 16, bounds.size.w - 8, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void picker_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_picker_value + s_picker_step <= s_picker_max) {
    s_picker_value += s_picker_step;
  }
  layer_mark_dirty(s_picker_layer);
}

static void picker_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_picker_value - s_picker_step >= s_picker_min) {
    s_picker_value -= s_picker_step;
  }
  layer_mark_dirty(s_picker_layer);
}

static void picker_select_handler(ClickRecognizerRef recognizer, void *context) {
  // Apply value
  switch (s_picker_type) {
    case PICKER_DAILY_GOAL:
      s_daily_goal = (uint16_t)s_picker_value;
      break;
    case PICKER_REMINDER_INTERVAL:
      s_reminder_interval = (uint16_t)s_picker_value;
      break;
    case PICKER_START_HOUR:
      s_active_start_hour = (uint8_t)s_picker_value;
      // Enforce: start < end
      if (s_active_start_hour >= s_active_end_hour) {
        s_active_end_hour = s_active_start_hour + 1;
        if (s_active_end_hour > 23) s_active_end_hour = 23;
      }
      break;
    case PICKER_END_HOUR:
      s_active_end_hour = (uint8_t)s_picker_value;
      // Enforce: end > start
      if (s_active_end_hour <= s_active_start_hour) {
        s_active_start_hour = s_active_end_hour - 1;
        if (s_active_start_hour > 23) s_active_start_hour = 0; // underflow wrap
      }
      break;
  }

  save_settings();
  vibes_short_pulse();

  // Refresh settings menu to show updated values
  if (s_settings_menu_layer) {
    menu_layer_reload_data(s_settings_menu_layer);
  }

  window_stack_pop(true);
}

static void picker_back_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

static void picker_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, picker_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, picker_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, picker_select_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, picker_back_handler);
}

static void picker_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_picker_layer = layer_create(bounds);
  layer_set_update_proc(s_picker_layer, picker_layer_update_proc);
  layer_add_child(window_layer, s_picker_layer);

  window_set_click_config_provider(window, picker_click_config_provider);
}

static void picker_window_unload(Window *window) {
  layer_destroy(s_picker_layer);
  s_picker_layer = NULL;
}

static void open_picker(PickerType type, int current, int min, int max, int step) {
  s_picker_type  = type;
  s_picker_value = current;
  s_picker_min   = min;
  s_picker_max   = max;
  s_picker_step  = step;

  if (!s_picker_window) {
    s_picker_window = window_create();
    window_set_window_handlers(s_picker_window, (WindowHandlers) {
      .load = picker_window_load,
      .unload = picker_window_unload
    });
  }
  window_stack_push(s_picker_window, true);
}

// ============================================================================
// Settings Menu Window
// ============================================================================
#define SETTINGS_MENU_NUM_ROWS 4

static uint16_t settings_menu_get_num_rows(MenuLayer *menu_layer,
                                            uint16_t section_index,
                                            void *data) {
  return SETTINGS_MENU_NUM_ROWS;
}

static void settings_menu_draw_row(GContext *ctx, const Layer *cell_layer,
                                    MenuIndex *cell_index, void *data) {
  static char subtitle_buf[32];

  switch (cell_index->row) {
    case 0:
      snprintf(subtitle_buf, sizeof(subtitle_buf), "%d Pushups", s_daily_goal);
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Tagesziel", "Daily Goal"),
                           subtitle_buf, NULL);
      break;
    case 1:
      snprintf(subtitle_buf, sizeof(subtitle_buf), "%d min", s_reminder_interval);
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Erinnerungs-Intervall", "Reminder Interval"),
                           subtitle_buf, NULL);
      break;
    case 2:
      snprintf(subtitle_buf, sizeof(subtitle_buf), "%02d:00", s_active_start_hour);
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Startzeit", "Start Time"),
                           subtitle_buf, NULL);
      break;
    case 3:
      snprintf(subtitle_buf, sizeof(subtitle_buf), "%02d:00", s_active_end_hour);
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Endzeit", "End Time"),
                           subtitle_buf, NULL);
      break;
  }
}

static void settings_menu_select_callback(MenuLayer *menu_layer,
                                           MenuIndex *cell_index,
                                           void *data) {
  switch (cell_index->row) {
    case 0:
      open_picker(PICKER_DAILY_GOAL, s_daily_goal, 1, 500, 5);
      break;
    case 1:
      open_picker(PICKER_REMINDER_INTERVAL, s_reminder_interval, 15, 240, 15);
      break;
    case 2:
      open_picker(PICKER_START_HOUR, s_active_start_hour, 0, 22, 1);
      break;
    case 3:
      open_picker(PICKER_END_HOUR, s_active_end_hour, 1, 23, 1);
      break;
  }
}

static void settings_menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_settings_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_settings_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = settings_menu_get_num_rows,
    .draw_row = settings_menu_draw_row,
    .select_click = settings_menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_settings_menu_layer, window);

  #if defined(PBL_COLOR)
    menu_layer_set_normal_colors(s_settings_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_settings_menu_layer, GColorCobaltBlue, GColorWhite);
  #endif

  layer_add_child(window_layer, menu_layer_get_layer(s_settings_menu_layer));
}

static void settings_menu_window_unload(Window *window) {
  menu_layer_destroy(s_settings_menu_layer);
  s_settings_menu_layer = NULL;
}

// ============================================================================
// Main Menu Window
// ============================================================================
#define MAIN_MENU_NUM_ROWS 4

static uint16_t main_menu_get_num_rows(MenuLayer *menu_layer,
                                        uint16_t section_index,
                                        void *data) {
  return MAIN_MENU_NUM_ROWS;
}

static void main_menu_draw_row(GContext *ctx, const Layer *cell_layer,
                                MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case 0:
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Session starten", "Start Session"),
                           translate("Pushups tracken", "Track push-ups"), NULL);
      break;
    case 1:
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Schnell eintragen", "Quick Log"),
                           translate("Manuell hinzufügen", "Add manually"), NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Verlauf", "History"),
                           translate("Letzte 7 Tage", "Last 7 days"), NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Einstellungen", "Settings"),
                           translate("Ziel, Timer, Zeitfenster", "Goal, Timer, Window"), NULL);
      break;
  }
}

static void main_menu_select_callback(MenuLayer *menu_layer,
                                       MenuIndex *cell_index,
                                       void *data) {
  switch (cell_index->row) {
    case 0:
      // TODO: US-4 - Start Session
      vibes_short_pulse();
      break;
    case 1:
      // TODO: US-5 - Quick Log
      vibes_short_pulse();
      break;
    case 2:
      // TODO: US-5 - History
      vibes_short_pulse();
      break;
    case 3:
      // Open Settings Menu
      if (!s_settings_menu_window) {
        s_settings_menu_window = window_create();
        window_set_window_handlers(s_settings_menu_window, (WindowHandlers) {
          .load = settings_menu_window_load,
          .unload = settings_menu_window_unload
        });
      }
      window_stack_push(s_settings_menu_window, true);
      break;
  }
}

static void main_menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_main_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_main_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = main_menu_get_num_rows,
    .draw_row = main_menu_draw_row,
    .select_click = main_menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_main_menu_layer, window);

  #if defined(PBL_COLOR)
    menu_layer_set_normal_colors(s_main_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_main_menu_layer, GColorCobaltBlue, GColorWhite);
  #endif

  layer_add_child(window_layer, menu_layer_get_layer(s_main_menu_layer));
}

static void main_menu_window_unload(Window *window) {
  menu_layer_destroy(s_main_menu_layer);
  s_main_menu_layer = NULL;
}

// ============================================================================
// AppMessage Handler
// ============================================================================
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Language
  Tuple *lang_t = dict_find(iter, MESSAGE_KEY_LANGUAGE);
  if (lang_t) {
    s_language = (AppLanguage)lang_t->value->uint8;
    persist_write_int(PERSIST_KEY_LANGUAGE, s_language);
    if (s_main_menu_layer) menu_layer_reload_data(s_main_menu_layer);
    if (s_settings_menu_layer) menu_layer_reload_data(s_settings_menu_layer);
  }

  // Daily Goal
  Tuple *goal_t = dict_find(iter, MESSAGE_KEY_DAILY_GOAL);
  if (goal_t) {
    s_daily_goal = goal_t->value->uint16;
    persist_write_int(PERSIST_KEY_DAILY_GOAL, s_daily_goal);
    if (s_settings_menu_layer) menu_layer_reload_data(s_settings_menu_layer);
  }

  // Reminder Interval
  Tuple *interval_t = dict_find(iter, MESSAGE_KEY_REMINDER_INTERVAL);
  if (interval_t) {
    s_reminder_interval = interval_t->value->uint16;
    persist_write_int(PERSIST_KEY_REMINDER_INTERVAL, s_reminder_interval);
    if (s_settings_menu_layer) menu_layer_reload_data(s_settings_menu_layer);
  }

  // Active Start Hour
  Tuple *start_t = dict_find(iter, MESSAGE_KEY_ACTIVE_START_HOUR);
  if (start_t) {
    s_active_start_hour = start_t->value->uint8;
    persist_write_int(PERSIST_KEY_ACTIVE_START_HOUR, s_active_start_hour);
    if (s_settings_menu_layer) menu_layer_reload_data(s_settings_menu_layer);
  }

  // Active End Hour
  Tuple *end_t = dict_find(iter, MESSAGE_KEY_ACTIVE_END_HOUR);
  if (end_t) {
    s_active_end_hour = end_t->value->uint8;
    persist_write_int(PERSIST_KEY_ACTIVE_END_HOUR, s_active_end_hour);
    if (s_settings_menu_layer) menu_layer_reload_data(s_settings_menu_layer);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Pushups: Inbox dropped: %d", reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Pushups: Outbox failed: %d", reason);
}

// ============================================================================
// App Init / Deinit
// ============================================================================
static void init(void) {
  // Load persistent settings
  load_settings();

  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(256, 64);

  // Create and push main menu window
  s_main_menu_window = window_create();
  window_set_window_handlers(s_main_menu_window, (WindowHandlers) {
    .load = main_menu_window_load,
    .unload = main_menu_window_unload
  });
  window_stack_push(s_main_menu_window, true);
}

static void deinit(void) {
  // Save settings on exit
  save_settings();

  // Destroy windows
  if (s_picker_window) {
    window_destroy(s_picker_window);
  }
  if (s_settings_menu_window) {
    window_destroy(s_settings_menu_window);
  }
  window_destroy(s_main_menu_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
