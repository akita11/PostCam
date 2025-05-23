//#include <Arduino.h>
#include "M5TimerCAM.h"
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define EAP_IDENTITY ""
#define EAP_USERNAME "USER"
#define EAP_PASSWORD "PWD"
const char* ssid = "SSID";
//const char* ssid = "SSID_WPA2";
//const wifi_pwd = "WIFI_PWD"
#define POST_URL "/URL/put.php"

#define WIFI_TRIAL 120  // WiFi接続の最大試行回数

#define TargetSERVER "TARGET_SERVER"

#define DEBUG

WiFiClient wifi;
HttpClient client = HttpClient(wifi, TargetSERVER);

void showError(int T, int N){
  for (uint8_t i = 0; i < N; i++){
    TimerCAM.Power.setLed(50); delay(T); TimerCAM.Power.setLed(0); delay(T);
  }
}

bool connectToWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  printf("Connecting to %s...", ssid);
#ifdef EAP_IDENTITY
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
#else
  WiFi.begin(ssid, "hogeupip5");
#endif
  WiFi.setSleep(false);
  uint8_t f = 0;
  uint8_t trial = 0;
  while (WiFi.status() != WL_CONNECTED && trial < WIFI_TRIAL) {
    delay(500);
    if (f == 0) TimerCAM.Power.setLed(10);
    else TimerCAM.Power.setLed(0);
    f = 1 - f;
    trial++;
  }
  TimerCAM.Power.setLed(0);
  if (WiFi.status() == WL_CONNECTED) {
#ifdef DEBUG
    printf("connected\n");
#endif
    return true;
  } else {
#ifdef DEBUG
    printf("connection failed after %d trials\n", WIFI_TRIAL);
#endif
		showError(100, 10);
    return false;
  }
}

void setup() {
  TimerCAM.begin(true); // enable RTC power management
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

  uint8_t res;
	//  res = TimerCAM.Camera.get(); // dummy capture

  rtc_time_t timeStruct;
  rtc_date_t dateStruct;
  TimerCAM.Rtc.getTime(&timeStruct);
  TimerCAM.Rtc.getDate(&dateStruct);
#ifdef DEBUG
  printf("Current: %04d/%02d/%02d %02d:%02d:%02d\n", dateStruct.year, dateStruct.month, dateStruct.date, timeStruct.hours, timeStruct.minutes, timeStruct.seconds);
#endif

  // RTCの年が2024年以前の場合のみNTP同期を実行
  if (dateStruct.year < 2024) {
    printf("RTC year is before 2024, starting NTP sync...\n");
    if (connectToWiFi() == true){
	    // NTPサーバーから時刻を取得
  	  WiFiUDP ntpUDP;
    	NTPClient timeClient(ntpUDP, "ntp.nict.jp", 9 * 3600, 60000); // JST (UTC+9)  
	    timeClient.begin();
	    timeClient.update();
    
  	  // 現在の時刻を取得
    	time_t epochTime = timeClient.getEpochTime();
    	struct tm *ptm = gmtime((time_t *)&epochTime);
    
	    // RTCの時刻をNTPから取得した時刻で更新
  	  timeStruct.hours = timeClient.getHours();
    	timeStruct.minutes = timeClient.getMinutes();
  		timeStruct.seconds = timeClient.getSeconds();
    	TimerCAM.Rtc.setTime(&timeStruct);
    	dateStruct.year = ptm->tm_year + 1900;  // tm_yearは1900年からの年数
    	dateStruct.month = ptm->tm_mon + 1;     // tm_monは0-11
    	dateStruct.date = ptm->tm_mday;
    	TimerCAM.Rtc.setDate(&dateStruct);
    	// 更新後の時刻を表示
    	TimerCAM.Rtc.getDate(&dateStruct);	
    	TimerCAM.Rtc.getTime(&timeStruct);
    	printf("Updated: %04d/%02d/%02d %02d:%02d:%02d\n", dateStruct.year, dateStruct.month, dateStruct.date, timeStruct.hours, timeStruct.minutes, timeStruct.seconds);
    	WiFi.disconnect(true);
    	TimerCAM.Power.setLed(0); // LED消灯
		}
  }

  // カメラ撮影とアップロード用にWiFi再接続
  res = TimerCAM.Camera.get();
  if (res){
	  if (connectToWiFi() == true){
#ifdef DEBUG
			printf("posting image...");
#endif
 	  	String contentType = "image/jpeg";
      // ヘッダーを追加
      client.beginRequest();
      client.post(POST_URL);
      client.sendHeader("Content-Type", contentType);
      client.sendHeader("Content-Length", String(TimerCAM.Camera.fb->len));
      client.beginBody();
      // 画像データを送信
  	  client.write((const uint8_t*)TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len);
      client.endRequest();
      
      // レスポンスを確認
      int statusCode = client.responseStatusCode();
      String response = client.responseBody();
//      printf("HTTP Status: %d\n", statusCode);
//      printf("Response: %s\n", response.c_str());
  		}
		else{
#ifdef DEBUG
			printf("failed to connect WiFi\n");
#endif
		}
    TimerCAM.Camera.free();
#ifdef DEBUG
		printf("done\n");
#endif
  }
  else{
    // image capture error
#ifdef DEBUG
		printf("image capture error\n");
#endif
		showError(50, 50);
  }

  // 現在時刻を取得
  rtc_time_t currentTime;
  TimerCAM.Rtc.getTime(&currentTime);

  rtc_time_t alarmTime;
/*
	// for alarm/wake test, every 2min
  alarmTime.hours = currentTime.hours;
  alarmTime.minutes = currentTime.minutes + 2;
  alarmTime.seconds = 0;
  printf("Next alarm: %d:%d\n", alarmTime.hours, alarmTime.minutes);
*/

  // 次のアラーム時間を設定（10時または16時）
  if (currentTime.hours < 10 || currentTime.hours >= 16) {
    // 現在時刻が10時前なら10時に設定
    printf("Next alarm: 10:00\n");
    alarmTime.hours = 10;
  } else if (currentTime.hours < 16) {
    // 現在時刻が10時以降16時前なら16時に設定
#ifdef DEBUG
    printf("Next alarm: 16:00\n");
#endif
    alarmTime.hours = 16;
	}
  alarmTime.minutes = 0;
  alarmTime.seconds = 0;

  TimerCAM.Power.timerSleep(alarmTime); // sleep until alarm
}

void loop() {
}
