#include "hue.h"
#include "hueResponses.cpp"
#include <EEPROM.h>
#include <Arduino.h>
#include "eink.h"
#include "entertainment.h"
#include "Firebase.h"

char example_string[] = "~New eeprom string";

const int eeprom_size = 500; // values saved in eeprom should never exceed 500 bytes
char eeprom_buffer[eeprom_size];

char first_eeprom_value;

Hue::Hue()
{
}

void writeString(char add, String data)
{
	int _size = data.length();
	int i;
	for (i = 0; i < _size; i++)
	{
		EEPROM.write(add + i, data[i]);
	}
	EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
	EEPROM.commit();
}

String read_String(char add)
{
	char data[100]; //Max 100 Bytes
	int len = 0;
	unsigned char k;
	k = EEPROM.read(add);
	while (k != '\0' && len < 500) //Read until null character
	{
		k = EEPROM.read(add + len);
		data[len] = k;
		len++;
	}
	data[len] = '\0';
	return String(data);
}

void Hue::begin(String username)
{
	Serial.println("Starting Hue Component");
	_hueBridgeIP = _discoverBridges();
}

void Hue::getNewIds()
{

	if (getUserId(_userId))
	{
		getClientKey(_clientKey);
	}
	else
	{
		_registerApp("Simon", _hueBridgeIP);
		setUserId(_userId);
		setClientKey(_clientKey);
	}
}

