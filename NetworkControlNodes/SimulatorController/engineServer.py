from http.server import BaseHTTPRequestHandler, HTTPServer
import serial
import json
serial  = serial.Serial('/dev/ttyACM0')
class EngineHandler(BaseHTTPRequestHandler):
    def _set_response(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def _set_bad_response(self):
        self.send_response(500)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        try:
            post_data = json.loads(self.rfile.read(content_length).decode('utf-8'))
        except Exception:
            self._set_bad_response()
            return		
        if post_data['command'] == 'on':
            serial.write('50,1'.encode('ASCII'))
        elif post_data['command'] == 'off':
            serial.write('50,0'.encode('ASCII'))
        else:	
            self._set_bad_response()
            return
        self._set_response()

def run(server_class=HTTPServer, handler_class=EngineHandler, port=8080):
    server_address = ('0.0.0.0', port)
    httpd = server_class(server_address, handler_class)
    print('Starting httpd...\n')
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print('Stopping httpd...\n')

if __name__ == '__main__':
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()
