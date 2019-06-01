#!/bin/bash

apt-get update
apt-get upgrade -y
cat apt-requirements | xargs apt-get install -y 

pip install -r requirements.txt

mysql_secure_installation
