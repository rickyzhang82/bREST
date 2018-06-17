from __future__ import absolute_import, division, print_function, unicode_literals

import sys
import json
from brest.config import Config

if 2 == sys.version_info[0]:
    # TODO Python 2 hack
    import httplib as CT
    # TODO Python 2 hack
    import urllib as URLLIB
    text = unicode
else:
    import http.client as CT
    import urllib.parse as URLLIB
    text = str


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class Request:
    HTTP_METHOD_GET = 'GET'
    HTTP_METHOD_PUT = 'PUT'

    RESPONSE_KEY_MESSAGE = 'message'

    def __init__(self, conf):
        self._server = conf.get_value(Config.CFG_KEY_SERVER_URL)
        self._port = conf.get_value(Config.CFG_KEY_SERVER_PORT)
        self._is_debug_on = conf.get_value(Config.CFG_KEY_SYSTEM_DEBUG_ON)

    def send(self, resource_id, parm_dict=dict(), http_method=HTTP_METHOD_GET):
        assert isinstance(resource_id, text)
        assert isinstance(parm_dict, dict)

        url = 'http://' + self._server + ':' + self._port + '/' + resource_id
        parms_string = ''

        is_first = True
        for k in parm_dict.keys():
            if is_first:
                parms_string = '/?'
            else:
                parms_string = parms_string + '&'
            v = parm_dict.get(k)
            parms_string = parms_string + k + '=' + v
            # flip boolean flag
            if is_first:
                is_first = False

        url = url + parms_string

        headers = {"Content-Type": "application/json"}
        conn = CT.HTTPConnection(self._server + ':' + self._port)
        conn.request(http_method, url, "", headers)
        response = conn.getresponse()
        response_text, response_code = Request.parse_response(response)
        if self._is_debug_on:
            print('Request : ' + http_method + ' ' + url)
            print('Response code: ' + text(response_code))
            print('Response text: ' + response_text)
        return response_text, response_code

    @staticmethod
    def parse_response(response):
        response_code = response.status
        response_text = response.read().decode('utf-8')
        if 500 == response_code:
            # parse error code and message
            error_response_object = json.load(response_text)
            message = error_response_object.get(Request.RESPONSE_KEY_MESSAGE)
            if message:
                eprint("Error: error code (" + str(response_code) + ") -- " + message)
        return response_text, response_code
