/*
  bREST library for Arduino. A fork from aREST repo.
*/
#ifndef bREST_H
#define bREST_H

#include <stdarg.h>

#include "aREST.h"

// Set maximum length of URL, eg "/pin1/?mode=digital&value=high". Default is 256.
#ifndef MAX_URL_LENGTH
#define MAX_URL_LENGTH          256
#endif

// Set maximum number of URL parameters. Default is 10.
#ifndef MAX_NUM_PARMS
#define MAX_NUM_PARMS           10
#endif

// Set maximum number of observer resources. Default is 20.
#ifndef MAX_NUM_RESOURCES
#define MAX_NUM_RESOURCES       20
#endif

// Set maximum length of HTTP body. Default is 1.
#ifndef MAX_HTTP_BODY_LENGTH
#define MAX_HTTP_BODY_LENGTH    1
#endif

typedef enum {
    STATE_START,
    STATE_IGNORE,
    STATE_IGNORE_URI,
    STATE_ACCEPT_URI,
    STATE_OVERFLOW_URI,
    STATE_ACCEPT_BODY,
    STATE_OVERFLOW_BODY,
    STATE_IN_GET_METHOD_G,
    STATE_IN_GET_METHOD_E,
    STATE_IN_GET_METHOD_T,
    STATE_IN_PUT_METHOD_P,
    STATE_IN_PUT_METHOD_U,
    STATE_IN_PUT_METHOD_T,
    STATE_IN_FIRST_SPACE,
    STATE_IN_URI,
    STATE_IN_FIRST_CR,
    STATE_IN_SECOND_CR,
    STATE_IN_FIRST_LF,
    STATE_IN_SECOND_LF,
    STATE_IN_BODY
} PARSER_STATE;

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_PUT,
    HTTP_METHOD_UNSET
} HTTP_METHOD;

typedef enum {
    CODE_OK                             = 200,
    CODE_ERROR_NO_VALID_DATA            = 501,
    CODE_ERROR_URL_PARSING_OVERFLOW     = 502,
    CODE_ERROR_INVALID_URL              = 503,
    CODE_ERROR_NO_OBSERVERS_ACTIVATED   = 504,
    CODE_ERROR_INVALID_COMMAND          = 505
} MESSAGE_STATUS_CODE;

//define bREST class
class bREST;

/**
 * @brief The Observer class is an abstract class for subscribed resource.
 * @details The virtual pure method update() is a call back method for corresponding RESTful API call.
 */
class Observer {
protected:
    // unique resource ID
    String id;

    /**
     * @brief find_parm find the index of parameters for key
     * @param parms parameter arrays
     * @param parm_count the number of parameter array
     * @param key parameter key
     * @return index of parameter if key is found. Otherwise, return -1
     */
    int find_parm(String parms[], int parm_count, const String& key) {
        int found_index = -1;
        for (int i = 0; i < parm_count; i++) {
            if (parms[i].equalsIgnoreCase(key)) {
                found_index = i;
                break;
            }
        }
        return found_index;
    }

public:
    Observer(String id) {
        this->id = id;
    }

    virtual ~Observer() {}
    /**
     * @brief update a call back method by bREST class.
     * @param method HTTP method of RESTful request
     * @param parms an array of parameter of RESTful request
     * @param value an array of value of RESTful request
     * @param parm_count number of element in parameter array. It assumes that the number of element between parameter array and value array are the same.
     * @param rest bREST object for appending returned JSON message
     */
    virtual void update(HTTP_METHOD method, String parms[], String value[], int parm_count, bREST* rest) = 0;

    /**
     * @brief get_resource_id get resource ID
     * @return a string of resource ID
     */
    String get_id() {
        return id;
    }

};

class bREST: public aREST {

protected:
    PARSER_STATE parser_state;
    PARSER_STATE uri_final_state;
    PARSER_STATE http_body_final_state;
    HTTP_METHOD http_method;
    String http_url;
    unsigned int url_length_counter;
    unsigned int body_length_counter;
    String parms[MAX_NUM_PARMS];
    String value[MAX_NUM_PARMS];
    String resource_id;
    unsigned int parm_counter;
    Observer* observer_list[MAX_NUM_RESOURCES];
    unsigned int observer_counter;
    unsigned char http_body[MAX_HTTP_BODY_LENGTH];

public:
    bREST():aREST() {
        reset_uri_state_vars();
        observer_counter = 0;
    }

