#ifndef __eink_h
#define __eink_h

#include "hue.h"
#include "entertainment.h"
#include "gdew027w3T.h"

Gdew027w3T* getDisplay();
void initEink(Hue &hue);
void loopEink();
void drawAllLights(int);
void touchEvent();
void updateBar(int16_t x, int16_t y, uint8_t value);
void Switch(int bulb, int id);
void toggleBehavior(int id, int buttonId);
void drawLight(int lightId, int);
void setCurrentLight(uint8_t id);
void drawHeader(const std::__cxx11::string &text, void (*currentPage)(int bulb, int id), boolean persistButtons = false);
void drawText(int16_t x, int16_t y, int16_t size, const std::__cxx11::string &text, uint16_t textColor, boolean center = false);
void refreshBody();
void switchGroup(int bulb);
void drawLine(int16_t x0, int16_t y0, int16_t x, int16_t y, uint16_t color = EPD_BLACK);
void drawCircle(int16_t x, int16_t y, uint16_t circleRadio = 10, uint16_t circleColor = EPD_BLACK, boolean selected = false, boolean withText = false, const std::__cxx11::string &text = NULL, void (*event)(int bulb, int id) = NULL, uint16_t bulbId = NULL, boolean on = false);
void drawBattery(void *pvParameters);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawBitmap(int16_t x, int16_t y, int16_t xTouchPadding, int16_t yTouchPadding, const uint8_t onBitmap[], const uint8_t offBitMap[], int16_t widthOfBitmap, int16_t heightOfBitmap, const std::string &text, void (*event)(int bulb, int id), uint16_t bulbId, boolean center = false, boolean on = false, int16_t brigthness = 0, boolean border = false);
void drawSquare(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, const std::string &text, boolean bordered, uint16_t borderColor, void (*event)(int bulb, int id), uint16_t bulbId);
void drawSquareWithBitmapInside(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, uint16_t color, boolean bordered, uint16_t borderColor, const uint8_t bitmap[], int16_t widthOfBitmap, int16_t heightOfBitmap, void (*event)(int bulb, int id), uint16_t bulbId);
void drawLightsFromArray(int start, int end);
void drawRectAndUpdate(int x, int y, int width, int height, uint16_t color);
void arrayShift(uint8_t arr[], uint8_t length, int value);
void selectLight(int lightId, int buttonId);
void drawSelectableLightsFromArray(int start, int end);
void drawRoomsFromJsonObject(int start, int end);
void refreshPagination();
void drawNextPage(int, int);
void drawPrevPage(int, int);
void drawPaginationButtons(uint8_t counter);
void toggleBehavior(int id, int buttonId);
void getBehaviorType(uint8_t behaviorId, std::string &behaviorType);
void updateBehaviors(void *xStruct);
void toggleRGBMode(int, int);
void drawBar(int16_t x, int16_t y, uint8_t value);
void drawLight(int lightId, int);
void drawRoomsFromArray(int start, int end);


struct Button
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t on;
    int16_t id;
    uint16_t buttonId;
    int16_t brightness;
    std::string name;
    void (*event)(int bulb, int id);
    bool isSelected;
};

extern std::string currentPageOnScreen;
extern String currentScreenType;
extern boolean RGBMode;
extern Hue *_hue;
extern Button pressedButton;
extern Room currentRoom;
extern uint8_t lastBehaviorId;
extern uint8_t allLightCount;
extern bool allLightsPagination;
extern Light currentLight;
extern int blinkingLightsEntries;
extern int blinkingLightsDelay;
extern bool firstTimeSetup;
extern int alarmEntries;
extern int alarmDelay;
extern int festEntries;
extern int festDelay;
extern int uiLevel;
extern int currentRoomID;
extern int lastRoom;
extern Room rooms[50];
extern Button buttons[30];
extern int16_t buttonCount;
extern Button pressedButton;
extern uint8_t topIndex;
extern Room currentRoom;
extern int lastRoom;
extern uint8_t roomsCount;
extern bool roomPagination;
extern Light lights[50];
extern uint8_t lightCount;
extern Light lightIds[50];
extern uint8_t allLightCount;
extern bool allLightsPagination;
extern Light currentLight;
extern Behavior behaviors[20];
extern Behavior lastBehavior;
extern uint8_t lastBehaviorId;
extern uint8_t behaviorCount;
extern int blinkingLightsEntries;
extern int blinkingLightsDelay;
extern bool firstTimeSetup;
extern int alarmEntries;
extern int alarmDelay;
extern int festEntries;
extern int festDelay;
extern Room entertainmentRoom;
extern Light entertainmentLights[20];
extern String currentScreenType;
extern TaskHandle_t taskHandlerbehaviorUpdater;

#endif