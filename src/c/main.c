#include <pebble.h>

// ============================================================================
// Constants & Persist Keys
// ============================================================================
#define PERSIST_KEY_DAILY_GOAL        1
#define PERSIST_KEY_REMINDER_INTERVAL 2
#define PERSIST_KEY_ACTIVE_START_HOUR 3
#define PERSIST_KEY_ACTIVE_END_HOUR   4
#define PERSIST_KEY_LANGUAGE          5
#define PERSIST_KEY_DAILY_COUNT       6
#define PERSIST_KEY_LAST_DATE_YDAY    7
#define PERSIST_KEY_WAKEUP_ID         8
#define PERSIST_KEY_HISTORY           9   // blob: DayRecord[14]
#define PERSIST_KEY_EFFECTIVE_GOAL    10
#define PERSIST_KEY_DAY_TYPE          11
#define PERSIST_KEY_CONSEC_TRAIN_DAYS 12

// Default values
#define DEFAULT_DAILY_GOAL        30
#define DEFAULT_REMINDER_INTERVAL 60
#define DEFAULT_ACTIVE_START_HOUR 8
#define DEFAULT_ACTIVE_END_HOUR   20

// Wakeup cookie
#define WAKEUP_COOKIE_REMINDER 0

// Adaptive goals constants
#define HISTORY_SIZE            14
#define MIN_DAILY_GOAL          10
#define MAX_CONSECUTIVE_TRAIN   3
#define OVERLOAD_INCREASE_PCT   8   // 8% increase on goal met
#define DELOAD_DECREASE_PCT     10  // 10% decrease on < 80% achieved
#define DELOAD_THRESHOLD_PCT    80  // below this % triggers deload
#define FATIGUE_EXCESS_PCT      150 // >150% of goal triggers rest day

// ============================================================================
// Adaptive Goals: Day Types & History
// ============================================================================
typedef enum {
  DAY_TYPE_TRAINING_NORMAL  = 0,
  DAY_TYPE_TRAINING_OVERLOAD = 1,
  DAY_TYPE_TRAINING_DELOAD   = 2,
  DAY_TYPE_REST              = 3
} DayType;

typedef struct __attribute__((packed)) {
  uint16_t target;     // goal for that day
  uint16_t achieved;   // actual push-ups done
  uint8_t  day_type;   // DayType enum
  uint16_t yday;       // tm_yday (0-365)
} DayRecord;

static DayRecord s_history[HISTORY_SIZE];
static uint8_t   s_history_count = 0;  // number of valid entries (0..14)

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

// Daily tracking state
static uint16_t s_daily_count = 0;
static int      s_last_date_yday = -1;
static WakeupId s_wakeup_id = -1;
static bool     s_launched_by_wakeup = false;

// Adaptive goals state
static uint16_t s_effective_daily_goal = DEFAULT_DAILY_GOAL;
static DayType  s_today_day_type = DAY_TYPE_TRAINING_NORMAL;
static uint8_t  s_consecutive_train_days = 0;

// ============================================================================
// UI Windows & Layers
// ============================================================================
static Window    *s_main_menu_window     = NULL;
static MenuLayer *s_main_menu_layer      = NULL;

static Window    *s_settings_menu_window = NULL;
static MenuLayer *s_settings_menu_layer  = NULL;

static Window *s_picker_window = NULL;
static Layer  *s_picker_layer  = NULL;

static Window *s_reminder_window = NULL;
static Layer  *s_reminder_layer  = NULL;

static Window *s_session_window = NULL;
static Layer  *s_session_layer  = NULL;

static Window *s_adjust_window = NULL;
static Layer  *s_adjust_layer  = NULL;

static Window *s_quicklog_window = NULL;
static Layer  *s_quicklog_layer  = NULL;

static Window    *s_history_window = NULL;
static MenuLayer *s_history_menu_layer = NULL;

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
static void reminder_window_load(Window *window);
static void reminder_window_unload(Window *window);
static void schedule_next_wakeup(void);
static void check_and_reset_daily_count(void);
static void run_adaptive_algorithm(void);
static void session_window_load(Window *window);
static void session_window_unload(Window *window);
static void adjust_window_load(Window *window);
static void adjust_window_unload(Window *window);
static void open_session(void);
static void open_quicklog(void);
static void open_history(void);
static void update_app_glance(void);

// ============================================================================
// Push-up Session State
// ============================================================================
typedef enum {
  SESSION_IDLE = 0,
  SESSION_ACTIVE,
  SESSION_PAUSED
} SessionState;

// Accelerometer detection state machine
typedef enum {
  DETECT_IDLE = 0,  // waiting for downward movement
  DETECT_DOWN,      // detected downward phase
  DETECT_UP         // detected upward phase (rep counted)
} DetectPhase;

static SessionState s_session_state = SESSION_IDLE;
static DetectPhase  s_detect_phase = DETECT_IDLE;
static uint16_t s_session_count = 0;
static uint16_t s_adjust_count = 0;
static time_t   s_session_start_time = 0;
static uint16_t s_session_elapsed_sec = 0;
static bool     s_session_accel_subscribed = false;

// Detection parameters
#define ACCEL_SAMPLE_RATE      ACCEL_SAMPLING_25HZ
#define ACCEL_BATCH_SIZE       5
#define PUSHUP_Z_DOWN_THRESH   (-600)  // Z-axis threshold for "down" phase (milli-g)
#define PUSHUP_Z_UP_THRESH     (-200)  // Z-axis threshold for "up" phase (milli-g)
#define PUSHUP_COOLDOWN_MS     400     // minimum time between reps in ms
#define PUSHUP_MAG_MIN         800     // minimum magnitude to filter noise

