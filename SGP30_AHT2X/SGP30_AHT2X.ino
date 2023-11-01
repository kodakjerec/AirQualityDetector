#include <Wire.h>
#include <WiFi.h>
#include "Adafruit_SGP30.h"  // SGP30 library
#include <AHTxx.h>           // include AHTxx library
#include <HTTPClient.h>
#include "time.h"
// TFT
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS    5   // Enter the chosen GPIO pin number for CS
#define TFT_RST   4   // Enter the chosen GPIO pin number for RST
#define TFT_DC    2    // Enter the chosen GPIO pin number for DC
// SCL 18
// SDA 23
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

Adafruit_SGP30 sgp;
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);  // I2C address and type of the AHT21 sensor

#define isDebug false  // debug

// WiFi and CloubAmeba constants
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// time
const char* ntpServer = "2.tw.pool.ntp.org";
const long gmtOffset_sec = 28800;  // GMT+8
const int daylightOffset_sec = 0;
char timeStringBuff[50] = "";
char payload[240] = "";

// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = ""
// WiFi
WiFiClient CloubAmeba;

void setup() {
  Serial.begin(115200);

  while (!Serial)
    ;
  if (!sgp.begin()) {
    Serial.println("Could not find a valid SGP30 Sensor not found :(");
    while (1)
      ;
  }

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
  
  // Initialize the ST7735 display
  tft.initR(INITR_BLACKTAB);    // Use your preferred display type

  // Set display rotation (optional)
  tft.setRotation(3);          // Adjust according to your setup

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  // 檢查 WiFi 連線狀態，如果未連線或斷線了，則進行重連
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
    Serial.println("WiFi reconnected!");
  }

  // get Time
  getTime();

  send_Data();

  show_tft();

  // Wait 1 sec before next measurement
  delay(1000);
}

// Setup WiFi
// 設定WiFi
void setup_WiFi() {
  // Connect to WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println();
  Serial.print("Waiting for WiFi connection...");
  int times = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    times++;
    if (times > 20) {
      times = 0;
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
uint16_t tvoc = 0;
uint16_t eco2 = 0;
float temperature = 0;
float humidity = 0;
float calTimes = 0; // 5秒送一次
void send_Data() {
  // Space to store values to send
  char str_TVOC[10];
  char str_eCO2[10];
  char str_H2[10];
  char str_Ethanol[10];
  char str_temperature[10];
  char str_humidity[10];

  // Read the temperature and humidity from the AHT21 sensor
  temperature = aht20.readTemperature();
  humidity = aht20.readHumidity();
  sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  sgp.IAQmeasure();
  tvoc = sgp.TVOC;
  eco2 = sgp.eCO2;
  sgp.IAQmeasureRaw();
  uint16_t h2 = sgp.rawH2;
  uint16_t ethanol = sgp.rawEthanol;

  calTimes = calTimes + 1;

  if (calTimes<=5) {
    return;
  }
  calTimes = 0;

  // Format the values as strings
  dtostrf(tvoc, 5, 0, str_TVOC);
  dtostrf(eco2, 5, 0, str_eCO2);
  dtostrf(h2, 5, 0, str_H2);
  dtostrf(ethanol, 5, 0, str_Ethanol);
  dtostrf(temperature, 5, 2, str_temperature);
  dtostrf(humidity, 5, 2, str_humidity);
  
  if (isDebug) {
    Serial.println("Publishing value for TVOC: " + String(str_TVOC));
    Serial.println("Publishing value for eCO2: " + String(str_eCO2));
    Serial.println("Publishing value for H2: " + String(str_H2));
    Serial.println("Publishing value for Ethanol: " + String(str_Ethanol));
    Serial.println("Publishing value for temperature: " + String(temperature));
    Serial.println("Publishing value for humidity: " + String(humidity));
  }

  String tempURL = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "datetime=" + String(timeStringBuff);
  sprintf(payload, "");
  sprintf(payload, "%s&tvoc=%s", payload, str_TVOC);
  sprintf(payload, "%s&eco2=%s", payload, str_eCO2);
  sprintf(payload, "%s&h2=%s", payload, str_H2);
  sprintf(payload, "%s&ethanol=%s", payload, str_Ethanol);
  sprintf(payload, "%s&temp=%s", payload, str_temperature);
  sprintf(payload, "%s&humi=%s", payload, str_humidity);
  String endURL = String(payload);
  endURL.replace(" ", "");

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
    Serial.print("Server Response Payload: ");
    Serial.println(responsePayload);
  } else {
    Serial.print("Server Respose Code： ");
    Serial.println(httpCode);
  }
  //---------------------------------------------------------------------
  http.end();
}

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));  // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                 // [mg/m^3]
  return absoluteHumidityScaled;
}

void show_tft() {
  temperature = temperature - 10;
  humidity = humidity + 20;

  char str_TVOC[10];
  char str_eCO2[10];
  char str_temperature[10];
  char str_humidity[10];
  dtostrf(tvoc, 5, 0, str_TVOC);
  dtostrf(eco2, 5, 0, str_eCO2);
  dtostrf(temperature, 2, 0, str_temperature);
  dtostrf(humidity, 2, 0, str_humidity);

  // Fill the screen with a color
  tft.fillScreen(ST7735_BLACK);

  // Set text color to white
  tft.setTextColor(ST7735_WHITE);

  // Set the cursor position
  tft.setTextSize(1);
  tft.setCursor(5, 15);
  tft.print("TVOC ");
  tft.setCursor(5, 45);
  tft.print("eCO2 ");

  tft.setTextSize(2);
  
  if (tvoc<200)
    tft.setTextColor(ST7735_GREEN);
  else if (tvoc>=200 and tvoc<1000)
    tft.setTextColor(ST7735_YELLOW);
  else if (tvoc>=1000)
    tft.setTextColor(ST7735_RED);
  tft.setCursor(40, 10);
  tft.print(String(str_TVOC));
  
  int radius = 15;
  int centerX = 130;
  int centerY = 30;
  if (tvoc < 200) {
    tft.fillCircle(centerX, centerY, radius, ST7735_GREEN);
  } else if (tvoc >= 200 && tvoc < 1000) {
    tft.fillCircle(centerX, centerY, radius, ST7735_YELLOW);
  } else if (tvoc >= 1000) {
    tft.fillCircle(centerX, centerY, radius, ST7735_RED);
  }
  
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(40, 40);
  tft.print(String(str_eCO2));
  
  tft.setTextSize(5);
  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(10, 80);
  tft.print(String(str_temperature));
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(100, 80);
  tft.print(String(str_humidity)); 
}
