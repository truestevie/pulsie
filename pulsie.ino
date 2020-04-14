#include <ESP8266WiFi.h>
#include "config.h"

const int LED_WIFI_PIN = 2;
const int LED_MAIN_PIN = 16;
bool led_main_is_on;
unsigned long led_main_timestamp_off = 0;
const int SENSOR_PIN = 5; // D1 pin - Use external pull-up resistor
bool previous_level = true;
volatile bool current_level = true; //Value changes in interrupt service routine, hence volatile
int total_n_pulses_detected = 0;
int previous_n_pulses_detected = 0;
const int DEBOUNCE_TIME = CONFIG_DEBOUNCE_TIME;
volatile unsigned long end_debounce_timestamp; //Value changes in interrupt service routine, hence volatile
volatile bool waiting_for_debounce = false;    //Value changes in interrupt service routine, hence volatile
unsigned long next_reporting_timestamp = 0;
const int REPORTING_INTERVAL = CONFIG_REPORTING_INTERVAL;

void ICACHE_RAM_ATTR handlePulse() //Interrupt service routine, hence add ICACHE_RAM_ATTR attribute
{
  current_level = digitalRead(SENSOR_PIN);
  if (!waiting_for_debounce)
  {
    if (current_level != previous_level)
    {
      end_debounce_timestamp = millis() + DEBOUNCE_TIME;
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
      current_level = digitalRead(SENSOR_PIN);
      waiting_for_debounce = false;
      if (current_level == LOW && previous_level == HIGH)
      {
        total_n_pulses_detected++;
        digitalWrite(LED_MAIN_PIN, LOW);
        led_main_is_on=true;
        led_main_timestamp_off = millis() + 100;
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

void dimLeds()
{
  if (led_main_is_on && led_main_timestamp_off < millis())
  {
    digitalWrite(LED_MAIN_PIN, HIGH);
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED_MAIN_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  digitalWrite(LED_MAIN_PIN, HIGH);
  digitalWrite(LED_WIFI_PIN, HIGH);
  led_main_is_on=false;
  attachInterrupt(SENSOR_PIN, handlePulse, CHANGE);
}

void loop()
{
  followUpPulse();
  dimLeds();
  next_reporting_timestamp = reportNumberOfCountedPulses(REPORTING_INTERVAL, next_reporting_timestamp);
}
