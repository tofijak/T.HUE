#ifndef __hue_H
#define __hue_H

#include <Arduino.h>
#include "Client.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <ESPmDNS.h>

#define LIGHT_ON "{\"on\":true}"
#define LIGHT_OFF "{\"on\":false}"
#define COLOR_LOOP_ON "{\"transitiontime\":0, \"effect\":\"colorloop\"}"
#define COLOR_LOOP_OFF "{\"transitiontime\":0, \"effect\":\"none\"}"
#define MAX_LIGHTS_PER_BRIDGE 50
#define MAX_GROUPS_PER_BRIDGE 64

//#define DEBUG
//#define NO_BRIDGE
/*#define DEBUG_IDs*/

struct Room
{
	const char *key;
	uint8_t id;
	std::string name;
	uint8_t on;
	int16_t bri;
	uint8_t lightIds[20];
	uint8_t lightCounter;
};

struct RoomTest
{
	const char *key;
	uint8_t id;
	uint8_t on;
	uint8_t lightIds[20];
	uint8_t lightCounter;
};

struct Light
{
	const char *key;
	uint8_t id;
	std::string name;
	boolean on;
	int16_t brightness;
	uint8_t hue;
	uint8_t sat;
	std::string effect;
	std::string alert;
	std::string colormode;
	uint8_t ct;
	uint8_t xy[2];
	bool onlyDimmable;
};

class Hue
{

public:
	String lightNames[MAX_LIGHTS_PER_BRIDGE] = {};
	String groupIds[MAX_GROUPS_PER_BRIDGE] = {}; //64 max# of groups per bridge
	String groupNames[MAX_GROUPS_PER_BRIDGE] = {};
	uint8_t numGroups = 0;
	uint8_t numLights = 0;

	Hue();
	void begin(String);

	/*Single /light endpoint methods*/
	String lightOn(uint8_t lightId);
	String lightOff(uint8_t lightId);
	String brightness(uint8_t lightId, uint8_t brightness);
	String hue(uint8_t lightId, uint16_t setHue);
	String sat(uint8_t lightId, uint8_t setSat);
	String colorLoop(uint8_t lightId, bool enable);
	String alert(uint8_t lightId, String alertState);
	String flash(uint8_t lightId);
	String colorTemp(uint8_t lightId, uint16_t temp);
	void getLightIds(Light lights[], boolean &pagination, uint8_t &lightCounter);
	void getLightStates(Light lights[], uint8_t lightIds[], uint8_t lightCount);
	void getNewIds();

	/*When updating light attributes, unless there are a dozen or more lights it is generally more 
	efficient to use the lights API. With larger numbers of lights, it can be more efficient to access 
	them as a group, using the groups API.*/

	/* /groups endpoint methods */
	void turnAllLightsOn(void);
	void turnAllLightsOff(void);
	String groupOn(uint8_t groupId);
	String groupOff(uint8_t groupId);
	void getGroups(Room rooms[], boolean &pagination, uint8_t &roomsCount);
	void getGroup(uint8_t roomId, Room &room);

	String groupBrightness(uint8_t groupId, uint8_t brightness);
	String groupHue(uint8_t groupId, uint16_t setHue);
	String groupSat(uint8_t groupId, uint8_t setSat);
	String groupColorLoop(uint8_t groupId, bool enable);
	String groupAlert(uint8_t groupId, String alertState);
	String groupFlash(uint8_t groupId);
	String groupColorTemp(uint8_t groupId, uint16_t temp);
	boolean startStreaming(uint8_t groupId);
	boolean createEntertainmentGroupWithAllLights(Light lightIds[]);

	void fetchRoomsAndLights(Room rooms[], Light lights[], boolean &roomPagination, boolean &lightPagination, uint8_t &roomCount, uint8_t lightCount);

	String _userId = "fR6aF1RbFHE-Iw3FPnj5umxK1plZaBn7D5lOA4Fu";
	String _hueBridgeIP;
	String _hueBridgeBareIP;
	String _clientKey = "D5592C72985F881182F9CF599B28FAFA";
	uint8_t _entertainmentGroup = 8;
	String _discoverBridges();


private:
	HTTPClient _httpClient;

	void _registerApp(String username, String bridgePath);
	String _lightStateEndpoint(uint8_t lightId);
	String _groupActionEndpoint(uint8_t groupId);
	String _lightEndpoint(uint8_t lightId);
	String _getLights(void);
	String _getGroups(void);
	String _getGroup(uint8_t groupId);
	void _checkStatus(int status);
};

#endif