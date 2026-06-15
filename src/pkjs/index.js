// Pebble Pushup Reminder - PebbleKit JS Companion (ES5)

// Hardcoded key mapping for Gadgetbridge compatibility
var myMessageKeys = {
  "LANGUAGE": 10000,
  "DAILY_GOAL": 10001,
  "REMINDER_INTERVAL": 10002,
  "ACTIVE_START_HOUR": 10003,
  "ACTIVE_END_HOUR": 10004
};

// Helper to duplicate payload keys (both string and integer) for Gadgetbridge and other runtimes
function prepareMessage(msg) {
  var prepared = {};
  for (var key in msg) {
    if (msg.hasOwnProperty(key)) {
      prepared[key] = msg[key];
      var intKey = myMessageKeys[key];
      if (intKey !== undefined) {
        prepared[intKey] = msg[key];
      }
      if (typeof messageKeys !== 'undefined' && messageKeys[key] !== undefined) {
        prepared[messageKeys[key]] = msg[key];
      }
    }
  }
  return prepared;
}

// Send settings to watch
function sendSettingsToWatch() {
  var language = localStorage.getItem("pushup_language") || "de";
  var langCode = (language === "en") ? 1 : 0;
  var dailyGoal = parseInt(localStorage.getItem("pushup_daily_goal") || "30", 10);
  var reminderInterval = parseInt(localStorage.getItem("pushup_reminder_interval") || "60", 10);
  var activeStartHour = parseInt(localStorage.getItem("pushup_active_start_hour") || "8", 10);
  var activeEndHour = parseInt(localStorage.getItem("pushup_active_end_hour") || "20", 10);

  var msg = prepareMessage({
    LANGUAGE: langCode,
    DAILY_GOAL: dailyGoal,
    REMINDER_INTERVAL: reminderInterval,
    ACTIVE_START_HOUR: activeStartHour,
    ACTIVE_END_HOUR: activeEndHour
  });

  Pebble.sendAppMessage(msg, function() {
    console.log("Pushups JS: Settings sent to watch successfully.");
  }, function(err) {
    console.log("Pushups JS: Failed to send settings: " + JSON.stringify(err));
  });
}

// Ready event
Pebble.addEventListener("ready", function(e) {
  console.log("Pushups JS: Ready!");
  sendSettingsToWatch();
});

// Show Configuration (opens settings page on phone)
Pebble.addEventListener("showConfiguration", function(e) {
  var language = localStorage.getItem("pushup_language") || "de";
  var dailyGoal = localStorage.getItem("pushup_daily_goal") || "30";
  var reminderInterval = localStorage.getItem("pushup_reminder_interval") || "60";
  var activeStartHour = localStorage.getItem("pushup_active_start_hour") || "8";
  var activeEndHour = localStorage.getItem("pushup_active_end_hour") || "20";

  var url = "https://sirtob1.github.io/pebble-pushup-reminder/config.html?v=" + Date.now() +
            "#language=" + encodeURIComponent(language) +
            "&daily_goal=" + encodeURIComponent(dailyGoal) +
            "&reminder_interval=" + encodeURIComponent(reminderInterval) +
            "&active_start_hour=" + encodeURIComponent(activeStartHour) +
            "&active_end_hour=" + encodeURIComponent(activeEndHour);

  console.log("Pushups JS: Opening config page.");
  Pebble.openURL(url);
});

// WebView Closed (settings page sends back JSON)
Pebble.addEventListener("webviewclosed", function(e) {
  if (e && e.response) {
    try {
      var settings = JSON.parse(decodeURIComponent(e.response));
      console.log("Pushups JS: Received settings: " + JSON.stringify(settings));

      if (settings.language !== undefined) {
        localStorage.setItem("pushup_language", settings.language);
      }
      if (settings.daily_goal !== undefined) {
        localStorage.setItem("pushup_daily_goal", settings.daily_goal.toString());
      }
      if (settings.reminder_interval !== undefined) {
        localStorage.setItem("pushup_reminder_interval", settings.reminder_interval.toString());
      }
      if (settings.active_start_hour !== undefined) {
        localStorage.setItem("pushup_active_start_hour", settings.active_start_hour.toString());
      }
      if (settings.active_end_hour !== undefined) {
        localStorage.setItem("pushup_active_end_hour", settings.active_end_hour.toString());
      }

      // Send updated settings to watch
      sendSettingsToWatch();
    } catch (err) {
      console.log("Pushups JS: Error parsing webview response: " + err);
    }
  }
});

// AppMessage listener (receive data from watch)
Pebble.addEventListener("appmessage", function(e) {
  var dict = e.payload;
  console.log("Pushups JS: Received AppMessage: " + JSON.stringify(dict));
});
