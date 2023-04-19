const swtWaterPump = document.getElementById("swtToggleWaterPump")
const lvlIndicator = document.getElementById("divLevel")
const lblWaterDistance = document.getElementById("lblWaterDistance")
const lblLevelPercent = document.getElementById("lblLevelPercent")


var gateway = `ws://${window.location.hostname}:8081/ws`;
var WaterDistanceCM;
var LevelPrcnt;
var WaterPumpPowerOn
var WaterPumpRest = false
var AutoEnable

window.addEventListener('load', onload);

const ToogleWaterPump = () => {
    if(!WaterPumpPowerOn && !WaterPumpRest) {
        websocket.send("turnOn")
    }

    if(WaterPumpPowerOn || WaterDistanceCM <= 20){
        setTimeout(() => {
            WaterPumpRest = true
            websocket.send("turnOff")
        },5000)
    }

    if(!WaterPumpPowerOn && WaterDistanceCM >= 20){
        setTimeout(() => {
            websocket.send("turnOn")
        },5000)
    }
    
}

setInterval(() => {
    getValues()

    if(WaterDistanceCM <= 20 && WaterPumpPowerOn){
        websocket.send("turnOff")
        WaterPumpRest = false
        document.getElementById("swtToggleWaterPump").disabled = true
        document.getElementById("swtAutoEnable").disabled = true

    }

    if(AutoEnable){
        ToogleWaterPump()
    }
},1500)

document.getElementById("swtAutoEnable").addEventListener("change",(e) => {
    if(e.currentTarget.checked){
        AutoEnable = true

        ToogleWaterPump()
    }
    else
        AutoEnable = false
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

    document.getElementById("lblWaterDistance").innerText = myObj["WaterDistance"]
    document.getElementById("lblLevelPercent").innerText = (100 - myObj["LevelPercent"]) + "%"
    document.getElementById("divLevel").style.height = (100 - myObj["LevelPercent"]) + "%"
    document.getElementById("swtToggleWaterPump").checked = myObj["WaterPumpState"] === "ON" ? true : false
    document.getElementById("lblWaterPumpState").innerText = myObj["WaterPumpState"] === "ON" ? "Encendida" : "Apagada"

    WaterDistanceCM = myObj["WaterDistance"]
    WaterPumpPowerOn = myObj["WaterPumpState"] === "ON" ? true : false

    if(myObj["WaterDistance"] >= 20)
        document.getElementById("swtToggleWaterPump").disabled = false
    else
        document.getElementById("swtToggleWaterPump").disabled = true
}