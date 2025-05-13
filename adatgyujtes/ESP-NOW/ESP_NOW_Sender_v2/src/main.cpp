#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "BMI270.h"
#include "I2C.h"
#include <FastLED.h>

//#define DEBUG

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

uint8_t receiverAddress[] = {0x98, 0x3D, 0xAE, 0x42, 0xD2, 0x78};

CRGB leds[NUM_LED];

esp_now_send_status_t sendStatus;

/*typedef struct
{
  float Xacc;
  float Yacc;
  float Zacc;

  uint32_t time;

}accelerometer_t;*/

typedef struct
{
  float accX;
  float accY;
  float accZ;

  float battVoltage;

  uint8_t accConf;
  uint8_t pwrCtrl;
  uint8_t pwrConf;
  uint8_t accRange;

  uint32_t measuredTime;
  bool isNewData;

}structMessage_t;

//accelerometer_t accelerometer;

esp_now_peer_info_t peerInfo;

uint8_t accelerometerArray[6];
int16_t result;
structMessage_t myData;

bool measureEnable = false, prevMeasureEnable = false;

uint32_t sampleTime = 0, measureStartTime = 0;

void readButton(uint8_t pin)
{
  
  static uint8_t counter = 0;
  static bool btnPressed = false;

  if(!digitalRead(pin))
  {
    counter++;

    if(counter >= DEBOUNCE_TIME)
    {
      btnPressed = true;
    }
  }
  else
  {
    counter = 0;
  }

  if(digitalRead(pin) && btnPressed)
  {
    btnPressed = false;
    measureEnable = !measureEnable;
    measureStartTime = millis();
  }
}

void sendEndOfMeasure()
{
  myData.accX = -100.0;
  myData.accY = -100.0;
  myData.accZ = -100.0;
  myData.measuredTime = -100.0;
  myData.battVoltage = -100.0;
  myData.isNewData = true;

  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

}

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LED).setCorrection(TypicalLEDStrip); 
  FastLED.setBrightness(5);

  delay(5000);

  #ifdef DEBUG  
    Serial.begin(115200);
  #endif

  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK)
  {
    while(1)
    {
      leds[0] = CRGB::Red; FastLED.show(); delay(500);
      leds[0] = CRGB::Black; FastLED.show(); delay(500);
    }
  }

  leds[0] = CRGB::Green; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  //esp_now_register_send_cb(onDataSent);

  memcpy(peerInfo.peer_addr, receiverAddress, sizeof(receiverAddress));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    while(1)
    {
      leds[0] = CRGB::Orange; FastLED.show(); delay(500);
      leds[0] = CRGB::Black; FastLED.show(); delay(500);
    }
  }

  leds[0] = CRGB::GreenYellow; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  InitI2C(SDA_PIN, SCL_PIN);
  

  if(!bmi270Begin())
  {
    while(1)
    {
      leds[0] = CRGB::Red; FastLED.show(); delay(100);
      leds[0] = CRGB::Black; FastLED.show(); delay(100);
    }
  }

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

  myData.accConf = readACC_CONF();
  myData.pwrCtrl = readPWR_CTRL();
  myData.pwrConf = readPWR_CONF();
  myData.accRange = readAcc_Range();

}

void loop()
{
  if(digitalRead(INT1) && measureEnable)
  {
    prevMeasureEnable = measureEnable;
    readAccelerometerData(accelerometerArray);

    #ifdef DEBUG
      uint8_t rslt;
      rslt = readInterrupt();
      if(rslt != 0x80)
      {
        Serial.printf("Interrupt 1 status: %02x\r\n", rslt);
      }
      

    #endif
    /*result = (accelerometerArray[1] << 8u) | (accelerometerArray[0]);
    accelerometer.Xacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[3] << 8u) | (accelerometerArray[2]);
    accelerometer.Yacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[5] << 8u) | (accelerometerArray[4]);
    accelerometer.Zacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    accelerometer.time = millis() - measureStartTime;*/

    result = (accelerometerArray[1] << 8u) | (accelerometerArray[0]);
    myData.accX = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[3] << 8u) | (accelerometerArray[2]);
    myData.accY = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    result = (accelerometerArray[5] << 8u) | (accelerometerArray[4]);
    myData.accZ = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

    myData.measuredTime = millis() - measureStartTime;
    myData.battVoltage = (float)((analogRead(BATTERY) * VREF) / ADC_RESOLUTION) / VOLTAGE_DIVIDER;
    myData.isNewData = true;

    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
  }

  if(millis() - sampleTime >= MEASURE_TIME)
  {
    readButton(BTN);
    sampleTime = millis();
  }

  if(!measureEnable && prevMeasureEnable)
  {
    prevMeasureEnable = false;
    sendEndOfMeasure();
  }

}
