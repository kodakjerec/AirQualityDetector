#include <WiFi.h>
#include "MQUnifiedsensor.h"
#include <SPI.h>
#include <AmebaST7735.h>

// // TFT
#define TFT_CS PA15   // TFT LCD的CS PIN腳
#define TFT_DC PA26   // TFT DC(A0、RS)
#define TFT_RST PA25  // TFT Reset
#define TFT_SCL PA14
#define TFT_SDA PA12
AmebaST7735 tft = AmebaST7735(TFT_CS, TFT_DC, TFT_RST);

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// WiFi
//SSID e senha da rede wifi para o ESP se conectar
char ssid[] = "kodakjerecHTC";
char pass[] = "xj4su6u83";
int status = WL_IDLE_STATUS;  // the Wifi radio's status


// Button
int ButtonStatus = LOW;
int lastButtonStatus = HIGH;
int debounceDelay = 50;
long lastDebounceTime = 0;
#define pinButton PA30

//Definitions
#define placa "Ameba BW16 typeC"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10  // For arduino UNO/MEGA/NANO

#define pinMQ2 A2
#define RatioMQ2CleanAir (9.83)  //RS / R0 = 9.83 ppm

//Declare Sensor
MQUnifiedsensor MQ2(placa, Voltage_Resolution, ADC_Bit_Resolution, pinMQ2, "MQ-2");

// ** all data
bool isCalibration = false;                                                              // calibration Mode
bool isLimit = false;                                                                    // add limit output
float getMQ2_H2, getMQ2_LPG, getMQ2_CO, getMQ2_Alcohol, getMQ2_Propane, getMQ2_Voltage;  // MQ sensor
// Wifi
IPAddress ip = { 0, 0, 0, 0 };

// 裝上啟動按鈕
bool isEnableButton = false;

void setup() {
  Serial.begin(115200);
  // TFT
  tft.setResolution(128, 160);
  tft.begin();
  tft.setRotation(2);
  tft.clr();  //設定螢幕背景為黑色

  // WIFI
  wifi_setup();

  // MQ
  MQ_setup();

  // Button
  if (isEnableButton) {
    pinMode(pinButton, INPUT_PULLUP);  // Button
  }
}

void loop() {
  // MQ
  MQ_loop();

  // Button
  if (isEnableButton) {
    button_loop();
  }

  // TFT
  tft_loop();

  delay(1000);
}

// wifi設定
void wifi_setup() {
  //Initialize serial and wait for port to open:
  int wiFiFailCount = 1; // fail count
  while (!Serial) {
    ;
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    tft.println("WiFi shield not present");
    // don't continue:
    return;
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    tft.println("Attempting to connect to WPA SSID: ");
    tft.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    
    delay(1000);
    tft.println(wiFiFailCount);
    wiFiFailCount--;

    if (wiFiFailCount<=0){
      return;
    }
  }

  // you're connected now, so print out the data:
  tft.clr();
  tft.setCursor(5, 7);
  tft.println();
  tft.println("You're connected to the network");
  printCurrentNet();
  delay(5000);
  printWifiData();
  delay(5000);
}
void printCurrentNet() {
  // print the SSID of the network you're attached to:
  tft.println("SSID: ");
  tft.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  tft.println("BSSID: ");
  tft.print(bssid[5], HEX);
  tft.print(":");
  tft.print(bssid[4], HEX);
  tft.print(":");
  tft.print(bssid[3], HEX);
  tft.print(":");
  tft.print(bssid[2], HEX);
  tft.print(":");
  tft.print(bssid[1], HEX);
  tft.print(":");
  tft.print(bssid[0], HEX);
  tft.println();

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  tft.println("signal strength (RSSI):");
  tft.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  tft.println("Encryption Type:");
  tft.print(encryption, HEX);
  tft.println();
}
void printWifiData() {
  // print your WiFi shield's IP address:
  ip = WiFi.localIP();
  tft.println("IP Address: ");
  tft.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  tft.println("MAC address: ");
  tft.print(mac[0], HEX);
  tft.print(":");
  tft.print(mac[1], HEX);
  tft.print(":");
  tft.print(mac[2], HEX);
  tft.print(":");
  tft.print(mac[3], HEX);
  tft.print(":");
  tft.print(mac[4], HEX);
  tft.print(":");
  tft.print(mac[5], HEX);
  tft.println();
}

