#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>


#include "display.hpp"
#include "scale.hpp"

WiFiClient espClient;
PubSubClient client(espClient);

//wifi not used for now. Maybe OTA later
const char *ssid = "ssid"; // Change this to your WiFi SSID
const char *password = "pw"; // Change this to your WiFi password
long lastReconnectAttempt = 0;


void setup() {


  /* Print chip information */
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
          CONFIG_IDF_TARGET,
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
          (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
          (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d, ", major_rev, minor_rev);
  if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
      printf("Get flash size failed");
      return;
  }

  printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

  Serial.begin(115200);
  while(!Serial){
    delay(50);
  }
  Serial.println("Serial line initialised");
  printf("Hello world!\n");
  
  
  setupDisplay();
  setupScale();

  Serial.println();
  Serial.println("******************************************************");
  // Serial.print("Connecting to ");
  // Serial.println(ssid);

  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
  // lastReconnectAttempt = 0;
  // client.setServer("192.168.1.201", 1883);
}

boolean reconnect() {
  if (client.connect("coffee-scale")) {
    // Once connected, publish an announcement...
    Serial.println("Connected to MQTT");
  }
  return client.connected();
}

void loop() {
  // if (!client.connected()) {
  //   long now = millis();
  //   if (now - lastReconnectAttempt > 5000) {
  //     lastReconnectAttempt = now;
  //     // Attempt to reconnect
  //     if (reconnect()) {
  //       lastReconnectAttempt = 0;
  //     }
  //   }
  // } else {
  //   client.loop();
  // }
  //rotary_loop();
  delay(1000);
}
