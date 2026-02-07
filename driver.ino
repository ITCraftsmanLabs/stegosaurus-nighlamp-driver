#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>
#include <WiFi.h>
#include <secrets.h>
#include <HTTPServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ESPmDNS.h>
#include <PubSubClient.h>

using namespace httpsserver;

#define TRIGGER_DELAY 1 // milliseconds to wait for bouncing to stop
#define TRIGGER_INSENSITIVITY 3 // milliseconds to wait before a new input chage considered new after another
#define COLOR_CHANGE_STEP 5 // amount fo color change in a single step
#define FASTLED_DELAY 20 // milliseconds delay between FastLED executions

#define NUM_LEDS 13 // number of LEDs in the LED strip
#define LED_PIN 4 // pin of the data for the WS2812B LEDs

#define RGB_BUILTIN 48 // pin of the built in LED
#define BUILTIN_LED_STRENGTH 10 // strenght of the built in LED

#define RED_CLK 5 // clock pin of the rotary encoder controlling color red
#define RED_DATA 6 // data pin of the rotary encoder controlling color red
#define RED_BUTTON 7 // pin of the push button on rotary encoder controlling color red
#define GREEN_CLK 15 // clock pin of the rotary encoder controlling color green
#define GREEN_DATA 16 // data pin of the rotary encoder controlling color green
#define GREEN_BUTTON 17 // pin of the push button on rotary encoder controlling color green
#define BLUE_CLK 18 // clock pin of the rotary encoder controlling color blue
#define BLUE_DATA 8 // data pin of the rotary encoder controlling color blue
#define BLUE_BUTTON 3 // pin of the push button on rotary encoder controlling color blue
#define BRIGHTNESS_CLK 46 // clock pin of the rotary encoder controlling brightness
#define BRIGHTNESS_DATA 9 // data pin of the rotary encoder controlling brightness
#define BRIGHTNESS_BUTTON 10 // pin of the push button on rotary encoder controlling brightness
#define MODE_BUTTON 11 // pin of the mode push button
#define ONOFF_BUTTON 12 // pin of the on-off push butto

#define MQTT_SERVER "homeassistant.local" // host of the mqtt server
#define MQTT_PORT 1883 // port of the MQTT server
#define MQTT_TOPIC "stegomqtt/01" // MQTT topic where the status data is published

CRGB leds[NUM_LEDS];
uint8_t startHue = 0;

void connectToWifi();
void wifiConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);
void wifiDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info);
void connectMqtt();
void handleInteractions();
bool handleTurnTrigger(volatile bool *trigger, volatile unsigned long *triggered_at, volatile uint8_t *direction, uint8_t clock, volatile uint8_t *color);
bool handleButtonPress(volatile bool *trigger, volatile unsigned long *triggered_at, uint8_t pin, volatile uint8_t *color);
void incrementColor(volatile uint8_t *color);
void decrementColor(volatile uint8_t *color);
void toggleColor(volatile uint8_t *color);
void handleTurn(volatile bool *trigger, volatile unsigned long *triggered_at, volatile uint8_t *direction);
void handleButtonPress(volatile bool *trigger, volatile unsigned long *triggered_at);
void handleRedTurn();
void handleRedButton();
void handleGreenTurn();
void handleGreenButton();
void handleBlueTurn();
void handleBlueButton();
void handleBrightnessTurn();
void handleBrightnessButton();
void handleModeButton();
void handleOnoffButton();
bool handleModeChange();
bool handleOnoff();
void loadPreferences();
void savePreferences();
void publishPreferences();
void changeMode();
void toggleOnoff();

