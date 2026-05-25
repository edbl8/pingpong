#include <WiFi.h>
#include <PubSubClient.h>

// =====================================
// WIFI
// =====================================

const char* ssid = "iPSK-UMU";
const char* password = "HejHopp!!";

// =====================================
// MQTT
// =====================================

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// =====================================
// UART
// =====================================

#define RX_PIN 16
#define TX_PIN 17

// =====================================
// GLOBALS
// =====================================

uint32_t next_time_1s = 1000;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// =====================================
// WIFI EVENTS
// =====================================

void WiFiStationConnected(
    WiFiEvent_t event,
    WiFiEventInfo_t info
) {
    Serial.println("WiFi: Ansluten!");
}

void WiFiGotIP(
    WiFiEvent_t event,
    WiFiEventInfo_t info
) {
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(
    WiFiEvent_t event,
    WiFiEventInfo_t info
) {
    Serial.println("WiFi tappad!");

    WiFi.begin(ssid, password);
}

// =====================================
// WIFI SETUP
// =====================================

void setup_wifi() {

    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {

        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected");
}

// =====================================
// MQTT CALLBACK
// =====================================

void mqtt_callback(
    char* topic,
    byte* message,
    unsigned int length
) {

    Serial.print("MQTT topic: ");
    Serial.println(topic);

    String cmd = "";

    // bygg sträng från payload
    for (int i = 0; i < length; i++) {

        cmd += (char)message[i];
    }

    cmd.trim();

    Serial.print("Kommando: ");
    Serial.println(cmd);

    // =====================================
    // AUTO MODE
    // CMD:MODE:AUTO
    // =====================================

    if(cmd == "CMD:MODE:AUTO") {

        Serial.println("AUTO MODE");

        Serial2.println("MODE:AUTO");
    }

    // =====================================
    // THERMAL MODE
    // CMD:MODE:THERMAL
    // =====================================

    else if(cmd == "CMD:MODE:THERMAL") {

        Serial.println("AUTO THERMAL");

        Serial2.println("MODE:THERMAL");
    }

    // =====================================
    // MANUAL MODE
    // CMD:MODE:MANUAL
    // =====================================

    else if(cmd == "CMD:MODE:MANUAL") {

        Serial.println("MANUAL MODE");

        Serial2.println("MODE:MANUAL");
    }

    // =====================================
    // AIM
    // CMD:AIM:1650
    // =====================================

    else if(cmd.startsWith("CMD:AIM:")) {

        String aimValue =
            cmd.substring(8);

        Serial.print("AIM: ");
        Serial.println(aimValue);

        Serial2.print("AIM:");
        Serial2.println(aimValue);
    }

    // =====================================
    // SPIN
    // CMD:SPIN:600:600
    // =====================================

    else if(cmd.startsWith("CMD:SPIN:")) {

        Serial.println("SPIN COMMAND");

        int firstColon =
            cmd.indexOf(':');

        int secondColon =
            cmd.indexOf(':', firstColon + 1);

        int thirdColon =
            cmd.indexOf(':', secondColon + 1);

        String motor1 =
            cmd.substring(
                secondColon + 1,
                thirdColon
            );

        String motor2 =
            cmd.substring(
                thirdColon + 1
            );

        Serial.print("Motor1: ");
        Serial.println(motor1);

        Serial.print("Motor2: ");
        Serial.println(motor2);

        // skicka vidare till STM32

        Serial2.print("SPIN:");
        Serial2.print(motor1);
        Serial2.print(":");
        Serial2.println(motor2);
    }

    // =====================================
    // SHOOT
    // CMD:SHOOT
    // =====================================

    else if(cmd == "CMD:SHOOT") {

        Serial.println("SHOOT");

        Serial2.println("SHOOT");
    }

    // =====================================
    // STOP
    // CMD:STOP
    // =====================================

    else if(cmd == "CMD:STOP") {

        Serial.println("STOP");

        Serial2.println("STOP");
    }

    // =====================================
    // UNKNOWN
    // =====================================

    else {

        Serial.println("UNKNOWN COMMAND");

        Serial2.println("UNKNOWN");
    }
}

// =====================================
// MQTT RECONNECT
// =====================================

void reconnect() {

    while (!mqttClient.connected()) {

        Serial.print(
            "Attempting MQTT connection..."
        );

        if(mqttClient.connect(
            "ESP32Client_umu_grupp7"
        )) {

            Serial.println("connected");

            mqttClient.subscribe(
                "umu_pingis_grupp7/control"
            );

        } else {

            Serial.print("failed, rc=");

            Serial.print(
                mqttClient.state()
            );

            Serial.println(
                " try again in 5 seconds"
            );

            delay(5000);
        }
    }
}

// =====================================
// EVERY 1 SECOND
// =====================================

void every_1s() {

    // debug här om ni vill

    // Serial.println("Live...");
}

// =====================================
// SETUP
// =====================================

void setup() {

    // USB serial
    Serial.begin(115200);

    Serial2.setRxBufferSize(1024);

    // UART till STM32
    Serial2.begin(
        115200,
        SERIAL_8N1,
        RX_PIN,
        TX_PIN
    );

    // WiFi events
    WiFi.onEvent(
        WiFiStationConnected,
        WiFiEvent_t::
        ARDUINO_EVENT_WIFI_STA_CONNECTED
    );

    WiFi.onEvent(
        WiFiGotIP,
        WiFiEvent_t::
        ARDUINO_EVENT_WIFI_STA_GOT_IP
    );

    WiFi.onEvent(
        WiFiStationDisconnected,
        WiFiEvent_t::
        ARDUINO_EVENT_WIFI_STA_DISCONNECTED
    );

    // connect wifi
    setup_wifi();

    // MQTT
    mqttClient.setServer(
        mqtt_server,
        mqtt_port
    );

    mqttClient.setCallback(
        mqtt_callback
    );

    mqttClient.setBufferSize(1024);
}

// =====================================
// LOOP
// =====================================

void loop() {

    // reconnect MQTT
    if(!mqttClient.connected()) {

        reconnect();
    }

    mqttClient.loop();

    // every second
    uint32_t time = millis();

    if (time >= next_time_1s) {

        next_time_1s += 1000;

        every_1s();
    }

    // =====================================
    // DATA FROM STM32
    // =====================================

    if(Serial2.available()) {

        String dataFromSTM =
            Serial2.readStringUntil('\n');

        dataFromSTM.trim();

        if(dataFromSTM.length() > 0) {

            mqttClient.publish(
                "umu_pingis_grupp7/telemetry",
                dataFromSTM.c_str()
            );

            Serial.print("STM data: ");

            Serial.println(dataFromSTM);
        }
    }
}
