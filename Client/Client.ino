#include <OneWire.h>
#include <DS18B20.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "SparkFunBME280.h"
#include "LowPower.h"
#include <gp20u7.h>
#include <SPI.h>
//#include <SparkFunLSM9DS1.h>

/*
   Sensors
*/
#define ONE_WIRE_BUS 40
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 water_temp_sensor(&oneWire);
BME280 airSensor;
const byte photoPin = A8;
GP20U7 gps = GP20U7(Serial2); //RX pin 19
Geolocation currentLocation;
//LSM9DS1 imu;

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
//#define IMU_FREQ 4
//#define IMU_PERIOD 300

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
//float g_x[(int)ceil(IMU_FREQ*IMU_PERIOD)];
//float g_y[(int)ceil(IMU_FREQ*IMU_PERIOD)];
//float g_z[(int)ceil(IMU_FREQ*IMU_PERIOD)];
//float a_x[(int)ceil(IMU_FREQ*IMU_PERIOD)];
//float a_y[(int)ceil(IMU_FREQ*IMU_PERIOD)];
//float a_z[(int)ceil(IMU_FREQ*IMU_PERIOD)];

void setup()
{
  Serial.begin(9600);
  /*
     GPS
  */
  Serial2.begin(9600);
  gps.begin();

  water_temp_sensor.begin();
  Wire.begin();
  if (airSensor.beginI2C() == false) //Begin communication over I2C
  {
    Serial.println("Failed to communicate with BME280.");
  }
//  if (imu.begin() == false) // with no arguments, this uses default addresses (AG:0x6B, M:0x1E) and i2c port (Wire).
//  {
//    Serial.println("Failed to communicate with LSM9DS1.");
//  }
}

void loop()
{
  //Reconnect to GSM
  
  //Send data every hour! Take into account reading times in the sleep.
  
  clearData();
  readData();
  updateJson();
  //Send data
  //waitRoutine();
}

/*
   Data handling
*/

/*
   clearData does not work at the moment because readings are gotten from the sensors directly.
   Have to check if they are connected and returning fresh data.
*/
void clearData() {
  memset(&water_temp,0x00, (int)ceil(WATER_FREQ*WATER_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_temp,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&air_pressure,0x00, (int)ceil(AIR_FREQ*AIR_PERIOD));
  memset(&light_res,0x00, (int)ceil(LIGHT_FREQ*LIGHT_PERIOD));
  memset(&longitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD));
  memset(&latitude,0x00, (int)ceil(GPS_FREQ*GPS_PERIOD));
//  memset(&g_x,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
//  memset(&g_y,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
//  memset(&g_z,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
//  memset(&a_x,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
//  memset(&a_y,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
//  memset(&a_z,0x00, (int)ceil(IMU_FREQ*IMU_PERIOD));
  
  
}
/*
 *Les data 
 */

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
  periodicRead(AIR_PERIOD, AIR_FREQ, readAirSensor);
  periodicRead(WATER_PERIOD, WATER_FREQ, readWaterTemp);
  periodicRead(LIGHT_PERIOD, LIGHT_FREQ, readLightIntensity);
  periodicRead(GPS_PERIOD, GPS_FREQ, readCoordinates);
  //imu_read();
}


void readWaterTemp(int index){
  water_temp[index] = getWaterTemp();
}

void readAirSensor(int index){
  air_temp[index] = airSensor.readTempC();
  air_humidity[index] = airSensor.readFloatHumidity();
  air_pressure[index] = airSensor.readFloatPressure();
}

void readLightIntensity(int index){
  light_res[index] = analogRead(photoPin);
}

void readCoordinates(int index){
  getCoordinates();
  longitude[index] = currentLocation.longitude;
  latitude[index] = currentLocation.latitude;
}


void updateJson() {
  data["general"]["water_temp"] = array_avg(water_temp, (int)ceil(WATER_FREQ*WATER_PERIOD));
  data["general"]["air_temp"] = array_avg(air_temp, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["humidity"] = array_avg(air_humidity, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["pressure"] = array_avg(air_pressure, (int)ceil(AIR_FREQ*AIR_PERIOD));
  data["general"]["light"] = array_avg(light_res, (int)ceil(LIGHT_FREQ*LIGHT_PERIOD));
  data["general"]["longitude"] = array_avg(longitude, (int)ceil(GPS_FREQ*GPS_PERIOD));
  data["general"]["latitude"] = array_avg(latitude, (int)ceil(GPS_FREQ*GPS_PERIOD));

//  data["personal"]["gx"] = imu.calcGyro(imu.gx);
//  data["personal"]["gy"] = imu.calcGyro(imu.gy);
//  data["personal"]["gz"] = imu.calcGyro(imu.gz);
//  data["personal"]["ax"] = imu.calcAccel(imu.ax);
//  data["personal"]["ay"] = imu.calcAccel(imu.ay);
//  data["personal"]["az"] = imu.calcAccel(imu.az);
  serializeJson(data, Serial);
  Serial.println();
}

float getWaterTemp() {
  water_temp_sensor.requestTemperatures();
  float temperature = 0;
  
  while(!water_temp_sensor.isConversionComplete(){
    temperature = water_temp_sensor.getTempC();
  }
  return temperature;
}


/*
    Updates global latitude and longitude variables
*/
void getCoordinates() {
  if(gps.read()){
    currentLocation = gps.getGeolocation();
  }
}

//void imu_read() {
//  /*
//   *  Ta 4 målinger i sekundet. 
//   *  Ta målinger i 5 minutter.
//   *  -> 4*5*60 målinger * 4 bytes
//   */
//  if (imu.gyroAvailable()) {
//    imu.readGyro();
//  }
//  if (imu.accelAvailable()) {
//    imu.readAccel();
//  }    
//}

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

void waitRoutine() {
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

/*
   Network
*/

void reconnect() {

}
