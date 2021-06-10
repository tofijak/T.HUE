#ifndef ENCODER_h
#define ENCODER_h

#include "hue.h"

void initEncoders(Hue &hue);
void loopEncoders();

void encoderLoop(void *pvParameters);

void adjustLight();
void chooseAndAlterLightInRoom();

extern bool calledFromEncoder;

#endif