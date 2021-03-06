/**
Copyright (C) 2020  Ricky Zhang

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

// Import required libraries
#include <ESP8266WiFi.h>

// ESP8266 timer
extern "C"
{
  #include "user_interface.h"
}

#define DEBUG 1
#define APP_DEBUG 1

#include <bREST.h>

/** NOTE: this is where you should place your wifi ssid and password. Create wifi_settings.h header file with the sample below:

#ifndef WIFI_SETTINGS_H
#define WIFI_SETTINGS_H
const char* ssid = "your_ssid";
const char* password = "your_password";
#endif // WIFI_SETTINGS_H

*/
#include "wifi_settings.h"

// The digital pin to switch on/off the relay
// This is the output pin number for Sonoff S20
const int ENABLE_PIN = 12;
// This is the input pin number for E-FW input button
// The state is high when switch is open.
const int BUTTON_PIN = 0;
// This is the output pin number for green LED
const int GREEN_LED_PIN = 13;

// define timer
os_timer_t myTimer;
// define timer cycle in milliseconds
const uint32_t TIMER_CYCLE_IN_MS = 10;

// button switch debounce in millieseconds
const long BUTTON_DEBOUNCE = 200;

// Create bREST instance
bREST rest = bREST();

// WiFi parameters
IPAddress ip(192, 168, 2, 44);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet_mask(255, 255, 255, 0);
IPAddress dns(192, 168, 2, 1);

// The port to listen for incoming TCP connections
const int LISTEN_PORT = 80;

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Step1: Define customized resource by inheriting Observer
//        Override call back method update()
class PowerPlug: public Observer {
public:
    PowerPlug(String resource_id): Observer(resource_id) {
        this->isPowerPlugOpen = true;
        this->enablePin = ENABLE_PIN;
        this->greenLEDPin = GREEN_LED_PIN;
        this->buttonInputPin = BUTTON_PIN;
        this->debounce = BUTTON_DEBOUNCE;
        this->buttonState = LOW;
        this->previous = HIGH;
        this->timeToToggle = 0;
    }

    virtual ~PowerPlug(){}

    // override call back function
    void update(HTTP_METHOD method, String parms[], String value[], int parm_count, bREST* rest) override {
        switch(method) {
        case HTTP_METHOD_GET:
            // Send back JSON message to client.
            rest->start_json_msg();
            rest->append_key_value_pair_to_json(String("message"), String("PowerPlug get fire up!"));
            rest->append_comma_to_json();
            rest->append_key_value_pair_to_json(String("code"), CODE_OK);
            rest->append_comma_to_json();
            rest->append_key_value_pair_to_json(String("is_switch_open"), isSwitchOpen());
            rest->end_json_msg();
            break;
        case HTTP_METHOD_PUT:
            if(1 == parm_count && parms[0] == "open") {
                if(value[0] == "true") {
                    openSwitch();
                    sendBackAffirmativeMessage(rest);
                }else if(value[0] == "false") {
                    closeSwitch();
                    sendBackAffirmativeMessage(rest);
                }else{
                    sendBackInvalidCommandMessage(rest);
                }
            }else{
                sendBackInvalidCommandMessage(rest);
            }
            break;
        }
    }

    void sendBackInvalidCommandMessage(bREST* rest) {
        // Send back JSON message to client.
        rest->start_json_msg();
        rest->append_key_value_pair_to_json(String("message"), String("Invalid command!"));
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("code"), CODE_ERROR_INVALID_COMMAND);
        rest->end_json_msg();
    }

    void sendBackAffirmativeMessage(bREST* rest) {
        // Send back JSON message to client.
        rest->start_json_msg();
        rest->append_key_value_pair_to_json(String("message"), String("Affirmative!"));
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("code"), CODE_OK);
        rest->end_json_msg();
    }

    void setup() {
        pinMode(enablePin, OUTPUT);
        pinMode(buttonInputPin, INPUT);
        pinMode(greenLEDPin, OUTPUT);
        openSwitch();
    }

    void openSwitch() {
        digitalWrite(enablePin, 0);
        digitalWrite(greenLEDPin, 0);
        isPowerPlugOpen = true;
    }

    void closeSwitch() {
        digitalWrite(enablePin, 1);
        digitalWrite(greenLEDPin, 1);
        isPowerPlugOpen = false;
    }

    bool checkButton() {
      int reading = digitalRead(buttonInputPin);

      // if the input just went from HIGH and LOW and we've waited long enough
      // to ignore any noise on the circuit, toggle the output pin and remember
      // the time
      if (reading == LOW && previous == HIGH && millis() - timeToToggle > debounce) {
        if (buttonState == LOW) {
          buttonState = HIGH;
          closeSwitch();
          log("Switch button turns on the power!\n");
        } else {
          buttonState = LOW;
          openSwitch();
          log("Switch button turns off the power!\n");
        }

        timeToToggle = millis();
      }

      previous = reading;

      return (buttonState == HIGH);
    }

    bool isSwitchOpen() {
        return isPowerPlugOpen;
    }

protected:
    int enablePin;
    int greenLEDPin;
    int buttonInputPin;
    int buttonState;
    int previous;
    long timeToToggle;
    long debounce;
    bool isPowerPlugOpen;
};

// Step 2: Allocate resource with unique ID
PowerPlug powerPlug("switch");

// Timer-controlled read in of digital input
void timerCallback(void *pArg) {
  powerPlug.checkButton();
}

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.config(ip, gateway, subnet_mask, dns);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    log(".");
  }
  log("\nWiFi connected\n");

  // setup powerPlug
  powerPlug.setup();

  // Start the server
  server.begin();
  log("Server started\n");

  // Print the IP address
  log("Local IP: %s\n", WiFi.localIP().toString().c_str());

  // Step 3: Add observer
  rest.add_observer(&powerPlug);

  // Configure timer
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, TIMER_CYCLE_IN_MS, true);
}

void loop() {

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);

}
