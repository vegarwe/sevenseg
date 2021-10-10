#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <time.h>

#include "wifi_mqtt.h"

static const char* ntpServer = "pool.ntp.org"; // TODO: Move to config.h
static const long  gmtOffset_sec = 1 * 3600;
static const int   daylightOffset_sec = 1 * 3600;

static HardwareSerial* debugger = NULL;

static Adafruit_7segment matrix = Adafruit_7segment();

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

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
}

void loop()
{
    drawCurrentTime();

    wifi_mqtt_loop();

    delay(100);
}

//--------------------------------------------------------------------------------
void drawCurrentTime()
{
    static struct tm lastTimeinfo = {0};

    struct tm timeinfo;

    uint16_t blinkcounter = 0;

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
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

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

void luup()
{
    // try to print a number thats too long
    matrix.print(10000, DEC);
    matrix.writeDisplay();
    delay(500);

    // print a hex number
    matrix.print(0xBEEF, HEX);
    matrix.writeDisplay();
    delay(500);

    // print a floating point
    matrix.print(12.34);
    matrix.writeDisplay();
    delay(500);

    // print with print/println
    for (uint16_t counter = 0; counter < 9999; counter++) {
        matrix.println(counter);
        matrix.writeDisplay();
        delay(10);
    }

    // method #2 - draw each digit
    uint16_t blinkcounter = 0;
    boolean drawDots = false;
    for (uint16_t counter = 0; counter < 9999; counter ++) {
        matrix.writeDigitNum(0, (counter / 1000), drawDots);
        matrix.writeDigitNum(1, (counter / 100) % 10, drawDots);
        matrix.drawColon(drawDots);
        matrix.writeDigitNum(3, (counter / 10) % 10, drawDots);
        matrix.writeDigitNum(4, counter % 10, drawDots);

        blinkcounter+=50;
        if (blinkcounter < 500) {
            drawDots = false;
        } else if (blinkcounter < 1000) {
            drawDots = true;
        } else {
            blinkcounter = 0;
        }
        matrix.writeDisplay();
        delay(10);
    }
}
