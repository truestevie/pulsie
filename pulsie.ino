#include <ESP8266WiFi.h>
#include "config.h"

const int LED_WIFI = 2;
const int sensor_pin = 5; // D1 pin - Use external pull-up resistor
bool previous_level = true;
volatile bool current_level = true; //Value changes in interrupt service routine, hence volatile
int total_n_pulses_detected = 0;
int previous_n_pulses_detected = 0;
const int debounce_time = DEBOUNCE_TIME;
volatile unsigned long end_debounce_timestamp; //Value changes in interrupt service routine, hence volatile
volatile bool waiting_for_debounce = false;    //Value changes in interrupt service routine, hence volatile
unsigned long next_reporting_timestamp = 0;
const int reporting_interval = REPORTING_INTERVAL;

void ICACHE_RAM_ATTR handlePulse() //Interrupt service routine, hence add ICACHE_RAM_ATTR attribute
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
}

void followUpPulse()
{
  if (waiting_for_debounce)
  {
    if (end_debounce_timestamp < millis())
    {
      current_level = digitalRead(sensor_pin);
      waiting_for_debounce = false;
      if (current_level == LOW && previous_level == HIGH)
      {
        total_n_pulses_detected++;
      }
      previous_level = current_level;
    }
  }
}

unsigned long reportNumberOfCountedPulses(const int reportInterval, unsigned long next_reporting_timestamp)
{
  if (next_reporting_timestamp < millis())
  {
    if (previous_n_pulses_detected != total_n_pulses_detected)
    {
      Serial.print("Total number of detected pulses: ");
      Serial.println(total_n_pulses_detected);
    }
    previous_n_pulses_detected = total_n_pulses_detected;
    next_reporting_timestamp = millis() + reportInterval;
  }
  return next_reporting_timestamp;
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED_WIFI, OUTPUT);
  attachInterrupt(sensor_pin, handlePulse, CHANGE);
}

void loop()
{
  followUpPulse();
  next_reporting_timestamp = reportNumberOfCountedPulses(reporting_interval, next_reporting_timestamp);
}
