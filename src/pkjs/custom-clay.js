module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var historyStr = clayConfig.meta.userData && clayConfig.meta.userData.historyStr;
    if (!historyStr) historyStr = localStorage.getItem("pushup_history");

    if (!historyStr || historyStr === "[]") {
      var btn = document.getElementById('btn-export-csv');
      if (btn) btn.style.display = 'none';
      return;
    }

    try {
      var history = JSON.parse(historyStr);
      if (!history || history.length === 0) {
        var btn = document.getElementById('btn-export-csv');
        if (btn) btn.style.display = 'none';
        return;
      }

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

      var currentLabels = [];
      var currentValues = [];
      var activeTimeframe = null;
      var activeViewMode = null;

      function renderDashboard() {
        if (activeTimeframe === null) {
          try { activeTimeframe = localStorage.getItem('dashboard_timeframe') || '7'; } catch(e) { activeTimeframe = '7'; }
        }
        if (activeViewMode === null) {
          try { activeViewMode = localStorage.getItem('dashboard_view') || 'chart'; } catch(e) { activeViewMode = 'chart'; }
        }

        var timeframe = activeTimeframe;
        var viewMode = activeViewMode;

        var dates = [];
        var today = new Date();
        today.setHours(0, 0, 0, 0);

        if (timeframe !== 'all') {
          var limit = parseInt(timeframe, 10);
          for (var i = limit - 1; i >= 0; i--) {
            var d = new Date(today.getTime() - i * 24 * 60 * 60 * 1000);
            var dateStr = d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate();
            dates.push(dateStr);
          }
        } else {
          var firstDate = today;
          if (history.length > 0) {
            firstDate = new Date(history[0].time * 1000);
            firstDate.setHours(0, 0, 0, 0);
          }
          var daysDiff = Math.floor((today.getTime() - firstDate.getTime()) / (1000 * 60 * 60 * 24));
          if (daysDiff > 1825) daysDiff = 1825; 
          if (daysDiff < 0) daysDiff = 0;
          for (var i = daysDiff; i >= 0; i--) {
            var d = new Date(today.getTime() - i * 24 * 60 * 60 * 1000);
            var dateStr = d.getFullYear() + "-" + (d.getMonth() + 1) + "-" + d.getDate();
            dates.push(dateStr);
          }
        }

        currentLabels = [];
        currentValues = [];
        for (var j = 0; j < dates.length; j++) {
          var parts = dates[j].split("-");
          currentLabels.push(parts[2] + "." + parts[1] + ".");
          currentValues.push(dailyTotals[dates[j]] || 0);
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
            activeTimeframe = e.target.value;
            try { localStorage.setItem('dashboard_timeframe', activeTimeframe); } catch(err) {}
            renderDashboard();
          });
        }
        var vwSelect = document.getElementById('view-select');
        if (vwSelect) {
          vwSelect.addEventListener('change', function(e) {
            activeViewMode = e.target.value;
            try { localStorage.setItem('dashboard_view', activeViewMode); } catch(err) {}
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
        
        var rawCsvContent = "Date,Timestamp,Pushups\n";
        for (var n = 0; n < history.length; n++) {
          var row = history[n];
          var rd = new Date(row.time * 1000);
          rawCsvContent += rd.toISOString() + "," + row.time + "," + row.count + "\n";
        }
        
        // Hide the export button
        var exportBtn = document.getElementById('btn-export-csv');
        if (exportBtn) exportBtn.style.display = 'none';
        
        // Replace dashboard chart with textarea
        var exportHTML = '<div style="margin-bottom:10px;">' +
          '<p style="font-size:14px; font-weight:bold; margin-top:0;">CSV Export (Copy Text below)</p>' +
          '<textarea id="csv-export-area" style="width:100%; height:180px; font-family:monospace; font-size:12px; padding:5px; box-sizing:border-box;">' + rawCsvContent + '</textarea>' +
          '<div style="display:flex; justify-content:flex-end; margin-top:10px;">' +
            '<button class="btn" id="btn-close-export" style="padding:4px 15px; font-size:14px; background-color:#aaa; border:none; border-radius:4px; color:#fff;">Close</button>' +
          '</div>' +
        '</div>';
        
        $('#dashboard-chart').set('innerHTML', exportHTML);
        
        // Auto-select text in textarea
        var textArea = document.getElementById('csv-export-area');
        if (textArea) {
          textArea.focus();
          textArea.select();
          try { textArea.setSelectionRange(0, 99999); } catch(err) {}
        }
        
        // Handle close button
        $('#btn-close-export').on('click', function(ev) {
          ev.preventDefault();
          if (exportBtn) exportBtn.style.display = 'block';
          renderDashboard();
        });
      });

    } catch (e) {
      try {
        $('#dashboard-chart').set('innerHTML', '<div style="color:red; font-size:12px;">Error: ' + e.message + '</div>');
      } catch(e2) {}
      console.log("Error rendering dashboard: " + e);
    }
  });
};
