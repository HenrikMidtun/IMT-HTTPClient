#ifndef NTNU_h
#define NTNU_h
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0

void IMT_SETUP(int n, int m);
void IMT_READ(int i);
void IMT_SEND(char** fields, float* readings);
void DEEP_SLEEP(int s); //Kobler ut Serial port, så da får man ikke output fra Serial Monitor etter å ha kalt DEEP_SLEEP

void printIMEI();
void printData();
#endif
