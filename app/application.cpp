#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "ssid_config.h"

Timer tmr;
// D2 = GPIO4
#define RELAY_PIN  4
// LED = D4 = GPIO2
#define LED_PIN  2

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
  debugf("%9d: Reconnect", RTC.getRtcSeconds());
  WifiStation.disconnect();
  WifiStation.connect();
  WifiStation.waitConnection(connectOk, 5, connectFail);
  //
  digitalWrite(RELAY_PIN, true);
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	debugf("%9d: Connect successfull!", RTC.getRtcSeconds());
	Serial.println(WifiStation.getIP().toString());
  tmr.initializeMs(5000, TimerDelegate(&Reconnect)).start();
  digitalWrite(RELAY_PIN, false);
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
	debugf("%9d: Connect failed!", RTC.getRtcSeconds());
  tmr.initializeMs(2000, TimerDelegate(&Reconnect)).start();
  digitalWrite(RELAY_PIN, false);
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready() {
	debugf("%9d: Ready, connect", RTC.getRtcSeconds());
  //
  WifiStation.waitConnection(connectOk, 5, connectFail);
}

void init() {
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Thanx, Sming!");
  
  digitalWrite(RELAY_PIN, false);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, true);
  pinMode(LED_PIN, OUTPUT);

  //SET higher CPU freq & disable wifi sleep
  system_update_cpu_freq(SYS_CPU_160MHZ);
  wifi_set_sleep_type(NONE_SLEEP_T);

	// Set system ready callback method
	System.onReady(ready);

  // Station - WiFi client
  WifiStation.enable(true);
  WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

  // Optional: Change IP addresses (and disable DHCP)
  WifiStation.setIP(WIFI_STATIC_IP);

  // Print available access points
  WifiStation.startScan(listNetworks); // In Sming we can start network scan from init method without additional code

}