static uint32_t s_last_rep_time_ms = 0;  // last rep timestamp (ms)

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
  if (persist_exists(PERSIST_KEY_DAILY_COUNT)) {
    s_daily_count = (uint16_t)persist_read_int(PERSIST_KEY_DAILY_COUNT);
  }
  if (persist_exists(PERSIST_KEY_LAST_DATE_YDAY)) {
    s_last_date_yday = persist_read_int(PERSIST_KEY_LAST_DATE_YDAY);
  }
  if (persist_exists(PERSIST_KEY_WAKEUP_ID)) {
    s_wakeup_id = (WakeupId)persist_read_int(PERSIST_KEY_WAKEUP_ID);
  }
  if (persist_exists(PERSIST_KEY_EFFECTIVE_GOAL)) {
    s_effective_daily_goal = (uint16_t)persist_read_int(PERSIST_KEY_EFFECTIVE_GOAL);
  } else {
    s_effective_daily_goal = s_daily_goal; // Initialize from user baseline
  }
  if (persist_exists(PERSIST_KEY_DAY_TYPE)) {
    s_today_day_type = (DayType)persist_read_int(PERSIST_KEY_DAY_TYPE);
  }
  if (persist_exists(PERSIST_KEY_CONSEC_TRAIN_DAYS)) {
    s_consecutive_train_days = (uint8_t)persist_read_int(PERSIST_KEY_CONSEC_TRAIN_DAYS);
  }
  // Load history blob
  if (persist_exists(PERSIST_KEY_HISTORY)) {
    int bytes_read = persist_read_data(PERSIST_KEY_HISTORY, s_history, sizeof(s_history));
    s_history_count = bytes_read / sizeof(DayRecord);
    if (s_history_count > HISTORY_SIZE) s_history_count = HISTORY_SIZE;
  } else {
    s_history_count = 0;
    memset(s_history, 0, sizeof(s_history));
  }
}

static void save_settings(void) {
  persist_write_int(PERSIST_KEY_DAILY_GOAL, s_daily_goal);
  persist_write_int(PERSIST_KEY_REMINDER_INTERVAL, s_reminder_interval);
  persist_write_int(PERSIST_KEY_ACTIVE_START_HOUR, s_active_start_hour);
  persist_write_int(PERSIST_KEY_ACTIVE_END_HOUR, s_active_end_hour);
  persist_write_int(PERSIST_KEY_LANGUAGE, s_language);
  persist_write_int(PERSIST_KEY_DAILY_COUNT, s_daily_count);
  persist_write_int(PERSIST_KEY_LAST_DATE_YDAY, s_last_date_yday);
  persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);
  persist_write_int(PERSIST_KEY_EFFECTIVE_GOAL, s_effective_daily_goal);
  persist_write_int(PERSIST_KEY_DAY_TYPE, s_today_day_type);
  persist_write_int(PERSIST_KEY_CONSEC_TRAIN_DAYS, s_consecutive_train_days);
  // Save history blob
  persist_write_data(PERSIST_KEY_HISTORY, s_history, s_history_count * sizeof(DayRecord));
}

// ============================================================================
// Daily Count Management & Adaptive Algorithm
// ============================================================================
static void archive_day_to_history(uint16_t target, uint16_t achieved, DayType type, uint16_t yday) {
  // Shift history if full (FIFO: oldest entry at index 0)
  if (s_history_count >= HISTORY_SIZE) {
    memmove(&s_history[0], &s_history[1], (HISTORY_SIZE - 1) * sizeof(DayRecord));
    s_history_count = HISTORY_SIZE - 1;
  }
  // Append new record
  DayRecord *rec = &s_history[s_history_count];
  rec->target = target;
  rec->achieved = achieved;
  rec->day_type = (uint8_t)type;
  rec->yday = yday;
  s_history_count++;
}

