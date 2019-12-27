#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Update.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

///////////////////////////
////    Connections    ////
///////////////////////////
const char *host = "BrewController";
const char *ssid = "******";
const char *password = "********";

/////////////////////////
////    GPIO Pins    ////
/////////////////////////
#define BOIL_PRIMARY 16
#define BOIL_SECONDARY 17
#define MASH_PRIMARY 18
#define PUMP 4

//////////////////////////////////////////
////    Relay High/Low Definitions    ////
//////////////////////////////////////////
#define RELAY_ON HIGH
#define RELAY_OFF LOW

/////////////////////////
////    Variables    ////
/////////////////////////
int dualTemp = 4;           // the degrees below mash temp when both elements are on
int targetTemp = 152;
float currentTemp;
int mode = 0;  // 0 - off, 1 - mash, 2 - boil

//////////////////////////
////    Web Server    ////
//////////////////////////
AsyncWebServer server(80);
bool shouldReboot = false;


void setup()
{

  Serial.begin(115200);
  Serial.print("Booting...");
  Serial.println();

  // setup the GPIOs as input/output
  pinMode(BOIL_PRIMARY, OUTPUT);
  pinMode(BOIL_SECONDARY, OUTPUT);
  pinMode(MASH_PRIMARY, OUTPUT);
  pinMode(PUMP, OUTPUT);

  setup_wifi();

  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    server.on("/", handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/firmware", HTTP_GET, handleFirmware);
    server.onNotFound(handleNotFound);
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index){
      Serial.printf("Update Start: %s\n", filename.c_str());
      uint32_t maxSketchSpace = 0x1E0000;
      if(!Update.begin(maxSketchSpace)){
        Update.printError(Serial);
      }
      // Update.runAsync(true);
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    } });

    server.begin(); // Start the server
    Serial.println("");
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi Failed");
  }

  Serial.println("Ready");
}

void loop()
{

  if (shouldReboot)
  {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  // ****************GET TEMPERATURE READING HERE***********************

  switch (mode) {
    case 0:
      break:
    case 1:
      control_mash();
      break;
    case 2:
      control_boil();
      break;  
  }
}

void setup_wifi()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(host);
  WiFi.begin(ssid, password);
}

control_mash(){
    
    if ((targetTemp - currentTemp > dualTemp)) {
        digitalWrite(BOIL_SECONDARY, RELAY_ON);         // turn secondary element on if temp is too low
    } else {
        digitalWrite(BOIL_SECONDARY, RELAY_OFF);        // turn secondary element off if temp is near mash temp
    }

}

control_boil(){
    digitalWrite(BOIL_PRIMARY, RELAY_ON);       // turn primary ON
    digitalWrite(BOIL_SECONDARY, RELAY_ON);     // turn secondary ON
    digitalWrite(PUMP, RELAY_OFF);              // turn pump OFF
}