volatile bool red_triggered = false;
volatile unsigned long red_triggered_at = 0;
volatile uint8_t red_direction;
volatile bool red_button_triggered = false;
volatile unsigned long red_button_triggered_at = 0;
volatile bool green_triggered = false;
volatile unsigned long green_triggered_at = 0;
volatile uint8_t green_direction;
volatile bool green_button_triggered = false;
volatile unsigned long green_button_triggered_at = 0;
volatile bool blue_triggered = false;
volatile unsigned long blue_triggered_at = 0;
volatile uint8_t blue_direction;
volatile bool blue_button_triggered = false;
volatile unsigned long blue_button_triggered_at = 0;
volatile bool brightness_triggered = false;
volatile unsigned long brightness_triggered_at = 0;
volatile uint8_t brightness_direction;
volatile bool brightness_button_triggered = false;
volatile unsigned long brightness_button_triggered_at = 0;
volatile bool mode_button_triggered = false;
volatile unsigned long mode_button_triggered_at = 0;
volatile bool onoff_button_triggered = false;
volatile unsigned long onoff_button_triggered_at = 0;

Preferences prefs;
volatile uint8_t brightness = 255;
volatile uint8_t red = 255;
volatile uint8_t green = 255;
volatile uint8_t blue = 255;
volatile uint8_t mode = 1;
volatile bool off = false;

unsigned long fastLedProcessed = 0;
unsigned long mqttProcessed = 0;

HTTPServer httpServer = HTTPServer();

void setupHttp();
void statusHandler(HTTPRequest * req, HTTPResponse * res);
void changeHandler(HTTPRequest * req, HTTPResponse * res);
void switchHandlerGet(HTTPRequest * req, HTTPResponse * res);
void switchHandlerPost(HTTPRequest * req, HTTPResponse * res);
void setHandlerPost(HTTPRequest * req, HTTPResponse * res);

void callback(char* topic, byte* payload, unsigned int length) {}

WiFiClient wifiClient;
PubSubClient psclient(wifiClient);

void setup() {
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  Serial.begin(115200);
  prefs.begin("nightlight");
  loadPreferences();

  WiFi.onEvent(wifiConnectedHandler, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(wifiDisconnectedHandler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  pinMode(RED_CLK, INPUT_PULLUP);
  pinMode(RED_DATA, INPUT_PULLUP);
  pinMode(RED_BUTTON, INPUT_PULLDOWN);
  pinMode(GREEN_CLK, INPUT_PULLUP);
  pinMode(GREEN_DATA, INPUT_PULLUP);
  pinMode(GREEN_BUTTON, INPUT_PULLDOWN);
  pinMode(BLUE_CLK, INPUT_PULLUP);
  pinMode(BLUE_DATA, INPUT_PULLUP);
  pinMode(BLUE_BUTTON, INPUT_PULLDOWN);
  pinMode(BRIGHTNESS_CLK, INPUT_PULLUP);
  pinMode(BRIGHTNESS_DATA, INPUT_PULLUP);
  pinMode(BRIGHTNESS_BUTTON, INPUT_PULLDOWN);
  pinMode(MODE_BUTTON, INPUT_PULLDOWN);
  pinMode(ONOFF_BUTTON, INPUT_PULLDOWN);

  attachInterrupt(RED_CLK, handleRedTurn, FALLING);
  attachInterrupt(RED_BUTTON, handleRedButton, RISING);
  attachInterrupt(GREEN_CLK, handleGreenTurn, FALLING);
  attachInterrupt(GREEN_BUTTON, handleGreenButton, RISING);
  attachInterrupt(BLUE_CLK, handleBlueTurn, FALLING);
  attachInterrupt(BLUE_BUTTON, handleBlueButton, RISING);
  attachInterrupt(BRIGHTNESS_CLK, handleBrightnessTurn, FALLING);
  attachInterrupt(BRIGHTNESS_BUTTON, handleBrightnessButton, RISING);
  attachInterrupt(MODE_BUTTON, handleModeButton, RISING);
  attachInterrupt(ONOFF_BUTTON, handleOnoffButton, RISING);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  connectToWifi();
  setupHttp();
}

void setupHttp() {
  ResourceNode * statusNode = new ResourceNode("/status", "GET", &statusHandler);
  ResourceNode * changeNode = new ResourceNode("/set/*/*", "GET", &changeHandler);
  ResourceNode * setPostNode = new ResourceNode("/set/*", "POST", &setHandlerPost);
  ResourceNode * switchGetNode = new ResourceNode("/switch/*", "GET", &switchHandlerGet);
  ResourceNode * switchPostNode = new ResourceNode("/switch/*", "POST", &switchHandlerPost);
  httpServer.registerNode(statusNode);
  httpServer.registerNode(changeNode);
  httpServer.registerNode(setPostNode);
  httpServer.registerNode(switchGetNode);
  httpServer.registerNode(switchPostNode);
  httpServer.start();
}

void connectToWifi() {
  neopixelWrite(RGB_BUILTIN, 0, 0, BUILTIN_LED_STRENGTH);
  WiFi.disconnect(true, true);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
}

void wifiConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WiFi-event] connection event, connected to %s.\n", info.wifi_sta_connected.ssid);
  MDNS.end();
  MDNS.begin("stegolamp");
  neopixelWrite(RGB_BUILTIN, 0, BUILTIN_LED_STRENGTH, 0);
}

