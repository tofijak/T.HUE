#ifndef ENTERTAINMENT_H
#define ENTERTAINMENT_H

#include "hue.h"

#define handle_error_mbedtls(e) _handle_error(e, __FUNCTION__, __LINE__)

struct RGBCommand
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct BehaviorEntry
{
  RGBCommand command;
};

// Behavior vars
struct Behavior
{
  const char *name;
  uint8_t enterainmentGroup;
  uint8_t lightIds[20];
  uint8_t lightCounter;
  bool activated;
  uint8_t behaviorId;
  BehaviorEntry entries[500];
  uint8_t entriesCount;
  int delay;
};
const uint8_t timeBehavior = 1;
const uint8_t repeatedBehavior = 2;
const uint8_t transitionBehavior = 3;

void initEntertainmentMode(Hue &hue, uint8_t roomId);
bool connect(Hue &hue, uint8_t roomId);
bool setColorRGB(uint8_t light_index, uint8_t red, uint8_t green, uint8_t blue);
bool update();
void initBehaviors(Hue &hue, Light lightIds[]);
void startBehavior(void *xStruct);
bool disconnect();
void updateBlinkingLightsEntries();
void updateAlarmLightsEntries();
void activateBehavior();

extern Behavior behaviors[20];
extern Behavior lastBehavior;
extern uint8_t behaviorCount;
extern bool fest;
extern bool startBehaviorNow;





#endif