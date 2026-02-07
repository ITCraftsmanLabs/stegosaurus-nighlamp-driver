# IT Craftsman stegosaurus night lamp driver
The  code was made to run on an ESP32 S3 development board using Arduino IDE. It drives a WS2812B LED strip controlling its color, brightness and a rainbow effect. It handles 4 rotary encoders with push buttons, one for each color and one for the brightness. It also handles two simple push buttons, one for selecting the mode (solid color or rainbow) and one for turning the led strip on and off.
## Used libraries
- [Arduino core for the ESP32 family of SoCs v3.3.6](https://github.com/espressif/arduino-esp32)
- [ESP32 HTTPS Server, IDF5 fork v1.1.1](https://github.com/jackjansen/esp32_idf5_https_server)
- [PubSubClient v2.8](https://github.com/knolleary/pubsubclient)
- [FastLED v3.10.3](https://github.com/FastLED/FastLED)
## Features
- Connects and reconnects to Wi-Fi
- Uses mDNS to register itself and to connect to MQTT broker
- It remembers the set color, brightness, mode and state and comes back with the right settings after a power outage
- It exposes REST endpoints for controlling color, brightness, mode and state
- It connects to a MQTT broker to publish color, brightness, mode and state changes to a topic
- Software debounces the rotary encoder and push button inputs
- Home Assistant integrable