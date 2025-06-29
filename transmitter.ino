#include <Password.h> //http://www.arduino.cc/playground/uploads/Code/Password.zip
#include <Keypad.h> //http://www.arduino.cc/playground/uploads/Code/Keypad.zip
#include <SPI.h>
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <Speck.h>
#include "SevSeg.h"

#define RFM95_CS 46
#define RFM95_RST 48
#define RFM95_INT 20

RH_RF95 rf95(RFM95_CS, RFM95_INT);
Speck myCipher;   // Instanciate a Speck block ciphering
RHEncryptedDriver myDriver(rf95, myCipher); // Instantiate the driver with those two

#define RF95_FREQ 915.0

unsigned char encryptkey[16] = {}; // The very secret key

Password entryKey = Password( "" );
Password chanelA = Password( "A" );
Password chanelB = Password( "B" );
Password chanelC = Password( "C" );
Password chanelD = Password( "D" );
Password chanelCA = Password( "CA" );
Password escWord = Password( "0" );


char messageA[] = "";
char messageB[] = "";
char messageC[] = "";
char messageD[] = "";
char messageCA[] = "";

uint8_t messageLen;

const byte ROWS = 5; //four rows
const byte COLS = 5; //three columns
char keys[ROWS][COLS] = {
    {'1','2','3','A','o'},
    {'4','5','6','B','o'},
    {'7','8','9','C','o'},
    {'*','0','#','D','o'},
    {'o','o','o','o','E'}
};

byte rowPins[ROWS] = {10, 12, 14, 16, A14}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {2, 4, 6, 8, A15}; //connect to the column pinouts of the kpd

const byte buzzerPin = 23;
const byte greenLed = 22;
int button;

int delayTime = 0;

byte state = 0;

String timeStringA;
String timeStringB;
String timeStringC;
String timeStringD;
String timeStringCA;

long startMillis = 0;
int displayChangeTime = 1000;
byte displayState = 0;
String currentChannel;

int timeA;
int timeB;
int timeC;
int timeD;
int timeCA;

SevSeg sevseg; //Instantiate a seven segment controller object
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  messageLen = strlen(messageA);
  byte numDigits = 4;
  byte digitPins[] = {A0, A6, A8, 33};
  byte segmentPins[] = {A2, A10, 37, 41, 43, A4, 35, 39};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE; // See README.md for options
  bool updateWithDelays = false; // Default. Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(90);
  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Feather LoRa RX Test!");
  digitalWrite(RFM95_RST, LOW); // manual reset
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
  if (!rf95.setFrequency(RF95_FREQ)) { // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ); // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  rf95.setTxPower(10, false); // The default transmitter power is 13dBm, using PA_BOOST. You can set transmitter powers from 5 to 23 dBm.
  myCipher.setKey(encryptkey, 16);// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  keypad.addEventListener(keypadEvent); // Add an event listener for this keypad
}

void loop() {
  // put your main code here, to run repeatedly:
  keypad.getKey();
  handleChanelTimeDisplay();
  sevseg.refreshDisplay(); // Must run repeatedly
  switch (state) {
    case 0:
      handleStartUp();
      break;
    case 1:
      handleLobby();
      break;
    case 2:
      delayTime = timeA;
      break;
    case 3:
      delayTime = timeB;
      break;
    case 4:
      delayTime = timeC;
      break;
    case 5:
      delayTime = timeD;
      break;
    case 6:
      delayTime = timeCA;
      break;
  }
}

void handleStartUp() {
    if (millis() - startMillis > displayChangeTime) {
      displayState = !displayState;
      startMillis = millis();
    }
    if (displayState) {
      sevseg.setChars("entr");
    }
    else {
      sevseg.setChars("pass");
    }
}

void handleLobby() {
    if (millis() - startMillis > displayChangeTime) {
      displayState = !displayState;
      startMillis = millis();
    }
    if (displayState) {
      sevseg.setChars("accs");
    }
    else {
      sevseg.setChars("grtd");
    }
}

void handleChanelTimeDisplay() {
  if (state > 1) {
    if (millis() - startMillis > displayChangeTime) {
      displayState++;
      startMillis = millis();
    }
    if (displayState == 1) {
      sevseg.setChars("chnl");
    }
    else if (displayState == 2) {
      sevseg.setChars(currentChannel.c_str());
    }
    else if (displayState == 3) {
      sevseg.setChars("ctdn");
    }
    else if (displayState == 4) {
      sevseg.setNumber(delayTime);
    }
    else if (displayState > 4) {
      displayState = 1;
    }
  }
}