static void run_adaptive_algorithm(void) {
  // Determine today's goal and day type based on yesterday's performance.
  // Called when a new day is detected.

  // If no history yet, use user-set baseline
  if (s_history_count == 0) {
    s_effective_daily_goal = s_daily_goal;
    s_today_day_type = DAY_TYPE_TRAINING_NORMAL;
    s_consecutive_train_days = 0;
    return;
  }

  // Get yesterday's record (most recent in history)
  DayRecord *yesterday = &s_history[s_history_count - 1];

  // --- Rule B: Immediate rest day if yesterday had >150% of goal ---
  if (yesterday->day_type != DAY_TYPE_REST && yesterday->target > 0) {
    uint32_t fatigue_threshold = (uint32_t)yesterday->target * FATIGUE_EXCESS_PCT / 100;
    if (yesterday->achieved > fatigue_threshold) {
      s_today_day_type = DAY_TYPE_REST;
      s_effective_daily_goal = 0;
      s_consecutive_train_days = 0;
      APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: REST DAY (fatigue - yesterday %d/%d)",
              yesterday->achieved, yesterday->target);
      return;
    }
  }

  // --- Rule A: Rest day after MAX_CONSECUTIVE_TRAIN consecutive training days ---
  if (s_consecutive_train_days >= MAX_CONSECUTIVE_TRAIN) {
    s_today_day_type = DAY_TYPE_REST;
    s_effective_daily_goal = 0;
    s_consecutive_train_days = 0;
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: REST DAY (scheduled recovery after %d days)",
            MAX_CONSECUTIVE_TRAIN);
    return;
  }

  // --- Training Day: Calculate new goal ---
  // If yesterday was a rest day, carry forward the last training target
  uint16_t base_goal = s_effective_daily_goal;
  if (yesterday->day_type == DAY_TYPE_REST) {
    // Find last training day's target from history
    for (int i = s_history_count - 1; i >= 0; i--) {
      if (s_history[i].day_type != DAY_TYPE_REST && s_history[i].target > 0) {
        base_goal = s_history[i].target;
        break;
      }
    }
    s_today_day_type = DAY_TYPE_TRAINING_NORMAL;
    s_effective_daily_goal = base_goal;
    s_consecutive_train_days++;
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: TRAINING NORMAL (post-rest), goal=%d",
            s_effective_daily_goal);
    return;
  }

  // Yesterday was a training day - evaluate performance
  if (yesterday->target > 0) {
    uint32_t pct = (uint32_t)yesterday->achieved * 100 / yesterday->target;

    if (pct >= 100) {
      // Goal met or exceeded: Progressive Overload
      uint16_t increase = (base_goal * OVERLOAD_INCREASE_PCT) / 100;
      if (increase < 1) increase = 1;
      s_effective_daily_goal = base_goal + increase;
      s_today_day_type = DAY_TYPE_TRAINING_OVERLOAD;
      APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: OVERLOAD %d -> %d (%d%% achieved)",
              base_goal, s_effective_daily_goal, (int)pct);
    } else if (pct < DELOAD_THRESHOLD_PCT) {
      // Below 80%: Deload
      uint16_t decrease = (base_goal * DELOAD_DECREASE_PCT) / 100;
      if (decrease < 1) decrease = 1;
      s_effective_daily_goal = base_goal > decrease ? base_goal - decrease : MIN_DAILY_GOAL;
      if (s_effective_daily_goal < MIN_DAILY_GOAL) s_effective_daily_goal = MIN_DAILY_GOAL;
      s_today_day_type = DAY_TYPE_TRAINING_DELOAD;
      APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: DELOAD %d -> %d (%d%% achieved)",
              base_goal, s_effective_daily_goal, (int)pct);
    } else {
      // Between 80-99%: Keep same goal
      s_effective_daily_goal = base_goal;
      s_today_day_type = DAY_TYPE_TRAINING_NORMAL;
      APP_LOG(APP_LOG_LEVEL_INFO, "Pushups Adaptive: NORMAL (keep %d, %d%% achieved)",
              s_effective_daily_goal, (int)pct);
    }
  } else {
    s_effective_daily_goal = s_daily_goal;
    s_today_day_type = DAY_TYPE_TRAINING_NORMAL;
  }

  // Enforce minimum
  if (s_effective_daily_goal < MIN_DAILY_GOAL) {
    s_effective_daily_goal = MIN_DAILY_GOAL;
  }

  s_consecutive_train_days++;
}

static void check_and_reset_daily_count(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int today_yday = t->tm_yday;

  if (s_last_date_yday != today_yday) {
    // Archive yesterday's data to history (if there was a previous day)
    if (s_last_date_yday >= 0) {
      archive_day_to_history(
        s_effective_daily_goal,
        s_daily_count,
        s_today_day_type,
        (uint16_t)s_last_date_yday
      );
    }

    // Reset daily count and run adaptive algorithm
    s_daily_count = 0;
    s_last_date_yday = today_yday;
    run_adaptive_algorithm();
    save_settings();
    update_app_glance();
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: New day. Effective goal=%d, type=%d, streak=%d",
            s_effective_daily_goal, s_today_day_type, s_consecutive_train_days);
  }
}

// ============================================================================
// Streak & History Utilities
// ============================================================================
static uint16_t calculate_streak(void) {
  uint16_t streak = 0;
  // Iterate backwards from the most recent day (index: s_history_count - 1)
  for (int i = s_history_count - 1; i >= 0; i--) {
    // If achieved meets target (note: Rest Days have target 0, so 0 >= 0 is true)
    if (s_history[i].achieved >= s_history[i].target) {
      streak++;
    } else {
      break; // Streak broken
    }
  }
  
  // Add today if already achieved
  if (s_daily_count >= s_effective_daily_goal) {
    streak++;
  }
  return streak;
}

// ============================================================================
// App Glance Integration
// ============================================================================
#if !PBL_PLATFORM_APLITE
static void glance_reload_callback(AppGlanceReloadSession *session, size_t limit, void *context) {
  if (limit < 1) return;

  static char glance_text[48];
  
  if (s_today_day_type == DAY_TYPE_REST) {
    snprintf(glance_text, sizeof(glance_text), "%s", translate("Ruhetag", "Rest Day"));
  } else {
    snprintf(glance_text, sizeof(glance_text), "%d/%d Push-ups", s_daily_count, s_effective_daily_goal);
  }

  AppGlanceSlice slice = {
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
    .layout = {
      .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string = glance_text,
    }
  };

  AppGlanceResult result = app_glance_add_slice(session, slice);
  if (result != APP_GLANCE_RESULT_SUCCESS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Glance Error: %d", result);
  }
}
#endif

