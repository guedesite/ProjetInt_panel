/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */


#include <SPI.h>
#include "printf.h"
#include "RF24.h"


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 2
#define DHTTYPE    DHT11
#define LEDPIN 3
#define SENSORPIR A5 

#define BUZZER 5

DHT_Unified dht(DHTPIN, DHTTYPE);

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin

// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
float payload = 0.0;

uint32_t delayMS;
long Update;

boolean ledHigh =false;
long UpdateLed;

bool alarm = false;
bool alarmActive = false;
bool alarmSound = false;
int alarmValue = 20;
long alarmUpdate = 0;
bool alarmHigh = false;

int lastTime = 0;

void setup() {

  Serial.begin(9600);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSORPIR, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {
      digitalWrite(LEDPIN, HIGH);
      delay(100);
      digitalWrite(LEDPIN, LOW);
      delay(100);
      } // hold in infinite loop
  }

  
  radioNumber = 1;

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

  radio.startListening(); // put radio in RX mode

  Update = millis();
  UpdateLed = millis();

  Serial.println("setup sensor");

  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  Serial.println("done");

      digitalWrite(LEDPIN, HIGH);
      delay(100);
      digitalWrite(LEDPIN, LOW);
      delay(100);
      digitalWrite(LEDPIN, HIGH);
      delay(100);
      digitalWrite(LEDPIN, LOW);
      delay(100);
      digitalWrite(LEDPIN, HIGH);
      delay(100);
      digitalWrite(LEDPIN, LOW);
} 


void loop() {

   if(UpdateLed < millis() - 100 and ledHigh) {
        digitalWrite(LEDPIN, LOW);
        ledHigh = false;
   }

  
  if (analogRead(SENSORPIR) > 2 and alarmActive) {
        if(alarmSound) {
          if(alarmUpdate < micros() - alarmValue) {
            if(alarmHigh) {
              alarmHigh = false;
              digitalWrite(BUZZER, LOW);
              alarmUpdate = micros();
            } else {
              alarmHigh = true;
              digitalWrite(BUZZER, HIGH);
              alarmUpdate = micros();
            }
          }
        }
        alarm = true;
  } else if(alarm) {
      if(alarmSound) {
        digitalWrite(BUZZER, LOW);
      }
      alarm = false;
  }
   
  if(Update < millis() - 1000 + (lastTime/1000)) {
    radio.stopListening();
      Update = millis();
      
      sensors_event_t event;

      float tosend0 = 0.3; 
      if(alarm) {
          tosend0 += 1.0;
      }
      float tosend1 = 0.1; 
      dht.temperature().getEvent(&event);
      if (isnan(event.temperature)) {
        tosend1 += 99.0;
      } else {
        tosend1 += (float)event.temperature;
      }
      
      float tosend2 = 0.2; 
      dht.humidity().getEvent(&event);
      if (isnan(event.relative_humidity)) {
        tosend2 += 99.0;
      } else {
        tosend2 += (float)event.relative_humidity;
      }
      unsigned long start_timer = micros();                    // start the timer
      bool report0 = radio.write(&tosend0, sizeof(float));      // transmit & save the report
      bool report1 = radio.write(&tosend1, sizeof(float));
      bool report2 = radio.write(&tosend2, sizeof(float));
      unsigned long end_timer = micros();                      // end the timer
      lastTime = end_timer - start_timer;
      if (report1 and report2 and report0) {
        Serial.print(F("Transmission successful! "));          // payload was delivered
        Serial.print(F("Time to transmit = "));
        Serial.print(lastTime);                 // print the timer result
        Serial.print(F(" us. Sent: "));
        Serial.print(tosend1);
        Serial.print(" and ");
        Serial.print(tosend2);  // print payload sent
        Serial.print(" and ");
        Serial.println(tosend0);
      } else {
        Serial.println(F("Transmission failed or timed out")); // payload was not delivered
      }
      radio.startListening();
  }

   uint8_t pipe;
    if (radio.available(&pipe)) {             // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
      radio.read(&payload, bytes);            // fetch payload from FIFO

      String str = String(payload);
      String cmd = str.substring(str.length()-2, str.length()-1);
      String value = str.substring(0,str.length()-3);

      Serial.print(F("Received cmd: "));
      Serial.print(cmd);
      Serial.print(":");
      Serial.println(value);

      if(cmd == "3") {
        alarmActive = value == "1" ? true : false;
      } else if(cmd == "2") {
        alarmSound = value == "1" ? true : false;
      } else if(cmd == "1") {
        alarmValue = ( (255 * value.toInt()) / 1023 );
      }

      if(!ledHigh) {
        digitalWrite(LEDPIN, HIGH);
        UpdateLed = millis();
        ledHigh = true;
      }
    }
  
} // loop
