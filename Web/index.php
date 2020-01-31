<html>
<?php

$servername = "";
$username = "";
$password = "";
$dbname = "";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if (!isset($_GET["client"])) {

	echo "Bitte Ort ausw&auml;hlen: <br>";
	
	$sql = "SELECT distinct kunde FROM temperaturen";
	$result = $conn->query($sql);
	
	while($row = $result->fetch_assoc()) {
		echo "<p><a href='?client=". $row[kunde] ."&time=24'>$row[kunde]</a></p>";
	}
	exit;
} else {
		echo "<p><a href='index.php'>Startseite</a></p><br />";
}

if (!isset($_GET["time"])) {
	$sql = "SELECT timestamp(zeit) as timestamp, zeit, kunde, raum, temp, feuchtigkeit FROM temperaturen where kunde = '".$_GET["client"]."' ORDER BY zeit";
} else {
	$hours = $_GET["time"];
	$sql = "SELECT timestamp(zeit) as timestamp, zeit, kunde, raum, temp, feuchtigkeit FROM temperaturen where kunde = '".$_GET["client"]."' and zeit >= NOW() - INTERVAL ". $hours." HOUR and raum != 'Serverschrank'  ORDER BY zeit";
}

$result = $conn->query($sql);

$TempDataRaw;

if ($result->num_rows > 0) {
    // output data of each row
    while($row = $result->fetch_assoc()) {
		$TempDataRaw[$row["raum"]][] = array(
								"x" => $row["timestamp"],
								"y" => $row["temp"]
								);
								
		$HumidityDataRaw[$row["raum"]][] = array(
								"x" => $row["timestamp"],
								"y" => $row["feuchtigkeit"]
								);	
		//echo "<br>zeit: " . $row["zeit"]. " - kunde: " . $row["kunde"]. " " . " - raum: " . $row["raum"]. " " ." - temp/feuchtigkeit: " . $row["temp"]. " / " .$row["feuchtigkeit"]. "";
    }
} else {
    echo "<br>0 results";
	exit;
}



$jsData;
$keys = array_keys($TempDataRaw);

//var_dump($keys);

for ($i=0;$i<sizeof($TempDataRaw);$i++) {
	$jsData[] = array(name => $keys[$i],
					 data => $TempDataRaw[$keys[$i]]
					 );
}

$jsHumidityData;
$keys = array_keys($HumidityDataRaw);

//var_dump($keys);

for ($i=0;$i<sizeof($HumidityDataRaw);$i++) {
	$jsHumidityData[] = array(name => $keys[$i],
					 data => $HumidityDataRaw[$keys[$i]]
					 );
}

$conn->close();

?>

<script>
    var temperaturen = <?php echo json_encode($jsData);?>;
	var feuchtigkeit = <?php echo json_encode($jsHumidityData);?>;
</script>


<a href="<?php echo "?client=". $_GET[client] .""  ?>">
	<button>ALLE
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=168"  ?>">
	<button>7T
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=120"  ?>">
	<button>5T
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=48"  ?>">
	<button>2T
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=24"  ?>">
	<button>1T
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=12"  ?>">
	<button>12S
	</button>
</a>
<a href="<?php echo "?client=". $_GET[client] ."&time=2"  ?>">
	<button>2S
	</button>
</a>


<div id="chart">
</div>

<div id="chart_humidity">
</div>

<script src="apexcharts.min.js"></script> <!-- Download @ https://apexcharts.com/ -->
<script src="chart.js"></script>
<script src="chart_humidity.js"></script>

</html>

