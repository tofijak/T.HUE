#include "encoder.h"
#include "eink.h"
#include "hue.h"
#include "RotaryEncoder.h"
#include "Firebase.h"

Hue *_hue2;

// Set pins for encoders
RotaryEncoder encoder(32, 33);
RotaryEncoder encoderCont(25, 26);

// Set pins for encoders' switches
#define encoderBtn 13

String currentScreen;

bool calledFromEncoder = false;
int firebaseBrightenssLogCooldown;

int increaseBrightnessArr[24] = {0, 15, 35, 54, 72, 90, 106, 122, 136, 150, 163, 175, 186, 196, 205, 214, 221, 228, 234, 239, 242, 246, 248, 254};
int decreaseBrightnessArr[19] = {0, 6, 10, 14, 16, 18, 23, 31, 42, 55, 71, 89, 109, 132, 158, 186, 217, 250, 254};

int getNearest(int x, int y, int target)
{
    if (target - x >= y - target)
        return y;
    else
        return x;
}

int getNearestElementInSortedArray(int arr[], int n, int target, bool increase)
{
    for (int i = 0; i < n - 1; i++)
    {
        if (increase)
        {
            if (arr[i] <= target && target < arr[i + 1])
            {
                if (i + 1 == n)
                {
                    return arr[i];
                }
                return arr[i + 1];
            }
        }
        else
        {
            if (arr[i] < target && target <= arr[i + 1])
            {
                return arr[i];
            }
        }
    }
    return 0;
}

void calcBrightness(uint16_t lightId, int16_t &currentBrightness, bool increase)
{
    Serial.println("currentBrightness: " + String(currentBrightness));
    if (increase)
    {
        int nearestIncElement = getNearestElementInSortedArray(increaseBrightnessArr, 24, currentBrightness, increase);
        Serial.println("NEAREST Inc: " + String(nearestIncElement));
        if(nearestIncElement == 0){
            nearestIncElement = 254;
        }
        currentBrightness = nearestIncElement;
    }
    else
    {
        int nearestDecElement = getNearestElementInSortedArray(decreaseBrightnessArr, 19, currentBrightness, increase);
        Serial.println("NEAREST Dec: " + String(nearestDecElement));
        currentBrightness = nearestDecElement;
    }
}

void initEncoders(Hue &hue)
{
    _hue2 = &hue;

    encoder.setPosition(0);
    encoderCont.setPosition(0);
}

void chooseLightInRoom()
{
    uint8_t counter = 0;
    uint8_t currentLightId = currentRoom.lightIds[counter];

    pinMode(encoderBtn, INPUT_PULLUP);

    while (currentScreen == "ROOM")
    {
        encoder.tick();

        int directionStep = (int)encoder.getDirection();

        if (directionStep == 1)
        {
            counter++;
            if (counter > currentRoom.lightCounter)
            {
                counter = 0;
            }

            printf("Counter %d \n", counter);
            printf("Light ID %d \n", currentRoom.lightIds[counter]);

            if (counter == currentRoom.lightCounter)
            {
                _hue2->groupFlash(currentRoom.id);
            }
            else
            {
                _hue2->flash(currentRoom.lightIds[counter]);
                currentLightId = currentRoom.lightIds[counter];
            }
        }
        else if (directionStep == -1)
        {
            counter--;
            if (counter > currentRoom.lightCounter)
            {
                counter = currentRoom.lightCounter;
            }

            printf("Counter %d \n", counter);
            printf("Light ID %d \n", currentRoom.lightIds[counter]);

            if (counter == currentRoom.lightCounter)
            {
                _hue2->groupFlash(currentRoom.id);
            }
            else
            {
                _hue2->flash(currentRoom.lightIds[counter]);
                currentLightId = currentRoom.lightIds[counter];
            }
        }
        currentScreen = currentScreenType;

        vTaskDelay(1);
    }
}

