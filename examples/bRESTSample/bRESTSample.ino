/*
  This a simple example of the bREST Library for the ESP8266 WiFi chip.
  See the README file for more details.

  Written in 2015 by Marco Schwartz under a GPL license.
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <bREST.h>

// Create bREST instance
bREST rest = bREST();

// WiFi parameters
const char* ssid = "your_ssid";
const char* password = "your_password";
IPAddress ip(192, 168, 2, 41);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet_mask(255, 255, 255, 0);
IPAddress dns(192, 168, 2, 1);

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Step1: Define customized resource by inheriting Observer
//        Override call back method update()
class SerialPortResource: public Observer {
public:
    SerialPortResource(String resource_id): Observer(resource_id) {}
    virtual ~SerialPortResource(){}
    // override call back function
    void update(HTTP_METHOD method, String parms[], String value[], int parm_count) override {
        Serial.println("*************************************");
        Serial.println("fire SerialPort update()!");
        Serial.print("HTTP Method:");
        Serial.println(get_method(method));
        Serial.println("Parameters and Value:");
        for (int i = 0; i < parm_count; i++) {
            Serial.print(parms[i]);
            Serial.print(" = ");
            Serial.println(value[i]);
        }
        Serial.println("*************************************");
    }
};

// Step 2: Allocate resource with unique ID
SerialPortResource mySerialPort("serial0");

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.config(ip, gateway, subnet_mask, dns);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  // Step 3: Add observer
  rest.add_observer(&mySerialPort);
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
