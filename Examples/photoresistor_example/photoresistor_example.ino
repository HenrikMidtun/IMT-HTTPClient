
void setup() {
  Serial.begin(9600);
}

void loop() {
  int value = analogRead(A8);
  Serial.println(value);
  delay(250);
}
