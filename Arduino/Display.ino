
/////////////////////////////////////////////////////////////////////////////////
/////// WIFI IMPORTS
/////////////////////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>

// WiFi
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <WiFiClientSecure.h>

//Filesystem
#include <ArduinoJson.h>

/////////////////////////////////////////////////////////////////////////////////
/////// DISPLAY IMPORTS
/////////////////////////////////////////////////////////////////////////////////

// mapping suggestion from Waveshare SPI e-Paper to Wemos D1 mini
// BUSY -> D2, RST -> D1, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V

// include library, include base class, make path known
#include <GxEPD.h>

// select the display class to use, only one
#include <GxGDEH029A1/GxGDEH029A1.h>      // 2.9" b/w


#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold36pt7b.h>
#include <Fonts/FreeMonoBold48pt7b.h>
#include <Fonts/FreeMonoBold60pt7b.h>
#include <Fonts/FreeMonoBold72pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>


#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>


// for SPI pin definitions see e.g.:
// C:\Users\xxx\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.2\variants\generic\common.h

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io, /*RST=D1*/ 5, /*BUSY=D2*/ 4); // default selection of D1(=5), D2(=4)



/////////////////////////////////////////////////////////////////////////////////
/////// CONFIGURATION
/////////////////////////////////////////////////////////////////////////////////

const char* host = "myhost.de";
const int httpsPort = 80;

/////////////////////////////////////////////////////////////////////////////////
/////// VARIABLES / OBJECTS
/////////////////////////////////////////////////////////////////////////////////

char kunde[30] = "";
char interval[4] = "1";
bool shouldSaveConfig = false; //flag for saving data to fs
String rooms[7];
String values[7];

/////////////////////////////////////////////////////////////////////////////////
/////// HELPER FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////


