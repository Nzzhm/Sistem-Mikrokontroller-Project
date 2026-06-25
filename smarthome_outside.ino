#include <ESP32Servo.h>

Servo s1;

#define RAIN_SENSOR 27
#define SERVO_PIN 18

int val = 0;

void setup()
{
    Serial.begin(115200);

    pinMode(RAIN_SENSOR, INPUT);

    s1.attach(SERVO_PIN);
}

void loop()
{
    val = digitalRead(RAIN_SENSOR);

    Serial.println(val);

    if(val == LOW)
    {
        Serial.println("Hujan");
        s1.write(90);
    }
    else
    {
        Serial.println("Tidak Hujan");
        s1.write(0);
    }

    delay(100);
}