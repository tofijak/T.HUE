#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "eink.h"
#include <vector>
#include <iostream>
#include <Arduino.h>
#include "utils.h"
#include "hue.h"
#include "ArduinoJson.h"
#include <stdlib.h>
#include "bitmaps.h"
#include "encoder.h"
#include "entertainment.h"
#include "Firebase.h"
#include "secret.h"

void drawHome(int, int);
void drawAllGroups(int, int);
void drawRoom(int, int);
void drawLight(int, int);
void drawAllLights(int, int);
void drawBehavior(int, int);
void drawAllBehaviors(int, int);
void drawHome(int);

const int drawHomeID = 0;
const int drawAllRoomsID = 1;
const int drawRoomID = 2;
const int drawLightID = 3;
const int drawAllLightsID = 4;
const int drawAllBehaviorID = 5;
const int drawBehaviorID = 6;
const int changeBehaviorID = 7;

Gdew027w3T* _display;

void drawRoom(int room, int)
{
  currentScreenType = "ROOM";
  currentRoomID = drawRoomID;
  Serial.println("Room: " + String(room));
  Serial.println("LastRoom: " + String(lastRoom));

  if (lastRoom != room)
  { //Declared variables have a value even when not initialized - uint8_t = 0 - ID of a room can never be zero
    currentRoom = rooms[room];
  }
  lastRoom = room;
  drawHeader(currentRoom.name, &drawRoom, false);
  currentPageOnScreen = "RUM";
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawRoom", currentRoom.name.c_str());
  drawText(20, 33, 1, "VÆLG EN LAMPE", EPD_BLACK);
  lightCount = currentRoom.lightCounter;
  Light roomLights[currentRoom.lightCounter];
  _hue->getLightStates(lights, currentRoom.lightIds, currentRoom.lightCounter);
  boolean pagination = false;
  if (currentRoom.lightCounter > 4)
    pagination = true;
  if (pagination)
  {
    topIndex = 4;
    drawPaginationButtons(lightCount);
  }
  drawLightsFromArray(0, 4);

  refreshBody();
}

void drawAllGroups(int, int)
{
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawAllRooms");
  currentScreenType = "ROOMS";
  currentRoomID = drawAllRoomsID;
  currentPageOnScreen = "ALLE RUM";
  drawHeader("ALLE RUM", &drawAllGroups);
  drawText(20, 33, 1, "VÆLG ET RUM", EPD_BLACK);
  if (roomPagination)
  {
    topIndex = 4;
    drawPaginationButtons(roomsCount);
  }
  drawRoomsFromArray(0, 4);
  refreshBody();
}



