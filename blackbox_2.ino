#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <DHT.h>

// =====================================================
// DHT11
// =====================================================

#define DHT_PIN D5
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// =====================================================
// MQ135
// =====================================================

const int MQ135_PIN = A0;

const int NORMAL_LIMIT = 400;
const int MODERATE_LIMIT = 550;

// =====================================================
// TEMPERATURE DANGER LIMIT
// =====================================================

const float HIGH_TEMPERATURE = 40.0;

// =====================================================
// WIFI ACCESS POINT
// =====================================================

const char* AP_SSID = "Forest-Fire-BlackBox";
const char* AP_PASSWORD = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// =====================================================
// WEB SERVER
// =====================================================

ESP8266WebServer server(80);

DNSServer dnsServer;

const byte DNS_PORT = 53;

// =====================================================
// SENSOR VARIABLES
// =====================================================

// These hold the LAST VALID readings.

float temperature = 0.0;
float humidity = 0.0;

int mq135Value = 0;

String airStatus = "SAFE";
String overallStatus = "SAFE";

// Used to know whether DHT has produced a valid reading
bool dhtOK = false;

// =====================================================
// SENSOR TIMING
// =====================================================

unsigned long lastSensorRead = 0;

const unsigned long SENSOR_INTERVAL = 2000;

// DHT11 should not be read too quickly.

// =====================================================
// READ MQ135
// =====================================================

int readMQ135() {

  long total = 0;

  for (int i = 0; i < 10; i++) {

    total += analogRead(MQ135_PIN);

    delay(5);
  }

  return total / 10;
}

// =====================================================
// DETERMINE AIR QUALITY
// =====================================================

String getAirStatus(int value) {

  if (value < NORMAL_LIMIT) {

    return "SAFE";

  }

  else if (value < MODERATE_LIMIT) {

    return "MODERATE";

  }

  else {

    return "DANGER";
  }
}

// =====================================================
// UPDATE ALL SENSORS
// =====================================================

void updateSensors() {

  // -----------------------------------------------
  // MQ135
  // -----------------------------------------------

  mq135Value = readMQ135();

  airStatus = getAirStatus(mq135Value);


  // -----------------------------------------------
  // DHT11
  // -----------------------------------------------

  float newTemperature = dht.readTemperature();

  float newHumidity = dht.readHumidity();


  // -----------------------------------------------
  // Check DHT reading
  // -----------------------------------------------

  if (!isnan(newTemperature) &&
      !isnan(newHumidity)) {

    temperature = newTemperature;

    humidity = newHumidity;

    dhtOK = true;

  }

  else {

    // Keep the previous valid reading.

    dhtOK = false;

    Serial.println(
      "WARNING: DHT11 reading failed"
    );
  }


  // -----------------------------------------------
  // OVERALL FIRE DANGER
  // -----------------------------------------------

  bool highTemperature =
    temperature >= HIGH_TEMPERATURE;

  bool dangerousAir =
    mq135Value >= MODERATE_LIMIT;


  /*
     FIRE DANGER:

     Temperature HIGH
     AND
     MQ135 AIR DANGER
  */

  if (highTemperature &&
      dangerousAir) {

    overallStatus = "DANGER";

  }

  else {

    overallStatus = "SAFE";
  }


  // -----------------------------------------------
  // SERIAL MONITOR
  // -----------------------------------------------

  Serial.println();
  Serial.println("-----------------------------");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("MQ135: ");
  Serial.println(mq135Value);

  Serial.print("Air: ");
  Serial.println(airStatus);

  Serial.print("System: ");
  Serial.println(overallStatus);

  Serial.println("-----------------------------");
}

// =====================================================
// MAIN WEB PAGE
// =====================================================

