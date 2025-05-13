import paho.mqtt.client as paho
from paho import mqtt
import datetime
import thingy as Thingy91
import json
import time

thingies = {}
#thingy = Thingy91.Thingy91("F98C")


def on_connect(client, userdata, flags, rc, properties=None):
    print("Connack received with code %s" %rc)
    client.subscribe("nrf-F98C/GNSS", qos=1)
    client.subscribe("espMAC/Care", qos=1)
    client.subscribe("mobilApp/data", qos=1)

def on_publish(client, userdata, mid, properties=None):
    print("mid: " + str(mid))

def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))

def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))

    if msg.topic == "nrf-F98C/GNSS":
        try:
            device_id = get_device_id_fom_topic(msg)
            date = datetime.datetime.now()
            print("GNSS", date)
            #thingy.processMSG(msg)
            thingies[device_id].processMSG(msg)
        except Exception as e:
            print(f"Error in nrf topic, error: {e}")

        return

    if msg.topic == "espMAC/Care":
        print("esp")
        return

    if msg.topic == "mobilApp/data":

        try:

            device_id, command = get_device_id(msg)

            if command == "getGNSS":
                print(repr(thingies[device_id]))
                send(device_id)
                #TODO: publish client data

            elif command == "getGNSSDay":
                items = thingies[device_id].get_today_GNSS()
                send_today_gnss(items)


            else:
                print("Unknown command: {command}")

        except Exception as e:
            print(f"Error in massage process: {e}")


def on_disconnect(client, userdata, rc):
    print("Disconnected %s" %rc)
    client.connect("hivmqURL.hivemq.cloud", 8883) #Change hivemq url

def get_device_id_fom_topic(msg):
    topic = msg.topic.strip()

    parts = [p.strip() for p in topic.split("/")]

    device_id, command = parts

    return device_id
def get_device_id(msg):
    payload = msg.payload.decode().strip()
    print(f"Incoming message: {payload}")

    parts = [p.strip() for p in payload.split(",")]

    if len(parts) != 2:
        print("Wrong massage format")
        return

    device_id, command = parts

    return device_id, command

def send(device_id):
    client.publish("server/sendData", repr(thingies[device_id]), qos=1)
    pass

def send_today_gnss(items):
    try:
        data = json.loads(items)

        filtered_data = [
            {
                "time (utc)": item["time (utc)"],
                "latitude": item["latitude"],
                "longitude": item["longitude"]
            }
            for item in data
            if "time (utc)" in item and "latitude" in item and "longitude" in item
        ]

        client.publish("server/sendData", json.dumps(filtered_data), qos=1)

    except json.JSONDecodeError:
        print("Wrong JSON input!")
    except Exception as e:
        print(f"Error: {e}")

def add_new_thingy(ID):
    if id in thingies:
        print(f"{ID} already exists")
    else:
        thingy = Thingy91.Thingy91(ID)
        thingies[ID] = thingy

    print("Thingies: ", thingies)

client = paho.Client(client_id="python_mqtt", userdata=None, protocol=paho.MQTTv311)

client.on_connect = on_connect

client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)

client.username_pw_set("UserName", "Password") #Change user name, passowrd
client.connect("hivmqURL.hivemq.cloud", 8883) #Change hivemq url

client.on_subscribe = on_subscribe
client.on_message = on_message
client.on_publish = on_publish
client.on_disconnect


if __name__ == "__main__":
    add_new_thingy("nrf-F98C")

    client.loop_forever()