// Adjust the brigthness and the color temperature or hue for a single light.
void adjustLight()
{
    uint16_t lightId = currentLight.id;
    int16_t lightBrigthness = currentLight.brightness;
    uint16_t buttonID = pressedButton.buttonId;
    boolean RGB = RGBMode;

    uint8_t incrementSize = 20;

    uint8_t hueGranularity = 20;
    uint8_t tempSteps = 20;

    uint8_t hueCounter = 0;
    uint8_t tempCounter = 10;

    printf("Initial Brightness: %d \n", currentLight.brightness);
    printf("Current Light ID: %d \n", lightId);

    pinMode(encoderBtn, INPUT_PULLUP);

    while (currentScreen == "LIGHT")
    {
        RGB = RGBMode;
        encoder.tick();
        encoderCont.tick();

        int btnState = digitalRead(encoderBtn);
        if (btnState == 0)
        {
            printf("Button Pressed!");
            Switch(0, buttonID);

            firebaseAppendUserAction(topDialInteraction, switchLight, "drawLight", currentLight.name.c_str());
        }

        int direction = (int)encoderCont.getDirection();
        int directionStep = (int)encoder.getDirection();

        if (direction == 1)
        {

            lightBrigthness += incrementSize;

            if (lightBrigthness > 254)
            {
                lightBrigthness = 254;
            }
            printf("Current Brightness: %d \n", lightBrigthness);

            currentLight.brightness = lightBrigthness;
            calcBrightness(currentLight.id, currentLight.brightness, true);

            _hue2->brightness(lightId, currentLight.brightness);
            updateBar(14, 110, currentLight.brightness);

            firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawLight", currentLight.name.c_str(), currentLight.brightness);
        };

        if (direction == -1)
        {

            lightBrigthness -= incrementSize;

            if (lightBrigthness < 1)
            {
                lightBrigthness = 1;
            }
            printf("Current Brightness: %d \n", lightBrigthness);

            currentLight.brightness = lightBrigthness;
            calcBrightness(currentLight.id, currentLight.brightness, false);
            _hue2->brightness(lightId, currentLight.brightness);
            updateBar(14, 110, currentLight.brightness);

            firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawLight", currentLight.name.c_str(), currentLight.brightness);
        };

        // Set RGB color
        if (RGB)
        {
            if (directionStep == 1)
            {
                hueCounter++;
                if (hueCounter >= hueGranularity)
                {
                    hueCounter = 0;
                }

                printf("HueCounter: %d \n", hueCounter);
                uint16_t hue = map(hueCounter, 0, hueGranularity, 0, 65535);
                printf("Set hue to: %d \n", hue);
                //_hue2->sat(lightId, 200);
                _hue2->hue(lightId, hue);

                firebaseAppendUserAction(lowerDialInteraction, "setHue", "drawLight", currentLight.name.c_str(), currentLight.hue);
            };
            if (directionStep == -1)
            {
                hueCounter--;
                if (hueCounter == 0)
                {
                    hueCounter = hueGranularity;
                }
                printf("HueCounter: %d \n", hueCounter);
                uint16_t hue = map(hueCounter, 0, hueGranularity, 0, 65535);
                printf("Set hue to: %d \n", hue);
                //_hue2->sat(lightId, 200);
                _hue2->hue(lightId, hue);

                firebaseAppendUserAction(lowerDialInteraction, "setHue", "drawLight", currentLight.name.c_str(), currentLight.hue);
            }
        }
        // Set color temperature
        else
        {
            if (directionStep == 1)
            {
                tempCounter++;
                if (tempCounter > tempSteps)
                {
                    tempCounter = tempSteps;
                }
                printf("TempCounter: %d \n", tempCounter);
                uint16_t temp = map(tempCounter, 0, tempSteps, 6500, 2000);
                printf("Set temperature to: %d \n", temp);
                _hue2->colorTemp(lightId, temp);

                firebaseAppendUserAction(lowerDialInteraction, setColorTemp, "drawLight", currentLight.name.c_str(), currentLight.ct);
            };
            if (directionStep == -1)
            {
                tempCounter--;
                if (tempCounter < 0)
                {
                    tempCounter = 0;
                }
                printf("TempCounter: %d \n", tempCounter);
                uint16_t temp = map(tempCounter, 0, tempSteps, 6500, 2000);
                printf("Set temperature to: %d \n", temp);
                _hue2->colorTemp(lightId, temp);

                firebaseAppendUserAction(lowerDialInteraction, setColorTemp, "drawLight", currentLight.name.c_str(), currentLight.ct);
            }
        }
        currentScreen = currentScreenType;
        vTaskDelay(1);
    }
}

