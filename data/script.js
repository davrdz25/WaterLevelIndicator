const swtWaterPump = document.getElementById("swtToggleWaterPump")
const lvlIndicator = document.getElementById("divLevel")
const lblWaterDistance = document.getElementById("lblWaterDistance")
const lblLevelPercent = document.getElementById("lblLevelPercent")


var gateway = `ws://${window.location.hostname}:8081/ws`;
var WaterDistanceCM;
var LevelPrcnt;
var WaterPumpPowerOn
var WaterPumpState
var AutoEnable

window.addEventListener('load', onload);

const ToogleWaterPump = () => {
    if(WaterPumpState == 1){
        setTimeout(() => {
            websocket.send("RestYes")
        },5000)
    }

    if(WaterPumpState == -1){
        setTimeout(() => {
            websocket.send("RestNo")
        },5000)
    }
}

setInterval(() => {
    getValues()

    if(AutoEnable)
        ToogleWaterPump()

},2000)

document.getElementById("swtAutoEnable").addEventListener("change",(e) => {
    if(e.currentTarget.checked){
        websocket.send("AutoON")
    }
    else
        websocket.send("AutoOFF")
})

document.getElementById("swtToggleWaterPump").addEventListener("change", (e) => {
    if(e.currentTarget.checked)
        websocket.send("turnOn")

    if(!e.currentTarget.checked)
        websocket.send("turnOff")
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

    console.log(myObj)
    document.getElementById("lblWaterDistance").innerText = myObj["WaterDistance"]
    document.getElementById("lblLevelPercent").innerText = (100 - myObj["LevelPercent"]) + "%"
    document.getElementById("divLevel").style.height = (100 - myObj["LevelPercent"]) + "%"
    document.getElementById("lblWaterPumpState").innerText = myObj["WaterPumpState"] === 1 ? "Encendida" : myObj["WaterPumpState"] === -1 ? "En reposo" : "Apagada"

    document.getElementById("swtToggleWaterPump").checked = myObj["WaterPumpState"] ==  1 ? true : false
    document.getElementById("swtAutoEnable").checked = myObj["AutoMode"] === "ON" ? true : false

    WaterDistanceCM = myObj["WaterDistance"]
    WaterPumpState = myObj["WaterPumpState"]
    AutoEnable = myObj["AutoMode"] === "ON" ? true : false
}