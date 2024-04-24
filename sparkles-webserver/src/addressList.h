#include <Arduino.h>
#pragma once

const char addressList[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .devicenum { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
        <div class="deviceList">
            <h4>
                <i class="fas fa-thermometer-half"></i> 
                DEVICE LIST
            </h4>
            <p>
                <span class="reading">
                    <span id="t1" class="devicenum"te></span>
                </span>
            </p>


        </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  var existingPacket = document.getElementById("pk" + obj.index);
  if (!existingPacket) {
    var newPacket = document.createElement("p");
    newPacket.className = "packet";
    newPacket.id = "pk" + obj.index;
    newPacket.innerHTML = "Board ID "+obj.index+": <span id='rt" + obj.index + "'></span>";
    var cardsContainer = document.querySelector(".deviceList");
    cardsContainer.appendChild(newPacket);
  }
  document.getElementById("rt" + obj.index).textContent = obj.address;
  document.getElementById("t1").textContent = String("Click me"+String(parseInt(obj.index)+1));

})};

function sendHttpRequest() {
    // Make a GET request to the /events endpoint using fetch
    fetch('/updateDeviceList')
        .then(response => {
            // Check if the response is successful
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            // Parse the response as JSON
            return response.json();
        })
        .then(data => {
            // Handle the received data
            console.log('Data received:', data);
        })
        .catch(error => {
            // Handle errors
            console.error('Fetch error:', error);
        });
}


// Function to handle click on the t1 span element
function handleSpanClick() {
  console.log("clicked");
    sendHttpRequest(); // Trigger HTTP request when span is clicked
}

// Add an event listener to the t1 span element
document.getElementById('t1').addEventListener('click', handleSpanClick);

</script>
</body>
</html>
)rawliteral";
