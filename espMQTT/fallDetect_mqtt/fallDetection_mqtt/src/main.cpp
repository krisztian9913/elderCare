#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "BMI270.h"
#include <FastLED.h>

#define ACCELEROMTER_SENSITIVITY  4096.0f
#define G_FORCE                   9.81f

#define NUM_LED    1u
#define DATA_PIN   10u

#define SDA_PIN    8u
#define SCL_PIN    9u

#define INT1       18u
#define BATTERY    3u

#define VREF            3.3f
#define ADC_RESOLUTION  4096.0f
#define VOLTAGE_DIVIDER 0.5f

#define BTN             7u
#define DEBOUNCE_TIME   2u  //20 ms
#define MEASURE_TIME    10u

#define POSITIVE_TRESHOLD   15.0f  //for magnitude
#define NEGATIVE_TRESHOLD   3.0f  //for magnitude

#define SSID            "SSID"
#define PASSWD          "PWD"

#define MQTT_BROKER     "hivemq.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883
#define USER_NAME       "User name"
#define USER_PASWD      "Password"


typedef struct
{
  float heartRate;

  float magnitude;

  bool fallDetected;

  uint32_t timeStamp;

}structMessage_t;

typedef enum
{
  IDLE, 
  HR_SAMPLING,
  ACCEl_SAMPLING,
  FALL_DETECTED
}SystemState;


CRGB leds[NUM_LED];

WiFiClientSecure espClient;
PubSubClient client(espClient);

uint8_t rateSpot = 0;
uint32_t lastBeat = 0;

float beatsPerMinute = 0.0;
int16_t beatAvg;

uint8_t accelerometerArray[6];
int16_t result;
float accX, accY, accZ, magnitude;
structMessage_t myData;


SystemState currentState = IDLE;

String data;

void reconnect(void)
{
  while(!client.connected())
  {
      String clientID = "ESP32_Care";
      clientID += String(random(0xffff), HEX);

      if(client.connect(clientID.c_str(), USER_NAME, USER_PASWD))
      {
        leds[0] = CRGB::Blue; FastLED.show();
        delay(500);
        leds[0] = CRGB::Black; FastLED.show();
        delay(500);
      }
      else
      {
        leds[0] = CRGB::Red; FastLED.show();
        delay(50);
        leds[0] = CRGB::Black; FastLED.show();
        delay(50);
      }
  }
}

void sendDataToMqtt(String dataToMqtt)
{
  if(!client.connected())
  {
    reconnect();
  }

  client.publish("esp-F9", dataToMqtt.c_str());

  client.disconnect();
}


float calculateMagnitude(float ax, float ay, float az)
{
  float mag;

  float axSquare = pow(ax, 2);
  float aySquare = pow(ay, 2);
  float azSquare = pow(az, 2);

  mag = sqrt(axSquare + aySquare + azSquare);

  return mag;
}

bool fallDetector(float mag)
{
  static uint32_t prevTime = 0, negPrevTime = 0;
  static bool isHigherPozTreshld = false; 
  float frequency;
  static uint8_t sampleCounter = 0;
  static uint32_t lastSample = 0;

  if(millis() - lastSample >= 700)
  {
    sampleCounter = 0;
  }

  if(mag >= POSITIVE_TRESHOLD)
  {
    if(!isHigherPozTreshld)
    {
      isHigherPozTreshld = true;

      if(millis() - prevTime <= 200)
      {
        sampleCounter++;
        if(sampleCounter >= 3)
        {
          sampleCounter = 0;
          return true;
        }
        lastSample = millis();
      }
      prevTime = millis();      
    }
  }

  if(mag < POSITIVE_TRESHOLD)
  {
    if(isHigherPozTreshld)
    {
      isHigherPozTreshld = false;
    }
  }
  return false;
}

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LED).setCorrection(TypicalLEDStrip); 
  FastLED.setBrightness(5);

  delay(5000);

  #ifdef DEBUG  
    Serial.begin(115200);
  #endif

  if(!bmi270Begin())
  {
    while(1)
    {
      leds[0] = CRGB::Red; FastLED.show(); delay(100);
      leds[0] = CRGB::Black; FastLED.show(); delay(100);
    }
  }

  delay(2000);

  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PASSWD);

  while(WiFi.status() != WL_CONNECTED)
  {
    leds[0] = CRGB::Green; FastLED.show();
    delay(100);
    leds[0] = CRGB::Black; FastLED.show();
    delay(100);
  }

  leds[0] = CRGB::Black; FastLED.show();

  espClient.setInsecure();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  reconnect();

  

  leds[0] = CRGB::Green; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  //esp_now_register_send_cb(onDataSent);

  leds[0] = CRGB::GreenYellow; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  

  delay(100);

  

  pinMode(INT1, INPUT);
  pinMode(BTN, INPUT);

  leds[0] = CRGB::Cyan; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  //uint8_t buffer[6];
  //setLowPowerMode(buffer);
  //setLowPowerACCMeasure();

  setLowPowerACCMeasure(dataRate400);

  #ifdef DEBUG
    delay(100);
    readACC_CONF();
    readPWR_CTRL();
    readPWR_CONF();
  #endif

}

void loop()
{
  
  uint32_t currentMillis =  millis();

  switch (currentState)
  {
  case IDLE:

    if(digitalRead(INT1))
    {
      currentState = ACCEl_SAMPLING;
    }

    if(currentMillis % 10000 == 0)
    {
      if(myData.magnitude <= 1.00)
      {
        data = "Time stamp: " + String(myData.timeStamp) + ", Magnitude: " + String(myData.magnitude);
        sendDataToMqtt(data);
      }
      
    }

    break;

  case ACCEl_SAMPLING:
    readAccelerometerData(accelerometerArray);

    result = (accelerometerArray[1] << 8u) | (accelerometerArray[0]);
    accX = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[3] << 8u) | (accelerometerArray[2]);
    accY = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[5] << 8u) | (accelerometerArray[4]);
    accZ = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    magnitude = calculateMagnitude(accX, accY, accZ);

    myData.magnitude = magnitude;
    myData.timeStamp = currentMillis;

    /*if(magnitude <= 1.0)
    {
      data = "Time stamp: " + String(myData.timeStamp) + "Fall detected < 1.00";
      sendDataToMqtt(data);
      leds[0] = CRGB::Green; FastLED.show(); 
      //currentState = FALL_DETECTED;
    }*/

    if(fallDetector(magnitude))
    {
      leds[0] = CRGB::Green; FastLED.show(); 
      currentState = FALL_DETECTED;
    }
    else
    {
      currentState = IDLE;
      leds[0] = CRGB::Black; FastLED.show(); 
    }

    break;

  case FALL_DETECTED:
    myData.fallDetected = true;
    myData.timeStamp = currentMillis;

    data = "Time stamp: " + String(myData.timeStamp) + ", Falldetected";

    sendDataToMqtt(data);

    myData.fallDetected = false;

    currentState = IDLE;
    break;
  
  default:
    currentState = IDLE;
    break;
  }

}

