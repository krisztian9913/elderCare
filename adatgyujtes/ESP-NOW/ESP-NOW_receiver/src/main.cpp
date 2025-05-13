#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

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

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&myData, incomingData, sizeof(myData));
  /*Serial.print("Bytes received: ");
  Serial.println(len);*/
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
  
  /*String str = "Acc conf: " + String(myData.accConf) + ", Acc range: " + String(myData.accRange) + ", Power conf: " + String(myData.pwrConf) + ", Power ctrl: " +String(myData.pwrCtrl);
  Serial.println(str);
  Serial.print("Time(ms), accelX(m/s2), accelY(m/s2), accelZ(m/s2), batteryVoltage(V)");
  Serial.println();*/
  prevMeasureEnable = true;
}

void loop()
{
  if(myData.isNewData)
  {
    myData.isNewData = false;

    

    if(prevMeasureEnable && !measureEnable)
    {
      prevMeasureEnable = false;
      String str = "Acc conf(" + String(myData.accConf) + "), Acc range(" + String(myData.accRange) + "), Power conf(" + String(myData.pwrConf) + "), Power ctrl(" +String(myData.pwrCtrl) + "), Dummy(2)";
      Serial.print(str);
      Serial.println();
      Serial.print("Time(ms), accelX(m/s2), accelY(m/s2), accelZ(m/s2), batteryVoltage(V)");
      Serial.println();
    }


    String str = String(myData.measuredTime) + ", " + String(myData.accX) + ", " + String(myData.accY) + ", " + String(myData.accZ) + ", " + String(myData.battVoltage);
    Serial.println(str);

    if(myData.accX <= -100.0)
    {
      measureEnable = false;
      prevMeasureEnable = !measureEnable;
    }
    else
    {
      measureEnable = true;
      prevMeasureEnable = !measureEnable;
    }

  }
}