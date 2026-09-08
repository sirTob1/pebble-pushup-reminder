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

      var labels = [];
      var values = [];
      for (var j = 0; j < dates.length; j++) {
        var parts = dates[j].split("-");
        labels.push(parts[2] + "." + parts[1] + ".");
        values.push(dailyTotals[dates[j]]);
      }

      var iframeHTML = `
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
  body, html { margin:0; padding:0; background:transparent; overflow:hidden; font-family:sans-serif; }
  canvas { display:block; width:100%; height:180px; }
</style>
</head>
<body>
<canvas id="chart"></canvas>
<script>
  let chart;
  window.addEventListener('message', function(e) {
    if(e.data && e.data.type === 'render') {
      if(chart) chart.destroy();
      const ctx = document.getElementById('chart').getContext('2d');
      chart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: e.data.labels,
          datasets: [{
            label: 'Pushups',
            data: e.data.values,
            backgroundColor: '#FF4700',
            borderRadius: 4
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: { legend: { display: false } },
          scales: {
            y: { beginAtZero: true, ticks: { precision: 0 } },
            x: { grid: { display: false } }
          }
        }
      });
    }
  });
  window.parent.postMessage('chartReady', '*');
</script>
</body>
</html>`;

      var encodedSrc = "data:text/html;charset=utf-8," + encodeURIComponent(iframeHTML.trim());
      var html = '<iframe id="chart-iframe" src="' + encodedSrc + '" style="width:100%; height:180px; border:none; margin-bottom:10px;"></iframe>';
      html += '<p style="font-size:12px; color:#666; text-align:center;">Total Entries: ' + history.length + '</p>';

      $('#dashboard-chart').set('innerHTML', html);

      window.addEventListener('message', function(e) {
        if(e.data === 'chartReady') {
          var iframe = document.getElementById('chart-iframe');
          if(iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({ type: 'render', labels: labels, values: values }, '*');
          }
        }
      });

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
