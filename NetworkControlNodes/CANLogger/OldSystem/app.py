
#!/usr/bin/env python
## -*- coding: utf-8 -*-
import os
import tornado.ioloop
import tornado.options
import tornado.httpserver
import sys
from NSFlogger import LogHandler
from NSFsettings import *
from NSFviews import *
from DB_status import DB_status

LOG_HANDLER = LogHandler()
STREAM_HANDLER = LogHandler()
# Assign handler to the server root  (127.0.0.1:PORT/)
application = tornado.web.Application([
    ("/", BaseHandler),
    (r"/log/api/deleteExperiment/([A-Za-z0-9\(\)]+)/", DeleteExperiment),
    (r"/log/api/getExperiment/([A-Za-z0-9\(\)]+)/([0-9]+)/", GetExperiment),
    (r"/log/api/getExperiment/([A-Za-z0-9\(\)]+)/plotdata/axlebasedvehiclespeed/", GetAxleBasedVehicleSpeed),
    (r"/log/api/getExperiment/([A-Za-z0-9\(\)]+)/plotdata/vin/", GetVin),
    (r"/log/api/getExperiment/([A-Za-z0-9\(\)]+)/plotdata/govspeed/", GetGovSpeed),
    (r"/log/start/", StartLogging, dict(loghandler=LOG_HANDLER)), #takes post {'experimentname': 'example'}
    ("/stream/start/", StartCanStream)
], debug=True,static_path=STATIC_PATH,cookie_secret=TORNADO_SECRET,xsrf_cookies=XSRF_COOKIES,)


if __name__ == "__main__":
    # Setup the server
    args = sys.argv
    args.append("--log_file_prefix=/var/log/tornado.log")
    tornado.options.parse_command_line(args)
    if (HTTPS):
        http_server = tornado.httpserver.HTTPServer(application, ssl_options={
            "certfile": CERTFILE,
            "keyfile": KEYFILE,
        })
        http_server.listen(HTTPS_PORT)
        # application.listen(HTTP_PORT)
        # REDIRECT_APP.listen(HTTP_PORT)
    else:
        application.listen(HTTP_PORT)
    try:
        tornado.ioloop.IOLoop.instance().start()
    except KeyboardInterrupt:
        print('Server Shutdown!')




