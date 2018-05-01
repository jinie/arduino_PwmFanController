#include <Arduino.h>
#pragma once
#include <ArduinoJson.h>

class TemperatureProtocol{
  private:
    boolean humid = false;
    boolean pres = false;
    int capacity;
  public:
    TemperatureProtocol(int capacity);
    ~TemperatureProtocol();

    String register_sensor(String host_name, String sensor_name, boolean humidity, boolean pressure, String sensorType);
    String reading(String host, String sensor, String timestamp, float temperature, float rh, long pressure, unsigned int fan_rpm, String custom);
};

