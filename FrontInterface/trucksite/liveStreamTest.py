import requests
LOGGER_IP="http://129.244.254.21:8080/stream/start/"
BRAKE_IP = "http://129.244.254.22:8080/set/speed/"
#interface = input("which interface? ")
#state = int(input("which state? "))
#jsonTest = {"streamstate": state, "interface": interface}
#requests.post(LOGGER_IP, json=jsonTest)
#response = requests.get(LOGGER_IP)
response = requests.post(BRAKE_IP,json={"Speed":50})
#print(response.status_code)
#print(response.text)
#print(response.json())