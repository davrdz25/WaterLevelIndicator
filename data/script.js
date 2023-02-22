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
    console.log(event)

    if(document.getElementById("waterLevel").id != "waterLevel")
    {
        for (var i = 0; i < keys.length - 1; i++){
            var key = keys[i];
    
            
                document.getElementById(key).innerHTML = myObj[key];
                document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
        }
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