void connectMqtt() {
  if (mqttProcessed + 1000 < millis()) {
    if (WiFi.isConnected() && !psclient.connected()) {
      psclient.disconnect();
      psclient.setServer(MQTT_SERVER, MQTT_PORT);
      if (psclient.connect("stegoclient", MQTT_USER, MQTT_PWD)) {
        Serial.println("[MTTQ-event] connection successful.");
        publishPreferences();
      } else {
        Serial.println("[MTTQ-event] connection failed.");
      }
    }
    mqttProcessed = millis();
  }
}

void wifiDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WiFi-event] disconnection event, disconnected form %s.\n", info.wifi_sta_disconnected.ssid);
  neopixelWrite(RGB_BUILTIN, BUILTIN_LED_STRENGTH, 0, 0);
}

void loadPreferences() {
  brightness = prefs.getInt("brightness", 255);
  red = prefs.getInt("red", 255);
  green = prefs.getInt("green", 255);
  blue = prefs.getInt("blue", 255);
  mode = prefs.getInt("mode", 1);
  off = prefs.getBool("off", false);
}

void savePreferences() {
  prefs.putInt("brightness", brightness);
  prefs.putInt("red", red);
  prefs.putInt("green", green);
  prefs.putInt("blue", blue);
  prefs.putInt("mode", mode);
  prefs.putBool("off", off);
  publishPreferences();
}

void publishPreferences() {
  if (psclient.connected()) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"red\":%d,\"green\":%d,\"blue\": %d,\"brightness\":%d,\"mode\":%d,\"off\":%s}", red, green, blue, brightness, mode, off ? "true" : "false");
    if (psclient.publish(MQTT_TOPIC, buffer)) {
      Serial.printf("[MTTQ-event] Published to topic %s\n.", MQTT_TOPIC);
    }
  }
}

void loop() {
  httpServer.loop();
  connectMqtt();
  handleInteractions();
  if (fastLedProcessed + FASTLED_DELAY < millis()) {
    FastLED.setBrightness(brightness);
    if (off) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
      switch(mode) {
        case 0:
          fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
          break;
        case 1:
          fill_rainbow(leds, NUM_LEDS, startHue++, 10);        
          break;
      }
    }
    FastLED.show();
    fastLedProcessed = millis();
  }
}

