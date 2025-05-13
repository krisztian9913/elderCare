#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "BMI270.h"
#include <FastLED.h>
#include <MAX30105.h>
#include <heartRate.h>

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

#define POSITIVE_TRESHOLD   20.0f  //for magnitude
#define NEGATIVE_TRESHOLD   10.0f  //for magnitude

#define RATE_SIZE       4u

#define SSID            "SSID"
#define PASSWD          "PWD"

#define MQTT_BROKER     "hivemq.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883
#define USER_NAME       "User name"
#define USER_PASWD      "Passwword"


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

MAX30105 particleSensor;

WiFiClientSecure espClient;
PubSubClient client(espClient);

uint8_t rates[RATE_SIZE];
uint8_t rateSpot = 0;
uint32_t lastBeat = 0;

float beatsPerMinute = 0.0;
int16_t beatAvg;

uint8_t accelerometerArray[6];
int16_t result;
float accX, accY, accZ, magnitude;
structMessage_t myData;

bool hrSampling = false;

float heartRate = 0.0;

uint32_t delta = 0;
uint32_t lastHRSample = 0;
uint32_t irValue;
uint16_t beatDetectCounter = 0;

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

  static uint32_t prevTime = 0;
  static bool isHigherPozTreshld = false; 
  float frequency;
  static uint8_t sampleCounter = 0;
  static uint32_t lastSample = 0;

  if(millis() - lastSample >= 1500)
  {
    sampleCounter = 0;
  }


  if(mag >= POSITIVE_TRESHOLD)
  {
    if(!isHigherPozTreshld)
    {
      frequency = 1/((millis() - prevTime)/1000.0);
      isHigherPozTreshld = true;
      //Serial.println("Frekvencia: " + String(frequency) + " Hz");
      prevTime = millis();

      if(frequency >= 50.0)
      {
        sampleCounter++;
        if(sampleCounter >= 2)
        {
          //Serial.println("Fall detected");
          sampleCounter = 0;
          return true;
        }
        lastSample = millis();
      }
    }
  }

  if(magnitude < POSITIVE_TRESHOLD)
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

  if(!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    leds[0] = CRGB::Red; FastLED.show(); delay(10000);
    leds[0] = CRGB::Black; FastLED.show(); delay(2500);
  }
  else
  {
    leds[0] = CRGB::Orange; FastLED.show(); delay(10000);
    leds[0] = CRGB::Black; FastLED.show(); delay(2500);

    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
  }

  

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
  //pox.update();
  

  uint32_t currentMillis =  millis();

  switch (currentState)
  {
  case IDLE:

    if(currentMillis - lastHRSample >= 60000)
    {
      currentState = HR_SAMPLING;
    }
    else if(digitalRead(INT1))
    {
      currentState = ACCEl_SAMPLING;
    }
    break;
  
  case HR_SAMPLING:

    irValue = particleSensor.getIR();

    if(checkForBeat(irValue))
    {

      beatDetectCounter++;

      delta = currentMillis - lastBeat;
      lastBeat = currentMillis;
  
      beatsPerMinute = 60 / (delta / 1000.0);
  
      if(beatsPerMinute < 255 && beatsPerMinute > 20)
      {
        rates[rateSpot++] = (uint8_t)beatsPerMinute;
        rateSpot %= RATE_SIZE;
  
        beatAvg = 0;
        for(uint8_t i = 0; i < RATE_SIZE; i++)
        {
          beatAvg += rates[i];
        }
  
        beatAvg /= RATE_SIZE;
      }
  
      myData.heartRate = beatAvg;
      myData.timeStamp = currentMillis;
  
      data = "Time stamp: " + String(myData.timeStamp) + ", Heart rate: " + String(myData.heartRate) + "BPM";

      sendDataToMqtt(data);
    }

    
    if(beatDetectCounter == 5)
    {
      currentState = IDLE;
      beatDetectCounter = 0;
      lastHRSample = currentMillis;
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

    if(fallDetector(magnitude))
    {
      currentState = FALL_DETECTED;
    }
    else
    {
      currentState = IDLE;
    }

    break;

  case FALL_DETECTED:
    myData.fallDetected = true;
    myData.timeStamp = currentMillis;

    data = "Time stamp: " + String(myData.timeStamp) + ", Falldetected";

    sendDataToMqtt(data);

    myData.fallDetected = false;

    currentState = HR_SAMPLING;
    break;
  
  default:
    currentState = IDLE;
    break;
  }

}

