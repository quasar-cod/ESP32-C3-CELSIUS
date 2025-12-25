<?php
include 'database.php';

// Endpoint JSON per richieste AJAX
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'fetch') {
    $sensor = isset($_POST['sensorName']) ? $_POST['sensorName'] : '';
    $start = isset($_POST['start']) ? $_POST['start'] : null; // expected 'YYYY-MM-DD HH:MM:SS'
    $end = isset($_POST['end']) ? $_POST['end'] : null;

    $pdo = Database::connect();

    $params = [];
    $sql = 'SELECT date, time, temperature, humidity, battery, RSSI FROM esp32_record WHERE 1=1 ';
    if ($sensor !== '') {
        $sql .= ' AND sensorName = :sensorName';
        $params[':sensorName'] = $sensor;
    }
    if ($start && $end) {
        // Use CONCAT to combine date and time for comparison
        $sql .= ' AND CONCAT(date, " ", time) BETWEEN :start AND :end';
        $params[':start'] = $start;
        $params[':end'] = $end;
    }
    $sql .= ' ORDER BY date ASC, time ASC';

    $stmt = $pdo->prepare($sql);
    $stmt->execute($params);

    $out = [];
    while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $ts = $row['date'] . ' ' . $row['time']; // format MySQL datetime
        $out[] = [
            'timestamp' => $ts,
            'temperature' => $row['temperature'],
            'humidity' => $row['humidity'],
            'battery' => $row['battery'],
            'RSSI' => $row['RSSI']
        ];
    }

    header('Content-Type: application/json');
    echo json_encode($out);
    Database::disconnect();
    exit;
}

// Pagina HTML (interfaccia grafico)
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Grafico Celsius</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 16px; }
        .controls { display:flex; gap:8px; flex-wrap:wrap; align-items:center; margin-bottom:12px; }
        label { font-size:14px; }
        input, select, button { padding:6px 8px; font-size:14px; }
        #chart-container { width:100%; max-width:900px; margin-top:12px; }
    </style>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.29.4/moment.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.min.js"></script>
</head>
<body>
    <h3 style="color:#0c6980;">Grafico per esp32_record (date + time)</h3>
    <div class="controls">
        <label>Sensor:</label>
        <input type="text" id="sensorName" value="SWBT01" />
        <label>Da:</label>
        <input type="datetime-local" id="start" />
        <label>A:</label>
        <input type="datetime-local" id="end" />
        <button id="btn1d">1 Giorno</button>
        <button id="btn7d">1 Settimana</button>
        <button id="load">Carica grafico</button>
    </div>

    <div id="chart-container">
        <canvas id="mainChart"></canvas>
    </div>

    <script>
        // Helper: format Date -> 'YYYY-MM-DD HH:MM:SS'
        function toMySQLDateTime(d) {
            function pad(n){return n<10? '0'+n : n}
            return d.getFullYear() + '-' + pad(d.getMonth()+1) + '-' + pad(d.getDate()) + ' ' + pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds());
        }

        // Set default last 24h or use URL params if provided
        (function setDefaults(){
            const params = new URLSearchParams(window.location.search);
            const endDefault = new Date();
            const startDefault = new Date(endDefault.getTime() - 24*60*60*1000);
            const toInput = (d) => d.toISOString().slice(0,16);

            if (params.has('sensorName')) {
                document.getElementById('sensorName').value = params.get('sensorName');
            }
            if (params.has('start')) {
                document.getElementById('start').value = params.get('start');
            } else {
                document.getElementById('start').value = toInput(startDefault);
            }
            if (params.has('end')) {
                document.getElementById('end').value = params.get('end');
            } else {
                document.getElementById('end').value = toInput(endDefault);
            }
        })();

        let chart = null;

        document.getElementById('load').addEventListener('click', ()=>{
            loadChart();
        });

        // Helper: convert Date -> 'YYYY-MM-DDTHH:MM' for datetime-local inputs
        function dateToInput(d){ return d.toISOString().slice(0,16); }

        // Quick interval buttons
        document.getElementById('btn1d').addEventListener('click', ()=>{
            const end = new Date();
            const start = new Date(end.getTime() - 24*60*60*1000);
            document.getElementById('start').value = dateToInput(start);
            document.getElementById('end').value = dateToInput(end);
            loadChart();
        });

        document.getElementById('btn7d').addEventListener('click', ()=>{
            const end = new Date();
            const start = new Date(end.getTime() - 7*24*60*60*1000);
            document.getElementById('start').value = dateToInput(start);
            document.getElementById('end').value = dateToInput(end);
            loadChart();
        });

        async function loadChart(){
            const sensor = document.getElementById('sensorName').value;
            const startVal = document.getElementById('start').value; // 'YYYY-MM-DDTHH:MM'
            const endVal = document.getElementById('end').value;
            if (!sensor) { alert('Inserisci sensorName'); return; }
            // convert to MySQL datetime
            const start = startVal ? startVal.replace('T',' ') + ':00' : '';
            const end = endVal ? endVal.replace('T',' ') + ':59' : '';
            const fd = new FormData();
            fd.append('action','fetch');
            fd.append('sensorName', sensor);
            if (start) fd.append('start', start);
            if (end) fd.append('end', end);

            const res = await fetch(window.location.href, { method:'POST', body: fd });
            const json = await res.json();

            const labels = json.map(r => r.timestamp);
            const tempData = json.map(r => r.temperature !== null ? parseFloat(r.temperature) : null);
            const humData = json.map(r => r.humidity !== null ? parseFloat(r.humidity) : null);

            const ctx = document.getElementById('mainChart').getContext('2d');
            if (chart) chart.destroy();
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [
                        {
                            label: 'Temperature (°C)',
                            data: tempData,
                            fill: false,
                            borderColor: 'rgba(12,105,128,0.9)',
                            backgroundColor: 'rgba(12,105,128,0.9)',
                            yAxisID: 'y-temp',
                            pointRadius: 3,
                            pointHoverRadius: 6,
                        },
                        {
                            label: 'Humidity (%)',
                            data: humData,
                            fill: false,
                            borderColor: 'rgba(200,85,25,0.9)',
                            backgroundColor: 'rgba(200,85,25,0.9)',
                            yAxisID: 'y-hum',
                            pointRadius: 3,
                            pointHoverRadius: 6,
                        }
                    ]
                },
                options: {
                    responsive: true,
                    tooltips: { mode: 'index', intersect: false },
                    hover: { mode: 'nearest', intersect: true },
                    scales: {
                        xAxes: [{
                            type: 'time',
                            time: { parser: 'YYYY-MM-DD HH:mm:ss', tooltipFormat: 'll HH:mm', displayFormats: { minute: 'HH:mm', hour: 'DD/MM HH:mm' } },
                            scaleLabel: { display: true, labelString: 'Date/Time' },
                            ticks: {
                                autoSkip: true,
                                maxRotation: 90,
                                minRotation: 90
                            }
                        }],
                        yAxes: [
                            { id: 'y-temp', position: 'left', scaleLabel: { display: true, labelString: 'Temperature (°C)' } },
                            { id: 'y-hum', position: 'right', scaleLabel: { display: true, labelString: 'Humidity (%)' }}//, ticks: { min: 0, max: 100 } }
                        ]
                    }
                }
            });
        }

        // Auto-load on open
        window.addEventListener('load', ()=>{ loadChart(); });
    </script>
</body>
</html>