void chooseAndAlterLightInRoom()
{
    uint8_t counter = currentRoom.lightCounter;
    uint8_t currentLightId = currentRoom.lightIds[counter];
    uint8_t roomId = currentRoom.id;

    bool roomState = currentRoom.on;

    int16_t lightBrigthness = 120;
    uint8_t incrementSize = 20;

    int btnState;
    pinMode(encoderBtn, INPUT_PULLUP);

    unsigned long turnMillis;
    unsigned long elapsedMillis = 0;
    unsigned long period = 10000;

    bool dialTurned = false;

    while (currentScreenType == "HOME" || currentScreenType == "ROOMS")
    {
        turnMillis = millis();

        if (turnMillis - elapsedMillis > period && dialTurned == true)
        {
            printf("Time elapsed: %lu \n", turnMillis - elapsedMillis);
            elapsedMillis = turnMillis;
            dialTurned = false;
        }

        if (dialTurned == false)
        {
            elapsedMillis = turnMillis;
        }

        encoder.tick();
        encoderCont.tick();

        int direction = (int)encoder.getDirection();
        int directionCont = (int)encoderCont.getDirection();

        // Switch group on/off
        btnState = digitalRead(encoderBtn);

        if (uiLevel == 1)
        {
            dialTurned = false;
        }

        if (dialTurned && btnState == 0 && counter != currentRoom.lightCounter)
        {
            calledFromEncoder = true;
            firebaseAppendUserAction(topDialInteraction, navigate, "drawLight", currentLight.name.c_str());

            drawLight(currentLightId, 0);
        }

        if (btnState == 0)
        {
            if (roomState == true && counter == currentRoom.lightCounter)
            {
                _hue2->groupOff(currentRoom.id);
                roomState = false;

                vTaskDelay(10);

                if (currentScreenType == "HOME")
                {
                    firebaseAppendUserAction(topDialInteraction, allLightOff, "drawHome", currentRoom.name.c_str());
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, allLightOff, "drawRooms", currentRoom.name.c_str());
                }
            }
            else if (counter == currentRoom.lightCounter)
            {
                _hue2->groupOn(currentRoom.id);
                roomState = true;

                vTaskDelay(10);

                if (currentScreenType == "HOME")
                {
                    firebaseAppendUserAction(topDialInteraction, allLightOn, "drawHome", currentRoom.name.c_str());
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, allLightOn, "drawRooms", currentRoom.name.c_str());
                }
            }
            else if (currentLight.on)
            {
                Serial.println("lightOff");
                _hue2->lightOff(currentLightId);
                currentLight.on = false;
                vTaskDelay(10);
            }
            else
            {

                Serial.println("lightOn");
                _hue2->lightOn(currentLightId);
                currentLight.on = true;
                vTaskDelay(10);
            }
        }

        // Set brightness for chosen light(s) in chosen room
        if (directionCont == 1)
        {
            if (counter != currentRoom.lightCounter)
            {
                calcBrightness(currentLightId, currentLight.brightness, true);
                _hue2->brightness(currentLightId, currentLight.brightness);

                if (currentScreenType == "HOME")
                {

                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawHome", currentLight.name.c_str(), currentLight.brightness);
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawRooms", currentLight.name.c_str(), currentLight.brightness);
                }
            }
            else
            {

                calcBrightness(currentRoom.id, currentRoom.bri, true);
                _hue2->groupBrightness(currentRoom.id, currentRoom.bri);

                if (currentScreenType == "HOME")
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawHome", currentRoom.name.c_str(), currentRoom.bri);
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawRooms", currentRoom.name.c_str(), currentRoom.bri);
                }
            }
        }
        else if (directionCont == -1)
        {
            if (counter != currentRoom.lightCounter)
            {

                calcBrightness(currentLight.id, currentLight.brightness, false);
                _hue2->brightness(currentLightId, currentLight.brightness);

                if (currentScreenType == "HOME")
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawHome", currentLight.name.c_str(), currentLight.brightness);
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawRoom", currentLight.name.c_str(), currentLight.brightness);
                }
            }
            else
            {

                calcBrightness(currentRoom.id, currentRoom.bri, false);
                _hue2->groupBrightness(currentRoom.id, currentRoom.bri);

                if (currentScreenType == "HOME")
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawHome", currentRoom.name.c_str(), currentRoom.bri);
                }
                else
                {
                    firebaseAppendUserAction(topDialInteraction, setBrigthness, "drawRooms", currentRoom.name.c_str(), currentRoom.bri);
                }
            }
        }

        // Choose a specific bulb
        if (direction == 1)
        {
            dialTurned = true;
            elapsedMillis = turnMillis;

            counter++;
            if (counter > currentRoom.lightCounter)
            {
                counter = 0;
            }

            printf("Counter %d \n", counter);
            printf("Light ID %d \n", currentRoom.lightIds[counter]);

            if (counter == currentRoom.lightCounter)
            {
                _hue2->groupFlash(currentRoom.id);
            }
            else
            {
                _hue2->flash(currentRoom.lightIds[counter]);
                currentLightId = currentRoom.lightIds[counter];
                setCurrentLight(currentLightId);
            }
        }
        else if (direction == -1)
        {
            dialTurned = true;
            elapsedMillis = turnMillis;

            counter--;
            if (counter > currentRoom.lightCounter)
            {
                counter = currentRoom.lightCounter;
            }

            printf("Counter %d \n", counter);
            printf("Light ID %d \n", currentRoom.lightIds[counter]);

            if (counter == currentRoom.lightCounter)
            {
                printf("Counter %d \n", counter);
                _hue2->groupFlash(currentRoom.id);
            }
            else
            {
                _hue2->flash(currentRoom.lightIds[counter]);
                currentLightId = currentRoom.lightIds[counter];
                setCurrentLight(currentLightId);
            }
        }
        currentScreen = currentScreenType;

        vTaskDelay(3);
    }
}

void encoderLoop(void *pvParameters)
{
    printf("Encoder loop initiated...");

    for (;;)
    {
        currentScreen = currentScreenType;

        if (currentScreen == "HOME")
        {
            printf("HOME");
            chooseAndAlterLightInRoom();
        }
        else if (currentScreen == "ROOMS")
        {
            printf("ROOMS");
            chooseAndAlterLightInRoom();
        }
        else if (currentScreen == "ROOM")
        {
            printf("ROOM");
            chooseLightInRoom();
        }
        else if (currentScreen == "LIGHT")
        {
            printf("LIGHT");
            adjustLight();
        }
    }
}