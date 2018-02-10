<h1>bREST</h1>


## What
bRest is an extension of [aREST project](https://github.com/marcoschwartz/aREST). Its main purpose is to provide truly flexible RESTful API in Arduino.

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
First, clone bREST repo to your `Arduino/libraries`.

Secondly, create your customized class by inheriting class `Observer`. Conceptually, your class should be one of resource such as servo, serial port or even a pin.

Thirdly, design your resource RESTful interface by overriding `update()` virtual method.

Last but not the least, add your customized resource object to `bREST` observers list. `bREST` will invoke proper call back method if HTTP request matches your resource ID.

Let's take a quick look into a sample. In the [sample code](https://github.com/rickyzhang82/bREST/blob/master/examples/1.ESP8266AsRESTfulCalculator/1.ESP8266AsRESTfulCalculator.ino), we created a customized resource called `CalculatorResource`. What it does is to take any numeric parameters from HTTP request and returns summation result in JSON.

A simple URL from any web browser such as `http://your_esp8266_ip/calc/?input1=1.2&input2=23&input3=-2` will return a JSON message:

```
{"message":"CalculatorResource get fire up!", "code":200 ,"sum":22.2}
```
However, any illegal HTTP request or invoking resource that is not registered in `bREST` observer list will return proper error JSON message.

As bREST's user, the first step in design is to abstract your RESTful API in terms of resource. In this case, calculator is a resource. When call back `update()`, bREST provides http method, parameters, value and also bREST object itself for constructing JSON message. See step 1 below:

```C++
...
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
        rest->start_json_msg();
        rest->append_key_value_pair_to_json(String("message"), String("CaculatorResource get fire up!"));
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("code"), CODE_OK);
        rest->append_comma_to_json();
        rest->append_key_value_pair_to_json(String("sum"), sum);
        rest->end_json_msg();
    }
};
```

When allocating resource, you should provide a unique resource ID. The resource ID will be served as the first level URI in http request.

```C++
// Step 2: Allocate resource with unique ID
CalculatorResource myESP8266Calculator("calc");
// Create bREST instance
bREST rest = bREST();
```

Don't forget to add your resource to bREST. Otherwise, you will see this error message `{"message": "Request has been processed. But no observers are activated!", "code":504}`.

```C++
void setup() {
    ...
    // Step 3: Add observer
    rest.add_observer(&myESP8266Calculator);
    ...
}
```

Happy Coding!

Ricky Zhang
