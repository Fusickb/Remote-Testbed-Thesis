import os

#Tornado settings
STATIC_PATH = os.path.join(os.path.dirname(__file__), "static")
TORNADO_SECRET = 'JUSTHIGHWINDS'
XSRF_COOKIES = False
BASEDIR=""

root = os.path.dirname(__file__)
template_root = os.path.join(root, 'templates')
blacklist_templates = ('layouts',)

#DB settings, this user will be created and granted permissions.
DB_USER_USER = 'nsfteam'
DB_USER_PASS = 'nsfteam2017'

#HTTP Settings
HTTPS = False
HTTPS_PORT = 8443
HTTP_PORT = 8080
#CERTFILE = BASEDIR + "/path/to/cert"
#KEYFILE = BASEDIR + "/path/to/key"

#Login settings
API_KEY = 'QWERTYUIOPASDFGHJKLZXCVBNM'

#jinja2 settings
JINJA2_SETTINGS = {
    'sidenav':False,
   'navbar':True,
}

#Arrow settings
HUMANIZE = True #If false, will use exact time settings below
DATE_FORMAT = 'YYYY-MM-DD HH:mm:ss'
TIMEZONE = 'US/Central'

#Number settings
MAX_NUMBER = 100 #inclusive

#Users settings:
USE_BCRYPT = True

LOG_DIR = 'experimentlogs/'
