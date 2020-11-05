/*
 * Libraries
 */
#include "ntnu.h"
#include "Arduino.h"
#include <MKRNB.h>
#include <PubSubClient.h>
#include <gp20u7.h>
#include "ArduinoLowPower.h"

/*
   Sensor declarations and PINS
*/
GP20U7 gps = GP20U7(Serial1); //RX pin for Serial1 (PIN 13, MKR 1500)

/*
 * JSON and global data fields
 */
StaticJsonDocument<200> DATA;
char DATA_POINTS[NUM_READINGS][200];
float longitude;
float latitude;
unsigned long timestamp;
Geolocation currentLocation; //GPS struct

const char PIN_CODE[] = "";
char mqttBroker[] = "illustrations.marin.ntnu.no";
int mqttPort = 1883;
char pubTopic[100]; //format: ntnu/username/data

NBClient nbClient;
GPRS gprs;
NB nbAccess;
PubSubClient mqttClient;
NBModem modem;

/*
 * Sensor reads
 */
void gpsBegin(){
  gps.begin();
}

int getCoordinates() {
  unsigned long time_reference = millis();
  while(millis()-time_reference < 800 ){
    if (gps.read()) {
      currentLocation = gps.getGeolocation();
      return 1;
    }
  }
  return 0;
}

void updateCoordinates(){
  if(getCoordinates()){
    longitude = currentLocation.longitude;
    latitude = currentLocation.latitude;  
  }
  else{
    longitude = 0;
    latitude = 0;
  }
}

void updateTimestamp(){
  if (nbClient.connected()){
    timestamp = nbAccess.getLocalTime();
  }
}

/*/
 * Network
 */

void mqttClientConfiguration(){
  mqttClient.setClient(nbClient);
  mqttClient.setServer(mqttBroker, mqttPort); //Illustrations.marin.ntnu.no, 1883 
}

String getIMEI(){
  if(modem.begin()){
    String imei = modem.getIMEI();
    return imei;
  } 
  else{
    return "Could not find IMEI";
  }
}

void setTopic(){
  strcpy(pubTopic,"ntnu/");
  strcat(pubTopic, MQTT_CLIENT_NAME);
  strcat(pubTopic,"/data");
}

boolean lteConnect(){
/*
 * Tries to make an LTE connection to the provider
 * nbAccess.begin() is a blocking call, this function will NOT return before connection is made...
 * Further development: Add timeout 
 */
  Serial.println("lteConnect()");
  return (nbAccess.begin(PIN_CODE) == NB_READY) && (gprs.attachGPRS() == GPRS_READY);
}

boolean lteReconnect(){
/*
 * Tries to reconnect the LTE connection for 15000 milliseconds
 */
  Serial.println("LTE Reconnect()");
  unsigned long time_reference = millis();
  while(millis()-time_reference < 15000) {
    if(lteConnect()){
      Serial.println("-> LTE Reconnected");
      return true;
    }
    delay(200);
  }
  Serial.println("-> LTE Reconnect timed out");
  return false;
}

boolean mqttConnect(){
/*
 * Tries to make an MQTT connection to the broker once
 */
  Serial.println("mqttConnect()");
  return mqttClient.connect(MQTT_CLIENT_NAME);
}

boolean mqttReconnect(){
/*
 * Tries to reconnect the MQTT client to the broker for 15000 milliseconds
 */    
  Serial.println("MQTT Reconnect()");
  unsigned long time_reference = millis();
  while (millis() - time_reference < 15000) {
    if(mqttConnect()){
      Serial.println("-> MQTT Reconnected");    
      return true;
    }
    else{
      delay(200);
    }
  }
  Serial.println("-> MQTT Reconnect timed out");
  return false;
}

boolean makeConnections(){
/*
 *  Used for the initial connection.
 *  Returns true if succesful, false if not 
 */
  Serial.println("makeConnections()");
  if(lteConnect()){
      return mqttConnect();
  }
  return false;
}

boolean checkConnection(){
/*
 * true if connection is OK or reconnect is OK, 
 * false if no connection available after trying to reconnect
 */
  if (mqttClient.connected()){
    return true;
  }
  if(nbClient.connected()){
    return mqttReconnect();  
  }
  return lteReconnect() && mqttReconnect();
}

/*
 * Utilities
 */

void updateData() {
  DATA["timestamp"] = timestamp;
  DATA["longitude"] = longitude;
  DATA["latitude"] = latitude;
}

void sendData(){
  char buffer[200];
  size_t n;
  for(int i=0; i<NUM_READINGS; i++){
    n = serializeJson(DATA, buffer);
    mqttClient.publish(pubTopic, buffer, n);
  }
}

void updateReadings() {
  updateCoordinates();
  updateTimestamp();
}

void beginSerial(){
  Serial.begin(9600);
  while(!Serial){;}
}

void networkSetup(){
  mqttClientConfiguration();
  makeConnections();
}

void IMT_SETUP(){
  beginSerial();
  printIMEI();
  setTopic();
  gpsBegin();
  //networkSetup();
}

void IMT_READ(){
  updateReadings();
  updateData();
}

void IMT_SEND(){
  if(checkConnection()){
    sendData();
  }
}

void DEEP_SLEEP(int seconds) {
  if(seconds > 0){
    LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, beginSerial, CHANGE); //Forsøk på å få serial output
    LowPower.sleep(seconds*1000); //Kobler ut Serial
  }  
}

void STORE_DATA(int index){
  char buff[200];
  serializeJson(DATA,buff);
  strcpy(DATA_POINTS[index], buff);
}

void printStorage(){
  for(int i=0; i<NUM_READINGS; i++){
    Serial.println(DATA_POINTS[i]);
  }
}

void printIMEI(){
  Serial.print("IMEI: ");
  Serial.println(getIMEI());
}

void printTopic(){
  Serial.println(pubTopic);
}

void printData(){
  serializeJson(DATA, Serial);
  Serial.println();
}