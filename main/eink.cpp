/**
 * This is a demo to be used with Good Display 2.7 touch epaper 
 * http://www.e-paper-display.com/products_detail/productId=406.html 264*176 px monochome epaper
 * 
 * The difference with the demo-touch.cpp demo is that:
 * 
 * In this demo Epd class gdew027w3T is used.
 * This class expects EpdSPI and FT6X36 to be injected. Meaning that then touch methods
 * can triggered directly from gdew027w3T class and also that would be automatic rotation aware
 */
#include <stdio.h>
#include "FT6X36.h"
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
#include "pages.h"


FT6X36 ts(13);
EpdSpi io;
Gdew027w3T display(io, ts);

Hue *_hue;

TaskHandle_t taskHandlerFirebase;

std::__cxx11::string prevHeaderText = "";
// Some GFX constants
uint16_t blockWidth = 42;
uint16_t blockHeight = display.height() / 4;
uint16_t lineSpacing = 18;
uint16_t circleColor = EPD_BLACK;
uint16_t circleRadio = 10;
uint16_t selectTextColor = EPD_WHITE;
uint16_t selectBackground = EPD_BLACK;
uint16_t tX = 0;
uint16_t tY = 0;
template <typename T>
static inline void
swap(T &a, T &b)
{
  T t = a;
  a = b;
  b = t;
}
uint8_t display_rotation = 0;


#include <Fonts/ubuntu/Ubuntu_M12pt8b.h>
#include <Fonts/ubuntu/Ubuntu_M8pt8b.h>
#include <Fonts/ubuntu/Ubuntu_M4pt7b.h>
#include <Fonts/ubuntu/Ubuntu_M6pt7b.h>
#include <Fonts/ubuntu/UbuntuMono_Bold6pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/IBMPlexMono-M6Pt.h>
const GFXfont *globalFont = &IBMPlexMono_M6pt7b;

// Battery constants
const uint8_t batteryPositionX = display.width() - batteryWidth - 5;
const uint8_t batteryPositionY = 6;
const float batteyMaxVoltage = 4.2;
const float batteryLowVoltage = 3.2;
float batteryVoltage;
TaskHandle_t taskHandlerBattery;

/* A button represents a pressable icon on the screen. 
   A button can represent a light or a lightgroup.  */
struct Button;

// Button vars
Button buttons[30];
int16_t buttonCount = 0;
Button pressedButton;
uint8_t topIndex;

// Room vars
Room currentRoom;
int lastRoom;
uint8_t roomsCount = 0;
Room rooms[50] = {};
bool roomPagination;

// Light vars
Light lights[50];
uint8_t lightCount = 0;
Light lightIds[50];
uint8_t allLightCount = 0;
bool allLightsPagination;
Light currentLight;

// UI Level
int uiLevel = 3;

// Behavior vars
Behavior behaviors[20];
Behavior lastBehavior;
uint8_t lastBehaviorId;
TaskHandle_t taskHandlerBehavior;
uint8_t behaviorCount;
int blinkingLightsEntries = 100;
int blinkingLightsDelay = 1000;
bool firstTimeSetup = false;
int alarmEntries = 10;
int alarmDelay = 60000;
int festEntries = 100;
int festDelay = 10;
Room entertainmentRoom;
Light entertainmentLights[20];

// Navigation vars
void (*prevPage)(int bulb, int id);
std::__cxx11::string currentPageOnScreen;
int pagePath[10];
int pagePageCounter = 0;
boolean isGoingBack = false;

// Page ids to navigate
const int drawHomeID = 0;
const int drawAllRoomsID = 1;
const int drawRoomID = 2;
const int drawLightID = 3;
const int drawAllLightsID = 4;
const int drawAllBehaviorID = 5;
const int drawBehaviorID = 6;
const int changeBehaviorID = 7;
int currentRoomID = drawHomeID;

// Task handler for Encoder tasks
TaskHandle_t taskHandlEncoder;

// Data structure to parse to encoder tasks.
String currentScreenType;



boolean RGBMode = false;

