//#include <Arduino.h>
#include "M5TimerCAM.h"
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <esp_wpa2.h> //wpa2 library for connections to Enterprise networks

#define EAP_IDENTITY ""
#define EAP_USERNAME "user"
#define EAP_PASSWORD "pwd"
const char* ssid = "KAINS-WiFi";

#define SERVER "httpbin.org"

WiFiClient wifi;
HttpClient client = HttpClient(wifi, SERVER);

void setup() {
  TimerCAM.begin(true); // enable RTC power management
  // TimerCAM.begin(true) results in camera init fail
  // https://github.com/m5stack/TimerCam-arduino/issues/16

  if (!TimerCAM.Camera.begin()) while(1){printf("Camera Init Fail\n"); delay(1000); }
  printf("Camera Init Success\n");

  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_QVGA);

  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);

  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode

  //provide identity
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); 

  //provide username --> identity and username is same
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME)); 

  //provide password
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)); 

  //https://github.com/espressif/arduino-esp32/issues/8492
  esp_wifi_sta_wpa2_ent_enable();
  WiFi.begin(ssid); //connect to wifi

  WiFi.setSleep(false);
  printf("Connecting to %s\n", ssid);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) { delay(500); printf("."); }

  printf("Connected to %s, IP addr=%s\n", ssid, WiFi.localIP().toString().c_str());
  if (TimerCAM.Camera.get()) {
    printf("making POST request\n");
    String contentType = "image/jpeg";
    // client.post("/post", contentType, postData);
    client.post("/post", contentType.c_str(), TimerCAM.Camera.fb->len, TimerCAM.Camera.fb->buf);
    // read the status code and body of the response
    int statusCode  = client.responseStatusCode();
    String response = client.responseBody();
    printf("Status code: %d, Response: %s\n", statusCode, response.c_str());
    TimerCAM.Camera.free();
  }
  printf("going to sleep for 5 sec\n");
  TimerCAM.Power.timerSleep(5);

}

void loop() {
}
