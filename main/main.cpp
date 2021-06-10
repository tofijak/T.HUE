#include <Arduino.h>
#include <WiFi.h>
#include "secret.h"
#include "hue.h"
#include "eink.h"
#include "utils.h"
#include "encoder.h"
#include "RotaryEncoder.h"
#include "entertainment.h"
#include "EEPROM.h"
#include "Firebase.h"

Hue hue;

TaskHandle_t taskHandleLoop;

bool taskCompleted = false;

void loopTask(void *pvParameters)
{
	Serial.begin(115200);
	Serial.println("looptask");
	setup();
	for (;;)
	{
		loopEink();
	}
}

extern "C" void app_main()
{
	initArduino();
	xTaskCreatePinnedToCore(&loopTask, "arduino_task", 20480, (void *)1, 4, &taskHandleLoop, 0);
}

void setup()
{
	printf("Starting");
	connectToWiFi();
	firebaseInit();
	hue.begin("Simon");
	initEink(hue);
	initEncoders(hue);
}
