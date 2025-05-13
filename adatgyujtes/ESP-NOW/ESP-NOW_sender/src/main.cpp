#include <Arduino.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <esp_now.h>
#include "BMI270.h"
#include "I2C.h"
#include <FastLED.h>

#define ACCELEROMTER_SENSITIVITY  4096.0f
#define G_FORCE                   9.81f

#define NUM_LED    1u
#define DATA_PIN   10u

#define SDA_PIN    8u
#define SCL_PIN    9u

#define INT1       18u

uint8_t receiverAddress[] = {0x98, 0x3D, 0xAE, 0x42, 0xD2, 0x78};

CRGB leds[NUM_LED];

xSemaphoreHandle xStatusSemaphore;
QueueHandle_t    dataQueue;
esp_now_send_status_t sendStatus;

typedef struct
{
  float Xacc;
  float Yacc;
  float Zacc;

  uint32_t time;

}accelerometer_t;

typedef struct
{
  float accX;
  float accY;
  float accZ;

  uint32_t measuredTime;
  bool isNewData;

}structMessage_t;


accelerometer_t accelerometer;

esp_now_peer_info_t peerInfo;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{

  portBASE_TYPE xHighPriorityTaskWoken = pdFALSE;

  sendStatus = status;
  xSemaphoreGiveFromISR(xStatusSemaphore, &xHighPriorityTaskWoken);
  
  if(xHighPriorityTaskWoken == pdTRUE)
  {
    taskYIELD();
  }
}

void dataReadTask(void *pvParameters)
{
  uint8_t accelerometerArray[6];
  int16_t result;
  structMessage_t myData;

  for(;;)
  {
    if(digitalRead(INT1))
    {
      //Serial.println("Interrupt, " + String(millis()));
      readAccelerometerData(accelerometerArray);
      
      result = (accelerometerArray[1] << 8u) | (accelerometerArray[0]);
      accelerometer.Xacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

      result = (accelerometerArray[3] << 8u) | (accelerometerArray[2]);
      accelerometer.Yacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

      result = (accelerometerArray[5] << 8u) | (accelerometerArray[4]);
      accelerometer.Zacc = result * (G_FORCE / ACCELEROMTER_SENSITIVITY);

      accelerometer.time = millis();

      /*myData.accX = accelerometer.Xacc;
      myData.accY = accelerometer.Yacc;
      myData.accZ = accelerometer.Zacc;
      myData.measuredTime = accelerometer.time;
      myData.isNewData = true;

      esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));*/

      xQueueSendToBack(dataQueue, (void *) &accelerometer, portMAX_DELAY);
      taskYIELD();
    }
  }
}

void dataSendTask(void *pvParameters)
{
  accelerometer_t dataToSend;
  structMessage_t myData;
  portBASE_TYPE xStatus;
  for(;;)
  {
    xStatus = xQueueReceive(dataQueue, &dataToSend, (5 / portTICK_RATE_MS));
    if(xStatus)
    {
      myData.accX = dataToSend.Xacc;
      myData.accY = dataToSend.Yacc;
      myData.accZ = dataToSend.Zacc;
      myData.measuredTime = dataToSend.time;
      myData.isNewData = true;

      esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

      if(result != ESP_OK)
      {
        leds[0] = CRGB::Red; FastLED.show(); delay(100);
        leds[0] = CRGB::Black; FastLED.show(); delay(100);
      }

    }
  }
}

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LED).setCorrection(TypicalLEDStrip); 
  FastLED.setBrightness(5);

  //Serial.begin(115200);

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

  esp_now_register_send_cb(onDataSent);

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
  pinMode(INT1, INPUT);

  if(!bmi270Begin())
  {
    while(1)
    {
      leds[0] = CRGB::Red; FastLED.show(); delay(100);
      leds[0] = CRGB::Black; FastLED.show(); delay(100);
    }
  }

  leds[0] = CRGB::Cyan; FastLED.show(); delay(500);
  leds[0] = CRGB::Black; FastLED.show(); delay(500);

  uint8_t buffer[6];
  setLowPowerMode(buffer);
  setLowPowerACCMeasure();

  xStatusSemaphore = xSemaphoreCreateBinary();
  dataQueue = xQueueCreate(5, sizeof(accelerometer));

  xTaskCreate(dataReadTask, "DataRead_Task", 1024, NULL, 1, NULL);
  xTaskCreate(dataSendTask, "DataSend_Task", 1024, NULL, 2, NULL);
  //xTaskCreate();

  //vTaskStartScheduler();
  for(;;);
}

void loop()
{

}
