//#include <Arduino.h>
#include "M5TimerCAM.h"
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#define EAP_IDENTITY ""
#define EAP_USERNAME "user"
#define EAP_PASSWORD "pwd"
const char* ssid = "ssid";

#define TargetSERVER "ifdl.jp"
WiFiClient wifi;
HttpClient client = HttpClient(wifi, TargetSERVER);

void setup() {
  TimerCAM.begin(true); // enable RTC power management
  // TimerCAM.begin(true) results in camera init fail
  // https://github.com/m5stack/TimerCam-arduino/issues/16
  // change "Wire1" -> "Wire" at begin() in RTC8563_Class.cpp, located at .pio/libdeps/m5stack-timer-cam/Timer-CAM/src/utility/

  TimerCAM.Power.setLed(10); delay(100); TimerCAM.Power.setLed(0);
  if (!TimerCAM.Camera.begin()) while(1){
    printf("Camera Init Fail\n"); delay(1000);
    while(1){
      TimerCAM.Power.setLed(100); delay(100); TimerCAM.Power.setLed(0); delay(100);
    }
  }
  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_QVGA);
  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);

  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
//  printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
  WiFi.setSleep(false);
  uint8_t f = 0;
  while (WiFi.status() != WL_CONNECTED) {
//    if (f == 0) TimerCAM.Power.setLed(100); else TimerCAM.Power.setLed(0);
//    f = 1 - f;
    delay(500);
  }
  TimerCAM.Power.setLed(0);

  printf("Connected to %s, IP addr=%s\n", ssid, WiFi.localIP().toString().c_str());

  if (TimerCAM.Camera.get()) {
    printf("making POST request\n");
    String contentType = "image/jpeg";
    client.post("/akita/putimage.php", contentType.c_str(), TimerCAM.Camera.fb->len, TimerCAM.Camera.fb->buf);
    // read the status code and body of the response
    int statusCode  = client.responseStatusCode();
    String response = client.responseBody();
    printf("Status code: %d, Response: %s\n", statusCode, response.c_str());
    TimerCAM.Camera.free();
  }
  else{
    // image capture error
    for (uint8_t i = 0; i < 10; i++){
      TimerCAM.Power.setLed(100); delay(100); TimerCAM.Power.setLed(0); delay(100);
    }
  }
  TimerCAM.Power.setLed(10); delay(100); TimerCAM.Power.setLed(0);
  printf("going to sleep for 60*60 sec\n");
  TimerCAM.Power.timerSleep(3600);
}

void loop() {
}
