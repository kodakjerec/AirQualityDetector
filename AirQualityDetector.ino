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
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

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
bool isCalibration = false;  // calibration Mode
bool isLimit = false;        // add limit output
float getMQ2_H2, getMQ2_LPG, getMQ2_CO, getMQ2_Alcohol, getMQ2_Propane, getMQ2_Voltage;

// 裝上啟動按鈕
bool isEnableButton = false;

void setup() {
  Serial.begin(9600);
  // TFT
  tft.setResolution(128, 160);
  tft.begin();
  tft.setRotation(2);
  tft.clr();  //設定螢幕背景為黑色

  // WIFI
  // wifi_setup();

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
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true)
      ;
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.println();
  Serial.println("You're connected to the network");
  printCurrentNet();
  printWifiData();
}
void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.println("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.println("BSSID: ");
  Serial.println(bssid[5], HEX);
  Serial.println(":");
  Serial.println(bssid[4], HEX);
  Serial.println(":");
  Serial.println(bssid[3], HEX);
  Serial.println(":");
  Serial.println(bssid[2], HEX);
  Serial.println(":");
  Serial.println(bssid[1], HEX);
  Serial.println(":");
  Serial.println(bssid[0], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.println("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.println("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}
void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.println("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.println("MAC address: ");
  Serial.println(mac[0], HEX);
  Serial.println(":");
  Serial.println(mac[1], HEX);
  Serial.println(":");
  Serial.println(mac[2], HEX);
  Serial.println(":");
  Serial.println(mac[3], HEX);
  Serial.println(":");
  Serial.println(mac[4], HEX);
  Serial.println(":");
  Serial.println(mac[5], HEX);
}

void MQ_setup() {
  //Set math model to calculate the PPM concentration and the value of constants
  MQ2.setRegressionMethod(1);  //_PPM =  a*ratio^b
  MQ2.init();

  if (isCalibration) {
    MQ2_Calibration();
  } else {
    MQ2.setR0(0.4);
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
  Serial.println("Calibrating please wait 10 sec.");
  float calR0 = 0;
  for (int i = 1; i <= 100; i++) {
    MQ2.update();
    calR0 += MQ2.calibrate(RatioMQ2CleanAir);
    if (i % 10 == 0) {
      Serial.print(i / 10);
      Serial.print(" ");
    }
    delay(100);
  }
  Serial.println("MQ2 done!.");

  if (isinf(calR0)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    return;
  }
  if (calR0 == 0) {
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    return;
  }
  MQ2.setR0(calR0 / 100);
  Serial.print("MQ2.getR0: ");
  Serial.print(MQ2.getR0());
  Serial.print(" | ");
  /*****************************  MQ CAlibration ********************************************/
  // MQ2.serialDebug(true);
}
// 取值
void MQ_loop() {
  MQ2.update();
  if (isCalibration) {
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
  getMQ2_Voltage = analogRead(pinMQ2)*5.0/1023.0;
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
        Serial.println("Start Calibration");
        delay(2000);
        isCalibration = true;
        MQ2_Calibration();
        MQ2.update();
        MQ2.readSensor();
        Serial.println();
        isCalibration = false;
      } else {
        Serial.println("off");
      }
    }
  }
  lastButtonStatus = reading;
}

// TFT
bool isTftFirstTime = true;
void tft_loop() {
  if (isTftFirstTime){
    tft.clr();  //設定螢幕背景為黑色
    tft.setCursor(5, 7);  //設定文字游標點
    tft.setForeground(WHITE);  //設定文字顏色
    tft.print("H2  :");printSpace(getMQ2_H2);tft.print(" ppm");

    tft.setCursor(5, 21);
    tft.print("LPG :");printSpace(getMQ2_LPG);tft.print(" ppm");
    tft.setCursor(5, 35);
    tft.print("CO  :");printSpace(getMQ2_CO);tft.print(" ppm");
    tft.setCursor(5, 49);
    tft.print("ALCO:");printSpace(getMQ2_Alcohol);tft.print(" ppm");
    tft.setCursor(5, 63);
    tft.print("C3H8:");printSpace(getMQ2_Propane);tft.print(" ppm");
    tft.setCursor(5, 77);
    tft.print("VOLT:");printSpace(getMQ2_Voltage);tft.print(" V");
    isTftFirstTime = false;
  } else {
    tft.fillRectangle(35,7,63,7,BLACK);
    tft.setCursor(35,7);printSpace(getMQ2_H2);
    tft.fillRectangle(35,21,63,7,BLACK);
    tft.setCursor(35,21);printSpace(getMQ2_LPG);
    tft.fillRectangle(35,35,63,7,BLACK);
    tft.setCursor(35,35);printSpace(getMQ2_CO);
    tft.fillRectangle(35,49,63,7,BLACK);
    tft.setCursor(35,49);printSpace(getMQ2_Alcohol);
    tft.fillRectangle(35,63,63,7,BLACK);
    tft.setCursor(35,63);printSpace(getMQ2_Propane);
    tft.fillRectangle(35,77,63,7,BLACK);
    tft.setCursor(35,77);printSpace(getMQ2_Voltage);
  }
  tft.setCursor(35,91);
  tft.print("Height: ");tft.print(tft.getHeight());
  tft.print("Width: ");tft.print(tft.getWidth());
}