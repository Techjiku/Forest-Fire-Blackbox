# Forest Fire Black Box 🌲

A standalone environmental monitoring prototype built with an **ESP8266 ESP-12E**, an **MQ135 air-quality sensor**, a **DHT11 temperature and humidity sensor**, and an **external solar-panel charging setup**.

The device creates its own Wi-Fi access point and hosts a local dashboard, so a phone or laptop can view sensor readings without a home router or internet connection.

> **Important:** This is an experimental monitoring prototype, not a certified forest-fire detection or life-safety system. The current logic can miss fires and can also produce false alarms. Do not use it as the sole means of fire detection.

## Features

- **Standalone Wi-Fi access point** — creates a local network named `Forest-Fire-BlackBox`.
- **Local web dashboard** — shows temperature, relative humidity, MQ135 raw reading, air-status classification, and overall system status.
- **Automatic refresh** — the browser requests updated data every 2 seconds.
- **Captive-portal-style redirects** — common connectivity-check URLs are redirected to the dashboard.
- **Sensor sampling** — reads the MQ135 several times and averages the ADC samples; attempts DHT11 readings every 2 seconds.
- **Over-the-air (OTA) firmware updates** — includes ArduinoOTA support when the development computer can reach the ESP8266 over the appropriate network.
- **Serial diagnostics** — prints readings and startup messages at 115200 baud.
- **Solar-powered field deployment intent** — external solar panels are used with a suitable charging and power-regulation circuit (the charging hardware is not controlled by this firmware).

## Hardware

| Component | Purpose | Firmware configuration |
|---|---|---|
| ESP8266 ESP-12E module | Main controller, Wi-Fi access point, web server | ESP8266 Arduino core |
| MQ135 sensor/module | Relative gas/air-quality indication | `A0` |
| DHT11 sensor | Temperature and relative humidity | `D5` (GPIO14 on common ESP8266 board mappings) |
| External solar panel(s) | Intended to recharge the power source | External charging/power circuit required |

### Wiring notes

- **DHT11 data → D5**. Connect sensor power and ground according to the specific DHT11 module/datasheet.
- **MQ135 analog output → A0**. Verify the allowed A0 input range for your exact ESP-12E development board or bare-module circuit before connecting it. ESP8266 bare-chip ADC range and development-board A0 divider ranges can differ; never assume the input is 0–3.3 V.
- **All sensor grounds and ESP8266 ground must be common** when powered from the same system.
- MQ135 heater current can be significant. Check the sensor module's supply requirements and provide a stable supply.
- Do not connect a solar panel directly to the ESP8266 or battery. Use a compatible solar charge controller, protected battery, and regulated output suited to the board and sensors. Include appropriate battery protection and power budgeting.

> The source code confirms the two sensor signal pins, but it does not specify the exact solar-panel wiring, battery chemistry, charge controller, regulator, or power-management circuit. Document those parts separately for your actual build.

## How it works

1. **Boot and initialise:** starts Serial at 115200 baud and initialises the DHT11.
2. **Create a local network:** starts Wi-Fi in access-point-only mode using `Forest-Fire-BlackBox`, with the local address `192.168.4.1`.
3. **Start local services:** starts a DNS wildcard redirect, the HTTP server on port 80, dashboard routes, and ArduinoOTA.
4. **Sample sensors:** reads the MQ135 ten times with short delays and averages the ADC readings. It also requests temperature and humidity from the DHT11.
5. **Classify air readings:** applies configurable raw ADC thresholds to label the air reading `SAFE`, `MODERATE`, or `DANGER`.
6. **Calculate overall status:** sets the overall status to `DANGER` only when the stored temperature is at least 40 °C **and** the MQ135 reading is at least 550. Otherwise, it reports `SAFE`.
7. **Serve the dashboard:** the browser fetches `/data` every 2 seconds. The ESP8266 returns the most recently sampled values as JSON rather than reading sensors during each HTTP request.

### Current thresholds in the firmware

| Reading | Rule | Label/result |
|---|---:|---|
| MQ135 raw ADC | `< 400` | `SAFE` air status |
| MQ135 raw ADC | `400–549` | `MODERATE` air status |
| MQ135 raw ADC | `≥ 550` | `DANGER` air status |
| Temperature | `≥ 40 °C` **and** MQ135 raw ADC `≥ 550` | Overall `DANGER` |
| Any other temperature/MQ135 combination | — | Overall `SAFE` |

These values are **prototype thresholds**, not calibrated ppm values or validated fire-detection limits. The MQ135 raw ADC depends on the sensor module, analogue input scaling, warm-up time, supply voltage, environment, and calibration. The firmware does not calculate gas concentration in ppm.

## Dashboard and endpoints

| URL / endpoint | Purpose |
|---|---|
| `http://192.168.4.1/` | Main monitoring dashboard |
| `http://192.168.4.1/data` | JSON response containing `temperature`, `humidity`, `air`, and `system` |
| Port `80` | Local HTTP server |

