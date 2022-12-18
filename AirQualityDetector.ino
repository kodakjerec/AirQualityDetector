#include <WiFi.h>
#include "MQUnifiedsensor.h"

// WiFi
//SSID e senha da rede wifi para o ESP se conectar
char ssid[] = "kodakjerecHTC";
char pass[] = "xj4su6u83";
int status = WL_IDLE_STATUS;     // the Wifi radio's status


// MQ5

//Definitions
#define placa "Arduino UNO"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO

#define pinMQ2 A2
#define RatioMQ2CleanAir (9.83) //RS / R0 = 9.83 ppm 

#define pinMQ5 PB1 //Analog input 0 of your arduino
#define RatioMQ5CleanAir (6.5)  //RS / R0 = 6.5 ppm

#define pinMQ135 PB2
#define RatioMQ135CleanAir (3.6)//RS / R0 = 3.6 ppm  

//#define calibration_button 13 //Pin to calibrate your sensor

//Declare Sensor
MQUnifiedsensor MQ2(placa, Voltage_Resolution, ADC_Bit_Resolution, pinMQ2, "MQ-2");
MQUnifiedsensor MQ5(placa, Voltage_Resolution, ADC_Bit_Resolution, pinMQ5, "MQ-5");
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pinMQ135, "MQ-135");

// ** all data
bool  isCalibration = false;
float  getMQ2_H2, getMQ2_LPG, getMQ2_CO, getMQ2_Alcohol, getMQ2_Propane;
float  getMQ5_H2, getMQ5_LPG, getMQ5_CH4, getMQ5_CO, getMQ5_Alcohol;
float  getMQ135_CO, getMQ135_Alcohol, getMQ135_CO2, getMQ135_Toluen, getMQ135_NH4, getMQ135_Aceton;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // wifi_setup();
  MQ_setup();
}


