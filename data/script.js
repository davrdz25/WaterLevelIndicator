var gateway = `ws://${window.location.hostname}:8081/ws`;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function getValues() {
    websocket.send("getValues");
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

    if(myObj["FullTank"]){
        document.getElementById("swtAutoEnable").disabled = true
        document.getElementById("swtAutoEnable").checked = false

        document.getElementById("swtToggleWaterPump").disabled = true
        document.getElementById("swtToggleWaterPump").checked = false

        var sliders = document.querySelectorAll(".slider")

        sliders.forEach(slider => {
            slider.style.backgroundColor = "#f00"
        });
    } else {
        document.getElementById("swtAutoEnable").disabled = false
        document.getElementById("swtToggleWaterPump").disabled = false

        var sliders = document.querySelectorAll(".slider")

        sliders.forEach(slider => {
            slider.style.backgroundColor = "#ccc"
        }); 
    }

    document.getElementById("lblWaterDistance").innerText = myObj["WaterDistance"]
    document.getElementById("lblLevelPercent").innerText = (100 - myObj["LevelPercent"]) + "%"
    document.getElementById("divLevel").style.height = (100 - myObj["LevelPercent"]) + "%"

    if (myObj["WaterPumpState"] === 1){
        document.getElementById("lblWaterPumpState").innerText = "Encendida"
        document.getElementById("swtToggleWaterPump").checked = true
    }

    if (myObj["WaterPumpState"] === 0 || myObj["WaterPumpState"] === undefined){
        document.getElementById("lblWaterPumpState").innerText = "Apagada"
        document.getElementById("swtToggleWaterPump").checked = false
    }

    if (myObj["WaterPumpState"] === -1)
        document.getElementById("lblWaterPumpState").innerText = "Suspendida"
 

    if(myObj["autoEnabled"]){
        document.querySelector(".slider").style.backgroundColor = myObj["WaterPumpState"] == 1 ? "#0f0" : myObj["WaterPumpState"] == -1 ? "#ff0" : "#ccc"
        document.getElementById("spnAutoEnable").style.backgroundColor = "#2196F3"
        document.getElementById("swtToggleWaterPump").disabled = true
    } else {
        document.querySelector(".slider").style.backgroundColor = myObj["WaterPumpState"] === 1 ? "#0f0" : myObj["FullTank"] ? "#f00" : "#ccc"
        document.getElementById("swtToggleWaterPump").disabled = myObj["FullTank"] ? true : false   
    }

    document.getElementById("lblAutoEnable").innerText = myObj["autoEnabled"] ? "Activado" : "Apagado/Encendido manual de la bomba"
    document.getElementById("swtAutoEnable").checked = myObj["autoEnabled"]
}


setInterval(() => {
    websocket.send("getValues")
},2000)

document.getElementById("swtAutoEnable").addEventListener("change", (e) => {
    if(e.currentTarget.checked){
        document.getElementById("swtToggleWaterPump").disabled = true;
        websocket.send("autoEnabled")
    }

    if(!e.currentTarget.checked){
        document.getElementById("swtToggleWaterPump").disabled = false;
        websocket.send("autoDisabled")
    }
})

document.getElementById("swtToggleWaterPump").addEventListener("change", (e) => {
    if (e.currentTarget.checked) {
        websocket.send("turnOnPump")
    }

    if (!e.currentTarget.checked) {
        websocket.send("turnOffPump")
    }
})