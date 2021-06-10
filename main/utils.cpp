#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "secret.h"

const unsigned int CONNECT_TIMEOUT_MS = 30000; // WiFi connnection timeout (ms)
WiFiUDP udpp;
boolean connected = false;

void WiFiEvent(WiFiEvent_t event){
	IPAddress ipadr(192,168,1,134);
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udpp.begin(ipadr,2100);
          connected = true;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}

void connectToWiFi()
{
	unsigned long startTime = millis();
	Serial.println("Connecting to: " + String(SSID));

	WiFi.disconnect();

	//WiFi.onEvent(WiFiEvent);

	WiFi.begin(SSID, SSID_PASSWORD);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		// Serial.print(".");

		if (millis() - startTime > CONNECT_TIMEOUT_MS)
		{
			Serial.println();
			Serial.println("Failed to connect.");
			return;
		}
	}

	WiFi.setAutoReconnect(true);

	Serial.print("Connected to WiFi with IP: ");
	Serial.println(WiFi.localIP());
}

float ReadBatteryVoltage()
{
	return analogRead(35) / 4096.0 * 7.445;
}

void storeStruct(void *data_source, size_t size, char startPosition)
{
  EEPROM.begin(512);
  for(size_t i = 0; i < size; i++)
  {
    char data = ((char *)data_source)[i];
    EEPROM.write(startPosition + i, data);
  }
  EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size, char loadPosition)
{
    EEPROM.begin(512);
    for(size_t i = 0; i < size; i++)
    {
        char data = EEPROM.read(loadPosition + i);
        ((char *)data_dest)[i] = data;
    }
}
