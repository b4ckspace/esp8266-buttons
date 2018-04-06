#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Bounce2.h>
#include "settings.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

uint8_t mqttRetryCounter = 0;

typedef struct {
  uint8_t pin;
  const char* mqttTopic;
  const char* valueUp;
  const char* valueDown;
  uint16_t debounceMs;
  Bounce *debouncer;
} t_buttonConfiguration;

t_buttonConfiguration buttons[] = {
  {D1, "sensor/button/hackcenter/0", "released", "pressed", 42, NULL},
  {D2, "sensor/button/hackcenter/1", "released", "pressed", 42, NULL},
  {D3, "sensor/button/hackcenter/2", "released", "pressed", 42, NULL},
  {D4, "sensor/button/hackcenter/3", "released", "pressed", 42, NULL}
};

WiFiClient wifiClient;
PubSubClient mqttClient;
Bounce debouncer = Bounce();

void setup() {

  delay(2500);
  
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
    
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  
  for (uint8_t i = 0; i < ARRAY_SIZE(buttons); i++) {
     pinMode(buttons[i].pin, INPUT_PULLUP);
     
     buttons[i].debouncer = new Bounce();
     buttons[i].debouncer->attach(buttons[i].pin);
     buttons[i].debouncer->interval(buttons[i].debounceMs);
  }
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(HOSTNAME, MQTT_TOPIC_LAST_WILL, 1, true, "disconnected")) {
      mqttClient.publish(MQTT_TOPIC_LAST_WILL, "connected", true);
      mqttRetryCounter = 0;
      
    } else {
            
      if (mqttRetryCounter++ > MQTT_MAX_CONNECT_RETRY) {
        ESP.restart();
      }
      
      delay(2000);
    }
  }
}

void loop() {
  
  mqttConnect();
  mqttClient.loop();
  ArduinoOTA.handle();
  
  for (uint8_t i = 0; i < ARRAY_SIZE(buttons); i++) {
    buttons[i].debouncer->update();
    
    if(buttons[i].debouncer->rose()) {
      mqttClient.publish(buttons[i].mqttTopic, buttons[i].valueUp, true);  
    } else if(buttons[i].debouncer->fell()) {
      mqttClient.publish(buttons[i].mqttTopic, buttons[i].valueDown, true);  
    }
  }
}
