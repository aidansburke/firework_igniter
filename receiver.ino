// Feather9x_RX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_TX

#include <SPI.h>
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <Speck.h>

#define RFM95_CS    8
#define RFM95_INT   3
#define RFM95_RST   4

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Speck myCipher;   // Instanciate a Speck block ciphering
RHEncryptedDriver myDriver(rf95, myCipher); // Instantiate the driver with those two

float frequency = 915.0; // Change the frequency here. 
unsigned char encryptkey[16] = {}; // The very secret key

const byte first = 6;
const byte second = 11;
const byte third = 12;
const byte fourth = 10;

unsigned long firstMillis = 0;
unsigned long secondMillis = 0;
unsigned long thirdMillis = 0;
unsigned long fourthMillis = 0;

const int ignitionTime = 10000;

void setup() {
  pinMode(first, OUTPUT);
  pinMode(second, OUTPUT);
  pinMode(third, OUTPUT);
  pinMode(fourth, OUTPUT);
  
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  Serial.println("Feather LoRa RX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(frequency)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(frequency);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  myCipher.setKey(encryptkey, sizeof(encryptkey));
}

void loop() {
  if (myDriver.available()) {
    // Should be a message for us now
    uint8_t buf[myDriver.maxMessageLength()];
    uint8_t len = sizeof(buf);
    if (myDriver.recv(buf, &len)) {
      Serial.print("Received: ");
      Serial.println((char *)&buf);
      if (strcmp((char *)&buf, "") == 0) {
        digitalWrite(first, HIGH);
        firstMillis = millis();
      }
      else if (strcmp((char *)&buf, "") == 0) {
        digitalWrite(second, HIGH);
        secondMillis = millis();
      }
      else if (strcmp((char *)&buf, "") == 0) {
        digitalWrite(third, HIGH);
        thirdMillis = millis();
      }
      else if (strcmp((char *)&buf, "") == 0) {
        digitalWrite(fourth, HIGH);
        fourthMillis = millis();
      }
                  // Serial.print("RSSI: ");
                  // Serial.println(rf95.lastRssi(), DEC);

                  // //Send a reply
                  // uint8_t data[] = "And hello back to you";
                  // rf95.send(data, sizeof(data));
                  // rf95.waitPacketSent();
                  // Serial.println("Sent a reply");
                  // digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("Receive failed");
    }
  }
  if (millis() - firstMillis  > ignitionTime) {
    digitalWrite(first, LOW);
  }
  if (millis() - secondMillis > ignitionTime) {
    digitalWrite(second, LOW);
  }
  if (millis() - thirdMillis > ignitionTime) {
    digitalWrite(third, LOW);
  }
  if (millis() - fourthMillis > ignitionTime) {
    digitalWrite(fourth, LOW);
  }
}
