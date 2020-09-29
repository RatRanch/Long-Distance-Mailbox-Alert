// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <RH_RF95.h>

// #define DEBUG  // uncomment to enable serial messages

// for feather32u4
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
//

#if defined(ESP8266)
/* for ESP w/featherwing */
#define RFM95_CS  2    // "E"
#define RFM95_RST 16   // "D"
#define RFM95_INT 15   // "B"

#elif defined(ESP32)
/* ESP32 feather w/wing */
#define RFM95_RST     27   // "A"
#define RFM95_CS      33   // "B"
#define RFM95_INT     12   //  next to A

#elif defined(NRF52)
/* nRF52832 feather w/wing */
#define RFM95_RST     7   // "A"
#define RFM95_CS      11   // "B"
#define RFM95_INT     31   // "C"

#elif defined(TEENSYDUINO)
/* Teensy 3.x w/wing */
#define RFM95_RST     9   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     4    // "C"
#endif

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);


void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  pinMode(13, OUTPUT); // LED pin
  pinMode(5, INPUT);  // Switch - NO

  digitalWrite(RFM95_RST, HIGH);

#if defined(DEBUG)
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
#endif

  delay(100);
#if defined(DEBUG)
  Serial.println("Feather LoRa TX Test!");
#endif
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    //Serial.println("LoRa radio init failed");
    //Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
#if defined(DEBUG)
  Serial.println("LoRa radio init OK!");
#endif
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
#if defined(DEBUG)
    //Serial.println("setFrequency failed");
#endif
    while (1);
  }
#if defined(DEBUG)
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
#endif
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  rf95.sleep();  // Turn off radio while we're not using it to save power

}

int16_t packetnum = 0;  // packet counter, we increment per xmission

int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin

// the follow variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
//long time = 0;         // the last time the output pin was toggled
//long debounce = 200;   // the debounce time, increase if the output flickers

void loop()
{

  reading = digitalRead(5); // Normally open dry contact.  1 = closed.
  if (previous != reading){
    digitalWrite(13, reading);  // LED
    transmitStatus(reading, checkBattery());
  }
  previous = reading;

}

/*
   This routine checks the voltage on the battery (if it exists) and returns that value.  Should be in the 3.2-4.2 range depending upon the battery used
*/
float checkBattery() {
  //This returns the current voltage of the battery on a Feather 32u4.
  float measuredvbat = analogRead(A9);

  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  measuredvbat *= 1.274;// doc says we there is a 2:1 voltage divider between BAT and A9 but I consistently get 78.5%

  return measuredvbat;
}

void transmitStatus(int reading, float batVoltage){
  // Serial.println("Transmitting..."); // Send a message to rf95_server
  
  // The status packet will have the contact state (0 or 1), followed by a space, then voltage
  // Example:  "0 4.20"
  char radiopacket[8];
  dtostrf(batVoltage, 6, 2, radiopacket);
  radiopacket[0] = (reading + 48); // ascii 48 is zero, 49 is one
#if defined(DEBUG)
  Serial.print("Sending ["); Serial.print(radiopacket); Serial.println("]");
#endif
  //radiopacket[19] = 0;

  //Serial.println("Sending...");

  delay(10);
  //rf95.send((uint8_t *)radiopacket, 20);
  rf95.send((uint8_t *)radiopacket, 8);

  //Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  rf95.sleep();  
}  