static void update_app_glance(void) {
#if !PBL_PLATFORM_APLITE
  app_glance_reload(glance_reload_callback, NULL);
#endif
}

// ============================================================================
// Wakeup Scheduling
// ============================================================================
static void schedule_next_wakeup(void) {
  // Cancel any existing wakeup
  if (s_wakeup_id >= 0) {
    wakeup_cancel(s_wakeup_id);
    s_wakeup_id = -1;
  }

  // Don't schedule if rest day or daily goal already met
  check_and_reset_daily_count();
  if (s_today_day_type == DAY_TYPE_REST) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: Rest day, no wakeup scheduled.");
    save_settings();
    return;
  }
  if (s_daily_count >= s_effective_daily_goal) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: Daily goal met (%d/%d), no wakeup scheduled.",
            s_daily_count, s_effective_daily_goal);
    save_settings();
    return;
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int current_hour = t->tm_hour;

  // Calculate next wakeup time
  time_t wakeup_time = now + (s_reminder_interval * 60);
  struct tm *wt = localtime(&wakeup_time);
  int wakeup_hour = wt->tm_hour;

  // If the wakeup would be outside the active window, schedule for the start of the next active window
  if (wakeup_hour < s_active_start_hour || wakeup_hour >= s_active_end_hour) {
    // If we're before the start hour today, schedule for today's start
    if (current_hour < s_active_start_hour) {
      struct tm tomorrow = *t;
      tomorrow.tm_hour = s_active_start_hour;
      tomorrow.tm_min = 0;
      tomorrow.tm_sec = 0;
      wakeup_time = mktime(&tomorrow);
    } else {
      // Schedule for tomorrow's start hour
      struct tm tomorrow = *t;
      tomorrow.tm_mday += 1;
      tomorrow.tm_hour = s_active_start_hour;
      tomorrow.tm_min = 0;
      tomorrow.tm_sec = 0;
      wakeup_time = mktime(&tomorrow);
    }
  }

  // Also check: if current time is outside window and wakeup is in future window, that's fine
  // But if current time IS in window and wakeup calc is still in window, use it directly

  s_wakeup_id = wakeup_schedule(wakeup_time, WAKEUP_COOKIE_REMINDER, true);

  if (s_wakeup_id >= 0) {
    struct tm *st = localtime(&wakeup_time);
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: Wakeup scheduled at %02d:%02d (id=%d)",
            st->tm_hour, st->tm_min, (int)s_wakeup_id);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Pushups: Failed to schedule wakeup: %d", (int)s_wakeup_id);
  }

  save_settings();
}

// ============================================================================
// Reminder Window (shown when wakeup fires)
// ============================================================================
static void reminder_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Accent header bar
  int header_h = 36;
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, header_h), 0, GCornerNone);

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, translate("PUSHUP ZEIT!", "PUSHUP TIME!"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(4, 4, bounds.size.w - 8, header_h - 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Progress display
  static char progress_buf[32];
  snprintf(progress_buf, sizeof(progress_buf), "%d / %d", s_daily_count, s_effective_daily_goal);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, progress_buf,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS),
                     GRect(10, header_h + 15, bounds.size.w - 20, 50),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Label
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, translate("Pushups heute", "Push-ups today"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(10, header_h + 65, bounds.size.w - 20, 24),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Dismiss hint
  graphics_draw_text(ctx, translate("Beliebige Taste: Schließen",
                                     "Any button: Dismiss"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, bounds.size.h - 20, bounds.size.w - 8, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void reminder_dismiss(ClickRecognizerRef recognizer, void *context) {
  // Pop reminder window
  window_stack_pop(true);

  // Schedule next wakeup
  schedule_next_wakeup();

  // If launched by wakeup and no other window behind, close the app
  if (s_launched_by_wakeup) {
    window_stack_pop_all(true);
  }
}

static void reminder_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, reminder_dismiss);
  window_single_click_subscribe(BUTTON_ID_DOWN, reminder_dismiss);
  window_single_click_subscribe(BUTTON_ID_SELECT, reminder_dismiss);
  window_single_click_subscribe(BUTTON_ID_BACK, reminder_dismiss);
}

static void reminder_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_reminder_layer = layer_create(bounds);
  layer_set_update_proc(s_reminder_layer, reminder_layer_update_proc);
  layer_add_child(window_layer, s_reminder_layer);

  window_set_click_config_provider(window, reminder_click_config_provider);
}

static void reminder_window_unload(Window *window) {
  layer_destroy(s_reminder_layer);
  s_reminder_layer = NULL;
}

static void show_reminder(void) {
  // Vibrate (respect Quiet Time)
  if (!quiet_time_is_active()) {
    // Distinct double-pulse pattern for push-up reminders
    static const uint32_t segments[] = { 200, 100, 200, 100, 400 };
    VibePattern pat = { .durations = segments, .num_segments = 5 };
    vibes_enqueue_custom_pattern(pat);
  }

  // Show reminder window
  if (!s_reminder_window) {
    s_reminder_window = window_create();
    window_set_window_handlers(s_reminder_window, (WindowHandlers) {
      .load = reminder_window_load,
      .unload = reminder_window_unload
    });
  }
  window_stack_push(s_reminder_window, true);
}

// Wakeup handler (called if app is already running when wakeup fires)
static void wakeup_handler(WakeupId id, int32_t reason) {
  if (reason == WAKEUP_COOKIE_REMINDER) {
    check_and_reset_daily_count();
    // Don't remind on rest days or when goal is met
    if (s_today_day_type == DAY_TYPE_REST) {
      schedule_next_wakeup();
      return;
    }
    if (s_daily_count < s_effective_daily_goal) {
      show_reminder();
    } else {
      // Goal met, don't remind
      schedule_next_wakeup();
    }
  }
}

// ============================================================================
// Push-up Session: Accelerometer Handler
// ============================================================================
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if (s_session_state != SESSION_ACTIVE) return;

  for (uint32_t i = 0; i < num_samples; i++) {
    // Skip noisy samples caused by vibration motor
    if (data[i].did_vibrate) continue;

    int16_t z = data[i].z;

    // Calculate rough magnitude to filter non-exercise noise
    // Using simplified |x|+|y|+|z| instead of sqrt for CPU efficiency
    int32_t mag = (data[i].x < 0 ? -data[i].x : data[i].x)
                + (data[i].y < 0 ? -data[i].y : data[i].y)
                + (z < 0 ? -z : z);
    if (mag < PUSHUP_MAG_MIN) continue;

    // State machine: detect push-up cycle via Z-axis
    // During push-up, wrist faces down. Z ~ -1000 at rest position (top).
    // Going down: Z becomes less negative (closer to 0 or positive).
    // Coming back up: Z returns to strongly negative.
    switch (s_detect_phase) {
      case DETECT_IDLE:
        // Wait for Z to rise above UP threshold (wrist moving down = body lowering)
        if (z > PUSHUP_Z_UP_THRESH) {
          s_detect_phase = DETECT_DOWN;
        }
        break;

      case DETECT_DOWN:
        // Wait for Z to drop below DOWN threshold (wrist returning = body pushing up)
        if (z < PUSHUP_Z_DOWN_THRESH) {
          // Check cooldown to avoid double counting
          uint32_t now_ms = (uint32_t)(time(NULL) * 1000);  // rough ms
          if (now_ms - s_last_rep_time_ms > PUSHUP_COOLDOWN_MS || s_last_rep_time_ms == 0) {
            s_session_count++;
            s_last_rep_time_ms = now_ms;
            s_detect_phase = DETECT_UP;

            // Haptic feedback (light tap)
            vibes_short_pulse();

            // Update display
            if (s_session_layer) {
              layer_mark_dirty(s_session_layer);
            }
          }
        }
        // If Z returns to very negative without completing: reset
        if (z < PUSHUP_Z_DOWN_THRESH - 200) {
          s_detect_phase = DETECT_IDLE;
        }
        break;

      case DETECT_UP:
        // Wait for Z to rise again above UP threshold (reset for next rep)
        if (z > PUSHUP_Z_UP_THRESH) {
          s_detect_phase = DETECT_IDLE;
        }
        break;
    }
  }
}