String Hue::lightOn(uint8_t lightId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.println(" ON");
#endif

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(LIGHT_ON);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::lightOff(uint8_t lightId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.println(" OFF");
#endif

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(LIGHT_OFF);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupOn(uint8_t groupId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.println(" ON");
#endif

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(LIGHT_ON);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupOff(uint8_t groupId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.println(" OFF");
#endif

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(LIGHT_OFF);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::brightness(uint8_t lightId, uint8_t brightness)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set brightness to ");
	Serial.println(brightness);
#endif
	String bri = "{\"transitiontime\":1, \"bri\":";

	bri += String(brightness);
	bri += "}";

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(bri);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::hue(uint8_t lightId, uint16_t setHue)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set hue to ");
	Serial.println(setHue);
#endif
	String hue = "{\"transitiontime\":0, \"hue\":";

	hue += String(setHue);
	hue += "}";

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(hue);
	_checkStatus(status);

	return _httpClient.getString();
}

//Input range 0-254
String Hue::sat(uint8_t lightId, uint8_t setSat)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set saturation to ");
	Serial.println(setSat);
#endif
	String sat = "{\"transitiontime\":0, \"sat\":";

	sat += String(setSat);
	sat += "}";

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(sat);
	_checkStatus(status);

	return _httpClient.getString();
}

//Input Raneg 6500 to 2000
String Hue::colorTemp(uint8_t lightId, uint16_t temp)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set temperature to ");
	Serial.println(temp);
#endif
	uint16_t ct = map(temp, 2000, 6500, 153, 500); //map to hue API range 153-500
	String ctVal = "{\"transitiontime\":0, \"ct\":";

	ctVal += String(ct);
	ctVal += "}";

	if ((ct > 500) || (ct < 153))
	{
		return "Temperature out of range, 2000K-6500K only!";
	}

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(ctVal);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::colorLoop(uint8_t lightId, bool enable)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set color loop to ");
	Serial.println(enable);
#endif
	_httpClient.begin(_lightStateEndpoint(lightId));
	int status;
	if (enable)
	{
		status = _httpClient.PUT(COLOR_LOOP_ON);
	}
	else
	{
		status = _httpClient.PUT(COLOR_LOOP_OFF);
	}
	_checkStatus(status);

	return _httpClient.getString();
}

//alertState types: none, select, or lselect
String Hue::alert(uint8_t lightId, String alertState)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set alert to ");
	Serial.println(alertState);
#endif
	String alert = "{\"transitiontime\":0, \"alert\":\"";

	alert += alertState;
	alert += "\"}";

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT(alert);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::flash(uint8_t lightId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Light ID: ");
	Serial.print(lightId);
	Serial.print(" set to flash ");
#endif

	_httpClient.begin(_lightStateEndpoint(lightId));
	int status = _httpClient.PUT("{\"transitiontime\":0, \"alert\":\"select\"}");
	delay(800);
	_checkStatus(status);

	return _httpClient.getString();
}

void Hue::getLightIds(Light lights[], boolean &pagination, uint8_t &lightCounter)
{
	DynamicJsonDocument jsonBuffer(14000);
	String json = "";
	json = _getLights();
	deserializeJson(jsonBuffer, json);
	JsonObject root = jsonBuffer.as<JsonObject>();
	int i = 0;
	for (JsonPair it : root)
	{
		Light light;
		light.key = it.key().c_str();
		light.id = strtol(light.key, NULL, 0);
		JsonVariant value = it.value();
		light.name = value["name"].as<std::__cxx11::string>();
		light.on = value["state"]["on"].as<boolean>();
		light.brightness = value["state"]["bri"].as<int16_t>();
		lights[i] = light;
		Serial.println(String(lights[i].name.c_str()));
		i++;
	}
	lightCounter = root.size();
	Serial.println("LightCounter: " + String(lightCounter));
	pagination = false;
	if (lightCounter > 4)
		pagination = true;
}

void Hue::getGroups(Room rooms[], boolean &pagination, uint8_t &roomsCount)
{
	DynamicJsonDocument jsonBuffer(14000);
	String json = "";
	json = _getGroups();
	deserializeJson(jsonBuffer, json);
	JsonObject root = jsonBuffer.as<JsonObject>();
	int i = 0;
	for (JsonPair it : root)
	{
		Room room;
		room.key = it.key().c_str();
		room.id = strtol(room.key, NULL, 0);
		JsonVariant value = it.value();
		room.name = value["name"].as<std::string>();

		Serial.println(room.name.c_str());
		room.on = value["action"]["on"].as<boolean>();
		room.bri = value["action"]["bri"].as<int16_t>();
		JsonArray jsonArray = value["lights"].as<JsonArray>();
		copyArray(jsonArray, room.lightIds);
		room.lightCounter = jsonArray.size();
		rooms[i] = room;
		i++;
	}
	// Handle pagination bool
	roomsCount = i - 1;
	pagination = false;
	if (roomsCount > 4)
		pagination = true;
	jsonBuffer.clear();
}

void Hue::getGroup(uint8_t roomId, Room &room)
{
	DynamicJsonDocument jsonBuffer(14000);
	String json = "";
	json = _getGroup(roomId);
	deserializeJson(jsonBuffer, json);
	JsonObject root = jsonBuffer.as<JsonObject>();
	room.id = roomId;
	room.name = root["name"].as<std::string>();
	room.on = root["action"]["on"].as<boolean>();
	JsonArray jsonArray = root["lights"].as<JsonArray>();
	copyArray(jsonArray, room.lightIds);
	room.lightCounter = jsonArray.size();
	jsonBuffer.clear();
}

void Hue::turnAllLightsOff(void)
{
	_getGroups(); //update groupId arrays
	delay(1000);  //For /groups commands you should keep to a maximum of 1 per second.

	for (int i = 0; i < numGroups; i++)
	{
		groupOff(groupIds[i].toInt());
		delay(1000); //For /groups commands you should keep to a maximum of 1 per second.
#ifdef DEBUG
		Serial.println();
		Serial.print("Turning off ");
		Serial.println(groupNames[i]);
#endif
	}
}

void Hue::turnAllLightsOn(void)
{
	_getGroups(); //update groupId arrays
	delay(1000);  //For /groups commands you should keep to a maximum of 1 per second.

	for (int i = 0; i < numGroups; i++)
	{
		groupOn(groupIds[i].toInt());
		delay(1000); //For /groups commands you should keep to a maximum of 1 per second.
#ifdef DEBUG
		Serial.println();
		Serial.print("Turning on ");
		Serial.println(groupNames[i]);
#endif
	}
}

String Hue::groupBrightness(uint8_t groupId, uint8_t brightness)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set brightness to ");
	Serial.println(brightness);
#endif
	String bri = "{\"transitiontime\":0, \"bri\":";

	bri += String(brightness);
	bri += "}";

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(bri);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupHue(uint8_t groupId, uint16_t setHue)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set hue to ");
	Serial.println(setHue);
#endif
	String hue = "{\"transitiontime\":0, \"hue\":";

	hue += String(setHue);
	hue += "}";

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(hue);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupSat(uint8_t groupId, uint8_t setSat)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set saturation to ");
	Serial.println(setSat);
#endif
	String sat = "{\"transitiontime\":0, \"sat\":";

	sat += String(setSat);
	sat += "}";

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(sat);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupColorLoop(uint8_t groupId, bool enable)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set color loop to ");
	Serial.println(enable);
#endif
	_httpClient.begin(_groupActionEndpoint(groupId));
	int status;
	if (enable)
	{
		status = _httpClient.PUT(COLOR_LOOP_ON);
	}
	else
	{
		status = _httpClient.PUT(COLOR_LOOP_OFF);
	}
	_checkStatus(status);
	return _httpClient.getString();
}

String Hue::groupAlert(uint8_t groupId, String alertState)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set alert to ");
	Serial.println(alertState);
