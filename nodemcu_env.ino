#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "Adafruit_BMP280.h"
#include "Adafruit_Si7021.h"
#include "Adafruit_CCS811.h"

#include "ESPDash.h"

#define PLACE "bath"

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#endif

const char *ssid = STASSID;
const char *pass = STAPSK;

unsigned long last = 0;
int last_hour = 0;
struct tm ntp_time;

IPAddress ipaddr(192, 168, 100, 15);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 100, 1);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(9, 9, 9, 9);

AsyncWebServer webserver(80);
ESPDash dashboard(&webserver);

Card temperature_bmp(&dashboard, TEMPERATURE_CARD, "Temperature (BMP)", "°C");
Card temperature_si(&dashboard, TEMPERATURE_CARD, "Temperature (Si)", "°C");
Card humidity(&dashboard, HUMIDITY_CARD, "Humidity", "%");
Card pressure(&dashboard, GENERIC_CARD, "Pressure", "hPa");
Card co2(&dashboard, GENERIC_CARD, "CO2", "ppm");
Card tvoc(&dashboard, GENERIC_CARD, "TVOC", "ppb");

Chart max_temperature_bmp_chart(&dashboard, BAR_CHART, "Temperature (BMP)");
Chart max_temperature_si_chart(&dashboard, BAR_CHART, "Temperature (Si)");
Chart max_humidity_chart(&dashboard, BAR_CHART, "Humidity");
Chart max_pressure_chart(&dashboard, BAR_CHART, "Pressure");
Chart max_co2_chart(&dashboard, BAR_CHART, "CO2");
Chart max_tvoc_chart(&dashboard, BAR_CHART, "TVOC");

String time_axis[] = {"0h", "1h", "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "10h", "11h", "12h", "13h", "14h", "15h", "16h", "17h", "18h", "19h", "20h", "21h", "22h", "23h"};
float max_temperature_bmp_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};
float max_temperature_si_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};
float max_humidity_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};
float max_pressure_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};
int max_co2_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};
int max_tvoc_axis[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,};


Adafruit_BMP280 bmp; // I2C
Adafruit_Si7021 si;
Adafruit_CCS811 ccs;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup...");

  if (!bmp.begin(0x76, 0x58)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  if (!si.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (1);
  }

  if (!ccs.begin()) {
    Serial.println("Failed to start CCS811! Please check your wiring.");
    while (1);
  }

  Serial.print("WiFi Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.config(ipaddr, gateway, subnet, dns1, dns2);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }
  Serial.println("Done");

  configTime(3600, 0, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  
  webserver.begin();
}

void loop() {
  auto now = millis();
  if (now - last > 10000) {
    ntp_time.tm_year = 0;
    if(getLocalTime(&ntp_time, 5000)) {
      Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (ntp_time.tm_year) + 1900, (ntp_time.tm_mon) + 1, ntp_time.tm_mday, ntp_time.tm_hour, ntp_time.tm_min, ntp_time.tm_sec);
      {
        auto temp = bmp.readTemperature();
        recordMaxValue(temp, max_temperature_bmp_chart, max_temperature_bmp_axis);
        temperature_bmp.update(temp);
      }
      {
        auto temp = bmp.readPressure()/100.0;
        recordMaxValue(temp, max_pressure_chart, max_pressure_axis);
        pressure.update((float)temp);
      }
      {
        auto temp = si.readHumidity();
        recordMaxValue(temp, max_humidity_chart, max_humidity_axis);
        humidity.update(temp);
      }
      {
        auto temp = si.readTemperature();
        recordMaxValue(temp, max_temperature_si_chart, max_temperature_si_axis);
        temperature_si.update(temp);
      }
      if (ccs.available()) {
        if (!ccs.readData()) {
          recordMaxValue(ccs.geteCO2(), max_co2_chart, max_co2_axis);
          co2.update(ccs.geteCO2());
          recordMaxValue(ccs.getTVOC(), max_tvoc_chart, max_tvoc_axis);
          tvoc.update(ccs.getTVOC());
        }
      }
      dashboard.sendUpdates();
    }
    last = now;
    last_hour = ntp_time.tm_hour;
  }
}

void recordMaxValue(auto value, Chart &chart, auto *axis) {
  if (last_hour != ntp_time.tm_hour) axis[ntp_time.tm_hour] = 0;
  if (value > axis[ntp_time.tm_hour]) {
    axis[ntp_time.tm_hour] = value;
    chart.updateX(time_axis, ntp_time.tm_hour+1);
    chart.updateY(max_temperature_bmp_axis, ntp_time.tm_hour+1);
  }
}

bool getLocalTime(struct tm * info, uint32_t ms) {
  uint32_t count = ms / 10;
  time_t now;

  time(&now);
  localtime_r(&now, info);

  if (info->tm_year > 100) {
    return true;
  }

  while (count--) {
    delay(10);
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > 100) {
      return true;
    }
  }
  return false;
}
