#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "ssid_config.h"

// D2 = GPIO4
#define RELAY_PIN  4
// LED = D4 = GPIO2
#define LED_PIN  2

class WifiRouterChecker {
public:
  void init() {
    failure_detected = false;
    // Set system ready callback method
    System.onReady(Delegate<void()>(&WifiRouterChecker::onReady, this));

    // Station - WiFi client
    WifiStation.enable(true);
    WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

    // Optional: Change IP addresses (and disable DHCP)
    WifiStation.setIP(WIFI_STATIC_IP);
  }
  
private:
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

  void reconnectAfterMs(uint32_t ms) {
    tmrReconnect.initializeMs(ms, TimerDelegate(&WifiRouterChecker::reconnect, this)).start();
  }
  void reconnect() {
    tmrReconnect.stop();
    //
    debugf("%9d: Reconnect", RTC.getRtcSeconds());
    WifiStation.disconnect();
    WifiStation.connect();
    connect(5);
  }

  void connect(uint32_t timeoutSec) {
    // Print available access points
    //WifiStation.startScan(Delegate<void(bool succeeded, BssList list)>(&WifiRouterChecker::listNetworks, this));
    //
    WifiStation.waitConnection(Delegate<void()>(&WifiRouterChecker::onConnectOk, this), timeoutSec, Delegate<void()>(&WifiRouterChecker::onConnectFail, this));
  }
  
  // Will be called when WiFi station was connected to AP
  void onConnectOk() {
    failure_detected = false;
//    debugf("%9d: Connect successfull!", RTC.getRtcSeconds());
//    Serial.println(WifiStation.getIP().toString());
    debugf("%9d: Connect successfull, ip: %s", RTC.getRtcSeconds(), WifiStation.getIP().toString().c_str());
    //
    reconnectAfterMs(5000);
  }

  // Will be called when WiFi station timeout was reached
  void onConnectFail()
  {
    failure_detected = true;
    debugf("%9d: Connect failed!", RTC.getRtcSeconds());
    reconnectAfterMs(2000);
  }

private:
  Timer tmrReconnect;
public:
  bool failure_detected;
};


class WifiRouterStatus {
public:
  enum state_t {stOff=0, stOn, stFail};
  void init() {
    // relay and led init
    digitalWrite(LED_PIN, false);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, false);
    pinMode(RELAY_PIN, OUTPUT);
    //
    state = stOn;
    //
    tmrBlink.initializeMs(200, TimerDelegate(&WifiRouterStatus::onTmrBlink, this)).start();
  }
  void on() {
    state = stOn;
    //tmrBlink.stop();
    digitalWrite(LED_PIN, false);
    digitalWrite(RELAY_PIN, false);
  }
  void off() {
    state = stOff;
    //tmrBlink.stop();
    digitalWrite(LED_PIN, true);
    digitalWrite(RELAY_PIN, true);
  }
  void fail() {
    state = stFail;
    blink_pin_state = false;
    digitalWrite(LED_PIN, blink_pin_state);
    digitalWrite(RELAY_PIN, false);
  }
private:
  void onTmrBlink() {
    if (state != stFail) return;
    blink_pin_state = !blink_pin_state;
    digitalWrite(LED_PIN, blink_pin_state);
  }
  Timer tmrBlink;
public:
  state_t state;
  bool blink_pin_state;
};

class WifiRouterRecovery {
public:
  enum state_t {stPowerOn=0, stOn, stTimingUp, stOff};
  void init() {
    checker.init();
    status.init();
    //
    debugf("%9d: PowerOn", RTC.getRtcSeconds());
    state = stPowerOn;
    n = 30;
    status.on();
    tmr.initializeMs(1000, TimerDelegate(&WifiRouterRecovery::step, this)).start();
  }
  
private:
  void step() {
    switch (state) {
      case stOn:
        if (checker.failure_detected) {
          debugf("%9d: TimingUp", RTC.getRtcSeconds());
          state = stTimingUp;
          n = 20;
          status.fail();
        }
        break;
      case stTimingUp:
        if (!checker.failure_detected) {
          debugf("%9d: On", RTC.getRtcSeconds());
          state = stOn;
          status.on();
        }
        if (--n == 0) { 
          debugf("%9d: Off", RTC.getRtcSeconds());
          state = stOff;
          n = 10;
          status.off();
        }
        break;
      case stOff:
        if (--n == 0) {
          debugf("%9d: PowerOn", RTC.getRtcSeconds());
          state = stPowerOn;
          n = 30;
          status.on();
        }
        break;
      case stPowerOn:
      default:
        if (--n == 0) {
          debugf("%9d: On", RTC.getRtcSeconds());
          state = stOn;
        }
        break;
    }
  }
public:
  WifiRouterChecker checker;
  WifiRouterStatus status;
  state_t state;
  Timer tmr;
  uint32_t n;
};

WifiRouterRecovery recovery;

void init() {
	Serial.begin(SERIAL_BAUD_RATE);
	//Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Thanx, Sming!");

  //SET higher CPU freq & disable wifi sleep
  system_update_cpu_freq(SYS_CPU_160MHZ);
  wifi_set_sleep_type(NONE_SLEEP_T);
  
  //
  recovery.init();
}
