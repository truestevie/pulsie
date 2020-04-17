#include <ESP8266WiFi.h>
#include "config.h"

const int LED_WIFI_PIN = 2;
const int LED_MAIN_PIN = 16;
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
const String WIFI_SSID = CONFIG_WIFI_SSID;
const String WIFI_PASSWORD = CONFIG_WIFI_PASSWORD;
wl_status_t previous_wifi_status = WL_DISCONNECTED;

class Led
{
private:
  byte pin;
  String mode;
  boolean isOn;
  int on_duration;
  int off_duration;
  unsigned long switchOnAt;
  unsigned long switchOffAt;
  int remaining_number_of_blinks;

public:
  Led(byte ledPin)
  {
    pin = ledPin;
    mode = "";
    on_duration = 0;
    off_duration = 0;
    remaining_number_of_blinks = 0;
    init();
  }
  void init()
  {
    pinMode(pin, OUTPUT);
    off();
  }
  void on()
  {
    if (mode == "blink")
    {
      if (remaining_number_of_blinks > 0)
      {
        simplyOn();
        switchOffAt = millis() + on_duration;
      }
    }
    else
    {
      simplyOn();
    }
  }

  void simplyOn()
  {
    digitalWrite(pin, LOW);
    isOn = true;
  }

  void off()
  {
    digitalWrite(pin, HIGH);
    isOn = false;

    if (mode == "blink")
    {
      remaining_number_of_blinks--;
      if (remaining_number_of_blinks > 0)
      {
        switchOnAt = millis() + off_duration;
      }
      else
      {
        mode = "";
      }
    }
  }

  void blink(int new_on_duration, int new_off_duration, int number_of_blinks)
  {
    // setMode("blink");
    mode = "blink";
    on_duration = new_on_duration;
    off_duration = new_off_duration;
    remaining_number_of_blinks = number_of_blinks;
    on();
  }

  void handle()
  {
    if (mode == "blink")
    {
      if (isOn && switchOffAt < millis())
      {
        off();
      }
      else if (!isOn && switchOnAt < millis())
      {
        on();
      }
    }
  }
};

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

Led followUpPulse(Led led)
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
        led.blink(100, 100, 1);
        // digitalWrite(LED_MAIN_PIN, LOW);
        // led_n_is_on = true;
        // led_main_timestamp_off = millis() + 100;
      }
      previous_level = current_level;
    }
  }
  return led;
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

void setWifiLed(Led led, wl_status_t wifi_status)
{
  if (wifi_status == WL_CONNECTED)
  {
    // led_wifi_timestamp_off = shortlyActivateLed(LED_WIFI_PIN, 100000);
    led.on();
  }
  if (wifi_status != WL_CONNECTED)
  {
    led.blink(100, 100, 20);
    // if (led_wifi_is_on && led_wifi_timestamp_off < millis())
    //   if (previous_wifi_status == WL_CONNECTED)
    //   {
    //     led_wifi_timestamp_off = millis();
    //     led_wifi_timestamp_on = millis() + 200;
    //   }
    //   else if (led_wifi_is_on && led_wifi_timestamp_off < millis())
    //   {
    //     digitalWrite(LED_WIFI_PIN, HIGH);
    //     led_wifi_is_on = false;
    //   }
    //   else if (!led_wifi_is_on && led_wifi_timestamp_on < millis())
    //   {
    //     digitalWrite(LED_WIFI_PIN, LOW);
    //     led_wifi_is_on = true;
    //   }
  }
  // previous_wifi_status = wifi_status;
}

Led led_wifi(LED_WIFI_PIN);
Led led_main(LED_MAIN_PIN);

void setup()
{
  Serial.begin(74880);
  attachInterrupt(SENSOR_PIN, handlePulse, CHANGE);
  // WiFi.persistent(false); // Switch off persistent mode, to avoid exception 3 when calling WiFi.begin()!!!
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop()
{
  led_main = followUpPulse(led_main);
  next_reporting_timestamp = reportNumberOfCountedPulses(REPORTING_INTERVAL, next_reporting_timestamp);
  led_main.handle();
}