The dashboard uses a dark, responsive HTML/CSS layout. JavaScript updates the displayed values and changes status styling based on the returned classifications.

## Getting started

### 1. Install the Arduino environment

Install the Arduino IDE and add support for ESP8266 boards using the official ESP8266 Arduino Core installation instructions. Select the correct board profile for your ESP-12E development board, then select the appropriate serial port.

Install these libraries if they are not already available through the ESP8266 core or Library Manager:

- **DHT sensor library** by Adafruit
- **Adafruit Unified Sensor** if required by the installed DHT library version

The sketch also uses ESP8266-core components: `ESP8266WiFi`, `ESP8266WebServer`, `DNSServer`, and `ArduinoOTA`.

### 2. Review configuration

Check these settings near the top of the sketch before uploading:

```cpp
#define DHT_PIN D5
#define DHT_TYPE DHT11

const int MQ135_PIN = A0;
const int NORMAL_LIMIT = 400;
const int MODERATE_LIMIT = 550;
const float HIGH_TEMPERATURE = 40.0;

const char* AP_SSID = "Forest-Fire-BlackBox";
const char* AP_PASSWORD = "12345678";
```

Change the Wi-Fi password before deployment. The current password is included in the source code and is not suitable for a public or unattended installation.

### 3. Upload and open the dashboard

1. Connect the ESP8266 to your computer over USB-to-serial using the correct voltage levels and boot configuration for your board.
2. Upload the sketch.
3. Open Serial Monitor at **115200 baud** and check the startup output.
4. From a phone or laptop, connect to Wi-Fi **`Forest-Fire-BlackBox`** using the configured password.
5. Open a browser and visit **`http://192.168.4.1/`**.
6. Check the displayed temperature, humidity, air classification, and overall status.

The access point is local-only; internet access is not provided by this network.

## OTA update notes

The sketch configures ArduinoOTA with hostname `Forest-Fire-BlackBox` and an OTA password set in the source. OTA availability depends on the network path and the ESP8266 Arduino environment being able to discover/reach the device. Because the firmware uses access-point-only mode, the development computer generally needs to be connected to that AP and have a compatible route/discovery setup. If OTA discovery does not work, upload over serial USB.

**Security:** replace the OTA password in the source before using OTA. Do not expose this device or its update service to an untrusted network.

## Solar power and field use

Solar charging is part of the physical build, not implemented by this sketch. A safe design normally needs:

- A solar panel matched to the selected charging controller.
- A charge controller compatible with the battery chemistry and panel input.
- A protected battery pack.
- A regulated supply with sufficient current for the ESP8266 Wi-Fi radio and MQ135 heater.
- Proper enclosure, cable strain relief, moisture protection, and thermal management.
- A way to monitor battery voltage and low-battery conditions if unattended operation is intended.

The current code does **not** measure panel output, battery voltage, charging current, or battery state of charge. Solar availability and runtime therefore cannot be inferred from the dashboard.

## Limitations and recommended improvements

- **Not a certified fire alarm:** the overall status only becomes dangerous when high temperature and a high MQ135 reading occur together. A fire may not satisfy both conditions, so the system can continue to show `SAFE` during a hazardous event.
- **No fault state for the full system:** the code tracks DHT11 read validity internally, but `/data` does not expose that validity. The dashboard can therefore display the last valid temperature/humidity even after a later read failure.
- **MQ135 is not fire-specific:** it responds to multiple gases and environmental conditions. Its raw ADC value is not a direct, universal measure of air quality or smoke concentration.
- **Thresholds require calibration:** the 400 and 550 values should be validated against the actual sensor, board ADC scaling, environment, and test conditions.
- **Captive portal behaviour varies:** phones and operating systems do not always automatically open a portal; entering `http://192.168.4.1/` manually is a reliable fallback.
- **No remote alerting:** there is no cloud connection, SMS, push notification, or internet-based alert mechanism in the current firmware.
- **No solar/battery telemetry:** the firmware does not report charging or battery information.
- **Credentials are hard-coded:** Wi-Fi and OTA passwords should be changed before deployment.

Potential next steps include adding an explicit `SENSOR_FAULT` state, exposing DHT validity and sensor freshness in `/data`, logging readings, adding battery-voltage monitoring, calibrating MQ135 behaviour, and designing a safer alarm rule validated through controlled testing.

## Repository contents

Suggested repository structure:

```text
forest-fire-black-box/
├── README.md
├── ForestFireBlackBox.ino
└── docs/
    ├── wiring.md
    └── power-system.md
```

Place the firmware source in `ForestFireBlackBox.ino` (or keep your existing filename) and add your own wiring diagram and solar charging/power schematic to `docs/` as they become available.

## Contributing

Issues, bug reports, wiring corrections, calibration results, and improvements to the monitoring logic are welcome. Include the board variant, sensor module type, supply voltage, ADC reading conditions, and reproduction steps when reporting a problem.

## Licence

No licence is specified in this repository yet. Add a `LICENSE` file before accepting contributions or permitting reuse, and choose a licence appropriate for your project.
