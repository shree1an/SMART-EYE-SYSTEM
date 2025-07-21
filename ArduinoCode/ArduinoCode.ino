#include <Servo.h> // Include the Servo library

const int bufferSize = 100; // Define the maximum size of the buffer
char incomingString[bufferSize]; // Character array to store the incoming string
int index = 0; // Index to track the position in the array

Servo servo1; // Create Servo object for the first servo (pin 10)
Servo servo2; // Create Servo object for the second servo (pin 11)

int rotationAngle = -15; // Variable for rotation angle (degrees)

const int pirPin = 6; // Pin connected to the PIR sensor
const int outputPin = 7; // Pin to send HIGH/LOW signal
int pirState = LOW; // Variable to store the PIR sensor state

unsigned long previousMillis = 0; // Stores the last time the PIR state was printed
const unsigned long interval = 2000; // Time interval in milliseconds (2 seconds)


void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  Serial.println("Serial communication ready. Waiting for strings...");

  // Attach the servos to their respective pins
  servo1.attach(10);
  servo2.attach(11);

  // Set initial positions for the servos
  servo1.write(90); // Set to neutral position (90 degrees)
  servo2.write(90); // Set to neutral position (90 degrees)

  // Initialize PIR sensor and output pins
  pinMode(pirPin, INPUT); // Set PIR pin as input
  pinMode(outputPin, OUTPUT); // Set outputPin as OUTPUT
}


void loop() {
  // Check if data is available to read
  while (Serial.available() > 0) {
    char incomingChar = Serial.read(); // Read one character from the buffer

    // Check for the end of the string (e.g., newline '\n')
    if (incomingChar == '\n') {
      incomingString[index] = '\0'; // Null-terminate the string

      // Trim carriage return or extra characters
      if (index > 0 && incomingString[index - 1] == '\r') {
        incomingString[index - 1] = '\0'; // Remove carriage return
      }

      Serial.print("Received string: ");
      Serial.println(incomingString); // Print the complete string

      // Compare the incoming string and take action
      if (strcmp(incomingString, "up") == 0) {
        // Get the current position of servo1
        int servo1Pos = servo1.read();
        // Rotate servo1 clockwise
        servo1.write(servo1Pos + rotationAngle);
        Serial.println("Servo1 rotated clockwise (up)");

      } else if (strcmp(incomingString, "down") == 0) {
        // Get the current position of servo1
        int servo1Pos = servo1.read();
        // Rotate servo1 counterclockwise
        servo1.write(servo1Pos - rotationAngle);
        Serial.println("Servo1 rotated counterclockwise (down)");

      } else if (strcmp(incomingString, "left") == 0) {
        // Get the current position of servo2
        int servo2Pos = servo2.read();
        // Rotate servo2 clockwise
        servo2.write(servo2Pos + rotationAngle);
        Serial.println("Servo2 rotated clockwise (left)");

      } else if (strcmp(incomingString, "right") == 0) {
        // Get the current position of servo2
        int servo2Pos = servo2.read();
        // Rotate servo2 counterclockwise
        servo2.write(servo2Pos - rotationAngle);
        Serial.println("Servo2 rotated counterclockwise (right)");
      }

      index = 0; // Reset the index for the next string
    } else {
      if (index < bufferSize - 1) { // Prevent buffer overflow
        incomingString[index] = incomingChar; // Store the character
        index++;
      }
    }
  }
}


// Read PIR sensor to detect motion
pirState = digitalRead(pirPin); // Read the state of the PIR sensor
// Get the current time
unsigned long currentMillis = millis();

// Print the PIR state only if the interval has passed
if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis; // Update the last print time

  if (pirState == HIGH) {
    Serial.println("Motion detected!");
    digitalWrite(outputPin, LOW); // Send LOW signal
    delay(100); // Keep it LOW briefly
    digitalWrite(outputPin, HIGH); // Set it back to HIGH for next detection
  } else {
    Serial.println("No motion detected.");
    digitalWrite(outputPin, HIGH); // Ensure it stays HIGH when no motion
  }
}

// Add a smaller delay to allow the loop to execute faster
delay(10); // A small 10ms delay to avoid overloading the CPU (adjustable) }