<?php
// Assumendo che 'database.php' contenga la classe Database con i metodi connect e disconnect
include 'database.php';

// Endpoint JSON per richieste AJAX
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'fetch') {
    // Il nome del sensore viene recuperato da FormData (inviato dal JS)
    $sensor = isset($_POST['sensorName']) ? $_POST['sensorName'] : '';
    $start = isset($_POST['start']) ? $_POST['start'] : null; // expected 'YYYY-MM-DD HH:MM:SS'
    $end = isset($_POST['end']) ? $_POST['end'] : null;

    $pdo = Database::connect();

    $params = [];
    $sql = 'SELECT date, time, temperature, humidity, battery, RSSI FROM esp32_record WHERE 1=1 ';
    
    // Si assicura che il sensore sia valorizzato e lo usa nella query
    if ($sensor !== '') {
        $sql .= ' AND sensorName = :sensorName';
        $params[':sensorName'] = $sensor;
    } else {
        // Se il sensore non è specificato, restituisci un array vuoto
        header('Content-Type: application/json');
        echo json_encode([]);
        Database::disconnect();
        exit;
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
        // Combina data e ora nel formato richiesto da Chart.js/Moment.js
        $ts = $row['date'] . ' ' . $row['time']; 
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

// Recupera il nome del sensore dalla URL (se presente)
$sensorNameFromUrl = isset($_GET['sensorName']) ? htmlspecialchars($_GET['sensorName']) : '';

// Pagina HTML (interfaccia grafico)
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Dati Sensore: <?= $sensorNameFromUrl ?: 'N/A' ?></title>

<style>
    body { 
        font-family: Arial, sans-serif; 
        margin: 8px; 
        font-size: 12px;
    }
    
    .controls { 
        display:flex; 
        gap: 4px; 
        flex-wrap: wrap; 
        align-items: center; 
        margin-bottom: 8px; 
        max-width: 100%; 
    }
    
    label { 
        font-size: 12px; 
        white-space: nowrap; 
    }
    
    input, select, button { 
        padding: 4px 6px; 
        font-size: 12px;
        box-sizing: border-box; 
    }
    
    /* Regola specifica per i campi data/ora */
    #start, #end {
        width: auto; 
        min-width: 100px; 
        flex-grow: 0;      
        flex-shrink: 1;    
        flex-basis: auto;  
        
        font-size: 12px !important; 
    }

    /* Regole specifiche per i pulsanti */
    .controls button {
        flex-grow: 0;
        flex-shrink: 0; 
        min-width: unset;
    }

    /* Regole specifiche per i pulsanti Avanti/Indietro */
    #btnPrev, #btnNext {
        background-color: #0c6980; 
        color: white;
        border: 1px solid #0c6980;
        border-radius: 4px;
        cursor: pointer;
    }
    #btnPrev:hover, #btnNext:hover {
        background-color: #0a5264; 
    }

    #chart-container { 
        width: 100%; 
        max-width: 900px; 
        margin-top: 8px; 
    }
    
    /* NASCONDI L'AREA DEL SENSORE (poiché è valorizzato esternamente) */
    #sensor-control {
        display: none;
    }
</style>