// ============================================================================
// Push-up Session: Timer Tick (once per second)
// ============================================================================
static void session_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (s_session_state == SESSION_ACTIVE && s_session_start_time > 0) {
    s_session_elapsed_sec = (uint16_t)(time(NULL) - s_session_start_time);
    if (s_session_layer) {
      layer_mark_dirty(s_session_layer);
    }
  }
}

// ============================================================================
// Push-up Session: Window
// ============================================================================
static void session_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Header bar with state
  int header_h = 28;
  GColor header_color = (s_session_state == SESSION_PAUSED) ? GColorDarkGray : GColorIslamicGreen;
  graphics_context_set_fill_color(ctx, header_color);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, header_h), 0, GCornerNone);

  const char *state_text = "";
  switch (s_session_state) {
    case SESSION_ACTIVE: state_text = translate("AKTIVE SESSION", "ACTIVE SESSION"); break;
    case SESSION_PAUSED: state_text = translate("PAUSIERT", "PAUSED"); break;
    default: state_text = ""; break;
  }
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, state_text,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(4, 2, bounds.size.w - 8, header_h - 2),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Large count display
  static char count_buf[8];
  snprintf(count_buf, sizeof(count_buf), "%d", s_session_count);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, count_buf,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                     GRect(10, header_h + 10, bounds.size.w - 20, 50),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Label
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, "PUSH-UPS",
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(10, header_h + 58, bounds.size.w - 20, 24),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Timer display
  static char timer_buf[16];
  uint16_t mins = s_session_elapsed_sec / 60;
  uint16_t secs = s_session_elapsed_sec % 60;
  snprintf(timer_buf, sizeof(timer_buf), "%02d:%02d", mins, secs);
  graphics_context_set_text_color(ctx, GColorCadetBlue);
  graphics_draw_text(ctx, timer_buf,
                     fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(10, header_h + 82, bounds.size.w - 20, 30),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Button hints at bottom
  graphics_context_set_text_color(ctx, GColorDarkGray);
  const char *hint = "";
  if (s_session_state == SESSION_ACTIVE) {
    hint = translate("SEL:Pause  BACK:Fertig", "SEL:Pause  BACK:Finish");
  } else if (s_session_state == SESSION_PAUSED) {
    hint = translate("SEL:Weiter  BACK:Fertig", "SEL:Resume  BACK:Finish");
  }
  graphics_draw_text(ctx, hint,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, bounds.size.h - 18, bounds.size.w - 8, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void session_select_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_session_state == SESSION_ACTIVE) {
    // Pause
    s_session_state = SESSION_PAUSED;
    if (s_session_accel_subscribed) {
      accel_data_service_unsubscribe();
      s_session_accel_subscribed = false;
    }
  } else if (s_session_state == SESSION_PAUSED) {
    // Resume
    s_session_state = SESSION_ACTIVE;
    s_detect_phase = DETECT_IDLE;
    accel_data_service_subscribe(ACCEL_BATCH_SIZE, accel_data_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLE_RATE);
    s_session_accel_subscribed = true;
  }
  if (s_session_layer) layer_mark_dirty(s_session_layer);
}

