<!DOCTYPE HTML>
<html>
  <head>
    <title>Celsius</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <link rel="icon" href="data:,">
    <style>
      html {font-family: Arial; display: inline-block; text-align: center;}
      p {font-size: 1.2rem;}
      h4 {font-size: 0.8rem;}
      body {margin: 0;}
      .topnav {overflow: hidden; background-color: #0c6980; color: white; font-size: 1rem; padding: 6px 10px;}
      .topnav h3 {margin: 0; font-size: 1rem; line-height: 1;}
      .topnav {overflow: hidden; background-color: #0c6980; color: white; font-size: 1rem; padding: 4px 8px; margin: 0 0 0 0;}
      .topnav h3 {margin: 0; font-size: 1rem; line-height: 1;}
      .content {padding: 2px 6px; }
      .card {background-color: white; box-shadow: 0px 0px 10px 1px rgba(140,140,140,.5); border: 1px solid #0c6980; border-radius: 15px; padding: 8px 10px;}
      .card.header {background-color: #1d0c80ff; color: white; border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; border-top-right-radius: 12px; border-top-left-radius: 12px; padding: 6px 8px; box-sizing: border-box; margin: -8px -10px 6px -10px;}
      .card.header2 {background-color: rgb(128, 12, 41); color: white; border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; border-top-right-radius: 12px; border-top-left-radius: 12px; padding: 6px 8px; box-sizing: border-box; margin: -8px -10px 6px -10px;}
      .card.header h3 {margin: 0; font-size: 0.95rem; line-height: 1;}
      .cards {max-width: 900px; margin: 0 auto; display: grid; grid-gap: 0.6rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}
      .card h3, .card h4, .card p {margin: 4px 0;}
      .reading {font-size: 1.3rem;}
      .packet {color: #bebebe;}
      .temperatureColor {color: #fd7e14;}
      .humidityColor {color: #1b78e2;}
      .pressureColor {color: #05a11aff;}
      /* ----------------------------------- */
    </style>
  </head>
  
  <body>
    <div class="topnav">
      <h3>celsius</h3>
    </div>
    <!-- __ DISPLAYS MONITORING AND CONTROLLING ____________________________________________________________________________________________ -->
    <div class="content">
      <div class="cards">
        <div class="card">
          <div class="card header">
            <h3 style="font-size: 1rem;">notte</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_01_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_01_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_01_LTRD"></span><span> batt: </span><span id="ESP_01_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT01')">Hist</button>
          <button onclick="OpenChart('SWBT01')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->
        <div class="card">
          <div class="card header">
            <h3 style="font-size: 1rem;">soggiorno</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_02_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_02_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_02_LTRD"></span><span> batt: </span><span id="ESP_02_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT02')">Hist</button>
          <button onclick="OpenChart('SWBT02')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->
        <div class="card">
          <div class="card header">
            <h3 style="font-size: 1rem;">tavernetta</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_03_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_03_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_03_LTRD"></span><span> batt: </span><span id="ESP_03_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT03')">Hist</button>
          <button onclick="OpenChart('SWBT03')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->        
        <div class="card">
          <div class="card header">
            <h3 style="font-size: 1rem;">giardino</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_04_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_04_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_04_LTRD"></span><span> batt: </span><span id="ESP_04_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT04')">Hist</button>
          <button onclick="OpenChart('SWBT04')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->
        <div class="card">
          <div class="card header2">
            <h3 style="font-size: 1rem;">termo anteriore</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_05_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_05_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_05_LTRD"></span><span> batt: </span><span id="ESP_05_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT05')">Hist</button>
          <button onclick="OpenChart('SWBT05')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->
        <div class="card">
          <div class="card header2">
            <h3 style="font-size: 1rem;">termo posteriore</h3>
          </div>
          <div style="display: flex; justify-content: space-around; align-items: center; margin-bottom: 10px;">
            <div style="text-align: center;">
              <h4 class="temperatureColor" style="margin-bottom: 0;"><i class="fas fa-thermometer-half"></i> Temp</h4>
              <p class="temperatureColor" style="margin-top: 0;"><span class="reading"><span id="ESP_06_Temp"></span> &deg;C</span></p>
            </div>
            <div style="text-align: center;">
              <h4 class="humidityColor" style="margin-bottom: 0;"><i class="fas fa-tint"></i> Humd</h4>
              <p class="humidityColor" style="margin-top: 0;"><span class="reading"><span id="ESP_06_Humd"></span> &percnt;</span></p>
            </div>
          </div>
          <h3 style="font-size: 0.7rem;"><span id="ESP_06_LTRD"></span><span> batt: </span><span id="ESP_06_Status"></span></h3>
          <button onclick="OpenRecordTable('SWBT06')">Hist</button>
          <button onclick="OpenChart('SWBT06')">Chart</button>
          <h3 style="font-size: 0.7rem;"></h3>
        </div>
        <!-- ======================================================================================================= -->
      </div>
    </div>
    <br>
    <div class="content">
      <div class="cards">
        <div class="card header">
          <h3 style="font-size: .5rem;">@ V. P.</h3>
        </div>
      </div>
    </div>
    <!-- ___________________________________________________________________________________________________________________________________ -->
    <script>
      //------------------------------------------------------------
      document.getElementById("ESP_01_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_01_Humd").innerHTML = "NN";
      document.getElementById("ESP_01_Status").innerHTML = "NN";
      document.getElementById("ESP_01_LTRD").innerHTML = "NN";
      document.getElementById("ESP_02_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_02_Humd").innerHTML = "NN";
      document.getElementById("ESP_02_Status").innerHTML = "NN";
      document.getElementById("ESP_02_LTRD").innerHTML = "NN";
      document.getElementById("ESP_03_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_03_Humd").innerHTML = "NN";
      document.getElementById("ESP_03_Status").innerHTML = "NN";
      document.getElementById("ESP_03_LTRD").innerHTML = "NN";
      document.getElementById("ESP_04_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_04_Humd").innerHTML = "NN";
      document.getElementById("ESP_04_Status").innerHTML = "NN";
      document.getElementById("ESP_04_LTRD").innerHTML = "NN";      
      document.getElementById("ESP_05_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_05_Humd").innerHTML = "NN";
      document.getElementById("ESP_05_Status").innerHTML = "NN";
      document.getElementById("ESP_05_LTRD").innerHTML = "NN";      
      document.getElementById("ESP_06_Temp").innerHTML = "NN"; 
      document.getElementById("ESP_06_Humd").innerHTML = "NN";
      document.getElementById("ESP_06_Status").innerHTML = "NN";
      document.getElementById("ESP_06_LTRD").innerHTML = "NN";      
      //------------------------------------------------------------
      Get_Data("SWBT01");
      Get_Data("SWBT02");
      Get_Data("SWBT03");
      Get_Data("SWBT04");
      Get_Data("SWBT05");
      Get_Data("SWBT06");
      setInterval(myTimer, 60000);
      //------------------------------------------------------------
      function myTimer() {
        Get_Data("SWBT01");
        Get_Data("SWBT02");
        Get_Data("SWBT03");
        Get_Data("SWBT04");
        Get_Data("SWBT05");
        Get_Data("SWBT06");
      }
      //------------------------------------------------------------
      function OpenRecordTable(id) {
        console.log("OpenRecordTable: ", id);
        window.open('table.php?sensorName=' + id );
      }
      //------------------------------------------------------------
      function OpenChart(sensor) {
        function pad(n){ return n<10? '0'+n : n }
        const end = new Date();
        const start = new Date(end.getTime() - 24*60*60*1000);
        const toInput = (d) => d.getFullYear() + '-' + pad(d.getMonth()+1) + '-' + pad(d.getDate()) + 'T' + pad(d.getHours()) + ':' + pad(d.getMinutes());
        const startIso = toInput(start);
        const endIso = toInput(end);
        const url = 'chart.php?sensorName=' + encodeURIComponent(sensor) + '&start=' + encodeURIComponent(startIso) + '&end=' + encodeURIComponent(endIso);
        window.open(url);
      }
      //------------------------------------------------------------
      function Get_Data(id) {
				if (window.XMLHttpRequest) {
          // code for IE7+, Firefox, Chrome, Opera, Safari
          xmlhttp = new XMLHttpRequest();
        } else {
          // code for IE6, IE5
          xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
        }
          console.log("Get_Data: ", id);
          xmlhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            const myObj = JSON.parse(this.responseText);
            if (myObj.sensorName == "SWBT01") {
              document.getElementById("ESP_01_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_01_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_01_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_01_LTRD").innerHTML = myObj.ls_date + " " + myObj.ls_time;
            }
            if (myObj.sensorName == "SWBT02") {
              document.getElementById("ESP_02_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_02_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_02_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_02_LTRD").innerHTML =  myObj.ls_date + " " + myObj.ls_time;
            }
            if (myObj.sensorName == "SWBT03") {
              document.getElementById("ESP_03_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_03_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_03_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_03_LTRD").innerHTML =  myObj.ls_date + " " + myObj.ls_time;
            }
            if (myObj.sensorName == "SWBT04") {
              document.getElementById("ESP_04_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_04_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_04_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_04_LTRD").innerHTML =  myObj.ls_date + " " + myObj.ls_time;
            }
            if (myObj.sensorName == "SWBT05") {
              document.getElementById("ESP_05_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_05_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_05_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_05_LTRD").innerHTML =  myObj.ls_date + " " + myObj.ls_time;
            }
            if (myObj.sensorName == "SWBT06") {
              document.getElementById("ESP_06_Temp").innerHTML = myObj.temperature;
              document.getElementById("ESP_06_Humd").innerHTML = myObj.humidity;
              document.getElementById("ESP_06_Status").innerHTML = myObj.battery;
              document.getElementById("ESP_06_LTRD").innerHTML =  myObj.ls_date + " " + myObj.ls_time;
            }
          }
        };
        xmlhttp.open("POST","getdata.php",true);
        xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        xmlhttp.send("sensorName="+id);
			}
      //------------------------------------------------------------
    </script>
  </body>
</html>