// callback notifying us of the need to save config to filesystem
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readConfigFromFilesystem () {
  //clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(kunde, json["kunde"]);

          Serial.print("Kunde1: ");
          Serial.println(kunde);
          
          strcpy(interval, json["interval"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
    else {
      Serial.println("config file not found");
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveConfigToFilesystem () {
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["kunde"] = kunde;
    json["interval"] = interval;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}

void connectToWifi () {
  WiFiManager wifiManager;

  //reset settings - for testing
  // wifiManager.resetSettings();

  String apName = "Temperatur-Display " + String(ESP.getChipId());
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Custom Settings
  WiFiManagerParameter custom_kunde_name("kunde", "Kunde", kunde, 30);  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_interval("interval", "Interval in minutes", interval, 4);  // id/name, placeholder/prompt, default, length
  wifiManager.addParameter(&custom_kunde_name);
  wifiManager.addParameter(&custom_interval);


  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(apName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // Go to deep sleep for 10min and then reset/retry
    ESP.deepSleep(600e6);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");


  //read updated parameters
  strcpy(kunde, custom_kunde_name.getValue());
  strcpy(interval, custom_interval.getValue());

  saveConfigToFilesystem ();

}

void readData () {

  String kundeName = String(kunde);

  String url = "/temperatur/read.php?client="+ kundeName;

  // Use WiFiClientSecure class to create TLS connection
  WiFiClient client;
  delay(2000);
  Serial.print("connecting to ");
  Serial.println(host);

  //client.setInsecure();

  delay(2000);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
  }
  else {

    // String url = "/repos/esp8266/Arduino/commits/master/status";
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    delay(1000);
        Serial.println("Respond:");
      char* buff[8];
      //String line = "";
      client.find("<START>");
      int i = 0;
    while(client.available()){

      String room = client.readStringUntil('\t');
      String temp = client.readStringUntil('\n');
    
      if (room == "<END>") {
        break;
     }

      rooms[i] = room;
      values[i] = temp.substring(0,4);
      i++;
      if (i > 6) {
        break;
      }
     
      Serial.println(room + "|" + temp);      
    }
 
  }
  client.stop();
}

/////////////////////////////////////////////////////////////////////////////////
/////// DISPLAY
/////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  display.init(115200); // enable diagnostic output on Serial

  display.setRotation(3);

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  delay(1000);

  readConfigFromFilesystem ();

  connectToWifi();
  Serial.print("Kunde2.6: ");
          Serial.println(kunde);

  for( int i = 0; i < 7;  i++ ) {
   rooms[i] = "";
  }

Serial.print("Kunde2.7: ");
          Serial.println(kunde);
  
  Serial.println("setup done");
}

void loop()
{
 // showBitmapExample();
 // delay(2000);

  //drawCornerTest();
  
  //showFont("FreeMonoBold9pt7b", &FreeMonoBold9pt7b);
  //showFont("FreeMonoBold12pt7b", &FreeMonoBold12pt7b);
  //showFont("FreeMonoBold18pt7b", &FreeMonoBold18pt7b);
  //showFont("FreeMonoBold24pt7b", &FreeMonoBold24pt7b);

  readData();

  showDashboard(&FreeMonoBold12pt7b);

  // Do deep Sleep
  Serial.println();
  int intervalInt = String(interval).toInt();
  if (intervalInt > 69) {
    intervalInt = 70;
  }
  unsigned int sleepTime = intervalInt * 60 * 1000000;
  Serial.println("I'm awake, but I'm going into deep sleep mode for " + String(interval) + " minutes");
  ESP.deepSleep(sleepTime);
  delay(3000);
}

#include "IMG_0001.h"

void showBitmapExample()
{
  display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1));
  delay(2000);
  display.drawExampleBitmap(BitmapExample2, sizeof(BitmapExample2));
  delay(2000);
  display.fillScreen(GxEPD_WHITE);
  display.drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
  display.update();
  delay(2000);
 // display.fillScreen(GxEPD_WHITE);
  display.drawExampleBitmap(gImage_IMG_0001,sizeof(gImage_IMG_0001));
 // display.update();
  delay(60000);
 // showBoat();
}

void showDashboard(const GFXfont* f)
{
  // Basics
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 10);
 // display.println();

  // Layout
  display.drawFastVLine(97, 0, 128, 0);
  display.drawFastVLine(98, 0, 128, 0);
  display.drawFastVLine(99, 0, 128, 0);
  
  display.drawFastVLine(197, 0, 128, 0);
  display.drawFastVLine(198, 0, 128, 0);
  display.drawFastVLine(199, 0, 128, 0);
  
  display.drawFastHLine(0, 63, 296, 0);
  display.drawFastHLine(0, 64, 296, 0);
  display.drawFastHLine(0, 65, 296, 0);
  
  // Anzeigen
  
  // Wert 1
  display.setCursor(0, 20);
  display.setFont(&FreeSansBold12pt7b);
  String room1 = rooms[0];  //"Innen";
  room1 = fitString(room1, 96);
  display.println(room1);
  
  display.setCursor(0, 60);
  display.setFont(&FreeSansBold24pt7b);  
  String value1 = values[0]; // "22.5°";
  display.println(value1);

  // Wert 2
  display.setCursor(100, 20);
  display.setFont(&FreeSansBold12pt7b);
  String room2 = rooms[1];  //"Aussen";
  room2 = fitString(room2, 96);
  display.println(room2);

  display.setCursor(100, 60);
  display.setFont(&FreeSansBold24pt7b);  
  String value2 = values[1]; //"13.7°";
  display.println(value2);

  // Wert 3
  display.setCursor(200, 20);
  display.setFont(&FreeSansBold12pt7b);
  String room3 = rooms[2];  //"Keller";
  room3 = fitString(room3, 96);
  display.println(room3);

  display.setCursor(200, 60);
  display.setFont(&FreeSansBold24pt7b);  
  String value3 = values[2]; //"21.4°";
  display.println(value3);

  // Wert 4
  display.setCursor(0, 86);
  display.setFont(&FreeSansBold12pt7b);
  String room4 = rooms[3];  //"Bad";
  room4 = fitString(room4, 93);
  display.println(room4);

  display.setCursor(0, 126);
  display.setFont(&FreeSansBold24pt7b);  
  String value4 = values[3]; //"23.3°";
  display.println(value4);

  // Wert 5
  display.setCursor(100, 86);
  display.setFont(&FreeSansBold12pt7b);
  String room5 = rooms[4];  //"Wohnzimmer";
  room5 = fitString(room5, 96);
  display.println(room5);

  display.setCursor(100, 126);
  display.setFont(&FreeSansBold24pt7b);  
  String value5 = values[4]; //"19.9°";
  display.println(value5);

  if (rooms[6].equals("")) {
      // Wert 6
    display.setCursor(200, 86);
    display.setFont(&FreeSansBold12pt7b);
    String room6 = rooms[5];  //"Bad_OG";
    room6 = fitString(room6, 96);
    display.println(room6);
  
    display.setCursor(200, 126);
    display.setFont(&FreeSansBold24pt7b);  
    String value6 = values[5]; //"28.2°";
    display.println(value6);
  } else {
      display.drawExampleBitmap(qrCode, 210, 66, 66, 66, GxEPD_BLACK);
  }



  //TEST Refresh
  /*
  for (int i=0;i<5;i++) {
      display.setCursor(0, 128);
  display.setFont(&FreeMonoBold36pt7b);  
  int val = i+22;
  String value1 = String(val);
  showPartialUpdate(value1);
  //display.println(value1);  
  //display.update();
  delay(2000);
  }
*/

  // Refresh
  //delay(2000);
  display.update();
  //delay(10000);
  
}


