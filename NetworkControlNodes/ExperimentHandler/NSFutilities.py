import arrow
import MySQLdb
from NSFsettings import *



def check_db():
    try:
        myDB = MySQLdb.connect(host='localhost', port=3306, user='root', passwd='hurricane', db='logger_data')
        cursor = myDB.cursor()
        cursor.execute('USE logger_data;')
        cursor.execute("SELECT user from mysql.user where user='{}';".format(DB_USER_USER))
        result = cursor.fetchone()
        return result and len(result) > 0
    except:
        return False

def setup_db():
    myDB = MySQLdb.connect(host='localhost', port=3306, user='root', passwd='hurricane')
    c = myDB.cursor()
    c.execute('CREATE DATABASE IF NOT EXISTS logger_data;')
    myDB.commit()
    c.execute('USE logger_data;')
    c.execute('CREATE TABLE IF NOT EXISTS `log` (timestamp varchar(32), message blob);')
    myDB.commit()
    try:
        c.execute("CREATE USER '{}'@'localhost' IDENTIFIED BY '{}';".format(DB_USER_USER, DB_USER_PASS))
    except:
        print ('creation of user defined in settings failed')
    c.execute("GRANT ALL PRIVILEGES ON logger_data.* TO '{}'@'localhost' WITH GRANT OPTION;".format(DB_USER_USER))
    myDB.commit()
    myDB.close()

def get_log_segment(timestamp, duration):
    myDB = MySQLdb.connect(host='localhost', port=3306, user=DB_USER_USER, passwd=DB_USER_PASS, db='logger_data')
    cursor = myDB.cursor()
    cursor.execute("SELECT timestamp, message FROM log WHERE timestamp>{} AND timestamp<{} ORDER BY timestamp ASC".format(timestamp,arrow.get(timestamp).replace(seconds=+int(duration)).timestamp))
    result = cursor.fetchall()
    return list(result)

def get_log_size():
    myDB = MySQLdb.connect(host='localhost', port=3306, user=DB_USER_USER, passwd=DB_USER_PASS, db='logger_data')
    cursor = myDB.cursor()
    cursor.execute("SELECT table_name AS `Table`, round(((data_length + index_length) / 1024 / 1024), 2) `Size in MB` FROM information_schema.TABLES WHERE table_schema = 'logger_data' AND table_name = 'log';")
    result = cursor.fetchone()
    return result

def log_message(message):
    myDB = MySQLdb.connect(host='localhost', port=3306, user=DB_USER_USER, passwd=DB_USER_PASS, db='logger_data')
    cursor = myDB.cursor()
    utc = arrow.utcnow()
    timestamp = utc.timestamp
    cursor.execute("INSERT INTO log VALUES ('{}','{}');".format(str(timestamp) + str(utc.microsecond), message))
    myDB.commit()

def log_message_block(block):
    myDB = MySQLdb.connect(host='localhost', port=3306, user=DB_USER_USER, passwd=DB_USER_PASS, db='logger_data')
    cursor = myDB.cursor()
    block_push = "INSERT INTO log VALUES {};".format(str(block)[1:-1])
    cursor.execute(block_push)
    myDB.commit()
