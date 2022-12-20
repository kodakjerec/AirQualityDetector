#include <WiFi.h>
#include "MQUnifiedsensor.h"

// WiFi
//SSID e senha da rede wifi para o ESP se conectar
char ssid[] = "kodakjerecHTC";
char pass[] = "xj4su6u83";
int status = WL_IDLE_STATUS;     // the Wifi radio's status


// Button
int ButtonStatus = LOW;
int lastButtonStatus = HIGH;
int debounceDelay = 50;
long lastDebounceTime = 0;

//Definitions
#define placa "Ameba BW16 typeC"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO

#define pinMQ2 A2
#define RatioMQ2CleanAir (9.83) //RS / R0 = 9.83 ppm

//#define calibration_button 13 //Pin to calibrate your sensor

//Declare Sensor
MQUnifiedsensor MQ2(placa, Voltage_Resolution, ADC_Bit_Resolution, pinMQ2, "MQ-2");

// ** all data
bool  isCalibration = true;  // calibration Mode
bool  isLimit = false; // add limit output
float  getMQ2_H2, getMQ2_LPG, getMQ2_CO, getMQ2_Alcohol, getMQ2_Propane;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // wifi_setup();
  MQ_setup();
  // pinMode(PA30, INPUT_PULLUP); // Button
}


void loop() {
  // put your main code here, to run repeatedly:
  MQ_loop();
  button_loop();
  delay(1000);
}

void wifi_setup(){
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // don't continue:
        while (true);
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

void MQ_setup(){
  //Set math model to calculate the PPM concentration and the value of constants
  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ2.init();

  if (isCalibration){
    MQ2_Calibration();
  }
  else {
   MQ2.setR0(1.7);
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
  Serial.println("Calibrating please wait.");
  float calR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update();
    calR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(i);Serial.print(" ");
    delay(100);
  }
  MQ2.setR0(calR0/10);
  Serial.println("MQ2 done!.");
  Serial.print("MQ2.getR0: ");Serial.print(MQ2.getR0());Serial.print(" | ");
  
  if(isinf(calR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  // MQ2.serialDebug(true);
}
// 取值
void MQ_loop(){
  MQ2.update();
  if (isCalibration) {
    MQ2.serialDebug();
    return;
  }
  // MQ2
  MQ2.setA(987.99); MQ2.setB(-2.162);
  getMQ2_H2 = MQ2.readSensor();
  MQ2.setA(574.25); MQ2.setB(-2.222);
  getMQ2_LPG = MQ2.readSensor();
  MQ2.setA(36974); MQ2.setB(-3.109);
  getMQ2_CO = MQ2.readSensor();
  MQ2.setA(3616.1); MQ2.setB(-2.675);
  getMQ2_Alcohol = MQ2.readSensor();
  MQ2.setA(658.71); MQ2.setB(-2.168);
  getMQ2_Propane = MQ2.readSensor();
  if (isLimit){
    if (getMQ2_H2<300) getMQ2_H2=0;
    else if (getMQ2_H2>5000) getMQ2_H2=5000;
    if (getMQ2_LPG<200) getMQ2_LPG=0;
    else if (getMQ2_LPG>5000) getMQ2_LPG=5000;
    if (getMQ2_CO<300) getMQ2_CO=0;
    else if (getMQ2_CO>5000) getMQ2_CO=5000;
    if (getMQ2_Alcohol<100) getMQ2_Alcohol=0;
    else if (getMQ2_Alcohol>2000) getMQ2_Alcohol=2000;
    if (getMQ2_Propane<200) getMQ2_Propane=0;
    else if (getMQ2_Propane>5000) getMQ2_Propane=5000;
  }

  Serial.println("  MQ2:H2  |    LPG   |    CO    |  Alcohol |  Propane |");
  printSpace(getMQ2_H2);
  Serial.print("|");
  printSpace(getMQ2_LPG);
  Serial.print("|");
  printSpace(getMQ2_CO);
  Serial.print("|");
  printSpace(getMQ2_Alcohol);
  Serial.print("|");
  printSpace(getMQ2_Propane);
  Serial.print("|");
  Serial.println("");
}
void printSpace(float object){
  int length = 6;
  int tempObject = object/1;
  while(tempObject/10>0){
    length--;
    tempObject = tempObject/10;
  }
  while(length>0){
    Serial.print(" ");
    length--;
  }
  Serial.print(object);
}

void button_loop(){
  // put your main code here, to run repeatedly:
  int reading = digitalRead(PA30);
  if (reading != lastButtonStatus) {
    lastDebounceTime = millis();
  }
  if (millis()-lastDebounceTime>50){
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
