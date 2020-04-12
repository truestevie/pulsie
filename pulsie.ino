#include <Arduino.h>
#include "config.h"

const int LED_WIFI = 2;
const int sensor_pin = 5; // D1 pin - Use external pull-up resistor
bool previous_level = true;
bool current_level = true;
int total_n_pulses_detected = 0;
int previous_n_pulses_detected = 0;
const int debounce_time = DEBOUNCE_TIME;
unsigned long end_debounce_timestamp;
bool waiting_for_debounce = false;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_WIFI, OUTPUT);
}

void loop()
{
  current_level = digitalRead(sensor_pin);
  if (!waiting_for_debounce)
  {
    if (current_level != previous_level)
    {
      end_debounce_timestamp = millis() + debounce_time;
      waiting_for_debounce = true;
    }
  }
  else
  {
    if (end_debounce_timestamp < millis())
    {
      // new_level = digitalRead(sensor_pin);
      waiting_for_debounce = false;
      if (current_level == LOW && previous_level == HIGH)
      {
        total_n_pulses_detected++;
      }
      previous_level = current_level;
    }
  }

  if (previous_n_pulses_detected != total_n_pulses_detected)
  {
    Serial.print("Total number of detected pulses: ");
    Serial.println(total_n_pulses_detected);
  }
  previous_n_pulses_detected = total_n_pulses_detected;
}
