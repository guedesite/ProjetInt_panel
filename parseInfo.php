<?php

ob_start();
session_set_cookie_params(0, "/", null, true, true);
error_reporting(E_ALL);

date_default_timezone_set('Europe/Paris');
setlocale(LC_TIME, "fr_FR");


    $bddAdresse = "localhost";
    $bddPort = "3306";
    $bddName = "bdd_main";

    $bddUser = "root";
    $bddPassword = "root";



try{
    $bdd = new PDO('mysql:host='.$bddAdresse.';dbname='.$bddName.';port='.$bddPort, $bddUser, $bddPassword);
}catch(PDOException $ex){
    throw new ErrorException("Impossible de se connecter à la base de donné: ".$ex->getMessage(), 1);
    exit();
}

$req = $bdd->query("SELECT * FROM table_info ORDER BY time DESC LIMIT 30");
$req = $req->fetchAll(PDO::FETCH_ASSOC);

if(!empty($req) && isset($req)) {

    $Atemperature = array();
    $Ahumidity = array();
    $Afailure = array();

    foreach($req as $value) {
        $date = date("Y-m-d H:i:S", $value["timestamp"]);
        array_push($Atemperature, [$date, $value["temperature"]]);
        array_push($Ahumidity, [$date, $value["humidity"]]);
        array_push($Afailure, [$date, $value["failure"]]);
    }

    $Return = array("temperature" => $Atemperature, "humidity" => $Ahumidity, "Afailure" => $Afailure);
    echo '[DIV]';
    echo json_encode($Return);
} else {
    echo json_encode(array());
}
ob_end_flush();
