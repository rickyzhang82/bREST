from __future__ import absolute_import, division, print_function, unicode_literals

import sys
import json

SERVER_IP = '192.168.2.41'
SERVER_PORT = 80
RESOURCE = 'switch'
PARM = 'open'


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


def change_swtich(is_open=True):
    if is_open:
        parm_value = 'true'
    else:
        parm_value = 'false'
    url = "http://%s:%d/%s/?%s=%s" % (SERVER_IP, SERVER_PORT, RESOURCE, PARM, parm_value)
    print("Start to invoke URL: " + url)
    conn = CT.HTTPConnection(SERVER_IP + ':' + text(SERVER_PORT))
    conn.request("PUT", url)
    response = conn.getresponse()
    msg = response.read().decode('utf-8')
    print("Response code: " + str(response.status))
    print("Return: " + msg)
    return msg


def get_swtich_status():
    url = "http://%s:%d/%s" % (SERVER_IP, SERVER_PORT, RESOURCE)
    print("Start to invoke URL: " + url)
    conn = CT.HTTPConnection(SERVER_IP + ':' + text(SERVER_PORT))
    conn.request("GET", url)
    response = conn.getresponse()
    msg = response.read().decode('utf-8')
    print("Response code: " + str(response.status))
    print("Return: " + msg)
    return msg


while True:
    get_swtich_status()
    command = input('Turn on (Y/N/Q)?\n')
    if command.upper() == 'Y':
        change_swtich(False)
    elif command.upper() == 'N':
        change_swtich(True)
    else:
        break
