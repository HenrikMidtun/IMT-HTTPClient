int BatteryVoltageRead, SolarVoltageRead;
float BatteryVoltage,SolarVoltage;
unsigned long measureTime=0;
unsigned long printTime=0;
float BattVol;
float SolVol;
int count=0;
void setup()
{
Serial.begin(115200);
Serial1.begin(115200);
while (!Serial1)
{
  ;
}

}

void loop()
{
 
if (Serial1.available()){
  Serial.write(Serial1.read());
}


if (Serial.available()){
  Serial1.write(Serial.read());
}
if (millis()-measureTime>1000){
  Serial.print(count);
BattVol +=analogRead(0) * (5.0 / 1023.0);
SolVol +=analogRead(1) * (5.0 / 1023.0);
measureTime=millis();
count++;
}

if (millis()-printTime>20000)
  {
  printTime=millis();
  Serial.print("Avg Battery voltage is: ");
  Serial.print(BattVol/count);
  Serial.print("   Avg Solar voltage is: ");
  Serial.println(SolVol/count);
  BattVol=0;
  SolVol=0;
  count=0;
  }

}
