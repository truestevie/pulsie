#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <string.h>
// #include <ESPAsyncTCP.h>

// AsyncClient* client = new AsyncClient;
// client->onData(&handleDate, client);

#include "./config.h"

const int LED_WIFI_PIN = 2;
const int LED_MAIN_PIN = 16;
const int SENSOR_PIN = 5;  // D1 pin - Use external pull-up resistor
bool previous_level = true;

// Values changes in interrupt service routine, hence volatile
volatile bool current_level = true;
volatile unsigned long end_debounce_timestamp;
volatile bool waiting_for_debounce = false;

int total_n_pulses_detected = 0;
int previous_n_pulses_detected = 0;
const int DEBOUNCE_TIME = CONFIG_DEBOUNCE_TIME;
unsigned long next_reporting_timestamp = 0;
const int REPORTING_INTERVAL = CONFIG_REPORTING_INTERVAL;
const String WIFI_SSID = CONFIG_WIFI_SSID;
const String WIFI_PASSWORD = CONFIG_WIFI_PASSWORD;
wl_status_t previous_wifi_status = WL_DISCONNECTED;
wl_status_t current_wifi_status;

String DB_URL = INFLUXDB_URL;
String DB_PAYLOAD_PREFIX = INFLUXDB_PAYLOAD_PREFIX;
String DB_AUTHORIZATION = INFLUXDB_AUTHORIZATION;

const int HTTP_TIMEOUT = INFLUXDB_TIMEOUT;

class Led {
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
    explicit Led(byte ledPin) {
        pin = ledPin;
        mode = "";
        on_duration = 0;
        off_duration = 0;
        remaining_number_of_blinks = 0;
        init();
    }
    void init() {
        pinMode(pin, OUTPUT);
        off();
    }
    void on() {
        if (mode == "blink") {
            if (remaining_number_of_blinks > 0 ||
                remaining_number_of_blinks == -1) {
                simplyOn();
                switchOffAt = millis() + on_duration;
            }
        } else {
            simplyOn();
        }
    }

    void simplyOn() {
        digitalWrite(pin, LOW);
        isOn = true;
    }

    void off() {
        digitalWrite(pin, HIGH);
        isOn = false;

        if (mode == "blink") {
            if (remaining_number_of_blinks != -1) {
                remaining_number_of_blinks--;
            }
            if (
                remaining_number_of_blinks > 0 ||
                remaining_number_of_blinks == -1) {
                switchOnAt = millis() + off_duration;
            } else {
                mode = "";
            }
        }
    }

    void blink(
        int new_on_duration,
        int new_off_duration,
        int number_of_blinks) {
        mode = "blink";
        on_duration = new_on_duration;
        off_duration = new_off_duration;
        remaining_number_of_blinks = number_of_blinks;
        on();
    }

    void constant_on() {
        mode = "constant on";
        on();
    }

    void handle() {
        if (mode == "blink") {
            if (isOn && switchOffAt < millis()) {
                off();
            } else if (!isOn && switchOnAt < millis()) {
                on();
            }
        } else if (mode == "constant on") {
            on();
        }
    }
};

// Interrupt service routine, hence add ICACHE_RAM_ATTR attribute
void ICACHE_RAM_ATTR handlePulse() {
    current_level = digitalRead(SENSOR_PIN);
    if (!waiting_for_debounce) {
        if (current_level != previous_level) {
            end_debounce_timestamp = millis() + DEBOUNCE_TIME;
            waiting_for_debounce = true;
        }
    }
}

Led followUpPulse(Led led) {
    if (waiting_for_debounce) {
        if (end_debounce_timestamp < millis()) {
            current_level = digitalRead(SENSOR_PIN);
            waiting_for_debounce = false;
            if (current_level == LOW && previous_level == HIGH) {
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

Led followUpWifiStatus(Led led) {
    current_wifi_status = WiFi.status();
    if (current_wifi_status != previous_wifi_status) {
        Serial.print("WIFI status: ");
        Serial.println(wl_status_to_string(current_wifi_status));
        led = setWifiLed(led, current_wifi_status);
        previous_wifi_status = current_wifi_status;
        if (current_wifi_status == WL_CONNECTED) {
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        }
    }
    return led;
}

bool uploadToInfluxDb(int value, int httpTimeout) {
    HTTPClient http;
    http.setTimeout(httpTimeout);
    unsigned long httpStarted = millis();
    http.begin(DB_URL);
    http.addHeader("Content-type", "text:plain");
    http.addHeader("Authorization", DB_AUTHORIZATION);
    String payload = DB_PAYLOAD_PREFIX;
    payload += value;
    Serial.print("Payload: ");
    Serial.println(payload);
    int returnCode = http.POST(payload);
    http.end();
    unsigned long httpEnded = millis();
    Serial.print("Return code: ");
    Serial.println(returnCode);
    Serial.print("HTTP POST took ms: ");
    Serial.println(httpEnded - httpStarted);
    if (returnCode == 204) {
        return true;
    } else {
        return false;
    }
}

unsigned long reportNumberOfCountedPulses(
    const int reportInterval,
    const int httpTimeout,
    unsigned long next_reporting_timestamp) {
    bool uploadSucceeded = false;
    if (next_reporting_timestamp < millis()) {
        int intervalPulses =
            total_n_pulses_detected - previous_n_pulses_detected;
        if (intervalPulses > 0) {
            Serial.print("Total number of detected pulses: ");
            Serial.println(total_n_pulses_detected);
            Serial.printf(
                "Number of pulses during this interval: %d\n",
                intervalPulses);
            uploadSucceeded = uploadToInfluxDb(intervalPulses, httpTimeout);
        }
        if (uploadSucceeded) {
            previous_n_pulses_detected = total_n_pulses_detected;
        }
        next_reporting_timestamp = millis() + reportInterval;
    }
    return next_reporting_timestamp;
}

const String wl_status_to_string(wl_status_t status) {
    switch (status) {
        case WL_NO_SHIELD:
            return "WL_NO_SHIELD";
        case WL_IDLE_STATUS:
            return "WL_IDLE_STATUS";
        case WL_NO_SSID_AVAIL:
            return "WL_NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED:
            return "WL_SCAN_COMPLETED";
        case WL_CONNECTED:
            return "WL_CONNECTED";
        case WL_CONNECT_FAILED:
            return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
            return "WL_CONNECTION_LOST";
        case WL_DISCONNECTED:
            return "WL_DISCONNECTED";
    }
}

Led setWifiLed(Led led, wl_status_t wifi_status) {
    if (wifi_status == WL_CONNECTED) {
        led.constant_on();
    } else if (wifi_status == WL_DISCONNECTED) {
        led.blink(200, 200, -1);
    } else if (wifi_status == WL_NO_SSID_AVAIL) {
        led.blink(100, 300, -1);
    } else {
        led.blink(50, 350, -1);
    }
    return led;
}

Led led_wifi(LED_WIFI_PIN);
Led led_main(LED_MAIN_PIN);

void setup() {
    Serial.begin(74880);
    attachInterrupt(SENSOR_PIN, handlePulse, CHANGE);
    // Switch off persistent mode,
    // to avoid exception 3 when calling WiFi.begin()
    WiFi.persistent(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
    led_main = followUpPulse(led_main);
    led_wifi = followUpWifiStatus(led_wifi);
    next_reporting_timestamp =
        reportNumberOfCountedPulses(
            REPORTING_INTERVAL, HTTP_TIMEOUT, next_reporting_timestamp);
    led_main.handle();
    led_wifi.handle();
}