void handleDetonations() {
    if (delayTime > 1) {
      unsigned long timer = millis();
      int deciSeconds = delayTime;
      while(1) {
        if (millis() - timer >= 1) {
          timer += 1;
          deciSeconds--; // 100 milliSeconds is equal to 1 deciSecond
          
          if (deciSeconds == 0) { // Reset to 0 after counting for 1000 seconds.
            deciSeconds=delayTime;
            digitalWrite(greenLed, HIGH);
            sevseg.setChars("fire");
            break;
          }
          sevseg.setNumber(deciSeconds);
        }

        sevseg.refreshDisplay(); // Must run repeatedly
      }
        }
    uint8_t data[messageLen+1] = {0};
    if (state == 2) {
      for(uint8_t i = 0; i<= messageLen; i++) data[i] = (uint8_t)messageA[i];
    }
    if (state == 3) {
      for(uint8_t i = 0; i<= messageLen; i++) data[i] = (uint8_t)messageB[i];
    }
    if (state == 4) {
      for(uint8_t i = 0; i<= messageLen; i++) data[i] = (uint8_t)messageC[i];
    }
    if (state == 5) {
      for(uint8_t i = 0; i<= messageLen; i++) data[i] = (uint8_t)messageD[i];
    }
    if (state == 6) {
      for(uint8_t i = 0; i<= messageLen; i++) data[i] = (uint8_t)messageCA[i];
    }
    myDriver.send(data, sizeof(data)); // Send out ID + Sensor data to LoRa gateway
    Serial.print("Sent: ");
    Serial.println((char *)&data);
    state = 1;
    displayState = 0;
    digitalWrite(greenLed, LOW);
}

void keypadEvent(KeypadEvent eKey) {
  switch (keypad.getState()){
    case PRESSED:
      digitalWrite(buzzerPin, HIGH);
      delay(50);
      digitalWrite(buzzerPin, LOW);
      Serial.print("Pressed: ");
      Serial.println(eKey);

	switch (eKey) {
	  case '*':
      if ((state == 2) && (timeStringA != "0") && timeStringA.length()){
        timeA = timeStringA.toInt();
      }
      else if ((state == 3) && (timeStringB != "0") && timeStringB.length()) {
        timeB = timeStringB.toInt();
      }
      else if ((state == 4) && (timeStringC != "0") && timeStringC.length()) {
        timeC = timeStringC.toInt();
      }
      else if ((state == 5) && (timeStringD != "0") && timeStringD.length()) {
        timeD = timeStringD.toInt();
      }
      else if ((state == 6) && (timeStringCA != "0") && timeStringCA.length()) {
        timeCA = timeStringCA.toInt();
      }
      timeStringA = timeStringB = timeStringC = timeStringD = timeStringCA = "";
      checkPasswords();
      entryKey.reset(); chanelA.reset(); chanelB.reset(); chanelC.reset(); chanelD.reset(); chanelCA.reset(); escWord.reset();
      break;
	  case '#':
      entryKey.reset(); chanelA.reset(); chanelB.reset(); chanelC.reset(); chanelD.reset(); chanelCA.reset(); escWord.reset();
      break;
    case 'E':
      if (state > 1) {
        handleDetonations();
      }
      break;
	  default:
      entryKey.append(eKey); chanelA.append(eKey); chanelB.append(eKey); chanelC.append(eKey); chanelD.append(eKey); chanelCA.append(eKey); escWord.append(eKey);
      if ((state == 2) && isDigit(eKey) && (timeStringA.length() != 0 || eKey != '0')) {
        timeStringA += eKey;
      }
      else if ((state == 3) && isDigit(eKey) && (timeStringB.length() != 0 || eKey != '0')) {
        timeStringB += eKey;
      }
      else if ((state == 4) && isDigit(eKey) && (timeStringC.length() != 0 || eKey != '0')) {
        timeStringC += eKey;
      }
      else if ((state == 5) && isDigit(eKey) && (timeStringD.length() != 0 || eKey != '0')) {
        timeStringD += eKey;
      }
      else if ((state == 6) && isDigit(eKey) && (timeStringCA.length() != 0 || eKey != '0')) {
        timeStringCA += eKey;
      }
    }
  }
}

void checkPasswords() {
  if (entryKey.evaluate() || escWord.evaluate()) {
    state = 1;
    displayState = 0;
  }
  else if (chanelA.evaluate() && state) {
    state = 2;
    displayState = 0;
    currentChannel = "A";
  }
  else if (chanelB.evaluate() && state) {
    state = 3;
    displayState = 0;
    currentChannel = "B";
  }
  else if (chanelC.evaluate() && state) {
    state = 4;
    displayState = 0;
    currentChannel = "C";
  }
  else if (chanelD.evaluate() && state) {
    state = 5;
    displayState = 0;
    currentChannel = "d";
  }
  else if (chanelCA.evaluate() && state) {
    state = 6;
    displayState = 0;
    currentChannel = "CA";
  }
  else {
    Serial.println("Wrong");
    for (int i = 0; i < 3; i++) {
      digitalWrite(greenLed, HIGH);
      delay(100);
      digitalWrite(greenLed, LOW);
      delay(100);
    }
  }
}