void loop() {
  // put your main code here, to run repeatedly:
  MQ_loop();
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

// #region-MQ5
void MQ_setup(){
  pinMode(PB1, INPUT);
  pinMode(PB2, INPUT);
  pinMode(A2, INPUT);
  //Set math model to calculate the PPM concentration and the value of constants
  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ5.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ2.init();
  MQ5.init();
  MQ135.init();

  if (isCalibration){
    MQ2_Calibration();
    MQ5_Calibration();
    MQ135_Calibration();
  }
  else {
   MQ2.setR0(3.07);
   MQ5.setR0(4.31);
   MQ135.setR0(6.43);
  }
}
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
    Serial.println(".");
  }
  MQ2.setR0(calR0/10);
  Serial.println("  done!.");
  
  if(isinf(calR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  MQ2.serialDebug(true);
}
void MQ5_Calibration() {
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
    MQ5.update(); // Update data, the arduino will read the voltage from the analog pin
    calR0 += MQ5.calibrate(RatioMQ5CleanAir);
    Serial.println(".");
  }
  MQ5.setR0(calR0/10);
  Serial.println("  done!.");
  
  if(isinf(calR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  MQ5.serialDebug(true);
}
void MQ135_Calibration() {
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
    MQ135.update();
    calR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.println(".");
  }
  MQ135.setR0(calR0/10);
  Serial.println("  done!.");
  
  if(isinf(calR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  Serial.println("** Values from MQ-135 ****");
  Serial.println("|    CO   |  Alcohol |   CO2  |  Toluen  |  NH4  |  Aceton  |"); 
}
void MQ_loop(){
  MQ2.update();
  MQ5.update();
  MQ135.update();
  if (isCalibration){
    Serial.print("MQ2.getR0: ");
    Serial.println(MQ2.getR0());
    MQ2.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    MQ2.serialDebug(); // Will print the table on the serial port
    Serial.print("MQ5.getR0: ");
    Serial.println(MQ5.getR0());
    MQ5.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    MQ5.serialDebug(); // Will print the table on the serial port
    Serial.print("MQ135.getR0: ");
    Serial.println(MQ135.getR0());
    MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    MQ135.serialDebug(); // Will print the table on the serial port
  }
  else {
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
    if (getMQ2_H2<300) getMQ2_H2=0;
    if (getMQ2_H2>5000) getMQ2_H2=5000;
    if (getMQ2_LPG<200) getMQ2_LPG=0;
    if (getMQ2_LPG>5000) getMQ2_LPG=5000;
    if (getMQ2_CO<300) getMQ2_CO=0;
    if (getMQ2_CO>5000) getMQ2_CO=5000;
    if (getMQ2_Alcohol<100) getMQ2_Alcohol=0;
    if (getMQ2_Alcohol>2000) getMQ2_Alcohol=2000;
    if (getMQ2_Propane<200) getMQ2_Propane=0;
    if (getMQ2_Propane>5000) getMQ2_Propane=5000;

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

    // MQ5
    MQ5.setA(1163.8); MQ5.setB(-3.874);
    getMQ5_H2 = MQ5.readSensor();
    MQ5.setA(80.897); MQ5.setB(-2.431);
    getMQ5_LPG = MQ5.readSensor();
    MQ5.setA(177.65); MQ5.setB(-2.56);
    getMQ5_CH4 = MQ5.readSensor();
    MQ5.setA(491204); MQ5.setB(-5.826);
    getMQ5_CO = MQ5.readSensor();
    MQ5.setA(97124); MQ5.setB(-4.918);
    getMQ5_Alcohol = MQ5.readSensor();
    if (getMQ5_H2<200) getMQ5_H2=0;
    if (getMQ5_H2>10000) getMQ5_H2=10000;
    if (getMQ5_LPG<200) getMQ5_LPG=0;
    if (getMQ5_LPG>10000) getMQ5_LPG=10000;
    if (getMQ5_CH4<200) getMQ5_CH4=0;
    if (getMQ5_CH4>10000) getMQ5_CH4=10000;
    if (getMQ5_CO<200) getMQ5_CO=0;
    if (getMQ5_CO>10000) getMQ5_CO=10000;
    if (getMQ5_Alcohol<200) getMQ5_Alcohol=0;
    if (getMQ5_Alcohol>10000) getMQ5_Alcohol=10000;

    Serial.println("  MQ5:H2  |    LPG   |    CH4   |    CO    |  Alcohol |");
    printSpace(getMQ5_H2);
    Serial.print("|");
    printSpace(getMQ5_LPG);
    Serial.print("|");
    printSpace(getMQ5_CH4);
    Serial.print("|");
    printSpace(getMQ5_CO);
    Serial.print("|");
    printSpace(getMQ5_Alcohol);
    Serial.print("|");
    Serial.println("");
    
    // MQ135
    MQ135.setA(605.18); MQ135.setB(-3.937);
    getMQ135_CO = MQ135.readSensor();
    MQ135.setA(77.255); MQ135.setB(-3.18);
    getMQ135_Alcohol = MQ135.readSensor();
    MQ135.setA(110.47); MQ135.setB(-2.862);
    getMQ135_CO2 = MQ135.readSensor();
    MQ135.setA(44.947); MQ135.setB(-3.445);
    getMQ135_Toluen = MQ135.readSensor();
    MQ135.setA(102.2 ); MQ135.setB(-2.473);
    getMQ135_NH4 = MQ135.readSensor();
    MQ135.setA(34.668); MQ135.setB(-3.369);
    getMQ135_Aceton = MQ135.readSensor();
    if (getMQ135_CO<10) getMQ135_CO=0;
    if (getMQ135_CO>1000) getMQ135_CO=1000;
    if (getMQ135_Alcohol<10) getMQ135_Alcohol=0;
    if (getMQ135_Alcohol>1000) getMQ135_Alcohol=1000;
    if (getMQ135_CO2<10) getMQ135_CO2=0;
    if (getMQ135_CO2>1000) getMQ135_CO2=1000;
    if (getMQ135_Toluen<10) getMQ135_Toluen=0;
    if (getMQ135_Toluen>1000) getMQ135_Toluen=1000;
    if (getMQ135_NH4<10) getMQ135_NH4=0;
    if (getMQ135_NH4>1000) getMQ135_NH4=1000;
    if (getMQ135_Aceton<10) getMQ135_Aceton=0;
    if (getMQ135_Aceton>1000) getMQ135_Aceton=1000;

    Serial.println(" MQ135:CO |  Alcohol |    CO2   |  Toluen  |    NH4   |  Aceton  |");
    printSpace(getMQ135_CO);
    Serial.print("|");
    printSpace(getMQ135_Alcohol);
    Serial.print("|");
    printSpace(getMQ135_CO2);
    Serial.print("|");
    printSpace(getMQ135_Toluen);
    Serial.print("|");
    printSpace(getMQ135_NH4);
    Serial.print("|");
    printSpace(getMQ135_Aceton);
    Serial.print("|");
    Serial.println("");

    // MQ2.serialDebug();
    // MQ5.serialDebug();
    // MQ135.serialDebug();
  }
  delay(1000);
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
// #endregion-MQ5