void handleInteractions() {
  bool changed = false;
  if (handleOnoff()) changed = true;
  if (!off) {
    if (handleTurnTrigger(&red_triggered, &red_triggered_at, &red_direction, RED_CLK, &red) || 
        handleTurnTrigger(&green_triggered, &green_triggered_at, &green_direction, GREEN_CLK, &green) ||
        handleTurnTrigger(&blue_triggered, &blue_triggered_at, &blue_direction, BLUE_CLK, &blue) ||
        handleTurnTrigger(&brightness_triggered, &brightness_triggered_at, &brightness_direction, BRIGHTNESS_CLK, &brightness) ||
        handleButtonPress(&red_button_triggered, &red_button_triggered_at, RED_BUTTON, &red) ||
        handleButtonPress(&green_button_triggered, &green_button_triggered_at, GREEN_BUTTON, &green) ||
        handleButtonPress(&blue_button_triggered, &blue_button_triggered_at, BLUE_BUTTON, &blue) ||
        handleButtonPress(&brightness_button_triggered, &brightness_button_triggered_at, BRIGHTNESS_BUTTON, &brightness) ||
        handleModeChange()) {
      changed = true;
    }
  }
  if (changed) savePreferences();
}

bool handleModeChange() {
  if (mode_button_triggered && mode_button_triggered_at + TRIGGER_DELAY < millis()) {
    mode_button_triggered = false;
    if(digitalRead(MODE_BUTTON)) {
      changeMode();
      return true;
    }
  }
  return false;
}

void changeMode() {
  mode++;
  if (mode > 1) mode = 0;
}

bool handleOnoff() {
  if (onoff_button_triggered && onoff_button_triggered_at + TRIGGER_DELAY < millis()) {
    onoff_button_triggered = false;
    if(digitalRead(ONOFF_BUTTON)) {
      toggleOnoff();
      return true;
    }
  }
  return false;
}

void toggleOnoff() {
  off = !off;
}

bool handleTurnTrigger(volatile bool *trigger, volatile unsigned long *triggered_at, volatile uint8_t *direction, uint8_t clock, volatile uint8_t *color) {
  if (*trigger && *triggered_at + TRIGGER_DELAY < millis()) {
    *trigger = false;
    if(!digitalRead(clock)) {
      if (*direction) {
        incrementColor(color);
      } else {
        decrementColor(color);
      }
      return true;
    }
  }
  return false;
}

bool handleButtonPress(volatile bool *trigger, volatile unsigned long *triggered_at, uint8_t pin, volatile uint8_t *color) {
  if (*trigger && *triggered_at + TRIGGER_DELAY < millis()) {
    *trigger = false;
    if(digitalRead(pin)) {
      toggleColor(color);
      return true;
    }
  }
  return false;
}

void incrementColor(volatile uint8_t *color) {
  if (*color + COLOR_CHANGE_STEP < 255) {
    *color = *color + COLOR_CHANGE_STEP;
  } else {
    *color = 255;
  }
}

void decrementColor(volatile uint8_t *color) {
  if (*color - COLOR_CHANGE_STEP > 0) {
    *color = *color - COLOR_CHANGE_STEP;
  } else{
    *color = 0;
  } 
}

void toggleColor(volatile uint8_t *color) {
  if (*color != 255) {
    *color = 255;
  } else{
    *color = 0;
  } 
}

void handleRedTurn() {
  handleTurn(&red_triggered, &red_triggered_at, &red_direction, RED_DATA);
}

void handleGreenTurn() {
  handleTurn(&green_triggered, &green_triggered_at, &green_direction, GREEN_DATA);
}

void handleBlueTurn() {
  handleTurn(&blue_triggered, &blue_triggered_at, &blue_direction, BLUE_DATA);
}

void handleBrightnessTurn() {
  handleTurn(&brightness_triggered, &brightness_triggered_at, &brightness_direction, BRIGHTNESS_DATA);
}

void handleTurn(volatile bool *trigger, volatile unsigned long *triggered_at, volatile uint8_t *direction, uint8_t data) {
  if (!*trigger && *triggered_at + TRIGGER_INSENSITIVITY < millis()) {
    *direction = digitalRead(data);
    *trigger = true;
    *triggered_at = millis();
  }
}

void handleRedButton() {
  handleButtonPress(&red_button_triggered, &red_button_triggered_at);
}

void handleGreenButton() {
  handleButtonPress(&green_button_triggered, &green_button_triggered_at);
}

