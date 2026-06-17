module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var historyStr = localStorage.getItem("pushup_history");
    if (!historyStr) return;

    try {
      var history = JSON.parse(historyStr);
      if (!history || history.length === 0) return;

      // Group by date (YYYY-MM-DD) for charting
      var dailyTotals = {};
      for (var i = 0; i < history.length; i++) {
        var entry = history[i];
        var d = new Date(entry.time * 1000);
        var dateStr = d.getFullYear() + "-" + (d.getMonth()+1) + "-" + d.getDate();
        if (!dailyTotals[dateStr]) {
          dailyTotals[dateStr] = 0;
        }
        dailyTotals[dateStr] += entry.count;
      }

      var dates = Object.keys(dailyTotals);
      // Keep last 7 days for the chart
      dates = dates.slice(-7);

      var maxCount = 0;
      for (var j = 0; j < dates.length; j++) {
        if (dailyTotals[dates[j]] > maxCount) maxCount = dailyTotals[dates[j]];
      }

      // Generate HTML chart
      var html = '<div style="display:flex; align-items:flex-end; height:100px; padding-bottom:10px; border-bottom:1px solid #ccc; margin-bottom:10px; gap:4px;">';
      
      for (var k = 0; k < dates.length; k++) {
        var dt = dates[k];
        var count = dailyTotals[dt];
        var heightPct = maxCount > 0 ? (count / maxCount * 100) : 0;
        html += '<div style="flex:1; display:flex; flex-direction:column; justify-content:flex-end; align-items:center;">';
        html += '<div style="font-size:10px; color:#555;">' + count + '</div>';
        html += '<div style="width:100%; height:' + heightPct + 'px; background:#0055AA; min-height:1px;"></div>';
        var shortDate = dt.split("-")[2] + "." + dt.split("-")[1] + ".";
        html += '<div style="font-size:9px; color:#888; margin-top:2px;">' + shortDate + '</div>';
        html += '</div>';
      }
      html += '</div>';
      html += '<p style="font-size:12px; color:#666;">Total Entries: ' + history.length + '</p>';

      $('#dashboard-chart').set('innerHTML', html);

      // Setup Export CSV
      $('#btn-export-csv').on('click', function(e) {
        e.preventDefault();
        var csvContent = "data:text/csv;charset=utf-8,";
        csvContent += "Date,Timestamp,Pushups\n";
        for (var n = 0; n < history.length; n++) {
          var row = history[n];
          var rd = new Date(row.time * 1000);
          csvContent += rd.toISOString() + "," + row.time + "," + row.count + "\n";
        }
        var encodedUri = encodeURI(csvContent);
        var link = document.createElement("a");
        link.setAttribute("href", encodedUri);
        link.setAttribute("download", "pushups_history.csv");
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
      });

    } catch (e) {
      console.log("Error rendering dashboard: " + e);
    }
  });
};
