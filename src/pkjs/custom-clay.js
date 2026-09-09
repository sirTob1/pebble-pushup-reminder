module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var historyStr = clayConfig.meta.userData && clayConfig.meta.userData.historyStr;
    if (!historyStr) historyStr = localStorage.getItem("pushup_history");
    if (!historyStr || historyStr === "[]") return;

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

      var datesAll = Object.keys(dailyTotals);

      var currentLabels = [];
      var currentValues = [];

      function renderDashboard() {
        var timeframe = '7';
        var viewMode = 'chart';
        try {
          timeframe = localStorage.getItem('dashboard_timeframe') || '7';
          viewMode = localStorage.getItem('dashboard_view') || 'chart';
        } catch(e) {}

        var dates = datesAll.slice();
        if (timeframe !== 'all') {
          var limit = parseInt(timeframe, 10);
          if (dates.length > limit) {
             dates = dates.slice(-limit);
          }
        }

        currentLabels = [];
        currentValues = [];
        for (var j = 0; j < dates.length; j++) {
          var parts = dates[j].split("-");
          currentLabels.push(parts[2] + "." + parts[1] + ".");
          currentValues.push(dailyTotals[dates[j]]);
        }

        var controlsHTML = '<div style="display:flex; justify-content:space-between; margin-bottom:10px;">' +
          '<select id="timeframe-select" style="flex:1; margin-right:10px; padding:5px; border-radius:4px; font-size:14px;">' +
            '<option value="7"' + (timeframe==='7'?' selected':'') + '>Last 7 Days</option>' +
            '<option value="30"' + (timeframe==='30'?' selected':'') + '>Last 30 Days</option>' +
            '<option value="365"' + (timeframe==='365'?' selected':'') + '>Last Year</option>' +
            '<option value="all"' + (timeframe==='all'?' selected':'') + '>All Time</option>' +
          '</select>' +
          '<select id="view-select" style="flex:1; padding:5px; border-radius:4px; font-size:14px;">' +
            '<option value="chart"' + (viewMode==='chart'?' selected':'') + '>Chart</option>' +
            '<option value="table"' + (viewMode==='table'?' selected':'') + '>Table</option>' +
          '</select>' +
        '</div>';

        var contentHTML = '';

        if (viewMode === 'chart') {
          var iframeHTML = '<!DOCTYPE html>' +
            '<html>' +
            '<head>' +
            '<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">' +
            '<script src="https://cdn.jsdelivr.net/npm/chart.js"></' + 'script>' +
            '<style>' +
            '  body, html { margin:0; padding:0; background:transparent; overflow:hidden; font-family:sans-serif; }' +
            '  canvas { display:block; width:100%; height:180px; }' +
            '</style>' +
            '</head>' +
            '<body>' +
            '<canvas id="chart"></canvas>' +
            '<script>' +
            '  var chart;' +
            '  window.addEventListener("message", function(e) {' +
            '    if(e.data && e.data.type === "render") {' +
            '      if(chart) chart.destroy();' +
            '      var ctx = document.getElementById("chart").getContext("2d");' +
            '      chart = new Chart(ctx, {' +
            '        type: "bar",' +
            '        data: {' +
            '          labels: e.data.labels,' +
            '          datasets: [{' +
            '            label: "Pushups",' +
            '            data: e.data.values,' +
            '            backgroundColor: "#FF4700",' +
            '            borderRadius: 4' +
            '          }]' +
            '        },' +
            '        options: {' +
            '          responsive: true,' +
            '          maintainAspectRatio: false,' +
            '          plugins: { legend: { display: false } },' +
            '          scales: {' +
            '            y: { beginAtZero: true, ticks: { precision: 0 } },' +
            '            x: { grid: { display: false } }' +
            '          }' +
            '        }' +
            '      });' +
            '    }' +
            '  });' +
            '  window.parent.postMessage("chartReady", "*");' +
            '</' + 'script>' +
            '</body>' +
            '</html>';

          var encodedSrc = "data:text/html;charset=utf-8," + encodeURIComponent(iframeHTML.trim());
          contentHTML = '<iframe id="chart-iframe" src="' + encodedSrc + '" style="width:100%; height:180px; border:none; margin-bottom:10px;"></iframe>';
        } else {
          contentHTML = '<div style="max-height:180px; overflow-y:auto; border:1px solid #ccc; border-radius:4px; margin-bottom:10px;">' +
            '<table style="width:100%; border-collapse:collapse; font-size:14px; font-family:sans-serif;">' +
            '<thead style="background:#eee; position:sticky; top:0;"><tr><th style="padding:8px; text-align:left; border-bottom:1px solid #ccc;">Date</th><th style="padding:8px; text-align:right; border-bottom:1px solid #ccc;">Pushups</th></tr></thead>' +
            '<tbody>';
          
          for (var k = dates.length - 1; k >= 0; k--) {
            contentHTML += '<tr><td style="padding:8px; border-bottom:1px solid #eee;">' + currentLabels[k] + '</td><td style="padding:8px; text-align:right; border-bottom:1px solid #eee;">' + currentValues[k] + '</td></tr>';
          }
          contentHTML += '</tbody></table></div>';
        }

        var sum = 0;
        for (var s = 0; s < currentValues.length; s++) sum += currentValues[s];

        var summaryHTML = '<p style="font-size:12px; color:#666; text-align:center;">Total Pushups in Period: ' + sum + '</p>';

        $('#dashboard-chart').set('innerHTML', controlsHTML + contentHTML + summaryHTML);

        var tfSelect = document.getElementById('timeframe-select');
        if (tfSelect) {
          tfSelect.addEventListener('change', function(e) {
            try { localStorage.setItem('dashboard_timeframe', e.target.value); } catch(err) {}
            renderDashboard();
          });
        }
        var vwSelect = document.getElementById('view-select');
        if (vwSelect) {
          vwSelect.addEventListener('change', function(e) {
            try { localStorage.setItem('dashboard_view', e.target.value); } catch(err) {}
            renderDashboard();
          });
        }
      }

      if (!window.__dashboardMessageListenerAdded) {
        window.addEventListener('message', function(e) {
          if(e.data === 'chartReady') {
            var iframe = document.getElementById('chart-iframe');
            if(iframe && iframe.contentWindow) {
              iframe.contentWindow.postMessage({ type: 'render', labels: currentLabels, values: currentValues }, '*');
            }
          }
        });
        window.__dashboardMessageListenerAdded = true;
      }

      renderDashboard();

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
      try {
        $('#dashboard-chart').set('innerHTML', '<div style="color:red; font-size:12px;">Error: ' + e.message + '</div>');
      } catch(e2) {}
      console.log("Error rendering dashboard: " + e);
    }
  });
};
