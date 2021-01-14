#include <TinyGPS++.h>


#include <SoftwareSerial.h>

// Choose two Arduino pins to use for software serial
int RXPin = 8;
int TXPin = 3;
int gsmRXPin = 4;
int gsmTXPin = 5;

int send_Location = 6;
int no_Button = 7;
int activate_Pi_interrupt  = 9;
int send_Message_Pi = 10;
int dont_Message = 11;

int GPSBaud = 9600;
String Coordinates = "";
//volatile String locations = Coordinates;

// Create a TinyGPS++ object
TinyGPSPlus GPS;

// Create a software serial port called "gpsSerial"
SoftwareSerial gpsSerial(RXPin, TXPin);
SoftwareSerial gsmSerial(gsmRXPin, gsmTXPin); //SIM800L Tx & Rx is connected to Arduino #3 & #2

void setup()
{
  // Start the Arduino hardware serial port at 9600 baud
  Serial.begin(9600);
  pinMode(send_Location, INPUT_PULLUP);
  pinMode(no_Button, INPUT_PULLUP);

  pinMode(activate_Pi_interrupt, OUTPUT);
  pinMode(dont_Message, OUTPUT);
  pinMode(send_Message_Pi, OUTPUT);

  analogWrite(activate_Pi_interrupt, 0);
  analogWrite(send_Message_Pi, 0);
  analogWrite(dont_Message, 0);

  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);
}

void loop() {
  while (gpsSerial.available() > 0)
    if (GPS.encode(gpsSerial.read())) {
      displayInfo();
      if (!digitalRead(send_Location)) {
        delay(500);
        Serial.println("Do you really want to send your location?");
        analogWrite(activate_Pi_interrupt, 160);     
        while (digitalRead(send_Location) && digitalRead(no_Button)) {
          delayMicroseconds(50);
        }
        if (!digitalRead(no_Button)) {
          Serial.println("no");
          analogWrite(dont_Message, 160);
          return;
        }
        else {
          Serial.println("Sending message");
          analogWrite(send_Message_Pi, 160);
          delay(2000);
          gpsSerial.end();
          send_Message();
          goto restart;
        }
      }
      restart:
      Coordinates = "";
      analogWrite(activate_Pi_interrupt, 0);
      analogWrite(dont_Message, 0);
      analogWrite(send_Message_Pi, 0);

    }
  if (millis() > 5000 && GPS.charsProcessed() < 10)  {
    Serial.println("No GPS detected");
    //while (true);
  }
}

void displayInfo() {
  if (GPS.location.isValid())  {
    Coordinates += "latitude: ";
    Coordinates += String(GPS.location.lat(), 6);
    Coordinates += " longitude: ";
    Coordinates += String(GPS.location.lng(), 6);
    Serial.println(Coordinates);
    //delay(1000);
  }
  else  {
    Serial.println("Location: Not Available");
  }
}

void send_Message() {
  //Begin serial communication with Arduino and SIM800L
  gsmSerial.begin(9600);
  Serial.println("Initializing...");
  delay(1000);

  gsmSerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();

  gsmSerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  gsmSerial.println("AT+CMGS=\"+639617521939\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  gsmSerial.print("An emergency situation occured I am at " + Coordinates + " please plot this at google maps and find me"); //text content
  updateSerial();
  gsmSerial.write(26);
  delay(2000);
  gsmSerial.end();
  Serial.println("message Sent");
  gpsSerial.begin(GPSBaud);
}

void updateSerial() {
  delay(500);
  while (Serial.available())  {
    gsmSerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (gsmSerial.available())  {
    Serial.write(gsmSerial.read());//Forward what Software Serial received to Serial Port
  }
}