static void session_finish(void);

static void session_back_handler(ClickRecognizerRef recognizer, void *context) {
  // Finish session -> go to adjustment screen
  session_finish();
}

static void session_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, session_select_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, session_back_handler);
}

static void session_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_session_layer = layer_create(bounds);
  layer_set_update_proc(s_session_layer, session_layer_update_proc);
  layer_add_child(window_layer, s_session_layer);

  window_set_click_config_provider(window, session_click_config_provider);
}

static void session_window_unload(Window *window) {
  // Cleanup accelerometer
  if (s_session_accel_subscribed) {
    accel_data_service_unsubscribe();
    s_session_accel_subscribed = false;
  }
  // Unsubscribe tick timer
  tick_timer_service_unsubscribe();

  layer_destroy(s_session_layer);
  s_session_layer = NULL;
  s_session_state = SESSION_IDLE;
}

// ============================================================================
// Push-up Session: Adjustment Window (post-session correction)
// ============================================================================
static void adjust_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Header
  int header_h = 30;
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, header_h), 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, translate("ERGEBNIS ANPASSEN", "ADJUST COUNT"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(4, 4, bounds.size.w - 8, header_h - 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Count display
  static char adj_buf[8];
  snprintf(adj_buf, sizeof(adj_buf), "%d", s_adjust_count);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, adj_buf,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS),
                     GRect(10, bounds.size.h / 2 - 30, bounds.size.w - 20, 50),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Arrows
  int arrow_y_up = header_h + 8;
  int arrow_y_dn = bounds.size.h - 22;
  graphics_context_set_fill_color(ctx, GColorDarkGray);

  GPoint up_pts[3] = {
    {bounds.size.w / 2, arrow_y_up},
    {bounds.size.w / 2 - 8, arrow_y_up + 10},
    {bounds.size.w / 2 + 8, arrow_y_up + 10}
  };
  GPathInfo up_info = { .num_points = 3, .points = up_pts };
  GPath *up_path = gpath_create(&up_info);
  gpath_draw_filled(ctx, up_path);
  gpath_destroy(up_path);

  GPoint dn_pts[3] = {
    {bounds.size.w / 2, arrow_y_dn + 10},
    {bounds.size.w / 2 - 8, arrow_y_dn},
    {bounds.size.w / 2 + 8, arrow_y_dn}
  };
  GPathInfo dn_info = { .num_points = 3, .points = dn_pts };
  GPath *dn_path = gpath_create(&dn_info);
  gpath_draw_filled(ctx, dn_path);
  gpath_destroy(dn_path);

  // Hint
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, translate("SEL: Best\xC3\xA4tigen | BACK: Abbruch",
                                     "SEL: Confirm | BACK: Cancel"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, bounds.size.h - 16, bounds.size.w - 8, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void adjust_up_handler(ClickRecognizerRef recognizer, void *context) {
  s_adjust_count++;
  if (s_adjust_layer) layer_mark_dirty(s_adjust_layer);
}

static void adjust_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_adjust_count > 0) s_adjust_count--;
  if (s_adjust_layer) layer_mark_dirty(s_adjust_layer);
}

static void adjust_select_handler(ClickRecognizerRef recognizer, void *context) {
  // Confirm: add to daily total
  s_daily_count += s_adjust_count;
  save_settings();
  update_app_glance();
  schedule_next_wakeup();

  vibes_double_pulse();

  // Refresh main menu
  if (s_main_menu_layer) {
    menu_layer_reload_data(s_main_menu_layer);
  }

  // Pop adjustment window, then session window
  window_stack_pop(true);  // pop adjust
  window_stack_pop(true);  // pop session

  APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: Session confirmed. Added %d, daily total now %d",
          s_adjust_count, s_daily_count);
}

static void adjust_back_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel: discard session results
  window_stack_pop(true);  // pop adjust
  window_stack_pop(true);  // pop session
}

static void adjust_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, adjust_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, adjust_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, adjust_select_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, adjust_back_handler);
}

static void adjust_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_adjust_layer = layer_create(bounds);
  layer_set_update_proc(s_adjust_layer, adjust_layer_update_proc);
  layer_add_child(window_layer, s_adjust_layer);

  window_set_click_config_provider(window, adjust_click_config_provider);
}

static void adjust_window_unload(Window *window) {
  layer_destroy(s_adjust_layer);
  s_adjust_layer = NULL;
}

// ============================================================================
// Push-up Session: Control Functions
// ============================================================================
static void session_finish(void) {
  // Stop accelerometer
  if (s_session_accel_subscribed) {
    accel_data_service_unsubscribe();
    s_session_accel_subscribed = false;
  }
  s_session_state = SESSION_IDLE;
  tick_timer_service_unsubscribe();

  // Initialize adjustment with session count
  s_adjust_count = s_session_count;

  // Push adjustment window
  if (!s_adjust_window) {
    s_adjust_window = window_create();
    window_set_window_handlers(s_adjust_window, (WindowHandlers) {
      .load = adjust_window_load,
      .unload = adjust_window_unload
    });
  }
  window_stack_push(s_adjust_window, true);
}

