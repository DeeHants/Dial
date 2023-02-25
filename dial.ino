// SPDX-FileCopyrightText: Copyright (c) 2022, Deanna Earley
// Inspiration from the Adafruit surfacedial_encoder_demo example
// SPDX-License-Identifier: BSD-3-Clause

#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <HID-Project.h>

// Set to 1 to enable the serial debug output
#define DEBUG 0

// Encoder
RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::FOUR3);
int last_rotary = 0; // Used to check for direction change

// Button
bool last_button = false;
int button_debounce = 0;
const int DEBOUNCE_TIME = 100;

// Pixel (neopixel pixel of 1)
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
const int PIXEL_BRIGHTNESS = 255; // full brightness
// Control the fade
int pixel_fade = 0; // Brightness while fading
int pixel_turned = 0; // Time pixel turned on
const int PIXEL_ON_TIME = 2000; // 2s
const int PIXEL_FADE_TIME = 2000; // 2s

// Dial emulation
const int dial_distance = 100;

void setup() {
  // Debug output
  #if DEBUG
  Serial.begin(115200);
  delay(100);

  Serial.println("Rotary Trinkey dial");
  #endif

  // Setup the encoder
  attachInterrupt(PIN_ENCODER_A, encoderInterrupt, CHANGE);
  attachInterrupt(PIN_ENCODER_B, encoderInterrupt, CHANGE);
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
void encoderInterrupt() {
  encoder.tick();
}

void loop() {
  bool activity = false;

  // Read encoder position and velocity
  int curr_rotary = encoder.getPosition();
  int velocity = curr_rotary - last_rotary;
  last_rotary = curr_rotary;

  // Handle dial rotation
  if (velocity != 0) {
    #if DEBUG
    Serial.println("Encoder value: " + String(curr_rotary) + ", velocity: " + String(velocity));
    #endif
    SurfaceDial.rotate(dial_distance * velocity);

    // Set the flag to enable the LED
    activity = true;
  }

  // Read switch state
  bool curr_button = digitalRead(PIN_ENCODER_SWITCH);
  int time_since_press = millis() - button_debounce;
  if (curr_button != last_button && time_since_press > DEBOUNCE_TIME) {
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
    button_debounce = millis();

    // Set the flag to enable the LED
    activity = true;
  }

  // Update pixel
  if (activity) {
    if (pixel_fade < PIXEL_BRIGHTNESS) {
      pixel.setBrightness(PIXEL_BRIGHTNESS);
      pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // blue
      pixel.show();
    }

    #if DEBUG
    Serial.println("Pixel turned on");
    #endif
    // Keep track of when it was enabled
    pixel_fade = PIXEL_BRIGHTNESS;
    pixel_turned = millis();

  } else if (pixel_fade > 0) {
    int time_since = millis() - pixel_turned;
    if (time_since > PIXEL_ON_TIME) {
      // Fade out over the next few seconds
      int fade_time = time_since - PIXEL_ON_TIME; // Time since the start of the fade
      float fade_multiplier = (float)1 - (fade_time / (float)PIXEL_FADE_TIME); // 0 -> 1, 1000 -> 0 (assuming 1s fade)
      pixel_fade = PIXEL_BRIGHTNESS * fade_multiplier;
      if (pixel_fade < 0) { pixel_fade = 0; } // Limit it to 0
      #if DEBUG
      Serial.println("Pixel fading for " + String(fade_time) + ", " + String(fade_multiplier) + ": " + String(pixel_fade));
      #endif
      pixel.setBrightness(pixel_fade);
      pixel.show();
    }
  }
}
