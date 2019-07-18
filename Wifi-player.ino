#include <Arduino.h>

#include <ESP8266WiFi.h>

#include "ESP8266WebServer.h"           
#include "DNSServer.h"                  
#include "WiFiManager.h"                // https://github.com/tzapu/WiFiManager

//https://github.com/earlephilhower/ESP8266Audio
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"


// Initialize the OLED display using Wire library
//GPIO0 = D3
//GPIO14 = D5
SSD1306Wire  display(0x3c, 0, 14);

// To run, set your ESP8266 build to 160MHz, update the SSID info, and upload.

int sensorPin = A0;    // The port to which the potentiometer is connected
int sensorValue = 0;  // Variable to store readings

AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;

String strMD1;
int MDSF;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64], s3[32], s4[32];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
  strMD1 = String(s2);
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void setup()
{
  system_update_cpu_freq(SYS_CPU_160MHZ);
  Serial.begin(115200);
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Wi-Fi player");
  Serial.println("Connected");

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  String URL;
  
  sensorValue = analogRead(sensorPin);
  if (sensorValue < 250){
    URL = "http://jazz.streamr.ru/jazz-64.mp3";
    MDSF = 1;
  } else if (sensorValue < 500) {
    URL = "http://kpradio.hostingradio.ru:8000/russia.radiokp128.mp3";
    MDSF = 2;
  } else if (sensorValue < 750) {
    URL = "http://retroserver.streamr.ru:8043/retro64";
    MDSF = 3;
  } else {
    URL = "http://online.radiorecord.ru:8102/chil_64";
    MDSF = 4;
  }
  
  file = new AudioFileSourceICYStream(URL.c_str());
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, 4096);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);
}


void loop()
{  
  static int lastms = 0;
  if (mp3->isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
     }
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
    Serial.flush();
    ESP.restart();
  }
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  
  sensorValue = analogRead(sensorPin);
  if (sensorValue < 250){
    display.drawString(0, 16, "1  Jazz FM");
    if (MDSF == 1) {
      display.setFont(ArialMT_Plain_10);
      display.drawStringMaxWidth(0, 40, 128, strMD1);
    }
  } else if (sensorValue < 500){
    display.drawString(0, 16, "2  News FM");
    if (MDSF == 2) {
      display.setFont(ArialMT_Plain_10);
      display.drawStringMaxWidth(0, 40, 128, strMD1);
    }
  }else if (sensorValue < 750){
    display.drawString(0, 16, "3  Retro FM");
    if (MDSF == 3) {
      display.setFont(ArialMT_Plain_10);
      display.drawStringMaxWidth(0, 40, 128, strMD1);
    }
  }else{
    display.drawString(0, 16, "4  Chillout");
    if (MDSF == 4) {
      display.setFont(ArialMT_Plain_10);
      display.drawStringMaxWidth(0, 40, 128, strMD1);
    }
  }
  
  // write the buffer to the display
  display.display();
}
