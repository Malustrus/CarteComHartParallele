/* #include <Arduino.h>
byte message[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x80, 0x00, 0x00, 0x82, 0xFF, 0xFF};

void setup() {
  Serial.begin(9600);
  Serial1.begin(1200, SERIAL_8O2);
  Serial2.begin(1200, SERIAL_8O2);
  Serial3.begin(1200, SERIAL_8O2);
  Serial.print("setup()");
}

void loop() {
  
  
  //Envoi du message
  for (size_t i = 0; i < sizeof(message); i++)
  {
    Serial1.write((byte)message[i]);
    Serial2.write((byte)message[i]);
    Serial3.write((byte)message[i]);
  }

  //Attente que les buffers (64 octets) soient vidés
  Serial1.flush();
  Serial2.flush();
  Serial3.flush();
  
  if(Serial1.available()) Serial.print("1:");
  while (Serial1.available())
  {
      char c = Serial1.read();
      Serial.print(c, HEX);
  }
  Serial.println();
  if(Serial2.available()) Serial.print("2:");
  while (Serial2.available())
  {
      char c = Serial2.read();
      Serial.print(c, HEX);
  }
  Serial.println();
  if(Serial2.available()) Serial.print("3:");
  while (Serial3.available())
  {
      char c = Serial3.read();
      Serial.print(c, HEX);
  }
  Serial.println();
  delay(1000);
}
 */