import sys
import argparse
import time
import struct
from RF24 import RF24, RF24_PA_LOW
import paho.mqtt.client as mqtt
import json;
import RPi.GPIO as GPIO;

import Adafruit_MCP3008 as MCP

radio = RF24(22, 0)

mcp = MCP.MCP3008(clk=21, cs=18, miso=19, mosi=20)

usleep = lambda x: time.sleep(x/1000000.0)
payload = [0.0]
infos = {"temperature": 0, "humidity": 0, "failure": 0, "alarm": 0}


def boot():
    lastTime = 0;
    radio.startListening();
    update = time.monotonic_ns()
    hasChange = False;
    while 1:
        if update < time.monotonic_ns() - 1000000000 + lastTime:
            if(hasChange is True):
               send_mqtt();
               hasChange = False;
            radio.stopListening()

            print("send Update:")
            print("MCP: 1:{}, 2: {}, 3: {}".format(mcp.read_adc(0), mcp.read_adc(1), mcp.read_adc(2)))
            start_timer = time.monotonic_ns()
            tosend = 0.0;

            tosend = 0.1 + mcp.read_adc(0);
            buffer = struct.pack("<f", tosend)
            result0 = radio.write(buffer)

            tosend = 0.2;
            if(mcp.read_adc(1) > 500):
                tosend += 1;
            buffer = struct.pack("<f", tosend)
            result1 = radio.write(buffer)

            tosend = 0.3;
            if(mcp.read_adc(2) > 500):
                tosend += 1;
            buffer = struct.pack("<f", tosend)
            result2 = radio.write(buffer)

            end_timer = time.monotonic_ns()
            lastTime = (end_timer - start_timer);

            if not result0 and not result1 and not result2:
                print("Transmission failed or timed out")
                infos["failure"] += 1

            else:
                print(
                    "Transmission successful! Time to Transmit: "
                    "{} us. Sent: {}".format(
                        (end_timer - start_timer) / 1000,
                        0
                    )
                )
            radio.startListening()
            update = time.monotonic_ns()
        has_payload, pipe_number = radio.available_pipe()
        if has_payload:
            buffer = radio.read(radio.payloadSize)
            payload[0] = struct.unpack("<f", buffer[:4])[0]
            parse = str(payload[0]);
            lst = parse.split(".");
            arr = False;
            if(lst[1][1] == "9"):
               arr = True;
            lst[1] = lst[1][0];
            if(arr is True):
               lst[1] = int(lst[1]) + 1;
            print(lst);
            if(int(lst[0] == 99)):
                infos["failure"] += 1
                print("99 error");
            else:
                if int(lst[1]) == 1:
                    infos["temperature"] =int(lst[0]);
                    hasChange = True;
                elif int(lst[1]) == 2:
                    infos["humidity"] = int(lst[0]);
                    hasChange = True;
                elif int(lst[1]) == 3:
                    infos["alarm"] = int(lst[0]);
                    hasChange = True;
                else:
                    infos["failure"] += 1
                    print(lst[1]);
  
        usleep(100)
        

def set_role():
   boot()
   return True

def send_mqtt():
    print(json.dumps(infos))
    if(client is not None):
        client.publish("test", json.dumps(infos));
        infos["failure"] = 0;


def on_connect(client, userdata, flags, rc):
  print("Connected with result code "+str(rc))

  
if __name__ == "__main__":

  
    if not radio.begin():
        raise RuntimeError("radio hardware is not responding")

    # For this example, we will use different addresses
    # An address need to be a buffer protocol object (bytearray)
    address = [b"1Node", b"2Node"]
    # It is very helpful to think of an address as a path instead of as
    # an identifying device destination

    print(sys.argv[0])  # print example name
    
    print("connect MQTT");
    try:
        client = mqtt.Client()
        client.connect("10.4.35.11");#10.4.35.11
        client.on_connect = on_connect
    except:
        print("can't connect to mqtt");

    # to use different addresses on a pair of radios, we need a variable to
    # uniquely identify which address this radio will use to transmit
    # 0 uses address[0] to transmit, 1 uses address[1] to transmit
    radio_number = False  # uses default value from `parser`
   
    # set the Power Amplifier level to -12 dBm since this test example is
    # usually run with nRF24L01 transceivers in close proximity of each other
    radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default

    # set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radio_number])  # always uses pipe 0

    # set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[not radio_number])  # using pipe 1

    # To save time during transmission, we'll set the payload size to be only
    # what we need. A float value occupies 4 bytes in memory using
    # struct.pack(); "<f" means a little endian unsigned float
    radio.payloadSize =  len(struct.pack("<f", payload[0]))

    # for debugging, we have 2 options that print a large block of details
    # (smaller) function that prints raw register values
    # radio.printDetails()
    # (larger) function that prints human readable data
    # radio.printPrettyDetails()

    try:
       set_role()
    except KeyboardInterrupt:
        print(" Keyboard Interrupt detected. Exiting...")
        GPIO.cleanup();
        radio.powerDown()
        sys.exit()

