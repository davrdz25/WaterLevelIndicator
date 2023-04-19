const swtWaterPump = document.getElementById("swtToggleWaterPump")
const lvlIndicator = document.getElementById("divLevel")
const lblWaterDistance = document.getElementById("lblWaterDistance")
const lblLevelPercent = document.getElementById("lblLevelPercent")

var gateway = `ws://${window.location.hostname}:8081/ws`;
var websocket;
var nIntervId;
var waterPumpTurnedOn
var elapsedTime
var remaingTime
var waterDistanceCM
var LevelPercent

window.addEventListener('load', onload);

setInterval(() => {
    getValues()
},10000)

swtWaterPump.addEventListener("change", (e) => {
    if(e.currentTarget.checked){
        websocket.send("turnOn")
    }

    if(!e.currentTarget.checked){
        websocket.send("turnOff");
    }
})

function onload(event) {
    initWebSocket();
}

function getValues() {
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

function onMessage(event) {
    var myObj = JSON.parse(event.data);

    document.getElementById("lblWaterDistance").innerText = myObj["WaterDistance"]
    document.getElementById("lblLevelPercent").innerText = myObj["LevelPercent"] + "%"
    document.getElementById("divLevel").style.height = myObj["LevelPercent"] + "%"
    document.getElementById("swtToggleWaterPump").checked = myObj["WaterPumpState"] === "ON" ? true : false
    document.getElementById("lblWaterPumpState").innerText = myObj["WaterPumpState"] === "ON" ? "Encendida" : "Apagada"
}