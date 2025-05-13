import re
import time
import paho.mqtt.client as paho
from paho import mqtt
import os
import folium

fileName = "data"
dataFile = " "

def on_connect(client, userdata, flags, rc, properties=None):
    global dataFile

    print("CONNACK received with code %s" %rc)

    fileNameTmp = fileName + ".txt"

    if os.path.isfile(fileNameTmp):
        i = 0
        while os.path.isfile(fileNameTmp):
            i += 1
            fileNameTmp = fileName + "(" + str(i) + ").txt"

    dataFile = fileNameTmp

def on_publish(client, userdata, mid, properties=None):
    print("mid: " + str(mid))

def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))

    string = msg.payload.decode("utf-8")
    cleandString = string.strip('\x00').strip().strip("'")
    print(cleandString)

    match = re.search(r"Time \(UTC\): ([\d:]+), Latitude: ([\d.]+), Longitude: ([\d.]+)", cleandString)

    if match:
        time_str = match.group(1)
        latitude = float(match.group(2))
        longitude = float(match.group(3))
        print(f"Idő: {time_str}, Szélesség: {latitude}, Hosszúság: {longitude}")

        value = "Idő (UTC): " + time_str + ", Szélesség: " + str(latitude) + ", Hosszúság: " + str(longitude) + "\n"

        f = open(dataFile, "a", newline="")
        f.truncate()
        f.write(value)
        f.close()
        makeMap(latitude, longitude)
    else:
        print("Nem sikerült adatokat kinyerni a payloadból:", cleandString)

client = paho.Client(client_id="python_mqtt", userdata=None, protocol=paho.MQTTv311)
client.on_connect = on_connect

client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)

client.username_pw_set("User name", "Password")
client.connect("hivemqURL.hivemq.cloud", 8883)

client.on_subscribe = on_subscribe
client.on_message = on_message
client.on_publish = on_publish

client.subscribe("devacademy/publish/doboKTopic", qos=1)

def makeMap(latitude, longitude):
    m = folium.Map(location=[latitude, longitude], zoom_start=13)

    folium.Marker(
        [latitude, longitude],
        popup="GNSS point",
        icon = folium.Icon(color="blue", icon="info-sign")
    ).add_to(m)

    m.save("terkep.html")
    print("Térkép elmentve: terkep.html")



if __name__ == '__main__':

    client.loop_forever()



