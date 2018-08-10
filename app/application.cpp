#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "ssid_config.h"

Timer tmr;
// D2 = GPIO4
#define RELAY_PIN  4
// LED = D4 = GPIO2
#define LED_PIN  2

class WifiRouterChecker {
public:
  WifiRouterChecker(): failure_detected(false) {
    // Set system ready callback method
    System.onReady(Delegate<void()>(&WifiRouterChecker::onReady, this));

    // Station - WiFi client
    WifiStation.enable(true);
    WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

    // Optional: Change IP addresses (and disable DHCP)
    WifiStation.setIP(WIFI_STATIC_IP);

  }
  // Will be called when WiFi hardware and software initialization was finished
  // And system initialization was completed
  void onReady() {
    debugf("%9d: WiFi ready, connect", RTC.getRtcSeconds());
    connect(5);
  }
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
private:
  void reconnectAfterMs(uint32_t ms) {
    tmr.initializeMs(ms, TimerDelegate(&WifiRouterChecker::reconnect, this)).start();
  }
  void reconnect() {
    tmr.stop();
    //
    debugf("%9d: Reconnect", RTC.getRtcSeconds());
    WifiStation.disconnect();
    WifiStation.connect();
    connect(5);
    //
    digitalWrite(RELAY_PIN, true);
  }

  void connect(uint32_t timeoutSec) {
    // Print available access points
    WifiStation.startScan(Delegate<void(bool succeeded, BssList list)>(&WifiRouterChecker::listNetworks, this));
    //
    WifiStation.waitConnection(Delegate<void()>(&WifiRouterChecker::onConnectOk, this), timeoutSec, Delegate<void()>(&WifiRouterChecker::onConnectFail, this));
  }
  
  // Will be called when WiFi station was connected to AP
  void onConnectOk()
  {
    debugf("%9d: Connect successfull!", RTC.getRtcSeconds());
    Serial.println(WifiStation.getIP().toString());
    //
    reconnectAfterMs(5000);
    digitalWrite(RELAY_PIN, false);
  }

  // Will be called when WiFi station timeout was reached
  void onConnectFail()
  {
    failure_detected = true;
    debugf("%9d: Connect failed!", RTC.getRtcSeconds());
    reconnectAfterMs(2000);
    digitalWrite(RELAY_PIN, false);
  }
  
public:
  bool failure_detected;
};


void init() {
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Thanx, Sming!");

  //SET higher CPU freq & disable wifi sleep
  system_update_cpu_freq(SYS_CPU_160MHZ);
  wifi_set_sleep_type(NONE_SLEEP_T);
  
  // realy and led init
  digitalWrite(LED_PIN, true);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, false);
  pinMode(RELAY_PIN, OUTPUT);
  
  //
  WifiRouterChecker checker;
}
