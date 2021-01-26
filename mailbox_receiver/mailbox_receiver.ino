// mailbox_receiver
// -*- mode: C++ -*-
// This is part of the Long Distance Mailbox Alert project:  http://jimandnoreen.com
// It is designed to work with mailbox_transmitter, and optionally, LoRa_to_Indigo_Gateway
//
// Based on an example for the LoRa Feather board by Adafruit.com
//
// You can attach a switch to pins 5 and 3V3
// Each press of the switch should toggle the flag between up and down positions
// Resetting the board will always move the flag to down position
//
// You can also attach a servo to pin 11 to move a flag (or something else)
//
// Received packets are forwarded to the hardware serial UART on pin 1 (TX).  You can attach
// the LoRa_to_Indigo_Gateway from this project or any other serial device

#include <SPI.h>
#include <RH_RF95.h>


#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7


/* for feather m0 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
*/

/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/

/* Feather 32u4 w/wing
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

//#if defined(ESP8266)
  /* for ESP w/featherwing */ 
//  #define RFM95_CS  2    // "E"
//  #define RFM95_RST 16   // "D"
//  #define RFM95_INT 15   // "B"

//#elif defined(ESP32)  
  /* ESP32 feather w/wing */
//  #define RFM95_RST     27   // "A"
//  #define RFM95_CS      33   // "B"
//  #define RFM95_INT     12   //  next to A

//#elif defined(NRF52)  
  /* nRF52832 feather w/wing */
//  #define RFM95_RST     7   // "A"
//  #define RFM95_CS      11   // "B"
//  #define RFM95_INT     31   // "C"
  
//#elif defined(TEENSYDUINO)
  /* Teensy 3.x w/wing */
//  #define RFM95_RST     9   // "A"
//  #define RFM95_CS      10   // "B"
//  #define RFM95_INT     4    // "C"
//#endif


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// pin used to control the servo
#define SERVO_PIN 11

// Flag's up position, in pulse duration
// You may need to tweak this
#define FLAG_UP 1500

// Flag's down position, in pulse duration
// You may need to tweak this
#define FLAG_DOWN 500

#define LED 13 // Attach external LED to pin 13 if desired

const int buttonPin = 5;    // Flag test button
int ledState = LOW;         // the current state of the output pin
int buttonState = LOW;  // variable for reading the pushbutton status
int lastButtonState = LOW;   // the previous reading from the input pin
int flagPosition = 0;

void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(SERVO_PIN, OUTPUT);
  
  // write flag to down position
  moveServo(FLAG_DOWN);
  flagPosition = 0; //Down

  pinMode(LED, OUTPUT);
  pinMode(buttonPin, INPUT);

  lastButtonState = digitalRead(buttonPin);
  // turn LED off:
  digitalWrite(LED, LOW); 

  Serial.begin(115200);
  //while (!Serial) {
  //  delay(1);
  //}
  delay(100);

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
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  Serial1.begin(115200); // Pins 0 and 1, not USB
}

void loop()
{
  // read the state of the test pushbutton value:
  buttonState = digitalRead(buttonPin);

  // If the switch changed:
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH){
      toggleFlag();
    }
  }
  lastButtonState = buttonState;  
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {

      //digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      
      Serial1.println((char*)buf); // Forward the packet to the WiFi bridge
      /*
      // Send a reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      */

      // The status packet will have the contact state (0 or 1), followed by a space, then voltage
      // Example:  "0 4.20"
      // So some rudimentary security to reject rogue packets
      
      if (buf[0] == 49 && buf[1] == 32 && buf[3] == 46 && buf[6] == 0) {  // ASCII Dec 49 = "1"; means mailbox just opened
        // Move the flag to the other position
        Serial.print("Toggling flag position");
        toggleFlag();
      }
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}
void toggleFlag() {
  if (flagPosition == 0) {
      Serial.println("Flag up!");
      // move flag up
      moveServo(FLAG_UP);
      flagPosition = 1;
      // turn LED on:
      digitalWrite(LED, HIGH);
  }
  else {
      Serial.println("Flag Down");
      // turn LED off:
      digitalWrite(LED, LOW); 
      moveServo(FLAG_DOWN);
      flagPosition = 0;         
  }
}

// The reason we're not just using Servo.h is that the Radiohead library
// and servo library both use timer 1.  We could modify one of these libraries
// to use timer 3 instead but for this simple up/down application it's less
// fuss to just do it in software.

void moveServo(int lenMicroSecondsOfPulse) {
  int lenMicroSecondsOfPeriod   = 20 * 1000; // 20 milliseconds (ms)
  // Servos work by sending a 20 ms pulse.  
  // 1.0 ms at the start of the pulse will turn the servo to the 0 degree position
  // 1.5 ms at the start of the pulse will turn the servo to the 90 degree position 
  // 2.0 ms at the start of the pulse will turn the servo to the 180 degree position 
  // Turn pin high to start the period and pulse

  // The servo up and down positions tend to drift if we dont send each positioning pulse multiple times
  // You may need to increase or decrease this value
  int numPulses = 10; // Number of positioning pulses to send
  int pulseCtr = 0;

  while (pulseCtr < numPulses) {
    digitalWrite(SERVO_PIN, HIGH);
    
    // Delay for the length of the pulse
    delayMicroseconds(lenMicroSecondsOfPulse);
    
    // Turn the voltage low for the remainder of the pulse
    digitalWrite(SERVO_PIN, LOW);
    
    // Delay this loop for the remainder of the period so we don't
    // send the next signal too soon or too late
    delayMicroseconds(lenMicroSecondsOfPeriod - lenMicroSecondsOfPulse);
    pulseCtr++;
  }
}
