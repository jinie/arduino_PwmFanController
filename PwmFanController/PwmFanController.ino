#define SLAVE_ADDRESS 0x08
#define MQTT_MAX_PACKET_SIZE 300


#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiSTA.h>

#include "temperature_protocol.h"
#include <PubSubClient.h>

#include <RtcDateTime.h>
#include <RtcDS1307.h>
#include <RtcTemperature.h>
#include <RtcUtility.h>
#include <NTPClient.h>

#include <WEMOS_DHT12.h>

#define ssid       "SSID"
#define pass       "PASSWORD"

#define hostname "esp8266"
#define sensor "sensor name"
#define mqclientid "esp8266client"
#define topicBase "/surveillance"

#define fanPulse 0
#define pwmPin 2
#define pwmFreq 15000
#define minTemp 25
#define minPwm 20


DHT12 dht;
WiFiClient net;

RtcDS1307<TwoWire> rtc(Wire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "dk.pool.ntp.org", 0 );

PubSubClient mqtt(net);
TemperatureProtocol tempProt(MQTT_MAX_PACKET_SIZE);

const char * server = "mqtt.server.name";
void mqPublish(String topic, String message) ;
unsigned long lastmillis = 0;
unsigned long lastPublish = 0;



unsigned long pulseDuration;

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(mqclientid)) {
      Serial.println("connected");
      delay(1000);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup_rtc() {
  //--------RTC SETUP ------------
  rtc.Begin();
  timeClient.setUpdateInterval(300000);
  timeClient.begin();


  // if you are using ESP-01 then uncomment the line below to reset the pins to
  // the available pins for SDA, SCL
  // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  timeClient.update();
  RtcDateTime dt;
  dt.InitWithEpoch32Time(timeClient.getEpochTime());
  Serial.print("Setting RTC: ");
  printDateTime(dt);
  rtc.SetDateTime(dt);


  if (!rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    rtc.SetIsRunning(true);
  }

  RtcDateTime now = rtc.GetDateTime();
}

void setup()
{
  Serial.begin(57600);
  pinMode(fanPulse, INPUT);
  digitalWrite(fanPulse, HIGH);
  setup_wifi();

  setup_rtc();
  mqtt.setServer(server, 1883);
  reconnect();
  mqPublish(topicBase, tempProt.register_sensor(hostname, sensor, true, false, "DHT12"));
  analogWriteFreq(pwmFreq); //Set Pwm frequency

}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqPublish(const char * topic, String message) {
  char buf[MQTT_MAX_PACKET_SIZE];
  memset(buf, 0, sizeof(buf));
  message.toCharArray(buf, sizeof(buf));
  Serial.println(buf);
  boolean res = mqtt.publish(topic, buf, false);
#ifndef _NDEBUG
  Serial.print("MQ Publish returned : ");
  Serial.println(res);
#endif
}

#define countof(a) (sizeof(a) / sizeof(a[0]))


void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
   countof(datestring),
   PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
   dt.Month(),
   dt.Day(),
   dt.Year(),
   dt.Hour(),
   dt.Minute(),
   dt.Second() );
  Serial.println(datestring);
}

String DateTimeToString(const RtcDateTime& dt)
{
  char datestring[36];

  snprintf_P(datestring,
   countof(datestring),
   PSTR("%04u-%02u-%02u %02u:%02u:%02u.%06u+00:00"),
   dt.Year(),
   dt.Month(),
   dt.Day(),
   dt.Hour(),
   dt.Minute(),
   dt.Second(),
   millis() % 1000000);
  return String(datestring);
}

String log_reading(int pwmVal, float temperature) {
  String msg = "Temp: ";
  msg += dht.cTemp;
  msg += ", pwm: ";
  msg += pwmVal;
  return msg;
}
float lastTemp = 0;

void loop()
{
  reconnect();
  if (abs(millis() - lastmillis) > 20000) {
    if (dht.get() == 0) {
      int pwmVal = minPwm;
      if (dht.cTemp > minTemp) {
        pwmVal += ((dht.cTemp - minTemp) * (pwmFreq / 5));
        pwmVal = (pwmVal > pwmFreq) ? pwmFreq : pwmVal;
      }
      String msg = log_reading(pwmVal, dht.cTemp);

      analogWrite(pwmPin, pwmVal);
      Serial.println(msg);
      if (abs(millis() - lastPublish) > 60000) {
        unsigned long pulseDuration = pulseIn(fanPulse, LOW);
        unsigned int rpm = (pulseDuration > 0) ? (unsigned int)((1000 / pulseDuration) / 2) : 0;
        RtcDateTime now = rtc.GetDateTime();
        String custom = "{\"pwm\":";
        custom += pwmVal;
        custom += "}";
        mqPublish(topicBase, tempProt.reading(hostname, sensor, DateTimeToString(now), dht.cTemp, dht.humidity, 0, rpm, custom));
        lastPublish = millis();
      }

    }
    lastmillis = millis();
  }
  mqtt.loop();
}
