<?php
echo htmlspecialchars($_GET["client"]);
echo "<br>";
echo htmlspecialchars($_GET["room"]);
echo "<br>";
echo htmlspecialchars($_GET["temp"]);
echo "<br>";
echo htmlspecialchars($_GET["humidity"]);
echo "<br>"; 

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


if (isset($_GET["client"])) {

	echo "<br>Hier nun SQL:<br>";

	$sql = "INSERT INTO temperaturen (kunde, raum, temp, feuchtigkeit) VALUES ('" . htmlspecialchars($_GET["client"]) . "', '" . trim(htmlspecialchars($_GET["room"])) . "', '" . htmlspecialchars($_GET["temp"]) . "', '" . htmlspecialchars($_GET["humidity"]) . "')";

	echo $sql;

	echo "<br>HIER SQL ENDE<br>";

	if ($conn->query($sql) === TRUE) {
		echo "New record created successfully";
	} else {
		echo "Error: " . $sql . "<br>" . $conn->error;
	}

}

$conn->close();

?>