    /**
     * @brief bREST constructor
     * @param rest_remote_server REST server IP
     * @param rest_port REST server port number
     */
    bREST(char* rest_remote_server, int rest_port): aREST(rest_remote_server, rest_port) {
        reset_uri_state_vars();
        reset_body_state_vars();
        observer_counter = 0;
    }

    virtual ~bREST() override {}

    /**
     * @brief add_observer add new resource to REST server
     * @param new_resource
     * @return true if successful. Otherwise, false
     */
    bool add_observer(Observer* new_resource) {
        if (observer_counter < MAX_NUM_RESOURCES) {
            observer_list[observer_counter++] = new_resource;
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief append_key_value_pair_to_json Add key value pair to returned JSON message.
     * @param key
     * @param value
     */
    void append_key_value_pair_to_json(const String& key, bool value) {
        append_key_to_json(key);
        addToBuffer(value, false);
    }

    /**
     * @brief append_key_value_pair_to_json Add key value pair to returned JSON message.
     * @param key
     * @param value
     */
    void append_key_value_pair_to_json(const String& key, const String& value) {
        append_key_to_json(key);
        addToBuffer(value, true);
    }

    /**
     * @brief append_key_value_pair_to_json Add key value pair to returned JSON message.
     * @param key
     * @param value
     */
    void append_key_value_pair_to_json(const String& key, int value) {
        append_key_to_json(key);
        addToBuffer(value, false);
    }

    /**
     * @brief append_key_value_pair_to_json Add key value pair to returned JSON message.
     * @param key
     * @param value
     */
    void append_key_value_pair_to_json(const String& key, float value) {
        append_key_to_json(key);
        addToBuffer(value, false);
    }

    /**
     * @brief append_key_value_pair_to_json Add key value pair to returned JSON message.
     * @param key
     * @param value
     */
    void append_key_value_pair_to_json(const String& key, const char* value) {
        append_key_to_json(key);
        addToBuffer(value, true);
    }

    /**
     * @brief append_comma_to_json Add comma separator to JSON message.
     */
    void append_comma_to_json() {
        addToBufferF(F(","));
    }

    /**
     * @brief start_json_msg start JSON message.
     */
    void start_json_msg() {
        // wrap JSON left bracket
        addToBufferF(F("{"));
    }

    /**
     * @brief end_json_msg end JSON message.
     */
    void end_json_msg() {
        // wrap JSON right bracket
        addToBufferF(F("}\r\n"));
    }

    /**
     * @brief get_http_body Get parsed HTTP body
     * @return HTTP body buffer
     */
    unsigned char* get_http_body() {
        return this->http_body;
    }

    /**
     * @brief get_http_body_length Get the length of HTTP body
     * @return  length of HTTP body
     */
    unsigned int get_http_body_length() {
        return this->body_length_counter;
    }

    /**
     * @brief get_method get string value of http method
     * @param method
     * @return a string of http method
     */
    static String get_method(HTTP_METHOD method) {
        switch(method) {
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_PUT:
            return "PUT";
        default:
            return "UNSET";
        }
    }

protected:
    /**
     * @brief process parses one and only one Request-Line i.e. (Method SP Request-URI SP HTTP-Version CRLF). Disregard the rest of HTTP conversation.
     *
     * @details
     *  - Support two methods: GET and PUT. Disregard the rest of HTTP methods.
     *      + GET method refers to READ value,
     *      + PUT method refers to WRITE value.
     *  - Support two types of request URI:
     *      + absoluteURI. i.e. http://wherever.com/pin1/?mode=digital&value=high
     *      + abs_path. i.e. /pin1/?mode=digital&value=high
     *
     * @ref [RFC2616](https://www.ietf.org/rfc/rfc2616.txt)
     * @param c one character from character stream
     */
    void process(char c) override {
        switch(parser_state) {
        // The length of URI is too long.
        case STATE_OVERFLOW_URI:
            if (c == '\r')
                parser_state = STATE_IN_FIRST_CR;
            break;

        case STATE_START:
            if (c == 'G')
                parser_state = STATE_IN_GET_METHOD_G;
            else if (c == 'P')
                parser_state = STATE_IN_PUT_METHOD_P;
            else
                parser_state = STATE_IGNORE_URI;
            break;
        // parse GET
        case STATE_IN_GET_METHOD_G:
            parser_state = (c == 'E')? STATE_IN_GET_METHOD_E: STATE_IGNORE_URI;
            break;
        case STATE_IN_GET_METHOD_E:
            parser_state = (c == 'T')? STATE_IN_GET_METHOD_T: STATE_IGNORE_URI;
            break;

        // parse PUT
        case STATE_IN_PUT_METHOD_P:
            parser_state = (c == 'U')? STATE_IN_PUT_METHOD_U: STATE_IGNORE_URI;
            break;
        case STATE_IN_PUT_METHOD_U:
            parser_state = (c == 'T')? STATE_IN_PUT_METHOD_T: STATE_IGNORE_URI;
            break;

        // end of parsing PUT and GET
        case STATE_IN_GET_METHOD_T:
            parser_state = (c == ' ')? STATE_IN_FIRST_SPACE: STATE_IGNORE_URI;
            http_method = HTTP_METHOD_GET;
            break;

        case STATE_IN_PUT_METHOD_T:
            parser_state = (c == ' ')? STATE_IN_FIRST_SPACE: STATE_IGNORE_URI;
            http_method = HTTP_METHOD_PUT;
            break;

        case STATE_IN_FIRST_SPACE:
            if (c == 'h' || c == '/') {
                parser_state = STATE_IN_URI;
                url_length_counter += 1;
                http_url += c;
            } else
                parser_state = STATE_IGNORE_URI;
            break;

        case STATE_IN_URI:
            if (url_length_counter >= MAX_URL_LENGTH)  {
                parser_state = STATE_OVERFLOW_URI;
                uri_final_state = STATE_OVERFLOW_URI;
                break;
            }

            if (c == ' ') {
                parser_state = STATE_IGNORE;
                uri_final_state = STATE_ACCEPT_URI;
            } else if (c == '\r') {
                reset_uri_state_vars();
                parser_state = STATE_IN_FIRST_CR;
            } else if (c == '\n') {
                reset_uri_state_vars();
                parser_state = STATE_IGNORE;
            } else {
                url_length_counter += 1;
                http_url += c;
            }
            break;

        case STATE_IGNORE_URI:
            if (c == '\r') {
                reset_uri_state_vars();
                parser_state = STATE_IN_FIRST_CR;
            }
            break;

        case STATE_IGNORE:
            if (c == '\r')
                parser_state = STATE_IN_FIRST_CR;
            break;

        case STATE_IN_FIRST_CR:
            if (c == '\n')
                parser_state = STATE_IN_FIRST_LF;
            else
                parser_state = STATE_IGNORE;
            break;

        case STATE_IN_FIRST_LF:
            if (c == '\r')
                parser_state = STATE_IN_SECOND_CR;
            else
                parser_state = STATE_IGNORE;
            break;

        case STATE_IN_SECOND_CR:
            if (c == '\n')
                parser_state = STATE_IN_SECOND_LF;
            else
                parser_state = STATE_IGNORE;
            break;

        case STATE_IN_SECOND_LF:
            parser_state = STATE_IN_BODY;
            http_body[body_length_counter++] = c;
            break;

        case STATE_IN_BODY:
            if (body_length_counter >= MAX_HTTP_BODY_LENGTH) {
                parser_state = STATE_OVERFLOW_BODY;
                http_body_final_state = STATE_OVERFLOW_BODY;
                break;
            }
            http_body[body_length_counter++] = c;
            break;

        case STATE_OVERFLOW_BODY:
            if (c == '\r')
                parser_state = STATE_IN_FIRST_CR;
            break;

        } // end of switch
    }

    /**
     * @brief send_command executes command based on method and request URI from HTTP request
     * @param headers
     * @param decodeArgs
     * @return
     */
    bool send_command(bool headers, bool decodeArgs) override {

        if (uri_final_state == STATE_OVERFLOW_URI) {
            append_msg_overflow(headers);
            return true;
        }

        if (uri_final_state != STATE_ACCEPT_URI) {
            append_msg_invalid_request(headers);
            return true;
        }

        if(decodeArgs)
            urldecode(http_url);   // Modifies http url

        if(!parse_url()) {
            append_msg_invalid_request(headers);
            return true;
        }

#if DEBUG
       log("bREST::send_command() -- Method: %s", bREST::get_method(http_method).c_str());
        for(int i = 0; i < parm_counter; i++) {
            log(", Parm: %s = %s", parms[i].c_str(), value[i].c_str());
        }
        log("\n");
#endif

        if(!notify_observers(headers)) {
            if(headers) {
                append_http_header(true);
                addToBufferF(F("{\"message\":\"Request has been processed. But no observers are activated!\",\"code\":504}\r\n"));
            } else {
                addToBufferF(F("\"message\":\"Request has been processed. But no observers are activated!\",\"code\":504\n"));
            }
        }

        return true;

    }

    /**
     * @brief notify_observers notifies subscribed observer to process HTTP request.
     * @param headers should include HTTP headers
     * @return true if trigger any observer update. Otherwise, false.
     */
    bool notify_observers(bool headers) {
        bool is_observer_fired = false;

        for (int i = 0 ; i < observer_counter; i++) {

            Observer* p_resource = observer_list[i];

            if(p_resource->get_id().equalsIgnoreCase(resource_id)) {
                is_observer_fired = true;

                if(headers)
                    append_http_header(true);

                // fire resource call back
                p_resource->update(http_method, parms, value, parm_counter, this);
            }
        }

        return is_observer_fired;
    }

    void append_http_header(bool isOK) {
        if(isOK)
            addToBufferF(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
        else
            addToBufferF(F("HTTP/1.1 500\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
    }

    void append_msg_overflow(bool headers) {
        if (headers) {
            append_http_header(false);
            addToBufferF(F("{\"message\":\"URL parsing overflow!\",\"code\":502}\r\n"));
        } else {
            addToBufferF(F("\"message\":\"URL parsing overflow!\",\"code\":502\n"));
        }
    }

    void append_msg_invalid_request(bool headers) {
        if (headers) {
            append_http_header(false);
            addToBufferF(F("{\"message\":\"Invalid URL request!\",\"code\":503}\r\n"));
        } else {
            addToBufferF(F("\"message\":\"Invalid URL request!\",\"code\":503\n"));
        }
    }

    void append_key_to_json(const String& key) {
        addToBufferF(F("\""));
        addToBuffer(key, false);
        addToBufferF(F("\":"));
    }

    /**
     * @brief parse_url parse resource, parameter and value.
     * @return true if url is valid.Otherwise, false.
     */
    bool parse_url() {

#if DEBUG
        log("bREST::Parse URL: %s\n", http_url.c_str());
#endif

        // preprocess http://host:port
        if (http_url.startsWith("http://")) {
            // remove "http://"
            http_url.remove(0, 7);
            int index_of_slash = http_url.indexOf('/');
            if (-1 == index_of_slash)
                return false;
            http_url.remove(0, index_of_slash);
        }

        // process abs_path

        // remove '/' preceding to abs_path
        http_url.remove(0, 1);

        int index_of_slash = http_url.indexOf('/');
        // no parms are provided
        if (-1 == index_of_slash) {
            resource_id = http_url;
            parm_counter = 0;
            return true;
        }

        resource_id = http_url.substring(0, index_of_slash);

        // remove the resource from url
        http_url.remove(0, index_of_slash);

        // parm list must start with '/?'
        if(!http_url.startsWith("/?"))
            return false;

        // remove '/?'
        http_url.remove(0, 2);

        // process parameters and value
        int index_of_amp;
        do{
            index_of_amp = http_url.indexOf('&');
            String statement = (index_of_amp == -1)? http_url: http_url.substring(0, index_of_amp);
            int index_of_assign = statement.indexOf('=');
            if (-1 == index_of_assign)
                return false;
            parms[parm_counter] = statement.substring(0, index_of_assign);
            if (statement.length() + 1 == parms[parm_counter].length())
                return false;
            value[parm_counter] = statement.substring(index_of_assign + 1);

            // remove statment and &
            if (index_of_amp != -1)
                http_url.remove(0, index_of_amp + 1);

            parm_counter += 1;
        } while(index_of_amp != -1 && parm_counter < MAX_NUM_PARMS);

        return true;
    }

    void reset_status() override {
        aREST::reset_status();
        reset_uri_state_vars();
        reset_body_state_vars();
    }

    /**
     * @brief reset_state_vars reset state variable for every line of HTTP conversation.
     */
    void reset_uri_state_vars() {
        parser_state = STATE_START;
        uri_final_state = STATE_START;
        http_method = HTTP_METHOD_UNSET;
        http_url = String("");
        url_length_counter = 0;
        parm_counter = 0;
    }

    void reset_body_state_vars() {
        body_length_counter = 0;
        http_body_final_state = STATE_START;
        memset((void*)http_body, 0, MAX_HTTP_BODY_LENGTH);
    }
};

#endif // BREST_H

