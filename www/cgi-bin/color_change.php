#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

$colors = ["#FF5733", "#33FF57", "#3357FF"];
$randomColor = $colors[array_rand($colors)];

echo "<!DOCTYPE html>";
echo "<html>";
echo "<head>";
echo "<title>Color Changer</title>";
echo "<style>";
echo "body {display:flex;justify-content:center;align-items:center;height:100vh;margin:0;background-color:$randomColor;}";
echo "</style>";
echo "</head>";
echo "<body>";
echo "</form>";
echo "</body>";
echo "</html>";
?>