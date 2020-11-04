#include "ntnu.h"

/*
  Før dere begynner, gå inn i ntnu.h filen og endre til ønsket navn på gruppen deres.
  Vårt forslag er å endre dette til deres unike IMEI som printes ut ved starten av programmet.
  Variabelen som endres er *MQTT_CLIENT_NAME*.
*/

void setup()
{  
  IMT_SETUP();
}

void loop()
{

  float windspeed = getWindspeed();
  DATA["windspeed"] = windspeed;

  

  int n = 3;
  int period = 1; //minutes
  int interval = (period*60*1000)/n;
  uint32_t t0,t1;
  for(int i=0; i<n; i++){
    t0 = millis();
    IMT_READ(); //Leser standard sensorer
    /*
      Start på egen kode
    */

    Serial.println("DoReading()");

    /*
      Slutt egen kode
    */
    if(i == n-1){
      Serial.println("sendData()");
      //IMT_SEND();
    }
    t1 = millis();
    uint32_t duration = t1-t0;
    delay(interval-duration);
  }

  //DEEP_SLEEP(60*60); Uncomment når dere ikke lenger trenger å følge med i Serial Monitor.
  //delay(60000*60);
}

float getWindspeed(){
  return 3.14;
}

