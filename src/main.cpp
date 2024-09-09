#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "FastLED.h"
#include "SPIFFS.h"
#include <HTTPUpdate.h>
#define EEPROM_SIZE 3
#define NUM_LEDS 7
#define DATA_PIN 5
CRGB leds[NUM_LEDS];
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <EasyButton.h>
#include <ArduinoJson.h>

#include <MFRC522.h>
#include <SPI.h>
#define SS_PIN 21 // kh?i dung cho module RFID
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define FIRMWARE_VERSION "1.7" // ngày 04/09/2024 version mới là 1.6

bool checkLEDVal; // bi?n ?i?u khi?n ch?p t?t khi ??a th? vao RFID
int updateDeadAlive_time = 600000;
unsigned long countingTimeDeadAlive = 0;
unsigned long countingTimeWIFIcheck = 0;
unsigned long espRestartTimeOut = 0;

unsigned long callLeaderCountDown = 0;
unsigned long callMaterialCountDown = 0;

const uint32_t connectTimeoutMs = 10000;
int state = 0;
int countState = 0;
int light_lock = 0;
int sample;
int wifi_check = 0;
const char *getOK = "Success";
#define LED_PIN GPIO_NUM_27 // LED ??n theo doi

#define BUTTON_PIN1 32
#define BUTTON_PIN2 33
#define BUTTON_PIN3 25
#define BUTTON_PIN4 26

EasyButton button1(BUTTON_PIN1);
EasyButton button2(BUTTON_PIN2);
EasyButton button3(BUTTON_PIN3);
EasyButton button4(BUTTON_PIN4);

const char *ssid = "pccgroup_IoT-2.4G";
// const char*psw = "qwerasdf";

String ID;
String rfid;
// String callLeader = "http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/1:" + ID + ":123456";
// String callMaterial = "http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/3:" + ID + ":123456";
// //String callalive = "http://172.21.143.74:8080/cmmsservice/repairCheckList/healthCheck/VY/" + ID + "";
// String callalive = "http://172.21.143.74:8080/cmmsservice/repairCheckList/healthCheck/VY/";
// Your Domain name with URL path or IP address with path

// 08:3A:F2:51:01:3C
// uint8_t newMACAddress[] = {0x08, 0x3A, 0xF2, 0x51, 0x01, 0x3C};
// MAC

// String serverName = "http://172.21.149.109:8005/aco_issue";
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void ID_check()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/ID.txt");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("File Content:");
  while (file.available())
  {
    ID += (char)file.read();
  }
  ID = ID.c_str();
  file.close();
  Serial.print("ID là: ");
  Serial.println(ID);
}

void update_check()
{
  WiFiClient client;

  // The line below is optional. It can be used to blink the LED on the board during flashing
  // The LED will be on during download of one buffer of data from the network. The LED will
  // be off during writing that buffer to flash
  // On a good connection the LED should flash regularly. On a bad connection the LED will be
  // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
  // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
  // httpUpdate.setLedPin(LED_BUILTIN, LOW);

  httpUpdate.rebootOnUpdate(false); // remove automatic update

  Serial.println(F("Update start now!"));

  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://192.168.1.125:3000/firmware/httpUpdateNew.bin");
  // Or:
  t_httpUpdate_return ret = httpUpdate.update(client, "172.21.149.109", 3000, "/update", FIRMWARE_VERSION);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    Serial.println(F("Retry in 10secs!"));
    delay(1000); // Wait 10secs
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    delay(1000); // Wait a second and restart
    ESP.restart();
    break;
  }
}

void den_trieu_hoi_thanh_dong()
{
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(0, 128, 128);
    FastLED.show();
    light_lock = 0;
    // Serial.println("den wifi");
  }
}

// sửa cái NFC

void den_trieu_hoi_thanh_dong_spi_hu()
{
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(108, 254, 10);
    FastLED.show();
    light_lock = 0;
    // Serial.println("den wifi");
  }
}

void check_WIFI()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    wifi_check = 1;
  }
  else
  {
    den_trieu_hoi_thanh_dong();
    Serial.println("WiFi not connected!");
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    long now = millis();
    if (now - espRestartTimeOut > 10000)
    {
      espRestartTimeOut = now;
      ESP.restart();
    }
  }
}