static void open_session(void) {
  // Reset session state
  s_session_count = 0;
  s_session_elapsed_sec = 0;
  s_session_start_time = time(NULL);
  s_session_state = SESSION_ACTIVE;
  s_detect_phase = DETECT_IDLE;
  s_last_rep_time_ms = 0;

  // Subscribe to accelerometer
  accel_data_service_subscribe(ACCEL_BATCH_SIZE, accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLE_RATE);
  s_session_accel_subscribed = true;

  // Subscribe to tick timer for elapsed time
  tick_timer_service_subscribe(SECOND_UNIT, session_tick_handler);

  // Push session window
  if (!s_session_window) {
    s_session_window = window_create();
    window_set_window_handlers(s_session_window, (WindowHandlers) {
      .load = session_window_load,
      .unload = session_window_unload
    });
  }
  window_stack_push(s_session_window, true);
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
// Quick Log Window
// ============================================================================
static uint16_t s_quicklog_count = 0;

static void quicklog_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Header
  int header_h = 30;
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, header_h), 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, translate("QUICK LOG", "QUICK LOG"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(4, 4, bounds.size.w - 8, header_h - 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Count display
  static char ql_buf[8];
  snprintf(ql_buf, sizeof(ql_buf), "%d", s_quicklog_count);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, ql_buf,
                     fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS),
                     GRect(10, bounds.size.h / 2 - 30, bounds.size.w - 20, 50),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Arrows
  int arrow_y_up = header_h + 8;
  int arrow_y_dn = bounds.size.h - 22;
  graphics_context_set_fill_color(ctx, GColorDarkGray);

  GPoint up_pts[3] = {
    {bounds.size.w / 2, arrow_y_up},
    {bounds.size.w / 2 - 8, arrow_y_up + 10},
    {bounds.size.w / 2 + 8, arrow_y_up + 10}
  };
  GPathInfo up_info = { .num_points = 3, .points = up_pts };
  GPath *up_path = gpath_create(&up_info);
  gpath_draw_filled(ctx, up_path);
  gpath_destroy(up_path);

  GPoint dn_pts[3] = {
    {bounds.size.w / 2, arrow_y_dn + 10},
    {bounds.size.w / 2 - 8, arrow_y_dn},
    {bounds.size.w / 2 + 8, arrow_y_dn}
  };
  GPathInfo dn_info = { .num_points = 3, .points = dn_pts };
  GPath *dn_path = gpath_create(&dn_info);
  gpath_draw_filled(ctx, dn_path);
  gpath_destroy(dn_path);

  // Hint
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, translate("SEL: Speichern | BACK: Abbruch",
                                     "SEL: Save | BACK: Cancel"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, bounds.size.h - 16, bounds.size.w - 8, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void quicklog_up_handler(ClickRecognizerRef recognizer, void *context) {
  s_quicklog_count++;
  if (s_quicklog_layer) layer_mark_dirty(s_quicklog_layer);
}

static void quicklog_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_quicklog_count > 0) s_quicklog_count--;
  if (s_quicklog_layer) layer_mark_dirty(s_quicklog_layer);
}

static void quicklog_select_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_quicklog_count > 0) {
    s_daily_count += s_quicklog_count;
    save_settings();
    update_app_glance();
    schedule_next_wakeup();
    vibes_double_pulse();

    if (s_main_menu_layer) {
      menu_layer_reload_data(s_main_menu_layer);
    }
    
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushups: Quick Log added %d, daily total now %d",
            s_quicklog_count, s_daily_count);
  }
  window_stack_pop(true);
}

static void quicklog_back_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

static void quicklog_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, quicklog_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, quicklog_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, quicklog_select_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, quicklog_back_handler);
}

static void quicklog_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_quicklog_layer = layer_create(bounds);
  layer_set_update_proc(s_quicklog_layer, quicklog_layer_update_proc);
  layer_add_child(window_layer, s_quicklog_layer);

  window_set_click_config_provider(window, quicklog_click_config_provider);
}

static void quicklog_window_unload(Window *window) {
  layer_destroy(s_quicklog_layer);
  s_quicklog_layer = NULL;
}

static void open_quicklog(void) {
  s_quicklog_count = 0;
  if (!s_quicklog_window) {
    s_quicklog_window = window_create();
    window_set_window_handlers(s_quicklog_window, (WindowHandlers) {
      .load = quicklog_window_load,
      .unload = quicklog_window_unload
    });
  }
  window_stack_push(s_quicklog_window, true);
}

// ============================================================================
// History Window
// ============================================================================
static uint16_t history_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // Row 0 = Streak, Row 1..N = up to 7 history days
  int count = 1 + (s_history_count > 7 ? 7 : s_history_count);
  return count;
}