// Declare functions
void drawRectAndUpdate(int x, int y, int width, int height, uint16_t color);
void fetchData(int, int);
void drawLightsFromArray(int, int);
void drawPaginationButtons(uint8_t counter);

void clearButtons()
{
  memset(buttons, 0, sizeof(buttons));
}

Gdew027w3T* getDisplay(){
  return &display;
}

void addButtonToArray(int16_t x, int16_t y, int16_t width, int16_t height, void (*event)(int bulb, int id), uint16_t bulbId, std::string bulbName, boolean on = false, int16_t brigthness = 0, bool isSelected = false)
{
  Button button;
  button.x = x;
  button.y = y;
  button.width = width;
  button.height = height;
  button.event = event;
  button.on = on;
  button.id = bulbId;
  button.brightness = brigthness;
  button.isSelected = isSelected;
  if (bulbName != "")
    button.name = bulbName;
  Serial.println("ButtonCount: " + String(buttonCount));
  buttons[buttonCount] = button;
  buttonCount++;
}

void drawSquareWithBitmapInside(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, boolean bordered, uint16_t borderColor, const uint8_t bitmap[], int16_t widthOfBitmap, int16_t heightOfBitmap, void (*event)(int bulb, int id), uint16_t bulbId)
{
  display.fillRoundRect(x, y, width, height, radius, color);
  if (bordered == true)
  {
    display.drawRoundRect(x, y, width, height, radius, borderColor);
  }
  uint16_t textColor = EPD_BLACK;
  if (color == EPD_BLACK)
  {
    textColor = EPD_WHITE;
  }
  display.setTextColor(textColor);
  display.setFont(globalFont);
  display.drawBitmap(x + width / 2 - widthOfBitmap / 2, y + height / 2 - heightOfBitmap / 2, bitmap, widthOfBitmap, heightOfBitmap, EPD_WHITE);
  if (event)
    addButtonToArray(x, y, width, height, event, bulbId, " ");
}

void drawSquare(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, const std::string &text, boolean bordered, uint16_t borderColor, void (*event)(int bulb, int id), uint16_t bulbId)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.fillRoundRect(x, y, width, height, radius, color);
  if (bordered == true)
  {
    display.drawRoundRect(x, y, width, height, radius, borderColor);
  }
  uint16_t textColor = EPD_BLACK;
  if (color == EPD_BLACK)
  {
    textColor = EPD_WHITE;
  }
  display.setTextColor(textColor);
  display.setFont(globalFont);
  display.getTextBounds(&text[0], x, y, &x1, &y1, &w, &h);
  display.setCursor(x + width / 2 - w / 2, y + height / 2 + 5); // Set text in the middle of square
  display.print(text);
  if (event)
    addButtonToArray(x, y, width, height, event, bulbId, text);
}

void drawBitmap(int16_t x, int16_t y, int16_t xTouchPadding, int16_t yTouchPadding, const uint8_t onBitmap[], const uint8_t offBitMap[], int16_t widthOfBitmap, int16_t heightOfBitmap, const std::string &text, void (*event)(int bulb, int id), uint16_t bulbId, boolean center, boolean on, int16_t brigthness, boolean border)
{
  int16_t x1, y1;
  uint16_t w, h;
  if (center)
  {
    x = x - widthOfBitmap / 2;
    y = y - heightOfBitmap / 2;
  }
  if (on == true)
  {
    display.drawBitmap(x, y, onBitmap, widthOfBitmap, heightOfBitmap, EPD_WHITE);
  }
  else
  {
    display.drawBitmap(x, y, offBitMap, widthOfBitmap, heightOfBitmap, EPD_WHITE);
  }
  if (border)
  {
    display.drawRect(x, y, widthOfBitmap, heightOfBitmap, EPD_BLACK);
  }
  display.setTextColor(EPD_BLACK);
  display.setFont(globalFont);
  display.getTextBounds(&text[0], x, y + heightOfBitmap, &x1, &y1, &w, &h);
  display.setCursor(x + widthOfBitmap / 2 - w / 2, y + heightOfBitmap + h / 2 + 5); // Set text in the middle of square
  display.print(text);
  if (event)
    addButtonToArray(x - xTouchPadding / 2, y - yTouchPadding / 2, widthOfBitmap + xTouchPadding, heightOfBitmap + yTouchPadding, event, bulbId, text, on, brigthness, border);
}

