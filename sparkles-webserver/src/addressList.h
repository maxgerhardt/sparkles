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
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); display: block; margin-bottom: 20px; flex: 1 0 100%; padding:5px;}
    .card span {display:block; }
    .cards { max-width: 700px; margin: 0 auto; display:flex; flex-flow: row wrap; gap:0.5em; flex-wrap: wrap; }
    .card--50 {flex-basis: 40%}
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .devicenum { color: #fd7e14; }
    .active { background-color: rgba(0, 139, 0, 0.2); }
    .inactive { background-color: rgba(255, 0, 0, 0.2) }
    .card.humidity { color: #1b78e2; }
    .card.deviceList { color: #010702; }

  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
        <div class="card deviceList">
            <h4>
                <i class="fas fa-wifi"></i> 
                DEVICE LIST
            </h4>
            <p>
                <span class="reading">
                    <span id="t1" class="devicenum">0</span>
                </span>
            </p>
          </div>
          <div class="card deviceList">
            <h4>
                <i class="fas fa-bell"></i> 
                STATUS
            </h4>
            <p>
                <span class="reading">
                    <span id="status" class="devicenum">0</span>
                </span>
            </p>
          </div>

          
        <div class="card card--50">
          <p>
              <span class="reading">
                  <span id="cmd_calibrate" class="devicenum">CALIBRATE</span>
              </span>
          </p>
        </div>
        <div class="card card--50">
          <p>
              <span class="reading">
                  <span id="cmd_animate" class="devicenum">ANIMATE</span>
              </span>
          </p>
        </div>
      </div>

      </div>
    </div>

  </div>
<script>



  var debug = 0;
  function boardCards(obj) {
    console.log(obj.index);
    var existingPacket = document.getElementById("boardCard" + obj.index);
  if (!existingPacket) {
    var newCard = document.createElement("div");
    newCard.className = "card";
    newCard.id = "boardCard" + obj.index;
    newCard.innerHTML = "<span id='b"+obj.index+"'>Board ID "+obj.index+":</span>\n<span id='addr" + obj.index + "'></span><span id='del"+obj.index+"'></span>";
    var cardsContainer = document.querySelector(".cards");
    cardsContainer.appendChild(newCard);
    document.getElementById('boardCard'+obj.index).addEventListener('click', function() {
        handleUpdateDeviceClick(String(obj.index));
    });

    //hier fehlt noch irgendwie die klarheit wo eigentlich der eventlistener hin muss. 
  }
  document.getElementById("addr" + obj.index).textContent = "Address: "+obj.address;
  if (obj.delay == 0) {
    document.getElementById("boardCard"+obj.index).classList.remove("active");
    document.getElementById("boardCard"+obj.index).classList.add("inactive");
  }
  else {
    document.getElementById("boardCard"+obj.index).classList.add("active");
    document.getElementById("boardCard"+obj.index).classList.remove("inactive");
  }
  
  document.getElementById("del"+obj.index).textContent = "Delay: "+obj.delay;
  document.getElementById("t1").textContent = String(parseInt(obj.index)+1);

    document.getElementById('status').textContent = obj.status;

  };
  function statusUpdate(obj) {
    document.getElementById('status').textContent = obj.status;
  }
  function calibrateUpdate(obj) {
    if (obj.status == "true") {
      document.getElementById('cmd_calibrate').textContent = "END CALIBRATION";
    }
    else {
      document.getElementById('cmd_calibrate').textContent = "CALIBRATE";
    }
  }
  if (debug == 0) {
    if (!!window.EventSource) {
    var sourceEvents = new EventSource('/events');
    console.log("ok worked");
    sourceEvents.addEventListener('open', function(e) {
      console.log("Events Connected");
    }, false);
    sourceEvents.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);
    sourceEvents.addEventListener('message', function(e) {

      console.log("message", e.data);
    }, false);
    
    sourceEvents.addEventListener('new_readings', function(e) {
      var obj = JSON.parse(e.data);
      boardCards(obj);
      
    })
    sourceEvents.addEventListener('new_status', function(e) {
      var obj = JSON.parse(e.data);
      statusUpdate(obj);
    })
    sourceEvents.addEventListener('calibrateStatus', function(e) {
      console.log("calibratestatus");
      console.log(e);
      var obj = JSON.parse(e.data);
      console.log(obj.status)
      calibrateUpdate(obj);
    })
};
  }

else if (debug == 1) {
  var boards = [];
   boards[0] = {
    index : 0,
    address: "68:b6:b3:08:e0:60",
    delay : 1001,
    status: "MODE_WAIT_FOR_TIMER"
  };
  boards[1] = {
    index : 1,
    address: "78:b6:b3:08:e0:66",
    delay : 0,
    status: "MODE_WAIT_FOR_TIMER"
  };

  boards.forEach(function(obj) {
  boardCards(obj);
  });
  updatedStatus = {
    statusId : 1,
    status : "WAIT_FOR_TIMER"
  };
  statusUpdate(updatedStatus);
  
}

function sendDeviceUpdateRequest(id) {
  console.log("ok worked");
    // Make a GET request to the /events endpoint using fetch
    if (id != -1) {
      var fetchUrl = '/updateDeviceList?id='+id;
    }
    else {
      var fetchUrl = '/updateDeviceList';
    }
  console.log(fetchUrl);
  console.log(id);

  fetch(fetchUrl)
        .then(response => {
            // Check if the response is successful
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            // Parse the response as JSON
            console.log("response ok");
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
function fetchMe(fetchUrl) {

    fetch(fetchUrl)
        .then(response => {
            // Check if the response is successful
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            // Parse the response as JSON
            console.log("response ok");
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
function handleUpdateDeviceClick(id) {
  console.log("clicked");
    sendDeviceUpdateRequest(id); // Trigger HTTP request when span is clicked
}

function handleCommandCalibrateClick() {
  console.log("calibrate!");
  var fetchUrl = '/commandCalibrate';
  fetchMe(fetchUrl);
}


// Add an event listener to the t1 span element
document.getElementById('t1').addEventListener('click', function() {handleUpdateDeviceClick(-1)});
document.getElementById('cmd_calibrate').addEventListener('click', handleCommandCalibrateClick);
document.addEventListener('DOMContentLoaded', function() {handleUpdateDeviceClick(-1)});</script>
</body>
</html>
)rawliteral";