void handleRoot() {

  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>Forest Fire Black Box</title>

<style>

body {

  margin: 0;

  font-family: Arial, sans-serif;

  background: #101820;

  color: white;

  text-align: center;
}


.header {

  padding: 22px;

  background: #17232c;
}


h1 {

  margin: 0;

  font-size: 25px;
}


.subtitle {

  margin-top: 7px;

  color: #aeb8bd;

  font-size: 14px;
}


.container {

  max-width: 500px;

  margin: auto;

  padding: 15px;
}


.card {

  background: #1d2b34;

  border-radius: 18px;

  padding: 25px;

  margin-top: 18px;

  box-shadow: 0 5px 20px rgba(0,0,0,0.3);
}


.label {

  color: #aeb8bd;

  font-size: 15px;

  margin-bottom: 8px;
}


.value {

  font-size: 42px;

  font-weight: bold;

  margin-bottom: 10px;
}


.air {

  font-size: 30px;

  font-weight: bold;

  margin-top: 10px;
}


.status {

  font-size: 28px;

  font-weight: bold;

  padding: 18px;

  border-radius: 15px;

  margin-top: 10px;
}


.safe {

  background: #167a3f;
}


.danger {

  background: #a52828;

  animation: dangerFlash 1s infinite;
}


.moderate {

  background: #9a7000;
}


@keyframes dangerFlash {

  0% {
    opacity: 1;
  }

  50% {
    opacity: 0.55;
  }

  100% {
    opacity: 1;
  }

}


.info {

  color: #aeb8bd;

  font-size: 14px;

  line-height: 1.6;
}


.small {

  font-size: 13px;

  color: #8e9aa0;

  margin-top: 10px;
}

</style>

</head>


<body>


<div class="header">

<h1>FOREST FIRE BLACK BOX</h1>

<div class="subtitle">

Environmental Emergency Monitoring System

</div>

</div>


<div class="container">


<!-- TEMPERATURE -->

<div class="card">

<div class="label">

TEMPERATURE

</div>

<div id="temperature"
class="value">

-- °C

</div>

</div>


<!-- HUMIDITY -->

<div class="card">

<div class="label">

HUMIDITY

</div>

<div id="humidity"
class="value">

-- %

</div>

</div>


<!-- AIR QUALITY -->

<div class="card">

<div class="label">

AIR QUALITY

</div>

<div id="air"
class="air">

SAFE

</div>

<div id="mqStatus"
class="small">

MQ135 monitoring active

</div>

</div>


<!-- OVERALL STATUS -->

<div class="card">

<div class="label">

EMERGENCY STATUS

</div>

<div id="systemStatus"
class="status safe">

SAFE

</div>

</div>


<!-- SYSTEM INFORMATION -->

<div class="card">

<div class="info">

Temperature danger threshold:
40 °C

<br><br>

MQ135:

<br>

SAFE &lt; 400

<br>

MODERATE: 400 - 549

<br>

DANGER: 550+

<br><br>

SSID:
Forest-Fire-BlackBox

<br>

IP:
192.168.4.1

</div>

</div>


</div>


<script>


function updateData() {


  fetch('/data')


  .then(response => response.json())


  .then(data => {


    // -----------------------------------
    // TEMPERATURE
    // -----------------------------------

    document.getElementById(
      "temperature"
    ).innerHTML =
      data.temperature.toFixed(1) + " °C";


    // -----------------------------------
    // HUMIDITY
    // -----------------------------------

    document.getElementById(
      "humidity"
    ).innerHTML =
      data.humidity.toFixed(1) + " %";


    // -----------------------------------
    // AIR QUALITY
    // -----------------------------------

    let air =
      document.getElementById("air");


    air.innerHTML =
      data.air;


    air.className = "air";


    if (data.air == "SAFE") {

      air.classList.add("safe");

    }

    else if (data.air == "MODERATE") {

      air.classList.add("moderate");

    }

    else {

      air.classList.add("danger");

    }


    // -----------------------------------
    // MQ135 STATUS
    // -----------------------------------

    document.getElementById(
      "mqStatus"
    ).innerHTML =
      "MQ135 air condition: " +
      data.air;


    // -----------------------------------
    // OVERALL SYSTEM
    // -----------------------------------

    let system =
      document.getElementById(
        "systemStatus"
      );


    system.innerHTML =
      data.system;


    if (data.system == "DANGER") {

      system.className =
        "status danger";

    }

    else {

      system.className =
        "status safe";

    }

  })


  .catch(error => {

    console.log(
      "Sensor communication error:",
      error
    );

  });

}


// Update every 2 seconds

setInterval(
  updateData,
  2000
);


updateData();


</script>


</body>

</html>

)rawliteral";


  server.send(
    200,
    "text/html",
    html
  );
}

// =====================================================
// JSON DATA
// =====================================================

