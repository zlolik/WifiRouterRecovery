#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "ssid_config.h"

Timer tmr;
// D4 = GPIO2
#define RELE_PIN  2

// Will be called when WiFi station network scan was completed
void listNetworks(bool succeeded, BssList list) {
	if (!succeeded) {
		Serial.println("Failed to scan networks");
		return;
	}
	for (int i = 0; i < list.count(); i++) {
		Serial.print("\tWiFi: ");
		Serial.print(list[i].ssid);
		Serial.print(", ");
		Serial.print(list[i].getAuthorizationMethodName());
		if (list[i].hidden) Serial.print(" (hidden)");
		Serial.println();
	}
}

void connectOk();
void connectFail();

void Reconnect() {
  tmr.stop();
  //
  WifiStation.disconnect();
  WifiStation.connect();
  WifiStation.waitConnection(connectOk, 5, connectFail);
  //
  digitalWrite(RELE_PIN, true);
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	debugf("I'm CONNECTED in %d", RTC.getRtcSeconds());
	Serial.println(WifiStation.getIP().toString());
  tmr.initializeMs(5000, TimerDelegate(&Reconnect)).start();
  digitalWrite(RELE_PIN, false);
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
	debugf("I'm NOT CONNECTED!");
  tmr.initializeMs(2000, TimerDelegate(&Reconnect)).start();
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready() {
	debugf("READY!");
	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
}

void init() {
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Thanx, Sming!");
  
  pinMode(RELE_PIN, OUTPUT);

	// Set system ready callback method
	System.onReady(ready);

  // Station - WiFi client
  WifiStation.enable(true);
  WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

  // Optional: Change IP addresses (and disable DHCP)
  WifiStation.setIP(WIFI_STATIC_IP);

  // Print available access points
  WifiStation.startScan(listNetworks); // In Sming we can start network scan from init method without additional code

  // Run our method when station was connected to AP (or not connected)
  WifiStation.waitConnection(connectOk, 5, connectFail); // We recommend 20+ seconds at start
}
