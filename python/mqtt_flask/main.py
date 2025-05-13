from flask import Flask, render_template, jsonify
import paho.mqtt.client as mqtt
import re
import json
import threading
import os

app = Flask(__name__)
DATA_FILE = "gps_data.json"

if not os.path.exists(DATA_FILE):
    with open(DATA_FILE, "w") as f:
        json.dump({"path":[]}, f)

def save_data(lat, lon):
    with open(DATA_FILE, "r") as f:
        data = json.load(f)

    data["path"].append([lat, lon])

    with open(DATA_FILE, "w") as f:
        json.dump(data, f)

def get_data():
    with open(DATA_FILE, "r") as f:
        return json.load(f)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/coords")
def coords():
    return jsonify(get_data())

def on_connect(client, userdata, flags, rc):
    print("MQTT csatlakozva")
    client.subscribe("devacademy/publish/doboKTopic", qos=1)


def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8", errors="ignore").strip('\x00').strip()
        match = re.search(r"Time \(UTC\): ([\d:]+), Latitude: ([\d.]+), Longitude: ([\d.]+)", payload)
        if match:
            lat = float(match.group(2))
            lon = float(match.group(3))
            print(f"🛰️ Koordináta: {lat}, {lon}")
            if lat > 0.0 and lon > 0.0:
                save_data(lat, lon)

    except Exception as e:
        print("⚠️ Hiba MQTT-ben:", e)

def mqtt_thread():
    client = mqtt.Client(client_id="python_mqtt", userdata=None, protocol=mqtt.MQTTv311)
    client.on_connect = on_connect
    client.tls_set(tls_version=mqtt.ssl.PROTOCOL_TLS)
    client.username_pw_set("DK_test", "Password1234")
    client.connect("e2f51f5d457d46a88721445a6d521eaf.s1.eu.hivemq.cloud", 8883)
    client.on_subscribe = on_subscribe
    client.on_message = on_message
    client.subscribe("devacademy/publish/doboKTopic", qos=1)

    client.loop_forever()

threading.Thread(target=mqtt_thread, daemon=True).start()

if __name__ == "__main__":
    #mqtt_thread()
    app.run(debug=True)