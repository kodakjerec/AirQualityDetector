#include <Wire.h>
#include <WiFi.h>
#include "ScioSense_ENS160.h"  // ENS160 library
#include <AHTxx.h>             // include AHTxx library
#include <HTTPClient.h>
#include "time.h"

ScioSense_ENS160 ens160(ENS160_I2CADDR_1);     // I2C address of the ENS160 sensor
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);  // I2C address and type of the AHT21 sensor

#define isDebug false // debug

// WiFi and CloubAmeba constants
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// time
const char* ntpServer = "2.tw.pool.ntp.org";
const long  gmtOffset_sec = 28800; // GMT+8
const int   daylightOffset_sec = 0;
char timeStringBuff[50] = "";
char payload[240] = "";


// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "";    // change Gscript ID

// WiFi
WiFiClient CloubAmeba;

void setup() {
  // Initialize serial communication and ENS160 sensor
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("ENS160 - Digital air quality sensor");
  if (!ens160.begin()) {
    Serial.println("Could not find a valid ENS160 sensor, check wiring!");
    while (1)
      ;
  }
  Serial.println("ENS160 sensor found");
  ens160.setMode(ENS160_OPMODE_STD);  // Set standard mode of operation
  Serial.println("ENS160 sensor set to standard mode");

  if (!aht20.begin()) {
    Serial.println("Could not find a valid ATH20 sensor, check wiring!");
    while (1)
      ;
  }
  Serial.println("ATH20 sensor found");
  //aht20.setResolution(AHTXX_14BIT_RESOLUTION); // Set 14-bit resolution
  Serial.println("ATH20 sensor set to 14-bit resolution");

  // Set WiFi
  setup_WiFi();

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

}

void loop() {
  // 檢查 WiFi 連線狀態，如果未連線或斷線了，則進行重連
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
    Serial.println("WiFi reconnected!");
  }

  // get Time
  getTime();

  send_Data();

  // Wait 5 sec before next measurement
  delay(5000);
}

// Setup WiFi
// 設定WiFi
void setup_WiFi() {
  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println();
  Serial.print("Waiting for WiFi connection...");
  int times=0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    times++;
    if (times>20){
      times=0;
      WiFi.reconnect();
    }
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// get current time
void getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d-%H:%M:%S", &timeinfo);
}

// Send MQTT message
// 發送MQTT
void send_Data() {
  // Space to store values to send
  char str_AQI[10];
  char str_TVOC[10];
  char str_eCO2[10];
  char str_HP0[10];
  char str_HP1[10];
  char str_HP2[10];
  char str_HP3[10];
  char str_temperature[10];
  char str_humidity[10];

  // Perform a measurement and read the sensor values
  ens160.measure(0);
  float aqi = ens160.getAQI();
  float tvoc = ens160.getTVOC();
  float eco2 = ens160.geteCO2();
  float hp0 = ens160.getHP0();
  float hp1 = ens160.getHP1();
  float hp2 = ens160.getHP2();
  float hp3 = ens160.getHP3();

  // Read the temperature and humidity from the AHT21 sensor
  float temperature = aht20.readTemperature();
  float humidity = aht20.readHumidity();

  if (isDebug) {
    Serial.println("Publishing value for AQI: " + String(aqi));
    Serial.println("Publishing value for TVOC: " + String(tvoc));
    Serial.println("Publishing value for eCO2: " + String(eco2));
    Serial.println("Publishing value for HP0: " + String(hp0));
    Serial.println("Publishing value for HP1: " + String(hp1));
    Serial.println("Publishing value for HP2: " + String(hp2));
    Serial.println("Publishing value for HP3: " + String(hp3));
    Serial.println("Publishing value for temperature: " + String(temperature));
    Serial.println("Publishing value for humidity: " + String(humidity));
  }

  // Format the values as strings
  dtostrf(aqi, 5, 2, str_AQI);
  dtostrf(tvoc, 5, 2, str_TVOC);
  dtostrf(eco2, 5, 2, str_eCO2);
  dtostrf(hp0, 5, 2, str_HP0);
  dtostrf(hp1, 5, 2, str_HP1);
  dtostrf(hp2, 5, 2, str_HP2);
  dtostrf(hp3, 5, 2, str_HP3);
  dtostrf(temperature, 5, 2, str_temperature);
  dtostrf(humidity, 5, 2, str_humidity);

  String tempURL = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"datetime=" + String(timeStringBuff);
  sprintf(payload, "");
  sprintf(payload, "%s&aqi=%s", payload, str_AQI);
  sprintf(payload, "%s&tvoc=%s", payload, str_TVOC);
  sprintf(payload, "%s&eco2=%s", payload, str_eCO2);
  sprintf(payload, "%s&hp0=%s", payload, str_HP0);
  sprintf(payload, "%s&hp1=%s", payload, str_HP1);
  sprintf(payload, "%s&hp2=%s", payload, str_HP2);
  sprintf(payload, "%s&hp3=%s", payload, str_HP3);
  sprintf(payload, "%s&temp=%s", payload, str_temperature);
  sprintf(payload, "%s&humi=%s", payload, str_humidity);
  String endURL = String(payload);
  endURL.replace(" ","");

  String finalURL = tempURL + endURL;
  Serial.print("POST data to spreadsheet:");
  Serial.println(finalURL);

  // HTTP send
  HTTPClient http;
  http.begin(finalURL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET(); 
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);
  //---------------------------------------------------------------------
  //getting response from google sheet
  if (httpCode == HTTP_CODE_OK) {
    String responsePayload = http.getString();
    Serial.println("Server Response Payload: ");
    Serial.println(responsePayload);
  } else {
    Serial.println("Server Respose Code：");
    Serial.println(httpCode);
  }
  //---------------------------------------------------------------------
  http.end();
}
