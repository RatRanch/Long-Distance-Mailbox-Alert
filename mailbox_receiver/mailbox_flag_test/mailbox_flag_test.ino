/*
 * This sketch can be used to test the mailbox hardware
 * Attach a switch to pins 5 and 3V3
 * Each press of the switch should toggle the flag between up and down positions
 * 
 * Complete project details at http://jimandnoreen.com
 */

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

void setup() {

  // start the serial connection
  Serial.begin(115200);

  pinMode(SERVO_PIN, OUTPUT);
  
  // write flag to down position
  moveServo(FLAG_DOWN);
  flagPosition = 0; //Down

  pinMode(LED, OUTPUT);
  pinMode(buttonPin, INPUT);

  lastButtonState = digitalRead(buttonPin);
  // turn LED off:
  digitalWrite(LED, LOW);  
}

void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // If the switch changed, due to noise or pressing:
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH){
      toggleFlag();
    }
  }
  lastButtonState = buttonState;
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
