#include "temperature_protocol.h"

TemperatureProtocol::TemperatureProtocol(int capacity=500) {
  this->capacity = capacity;
}

TemperatureProtocol::~TemperatureProtocol() {

}

String TemperatureProtocol::reading(String host, String sensor, String timestamp, float temperature, float rh, long pressure, unsigned int fan_rpm, String custom) {
  DynamicJsonBuffer jsonBuffer(this->capacity);
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& reg = jsonBuffer.createObject();
  root["reading"] = reg;
  reg["host"] = host;
  reg["sensor"] = sensor;
  reg["reading"] = temperature;
  reg["humidity"] = rh;
  reg["timestamp"] = timestamp;
  reg["fan_rpm"] = fan_rpm;
  reg["custom"] = custom;
  String ret;
  root.printTo(ret);
  return ret;
}

String TemperatureProtocol::register_sensor(String host_name, String sensor_name, boolean humidity, boolean pressure, String sensorType) {
  DynamicJsonBuffer jsonBuffer(this->capacity);
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& reg = jsonBuffer.createObject();
  root["register_sensor"] = reg;
  reg["host"] = host_name;
  reg["sensor"] = sensor_name;
  reg["humidity"] = humidity ? true : false;
  reg["pressure"] = pressure ? true : false;
  reg["sensor_type"] = sensorType;
  this->humid = humidity;
  this->pres = pressure;
  String ret;
  root.printTo(ret);
  return ret;
}


