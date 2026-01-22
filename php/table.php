<?php
// Includere database.php per la connessione
include 'database.php';

// Endpoint JSON per richieste AJAX
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action']) && $_POST['action'] === 'fetch') {
    // Il nome del sensore viene recuperato da FormData
    $sensor = isset($_POST['sensorName']) ? $_POST['sensorName'] : '';
    $start = isset($_POST['start']) ? $_POST['start'] : null;
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
    $sql .= ' ORDER BY date desc, time desc';

    $stmt = $pdo->prepare($sql);
    $stmt->execute($params);

    $out = [];
    while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        // Combina data e ora senza secondi
        $timeWithoutSeconds = substr($row['time'], 0, 5);
        $datetime = $row['date'] . ' ' . $timeWithoutSeconds; 
        $out[] = [
            'datetime' => $datetime,
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

// Pagina HTML (interfaccia tabella)
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Tabella Dati Sensore: <?= $sensorNameFromUrl ?: 'N/A' ?></title>

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
    
    #start, #end {
        width: auto; 
        min-width: 100px; 
        flex-grow: 0;      
        flex-shrink: 1;    
        flex-basis: auto;  
        font-size: 12px !important; 
    }

    .controls button {
        flex-grow: 0;
        flex-shrink: 0; 
        min-width: unset;
    }

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

    #table-container { 
        width: 100%; 
        margin-top: 8px; 
        overflow-x: auto;
    }
    
    table { 
        width: 100%; 
        border-collapse: collapse; 
        font-size: 12px;
    }
    
    table thead {
        background-color: #f0f0f0;
        font-weight: bold;
    }
    
    table th, table td {
        border: 1px solid #ddd;
        padding: 6px;
        text-align: left;
    }
    
    table tbody tr:nth-child(odd) {
        background-color: #f9f9f9;
    }
    
    table tbody tr:hover {
        background-color: #f0f0f0;
    }
    
    .no-data {
        padding: 20px;
        text-align: center;
        color: #666;
    }
    
    #sensor-control {
        display: none;
    }
</style>

<script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.29.4/moment.min.js"></script>
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

    <div id="table-container">
        <table id="dataTable">
            <thead>
                <tr>
                    <th>Data/Ora</th>
                    <th>Temperatura (°C)</th>
                    <th>Umidità (%)</th>
                    <th>Batteria</th>
                    <th>RSSI</th>
                </tr>
            </thead>
            <tbody id="tableBody">
            </tbody>
        </table>
        <div id="noDataMessage" class="no-data"></div>
    </div>

    <script>
        // Formato utilizzato per l'input datetime-local (Moment.js format string)
        const INPUT_DATETIME_FORMAT = "YYYY-MM-DDTHH:mm"; 

        // Helper: converte un oggetto Date o Moment in una stringa adatta all'input datetime-local
        function dateToInput(d){
            return moment(d).format(INPUT_DATETIME_FORMAT);
        }

        /**
         * Logica principale per caricare i dati e popolare la tabella.
         */
        async function loadTable(){
            const sensorInput = document.getElementById('sensorName');
            const sensor = sensorInput ? sensorInput.value : '';
            
            const tableBody = document.getElementById('tableBody');
            const noDataMessage = document.getElementById('noDataMessage');
            
            if (!sensor) { 
                tableBody.innerHTML = '';
                noDataMessage.innerHTML = 'Errore: Nome del sensore non specificato nella URL.';
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

                tableBody.innerHTML = '';
                noDataMessage.innerHTML = '';

                if (json.length === 0) {
                    noDataMessage.innerHTML = `Nessun dato trovato per il sensore "${sensor}" nell'intervallo selezionato.`;
                    return;
                }

                // Popola la tabella con i dati
                json.forEach(row => {
                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td>${row.datetime}</td>
                        <td>${row.temperature !== null ? parseFloat(row.temperature).toFixed(1) : '-'}</td>
                        <td>${row.humidity !== null ? Math.round(row.humidity) : '-'}</td>
                        <td>${row.battery !== null ? row.battery : '-'}</td>
                        <td>${row.RSSI !== null ? row.RSSI : '-'}</td>
                    `;
                    tableBody.appendChild(tr);
                });
            } catch (error) {
                console.error("Errore nel recupero o nel parsing dei dati:", error);
                tableBody.innerHTML = '';
                noDataMessage.innerHTML = 'Errore nel caricamento dei dati. Controlla il server PHP.';
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
            loadTable();
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
            loadTable();
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
            
            loadTable();
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
            loadTable(); 
        });
    </script>
</body>
</html>
