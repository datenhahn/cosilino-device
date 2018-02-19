/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Jonas Hahn <jonas.hahn@datenhahn.de>
 *
 */

#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

/* define ESP8266 pin usage*/
#define SERVO_PIN D1
#define PWM_PIN D4
#define RED_LED D8
#define YELLOW_LED D7
#define GREEN_LED D6
#define ONE_WIRE_BUS D2  // DS18B20 pin
#define DHTPIN D3
#define DHTTYPE DHT22 //DHT11, DHT21, DHT22

/* other constants */
#define MAX_SERVO_DEGREE 180
#define TOPIC_PREFIX "cosilino"
#define OUT_SUFFIX "status"
#define IN_SUFFIX "heaterpower"
#define MQTT_SERVER "cosilino-gateway"
#define DEFAULT_AP_PW "warmandcosy"
#define PUBLISH_INTERVAL_MS 10000

Servo servo;
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

int globalHeatRatio = 0;
long lastMsg = 0;

void setup() {

    setupLedPins();
    Serial.begin(115200);

    flashLed(GREEN_LED, 5);

    Serial.println("Trying to reconnect wifi ...");

    wifiManager.setAPCallback(configModeCallback);
    String apName = "cosilino-" + getDeviceId();
    wifiManager.autoConnect(apName.c_str(), DEFAULT_AP_PW);

    onlyGreenLed();
    delay(500);

    dht.begin();
    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(callback);
    setHeatRatio(50);
}


void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > PUBLISH_INTERVAL_MS) {
        lastMsg = now;
        publishMessage();
    }

}


/**
 * Called by WifiManager when it enters config mode.
 * Sets all LEDs to on.
 */
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
    allLedsOn();
}

void publishMessage() {
    float roomHumidity = dht.readHumidity();     //Luftfeuchte auslesen
    float roomTemperature = dht.readTemperature();  //Temperatur auslesen

    DS18B20.requestTemperatures();
    float heaterTemperature = DS18B20.getTempCByIndex(0);

    String mqttMessage = "{\"deviceId\":\"" + getDeviceId() + "\",";
    mqttMessage += "\"roomHum\":" + String(roomHumidity) + ",";
    mqttMessage += "\"roomTemp\":" + String(roomTemperature) + ",";
    mqttMessage += "\"heaterTemp\":" + String(heaterTemperature) + ",";
    mqttMessage += "\"heaterPower\":" + String(globalHeatRatio) + "}";

    Serial.print("Publish message: ");
    Serial.println(mqttMessage.c_str());

    String outTopic = TOPIC_PREFIX + "/" + getDeviceId() + "/" + OUT_SUFFIX;
    client.publish(outTopic.c_str(), mqttMessage.c_str());
}

void setHeatRatio(int heatRatio) {

    if (heatRatio < 0) {
        heatRatio = 0;
    }

    if (heatRatio > 100) {
        heatRatio = 100;
    }

    globalHeatRatio = heatRatio;

    if (heatRatio > 66) {
        red();
    } else if (heatRatio > 33) {
        yellow();
    } else {
        green();
    }

    int servoDegree = heatRatioToDegree(heatRatio);
    String output =
            "Setting: ratio=" + String(heatRatio) + " degree=" + String(servoDegree) + "\n";

    Serial.write(output.c_str());

    /* It is important here to attach and detach the servo. Otherwise it would constantly try to adjust its position,
     * which results in a constant buzzing noise if it is not able to move fully to the desired position.
     */
    servo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object
    delay(100);
    servo.write(servoDegree);
    delay(2000);
    servo.detach();
}

int heatRatioToDegree(int heatRatio) {
    return MAX_SERVO_DEGREE * heatRatio / 100;
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    String input = String((char *) payload);
    int heatRatio = input.toInt();
    setHeatRatio(heatRatio);
}

String getDeviceId() {
    return String(ESP.getChipId(), HEX);
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client")) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            String myId = "Connecting Device: " + getDeviceId();
            client.subscribe(IN_TOPIC);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/**
 * LED control
 */

void flashLed(int led, int times) {
    digitalWrite(led, HIGH);

    for (int i = 0; i < times; i++) {
        delay(500);
        digitalWrite(led, LOW);
    }
}

void setupLedPins() {
    pinMode(PWM_PIN, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    allLedsOff();
}

void allLedsOff() {
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
}

void allLedsOn() {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
}

void onlyRedLed() {
    allLedsOff();
    digitalWrite(RED_LED, HIGH);
}

void onlyYellowLed() {
    allLedsOff();
    digitalWrite(YELLOW_LED, HIGH);
}

void onlyGreenLed() {
    allLedsOff();
    digitalWrite(GREEN_LED, HIGH);
}
