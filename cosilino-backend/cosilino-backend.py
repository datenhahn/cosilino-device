#!/usr/bin/env python
import json
import datetime
from pprint import pprint

import paho.mqtt.client as mqtt
from pymongo import MongoClient

OUT_TOPIC = "cosilino/+/status"

client = MongoClient('localhost', 27017)

# Get the sampleDB database
db = client.cosilino
stats = db['statistics']

def parse_message(msg):
    try:
        payload = json.loads(msg)
        payload['date'] = datetime.datetime.now()
        stats.insert(payload)
    except Exception as e:
        print("Exception, doing nothing: " + str(e))

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(OUT_TOPIC)

def on_message(client, userdata, msg):
    parse_message(str(msg.payload.decode("utf-8") ))

client = mqtt.Client(protocol=mqtt.MQTTv31)
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)
client.loop_forever()
