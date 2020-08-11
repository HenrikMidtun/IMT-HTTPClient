#include <OneWire.h>
#include <DS18B20.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "SparkFunBME280.h"
#include "LowPower.h"
#include <gp20u7.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>


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
LSM9DS1 imu;




/*
   JSON and data fields
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
     GPS
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
  if (imu.begin() == false) // with no arguments, this uses default addresses (AG:0x6B, M:0x1E) and i2c port (Wire).
  {
    Serial.println("Failed to communicate with LSM9DS1.");
    //while(1);
  }
}

void loop()
{
  //Reconnect to GSM
  //Multipoint Measurements?
  //clearData();
  readData();
  updateJson();
  //Send data
  //waitRoutine();
  delay(500);
}

/*
   Data handling
*/

/*
   clearData does not work at the moment because readings are gotten from the sensors directly.
   Have to check if they are connected and returning fresh data.
*/
void clearData() {
  temp_water_C = NULL;
  temp_air_C = NULL;
  humidity_rel = NULL;
  pressure_Pa = NULL;
  light_res = NULL;
  longitude = NULL;
  latitude = NULL;
}
/*
 *Les data 
 */
void readData() {
  //temp_water_C = getWaterTemp();
  temp_air_C = airSensor.readTempC() ;
  humidity_rel = airSensor.readFloatHumidity();
  pressure_Pa = airSensor.readFloatPressure();
  //light_res = analogRead(photoPin);
  //getCoordinates();
  imu_read();

}

void updateJson() {
  //data["general"]["water_temp"] = temp_water_C;
  data["general"]["air_temp"] = temp_air_C;
  data["general"]["humidity"] = humidity_rel;
  data["general"]["pressure"] = pressure_Pa;
  //data["general"]["light"] = light_res;
  //data["general"]["longitude"] = longitude;
  //data["general"]["latitude"] = latitude;
  data["personal"]["gx"] = imu.calcGyro(imu.gx);
  data["personal"]["gy"] = imu.calcGyro(imu.gy);
  data["personal"]["gz"] = imu.calcGyro(imu.gz);
  data["personal"]["ax"] = imu.calcAccel(imu.ax);
  data["personal"]["ay"] = imu.calcAccel(imu.ay);
  data["personal"]["az"] = imu.calcAccel(imu.az);
  serializeJson(data, Serial);
  Serial.println();
}

float getWaterTemp() {
  unsigned long time_reference = millis();
  water_temp_sensor.requestTemperatures();
  while (!water_temp_sensor.isConversionComplete());
  float temp = water_temp_sensor.getTempC();
  return temp;
}


/*
    Updates global latitude and longitude variables
*/
int getCoordinates() {
  unsigned long time_reference = millis();
  while (millis() - time_reference < 3000 ) {
    if (gps.read()) {
      currentLocation = gps.getGeolocation();
      latitude = currentLocation.latitude;
      longitude = currentLocation.longitude;
      return 1;
    }
  }
  return -1;
}

void imu_read() {
  /*
   *  Ta 4 målinger i sekundet. 
   *  Ta målinger i 5 minutter.
   *  -> 4*5*60 målinger * 4 bytes
   */
  if (imu.gyroAvailable()) {
    imu.readGyro();
  }
  if (imu.accelAvailable()) {
    imu.readAccel();
  }    
}

void waitRoutine() {
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

/*
   Network
*/

void reconnect() {

}
