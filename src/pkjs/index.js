// Pebble Pushup Reminder - PebbleKit JS Companion (ES5)
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

// We must manually handle events to support the Gadgetbridge workarounds
// but Clay still handles the UI generation.
Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  
  // We don't send the dict directly because we want the Gadgetbridge 
  // integer keys workaround. We extract what clay parsed.
  var msg = {};
  for (var key in dict) {
    if (dict.hasOwnProperty(key)) {
       msg[key] = dict[key];
    }
  }

  // Send settings to watch using the Gadgetbridge compatibility wrapper
  var prepared = prepareMessage(msg);
  Pebble.sendAppMessage(prepared, function() {
    console.log("Pushups JS: Settings sent to watch successfully.");
  }, function(err) {
    console.log("Pushups JS: Failed to send settings: " + JSON.stringify(err));
  });
});

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

// Ready event
Pebble.addEventListener("ready", function(e) {
  console.log("Pushups JS: Ready!");
});

// AppMessage listener (receive data from watch)
Pebble.addEventListener("appmessage", function(e) {
  var dict = e.payload;
  console.log("Pushups JS: Received AppMessage: " + JSON.stringify(dict));
});
