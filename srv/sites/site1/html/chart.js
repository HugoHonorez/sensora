const createChart = (ctx, datasets, yLabel) => new Chart(ctx, {
    type: 'line',
    data: { datasets },
    options: {
      responsive: true,
      animation: false,
      interaction: { mode: 'nearest', intersect: false },
      plugins: {
        legend: { display: true }
      },
      scales: {
        x: {
          type: 'time',
          time: {
            tooltipFormat: 'dd/MM/yyyy HH:mm:ss',
            displayFormats: {
              second: 'HH:mm:ss',
              minute: 'HH:mm',
              hour: 'HH:mm',
            }
          },
          title: { display: true, text: 'Heure' }
        },
        y: {
          title: { display: true, text: yLabel }
        }
      }
    }
  });

  const tempChart = createChart(document.getElementById("tempChart").getContext("2d"), [
    { label: "Température (°C)", data: [], borderColor: 'red', backgroundColor: 'rgba(255,99,132,0.2)', tension: 0.3, pointRadius: 1 },
    { label: "Indice de chaleur (°C)", data: [], borderColor: 'orange', backgroundColor: 'rgba(255,159,64,0.2)', tension: 0.3, pointRadius: 1 }
  ], "Température (°C)");
  
  const humidityChart = createChart(document.getElementById("humidityChart").getContext("2d"), [
    { label: "Humidité (%)", data: [], borderColor: 'blue', backgroundColor: 'rgba(54,162,235,0.2)', tension: 0.3, pointRadius: 1 }
  ], "Humidité (%)");
  
  const pressureChart = createChart(document.getElementById("pressureChart").getContext("2d"), [
    { label: "Pression (hPa)", data: [], borderColor: 'purple', backgroundColor: 'rgba(153,102,255,0.2)', tension: 0.3, pointRadius: 1 }
  ], "Pression (hPa)");

  const lightChart = createChart(document.getElementById("lightChart").getContext("2d"), [
    { label: "Luminosité", data: [], borderColor: 'goldenrod', backgroundColor: 'rgba(255,206,86,0.2)', tension: 0.3, pointRadius: 1 }
  ], "Luminosité");

  const airqualityChart = createChart(document.getElementById("airqualityChart").getContext("2d"), [
    { label: "Qualité de l'air", data: [], borderColor: 'green', backgroundColor: 'rgba(86, 255, 108, 0.2)', tension: 0.3, pointRadius: 1 }
  ], "Qualité de l'air");
  
  function clearCharts() {
    [tempChart, humidityChart, pressureChart, lightChart, airqualityChart].forEach(chart =>
      chart.data.datasets.forEach(ds => ds.data = [])
    );
  }
  
  function updateCharts(data) {
    clearCharts();
    for (const dp of data) {
      const point = { x: dp.time, y: dp.value };
      switch (dp.field) {
        case "temperature": tempChart.data.datasets[0].data.push(point); break;
        case "heatindex": tempChart.data.datasets[1].data.push(point); break;
        case "humidity": humidityChart.data.datasets[0].data.push(point); break;
        case "pressure": pressureChart.data.datasets[0].data.push(point); break;
        case "light": lightChart.data.datasets[0].data.push(point); break;
        case "airquality": airqualityChart.data.datasets[0].data.push(point); break;
      }
    }
    tempChart.update();
    humidityChart.update();
    pressureChart.update();
    lightChart.update();
    airqualityChart.update();
  }
  
  const ws = new WebSocket("ws://" + location.hostname + ":8765");
  ws.onopen = () => {
    console.log("WebSocket ouvert.");
    sendQuery();
  };
  ws.onerror = (e) => {
    console.error("Erreur WebSocket", e);
  };
  ws.onclose = () => {
    console.warn("WebSocket fermé");
  };
  ws.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.type === "bulk") {
      updateCharts(message.data);
    }
  };
  
  function sendQuery() {
    const range = document.getElementById("filter-range").value;
    const step = document.getElementById("filter-step").value;
    const customStart = document.getElementById("filter-start").value;
    const customEnd = document.getElementById("filter-end").value;
  
    const payload = { step };
  
    if (customStart && customEnd) {
      payload.custom = {
        start: new Date(customStart).toISOString(),
        end: new Date(customEnd).toISOString()
      };
    } else {
      payload.range = range;
    }
  
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(payload));
    } else {
      console.warn("WebSocket non prêt.");
    }
  }
  
  document.getElementById("filter-form").addEventListener("submit", (e) => {
    e.preventDefault();
    sendQuery();
  });
  
  document.getElementById("filter-reset").addEventListener("click", () => {
    document.getElementById("filter-start").value = '';
    document.getElementById("filter-end").value = '';
    document.getElementById("filter-range").value = "-1h";
    document.getElementById("filter-step").value = "30s";
  
    sendQuery();
  });

  function formatNumber(value) {
    return value != null ? Number(value).toFixed(2) : '';
  }  

  function formatDateForExcel(date) {
    const d = new Date(date);
    return d.toLocaleString('fr-FR', {
      year: '2-digit',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      hour12: false
    }).replace(',', '');
  }    

  function downloadCSV() {
    const data = {
      temperature: tempChart.data.datasets[0].data,
      heatindex: tempChart.data.datasets[1].data,
      humidity: humidityChart.data.datasets[0].data,
      pressure: pressureChart.data.datasets[0].data,
      light: lightChart.data.datasets[0].data,
      airquality: airqualityChart.data.datasets[0].data
    };
  
    const headers = ['Time', 'Temperature (°C)', 'HeatIndex (°C)', 'Humidity (%)', 'Pressure (hPa)', 'Light', 'AirQuality'];
    
    let csvContent = headers.join(';') + '\n';
  
    const numDataPoints = data.temperature.length;
  
    for (let i = 0; i < data.temperature.length; i++) {
      const row = [
        formatDateForExcel(data.temperature[i].x),
        data.temperature[i].y.toFixed(2),
        data.heatindex[i]?.y.toFixed(2) || '',
        data.humidity[i]?.y.toFixed(2) || '',
        data.pressure[i]?.y.toFixed(2) || '',
        data.light[i]?.y.toFixed(2) || '',
        data.airquality[i]?.y.toFixed(2) || ''
      ];
      csvContent += row.join(';') + '\n';
    }
  
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = 'data.csv';
    link.click();
  }

  document.getElementById("filter-download").addEventListener("click", downloadCSV);
  
  