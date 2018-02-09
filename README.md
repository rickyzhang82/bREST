<h1>bREST</h1>


## What
bRest is an extension of [aREST project](https://github.com/marcoschwartz/aREST). Its main purpose is to provide truly flexible RESTful API on ESP8266 chip.

bREST supports the following [HTTP RFC 2616](https://www.ietf.org/rfc/rfc2616.txt) standards:
- Parse one and only one line of HTTP request `Method SP Request-URI SP HTTP-Version CRLF`. Disregard all the noises.
- Support two methods: GET and PUT. Disregard the rest of HTTP methods.
  + GET method refers to get resource status.
  + PUT method refers to update resource status.
- Support two types of request URI:
  + absoluteURI. i.e. http://wherever.com/pin1/?mode=digital&value=high
  + abs_path. i.e. /pin1/?mode=digital&value=high
- Support one and only one level of URI, which represents the unique ID of your resource.
  + For example, a HTTP request `PUT http://wherever.com/servo1/?angle=120 HTTP/1.1`. It means update resource `servo1` with angle `120`.
- Support unlimited number of parameters and value pair in URL.
  + For example, a HTTP request `PUT http://wherever.com/pan_tilt_camera/?pan_angle_delta=20&tilt_angle_delta=-10 HTTP/1.1`. It means update resource pan tilt camera with pan angle delta `20` and tilt angle delta `-10`.

Compared to aREST, bREST is more
- Fault tolerance. If HTTP request doesn't follow the requirements above, it outputs errors and stop processing.
- Resource oriented. Your customized class is a resource. A true thinking in RESTful API.
- Object oriented. Your customized class inherits from a observer. bREST will automatically invoke your call back method. Observer pattern is more readable than a function pointer.
- Flexible input and output. Zero restriction on the number of input parameters from URL. Zero restriction on the return of JSON.

## How
First, clone bREST repo to your `Arduino/libraries`

Secondly, design your customized class by inheriting class `Observer` and overriding call back method `update()`. Conceptually, your class should be one of resource such as servo, serial port or even a pin.

Thirdly, add your customized resource object to `bREST` observers list. `bREST` will invoke call back method if HTTP request matches your resource ID.

In the sample code below, we created a customized resource called `CaculatorResource`. It takes numeric parameters from HTTP request and returns summation result in JSON. A simple URL from any web browser such as `http://your_esp8266_ip/calc/?input1=1.2&input2=23&input3=-2` will return a JSON message:

```
{"message":"CaculatorResource get fire up!","sum":22.2}
```
Any illegal HTTP request or invoking resource that is not registered in `bREST` observer list will return proper error JSON message.

See Step 1, Step 2 and Step 3 in sample below:
```C++
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
class CaculatorResource: public Observer {
public:
    CaculatorResource(String resource_id): Observer(resource_id) {}
    virtual ~CaculatorResource(){}
    // override call back function
    void update(HTTP_METHOD method, String parms[], String value[], int parm_count, bREST* rest) override {
        Serial.println("*************************************");
        Serial.println("fire SerialPort update()!");
        Serial.print("HTTP Method:");
        Serial.println(get_method(method));
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
        rest->append_key_value_pair_to_json(String("message"), String("CaculatorResource get fire up!"));
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("sum"), sum);

    }
};

// Step 2: Allocate resource with unique ID
CaculatorResource myESP8266Caculator("calc");

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

  // Step 3: Add caulator resource to observer list
  rest.add_observer(&myESP8266Caculator);
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

```