void dead_alive()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("http://172.21.143.74:8080/cmmsservice/repairCheckList/healthCheck/VY/" + ID + "");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void dencanbo()
{
  light_lock = 1;
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(0, 0, 255);
    FastLED.show();
  }
  Serial.println("den can bo");
}
void denbaotri()
{
  light_lock = 1;
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(0, 255, 20);
    FastLED.show();
  }
  Serial.println("den bao tri");
}
void denhoatdong()
{
  light_lock = 1;
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(236, 236, 236);
    FastLED.show();
  }
  Serial.println("den hoat dong");
}
void den_nhan_don()
{
  light_lock = 1;
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(255, 0, 0);
    FastLED.show();
  }
  Serial.println("den nhan don");
}
void denhetlieu()
{
  light_lock = 1;
  for (int i = 0; i < 7; i++)
  {
    leds[i] = CRGB(249, 244, 0);
    FastLED.show();
  }
  Serial.println("den het lieu");
}

void check_RFID()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      checkLEDVal = 1;
    }
  }
  else
  {
    checkLEDVal = 0;
  }
}

void leader()
{
  light_lock = 0;
  if ((state == 3) || (state == 4))
  {
    return;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    // http.begin(callLeader_Str.c_str());
    http.begin("http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/1:" + ID + ":123456");

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
      state = 1;
      callLeaderCountDown = millis();
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
    // line_report_gunstuck();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void material()
{
  light_lock = 0;
  if ((state == 3) || (state == 4))
  {
    return;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/3:" + ID + ":123456");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);

      state = 2;
      callMaterialCountDown = millis();
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
    // line_report_overCement();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void send_rfid_callTPM()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String rfid_Str = rfid;
    http.begin(rfid_Str.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      const char *payloadJsonTPM = payload.c_str();

      StaticJsonDocument<96> doc;

      DeserializationError error = deserializeJson(doc, payloadJsonTPM);

      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      int statusCode = doc["statusCode"];   // 1
      const char *message = doc["message"]; // "Success"
      Serial.println(statusCode);
      Serial.println(message);
      if (strcmp(getOK, message) == 0)
      {
        Serial.println(payload);
        state = 3;
        EEPROM.write(0, state);
        EEPROM.commit();
        Serial.println("State saved in flash memory");
      }
      else
      {
        Serial.println("Send no success");
      }
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void send_rfid_Receive()
{
  //  if(countState>=2)
  //     {
  //       countState=1;
  //       EEPROM.write(1, countState);
  //       EEPROM.commit();
  //       Serial.println("reset countstate do b?m linh tinh");
  //     }
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String rfid_Str = rfid;
    http.begin(rfid_Str.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();

      // This code is used for check status and message
      const char *payloadJson = payload.c_str();

      StaticJsonDocument<96> doc;

      DeserializationError error = deserializeJson(doc, payloadJson);

      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      int statusCode = doc["statusCode"];   // 1
      const char *message = doc["message"]; // "Success"
      Serial.println(statusCode);
      Serial.println(message);
      ///////////////////////////////////////////////////
      Serial.println(payload);
      if (strcmp(getOK, message) == 0)
      {
        state = 4;
        countState++;
        EEPROM.write(0, state);
        EEPROM.write(1, countState);
        EEPROM.commit();
        Serial.print("countState: ");
        Serial.println(countState);
        Serial.println("State saved in flash memory");
      }
      else
      {
        Serial.println("Send no success");
      }
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void call_tpm()
{
  light_lock = 0;
  if (state == 3)
  {
    return;
  }
  if (countState > 0)
  {
    return;
  }
  Serial.println("Got it");
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    Serial.println("RFID read");
    return;
  }
  // Select a card
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  mfrc522.PICC_HaltA();      // halt PICC
  mfrc522.PCD_StopCrypto1(); // stop encryption on PCD
  Serial.println(content);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  vTaskDelay(100);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  vTaskDelay(100);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  rfid = "http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/0:" + ID + ":" + content + "";
  send_rfid_callTPM();
}

void receive_order()
{
  light_lock = 0;
  Serial.println("Got it");
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    Serial.println("RFID read");
    return;
  }
  // Select a card
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  mfrc522.PICC_HaltA();      // halt PICC
  mfrc522.PCD_StopCrypto1(); // stop encryption on PCD
  Serial.println(content);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  vTaskDelay(100);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  vTaskDelay(100);
  digitalWrite(LED_PIN, HIGH);
  vTaskDelay(100);
  digitalWrite(LED_PIN, LOW);
  rfid = "http://172.21.143.74:8080/cmmsservice/repairCheckList/up/VY/2:" + ID + ":" + content + "";
  send_rfid_Receive();
}

bool checkSPIConnection()
{
  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);

  // MFRC522 VersionReg should return a specific value (0x92 for RC522)
  if (version == 0x92)
  {
    return true; // Connection is successful
  }
  else
  {
    return false; // Connection has failed
  }
}

void Steve_checkSPIConnection()
{
  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);

  // MFRC522 VersionReg should return a specific value (0x92 for RC522)
  if (version == 0x92)
  {
    Serial.println("SPI connection is working!");
  }
  else
  {
    den_trieu_hoi_thanh_dong_spi_hu();
    Serial.println("WiFi not connected!");
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    ESP.restart();
  }
}
void resetEEprom()
{
  state = 0;
  countState = 0;
  light_lock = 0;
  EEPROM.write(0, state);
  EEPROM.write(1, countState);
  EEPROM.commit();
  Serial.print("countState: ");
  Serial.println(countState);
  Serial.println("State saved in flash memory");
  Serial.println("ESP reset in 2s");
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  ESP.restart();
}

void resetFunction()
{
  Serial.println("ESP reset in 2s");
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  ESP.restart();
}

void setup()
{
  Serial.begin(115200);

  delay(10);
  SPI.begin();
  EEPROM.begin(EEPROM_SIZE);
  delay(100);
  state = EEPROM.read(0);
  countState = EEPROM.read(1);
  // ham ch?y cho chip n?p l?n ??u
  if (state == 255 && countState == 255)
  {
    EEPROM.write(0, 0);
    EEPROM.write(1, 0);
    EEPROM.commit();
  }
  if (countState >= 2)
  {
    countState = 0;
    EEPROM.write(1, countState);
    EEPROM.commit();
    Serial.println("reset countstate do b?m linh tinh");
  }
  Serial.print("countState: ");
  Serial.println(countState);
  mfrc522.PCD_Init();
  if (checkSPIConnection())
  {
    Serial.println("Initial SPI connection successful.");
  }
  else
  {
    Serial.println("Initial SPI connection failed.");
    den_trieu_hoi_thanh_dong_spi_hu();
  }
  delay(50);
  FastLED.addLeds<WS2813, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  button1.begin();
  button2.begin();
  button3.begin();
  button4.begin();
  button1.onPressed(receive_order);
  button2.onPressed(leader);
  button2.onPressedFor(4000, resetFunction);
  button3.onPressed(material);
  button4.onPressed(call_tpm);
  button4.onPressedFor(5000, resetEEprom);
  ID_check();

  WiFi.mode(WIFI_STA);

  // MAC
  // esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  // Serial.print("[NEW] ESP32 Board MAC Address:  ");
  // Serial.println(WiFi.macAddress());
  // MAC

  WiFi.begin(ssid);
  while (wifi_check == 0)
  {
    check_WIFI();
  }
  dead_alive();
  update_check();
}
void loop()
{
  check_RFID();
  if (checkLEDVal == 1)
  {
    digitalWrite(LED_PIN, HIGH);
  }
  if (checkLEDVal == 0)
  {
    digitalWrite(LED_PIN, LOW);
  }
  button1.read();
  button2.read();
  button3.read();
  button4.read();
  if (millis() - countingTimeDeadAlive >= updateDeadAlive_time)
  {
    dead_alive();
    countingTimeDeadAlive = millis();
  }
  if (millis() - countingTimeWIFIcheck >= connectTimeoutMs)
  {
    check_WIFI();
    Steve_checkSPIConnection();
    countingTimeWIFIcheck = millis();
  }
  switch (state)
  {
  case 0:
    if (light_lock == 1)
    {
      return;
    }
    denhoatdong();
    break;

  case 1:
    if (millis() - callLeaderCountDown >= 60000)
    {
      state = 0;
      callLeaderCountDown = millis();
      light_lock = 0;
      break;
    }
    if (light_lock == 1)
    {
      return;
    }
    dencanbo();
    break;
  case 2:
    if (millis() - callMaterialCountDown >= 60000)
    {
      state = 0;
      callMaterialCountDown = millis();
      light_lock = 0;
      break;
    }
    if (light_lock == 1)
    {
      return;
    }
    denhetlieu();
    break;
  case 3:
    if (light_lock == 1)
    {
      return;
    }
    denbaotri();
    break;
  case 4:
    if (light_lock == 1)
    {
      return;
    }
    den_nhan_don();
    if (countState == 2)
    {
      state = 0;
      countState = 0;
      light_lock = 0;
      EEPROM.write(0, state);
      EEPROM.write(1, countState);
      EEPROM.commit();
      Serial.print("countState: ");
      Serial.println(countState);
      Serial.println("State saved in flash memory");
    }
    break;
  }
}