void handleBlueButton() {
  handleButtonPress(&blue_button_triggered, &blue_button_triggered_at);
}

void handleBrightnessButton() {
  handleButtonPress(&brightness_button_triggered, &brightness_button_triggered_at);
}

void handleModeButton() {
  handleButtonPress(&mode_button_triggered, &mode_button_triggered_at);
}

void handleOnoffButton() {
  handleButtonPress(&onoff_button_triggered, &onoff_button_triggered_at);
}

void handleButtonPress(volatile bool *trigger, volatile unsigned long *triggered_at) {
  if (!*trigger && *triggered_at + TRIGGER_INSENSITIVITY < millis()) {
    *trigger = true;
    *triggered_at = millis();
  }
}

void statusHandler(HTTPRequest * req, HTTPResponse * res) {
  ResourceParameters * params = req->getParams();
  res->setHeader("Content-Type", "application/json");
  res->printf("{\"red\":%d,\"green\":%d,\"blue\":%d,\"brightness\":%d,\"mode\":%d,\"off\":%s}", red, green, blue, brightness, mode, off ? "true" : "false");
}

void changeHandler(HTTPRequest * req, HTTPResponse * res) {
  ResourceParameters * params = req->getParams();
  std::string parameter1, parameter2;
  bool changed = false;
  if (params->getPathParameter(0, parameter1) && params->getPathParameter(1, parameter2)) {
    if (parameter1 == "toggle") {
      changed = true;
      if (parameter2 == "red") {
        toggleColor(&red);
      }
      if (parameter2 == "green") {
        toggleColor(&green);
      }
      if (parameter2 == "blue") {
        toggleColor(&blue);
      }
      if (parameter2 == "brightness") {
        toggleColor(&blue);
      }
      if (parameter2 == "mode") {
        changeMode();
      }
      if (parameter2 == "onoff") {
        toggleOnoff();
      }
    }
  }
  if (changed) {
    savePreferences();
  }
}

void switchHandlerGet(HTTPRequest * req, HTTPResponse * res) {
  ResourceParameters * params = req->getParams();
  res->setHeader("Content-Type", "text/plain");
  std::string parameter1;
  if (params->getPathParameter(0, parameter1)) {
    if (parameter1 == "mode") {
      res->print(mode);
    }
    if (parameter1 == "onoff") {
      res->print(off ? "0": "1");
    }
  }
}

void switchHandlerPost(HTTPRequest * req, HTTPResponse * res) {
  ResourceParameters * params = req->getParams();
  std::string parameter1;
  if (params->getPathParameter(0, parameter1)) {
    if(!(req->requestComplete())) {
      char* buffer = new char[1];
      req->readChars(buffer, 1);
      req->discardRequestBody();
      if (parameter1 == "mode") {
        mode = buffer[0] == '1' ? 1 : 0;
        res->print(mode);
      }
      if (parameter1 == "onoff") {
        off = buffer[0] == '1' ? 0 : 1;
        res->print(off ? "0": "1");
      }
      savePreferences();
    }
  }
}

void setHandlerPost(HTTPRequest * req, HTTPResponse * res) {
  ResourceParameters * params = req->getParams();
  std::string parameter1;
  if (params->getPathParameter(0, parameter1)) {
    if(!(req->requestComplete())) {
      char* buffer = new char[12]();
      req->readChars(buffer, 11);
      req->discardRequestBody();
      if (parameter1 == "brightness") {
        brightness = atoi(buffer);
        res->print(brightness);
      }
      if (parameter1 == "color") {
        char * ptr = strtok(buffer, ",");
        for (int i = 0; ptr != NULL && i < 3; i++) {
          switch (i) {
            case 0:
              red = atoi(ptr);
              break;
            case 1:
              green = atoi(ptr);
              break;
            case 2:
              blue = atoi(ptr);
              break;
          } 
          ptr = strtok(NULL, ",");
        }
        res->printf("%d,%d,%d", red, green, blue);
      }
    }
    savePreferences();
  }
}