#ifndef NTNU_h
#define NTNU_h
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#include <ArduinoJson.h>

/*
    Felter som kan endres på
*/
#define MQTT_CLIENT_NAME "352753096010838" //klientens navn, dere må endre dette til deres IMEI
#define MSG_INTERVAL 180 //Antall minutter(?) mellom hver melding IKKE I BRUK
#define NETWORK_TIMEOUT 20 //Antall minutter før man prøver å få dekning, 0 for evig forsøk IKKE I BRUK
#define STANDARD_SENSORS 1 //1 hvis man bruker standard sensorer, 0 hvis ikke IKKE I BRUK

extern StaticJsonDocument<200> DATA;

void IMT_SETUP();
void IMT_READ();
void IMT_SEND();
void DEEP_SLEEP(int seconds); //Kobler ut Serial port, så da får man ikke output fra Serial Monitor etter å ha kalt DEEP_SLEEP

void printData();
void printIMEI();
void printTopic();
#endif