#endif
	String alert = "{\"transitiontime\":0, \"alert\":\"";

	alert += alertState;
	alert += "\"}";

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(alert);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupFlash(uint8_t groupId)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set to flash");
#endif

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT("{\"transitiontime\":0, \"alert\":\"select\"}");
	delay(800);
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::groupColorTemp(uint8_t groupId, uint16_t temp)
{
#ifdef DEBUG
	Serial.println();
	Serial.print("Group ID: ");
	Serial.print(groupId);
	Serial.print(" set temperature to ");
	Serial.println(temp);
#endif
	uint16_t ct = map(temp, 2000, 6500, 153, 500); //map to hue API range 153-500
	String ctVal = "{\"transitiontime\":0, \"ct\":";

	ctVal += String(ct);
	ctVal += "}";

	if ((ct > 500) || (ct < 153))
	{
		return "Temperature out of range, 2000K-6500K only!";
	}

	_httpClient.begin(_groupActionEndpoint(groupId));
	int status = _httpClient.PUT(ctVal);
	_checkStatus(status);

	return _httpClient.getString();
}

void Hue::getLightStates(Light lights[], uint8_t lightIds[], uint8_t lightCount)
{
	for (int i = 0; i < lightCount; i++)
	{
		int lightId = lightIds[i];
		_httpClient.begin(_lightEndpoint(lightId));
		int status = _httpClient.GET();
		_checkStatus(status);
		DynamicJsonDocument jsonBuffer(14000);
		deserializeJson(jsonBuffer, _httpClient.getString());
		Light light;
		light.id = lightId;
		Serial.println(light.id);
		light.name = jsonBuffer["name"].as<std::__cxx11::string>();
		light.on = jsonBuffer["state"]["on"].as<boolean>();
		light.brightness = jsonBuffer["state"]["bri"].as<uint8_t>();
		light.alert = jsonBuffer["state"]["alert"].as<std::__cxx11::string>();

		std::string productName = jsonBuffer["productname"].as<std::__cxx11::string>();

		if (productName.compare("Hue white lamp") == 0)
		{
			light.onlyDimmable = true;
		}
		else
		{
			light.hue = jsonBuffer["state"]["hue"].as<uint8_t>();
			light.sat = jsonBuffer["state"]["sat"].as<uint8_t>();
			light.effect = jsonBuffer["state"]["effect"].as<std::__cxx11::string>();
			light.xy[0] = jsonBuffer["state"]["xy"][0].as<uint8_t>();
			light.xy[1] = jsonBuffer["state"]["xy"][1].as<uint8_t>();
			light.ct = jsonBuffer["state"]["ct"].as<uint8_t>();
			light.colormode = jsonBuffer["state"]["colormode"].as<std::__cxx11::string>();
			light.onlyDimmable = false;

			printf("Product Name: %s", productName.c_str());
			printf(" IS WHTIE: %d \n", productName.compare("Hue white lamp"));
		}
		lights[i] = light;
	}
}

String Hue::_lightStateEndpoint(uint8_t lightId)
{
	String endpoint = _hueBridgeIP + "/api/";

	endpoint += _userId;
	endpoint += "/lights/";
	endpoint += lightId;
	endpoint += "/state";

	return endpoint;
}

String Hue::_lightEndpoint(uint8_t lightId)
{

	String endpoint = _hueBridgeIP + "/api/";

	endpoint += _userId;
	endpoint += "/lights/";
	endpoint += lightId;

	return endpoint;
}

String Hue::_groupActionEndpoint(uint8_t groupId)
{
	String endpoint = _hueBridgeIP;
	endpoint += "/api/";
	endpoint += _userId;
	endpoint += "/groups/";
	endpoint += groupId;
	endpoint += "/action";
	return endpoint;
}

String Hue::_getGroup(uint8_t groupId)
{
	String endpoint = _hueBridgeIP;
	endpoint += "/api/";
	endpoint += _userId;
	endpoint += "/groups/";
	endpoint += groupId;

	Serial.println(endpoint);
	_httpClient.begin(endpoint);
	int status = _httpClient.GET();
	_checkStatus(status);

	return _httpClient.getString();
}

String Hue::_getGroups(void)
{
	String endpoint = _hueBridgeIP;
	endpoint += "/api/";

	endpoint += _userId;
	endpoint += "/groups";

	_httpClient.begin(endpoint);
	int status = _httpClient.GET();
	_checkStatus(status);

#ifdef NO_BRIDGE
	return mockGetAllGroupsResponse;
#endif

	return _httpClient.getString();
}

