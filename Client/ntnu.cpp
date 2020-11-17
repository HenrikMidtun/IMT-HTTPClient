#include "ntnu.h"
#include "Arduino.h"
#include <MKRNB.h>
#include <gp20u7.h>
#include "ArduinoLowPower.h"

#define NETWORK_TIMEOUT 240 //seconds

/*
   Sensor declarations and PINS
*/
GP20U7 gps = GP20U7(Serial1); //RX pin for Serial1 (PIN 13, MKR 1500)

/*
 * JSON and global data fields
 */

float GPS_DATA[20][2];
long TIME_DATA[20][1];
int NUM_READINGS;
int NUM_FIELDS;
Geolocation currentLocation; //GPS struct

String IMEI = "";
const char PIN_CODE[] = "";
char httpServer[] = "sensor.marin.ntnu.no";
int port = 80;

NBClient nbClient(false);
NBScanner scannerNetworks;
GPRS gprs;
NB nbAccess;
NBModem modem;

/*
 * GPS
 */

void gpsBegin()
{
  gps.begin();
}

int getCoordinates()
{
  unsigned long time_reference = millis();
  while (millis() - time_reference < 2000)
  {
    if (gps.read())
    {
      currentLocation = gps.getGeolocation();
      return 1;
    }
  }
  return 0;
}

/*
 * Network
 */

boolean lteConnect()
{
  /*
 * Tries to make an LTE connection to the provider
 */
  return (nbAccess.begin(PIN_CODE) == NB_READY) && (gprs.attachGPRS() == GPRS_READY);
}

boolean lteReconnect()
{
  /*
 * Tries to reconnect the LTE connection with the provider.
 * Tries to reconnect for at least the duration of NETWORK_TIMEOUT
 */
  unsigned long time_reference = millis();
  while (millis() - time_reference < NETWORK_TIMEOUT * 1000)
  {
    if (lteConnect())
    {
      return true;
    }
  }
  Serial.println("-> LTE connection timed out");
  return false;
}

boolean clientConnect()
{
  /*
 * Creates a connection from the HTTP client to the server.
 * The client will try to reconnect at least until NETWORK_TIMEOUT has been reached
 */
  if (!nbClient.connected())
  {
    unsigned long time_reference = millis();
    while (millis() - time_reference < NETWORK_TIMEOUT * 1000)
    {
      if (nbClient.connect(httpServer, port))
      {
        return true;
      }
    }
    Serial.println("-> Client connection timed out");
    return false;
  }
  return true;
}

boolean makeConnections()
{
  /*
 *  Used for the initial connection.
 *  Returns true if succesful, false if not 
 */
  if (lteReconnect())
  {
    scannerNetworks.begin();
    Serial.print("Network provider: ");
    Serial.println(scannerNetworks.getCurrentCarrier());
    Serial.print("Signal strength [0-31, 99='not detectable']: ");
    Serial.println(scannerNetworks.getSignalStrength());
    Serial.println("Connecting to server...");
    return clientConnect();
  }
  Serial.println("WARNING: Could not make connections.");
  return false;
}

boolean checkConnection()
{
  /*
 * true if connection is OK or reconnect is OK, 
 * false if no connection available after trying to reconnect
 */
  if (nbClient.connected())
  {
    return true;
  }
  return lteReconnect() && clientConnect();
}

void networkSetup()
{
  Serial.println("Network setup");
  Serial.println("This may take a while...");
  nbAccess.setTimeout(NETWORK_TIMEOUT * 1000);
  makeConnections();
}

void HTTP_POST(char *path)
{
  // Make a HTTP request
  if (clientConnect())
  { //Ved mindre serveren har mulighet til å holde en 'persistent connection', så må klienten koble til igjen
    nbClient.print("GET ");
    nbClient.print(path);
    nbClient.println(" HTTP/1.1");
    nbClient.print("Host: ");
    nbClient.println(httpServer);
    nbClient.println("Connection: close");
    nbClient.println();
    nbClient.stop();
  }
}

