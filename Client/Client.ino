#include "ntnu.h"
#include "SparkFunBME280.h"
#include <OneWire.h>
#include <DS18B20.h>

/*
  Før dere begynner, gå inn i ntnu.h filen og endre til ønsket navn på gruppen deres.
  Vårt forslag er å endre dette til deres unike IMEI som printes ut ved starten av programmet.
  Variabelen som endres er *MQTT_CLIENT_NAME*.
*/

#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 waterTempSensor(&oneWire);
BME280 airSensor;

float water_temp;
float air_temp;
float air_humidity;
float air_pressure;

void setup()
{  
  IMT_SETUP();

  if (waterTempSensor.begin() == false){
    Serial.println("WARNING: Failed to communicate with DS18B20");
  }
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
    IMT_READ();

    /*
      Start på egen kode
    */
    updateAirSensor();
    DATA["air_temp"] = air_temp;
    DATA["air_humidity"] = air_humidity;
    DATA["air_pressure"] = air_pressure;

    updateWaterTemp();
    DATA["water_temp"] = water_temp;
    /*
      Slutt egen kode
    */

    STORE_DATA(i);
    if(i == NUM_READINGS-1){
      IMT_SEND();
    }
    t1 = millis();
    uint32_t duration = t1-t0;
    delay(interval-duration);
  }
  DEEP_SLEEP(60*60); //Uncomment når dere ikke lenger trenger å følge med i Serial Monitor.
  //delay(60000*60);
}

void updateAirSensor(){
  air_temp = airSensor.readTempC();
  air_humidity = airSensor.readFloatHumidity();
  air_pressure = airSensor.readFloatPressure();
}

void updateWaterTemp() {
  waterTempSensor.requestTemperatures();
  float w_temp = 0;
  uint32_t timeout = millis();
  while (!waterTempSensor.isConversionComplete())
  {
    if (millis() - timeout >= 800) // check for timeout, 800ms
    {
      water_temp = -99; //Greide ikke å ta en måling, setter til en vilkårlig urealistisk verdi.
    }
  }
  water_temp = waterTempSensor.getTempC();
}

