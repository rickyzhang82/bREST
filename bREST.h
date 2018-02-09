#ifndef bREST_H
#define bREST_H

#include "aREST.h"

#define DEBUG 0

const int MAX_URL_LENGTH    = 1024;
const int MAX_NUM_PARMS     = 10;
const int MAX_NUM_RESOURCES = 20;

typedef enum {
    STATE_START,
    STATE_ACCEPT,
    STATE_IGNORE,
    STATE_OVERFLOW,
    STATE_IN_GET_METHOD_G,
    STATE_IN_GET_METHOD_E,
    STATE_IN_GET_METHOD_T,
    STATE_IN_PUT_METHOD_P,
    STATE_IN_PUT_METHOD_U,
    STATE_IN_PUT_METHOD_T,
    STATE_IN_FIRST_SPACE,
    STATE_IN_URI
} PARSER_STATE;

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_PUT,
    HTTP_METHOD_UNSET
} HTTP_METHOD;

class Observer {
protected:
    String id;
public:
    Observer(String id) {
        this->id = id;
    }

    virtual ~Observer() {}
    virtual void update(HTTP_METHOD method, String parms[], String value[], int parm_count) = 0;

    /**
     * @brief get_resource_id get resource ID
     * @return a string of resource ID
     */
    String get_id() {
        return id;
    }

    /**
     * @brief get_method get string value of http method
     * @param method
     * @return a string of http method
     */
    String get_method(HTTP_METHOD method) {
        switch(method) {
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_PUT:
            return "PUT";
        default:
            return "UNSET";
        }
    }
};

class bREST: public aREST {

protected:
    PARSER_STATE parser_state;
    HTTP_METHOD http_method;
    String http_url;
    unsigned int url_length_counter;
    String parms[MAX_NUM_PARMS];
    String value[MAX_NUM_PARMS];
    String resource_id;
    unsigned int parm_counter;
    Observer* observer_list[MAX_NUM_RESOURCES];
    unsigned int observer_counter;

public:
    bREST():aREST() {
        reset_state_vars();
        observer_counter = 0;
    }

    bREST(char* rest_remote_server, int rest_port): aREST(rest_remote_server, rest_port) {
        reset_state_vars();
        observer_counter = 0;
    }

    virtual ~bREST() override {}

    /**
     * @brief subscribe add new resource to REST server
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

protected:
    /**
     * @brief process method parses one and only one Request-Line i.e. (Method SP Request-URI SP HTTP-Version CRLF). Disregard the rest of HTTP conversation.
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
        // once accept state, ignore rest of HTTTP conversation
        case STATE_ACCEPT:
            break;
        // The length of URI is too long.
        case STATE_OVERFLOW:
            break;
        case STATE_START:
            if (c == 'G')
                parser_state = STATE_IN_GET_METHOD_G;
            else if (c == 'P')
                parser_state = STATE_IN_PUT_METHOD_P;
            else
                parser_state = STATE_IGNORE;
            break;
        // parse GET
        case STATE_IN_GET_METHOD_G:
            parser_state = (c == 'E')? STATE_IN_GET_METHOD_E: STATE_IGNORE;
            break;
        case STATE_IN_GET_METHOD_E:
            parser_state = (c == 'T')? STATE_IN_GET_METHOD_T: STATE_IGNORE;
            break;

        // parse PUT
        case STATE_IN_PUT_METHOD_P:
            parser_state = (c == 'U')? STATE_IN_PUT_METHOD_U: STATE_IGNORE;
            break;
        case STATE_IN_PUT_METHOD_U:
            parser_state = (c == 'T')? STATE_IN_PUT_METHOD_T: STATE_IGNORE;
            break;

        // end of parsing PUT and GET
        case STATE_IN_GET_METHOD_T:
            parser_state = (c == ' ')? STATE_IN_FIRST_SPACE: STATE_IGNORE;
            http_method = HTTP_METHOD_GET;
            break;
        case STATE_IN_PUT_METHOD_T:
            parser_state = (c == ' ')? STATE_IN_FIRST_SPACE: STATE_IGNORE;
            http_method = HTTP_METHOD_PUT;
            break;

        case STATE_IN_FIRST_SPACE:
            if (c == 'h' || c == '/') {
                parser_state = STATE_IN_URI;
                url_length_counter += 1;
                http_url += c;
            } else
                parser_state = STATE_IGNORE;
            break;

        case STATE_IN_URI:
            if (url_length_counter > MAX_URL_LENGTH)  {
                parser_state = STATE_OVERFLOW;
                break;
            }

            if (c == ' ')
                parser_state = STATE_ACCEPT;
            else if (c == '\n')
                reset_state_vars();
            else {
                url_length_counter += 1;
                http_url += c;
            }
            break;

        case STATE_IGNORE:
            // once get LF, return to start state
            if (c== '\n')
                parser_state = STATE_START;
            break;
        }

    }

    /**
     * @brief send_command executes command based on method and request URI from HTTP request
     * @param headers
     * @param decodeArgs
     * @return
     */
    bool send_command(bool headers, bool decodeArgs) override {

        if (parser_state == STATE_OVERFLOW) {
            append_msg_overflow(headers);
            return true;
        }

        if (parser_state != STATE_ACCEPT) {
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
        Serial.println("**********************************");
        Serial.print("Resource:");
        Serial.println(resource);
        for(int i = 0; i < parm_counter; i++) {
            Serial.print("Parm:");
            Serial.print(parms[i]);
            Serial.print("=");
            Serial.println(value[i]);
        }
        Serial.println("**********************************");
#endif

        notify_observers();

        if(headers)
            append_http_header(true);
        addToBufferF(F("{\"message\": \"Finished!\"}\r\n"));
        return true;

    }

    void notify_observers() {
        for (int i = 0 ; i < observer_counter; i++) {
            Observer* p_resource = observer_list[i];
            if(p_resource->get_id().equalsIgnoreCase(resource_id)) {
                //TODO add buffer
                p_resource->update(http_method, parms, value, parm_counter);
            }
        }
    }

    void append_http_header(bool isOK) {
        if(isOK)
            addToBufferF(F("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
        else
            addToBufferF(F("HTTP/1.1 500\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"));
    }

    void append_msg_overflow(bool headers) {
        if (headers)
            append_http_header(false);
        addToBufferF(F("{\"message\": \"URL parsing overflow!\"}\r\n"));
    }

    void append_msg_invalid_request(bool headers) {
        if (headers)
            append_http_header(false);
        addToBufferF(F("{\"message\": \"Invalid URL request!\"}\r\n"));
    }

    /**
     * @brief parse_url parse resource, parameter and value.
     * @return true if url is valid.Otherwise, false.
     */
    bool parse_url() {

#if DEBUG
            Serial.print("Parse URL:");
            Serial.println(http_url);
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
        reset_state_vars();
    }

    void reset_state_vars() {
        parser_state = STATE_START;
        http_method = HTTP_METHOD_UNSET;
        http_url = String("");
        url_length_counter = 0;
        parm_counter = 0;
    }
};

#endif // BREST_H

