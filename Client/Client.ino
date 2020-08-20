/*
 * Libraries
 */
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

/*
 *      TODO
 *  -Test with battery power
 *  -Final test
 */

/*
   Sensor declarations and PINS
*/
#define PHOTO_PIN A1
#define BATTERY_PIN A2
#define PANEL_PIN A3
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 water_temp_sensor(&oneWire);
BME280 airSensor;
GP20U7 gps = GP20U7(Serial1); //RX pin for Serial1 (PIN 13, MKR 1500)

/*
 * Frequency (Hz) and Period (seconds) of sensor measurements
 */
#define WATER_FREQ    0.2
#define WATER_PERIOD  10
#define AIR_FREQ      0.2
#define AIR_PERIOD    10
#define LIGHT_FREQ    0.2
#define LIGHT_PERIOD  10
#define GPS_FREQ      1
#define GPS_PERIOD    10
#define VOLTAGE_FREQ 0.1
#define VOLTAGE_PERIOD 5

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
float battery_voltage[(int)ceil(VOLTAGE_FREQ*VOLTAGE_PERIOD)];
float panel_voltage[(int)ceil(VOLTAGE_FREQ*VOLTAGE_PERIOD)];
unsigned long timestamp;
Geolocation currentLocation; //GPS struct

/*
 * Message frequency (seconds)
 */
#define MSG_INTERVAL 10

/*/
 * Network
 */
 
 #define MQTT_CLIENT_NAME "ntnu_test"
 #define MQTT_CLIENT_USERNAME "test"
 #define MQTT_CLIENT_PASSWORD "test"
 const char PIN_CODE[] = "";
 char mqttBroker[] = "illustrations.marin.ntnu.no";
 int mqttPort = 1883;
 char pubTopic[] = "ntnu/client_id/1";

 NBClient nbClient;
 GPRS gprs;
 NB nbAccess;
 PubSubClient mqttClient;
 
 

void setup()
{
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  writeSummary();
  checkInterval();
  sensorBegin();
  mqttClientConfiguration();
  lteConnect();
  mqttConnect();
}

void loop()
{ 
  unsigned long time_reference = millis();
  clearData();
  readData();
  updateJson();
  if(checkConnection()){
     sendData();   
  }
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
  Serial.println("clearData()");
  memset(&water_temp,0x00, (int)ceil(WATER_FREQ*WATER_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_pressure,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&light_res,0x00, (int)ceil(LIGHT_FREQ*LIGHT_PERIOD));
  memset(&longitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD));
  memset(&latitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD));
  memset(&battery_voltage,0x00, (int)ceil(VOLTAGE_FREQ*VOLTAGE_PERIOD));
  memset(&panel_voltage,0x00, (int)ceil(VOLTAGE_FREQ*VOLTAGE_PERIOD));
  memset(&timestamp,0x00, sizeof(unsigned long));
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
  periodicRead(VOLTAGE_PERIOD, VOLTAGE_FREQ, readVoltage);
  readTimestamp();
}

/*
 * Sensor reads
 */


void readAirSensor(int index){
  air_temp[index] = airSensor.readTempC();
  air_humidity[index] = airSensor.readFloatHumidity();
  air_pressure[index] = airSensor.readFloatPressure();
}

void readLightIntensity(int index){
  light_res[index] = analogRead(PHOTO_PIN);
}

void readWaterTemp(int index){
  water_temp[index] = getWaterTemp();
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

void readTimestamp(){
  if (nbClient.connected()){
    timestamp = nbAccess.getLocalTime();
  }
}

void readVoltage(int index){
  battery_voltage[index] = getVoltageBattery();
  panel_voltage[index] = getVoltagePanel();
}

float getVoltageBattery(){
  float batteryVoltage = analogRead(BATTERY_PIN) * (5/1023);
}

float getVoltagePanel(){
  float panelVoltage = analogRead(PANEL_PIN) * (5/1023);
}

/*
   Network
*/
void sendData(){
  Serial.println("sendData()");
  char buffer[200];
  size_t n = serializeJson(data, buffer);
  mqttClient.publish(pubTopic, buffer, n);
  Serial.println("-> Published data");
}

/*
 * Setup for MKR NB 1500 og MQTT attach
 */

void mqttClientConfiguration(){
  mqttClient.setClient(nbClient);
  mqttClient.setServer(mqttBroker, mqttPort); //Illustrations.marin.ntnu.no, 1883 
}

int lteConnect(){
  Serial.println("lteConnect()");
  unsigned long time_reference = millis();
  while(millis()-time_reference < 15000) {
    if((nbAccess.begin(PIN_CODE) == NB_READY) && (gprs.attachGPRS() == GPRS_READY))
    {
      Serial.println("-> Connected");
      return 1;
    }
    delay(200);
  }
  Serial.println("-> Not Connected");
  return 0;
}

boolean mqttConnect(){
  return mqttClient.connect(MQTT_CLIENT_NAME, MQTT_CLIENT_USERNAME, MQTT_CLIENT_PASSWORD);
}

boolean checkConnection(){
  if (mqttClient.connected()){
    return true;
  }
  return reconnect();
}

boolean reconnect() {
  Serial.println("Reconnect()");
  unsigned long time_reference = millis();
  while (millis() - time_reference < 20000) {
    // Attemp to connect for 20 seconds
    if(mqttConnect()){
      Serial.println("-> Reconnected");    
      return true;
    }
    else{
      // Wait 1 second before retrying
      delay(1000);
    }
  }
  Serial.println("-> Reconnect timed out");
  return false;
}

/*
 * Utilities
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
  Serial.print("VOLTAGE\t");
  Serial.print(VOLTAGE_FREQ);
  Serial.print("\t");
  Serial.println(VOLTAGE_PERIOD);
  Serial.println();
  Serial.print("MSG INTERVAL\t");
  Serial.println(MSG_INTERVAL);
  Serial.println();
}

void updateJson() {
  data["general"]["timestamp"] = timestamp;
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
    //LowPower.sleep(seconds_left*1000);
    delay(seconds_left*1000);
  }  
}

void checkInterval(){
  if(MSG_INTERVAL < AIR_PERIOD + WATER_PERIOD + GPS_PERIOD + LIGHT_PERIOD){
    Serial.println("WARNING: Message Interval shorter than Measurement Period!");
  }
}
