
/////////////////////////////////////////////////////////////////////////////////
/////// IMPORTS
/////////////////////////////////////////////////////////////////////////////////

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// DHT22 Sensor
#include "DHT.h"

#include <ESP8266WiFi.h>

// WiFi
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// IFTTT
#include <WiFiClientSecure.h>

//Filesystem
#include <ArduinoJson.h>

/////////////////////////////////////////////////////////////////////////////////
/////// CONFIGURATION
/////////////////////////////////////////////////////////////////////////////////

// Decide wich sensor shoud be used
 #define SENSOR_DHT22
// #define SENSOR_DS18B20

#define ONE_WIRE_BUS D2
#define DS18B20_Aufloesung 12

#define DHT22PIN D3
#define DHT22TYPE DHT22

const char* host = "my-url.de";
const int httpsPort = 443;


/////////////////////////////////////////////////////////////////////////////////
/////// VARIABLES / OBJECTS
/////////////////////////////////////////////////////////////////////////////////
const float No_Val = 999.99;
float Temperatur = No_Val;
float Humidity = 0;
bool shouldSaveConfig = false; //flag for saving data to fs

//define your default values here, if there are different values in config.json, they are overwritten.
String apName = "Temperature-Sensor " + String(ESP.getChipId());
char kunde[30] = "location";
char room[30] = "room";
char interval[4] = "30";

#ifdef SENSOR_DS18B20
DeviceAddress DS18B20_Adresse;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature myDS18B20(&oneWire);
#endif

#ifdef SENSOR_DHT22
DHT dht22(DHT22PIN, DHT22TYPE);
#endif




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
          strcpy(room, json["room"]);
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
    json["room"] = room;
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

  String apName = "Temperatur-Sensor " + String(ESP.getChipId());
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Custom Settings
  WiFiManagerParameter custom_kunde_name("kunde", "Kunde", kunde, 30);  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_room_name("room", "Room", room, 30);  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_interval("interval", "Interval in minutes", interval, 4);  // id/name, placeholder/prompt, default, length
  wifiManager.addParameter(&custom_kunde_name);
  wifiManager.addParameter(&custom_room_name);
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
  strcpy(room, custom_room_name.getValue());
  strcpy(interval, custom_interval.getValue());

  saveConfigToFilesystem ();

}


void sendToIFTTT () {
  char result[8]; // Buffer big enough for 7-character float
  dtostrf(Temperatur, 6, 2, result); // Leave room for too large numbers!
  String roomName = String(room);
  roomName.replace(" ", "_");

  String kundeName = String(kunde);
  kundeName.replace(" ", "_");

#ifdef SENSOR_DHT22
  String url = "/temperatur/insert.php?client="+ kundeName +"&room=" + roomName + "&temp=" + String(Temperatur) + "&humidity=" + String(Humidity) + "";
#endif

#ifdef SENSOR_DS18B20
  String url = "/temperatur/insert.php?client="+ kundeName +"&room=" + roomName + "&temp=" + String(Temperatur) + "";
#endif

  // Use WiFiClientSecure class to create TLS connection
  // WiFiClient client;
  WiFiClientSecure client;
  delay(2000);
  Serial.print("connecting to ");
  Serial.println(host);

  //client.setInsecure(); // Enable if you do not want to check certificate

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

    delay(100);
        Serial.println("Respond:");
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    
  }
}


void printValues(String sensor, float humidy, float temp) {
  Serial.print(sensor);
  Serial.print("\t");
  Serial.print("Luftfeuchte: ");
  Serial.print(humidy);
  Serial.print(" %\t");
  Serial.print("Temperatur: ");
  Serial.print(temp);
  Serial.println(" C");
}

/////////////////////////////////////////////////////////////////////////////////
/////// MAIN FUNCTIONS (SETUP / LOOP)
/////////////////////////////////////////////////////////////////////////////////



void setup(void) {
  Serial.begin(9600);

  Serial.println("Wifi Temperatur Sensor");
  Serial.println();

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  delay(1000);

  readConfigFromFilesystem ();

  connectToWifi();

#ifdef SENSOR_DS18B20
  myDS18B20.begin();

  if (myDS18B20.getAddress(DS18B20_Adresse, 0)) {
    myDS18B20.setResolution(DS18B20_Adresse, DS18B20_Aufloesung);
  }
#endif

#ifdef SENSOR_DHT22
  dht22.begin();
#endif

}


void loop(void) {
  // Temperatur auslesen

#ifdef SENSOR_DS18B20

  myDS18B20.requestTemperatures();

  Serial.print("Sensor 1: ");

  Temperatur = myDS18B20.getTempCByIndex(0);
  if (Temperatur == DEVICE_DISCONNECTED_C) {
    Temperatur = No_Val;
    Serial.println("Fehler");

    // Go to deep sleep for 10min and then reset/retry
    ESP.deepSleep(600e6);
  }
  else {
    printValues("DS18B20", 0, Temperatur);
  }
#endif

#ifdef SENSOR_DHT22
  float humidyDHT22 = dht22.readHumidity(); //relative Luftfeuchtigkeit vom Sensor DHT22 lesen
  float tempDHT22 = dht22.readTemperature(); //Temperatur vom Sensor DHT22 lesen
  // Prüfen ob eine gültige gleitkomma Zahl empfangen wurde.
  // Wenn NaN (not a number) zurückgegeben wird, soll eine Fehlermeldung ausgeben werden.
  if (isnan(humidyDHT22) || isnan(tempDHT22)) {
    Serial.println("DHT22 konnte nicht ausgelesen werden");
    // Go to deep sleep for 10min and then reset/retry
    ESP.deepSleep(600e6);
  } else {
    Temperatur = tempDHT22;
    Humidity = humidyDHT22;
    printValues("DHT22", humidyDHT22, tempDHT22);
  }
#endif

  sendToIFTTT();

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


