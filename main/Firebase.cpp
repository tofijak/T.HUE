#include <Arduino.h>
#include "FirebaseESP32.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "secret.h"
#include "Firebase.h"

//Provide the token generation process info.
#include "TokenHelper.h"

/* 2. Define the API Key */
#define API_KEY "AIzaSyBwHrKJzegsovWotZ3yVwWWUE4vB_nW01M"

/* 3. Define the RTDB URL */
#define DATABASE_URL "kandidat-8bba9-default-rtdb.europe-west1.firebasedatabase.app" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "test@test.dk"
#define USER_PASSWORD "test123"

/* 5. Define Firebase32 data object for data sending and receiving */
FirebaseData *fbdo = new FirebaseData();

FirebaseConfig config;

FirebaseAuth auth;

String formattedDate;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String tempInteraction;
String tempAction;
String tempContext;
String tempWhat;
int tempValue;
bool sendMessage;
bool sendMessageWithWhat;
bool sendMessageWithWhatAndValue;

unsigned long timeSinceLastLog = 0;

void firebaseInit()
{
    config.api_key = API_KEY;

    /* 9. Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* 10. Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* 11. Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    /* 13. Enable auto reconnect the WiFi when connection lost */
    Firebase.reconnectWiFi(true);

    Serial.println("Firebase Activated!");
}

void begin()
{
    fbdo = new FirebaseData();
}

void initNTPClient()
{
    timeClient.begin();
    timeClient.setTimeOffset(3600);
}

void firebaseEnd()
{
    Firebase.reconnectWiFi(false);
    Firebase.endStream(*fbdo);
    Firebase.removeStreamCallback(*fbdo);
    delete fbdo;
    fbdo = nullptr;
    Serial.println("Firebase stopped");
}

void taskFireBaseAppendUserAction()
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        initNTPClient();

        FirebaseJson json;

        while (!timeClient.update())
        {
            timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedDate();
        json.set("timestamp", formattedDate);
        json.set("interaction", tempInteraction);
        json.set("action", tempAction);
        json.set("context", tempContext);

        String path;
        path += "/test/";
        path += HOUSEHOLD_ID;
        if (Firebase.pushJSON(*fbdo, path, json))
        {

            Serial.println(fbdo->dataPath());

            Serial.println(fbdo->pushName());

            Serial.println(fbdo->dataPath() + "/" + fbdo->pushName());
        }
        else
        {
            Serial.println(fbdo->errorReason());
        }
        timeClient.end();
    }
}

void taskFireBaseAppendUserActionWithWhat()
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        initNTPClient();

        FirebaseJson json;

        while (!timeClient.update())
        {
            timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedDate();
        Serial.println(formattedDate);

        json.set("timestamp", formattedDate);
        json.set("interaction", tempInteraction);
        json.set("action", tempAction);
        json.set("context", tempContext);
        json.set("what", tempWhat);

        String path;
        path += "/test/";
        path += HOUSEHOLD_ID;
        if (Firebase.pushJSON(*fbdo, path, json))
        {

            Serial.println(fbdo->dataPath());

            Serial.println(fbdo->pushName());

            Serial.println(fbdo->dataPath() + "/" + fbdo->pushName());
        }
        else
        {
            Serial.println(fbdo->errorReason());
        }
        timeClient.end();
    }
}

void taskFireBaseAppendUserActionWithWhatAndValue()
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        initNTPClient();

        FirebaseJson json;

        while (!timeClient.update())
        {
            timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedDate();
        Serial.println(formattedDate);

        json.set("timestamp", formattedDate);
        json.set("interaction", tempInteraction);
        json.set("action", tempAction);
        json.set("context", tempContext);
        json.set("what", tempWhat);
        json.set("value", tempValue);

        String path;
        path += "/test/";
        path += HOUSEHOLD_ID;
        if (Firebase.pushJSON(*fbdo, path, json))
        {
            Serial.println(fbdo->dataPath());

            Serial.println(fbdo->pushName());

            Serial.println(fbdo->dataPath() + "/" + fbdo->pushName());
        }
        else
        {
            Serial.println(fbdo->errorReason());
        }
        timeClient.end();
    }
}

