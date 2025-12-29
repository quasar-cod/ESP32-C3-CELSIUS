<?php
  require 'database.php';
    //........................................ Get the time and date.
    date_default_timezone_set("Europe/Rome"); // Look here for your timezone : https://www.php.net/manual/en/timezones.php
    $tm = date("H:i:s");
    $dt = date("Y-m-d");     
  //---------------------------------------- Condition to check that POST value is not empty.
  if (!empty($_POST)) {
    $board = $_POST['board'];
    $sensorName = $_POST['sensorName'];
    $temperature = $_POST['temperature'];
    $humidity = $_POST['humidity'];
    $battery = $_POST['battery'];
    $RSSI = $_POST['RSSI'];
    $tm = $_POST['time'];
    $dt = $_POST['date'];
   //........................................ Entering data into a table.
    $pdo = Database::connect();
    //:::::::: The process of entering data into a table.
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
		$sql = "INSERT INTO esp32_record (board,sensorName,temperature,humidity,battery,RSSI,time,date) values(?, ?, ?, ?, ?, ?, ?, ?)";
		$q = $pdo->prepare($sql);
		$q->execute(array($board,$sensorName,$temperature,$humidity,$battery,$RSSI,$tm,$dt));
    //::::::::
    $myObj = (object)array();
    $myObj->board = $board;
    $myObj->sensorName = $sensorName;
    $myObj->temperature = $temperature;
    $myObj->humidity = $humidity;
    $myObj->battery = $battery;
    $myObj->RSSI = $RSSI;
    $myObj->ls_time = $tm;
    $myObj->ls_date = $dt;
    $myJSON = json_encode($myObj);  
    echo $myJSON;
    Database::disconnect();
    //........................................ 
  }else{
    $myObj = (object)array();
    $myObj->sql = $sql;
    $myJSON = json_encode($myObj);  
    echo $myJSON;
  }
?>