void createMessageAndSend(char **fields, float *readings)
{
  /*
    Create URLs with headers and data. Send HTTP request.
  */
  /*
    Første melding, inneholder feltene som skal lagres
  */
  char first_path[300] = "/cgi-bin/update.cgi?";
  strcat(first_path, IMEI.c_str());
  strcat(first_path, ",time,la,lo");
  for (int i = 0; i < NUM_FIELDS; i++)
  {
    strcat(first_path, ",");
    strcat(first_path, fields[i]);
  }
  HTTP_POST(first_path);
  /*
    Resterende meldinger
  */
  for (int i = 0; i < NUM_READINGS; i++)
  {
    /*
      IMT felter
    */
    char data_path[300] = "/cgi-bin/update.cgi?";
    String buff;
    buff.concat(IMEI);
    buff.concat(",");
    buff.concat(TIME_DATA[i][0]);
    buff.concat(",");
    buff.concat(String(GPS_DATA[i][0], 2));
    buff.concat(",");
    buff.concat(String(GPS_DATA[i][1], 2));

    /*
      Bruker felter
    */
    for (int j = 0; j < NUM_FIELDS; j++)
    {
      float val = *(readings + i * NUM_FIELDS + j);
      buff += ",";
      buff += String(val, 2);
    }
    strcat(data_path, buff.c_str());
    buff = "";
    HTTP_POST(data_path);
  }
}

/*
 * DATA
 */

void initData(int n, int m)
{
  /*
      Allocates memory for GPS_DATA and TIME_DATA
      Sets value of NUM_READINGS and NUM_FIELDS
  */
  if (n > 0)
  {
    NUM_READINGS = n;
  }
  else
  {
    NUM_READINGS = 1;
  }
  if (m >= 0)
  {
    NUM_FIELDS = m;
  }
  else
  {
    NUM_FIELDS = 0;
  }

  for (int i = 0; i < NUM_READINGS; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      GPS_DATA[i][j] = 0.;
    }
  }

  for (int i = 0; i < NUM_READINGS; i++)
  {
    TIME_DATA[i][0] = 0;
  }
}

void updateReadings(int i)
{
  /*
    Updates IMT readings
  */
  TIME_DATA[i][0] = nbAccess.getLocalTime();

  if (getCoordinates())
  {
    GPS_DATA[i][0] = currentLocation.latitude;
    GPS_DATA[i][1] = currentLocation.longitude;
  }
  else
  {
    GPS_DATA[i][0] = 0;
    GPS_DATA[i][1] = 0;
  }
}

void setIMEI()
{
  modem.begin();
  IMEI = modem.getIMEI();
  while (IMEI == NULL || IMEI == "")
  {
    IMEI = modem.getIMEI();
    while (!modem.begin())
    {
      ;
    }
  }
}

/*
 *  Helpers
 */

void printIMEI()
{
  Serial.print("IMEI: ");
  Serial.println(IMEI);
}

void printDATA()
{
  for (int i = 0; i < NUM_READINGS; i++)
  {
    Serial.print("[");
    for (int j = 0; j < 3; j++)
    {
      if (j > 0)
      {
        Serial.print(GPS_DATA[i][j]);
      }
      else
      {
        Serial.print(TIME_DATA[i][0]);
      }
      if (j != 2)
      {
        Serial.print(", ");
      }
    }
    Serial.print("]");
    Serial.println();
  }
}

void beginSerial()
{
  /*
    Try to connect to Serial for a maximum of 5 seconds.
    Timeout is here in case of Arduino being run on battery power.
    Then Serial connection can not be established.
  */
  Serial.begin(9600);

  unsigned long time_reference = millis();
  while (!Serial && millis() - time_reference < 5000)
  {
    ;
  }
}

void SLEEP(int milliseconds, bool development)
{
  if (milliseconds > 0)
  {
    if (development)
    {
      delay(milliseconds);
    }
    else
    {
      LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, beginSerial, CHANGE); //Forsøk på å få serial output
      LowPower.sleep(milliseconds);                                          //Kobler ut Serial
    }
  }
}

/*
 *  IMT
 */

void IMT_SETUP(int num_reads, int num_fields)
{
  beginSerial();
  setIMEI();
  printIMEI();
  gpsBegin();
  initData(num_reads, num_fields);
  networkSetup();
}

void IMT_READ(int i)
{
  updateReadings(i);
}

void IMT_SEND(char **fields, float *readings)
{
  if (checkConnection())
  {
    createMessageAndSend(fields, readings);
  }
}
