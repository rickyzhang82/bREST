# What
This is a customized firmware for Sonoff S20.

## New Features

- Turn on the green LED and turn off the red LED when the switch is open.
- Turn off the green LED and turn on the red LED when the switch is closed.
- Enable button switch as a physical switch button.

# How
- Change your wifi_settings.h and IP address in Arduino sketch.
- Solder 4 jumper wires to VCC, GND, TX, RX.
- Hook up USB-to-Serial adapter. VCC => 3V, GND => GND, TX => TXD, RX => RXD. (US version Sonoff S20 fuckup RX and TX)
- Hold the power button of Sonoff S20 before plugin USB-to-Serial adapter to computer. Keep holding button for 3 more seconds so that the ESP8266 boot into flash mode.
- Upload the firmware.
- You should expect the green LED light is on.
- Run Python script demo_power_plug.py as your http client
