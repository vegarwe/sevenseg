#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <HardwareSerial.h>
#include <RCSwitch.h>
#include <SPI.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>

#include "wifi_mqtt.h"
#include "mqtt_ota.h"
#include "config.h"

static const char* ntpServer = "pool.ntp.org"; // TODO: Move to config.h

static HardwareSerial*  debugger = NULL;
static String           mqttPrefix;

static Adafruit_7segment matrix = Adafruit_7segment();

static RCSwitch         mySwitch;

//--------------------------------------------------------------------------------
void drawCurrentTime()
{
    static struct tm lastTimeinfo = {0};

    struct tm timeinfo;

    if (! getLocalTime(&timeinfo))
    {
        return;
    }

    if (    // Time has changed (since what's on the display)
            lastTimeinfo.tm_mday != timeinfo.tm_mday
            ||
            lastTimeinfo.tm_mon  != timeinfo.tm_mon
            ||
            lastTimeinfo.tm_year != timeinfo.tm_year
            ||
            lastTimeinfo.tm_sec  != timeinfo.tm_sec
        )
    {
        if (debugger) debugger->println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

        matrix.writeDigitNum(0, (timeinfo.tm_hour / 10), (lastTimeinfo.tm_sec + 1) >= 12 && lastTimeinfo.tm_sec != 59);
        matrix.writeDigitNum(1, (timeinfo.tm_hour % 10), (lastTimeinfo.tm_sec + 1) >= 24 && lastTimeinfo.tm_sec != 59);
        matrix.drawColon((timeinfo.tm_sec % 2) == 1);
        matrix.writeDigitNum(3, (timeinfo.tm_min  / 10), (lastTimeinfo.tm_sec + 1) >= 36 && lastTimeinfo.tm_sec != 59);
        matrix.writeDigitNum(4, (timeinfo.tm_min  % 10), (lastTimeinfo.tm_sec + 1) >= 48 && lastTimeinfo.tm_sec != 59);

        matrix.writeDisplay();
    }

    lastTimeinfo = timeinfo;
}


//--------------------------------------------------------------------------------
static void mqttMessageReceived(MQTTClient *client, char topicBuffer[], char payloadBuffer[], int length)
{
    String topic(topicBuffer);
    if (Update.isRunning() || topic.startsWith(mqttPrefix + "/ota/"))
    {
        if (! topic.startsWith(mqttPrefix + "/ota/")) return;

        if (! Update.isRunning())
        {
            // Nothing to do here
        }
        return mqtt_ota_handle_payload(topic, payloadBuffer, length);
    }

    String payload(payloadBuffer);

    if (debugger)
    {
        debugger->print("mqttMessageReceived: [");
        debugger->print(topic);
        debugger->print("] ");
        debugger->println(payload);
    }
}


//--------------------------------------------------------------------------------
void lpd433_loop()
{
    static uint64_t      lastValue = 0;
    static unsigned long lastStamp = 0;

    if (mySwitch.available()) {
        uint64_t recvValue = mySwitch.getReceivedValue();
        unsigned long now = millis();

        if (lastValue == recvValue && (now - lastStamp) < 1400)
        {
            //Serial.printf("Skipping lastStamp %lu now %lu, %lu\n", lastStamp, now, now - lastStamp);
        }
        else
        {
            if (debugger) {
                debugger->printf("value: %08llx ", recvValue);
                debugger->printf("bitlen: %d ",    mySwitch.getReceivedBitlength());
                debugger->printf("delay:  %d ",    mySwitch.getReceivedDelay());
                debugger->printf("proto:  %d\n",   mySwitch.getReceivedProtocol());
            }

            //if (debugger) {
            //    unsigned int * timings = mySwitch.getReceivedRawdata();

            //    for (int i = 0; i < mySwitch.getReceivedBitlength() * 2; i++) {
            //        Serial.printf("%d,", timings[i]);
            //    }
            //    Serial.println();
            //}

            char hexValue[] = "0x123456789abcdef0";
            snprintf(hexValue, sizeof(hexValue), "0x%08llx", recvValue);
            mqtt.publish(mqttPrefix + "/lpd433/up", hexValue);
        }

        lastValue = recvValue;
        lastStamp = now;

        mySwitch.resetAvailable();
    }
}


//--------------------------------------------------------------------------------
void setup()
{
    debugger = &Serial;

    if (debugger)
    {
        debugger->begin(115200);
        debugger->println("7 Segment Backpack starting");
    }

    matrix.begin(0x70);
    matrix.setBrightness(2);

    pinMode(19, INPUT);
    mySwitch.enableReceive(digitalPinToInterrupt(19));

    mqttPrefix = String(MQTT_ROOT "/") + WiFi.macAddress();
    wifi_mqtt_setup(debugger, mqttPrefix, mqttMessageReceived);
    mqtt_ota_setup(debugger, mqttPrefix);

    configTime(0, 0, ntpServer);
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    mqtt.publish(MQTT_ROOT "/control/up", "starting: " + WiFi.macAddress() + " 0x05");
}

//--------------------------------------------------------------------------------
void loop()
{
    drawCurrentTime();

    wifi_mqtt_loop();
    mqtt_ota_loop(); // Will block once update starts

    lpd433_loop();

    delay(10);
}