using namespace std;
boolean onTouchEvent(int16_t x, int16_t y)
{
  for (int i = 0; i < buttonCount; i++)
  {
    if (buttons[i].x < x && x < buttons[i].width + buttons[i].x && buttons[i].y < y && y < buttons[i].y + buttons[i].height)
    {
      printf("Button pressed. %d\n", i);
      printf("Bulb name %s\n", buttons[i].name.c_str());
      pressedButton = buttons[i];
      buttons[i].event(buttons[i].id, i);
      return true;
    }
  }
  return false;
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  display.fillRect(x, y, w, h, color);
  display.updateWindow(x, y, w, h);
}

void setCurrentLight(uint8_t id)
{
  for (int i = 0; i < allLightCount; i++)
  {
    if (id == lightIds[i].id)
    {
      currentLight = lightIds[i];
    }
  }
  Serial.println("Current light set to ID: " + String(currentLight.id));
}

void drawText(int16_t x, int16_t y, int16_t size, const std::__cxx11::string &text, uint16_t textColor, boolean center)
{
  display.setFont(globalFont);
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextColor(textColor);
  display.setTextSize(size);
  display.getTextBounds(&text[0], x, y, &x1, &y1, &w, &h);
  if (center) // Set text in the middle of x coordinate
    x = x - w / 2;
  display.setCursor(x, y);
  display.println(text);
  if (center)
    display.updateWindow(x - w / 2 - 5, y - h / 2 - 5, w * 2 + 5, h + 5);
  else
    display.updateWindow(x - 5, y - h / 2 - 5, w + 10, h + 5);
}

void drawBattery(void *pvParameters)
{
  for (;;)
  {
    printf("running... \n");
    float newBatteryVoltage = ReadBatteryVoltage();
    if (newBatteryVoltage != batteryVoltage)
    {
      drawBitmap(batteryPositionX, batteryPositionY, 0, 0, NULL, batteryBitMap, batteryWidth, batteryHeight, "", NULL, NULL);
      batteryVoltage = ReadBatteryVoltage();
      float batteryPercentage = 0;
      if (batteryVoltage > 4.3)
      {
        drawBitmap(batteryPositionX, batteryPositionY, 0, 0, NULL, batteryChargeBitMap, batteryChargeWidth, batteryChargeHeight, "", NULL, NULL);
        //batteryPercentage = 100;
      }
      else
      {
        batteryPercentage = 100 * (batteryVoltage - batteryLowVoltage) / (batteyMaxVoltage - batteryLowVoltage);
      }
      float bars = 12.0 / 100.0 * batteryPercentage;
      for (int i = 0; i < bars; i++)
      {
        for (int y = 0; y < 6; y++)
        {
          display.drawPixel(batteryPositionX + 1 + i, batteryPositionY + 1 + y, EPD_BLACK);
        }
      }
      display.updateWindow(batteryPositionX - 2, batteryPositionY - 2, batteryWidth + 2, batteryHeight + 2);
    }
    vTaskDelay(200000);
  }
} // Start: (8,10) (8,19) Slut: (33,10) (33,19) ned af - 25 rækker = 100%

void drawCircle(int16_t x, int16_t y, uint16_t circleRadio, uint16_t circleColor, boolean selected, boolean withText, const std::__cxx11::string &text, void (*event)(int bulb, int id), uint16_t bulbId, boolean on)
{
  printf("Draw a %d px circle\n", circleRadio);
  // Print a small circle in selected color
  if (!selected)
  {
    display.fillCircle(x, y, circleRadio, EPD_BLACK);
    display.fillCircle(x, y, circleRadio - 3, EPD_WHITE);
    if (withText)
    {
      drawText(x, y, 1, text, EPD_BLACK, true);
    }
  }
  else
  {
    display.fillCircle(x, y, circleRadio, circleColor);
    if (withText)
    {
      drawText(x, y, 1, text, EPD_WHITE, true);
    }
  }

  if (event)
    addButtonToArray(x - circleRadio, y - circleRadio, circleRadio * 2, circleRadio * 2, event, bulbId, text, on);
}