static void history_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  static char title_buf[32];
  static char subtitle_buf[32];

  if (cell_index->row == 0) {
    uint16_t streak = calculate_streak();
    snprintf(title_buf, sizeof(title_buf), "%s: %d", translate("Aktuelle Streak", "Current Streak"), streak);
    snprintf(subtitle_buf, sizeof(subtitle_buf), "%s", translate("Tage in Folge", "Consecutive days"));
    menu_cell_basic_draw(ctx, cell_layer, title_buf, subtitle_buf, NULL);
  } else {
    // Offset by 1 for the streak row. Days are displayed from newest (index history_count - 1) backwards.
    int history_idx = s_history_count - cell_index->row;
    if (history_idx >= 0 && history_idx < s_history_count) {
      DayRecord *record = &s_history[history_idx];
      
      snprintf(title_buf, sizeof(title_buf), "%s %d", translate("Vor", "Days ago:"), cell_index->row);
      
      if (record->day_type == DAY_TYPE_REST) {
        snprintf(subtitle_buf, sizeof(subtitle_buf), "%s", translate("Ruhetag", "Rest Day"));
      } else {
        snprintf(subtitle_buf, sizeof(subtitle_buf), "%d / %d", record->achieved, record->target);
      }
      menu_cell_basic_draw(ctx, cell_layer, title_buf, subtitle_buf, NULL);
    }
  }
}

static void history_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_history_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_history_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = history_menu_get_num_rows,
    .draw_row = history_menu_draw_row,
  });
  menu_layer_set_click_config_onto_window(s_history_menu_layer, window);

  #if defined(PBL_COLOR)
    menu_layer_set_normal_colors(s_history_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_history_menu_layer, GColorCobaltBlue, GColorWhite);
  #endif

  layer_add_child(window_layer, menu_layer_get_layer(s_history_menu_layer));
}

static void history_window_unload(Window *window) {
  menu_layer_destroy(s_history_menu_layer);
  s_history_menu_layer = NULL;
}

static void open_history(void) {
  if (!s_history_window) {
    s_history_window = window_create();
    window_set_window_handlers(s_history_window, (WindowHandlers) {
      .load = history_window_load,
      .unload = history_window_unload
    });
  }
  window_stack_push(s_history_window, true);
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
      snprintf(subtitle_buf, sizeof(subtitle_buf),
               translate("%d (Adaptiv: %d)", "%d (Adaptive: %d)"),
               s_daily_goal, s_effective_daily_goal);
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Basisziel", "Baseline Goal"),
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
  static char status_buf[48];
  switch (cell_index->row) {
    case 0: {
      // Show today's status with adaptive info
      const char *status = "";
      switch (s_today_day_type) {
        case DAY_TYPE_TRAINING_OVERLOAD:
          status = translate("Training: Steigerung", "Training: Overload");
          break;
        case DAY_TYPE_TRAINING_DELOAD:
          status = translate("Training: Erholung", "Training: Deload");
          break;
        case DAY_TYPE_TRAINING_NORMAL:
          status = translate("Training: Normal", "Training: Normal");
          break;
        case DAY_TYPE_REST:
          status = translate("Ruhetag: Regeneration", "Rest Day: Recovery");
          break;
      }
      if (s_today_day_type == DAY_TYPE_REST) {
        snprintf(status_buf, sizeof(status_buf), "%s", status);
      } else {
        snprintf(status_buf, sizeof(status_buf), "%d/%d - %s",
                 s_daily_count, s_effective_daily_goal, status);
      }
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Session starten", "Start Session"),
                           status_buf, NULL);
      break;
    }
    case 1:
      menu_cell_basic_draw(ctx, cell_layer,
                           translate("Schnell eintragen", "Quick Log"),
                           translate("Manuell hinzuf\xC3\xBCgen", "Add manually"), NULL);
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
      // US-4: Start Push-up Session
      open_session();
      break;
    case 1:
      // US-5: Quick Log
      open_quicklog();
      break;
    case 2:
      // US-5: History
      open_history();
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

  // Check for day rollover
  check_and_reset_daily_count();

  // Subscribe to wakeup service (handles wakeups while app is running)
  wakeup_service_subscribe(wakeup_handler);

  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(256, 64);

  // Check if app was launched by a wakeup
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    s_launched_by_wakeup = true;

    WakeupId id = 0;
    int32_t reason = 0;
    wakeup_get_launch_event(&id, &reason);

    if (reason == WAKEUP_COOKIE_REMINDER
        && s_today_day_type != DAY_TYPE_REST
        && s_daily_count < s_effective_daily_goal) {
      // Show reminder directly (main menu will be pushed below but reminder on top)
      s_main_menu_window = window_create();
      window_set_window_handlers(s_main_menu_window, (WindowHandlers) {
        .load = main_menu_window_load,
        .unload = main_menu_window_unload
      });
      window_stack_push(s_main_menu_window, false);
      show_reminder();
      return;
    }
  }

  // Normal launch: show main menu
  s_main_menu_window = window_create();
  window_set_window_handlers(s_main_menu_window, (WindowHandlers) {
    .load = main_menu_window_load,
    .unload = main_menu_window_unload
  });
  window_stack_push(s_main_menu_window, true);

  // Schedule next wakeup if none pending
  if (s_wakeup_id < 0 || !wakeup_query(s_wakeup_id, NULL)) {
    schedule_next_wakeup();
  }
}

static void deinit(void) {
  // Schedule next wakeup before exiting
  schedule_next_wakeup();

  // Save settings on exit
  save_settings();

  // Destroy windows
  if (s_history_window) {
    window_destroy(s_history_window);
  }
  if (s_quicklog_window) {
    window_destroy(s_quicklog_window);
  }
  if (s_adjust_window) {
    window_destroy(s_adjust_window);
  }
  if (s_session_window) {
    window_destroy(s_session_window);
  }
  if (s_reminder_window) {
    window_destroy(s_reminder_window);
  }
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
