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
  IMT_READ(); //Leser standard sensorer
  /*
    Start på egen kode
  */
  float windspeed = getWindspeed();
  DATA["windspeed"] = windspeed;
  /*
    Slutt egen kode
  */
  IMT_SEND();
  delay(60000*60);
  //DEEP_SLEEP(60*60); Uncomment når dere ikke lenger trenger å følge med i Serial Monitor.
}

float getWindspeed(){
  return 3.14;
}