void incrementBlinkingLightsSpeed(int, int)
{
  std::string delay = std::to_string(blinkingLightsDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_WHITE);
  blinkingLightsDelay += 1000;
  delay = std::to_string(blinkingLightsDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  behaviors[0].delay = blinkingLightsDelay / 10;
}

void decrementBlinkingLightsSpeed(int, int)
{
  std::string delay = std::to_string(blinkingLightsDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_WHITE);
  if (blinkingLightsDelay > 1000)
    blinkingLightsDelay -= 1000;
  delay = std::to_string(blinkingLightsDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  behaviors[0].delay = blinkingLightsDelay / 10;
}

void incrementBlinkingLightsCount(int, int)
{
  std::string count = std::to_string(blinkingLightsEntries);
  drawText(14, 169, 1, count + " gange", EPD_WHITE);
  blinkingLightsEntries += 10;
  count = std::to_string(blinkingLightsEntries);
  Serial.println(count.c_str());
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}

void decrementBlinkingLightsCount(int, int)
{
  std::string count = std::to_string(blinkingLightsEntries);
  drawText(14, 169, 1, count + " gange", EPD_WHITE);
  if (blinkingLightsEntries > 10)
    blinkingLightsEntries -= 10;
  count = std::to_string(blinkingLightsEntries);
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}
void incrementAlarmDuration(int, int)
{
  std::string delay = std::to_string(alarmDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_WHITE);
  alarmDelay += 10000;
  delay = std::to_string(alarmDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  behaviors[1].delay = alarmDelay / 10;
}

void decrementAlarmDuration(int, int)
{
  std::string delay = std::to_string(alarmDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_WHITE);
  if (alarmDelay > 1000)
    alarmDelay -= 10000;
  delay = std::to_string(alarmDelay / 1000);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  behaviors[1].delay = alarmDelay / 10;
}

void incrementAlarmFlashCount(int, int)
{
  std::string count = std::to_string(alarmEntries);
  drawText(14, 169, 1, count + " gange", EPD_WHITE);
  alarmEntries += 10;
  count = std::to_string(alarmEntries);
  Serial.println(count.c_str());
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}

void decrementAlarmFlashCount(int, int)
{
  std::string count = std::to_string(alarmEntries);
  drawText(14, 169, 1, count + " gange", EPD_WHITE);
  if (alarmEntries > 10)
    alarmEntries -= 10;
  count = std::to_string(alarmEntries);
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}

void incrementFestDuration(int, int)
{
  std::string duration = std::to_string(festEntries / 1000);
  drawText(14, 69, 1, duration + " blink i alt", EPD_WHITE);
  if (festEntries < 500)
    festEntries += 50;
  duration = std::to_string(festEntries / 1000);
  drawText(14, 69, 1, duration + " blink i alt", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}

void decrementFestDuration(int, int)
{
  std::string duration = std::to_string(festEntries / 1000);
  drawText(14, 69, 1, duration + " blink i alt", EPD_WHITE);
  if (festEntries > 1)
    festEntries -= 50;
  duration = std::to_string(festEntries / 1000);
  drawText(14, 69, 1, duration + " blink i alt", EPD_BLACK);
  xTaskCreate(&updateBehaviors, "behavior_update_task", 2048, (void *)1, 2, &taskHandlerbehaviorUpdater);
}

void incrementFestSpeed(int, int)
{
  std::string count = std::to_string(festDelay);
  drawText(14, 169, 1, count + " blink per sekund", EPD_WHITE);
  festDelay += 1;
  count = std::to_string(festDelay);
  drawText(14, 169, 1, count + " blink per sekund", EPD_BLACK);
  behaviors[2].delay = 1000 / festDelay / 10;
}

void decrementFestSpeed(int, int)
{
  std::string count = std::to_string(festDelay);
  drawText(14, 169, 1, count + " blink per sekund", EPD_WHITE);
  if (festDelay > 1)
    festDelay -= 1;
  count = std::to_string(festDelay);
  drawText(14, 169, 1, count + " blink per sekund", EPD_BLACK);
  behaviors[2].delay = 1000 / festDelay / 10;
}

void changeBehaviorForTraining(int id, int)
{
  currentRoomID = changeBehaviorID;
  Behavior &behavior = behaviors[id];
  lastBehavior = behavior;
  lastBehaviorId = id;
  drawHeader(behavior.name, &drawBehavior);
  firebaseAppendUserAction(touchScreenInteraction, navigate, "changeBehaviorForTraining", behavior.name);
  std::string delay = std::to_string(blinkingLightsDelay / 1000);
  drawText(14, 34, 1, "ÆNDRE HVOR HURTIGT", EPD_BLACK);
  drawText(14, 49, 1, "LYSET SKAL BLINKE", EPD_BLACK);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  _display->setTextSize(2);
  drawSquare(14, 82, 74, 40, 0, EPD_WHITE, "-", true, EPD_BLACK, &decrementBlinkingLightsSpeed, id);
  drawSquare(14 + 74, 82, 74, 40, 0, EPD_WHITE, "+", true, EPD_BLACK, &incrementBlinkingLightsSpeed, id);
  _display->setTextSize(1);
  std::string count = std::to_string(blinkingLightsEntries);
  drawText(14, 134, 1, "ÆNDRE HVOR MANGE GANGE", EPD_BLACK);
  drawText(14, 149, 1, "LYSET SKAL BLINKE", EPD_BLACK);
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  _display->setTextSize(2);
  drawSquare(14, 184, 74, 40, 0, EPD_WHITE, "-", true, EPD_BLACK, &decrementBlinkingLightsCount, id);
  drawSquare(14 + 74, 184, 74, 40, 0, EPD_WHITE, "+", true, EPD_BLACK, &incrementBlinkingLightsCount, id);
  _display->setTextSize(1);
  refreshBody();
}

void changeBehaviorForAlarm(int id, int)
{
  currentRoomID = changeBehaviorID;
  Behavior &behavior = behaviors[id];
  lastBehavior = behavior;
  lastBehaviorId = id;
  drawHeader(behavior.name, &drawBehavior);
  firebaseAppendUserAction(touchScreenInteraction, navigate, "changeBehaviorForAlarm", behavior.name);
  std::string delay = std::to_string(alarmDelay / 1000);
  drawText(14, 34, 1, "ÆNDRE HVOR LANG TID", EPD_BLACK);
  drawText(14, 49, 1, "ALARM VARER", EPD_BLACK);
  drawText(14, 69, 1, delay + " sekunder", EPD_BLACK);
  _display->setTextSize(2);
  drawSquare(14, 82, 74, 40, 0, EPD_WHITE, "-", true, EPD_BLACK, &decrementAlarmDuration, id);
  drawSquare(14 + 74, 82, 74, 40, 0, EPD_WHITE, "+", true, EPD_BLACK, &incrementAlarmDuration, id);
  _display->setTextSize(1);
  std::string count = std::to_string(blinkingLightsEntries);
  drawText(14, 134, 1, "ÆNDRE HVOR MANGE GANGE", EPD_BLACK);
  drawText(14, 149, 1, "ALARMEN SKAL BLINKE", EPD_BLACK);
  drawText(14, 169, 1, count + " gange", EPD_BLACK);
  _display->setTextSize(2);
  drawSquare(14, 184, 74, 40, 0, EPD_WHITE, "-", true, EPD_BLACK, &decrementAlarmFlashCount, id);
  drawSquare(14 + 74, 184, 74, 40, 0, EPD_WHITE, "+", true, EPD_BLACK, &incrementAlarmFlashCount, id);
  _display->setTextSize(1);
  refreshBody();
}

void changeBehaviorForFest(int id, int)
{
  currentRoomID = changeBehaviorID;
  Behavior &behavior = behaviors[id];
  lastBehavior = behavior;
  lastBehaviorId = id;
  drawHeader(behavior.name, &drawBehavior);
  firebaseAppendUserAction(touchScreenInteraction, navigate, "changeBehaviorForFest", behavior.name);
  _display->setTextSize(1);
  std::string count = std::to_string(festDelay);
  drawText(14, 134, 1, "ÆNDRE HVOR HURTIGT", EPD_BLACK);
  drawText(14, 149, 1, "LYSENE SKAL BLINKE", EPD_BLACK);
  drawText(14, 169, 1, count + " blink per sekund", EPD_BLACK);
  _display->setTextSize(2);
  drawSquare(14, 184, 74, 40, 0, EPD_WHITE, "-", true, EPD_BLACK, &decrementFestSpeed, id);
  drawSquare(14 + 74, 184, 74, 40, 0, EPD_WHITE, "+", true, EPD_BLACK, &incrementFestSpeed, id);
  _display->setTextSize(1);
  refreshBody();
}

void drawBehavior(int id, int)
{
  currentRoomID = drawBehaviorID;
  Behavior &behavior = behaviors[id];
  lastBehavior = behavior;
  lastBehaviorId = id;
  drawHeader(behavior.name, &drawBehavior);
  currentPageOnScreen = "drawBehavior";
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawBehavior", behavior.name);
  std::string lightCount = std::to_string(behavior.lightCounter);
  drawText(14, 33, 1, lightCount + " PÆRER ER VALGT I EN ", EPD_BLACK);
  std::string behaviorType;
  getBehaviorType(behavior.behaviorId, behaviorType);
  drawText(14, 48, 1, behaviorType + " OPFØRSEL", EPD_BLACK);
  drawSquare(23, 65, 130, 30, 0, EPD_BLACK, "REDIGER PÆRER", true, EPD_BLACK, &drawAllLights, id);
  if (behavior.behaviorId == repeatedBehavior)
    drawSquare(23, 110, 130, 30, 0, EPD_BLACK, "REDIGER OPFØRSEL", true, EPD_BLACK, &changeBehaviorForTraining, id);
  if (behavior.behaviorId == timeBehavior)
    drawSquare(23, 110, 130, 30, 0, EPD_BLACK, "REDIGER OPFØRSEL", true, EPD_BLACK, &changeBehaviorForAlarm, id);
  if (behavior.behaviorId == transitionBehavior)
    drawSquare(23, 110, 130, 30, 0, EPD_BLACK, "REDIGER OPFØRSEL", true, EPD_BLACK, &changeBehaviorForFest, id);
  drawText(14, 162, 1, "START ELLER STOP", EPD_BLACK);
  drawText(14, 177, 1, "OPFØRSEL", EPD_BLACK);
  if (behavior.activated == false)
  {
    drawSquare(14, 184, 74, 40, 0, EPD_WHITE, "START", true, EPD_BLACK, &toggleBehavior, id);
    drawSquare(14 + 74, 184, 74, 40, 0, EPD_BLACK, "STOP", true, EPD_BLACK, &toggleBehavior, id);
  }
  else
  {
    drawSquare(14, 184, 74, 40, 0, EPD_BLACK, "START", true, EPD_BLACK, &toggleBehavior, id);
    drawSquare(14 + 74, 184, 74, 40, 0, EPD_WHITE, "STOP", true, EPD_BLACK, &toggleBehavior, id);
  }
  refreshBody();
}

void drawAllBehaviors(int, int)
{
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawAllBehaviors");
  currentRoomID = drawAllBehaviorID;
  drawHeader("OPFØRSLER", &drawAllBehaviors);
  drawText(14, 33, 1, "VÆLG EN OPFØRSEL", EPD_BLACK);
  int x = 20;
  int y = 56;
  for (int i = 0; i < behaviorCount; i++)
  {
    Behavior &behavior = behaviors[i];
    std::string s = behavior.name;
    drawBitmap(x, y, 0, 0, NULL, bulbOutlinedBitmap, bulbWidth, bulbHeight, s, &drawBehavior, i, false);
    if (i % 2 == 1)
    {
      y += 80;
      x = 20;
    }
    else
    {
      x += 80;
    }
  }
  refreshBody();
}


void drawLight(int lightId, int)
{
  std::string name;
  if (!calledFromEncoder)
  {
    firebaseAppendUserAction(touchScreenInteraction, navigate, "drawLight", currentLight.name.c_str());
    calledFromEncoder = false;

    for (int i = 0; i < allLightCount; i++)
    {
      printf("ID: %d \n", lights[i].id);
      if (lights[i].id == pressedButton.id)
      {
        currentLight = lights[i];
      }
    }
    name = pressedButton.name;
  }
  else
  {
    for (int i = 0; i < allLightCount; i++)
    {
      printf("ID: %d \n", lights[i].id);
      if (lights[i].id == lightId)
      {
        currentLight = lights[i];
      }
    }
    name = currentLight.name;
  }

  printf("LightId: %d \n", lightId);
  printf("LightName: %s \n", name.c_str());

  currentScreenType = "LIGHT";
  currentRoomID = drawLightID;
  drawHeader(name, &drawLight);
  drawText(14, 28, 0, "SKIFT PÆRENS UDSEENDE", EPD_BLACK);
  drawBitmap(_display->width() / 2, 60, 0, 0, bulbOutlinedBitmap, bulbBitmap, bulbWidth, bulbHeight, name, &Switch, currentLight.id, true, currentLight.on);
  drawBar(14, 110, currentLight.brightness);

  if (!currentLight.onlyDimmable)
  {
    drawText(14, 135, 0, "VÆLG FARVE SKALA", EPD_BLACK);
    drawCircle(14 + 35, 152 + 35, 35, EPD_BLACK, false, true, "FARVE", &toggleRGBMode, currentLight.id, true);
    drawCircle(127, 152 + 35, 35, EPD_BLACK, true, true, "TEMP", &toggleRGBMode, currentLight.id, false);
  }
  refreshBody();
}


void drawAllLights(int, int)
{
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawAllLightsInBehavior");
  currentRoomID = drawAllLightsID;
  drawHeader("ALLE LAMPER", &drawAllLights);
  drawText(14, 33, 0, "VÆLG LAMPER", EPD_BLACK);
  if (allLightsPagination)
  {
    topIndex = 4;
    drawPaginationButtons(entertainmentRoom.lightCounter);
    drawSelectableLightsFromArray(0, topIndex);
  }
  else
  {
    drawSelectableLightsFromArray(0, entertainmentRoom.lightCounter);
  }
  refreshBody();
}

void drawHome(int, int)
{
  _display = getDisplay();
  firebaseAppendUserAction(touchScreenInteraction, navigate, "drawHome");
  if (uiLevel == 1)
  {
    currentScreenType = "HOME";
    currentRoomID = 1;
    currentRoom = rooms[currentRoomID];
    drawHeader("HJEM", &drawHome);
    drawText(_display->width() / 2, 50, 1, "JEG ER I", EPD_BLACK, true);
    drawText(_display->width() / 2, 100, 1, currentRoom.name, EPD_BLACK, true);
  }
  else if (uiLevel >= 2)
  {
    currentScreenType = "HOME";
    currentRoomID = drawHomeID;
    drawHeader("HJEM", &drawHome);
    std::string statusText = "";
    if (currentRoom.name == "")
    {
      statusText = "VÆLG ET RUM";
    }
    else
    {
      statusText = "JEG ER I " + currentRoom.name;
    }
    drawText(20, 33, 1, statusText, EPD_BLACK);
    drawBitmap(30, 56, 0, 0, NULL, allRoomsBitmap, allRoomsBitmapWidth, allRoomsBitmapHeight, "ALLE RUM", &drawAllGroups, NULL, NULL);
    if (uiLevel == 3)
    {
      drawBitmap(30 + 80, 56, 0, 0, NULL, behaviorBitmap, behaviorBitmapWidth, behaviorBitmapWidth, "OPFØRSEL", &drawAllBehaviors, NULL, NULL);
    }
  }
  refreshBody();
}