void drawLine(int16_t x0, int16_t y0, int16_t x, int16_t y, uint16_t color)
{
  display.drawLine(x0, y0, x, y, color);
}

void Switch(int bulb, int id)
{
  //Serial.println("ON? :" + String(buttons[id].on));
  //if (buttons[id].on == true)
  if (currentLight.on == true)
  { // Turn off
    display.drawBitmap(buttons[2].x, buttons[2].y, bulbBitmap, bulbWidth, bulbHeight, EPD_WHITE);
    display.updateWindow(buttons[2].x, buttons[2].y, bulbWidth, bulbHeight);
    _hue->lightOff(currentLight.id);
    currentLight.on = false;
    firebaseAppendUserAction(touchScreenInteraction, "lightOff", "Switch", buttons[2].name.c_str());
  }
  else
  { // Turn on
    display.drawBitmap(buttons[2].x, buttons[2].y, bulbOutlinedBitmap, bulbWidth, bulbHeight, EPD_WHITE);
    display.updateWindow(buttons[2].x, buttons[2].y, bulbWidth, bulbHeight);
    _hue->lightOn(currentLight.id);
    currentLight.on = true;
    firebaseAppendUserAction(touchScreenInteraction, "lightOn", "Switch", buttons[2].name.c_str());
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

void switchGroup(int bulb)
{
  Serial.print("Switch bulb: ");
  Serial.println(bulb);
  Button button = buttons[bulb];
  if (button.on == true)
  { // Turn off
    button.on = false;
    display.drawBitmap(button.x, button.y, bulbBitmap, bulbWidth, bulbHeight, EPD_WHITE);
    display.updateWindow(button.x, button.y, bulbWidth, bulbHeight);
    _hue->groupOff(button.id);
  }
  else
  { // Turn on
    button.on = true;
    display.drawBitmap(button.x, button.y, bulbOutlinedBitmap, bulbWidth, bulbHeight, EPD_WHITE);
    display.updateWindow(button.x, button.y, bulbWidth, bulbHeight);
    _hue->groupOn(button.id);
  }
  buttons[bulb] = button;
}

void refreshBody()
{
  display.updateWindow(0, 19, 175, 225);
}

void goBack(int, int)
{
  int pageID = pagePath[pagePageCounter - 2];
  pagePageCounter--;
  isGoingBack = true;
  if (pageID == drawHomeID)
  {
    drawHome(0, 0);
  }
  else if (pageID == drawAllRoomsID)
  {
    drawAllGroups(0, 0);
  }
  else if (pageID == drawRoomID)
  {
    drawRoom(lastRoom, 0);
  }
  else if (pageID == drawLightID)
  {
    drawLight(0, 0);
  }
  else if (pageID == drawAllLightsID)
  {
    drawAllLights(0, 0);
  }
  else if (pageID == drawAllBehaviorID)
  {
    drawAllBehaviors(0, 0);
  }
  else if (pageID == drawBehaviorID)
  {
    drawBehavior(lastBehaviorId, 0);
  }
}

void addPageToPagePath()
{
  if (isGoingBack)
  {
    isGoingBack = false;
    return;
  }
  Serial.println("Adding page to path: " + String(currentRoomID));
  pagePath[pagePageCounter] = currentRoomID;
  pagePageCounter++;
  isGoingBack = false;
}

void drawHomeFromHomeButton(int, int)
{
  pagePageCounter = 0;
  drawHome(0, 0);
}


// This draws the header and navbar in each page and handles various logic
void drawHeader(const std::__cxx11::string &text, void (*currentPage)(int bulb, int id), boolean persistButtons)
{
  if (!persistButtons)
  {
    buttonCount = 0;
    clearButtons();
  }
  addPageToPagePath();
  currentPageOnScreen = text;
  drawLine(0, 18, display.width(), 18, EPD_BLACK);
  drawText(display.width() / 2, 12, 1, prevHeaderText, EPD_WHITE, true);
  drawText(display.width() / 2, 12, 1, text, EPD_BLACK, true);
  display.updateWindow(1, 1, 175, 19);
  prevHeaderText = text;
  fillRect(0, 19, 175, 264 - 40, EPD_WHITE);
  printf("width: %d height: %d\n", display.width(), display.height());
  if (text != "HJEM")
  {
    drawBitmap(display.width() / 3 / 2, 238 + 26 / 2, display.width() / 3 - backArrowBitmapWidth, 26 - backArrowBitmapHeight, NULL, backArrowBitmap, backArrowBitmapWidth, backArrowBitmapHeight, " ", &goBack, 12, true);
    drawBitmap(display.width() / 2, 238 + 26 / 2, display.width() / 3 - homeBitmapWidth, 26 - homeBitmapHeight, NULL, homeBitmap, homeBitmapWidth, homeBitmapHeight, " ", &drawHomeFromHomeButton, 12, true);
  }
  else
  {
    drawBitmap(display.width() / 3 / 2, 238 + 26 / 2, display.width() / 3 - refreshBitmapWidth, 26 - refreshBitmapHeight, NULL, refreshBitmap, refreshBitmapWidth, refreshBitmapHeight, " ", &fetchData, 12, true);
    drawBitmap(display.width() / 2, 238 + 26 / 2, display.width() / 3 - homeBitmapWidth, 26 - homeBitmapHeight, NULL, homeBitmap, homeBitmapWidth, homeBitmapHeight, " ", &drawHomeFromHomeButton, 12, true);
  }
  display.updateWindow(1, 238, display.width() - 2, 23);
  prevPage = currentPage;
}


void drawLightsFromArray(int start, int end)
{
  int x = 22;
  int y = 56;
  topIndex = end;
  if (end > lightCount)
  {
    end = lightCount;
  }
  for (int i = start; i < end; i++)
  {
    if (lights[i].name == "")
      break;
    drawBitmap(x, y, 0, 0, bulbOutlinedBitmap, bulbBitmap, bulbWidth, bulbHeight, lights[i].name, &drawLight, lights[i].id, false, lights[i].on, lights[i].brightness);
    if (i % 2 == 1)
    {
      y += 85;
      x = 22;
    }
    else
    {
      x += 85;
    }
  }
}

void drawRectAndUpdate(int x, int y, int width, int height, uint16_t color)
{
  display.drawRect(x, y, width, height, color);
  display.updateWindow(x - 2, y - 2, width + 2, height + 2);
}

void arrayShift(uint8_t arr[], uint8_t length, int value)
{
  for (int i = 0; i < length; i++)
  {
    if (arr[i] == value)
    {
      for (int k = i; k < length - 1; k++)
      {
        arr[k] = arr[k + 1];
      }

      arr[length - 1] = 0;
      i--;
    }
  }
}

void selectLight(int lightId, int buttonId)
{
  if (buttons[buttonId].isSelected)
  { // Remove light from selected
    arrayShift(behaviors[lastBehaviorId].lightIds, behaviors[lastBehaviorId].lightCounter, lightId);
    behaviors[lastBehaviorId].lightCounter--;
    buttons[buttonId].isSelected = false;
    _hue->flash(lightId);
    drawRectAndUpdate(buttons[buttonId].x, buttons[buttonId].y, buttons[buttonId].width, buttons[buttonId].height, EPD_WHITE);
    firebaseAppendUserAction(touchScreenInteraction, "removeBehaviorLight", "selectLight", buttons[buttonId].name.c_str());
  }
  else
  { // Add light to selected
    behaviors[lastBehaviorId].lightIds[behaviors[lastBehaviorId].lightCounter] = lightId;
    behaviors[lastBehaviorId].lightCounter++;
    buttons[buttonId].isSelected = true;
    _hue->flash(lightId);
    drawRectAndUpdate(buttons[buttonId].x, buttons[buttonId].y, buttons[buttonId].width, buttons[buttonId].height, EPD_BLACK);
    firebaseAppendUserAction(touchScreenInteraction, "adddedBehaviorLight", "selectLight", buttons[buttonId].name.c_str());
  }
}

void drawSelectableLightsFromArray(int start, int end)
{
  int x = 22;
  int y = 56;
  topIndex = end;
  for (int i = start; i < end; i++)
  {
    if (entertainmentLights[i].name == "")
      break;
    bool isSelected = false;
    for (int p = 0; p < behaviors[lastBehaviorId].lightCounter; p++)
    {
      if (behaviors[lastBehaviorId].lightIds[p] == entertainmentLights[i].id)
      {
        isSelected = true;
      }
    }
    drawBitmap(x, y, 0, 0, bulbOutlinedBitmap, bulbBitmap, bulbWidth, bulbHeight, entertainmentLights[i].name, &selectLight, entertainmentLights[i].id, false, entertainmentLights[i].on, entertainmentLights[i].brightness, isSelected);
    if (i % 2 == 1)
    {
      y += 85;
      x = 22;
    }
    else
    {
      x += 85;
    }
  }
}

void drawRoomsFromArray(int start, int end)
{
  int x = 22;
  int y = 56;
  topIndex = end;
  for (int i = start; i < end; i++)
  {
    Room room = rooms[i];
    if (room.name == "")
      break;
    drawBitmap(x, y, 0, 0, NULL, allRoomsBitmap, allRoomsBitmapWidth, allRoomsBitmapHeight, room.name, &drawRoom, i, false);
    if (i % 2 == 1)
    {
      y += 85;
      x = 22;
    }
    else
    {
      x += 85;
    }
  }
}

void refreshPagination()
{
  display.updateWindow(0, 40, 175, 170);
}

void drawNextPage(int, int)
{
  fillRect(1, 40, 175, 223, EPD_WHITE);
  buttonCount = 3;
  if (currentPageOnScreen.compare("ALLE RUM") == 0)
  {
    drawRoomsFromArray(topIndex, topIndex + 4);
    drawPaginationButtons(roomsCount);
  }
  else if (currentPageOnScreen.compare("RUM") == 0)
  {
    drawLightsFromArray(topIndex, topIndex + 4);
    drawPaginationButtons(lightCount);
  }
  else if (currentPageOnScreen.compare("ALLE LAMPER") == 0)
  {
    drawSelectableLightsFromArray(topIndex, topIndex + 4);
    drawPaginationButtons(allLightCount);
  }
  refreshBody();
}

void drawPrevPage(int, int)
{
  fillRect(1, 40, 175, 223, EPD_WHITE);
  buttonCount = 3;
  if (currentPageOnScreen.compare("ALLE RUM") == 0)
  {
    drawRoomsFromArray(topIndex - 8, topIndex - 4);
    drawPaginationButtons(roomsCount);
  }
  else if (currentPageOnScreen.compare("RUM") == 0)
  {
    drawLightsFromArray(topIndex - 8, topIndex - 4);
    drawPaginationButtons(lightCount);
  }
  else if (currentPageOnScreen.compare("ALLE LAMPER") == 0)
  {
    drawSelectableLightsFromArray(topIndex - 8, topIndex - 4);
    drawPaginationButtons(allLightCount);
  }
  display.updateWindow(1, 40, 175, 223);
  refreshBody();
}

void drawPaginationButtons(uint8_t counter)
{
  buttonCount = 7;
  if (topIndex != 4)
  {
    drawSquareWithBitmapInside(29, 212, 55, 20, 0, EPD_WHITE, true, EPD_BLACK, leftArrowBitmap, leftArrowBitmapWidth, leftArrowBitmapHeight, &drawPrevPage, 1);
  }
  else
  {
    fillRect(29, 212, 55, 20, EPD_BLACK);
  }
  if (topIndex < counter)
  {
    drawSquareWithBitmapInside(55 + 29, 212, 55, 20, 0, EPD_WHITE, true, EPD_BLACK, rightArrowBitmap, rightArrowBitmapWidth, rightArrowBitmapHeight, &drawNextPage, 1);
  }
  else
  {
    fillRect(55 + 29, 212, 55, 20, EPD_BLACK);
  }
}

void toggleBehavior(int id, int buttonId)
{
  Button button = pressedButton;
  Behavior &behavior = behaviors[id];
  Serial.println("id: " + String(id));
  Serial.println("buttonId: " + String(buttonId));
  if (buttonId == 5)
  {
    if (behavior.activated)
    {
      if (currentPageOnScreen.compare("drawBehavior") == 0)
      {
        drawSquare(14, 184, 74, 40, 0, EPD_WHITE, "START", true, EPD_BLACK, NULL, NULL);
        drawSquare(14 + 74, 184, 74, 40, 0, EPD_BLACK, "STOP", true, EPD_BLACK, NULL, NULL);
      }
      behavior.activated = false;
      startBehaviorNow = false;
      fest = false;
      begin();
      firebaseAppendUserAction(touchScreenInteraction, behaviorOff, "toggleBehavior", behavior.name);
      display.updateWindow(14, 184, 74 + 74, 40);
      disconnect();
    }
  }
  else
  {
    if (behavior.lightCounter != 0)
    {
      runTaskFireBaseAppendUserActionWithWhat(touchScreenInteraction, behaviorOn, "toggleBehavior", behavior.name);
      Serial.println("Start Behavior!");
      firebaseEnd();
      drawSquare(14, 184, 74, 40, 0, EPD_BLACK, "START", true, EPD_BLACK, NULL, NULL);
      drawSquare(14 + 74, 184, 74, 40, 0, EPD_WHITE, "STOP", true, EPD_BLACK, NULL, NULL);
      behavior.activated = true;
      display.updateWindow(14, 184, 74 + 74, 40);
      activateBehavior();
    }
  }
}

void getBehaviorType(uint8_t behaviorId, std::string &behaviorType)
{
  if (behaviorId == timeBehavior)
  {
    behaviorType = "TIDSBASERET";
    return;
  }
  if (behaviorId == repeatedBehavior)
  {
    behaviorType = "GENTAGELSES";
    return;
  }
  if (behaviorId == transitionBehavior)
  {
    behaviorType = "OVERGANGS";
    return;
  }
}

void changeBehaviorLights(int id, int buttonId)
{
  Behavior &behavior = behaviors[id];
}

TaskHandle_t taskHandlerbehaviorUpdater;

void updateBehaviors(void *xStruct)
{
  while (lastBehavior.activated)
  {
    delay(5000);
  }
  if (lastBehavior.behaviorId == timeBehavior)
  {
    behaviors[1].entriesCount = alarmEntries;
    updateAlarmLightsEntries();
  }
  else if (lastBehavior.behaviorId == repeatedBehavior)
  {
    behaviors[0].entriesCount = blinkingLightsEntries;
    updateBlinkingLightsEntries();
  }
  else if (lastBehavior.behaviorId == transitionBehavior)
  {
    behaviors[2].entriesCount = blinkingLightsEntries;
  }
  vTaskDelete(taskHandlerbehaviorUpdater);
}

void toggleRGBMode(int, int)
{
  Button button = pressedButton;

  if (pressedButton.on)
  {
    RGBMode = true;
    printf("RGB Mode toggled to %s \n", RGBMode ? "true" : "false");
    firebaseAppendUserAction(touchScreenInteraction, rgbSwitch, "toggleRGBMode", "From Brightness to RGB");
    drawCircle(14 + 35, 152 + 35, 35, EPD_BLACK, true, true, "FARVE");
    drawCircle(127, 152 + 35, 35, EPD_BLACK, false, true, "TEMP");
    display.updateWindow(14, 152, 150, 70);
  }
  else
  {
    RGBMode = false;
    printf("RGB Mode toggled to %s \n", RGBMode ? "true" : "false");
    firebaseAppendUserAction(touchScreenInteraction, rgbSwitch, "toggleRGBMode", "From RGB to Brightness");
    fillRect(14 + 35, 152 + 35, 150, 70, EPD_WHITE);
    drawCircle(14 + 35, 152 + 35, 35, EPD_BLACK, false, true, "FARVE");
    drawCircle(127, 152 + 35, 35, EPD_BLACK, true, true, "TEMP");
    display.updateWindow(14, 152, 150, 70);
  }
}

void drawBar(int16_t x, int16_t y, uint8_t value)
{
  uint8_t percentage = map(value, 1, 254, 0, 100);
  printf("Percentage %d \n", percentage);

  uint8_t length = map(percentage, 0, 100, 0, display.width() - 28);

  printf("Length: %d \n", length);

  drawLine(x, y - 2, x, y + 7);
  drawLine(x + display.width() - 28, y - 2, x + display.width() - 28, y + 7);
  drawLine(x, y + 2, x + display.width() - 28, y + 2);
  drawSquare(x, y, length, 5, 0, EPD_BLACK, "", false, EPD_BLACK, NULL, NULL);
}

void updateBar(int16_t x, int16_t y, uint8_t value)
{
  fillRect(x, y - 2, display.width(), 15, EPD_WHITE);
  drawBar(x, y, value);
  display.updateWindow(x, y - 2, 150, 15);
}


void touchEvent(TPoint p, TEvent e)
{
  #if defined(DEBUG_COUNT_TOUCH) && DEBUG_COUNT_TOUCH == 1
  ++t_counter;
  ets_printf("e %x %d  ", e, t_counter); // Working
#endif

  if (e != TEvent::Tap && e != TEvent::DragStart && e != TEvent::DragMove && e != TEvent::DragEnd)
    return;

  std::string eventName = "";
  switch (e)
  {
  case TEvent::Tap:
    eventName = "tap";
    break;
  case TEvent::DragStart:
    eventName = "DragStart";
    return;
    break;
  case TEvent::DragMove:
    eventName = "DragMove";
    return;
    break;
  case TEvent::DragEnd:
    eventName = "DragEnd";
    return;
    break;
  default:
    eventName = "UNKNOWN";
    return;
    break;
  }

  //printf("X: %d Y: %d E: %s\n", p.x, p.y, eventName.c_str());
  if (onTouchEvent(p.x, p.y) == false)
  {
    //drawCircle(p.x, p.y, NULL);
  }
}

void fetchData(int, int)
{
  drawText(display.width() / 2, display.height() / 2, 1, "OPDATERER", EPD_BLACK, true);
  _hue->_discoverBridges();
  _hue->getNewIds();
  _hue->getGroup(ENTERTAINMENT_ID, entertainmentRoom);
  _hue->getLightStates(entertainmentLights, entertainmentRoom.lightIds, entertainmentRoom.lightCounter);
  _hue->getGroups(rooms, roomPagination, roomsCount);
  _hue->getLightIds(lightIds, allLightsPagination, allLightCount);
  firebaseEnd();
  begin();
  initBehaviors(*_hue, entertainmentLights);
  firstTimeSetup = false;
  getUILevel(uiLevel);
  drawHome(0, 0);
  display.update();
}

void initEink(Hue &hue)
{
  _hue = &hue;
  printf("CalEPD version: %s\n", CALEPD_VERSION);
  display.init(false);
  display.displayRotation(2);

  printf("display.colors_supported:%d display.rotation: %d\n", display.colors_supported, display_rotation);

  // Start encoder task
  xTaskCreate(&encoderLoop, "encoder_task", 4096, (void *)1, 3, &taskHandlEncoder);
  xTaskCreate(&drawBattery, "battery_task", 2048, (void *)1, 0, &taskHandlerBattery);
  xTaskCreate(&firebaseTask, "firebase_task", 10240, (void *)1, 1, &taskHandlerFirebase);
  xTaskCreate(&startBehavior, "behavior_task", 8192, (void *)1, 1, &taskHandlerBehavior);

  //_hue->createEntertainmentGroupWithAllLights(lightIds); // Do it manually, as we can't account for bulbs that do not support entertainment API

  fetchData(0, 0);

  display.registerTouchHandler(touchEvent);
}

void loopEink()
{
  display.touchLoop();
}