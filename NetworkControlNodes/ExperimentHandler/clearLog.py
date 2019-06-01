import MySQLdb
import arrow
import random

myDB = MySQLdb.connect(host='localhost', port=3306, user='root', passwd='hurricane', db='logger_data')
cursor = myDB.cursor()
cursor.execute('DROP TABLE log;')
cursor.execute('CREATE TABLE IF NOT EXISTS log (timestamp CHAR(25) NOT NULL PRIMARY KEY, message CHAR(50));')
myDB.commit()
