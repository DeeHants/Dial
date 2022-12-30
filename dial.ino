// SPDX-FileCopyrightText: Copyright (c) 2022, Deanna Earley
// Inspiration from the Adafruit surfacedial_encoder_demo example
// SPDX-License-Identifier: BSD-3-Clause

#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <HID-Project.h>

// Set to 1 to enable the serial debug output
#define DEBUG 1

// Encoder
RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
int last_rotary = 0; // Used to check for direction change
bool last_button = false;

// Pixel (neopixel pixel of 1)
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
const int pixel_brightness = 255; // full brightness
int pixel_fade = 0; // Brightness while fading
int pixel_turned = 0; // Time pixel turned on

// Dial emulation
const int dial_distance = 100;

void setup() {
  // Debug output
  #if DEBUG
  Serial.begin(115200);
  // while (!Serial);
  delay(100);

  Serial.println("Rotary Trinkey dial");
  #endif

  // Setup the encoder
  attachInterrupt(PIN_ENCODER_A, checkPosition, CHANGE);
  attachInterrupt(PIN_ENCODER_B, checkPosition, CHANGE);
  // and button
  pinMode(PIN_ENCODER_SWITCH, INPUT_PULLDOWN);
  // Read current encoder position
  last_rotary = encoder.getPosition();

  // Setup the LED
  pixel.begin();
  pixel.setBrightness(0);
  pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // blue
  pixel.show(); // Initialize all pixels to 'off'
  pixel_fade = 0;

  // Send the first surface dial report
  SurfaceDial.begin();
}

// Interrupt to handle rotary changes
void checkPosition() {
  encoder.tick();
}

void loop() {
  bool activity = false;

  // Read encoder position
  int curr_rotary = encoder.getPosition();
  // Calculate the direction from our value, else we can have race conditions where direction and position don't update together
  RotaryEncoder::Direction direction;
  if (last_rotary > curr_rotary) {
    direction = RotaryEncoder::Direction::COUNTERCLOCKWISE;
  } else if (last_rotary < curr_rotary) {
    direction = RotaryEncoder::Direction::CLOCKWISE;
  } else {
    direction = RotaryEncoder::Direction::NOROTATION;
  }
  last_rotary = curr_rotary;

  // Handle dial rotation
  if (direction != RotaryEncoder::Direction::NOROTATION) {
    #if DEBUG
    Serial.println("Encoder value: " + String(curr_rotary) + ", direction: " + String((int)direction));
    #endif

    if (direction == RotaryEncoder::Direction::CLOCKWISE) {
      #if DEBUG
      Serial.println(" Rotate+");
      #endif
      SurfaceDial.rotate(dial_distance);
    }
    if (direction == RotaryEncoder::Direction::COUNTERCLOCKWISE) {
      #if DEBUG
      Serial.println(" Rotate-");
      #endif
      SurfaceDial.rotate(-dial_distance);
    }

    // Set the flag to enable the LED
    activity = true;
  }

  // Read switch state
  bool curr_button = digitalRead(PIN_ENCODER_SWITCH);
  if (curr_button != last_button) {
    if (curr_button && !last_button) { // Press
      #if DEBUG
      Serial.println("Press");
      #endif
      SurfaceDial.press();
    }
    if (!curr_button && last_button) { // Release
      #if DEBUG
      Serial.println("Release");
      #endif
      SurfaceDial.release();
    }
    last_button = curr_button;

    // Set the flag to enable the LED
    activity = true;
  }

  // Update pixel
  if (activity) {
    if (pixel_fade < pixel_brightness) {
      pixel.setBrightness(pixel_brightness);
      pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // blue
      pixel.show();
    }

    #if DEBUG
    Serial.println("Pixel turned on");
    #endif
    // Keep track of when it was enabled
    pixel_fade = pixel_brightness;
    pixel_turned = millis();

  } else if (pixel_fade > 0 && (millis() - pixel_turned) > 1000) {
    pixel_fade = pixel_fade - 1;
    if (pixel_fade < 0) { pixel_fade = 0; } // Limit it to 0
    #if DEBUG
    Serial.println("Pixel fading " + String(pixel_fade));
    #endif
    pixel.setBrightness(pixel_fade);
    pixel.show();
  }

  delay(10); // Stop repeated triggers
}