void handleData() {

  /*
     IMPORTANT:

     We DON'T read the sensors here.

     The sensors are continuously updated
     in the main loop.

     This makes the webserver much more reliable.
  */


  String json = "{";


  json += "\"temperature\":";
  json += String(temperature, 1);


  json += ",\"humidity\":";
  json += String(humidity, 1);


  json += ",\"air\":\"";
  json += airStatus;


  json += "\",\"system\":\"";
  json += overallStatus;


  json += "\"";


  json += "}";


  server.send(
    200,
    "application/json",
    json
  );
}

// =====================================================
// CAPTIVE PORTAL
// =====================================================

void handleCaptivePortal() {

  server.sendHeader(
    "Location",
    "http://192.168.4.1/",
    true
  );

  server.send(
    302,
    "text/plain",
    ""
  );
}

// =====================================================
// OTA
// =====================================================

void setupOTA() {

  ArduinoOTA.setHostname(
    "Forest-Fire-BlackBox"
  );

  ArduinoOTA.setPassword(
    "firebox"
  );


  ArduinoOTA.onStart([]() {

    Serial.println();

    Serial.println(
      "OTA UPDATE STARTED"
    );

  });


  ArduinoOTA.onEnd([]() {

    Serial.println();

    Serial.println(
      "OTA UPDATE FINISHED"
    );

  });


  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {

      Serial.printf(
        "OTA Progress: %u%%\r",
        progress / (total / 100)
      );

    }
  );


  ArduinoOTA.onError(
    [](ota_error_t error) {

      Serial.printf(
        "OTA Error[%u]\n",
        error
      );

    }
  );


  ArduinoOTA.begin();


  Serial.println(
    "OTA READY"
  );
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("==============================");
  Serial.println("FOREST FIRE BLACK BOX");
  Serial.println("==============================");


  // -----------------------------------------------
  // DHT11
  // -----------------------------------------------

  dht.begin();

  Serial.println(
    "DHT11 started"
  );


  // -----------------------------------------------
  // ACCESS POINT
  // -----------------------------------------------

  WiFi.mode(WIFI_AP);


  WiFi.softAPConfig(
    local_IP,
    gateway,
    subnet
  );


  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD
  );


  Serial.println();

  Serial.println(
    "Wi-Fi AP started"
  );


  Serial.print(
    "SSID: "
  );

  Serial.println(
    AP_SSID
  );


  Serial.print(
    "IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // -----------------------------------------------
  // DNS CAPTIVE PORTAL
  // -----------------------------------------------

  dnsServer.start(
    DNS_PORT,
    "*",
    local_IP
  );


  // -----------------------------------------------
  // WEB ROUTES
  // -----------------------------------------------

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );


  server.on(
    "/data",
    HTTP_GET,
    handleData
  );


  server.on(
    "/generate_204",
    HTTP_GET,
    handleCaptivePortal
  );


  server.on(
    "/gen_204",
    HTTP_GET,
    handleCaptivePortal
  );


  server.on(
    "/hotspot-detect.html",
    HTTP_GET,
    handleCaptivePortal
  );


  server.on(
    "/connecttest.txt",
    HTTP_GET,
    handleCaptivePortal
  );


  server.on(
    "/ncsi.txt",
    HTTP_GET,
    handleCaptivePortal
  );


  server.onNotFound(
    handleCaptivePortal
  );


  server.begin();


  Serial.println(
    "Web server started"
  );


  // -----------------------------------------------
  // OTA
  // -----------------------------------------------

  setupOTA();


  Serial.println();

  Serial.println("==============================");

  Serial.println(
    "BLACK BOX READY"
  );

  Serial.println("==============================");


  // -----------------------------------------------
  // INITIAL SENSOR READING
  // -----------------------------------------------

  updateSensors();

  lastSensorRead = millis();
}

// =====================================================
// LOOP
// =====================================================

void loop() {


  // -----------------------------------------------
  // CAPTIVE PORTAL
  // -----------------------------------------------

  dnsServer.processNextRequest();


  // -----------------------------------------------
  // WEB SERVER
  // -----------------------------------------------

  server.handleClient();


  // -----------------------------------------------
  // OTA
  // -----------------------------------------------

  ArduinoOTA.handle();


  // -----------------------------------------------
  // SENSOR UPDATE
  // -----------------------------------------------

  if (
    millis() - lastSensorRead >=
    SENSOR_INTERVAL
  ) {

    lastSensorRead = millis();

    updateSensors();
  }
}