<script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.29.4/moment.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.min.js"></script>
</head>
<body>
    <div class="controls">
        <div id="sensor-control">
            <label>Sensor:</label>
            <input type="hidden" id="sensorName" value="<?= $sensorNameFromUrl ?>">
            <label id="sensorDisplayName"><?= $sensorNameFromUrl ?></label>
        </div>


        <label>Da:</label>
        <input type="datetime-local" id="start" />
        <label>A:</label>
        <input type="datetime-local" id="end" />
        <button id="load">Carica</button>

        <button id="btn1h">Ora</button>
        <button id="btn1d">Giorno</button>
        <button id="btn7d">Settimana</button>
        <button id="btn1mo">Mese</button>
        <button id="btnPrev">«««</button>
        <button id="btnNext">»»»</button>
        
    </div>

    <div id="chart-container">
        <canvas id="mainChart"></canvas>
    </div>

    <script>
        // Variabile globale per il grafico
        let chart = null; 
        
        // Formato utilizzato per l'input datetime-local (Moment.js format string)
        const INPUT_DATETIME_FORMAT = "YYYY-MM-DDTHH:mm"; 

        // Helper: converte un oggetto Date o Moment in una stringa adatta all'input datetime-local
        function dateToInput(d){
            // Utilizza Moment.js per garantire il formato YYYY-MM-DDTHH:mm (24 ore)
            return moment(d).format(INPUT_DATETIME_FORMAT);
        }

        /**
         * Logica principale per caricare i dati e inizializzare/aggiornare il grafico.
         */
        async function loadChart(){
            const sensorInput = document.getElementById('sensorName');
            const sensor = sensorInput ? sensorInput.value : '';
            
            const ctx = document.getElementById('mainChart').getContext('2d');
            
            if (!sensor) { 
                if (chart) chart.destroy();
                ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
                ctx.font = '16px Arial';
                ctx.fillText("Errore: Nome del sensore non specificato nella URL.", 10, 50);
                return; 
            }
            
            const startVal = document.getElementById('start').value; 
            const endVal = document.getElementById('end').value;
            
            // CONVERSIONE: da 'YYYY-MM-DDTHH:MM' (input) a 'YYYY-MM-DD HH:MM:SS' (MySQL)
            const start = startVal ? startVal.replace('T',' ') + ':00' : '';
            const end = endVal ? endVal.replace('T',' ') + ':59' : '';
            
            const fd = new FormData();
            fd.append('action','fetch');
            fd.append('sensorName', sensor); 
            if (start) fd.append('start', start);
            if (end) fd.append('end', end);

            try {
                const res = await fetch(window.location.href, { method:'POST', body: fd });
                const json = await res.json();

                const labels = json.map(r => r.timestamp);
                const tempData = json.map(r => r.temperature !== null ? parseFloat(r.temperature) : null);
                const humData = json.map(r => r.humidity !== null ? parseFloat(r.humidity) : null);

                if (chart) chart.destroy();

                if (labels.length === 0) {
                    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
                    ctx.font = '16px Arial';
                    ctx.fillText(`Nessun dato trovato per il sensore "${sensor}" nell'intervallo selezionato.`, 10, 50);
                    return;
                }

                // Inizializzazione del grafico
                chart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: labels,
                        datasets: [
                            {
                                label: 'Temperature (°C)',
                                data: tempData,
                                fill: false,
                                borderColor: 'rgba(200,85,25,0.9)',
                                backgroundColor: 'rgba(200,85,25,0.9)',
                                yAxisID: 'y-temp',
                                pointRadius: 1,
                                pointHoverRadius: 4,
                            },
                            {
                                label: 'Humidity (%)',
                                data: humData,
                                fill: false,
                                borderColor: 'rgba(19, 173, 194, 0.9)',
                                backgroundColor: 'rgba(19, 173, 194, 0.9)',
                                yAxisID: 'y-hum',
                                pointRadius: 1,
                                pointHoverRadius: 4,
                            }
                        ]
                    },
                    options: {
                        responsive: true,
                        tooltips: { 
                            mode: 'index', 
                            intersect: false, 
                            // Formato tooltip coerente con il 24h
                            callbacks: {
                                title: function(tooltipItem, data) {
                                    return moment(data.labels[tooltipItem[0].index], 'YYYY-MM-DD HH:mm:ss').format('DD/MM/YYYY HH:mm');
                                },
                                // Assicura che la visualizzazione dei valori nel tooltip sia corretta
                                label: function(tooltipItem, data) {
                                    let label = data.datasets[tooltipItem.datasetIndex].label || '';
                                    if (label) {
                                        label += ': ';
                                    }
                                    let value = tooltipItem.yLabel;
                                    if (tooltipItem.datasetIndex === 0) { // Temperatura
                                        return label + value.toFixed(1) + ' °C';
                                    } else if (tooltipItem.datasetIndex === 1) { // Umidità
                                        return label + Math.round(value) + ' %'; // Arrotonda per intero
                                    }
                                    return label + value;
                                }
                            }
                        },
                        hover: { mode: 'nearest', intersect: true },
                        scales: {
                            xAxes: [{
                                type: 'time',
                                time: { 
                                    parser: 'YYYY-MM-DD HH:mm:ss', 
                                    tooltipFormat: 'DD/MM/YYYY HH:mm', // Già in 24h
                                    displayFormats: { 
                                        minute: 'DD HH:mm', 
                                        hour: 'DD HH:mm',    
                                        day: 'DD',           
                                        week: 'DD MMM', 
                                        month: 'MM/YYYY'
                                    }
                                },
                                scaleLabel: { display: false, labelString: 'Date/Time' }, 
                                ticks: {
                                    autoSkip: true, 
                                    maxRotation: 90,
                                    minRotation: 90
                                }
                            }],
                            yAxes: [
                                { 
                                    id: 'y-temp', 
                                    position: 'left', 
                                    scaleLabel: { display: true, labelString: 'Temperature (°C)' },
                                    ticks: {
                                        maxTicksLimit: 40,
                                        // Mantiene 1 decimale per la temperatura
                                        callback: function(value, index, values) {
                                            return value.toFixed(1);
                                        } 
                                    }
                                },
                                { 
                                    id: 'y-hum', 
                                    position: 'right', 
                                    scaleLabel: { display: true, labelString: 'Humidity (%)' },
                                    ticks: {
                                        maxTicksLimit: 40,
                                        // CORREZIONE: Rimuove i decimali per l'umidità (solo intero)
                                        callback: function(value, index, values) {
                                            return Math.round(value); 
                                        } 
                                    }
                                }
                            ]
                        }
                    }
                });
            } catch (error) {
                console.error("Errore nel recupero o nel parsing dei dati:", error);
                if (chart) chart.destroy();
                ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
                ctx.font = '16px Arial';
                ctx.fillText("Errore nel caricamento dei dati. Controlla il server PHP.", 10, 50);
            }
        }

        // Set default last 24h or use URL params if provided
        (function setDefaults(){
            const params = new URLSearchParams(window.location.search);
            const endDefault = moment();
            const startDefault = moment().subtract(24, 'hours');
            
            // Imposta le date/ore
            if (params.has('start')) {
                document.getElementById('start').value = params.get('start');
            } else {
                document.getElementById('start').value = dateToInput(startDefault);
            }
            if (params.has('end')) {
                document.getElementById('end').value = params.get('end');
            } else {
                document.getElementById('end').value = dateToInput(endDefault);
            }
        })();


        document.getElementById('load').addEventListener('click', ()=>{
            loadChart();
        });

        /**
         * Sposta l'intervallo di tempo (start e end) in avanti o indietro
         */
        function shiftInterval(direction) {
            const startInput = document.getElementById('start');
            const endInput = document.getElementById('end');

            if (!startInput.value || !endInput.value) {
                alert('Seleziona prima un intervallo di tempo.');
                return;
            }

            const startMoment = moment(startInput.value, INPUT_DATETIME_FORMAT);
            const endMoment = moment(endInput.value, INPUT_DATETIME_FORMAT);

            const durationMs = endMoment.valueOf() - startMoment.valueOf();

            if (durationMs <= 0) {
                alert('L\'intervallo di tempo non è valido (Data Da deve essere precedente a Data A).');
                return;
            }

            const newStart = moment(startMoment).add(durationMs * direction, 'milliseconds');
            const newEnd = moment(endMoment).add(durationMs * direction, 'milliseconds');

            startInput.value = dateToInput(newStart);
            endInput.value = dateToInput(newEnd);
            loadChart();
        }

        document.getElementById('btnPrev').addEventListener('click', ()=>{
            shiftInterval(-1); // Sposta indietro
        });

        document.getElementById('btnNext').addEventListener('click', ()=>{
            shiftInterval(1); // Sposta avanti
        });

        // Quick interval buttons (1 Ora, 1 Giorno, 7 Giorni, 1 Mese)
        function setQuickInterval(durationUnit, durationValue) {
            const end = moment();
            const start = moment().subtract(durationValue, durationUnit); 
            
            document.getElementById('start').value = dateToInput(start);
            document.getElementById('end').value = dateToInput(end);
            
            loadChart();
        }

        document.getElementById('btn1h').addEventListener('click', ()=>{
            setQuickInterval('hours', 1); 
        });
        
        document.getElementById('btn1d').addEventListener('click', ()=>{
            setQuickInterval('days', 1);
        });

        document.getElementById('btn7d').addEventListener('click', ()=>{
            setQuickInterval('days', 7);
        });
        
        document.getElementById('btn1mo').addEventListener('click', ()=>{
            setQuickInterval('months', 1); 
        });


        // Auto-load on open
        window.addEventListener('load', ()=>{ 
            loadChart(); 
        });
    </script>
</body>
</html>
