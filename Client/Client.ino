#include <OneWire.h>
#include <DS18B20.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "SparkFunBME280.h"
#include "LowPower.h"
#include <gp20u7.h>

/*
 * Sensors
 */
#define ONE_WIRE_BUS 40
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 water_temp_sensor(&oneWire);
BME280 airSensor;
const byte photoPin = A8;
GP20U7 gps = GP20U7(Serial2); //RX pin 19
Geolocation currentLocation;



/*
 * JSON and data fields
 */
 StaticJsonDocument<200> data;
 float temp_water_C = NULL;
 float temp_air_C = NULL;
 float humidity_rel = NULL;
 float pressure_Pa = NULL;
 int light_res = NULL;
 float longitude = NULL;
 float latitude = NULL;


void setup()
{
  Serial.begin(9600);
  /*
   * GPS
   */
  Serial2.begin(9600);
  gps.begin();
  
  water_temp_sensor.begin();
  Wire.begin();
  if (airSensor.beginI2C() == false) //Begin communication over I2C
  {
    Serial.println("The airsensor did not respond. Please check wiring.");
    //while(1);
  }
}

void loop()
{
  //Reconnect to GSM
  //Multipoint Measurements?
  readData();
  updateJson();
  //Send data
  //waitRoutine();
}

/*
 * Data handling
 */

void clearData(){
 temp_water_C = NULL;
 temp_air_C = NULL;
 humidity_rel = NULL;
 pressure_Pa = NULL;
 light_res = NULL;
 longitude = NULL;
 latitude = NULL;
}

void readData(){
  temp_water_C = getWaterTemp();
  temp_air_C = airSensor.readTempC() ;
  humidity_rel = airSensor.readFloatHumidity();
  pressure_Pa = airSensor.readFloatPressure();
  light_res = analogRead(photoPin);
  getCoordinates();
  
}

void updateJson(){
  data["general"]["water_temp"] = temp_water_C;
  data["general"]["air_temp"] = temp_air_C;
  data["general"]["humidity"] = humidity_rel;
  data["general"]["pressure"] = pressure_Pa;
  data["general"]["light"] = light_res;
  data["general"]["longitude"] = longitude;
  data["general"]["latitude"] = latitude;
  serializeJson(data, Serial);
  Serial.println();
}

float getWaterTemp(){
  water_temp_sensor.requestTemperatures();
  while(!water_temp_sensor.isConversionComplete());
  float temp = water_temp_sensor.getTempC();
  return temp;
}


/*
 *  Updates global latitude and longitude variables 
 */
int getCoordinates() {
  unsigned long time_reference = millis();
  while(millis()-time_reference < 3000 ){
    if (gps.read()) {
      currentLocation = gps.getGeolocation();
      latitude = currentLocation.latitude;
      longitude = currentLocation.longitude;
      return 1;
    }
  }
  longitude = NULL;
  latitude = NULL;
  return -1;
}
float getLongitude(){
  
}

float getLatitude(){
  
}

void waitRoutine(){
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

/*
 * Network
 */

 void reconnect(){
  
 }
 