void MQ_setup() {
  //Set math model to calculate the PPM concentration and the value of constants
  MQ2.setRegressionMethod(1);  //_PPM =  a*ratio^b
  MQ2.init();

  if (isCalibration) {
    MQ2_Calibration();
  } else {
    MQ2.setR0(0.61);
  }
}
// 重設MQ2
void MQ2_Calibration() {
  /*****************************  MQ CAlibration ********************************************/
  // Explanation:
  // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  tft.println("Calibrating please wait 10 sec.");
  float calR0 = 0;
  for (int i = 1; i <= 100; i++) {
    MQ2.update();
    calR0 += MQ2.calibrate(RatioMQ2CleanAir);
    if (i % 10 == 0) {
      tft.print(i / 10);
      tft.print(" ");
    }
    delay(100);
  }
  tft.println("MQ2 done!.");

  if (isinf(calR0)) {
    tft.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    return;
  }
  if (calR0 == 0) {
    tft.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    return;
  }
  MQ2.setR0(calR0 / 100);
  tft.print("MQ2.getR0: ");
  tft.print(MQ2.getR0());
  tft.print(" | ");
  /*****************************  MQ CAlibration ********************************************/
  // MQ2.serialDebug(true);
}
// 取值
void MQ_loop() {
  MQ2.update();
  if (isCalibration) {
    tft.print("MQ2.getR0: ");
    tft.println(MQ2.getR0());
    MQ2.serialDebug();
  }
  // MQ2
  MQ2.setA(987.99);
  MQ2.setB(-2.162);
  getMQ2_H2 = MQ2.readSensor();
  MQ2.setA(574.25);
  MQ2.setB(-2.222);
  getMQ2_LPG = MQ2.readSensor();
  MQ2.setA(36974);
  MQ2.setB(-3.109);
  getMQ2_CO = MQ2.readSensor();
  MQ2.setA(3616.1);
  MQ2.setB(-2.675);
  getMQ2_Alcohol = MQ2.readSensor();
  MQ2.setA(658.71);
  MQ2.setB(-2.168);
  getMQ2_Propane = MQ2.readSensor();
  getMQ2_Voltage = analogRead(pinMQ2) * 5.0 / 1023.0;
  if (isLimit) {
    if (getMQ2_H2 < 300) getMQ2_H2 = 0;
    else if (getMQ2_H2 > 5000) getMQ2_H2 = 5000;
    if (getMQ2_LPG < 200) getMQ2_LPG = 0;
    else if (getMQ2_LPG > 5000) getMQ2_LPG = 5000;
    if (getMQ2_CO < 300) getMQ2_CO = 0;
    else if (getMQ2_CO > 5000) getMQ2_CO = 5000;
    if (getMQ2_Alcohol < 100) getMQ2_Alcohol = 0;
    else if (getMQ2_Alcohol > 2000) getMQ2_Alcohol = 2000;
    if (getMQ2_Propane < 200) getMQ2_Propane = 0;
    else if (getMQ2_Propane > 5000) getMQ2_Propane = 5000;
  }
}
// 格式排版
void printSpace(float object) {
  int length = 6;
  int tempObject = object / 1;
  while (tempObject / 10 > 0) {
    length--;
    tempObject = tempObject / 10;
  }
  while (length > 0) {
    tft.print(" ");
    length--;
  }
  tft.print(object);
}

// 重新測量按鈕
void button_loop() {
  // put your main code here, to run repeatedly:
  int reading = digitalRead(pinButton);
  if (reading != lastButtonStatus) {
    lastDebounceTime = millis();
  }
  if (millis() - lastDebounceTime > 50) {
    if (reading != ButtonStatus) {
      ButtonStatus = reading;
      if (ButtonStatus == LOW) {
        if (isCalibration) return;
        tft.println("Start Calibration");
        delay(2000);
        isCalibration = true;
        MQ2_Calibration();
        MQ2.update();
        MQ2.readSensor();
        tft.println();
        isCalibration = false;
      } else {
        tft.println("off");
      }
    }
  }
  lastButtonStatus = reading;
}

// TFT
bool isTftFirstTime = true;
void tft_loop() {
  if (isTftFirstTime) {
    tft.clr();                 //設定螢幕背景為黑色
    tft.setCursor(5, 7);       //設定文字游標點
    tft.setForeground(WHITE);  //設定文字顏色

    tft.print("H2  :");
    printSpace(getMQ2_H2);
    tft.print(" ppm");
    tft.setCursor(5, 21);
    tft.print("LPG :");
    printSpace(getMQ2_LPG);
    tft.print(" ppm");
    tft.setCursor(5, 35);
    tft.print("CO  :");
    printSpace(getMQ2_CO);
    tft.print(" ppm");
    tft.setCursor(5, 49);
    tft.print("ALCO:");
    printSpace(getMQ2_Alcohol);
    tft.print(" ppm");
    tft.setCursor(5, 63);
    tft.print("C3H8:");
    printSpace(getMQ2_Propane);
    tft.print(" ppm");
    tft.setCursor(5, 77);
    tft.print("VOLT:");
    printSpace(getMQ2_Voltage);
    tft.print(" V");

    tft.setCursor(5, 91);
    tft.println("IP Address: ");
    tft.setForeground(YELLOW);  //設定文字顏色
    tft.setCursor(5, 105);
    tft.println(ip);
    tft.setCursor(5, 112);
    tft.println(getWiFiStatus());

    isTftFirstTime = false;
  } else {
    tft.setForeground(YELLOW);  //設定文字顏色
    tft.fillRectangle(35, 7, 63, 7, BLACK);
    tft.setCursor(35, 7);
    printSpace(getMQ2_H2);
    tft.fillRectangle(35, 21, 63, 7, BLACK);
    tft.setCursor(35, 21);
    printSpace(getMQ2_LPG);
    tft.fillRectangle(35, 35, 63, 7, BLACK);
    tft.setCursor(35, 35);
    printSpace(getMQ2_CO);
    tft.fillRectangle(35, 49, 63, 7, BLACK);
    tft.setCursor(35, 49);
    printSpace(getMQ2_Alcohol);
    tft.fillRectangle(35, 63, 63, 7, BLACK);
    tft.setCursor(35, 63);
    printSpace(getMQ2_Propane);
    tft.fillRectangle(35, 77, 63, 7, BLACK);
    tft.setCursor(35, 77);
    printSpace(getMQ2_Voltage);
  }
}
// return wifi status-string
unsigned long previousMillis = 0;
unsigned long interval = 30000;
char* getWiFiStatus() {
  //print the Wi-Fi status every 30 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        return "Configured SSID cannot be reached";
        break;
      case WL_CONNECTED:
        return "Connection successfully established";
        break;
      case WL_CONNECT_FAILED:
        return "Connection failed";
        break;
    }
    previousMillis = currentMillis;
  }
  return "";
}