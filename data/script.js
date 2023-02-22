var gateway = `ws://${window.location.hostname}:8081/ws`;
var websocket;
var nIntervId;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateSliderPWM(element) {
    var sliderNumber = element.id.charAt(element.id.length-1);
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById("sliderValue"+sliderNumber).innerHTML = sliderValue;
    
    websocket.send(sliderNumber+"s"+sliderValue.toString());
}

function updateInputDistance(element) {
    var sensorValue = document.getElementById(element.id).value;
    document.getElementById(element.id).innerHTML = sensorValue;
    websocket.send("WD");
}

function onMessage(event) {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    if(document.getElementById("waterLevel").id != "waterLevel")
    {
        document.getElementById("slider1").innerHTML = myObj["sliderValue1"];
        document.getElementById("slider1").value = myObj["sliderValue1"];
        
    }
    else 
    {
        document.getElementById("waterLevel").innerHTML = myObj["WaterDistance"];
        document.getElementById("waterLevel").value = myObj["WaterDistance"];
    }
}

var interval = setInterval(function(){
    updateInputDistance(document.getElementById("waterLevel"))
},60000)