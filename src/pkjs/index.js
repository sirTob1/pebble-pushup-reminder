Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready for Pushup Reminder!');
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Show settings page
  Pebble.openURL('https://sirTob1.github.io/pebble-pushup-reminder/config.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    try {
      var settings = JSON.parse(decodeURIComponent(e.response));
      console.log('Settings received: ' + JSON.stringify(settings));
      // Send settings to watch
    } catch(err) {
      console.log('Error parsing settings: ' + err);
    }
  }
});
