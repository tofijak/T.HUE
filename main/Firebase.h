#ifndef __Firebase_H
#define __Firebase_H

// Interactions
#define touchScreenInteraction "touchScreenInteraction" // Tryk på touchskærm
#define topDialInteraction "topDialInteraction"         // Øverste dial (som regel brugt til brightness)
#define lowerDialInteraction "lowerDialInteraction"     // Nederste dial - selector mellem lys
#define topButtonInteraction "topButtonInteraction"     // Fysisk knap på toppen - Sluk og tænd
#define systemInteraction "systemInteraction"           // Automatisk interaction af systemet
#define allLightOn "allLightOn"                         // Tænd alle lys
#define allLightOff "allLightOff"                       // Sluk alle lys
#define behaviorOn "behaviorOn"                         // Behavior tændt
#define behaviorOff "behaviorOff"                       // Behavior slukket
#define rgbSwitch "rgbSwitch"                           // Skift mellem rgb state
#define changeLight "changeLight"                       // Skift mellem lys
#define navigate "navigate"                             // Skift mellem lys
#define setBrigthness "setBrigthness"                   // Sæt brightness
#define setColorTemp "setColorTemp"                     // Sæt farve temperatur for pære
#define switchLight "switchLight"                       // Tænd eller sluk pære
#define chooseLight "chooseLight"                       // Vælg en pærer

extern TaskHandle_t taskHandlerFirebase;

void firebaseInit();
void begin();
void firebaseEnd();
void firebaseAppendUserAction(String interaction, String action, String context);
void firebaseAppendUserAction(String interaction, String action, String context, String what);
void firebaseAppendUserAction(String interaction, String action, String context, String what, int value);
void runTaskFireBaseAppendUserActionWithWhat(String interaction, String action, String context, String what);
void getUILevel(int &level);
//void firebaseAppendUserAction(String action, String state);
//void firebaseAppendUserAction(String action, String from, String to);

bool getEntertainmentGroupId(int &id);
bool setEntertainmentGroupId(int id);

bool setClientKey(String id);
bool getClientKey(String &id);
bool setUserId(String id);
bool getUserId(String &id);

void firebaseTask(void *xStruct);





#endif