String fitString(String text, int space) {
  if (text.length() ==0) {
    return "";
  }
  // Get the size of the bounding box to erase the old text
  int16_t x1, y1;
  uint16_t w, h;

   display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  
  while (w > space && text.length() >0 ) {
    text = text.substring(0, text.length()-1);
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  }

  if (text.length() == 0) {
    return "ERR";
  }
  
  return text;
}

  
void showPartialUpdate(String value)
{
  uint16_t box_x = 30;
  uint16_t box_y = 70;
  uint16_t box_w = 30;
  uint16_t box_h = 50;
  uint16_t cursor_y = box_y + 50;
  display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
  display.setCursor(box_x, cursor_y);
  display.print(value);
  display.updateWindow(box_x, box_y, box_w, box_h, true);
 // display.updateWindow(box_x, box_y, box_w, box_h, true);
}

void showFont(const char name[], const GFXfont* f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
  display.update();
  delay(5000);
}

#include "IMG_0001.h"
void showBoat()
{
  // thanks to bytecrusher: http://forum.arduino.cc/index.php?topic=487007.msg3367378#msg3367378
  uint16_t x = (display.width() - 64) / 2;
  uint16_t y = 5;
  display.fillScreen(GxEPD_WHITE);
  display.drawExampleBitmap(gImage_IMG_0001, x, y, 64, 180, GxEPD_BLACK);
  display.update();
  delay(500);
  uint16_t forward = GxEPD::bm_invert | GxEPD::bm_flip_x;
  uint16_t reverse = GxEPD::bm_invert | GxEPD::bm_flip_x | GxEPD::bm_flip_y;
  for (; y + 180 + 5 <= display.height(); y += 5)
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawExampleBitmap(gImage_IMG_0001, x, y, 64, 180, GxEPD_BLACK, forward);
    display.updateWindow(0, 0, display.width(), display.height());
    delay(500);
  }
  delay(1000);
  for (; y >= 5; y -= 5)
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawExampleBitmap(gImage_IMG_0001, x, y, 64, 180, GxEPD_BLACK, reverse);
    display.updateWindow(0, 0, display.width(), display.height());
    delay(1000);
  }
  display.update();
  delay(1000);
}
