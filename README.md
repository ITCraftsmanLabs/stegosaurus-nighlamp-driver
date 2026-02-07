# IT Craftsman stegosaurus night lamp driver
The  code was made to run on an ESP32 S3 development board using Arduino IDE. It drives a WS2812B LED strip controlling its color, brightness and a rainbow effect. It handles 4 rotary encoders with push buttons, one for each color and one for the brightness. It also handles two simple push buttons, one for selecting the mode (solid color or rainbow) and one for turning the led strip on and off.
## Features
- Connect and reconnects to Wi-Fi
- It remembers the set state and comes back with the right settings after a power outage
- It exposes REST endpoints for controlling color, brightness, mode and state
- It connects to a MQTT broker to publish color, brightness, mode and state changes to a topic
- Software debounces the rotary encoder and push button inputs