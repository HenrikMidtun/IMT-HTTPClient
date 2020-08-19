#include <OneWire.h>
#include <DS18B20.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "SparkFunBME280.h"
#include <ArduinoLowPower.h>
#include <gp20u7.h>
#include <SPI.h>
#include <MKRNB.h>
#include <PubSubClient.h>


/*/
 *      TODO
 *  
 *  -Network functionality
 *  -MQTT
 *  -Timestamps
 */

/*
   Sensors
*/
#define ONE_WIRE_BUS 5
#define PHOTO_PIN A1
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 water_temp_sensor(&oneWire);
BME280 airSensor;
GP20U7 gps = GP20U7(Serial1); //RX pin for Serial1 (PIN 13, MKR 1500)
Geolocation currentLocation;

/*
 * Frequency (Hz) and Period (seconds) of sensor measurements
 */
#define WATER_FREQ 0.2
#define WATER_PERIOD 10
#define AIR_FREQ 0.2
#define AIR_PERIOD 10
#define LIGHT_FREQ 0.2
#define LIGHT_PERIOD 10
#define GPS_FREQ 1
#define GPS_PERIOD 10

/*
 * JSON and global data fields
 */
StaticJsonDocument<200> data;
float water_temp[(int)ceil(WATER_FREQ*WATER_PERIOD)];
float air_temp[(int)ceil(AIR_FREQ*AIR_PERIOD)];
float air_humidity[(int)ceil(AIR_FREQ*AIR_PERIOD)];
float air_pressure[(int)ceil(AIR_FREQ*AIR_PERIOD)];
float light_res[(int)ceil(LIGHT_FREQ*LIGHT_PERIOD)];
float longitude[(int)ceil(GPS_FREQ*GPS_PERIOD)];
float latitude[(int)ceil(GPS_FREQ*GPS_PERIOD)];

/*
 * Message frequency (seconds)
 */
#define MSG_INTERVAL 10

/*/
 * Network
 */
 const char PINNUMBER[] = "";
 #define MQTT_CLIENT_NAME "ntnu_test"
 char mqttBroker[] = "illustrations.marin.ntnu.no";
 int mqttPort = 1883;
 char pubTopic[] = "ntnu/test/1";

 NBClient nbClient;
 GPRS gprs;
 NB nbAccess;
 PubSubClient client(nbClient);
 

void setup()
{
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  writeSummary();
  checkInterval();
  sensorBegin();
  lteConnect();
}

void loop()
{ 
  //Send data every hour! Take into account reading times in the sleep.
  
  unsigned long time_reference = millis();
  clearData();
  readData();
  updateJson();
  sendData();
  unsigned long time_elapsed = millis() - time_reference;
  waitRoutine(time_elapsed);
}

/*/
 * Sensor setup
 */

void sensorBegin(){
  gps.begin();
  water_temp_sensor.begin();
  Wire.begin();
  if (airSensor.beginI2C() == false) //Begin communication over I2C
  {
    Serial.println("WARNING: Failed to communicate with BME280.");
  }
}
/*
   Data handling
*/
void clearData() {
  memset(&water_temp,0x00, (int)ceil(WATER_FREQ*WATER_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_pressure,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&light_res,0x00, (int)ceil(LIGHT_FREQ*LIGHT_PERIOD));
  memset(&longitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD));
  memset(&latitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD)); 
}

void periodicRead(int period, double freq, void readFunc(int)){
  unsigned long time_reference = millis();
  unsigned long previous_time;
  int i = 0;
  do{
    previous_time = millis();
    readFunc(i);
    i++;
    while(millis() - previous_time < (1/freq)*1000){;}
  }while(millis() - time_reference < period*1000);
}

void readData() {
  Serial.println("readData()");
  periodicRead(AIR_PERIOD, AIR_FREQ, readAirSensor);
  periodicRead(WATER_PERIOD, WATER_FREQ, readWaterTemp);
  periodicRead(LIGHT_PERIOD, LIGHT_FREQ, readLightIntensity);
  periodicRead(GPS_PERIOD, GPS_FREQ, readCoordinates);
}

/*
 * Sensor reads
 */
void readWaterTemp(int index){
  water_temp[index] = getWaterTemp();
}

void readAirSensor(int index){
  air_temp[index] = airSensor.readTempC();
  air_humidity[index] = airSensor.readFloatHumidity();
  air_pressure[index] = airSensor.readFloatPressure();
}

void readLightIntensity(int index){
  light_res[index] = analogRead(PHOTO_PIN);
}


