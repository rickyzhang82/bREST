/*
  This a simple example of the bREST Library for the ESP8266 WiFi chip.
  See the README file for more details.

  Written in 2015 by Marco Schwartz under a GPL license.
*/
/*
  This sample code demo a caculator resource hosted in ESP8266 chip. It returns sum value from request.
  It demos:
  - how to use observer pattern for customized resource class
  - how to take arbitary number of parameter and value from URL.
  - how to return an arbitary JSON string back to client.
  Update by Ricky Zhang in 2018
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
class CalculatorResource: public Observer {
public:
    CalculatorResource(String resource_id): Observer(resource_id) {}
    virtual ~CalculatorResource(){}
    // override call back function
    void update(HTTP_METHOD method, String parms[], String value[], int parm_count, bREST* rest) override {
        Serial.println("*************************************");
        Serial.println("fire SerialPort update()!");
        Serial.print("HTTP Method:");
        Serial.println(bREST::get_method(method));
        Serial.println("Parameters and Value:");
        float sum = 0;
        // Iterate parameter array and value array
        for (int i = 0; i < parm_count; i++) {
            Serial.print(parms[i]);
            Serial.print(" = ");
            Serial.println(value[i]);
            sum += value[i].toFloat();
        }
        Serial.println("*************************************");
        // Send back JSON message to client.
        rest->start_json_msg();
        rest->append_key_value_pair_to_json(String("message"), String("CalculatorResource get fire up!"));
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("code"), CODE_OK);
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("sum"), sum);
        rest->end_json_msg();
    }
};

// Step 2: Allocate resource with unique ID
CalculatorResource myESP8266Calculator("calc");

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
  rest.add_observer(&myESP8266Calculator);
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