String Hue::_getLights(void)
{
	String endpoint = _hueBridgeIP;
	endpoint += "/api/";
	endpoint += _userId;
	endpoint += "/lights";

	Serial.print("endpoint: ");
	Serial.println(endpoint);
	_httpClient.begin(endpoint);
	int status = _httpClient.GET();
	Serial.println(endpoint);
	_checkStatus(status);

#ifdef NO_BRIDGE
	return mockGetAllLightsResponse;
#endif
	return _httpClient.getString();
}

void Hue::_checkStatus(int status)
{
	if (status > 0)
	{
		Serial.printf("Code: %d\n", status);
	}
	else
	{
		Serial.printf("Error: %s\n", _httpClient.errorToString(status).c_str());
	}
}

String Hue::_discoverBridges()
{
	if (mdns_init() != ESP_OK)
	{
		Serial.println("mDNS failed to start");
		return "";
	}
	int nrOfServices = MDNS.queryService("hue", "tcp");
	if (nrOfServices == 0)
	{
		Serial.println("No services were found.");
	}

	else
	{
		Serial.print("Number of services found: ");
		Serial.println(nrOfServices);

		for (int i = 0; i < nrOfServices; i = i + 1)
		{

			Serial.println("---------------");

			Serial.print("i: ");
			Serial.println(i);

			Serial.print("Hostname: ");
			Serial.println(MDNS.hostname(i));

			Serial.print("IP address: ");
			Serial.println(MDNS.IP(i));

			Serial.print("Port: ");
			Serial.println(MDNS.port(i));

			Serial.println("---------------");
		}
	}
	_hueBridgeBareIP = MDNS.IP(0).toString();
	return "https://" + MDNS.IP(0).toString() + ":" + MDNS.port(0);
}

void Hue::_registerApp(String username, String bridgePath)
{
	String registerPath = bridgePath + "/api";
	Serial.print("Register new app path: ");
	Serial.println(registerPath);
	_httpClient.begin(registerPath);

	String body = "{\"devicetype\":\"";
	body += username;
	body += "\",\"generateclientkey\":true";
	body += "}";

	Serial.println(body);

	// Send HTTP GET request
	int httpResponseCode = _httpClient.POST(body);
	if (httpResponseCode > 0)
	{
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		String payload = _httpClient.getString(); //"[{\"success\":{\"username\":\"testuser\"}}]";
		Serial.println(payload.c_str());
		StaticJsonDocument<200> doc;
		deserializeJson(doc, payload.c_str());
		JsonArray array = doc.as<JsonArray>();
		_userId = array[0]["success"]["username"].as<String>();
		_clientKey = array[0]["success"]["clientkey"].as<String>();
		Serial.println("_userId: " + _userId);
		Serial.println("_clientKey: " + _clientKey);
	}
	else
	{
		Serial.print("Error code: ");
		Serial.println(httpResponseCode);
	}
}

boolean Hue::startStreaming(uint8_t groupId)
{
	String endpoint = _hueBridgeIP + "/api/";

	endpoint += _userId;
	endpoint += "/groups/";
	endpoint += String(groupId);

	String stream = "{\"stream\":{\"active\":true}}";

	_httpClient.begin(endpoint);
	int status = _httpClient.PUT(stream);

	_checkStatus(status);

	Serial.printf("Response: %s\n", _httpClient.getString().c_str());
	return true;
}

boolean Hue::createEntertainmentGroupWithAllLights(Light lightIds[])
{
	int id;
	if (getEntertainmentGroupId(id))
	{
		_entertainmentGroup = id;
		return true;
	}
	String endpoint = _hueBridgeIP + "/api/";
	endpoint += _userId;
	endpoint += "/groups/";

	String body = "{\"type\": \"Entertainment\", \"lights\":[";
	uint8_t lightCounter = 0;
	bool pagination = false;
	getLightIds(lightIds, allLightsPagination, lightCounter);
	allLightCount = lightCounter;
	for (int i = 0; i < lightCounter; i++)
	{
		body += "\"" + String(lightIds[i].id) + "\"";
		if (i != lightCounter - 1)
		{
			body += ",";
		}
	}
	body += "], \"class\": \"TV\"}";
	_httpClient.begin(endpoint);
	int httpResponseCode = _httpClient.POST(body);
	if (httpResponseCode > 0)
	{
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		String payload = _httpClient.getString(); //"[{\"success\":{\"username\":\"testuser\"}}]";
		Serial.println(payload.c_str());
		StaticJsonDocument<200> doc;
		deserializeJson(doc, payload.c_str());
		JsonArray array = doc.as<JsonArray>();
		_entertainmentGroup = array[0]["success"]["id"].as<uint8_t>();
		setEntertainmentGroupId(_entertainmentGroup);
	}
	else
	{
		Serial.print("Error code: ");
		Serial.println(httpResponseCode);
		return false;
	}
	return true;
}
