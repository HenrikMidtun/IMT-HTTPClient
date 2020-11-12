#include "ntnu.h"
#include "SparkFunBME280.h"
#include <Wire.h>
#include <OneWire.h>
#include <DS18B20.h>

#define NUM_READINGS 3  //Hvor mange målinger i løpet av en periode
#define NUM_FIELDS 4    //Hvor mange felter dere ønsker å sende
#define PERIOD 1        //Perioden i minutter

#define ONE_WIRE_BUS 5 //Digital pin 5
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 waterTempSensor(&oneWire);
BME280 airSensor;

char* fields[NUM_FIELDS] = {"water_temp","air_temp","air_humidity","air_pressure"};
float readings[NUM_READINGS][NUM_FIELDS];

void setup()
{ 
  IMT_SETUP(NUM_READINGS, NUM_FIELDS);
  if (waterTempSensor.begin() == false){
    Serial.println("WARNING: Failed to communicate with DS18B20");
  }
  Wire.begin();
  if (airSensor.beginI2C() == false)
  {
    Serial.println("WARNING: Failed to communicate with BME280.");
  }
}

void loop()
{
  int interval = (PERIOD*60*1000)/NUM_READINGS;
  uint32_t t0,t1;
  for(int i=0; i<NUM_READINGS; i++){
    t0 = millis();
    IMT_READ(i);

    /*
      Start på egen kode
    */
    readings[i][0] = getWaterTemp();
    readings[i][1] = airSensor.readTempC();
    readings[i][2] = airSensor.readFloatHumidity();
    readings[i][3] = airSensor.readFloatPressure();
    /*
      Slutt egen kode
    */
    
    if(i == NUM_READINGS-1){
      IMT_SEND(fields, readings[0]);
    }
    t1 = millis();
    uint32_t duration = t1-t0;
    SLEEP(interval-duration, true); //endre til false når dere ikke lengre trenger å følge med i Serial Monitor, altså ved sjøsetting.
  }
  printReadings();
}

void printReadings(){
  for(int i=0; i<NUM_READINGS; i++){
    Serial.print("[");
    for(int j=0; j<NUM_FIELDS; j++){
      Serial.print(readings[i][j]);
      if(j != NUM_FIELDS-1){
        Serial.print(", ");
      }
    }
    Serial.print("]");
    Serial.println();
  }
}

float getWaterTemp() {
  waterTempSensor.requestTemperatures();
  float w_temp = 0;
  uint32_t timeout = millis();
  while (!waterTempSensor.isConversionComplete())
  {
    if (millis() - timeout >= 800) // check for timeout, 800ms
    {
      w_temp = -99; //Greide ikke å ta en måling, setter til en vilkårlig urealistisk verdi. ;)
      return w_temp;
    }
  }
  w_temp = waterTempSensor.getTempC();
  return w_temp;
}

