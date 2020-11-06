/*
 * Libraries
 */
#include "ntnu.h"
#include "Arduino.h"
#include <MKRNB.h>
#include <PubSubClient.h>
#include <gp20u7.h>
#include "ArduinoLowPower.h"

#define NETWORK_TIMEOUT 240 //seconds

/*
   Sensor declarations and PINS
*/
GP20U7 gps = GP20U7(Serial1); //RX pin for Serial1 (PIN 13, MKR 1500)

/*
 * JSON and global data fields
 */
StaticJsonDocument<200> DATA;
char **DATA_POINTS;
int NUM_READINGS;
float longitude;
float latitude;
unsigned long timestamp;
Geolocation currentLocation; //GPS struct

String IMEI = "";
const char PIN_CODE[] = "";
char mqttBroker[] = "illustrations.marin.ntnu.no";
int mqttPort = 1883;
char pubTopic[100]; //format: ntnu/username/data

NBClient nbClient;
NBScanner scannerNetworks;
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
  timestamp = nbAccess.getLocalTime();
}

/*/
 * Network
 */

void mqttClientConfiguration(){
  mqttClient.setClient(nbClient);
  mqttClient.setServer(mqttBroker, mqttPort); //Illustrations.marin.ntnu.no, 1883 
}

boolean lteConnect(){
/*
 * Tries to make an LTE connection to the provider
 */
  return (nbAccess.begin(PIN_CODE) == NB_READY) && (gprs.attachGPRS() == GPRS_READY);
}

boolean lteReconnect(){
/*
 * Tries to reconnect the LTE connection with the provider.
 * Tries to reconnect for at least the duration of NETWORK_TIMEOUT
 */
  unsigned long time_reference = millis();
  while(millis()-time_reference < NETWORK_TIMEOUT*1000) {
    if(lteConnect()){
      Serial.println("-> LTE Connected");
      return true;
    }
  }
  Serial.println("-> LTE connection timed out");
  return false;
}

boolean mqttConnect(){
/*
 * Tries to make an MQTT connection to the broker once.
 * This may take a while...  Approximately 3 minutes
 */
  return mqttClient.connect(IMEI.c_str());
}

boolean mqttReconnect(){
/*
 * Tries to reconnect the MQTT client to the broker.
 * The client will try to reconnect at least until NETWORK_TIMEOUT has been reached
 */    
  if(!mqttClient.connected()){
    unsigned long time_reference = millis();
    while(millis() - time_reference < NETWORK_TIMEOUT*1000){
      if(mqttConnect()){
        Serial.println("-> Client Connected");    
        return true;
      }
    }
  Serial.println("-> Client connection timed out");
  return false;
  }

}

boolean makeConnections(){
/*
 *  Used for the initial connection.
 *  Returns true if succesful, false if not 
 */
  if(lteReconnect()){
    scannerNetworks.begin();
    Serial.print("Network provider: ")
    Serial.println(scannerNetworks.getCurrentCarrier());
    
    return mqttReconnect();
      
  }
  Serial.println("WARNING: Could not make connections.");
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
    mqttClient.publish(pubTopic, DATA_POINTS[i]);
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

void setIMEI(){
  modem.begin();
  IMEI = modem.getIMEI();
  while(IMEI == NULL || IMEI == ""){
    IMEI = modem.getIMEI();
    while(!modem.begin()){;}
  }
}

void setTopic(){
  strcpy(pubTopic,"ntnu/");
  strcat(pubTopic, IMEI.c_str());
  strcat(pubTopic,"/data");
}

void setNReadings(int n){
  if(n > 0){
    NUM_READINGS = n;
  }
  else{
    NUM_READINGS = 1;
  }
}

void initDataPoints(){
  DATA_POINTS = new char*[NUM_READINGS];
  for(int i=0; i<NUM_READINGS; i++){
    DATA_POINTS[i] = new char[200];
  }
}

void networkSetup(){
  Serial.println("Network setup");
  Serial.println("This may take a while...");
  nbAccess.setTimeout(NETWORK_TIMEOUT*1000);
  mqttClientConfiguration();
  makeConnections();
}

void IMT_SETUP(int n){
  beginSerial();
  setIMEI();
  printIMEI();
  setTopic();
  gpsBegin();
  setNReadings(n);
  initDataPoints();
  networkSetup();
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

void DEEP_SLEEP(int milliseconds) {
  if(milliseconds > 0){
    LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, beginSerial, CHANGE); //Forsøk på å få serial output
    LowPower.sleep(milliseconds); //Kobler ut Serial
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
  Serial.println(IMEI);
}

void printTopic(){
  Serial.println(pubTopic);
}

void printData(){
  serializeJson(DATA, Serial);
  Serial.println();
}