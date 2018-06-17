from brest.config import Config
from brest.request import Request
import sys

if 2 == sys.version_info[0]:
    text = unicode
else:
    text = str

conf = Config()
conf.set_server_url('192.168.2.41')
conf.set_system_debug_on(True)
req = Request(conf)
parms = {'resource': 'pan_servo', 'timeout': '2'}
resource_id = text('proxy')
http_method = Request.HTTP_METHOD_GET
text, code = req.send(resource_id, parms, http_method)