void firebaseAppendUserAction(String interaction, String action, String context)
{
    if (millis() > timeSinceLastLog + 1000)
    {
        timeSinceLastLog = millis();
        tempInteraction = interaction;
        tempAction = action;
        tempContext = context;
        sendMessage = true;
    }
}

void firebaseAppendUserAction(String interaction, String action, String context, String what)
{
    if (millis() > timeSinceLastLog + 1000)
    {
        timeSinceLastLog = millis();
        tempInteraction = interaction;
        tempAction = action;
        tempContext = context;
        tempWhat = what;
        sendMessageWithWhat = true;
    }
}

void firebaseAppendUserAction(String interaction, String action, String context, String what, int value)
{
    if (millis() > timeSinceLastLog + 1000)
    {
        timeSinceLastLog = millis();
        tempInteraction = interaction;
        tempAction = action;
        tempContext = context;
        tempWhat = what;
        tempValue = value;
        sendMessageWithWhatAndValue = true;
    }
}

void runTaskFireBaseAppendUserActionWithWhat(String interaction, String action, String context, String what)
{
    tempInteraction = interaction;
    tempAction = action;
    tempContext = context;
    tempWhat = what;
    taskFireBaseAppendUserActionWithWhat();
}

void getUILevel(int &level)
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/UI_Level";
        Serial.println(path);
        if (Firebase.getInt(*fbdo, path))
        {
            if (fbdo->dataType() == "int")
            {
                level = fbdo->intData();

                Serial.println(level);
            }
        }
        else
        {
            Serial.println(fbdo->errorReason());
        }
    }
}

bool getEntertainmentGroupId(int &id)
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/EntertainmentGroupId";
        Serial.println(path);
        if (Firebase.getInt(*fbdo, path))
        {
            if (fbdo->dataType() == "int")
            {
                id = fbdo->intData();
                return true;
            }
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    return false;
}

bool setEntertainmentGroupId(int id)
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/EntertainmentGroupId";
        Serial.println(path);
        if (Firebase.setInt(*fbdo, path, id))
        {
            return true;
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    return false;
}

bool getUserId(String &id)
{
    while (!Firebase.ready())
    {
    }
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/UserId";
        if (Firebase.getString(*fbdo, path))
        {
            if (fbdo->stringData().equals("null") || fbdo->stringData().equals(""))
            {
                return false;
            }
            else
            {
                id = fbdo->stringData();
                return true;
            }
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    else
    {
        Serial.println("Firebase not ready");
    }
    return false;
}

bool setUserId(String id)
{
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/UserId";
        if (Firebase.setString(*fbdo, path, id))
        {
            return true;
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    return false;
}

bool getClientKey(String &id)
{
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/ClientKey";
        if (Firebase.getString(*fbdo, path))
        {
            if (fbdo->stringData().equals("null"))
            {
                return false;
            }
            else
            {
                id = fbdo->stringData();
                return true;
            }
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    return false;
}

bool setClientKey(String id)
{
    if (Firebase.ready())
    {
        String path;
        path += "/Hjem_";
        path += HOUSEHOLD_ID;
        path += "_Config/ClientKey";
        if (Firebase.setString(*fbdo, path, id))
        {
            return true;
        }
        else
        {
            Serial.println(fbdo->errorReason());
            return false;
        }
    }
    return false;
}

void firebaseTask(void *xStruct)
{
    for (;;)
    {
        if (sendMessage)
        {
            taskFireBaseAppendUserAction();
            sendMessage = false;
        }
        if (sendMessageWithWhat)
        {
            taskFireBaseAppendUserActionWithWhat();
            sendMessageWithWhat = false;
        }
        if (sendMessageWithWhatAndValue)
        {
            taskFireBaseAppendUserActionWithWhatAndValue();
            sendMessageWithWhatAndValue = false;
        }

        vTaskDelay(100);
    }
}