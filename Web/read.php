<?php
//echo htmlspecialchars($_GET["client"]);
//echo "<br>";

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

$sql = "SELECT t1.raum, t1.temp FROM temperaturen t1, (SELECT raum, max(zeit) zeit from temperaturen t2 where kunde = '" . htmlspecialchars($_GET["client"]) . "' group by raum) as t2 WHERE kunde = '" . htmlspecialchars($_GET["client"]) . "' and t1.zeit = t2.zeit and t1.raum = t2.raum and t1.zeit >= NOW() - INTERVAL 12 HOUR ORDER BY t1.raum";

$result = $conn->query($sql);

if ($result->num_rows > 0) {
    // output data of each row
	echo "<START>";
    while($row = $result->fetch_assoc()) {
        echo "" . $row["raum"]. "\t" . $row["temp"]. "\n";
    }
} else {
    echo "<br>0 results";
}
	echo "<END>\t\n";

}

$conn->close();

?>