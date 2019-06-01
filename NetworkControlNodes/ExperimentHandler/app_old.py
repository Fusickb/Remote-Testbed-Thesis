
#!/usr/bin/env python
## -*- coding: utf-8 -*-
import os
import tornado.ioloop
import tornado.options
import tornado.httpserver
import sys
from NSFlogger import NSFlogger
from NSFsettings import *
from NSFviews import *
from DB_status import DB_status

# Assign handler to the server root  (127.0.0.1:PORT/)
application = tornado.web.Application([
    (r"/api/postexperiment/", PostExperiment),
    (r"/robots.txt", tornado.web.StaticFileHandler, {'path' : '/robots.txt'})
], debug=True,static_path=STATIC_PATH,cookie_secret=TORNADO_SECRET,xsrf_cookies=XSRF_COOKIES,)



if __name__ == "__main__":
    # Setup the server
    args = sys.argv
    args.append("--log_file_prefix=/var/log/tornado.log")
    tornado.options.parse_command_line(args)
    application.listen(HTTP_PORT)

    try:
        tornado.ioloop.IOLoop.instance().start()
    except KeyboardInterrupt:
        print('Server Shutdown!')
