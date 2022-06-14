<?php

ob_start();
session_set_cookie_params(0, "/", null, true, true);
error_reporting(E_ALL);

date_default_timezone_set('Europe/Paris');
setlocale(LC_TIME, "fr_FR");


    $bddAdresse = "localhost";
    $bddPort = "3306";
    $bddName = "mesure";

    $bddUser = "userdb";
    $bddPassword = "salut";



try{
    $bdd = new PDO('mysql:host='.$bddAdresse.';dbname='.$bddName.';port='.$bddPort, $bddUser, $bddPassword);
}catch(PDOException $ex){
    throw new ErrorException("Impossible de se connecter à la base de donné: ".$ex->getMessage(), 1);
    exit();
}
$begin = time();

$req = $bdd->prepare("SELECT * FROM table_info WHERE timestamp > :time ORDER BY timestamp DESC");
$req->execute(array("time"=>$begin-35));
$req = $req->fetchAll(PDO::FETCH_ASSOC);



if(!empty($req) && isset($req)) {

    $Atemperature = array();
    $Ahumidity = array();
    $Afailure = array();

    $first = true;
    $alarm = 0;
    for($i = 0; $i<=30; $i++) {
        $found = false;
        foreach($req as $value) {
            if($value["timestamp"] == $begin-$i) {
                if($first) {
                    $first = false;
                    $alarm = $value["alarm"];
                }
                $date = date("Y-m-d H:i:s", $value["timestamp"]);
                array_push($Atemperature, [$date, $value["temperature"]]);
                array_push($Ahumidity, [$date, $value["humidity"]]);
                array_push($Afailure, [$date, $value["failure"]]);
                $found = true;
                break;
            }
        }
        if(!$found) {
            $date = date("Y-m-d H:i:s", $begin-$i);
            array_push($Atemperature, [$date, -1]);
            array_push($Ahumidity, [$date, -1]);
            array_push($Afailure, [$date, -1]);
        }
    }

  
    $Return = array("temperature" => $Atemperature, "humidity" => $Ahumidity, "failure" => $Afailure, "alarm" => $alarm);
    echo '[DIV]';
    echo json_encode($Return);
} else {
    echo json_encode(array());
}
ob_end_flush();
