var Clay = require('pebble-clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

// We must manually handle events to support the Gadgetbridge workarounds
// but Clay still handles the UI generation.
Pebble.addEventListener('showConfiguration', function(e) {
  var historyStr = localStorage.getItem("pushup_history") || "[]";
  clay.meta.userData = { historyStr: historyStr };
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
       if (key === 'LANGUAGE' && typeof dict[key] === 'string') {
         msg[key] = parseInt(dict[key], 10);
       } else {
         msg[key] = dict[key];
       }
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
  "ACTIVE_END_HOUR": 10004,
  "LOG_PUSHUPS": 10005,
  "LOG_TIMESTAMP": 10006,
  "REQUEST_SYNC": 10008,
  "SYNC_OFFLINE_DATA": 10009
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
  var msg = prepareMessage({"REQUEST_SYNC": 1});
  Pebble.sendAppMessage(msg, function() {
    console.log("Pushups JS: Requested offline sync.");
  });
});

// AppMessage listener (receive data from watch)
Pebble.addEventListener("appmessage", function(e) {
  var dict = e.payload;
  console.log("Pushups JS: Received AppMessage: " + JSON.stringify(dict));

  var logPushups = dict.LOG_PUSHUPS !== undefined ? dict.LOG_PUSHUPS : dict["10005"];
  var logTimestamp = dict.LOG_TIMESTAMP !== undefined ? dict.LOG_TIMESTAMP : dict["10006"];
  var syncData = dict.SYNC_OFFLINE_DATA !== undefined ? dict.SYNC_OFFLINE_DATA : dict["10009"];

  if (logPushups !== undefined && logTimestamp !== undefined) {
    var history = [];
    try {
      history = JSON.parse(localStorage.getItem("pushup_history") || "[]");
    } catch (err) {}
    
    history.push({
      count: logPushups,
      time: logTimestamp
    });
    
    localStorage.setItem("pushup_history", JSON.stringify(history));
    console.log("Pushups JS: Saved pushup session to history.");
  }

  if (syncData !== undefined) {
    var num_records = syncData[0];
    var hist = [];
    try {
      hist = JSON.parse(localStorage.getItem("pushup_history") || "[]");
    } catch (err) {}

    function getYearDay(timestamp) {
      var d = new Date(timestamp * 1000);
      var start = new Date(d.getFullYear(), 0, 0);
      var diff = (d - start) + ((start.getTimezoneOffset() - d.getTimezoneOffset()) * 60 * 1000);
      var oneDay = 1000 * 60 * 60 * 24;
      return Math.floor(diff / oneDay);
    }
    
    var dailyTotals = {};
    for (var i = 0; i < hist.length; i++) {
      var yday = getYearDay(hist[i].time);
      if (!dailyTotals[yday]) dailyTotals[yday] = 0;
      dailyTotals[yday] += hist[i].count;
    }

    var currentYDay = getYearDay(Math.floor(Date.now() / 1000));
    var currentYear = new Date().getFullYear();
    var added = false;

    for (var r = 0; r < num_records; r++) {
      var offset = 1 + r * 4;
      var achieved = syncData[offset] | (syncData[offset + 1] << 8);
      var r_yday = syncData[offset + 2] | (syncData[offset + 3] << 8);
      
      var existing = dailyTotals[r_yday] || 0;
      if (achieved > existing) {
        var missing = achieved - existing;
        var r_year = (r_yday > currentYDay + 10) ? currentYear - 1 : currentYear;
        var dummyDate = new Date(r_year, 0, r_yday);
        dummyDate.setHours(12, 0, 0, 0);
        
        hist.push({
          count: missing,
          time: Math.floor(dummyDate.getTime() / 1000)
        });
        added = true;
        console.log("Pushups JS: Synced " + missing + " missing pushups for yday " + r_yday);
      }
    }

    if (added) {
      localStorage.setItem("pushup_history", JSON.stringify(hist));
    }
  }
});
