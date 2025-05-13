#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define POSITIVE_TRESHOLD   20.0f  //for magnitude
#define NEGATIVE_TRESHOLD   10.0f  //for magnitude

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

structMessage_t myData;
bool measureEnable = false, prevMeasureEnable = false;

uint32_t prevTime = 0, lastSample = 0;
uint8_t sampleCounter = 0;
bool isHigherPozTreshld = false, isLowerNegTreshld = false;
float magnitude = 0.0;
float frequency = 0.0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&myData, incomingData, sizeof(myData));
  /*Serial.print("Bytes received: ");
  Serial.println(len);*/
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

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    while(1);
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  prevMeasureEnable = true;

}

void loop()
{
  if(myData.isNewData)
  {
    myData.isNewData = false;

    magnitude = calculateMagnitude(myData.accX, myData.accY, myData.accZ);

    if(magnitude >= POSITIVE_TRESHOLD)
    {
      if(!isHigherPozTreshld)
      {
        frequency = 1/((myData.measuredTime - prevTime)/1000.0);
        isHigherPozTreshld = true;
        Serial.println("Frekvencia: " + String(frequency) + " Hz");
        prevTime = myData.measuredTime;

        if(frequency >= 50.0)
        {
          sampleCounter++;
          if(sampleCounter >= 2)
          {
            Serial.println("Fall detected");
            sampleCounter = 0;
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

    if(magnitude <= NEGATIVE_TRESHOLD)
    {
      if(!isLowerNegTreshld)
      {
        isLowerNegTreshld = true;
      }
    }

    if(magnitude > NEGATIVE_TRESHOLD)
    {
      if(isLowerNegTreshld)
      {
        isLowerNegTreshld = false;
      }
    }
  }

  if(millis() - lastSample >= 700)
  {
    sampleCounter = 0;
  }

}