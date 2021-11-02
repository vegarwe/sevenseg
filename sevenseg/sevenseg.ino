#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <time.h>
#include <SPI.h>
#include <Wire.h>

#include "wifi_mqtt.h"

static const char* ntpServer = "pool.ntp.org"; // TODO: Move to config.h
static const long  gmtOffset_sec = 1 * 3600;
static const int   daylightOffset_sec = 1 * 3600;

static HardwareSerial* debugger = NULL;

static Adafruit_7segment matrix = Adafruit_7segment();

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
static void mqtt_on_message(String &topic, String &payload)
{
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

    wifi_mqtt_setup(debugger, mqtt_on_message);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

//--------------------------------------------------------------------------------
void loop()
{
    drawCurrentTime();

    wifi_mqtt_loop();

    delay(10);
}

