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
char httpServer[] = "34.90.123.229";
int port = 80;
char pubTopic[100]; //format: ntnu/username/data

NBClient nbClient;
NBScanner scannerNetworks;
GPRS gprs;
NB nbAccess;
NBModem modem;
PubSubClient mqttClient;


/*
 * Sensor reads
 */
void gpsBegin(){
  gps.begin();
}

int getCoordinates() {
  unsigned long time_reference = millis();
  while(millis()-time_reference < 2000 ){
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

/*
 * Network
 */

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
      return true;
    }
  }
  Serial.println("-> LTE connection timed out");
  return false;
}

boolean clientConnect(){
/*
 * Tries to make a HTTP connection to the server once.
 */
  Serial.println("clientConnect()");
  return nbClient.connect(httpServer, port);
}

boolean clientReconnect(){
/*
 * Tries to reconnect the HTTP client to the server.
 * The client will try to reconnect at least until NETWORK_TIMEOUT has been reached
 */
  Serial.println("clientReconnect()");
  if(!nbClient.connected()){
    unsigned long time_reference = millis();
    while(millis() - time_reference < NETWORK_TIMEOUT*1000){
      if(clientConnect()){
        Serial.println("Client connected!");
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
    Serial.print("Network provider: ");
    Serial.println(scannerNetworks.getCurrentCarrier());
    Serial.print("Signal strength [0-31]: ");
    Serial.println(scannerNetworks.getSignalStrength());
    return clientReconnect();
  }
  Serial.println("WARNING: Could not make connections.");
  return false;
}

boolean checkConnection(){
/*
 * true if connection is OK or reconnect is OK, 
 * false if no connection available after trying to reconnect
 */
  if(nbClient.connected()){
    return true;  
  }
  return lteReconnect() && clientReconnect();
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
  /*
    Try to connect to Serial for a maximum of 5 seconds.
    Timeout is here in case of Arduino being run on battery power.
    Then Serial connection can not be established.
  */
  Serial.begin(9600);

  unsigned long time_reference = millis();  
  while(!Serial && millis() - time_reference < 5000){;}
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