float getWaterTemp() {
  water_temp_sensor.requestTemperatures();
  float w_temp = 0;
  
  uint32_t timeout = millis();
  while (!water_temp_sensor.isConversionComplete())
  {
    if (millis() - timeout >= 800) // check for timeout, 800ms
    {
      return w_temp;
    }
  }
  w_temp = water_temp_sensor.getTempC();
  return w_temp;
}


void readCoordinates(int index){
  if(getCoordinates()){
    longitude[index] = currentLocation.longitude;
    latitude[index] = currentLocation.latitude;  
  }
  else{
    longitude[index] = 0;
    latitude[index] = 0;
  }
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

/*
 * Utilities
 */

void updateJson() {
  data["general"]["water_temp"] = array_avg(water_temp, (int)ceil(WATER_FREQ*WATER_PERIOD));
  data["general"]["air_temp"] = array_avg(air_temp, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["humidity"] = array_avg(air_humidity, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["pressure"] = array_avg(air_pressure, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["light"] = array_avg(light_res, (int)ceil(LIGHT_FREQ*LIGHT_PERIOD));
  data["general"]["longitude"] = array_avg(longitude, (int)ceil(GPS_FREQ*GPS_PERIOD));
  data["general"]["latitude"] = array_avg(latitude, (int)ceil(GPS_FREQ*GPS_PERIOD));
}

float array_avg(float* arr, int len){
  float sum=0;
  int zero_fields=0;
  for(int i=0; i<len; i++){
    sum=sum+arr[i];
    if(arr[i] == 0){
      zero_fields++;
    }
  }
  float avg = 0;
  if(len-zero_fields > 0){
    avg = sum/(len-zero_fields);  
  }
  return  avg;
}

/*
 * Should handle if elapsed time is longer than msg interval
 */
void waitRoutine(unsigned long elapsed) {
  Serial.println("waitRoutine()");
  int seconds_left = floor(MSG_INTERVAL - elapsed / 1000);
  if(seconds_left > 0){
    LowPower.sleep(seconds_left*1000);
  }    
}


void checkInterval(){
  if(MSG_INTERVAL < AIR_PERIOD + WATER_PERIOD + GPS_PERIOD + LIGHT_PERIOD){
    Serial.println("WARNING: Message Interval shorter than Measurement Period!");
  }
}
/*
   Network
*/
void sendData(){
  Serial.println("sendData()");
  if(!client.connected()){
    reconnect();
  }
  char buffer[200];
  size_t n = serializeJson(data, buffer);
  client.publish(pubTopic, buffer, n);
  Serial.println("-> Published data");
}

void lteConnect(){
  Serial.println("lteConnect()");
  boolean connected = false;

  while(!connected) {
    if((nbAccess.begin(PINNUMBER) == NB_READY) && (gprs.attachGPRS() == GPRS_READY)) //takes approximately 5 minutes
    {
      connected = true;
    } 
    else{
      Serial.println("-> Not connected");
      delay(1000);
    }
  }
  Serial.println("-> Connected");

  //set the connection
  client.setServer(mqttBroker, mqttPort);
}

void reconnect() {
  Serial.println("Reconnect()");
  unsigned long time_reference = millis();
  while (millis() - time_reference < 20000) {
    // Attemp to connect for 20 seconds
    if (client.connect(MQTT_CLIENT_NAME)){
      Serial.println("-> Reconnected");
      return;
    } 
    else{
      // Wait 1 second before retrying
      delay(1000);
    }
  }
  Serial.println("-> Reconnect timed out");
}

/*/
 * Utils
 */

 void writeSummary(){
  Serial.println("Hello, I am Agnes 3!");
  Serial.println();
  Serial.println("\tFREQ\tPERIOD");
  Serial.print("WATER\t");
  Serial.print(WATER_FREQ);
  Serial.print("\t");
  Serial.println(WATER_PERIOD);
  Serial.print("AIR\t");
  Serial.print(AIR_FREQ);
  Serial.print("\t");
  Serial.println(AIR_PERIOD);
  Serial.print("LIGHT\t");
  Serial.print(LIGHT_FREQ);
  Serial.print("\t");
  Serial.println(LIGHT_PERIOD);
  Serial.print("GPS\t");
  Serial.print(GPS_FREQ);
  Serial.print("\t");
  Serial.println(GPS_PERIOD);
  Serial.println();
  Serial.print("MSG INTERVAL\t");
  Serial.println(MSG_INTERVAL);
  Serial.println();
 }
