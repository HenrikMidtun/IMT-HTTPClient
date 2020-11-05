#ifndef NTNU_h
#define NTNU_h
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#include <ArduinoJson.h>

extern StaticJsonDocument<200> DATA;

void IMT_SETUP(int n);
void IMT_READ();
void IMT_SEND();
void STORE_DATA(int index);
void DEEP_SLEEP(int seconds); //Kobler ut Serial port, så da får man ikke output fra Serial Monitor etter å ha kalt DEEP_SLEEP

void printData();
void printIMEI();
void printTopic();
void printStorage();
#endif
